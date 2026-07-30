# Silent Failures and Unchecked Writes

An audit of every place the engine corrupts memory or swallows an error instead of
complaining. Compile-only analysis: **we cannot run the game**, so every failure scenario
below is derived by reading the code and the address map, never by observation.

Evidence base: `docs/address-map.json` (113 sized overlaid regions, 0 overlaps),
`docs/ADDRESS-MAP.md`, and the sources named at each finding.

---

## 0. Scope, method, and a mid-audit change on disk

### 0.1 What was read

Read in full: `game/object/Stats.cpp`, `game/object/Weapons.cpp`,
`game/object/Dependencies.cpp`, `game/object/Upgrade.cpp`, `engine/core/Config.cpp`, and
the object/type-loading paths reachable from `game/GameState.cpp:566-571`.

Then a targeted sweep for the same bug *class* across the tree: every call to the
unbounded `Gods98::Util_Tokenise` (`engine/core/Utils.cpp:41`), every `std::strcpy`/
`std::sprintf` into a fixed buffer fed from config, and every `table[counter++]` whose
counter is driven by a config item count.

### 0.2 `Dependencies.cpp` and `Upgrade.cpp` contain no code

Both files are **entirely commented-out declarations**. Every function they nominally own
is still a raw address macro into the 1999 executable:

- `Dependencies.cpp:24-55` — all twelve `Dependencies_*` entry points are commented out.
  The only live statement in the file is `Dependencies.cpp:14`, which binds
  `dependencyGlobs` to `0x004b9bc8`.
- `Upgrade.cpp:13-47` — all twelve `Upgrade_*` entry points are commented out. The file
  contains no executable statement at all.

So `Dependencies_Initialise` (`GameState.cpp:565`) and `Upgrade_Load` both parse config
and both index per-type/per-ID tables — but **inside the exe**, where we cannot inspect or
fix them. `dependencyGlobs` is 0xe5b4 bytes at `0x004b9bc8`, sitting immediately after
`damageTextGlobs` (ends `0x004b9bc4`) and immediately before `effectGlobs`
(`0x004c8180`). Anything the exe's `Dependencies_Initialise` writes out of range lands in
`effectGlobs`. **This is stated as a known unknown, not a finding.** The same applies to
`LegoObject_LoadObjTtsSFX` (`Object.h:1628`, address `0x0044af80`), which fills
`objectGlobs.objectTtSFX[20][15]` (`Object.h:454`) — the identical `[20][15]` shape as the
ground-truth bug, still exe-side.

### 0.3 Ground-truth item #3 was fixed on disk *during* this audit

The task brief states that `Stats_Initialise` writes `objectLevels[type][id]` with no
bounds check. That was true of `HEAD` (commit `8de5929`). It is **no longer true of the
working tree**: `src/openlrr/game/object/Stats.cpp` was modified at 07:20:35 on
2026-07-30, mid-audit, adding a guard. `git status` shows the file as `M` (uncommitted).

The added guard is `Stats.cpp:143-172`:

```cpp
if ((uint32)type >= (uint32)LegoObject_Type_Count ||
    (uint32)id   >= (uint32)LegoObject_ID_Count)
{
    Config_FatalItemF(true, prop, "Stats entry \"%s\" resolved to out-of-range indices ...");
    continue;
}
```

This is the correct guard in the correct place, and the `(uint32)` casts correctly catch
`LegoObject_TVCamera = -1` (`GameCommon.h:1067`). **All line numbers in this document
refer to the current working tree**, which is offset +31 from `HEAD` for everything below
`Stats.cpp:140`.

Two consequences follow, and both are findings in their own right:

1. The guard uses `Config_FatalItemF`, which **terminates the process by default**. See
   F-14. The comment in the guard claims it "complains AND skips"; with stock log levels
   the `continue` is unreachable.
2. The guard fixed `Stats.cpp`. It did **not** fix the identical unguarded write in
   `Weapons.cpp:165`, which is now the highest-severity finding in this document (F-01).

---

## 1. Unbounded indexed writes

The engine's characteristic bug. An index is produced by name lookup or by counting config
lines, then used to write a fixed-size table whose bound is welded to the 1999 data
segment. Ranked hardest-first.

---

### F-01 — `Weapon_Initialise` writes `objectCoefs[objType][objID][objLevel]` with no bounds check

**Severity: CORRUPTION.** This is the exact twin of ground-truth item #3, in the file next
door, still unguarded.

**Location:** `game/object/Weapons.cpp:152-166`. Ours (real C++, hooked over the exe at
`interop.cpp` — the function body is at `Weapons.cpp:38`).

```cpp
LegoObject_Type objType = LegoObject_None;
LegoObject_ID objID = (LegoObject_ID)0;
if (Lego_GetObjectByName(Gods98::Config_GetItemName(stat), &objType, &objID, nullptr)) {
    const uint32 objLevelCount = Stats_GetLevels(objType, objID);
    ...
    for (uint32 objLevel = 0; objLevel < objLevelCount; objLevel++) {
        weaponStats->objectCoefs[objType][objID][objLevel] = (real32)std::atof(...);
    }
}
```

**The index:** `objType` and `objID` come straight out of `Lego_GetObjectByName`, which is
**still an exe address macro** (`Game.h:1511`, `0x0042e7e0`). It returns whatever indices
its own name tables hold. Those tables are sized by `legoGlobs.rockMonsterCount`,
`vehicleCount`, `buildingCount`, `miniFigureCount` (`Game.h:1bc-1cc`) — plain `uint32`
counters filled from the user's `Lego.cfg`, with **no ceiling anywhere on our side**.

**The bound that should apply:** `objType < LegoObject_Type_Count` (20, `GameCommon.h:1092`)
and `objID < LegoObject_ID_Count` (15, `GameCommon.h:1143`). Neither is checked.
`objLevel < OBJECT_MAXLEVELS` (16, `GameCommon.h:76`) *is* effectively bounded, because
`Stats_GetLevels` returns `statsGlobs.objectLevels[type][id]` which `Stats.cpp:185` caps —
but only for entries that reached `Stats_Initialise` at all.

**What gets clobbered.** `objectCoefs` is `real32[20][15][16]` = 0x4b00 bytes at offset 0
of `WeaponStats` (`Weapons.h:83`), and `WeaponStats` is 0x4b68 (`Weapons.h:95`). The
element stride is 15*16*4 = 960 bytes per `objType`, 64 bytes per `objID`. So:

| Overflowing index | Byte offset | What it actually is |
|---|---|---|
| `objID == 15`, `objType == 3` (RockMonster) | 0x0F00 | `objectCoefs[4][0][*]` — **Building ID 0's damage coefficients** (`ToolStation` in stock cfg) |
| `objType == 20` | 0x4B00 | `WeaponStats::isSlowDeath` (`Weapons.h:84`) |
| `objType == 20, objID == 0, objLevel == 1..2` | 0x4B04-0x4B08 | `slowDeathInitialCoef`, `slowDeathDuration` |
| `objType == 20, objID == 0, objLevel == 3..7` | 0x4B0C-0x4B1C | `rechargeTime`, `damage`, `dischargeRate`, `ammo`, `weaponRange` |
| `objType == 20, objID == 0, objLevel >= 8` | >= 0x4B20 | `wallDestroyTimes[]`, then the **next `WeaponStats` element**, then past the end of the `Mem_Alloc` at `Weapons.cpp:56` |

`weaponStatsList` is heap (`Weapons.cpp:56`), not the data segment, so the last row is a
heap-allocator metadata smash rather than a named-global smash. `weaponGlobs` itself
(`0x00504870`, 0x1b90, `Weapons.cpp:27`) holds only the *pointer*, and its next sized
neighbour is `Gods98::mainGlobs` at `0x00506800` (`engine/Main.cpp:75`) — 0x400 of raw gap
away, so `weaponGlobs` overflow is not the risk here; the heap is.

**Concrete failure scenario.** A modder adds a 16th `RockMonsterTypes` entry (`Slimy`) to
`Lego.cfg`. `Lego_GetObjectByName("Slimy")` returns `objType = 3, objID = 15`. The
`WeaponTypes::Boulder::Slimy` line writes 16 floats at byte offset 0x0F00 of the Boulder
`WeaponStats` — which is `objectCoefs[LegoObject_Building][0][0..15]`. Every boulder that
hits the player's Tool Store now does `Slimy`'s damage coefficient. No message is printed.
The player reports "boulders one-shot my Tool Store" and nothing in the log mentions
weapons, monsters, or the 16th species.

The *second* scenario is worse and needs no 16th monster: any `WeaponTypes` sub-key that
`Lego_GetObjectByName` resolves to a type >= 20 sets `isSlowDeath` to a nonzero float bit
pattern, silently converting that weapon into a damage-over-time weapon with a garbage
duration — `Weapon_GenericDamageObject` (`Weapons.cpp:242`) then takes the `isSlowDeath`
branch forever.

**Fix (unconditional).** The alternative to this guard is an out-of-bounds write; there is
no defined vanilla behaviour to preserve. Mirror the guard that just landed in
`Stats.cpp:162`, but *warn* rather than kill (see F-14):

```cpp
LegoObject_Type objType = LegoObject_None;
LegoObject_ID objID = (LegoObject_ID)0;
if (Lego_GetObjectByName(Gods98::Config_GetItemName(stat), &objType, &objID, nullptr)) {

    /// DEEPCORE: objType/objID come from Lego_GetObjectByName, still exe code
    /// (Game.h:1511). objectCoefs is real32[20][15][16] welded to WeaponStats'
    /// 0x4b68 layout (Weapons.h:83,95): objID 15 aliases the NEXT object type's
    /// row, and objType 20 lands exactly on WeaponStats::isSlowDeath (0x4b00).
    /// Unconditional: the behaviour being replaced is an out-of-bounds write.
    if ((uint32)objType >= (uint32)LegoObject_Type_Count ||
        (uint32)objID   >= (uint32)LegoObject_ID_Count)
    {
        Config_WarnItemF(true, stat,
            "WeaponTypes object coef \"%s\" resolved to out-of-range indices "
            "(type %i, max %i; id %i, max %i); entry skipped.",
            Gods98::Config_GetItemName(stat),
            (sint32)objType, (sint32)LegoObject_Type_Count,
            (sint32)objID,   (sint32)LegoObject_ID_Count);
        continue;   // next `stat` in the enclosing for-loop
    }

    const uint32 objLevelCount = std::min(Stats_GetLevels(objType, objID),
                                          (uint32)OBJECT_MAXLEVELS);
    ...
}
```

Note the `std::min` on `objLevelCount` as well: `Stats_GetLevels` is a raw table read
(`Stats.cpp:1176`) with no clamp of its own, and if `objectLevels` was ever corrupted by
something upstream this is the loop that turns it into a write.

**Gate: unconditional.** Guard against UB, not a behaviour change.

---

### F-02 — `Priorities_LoadLevel` appends to nine parallel `[27]` arrays with an uncapped counter

**Severity: CORRUPTION.** The most self-destructive overflow in the tree, because it
overwrites its own loop counter.

**Location:** `game/interface/Priorities.cpp:92-110`. Ours.

```cpp
prioritiesGlobs.count = 0;
for (prop = Config_FindArray(config, arrayID); prop != nullptr; prop = Config_GetNextItem(prop)) {
    if (AIPriority_GetType(Gods98::Config_GetItemName(prop), &locAiPro)) {
        prioritiesGlobs.buttonTypes[prioritiesGlobs.count] = locAiPro;
        prioritiesGlobs.initialTypes[prioritiesGlobs.count] = locAiPro;
        prioritiesGlobs.initialValues[prioritiesGlobs.count] = ...;
        ...
        prioritiesGlobs.initialOff[prioritiesGlobs.count] = FALSE;
        ...
        prioritiesGlobs.count++;
    }
}
```

**The index:** `prioritiesGlobs.count`, incremented once per recognised entry in the
level's `Priorities` block. **Duplicate names are accepted** — `AIPriority_GetType` is a
pure name match, so listing `Crystal` thirty times gives `count == 30`.

**The bound that should apply:** `AI_Priority_Count` = **27** (`GameCommon.h:236-266`).

**What gets clobbered.** `Priorities_Globs` is 0x4c0, `assert_sizeof` at
`Priorities.h:71`, overlaid at `0x00501f00` (`Priorities.cpp:31`). Each `[27]` array is
0x6c bytes. Writing index 27 of each array lands, by construction, on the *start of a
later field of the same struct*:

| Write | Struct offset | Lands on |
|---|---|---|
| `buttonTypes[27]` | 0x21c + 0x6c = 0x288 | `buttonPoints[0].x` (`Priorities.h:61`) |
| `initialTypes[27]` | 0x360 + 0x6c = 0x3cc | `initialValues[0]` (`Priorities.h:63`) |
| `initialValues[27]` | 0x3cc + 0x6c = 0x438 | `initialOff[0]` (`Priorities.h:64`) |
| `initialOff[27]` | 0x438 + 0x6c = 0x4a4 | **`prioritiesGlobs.count` itself** (`Priorities.h:65`) |

**Concrete failure scenario.** A level cfg lists 28 priority lines. On the 28th iteration
`initialOff[27] = TRUE` writes `1` into `count`; then `count++` makes it `2`. The 29th
entry therefore writes `buttonTypes[2]`, silently overwriting the third real priority. The
loop never escapes the struct and never terminates abnormally — it just scrambles the
priority table into nonsense and returns `TRUE`. The player sees a priority bar with
wrong icons in wrong slots and a `Priorities_Reset` (`Priorities.cpp:75`) that restores the
wrong values. Nothing is logged.

If the struct layout were different this would walk off the end of `prioritiesGlobs` into
`LegoRR::s_ScrollInfo_BOOL_005023c0` (`ScrollInfo.cpp:14`), the very next sized region.

**Fix (unconditional).**

```cpp
if (AIPriority_GetType(Gods98::Config_GetItemName(prop), &locAiPro)) {

    /// DEEPCORE: every array indexed below is [AI_Priority_Count] == 27, welded
    /// to Priorities_Globs' 0x4c0 layout (Priorities.h:71). Index 27 aliases a
    /// LATER FIELD of the same struct -- initialOff[27] is `count` itself, so
    /// overflowing here silently rewrites the loop counter.
    if (prioritiesGlobs.count >= (uint32)AI_Priority_Count) {
        Config_WarnItemF(true, prop,
            "More than %i entries in the Priorities block; \"%s\" and everything "
            "after it is ignored.",
            (sint32)AI_Priority_Count, Gods98::Config_GetItemName(prop));
        break;
    }
    ...
}
```

**Gate: unconditional.**

---

### F-03 — `Priorities_LoadImages`: `strcpy` from config into `buff[512]`, then four unchecked `stringParts[]` reads

**Severity: CORRUPTION.** Three separate defects in five lines.

**Location:** `game/interface/Priorities.cpp:45-66`. Ours.

```cpp
char* stringParts[10];
char* stringPartsSub[10];
char buff[512];
...
    std::strcpy(buff, Gods98::Config_GetDataString(prop));
    Gods98::Util_Tokenise(buff, stringParts, ":");
    int numSubParts = Gods98::Util_Tokenise(stringParts[0], stringPartsSub, "|");
    prioritiesGlobs.langPriorityName[priorityTypeLoc] = Gods98::Util_StrCpy(stringPartsSub[0]);
    ...
    prioritiesGlobs.priorityImage[priorityType]      = Gods98::Image_LoadBMPScaled(stringParts[1], 0, 0);
    prioritiesGlobs.priorityPressImage[priorityType] = Gods98::Image_LoadBMPScaled(stringParts[2], 0, 0);
    prioritiesGlobs.priorityOffImage[priorityType]   = Gods98::Image_LoadBMPScaled(stringParts[3], 0, 0);
```

1. **`Priorities.cpp:54`** — `std::strcpy` into `char buff[512]` from a config data string
   of arbitrary length. Stack smash at 512 bytes. It is also an unchecked `nullptr`:
   `Config_GetDataString` returns the raw `dataString` field, which `Config_Load2`
   explicitly leaves `nullptr` for a trailing key with no value and merely warns about
   (`Config.cpp:210-215`).
2. **`Priorities.cpp:55`** — `Util_Tokenise` is the *unbounded* variant
   (`Utils.cpp:41-44` forwards `count = UINT32_MAX`), writing into `stringParts[10]`. A
   `PriorityImages` value with 11+ colons overruns the array; at index 10 it hits
   `stringPartsSub`, and past that, the saved frame pointer.
3. **`Priorities.cpp:59-61`** — `stringParts[1]`, `[2]`, `[3]` are read with **no check on
   the token count**. A line with only two colon-separated fields leaves `stringParts[3]`
   holding stale stack garbage, which is handed to `Image_LoadBMPScaled` as a filename.

**Concrete failure scenario.** A modder writes `Crystal  Crystal:crystal.bmp` (two fields
instead of four). `stringParts[2]` and `stringParts[3]` are uninitialised stack slots.
`Image_LoadBMPScaled` is called on two wild `char*`. Best case an access violation on a
BMP load; worst case a stack address that happens to be readable, producing a corrupt
image handle stored in `prioritiesGlobs.priorityOffImage[]` for the rest of the session.

**Fix (unconditional).** Replace with the bounded API that already exists,
`Util_TokeniseSafe` (`Utils.cpp:47`), and check the count:

```cpp
/// DEEPCORE: was strcpy into a 512-byte stack buffer from an arbitrary-length
/// config value, then read stringParts[1..3] without checking the token count.
const char* dataStr = Gods98::Config_GetDataString(prop);
if (dataStr == nullptr) {
    Config_WarnItemF(true, prop, "PriorityImages \"%s\" has no value; skipped.",
                     Gods98::Config_GetItemName(prop));
    continue;
}

char* value = Gods98::Util_StrCpy(dataStr);   // heap, exact size; no 512 cap
const uint32 numParts = Gods98::Util_TokeniseSafe(value, stringParts, ":",
                                                  _countof(stringParts));
if (numParts < 4) {
    Config_WarnItemF(true, prop,
        "PriorityImages \"%s\" needs 4 colon-separated fields "
        "(name[|sfx]:normal:pressed:off), found %i; skipped.",
        Gods98::Config_GetItemName(prop), (sint32)numParts);
    Gods98::Mem_Free(value);
    continue;
}
const uint32 numSubParts = Gods98::Util_TokeniseSafe(stringParts[0], stringPartsSub,
                                                     "|", _countof(stringPartsSub));
...
Gods98::Mem_Free(value);
```

`Util_StrCpy` also removes the fixed 512-byte ceiling entirely, and `Mem_Free` matches the
existing ownership convention used at `Priorities.cpp:132`.

**Gate: unconditional** for the `TokeniseSafe` swap and the `nullptr` guard (both replace
UB). The `continue` on a short line is a control-flow change, but the behaviour it replaces
is "load a BMP from an uninitialised pointer", which is not behaviour worth preserving.

---

### F-04 — `SFX_LoadSampleProperty` appends to `sampleGroupTable[200]` with an uncapped counter

**Severity: CORRUPTION.** Overflows out of `sfxGlobs` and directly into `statsGlobs`.

**Location:** `game/audio/SFX.cpp:200`. Ours.

```cpp
curItem = &sfxGlobs.sampleGroupTable[sfxGlobs.sampleGroupCount++];
```

**The index:** `sfxGlobs.sampleGroupCount`, incremented once per *extra* comma-separated
sample in a `Samples` entry, accumulated across every `Samples` line in the cfg. Never
reset within the loop, never checked.

**The bound that should apply:** `SFX_MAXSAMPLEGROUPS` = **200** (`SFX.h:35`).

**What gets clobbered.** `SFX_Globs` is 0x1770, `assert_sizeof` at `SFX.h:115`, overlaid at
`0x00502468` (`SFX.cpp:21`). `sampleGroupTable` is at +0x0f78, size 0x640, so each
`SFX_Property` is 8 bytes:

| Write | Struct offset | Lands on |
|---|---|---|
| `sampleGroupTable[200]` | 0x15b8 | `hashNameList` (a `uint32*`) and `hashNameCount` (`SFX.h:98-99`) |
| `sampleGroupTable[201]` | 0x15c0 | `sampleGroupCount` itself, and `flags` (`SFX.h:100-101`) |
| `sampleGroupTable[202..]` | 0x15c8+ | `sfxInstanceTable[10]`, `soundQueue*` tables |
| `sampleGroupTable[293+]` | >= 0x1770 | **past the end of `sfxGlobs` — `sfxGlobs` ends at exactly `0x00503bd8`, which is `LegoRR::statsGlobs` (`Stats.cpp:41`)**, i.e. `statsGlobs.objectStats[0..]`, the `ObjectStats**` pointer table |

This is the only region pair in the map that is *exactly* adjacent with zero slack:
`0x00502468 + 0x1770 == 0x00503bd8`.

**Concrete failure scenario.** An expanded sound pack defines 210 grouped samples across
the `Samples` block. Group 200 overwrites `hashNameList` with an `sint32` sound handle
(a small integer like `0x0000004A`). `SFX_Random_GetSound3DHandle` and its siblings later
dereference `hashNameList` — a near-null pointer — and the game access-violates on the
first random sound. The crash address points into SFX code, giving no hint that the actual
cause is one sample too many in the cfg.

**Fix (unconditional).**

```cpp
/// DEEPCORE: sampleGroupTable is SFX_MAXSAMPLEGROUPS (200) entries welded to
/// SFX_Globs' 0x1770 layout (SFX.h:35,115). Entry 200 overwrites hashNameList,
/// entry 201 overwrites sampleGroupCount itself, and sfxGlobs ends at exactly
/// 0x00503bd8 == statsGlobs, so a large enough overflow walks into the object
/// stats pointer table.
if (sfxGlobs.sampleGroupCount >= SFX_MAXSAMPLEGROUPS) {
    Error_WarnF2(true, "SFX: sample group table full (%i); extra grouped samples "
                       "for this SFX are ignored.\n", (sint32)SFX_MAXSAMPLEGROUPS);
    success = false;
    break;
}
curItem = &sfxGlobs.sampleGroupTable[sfxGlobs.sampleGroupCount++];
```

**Gate: unconditional.**

---

### F-05 — `SFX_LoadSampleProperty` volume parser: unbounded copy into `volBuff[64]`, unbounded read past end of string

**Severity: CORRUPTION.**

**Location:** `game/audio/SFX.cpp:142` (`char volBuff[64]`), `SFX.cpp:167-175`. Ours.

```cpp
if (*s == '#') { // volume modifier: #-vol#
    s++;
    char* v = volBuff;
    while (*s != '#') {
        *v++ = *s++;   // Copy number between #...#
    }
    s++;
    *v = '\0';
    volume = std::atoi(volBuff);
}
```

Two unbounded operations in one loop:

- **Write:** no check that `v` stays inside `volBuff[64]`. `*v = '\0'` after the loop can
  itself be the 65th byte.
- **Read:** if there is no closing `#`, `s` walks off the end of the token — and because
  `Util_Tokenise` replaced the separators with `'\0'` in-place (`Utils.cpp:60`), it walks
  through the *rest of the config file's data buffer* until it happens to find a `#` or
  faults.

**Concrete failure scenario.** A cfg line reads `SFX_Drill  #80drill.wav` — a missing
closing `#`, an easy typo. The loop copies `80drill.wav\0` and then keeps going through
every subsequent byte of `rootConf->fileData` (the whole Lego.cfg, `Config.cpp:87`) into a
64-byte stack buffer. Stack smash within microseconds, in a function whose name never
appears in the resulting crash context.

**Fix (unconditional).**

```cpp
if (*s == '#') {
    s++;
    char* v = volBuff;
    char* const vEnd = volBuff + _countof(volBuff) - 1;   // reserve the terminator
    while (*s != '#' && *s != '\0' && v < vEnd) {
        *v++ = *s++;
    }
    *v = '\0';
    /// DEEPCORE: complain instead of running off the end of the token. The
    /// separators were already turned into '\0' in place by Util_Tokenise
    /// (Utils.cpp:60), so an unterminated '#' previously scanned the entire
    /// config file buffer into a 64-byte stack array.
    Error_WarnF2((*s != '#'), "SFX: unterminated '#volume#' modifier in \"%s\"; "
                              "treating volume as %i.\n", sampleNames[i], std::atoi(volBuff));
    if (*s == '#') s++;
    volume = std::atoi(volBuff);
}
```

**Gate: unconditional.**

---

### F-06 — `Config_BuildStringID`: unbounded `strcpy`/`strcat` into the 1024-byte `s_JoinPath_string`

**Severity: CORRUPTION.** Reachable from every config-driven subsystem, including
`Stats_ID` on every single stat of every single object.

**Location:** `engine/core/Config.cpp:225-245`. Ours.

```cpp
std::strcpy(configGlobs.s_JoinPath_string, s);
va_start(args, s);
while (curr = va_arg(args, const char*)) {
    std::strcat(configGlobs.s_JoinPath_string, CONFIG_SEPARATOR);
    std::strcat(configGlobs.s_JoinPath_string, curr);
}
```

**The bound that should apply:** `CONFIG_MAXSTRINGID` = **1024** (`Config.h:47`), the
declared size of `Config_Globs::s_JoinPath_string` (`Config.h:122`).

**What gets clobbered.** `Config_Globs` is 0x48c, `assert_sizeof` at `Config.h:129`,
overlaid at `0x00507098` (`Config.cpp:21`). `s_JoinPath_string` occupies +0x000..+0x400.
Immediately after it, inside the same struct:

| Overflow byte | Struct offset | Lands on |
|---|---|---|
| 1025..1152 | 0x400..0x480 | `Config* listSet[CONFIG_MAXLISTS]` — **the 32 head pointers of the config ListSet** (`Config.h:123`) |
| 1153..1156 | 0x480 | `freeList` (`Config.h:124`) |
| 1157..1160 | 0x484 | `listCount` (`Config.h:125`) |
| 1161..1164 | 0x488 | `flags` (`Config.h:126`) |
| 1165+ | >= 0x48c | past `configGlobs`; next sized region is `Gods98::fontGlobs` at `0x00507528` (`Fonts.cpp:19`) |

**Concrete failure scenario.** The composed ID for a stat is
`<gameName>::Stats::<objectName>::<statName>` (`Stats.cpp:119`). A pathological or
machine-generated `Lego.cfg` with a ~1000-character object name overruns
`s_JoinPath_string` into `listSet[]`. Nothing crashes immediately — the ASCII bytes are
valid-looking pointers. The crash comes later, at `Config_Free`/`configListSet.Shutdown`
(`Config.cpp:50`, `Config.cpp:617-638`), when the allocator is handed a pointer made of
the modder's object name. Attributing that crash to the object name is essentially
impossible from the dump.

**Fix (unconditional, message-only + truncation).** Truncation is not a behaviour change in
any well-formed config; the only inputs it affects are ones that currently corrupt memory:

```cpp
const char* __cdecl Gods98::Config_BuildStringID(const char* s, ...)
{
    log_firstcall();

    std::va_list args;
    const char* curr;

    /// DEEPCORE: was strcpy + unbounded strcat into a 1024-byte array welded to
    /// Config_Globs' 0x48c layout (Config.h:122,129). Overflowing it walks into
    /// listSet[32] -- the config ListSet head pointers -- so the resulting crash
    /// surfaces much later, inside Config_Free, with no link to the long key.
    char* const dst    = configGlobs.s_JoinPath_string;
    const size_t cap   = _countof(configGlobs.s_JoinPath_string);
    size_t       used  = 0;
    bool         truncated = false;

    const size_t firstLen = std::strlen(s);
    if (firstLen < cap) { std::memcpy(dst, s, firstLen); used = firstLen; }
    else                { std::memcpy(dst, s, cap - 1);  used = cap - 1; truncated = true; }
    dst[used] = '\0';

    va_start(args, s);
    while (curr = va_arg(args, const char*)) {
        const size_t sepLen  = std::strlen(CONFIG_SEPARATOR);
        const size_t currLen = std::strlen(curr);
        if (used + sepLen + currLen >= cap) { truncated = true; break; }
        std::memcpy(dst + used, CONFIG_SEPARATOR, sepLen); used += sepLen;
        std::memcpy(dst + used, curr, currLen);            used += currLen;
        dst[used] = '\0';
    }
    va_end(args);

    Error_WarnF2(truncated, "Config: string ID exceeds %i characters and was "
                            "truncated to \"%s\".\n", (sint32)cap, dst);
    return dst;
}
```

(Note the existing `va_end(s)` at `Config.cpp:242` passes the wrong argument — it should be
`va_end(args)`. Harmless on MSVC x86, but it is wrong and the rewrite fixes it in passing.)

**Gate: unconditional.**

---

### F-07 — `Config_GetStringID`: `hierarchy[parent->depth]` with an unbounded, underflowable depth

**Severity: CORRUPTION.** Two defects that compound.

**Location A:** `engine/core/Config.cpp:258-263`. Ours.

```cpp
const char* hierarchy[CONFIG_MAXDEPTH] = { nullptr };
for (const Config* parent = conf; parent != nullptr; parent = Config_GetParentItem(parent)) {
    hierarchy[parent->depth] = parent->itemName;
}
```

`hierarchy` is `[CONFIG_MAXDEPTH]` = 100 (`Config.h:41`). `depth` is `uint32`
(`Config.h:106`) incremented once per `{` in `Config_Load2` (`Config.cpp:180`), with **no
ceiling**. A config file 100 braces deep writes `hierarchy[100]` — one slot past a
100-element stack array — and every brace beyond that walks further up the stack frame.

**Location B:** `engine/core/Config.cpp:163-165`. Ours.

```cpp
Error_WarnF2((conf->depth == 0), "%s (%i): Warning: Config close brace \"%s\" used at depth 0.\n",
             filename, lineNumber, CONFIG_CLOSEBLOCK);
conf->depth--;
```

The warning fires and then **the decrement happens anyway**. `depth` is unsigned, so a
stray `}` at depth 0 sets it to `0xFFFFFFFF`. That value is then copied to every
subsequent child node (`Config.cpp:668`). When `Config_GetStringID` later runs on any such
node, `hierarchy[0xFFFFFFFF]` is a wild write roughly 16 GB below the stack pointer — an
immediate, unattributable access violation.

This is a "warns but continues into undefined behaviour" case: the diagnostic exists and is
useless, because the code proceeds to do the corrupting thing regardless. The comment on
`Config.cpp:162` ("Treat as non-fatal for when we introduce hot-reloading settings")
explains the intent but not the missing clamp.

**Concrete failure scenario.** A modder's hand-edited `Lego.cfg` has one unbalanced `}`
before the first `{`. Every property after it carries `depth == 0xFFFFFFFF`.
`Config_FindItem` (`Config.cpp:758`) then compares `item->depth != count - 1` and never
matches, so *every lookup in the entire file silently returns null* — the game boots with
zero stats, zero weapons, zero priorities, and the only message printed is a single
"close brace used at depth 0" warning near the top of the log.

**Fix (unconditional).** Two edits.

```cpp
/// Config.cpp:163 -- clamp instead of underflowing. `depth` is uint32 and is
/// copied into every child node (Config.cpp:668); 0xFFFFFFFF then makes every
/// Config_FindItem miss and indexes hierarchy[] wildly in Config_GetStringID.
Error_WarnF2((conf->depth == 0), "%s (%i): Warning: Config close brace \"%s\" used "
                                 "at depth 0; ignoring it.\n",
             filename, lineNumber, CONFIG_CLOSEBLOCK);
if (conf->depth > 0) conf->depth--;
```

```cpp
/// Config.cpp:180 -- cap the depth so hierarchy[CONFIG_MAXDEPTH] can never be
/// indexed out of range in Config_GetStringID (Config.cpp:261).
if (c == CONFIG_OPENBLOCKCHAR && cnext == '\0') {
    Error_WarnF2((conf->depth + 1 >= CONFIG_MAXDEPTH),
                 "%s (%i): Warning: Config nesting exceeds max depth of %i; "
                 "further nesting is flattened.\n", filename, lineNumber, CONFIG_MAXDEPTH);
    if (conf->depth + 1 < CONFIG_MAXDEPTH) conf->depth++;
}
```

**Gate: unconditional.** Both replace UB.

---

### F-08 — `Stats_Initialise`: 22 unbounded `Util_Tokenise` calls into `char* argv[32]`

**Severity: CORRUPTION (stack).**

**Location:** `game/object/Stats.cpp:132` declares `char* argv[32]`. It is then filled by
the **unbounded** `Util_Tokenise` at `Stats.cpp:201, 227, 331, 380, 392, 402, 412, 422,
432, 442, 452, 463, 477, 490, 617, 629, 641, 653, 665, 677` — twenty sites, none of which
caps the token count. Ours.

`Util_Tokenise` forwards `count = std::numeric_limits<uint32>::max()` (`Utils.cpp:41-44`),
so it writes one `char*` per separator found, forever.

**The bound that should apply:** 32, the declared size. Note the *reads* are safe:
`argv_minLevels(lvl)` (`Stats.cpp:121`) is `argv[std::min(argcLevels-1, lvl)]` and `lvl`
never exceeds `OBJECT_MAXLEVELS-1` = 15, so the read index is at most 15. It is only the
**write** side that overflows. (The `argcLevels == 0` underflow that this macro would
otherwise permit is *not* reachable: `Config_Load2` only assigns `dataString` at a
non-`'\0'` byte (`Config.cpp:155,177`), so `Util_TokeniseSafe`'s empty-string early-out at
`Utils.cpp:51` never fires here. Stated explicitly because it looks like a bug and is not.)

**What gets clobbered.** `argv` is a stack local of `Stats_Initialise`. Byte 129 onward
overwrites whatever the compiler placed after it — in a Debug|x86 build with `/RTC`, the
guard bytes; in Release|x86, plausibly `prop`, `type`, `id`, or the return address.

**Concrete failure scenario.** A stat line `Lego1::Stats::LargeDigger::RouteSpeed` with 33
colon-separated levels (an easy copy-paste error when someone pads a list to "one per
possible level" and miscounts). The 33rd token pointer is written past the end of `argv`.
In Release the most likely victim is the loop's own `prop` pointer, so the config walk then
resumes from a `Config*` made of a pointer into the config file's text buffer.

**Fix (unconditional).** Mechanical: swap all twenty sites to the bounded variant that
already exists.

```cpp
uint32 argcLevels = Gods98::Util_TokeniseSafe(str, argv, ":", _countof(argv));
Config_WarnLastF(argcLevels >= _countof(argv), config,
    "More than %i levels in Stats %s; extra values ignored.",
    (sint32)_countof(argv) - 1, "RouteSpeed");
```

`Util_TokeniseSafe` (`Utils.cpp:47`) stops writing at `count` and returns the capped index,
so `argv_minLevels` continues to behave identically for every well-formed input.

**Gate: unconditional.** No well-formed config reaches 32 tokens; the only inputs whose
behaviour changes are the ones that currently smash the stack.

The identical swap is needed at the remaining unbounded `Util_Tokenise` sites found by the
sweep (all ours):

| Site | Array | Size |
|---|---|---|
| `game/front/FrontEnd.cpp:790` | `stringParts` | **4** (`FrontEnd.cpp:780`) — see F-09 |
| `game/front/FrontEnd.cpp:392` | `stringParts` | 3 (`FrontEnd.cpp:391`) |
| `game/front/FrontEnd.cpp:4327` | `linkNames` | 15 (`FrontEnd.cpp:4317`) |
| `game/interface/Advisor.cpp:138` | `parts` | 3 (`Advisor.cpp:116`) |
| `game/interface/Advisor.cpp:205` | `parts` | 6 (`Advisor.cpp:186`) |
| `game/interface/Priorities.cpp:55,56` | `stringParts`, `stringPartsSub` | 10, 10 — see F-03 |
| `game/interface/Priorities.cpp:125` | `parts` | 16 |
| `game/audio/SFX.cpp:152` | `sampleNames` | 100 |
| `game/front/Loader.cpp:74` | `stringParts` | 10 |
| `game/GameState.cpp:337,471,492,715,731,747,763` | various | small fixed |
| `engine/gfx/Containers.cpp:540,3080` | `argv` | 20, 10 |
| `engine/gfx/Activities.cpp:31` | `argv` | `ACTIVITY_MAXARGS` |
| `engine/gfx/Lws.cpp:115,350` | `argv` | fixed |

---

### F-09 — `Front_Menu` BMP spec: `strcpy` into `buff[1024]` then unbounded tokenise into a **4**-element array

**Severity: CORRUPTION (stack).** Called out separately from F-08 because the array is only
four entries, making it by far the easiest to overflow.

**Location:** `game/front/FrontEnd.cpp:780-800`. Ours.

```cpp
char* stringParts[4] = { nullptr }; // For once, this is NOT a dummy init
char buff[1024];
...
std::strcpy(buff, filename);
// cfg: filename.bmp[:xPos:yPos[:trans=0/1]]
uint32 numParts = Gods98::Util_Tokenise(buff, stringParts, ":");
```

Five colon-separated fields is enough to write `stringParts[4]`. `std::strcpy` into
`buff[1024]` from a config string is separately unbounded.

The subsequent reads (`FrontEnd.cpp:793-800`) *are* guarded by `numParts >= 3` and
`numParts == 4` — so this one is a pure write overflow, correctly guarded on the read side.
That asymmetry is itself informative: the author checked the count where it was obvious and
missed it where it was implicit.

**Concrete failure scenario.** A Windows path with a drive letter, e.g.
`C:\LRR\Data\menu.bmp:10:20:1`, tokenises to five parts (`C`, `\LRR\...`, `10`, `20`, `1`).
An absolute path in a cfg that expects a relative one silently smashes the stack.

**Fix (unconditional):** `Util_TokeniseSafe(buff, stringParts, ":", _countof(stringParts))`
plus `Util_StrCpy`/length check in place of the raw `strcpy`.

---

### F-10 — `Stats_AddToolTaskType`: `taskTypes[taskCount++]` with no cap

**Severity: CORRUPTION (latent — not reachable today).**

**Location:** `game/object/Stats.cpp:886-890`. Ours.

```cpp
void __cdecl LegoRR::Stats_AddToolTaskType(LegoObject_ToolType toolType, AITask_Type taskType)
{
    ToolStats* toolStats = &statsGlobs.toolStats[toolType];
    toolStats->taskTypes[toolStats->taskCount++] = taskType;
}
```

`taskTypes` is `[STATS_MAXTOOLAITASKS]` = 3 (`Stats.h:30`, `Stats.h:218`). `taskCount` is
at offset 0x0c of the 0x10-byte `ToolStats` (`Stats.h:219`, `assert_sizeof` at
`Stats.h:222`) — so **`taskTypes[3]` *is* `taskCount`**. `toolType` is likewise unchecked
against `LegoObject_ToolType_Count` (11, `GameCommon.h:1016`).

**Why it is latent:** `Stats_Initialise` is called exactly once, from `GameState.cpp:566`,
and the four calls at `Stats.cpp:876-879` use four *different* tool types, so each
`taskCount` reaches 1. `Stats_Initialise` also never resets `toolStats`.

**Why it still matters:** the file's own `_modifiedStatsListSet.Initialise()` at
`Stats.cpp:126` and `Config_Load2`'s comment about "when we introduce hot-reloading
settings" (`Config.cpp:162`) both anticipate re-initialisation. The fourth call to
`Stats_Initialise` in a process would write `taskTypes[3]`, i.e. set `taskCount` to
`AITask_Type_Dig`. `Stats_FindToolFromTaskType` (`Stats.cpp:1266-1278`) then iterates
`j < toolStats->taskCount` over a 3-element array with a garbage bound.

There is also a plain **leak** in the same re-entry scenario: `Stats.cpp:174-181` only
allocates `objectStats[type]` when it is `nullptr`, but `Stats.cpp:195` re-allocates
`objectStats[type][id]` unconditionally, orphaning the previous 16-level block on every
re-run.

**Fix (unconditional).**

```cpp
void __cdecl LegoRR::Stats_AddToolTaskType(LegoObject_ToolType toolType, AITask_Type taskType)
{
    /// DEEPCORE: taskTypes is [STATS_MAXTOOLAITASKS] == 3 and taskCount lives at
    /// offset 0x0c of the 0x10-byte ToolStats (Stats.h:218-222) -- writing
    /// taskTypes[3] IS writing taskCount. Latent today (Stats_Initialise runs
    /// once, GameState.cpp:566) but fatal the moment stats are reloaded.
    if ((uint32)toolType >= (uint32)LegoObject_ToolType_Count) {
        Error_WarnF2(true, "Stats_AddToolTaskType: tool type %i out of range (max %i).\n",
                     (sint32)toolType, (sint32)LegoObject_ToolType_Count);
        return;
    }
    ToolStats* toolStats = &statsGlobs.toolStats[toolType];
    if (toolStats->taskCount >= STATS_MAXTOOLAITASKS) {
        Error_WarnF2(true, "Stats_AddToolTaskType: tool %i already has %i task types.\n",
                     (sint32)toolType, (sint32)STATS_MAXTOOLAITASKS);
        return;
    }
    toolStats->taskTypes[toolStats->taskCount++] = taskType;
}
```

**Gate: unconditional.**

---

### F-11 — `Weapon_Initialise`: array index `i` derived from a second, independent walk

**Severity: SILENT-WRONG-BEHAVIOUR (defensive).**

**Location:** `game/object/Weapons.cpp:51-53` counts; `Weapons.cpp:97-98` indexes.

```cpp
for (const Gods98::Config* item = arrayFirst; item != nullptr; item = Gods98::Config_GetNextItem(item)) {
    weaponGlobs.weaponCount++;
}
...
for (uint32 i = 0; item != nullptr; i++, item = Gods98::Config_GetNextItem(item)) {
    WeaponStats* weaponStats = &weaponGlobs.weaponStatsList[i];
```

The two walks are over the same list with the same stepping function, so today `i` cannot
exceed `weaponCount - 1`. But the allocation size (`Weapons.cpp:55`) and the write index
come from *textually separate* traversals, which is exactly the coupling that breaks when
someone later adds a filter to one loop. Add the invariant as a cheap assert-style warning:

```cpp
for (uint32 i = 0; item != nullptr; i++, item = Gods98::Config_GetNextItem(item)) {
    /// DEEPCORE: weaponStatsList holds exactly weaponCount elements of 0x4b68
    /// (Weapons.cpp:55-56). The count comes from a separate walk at :51-53.
    if (i >= weaponGlobs.weaponCount) {
        Config_WarnItemF(true, item, "WeaponTypes list grew during load at \"%s\"; ignored.",
                         Gods98::Config_GetItemName(item));
        break;
    }
```

**Gate: unconditional** (message + break where the alternative is a heap overflow).

---

## 2. Silent continues and swallowed lookups

### F-12 — `Weapon_GetWeaponIDByName` returns an **out-of-range** sentinel that every caller uses as an index

**Severity: CRASH.** The single worst "silent failure" in the audit, because the failure
path is indistinguishable from success at every call site.

**Location:** `game/object/Weapons.cpp:184-195`. Ours.

```cpp
// On failure, returns g_WeaponTypes_COUNT
// Weapon IDs are 1-indexed it seems...
uint32 __cdecl LegoRR::Weapon_GetWeaponIDByName(const char* weaponName)
{
    for (uint32 weaponID = 0; weaponID < weaponGlobs.weaponCount; weaponID++) {
        if (::_stricmp(weaponGlobs.weaponNameList[weaponID], weaponName) == 0) {
            return weaponID;
        }
    }
    return weaponGlobs.weaponCount + 1; // Invalid weaponID??
}
```

The comment's own `??` is the tell. The sentinel is `weaponCount + 1` — **two elements past
the end** of `weaponStatsList` (`Weapons.cpp:56`, `weaponCount * 0x4b68` bytes). Not one
caller checks it:

| Caller | What it does with the sentinel |
|---|---|
| `Weapons.cpp:1301` | `Weapon_GetRechargeTime(weaponID)` -> `weaponStatsList[weaponID].rechargeTime` (`Weapons.cpp:200`) |
| `Weapons.cpp:1311,1317` | `Weapon_GetWeaponRange(weaponID)` (`Weapons.cpp:212`) |
| `ElectricFence.cpp:754` | `Weapon_GetRechargeTime` for `"FenceSpark"` |
| `ElectricFence.cpp:781` | `Weapon_GenericDamageObject` -> `Weapon_GetDamageForObject` -> `objectCoefs[...]` (`Weapons.cpp:224`) |
| `Object.cpp:5453` | `Weapon_GetDamageForObject` for `"BatAttack"` |

`Weapon_GetDamageForObject` reads at `weaponStatsList[weaponCount+1].objectCoefs[type][id][level]`
— i.e. roughly `2 * 0x4b68` = 38,608 bytes past a valid heap buffer, plus the per-object
offset. `Weapon_GetRechargeTime` reads at `+0x4b0c` of that same phantom element.

**Concrete failure scenario.** A modder's `Lego.cfg` renames `FenceSpark` to `FenceZap`.
Nothing complains at load. The first time a monster touches an electric fence,
`ElectricFence_SparkObject` (`ElectricFence.cpp:781`) calls
`Weapon_GenericDamageObject(liveObj, weaponCount+1, ...)`, which reads `isSlowDeath` and a
damage coefficient from unmapped or unrelated heap. Either an access violation in the
middle of combat, or — worse — a plausible-looking float that makes fences do random
damage. The log is empty.

**Fix.** Two parts.

*(a) Unconditional, message-only in the lookup:*

```cpp
#define WEAPON_ID_INVALID   ((uint32)-1)   // Weapons.h, next to WEAPON_MAXWEAPONS

uint32 __cdecl LegoRR::Weapon_GetWeaponIDByName(const char* weaponName)
{
    for (uint32 weaponID = 0; weaponID < weaponGlobs.weaponCount; weaponID++) {
        if (::_stricmp(weaponGlobs.weaponNameList[weaponID], weaponName) == 0) {
            return weaponID;
        }
    }
    /// DEEPCORE: the original returned weaponCount+1, which is TWO elements past
    /// the end of weaponStatsList (Weapons.cpp:56) and which every caller used
    /// directly as an index. Return an unmistakable sentinel instead.
    Error_WarnF2(true, "Weapon type \"%s\" is not defined in WeaponTypes.\n", weaponName);
    return WEAPON_ID_INVALID;
}
```

*(b) Unconditional guards in the accessors,* so an unchecked caller degrades to a defined
value instead of reading off the heap:

```cpp
real32 __cdecl LegoRR::Weapon_GetRechargeTime(uint32 weaponID)
{
    if (weaponID >= weaponGlobs.weaponCount) return 0.0f;
    return weaponGlobs.weaponStatsList[weaponID].rechargeTime;
}
```

…and the same one-line guard in `Weapon_GetDischargeRate` (`Weapons.cpp:204`),
`Weapon_GetWeaponRange` (`:210`), `Weapon_GetWallDestroyTime` (`:216`),
`Weapon_GetDamageForObject` (`:222`), and `Weapon_GenericDamageObject` (`:234`).

**Gate: unconditional.** Changing the sentinel from "an out-of-bounds index" to "an
out-of-bounds index that is obviously out of bounds" preserves no defined behaviour, and
the accessor guards only alter the path that currently reads unmapped memory.

**Note on the warning:** `Weapon_GetWeaponIDByName` is called from
`Weapon_LegoObject_FUN_...` inside a per-frame update (`Weapons.cpp:1301`). A raw
`Error_WarnF2` there would spam every frame. Follow the `DeepCore.cpp` precedent for
once-only warnings (`DeepCore.cpp:244-262`, `_waterOverflowWarned`): keep a small
DLL-side `std::map<std::string,bool>` of already-reported names. **DLL-side storage only** —
`Weapon_Globs` is `assert_sizeof(Weapon_Globs, 0x1b90)` (`Weapons.h:140`) and must not grow.

---

### F-13 — `Stats_Initialise` skips an unresolved object name; the object is then silently inert

**Severity: SILENT-WRONG-BEHAVIOUR.**

**Location:** `game/object/Stats.cpp:136-141`. Ours.

```cpp
if (!Lego_GetObjectByName(Config_GetItemName(prop), &type, &id, nullptr)) {
    Config_WarnItem(true, prop, "Object name in Stats not found");
    continue;
}
```

This one *does* warn, so it is not silent at the point of failure. The silence is
downstream: nothing else ever notices that the object has no stats. `statsGlobs.objectLevels`
stays 0 for it, so `Stats_GetLevels` (`Stats.cpp:1176`) returns 0, so
`StatsObject_SetObjectLevel` (`Stats.cpp:968`) always returns false, so the object exists
in the world with `liveObj->stats` pointing at whatever it was initialised to. Every
`StatsObject_Get*` accessor (`Stats.cpp:995-1362`) then dereferences it unconditionally.

The same shape applies to the freshly-added guard at `Stats.cpp:143-172` and to F-01's
proposed guard: skipping is right, but "skipped" needs to be visible at the point where it
*matters*, not only at parse time.

**Fix (unconditional, message-only).** After the config walk completes, sweep the table and
report every type/ID combination the engine believes exists but has no stats:

```cpp
/// after the `for (prop = ...)` loop, before Stats_AddToolTaskType at Stats.cpp:876
/// DEEPCORE: message-only. An object with zero levels is not a crash, but every
/// StatsObject_* accessor will happily read whatever liveObj->stats points at, so
/// name the gap once at load instead of never.
for (uint32 t = 0; t < (uint32)LegoObject_Type_Count; t++) {
    if (statsGlobs.objectStats[t] == nullptr) continue;
    const uint32 idCount = std::min(Lego_GetObjectTypeIDCount((LegoObject_Type)t),
                                    (uint32)LegoObject_ID_Count);
    for (uint32 d = 0; d < idCount; d++) {
        Error_WarnF2((statsGlobs.objectLevels[t][d] == 0),
                     "Stats: object type %i id %i has no Stats entry (0 levels).\n",
                     (sint32)t, (sint32)d);
    }
}
```

`Lego_GetObjectTypeIDCount` is an exe macro (`Game.h:1519`, `0x0042ee70`) and is already
used this way at `Object.cpp:253`; the `std::min` is there because it, too, is unbounded.

**Gate: unconditional** — adds a message, cannot change behaviour.

---

## 3. Config parses that accept garbage

### F-14 — The new `Stats.cpp` bounds guard kills the process by default

**Severity: CRASH (self-inflicted).** An `Error_Fatal` where a warning + skip is strictly
better — and where the code's own comment says it intended warning + skip.

**Location:** `game/object/Stats.cpp:165-171` (added in the working tree today). Ours.

The guard's comment reads: *"We both complain AND skip -- fatal visibility is a
runtime-toggleable log level (Errors.h), so the `continue` is what actually guarantees the
corrupting write never happens."*

The premise is wrong for the default configuration:

- `Config_FatalItemF` -> `Config_FatalItem` -> `Error_FatalF2` (`Config.h:343`,
  `Errors.h:111`) calls `Gods98::Error_Out(true, ...)`.
- `Error_Out` with `ErrFatal != 0` calls `Error_TerminateProgram` on **every** return path
  (`Errors.cpp:177, 245, 255, 265, 275, 294`).
- `errorLogLevels` defaults to `{ false, true, true, true, true }` — **`fatalVisible` is
  `true`** (`Errors.cpp:26`).

So with stock settings a 16th monster in `Lego.cfg` now closes the game at load rather than
corrupting `objectLevels`. That is a real improvement over silent corruption, but it is not
what the comment claims and it is the opposite of the precedent this project already set:
`DeepCore::WaterOverflow` (`DeepCore.cpp:248-262`) exists precisely so that hitting a
1999-era fixed limit degrades gracefully instead of crashing to desktop.

**Fix (unconditional).** Change the severity, keep the skip:

```cpp
if ((uint32)type >= (uint32)LegoObject_Type_Count ||
    (uint32)id   >= (uint32)LegoObject_ID_Count)
{
    /// DEEPCORE: WARN, not FATAL. Error_FatalF2 -> Error_Out(true, ...) terminates
    /// on every path (Errors.cpp:177,245,255,265,275,294) and fatalVisible defaults
    /// to true (Errors.cpp:26), so Config_FatalItemF here closes the game at load.
    /// The `continue` alone already guarantees the corrupting write never happens;
    /// this matches the graceful-degradation precedent in DeepCore.cpp:248-262.
    Config_WarnItemF(true, prop,
        "Stats entry \"%s\" resolved to out-of-range indices (type %i, max %i; "
        "id %i, max %i). Writing it would corrupt adjacent memory in the original "
        "executable. Entry skipped.",
        Config_GetItemName(prop),
        (sint32)type, (sint32)LegoObject_Type_Count,
        (sint32)id,   (sint32)LegoObject_ID_Count);
    continue;
}
```

**Gate: unconditional.** Fatal -> warn strictly widens the set of configs that boot; it
cannot make a previously-working config fail.

---

### F-15 — `Config_FatalLast` on `Levels > OBJECT_MAXLEVELS` kills the process for a typo

**Severity: CRASH.** `Error_Fatal` where warn + clamp is better.

**Location:** `game/object/Stats.cpp:185`. Ours.

```cpp
uint32 levels = (uint32)Config_GetIntValue(config, Stats_ID("Levels"));
Config_FatalLast(levels > OBJECT_MAXLEVELS, config, "Cannot have levels greater than maximum in Stats");
```

The check itself is correct and load-bearing — `levels` is the loop bound for every write
into the 16-element `objectStats[type][id]` block (`Stats.cpp:192-196`), and the `(uint32)`
cast means a negative `atoi` result becomes huge and is caught. But the consequence of a
single mistyped digit is the game closing.

**Also in this line:** `Config_GetIntValue` is `Config_GetIntValue2` (`Config.h:323`), which
returns **0** when the key is absent (`Config.cpp:851-858`). A `Stats` block with no
`Levels` key therefore yields `levels == 0` — no fatal, no warning, and an object with an
allocated but entirely zeroed stats block. That is the "missing key defaults to 0 where 0 is
invalid" case the brief asks about, and it is silent.

**Fix.** Two parts.

*(a) Unconditional, message-only:* warn on the missing/zero key.

```cpp
uint32 levels = (uint32)Config_GetIntValue(config, Stats_ID("Levels"));
/// DEEPCORE: Config_GetIntValue2 returns 0 for an absent key (Config.cpp:851-858).
/// levels == 0 makes every write loop below a no-op, producing an object that
/// exists but has no stats at all -- previously with no diagnostic whatsoever.
Config_WarnLast(levels == 0, config, "Stats entry has no Levels (or Levels 0); "
                                     "this object will have no stats at all");
```

*(b) Gated:* fatal -> warn + clamp. This one **does** need a gate, because it changes
defined behaviour: a config that currently terminates the process would begin to load.

```cpp
if (levels > OBJECT_MAXLEVELS) {
    if (DeepCore::SurviveConfigErrors()) {
        Config_WarnLastF(true, config, "Levels %i exceeds the maximum of %i; clamped.",
                         (sint32)levels, (sint32)OBJECT_MAXLEVELS);
        levels = OBJECT_MAXLEVELS;
    }
    else {
        Config_FatalLast(true, config, "Cannot have levels greater than maximum in Stats");
    }
}
```

`DeepCore::SurviveConfigErrors()` is a new gate modelled exactly on
`settings.surviveWaterOverflow` (`DeepCore.hpp`, `DeepCore.cpp:250`): a `bool` in
`DeepCore::Settings`, default `false`, read from `DeepCore.cfg`. It is DLL-side storage
only and touches no `assert_sizeof` struct.

---

### F-16 — `Weapon_Initialise` `SlowDeath`: `Config_FatalItem` on a short value

**Severity: CRASH.** Same class as F-15.

**Location:** `game/object/Weapons.cpp:110-116`. Ours.

```cpp
char* str = Gods98::Util_StrCpy(Gods98::Config_GetDataString(stat));
const uint32 argc = Gods98::Util_TokeniseSafe(str, stringParts, ":", 2);
Config_FatalItem(argc < 2, stat, "Not enough parts in WeaponTypes SlowDeath");

weaponStats->isSlowDeath = true;
weaponStats->slowDeathInitialCoef = (real32)std::atof(stringParts[0]);
weaponStats->slowDeathDuration = (real32)std::atof(stringParts[1]);
```

Note that the fatal is *load-bearing* here in a way F-15's is not: if `fatalVisible` were
ever turned off, execution would fall through to `stringParts[1]`, which
`Util_TokeniseSafe` never wrote — an uninitialised stack pointer handed to `atof`. So this
site is simultaneously "fatal where a skip would do" **and** "silently continues where it
must not". Both need fixing together.

**Fix (unconditional, and it removes a latent UB):**

```cpp
char* str = Gods98::Util_StrCpy(Gods98::Config_GetDataString(stat));
const uint32 argc = Gods98::Util_TokeniseSafe(str, stringParts, ":", 2);
/// DEEPCORE: was Config_FatalItem, which both (a) closes the game over one cfg
/// line and (b) leaves stringParts[1] uninitialised if fatalVisible is off
/// (Errors.h:111 -- the macro is a no-op then, and execution falls through).
if (argc < 2) {
    Config_WarnItemF(true, stat, "WeaponTypes SlowDeath needs 2 colon-separated "
                                 "parts (initialCoef:duration), found %i; ignored.",
                     (sint32)argc);
    Gods98::Mem_Free(str);
    continue;
}
```

---

### F-17 — `atof`/`atoi` applied directly to `Config_GetDataString`, which can be `nullptr`

**Severity: CRASH.**

**Locations (all ours, all in `Weapon_Initialise`):**
`Weapons.cpp:110` (`Util_StrCpy`), `:121`, `:124`, `:127`, `:131`, `:134`, `:141`, `:159`.

```cpp
weaponStats->rechargeTime = (real32)std::atof(Gods98::Config_GetDataString(stat));
```

`Config_GetDataString` is the raw struct field (`Config.h:107`), not a checked accessor.
`Config_Load2` leaves it `nullptr` for a trailing property whose value is missing, and says
so explicitly at `Config.cpp:210-215`:

> `"%s (%i): Warning: Last property \"%s\" missing value string.\n"`

`Util_StrCpy` (`Utils.cpp:100-107`) calls `std::strlen(str)` with no null check, so
`Weapons.cpp:110` and `:159` fault on the same input.

**Concrete failure scenario.** A `Lego.cfg` whose final line is `RechargeTime` with the
value accidentally deleted. The config loads with a warning; `Weapon_Initialise` then calls
`std::atof(nullptr)` and the process faults during startup, several subsystems after the
warning was printed.

**Fix (unconditional).** Add a checked accessor next to the existing ones in `Config.h` and
use it at all eight sites:

```cpp
/// Config.h -- CUSTOM, alongside Config_GetIntValue2 / Config_GetRealValue2.
/// Config_GetDataString is a raw field read and is nullptr for a trailing key
/// with no value (Config.cpp:210-215). Everything downstream assumes a string.
#define Config_GetDataStringOrEmpty(conf) \
    (Gods98::Config_GetDataString((conf)) ? Gods98::Config_GetDataString((conf)) : "")
```

At the two `Util_StrCpy` sites, warn and skip instead:

```cpp
const char* dataStr = Gods98::Config_GetDataString(stat);
if (dataStr == nullptr) {
    Config_WarnItemF(true, stat, "WeaponTypes \"%s\" has no value; ignored.",
                     Gods98::Config_GetItemName(stat));
    continue;
}
char* str = Gods98::Util_StrCpy(dataStr);
```

---

### F-18 — `CollBox` reads `argv[1]` without a count check, from a **freed** allocation

**Severity: CORRUPTION (use-after-free read).**

**Location:** `game/object/Stats.cpp:329-340`. Ours.

```cpp
else if ((str = Config_GetStringValue(config, Stats_ID("CollBox")))) {
    Gods98::Util_Tokenise(str, argv, ",");        // return value DISCARDED
    collBox.width  = (real32)std::atof(argv[0]);
    collBox.height = (real32)std::atof(argv[1]);  // never validated
```

`argv` is the function-scope `char* argv[32]` (`Stats.cpp:132`), reused by every tokenise in
the loop body **and across loop iterations**. Every earlier block frees its string —
`Mem_Free(str)` at `Stats.cpp:207`, `:235`, and so on. So if `CollBox` has a single value
(no comma), `Util_Tokenise` writes only `argv[0]`, and `argv[1]` still holds a pointer into
a heap block that was freed at `Stats.cpp:207` during the `RouteSpeed` parse of *this or an
earlier object*.

**Concrete failure scenario.** `CollBox  12.0` (one value where two are expected).
`std::atof` reads freed heap. With MSVC's debug allocator the freed block is `0xDDDDDDDD`
fill, so `atof` reads garbage and `collBox.height` becomes 0 — meaning `collRadius` is
`max(12.0, 0.0) * 0.5 = 6.0` instead of the intended value, and the unit's collision box is
silently wrong for the whole game. In Release the block may have been reused by another
allocation, producing a different wrong number every run. Nothing is printed.

**Fix (unconditional).**

```cpp
else if ((str = Config_GetStringValue(config, Stats_ID("CollBox")))) {
    /// DEEPCORE: the return value was discarded and argv[1] read unconditionally.
    /// argv is function-scope (Stats.cpp:132) and reused across iterations, and
    /// every earlier block Mem_Free()s its string (e.g. Stats.cpp:207) -- so a
    /// one-value CollBox previously read a dangling pointer from a freed block.
    const uint32 argcBox = Gods98::Util_TokeniseSafe(str, argv, ",", _countof(argv));
    if (argcBox < 2) {
        Config_WarnLastF(true, config, "Stats CollBox needs 2 comma-separated values, "
                                       "found %i; CollBox ignored.", (sint32)argcBox);
    }
    else {
        collBox.width  = (real32)std::atof(argv[0]);
        collBox.height = (real32)std::atof(argv[1]);
        collRadius = std::max(collBox.width, collBox.height) * 0.5f;
        flags1 |= StatsFlags1::STATS1_COLLBOX;
    }
    Gods98::Mem_Free(str);
}
```

---

### F-19 — `Advisor` and `Priorities` position parsers read fixed token slots with no count check

**Severity: CRASH / SILENT-WRONG-BEHAVIOUR.** Same shape as F-18, three more sites.

| Location | Reads | Array | Guard |
|---|---|---|---|
| `game/interface/Advisor.cpp:140-146` | `parts[0]`, `parts[1]`, `parts[2]` | `char* parts[3]` (`Advisor.cpp:116`) | none |
| `game/interface/Advisor.cpp:207-215` | `parts[0]`..`parts[5]` | `char* parts[6]` (`Advisor.cpp:186`) | none |
| `game/interface/Priorities.cpp:125-127` | `parts[0]`, `parts[1]` | `char* parts[16]` | none |

In each case the tokenise return value is discarded and the slots are read regardless.
`Advisor.cpp:143` hands `parts[0]` to `Container_Load` and `parts[1..2]` to `atof`;
`Advisor.cpp:208-215` hands `parts[1]`, `parts[2]`, `parts[5]` to `_stricmp` /
`Text_GetTextType` / `SFX_GetType` / `Panel_GetPanelType`. A short line means `_stricmp` on
an uninitialised stack pointer.

**Fix (unconditional):** capture the `Util_TokeniseSafe` count and early-out with
`Config_WarnItemF` when it is below the number of slots the block reads. Identical shape to
F-18's sketch.

---

### F-20 — `Config_FindItem`'s max-depth guard is dead code

**Severity: SILENT-WRONG-BEHAVIOUR.**

**Location:** `engine/core/Config.cpp:743-748`. Ours.

```cpp
const uint32 count = Util_TokeniseSafe(tempstring, argv, CONFIG_SEPARATOR, CONFIG_MAXDEPTH);
if (count > CONFIG_MAXDEPTH) {
    Mem_Free(tempstring);
    Error_WarnF2(true, "%s: Warning: StringID exceeds max depth of %i \"%s\".\n", ...);
    return nullptr;
}
```

`Util_TokeniseSafe` increments `index` only while `index < count` (`Utils.cpp:57,61`) and
returns `index`, so its return value **can never exceed** the `count` argument. The
condition `count > CONFIG_MAXDEPTH` is therefore always false: the warning never fires, and
a string ID with more than 100 `::` segments is silently truncated to its first 100 and
matched against the wrong hierarchy.

**Fix (unconditional, message-only).** Either compare with `==` and re-scan, or —
simpler and honest — tokenise with one slot of headroom:

```cpp
char* argv[CONFIG_MAXDEPTH + 1];
uint32 hashv[CONFIG_MAXDEPTH + 1];
char* tempstring = Util_StrCpy(stringID);
/// DEEPCORE: Util_TokeniseSafe caps its own return at the count argument
/// (Utils.cpp:57,61), so the original `count > CONFIG_MAXDEPTH` test could never
/// be true and over-deep IDs were silently truncated. Tokenise with one slot of
/// headroom so the overflow is actually observable.
const uint32 count = Util_TokeniseSafe(tempstring, argv, CONFIG_SEPARATOR, CONFIG_MAXDEPTH + 1);
if (count > CONFIG_MAXDEPTH) { ... }
```

---

### F-21 — `Stats` "not enough levels" warnings name the object instead of the stat

**Severity: SILENT-WRONG-BEHAVIOUR (diagnostic).**

**Location:** `game/object/Stats.cpp:228`. Ours.

```cpp
Config_WarnLastF(argcLevels < levels, config, "Not enough levels in Stats %s",
                 Gods98::Config_GetItemName(prop));
```

`prop` is the **object** item (e.g. `LargeDigger`), not the drill-time key. Every one of the
six drill-time stats therefore reports `"Not enough levels in Stats LargeDigger"`, giving
the reader no way to tell whether `SoilDrillTime` or `HardDrillTime` was short. The sibling
warnings at `Stats.cpp:202`, `:381`, `:393` correctly hardcode the stat name.

**Fix (unconditional, message-only):** use `drillTimeNames[i]`, which is in scope
(`Stats.cpp:211-218`).

```cpp
Config_WarnLastF(argcLevels < levels, config, "Not enough levels in Stats %s for %s",
                 drillTimeNames[i], Gods98::Config_GetItemName(prop));
```

---

### F-22 — Zero-means-default conflated with zero-means-absent, ~25 times

**Severity: SILENT-WRONG-BEHAVIOUR.**

**Location:** `game/object/Stats.cpp:287-288` and `:778-846`. Ours.

```cpp
if (rubbleCoef == 0.0f) rubbleCoef = 1.0f;
if (pathCoef   == 0.0f) pathCoef   = 1.0f;
...
real32 flocks_Turn = Config_GetRealValue(config, Stats_ID("Flocks_Turn"));
if (flocks_Turn == 0.0f) flocks_Turn = 0.06f;
```

`Config_GetRealValue2` returns 0.0 both when the key is absent **and** when the modder
explicitly wrote `0`. So `RubbleCoef 0` (a legitimate "instant rubble clearing") silently
becomes `1.0`, and `Flocks_Turn 0` silently becomes `0.06`. This is faithful to the 1999
behaviour and is *not* worth changing — but it is worth naming, because the identical
pattern appears in `DeepCore.cpp:322-342`, where the project already chose the better
approach:

```cpp
if (Gods98::Config_FindItem(config, DeepCore_ID("WaveIntervalSeconds")) != nullptr) { ... }
```

**Recommendation: no change to `Stats.cpp`.** Changing these would alter defined behaviour
for every existing mod. Documented here so it is not re-discovered as a bug. If a future
DeepCore stat wants a real "explicit zero", it should use the `Config_FindItem` presence
test the settings layer already uses.

---

### F-23 — `Error_Format` / `Error_Out` `vsprintf` into 1024-byte buffers with config-supplied strings

**Severity: CORRUPTION.** The reporting path itself is a buffer overflow, so the more
diagnostics we add (F-01, F-02, F-13…), the more this matters.

**Location:** `engine/core/Errors.cpp:153-163` and `:171-174`, `:187-193`. Ours.

```cpp
const char* __cdecl Gods98::Error_Format(const char* msg, ...)
{
    static char res[1024];
    std::vsprintf(res, msg, args);   // unbounded
    return res;
}
```

Every `Config_WarnItemF` / `Config_FatalItemF` routes through `Error_Format`
(`Config.h:347-348`) and then through `Error_Out`, which does the same thing twice more into
`char msg[1024]` (`Errors.cpp:171`) and `char achBuffer[1024]` (`Errors.cpp:187`). The
format arguments include config file names, item names, and data strings — all
attacker-length. `Error_Format`'s buffer is `static`, so overflowing it corrupts the
DLL's own data segment rather than the stack; `Error_Out`'s two are stack arrays.

There is a fourth: `Errors.cpp:284` does `::wsprintf(lpszSharedMem + sizeof(DWORD), "%s",
achBuffer)` into a `MapViewOfFile` of **512** bytes (`Errors.cpp:269`) from a buffer of up
to 1024.

**Fix (unconditional, mechanical):** `std::vsnprintf(res, _countof(res), msg, args)` at all
three `vsprintf` sites, and `::wnsprintf` / an explicit 512-cap at `Errors.cpp:284`.
`vsnprintf` is available in the v142 toolset and is a drop-in that adds no warning.

---

## 4. Findings ranked

| # | Severity | Site | One line |
|---|---|---|---|
| **F-01** | CORRUPTION | `Weapons.cpp:152-166` | `objectCoefs[objType][objID][objLevel]` unbounded — the ground-truth bug, still unfixed, in the file next door |
| **F-02** | CORRUPTION | `Priorities.cpp:92-110` | uncapped `count` into nine `[27]` arrays; index 27 of `initialOff` **is** `count` |
| **F-04** | CORRUPTION | `SFX.cpp:200` | `sampleGroupTable[sampleGroupCount++]` uncapped; overflows `sfxGlobs` into `statsGlobs` |
| **F-03** | CORRUPTION | `Priorities.cpp:54-61` | `strcpy` into `buff[512]`, unbounded tokenise, `stringParts[1..3]` unchecked |
| **F-06** | CORRUPTION | `Config.cpp:225-245` | unbounded `strcpy`/`strcat` into `s_JoinPath_string[1024]` -> `configGlobs.listSet[]` |
| **F-05** | CORRUPTION | `SFX.cpp:167-175` | `while (*s != '#') *v++ = *s++;` — unbounded both ways |
| **F-07** | CORRUPTION | `Config.cpp:163-165`, `:258-263` | `depth--` underflow to `0xFFFFFFFF`; `hierarchy[depth]` uncapped |
| **F-12** | CRASH | `Weapons.cpp:184-195` + 5 call sites | failure sentinel is `weaponCount + 1`, used unchecked as an index |
| **F-08** | CORRUPTION | `Stats.cpp:132` + 20 sites | unbounded `Util_Tokenise` into `char* argv[32]` |
| **F-09** | CORRUPTION | `FrontEnd.cpp:780-790` | `strcpy` into `buff[1024]`, unbounded tokenise into a **4**-slot array |
| **F-18** | CORRUPTION | `Stats.cpp:329-340` | `CollBox` reads `argv[1]` unchecked — a pointer into freed heap |
| **F-23** | CORRUPTION | `Errors.cpp:153-163`, `:171`, `:187`, `:284` | `vsprintf` into 1024-byte buffers from config strings |
| **F-17** | CRASH | `Weapons.cpp:110,121,124,127,131,134,141,159` | `atof`/`atoi`/`strlen` on a `Config_GetDataString` that can be `nullptr` |
| **F-19** | CRASH | `Advisor.cpp:140,207`; `Priorities.cpp:125` | fixed token slots read with no count check |
| **F-14** | CRASH | `Stats.cpp:165-171` | today's new guard is `FATAL`; `fatalVisible` defaults true, so it closes the game |
| **F-16** | CRASH | `Weapons.cpp:112` | `Config_FatalItem` on short `SlowDeath`; and falls through to uninit `stringParts[1]` if fatals are off |
| **F-15** | CRASH | `Stats.cpp:185` | `Config_FatalLast` on `Levels`; plus missing key silently means 0 |
| **F-10** | CORRUPTION (latent) | `Stats.cpp:886-890` | `taskTypes[taskCount++]` uncapped; `taskTypes[3]` **is** `taskCount` |
| **F-13** | SILENT-WRONG | `Stats.cpp:136-141` | skipped objects are never reported as statless downstream |
| **F-20** | SILENT-WRONG | `Config.cpp:743-748` | max-depth guard is dead code; over-deep IDs silently truncate |
| **F-21** | SILENT-WRONG | `Stats.cpp:228` | drill-time warning prints the object name, not the stat name |
| **F-11** | SILENT-WRONG | `Weapons.cpp:97-98` | write index and allocation size come from two separate walks |
| **F-22** | SILENT-WRONG | `Stats.cpp:287-288`, `:778-846` | absent key and explicit `0` are indistinguishable (~25 stats) — **no change recommended** |

### Gating summary

| Gate | Findings |
|---|---|
| **Unconditional** (guards against UB, or message-only) | F-01, F-02, F-03, F-04, F-05, F-06, F-07, F-08, F-09, F-10, F-11, F-12, F-13, F-14, F-16, F-17, F-18, F-19, F-20, F-21, F-23, and part (a) of F-15 |
| **Behind a new `DeepCore::SurviveConfigErrors` gate** | part (b) of F-15 only — fatal -> warn + clamp on `Levels`, because that alters defined behaviour (a config that currently terminates would begin to load) |
| **No change** | F-22 |

That ratio is the point: **almost everything here is a guard against undefined behaviour, and
guards against UB do not need a gate.** The project's existing gate precedent
(`SurviveWaterOverflow`, `DeepCore.cpp:248-262`) is for cases where the vanilla behaviour is
*defined* and we are choosing to deviate. Out-of-bounds writes are not defined behaviour, so
there is nothing to preserve and nothing to bisect.

---

## 5. What could not be determined

- **Whether `Lego_GetObjectByName` can actually return an out-of-range ID.** It is an exe
  address macro (`Game.h:1511`, `0x0042e7e0`) with no disassembly in-tree. The counters that
  size its tables (`legoGlobs.rockMonsterCount` etc., `Game.h:1bc-1cc`) are plain `uint32`
  filled from config by exe code, and nothing on our side caps them. Every guard proposed
  above is therefore written to be correct whether or not the exe already clamps —
  a guard that never fires costs one compare.
- **Whether `Lego_GetObjectTypeIDCount` (`Game.h:1519`, `0x0042ee70`) clamps to 15.**
  `Object.cpp:253` already trusts it. F-13's sketch wraps it in `std::min` regardless.
- **What the exe's `Dependencies_Initialise` and `Upgrade_Load` do with the same indices.**
  Not implemented in-tree (§0.2). `dependencyGlobs` is 0xe5b4 at `0x004b9bc8`, immediately
  before `effectGlobs` (`0x004c8180`), so an overflow there would land in `effectGlobs` —
  unverifiable and unfixable without implementing the function.
- **Runtime confirmation of any scenario.** We cannot run the game. Every "concrete failure
  scenario" above is derived from the code and `docs/address-map.json`, never observed.
- **Warning-count impact.** The build contract is 0 errors / exactly 44 warnings
  (Debug|x86 and Release|x86, v142). Every sketch above uses explicit casts and `_countof`
  to avoid signed/unsigned and truncation warnings, but **that is a design intent, not a
  measurement** — none of these edits has been compiled.

---

## 6. DECISION — the ordered list to fix first

Grouped into three landings, each independently compilable and independently reviewable.

### Landing 1 — the ground-truth bug class, finished (do this first)

The guard that landed today fixed one of two identical sites. Finish the job before doing
anything else.

1. **F-01** — bounds-check `objType`/`objID` in `Weapon_Initialise` (`Weapons.cpp:152-166`),
   and clamp `objLevelCount` with `std::min(..., OBJECT_MAXLEVELS)`.
2. **F-14** — change the new `Stats.cpp:165` guard from `Config_FatalItemF` to
   `Config_WarnItemF`. One-token edit; without it, the very config the guard exists to
   tolerate closes the game.
3. **F-02** — cap `prioritiesGlobs.count` in `Priorities_LoadLevel` (`Priorities.cpp:92`).
4. **F-04** — cap `sampleGroupCount` in `SFX_LoadSampleProperty` (`SFX.cpp:200`).
5. **F-10** — cap `taskCount` and `toolType` in `Stats_AddToolTaskType` (`Stats.cpp:886`).

Rationale for this order: 1-2 close the exact hole the brief was written about; 3-5 are the
only other `table[uncapped_counter]` writes into `assert_sizeof` structs in the tree, and
each is a five-line change with no behavioural surface.

### Landing 2 — the parsing infrastructure everything else sits on

6. **F-08** — mechanical swap of all 20 `Util_Tokenise` -> `Util_TokeniseSafe` in
   `Stats.cpp`, then the sweep table's remaining sites.
7. **F-06** — bounded `Config_BuildStringID`. Fixes the one overflow that can reach the
   config ListSet pointer table, and it is on the path of *every* `Stats_ID` call.
8. **F-07** — clamp `depth` on both the `{` and `}` paths in `Config_Load2`.
9. **F-23** — `vsprintf` -> `vsnprintf` in `Errors.cpp`. Do this **before** landing 3, so the
   new diagnostics added there cannot themselves overflow.
10. **F-03**, **F-09**, **F-05** — the three remaining fixed-buffer `strcpy`/copy loops.
11. **F-12** — `WEAPON_ID_INVALID` sentinel plus one-line guards in the six accessors.
12. **F-17**, **F-16**, **F-18**, **F-19** — the unchecked-token and `nullptr`-data-string
    reads. Grouped because they share one fix shape.

### Landing 3 — diagnostics and the one gated behaviour change

13. **F-13** — the post-load "object has 0 levels" sweep.
14. **F-20**, **F-21**, **F-11** — dead guard, wrong warning text, defensive index check.
15. **F-15(a)** — warn on a missing/zero `Levels` key. Message-only, unconditional.
16. **F-15(b)** — add `DeepCore::settings.surviveConfigErrors` (default `false`, exactly
    mirroring `surviveWaterOverflow` at `DeepCore.cpp:250`) and use it to turn the `Levels`
    fatal into warn + clamp. **This is the only gated item in the entire audit.**

### Explicitly not doing

- **F-22** — leave the zero-means-default conflation alone. Changing it would alter defined
  behaviour for every existing mod, and the settings layer already models the better
  approach for new keys (`DeepCore.cpp:322-342`).
- Anything in `Dependencies.cpp`, `Upgrade.cpp`, or `LegoObject_LoadObjTtsSFX` — those are
  exe address macros, not our code (§0.2).
- Any change to a struct carrying `assert_sizeof`. The once-only-warning bookkeeping that
  F-12 needs goes in DLL-side storage, following the `PowerGrid` precedent at
  `Game.cpp:168-170` and the `_waterOverflowWarned` map at `DeepCore.cpp:245`.
