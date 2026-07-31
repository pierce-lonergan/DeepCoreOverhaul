<!-- Permanent engineering document. Every claim below is either (a) an asymptotic property
     provable by reading the cited source, (b) a count taken mechanically from the tree, or
     (c) an assembly-level result reproduced on this machine with the recipe given in §8.
     Nothing here is a measurement of the running game. This project has no installation of
     the original game; see §8 for what that permits us to claim and what it forbids. -->

# PERFORMANCE — the frame, the allocations, the layout, the build

> ## CORRECTION, 2026-07-30 — the build-flag headline in this document is WRONG
>
> This document's lead finding was that `Release|Win32` emits no `/O` switch and therefore
> "ships unoptimised". **That is refuted.** Measured directly on this tree:
>
> ```
> msbuild src/openlrr/openlrr.vcxproj -t:ClCompile -p:Configuration=Release -p:Platform=x86 -v:diagnostic
>   -> Task Parameter:Optimization=MaxSpeed
> msbuild ... -v:detailed | grep -oE "/O[0-9a-z]+|/GL"
>   -> /O2  /Oi  /Oy  /Gy  /GL
> ```
>
> MSBuild supplies `Optimization=MaxSpeed` as a default, so Release was always compiling
> with `/O2 /Oi /Oy /Gy /GL`. The analysis that produced the wrong conclusion tested raw
> `cl` invocations without `/O`, which is not what MSBuild emits — the right experiment is
> to ask MSBuild what it passed, not to reason about what the project file omits.
>
> The contrast run did surface a real defect the document missed: **`Debug|Win32` was ALSO
> compiling with `/O2`**, because it too declared no `<Optimization>` and inherited the same
> default. A configuration named Debug was producing inlined frames and optimised-away
> locals, which makes it useless to the one audience that matters for it — someone who can
> actually run the game. Fixed: Debug is now explicitly `Disabled`, Release explicitly
> `MaxSpeed`, both verified by re-reading the task parameters, both still building at
> exactly 44 warnings.
>
> Item **D1** ("land MaxSpeed") in the plan below is therefore already true and should be
> struck. Everything else in this document — the `ListSet` O(capacity) enumeration, the two
> genuine O(N²) paths, and the per-frame allocation list — was NOT part of the refuted
> claim and still stands on its own evidence, but treat every remaining number here as
> provisional until re-derived by hand.


Status: **doctrine**, not a research note. §8 is binding on every future performance claim
made in this repository.

**Scope of "verified".** As everywhere else in this project, *verified* means one of exactly
two things: a number was re-derived by hand from source, or a result was reproduced from a
toolchain on the authoring machine. It never means behaviour was observed in a running game.
**We cannot run the game.** Every statement about wall-clock time in situ is marked
UNDETERMINED and stays that way until somebody with an installation measures it.

---

## 0. The shape of the problem

Three facts constrain everything in this document.

1. **Most of the frame is not ours.** Of the 159 distinct callees named directly in the body
   of `Lego_MainLoop` (`game/GameState.cpp:890-1870`), **36 are raw exe address macros** —
   1999 machine code reached through a function-pointer cast, e.g.
   `#define AITask_UpdateAll ((void (__cdecl* )(real32 elapsedGame))0x00402150)`
   (`game/object/AITask.h:245`), `#define Lego_HandleWorld (... )0x00426450)`
   (`game/Game.h`), `#define Panel_Draw (... )0x0045a9f0)`, `#define Map3D_Update
   (... )0x0044f2b0)` (`game/world/Map3D.h:220`). Those are not profilable, not modifiable,
   and — because `hook_write_jmpret` (`src/openlrr/hook.cpp:36`) destroys the prologue and
   every restore path is commented out (`hook.cpp:30-32,47-49`) — not wrappable. Any
   reimplementation must be total. This document therefore audits the ~110 callees that are
   ours and states honestly where the trail goes cold.

2. **The hot data layout is frozen.** `LegoObject` is pinned at `0x40c` bytes by
   `assert_sizeof(LegoObject, 0x40c)` (`game/object/Object.h:447`) and the exe indexes its
   fields at fixed offsets. Nothing in it may move. See §5 for what that costs and what may
   be done instead.

3. **Rendering is Direct3D Retained Mode.** `Viewport_Render`, `Container_Mesh_GetVertices`
   / `SetVertices` and the `Draw_*` family bottom out in a 1999 API. Their cost dominates
   the frame and is not addressable here. The `docs/research/rendering-ceiling.md`
   conclusion — a modern backend is ruled out — is *not* re-litigated.

Notation used throughout:

| Symbol | Meaning |
| --- | --- |
| **N** | live objects (`objectListSet.CountAlive()`) |
| **C** | `objectListSet` *slot capacity* = `2^listCount − 1` (`ListSet.hpp:67-71`). **C ≥ N, and C never shrinks during a level.** This distinction is the single most important finding in §1. |
| **B** | map blocks = `level->width × level->height` (`game/Game.h:396-397`) |
| **P** | water pools; **W** total pool blocks; **D** total pool drains |
| **T** | `aiListSet` capacity; **F** live electric fences; **Z** construction zones |

---

## 1. Per-frame path table

Entry is `OpenLRR_MainLoop_Wrapper` (`src/openlrr/OpenLRR.cpp:971`), which calls
`legoState.MainLoop` → `Lego_MainLoop` (`game/GameState.cpp:890`), then `DeepCore::Waves::Update`
(`OpenLRR.cpp:984`), then optionally `OpenLRR_UpdateWobblyWorld` (`OpenLRR.cpp:987-989`).

**Owner** column: `OURS` = implemented C++ in this tree; `EXE` = raw address macro, 1999
machine code. **Verdict**: `—` acceptable, `!` cost worth knowing, `DEFECT` should be fixed.

| # | Path | file:line | Owner | Complexity | Verdict |
| ---: | --- | --- | --- | --- | --- |
| 1 | `OpenLRR_MainLoop_Wrapper` | `OpenLRR.cpp:971` | OURS | O(1) + callees | — |
| 2 | `DeepCore::Waves::Update`, Waiting phase | `WaveDirector.cpp:281-346` | OURS | O(1) — six branches and two float adds before the `timer < interval` return at `:346` | — |
| 3 | `DeepCore::Waves::Update`, **interval expiry** (once per 45–150 s) | `WaveDirector.cpp:351-377` | OURS | **O(C) ×2 + O(B) + O(B·Bld) + 2 heap vectors** | **DEFECT** (spike) |
| 4 | `OpenLRR_UpdateWobblyWorld` | `OpenLRR.cpp:940-952` | OURS | **O(B) every frame**, unconditional double loop over `blockHeight × blockWidth` calling `Map3D_SetBlockUVWobbles` | **DEFECT** (debug feature, default off) |
| 5 | `Shortcuts::ShortcutManager::Update` | `Shortcuts.cpp:251-255` | OURS | O(114) virtual calls through `shared_ptr` (`ShortcutID::Count` = 114, `Shortcuts.hpp:310`) | ! |
| 6 | `Teleporter_Update` | `world/Teleporter.h:173` | EXE | unknown | UNDETERMINED |
| 7 | `Level_Emerge_FUN_0042c370` | `Game.h` macro | EXE | unknown | UNDETERMINED |
| 8 | `Lego_UpdateFallins` | `Game.cpp:3234` | OURS | O(active fallins) | — |
| 9 | `Lego_UpdateSlug_FUN_004260f0` | `Game.h` macro | EXE | unknown | UNDETERMINED |
| 10 | `AITask_UpdateAll` | `object/AITask.h:245` | EXE | presumed O(T); `AITask_RunThroughLists` (`AITask.cpp:530`) shows the idiom is capacity-walking | UNDETERMINED |
| 11 | `Level_UpdateEffects` | `Game.cpp:1067` | OURS | O(rockfalls completed this tick) | — |
| 12 | `DamageText_UpdateAll` | `effects/DamageText.cpp:262` | OURS | O(active texts) | — |
| 13 | **`LegoObject_UpdateAll`** | `object/Object.cpp:1673-1685` | OURS | O(C) outer, **but `LegoObject_UpdateSlipAndScare` (`Object.cpp:5313-5320`) runs a full `EnumerateSkipUpgradeParts` for every RockMonster** ⇒ **O(C · M)** | **DEFECT — quadratic over live objects** |
| 13a | ↳ ticking-dynamite scare loop | `Object.cpp:5333` | OURS | another full O(C) per armed Dynamite/OohScary in its last 3 s | **DEFECT** |
| 14 | **`Weapon_Update`** | `object/Weapons.cpp:372-400` | OURS | O(C) at `:381`, **plus a full O(C) per airborne projectile** via `Weapon_Projectile_UpdatePath` → `Weapons.cpp:348` ⇒ **O(C · projectiles)**, plus three unconditional fixed sweeps of 10 | **DEFECT — quadratic over live objects** |
| 15 | **`Erosion_Update`** | `world/Erosion.cpp:93,154,218` | OURS | fixed sweep of `EROSION_MAXERODEBLOCKS`=2000 then `EROSION_MAXLOCKEDBLOCKS`=1000 **regardless of occupancy** (`Erosion.h:26-28`) — ~12 KB of `bool32`/`real32` streamed per frame to find, typically, zero set entries | **DEFECT — dense sweep of a sparse fixed array** |
| 16 | `Level_BlockActivity_UpdateAll` | `Game.cpp:3540` | OURS | O(active) intrusive list | — |
| 17 | `Message_Update` | `mission/Messages.cpp:198` | OURS | O(hotkeys + queued events) | — |
| 18 | `Camera_Update` ×3 | `world/Camera.cpp:200` | OURS | O(1) each | — |
| 19 | `Construction_UpdateAll` | `world/Construction.cpp:457` | OURS | O(Z); **each READY zone calls `Construction_Zone_NoForeignObjectsInside` (`Construction.cpp:493-500`) which is a full O(C)** ⇒ O(Z·C) | ! (Z is small; still a nested full walk) |
| 20 | `Interface_FUN_0041b940` | `interface/Interface.h` macro | EXE | unknown | UNDETERMINED |
| 21 | `Lego_UnkUpdateMapsWorldUnk_FUN_004290d0` ×2 | `Game.h:1315` | EXE | unknown, called twice per frame (`GameState.cpp:1122,1316`) | UNDETERMINED |
| 22 | `LegoObject_HideAllCertainObjects` ×2 | `Object.cpp:5737-5741` | OURS | O(C) each, called at `GameState.cpp:1124` and `:1317` | ! |
| 23 | **`ElectricFence_UpdateAll`** | `world/ElectricFence.cpp:250-268` | OURS | O(F) + **two full O(C) object walks with no early-out when F == 0** | **DEFECT** |
| 24 | `Lego_HandleWorld` | `Game.h` macro `0x00426450` | EXE | unknown; mouse↔world interaction, almost certainly iterates objects | UNDETERMINED |
| 25 | `Map3D_Update` | `world/Map3D.h:220` | EXE | unknown, presumed O(visible blocks) | UNDETERMINED |
| 26 | `Water_Update` | `world/Water.cpp:805-904` | OURS | O(P·D) always; **O(W) with two D3DRM mesh calls per pool block** when a pool is rising or lowering (`Water.cpp:891-901`) | ! (see §1.3) |
| 27 | **`Water_UpdateNotHotBlocks`** | `Water.cpp:905,909-963` | OURS | O(P·D) **unconditional**, and each drain issues `Map3D_GetBlockVertexPositions` + 2 × `Level_Block_SetNotHot` (EXE, `Game.h:1536`) *whether or not anything changed* | **DEFECT** |
| 28 | `Smoke_Update` | `effects/Smoke.cpp:364` | OURS | O(smokes × groups), intrusive list | — |
| 29 | `SpiderWeb_Update` | `world/SpiderWeb.h:123` | EXE | unknown | UNDETERMINED |
| 30 | `LightEffects_Update` | `effects/LightEffects.cpp:285` | OURS | O(1) | — |
| 31 | `Fallin_Update` | `world/Fallin.cpp:34-56` | OURS | O(1) amortised — timer-gated, then `FALLIN_MAXTRIES` random probes | — |
| 32 | `Debug_RouteVisual_UpdateAll` | `Debug.cpp:433-487` | OURS | O(R log R) `std::map` walk; **+ full O(C) when auto-mode is `All`/`AllFriendly`** (`Debug.cpp:477`) | ! (default off) |
| 33 | `Viewport_Clear` / `Viewport_Render` | `GameState.cpp:1202-1203,1252-1253,1326` | D3DRM | dominant, opaque | UNMEASURABLE |
| 34 | `Lego_DrawAllSelectedUnitBoxes` / `…UnitNames` | `Game.cpp:1153,1170` | OURS | O(selected) — uses `Message_GetSelectedUnits`, not a list walk | — (good pattern) |
| 35 | `Lego_DrawAllLaserTrackerBoxes` / `…Names` | `Game.cpp:1133,1142` | OURS | O(C) **each**, to find the ≤1 object with `LIVEOBJ4_LASERTRACKERMODE` | **DEFECT** (two full walks for one flag) |
| 36 | `Bubble_DrawAllObjInfos` | `interface/hud/Bubbles.cpp:359-440` | OURS | up to **3 × O(C)** (`:365`, `:380`, `:423`) plus fixed sweeps of `BUBBLE_MAXSHOWHEALTHBARS` / `…POWEROFFS` | **DEFECT** (three walks, same predicate) |
| 37 | `RadarMap_Draw` | `interface/RadarMap.cpp:347-470` | OURS | O(visible blocks) + O(C) at `:463`; **one `Mem_Alloc`/`Mem_Free` pair per frame** at `:379`/`:470` | **DEFECT** (see §2) |
| 38 | `LegoObject_UpdateAllRadarSurvey` | `Object.cpp:2007-2016` | OURS | O(C) | ! |
| 39 | `Panel_Draw` ×9, `Info_Draw`, `Info_DrawPanel`, `ScrollInfo_Update`, `Interface_FUN_0041b3c0/b860`, `Panel_Crystals_Draw`, `Panel_CryOreSideBar_Draw`, `HelpWindow_FUN_00418930` | `GameState.cpp:1362-1413` | EXE | unknown | UNDETERMINED |
| 40 | `Priorities_Draw` | `interface/Priorities.cpp:393` | OURS | O(priority buttons) | — |
| 41 | `Objective_Update` | `mission/Objective.cpp:912` | OURS | O(1) until crystal/ore prerequisites are met, then O(C) at `Objective.cpp:1119` | — |
| 42 | Level-loss failsafe | `GameState.cpp:1442-1455` | OURS | O(pilot levels) × O(1) table reads (`LegoObject_GetLevelObjectsBuilt`, `Object.cpp:383`) | — (correctly O(1) per query) |
| 43 | **`SFX_GetType("SND_AirBeat", …)` ×3** | `GameState.cpp:1715,1725,1729` | OURS | each call hashes an 11-char literal then linear-scans `hashNameList` up to `hashNameCount + SFX_Preload_Count` (`audio/SFX.cpp:112-123`) — **three times per frame, for a value fixed at load** | **DEFECT** |
| 44 | `Font_PrintF` family (every debug/HUD string) | `engine/drawing/Fonts.cpp:194,217,222` | OURS | **1024-byte zero-fill per call** from `char fmtLine[FONT_MAXSTRINGLEN] = { '\0' }` | **DEFECT** |
| 45 | `Level_PowerGrid_UnpowerPoweredBlocks` + `…UpdateUnpoweredBlockSurfaces` | `Game.cpp:3849,3871` | OURS | O(powered blocks), but performs a **full `std::vector` copy-assign** at `Game.cpp:3874` where a swap would do | **DEFECT** |
| 46 | `Roof_Update` (FP view only) | `world/Roof.cpp:169-220` | OURS | O(roof grid) when `needsUpdate`; **a 128-byte `Point2I DIRECTIONS_4X4[4][4]` is rebuilt on the stack inside the per-block body** (`Roof.cpp:188`) | ! |
| 47 | `Text_Update`, `Advisor_Update`, `Pointer_Update`, `ToolTip_Update`, `SFX_Update` | `TextMessages.cpp:223`, `Advisor.cpp:304`, `Pointers.cpp:257`, `ToolTip.cpp:281`, `SFX.cpp:489` | OURS | O(1)–O(queue) | — |

### 1.1 The structural defect: enumeration is O(capacity), not O(alive)

This is the finding that makes half the table worse than it looks.

`ListSet` allocates geometrically: list *i* holds `2^i` items (`ListSet.hpp:78-82`), so
`listCount` lists give capacity `2^listCount − 1` (`ListSet.hpp:67-71`). `BaseIterator::operator++`
(`ListSet.hpp:327-361`) walks **every slot of every allocated list**, calling the filter
predicate on each and skipping the ones that fail. `Remove` (`ListSet.hpp:726-736`) only
splices the item back onto `freeList`; **`PopList` is `protected` and never called from
`Remove`, so capacity is monotonic for the life of the level.**

Consequences, all provable from the cited lines:

- A level that once peaked at 200 objects pays 255-slot enumeration cost forever after, even
  with three objects alive.
- `LegoObject` is 1036 bytes (`Object.h:355`, `assert_sizeof` at `:447`). The predicate reads
  `nextFree` at offset `0x408` (`Object.h:444`); `FilterSkipUpgradeParts` additionally reads
  `flags3` at `0x3e8` (`Object.cpp:72-75`, `Object.h:436`). Those are in **different 64-byte
  cache lines** (`0x3e8` → line 15, `0x408` → line 16). So merely *rejecting* a dead slot
  costs one to two cache lines, and no useful payload rides along.
- `ListSet` already tracks `m_aliveCount` and exposes O(1) `CountAlive()` (`ListSet.hpp:663`),
  but **there is no O(alive) iteration path.** Every `Enumerate*` is O(capacity).
- `FilterSkipUpgradeParts` is defined **out of line** in `Object.cpp:72` (the header body is
  commented out at `Object.h:514-516`) and passed as a non-type template parameter of
  function type. Without cross-module inlining it is a real `call` per slot visited. §6
  shows that the shipping Release build does not inline it.

Counting only paths I verified as ours and unconditional, a top-down frame with the radar
map open performs **at least eight full O(C) walks** of the object list (`Object.cpp:1683`,
`Weapons.cpp:381`, `Object.cpp:5739` ×2, `ElectricFence.cpp:257,264`, `RadarMap.cpp:463`,
`Object.cpp:2015`), before counting the nested ones in rows 13, 13a, 14 and 19.

### 1.2 The quadratic paths, named

Two paths are genuinely O(N²) over live objects and both are ours:

```cpp
// game/object/Object.cpp:5313  — called for EVERY object from LegoObject_Callback_Update:3583
void __cdecl LegoRR::LegoObject_UpdateSlipAndScare(LegoObject* liveObj, real32 elapsed)
{
    if (liveObj->type == LegoObject_RockMonster) {
        for (auto obj : objectListSet.EnumerateSkipUpgradeParts()) {   // <-- full walk, per monster
            if (LegoObject_Callback_SlipAndScare(obj, liveObj))
                break;
        }
    }
    ...
```

```cpp
// game/object/Weapons.cpp:337-351  — inside Weapon_Projectile_UpdatePath, per airborne shot
SearchWeapons18_2 search = { 0 };
...
for (auto obj : objectListSet.EnumerateSkipUpgradeParts()) {           // <-- full walk, per projectile
    Weapon_LegoObject_Callback_FUN_00471630(obj, &search);
}
```

In a Call-to-Arms firefight — many monsters *and* many projectiles simultaneously — these
multiply. `WEAPON_MAXPROJECTILES` is only 10 (`Weapons.h:32`), which caps the second at
`10 · C`; the first is uncapped in the monster count, and the **wave director exists
specifically to raise the monster count** (`WaveDirector.cpp`, `settings.waveMaxAlive`
default 6, `DeepCore.hpp:114`). We are shipping a feature whose whole purpose is to push
harder on the one path that is quadratic in exactly that quantity. That is the single most
important thing in this document for anyone tuning `waveMaxAlive` upward.

### 1.3 Water: what the index indirection actually costs

The relocation (`world/Water.cpp:34-352`) replaced direct `waterGlobs.poolList[i].field`
access with accessor functions, each of the form:

```cpp
// game/world/Water.cpp:153
static Point2F& Pool_Block(uint32 pi, uint32 bi)
{
    return WaterRelocated() ? _dcPools[pi]->blocks[bi]
                            : LegoRR::waterGlobs.poolList[pi].blocks[bi];
}
```

The module's own comment (`Water.cpp:97-99`) states the design contract precisely:

> "Every accessor below is a single ternary whose untaken arm is never evaluated, so with the
> gate off each one reduces to exactly the expression it replaced."

**That reduction is a statement about an optimising compiler, and §6 shows the shipping
Release build is not one.** Under `/Od` every accessor is an out-of-line `call`, every
`WaterRelocated()` re-reads `DeepCore::settings.relocateWaterTables`, and every
`_dcPools[pi]->blocks[bi]` is three dependent loads (vector buffer → `unique_ptr` →
`DeepCorePool` → inner vector buffer → element) where vanilla was one indexed load into a
flat `0x428`-byte struct.

Independently of the compiler, three source-level costs are real:

- **Loop conditions re-evaluate the accessor every iteration**: `i < Pool_Count()`
  (`Water.cpp:812`, `:925`, `:976`), `j < Pool_DrainCount(i)` (`:836`, `:926`, `:977`),
  `j < Pool_BlockCount(i)` (`:891`). Each is a fresh branch on the gate plus a
  `std::vector::size()`. Hoisting them is free and behaviour-preserving.
- **`Pool_Mesh(i)` is re-fetched inside the vertex loop** (`Water.cpp:894`, `:900`), once per
  pool block per frame.
- `Water_UpdateNotHotBlocks` (`Water.cpp:909`) runs **unconditionally every frame** — it is
  not gated by `lowering || rising` — and issues two `Level_Block_SetNotHot` calls per drain.
  That function is an exe macro (`Game.h:1536`) and the source comment at `Water.cpp:945`
  notes that "SetNotHot updates the block surface", i.e. it can trigger texture work. Whether
  the exe short-circuits on an unchanged value is **UNDETERMINED** and unknowable without the
  binary.

There is also a plain debug leftover on the frame path: `Error_InfoF` at `Water.cpp:823`,
guarded only by a flag-change test on pool 0, with two file-scope statics at `:809-810`.

### 1.4 The WaveDirector, audited without indulgence

This is new DeepCore code and it gets no discount.

**Good, and worth keeping.** The Waiting-phase path is genuinely O(1): `Update`
(`WaveDirector.cpp:281`) returns at `:285` when the feature is off, `:291` when out of level,
`:295` when paused, and `:346` when the interval has not elapsed. `CountLiveMonsters`
(`:92`) and `CountMiniFigures` (`:109`) — each a full O(C) walk — are correctly placed
*after* the interval gate, so they run once per wave, not once per frame. That is the right
structure and it was not an accident.

**Bad, and it is a frame-time spike.** When the interval does expire, one frame does all of
this (`WaveDirector.cpp:351-377`):

- `CountMiniFigures()` — O(C) walk (`:351`)
- `CountLiveMonsters()` — a second O(C) walk (`:355`)
- `GatherCandidates()` (`:181-212`) — a third O(C) walk to collect building positions
  (`:189-195`), **then a full O(B) sweep of the map** (`:199-201`), and for every block that
  passes `IsFairSpawnBlock` a **linear scan over every building** (`:204-206`) ⇒ **O(B · Bld)**
- `IsFairSpawnBlock` (`:129`) reads `blockValue(level, bx, by)` **twice** for the same block
  (`:137` and `:138`) and then reads four neighbours at ±1 and ±`width` stride (`:161-167`),
  which breaks the sequential access pattern on a `0x48`-byte-stride array.

On a 64×64 map with 20 buildings that is ~82,000 distance tests plus ~25,000 block reads in a
single frame; on a 128×128 custom map with 40 buildings, ~650,000 distance tests. Nothing
amortises this across frames. It fires roughly every 45–150 world-seconds, which means it is
a *hitch*, and hitches are more visible than throughput. **UNDETERMINED in situ** — but the
work is real and countable from source, and it is avoidable (§7, fix A4).

**Also honest:** `GatherCandidates` allocates two heap vectors per call (`bases` at `:188`,
and the caller's `candidates` local at `:358`), and `Telegraph`/`SpawnMonsterAtBlock` are
fine. `static const sint32 DX[4]/DY[4]` at `:159-160` are constant-initialised function
statics, so they carry no guard variable — correct as written.

---

## 2. Allocation audit

**Doctrine: nothing on the frame path may allocate.** The DLL is injected into a 1999
process and shares one CRT heap with it (§6); a per-frame `malloc`/`free` pair is both a
latency spike and a fragmentation source in a heap we do not control.

Every allocation reachable per frame, found by grepping `Mem_Alloc`, `new`, `make_unique`,
`push_back`, `emplace_back`, `.resize(`, `.insert(`, `std::string` and `std::map`
construction across `game/` and `engine/`, then walking call chains back to `Lego_MainLoop`:

### 2.1 Violations — these allocate on the frame path

| # | Site | file:line | What | When |
| ---: | --- | --- | --- | --- |
| **V1** | `RadarMap_Draw` | `interface/RadarMap.cpp:379` and `:470` | `Gods98::Mem_Alloc(rectCount * sizeof(Gods98::Draw_Rect))` + matching `Mem_Free`. `rectCount = max(1500, gridW+gridH)` (`:375`), `sizeof(Draw_Rect) == 0x1c` (`engine/drawing/Draw.h:108`) ⇒ **≥ 42,000 bytes malloc'd and freed every frame** the radar map is open. | every frame in radar-map view |
| **V2** | `Level_PowerGrid_UnpowerPoweredBlocks` | `Game.cpp:3874` | `_unpoweredBlockList = _poweredBlockList;` — a `std::vector<Point2I>` **copy-assign**. Capacity is retained across frames (the destination is `clear()`ed, not freed, at `Game.cpp:3862`), so it usually reuses the buffer — but it is an unconditional O(n) copy where `std::swap` is O(1), and it *will* allocate whenever the powered-block set grows past the previous high-water mark. | every frame while `OBJECT_GLOB_FLAG_POWERUPDATING` (`Object.cpp:1676,1692`) |
| **V3** | `Level_PowerGrid_AddPoweredBlock` / `AddDrainPowerBlock` | `Game.cpp:3835`, `:3903` | `push_back` into file-scope vectors. Amortised O(1) and capacity is retained, so this is *bounded* rather than per-frame — but it is unreserved growth on a frame path. | during power-grid recalculation |
| **V4** | `Debug_RouteVisual_Add` (auto-mode `All`/`AllFriendly`) | `Debug.cpp:248`, reached from `Debug.cpp:477` | `_routeVisuals[liveObj] = routeVisual;` — a `std::map` node allocation per newly-tracked object, plus a red-black `find` per object per frame. | every frame, debug feature, default off |
| **V5** | `Font_VPrintF2` | `engine/drawing/Fonts.cpp:222` | Not a heap allocation, but a **1024-byte stack zero-fill on every call**: `char line[FONT_MAXSTRINGLEN], fmtLine[FONT_MAXSTRINGLEN] = { '\0' };`. The `= { '\0' }` is unnecessary — the loop at `:228-238` writes `fmtLine` and null-terminates it explicitly at `:238`. Every HUD/debug string pays a 1 KB `memset`. | every `Font_PrintF`, several per frame |

### 2.2 Per-wave allocations in `WaveDirector.cpp` — named, as required

Not per *frame*, but named here because the brief asks for them and because they land inside
the spike described in §1.4:

| Site | file:line | What |
| --- | --- | --- |
| `GatherCandidates` | `WaveDirector.cpp:188` | `std::vector<Point2I> bases;` — fresh vector, allocated and destroyed every wave. |
| `GatherCandidates` | `WaveDirector.cpp:194` | `bases.push_back(bp)` — unreserved growth, `O(log Bld)` reallocations. |
| `Waves::Update` | `WaveDirector.cpp:358` | `std::vector<Point2I> candidates;` — fresh vector, allocated and destroyed every wave; on a 64×64 map this can reach thousands of entries and reallocate ~12 times. |
| `GatherCandidates` | `WaveDirector.cpp:209` | `out.push_back(...)` — unreserved. |
| `Waves::Update` | `WaveDirector.cpp:374` | `_s.pendingBlocks.push_back(...)` — **this one is fine**: `pendingBlocks` is a `State` member (`:56`) that is `clear()`ed (`:323`, `:371`) rather than destroyed, so its capacity is reused. This is the pattern the other two should follow. |

### 2.3 Cleared — allocations that are *not* per frame

Checked and found load-time, level-time or bounded-cache only, so they are **not** violations:

- `ListSet::AddList` → `Mem_Alloc` (`ListSet.hpp:747`): once per capacity doubling, ≤ 32 times
  per level for objects (`OBJECT_MAXLISTS`, `Object.h:48`).
- `_objInfoHealthBarsCache` (`interface/hud/ObjInfo.cpp:34`, insert at `:158`): keyed by
  health-bar pixel width, so bounded by the bar width; steady-state is pure lookup.
- `_interfaceHoverOutlinesCache` (`interface/Interface.cpp:49`, insert at `:268`): keyed by
  outline size; bounded by the number of distinct UI element sizes.
- `_beamStyleByWeapon` (`DeepCore.cpp:319`): keyed by weapon index, at most `weaponCount`
  entries, resolved once each (`DeepCore.cpp:337-350`).
- `_resolvedSpecies` / `_resolvedWaveSpecies` (`DeepCore.cpp:67,73`): resolved once, guarded
  by `_speciesResolved` / `_waveSpeciesResolved`.
- `_dcPools` / `_dcMergeList` (`Water.cpp:53,58`): populated during `Water_Initialise`, never
  grown by `Water_Update`.
- `_filledPoolBlocks` (`Water.cpp:22`): flood-fill scratch, level load only.
- `Smoke_Add` → `Mem_Alloc` (`effects/Smoke.cpp:75`), `Construction_Zone` (`Construction.cpp:537`),
  `MeshLOD` (`object/MeshLOD.cpp:32,48,76`), `Camera_Create` (`world/Camera.cpp:38`),
  `Roof` grid (`world/Roof.cpp:37`), `Encyclopedia` file tables (`Encyclopedia.cpp:67-112`):
  all event- or load-driven.
- `Error_Format` (`engine/core/Errors.cpp:153-163`) uses a `static char res[1024]` — no
  allocation, but note it is **not reentrant**; nesting two `Error_*F` formats corrupts the
  outer one.

---

## 3. Layout — what is ours to repack and what is frozen

The rule is `docs/ADDRESS-MAP.md`: 113 sized regions, 0 overlaps, and **growing an
exe-overlaid type overwrites its neighbour rather than moving it.** Field *reordering* inside
such a type is equally forbidden — the 1999 code indexes fields at fixed byte offsets, which
`assert_sizeof` cannot detect.

### 3.1 Frozen — exe-overlaid, `assert_sizeof`-pinned, do not touch

| Type / glob | Size | Pin | Why it matters per frame |
| --- | --- | --- | --- |
| `LegoObject` | `0x40c` (1036 B) | `Object.h:447` | **The single hottest layout in the game and the one we cannot fix.** `type` @ `0x000`, `health` @ `0x340`, `flags1..4` @ `0x3e0-0x3ec`, `nextFree` @ `0x408` (`Object.h:357,403,434-437,444`). A predicate reading `{type, health, flags1}` plus the list filter's `{nextFree, flags3}` touches **4 of the 17 cache lines** in each object, at a 1036-byte stride. |
| `LegoObject_Globs` (`objectGlobs`) | `0xc644` | `Object.h:491`, bound at `Object.cpp:56` (`0x004df790`) | Holds `listSet[32]` (`Object.h:452`) plus `objectTotalLevels`/`objectPrevLevels` `[20][15][16]` tables (`:463-464`). |
| `AITask` / `AITask_Globs` | `0x68` / `0x4e9c` | `AITask.h:130,152` | `AITASK_MAXLISTS` = 12 ⇒ capacity ceiling 4095 (`AITask.h:30`). |
| `Water_Pool` / `Water_Globs` | `0x428` / `0x29ec` | `Water.h:83,97` (glob bound at `Water.cpp:19`, `0x0054a520`) | Deliberately left **zeroed** when `RelocateWaterTables` is on, except for the legacy shadow at `poolList[0]` (`Water.cpp:370-392`). |
| `Erosion_Globs` | `0xfa14` | `Erosion.h:62`, bound at `Erosion.cpp:21` (`0x004c8eb0`) | The 2000/1000-entry parallel arrays swept every frame (§1, row 15) — `activeBlocks`, `activeUsed`, `activeTimers`, `lockedBlocks`, `lockedTimers`, `lockedUsed` (`Erosion.h:48-55`). **Classic AoSoA-hostile split we cannot repack.** |
| `Smoke` / `Smoke_Globs` | `0x2a8` / `0x34` | `effects/Smoke.h:101,112` | |
| `Draw_Rect` | `0x1c` | `engine/drawing/Draw.h:108` | Allocated 1500 at a time per frame (§2, V1). |
| `Lego_Block` | `0x48` | `Game.h:371` | `B` of these; `blockValue` is `((l)->blocks[((l)->width*(by))+(bx)])` (`Game.h:780,784`). Any full-map sweep streams `B × 72` bytes. |
| `Stats_Globs`, `SFX_Globs`, `Text_Globs`, `LegoObject_Globs`, … | — | `docs/ADDRESS-MAP.md` | 113 regions total. `sfxGlobs` ends at exactly `0x00503bd8` = `statsGlobs`, the only zero-slack pair in the map. |

**Conclusion for §3:** the hot loops' cache behaviour is set by types we are forbidden to
change. Repacking is not on the table. The only levers are (a) **fewer passes over the same
data** and (b) **DLL-side side-tables** that hold the hot predicate bits densely — which is
exactly what fix A1 in §7 proposes.

### 3.2 Ours — DLL-side, repackable, and their honest verdicts

| Container / struct | file:line | Layout verdict |
| --- | --- | --- |
| `DeepCorePool` | `Water.cpp:34-42` | **Cache-hostile by construction, and it is on the frame path.** Two `std::vector`s by value inside a heap object held by `unique_ptr` inside another vector. Reaching one water block is 4 dependent loads. The `unique_ptr` is load-bearing for pointer stability (`Water.cpp:48-53`) and must not be removed, but the *inner* `blocks` vector could be a flat DLL-side arena indexed by (pool, block) with no loss of stability. |
| `_dcPools` | `Water.cpp:53` | `std::vector<std::unique_ptr<DeepCorePool>>` — pointer-chase per pool. P is small (default cap 4096, realistically < 10), so this is acceptable; it is `DeepCorePool`'s *interior* that costs. |
| `_dcMergeList` | `Water.cpp:58` | `std::vector<std::pair<uint32,uint32>>` — flat, fine, and load-time only. |
| `_poweredBlockList` / `_unpoweredBlockList` / `_drainPowerBlockList` | `Game.cpp:171-173` | Flat `std::vector<Point2I>` — **good layout**, correct replacement for the old fixed arrays. Spoiled only by the copy at `:3874` (§2, V2). |
| `Waves::State` | `WaveDirector.cpp:45-57` | Field order is fine (4-byte scalars then the vector); the vector member is the right call because it retains capacity. No repacking needed. |
| `DeepCore::Settings` | `DeepCore.hpp:56+` | Large struct mixing `bool`, `real32`, `sint32`, `std::vector<std::string>` and a `BeamStyle` table. Read on the frame path only through single-field probes (`settings.waveDirector` at `WaveDirector.cpp:285`, `settings.relocateWaterTables` at `Water.cpp:104`), which each pull one cache line that then stays hot. **Acceptable as is**, but the frequently-probed booleans should be grouped at the front so one line covers all of them. |
| `Shortcuts::ShortcutInfo` / `shortcutInfos` | `Shortcuts.hpp:325-371` | **Cache-hostile and touched every frame.** Each element carries two `std::string`s (32 B each under MSVC) and a `shared_ptr` (`:325,328,330`); `Update` (`Shortcuts.cpp:251-255`) walks all 114 and makes a virtual call through the `shared_ptr` for each. The strings are *never* read on the frame path — only in `ToString`/`Load`. A parallel `std::vector<InputButtonBase*>` for the update sweep would turn 114 scattered heap objects into one dense pointer array. |
| `_routeVisuals` | `Debug.cpp:70` | `std::map<LegoObject*, Debug_RouteVisual*>` — a red-black tree keyed by pointer, i.e. maximally cache-hostile. Debug feature, default off; not worth changing until it is on by default. |
| `_objInfoHealthBarsCache` | `interface/hud/ObjInfo.cpp:34` | `std::map<uint32, Image*>` keyed by a small dense integer (health-bar pixel width). Should be a flat `std::vector<Image*>` indexed directly — same semantics, one load instead of a tree walk, and it is on the per-object HUD path. |
| `_beamStyleByWeapon` | `DeepCore.cpp:319` | `std::map<sint32,sint32>` keyed by a small dense weapon index. Same comment; lower stakes (per laser spawn, not per object). |
| `_interfaceHoverOutlinesCache` | `interface/Interface.cpp:49` | `std::map<std::pair<uint32,uint32>, Image*>` — a handful of entries, hit once or twice per frame. Fine. |

---

## 4. Build configuration

Read from `src/openlrr/openlrr.vcxproj`. **Findings here outrank everything in §7 that is not
a fix to a quadratic**, because they scale every line of C++ in the DLL at once.

### 4.1 What is set

| Setting | Debug\|Win32 | Release\|Win32 | Line |
| --- | --- | --- | --- |
| `ConfigurationType` | `DynamicLibrary` | `DynamicLibrary` | `:30`, `:36` |
| `UseDebugLibraries` | **`false`** | `false` | `:31`, `:37` |
| `PlatformToolset` | `v142` | `v142` | `:32`, `:38` |
| `WholeProgramOptimization` | *(absent ⇒ false)* | **`true`** | `:39` |
| `LanguageStandard` | `stdcpp17` | `stdcpp17` | `:105`, `:133` |
| `ConformanceMode` | `true` (`/permissive-`) | `true` | `:102`, `:131` |
| `SDLCheck` | `true` (`/sdl`) | **`true` (`/sdl`)** | `:100`, `:129` |
| `WarningLevel` | `Level3` | `Level3` | `:99`, `:126` |
| `BasicRuntimeChecks` | **`Default`** (no `/RTC`) | *(absent)* | `:103` |
| `FunctionLevelLinking` | *(absent)* | `true` (`/Gy`) | `:127` |
| `IntrinsicFunctions` | *(absent)* | `true` (`/Oi`) | `:128` |
| `PreprocessorDefinitions` | no `_DEBUG`, no `NDEBUG` | `NDEBUG` | `:101`, `:130` |
| `LinkIncremental` | `true` | `false` | `:74`, `:83` |
| `EnableCOMDATFolding` / `OptimizeReferences` | — | `true` / `true` | `:137-138` |
| `GenerateDebugInformation` | `true` | `true` | `:109`, `:139` |

### 4.2 What is **not** set, and why that is the headline

`Release|Win32` has **no `<Optimization>` element**, no `<InlineFunctionExpansion>`, no
`<FavorSizeOrSpeed>`, no `<OmitFramePointers>`, no `<BufferSecurityCheck>`, no
`<StringPooling>`, no `<RuntimeLibrary>`, no `<FloatingPointModel>`.

I verified that **no MSBuild props file supplies a default for `ClCompile/Optimization`**:
`Microsoft.Cpp.Default.props` and `Microsoft.Cpp.Common.props` do not set it, no `.props`
under `MSBuild/Microsoft/VC` contains an `<Optimization>` element, and the rule file
`1033/cl.xml:389-424` declares the enum with **no `Default` attribute**. Empty metadata means
the `CL` task emits no `/O` switch, and `cl.exe` with no `/O` switch does not optimise.

`WholeProgramOptimization=true` does *not* rescue this. It imports
`Microsoft.Cpp.WholeProgramOptimization.props`, which sets `ClCompile/WholeProgramOptimization`
(⇒ `/GL`) and `Link/LinkTimeCodeGeneration` (⇒ `/LTCG`). LTCG then runs — and generates
unoptimised code, because the optimisation level recorded in the IL is "none".

**This was reproduced on the authoring machine**, not inferred. Using the v142 toolset
(`MSVC 14.29.30133`, `cl 19.29.30159`, x86), two builds of the same two translation units,
identical except for `/O2`:

```cpp
// t.cpp
int f(int a, int b) { return a * 2 + b * 2; }

// u.cpp
extern int f(int,int);
int g(int n){ int s=0; for(int i=0;i<n;i++) s += f(i,i+1); return s; }
int main(){ return g(7); }
```

`cl -c -GL t.cpp u.cpp` then `link -LTCG` — the linker prints "Generating code / Finished
generating code" and produces:

```
00401010: 55                 push        ebp          ; f(), out of line
00401011: 8B EC              mov         ebp,esp
00401013: 8B 45 0C           mov         eax,dword ptr [ebp+0Ch]
...
00401030: 55                 push        ebp          ; g(), real loop, spilled locals
00401033: 83 EC 08           sub         esp,8
00401036: C7 45 F8 00 00 00  mov         dword ptr [ebp-8],0
...
00401062: E8 A9 FF FF FF     call        00401010     ; calls f() seven times at runtime
```

`cl -c -GL -O2 t.cpp u.cpp` then the identical `link -LTCG`:

```
00401000: B8 62 00 00 00     mov         eax,62h      ; whole program folded to a constant
00401005: C3                 ret
```

**Verdict: `Release|Win32` as configured ships `/Od`-quality code with `/GL` and `/LTCG`
along for the ride.** No inlining, no register allocation worth the name, frame pointers
everywhere, and every `static` helper in `Water.cpp`, every `ListSet` iterator step, every
`Maths_Vector*` call is a real out-of-line `call`. This is not a micro-optimisation
opportunity; it is the difference between the code we wrote and the code we ship.

Two related consequences:

- **`/GS` is on by default in both configurations.** Reproduced: a function with a local
  `char buf[64]` compiles with a `__security_check_cookie` reference under both `-Od` and
  `-O2`; adding `-GS-` removes it and shrinks that one object file from 1192 to 877 bytes
  (−26%). `/sdl` (`:100`, `:129`) forces `/GS` on regardless of any `BufferSecurityCheck`
  setting, and additionally promotes a set of warnings to errors — I confirmed that `/sdl`
  turns `C4996` (`strncpy` deprecation) into a hard error, which is precisely why
  `_CRT_SECURE_NO_WARNINGS` (`:101`, `:130`) is load-bearing for this project.
- **`FunctionLevelLinking` and `IntrinsicFunctions` are set but largely inert** without an
  `/O` switch: `/Gy` only helps `/OPT:REF` fold what the optimiser produced, and `/Oi` cannot
  do much when nothing else is optimised.

### 4.3 `UseDebugLibraries=false` in the Debug configuration (`openlrr.vcxproj:31`)

Unusual, and the implications cut both ways. Since `RuntimeLibrary` is never set explicitly,
it is derived from `UseDebugLibraries`; `false` selects `MultiThreadedDLL` (`/MD`), i.e. the
**release CRT**, in *both* configurations. Combined with the absence of `_DEBUG` from
`Debug|Win32`'s `PreprocessorDefinitions` (`:101`) and `BasicRuntimeChecks=Default` (`:103`):

**Why it is probably deliberate and should be kept.** This DLL is injected into a 1999
executable that links the release CRT. Two CRT instances in one process means two heaps, and
`Mem_Alloc` in one module freed in the other is undefined behaviour — a hazard this project
cannot afford. It also means end users need no debug-CRT redistributable to run the Debug
build. Keeping one release CRT across both configurations is the safe choice here, and it
should be recorded as intentional rather than quietly "fixed".

**What it silently costs, and this is the part nobody has written down.** With `_DEBUG`
undefined, `_ITERATOR_DEBUG_LEVEL` defaults to **0**. There are therefore **no checked
iterators and no `std::vector` bounds assertions in either configuration.** Every unchecked
subscript in DLL-side container code is unchecked *everywhere*:

- `_dcPools[pi]` and `_dcPools[pi]->blocks[bi]` (`Water.cpp:149-308`, ~15 sites)
- `_dcMergeList[i]` (`Water.cpp:344,350`)
- `shortcutInfos[static_cast<size_t>(id)]` (`Shortcuts.hpp:409`) — reached by every
  `Shortcut_IsPressed` macro (`Shortcuts.hpp:442`)
- `candidates[(size_t)pick % candidates.size()]` (`WaveDirector.cpp:374`) — safe *because* of
  the modulo, which is doing real work, not defensive decoration

There is also no `/RTC` (`:103`), so uninitialised-local and stack-corruption detection are
both off. **The Debug build is not a checked build.** It differs from Release only in
`NDEBUG` (so `assert` is live) and in link incrementality. Anyone who assumes "it passed
Debug, so bounds are fine" is assuming something the project does not provide.

### 4.4 Suspicious 2019-era leftovers

- **The `x64` configurations are dead weight and would not build.** `Debug|x64` and
  `Release|x64` declare `ConfigurationType=Application` (`:43`, `:49`) — not
  `DynamicLibrary` — with `CharacterSet=Unicode`, no `IncludePath`/`LibraryPath` for the
  vendored `lib/d3drm` headers, no `d3drm.lib` in `AdditionalDependencies`, and none of the
  `_GODS98COMPILE` / `_GODS98_USEWAD_` defines (`:154-181`). This is a stock VS template
  residue in a project that is architecturally x86-only (it injects into a 32-bit 1999 exe).
  They should be deleted so nobody selects one and gets a confusing wall of errors.
- `LanguageStandard` is absent from both `x64` configurations, so they would compile as C++14
  and reject `ListSet.hpp`.
- `GenerateDebugInformation=true` in Release (`:139`) is correct and costs nothing at runtime,
  but note it produces a PDB that is not shipped by CI.
- `SubSystem=Console` on a `DynamicLibrary` (`:108`, `:136`) is harmless but meaningless.

### 4.5 What `Release|Win32` should be

```xml
<ClCompile>
  <WarningLevel>Level3</WarningLevel>
  <Optimization>MaxSpeed</Optimization>                    <!-- /O2  — the whole point -->
  <InlineFunctionExpansion>AnySuitable</InlineFunctionExpansion>  <!-- /Ob2 -->
  <FavorSizeOrSpeed>Speed</FavorSizeOrSpeed>               <!-- /Ot -->
  <IntrinsicFunctions>true</IntrinsicFunctions>            <!-- /Oi  (already set) -->
  <FunctionLevelLinking>true</FunctionLevelLinking>        <!-- /Gy  (already set) -->
  <StringPooling>true</StringPooling>                      <!-- /GF -->
  <WholeProgramOptimization>true</WholeProgramOptimization><!-- /GL  (via Configuration, set) -->
  <SDLCheck>true</SDLCheck>                                <!-- keep: it is a real gate -->
  <!-- Leave BufferSecurityCheck alone. /sdl forces /GS on regardless, and turning it
       off in a DLL that runs inside someone else's 1999 process is a bad trade. -->
  <!-- Do NOT set FloatingPointModel=Fast without a separate decision: this code
       shares float state with exe machine code we cannot inspect. -->
</ClCompile>
```

`OmitFramePointers` is deliberately **not** proposed: `/O2` already implies `/Oy` on x86, and
keeping it implicit avoids arguing about stack walks in crash reports.

**Warning-count risk.** The build contract is 0 errors and *exactly* 44 warnings at
`Level3`. Enabling `/O2` turns on the flow-analysis-dependent warnings — C4701
(potentially uninitialised local), C4702 (unreachable code), C4703 — which are simply not
emitted at `/Od`. **Any change to §4.5 must be landed behind a full `-t:Rebuild` of both
configurations and the warning tally re-verified before it is committed.** Do not assume the
count holds. If new warnings appear, fix the code; do not add `DisableSpecificWarnings`.

---

## 5. Ranked fix list

Ordered algorithmic → allocation → layout → build, as instructed. Within each tier, ordered
by expected effect. **Measurability** is stated honestly: `harness` means it can be measured
today or after a stated seam is added; `in situ only` means the only honest verdict is
"algorithmically better, unmeasured in situ".

### Tier A — algorithmic

**A1. Give `ListSet` an O(alive) iteration path.** *(the root fix; everything else in Tier A
shrinks if this lands)*
Add a DLL-side dense index alongside each `ListSet`: a `std::vector<value_type*>` of live
items maintained in `Add` (`ListSet.hpp:701`) and `Remove` (`ListSet.hpp:726`), plus a
`std::vector<uint8>` of hot predicate bits keyed by `ListSet::IndexOfInListSet`
(`ListSet.hpp:103`). Enumeration then walks a dense pointer array instead of `2^listCount − 1`
slots at a 1036-byte stride.
*Expected effect:* turns every one of the eight-plus per-frame full walks from O(C) into
O(N), and removes the "peaked at 200 objects, pays 255 forever" property entirely.
*OURS.* `engine/core/ListSet.hpp` is entirely ours — no exe-overlaid struct grows, because the
index lives DLL-side. `ADDRESS-MAP.md` is unaffected.
**Precondition, and it is not optional:** this is only sound if *every* creator and destroyer
of list items is C++. The codebase already flags this doubt in its own source —
`GameState.cpp:1792-1795`: "`CountAlive()` can only be used when we are SURE we've hooked and
replaced all functions creating and removing listset items." That audit must be done first
and written down. `LegoObject_Create` (`Object.h:664`) and `LegoObject_Remove`
(`Object.cpp:615`) are implemented, which is encouraging but not proof.
*Measurability:* **harness.** `ListSet.hpp` is header-only and depends only on `Errors.h` /
`Memory.h`; it can be instantiated over a synthetic 1036-byte POD and benchmarked headlessly
today, with no game and no exe. This is the one thing in this document that is fully
measurable right now.

**A2. Kill the two quadratic paths.**
- `LegoObject_UpdateSlipAndScare` (`Object.cpp:5313`): the inner walk exists to find units
  near one monster. Replace with a query against a coarse block-bucket index (objects binned
  by `blockIndex`, `Game.h:780`) so the search is O(neighbours) instead of O(C).
- `Weapon_Projectile_UpdatePath` (`Weapons.cpp:348`): same shape — a ray-vs-object search
  over the entire object list, run per projectile per frame. Same bucket index applies.
*Expected effect:* removes the only two paths whose cost grows as N², which are precisely the
paths the WaveDirector is designed to stress.
*OURS* (both). Bucket index is DLL-side; no overlaid struct changes.
*Measurability:* **harness**, if and only if the bucket index is written as a standalone
DLL-side component with its own tests. The *call sites* are not harnessable — `in situ only`.

**A3. Stop walking the object list for one-flag questions.** Merge the redundant walks:
- `Bubble_DrawAllObjInfos` — three walks (`Bubbles.cpp:365,380,423`) over the same filtered
  set, doing health bars, then images. Two of the three can be merged; the split exists for
  Draw-lock batching (`ObjInfo_TryBeginDraw`), so merge into one walk that fills two small
  work lists rather than three walks.
- `Lego_DrawAllLaserTrackerBoxes` + `Lego_DrawAllLaserTrackerNames` (`Game.cpp:1133,1142`) —
  two full walks to find objects with `LIVEOBJ4_LASERTRACKERMODE`, of which there is at most
  one. Track it in a DLL-side pointer, exactly as `Message_GetSelectedUnits` already does for
  the selection (`Game.cpp:1160-1161` shows the correct pattern).
- `LegoObject_HideAllCertainObjects` (`Object.cpp:5737`) is called twice per frame
  (`GameState.cpp:1124`, `:1317`); the second call is inside the radar-tracker branch and may
  be redundant with the first.
*Expected effect:* removes ~4 full list walks per frame in top-down view.
*OURS.* *Measurability:* `in situ only` for wall-clock; the **walk count itself is
statically verifiable** and should be asserted by the linter proposed in §6.

**A4. Amortise the WaveDirector's candidate scan.** `GatherCandidates`
(`WaveDirector.cpp:181-212`) does O(B · Bld) in one frame. Three changes, in order of value:
1. Replace the per-block linear scan over `bases` (`:204-206`) with a **precomputed
   base-proximity bitmask over blocks**, built once per wave in O(Bld · r²) where r =
   `waveMinDistanceFromBase` (default 8, `DeepCore.hpp:119`) — i.e. stamp a disc around each
   building instead of testing every block against every building. This alone removes the
   product.
2. **Spread the O(B) sweep across the telegraph window.** The director already has a
   `Telegraph` phase lasting `waveTelegraphSeconds` (default 6 s, `DeepCore.hpp:103`) during
   which it does nothing. Scan a slice of the map per frame during the *preceding* Waiting
   phase and the spike disappears entirely.
3. Hoist `candidates` (`:358`) and `bases` (`:188`) into `Waves::State` so their capacity is
   reused, matching what `pendingBlocks` already does correctly (`:56`).
Also: read `blockValue(level, bx, by)` once in `IsFairSpawnBlock` (`:137-138`) rather than
twice — the compiler probably CSEs it, but at `/Od` (§4) it does not, and the source should
not depend on that.
*Expected effect:* converts a periodic multi-hundred-thousand-operation frame into a bounded
per-frame slice. *OURS.*
*Measurability:* **harness** — `GatherCandidates` operates on a `Lego_Level` whose `blocks`
array is a plain `Lego_Block[]`; a synthetic level can be constructed headlessly once the
function is moved out of the anonymous namespace (`WaveDirector.cpp:83-278`) behind a
testable seam. That seam does not exist today.

**A5. Early-out `ElectricFence_UpdateAll` when there are no fences.**
`ElectricFence.cpp:250-268` performs two full object walks unconditionally. `efenceListSet`
exposes O(1) `CountAlive()` (`ListSet.hpp:663`). One guard removes two O(C) walks per frame
on every level without electric fences — which is most of them.
*Expected effect:* −2 full walks/frame on most levels, for a three-line change.
*OURS.* *Measurability:* `in situ only`; walk count statically verifiable.

**A6. Make `Erosion_Update` iterate occupancy, not capacity.** `Erosion.cpp:154` and `:218`
sweep 2000 and 1000 entries every frame to find, typically, a handful of set flags.
`erosionGlobs` is **exe-overlaid and frozen** (`Erosion.h:62`, `Erosion.cpp:21`) so the arrays
cannot be repacked — but a **DLL-side free-list / active-index** over the same arrays is
allowed and requires no layout change. `Erosion_GetFreeActiveIndex` and
`Erosion_AddOrRemoveLavaBlock` are ours, so all mutations are interceptable.
*Expected effect:* removes ~12 KB of pointless streaming per frame.
*OURS* (the index), *EXE MACRO adjacency* (the arrays are inside a frozen glob — read/write
only, never resize).
*Measurability:* `in situ only`.

**A7. Gate `Water_UpdateNotHotBlocks` on change.** `Water.cpp:905,909` runs every frame and
issues two `Level_Block_SetNotHot` calls per drain regardless of whether `elapsedUp_c` /
`elapsedDown_10` crossed a block boundary. Cache the last `(bxW, byW)` per drain and skip the
exe call when unchanged. Note `Level_Block_SetNotHot` is an **EXE MACRO** (`Game.h:1536`) —
we cannot see what it costs, which is an argument for calling it less, not more.
*OURS* (the gate). *Measurability:* `in situ only`, **and the benefit is UNDETERMINED** because
the exe side may already short-circuit. Land it for correctness of intent, not for a claimed
number.

**A8. Hoist the `Water` accessor loop conditions.** `Water.cpp:812`, `:836`, `:891`, `:925`,
`:926`, `:976`, `:977` all call an accessor in the loop condition. Hoist into locals; re-fetch
`Pool_Mesh(i)` once outside the vertex loop (`:894`, `:900`).
*Expected effect:* small under `/O2`, **material under the current `/Od` Release** (§4).
*OURS.* *Measurability:* `in situ only`.

**A9. Remove the debug `Error_InfoF` from `Water_Update`** (`Water.cpp:823`, statics at
`:809-810`). It is a bring-up leftover on the frame path that formats through a shared
non-reentrant `static char res[1024]` (`Errors.cpp:155`).
*OURS.* *Measurability:* not a perf claim; a hygiene fix.

**A10. Resolve `SND_AirBeat` once.** `GameState.cpp:1715,1725,1729` calls `SFX_GetType` with a
string literal up to three times per frame; each call hashes the literal and linear-scans
`hashNameList` (`SFX.cpp:112-123`). Resolve into a file-scope `SFX_ID` at level load and
reuse. *OURS.* *Measurability:* `in situ only`, but the removed work is exactly countable.

**A11. `OpenLRR_UpdateWobblyWorld` is O(B) per frame** (`OpenLRR.cpp:940-952`). It is a
default-off novelty and the cost is inherent to what it does. Leave it — but **document at
the call site (`OpenLRR.cpp:987`) that enabling it costs a full map sweep every frame**, so
nobody mistakes it for free.

### Tier B — allocation

**B1. Hoist the RadarMap rect buffer out of the frame.** (`RadarMap.cpp:379`, `:470`) Replace
the per-frame `Mem_Alloc`/`Mem_Free` with a file-scope `std::vector<Gods98::Draw_Rect>` grown
on demand and never shrunk. `_radarMapCurrMaxDrawRects` (`:374`) already exists to track the
capacity; the `blockCount >= rectCount` guard at `:386` and the one at `:637` stay as they
are. The original comment at `:380` ("Allocate rectList because it takes up way too much
stack space") is right about the stack and wrong about the remedy.
*Expected effect:* removes a ≥42 KB malloc/free pair per frame from a heap shared with 1999
code. *OURS.* *Measurability:* **statically verifiable** (allocation count), `in situ only`
for latency.

**B2. Swap, don't copy, the power-grid block lists.** `Game.cpp:3874`:
```cpp
// before
_unpoweredBlockList = _poweredBlockList;
_poweredBlockList.clear();
// after
_unpoweredBlockList.swap(_poweredBlockList);
_poweredBlockList.clear();
```
Semantically identical given that `_unpoweredBlockList` is always drained by
`Level_PowerGrid_UpdateUnpoweredBlockSurfaces` (`Game.cpp:3849-3862`) before the next
`UnpowerPoweredBlocks`; **verify that ordering invariant before landing** — it is enforced by
the flag dance at `Object.cpp:1676` / `:1692` and should be asserted, not assumed.
*Expected effect:* O(n) copy → O(1) pointer swap; removes the growth-path allocation.
*OURS.* *Measurability:* `in situ only`; the removed copy is exactly countable.

**B3. Drop the 1 KB zero-fill in `Font_VPrintF2`.** `engine/drawing/Fonts.cpp:222`: change
`char fmtLine[FONT_MAXSTRINGLEN] = { '\0' };` to `char fmtLine[FONT_MAXSTRINGLEN];`. The
function writes and null-terminates it explicitly (`:228-238`). **Check every path out of the
loop first** — the `Error_Fatal` at `:229` does not return, but "fatal is a log level, not a
guarantee" (`Errors.cpp:26`, `fatalVisible` defaults true), so control *can* fall through with
`fmtLine` partly written. It is terminated at `:238` on every path that reaches the
`vsprintf` at `:242`, so the change is safe — but that reasoning must be in the commit message.
*Expected effect:* removes a 1 KB `memset` from every HUD and debug string.
*OURS.* *Measurability:* **harness** — `Fonts.cpp` is not trivially isolatable, but a
standalone reimplementation of the format pre-pass can be benchmarked; honestly, the effect is
small and the fix is worth it mainly because it is free.

**B4. Reserve the WaveDirector vectors.** Covered by A4.3; listed separately because it is a
two-line change that can land independently.

### Tier C — layout

**C1. Split the shortcut update sweep from the shortcut metadata.** (`Shortcuts.hpp:325-371`,
`Shortcuts.cpp:251-255`) Keep `shortcutInfos` as the authoritative record; add a
`std::vector<Gods98::InputButtonBase*>` rebuilt on `Load`, and iterate *that* in `Update`.
114 scattered ~88-byte structs plus 114 `shared_ptr` derefs become one dense pointer array.
*Expected effect:* small in absolute terms, but it is a genuinely per-frame sweep.
*OURS.* *Measurability:* **harness** — `InputButton` (`engine/input/InputButton.hpp`) has no
game dependencies and can be benchmarked headlessly.

**C2. Flatten the small integer-keyed `std::map`s.** `_objInfoHealthBarsCache`
(`ObjInfo.cpp:34`, keyed by health-bar pixel width) and `_beamStyleByWeapon`
(`DeepCore.cpp:319`, keyed by weapon index) are both keyed by small dense integers and should
be `std::vector` indexed directly. The first is on the per-object HUD path (`ObjInfo.cpp:141`).
*OURS.* *Measurability:* `in situ only`.

**C3. Flatten `DeepCorePool::blocks` into a shared DLL-side arena.** (`Water.cpp:34-42`)
Keep the `unique_ptr<DeepCorePool>` — its pointer-stability rationale (`Water.cpp:48-53`) is
correct and must not be undone — but move `blocks` to a single `std::vector<Point2F>` with
per-pool `(offset, count)`, so the per-block access is two loads instead of four. Note this
interacts with `Pool_AppendBlocks` (`Water.cpp:198-211`) and `Pool_SortByBlockCount`
(`:214-228`); the sort currently relies on moving `unique_ptr`s, which still works.
*OURS.* *Measurability:* `in situ only`. **Do this last**, if at all — P and W are small and
the relocation is new, untested-in-situ code. Churn here has a poor risk/reward ratio.

**C4. Group the frequently-probed booleans at the front of `DeepCore::Settings`.**
(`DeepCore.hpp:56+`) Cosmetic; one cache line instead of two or three for the per-frame gate
reads. *OURS.* *Measurability:* `in situ only`. Lowest priority in this document.

**C5. Nothing else can be repacked.** `LegoObject`, `Erosion_Globs`, `Water_Globs`,
`Lego_Block`, `Draw_Rect`, `AITask` and the rest of `docs/ADDRESS-MAP.md`'s 113 regions are
frozen. Any proposal that begins "if we reorder the fields of `LegoObject`…" is wrong and
should be rejected on sight.

### Tier D — build flags

**D1. Set `<Optimization>MaxSpeed</Optimization>` in `Release|Win32`.** (`openlrr.vcxproj:125-134`)
This is the highest-leverage single line in the repository. See §4.2 for the reproduced
evidence and §4.5 for the full recommended block.
*Expected effect:* every DLL-side C++ line in the frame path gets an optimiser for the first
time — including the `ListSet` iterator, every `static` accessor in `Water.cpp`, and the
out-of-line `FilterSkipUpgradeParts` (`Object.cpp:72`) that `/GL` + `/LTCG` is *already
positioned* to inline and currently does not.
*Measurability:* **harness, and already demonstrated** — the §4.2 disassembly is the
measurement. In-game frame-time effect: `in situ only`, UNDETERMINED.
**Gate:** full `-t:Rebuild` of both configurations; the 44-warning contract must be
re-verified, and C4701/C4702-class warnings that only appear under `/O2` must be fixed in
code, not suppressed.

**D2. Add `/Ob2`, `/Ot`, `/GF`** alongside D1 (`InlineFunctionExpansion`, `FavorSizeOrSpeed`,
`StringPooling`). Same gate.

**D3. Delete the `x64` configurations.** (`openlrr.vcxproj:12-19,42-54,91-96,154-181`) They
declare `Application` rather than `DynamicLibrary`, omit the vendored d3drm paths and the
`_GODS98*` defines, and target C++14. They cannot build and their only function is to waste
somebody's afternoon.

**D4. Leave `/sdl` and `/GS` on.** Documented here so it is a decision rather than an
oversight. This DLL executes inside a process it does not own, next to 1999 code with known
`strcpy`-into-fixed-buffer defects (`docs/research/silent-failures.md`, F-02…F-12). The
measured cost of `/GS` on one buffer-bearing function was 26% of that function's object size;
that is a price worth paying here.

**D5. Do not set `/fp:fast`.** Recorded as an explicit non-recommendation. Float state is
shared with exe machine code we cannot inspect, `Maths.cpp` is one of the five files carrying
the project's inherited warnings, and the payoff is unquantifiable without running the game.

---

## 6. Method — how performance claims are made in this project

**Binding.** Any pull request, commit message, or document in this repository that asserts a
performance improvement must satisfy one of the four tiers below and must say which tier it
is using. There is no fifth tier, and "it should be faster" is not one of them.

### Tier 1 — Asymptotic (strongest available; always required)

A complexity claim derived by reading the source, with `file:line` for every loop and every
call that changes the bound. This is the only tier that can be checked by a reviewer with no
special equipment, so it is mandatory for *every* claim regardless of what else is offered.

Required form: *"`X` is O(f) because of the loop at `a.cpp:N` and the nested full enumeration
at `b.cpp:M`; after the change it is O(g) because …"*

### Tier 2 — Mechanical counts

Numbers extracted from the tree by a reproducible command, not by eye. §1 uses this: "36 of
159 callees in `Lego_MainLoop` are exe address macros" and "at least eight full O(C) object
walks per frame" are both counts, and both are re-derivable.

**Build this into CI.** `tools/addrlint/addrlint.py` already proves the pattern works — a
static linter that reconstructs a structural property and fails the build when it drifts.
The natural companion is a **`frameaudit`** tool that:

- walks call graphs rooted at `Lego_MainLoop` and `DeepCore::Waves::Update`,
- fails on any `Mem_Alloc` / `new` / `make_unique` / bare `push_back` / `std::string`
  construction reachable per frame without an explicit `// FRAME-ALLOC-OK:` annotation,
- counts `Enumerate*` call sites reachable per frame and fails when the count increases.

That converts §2 and fixes A3/A5 from prose into machine-enforced invariants, which is the
only way any of this survives contact with a future session.

### Tier 3 — Toolchain-level evidence

Assembly, object sizes, or linker output produced on the authoring machine and quoted
verbatim, with the exact command line. §4.2 is the worked example, and it is the reason the
build finding is stated as fact rather than as suspicion.

Reproduce it as follows (nothing here touches the project or its shared intermediate
directory; work in a scratch directory):

```
cl.exe  : <VS>/VC/Tools/MSVC/14.29.30133/bin/Hostx64/x86/cl.exe   (v142, matches the project)
INCLUDE : <MSVC>/include ; <WindowsKit>/Include/<ver>/{ucrt,shared,um}

cl  -c -nologo -GL      t.cpp u.cpp
link   -nologo -LTCG -out:a.exe -subsystem:console -nodefaultlib -entry:main t.obj u.obj s.obj
dumpbin -disasm -nologo a.exe

cl  -c -nologo -GL -O2  t.cpp u.cpp        # then relink and disassemble identically
```

(`s.cpp` supplies a stub `__security_check_cookie` so `-nodefaultlib` links; its presence is
itself the evidence that `/GS` is on by default.)

Note the shell trap on this machine: MSYS/Git-Bash rewrites `/GL` into a Windows path.
**Use `-GL`, `-O2`, `-c` with leading dashes**, never slashes.

### Tier 4 — Headless harness

A benchmark that compiles and runs without the game. Today, **exactly one component
qualifies unconditionally**: `engine/core/ListSet.hpp` is header-only, depends only on
`Errors.h` and `Memory.h`, and can be instantiated over a synthetic 1036-byte POD to
reproduce the O(capacity)-vs-O(alive) behaviour in §1.1 and to validate fix A1. Anything
proposing a change to `ListSet` **must** ship such a harness.

`engine/input/InputButton.hpp` (fix C1) and a extracted `GatherCandidates` (fix A4) are
harnessable *after* a stated seam is added. Everything else — anything that touches
`legoGlobs`, a `Container*`, a `Viewport*`, or any address macro — is not harnessable and
must not pretend to be.

### What may never be claimed

- **No wall-clock or frame-rate number may be stated for the running game.** Not as an
  estimate, not as a range, not as "roughly". We have no installation. Every such claim in
  this document is marked UNDETERMINED and future ones must be too.
- **No claim about exe-side cost.** `AITask_UpdateAll` (`AITask.h:245`), `Lego_HandleWorld`,
  `Map3D_Update`, `Panel_Draw`, `Level_Block_SetNotHot` and the other 31 address macros in the
  frame path are opaque. "This makes `Level_Block_SetNotHot` cheaper" is unknowable; "this
  calls `Level_Block_SetNotHot` fewer times" is a Tier 2 fact. Prefer the second phrasing
  always.
- **No claim about D3DRM.** `Viewport_Render`, `Container_Mesh_Get/SetVertices` and the
  `Draw_*` family dominate the frame and are not ours.
- **No claim that a `/O2` change is safe until a full `-t:Rebuild` has confirmed 0 errors and
  exactly 44 warnings** in both configurations.

### The honest summary sentence for this project

> "Algorithmically better, unmeasured in situ."

Use it. It is not an apology — it is the accurate description of what a compile-verified
project can establish, and it is worth more than a fabricated percentage.

---

## 7. Decision

**Land D1 first.** Setting `<Optimization>MaxSpeed</Optimization>` in `Release|Win32` is one
line, it is backed by reproduced disassembly rather than inference, and it multiplies the
value of every other fix in this document. Everything else in Tier A is currently being
compiled by a non-optimising compiler, which means the measured-on-paper wins are being
thrown away at the last step. Land it behind a full rebuild and a re-verified 44-warning
tally.

**Then A5, A10, A9, B2 and B1** — five small, local, individually reviewable changes that
remove two full object walks per frame, three per-frame string hashes, a debug log, an O(n)
vector copy and a 42 KB per-frame malloc. None of them requires a design decision.

**Then A1**, gated on the creator/destroyer audit that `GameState.cpp:1792-1795` already asks
for, with the mandatory `ListSet` harness. This is the root fix: it is the only change that
makes the eight-plus per-frame walks proportional to what is alive rather than to what was
once alive, and it is the only item here that is fully measurable today.

**Then A2 and A4**, which are the two that actually matter for the direction this project is
going. The WaveDirector exists to put more monsters on the map; `LegoObject_UpdateSlipAndScare`
is quadratic in exactly the monster count; and `GatherCandidates` spikes O(B · Bld) in a
single frame every time a wave is scheduled. Raising `waveMaxAlive` without landing A2 first
is spending frame time on a curve that bends the wrong way.

**Defer C3 and C4 indefinitely.** The water relocation is new code that has never been run;
re-laying it out now trades a real correctness risk for an unmeasurable gain.

**Reject on sight** any proposal to reorder or grow a type listed in §3.1. That door is
closed, `addrlint` enforces it, and `docs/ADDRESS-MAP.md` records why.
