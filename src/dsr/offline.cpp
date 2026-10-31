#include "offline.h"

#include "log.h"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#include <atomic>
#include <cstdio>
#include <cstring>

// BlockOnline. The game reaches FromSoftware's servers with its own Winsock calls: it resolves the server
// name (getaddrinfo / gethostbyname) and connects to it (connect, send / sendto). Matchmaking, summons and
// invasions all start from that login, and Steam's peer-to-peer traffic only follows it. The game's own
// import table entries for those functions are pointed at the wrappers below:
//   * name lookups fail with "host not found", so the game cannot even find the server;
//   * connect / sendto to anything but this computer (127.0.0.0/8, ::1) fail with "network unreachable".
// The game then behaves as when the servers are down: it plays offline. Only the game executable's own
// imports are changed (found by name, so this does not depend on the build); Steam and other DLLs are not
// touched.

namespace {

using GetAddrInfoFn = int(WSAAPI*)(PCSTR, PCSTR, const ADDRINFOA*, PADDRINFOA*);
using GetHostByNameFn = hostent*(WSAAPI*)(const char*);
using ConnectFn = int(WSAAPI*)(SOCKET, const sockaddr*, int);
using SendToFn = int(WSAAPI*)(SOCKET, const char*, int, int, const sockaddr*, int);

GetAddrInfoFn g_getaddrinfo = nullptr;
GetHostByNameFn g_gethostbyname = nullptr;
ConnectFn g_connect = nullptr;
SendToFn g_sendto = nullptr;
std::atomic<int> g_block_lines{0};
std::atomic<uint64_t> g_blocked{0};

bool is_local(const sockaddr* addr, int len) {
    if (!addr) {
        return true;  // nothing to send to; let Winsock report the error
    }
    if (addr->sa_family == AF_INET && len >= static_cast<int>(sizeof(sockaddr_in))) {
        const auto* in = reinterpret_cast<const sockaddr_in*>(addr);
        const unsigned char first = reinterpret_cast<const unsigned char*>(&in->sin_addr)[0];
        return first == 127;
    }
    if (addr->sa_family == AF_INET6 && len >= static_cast<int>(sizeof(sockaddr_in6))) {
        const auto* in6 = reinterpret_cast<const sockaddr_in6*>(addr);
        static const unsigned char loopback[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
        return std::memcmp(&in6->sin6_addr, loopback, 16) == 0;
    }
    return false;
}

void note_block(const char* what, const char* detail) {
    g_blocked.fetch_add(1, std::memory_order_relaxed);
    if (g_block_lines.fetch_add(1) < 20) {
        LOG_INFO("BlockOnline: blocked %s%s%s", what, detail ? " " : "", detail ? detail : "");
    }
}

int WSAAPI hook_getaddrinfo(PCSTR node, PCSTR service, const ADDRINFOA* hints, PADDRINFOA* result) {
    if (node && (std::strcmp(node, "localhost") == 0 || std::strncmp(node, "127.", 4) == 0)) {
        return g_getaddrinfo(node, service, hints, result);
    }
    note_block("server lookup (getaddrinfo)", node);
    if (result) {
        *result = nullptr;
    }
    WSASetLastError(WSAHOST_NOT_FOUND);
    return WSAHOST_NOT_FOUND;
}

hostent* WSAAPI hook_gethostbyname(const char* name) {
    if (name && (std::strcmp(name, "localhost") == 0 || std::strncmp(name, "127.", 4) == 0)) {
        return g_gethostbyname(name);
    }
    note_block("server lookup (gethostbyname)", name);
    WSASetLastError(WSAHOST_NOT_FOUND);
    return nullptr;
}

int WSAAPI hook_connect(SOCKET s, const sockaddr* addr, int len) {
    if (is_local(addr, len)) {
        return g_connect(s, addr, len);
    }
    note_block("connection (connect)", nullptr);
    WSASetLastError(WSAENETUNREACH);
    return SOCKET_ERROR;
}

int WSAAPI hook_sendto(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int tolen) {
    if (!to || is_local(to, tolen)) {
        return g_sendto(s, buf, len, flags, to, tolen);
    }
    note_block("packet (sendto)", nullptr);
    WSASetLastError(WSAENETUNREACH);
    return SOCKET_ERROR;
}

struct Wrap {
    const char* name;
    void** original;
    void* hook;
};

bool patch_slot(void** slot, void* value) {
    DWORD old = 0;
    if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &old)) {
        return false;
    }
    *slot = value;
    DWORD ignored = 0;
    VirtualProtect(slot, sizeof(void*), old, &ignored);
    return true;
}


// The 2022 Steam build's import address table slots for these four functions (FirstThunk of WS2_32 at
// 0x2017D5C). The game calls them through these slots (call [rip+...] at 0x99503E, 0x9950DC, ...). The copy
// protection garbles most import names in memory after start-up, so the slots are taken from this table when
// the executable is that build, and found by name otherwise.
struct SlotRva {
    const char* name;
    uint32_t rva;
};
constexpr SlotRva kSteamSlots[] = {
    {"getaddrinfo", 0x2017DBC},
    {"gethostbyname", 0x2017DF4},
    {"connect", 0x2017E94},
    {"sendto", 0x2017D7C},
};
constexpr uint32_t kSteamTimestamp = 0x6344CA56;
constexpr uint32_t kSteamImageSize = 0x0319B000;

void** g_slots[4] = {};

const Wrap kWraps[4] = {
    {"getaddrinfo", reinterpret_cast<void**>(&g_getaddrinfo), reinterpret_cast<void*>(hook_getaddrinfo)},
    {"gethostbyname", reinterpret_cast<void**>(&g_gethostbyname), reinterpret_cast<void*>(hook_gethostbyname)},
    {"connect", reinterpret_cast<void**>(&g_connect), reinterpret_cast<void*>(hook_connect)},
    {"sendto", reinterpret_cast<void**>(&g_sendto), reinterpret_cast<void*>(hook_sendto)},
};

const char* describe_target(void* p, char* buf, size_t size) {
    HMODULE m = nullptr;
    if (p && GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                static_cast<LPCWSTR>(p), &m) && m) {
        char path[MAX_PATH] = {};
        GetModuleFileNameA(m, path, MAX_PATH);
        const char* slash = std::strrchr(path, '\\');
        std::snprintf(buf, size, "%s+0x%llX", slash ? slash + 1 : path,
                      static_cast<unsigned long long>(static_cast<uint8_t*>(p) - reinterpret_cast<uint8_t*>(m)));
    } else {
        std::snprintf(buf, size, "%p (no module)", p);
    }
    return buf;
}

// Installs one wrapper in its slot. The slot's current value becomes the function the wrapper calls.
bool install_slot(int k, bool log) {
    void** slot = g_slots[k];
    void* current = *slot;
    if (current == kWraps[k].hook) {
        return true;
    }
    if (!current) {
        return false;
    }
    *kWraps[k].original = current;
    if (!patch_slot(slot, kWraps[k].hook)) {
        LOG_ERROR("BlockOnline: could not redirect %s", kWraps[k].name);
        return false;
    }
    if (log) {
        char where[MAX_PATH + 32];
        LOG_INFO("BlockOnline: %s redirected (slot %p held %s)", kWraps[k].name, slot,
                 describe_target(current, where, sizeof(where)));
    }
    return true;
}

// Keeps the slots redirected: anything that writes them again (the copy protection resolving imports late, or
// another mod) is logged and the wrapper is put back. Every 100 ms for the first five minutes, then every 2 s.
DWORD WINAPI guard_thread(void*) {
    const ULONGLONG start = GetTickCount64();
    int lines = 0;
    for (;;) {
        for (int k = 0; k < 4; ++k) {
            if (g_slots[k] && *g_slots[k] != kWraps[k].hook) {
                char where[MAX_PATH + 32];
                if (lines++ < 20) {
                    LOG_ERROR("BlockOnline: the %s slot was rewritten (now %s); redirecting it again", kWraps[k].name,
                              describe_target(*g_slots[k], where, sizeof(where)));
                }
                install_slot(k, false);
            }
        }
        Sleep(GetTickCount64() - start < 300000 ? 100 : 2000);
    }
    return 0;
}

}  // namespace

bool offline_install() {
    auto* image = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image);
    if (!image || dos->e_magic != IMAGE_DOS_SIGNATURE) {
        LOG_ERROR("BlockOnline: could not read the game image");
        return false;
    }
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(image + dos->e_lfanew);
    const bool steam_build = nt->FileHeader.TimeDateStamp == kSteamTimestamp &&
                             nt->OptionalHeader.SizeOfImage == kSteamImageSize;
    if (steam_build) {
        for (int k = 0; k < 4; ++k) {
            g_slots[k] = reinterpret_cast<void**>(image + kSteamSlots[k].rva);
        }
    } else {
        // Another build: find the slots by import name (works when the names are still readable).
        const IMAGE_DATA_DIRECTORY& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        for (auto* desc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(image + dir.VirtualAddress);
             dir.VirtualAddress && desc->Name; ++desc) {
            if (_stricmp(reinterpret_cast<const char*>(image + desc->Name), "WS2_32.dll") != 0 ||
                !desc->OriginalFirstThunk) {
                continue;
            }
            auto* names = reinterpret_cast<IMAGE_THUNK_DATA64*>(image + desc->OriginalFirstThunk);
            auto* slots = reinterpret_cast<void**>(image + desc->FirstThunk);
            for (size_t i = 0; names[i].u1.AddressOfData; ++i) {
                if (IMAGE_SNAP_BY_ORDINAL64(names[i].u1.Ordinal)) {
                    continue;
                }
                const auto* by_name = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(image + names[i].u1.AddressOfData);
                for (int k = 0; k < 4; ++k) {
                    if (std::strcmp(reinterpret_cast<const char*>(by_name->Name), kWraps[k].name) == 0) {
                        g_slots[k] = &slots[i];
                    }
                }
            }
        }
    }
    int done = 0;
    for (int k = 0; k < 4; ++k) {
        if (!g_slots[k]) {
            LOG_ERROR("BlockOnline: the %s import was not found", kWraps[k].name);
            continue;
        }
        if (install_slot(k, true)) {
            ++done;
        }
    }
    if (done == 0) {
        LOG_ERROR("BlockOnline: nothing could be redirected; the game is NOT blocked from going online");
        return false;
    }
    HANDLE thread = CreateThread(nullptr, 0, guard_thread, nullptr, 0, nullptr);
    if (thread) {
        CloseHandle(thread);
    }
    LOG_INFO("BlockOnline: online play is blocked (%d of 4 Winsock imports redirected%s: server lookups fail, only "
             "connections to this computer are allowed)",
             done, steam_build ? ", Steam build table" : ", found by name");
    return done == 4;
}

uint64_t offline_blocked_count() {
    return g_blocked.load(std::memory_order_relaxed);
}
