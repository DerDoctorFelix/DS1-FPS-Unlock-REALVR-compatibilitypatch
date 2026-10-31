#include "skipintro.h"

#include "log.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>

// SkipIntro (Remastered). The title screen is the step machine TitleStep_forWin32: [step+0x10] is the current
// step and [step+0x20] points at its handler table (0x1C8B080, filled by a static initializer at 0x371B6),
// which Step<>::Update (0x808F30) calls each frame. On PC the start-up runs:
//   1 InitMenu (0x801BC0)            -> 3
//   3 Logo (0x803330)                once the menus are ready (0x681EA0), opens dialog 60 (the logo screen,
//                                    0x71ABF0(60, 1)) -> 4
//   4 LogoWait (0x803370)            while dialog 60 is open, waits -> 5
//   5 AdvertiseMovie (0x8033A0)      -> 6 (nothing to show on PC)
//   6 AdvertiseMovieWait (0x8033B0)  -> 7
//   7 InitMenuData (0x8033C0) ...    the title screen
// The Logo entry of the handler table is replaced by skip_logo_step: the same readiness wait, then on to step 5
// without opening the logo screen. Only that data entry changes, no game code: the copy protection rejects code
// writes in the first seconds after start, which is exactly when the logos run.

namespace {

constexpr uint32_t kTimestamp = 0x6344CA56;
constexpr uint32_t kImageSize = 0x0319B000;
constexpr uint32_t kHandlerTable = 0x1C8B080;
constexpr uint32_t kLogoStep = 3;
constexpr uint32_t kLogoHandler = 0x803330;
constexpr uint32_t kLogoWaitHandler = 0x803370;
constexpr uint32_t kAdvertiseHandler = 0x8033A0;
constexpr uint32_t kMenusReady = 0x681EA0;  // bool (): no blocking dialog open, ready for the logos
constexpr int32_t kAfterLogos = 5;          // STEP_NO_AdvertiseMovie

using ReadyFn = bool (*)();
uint8_t* g_image = nullptr;
ReadyFn g_ready = nullptr;
volatile LONG g_skipped = 0;

void skip_logo_step(uint8_t* step) {
    if (!g_ready()) {
        return;  // as the original step: wait for the menus
    }
    *reinterpret_cast<volatile int32_t*>(step + 0x10) = kAfterLogos;
    if (InterlockedExchange(&g_skipped, 1) == 0) {
        LOG_INFO("SkipIntro: start-up logos skipped");
    }
}

DWORD WINAPI install_thread(void*) {
    auto** table = reinterpret_cast<void**>(g_image + kHandlerTable);
    void* const logo = g_image + kLogoHandler;
    // The table is filled by the game's static initializers, which run right after this DLL is loaded.
    for (int waited = 0; waited < 20000; waited += 2) {
        if (table[kLogoStep] == logo && table[kLogoStep + 1] == g_image + kLogoWaitHandler &&
            table[kLogoStep + 2] == g_image + kAdvertiseHandler) {
            DWORD old = 0;
            if (!VirtualProtect(&table[kLogoStep], sizeof(void*), PAGE_READWRITE, &old)) {
                LOG_ERROR("SkipIntro: could not write the title step table (Win32=%lu)", GetLastError());
                return 0;
            }
            table[kLogoStep] = reinterpret_cast<void*>(&skip_logo_step);
            DWORD ignored = 0;
            VirtualProtect(&table[kLogoStep], sizeof(void*), old, &ignored);
            LOG_INFO("SkipIntro: title step %u (logos) replaced after %d ms", kLogoStep, waited);
            return 0;
        }
        if (table[kLogoStep] && table[kLogoStep] != logo) {
            LOG_ERROR("SkipIntro: title step %u is %p, expected %p; the logos are shown", kLogoStep, table[kLogoStep],
                      logo);
            return 0;
        }
        Sleep(2);
    }
    LOG_ERROR("SkipIntro: the title step table was not filled within 20 s; the logos are shown");
    return 0;
}

}  // namespace

bool skipintro_install(bool skip) {
    if (!skip) {
        LOG_INFO("SkipIntro: false (the start-up logos are shown)");
        return true;
    }
    g_image = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(g_image);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(g_image + dos->e_lfanew);
    if (nt->FileHeader.TimeDateStamp != kTimestamp || nt->OptionalHeader.SizeOfImage != kImageSize) {
        LOG_ERROR("SkipIntro: not the supported 2022 Steam build; the logos are shown");
        return false;
    }
    g_ready = reinterpret_cast<ReadyFn>(g_image + kMenusReady);
    HANDLE thread = CreateThread(nullptr, 0, install_thread, nullptr, 0, nullptr);
    if (!thread) {
        LOG_ERROR("SkipIntro: could not start (Win32=%lu)", GetLastError());
        return false;
    }
    CloseHandle(thread);
    return true;
}
