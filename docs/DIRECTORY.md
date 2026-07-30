<!-- Deliverable Zero: a complete recursive inventory of this repository.
     Authored by source reading and reproducible shell commands only. This project has no
     installation of the original game, so nothing here is play-tested and nothing here
     should be read as a behavioural claim. Compile-verified is the honest ceiling. -->

# DIRECTORY — the whole repository, file by file

Repo: `C:/Users/Pierce Lonergan/Documents/GitHub/DeepCoreOverhaul`
Branch `main`, HEAD `eae7f75` at time of writing.

**286 files** outside `.git/`, `bin/` and `__pycache__/`: 124 `.h`, 110 `.cpp`, 8 `.hpp`,
11 `.md` (12 with this file), 3 `.vcxproj`, 3 `.py`, 2 `.rc`, 2 `.cfg`, plus icons, a
solution file, a CI workflow, a manifest, a `.lib`, a `.def`, a `.bat` and one silent
`.avi`. Verify with:

```bash
find . -type f -not -path "./.git/*" -not -path "./bin/*" -not -path "*__pycache__*" \
  | sed 's/.*\.//' | sort | uniq -c | sort -rn
```

---

## 1. Orientation — what this is and what comes out of it

DeepCoreOverhaul is a fan fork of **OpenLRR** by trigger-segfault (`README.md:3-6`,
`NOTICE.md:3-19`). OpenLRR is an incremental re-implementation of a 1999 PC game: it does
not replace the original executable, it **moves into it**. Upstream carries no license, so
this fork claims no rights over the inherited code — read `NOTICE.md` before doing anything
with it.

The **"L" in "LRR" is never expanded**, anywhere, in prose, code or filenames
(`README.md:35`, `NOTICE.md:44-49`). This is inherited from upstream and honoured here.

### The architecture, in one paragraph

The project builds a DLL that is loaded into the original 1999 executable's process. On
load it rewrites that executable's own machine code in memory: `hook_write_jmpret`
(`src/openlrr/hook.cpp:36`) overwrites the first **6 bytes** of a target function's
prologue with `E9 rel32` + `C3`, a relative jump into the DLL's C++ body followed by a
return. Every hook in the program is installed in one shot by `interop_hook_all`
(`src/openlrr/interop.cpp:4454`), called from `LaunchOpenLRR`
(`src/openlrr/OpenLRR.cpp:1095`) before the engine's main loop starts. Functions that have
**not** been re-implemented are not stubbed out — they are called through as raw address
constants, e.g. `#define Lego_LoadRockMonsterTypes ((bool32 (__cdecl* )(void))0x0042d030)`
(`src/openlrr/game/Game.h:1474`). Global state is not owned either: dozens of structs are
C++ **references overlaid onto the exe's data segment at fixed addresses**, e.g.
`LegoRR::Stats_Globs & LegoRR::statsGlobs = *(LegoRR::Stats_Globs*)0x00503bd8;`
(`src/openlrr/game/object/Stats.cpp:41`), pinned by `assert_sizeof(Stats_Globs, 0x5b0)`
(`src/openlrr/game/object/Stats.h:232`). The 1999 machine code still reads those bytes at
those offsets, so growing such a struct does not move its neighbour — it **overwrites** it.
Rendering is Direct3D Retained Mode, whose headers and import library are vendored in
`lib/d3drm/` because modern SDKs no longer ship them.

The hook is **destructive, not a trampoline**. The original prologue is gone and the
restore path is commented out (`src/openlrr/hook.cpp:30-32,47-49`); nothing in the tree ever
passes a backup buffer. **There is no way to call the original implementation.** A
re-implementation must be complete, never a wrapper. Full detail in
`docs/HOOK-ARCHITECTURE.md`.

### The three build outputs

All land in `bin/` (`src/openlrr/openlrr.vcxproj:75`, `:84`; the two tool projects set the
same `OutDir`). Debug builds carry a `-d` suffix (`TargetName`, `openlrr.vcxproj:80,89`;
`openlrr-injector.vcxproj:78,84`; `openlrr-makeexe.vcxproj:78,84`).

| Output | Project | What it is |
| --- | --- | --- |
| `bin/openlrr-d.dll` (rel. `openlrr.dll`) | `src/openlrr/openlrr.vcxproj` — `ConfigurationType DynamicLibrary` (`:30`) | **The injected game code.** Everything in `src/openlrr/`. This is the deliverable; the CI artifact contains only this plus `data/` and `NOTICE.md` (`.github/workflows/build_artifacts.yml`). |
| `bin/OpenLRR-d.exe` (rel. `OpenLRR.exe`) | `src/openlrr-injector/openlrr-injector.vcxproj` — `Application` (`:31`) | **The injector.** Launches the game suspended, parks its entrypoint on `EB FE`, `CreateRemoteThread(LoadLibrary)` the DLL in, waits for the WinMain hook to land, restores the entrypoint. Needs no patched executable. |
| `bin/OpenLRR-MakeExe-d.exe` (rel. `OpenLRR-MakeExe.exe`) | `src/openlrr-makeexe/openlrr-makeexe.vcxproj` — `Application` (`:31`) | **The PE patcher.** Statically rewrites *your own* `LegoRR.exe` into a launcher: inserts an `.idata2` import section for `openlrr.dll!StartOpenLRR` and overwrites `WinMain` with a call to it (`src/openlrr-makeexe/README.md`). |

> **Naming collision, worth stating plainly.** `bin/OpenLRR.exe` and `bin/OpenLRR-d.exe`
> are the **injector**. Two *files with the same names* used to be tracked at the repo root
> and were **the original 1999 game executable** with an import appended; they were purged
> and are now gitignored (`.gitignore:75-79`, `NOTICE.md:66-93`). If you see a
> 737,280-byte `OpenLRR.exe`, that is game code, not this project's.

### Build contract

`Debug|x86` and `Release|x86`, toolset **v142** (`openlrr.vcxproj:32`), **0 errors and
exactly 44 warnings**. Do not land anything that adds a warning. `x64` configurations exist
in `openlrr.sln` but are not the contract and cannot work — the whole design is pinned to
32-bit absolute addresses.

**We cannot run the game.** Compile-verified is the ceiling. Nothing in this repository has
been play-tested; never write or imply otherwise.

---

## 2. Directory tree — every directory, one line each

```
DeepCoreOverhaul/
├── .github/                     GitHub metadata: CI and issue templates
│   ├── ISSUE_TEMPLATE/          Bug-report form
│   └── workflows/               CI: address-space lint, no-game-binaries gate, x86 Debug+Release builds
├── bin/                         BUILD OUTPUT ONLY — gitignored, never committed (see §7)
│   ├── Data/                    Copied from data/Data by the vcxproj CustomBuildStep
│   ├── Settings/                Copied from data/Settings by the same step
│   └── tmp/                     MSVC intermediate objects, one subdir per project
├── data/                        Files shipped alongside the DLL at runtime
│   ├── Data/OpenLRR/            Runtime assets owned by this project (currently one silent AVI)
│   └── Settings/                User-editable config read from the exe directory
├── docs/                        Project documentation and generated address evidence
├── lib/                         Vendored third-party binaries and headers
│   └── d3drm/                   Direct3D Retained Mode import lib + .def + regeneration notes
│       └── inc/                 Microsoft's 1995-1997 D3DRM headers, no longer in modern SDKs
├── resources/                   Win32 resource scripts, icons, manifest
│   └── logo/                    Application icons, .ico + .png, several colourways
├── scripts/                     Developer-machine helper scripts
├── src/                         All source
│   ├── openlrr/                 THE DLL. Root files = entry, hooking, interop registry
│   │   ├── cmdline/             Command-line parsing and the generated command-line spec
│   │   ├── engine/              Gods98 engine re-implementation (see engine/README.md)
│   │   │   ├── audio/           Music, 3D sound playback, wave loading
│   │   │   ├── core/            Config, errors, files, maths, memory, RNG, WAD archives
│   │   │   ├── drawing/         2D drawing: images, fonts, BMP, FLIC, DirectDraw, text windows
│   │   │   ├── gfx/             3D: containers, meshes, LightWave scene/object loaders, viewports
│   │   │   ├── input/           Keyboard/mouse state and key-name lookup
│   │   │   ├── util/            Engine-only utilities: RNC decompression, DX error decoding, registry
│   │   │   └── video/           AVI animation and movie playback
│   │   ├── game/                Game logic re-implementation (see game/README.md)
│   │   │   ├── audio/           SFX name lookup, sample groups, speech
│   │   │   ├── effects/         3D world effects and particles
│   │   │   ├── front/           Screens and menus not tied to gameplay
│   │   │   ├── interface/       Icon menus, panels, HUD, shared 2D UI drawing
│   │   │   │   └── hud/         Heads-up elements drawn over the world
│   │   │   ├── mission/         Level goals, quotas, NERPs scripting, PTL files
│   │   │   ├── object/          In-game entity AI, stats, logic, models
│   │   │   └── world/           Map grid, camera, block-tied objects, world rendering
│   │   └── platform/            Win32/DirectX header shims and D3DRM IID bindings
│   ├── openlrr-injector/        The injector executable
│   └── openlrr-makeexe/         The PE patcher executable
│       └── pe/                  Minimal PE reader/writer used by the patcher
└── tools/                       Repo tooling not part of any build output
    └── addrlint/                Address-space linter + self-tests; generates docs/ADDRESS-MAP.md
```

---

## 3. Ownership — where you are allowed to change behaviour at all

This is the most useful section in the document. It answers one question: *if I edit this
file, can anything happen?*

### 3.1 Method, reproducible

The convention is exact and mechanical. A **not-yet-implemented** function is a live macro;
when it gets implemented the macro is **commented out** and the real declaration directly
beneath it is uncommented. Compare `Game.h:1474-1475` (live macro, declaration commented)
against `Game.h:1426-1427` (macro commented, declaration live):

```cpp
// <LegoRR.exe @0042d030>
#define Lego_LoadRockMonsterTypes ((bool32 (__cdecl* )(void))0x0042d030)   // still the exe's
//bool32 __cdecl Lego_LoadRockMonsterTypes(void);

// <LegoRR.exe @0042c260>
//#define Level_HandleEmergeTriggers ((bool32 (__cdecl* )(Lego_Level*, const Point2I*, OUT Point2I*))0x0042c260)
bool32 __cdecl Level_HandleEmergeTriggers(Lego_Level*, const Point2I*, OUT Point2I*);  // OURS
```

**Run this from the repo root (Git Bash):**

```bash
LIVE='^[[:space:]]*#define[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]+\(\('
IMPL='^[[:space:]]*//[[:space:]]*#define[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]+\(\('

# per-directory
for d in $(find src/openlrr -type d | sort); do
  files=$(find "$d" -maxdepth 1 -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' \))
  [ -z "$files" ] && continue
  a=$(grep -Eh "$LIVE" $files 2>/dev/null | wc -l)
  b=$(grep -Eh "$IMPL" $files 2>/dev/null | wc -l)
  printf "%-40s live=%-5d impl=%-5d\n" "$d" $a $b
done

# per-file
for f in $(find src/openlrr \( -name '*.h' -o -name '*.hpp' \) | sort); do
  printf "%-52s %3d %3d\n" "$f" \
    "$(grep -cE "$LIVE" "$f")" "$(grep -cE "$IMPL" "$f")"
done

# grand totals
grep -rEh "$LIVE" --include=*.h --include=*.hpp --include=*.cpp src/openlrr | wc -l  # 876
grep -rEh "$IMPL" --include=*.h --include=*.hpp --include=*.cpp src/openlrr | wc -l  # 921
```

**Totals: 876 live macros still pointing at the exe, 921 implemented — 51.3 % of the
tracked surface.** (Running the same greps over all of `src/` yields 880, because
`src/openlrr-makeexe/openlrr-makeexe.cpp:24,28,31,32` contain four unrelated
`((uint16_t)…)` casts that match the pattern. Scope the grep to `src/openlrr` for the real
figure.) These numbers agree with `docs/HOOK-ARCHITECTURE.md` §2, which is the authority if
they ever diverge.

**Two traps in reading these counts:**

1. **A `0 / 0` header is ambiguous.** In `engine/` — and in a dozen `game/` modules — a
   finished function had its macro **deleted outright** rather than commented. So `0 live /
   0 impl` means *either* "fully done" *or* "this header never had exe functions." Resolve
   it with the address-tag count, which survives either way:
   `grep -c '<LegoRR.exe @' <header>`. A header with tags > 0 and live = 0 is **fully
   implemented**. A header with 0 tags is **project-original**.
2. **"impl" is not the same as "has a C++ body."** `Panels.cpp` carries 53
   `// <LegoRR.exe @…>` tags but only **two** real bodies (`Panel_RadarMap_ZoomIn/Out`,
   `Panels.cpp:27,35`); the remaining 51 are pre-staged commented-out declaration stubs.
   Cross-check with a body count:
   `grep -cE '^[A-Za-z_].*(__cdecl|__stdcall)[[:space:]]+(LegoRR|Gods98)::' <file>`
   (a lower bound — custom functions like `Panels.cpp:27` omit `__cdecl`).

### 3.2 Per-directory result

| Directory | live (exe) | impl (ours) | % implemented |
| --- | ---: | ---: | ---: |
| `src/openlrr/` root, `cmdline/`, `platform/` | 0 | 0 | project-original / fully ours |
| `engine/` — **all 7 subdirs + root** | **0** | **0** | **effectively 100 % (macros deleted)** |
| `game/` root (`Game.h`) | 141 | 83 | 37.1 % |
| `game/audio/` | 0 | 0 | 100 % (27 tags, all done) |
| `game/effects/` | 0 | 57 | 100 % |
| `game/front/` | 48 | 69 | 59.0 % |
| `game/interface/` | 216 | 44 | 16.9 % |
| `game/interface/hud/` | 5 | 19 | 79.2 % |
| `game/mission/` | 33 | 323 | 90.7 % |
| `game/object/` | 344 | 234 | 40.5 % |
| `game/world/` | 89 | 92 | 50.8 % |
| **TOTAL (`src/openlrr`)** | **876** | **921** | **51.3 %** |

**The engine is done.** Verified: `grep -rn '0x00[0-9a-f]\{6\}' --include=*.h --include=*.hpp
src/openlrr/engine` returns exactly **three** hits, all commented-out
(`engine/core/Memory.h:114,122,130`). The only live raw-address call-throughs left anywhere
in `engine/` are `engine/Main.cpp:52` (a `Lego_Gods_Go` thunk) and
`engine/util/Compress.cpp:113,195` (RNC decompression). **100 % of the remaining frontier is
in `game/`.**

---

## 4. Per-directory file tables

Legend for the OWNERSHIP column (`game/` and `engine/` only):

- **OURS** — implemented C++. Editing the body changes behaviour, provided the hook is
  installed in `interop.cpp`.
- **MIXED (n live / m impl)** — partly implemented. Only the implemented half is editable.
- **EXE (n live / m impl)** — mostly or entirely raw address macros. **Editing the `.cpp`
  changes nothing** until the function is implemented *and* hooked.
- **PROJECT** — no exe counterpart at all: types, enums, or code this fork authored.

`.h` and `.cpp` of one module share a row; every filename appears.

### 4.1 Repository root

| File | Purpose |
| --- | --- |
| `README.md` | Front door: fork status, the three opt-in features, the roster ceiling, the naming rule, the build contract. |
| `NOTICE.md` | Attribution, provenance, the unlicensed status inherited from Gods98, the no-game-assets rule, and the record of the two purged executables. |
| `openlrr.sln` | VS2019 solution: 3 projects (`openlrr`, `openlrr-injector`, `openlrr-makeexe`) × {Debug,Release} × {x86,x64}. Only `x86` is real. |
| `openlrr-vscode.code-workspace` | VS Code workspace: include paths, defines, IntelliSense settings for editing outside Visual Studio. |
| `.editorconfig` | Formatting rules; also listed as a Solution Item (`openlrr.sln:11-13`). |
| `.gitattributes` | Line-ending / diff attributes. |
| `.gitignore` | Ignores `/bin`, `/bin-*`, `*.pdb`, `_wip/`, `_unused/`, and — explicitly — `/OpenLRR.exe` and `/OpenLRR-d.exe` with a comment explaining those are game code (`:75-79`). Whitelists `lib/d3drm/*.lib` and `*.def` back in. |

### 4.2 `.github/`

| File | Purpose |
| --- | --- |
| `workflows/build_artifacts.yml` | Four jobs. `Address_space_lint` runs `test_addrlint.py`, then `addrlint.py --check`, then diffs a freshly generated `ADDRESS-MAP.md` against the committed one and fails if stale. `No_game_binaries` rejects any tracked `.exe`/`.dll`/`.wad`. `Release_build` and `Debug_build` (gated on both) run `msbuild /t:openlrr /p:Platform=x86` and upload DLL + `data/` + `NOTICE.md`. |
| `ISSUE_TEMPLATE/bug_report.md` | Bug-report form. |

### 4.3 `docs/`

| File | Purpose |
| --- | --- |
| `ADDRESS-MAP.md` | **Generated — do not edit by hand.** 113 sized exe-overlaid regions, 1 unsized, **0 overlaps**, sorted by address, with a "tightest neighbours" slack table. Regenerate: `python tools/addrlint/addrlint.py --markdown docs/ADDRESS-MAP.md`. CI fails if stale. |
| `address-map.json` | Same data, machine-readable: `{regions, unsized, overlaps, gaps}`. Each region carries `name`, `type`, `addr`, `where` (file:line), `size`, `size_src`. Use this as evidence in any layout argument. |
| `HOOK-ARCHITECTURE.md` | Measured account of how a re-implemented function replaces the exe's: the five hook techniques, the two install phases, the three launch paths, merged-function hazards, and the quantified decompilation frontier. |
| `OVERHAUL-PLAN.md` | The engineering plan and the reckoning behind it — why the 15-ID-per-category roster ceiling cannot be raised, with addresses and arithmetic; where the real headroom is (weapons, per-(type,ID) stats). |
| `WORKLOG.md` | Append-only log; newest at the bottom. Every entry states what changed, what was verified, and what remains unproven. |
| `DIRECTORY.md` | This file. |

### 4.4 `data/` — shipped alongside the DLL

Copied into `bin/` on every build by a `CustomBuildStep` (`openlrr.vcxproj:117-119`,
`:147-149`) and into the CI artifact by `build_artifacts.yml`.

| File | Purpose |
| --- | --- |
| `Settings/DeepCore.cfg` | **This fork's settings file.** Lego.cfg syntax (`;` comments, `//` is *not* a comment). Read as `Lego*::DeepCore::<key>`. Every gate defaults FALSE = vanilla. Documents `VerboseStartup`, `MultiSpeciesEmerge` + `EmergeSpeciesPool`, `WaveDirector`/`WaveIntervalSeconds`/`WaveMaxAlive`, `CreatureVariants` + a `Variants { }` table, and `SurviveWaterOverflow`. |
| `Settings/Shortcuts.cfg` | Upstream's key-binding definitions, `Lego*::KeyBinds` block. Supports key IDs, keycodes, mouse buttons, `OFF`/`NULL`/`ON`. |
| `Data/OpenLRR/EmptyAudio.avi` | A silent AVI, supplied so the movie path has something valid to open when no audio track exists. The only binary asset this project ships, and it is not game content. |

### 4.5 `lib/`

| File | Purpose |
| --- | --- |
| `d3drm/d3drm.lib` | Import library for Direct3D Retained Mode, linked by every configuration (`openlrr.vcxproj:110`, `:140`). |
| `d3drm/d3drm.def` | Module-definition file the `.lib` was generated from. |
| `d3drm/undecorate.py` | Helper for the name-decoration fix-up described in the README. |
| `d3drm/README.md` | How to regenerate `d3drm.lib` — `lib /def:… /machine:x86`, then a hex edit of the name-type flag byte from `08` to `0C` to get undecorated names. |
| `d3drm/inc/d3drm.h`, `d3drmdef.h`, `d3drmobj.h`, `d3drmwin.h` | Microsoft's 1995-1997 D3DRM headers. Vendored because modern DirectX SDKs no longer ship retained mode (`NOTICE.md`, third-party table). |

### 4.6 `resources/`

| File | Purpose |
| --- | --- |
| `OpenLRR.rc` | Resource script for the DLL (UTF-16LE). |
| `OpenLRR-Injector.rc` | Resource script for the injector executable (UTF-16LE). |
| `resource.h` | Generated resource IDs for `OpenLRR.rc`, plus a preprocessor switch for including the visual-styles manifest. |
| `resource-injector.h` | Generated resource IDs for the injector (`IDI_OPENLRR 105`, …). |
| `manifest-visualstyles.xml` | Requests `asInvoker` privileges and Common-Controls 6.0 so dialogs get modern theming. |
| `logo/icon.ico` `.png`, `icon-gold.*`, `icon-teal.*`, `icon-teal-rr.*`, `icon-old.*` | Application icon in several colourways. `OpenLRR-MakeExe` can substitute one of these into a generated launcher (`src/openlrr-makeexe/README.md`, step 6). |

### 4.7 `scripts/`

| File | Purpose |
| --- | --- |
| `reset_fth.bat` | Resets the Windows Fault Tolerant Heap (`Rundll32 fthsvc.dll,FthSysprepSpecialize`). Needed when repeated crashes make Windows silently apply a heap shim that masks the bug you are chasing. Requires admin. |

### 4.8 `tools/addrlint/`

| File | Purpose |
| --- | --- |
| `addrlint.py` | The address-space linter. Reconstructs the exe data-segment layout from source alone: parses every `= *(T*)0x00……` overlay binding, joins it to its `assert_sizeof` size, sorts by address, checks for overlap. `--check` exits non-zero on overlap; `--markdown` / `--json` emit `docs/ADDRESS-MAP.md` / `address-map.json`. Its own docstring calls it *"the only automated safety net this project has"*. |
| `test_addrlint.py` | Self-tests: the linter must find real overlaps, must not invent false ones, must ignore commented-out declarations, and must refuse to guess unknown sizes. Run by CI before the linter itself. |

*(`tools/addrlint/__pycache__/` is transient Python bytecode. It is not in `.gitignore` as
of this writing — see §8.)*

### 4.9 `src/openlrr/` — DLL root (13 files)

| Files | Purpose | Ownership |
| --- | --- | --- |
| `dllmain.cpp` / `dllmain.h` | `DllMain`; on `DLL_PROCESS_ATTACH` calls `InjectOpenLRR` (`dllmain.cpp:43`). 59 lines. | PROJECT |
| `OpenLRR.cpp` / `OpenLRR.h` | The real entry point. `InjectOpenLRR` (`:59`) byte-sniffs the exe to decide which of three launch paths is in play (`Injector` / `DllStart` / `DllImport`, `:67-79`); `LaunchOpenLRR` (`:1066`) initialises the engine and calls `interop_hook_all()` at **`:1095`**. 1,141 lines. | PROJECT |
| `interop.cpp` / `interop.h` | **The hook registry.** 4,582 lines; `interop_hook_all` at `:4454`. 1,206 active `hook_write_jmpret`, 309 commented out, 6 `hook_write_call`, 2 `hook_write_addr`. Also holds the merged-function warnings and the hand-assembled Panel splice (`:4401-4448`). `interop.h:6-7` defines `PROCESS_EIP 0x0048f2c0` and `PROCESS_WINMAIN 0x00477a60`. | PROJECT |
| `hook.cpp` / `hook.h` | The four byte-pattern writers over one `VirtualProtect`/`memcpy`/`VirtualProtect` primitive (`hook.cpp:76-103`): `hook_write_jmp` (unused), `hook_write_jmpret` (the workhorse), `hook_write_call`, `hook_write_addr`. 141 lines. | PROJECT |
| `legacy.cpp` / `legacy.h` | Access to the exe's own C-runtime functions, so re-implemented code can free memory the original allocated. 35 lines. | PROJECT |
| `common.h` | Project-wide includes, fixed-width typedefs, and **`assert_sizeof`** — the `static_assert(sizeof(T)==n)` that pins every overlaid struct (`:207`). 272 lines. | PROJECT |
| `cpp.hint` | IntelliSense-only hint file so Visual Studio stops mis-parsing macros such as `DEFINE_ENUM_FLAG_OPERATORS`. No effect on the build. | PROJECT |
| `openlrr.vcxproj` | The DLL project: `DynamicLibrary`, v142, `OutDir bin\`, links `lib\d3drm\d3drm.lib`, 106 `ClCompile` + 122 `ClInclude` entries, post-build copy of a launcher **if present** (`:113`, `:143` — absence is expected and no longer fails the build), and a `CustomBuildStep` that copies `data/` into `bin/`. | PROJECT |

### 4.10 `src/openlrr/cmdline/`

| Files | Purpose | Ownership |
| --- | --- | --- |
| `CommandLine.cpp` / `CommandLine.hpp` | Command-line parsing for the DLL. 271 lines. | PROJECT |
| `CLGen.cpp` / `CLGen.h` | Generated command-line specification / option table. 345 lines. | PROJECT |

### 4.11 `src/openlrr/platform/`

Header shims that let 1999-era DirectX interfaces compile against a modern SDK, plus the
D3DRM interface IIDs bound to their addresses in the exe.

| Files | Purpose | Ownership |
| --- | --- | --- |
| `windows.h` / `windows.cpp` | Central Win32 include, version pinning, and DirectX version selection. | PROJECT |
| `targetver.h` | `_WIN32_WINNT` / SDK version targeting. | PROJECT |
| `d3drm.h` / `d3drm.cpp` | Direct3D Retained Mode glue. Binds 9 `Idl::IID_IDirect3DRM*` GUIDs to fixed exe addresses (`d3drm.cpp:16,19,22,25,28,31,34,…`) so exe and DLL agree on interface identity. | Binds exe data — see §6 |
| `ddraw.h` | DirectDraw compatibility shim. | PROJECT |
| `dinput.h` | DirectInput shim (version selection noted in `windows.h`). | PROJECT |
| `dsound.h` | DirectSound shim. | PROJECT |
| `timeapi.h` | `winmm` timer API shim. | PROJECT |
| `vfw.h` | Video for Windows (AVI) shim. | PROJECT |

### 4.12 `src/openlrr/engine/` — Gods98 re-implementation

**Ownership for this entire subtree: OURS.** 0 live address macros; the only residual exe
call-throughs are `Main.cpp:52` and `Compress.cpp:113,195`, listed below. Purposes follow
`src/openlrr/engine/README.md`. `.cpp` line counts given for scale.

#### `engine/` root (10 files)

| Files | Purpose | Ownership |
| --- | --- | --- |
| `Main.cpp` / `Main.h` | Main engine functionality and the main loop. 2,614 lines. **Contains one live raw-address call-through at `Main.cpp:52`** (`Lego_Gods_Go` thunk). Binds `Main_Globs @0x00506800`. | OURS (1 call-through) |
| `Graphics.cpp` / `Graphics.h` | Main rendering functionality, split off from `Main`. 363 lines. | OURS |
| `Init.cpp` / `Init.h` | Mode-selection dialog and display-settings selection. 425 lines. Binds `Init_Globs @0x005590a0`. | OURS |
| `colour.h` | Colour types and structures (`ColourRGBI`, …). 8 `assert_sizeof`. | PROJECT (types) |
| `geometry.h` | Maths types, structures and enums (`Direction`, `Point2I`, …). 28 `assert_sizeof` — the most of any header. | PROJECT (types) |
| `undefined.h` | `undefined`/`undefined1/2/4/8` typedefs used for unknown fields carried over from Ghidra. | PROJECT (types) |
| `README.md` | Upstream's engine overview. **Load-bearing**: `:3` is the sentence `NOTICE.md` cites for why the project cannot carry a license. | — |

#### `engine/audio/` (4 files)

| Files | Purpose | Ownership |
| --- | --- | --- |
| `Sound.cpp` / `Sound.h` | Wave loading and sound playback. 842 lines. Binds `Sound_Globs @0x00545868`. | OURS |
| `3DSound.cpp` / `3DSound.h` | Positional 3D sound. 1,621 lines. Binds `Sound3D_Globs @0x005075b8`. 7 `assert_sizeof`. | OURS |

#### `engine/core/` (17 files)

| Files | Purpose | Ownership |
| --- | --- | --- |
| `Config.cpp` / `Config.h` | The Lego.cfg / `.ol` / `.ptl` / `.ae` config parser — used by `DeepCore.cpp` too. 870 lines. Binds `Config_Globs @0x00507098`. | OURS |
| `Errors.cpp` / `Errors.h` | Error reporting: `Error_Warn`, `Error_Fatal`, `Error_DebugF2`. 324 lines. Binds `Error_Globs @0x00576ce0`. | OURS |
| `Files.cpp` / `Files.h` | File I/O, search paths, `FILE_FLAG_EXEDIR` / `FILE_FLAG_NOCD`. 1,459 lines. Binds `File_Globs @0x005349a0` and `FileCheck_Globs @0x005779e0`. | OURS |
| `Maths.cpp` / `Maths.h` | Fixed/float maths helpers, trig tables. 879 lines. | OURS |
| `Memory.cpp` / `Memory.h` | `Mem_Alloc`/`Mem_Free` wrappers. 102 lines. Binds `Mem_Globs @0x00545a20`. `Memory.h:114,122,130` hold the engine's only commented-out address macros. | OURS |
| `Wad.cpp` / `Wad.h` | WAD archive reading. 486 lines. Binds `Wad_Globs @0x005764f4`. | OURS |
| `Utils.cpp` / `Utils.h` | Assorted string/misc helpers. 233 lines. | OURS |
| `Random.cpp` / `Random.hpp` | RNG. | OURS |
| `ListSet.hpp` | Template list/set container used across the engine. | PROJECT |

#### `engine/drawing/` (14 files)

| Files | Purpose | Ownership |
| --- | --- | --- |
| `DirectDraw.cpp` / `DirectDraw.h` | DirectDraw surface and mode management. 1,175 lines. Binds `DirectDraw_Globs @0x0076bc80`. | OURS |
| `Draw.cpp` / `Draw.h` | Primitive 2D drawing. 913 lines. Binds `Draw_Globs @0x005417e8`. | OURS |
| `Images.cpp` / `Images.h` | Image loading, blitting, colour keys. 1,030 lines. Binds `Image_Globs @0x00534908`. | OURS |
| `Flic.cpp` / `Flic.h` | FLIC animation decoding. 1,439 lines. 8 `assert_sizeof`. | OURS |
| `Fonts.cpp` / `Fonts.h` | Bitmap font loading and text measurement. 412 lines. Binds `Font_Globs @0x00507528`. | OURS |
| `Bmp.cpp` / `Bmp.h` | BMP decoding. 228 lines. | OURS |
| `TextWindow.cpp` / `TextWindow.h` | Scrolling text windows. 515 lines. | OURS |

#### `engine/gfx/` (16 files)

| Files | Purpose | Ownership |
| --- | --- | --- |
| `Containers.cpp` / `Containers.h` | D3DRM frame/container hierarchy — the largest engine file at 3,902 lines, 20 `assert_sizeof`. Binds `Container_Globs @0x0076bd80`. | OURS |
| `Mesh.cpp` / `Mesh.h` | Mesh creation, groups, textures. 2,473 lines. Binds `Mesh_Globs @0x005353c0`. | OURS |
| `Lws.cpp` / `Lws.h` | LightWave **scene** loading (animated hierarchies). 974 lines. Binds `Lws_Globs @0x00541838`. | OURS |
| `Lwt.cpp` / `Lwt.h` | LightWave **object** loading (geometry). 782 lines. | OURS |
| `Viewports.cpp` / `Viewports.h` | D3DRM viewports and camera projection. 428 lines. Binds `Viewport_Globs @0x0076bce0`. | OURS |
| `AnimClone.cpp` / `AnimClone.h` | Cloned animated meshes. 352 lines. Note `AnimClone_IsLws` is a **merged function** and must be hooked per call site (`interop.cpp:305-321`). | OURS |
| `Materials.cpp` / `Materials.h` | Material creation and caching. 71 lines. | OURS |

#### `engine/input/` (8 files)

| Files | Purpose | Ownership |
| --- | --- | --- |
| `Input.cpp` / `Input.h` | Keyboard and mouse state. 264 lines. Binds `Input_Globs @0x0076ba00`. | OURS |
| `InputButton.cpp` / `InputButton.hpp` | Button/edge-detection abstraction. 470 lines. | PROJECT |
| `Keys.cpp` / `Keys.h` | Key-name ↔ code lookup (the table `Shortcuts.cfg` names). 160 lines. Binds `Key_Globs @0x005775e0`. | OURS |
| `MouseButtons.cpp` / `MouseButtons.h` | Mouse button enumeration and helpers. | OURS |

#### `engine/util/` (6 files)

| Files | Purpose | Ownership |
| --- | --- | --- |
| `Compress.cpp` / `Compress.h` | RNC decompression. 474 lines. **Two live raw-address call-throughs at `Compress.cpp:113,195`** — the decompressor itself is still the exe's. Binds `RNC_Globs @0x00558d68`. | OURS (2 call-throughs) |
| `Dxbug.cpp` / `Dxbug.h` | DirectX HRESULT decoding for diagnostics. 316 lines. Binds `Dxbug_Globs @0x005498a8`. | OURS |
| `Registry.cpp` / `Registry.h` | Windows registry reads for install paths/settings. 146 lines. | OURS |

#### `engine/video/` (6 files)

| Files | Purpose | Ownership |
| --- | --- | --- |
| `Animation.cpp` / `Animation.h` / `Animation.hpp` | AVI animation playback. 647 lines. Binds `Animation_Globs @0x00534998`. | OURS |
| `Movie.cpp` / `Movie.h` / `Movie.hpp` | Full-motion movie playback (the consumer of `data/Data/OpenLRR/EmptyAudio.avi`). 348 lines. | OURS |

### 4.13 `src/openlrr/game/` — where the frontier is

#### `game/` root (11 files)

| Files | Purpose | Ownership |
| --- | --- | --- |
| `Game.h` | The `Lego` module header: `Lego_Globs`, `Lego_Level`, all five type-loaders. **229 exe tags, 12 `assert_sizeof`.** | **MIXED (141 live / 83 impl — 37 %)** |
| `Game.cpp` | Main game functionality. 4,260 lines, ~70 bodies. Binds **three** globs: `LegoUpdate_Globs @0x004a4558`, `GameControl_Globs @0x004df410`, `Lego_Globs @0x005570c0`. Holds `Level_HandleEmergeTriggers` (`:3040`), the multi-species emerge site, and the **DLL-side storage pattern** at `:168-170` (PowerGrid `std::vector`s). | MIXED |
| `GameState.cpp` | Game/level state machine; calls the five type-loaders at `:513`. 2,595 lines. No `GameState.h` — it uses `Game.h`. | MIXED |
| `GameCommon.h` | Shared defines, structs and enums not yet assigned to a module. **34 `assert_sizeof`, the most in `game/`.** Home of `LegoObject_ID_Count = 15` (`:1143`), the RockMonster IDs (`:137-150`), vehicle/building IDs, `Interface_Menu_*` values, tool types (`:1000-1017`). | PROJECT (types) — but see §6 |
| `DeepCore.hpp` / `DeepCore.cpp` | **This fork's settings layer.** 210 + 425 lines, namespace `DeepCore`. Loads `Settings\DeepCore.cfg` via `Config_Load2` with `FILE_FLAG_EXEDIR\|FILE_FLAG_NOCD`, modelled on `Shortcuts.cpp`. Header comment states the one rule: nothing here may change the layout of any `assert_sizeof` struct. Every gate defaults to vanilla. | PROJECT (ours, shipped) |
| `Shortcuts.hpp` / `Shortcuts.cpp` | Key-binding system reading `Settings\Shortcuts.cfg`. 480 + 618 lines. The sanctioned in-tree pattern for OpenLRR-owned config, and the model `DeepCore` copied. | PROJECT |
| `Debug.h` / `Debug.cpp` | Debug overlays and developer commands. 612 lines. | PROJECT |
| `README.md` | Upstream's game-code overview, one line per subdirectory. | — |

#### `game/audio/` (2 files)

| Files | Purpose | Ownership |
| --- | --- | --- |
| `SFX.h` / `SFX.cpp` | SFX name lookup, `Samples` config block, random sample groups, speech management. 569 lines, 26 bodies, 27 exe tags. Binds `SFX_Globs @0x00502468`. | **OURS (0 live)** |

#### `game/effects/` (8 files)

| Files | Purpose | Ownership |
| --- | --- | --- |
| `Effects.h` / `Effects.cpp` | 3D world effects. 633 lines, 27 bodies, 6 `assert_sizeof`. Binds `Effect_Globs @0x004c8180`. | **OURS (0 / 27)** |
| `LightEffects.h` / `LightEffects.cpp` | Dynamic light effects. 659 lines, 21 bodies. Binds `LightEffects_Globs @0x004ebdd8`. | **OURS (0 / 21)** |
| `Smoke.h` / `Smoke.cpp` | Smoke particles. 547 lines, 12 bodies, 13 exe tags, 4 `assert_sizeof`. Binds `Smoke_Globs @0x00553100`. | **OURS (macros deleted)** |
| `DamageText.h` / `DamageText.cpp` | Floating damage numbers. 307 lines, 9 bodies. Binds `DamageText_Globs @0x004b9a58`. | **OURS (0 / 9)** |

#### `game/front/` (9 files)

| Files | Purpose | Ownership |
| --- | --- | --- |
| `FrontEnd.h` / `FrontEnd.cpp` | Menus, level select, save/load screens. **5,055 lines, 148 bodies, 30 `assert_sizeof`, 168 exe tags** — the most-implemented single module. Binds `Front_Globs @0x00557fc0` **plus 17 individual scalar globals** (`:39-103`), more than any other file. | **OURS (0 / 63)** |
| `Reward.h` / `Reward.cpp` | End-of-mission reward screen. 507 lines, only 4 bodies against 53 exe tags. 8 `assert_sizeof`. Binds `Reward_Globs @0x00553980`. | **EXE (48 live / 4 impl — 8 %)** |
| `RewardScroll.h` | **Empty placeholder** — one line, `#pragma once`. No `.cpp`. | PROJECT (stub) |
| `Loader.h` / `Loader.cpp` | Loading screen. 254 lines, 5 bodies. Binds `Loader_Globs @0x00556e00`. | **OURS (macros deleted)** |
| `Credits.h` / `Credits.cpp` | Credits roll. 171 lines, 1 body, 1 exe tag. | **OURS (macros deleted)** |

#### `game/interface/` (24 files) — **the least-owned subtree, 16.9 %**

| Files | Purpose | Ownership |
| --- | --- | --- |
| `Interface.h` / `Interface.cpp` | Icon menus and interface state. 504 lines, 3 bodies against 103 exe tags. `INTERFACE_MENU_MAXICONS = 11` (`Interface.h:30`) — the constant that caps the building menu. Binds `Interface_Globs @0x004ddd58` + 3 scalars. | **EXE (96 live / 3 impl — 3 %)** |
| `Panels.h` / `Panels.cpp` | Screen panels. 198 lines but only **2 real bodies** (`Panel_RadarMap_ZoomIn/Out`, `:27,35`) — the other 51 tags are commented stubs. 4 `assert_sizeof`. Binds `Panel_Globs @0x005010e0`. Behaviour here is reached by the hand-assembled splice at `interop.cpp:4401-4448`. | **EXE (52 live / 0 impl — 0 %)** |
| `InfoMessages.h` / `InfoMessages.cpp` | Info-message queue. 138 lines, 0 bodies. 4 `assert_sizeof`. Binds `Info_Globs @0x004dd658`. | **EXE (38 / 0 — 0 %)** |
| `HelpWindow.h` / `HelpWindow.cpp` | Help window. 81 lines, 0 bodies. Binds `HelpWindow_Globs @0x004dc8e8`. | **EXE (19 / 0 — 0 %)** |
| `ScrollInfo.h` / `ScrollInfo.cpp` | Scrolling info display. 60 lines, 0 bodies. | **EXE (11 / 0 — 0 %)** |
| `RadarMap.h` / `RadarMap.cpp` | Radar minimap. 976 lines, 15 bodies. Binds `RadarMap_Globs @0x004a9f28`. Note `RadarMap_Free` is a **merged function — do not hook** (`interop.cpp:3918`). | **OURS (0 / 14)** |
| `ToolTip.h` / `ToolTip.cpp` | Tooltips. 621 lines, 11 bodies. Binds `ToolTip_Globs @0x0054cf20` + 1 scalar. | **OURS (0 / 11)** |
| `Pointers.h` / `Pointers.cpp` | Mouse cursors. 267 lines, 9 bodies. Binds `Pointer_Globs @0x00501a98`. | **OURS (0 / 9)** |
| `Encyclopedia.h` / `Encyclopedia.cpp` | In-game encyclopedia. 339 lines, 7 bodies. Binds `Encyclopedia_Globs @0x004c8e88`. | **OURS (0 / 6)** |
| `TextMessages.h` / `TextMessages.cpp` | Localised text strings. 341 lines, 10 bodies. Binds **`Text_Globs @0x00504190`** — the neighbour that caps `Stats_Globs`. See §6. | **OURS (0 / 1)** |
| `Advisor.h` / `Advisor.cpp` | Advisor character. 405 lines, 17 bodies, 18 exe tags. Binds `Advisor_Globs @0x004b3db0`. | **OURS (macros deleted)** |
| `Priorities.h` / `Priorities.cpp` | Task priority panel. 398 lines, 14 bodies, 17 exe tags. Binds `Priorities_Globs @0x00501f00` + 2 scalars. | **OURS (macros deleted)** |

#### `game/interface/hud/` (4 files)

| Files | Purpose | Ownership |
| --- | --- | --- |
| `Bubbles.h` / `Bubbles.cpp` | Status bubbles over units. 762 lines, 18 bodies. Binds `Bubble_Globs @0x00558860` + 1 scalar. | **OURS (0 / 16)** |
| `ObjInfo.h` / `ObjInfo.cpp` | Selected-object info panel. 323 lines, 3 bodies. Binds `ObjInfo_Globs @0x00500e68`. | **MIXED (5 / 3)** |

#### `game/mission/` (13 files) — **the most-owned subtree, 90.7 %**

| Files | Purpose | Ownership |
| --- | --- | --- |
| `NERPsFunctions.h` / `NERPsFunctions.cpp` | Every NERPs script opcode. 2,470 lines, **294 bodies, 294 exe tags, 0 live macros** — the single most completely re-implemented module in the project. | **OURS (0 / 286)** |
| `NERPsFile.h` / `NERPsFile.cpp` | NERPs bytecode file parsing and the runtime interpreter. 1,287 lines, 15 bodies, 15 `assert_sizeof`. Binds **two** globs: `NERPsRuntime_Globs @0x00500958` and `NERPsFile_Globs @0x00556d40`, plus 7 scalars. | **MIXED (33 / 9 — 21 %)** |
| `NERPsRuntime.h` / `NERPsRuntime.cpp` | **Both empty** — header is one `#pragma once`, `.cpp` is 0 bytes. The runtime actually lives in `NERPsFile.cpp`. Kept for structural symmetry. | PROJECT (stub) |
| `Objective.h` / `Objective.cpp` | Mission objectives and completion. 1,152 lines, 15 bodies. Binds `Objective_Globs @0x00500bc0`. | **OURS (0 / 15)** |
| `Messages.h` / `Messages.cpp` | Message/event dispatch. 972 lines, 26 bodies, 3 `assert_sizeof`. Binds `Message_Globs @0x004ebf20`. | **OURS (0 / 13)** |
| `PTL.h` / `PTL.cpp` | `.ptl` file handling. 69 lines, 2 bodies. Binds `PTL_Globs @0x00556be0`. | **OURS (macros deleted)** |
| `Quota.h` | **Empty placeholder** — one line, `#pragma once`. No `.cpp`. | PROJECT (stub) |

#### `game/object/` (28 files) — **344 live macros, the largest hole**

| Files | Purpose | Ownership |
| --- | --- | --- |
| `Object.h` / `Object.cpp` | `LegoObject` — the central entity type and its lifecycle. **6,170 lines, 90 bodies, 289 exe tags, 7 `assert_sizeof`.** Binds `LegoObject_Globs @0x004df790` (`Object.cpp:56`; `assert_sizeof 0xc644`, `Object.h:491`) + 2 scalars. Holds the `LegoObject_RockMonster` branch that implements per-instance creature variants. | **MIXED (196 live / 90 impl — 31 %). Biggest single frontier.** |
| `AITask.h` / `AITask.cpp` | Creature and unit AI tasks. 539 lines, 11 bodies against 106 exe tags. Binds `AITask_Globs @0x004b41c8`. | **EXE (96 / 9 — 8 %). AI is essentially untouched.** |
| `Stats.h` / `Stats.cpp` | Per-(type, ID, level) object statistics parsed from Lego.cfg. 1,348 lines, 63 bodies, **0 live macros, 68 exe tags**. Binds `Stats_Globs @0x00503bd8` + four `c_ObjectStats_*` constants (`:47,50,53,56`). **This is the canonical layout hazard — see §6.** | **OURS (macros deleted)** |
| `Weapons.h` / `Weapons.cpp` | Weapon types and damage. 1,411 lines, 48 bodies, 6 `assert_sizeof`. Binds `Weapon_Globs @0x00504870`. Weapon **count is unbounded** — allocated from the cfg array size, and the interop hook is deliberately disabled (`interop.cpp:4345`) because the only caller is C++. | **OURS (0 / 48)** |
| `Vehicle.h` / `Vehicle.cpp` | Vehicle behaviour. 983 lines, 31 bodies. | **OURS (0 / 29)** |
| `Creature.h` / `Creature.cpp` | Creature behaviour. 415 lines, 28 bodies, 25 exe tags. | **OURS (0 / 24)** |
| `Building.h` / `Building.cpp` | Building behaviour. 672 lines, 27 bodies. | **OURS (0 / 24)** |
| `MeshLOD.h` / `MeshLOD.cpp` | Level-of-detail mesh switching. 194 lines, 6 bodies. | **OURS (0 / 6)** |
| `Collision.h` / `Collision.cpp` | Object collision. 92 lines, 4 bodies. | **OURS (0 / 4)** |
| `BezierCurve.h` / `BezierCurve.cpp` | Bezier path maths. 225 lines, 6 bodies, 6 exe tags. | **OURS (macros deleted)** |
| `ObjectRecall.h` / `ObjectRecall.cpp` | Cross-level object persistence. 191 lines, 8 bodies, 4 `assert_sizeof`. Binds `ObjectRecall_Globs @0x00500e50`. | **OURS (macros deleted)** |
| `Flocks.h` / `Flocks.cpp` | Bat/creature flocking. 111 lines, 0 bodies. Binds `Flocks_Globs @0x00558850`. | **EXE (29 / 0 — 0 %)** |
| `Upgrade.h` / `Upgrade.cpp` | Unit upgrades. 49 lines, 0 bodies. 3 `assert_sizeof`. | **EXE (12 / 0 — 0 %)** |
| `Dependencies.h` / `Dependencies.cpp` | Build-dependency tree. 57 lines, 0 bodies. 4 `assert_sizeof`. Binds `Dependencies_Globs @0x004b9bc8`. | **EXE (11 / 0 — 0 %)** |

#### `game/world/` (24 files)

| Files | Purpose | Ownership |
| --- | --- | --- |
| `Map3D.h` / `Map3D.cpp` | The 3D block/terrain map. 192 lines, **0 bodies against 57 exe tags**. 3 `assert_sizeof`. | **EXE (56 / 0 — 0 %). Largest untouched world module.** |
| `Construction.h` / `Construction.cpp` | Building placement and construction sites. 1,108 lines, 33 bodies. Binds `Construction_Globs @0x004b9a20`. | **OURS (0 / 32)** |
| `ElectricFence.h` / `ElectricFence.cpp` | Electric fences and their arcs. 861 lines, 27 bodies, 3 `assert_sizeof`. Binds `ElectricFence_Globs @0x004c8df8`. | **OURS (0 / 27)** |
| `Water.h` / `Water.cpp` | Water pools and simulation. 734 lines, 13 bodies, 4 `assert_sizeof`. Binds `Water_Globs @0x0054a520`. **Holds the 6 gated `Error_Fatal` sites** that `SurviveWaterOverflow` turns into warn-and-skip. | **OURS (0 / 13)** |
| `Camera.h` / `Camera.cpp` | Game cameras (top / radar / first-person). 712 lines, 28 bodies, 29 exe tags. Binds `Camera_Globs @0x005570a0`. `Camera_ToggleFreeMovement` is hooked at the **mid-function** address `0x00435cc1` (`Camera.h:133`, `interop.cpp:2892`). | **OURS (macros deleted)** |
| `Erosion.h` / `Erosion.cpp` | Lava erosion spread. 340 lines, 9 bodies. Binds `Erosion_Globs @0x004c8eb0`. | **OURS (0 / 9)** |
| `Fallin.h` / `Fallin.cpp` | Cave-ins. 262 lines, 6 bodies. Binds `Fallin_Globs @0x004a2ee4` + 1 scalar. | **OURS (0 / 6)** |
| `SelectPlace.h` / `SelectPlace.cpp` | Block selection / placement cursor. 537 lines, 4 bodies. | **OURS (0 / 4)** |
| `Roof.h` / `Roof.cpp` | Cavern roof rendering. 283 lines, 9 bodies, 10 exe tags. Binds `Roof_Globs @0x00553180`. | **OURS (macros deleted)** |
| `Teleporter.h` / `Teleporter.cpp` | Teleport pads and unit arrival. 63 lines, 0 bodies. 3 `assert_sizeof`. Binds `Teleporter_Globs @0x004ab450` **and `g_Teleporter_BOOL_00504188` (`Teleporter.cpp:17`)** — see §6. | **EXE (12 / 0 — 0 %)** |
| `SpiderWeb.h` / `SpiderWeb.cpp` | Spider webs. 54 lines, 0 bodies. Binds `SpiderWeb_Globs @0x005530e8`. | **EXE (10 / 0 — 0 %)** |
| `Detail.h` / `Detail.cpp` | Surface detail decoration. 46 lines, 0 bodies. | **EXE (11 / 0 — 0 %)** |

### 4.14 `src/openlrr-injector/` (2 files)

| File | Purpose |
| --- | --- |
| `openlrr-injector.cpp` | The injector. `CreateProcess(CREATE_SUSPENDED)` → patch the entrypoint to `EB FE` (self-jump, `:262`) → `ResumeThread` → `Sleep(100)` → `CreateRemoteThread(LoadLibrary, "openlrr-d.dll")` → spin-wait on `PROCESS_WINMAIN` until the hook lands (`:326-335`) → restore the entrypoint. Duplicates `PROCESS_EIP` / `PROCESS_WINMAIN` at `:23-24`. |
| `openlrr-injector.vcxproj` | `Application`, v142, `OutDir bin\`, `TargetName OpenLRR-d` / `OpenLRR`. |

### 4.15 `src/openlrr-makeexe/` (4 files + `pe/` 6 files)

| File | Purpose |
| --- | --- |
| `openlrr-makeexe.cpp` | The PE patcher: `OpenLRR-MakeExe [-d] <LEGORREXE> [ICOFILE]`. Inserts an `.idata2` section importing `openlrr.dll!StartOpenLRR` and rewrites `WinMain` (`:400-448`). Duplicates `PROCESS_WINMAIN` at `:35`. |
| `common.h` | Shared typedefs for the tool. |
| `README.md` | Usage, the six-step patch procedure, and the exact PE header fields read and modified. Explains why `.idata2` goes before `.rsrc` rather than after. |
| `openlrr-makeexe.vcxproj` | `Application`, v142, `TargetName OpenLRR-MakeExe-d` / `OpenLRR-MakeExe`. |
| `pe/PEFile.cpp` / `PEFile.h` | PE image reading, section table manipulation, directory RVA fix-ups. |
| `pe/PESectionStream.cpp` / `PESectionStream.h` | Byte-stream writer over a PE section. |
| `pe/PECommon.h` | PE structure definitions and constants. |
| `pe/PEResources.h` | Resource-directory walking and rebasing (needed because inserting a section shifts `.rsrc`). |

### 4.16 `bin/` — build output, never committed

`bin/` is gitignored (`.gitignore:6`) and its contents are **not** part of the repository.
Described here only so nobody mistakes it for source. After a full build it contains:

- `openlrr.dll` / `openlrr-d.dll` (+ `.exp`, `.lib`, `.manifest`, `.pdb`) — the injected game code.
- `OpenLRR.exe` / `OpenLRR-d.exe` (+ `.pdb`) — **the injector** (see the naming warning in §1).
- `OpenLRR-MakeExe.exe` / `OpenLRR-MakeExe-d.exe` (+ `.pdb`) — the PE patcher.
- `Settings/DeepCore.cfg`, `Settings/Shortcuts.cfg`, `Data/OpenLRR/…` — copied from `data/`
  by the `CustomBuildStep` (`openlrr.vcxproj:117-119`, `:147-149`).
- `tmp/openlrr/`, `tmp/openlrr-injector/`, `tmp/openlrr-makeexe/` — MSVC intermediates
  (`IntDir`, `openlrr.vcxproj:76`).

CI enforces that nothing of this shape is ever tracked: the `No_game_binaries` job rejects
any tracked `.exe`, `.dll` or `.wad`.

---

## 5. Start here — twelve files, in reading order

For someone new. Read these in this order and you will understand the project.

| # | File | Why |
| ---: | --- | --- |
| 1 | `NOTICE.md` | Provenance, the unlicensed status, the no-game-assets rule, the naming rule. Read before touching anything. |
| 2 | `README.md` | What ships, what is impossible, the build contract. |
| 3 | `docs/OVERHAUL-PLAN.md` §1 | "The reckoning" — the arithmetic proving the 15-ID ceiling cannot be raised. This is the constraint every design decision bends around. |
| 4 | `src/openlrr/hook.cpp` (141 lines) | The whole hooking mechanism. Small, and everything else rests on it. |
| 5 | `docs/HOOK-ARCHITECTURE.md` | The measured account: five hook techniques, two install phases, three launch paths, merged-function hazards, the quantified frontier. |
| 6 | `src/openlrr/OpenLRR.cpp:59-120` and `:1066-1100` | `InjectOpenLRR` (which launch path am I on?) and `LaunchOpenLRR` (where hooks get installed). |
| 7 | `src/openlrr/interop.cpp:4454-4580` | `interop_hook_all`. Skim the ordering comment at `:4458-4460`; this is the manifest of what the project has actually taken over. |
| 8 | `src/openlrr/common.h:207` | `assert_sizeof`. One line, and it is the guardrail that stops the cardinal mistake. |
| 9 | `src/openlrr/game/object/Stats.h:225-232` (struct + `assert_sizeof`) + `Stats.cpp:41` | The canonical overlaid struct: definition, `assert_sizeof`, and the address binding, in that order. |
| 10 | `docs/ADDRESS-MAP.md` (tightest-neighbours table) | See how little slack actually exists. Then `tools/addrlint/addrlint.py`'s docstring for why it is generated rather than written. |
| 11 | `src/openlrr/game/Game.cpp:168-170` | The **DLL-side storage pattern** (PowerGrid `std::vector`s). This is how you add state without touching the exe's layout. Copy this, always. |
| 12 | `src/openlrr/game/DeepCore.hpp` + `data/Settings/DeepCore.cfg` | This fork's own surface: how a feature is gated, why every gate defaults to vanilla, and how the config reaches it. |

Optional 13th, if you intend to implement a function: `src/openlrr/game/Game.h:1422-1430` —
five consecutive lines showing an unimplemented function and an implemented one side by
side. That diff *is* the contribution workflow.

---

## 6. Do not touch without reading `docs/ADDRESS-MAP.md`

These files bind C++ references onto the original executable's data segment. **Growing any
type bound here does not move its neighbour — it overwrites it,** and the 1999 machine code
keeps reading the old offsets. `assert_sizeof` will stop the build; deleting the assert does
not unlock anything, it only removes the warning.

**Extra storage must live DLL-side.** The proven in-tree pattern is
`src/openlrr/game/Game.cpp:168-170`.

Regenerate the evidence with:

```bash
python tools/addrlint/addrlint.py --check                       # exits non-zero on overlap
python tools/addrlint/addrlint.py --markdown docs/ADDRESS-MAP.md
grep -rn '_Globs *& .*= \*(' --include=*.cpp src/openlrr        # the 68 glob bindings
grep -rc 'assert_sizeof(' --include=*.h --include=*.hpp src/openlrr | grep -v ':0$'
```

Current state: **113 sized regions, 1 unsized, 0 overlaps, 399 `assert_sizeof` declarations,
68 `*_Globs` bindings.**

### 6.1 The four highest-risk sites

| Site | Why |
| --- | --- |
| `game/object/Stats.cpp:41` + `Stats.h:232` | `statsGlobs @0x00503bd8`, `assert_sizeof(Stats_Globs, 0x5b0)` ⇒ ends at `0x00504188`. `textGlobs` starts at `0x00504190` (`interface/TextMessages.cpp:24`). Raw gap 8 bytes — but `g_Teleporter_BOOL_00504188` (`world/Teleporter.cpp:17`) occupies 4 of them. **Only 4 free bytes.** Raising `LegoObject_ID_Count` 15→16 (`GameCommon.h:1143`) grows `objectLevels[20][15]` by 80 bytes and drives `Stats_Globs` 72 bytes into `Text_Globs`, while shifting `toolStats` off its baked `0x500` offset. |
| `game/object/Object.cpp:56` + `Object.h:491` | `objectGlobs @0x004df790`, `assert_sizeof(LegoObject_Globs, 0xc644)` — the largest overlaid struct in the project. |
| `game/GameCommon.h` | Does not bind an address itself, but **defines the counts and enums that size the structs that do**: `LegoObject_ID_Count = 15` (`:1143`, commented "Hardcoded so many places in the game"), `LegoObject_Type_Count = 20`, `LegoObject_ToolType_Count = 11`. Changing a count here silently resizes overlaid structs across six files. 34 `assert_sizeof`. |
| `game/front/FrontEnd.cpp:39-103` | Binds `Front_Globs @0x00557fc0` **plus 17 individual scalars packed 4 bytes apart** — six consecutive `0 B`-slack neighbours in the `ADDRESS-MAP.md` table. Nothing here can grow by even one byte. |

### 6.2 Complete list of files that bind exe-overlaid globs

Every file below appears in `docs/address-map.json` under `regions[].where`.

**`game/` — 45 files.** `Game.cpp` (3 globs: `LegoUpdate_Globs`, `GameControl_Globs`,
`Lego_Globs`), `mission/NERPsFile.cpp` (2: `NERPsRuntime_Globs`, `NERPsFile_Globs`),
`object/Stats.cpp` (1 glob + 4 `c_ObjectStats_*` constants), `front/FrontEnd.cpp` (1 glob +
17 scalars), and one glob each in:
`audio/SFX.cpp`, `effects/DamageText.cpp`, `effects/Effects.cpp`,
`effects/LightEffects.cpp`, `effects/Smoke.cpp`, `front/Loader.cpp`, `front/Reward.cpp`,
`interface/Advisor.cpp`, `interface/Encyclopedia.cpp`, `interface/HelpWindow.cpp`,
`interface/InfoMessages.cpp`, `interface/Interface.cpp`, `interface/Panels.cpp`,
`interface/Pointers.cpp`, `interface/Priorities.cpp`, `interface/RadarMap.cpp`,
`interface/ScrollInfo.cpp`, `interface/TextMessages.cpp`, `interface/ToolTip.cpp`,
`interface/hud/Bubbles.cpp`, `interface/hud/ObjInfo.cpp`, `mission/Messages.cpp`,
`mission/Objective.cpp`, `mission/PTL.cpp`, `object/AITask.cpp`,
`object/Dependencies.cpp`, `object/Flocks.cpp`, `object/Object.cpp`,
`object/ObjectRecall.cpp`, `object/Weapons.cpp`, `world/Camera.cpp`,
`world/Construction.cpp`, `world/ElectricFence.cpp`, `world/Erosion.cpp`,
`world/Fallin.cpp`, `world/Roof.cpp`, `world/SpiderWeb.cpp`, `world/Teleporter.cpp`,
`world/Water.cpp`.

**`engine/` — 21 files.** `Init.cpp`, `Main.cpp`, `audio/3DSound.cpp`, `audio/Sound.cpp`,
`core/Config.cpp`, `core/Errors.cpp`, `core/Files.cpp` (2 globs), `core/Memory.cpp`,
`core/Wad.cpp`, `drawing/DirectDraw.cpp`, `drawing/Draw.cpp`, `drawing/Fonts.cpp`,
`drawing/Images.cpp`, `gfx/Containers.cpp`, `gfx/Lws.cpp`, `gfx/Mesh.cpp`,
`gfx/Viewports.cpp`, `input/Input.cpp`, `input/Keys.cpp`, `util/Compress.cpp`,
`util/Dxbug.cpp`, `video/Animation.cpp`.

**Elsewhere — 2 files.** `src/openlrr/platform/d3drm.cpp` (9 `Idl::IID_IDirect3DRM*` GUIDs
at fixed addresses) and `src/openlrr/legacy.cpp` (4 bindings into the exe's C runtime).

**The headers that pin those layouts** are the ones carrying `assert_sizeof`. The densest,
and therefore the ones to be most careful with: `game/GameCommon.h` (34),
`game/front/FrontEnd.h` (30), `engine/geometry.h` (28), `engine/gfx/Containers.h` (20),
`game/mission/NERPsFile.h` (15), `game/Game.h` (12), `engine/gfx/Mesh.h` (12).

---

## 7. The sibling artifact outside the repository — do not delete

```
C:/Users/Pierce Lonergan/Documents/GitHub/DeepCoreOverhaul-prepurge-backup.bundle
```

**2,696,343 bytes.** A git bundle taken *before* the history rewrite that purged
`OpenLRR.exe` and `OpenLRR-d.exe` from all 352 inherited commits (`docs/WORKLOG.md`,
2026-07-29 entry; `NOTICE.md:66-93`). It sits **outside** the repository, one level up in
`Documents/GitHub/`, precisely so it is not caught by any repo-scoped cleanup.

It is the only remaining copy of the pre-purge history. If a rewrite turns out to have
dropped a commit, or authorship attribution needs to be reconstructed, this bundle is the
sole recourse. **Do not delete it, and do not commit it** — it contains the two game
executables the purge removed.

---

## 8. Things I could not determine, and one loose end

Stated plainly rather than guessed at.

1. **Nothing here is play-tested.** Every purpose in the tables above is inferred from
   source, filenames, and the two module `README.md` files. No behaviour was observed. The
   `data/Data/OpenLRR/EmptyAudio.avi` purpose in particular ("supplied so the movie path has
   something valid to open") is inferred from its name and location, not verified against
   `engine/video/Movie.cpp`.
2. **`resources/OpenLRR.rc` and `OpenLRR-Injector.rc` are UTF-16LE** and were not read in
   full; their purpose is stated from filename and from `resource.h` / `resource-injector.h`.
   I did not enumerate which icon each build actually embeds.
3. **The "OURS (macros deleted)" classification is an inference,** not a proof. It means
   *zero live macros, zero commented macros, but `<LegoRR.exe @…>` tags present*. That is
   consistent with a finished module, and the function-body counts corroborate it in every
   case I checked (`SFX.cpp` 26 bodies / 27 tags, `Advisor.cpp` 17/18, `Camera.cpp` 28/29).
   It is **not** consistent for `Teleporter.cpp`, `SpiderWeb.cpp`, `Flocks.cpp`,
   `Upgrade.cpp`, `Dependencies.cpp`, `Detail.cpp`, `Map3D.cpp`, `Panels.cpp`,
   `HelpWindow.cpp`, `InfoMessages.cpp` and `ScrollInfo.cpp` — those have **0 bodies**, and
   they are classified **EXE** above on that basis, which is the safe reading.
4. **Function-body counts are lower bounds.** The regex requires `__cdecl`/`__stdcall`, so
   custom functions written without it (e.g. `Panels.cpp:27,35`) are not counted.
5. **`docs/WORKLOG.md` was being written concurrently with this document** (mtime moved
   twice during the survey). Its contents may have advanced past what is summarised here.
6. **Loose end: `tools/addrlint/__pycache__/` is untracked but not ignored.** `.gitignore`
   has no `__pycache__/` or `*.pyc` entry. It has not been committed, but nothing prevents
   it. This is the only inventory discrepancy I found; it is cosmetic, not a hazard.
7. **Not determined:** whether the `x64` solution configurations in `openlrr.sln` ever
   built. They cannot work — the entire design rests on 32-bit absolute addresses — but I
   did not attempt them and cannot say whether they fail at compile or at link.

---

## 9. DECISION

**This inventory is complete and the tree is coherent. Adopt the ownership map in §3–§4 as
the standing answer to "may I change this?", and treat §6 as a hard gate.**

Three concrete rules follow, and I recommend they be enforced rather than remembered:

1. **Before editing any `game/` file, check its ownership row.** If it reads **EXE**, the
   `.cpp` is inert — a change there compiles cleanly and does nothing at runtime, which is
   the most expensive kind of bug this project can produce because it survives the only
   verification available to us. Implement *and* hook, or do not touch. The eleven
   zero-body modules named in §8.3 are the trap list.
2. **Before editing any file in §6.2, run `python tools/addrlint/addrlint.py --check`
   afterwards.** CI already does this on push (`build_artifacts.yml`, `Address_space_lint`
   gates both build jobs), so the only thing this buys is finding out sooner — but on a
   layout mistake, sooner matters, because the failure mode is silent memory corruption in a
   game we cannot run.
3. **New state goes DLL-side.** `Game.cpp:168-170` is the pattern. There is no case in which
   growing an `assert_sizeof` struct is correct.

**Recommended follow-ups, in priority order:**

- **Add `__pycache__/` and `*.pyc` to `.gitignore`** (§8.6). One line, closes the only
  inventory discrepancy in the tree.
- **Link this document from `README.md`.** It is the map; it should not be findable only by
  knowing it exists.
- **Consider deleting or documenting the four stub files**: `front/RewardScroll.h`,
  `mission/Quota.h`, `mission/NERPsRuntime.h`, `mission/NERPsRuntime.cpp` (0 bytes). They
  are inherited scaffolding and currently read as "someone started this," which is
  misleading. Documenting them is the lower-risk option — they are referenced by the
  `.vcxproj` file lists, so deletion is a project-file edit, not just a file removal.
- **If the next work item is a behavioural feature, prefer `game/mission/` (90.7 % ours),
  `game/effects/` (100 %), or `game/object/` weapons and stats (both 100 %).** Avoid
  `game/interface/` (16.9 %) and `game/world/Map3D` (0 %) unless the goal is explicitly to
  push the decompilation frontier rather than to ship behaviour.
