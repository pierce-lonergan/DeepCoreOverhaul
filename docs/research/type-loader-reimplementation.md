# Reimplementing `Lego_LoadRockMonsterTypes` (@`0x0042d030`)

**Question asked:** exactly what does it take to write this function in C++, and does doing so
actually unlock anything?

**Answer in one line:** the reimplementation is cheap, safe, and hook-free — and it unlocks
**nothing** on its own, because the 15-ID ceiling is enforced by six exe-overlaid tables and by
eight *exe-side* consumers that the loader never touches. The correct next increment is not the
loader; it is the guard rail that turns today's silent memory corruption into a loud warning.

Everything below is compile-time / source evidence only. **We cannot run the game.** No claim in
this document has been play-tested, and none is presented as if it had been.

---

## 0. Evidence base

| Kind | Source |
| --- | --- |
| Call graph | `grep` over `src/openlrr`, quoted inline with `file:line` |
| Struct layout | the `assert_sizeof` static-asserts, quoted inline |
| Address adjacency | `tools/addrlint/addrlint.py` → `docs/address-map.json` (113 sized regions, **0 overlaps**) |
| Allocation contract | the *free* path, `GameState.cpp:1915-1922` — the one place the exe's allocation shape is visible in C++ |

Terminology used throughout:

- **OURS** — a real C++ body exists in this tree.
- **EXE** — still an address macro (`#define X ((sig)0xADDR)`); the 1999 machine code runs.

---

## 1. Call sites of the five loaders — and whether a hook is needed

### 1.1 The complete call graph

`grep -rn "Lego_Load(Vehicle|MiniFigure|RockMonster|Building|Upgrade)Types"` over the whole tree
returns exactly **three** kinds of hit: the macro definitions, the pre-staged commented prototypes,
and **one** call site.

| Loader | Address | Declaration | Pre-staged body slot | Call site |
| --- | --- | --- | --- | --- |
| `Lego_LoadVehicleTypes` | `0x0042ccd0` | `Game.h:1466` (macro), `1467` (commented decl) | `Game.cpp:3257` | `GameState.cpp:513` |
| `Lego_LoadMiniFigureTypes` | `0x0042ce80` | `Game.h:1470` / `1471` | `Game.cpp:3260` | `GameState.cpp:513` |
| **`Lego_LoadRockMonsterTypes`** | **`0x0042d030`** | **`Game.h:1474` / `1475`** | **`Game.cpp:3263`** | **`GameState.cpp:513`** |
| `Lego_LoadBuildingTypes` | `0x0042d1e0` | `Game.h:1478` / `1479` | `Game.cpp:3266` | `GameState.cpp:514` |
| `Lego_LoadUpgradeTypes` | `0x0042d390` | `Game.h:1482` / `1483` | `Game.cpp:3269` | `GameState.cpp:512` |

The single call site is one boolean chain:

```cpp
// GameState.cpp:512-515
if (Lego_LoadLighting() && Lego_LoadGraphicsSettings() && Lego_LoadUpgradeTypes() &&
    Lego_LoadVehicleTypes() && Lego_LoadMiniFigureTypes() && Lego_LoadRockMonsterTypes() &&
    Lego_LoadBuildingTypes())
{
```

The two `ObjectNames` loaders that finish the job are called 43 lines later, *after* the type
loaders, in the same function:

```cpp
// GameState.cpp:556-557
Lego_LoadObjectNames(legoConfig);
Lego_LoadObjectTheNames(legoConfig);
```

Ordering matters and is load-bearing: `Lego_LoadObjectNames` must resolve `[ObjectNames]` keys to
IDs, which only exist once the type loaders have populated `legoGlobs.*Name[]`.

### 1.2 Is the caller already C++? Yes, and it is not even hooked.

The enclosing function is `LegoRR::Lego_Initialise` — **OURS**, `GameState.cpp:95`
(`// <LegoRR.exe @0041fa80>` at `GameState.cpp:94`).

More importantly, `grep -n "0x0041fa80" src/openlrr/interop.cpp` returns **nothing**.
`Lego_Initialise` is *not* installed with `hook_write_jmpret` at all. It reaches the exe through a
function pointer:

```cpp
// Game.cpp:77-79   (LegoRR::Lego_Gods_Go_Setup)
mainState->Initialise = Lego_Initialise;
mainState->MainLoop   = Lego_MainLoop;
mainState->Shutdown   = Lego_Shutdown_Full;
```

That `Main_State` is what `Gods98::Main_SetState` receives (`Game.cpp:98`). So the whole init path
is DLL-resident already.

### 1.3 Verdict on the hook question

**No `interop.cpp` entry is required. The binding is resolved by the C++ compiler.**

Today `Lego_LoadRockMonsterTypes` at `GameState.cpp:513` expands to
`((bool32(__cdecl*)(void))0x0042d030)()` — a call through a literal address. The moment you comment
`Game.h:1474` and uncomment `Game.h:1475`, the same source text becomes an ordinary call to
`LegoRR::Lego_LoadRockMonsterTypes`, and the linker points it at your body in `Game.cpp:3263`.
Nothing else in the tree references the symbol, so nothing else changes.

This is not a novel pattern — it is exactly the shape of `Weapon_Initialise`, whose hook is
deliberately commented out because its only caller is C++:

```cpp
// interop.cpp:4345
//result &= hook_write_jmpret(0x0046ee40, LegoRR::Weapon_Initialise);
```
(its caller: `GameState.cpp:571`.) The same treatment is applied to `Weapon_Update`
(`interop.cpp:4356`, caller `GameState.cpp:1078`).

**The residual risk, stated honestly.** A hook would additionally guarantee that any
*un-decompiled exe* function calling `0x0042d030` also lands in our code. We cannot enumerate exe
callers from this tree. The inference that `Lego_Initialise` is the only one is strong — the
function is a one-shot startup loader, the exe's own `Lego_Initialise` has been replaced wholesale,
and the four sibling loaders share the identical single-call-site pattern — but it is an
**inference, not a verification**. If you do reimplement, adding the hook costs one line and closes
that hole:

```cpp
// interop_hook_LegoRR_Game(), interop.cpp:2696
// used by: Lego_Initialise  (C++; hook is belt-and-braces for un-decompiled exe callers)
result &= hook_write_jmpret(0x0042d030, LegoRR::Lego_LoadRockMonsterTypes);
```
The Game module carries **no** blanket "merged functions" warning (those exist only at
`interop.cpp:1766` for Building and `1920` for Creature), so `hook_write_jmpret` is legitimate here
— subject to the usual check that `0x0042d030` is not itself a merged address, which we cannot
confirm from source.

---

## 2. What must the loader actually DO?

### 2.1 The destinations — all five are heap pointers

```
Game.h:533   /*11c,4*/  CreatureModel* rockMonsterData;        // (cfg: RockMonsterTypes)
Game.h:538   /*130,4*/  char**         rockMonsterName;        // (cfg: RockMonsterTypes)
Game.h:545   /*19c,4*/  char**         langRockMonster_name;   // (cfg: ObjectNames)
Game.h:550   /*1b0,4*/  char**         langRockMonster_theName;// (cfg: ObjectTheNames)
Game.h:555   /*1c4,4*/  uint32         rockMonsterCount;       // (cfg: RockMonsterTypes)
```

**This is the single most important structural fact in this document.** Every destination is a
4-byte pointer or a 4-byte count *inside* `Lego_Globs`. `Lego_Globs` is overlaid at `0x005570c0`
(`Game.cpp:152`) with `assert_sizeof(Lego_Globs, 0xf00)` (`Game.h:672`), and `addrlint` reports its
neighbour `frontGlobs` starts at exactly `0x00557fc0` — **gap = 0**. So there is zero slack, and
also zero need for slack: writing these fields changes no layout whatsoever. Relocation of the
monster roster to heap of any size is free.

### 2.2 The allocation contract, derived from the free path

The exe's allocation shape is fully observable, because the *teardown* is already C++:

```cpp
// GameState.cpp:1915-1922   (LegoRR::Lego_Shutdown_Full)
// Free monster object data:
for (uint32 i = 0; i < legoGlobs.rockMonsterCount; i++) {
    Creature_Remove(&legoGlobs.rockMonsterData[i]);
    Gods98::Mem_Free(legoGlobs.rockMonsterName[i]);
}
Gods98::Mem_Free(legoGlobs.rockMonsterData);
Gods98::Mem_Free(legoGlobs.rockMonsterName);
Gods98::Mem_Free(legoGlobs.langRockMonster_name);
```

Read off, unambiguously:

1. `rockMonsterData` is **one** `Mem_Alloc` block of `count * sizeof(CreatureModel)`, freed as one
   block, with each element `Creature_Remove`d in place. `assert_sizeof(CreatureModel, 0x74)`
   (`Creature.h:80`).
2. `rockMonsterName` is **one** block of `count * sizeof(char*)`, and each element is
   **individually owned** — so each name is its own allocation. `Gods98::Config_GetStringValue`
   returns `Util_StrCpy(conf->dataString)` (`Config.cpp:367-378`), i.e. a `Mem_Alloc`'d duplicate.
   That is the matching allocator.
3. `langRockMonster_name` is **one** block, elements **not** freed → the elements are non-owning
   `Config_GetTempStringValue` pointers into the still-live `Gods98::Config`.
4. `langRockMonster_theName` is **never freed** — the pre-existing `// FIXME: There seems to be
   memory cleanup for lang*_name, but not lang*_theName...` at `GameState.cpp:1895`. A
   reimplementation must preserve the allocation shape, not the leak.

### 2.3 The config schema

`RockMonsterTypes` is an array block read with the standard Gods98 array walk. The exact idiom is
already used four times in code we own, e.g. `Stats_Initialise`:

```cpp
// Stats.cpp:129-134
const char* arrayID = Config_ID(gameName, "Stats");
for (prop = Config_FindArray(config, arrayID); prop != nullptr; prop = Config_GetNextItem(prop)) {
```

with `Config_ID(s, ...)` → `Gods98::Config_BuildStringID` (`Config.h:336`) and the game-scoped
shorthand `#define Lego_ID(...)` at `Game.h:793`.

Each item then yields:
- **key** = `Gods98::Config_GetItemName(prop)` (`Config.h:215`) → the object name, e.g.
  `"RockMonster"`, `"IceMonster"`, `"TinyRM"`, `"Slug"` (the canonical strings are macro'd at
  `GameCommon.h:137-150`).
- **value** = `Gods98::Config_GetDataString(prop)` (`Config.h:218`) → the model directory passed
  straight to `Creature_Load`.

So the schema is `Name  ModelPath`, one per line, inside `[RockMonsterTypes]` under the game
section. **The array-of-pairs shape is verified** (it is what `Config_FindArray`/`GetItemName`/
`GetDataString` provide and what every sibling block uses); the **specific token layout of the
value string is inferred**, not read — we have no `Lego.cfg` in this tree (`data/` contains only
`Settings/DeepCore.cfg` and `Settings/Shortcuts.cfg`).

### 2.4 The per-entry work

`Creature_Load` is **OURS** and hooked, and its signature tells you the whole call:

```cpp
// Creature.cpp:112  ( // <LegoRR.exe @004068c0> )
bool32 __cdecl LegoRR::Creature_Load(OUT CreatureModel* creature, LegoObject_ID objID,
                                     Gods98::Container* root, const char* filename,
                                     const char* gameName);
```
`interop.cpp:1924` documents its callers as exactly `// used by: Lego_LoadMiniFigureTypes,
Lego_LoadRockMonsterTypes`. That is direct confirmation that the loader's per-entry body is a
`Creature_Load` call and that `objID` is the **positional index in the cfg block**.

`Creature_Load` itself `memset`s the model (`Creature.cpp:114`), loads the `ACT` container
(`:121`), opens `<path>\<leafname>.ae` (`:130`), reads the null names and mesh LODs (`:142-159`),
and sets `creature->flags = CREATURE_FLAG_SOURCE; creature->objID = objID;` (`:161-162`).

The template model must then be hidden — the sibling building loader proves the pattern:
`interop.cpp:1784` reads `// used by: Lego_LoadBuildingTypes, LegoObject_Hide` above
`hook_write_jmpret(0x004085a0, LegoRR::Building_Hide)`. The creature equivalent is
`Creature_Hide` (`Creature.cpp:56`), which is **OURS but flagged `// Merged function: Object_Hide`**
(`Creature.cpp:54`) — call it directly from C++, **never** hook it (`interop.cpp:1920`).

### 2.5 Reference implementation sketch

```cpp
// Game.cpp:3263  — replaces the commented prototype
// <LegoRR.exe @0042d030>
bool32 __cdecl LegoRR::Lego_LoadRockMonsterTypes(void)
{
    const Gods98::Config* arrayFirst =
        Gods98::Config_FindArray(legoGlobs.config, Lego_ID("RockMonsterTypes"));
    if (arrayFirst == nullptr) {
        Error_Warn(true, "No RockMonsterTypes block in config");
        return false;
    }

    // Pass 1: count. (Same two-pass shape as Weapon_Initialise, Weapons.cpp:50-58.)
    uint32 count = 0;
    for (const Gods98::Config* item = arrayFirst; item != nullptr;
         item = Gods98::Config_GetNextItem(item)) {
        count++;
    }

    /// DEEPCORE: hard ceiling. See docs/research/type-loader-reimplementation.md §4.
    /// IDs >= LegoObject_ID_Count alias onto LegoObject_Building ID 0 in six
    /// exe-overlaid [20][15] tables. Refuse loudly instead of corrupting silently.
    if (count > (uint32)LegoObject_ID_Count) {
        Error_WarnF(true, "RockMonsterTypes declares %u entries; the engine ceiling is %i. "
                          "Extra entries are ignored.", count, (sint32)LegoObject_ID_Count);
        count = (uint32)LegoObject_ID_Count;
    }

    // Pass 2: allocate exactly the block shape Lego_Shutdown_Full frees
    // (GameState.cpp:1915-1922).
    legoGlobs.rockMonsterCount = count;
    legoGlobs.rockMonsterData  = (CreatureModel*)Gods98::Mem_Alloc(count * sizeof(CreatureModel));
    legoGlobs.rockMonsterName  = (char**)        Gods98::Mem_Alloc(count * sizeof(char*));
    if (legoGlobs.rockMonsterData == nullptr || legoGlobs.rockMonsterName == nullptr) {
        return false;
    }
    std::memset(legoGlobs.rockMonsterData, 0, count * sizeof(CreatureModel));
    std::memset(legoGlobs.rockMonsterName, 0, count * sizeof(char*));

    uint32 i = 0;
    for (const Gods98::Config* item = arrayFirst; item != nullptr && i < count;
         item = Gods98::Config_GetNextItem(item), i++) {

        // Owned copy: matches the per-element Mem_Free at GameState.cpp:1918.
        legoGlobs.rockMonsterName[i] = Gods98::Util_StrCpy(Gods98::Config_GetItemName(item));

        const char* modelPath = Gods98::Config_GetDataString(item);

        /// CUSTOM: the exe fails silently here. Say what broke, and which entry.
        if (!Creature_Load(&legoGlobs.rockMonsterData[i], (LegoObject_ID)i,
                           legoGlobs.rootCont, modelPath, legoGlobs.gameName)) {
            Error_WarnF(true, "RockMonsterTypes: failed to load \"%s\" from \"%s\"",
                        legoGlobs.rockMonsterName[i], modelPath);
            continue; // leave a zeroed CreatureModel; Creature_Remove tolerates it
        }
        Creature_Hide(&legoGlobs.rockMonsterData[i], true); // merged Object_Hide — do NOT hook
    }
    return true;
}
```

Notes on the sketch:
- `Error_Warn` / `Error_WarnF` are the in-tree macros (`Errors.h:110-123`), not new machinery.
- `Gods98::Util_StrCpy` is what `Config_GetStringValue` uses internally (`Config.cpp:374`); using
  `Config_GetStringValue` on the *item name* is not possible, since that API keys by string-ID, so
  the duplicate must be made explicitly.
- **Unverified in the sketch:** whether the original also ticks the loading bar
  (`Loader_display_loading_bar`, `Loader.cpp:137`, called at `GameState.cpp:361` and `:859`), and
  whether it does anything beyond load+hide. Both are cosmetic-to-minor, but they are genuine
  unknowns.

### 2.6 What `Lego_LoadObjectNames` must do (still EXE)

`Lego_LoadObjectNames` (`Game.h:1486`, `0x0042d530`) allocates `langRockMonster_name` as a
`count`-sized block and fills index `objID` with the `[ObjectNames]` value for that object's name —
which is why it runs *after* the type loaders and why the reverse lookup
(`legoGlobs.langRockMonster_name[objID]`, `Debug.cpp:113`) is a plain index. If you reimplement the
type loader but not this one, nothing breaks: the exe version reads `rockMonsterName`/
`rockMonsterCount` from `legoGlobs`, which your C++ has just written in the same shape.

---

## 3. Do `Lego_GetObjectByName` and `Lego_GetObjectTypeIDCount` need reimplementing?

### 3.1 `Lego_GetObjectByName` — `0x0042e7e0`, **EXE** (`Game.h:1511`)

```cpp
bool32 __cdecl Lego_GetObjectByName(const char* objName, OUT LegoObject_Type* objType,
                                    OUT LegoObject_ID* objID, OPTIONAL OUT ObjectModel** objModel);
```

**What it reads (inferred, high confidence, not verified):** the only storage in the process that
maps a name to an ID is `legoGlobs.{vehicle,miniFigure,rockMonster,building,upgrade}Name[]`
(`Game.h:536-540`), bounded by the matching `*Count` (`Game.h:553-557`); the only storage for the
`objModel` out-param is `legoGlobs.*Data[]` (`Game.h:531-535`). So it is a per-type
`_stricmp` scan over those heap arrays. It cannot be anything else, because nothing else exists.

**Consequence: it does NOT need reimplementing for new IDs below the ceiling.** Because it is
count-driven over heap arrays, it already resolves any name the loader put there. That is why the
free RockMonster slots are reachable today with zero new code.

**Consequence: reimplementing it is actively risky.** It is **not hooked** (`grep` over
`interop.cpp` finds no `0x0042e7e0`), and it has many *exe-side* callers that we can name because
their own entry points are still macros:

| Exe caller | Status |
| --- | --- |
| `Lego_LoadObjectNames` `0x0042d530` | EXE — `Game.h:1486` |
| `Lego_LoadObjectTheNames` `0x0042d950` | EXE — `Game.h:1490` |
| `Lego_LoadOLObjectList` `0x0042df50` | EXE — `Game.h:1507` |
| `LegoObject_LoadObjTtsSFX` `0x0044af80` | EXE — `Object.h:1628` |
| `Dependencies_Initialise` `0x0040aaa0` | EXE — `Dependencies.h:137` |
| `HelpWindow_Initialise` `0x004180e0` | EXE — `HelpWindow.h:150` |
| `ObjInfo_Initialise` `0x004597f0` | EXE — `ObjInfo.h:110` |

If you implement it in C++ **without** hooking, you create two divergent resolvers: DLL callers
(`Stats.cpp:138`, `Weapons.cpp:155`, `Objective.cpp:550`, `NERPsFunctions.cpp:476-826`,
`DeepCore.cpp:93,173`, `Game.cpp:2205`, `Object.cpp:3719,5472`) use yours; the seven above keep
using the exe's. That is a silent, hard-to-diagnose split. **If reimplemented, it must be hooked.**

### 3.2 `Lego_GetObjectTypeIDCount` — `0x0042ee70`, **EXE** (`Game.h:1519`)

Consumers in C++ today:

```cpp
// Object.cpp:252-253   (LegoObject_GetObjectsBuilt)
/// FIX APPLY: Don't enumerate through object IDs that don't exist!
const uint32 objIDCount = Lego_GetObjectTypeIDCount(objType); // LegoObject_ID_Count;
```
```cpp
// Encyclopedia.cpp:66, 81, 96, 111  (Encyclopedia_Initialise) — sizes four Mem_Alloc'd file tables
count = Lego_GetObjectTypeIDCount(LegoObject_RockMonster);
encyclopediaGlobs.rockmonsterFiles = (Gods98::File**)Gods98::Mem_Alloc(count * sizeof(Gods98::File*));
```

That commented-out `// LegoObject_ID_Count;` at `Object.cpp:253` is the tell: the OpenLRR authors
already replaced a hardcoded `15` with this call, i.e. **the C++ side is already written to be
count-driven rather than assuming 15.** Good news for the loader; irrelevant to the ceiling.

**Reimplementation difficulty:** trivial for the five model types (return the matching
`legoGlobs.*Count`). **Unknown for the other fifteen** `LegoObject_Type` values
(`GameCommon.h:1066-1093`) — `LegoObject_Ore` plainly returns 2
(`LegoObject_ID_Ore_Count`, `GameCommon.h:1139`), and the rest presumably 1, but that is a guess.
Guessing wrong changes an allocation size in `Encyclopedia_Initialise` and a loop bound in
`LegoObject_GetObjectsBuilt`. Same hook caveat as §3.1.

**Verdict for both:** neither is required to reimplement the loader, and neither is required for
IDs below the ceiling. Reimplement them only as part of a deliberate, hooked, whole-family port.

---

## 4. THE HARD QUESTION — would a 16th RockMonster ID work end-to-end?

**No. Definitively no. And the failure mode is worse than an out-of-bounds write.**

### 4.1 The constant, and everything sized by it

```cpp
// GameCommon.h:1134-1147
enum LegoObject_ID : sint32 {
    LegoObject_ID_Ore          = 0,
    LegoObject_ID_ProcessedOre = 1,
    LegoObject_ID_Ore_Count,
    LegoObject_ID_Pilot = 0,
    LegoObject_ID_Count = 15,
    LegoObject_ID_Invalid = 20,
};
```

`grep -rn LegoObject_ID_Count` finds it structurally load-bearing in **seven** structs:

| Struct | Field | Decl | Storage | Δ if 15→16 | Address | Slack to next declared symbol |
| --- | --- | --- | --- | --- | --- | --- |
| `Stats_Globs` | `objectLevels[20][15]` | `Stats.h:228` | **fixed** `0x00503bd8` | **+80** | `Stats.cpp:41` | **0** (`g_Teleporter_BOOL_00504188`); 8 to `textGlobs` @`0x00504190` → **overruns 72 bytes into `Text_Globs`** |
| `Object_Globs` | `objectTtSFX[20][15]`, `objectTotalLevels[20][15][16]`, `objectPrevLevels[20][15][16]` | `Object.h:454,463,464` | **fixed** `0x004df790` | **+2640** | `Object.cpp:56` | **4** (`lightGlobs`) |
| `AITask_Globs` | `requestObjCounts[20][15][16]` | `AITask.h:147` | **fixed** `0x004b41c8` | **+1280** | `AITask.cpp:17` | 2484 to next *declared* symbol — but the exe's own undeclared globals live in that gap; not free space |
| `Dependencies_Globs` | `table[20][15]` of `DependencyData` (`0xc4`) | `Dependencies.h:91` | **fixed** `0x004b9bc8` | **+3920** | `Dependencies.cpp:14` | **4** (`effectGlobs`) |
| `Interface_Globs` | `objectBools[20][15]` | `Interface.h:143` | **fixed** `0x004ddd58` | **+80** | `Interface.cpp:45` | **4** (`s_ShowBlockToolTip_MousePos`) |
| `HelpWindow_Globs` | `VehicleInfos/MiniFigureInfos/BuildingInfos[15][16]` | `HelpWindow.h:91-93` | **fixed** `0x004dc8e8` | **+192** | `HelpWindow.cpp:14` | **4** (`infoGlobs`) |
| `WeaponStats` | `objectCoefs[20][15][16]` | `Weapons.h:83` | **heap** | +1280 | alloc `Weapons.cpp:56-58` | n/a |

Six of seven are exe-overlaid with 0–4 bytes of slack. **Raising the constant is out of the
question** — it is the exact edit the project rule forbids ("if you find yourself editing a number
inside `assert_sizeof`, stop"). `addrlint` reports 0 overlaps today; this change would create six.

Note also `HelpWindow_Globs` has **no** RockMonster array at all — only Vehicle/MiniFigure/Building
(`HelpWindow.h:91-93`). It is collateral damage of the shared constant, not a RockMonster consumer.

### 4.2 The subtler failure: ID 15 does not run off the end — it *aliases Building ID 0*

Suppose you do **not** touch the constant (correct), and simply let a 16th monster exist with
`objID == 15`. In a `[20][15]` table, the linear index of `[3][15]` is `3*15 + 15 = 60`, which is
`[4][0]`. In a `[20][15][16]` table, `[3][15][L]` is `3*240 + 15*16 + L = 960 + L`, which is
`[4][0][L]`.

`LegoObject_Building == 4` (`GameCommon.h:1075`). **Every RockMonster ID-15 access silently reads
and writes the row belonging to Building ID 0** — the first entry of the `BuildingTypes` block,
which in stock data is the Tool Store. No crash, no warning, no `assert_sizeof` trip. Just a
progressively wrong Tool Store.

### 4.3 The one true out-of-bounds write is in code we own

```cpp
// Stats.cpp:143-149   (Stats_Initialise)
if (statsGlobs.objectStats[type] == nullptr) {
    /// SANITY: Allocate size of max IDs for poorly written object type enumeration loops.
    uint32 subtypeArraySize = LegoObject_ID_Count * sizeof(ObjectStats*);
    statsGlobs.objectStats[type] = (ObjectStats**)Gods98::Mem_Alloc(subtypeArraySize);
    std::memset(statsGlobs.objectStats[type], 0, subtypeArraySize);
}
...
// Stats.cpp:163-165
statsGlobs.objectLevels[type][id] = levels;
statsGlobs.objectStats[type][id] = (ObjectStats*)Gods98::Mem_Alloc(levelsArraySize);
std::memset(statsGlobs.objectStats[type][id], 0, levelsArraySize);
```

With `id == 15`, line 164 writes 4 bytes past a 60-byte heap block — a genuine heap overrun — and
line 163 writes the Tool Store's level count. `Stats_Initialise` is **OURS** (`Stats.cpp:124`,
hooked `interop.cpp:4096`), which means this is also the one place we can *fix* cheaply. See §7.

And this path is unavoidable: an object with no `[Stats]` entry has `objectStats[type][id] == nullptr`,
so every `Stats_Get*` accessor and `StatsObject_SetObjectLevel` would dereference null. A 16th
monster **must** go through `Stats_Initialise`, and going through it is what corrupts.

### 4.4 Consumer enumeration — OURS vs EXE

| Consumer | Indexes with a RockMonster objID? | Status | Effect at ID 15 |
| --- | --- | --- | --- |
| `Lego_LoadRockMonsterTypes` `0x0042d030` | writes `rockMonsterData/Name/Count` | **EXE** | fine — heap, count-sized |
| `Lego_LoadObjectNames` `0x0042d530` | `langRockMonster_name[id]` | **EXE** | fine — heap, count-sized |
| `Lego_GetObjectByName` `0x0042e7e0` | returns the id | **EXE** | fine — but it is what *feeds* every row below |
| `Lego_GetObjectTypeIDCount` `0x0042ee70` | returns `rockMonsterCount` | **EXE** | fine |
| `Lego_Shutdown_Full` | `rockMonsterData[i]`, `rockMonsterName[i]` | **OURS** `GameState.cpp:1916-1922` | fine |
| **`Stats_Initialise`** | `objectStats[3][15]`, `objectLevels[3][15]` | **OURS** `Stats.cpp:143-165` | **heap overrun + Building[0] clobber** |
| `Stats_Get*` / `StatsObject_SetObjectLevel` | `objectStats[type][id][lvl]` | **OURS** (`Stats.h` has **0** live macros) | reads the clobbered row |
| `LegoObject_TryGenerateRMonster` `0x0043b1f0` | takes `objModel`+`objType`+`objID` | **EXE** `Object.h:974` | passes the id onward to `LegoObject_Create` |
| `LegoObject_TryGenerateRMonsterAtRandomBlock` `0x0043b160` | — | **EXE** `Object.h:969` | as above |
| `LegoObject_Create` RockMonster branch | `Creature_Clone` from `objModel` | **OURS** `Object.cpp:947-960` | fine (pointer, not index) |
| level-object counters | `objectTotalLevels[t][id][lvl]`, `objectPrevLevels[...]` | **OURS** `Object.cpp:378-406,453,1146`; `Stats.cpp:975-976` | **aliases Building[0]** → Tool Store "objects built" counts corrupt, which drives objectives and `Dependencies` |
| `LegoObject_LoadObjTtsSFX` `0x0044af80` → `objectTtSFX[20][15]` | yes | **EXE** `Object.h:1628` | aliases Building[0]; **avoidable only by omitting the monster from `[ObjTtSFXs]`** |
| `Dependencies_Initialise` `0x0040aaa0` → `table[20][15]` | yes | **EXE** `Dependencies.h:137` (module **0/11** implemented) | aliases Building[0]; avoidable by omission |
| `Interface_*` → `objectBools[20][15]` | yes | **EXE** `Interface.h:143`, module **3/99** | aliases Building[0]; RockMonsters have no build icons, so probably never reached — **not verified** |
| `AITask_Callback_UpdateObject` `0x004041b0` → `aiGlobs.requestObjCounts[20][15][16]` | **undetermined** | **EXE** `AITask.h:600`, module **9/105** | see §4.5 |
| `HelpWindow_*` | no RockMonster array exists | **EXE**, module **0/19** | n/a |
| `Weapon_GetDamageForObject` → `objectCoefs[t][id][lvl]` | yes | **OURS** `Weapons.cpp:224`; module **48/48** | aliases Building[0] in a heap struct — the *only* row that is mirrorable today |
| `Encyclopedia_Initialise` | `rockmonsterFiles[i]`, sized by count | **OURS** `Encyclopedia.cpp:96-108` | fine |
| `DeepCore::PickEmergeSpecies` / `ApplyCreatureVariant` | guarded | **OURS** `DeepCore.cpp:107-115,179-182,197-199` | **already refuses id ≥ 15** |

Module implementation counts above are `grep -cE '^#define [A-Za-z_0-9]+ \(\(' ` (live macros)
versus `^\s*//#define` (implemented) per header:
`Weapons.h` 0/48 live → **48 implemented**; `Stats.h` 0 macros → **fully C++**;
`Creature.h` 0/24 live → **24 implemented**; `AITask.h` **96 live / 9 implemented**;
`Interface.h` **96 live / 3**; `Dependencies.h` **11 live / 0**; `HelpWindow.h` **19 live / 0**;
`Object.h` **196 live / 90**; `Game.h` **141 live / 83**.

### 4.5 The item I could not determine

`aiGlobs.requestObjCounts[LegoObject_Type_Count][LegoObject_ID_Count][OBJECT_MAXLEVELS]`
(`AITask.h:147`). The name says it counts *requested* objects (things a unit has been asked to
build or train), which would be indexed by the **requested** type/ID — vehicles and buildings —
not by the requesting monster's own ID. If so, a RockMonster ID never indexes it. **But
`AITask_Callback_UpdateObject` (`0x004041b0`, `AITask.h:600`) is EXE and the module is 9/105
implemented, so this cannot be confirmed from source.** Treat it as unresolved. It does not change
the verdict, because §4.3 already kills the increment on its own.

### 4.6 Verdict on ID 15

**ID 15 is not usable.** Not "risky" — unusable, with a silent corruption signature that would be
extremely hard to diagnose from a bug report ("my Tool Store's stats drift after monsters spawn").

**What would have to be reimplemented first**, in dependency order:

1. `Stats_Initialise` — already OURS; needs a mirror instead of `objectStats[type][id]`. *Cheap.*
2. The object-level counters — already OURS (`Object.cpp:378-406,453,1146`; `Stats.cpp:975-976`);
   need a mirror. *Cheap, but must confirm no exe writer exists — unverifiable.*
3. `WeaponStats.objectCoefs` — OURS end-to-end and **heap**; mirrorable. *Cheap.*
4. `Dependencies` — **11 functions, 0 implemented.**
5. `Interface` — **96 functions, 3 implemented.**
6. `AITask` — **96 functions, 9 implemented.**
7. `LegoObject_LoadObjTtsSFX`, `Lego_GetObjectByName` (+ hook), `Lego_GetObjectTypeIDCount` (+ hook).

Items 4–6 alone are **~200 exe functions**. That is the real price of a 16th ID, and it is
*decompilation* work, not design work.

---

## 5. The three candidate increments, evaluated

### (a) Reimplement the loader, keep the cap at 15

- **Cost:** ~90 lines (§2.5), one header flip, optionally one interop line. Zero layout risk — all
  destinations are heap pointers (§2.1). No new compiler warnings.
- **Gains:** loud, per-entry errors where the exe fails silently; a roster log; a natural site for
  the ceiling check; a DeepCore hook point at load time.
- **Unlocks:** *nothing new for content.* The free slots below 15 already work with no code, because
  `Lego_GetObjectByName` and `Stats_Initialise` are count-driven (§3.1, §3.2).
- **Risk:** we are replacing a 432-byte exe function we cannot read
  (`0x0042d030` → `0x0042d1e0`), and **we cannot play-test it.** If it does one thing we did not
  infer — a loading-bar tick, an activity registration, a flag — we ship a regression that compiles
  perfectly. This is the whole risk, and it is not small for a change that unlocks nothing.

### (b) Reimplement + DLL-side mirror tables for IDs ≥ 15

- Mirrors are the proven in-tree pattern for escaping fixed exe arrays — `static std::vector<Point2I>
  _drainPowerBlockList / _poweredBlockList / _unpoweredBlockList;` with the comment
  `// PowerGrid replacements for infinite grid sizes.` (`Game.cpp:169-172`).
- **Blocked.** A mirror only works if *every* toucher of the table is C++. Of the six tables a
  monster ID reaches, only `Stats_Globs`, the `Object_Globs` level counters, and `WeaponStats` are
  C++-owned. `Dependencies` (0/11), `Interface` (3/99) and `AITask` (9/105) are not, and they are
  fed by `Lego_GetObjectByName`, which is EXE with seven EXE callers (§3.1). You cannot mirror a
  table whose writer is 1999 machine code.
- Worse: for the exe consumers there is no "safe" id to hand out. They *need* a valid index, and
  every index ≥ 15 aliases Building 0.

### (c) Not worth it yet

- Correct about the >15 ambition. **Incomplete as a plan**, because it leaves a live hazard in
  place: today, a modder who adds a 16th `RockMonsterTypes` entry gets no error from anything. The
  exe loader allocates 16 heap slots happily; `Lego_GetObjectByName` returns 15 happily;
  `Stats_Initialise` then overruns a heap block and clobbers Building 0 — silently.

---

## 6. DECISION

> **Build (a), reduced to its load-bearing half — and build it in the code we already own.**
>
> **Do not write the `Lego_LoadRockMonsterTypes` body in this increment.** Ship the **ID-ceiling
> guard rail** instead. Revisit the loader body only when something concrete needs a per-species
> load-time hook.

Reasoning, stated plainly:

1. Reimplementing the loader **unlocks zero capability**. Every gain attributed to it — validation,
   loud errors, control — is available without it, in functions that are already C++.
2. Reimplementing it **carries the one risk this project cannot absorb**: an unverifiable
   behavioural regression in startup code, on a project whose honest ceiling is "compiles clean".
   Trading real regression risk for zero capability is a bad trade.
3. The **actual defect** discovered by this investigation is not the loader — it is
   `Stats.cpp:163-164` writing out of bounds and clobbering Building ID 0 when handed an ID ≥ 15,
   with nothing anywhere in the tree warning about it. That is worth fixing today, costs ~25 lines,
   touches only C++ we own, adds no warnings, and cannot regress anything (the guarded path is
   currently *already* broken).
4. It also **pre-pays for the loader**: when the loader is eventually written, the ceiling check in
   §2.5 becomes a second line of defence rather than the only one.

### 6.1 Code sketch — the increment to build

**Fix 1 — make `Stats_Initialise` refuse out-of-range IDs** (`Stats.cpp`, immediately after the
existing `Lego_GetObjectByName` block at `Stats.cpp:138-141`):

```cpp
// Stats.cpp — insert after line 141
/// DEEPCORE: Reject IDs at or past the engine ceiling. LegoObject_ID_Count is welded to
/// the exe's data-segment layout (Stats.h:228, Object.h:454/463/464, AITask.h:147,
/// Dependencies.h:91, Interface.h:143, Weapons.h:83). An id of 15 does NOT run off the
/// end of objectLevels[20][15] -- it lands on [4][0], i.e. LegoObject_Building ID 0,
/// and objectStats[type][id] overruns the LegoObject_ID_Count-sized block allocated
/// below. Both are silent. Refuse loudly instead.
/// See docs/research/type-loader-reimplementation.md §4.
if ((uint32)id >= (uint32)LegoObject_ID_Count) {
    Config_WarnItem(true, prop, "Object ID exceeds the engine ceiling; Stats entry ignored");
    continue;
}
```
`Config_WarnItem` is already the idiom two lines above (`Stats.cpp:139`), so this introduces no new
dependency and no new warning.

**Fix 2 — a startup audit of the five rosters**, in the DeepCore layer that already owns this kind
of check (`DeepCore.cpp:107-115` and `:179-182` already refuse ids ≥ 15 for emerges and variants —
this generalises the same rule to the tables themselves):

```cpp
// DeepCore.hpp
/// Verify the loaded object rosters against the engine's hard ID ceiling.
/// Returns false if any roster overflows; the caller decides whether that is fatal.
bool AuditObjectRosters(void);
```

```cpp
// DeepCore.cpp
bool DeepCore::AuditObjectRosters(void)
{
    using namespace LegoRR; // required: legoGlobs macros/types resolve unqualified

    struct Roster { const char* cfgBlock; uint32 count; };
    const Roster rosters[] = {
        { "VehicleTypes",     legoGlobs.vehicleCount    },
        { "MiniFigureTypes",  legoGlobs.miniFigureCount },
        { "RockMonsterTypes", legoGlobs.rockMonsterCount},
        { "BuildingTypes",    legoGlobs.buildingCount   },
        { "UpgradeTypes",     legoGlobs.upgradeCount    },
    };

    bool ok = true;
    for (const Roster& r : rosters) {
        if (r.count > (uint32)LegoObject_ID_Count) {
            ok = false;
            DeepCore_WarnF(true,
                "%s declares %u entries but the engine ceiling is %i. Entries at index %i and "
                "above will corrupt the first BuildingTypes entry -- this limit lives in the "
                "original executable's data layout and cannot be raised.",
                r.cfgBlock, r.count, (sint32)LegoObject_ID_Count, (sint32)LegoObject_ID_Count);
        }
        else if (settings.verboseStartup) {
            DeepCore_LogF("%s: %u/%i", r.cfgBlock, r.count, (sint32)LegoObject_ID_Count);
        }
    }
    return ok;
}
```

**Fix 3 — call it at the one place the rosters become valid**, inside the existing `if` body at
`GameState.cpp:515`, before `Lego_LoadObjectNames`:

```cpp
// GameState.cpp — first statement inside the block opened at :515
/// DEEPCORE: the five type loaders have just run; the rosters are now the source of truth
/// for every [20][15] table the engine indexes. Audit before anything consumes them.
DeepCore::AuditObjectRosters();
```

This is the whole increment: three edits, one new function, no interop entry, no layout change, no
new warning, and no behaviour change on well-formed data.

### 6.2 Deferred, with the trigger written down

Write the real `Lego_LoadRockMonsterTypes` body (§2.5) **when, and only when**, one of these is
true:

- a DeepCore feature needs a per-species hook at *load* time (e.g. per-species template metadata
  that must exist before `Creature_Clone`), or
- a disassembly of `0x0042d030` becomes available in-tree, removing the inference risk, or
- the sibling loaders are being ported as a family for an unrelated reason.

At that point the checklist is: comment `Game.h:1474`, uncomment `Game.h:1475`, write the body at
`Game.cpp:3263`, keep the ceiling clamp, and add the `hook_write_jmpret(0x0042d030, ...)` line to
`interop_hook_LegoRR_Game` (`interop.cpp:2696`) as insurance against un-enumerated exe callers.

---

## 7. What I could not determine

1. **The exact `RockMonsterTypes` value-string layout.** No `Lego.cfg` exists in this tree
   (`data/` holds only `Settings/DeepCore.cfg` and `Settings/Shortcuts.cfg`). The `Name  Path`
   pair shape is certain from the Config API; the value being *only* a path is inferred from
   `Creature_Load`'s signature (`Creature.cpp:112`).
2. **Whether `0x0042d030` does anything beyond count → allocate → `Creature_Load` → `Creature_Hide`.**
   No disassembly is available in-tree. The function occupies `0x0042d030`–`0x0042d1e0` = 432 bytes,
   identical to the vehicle, minifigure and building loaders, which is consistent with the simple
   two-pass shape — but that is circumstantial.
3. **Whether `0x0042d030`, `0x0042e7e0` or `0x0042ee70` are merged functions.** If merged,
   `hook_write_jmpret` is forbidden (`interop.cpp:1766`, `1920`) and call sites must be patched
   individually.
4. **Whether any exe function other than `Lego_Initialise` calls the five loaders.** Unenumerable
   from source; mitigated by adding the hook.
5. **What `Lego_GetObjectTypeIDCount` returns for the fifteen non-model `LegoObject_Type` values.**
6. **Whether `aiGlobs.requestObjCounts` is ever indexed by the acting monster's own ID** (§4.5).
7. **Whether any exe code writes `objectGlobs.objectTotalLevels` / `objectPrevLevels` directly.**
   All C++ writers are enumerated (`Object.cpp:378-406,453,1146`; `Stats.cpp:975-976`); exe writers
   cannot be ruled out, which is why the mirror in option (b) is not safe even for that table.
8. **Whether stock `Lego.cfg` declares all eleven RockMonster names** (`GameCommon.h:137-150`).
   The commonly quoted "~4 free slots" follows from 15 − 11, but the actual free count depends on
   the user's own cfg.
