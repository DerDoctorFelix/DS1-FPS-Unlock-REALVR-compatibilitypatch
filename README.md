# DS1 FPS Unlock: R.E.A.L. VR compatibility fork (Dark Souls: Remastered)

An unofficial fork of [zofrea1's DS1 FPS Unlock](https://github.com/zofrea1/DS1-FPS-Unlock-PTDE-Remastered) 1.9.0 that makes the Remastered unlocker work together with Luke Ross's **R.E.A.L. VR** mod.

R.E.A.L. VR unlocks the frame rate itself, but it does not have the unlocker's gameplay and physics fixes. With R.E.A.L. VR the game runs at about twice the headset's refresh rate (often 180 to 240 FPS), so those bugs are even worse in VR than in 2D. The best-known one: sprinting into a wall or a corner slows the character down for the rest of the sprint. Running the original unlocker next to R.E.A.L. VR does not work either: the game then runs in slow motion. This fork is the compatibility patch between the two. R.E.A.L. VR keeps control of the frame rate, and the unlocker's physics fixes follow the frame time the game really runs at.

> [!WARNING]
> **Offline play only.** Do not play online with this mod, R.E.A.L. VR or any other mod. FromSoftware can restrict ("soft ban") accounts that play online with a modified game. To be safe, this fork **blocks online play by default** (`BlockOnline = true`): the game cannot reach FromSoftware's servers and plays as if they were down. Before you ever play online again, remove every mod from the game folder.

This is not an official release of the original mod. Please report problems with this fork here, not to the original author.

## Contents

- [Requirements](#requirements)
- [Install](#install)
- [What this fork changes](#what-this-fork-changes)
- [Settings](#settings)
- [Checking that it works](#checking-that-it-works)
- [Using it in 2D](#using-it-in-2d)
- [Known limits](#known-limits)
- [How it works](#how-it-works)
- [Building](#building)
- [Credits and license](#credits-and-license)

## Requirements

- **Dark Souls: Remastered, current Steam build** (1.03.1, executable SHA-1 `9150CC63C617332ED3C2C66E7566ED67E3292DA0`). Like the original mod, this fork checks the executable and refuses to patch anything else.
- **R.E.A.L. VR** by Luke Ross, from his Patreon. It is not included here. Tested with R.E.A.L. VR 2607.31.1.
- Windows, 64-bit.

## Install

1. Install R.E.A.L. VR as usual. Its installer puts `dxgi.dll` in the game folder; leave it there.
2. Copy `dinput8.dll` and `DSR-FPS-Unlock.ini` from the release next to `DarkSoulsRemastered.exe`. If the original FPS unlocker is installed, replace its two files. Do not rename the DLL.
3. Start the game the way R.E.A.L. VR tells you to.

The INI is read from the folder the DLL is in, and the log `DSR-FPS-Unlock.log` is written there.

**Uninstall:** delete `dinput8.dll` and `DSR-FPS-Unlock.ini`. The mod changes the game only in memory while it runs. No game file or save is modified.

## What this fork changes

Everything else is the original 1.9.0. All of its physics, camera, animation, HUD and timing fixes are kept, and the original README is kept as [README-original.md](README-original.md).

| Change | Setting | What it does |
| --- | --- | --- |
| R.E.A.L. VR compatibility | `ExternalFrameUnlock` | Leaves the frame rate to R.E.A.L. VR and passes its time steps through, instead of scaling them a second time (that was the slow motion). The physics fixes follow the step the game really uses. |
| Fixes install next to R.E.A.L. VR | (always) | With R.E.A.L. VR loaded, almost no memory was left close to the game's code, and most of the original's fixes failed to install. Only a few got through, among them the sprint fix. They now all install. |
| Sprint fix watchdog | (always) | Checks every 2 seconds that the sprint (wall graze) fix is still in place and puts it back if anything undid it. |
| VR camera | `FixCamera = false` | The follow camera stays tight around the character's torso, as with R.E.A.L. VR alone. |
| No recenter on lock-on without a target | `NoLockOnRecenter` | Pressing lock-on (R3) with nothing to lock onto no longer swings the camera behind the character, which is jarring in VR. Locking onto enemies is unchanged. |
| Skip the start-up logos | `SkipIntro` | Goes straight to the title screen. |
| Block online play | `BlockOnline` | Keeps the game off FromSoftware's servers (see the warning above). |
| Camera diagnostics | `CameraProbe` | For development only, off by default. |

## Settings

All settings are in `DSR-FPS-Unlock.ini`, and the file explains each one. Restart the game after changing any of them.

### New settings

| Setting | Default in the INI | Description |
| --- | --- | --- |
| `BlockOnline` | `true` | The game cannot look up or connect to FromSoftware's servers, so it plays offline: no summons, invasions, messages or bloodstains from other players. Connections to your own PC and Steam itself are not affected. This works even with `FPSUnlock = false`. Only set it to `false` with every mod removed. |
| `ExternalFrameUnlock` | `true` | `true`: R.E.A.L. VR (or another mod) unlocks the frame rate. This mod leaves the game's frame pacer alone, does not cap the frame rate, and changes a time step only when it is still the game's fixed 1/60 s. `false`: the original stand-alone unlocker, for the 2D game. |
| `ReplaceRetailSteps` | `true` | With `ExternalFrameUnlock`: a step that is still the fixed 1/60 s (R.E.A.L. VR did not change it) is replaced by the measured frame time, so that part of the game does not run too fast. If the game ever runs in slow motion with R.E.A.L. VR, set this to `false`: then no step is changed at all, and only the physics fixes follow the frame time. |
| `SkipIntro` | `true` | Skips the start-up logos and goes straight on to the title screen. |
| `NoLockOnRecenter` | `true` | Lock-on (R3) with nothing to lock onto leaves the camera where it is, instead of turning it behind the character over 0.3 s. |
| `FixMouse` | `true` | Mouse look follows the frame time (the original mod always did this). If mouse look is far too fast or too slow in VR, set `false`. |
| `FixHavokStep` | `true` | The Havok cloth step follows the frame time (the original mod always did this). With `ExternalFrameUnlock` only a step that is still the fixed 1/60 s is scaled, so it cannot double up with R.E.A.L. VR. |
| `CameraProbe` | `false` | Diagnostic: logs what the lock-on button changes in the follow camera. Leave it off. |

### Changed defaults

| Setting | Original | VR preset | Why |
| --- | --- | --- | --- |
| `FixCamera` | `true` | `false` | With `true` the follow camera lags behind the character as in the 2D game, which feels wrong in VR. With `false`, lock-on target switches also snap faster, as with R.E.A.L. VR alone. |
| `MaxFPS` | `120` | `120` | Unchanged, but it does nothing in VR. R.E.A.L. VR sets the frame rate: with its alternate eye rendering, the game runs at twice the headset's refresh rate, or at 1/2 or 1/3 of that with AER v2's "1/2 rate" and "1/3 rate" modes. It only matters with `ExternalFrameUnlock = false`. |

**A setting missing from the INI** behaves as in the original mod: `ExternalFrameUnlock = false`, `FixCamera = true`, `NoLockOnRecenter = false`. The two exceptions are `BlockOnline` and `SkipIntro`, which default to `true`. So an INI from the original 1.9.0 gives the original behaviour, with online play blocked and the logos skipped.

## Checking that it works

Open `DSR-FPS-Unlock.log` next to the DLL after playing. Near the top you should see:

- `BlockOnline: online play is blocked (4 of 4 Winsock imports redirected, Steam build table ...)`
- `SkipIntro: start-up logos skipped`
- `... active in ExternalFrameUnlock mode ...`
- after loading a save, `Cave block 1 at ...` followed by one line for each fix (for example `Sprint graze check retargeted ...`), and **no** `Could not allocate ... (Win32=487)` errors.
- `Simulation step #120: ... (set by the other mod, passed through)`, which shows that R.E.A.L. VR is supplying the time step.
- once a minute, `Sprint graze check #...: patch in place`.

If you try to log in, `BlockOnline: blocked connection (connect)` shows that the block stopped the game reaching the server.

## Using it in 2D

The DLL also works without R.E.A.L. VR. For the 2D game, set:

```ini
ExternalFrameUnlock = false
FixCamera = true
```

Then it behaves like the original 1.9.0, with `MaxFPS` as the frame cap, plus `BlockOnline`, `SkipIntro` and `NoLockOnRecenter`. In the game, under System -> PC Settings -> Display, set Frequency to your monitor's refresh rate and Vertical sync to off, as the original mod asks. To be sure of the original behaviour in 2D, you can of course just use the original mod.

## Known limits

- Tested on one PC and one headset, with R.E.A.L. VR 2607.31.1. Later R.E.A.L. VR versions may behave differently. If the game runs in slow motion or too fast, send the log.
- Only the 2022 Steam build is supported. On any other build the mod refuses to patch, like the original.
- `BlockOnline` is tested on the Steam build. On any other build it falls back to finding the network functions by name, which the game's copy protection may prevent. Check the log line.
- `FixMouse` with R.E.A.L. VR has not been tested much, since VR is usually played with a controller.
- The Prepare to Die Edition part of the original repository is unchanged. This fork is about Remastered only.

## How it works

Addresses are offsets in `DarkSoulsRemastered.exe` 1.03.1 (image base `0x140000000`). See `src/dsr/patches.cpp`, `offline.cpp` and `skipintro.cpp`.

- **External frame unlock.** The original mod paces the game itself, and it hands every scheduler step (simulation, effects, menus, cinematics, Havok) the real frame time by scaling the game's fixed 1/60 s. R.E.A.L. VR already hands those steps the real frame time, so the original scaled them a second time, and the game ran at frame time x 60 of its speed (slow motion). In `ExternalFrameUnlock` mode the frame pacer hooks are not installed, the step hooks chain onto whatever is in their slots, and a step is scaled only if it is exactly the game's fixed 1/60 s. The Havok relay does the same check in its stub (`ucomiss`). The physics constants are recomputed every frame from the step the simulation really receives.
- **Memory for the fixes.** Each fix needs a small block of memory within 2 GB of the game's code. R.E.A.L. VR's allocations leave almost no free 64 KB blocks above the game image, so `alloc_near` failed with `ERROR_INVALID_ADDRESS` (487) for nearly every fix. The allocator now searches above and below the image, and splits each 64 KB block into 4 KB pages.
- **Sprint fix watchdog.** About every 2 seconds the simulation hook checks that the three retargeted operands of the wall graze check (`0x379B9B`, `0x379BB9`, `0x379BEC`) still point at the mod's values. If they don't, it logs the bytes it found and patches them again.
- **NoLockOnRecenter.** The player's lock-on handler (`0x40B0F0`) asks the camera manager for a lock-on when it finds a target (`0x40BCD8`), and otherwise for a recenter (`0x40BD1C`: `[mgr+0xA2] = 1`). The manager passes the recenter request on to the follow camera (`0x234BA7`, `[cam+0x262]`), which turns behind the character over 0.3 s (`0x237EB5`). The fork changes the 1 stored at `0x40BD1C` to 0 (after checking the bytes), so the recenter is never requested. No other user of the camera manager's recenter is affected.
- **SkipIntro.** The title screen is a step machine (`TitleStep_forWin32`). Its handler table at `0x1C8B080` is filled by a static initializer, and on PC step 3 (`STEP_NO_Logo`, `0x803330`) waits for the menus and then opens the logo screen (dialog 60). The fork replaces that one table entry, after checking entries 3 to 5, with a function that does the same wait and then goes on to step 5. No game code changes, which matters because the game's copy protection rejects code writes during the first seconds after start-up.
- **BlockOnline.** The game reaches FromSoftware's servers with its own Winsock calls. Its import table entries for `getaddrinfo`, `gethostbyname`, `connect` and `sendto` (`0x2017DBC`, `0x2017DF4`, `0x2017E94`, `0x2017D7C`) are pointed at wrappers: name lookups fail, and connections to anything but this PC (`127.0.0.0/8`, `::1`) fail with "network unreachable". The copy protection garbles the import names in memory after start-up, so the slots are taken from this build's table, not found by name. A guard thread puts the wrappers back if anything rewrites the slots. In testing, the login was stopped at `connect`: the game connects to an IP address directly, without a name lookup.
- **CameraProbe.** Snapshots the follow camera after each update, and logs every field that changes in the second after a lock-on press. This is how the recenter fields were found.

## Building

- **MSVC (official):** `build_dsr.bat`, from a Visual Studio 2022 developer prompt or with VS 2022 Community or Enterprise installed. The output is `build\dsr\dinput8.dll`. `package_release_vr.bat` then makes the release zip.
- **GitHub Actions:** `.github/workflows/build-remastered.yml` builds the DLL with MSVC on every push. Download it from the run's artifacts.
- **Linux (development only):** `build_dsr_mingw.sh` cross-builds with clang and mingw-w64. Releases should be built with MSVC.
- The Prepare to Die Edition build (`build_ptde.bat`) is unchanged.

## Credits and license

- **zofrea1:** DS1 FPS Unlock, the original mod, which does all the real work this fork builds on. MIT License.
- **Luke Ross:** R.E.A.L. VR (not included, available on his Patreon).
- **bladecoding:** the DS3 / Elden Ring / PTDE "Remove Intro Screens" mod showed the approach of patching the title screen's logo step.
- Fork by **DerDoctorFelix**.

Released under the MIT License, like the original. See [LICENSE](LICENSE). Dark Souls is a trademark of Bandai Namco Entertainment and FromSoftware. This project is not affiliated with them, or with Luke Ross.
