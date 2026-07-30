# Silent Failures — Ready-to-Apply Fix Plan

Re-verification of `docs/research/silent-failures.md` against the working tree at commit
`ea63ee1` (clean, `git status` empty), plus an independent sweep for what that audit missed.

**Compile-only analysis. We cannot run the game.** Every failure scenario below is derived
from source and `docs/address-map.json`, never observed. Nothing here is play-tested, and
none of these patches has been compiled — the build contract (0 errors / exactly 44 warnings,
Debug|x86 and Release|x86, v142) is a *constraint on the patches*, not a measurement of them.
Each patch is written with explicit casts and `_countof` for that reason; see §6.

---

## 0. Verification summary

### 0.1 Already fixed — confirmed, not re-proposed

| Finding | Where it landed | Status |
|---|---|---|
| **F-01** `Weapon_Initialise` bounds check | `game/object/Weapons.cpp:175-186` (commit `83a21d8`) | Guard present and correct — but see **A-02** and **A-03**, two residual defects *inside* it |
| **Stats_Initialise** bounds check | `game/object/Stats.cpp:162-172` (commit `eae7f75`) | Guard present and correct — but see **A-01**, it is `FATAL` |
| **F-04** `sampleGroupTable` cap | `game/audio/SFX.cpp:223-229` (commit `4c4f545`) | Fully fixed. Nothing further. |

### 0.2 Line numbers that moved

The audit was written against a mid-edit tree. Corrected anchors:

| Audit said | Actually is now | Note |
|---|---|---|
| F-02 `Priorities.cpp:92-110` | `Priorities.cpp:90-108` | shifted −2 |
| F-03 `Priorities.cpp:45-66` / image reads `:59-61` | `Priorities.cpp:43-69`, `strcpy` at `:54`, reads at **`:64-66`** | reads moved +5 |
| F-04 `SFX.cpp:200` | `SFX.cpp:231` (guard now at `:223`) | fixed |
| F-05 `SFX.cpp:142`, `:167-175` | `SFX.cpp:142`, **`:164-175`** | unchanged code, `#` test at `:164` |
| F-12 `Weapons.cpp:184-195` | **`Weapons.cpp:216-227`**; accessors `:230,236,242,248,254,266` | shifted +32 |
| F-16 `Weapons.cpp:110-116` | **`Weapons.cpp:110-119`** | `Config_FatalItem` at `:113` |
| F-17 `Weapons.cpp:110,121,…,159` | **`:111, 122, 125, 128, 132, 135, 142, 191`** | |
| F-11 `Weapons.cpp:51-53` / `:97-98` | `Weapons.cpp:52-54` / **`:98-99`** | |
| F-10 `Stats.cpp:886-890` | `Stats.cpp:885-890`, write at **`:889`** | |
| F-06, F-07, F-20, F-23, F-08, F-09, F-18, F-19, F-13, F-14, F-15, F-21, F-22 | unchanged | |

### 0.3 Two audit claims that are wrong and are corrected here

1. **F-02 says "nine parallel `[27]` arrays".** It is **four**: `buttonTypes`, `initialTypes`,
   `initialValues`, `initialOff` (`Priorities.cpp:92,93,95,100,104`). The offset table in the
   audit is nonetheless correct, including the one that matters — `initialOff[27]` *is* `count`.
2. **F-07 Location A (`Config_GetStringID`) is dead code.** `Gods98::Config_GetStringID` has
   **zero callers** in the tree: `Config.cpp:254` (definition) and `Config.h:196` (declaration)
   are the only two occurrences. Its `hierarchy[CONFIG_MAXDEPTH]` write and its own unbounded
   `strcpy`/`strcat` into `s_stringID[1024]` (`Config.cpp:266-271`) are therefore unreachable
   today. F-07's severity collapses onto **Location B only** (`Config.cpp:165`), which is real
   and on every config load. Ranked accordingly (item 19, not item 7).

### 0.4 A reachability fact the audit did not state

`Weapon_Initialise` is **not hooked**. `interop.cpp:4345` is commented out:

```cpp
//result &= hook_write_jmpret(0x0046ee40, LegoRR::Weapon_Initialise);
```

Our implementation is still reached, but only because `Lego_Initialise` — which is ours, and
is installed as `mainState->Initialise` at `Game.cpp:73,78,113` rather than hooked over an exe
address — calls it directly at `GameState.cpp:571`. **Consequence:** if anything ever calls
`0x0046ee40` from exe code, it gets the 1999 implementation with none of these guards. Nothing
in-tree does. Leave the hook commented out; re-enabling it is a separate decision with no
trampoline to fall back on.

### 0.5 Ownership

Every file patched below is **OURS** (real C++ with a hooked body): `Stats.cpp`, `Weapons.cpp`,
`Priorities.cpp`, `SFX.cpp`, `Config.cpp`, `Errors.cpp`, `Advisor.cpp`, `Pointers.cpp`,
`Effects.cpp`, `FrontEnd.cpp`, `TextMessages.cpp`, `Utils.cpp`.

The index *sources* are **EXE ADDRESS MACROS** and cannot be inspected or fixed:
- `Lego_GetObjectByName` — `game/Game.h:1511`, `0x0042e7e0`
- `Lego_GetObjectTypeIDCount` — `game/Game.h:1519`, `0x0042ee70`
- the counters that size their tables — `legoGlobs.vehicleCount/miniFigureCount/rockMonsterCount/buildingCount`, `game/Game.h:553-556`, plain `uint32`, filled by exe code from `Lego.cfg`, with no ceiling anywhere on our side.

### 0.6 Indentation — read before pasting

This tree is **not uniformly tabbed**. Per-file, measured:

| File | Region being patched | Indent |
|---|---|---|
| `Priorities.cpp`, `Weapons.cpp`, `Config.cpp`, `Errors.cpp`, `Advisor.cpp`, `Utils.cpp`, `Loader.cpp`, `GameState.cpp` | all | **TAB** |
| `Stats.cpp` | `:162-172` (DeepCore guard), `:211-237`, `:885-890` | **TAB** |
| `Stats.cpp` | `:174-208`, `:326-341` (upstream body) | **4 SPACES** |
| `SFX.cpp` | `SFX_LoadSampleProperty` (`:140-260`) | **4 SPACES** |
| `FrontEnd.cpp` | all patched regions | **TAB** |
| `TextMessages.cpp`, `Pointers.cpp`, `Effects.cpp` | all | **TAB** |

Every OLD block below is a byte-exact copy of the current file. Every NEW block uses the same
indent character as the OLD block it replaces. Verify after applying:

```sh
git diff --check          # trailing whitespace / mixed indent
python tools/addrlint/addrlint.py --check
```

---

## 1. The ordered plan

Ordered by **severity × reachability**. "Reachability" tiers used:

- **R1 — mandatory startup.** Runs on every launch from `Lego_Initialise` (`GameState.cpp`).
- **R2 — normal content load.** Runs on every menu/level/sound load.
- **R3 — gameplay runtime.** Needs a specific in-game event.
- **R4 — latent.** Not reachable in the current call graph.

| # | ID | Severity | Reach | Site | Gate |
|---|---|---|---|---|---|
| 1 | **A-01** | CRASH (self-inflicted) | R1 | `Stats.cpp:165` | uncond |
| 2 | **A-02** | CRASH (self-inflicted) | R1 | `Weapons.cpp:178` | uncond |
| 3 | **A-03** | CORRUPTION | R1 | `Weapons.cpp:188,197` | uncond |
| 4 | **F-06** | CORRUPTION | R1 | `Config.cpp:225-245` | uncond |
| 5 | **F-23** | CORRUPTION | R1 | `Errors.cpp:159,174,192,285,308` | uncond |
| 6 | **F-08** | CORRUPTION (stack) | R1 | `Stats.cpp:132` + 20 sites | uncond |
| 7 | **F-02** | CORRUPTION | R2 | `Priorities.cpp:90-108` | uncond |
| 8 | **F-03** | CORRUPTION | R2 | `Priorities.cpp:54-66` | uncond |
| 9 | **A-04** | CORRUPTION | R2 | `Pointers.cpp:117,139` | uncond |
| 10 | **A-05** | CORRUPTION | R2 | `Effects.cpp:144` | uncond |
| 11 | **F-09** | CORRUPTION (stack) | R2 | `FrontEnd.cpp:786,790` | uncond |
| 12 | **A-06** | CORRUPTION | R2 | `FrontEnd.cpp:4327-4336` | uncond |
| 13 | **A-07** | CORRUPTION (stack) | R2 | `FrontEnd.cpp:392` | uncond |
| 14 | **F-05** | CORRUPTION | R2 | `SFX.cpp:164-175` | uncond |
| 15 | **F-18** | CORRUPTION (UAF read) | R1 | `Stats.cpp:329-341` | uncond |
| 16 | **F-19** | CRASH | R2 | `Advisor.cpp:138,205`; `Priorities.cpp:125` | uncond |
| 17 | **A-08** | CORRUPTION | R2 | `TextMessages.cpp:168` | uncond |
| 18 | **F-12** | CRASH | R3 | `Weapons.cpp:226` + 6 accessors | uncond |
| 19 | **F-17** | CRASH | R1 | `Weapons.cpp:111,122,…,191` | uncond |
| 20 | **F-16** | CRASH + latent UB | R1 | `Weapons.cpp:113` | uncond |
| 21 | **F-07B** | SILENT-WRONG (total) | R1 | `Config.cpp:165` | uncond |
| 22 | **F-15a** | SILENT-WRONG | R1 | `Stats.cpp:184` | uncond |
| 23 | **F-15b** | CRASH | R1 | `Stats.cpp:185` | **GATED** |
| 24 | **F-13** | SILENT-WRONG | R1 | after `Stats.cpp:874` | uncond |
| 25 | **F-21** | diagnostic | R1 | `Stats.cpp:228` | uncond |
| 26 | **F-20** | SILENT-WRONG | R1 | `Config.cpp:740-748` | uncond |
| 27 | **F-10** | CORRUPTION | R4 | `Stats.cpp:885-890` | uncond |
| 28 | **F-11** | defensive | R1 | `Weapons.cpp:98-99` | uncond |
| 29 | **F-07A** | dead code | — | `Config.cpp:254-274` | **do not fix** |
| 30 | **F-22** | by design | — | `Stats.cpp:287-288`, `:778-846` | **no change** |

---

## 2. The patches

### 1. A-01 — the `Stats_Initialise` guard is `FATAL`, so it closes the game

**Status: NEW (consequence of the shipped fix). Severity: CRASH. Reach: R1.**
**Location:** `game/object/Stats.cpp:165-170`. OURS. Indent: **TAB**.

`Config_FatalItemF` → `Config_FatalItem` (`Config.h:343`) → `Error_FatalF2` (`Errors.h:111`)
→ `Gods98::Error_Out(true, …)`, which calls `Error_TerminateProgram` on **every** return path
(`Errors.cpp:177, 246, 256, 266, 276, 295`). `errorLogLevels` defaults to
`{ false, true, true, true, true }` — `fatalVisible` is **`true`** (`Errors.cpp:26`).

The guard's own comment (`Stats.cpp:159-161`) says it "complains AND skip[s]" and that the
`continue` is what guarantees safety. With stock log levels the `continue` is unreachable: the
process exits inside `Error_Out` first. So a `Lego.cfg` with a 16th `RockMonsterTypes` entry —
the exact config the guard exists to survive — now **closes the game at load**. This is the
opposite of the precedent the project already set at `DeepCore.cpp:377-391`
(`DeepCore::WaterOverflow`), which exists so a 1999-era fixed limit degrades gracefully.

Note this is not "fatal is safer". A `continue` after a *warning* is strictly safer than a
fatal, because it prevents the write **and** keeps working when someone turns fatals off.

**OLD** (`Stats.cpp:165-170`):
```
			Config_FatalItemF(true, prop,
				"Stats entry \"%s\" resolved to out-of-range indices (type %i, max %i; id %i, max %i). "
				"Writing it would corrupt adjacent memory in the original executable. Entry skipped.",
				Config_GetItemName(prop),
				(sint32)type, (sint32)LegoObject_Type_Count,
				(sint32)id,   (sint32)LegoObject_ID_Count);
```

**NEW:**
```
			/// DEEPCORE: WARN, not FATAL. Error_FatalF2 -> Error_Out(true, ...) terminates on
			/// every path (Errors.cpp:177,246,256,266,276,295) and fatalVisible defaults to
			/// true (Errors.cpp:26), so a fatal here closes the game over one config line --
			/// precisely the line this guard exists to survive. The `continue` below already
			/// guarantees the corrupting write never happens, and it keeps working if fatals
			/// are ever switched off. Matches DeepCore::WaterOverflow (DeepCore.cpp:377-391).
			Config_WarnItemF(true, prop,
				"Stats entry \"%s\" resolved to out-of-range indices (type %i, max %i; id %i, max %i). "
				"Writing it would corrupt adjacent memory in the original executable. Entry skipped.",
				Config_GetItemName(prop),
				(sint32)type, (sint32)LegoObject_Type_Count,
				(sint32)id,   (sint32)LegoObject_ID_Count);
```

Also delete the now-false claim in the comment above it — **OLD** (`Stats.cpp:159-161`):
```
		/// corruption, so there is no vanilla semantics worth preserving, and any config
		/// that reaches here is already broken. We both complain AND skip -- fatal
		/// visibility is a runtime-toggleable log level (Errors.h), so the `continue` is
		/// what actually guarantees the corrupting write never happens.
```
**NEW:**
```
		/// corruption, so there is no vanilla semantics worth preserving, and any config
		/// that reaches here is already broken. We warn and skip: warn visibility is a
		/// runtime-toggleable log level (Errors.h), so the `continue` is what actually
		/// guarantees the corrupting write never happens.
```

**Gate: unconditional.** Fatal → warn strictly widens the set of configs that boot; it cannot
make a previously-working config fail.

**Risk to legitimate behaviour:** none. The only inputs whose behaviour changes are ones that
currently terminate the process.

---

### 2. A-02 — the shipped F-01 guard has the identical `FATAL` defect

**Status: NEW (consequence of the shipped fix). Severity: CRASH. Reach: R1.**
**Location:** `game/object/Weapons.cpp:178-184`. OURS. Indent: **TAB**.

`Weapon_Initialise` runs from `GameState.cpp:571` on every launch. Same analysis as A-01.
The audit's own F-01 sketch used `Config_WarnItemF`; the commit used `Config_FatalItemF`.

**OLD** (`Weapons.cpp:178-184`):
```
								Config_FatalItemF(true, stat,
									"WeaponTypes object coef \"%s\" resolved to out-of-range indices "
									"(type %i, max %i; id %i, max %i). Writing it would corrupt the "
									"weapon coefficient table. Entry skipped.",
									Gods98::Config_GetItemName(stat),
									(sint32)objType, (sint32)LegoObject_Type_Count,
									(sint32)objID,   (sint32)LegoObject_ID_Count);
```

**NEW:**
```
								/// DEEPCORE: WARN, not FATAL -- see Stats.cpp. Error_FatalF2
								/// terminates the process on every path (Errors.cpp:177 ff.)
								/// and fatalVisible defaults to true (Errors.cpp:26). The
								/// `continue` is what prevents the write; a fatal would just
								/// close the game over one WeaponTypes line.
								Config_WarnItemF(true, stat,
									"WeaponTypes object coef \"%s\" resolved to out-of-range indices "
									"(type %i, max %i; id %i, max %i). Writing it would corrupt the "
									"weapon coefficient table. Entry skipped.",
									Gods98::Config_GetItemName(stat),
									(sint32)objType, (sint32)LegoObject_Type_Count,
									(sint32)objID,   (sint32)LegoObject_ID_Count);
```

And correct the stale comment — **OLD** (`Weapons.cpp:171-174`):
```
							/// Unconditional, not gated: the behaviour it replaces is memory
							/// corruption, so there is no vanilla semantics worth keeping. We
							/// complain AND skip, because fatal visibility is a runtime-toggleable
							/// log level (Errors.h) -- the `continue` is what actually prevents it.
```
**NEW:**
```
							/// Unconditional, not gated: the behaviour it replaces is memory
							/// corruption, so there is no vanilla semantics worth keeping. We
							/// warn and skip, because warn visibility is a runtime-toggleable
							/// log level (Errors.h) -- the `continue` is what actually prevents it.
```

**Gate: unconditional.** **Risk:** none.

---

### 3. A-03 — residual overflow inside the guarded block: `objLevelCount` is still unclamped

**Status: NEW (the audit recommended this clamp; it was not applied). Severity: CORRUPTION. Reach: R1.**
**Location:** `game/object/Weapons.cpp:188` and `:197`. OURS. Indent: **TAB**.

`Stats_GetLevels` (`Stats.cpp:1176-1179`) is a raw table read of
`statsGlobs.objectLevels[objType][objID]` with **no clamp of its own**. The only thing that
keeps that value ≤ 16 is `Config_FatalLast(levels > OBJECT_MAXLEVELS, …)` at `Stats.cpp:185` —
a **fatal**, i.e. exactly the thing F-15b proposes to soften, and one that is a no-op if
`fatalVisible` is off. So `objLevelCount` can exceed `OBJECT_MAXLEVELS` (16, `GameCommon.h:76`).

**What gets clobbered.** `objectCoefs` is `real32[20][15][16]` = 0x4b00 at offset 0 of
`WeaponStats` (`Weapons.h:83`), which is 0x4b68 (`Weapons.h:95`). Stride: 64 bytes per `objID`,
960 per `objType`. With `objLevel ≥ 16` the write walks the *next* `objID` row, then the next
`objType` row, and at `objType == 19, objID == 14, objLevel ≥ 16` it lands at offset 0x4b00 —
`WeaponStats::isSlowDeath`, then `slowDeathInitialCoef`, `slowDeathDuration`, `rechargeTime`,
`damage`, `dischargeRate`, `ammo`, `weaponRange`, `wallDestroyTimes[18]` (`Weapons.h:84-92`),
then the **next `WeaponStats` element**, then past the `Mem_Alloc` at `Weapons.cpp:57`.

`weaponStatsList` is heap, so the far end is a heap-allocator metadata smash, not a named-global
smash. `weaponGlobs` itself (`0x00504870`, 0x1b90, `Weapons.cpp:28`) holds only the pointer, and
per `docs/address-map.json` its next sized neighbour is `Gods98::mainGlobs` at `0x00506800` —
**1024 bytes of slack**, so `weaponGlobs` overflow is not the risk here. The heap is.

Second defect on the same lines: `stringParts[std::min(argc-1, objLevel)]` at `:197`. `argc` is
`uint32`; if it were ever 0, `argc-1` is `0xFFFFFFFF` and `std::min` yields `objLevel`, reading
an uninitialised `stringParts` slot. Today `argc == 0` is **not reachable** —
`Util_TokeniseSafe` returns 0 only for `str[0] == '\0'` (`Utils.cpp:51`), and `Config_Load2`
only assigns `dataString` at a non-`'\0'` byte (`Config.cpp:155,177`), so an empty value is
`nullptr` and faults earlier in `Util_StrCpy` (that is F-17). Stated so it is not re-found;
the clamp below closes it anyway.

**OLD** (`Weapons.cpp:188`):
```
							const uint32 objLevelCount = Stats_GetLevels(objType, objID);
```

**NEW:**
```
							/// DEEPCORE: clamp the loop bound. Stats_GetLevels is a raw read of
							/// statsGlobs.objectLevels[type][id] (Stats.cpp:1176-1179) with no
							/// clamp of its own; the only thing holding it to 16 is a FATAL at
							/// Stats.cpp:185, which is a no-op when fatals are switched off.
							/// objectCoefs is real32[20][15][16] welded to WeaponStats' 0x4b68
							/// layout (Weapons.h:83,95), so objLevel >= 16 walks the next objID
							/// row and eventually lands on isSlowDeath at offset 0x4b00.
							const uint32 objLevelCount = std::min(Stats_GetLevels(objType, objID),
																  (uint32)OBJECT_MAXLEVELS);
```

**OLD** (`Weapons.cpp:197`):
```
								weaponStats->objectCoefs[objType][objID][objLevel] = (real32)std::atof(stringParts[std::min(argc-1, objLevel)]);
```
**NEW:**
```
								/// DEEPCORE: argc is uint32; argc-1 underflows to 0xFFFFFFFF if
								/// argc is ever 0, which would make std::min pick objLevel and
								/// read an unwritten stringParts slot. Not reachable today (see
								/// F-17), guarded anyway because it costs one compare.
								const uint32 partIndex = (argc > 0 ? std::min(argc - 1, objLevel) : 0);
								weaponStats->objectCoefs[objType][objID][objLevel] = (real32)std::atof(stringParts[partIndex]);
```

**Gate: unconditional** — guards against an out-of-bounds write.

**Risk to legitimate behaviour:** `std::min` changes behaviour only for objects declaring more
than 16 levels, which `Stats.cpp:185` already treats as invalid. For it to be safe there must
be no vanilla `Lego.cfg` with `Levels > 16` — guaranteed by the existing check, which fires
before `Weapon_Initialise` runs (`GameState.cpp:566` precedes `:571`). **Verified by reading
the call order, not by running.** With `argc > 0 ? … : 0`, `partIndex` is identical to the old
expression for every reachable input, so `:197` is a strict no-op today.

---

### 4. F-06 — `Config_BuildStringID`: unbounded `strcpy`/`strcat` into `s_JoinPath_string[1024]`

**Status: CONFIRMED at `engine/core/Config.cpp:224-245`. Severity: CORRUPTION. Reach: R1.**
OURS (hooked at `interop.cpp:380`, `0x00479210`). Indent: **TAB**.

Every `Config_ID(...)` (`Config.h:336`) routes here, which means every `Stats_ID` on every stat
of every object, every `Lego_ID`, every `Main_ID`.

**What gets clobbered.** `Config_Globs` is 0x48c, `assert_sizeof` at `Config.h:132`, overlaid at
`0x00507098` (`Config.cpp:21`). `s_JoinPath_string` occupies `+0x000..+0x400`
(`Config.h:125`). Confirmed against `docs/address-map.json`:

| Overflow byte | Struct offset | Lands on |
|---|---|---|
| 1025..1152 | 0x400..0x47f | `Config* listSet[CONFIG_MAXLISTS]` — the **32 head pointers of the config ListSet** (`Config.h:126`) |
| 1153..1156 | 0x480 | `freeList` (`Config.h:127`) |
| 1157..1160 | 0x484 | `listCount` (`Config.h:128`) |
| 1161..1164 | 0x488 | `flags` (`Config.h:129`) |
| 1165+ | ≥ 0x48c | **past `configGlobs`.** `0x00507098 + 0x48c = 0x00507524`; next sized region is `Gods98::fontGlobs` at `0x00507528` — **4 bytes of slack**, then the font globals. |

**Concrete failure.** A machine-generated `Lego.cfg` with a ~1000-character object name makes
the composed ID `<gameName>::Stats::<objectName>::<statName>` (`Stats.cpp:119`) overrun into
`listSet[]`. Nothing faults immediately — ASCII bytes look like plausible pointers. The crash
arrives later, in `Config_Free` / `configListSet.Shutdown` (`Config.cpp:50`), when the allocator
is handed a pointer made out of the modder's object name. Attributing that to the object name
from a dump is effectively impossible.

Also fixed in passing: `va_end(s)` at `Config.cpp:242` passes the wrong argument (should be
`args`). Harmless under MSVC x86 where `va_end` is a no-op, but it is wrong.

**OLD** (`Config.cpp:224-245`, whole function):
```
// <LegoRR.exe @00479210>
const char* __cdecl Gods98::Config_BuildStringID(const char* s, ...)
{
	log_firstcall();

	std::va_list args;
	const char* curr;

	//static char s_JoinPath_string[CONFIG_MAXSTRINGID];

	// Strcpy for the first part to overwrite the previous string.
	std::strcpy(configGlobs.s_JoinPath_string, s);

	va_start(args, s);
	while (curr = va_arg(args, const char*)) {
		std::strcat(configGlobs.s_JoinPath_string, CONFIG_SEPARATOR);
		std::strcat(configGlobs.s_JoinPath_string, curr);
	}
	va_end(s);

	return configGlobs.s_JoinPath_string;
}
```

**NEW:**
```
// <LegoRR.exe @00479210>
const char* __cdecl Gods98::Config_BuildStringID(const char* s, ...)
{
	log_firstcall();

	std::va_list args;
	const char* curr;

	//static char s_JoinPath_string[CONFIG_MAXSTRINGID];

	/// DEEPCORE: was strcpy + unbounded strcat into a 1024-byte array welded to
	/// Config_Globs' 0x48c layout (Config.h:125,132), overlaid at 0x00507098
	/// (Config.cpp:21). Overflowing it walks straight into listSet[32] -- the config
	/// ListSet head pointers -- so the resulting crash surfaces much later inside
	/// Config_Free, with nothing linking it back to one long config key. Truncating
	/// is not a behaviour change for any well-formed config: the only inputs affected
	/// are ones that previously corrupted memory. Also fixes va_end(s) -> va_end(args).
	char* const  dst = configGlobs.s_JoinPath_string;
	const size_t cap = _countof(configGlobs.s_JoinPath_string);
	size_t       used = 0;
	bool         truncated = false;

	const size_t firstLen = std::strlen(s);
	if (firstLen < cap) {
		std::memcpy(dst, s, firstLen);
		used = firstLen;
	}
	else {
		std::memcpy(dst, s, cap - 1);
		used = cap - 1;
		truncated = true;
	}
	dst[used] = '\0';

	va_start(args, s);
	while (curr = va_arg(args, const char*)) {
		const size_t sepLen  = std::strlen(CONFIG_SEPARATOR);
		const size_t currLen = std::strlen(curr);
		if (used + sepLen + currLen >= cap) {
			truncated = true;
			break;
		}
		std::memcpy(dst + used, CONFIG_SEPARATOR, sepLen);
		used += sepLen;
		std::memcpy(dst + used, curr, currLen);
		used += currLen;
		dst[used] = '\0';
	}
	va_end(args);

	Error_WarnF2(truncated, "Config: string ID exceeds %i characters and was truncated to \"%s\".\n",
				 (sint32)cap, dst);

	return configGlobs.s_JoinPath_string;
}
```

**Gate: unconditional.**

**Risk:** a config whose IDs legitimately exceed 1023 characters would now be truncated instead
of corrupting memory — it would fail to match in `Config_FindItem` and warn, rather than crash
later. For truncation to be invisible, no real `Lego.cfg` may have a >1023-char composed ID.
Stock IDs are ~40 characters. **UNDETERMINED for arbitrary third-party mods**, but the
alternative on those inputs is a smashed pointer table, so this is strictly better.

---

### 5. F-23 — `vsprintf` into fixed buffers, on the reporting path itself

**Status: CONFIRMED. Severity: CORRUPTION. Reach: R1.** OURS. Indent: **TAB**.
**Land this before items 6-30**, because every one of them adds diagnostics that route
through here.

Five sites, one more than the audit found:

| Site | Buffer | Note |
|---|---|---|
| `Errors.cpp:159` | `static char res[1024]` | `Error_Format` — used by **every** `*_F` macro (`Errors.h:120-124`, `Config.h:345-348`) |
| `Errors.cpp:174` | `char msg[1024]` (stack) | `Error_Out`, dump-file branch |
| `Errors.cpp:192` | `char achBuffer[1024]` (stack) | `Error_Out`, DBWIN branch |
| `Errors.cpp:285` | `MapViewOfFile(..., 512)` at `:270` | `::wsprintf` of a **1024**-byte buffer into a **512**-byte view |
| `Errors.cpp:308` | `char msg[1024]` (stack) | `Error_Log` — **missed by the audit** |

Format arguments include config file names, item names and data strings — all attacker-length.
`Error_Format`'s buffer is `static`, so overflowing it corrupts the **DLL's own** data segment
(not an overlaid exe region); the other three are stack arrays in `Error_Out`/`Error_Log`.

**OLD** (`Errors.cpp:159`):
```
	std::vsprintf(res, msg, args);
```
**NEW:**
```
	/// DEEPCORE: bounded. Every Config_WarnItemF/Config_FatalItemF routes through here
	/// (Config.h:345-348) with config-supplied, arbitrary-length arguments.
	std::vsnprintf(res, _countof(res), msg, args);
```

**OLD** (`Errors.cpp:174`):
```
		std::vsprintf(msg, lpOutputString, args);
```
**NEW:**
```
		std::vsnprintf(msg, _countof(msg), lpOutputString, args); /// DEEPCORE: bounded
```

**OLD** (`Errors.cpp:192`):
```
		std::vsprintf(achBuffer, lpOutputString, args);
```
**NEW:**
```
		std::vsnprintf(achBuffer, _countof(achBuffer), lpOutputString, args); /// DEEPCORE: bounded
```

**OLD** (`Errors.cpp:308`):
```
		std::vsprintf(msg, lpOutputString, args);
```
**NEW:**
```
		std::vsnprintf(msg, _countof(msg), lpOutputString, args); /// DEEPCORE: bounded
```

**OLD** (`Errors.cpp:283-285`):
```
		/* write it to the shared memory */
		*((LPDWORD)lpszSharedMem) = ::_getpid();
		::wsprintf(lpszSharedMem + sizeof(DWORD), "%s", achBuffer);
```
**NEW:**
```
		/* write it to the shared memory */
		/// DEEPCORE: the shared view is 512 bytes (MapViewOfFile at Errors.cpp:270) but
		/// achBuffer is 1024, so a long message overran the mapping by up to 512 bytes.
		*((LPDWORD)lpszSharedMem) = ::_getpid();
		{
			const size_t sharedCap = 512 - sizeof(DWORD);
			std::strncpy(lpszSharedMem + sizeof(DWORD), achBuffer, sharedCap - 1);
			lpszSharedMem[sizeof(DWORD) + sharedCap - 1] = '\0';
		}
```

**Gate: unconditional.**

**Build-contract note:** `std::strncpy` raises MSVC **C4996** under `/sdl` or when
`_CRT_SECURE_NO_WARNINGS` is not defined. Check the project's preprocessor defines before
pasting the `:285` patch; if C4996 is live, use `::lstrcpynA(lpszSharedMem + sizeof(DWORD),
achBuffer, (int)sharedCap)` instead (already available via `platform/windows.h`, included at
`Errors.cpp:4`). `std::vsnprintf` is C++11 and raises no deprecation warning. **This is design
intent, not a measurement — nothing here has been compiled.**

**Risk:** messages longer than the buffer are now truncated rather than smashing the stack.
Diagnostic text only.

---

### 6. F-08 — 20 unbounded `Util_Tokenise` calls into `char* argv[32]` in `Stats_Initialise`

**Status: CONFIRMED. Severity: CORRUPTION (stack). Reach: R1.** OURS (hooked, `interop.cpp:4096`).

`char* argv[32]` at `Stats.cpp:132`. `Util_Tokenise` forwards `count = UINT32_MAX`
(`Utils.cpp:41-44`), so it writes one `char*` per separator, forever. Sites (all confirmed
present):

`Stats.cpp:201, 227, 331, 380, 392, 402, 412, 422, 432, 442, 452, 463, 477, 490, 617, 629,
641, 653, 665, 677` — twenty.

The **reads** are safe: `argv_minLevels(lvl)` (`Stats.cpp:121`) is
`argv[std::min(argcLevels-1, lvl)]` and `lvl < levels ≤ OBJECT_MAXLEVELS` = 16. Only the write
side overflows. `argcLevels == 0` is unreachable — see A-03's note.

**What gets clobbered.** `argv` is a stack local of `Stats_Initialise`. Byte 129 onward hits
whatever the compiler placed next: in Debug|x86 with `/RTC`, the guard bytes (loud); in
Release|x86, plausibly `prop`, `type`, `id`, or the return address (silent).

**Concrete failure.** A stat line with 33 colon-separated levels — an easy miscount when
someone pads a list to "one per possible level". In Release the likeliest victim is the loop's
own `prop` pointer, so the config walk resumes from a `Config*` made of a pointer into the
config file's text buffer.

**The swap is mechanical.** All 20 sites take the same shape. Two indent variants exist in this
function; use the one matching the line you are editing.

**12-space (spaces) variant** — sites `:201, 380, 392, 402, 412, 422, 432, 442, 452, 463, 477,
490, 617, 629, 641, 653, 665, 677`. Example, `Stats.cpp:201`:

**OLD:**
```
            uint32 argcLevels = Gods98::Util_Tokenise(str, argv, ":");
```
**NEW:**
```
            /// DEEPCORE: bounded. argv is char*[32] (Stats.cpp:132) and Util_Tokenise
            /// forwards count = UINT32_MAX (Utils.cpp:41-44), so a stat line with 33+
            /// colon-separated levels wrote past the end of a stack array.
            uint32 argcLevels = Gods98::Util_TokeniseSafe(str, argv, ":", _countof(argv));
            Config_WarnLastF(argcLevels >= _countof(argv), config,
                             "More than %i levels in a Stats entry; extra values ignored.",
                             (sint32)_countof(argv) - 1);
```

For the remaining 17 space-indented sites, apply only the one-line `Util_Tokenise` →
`Util_TokeniseSafe(str, argv, ":", _countof(argv))` swap (the extra warning once, at `:201`,
is enough — it fires on the first over-long line and the shape is identical everywhere).

**4-tab variant** — site `:227` only (inside the drill-time loop):

**OLD** (`Stats.cpp:227`):
```
				uint32 argcLevels = Gods98::Util_Tokenise(str, argv, ":");
```
**NEW:**
```
				uint32 argcLevels = Gods98::Util_TokeniseSafe(str, argv, ":", _countof(argv)); /// DEEPCORE: bounded
```

Site `:331` (`CollBox`, comma separator) is handled by **item 15 / F-18** below — do not patch
it twice.

**The same swap is needed at the remaining unbounded sites found by the sweep.** All present,
all OURS, all re-verified:

| Site | Array | Declared size | Covered by |
|---|---|---|---|
| `game/front/FrontEnd.cpp:790` | `stringParts` | **4** (`:780`) | item 11 (F-09) |
| `game/front/FrontEnd.cpp:392` | `stringParts` | 3 (`:391`) | item 13 (A-07) |
| `game/front/FrontEnd.cpp:4327` | `linkNames` | 15 (`:4317`) | item 12 (A-06) |
| `game/interface/Advisor.cpp:138` | `parts` | 3 (`:116`) | item 16 (F-19) |
| `game/interface/Advisor.cpp:205` | `parts` | 6 (`:186`) | item 16 (F-19) |
| `game/interface/Priorities.cpp:55,56` | `stringParts`, `stringPartsSub` | 10, 10 (`:46,47`) | item 8 (F-03) |
| `game/interface/Priorities.cpp:125` | `parts` | 16 (`:116`) | item 16 (F-19) |
| `game/audio/SFX.cpp:152` | `sampleNames` | 100 (`:143`) | below |
| `game/front/Loader.cpp:74` | `stringParts` | 10 (`:33`) | below |
| `game/GameState.cpp:337,471,492,715,731,747,763` | various | small fixed | below |
| `engine/gfx/Containers.cpp:540,3080` | `argv` | 20, 10 | below |
| `engine/gfx/Activities.cpp:31` | `argv` | `ACTIVITY_MAXARGS` | below |
| `engine/gfx/Lws.cpp:115,350` | `argv` | fixed | below |
| `engine/gfx/Lws.cpp:85`, `engine/gfx/Mesh.cpp:780` | `argv` (`Util_WSTokenise`) | fixed | below |

"Below" = same one-line `…Safe(…, _countof(array))` swap, no message needed. `game/front/Credits.cpp:67`
is **excluded**: its `lines` array is `Mem_Alloc`'d from a matching CRLF count at `:57-66`, so
it is sized correctly. (It does have a separate `fileSize == 0` underflow at `:58`; out of
scope, noted only so it is not re-found.)

**Gate: unconditional.** `Util_TokeniseSafe` (`Utils.cpp:47-67`) stops writing at `count` and
returns the capped index, so every well-formed input behaves identically.

**Risk:** for a config that legitimately declares more than 32 levels, values past 32 are now
dropped instead of smashing the stack. `OBJECT_MAXLEVELS` is 16, so no legitimate config can
reach 32 — safe.

---

### 7. F-02 — `Priorities_LoadLevel` overflows four `[27]` arrays and overwrites its own counter

**Status: CONFIRMED at `game/interface/Priorities.cpp:90-108`. Severity: CORRUPTION. Reach: R2.**
OURS (hooked, `interop.cpp:3866`, `0x0045d210`). Indent: **TAB**.

`prioritiesGlobs.count` is incremented once per recognised entry. `AIPriority_GetType`
(`AITask.cpp:47-56`) is a pure name match over `AI_Priority_Count` names and **accepts
duplicates**, so listing `Crystal` thirty times yields `count == 30`. Bound that should apply:
`AI_Priority_Count` = **27** (`GameCommon.h:266`).

**What gets clobbered.** `Priorities_Globs` is 0x4c0, `assert_sizeof` at `Priorities.h:71`,
overlaid at `0x00501f00` (`Priorities.cpp:31`). Each `[27]` array is 0x6c bytes. Index 27 of
each lands, by construction, on a **later field of the same struct**:

| Write | Offset | Lands on |
|---|---|---|
| `buttonTypes[27]` (`:92`) | 0x21c + 0x6c = 0x288 | `buttonPoints[0].x` (`Priorities.h:61`) |
| `initialTypes[27]` (`:93`) | 0x360 + 0x6c = 0x3cc | `initialValues[0]` (`Priorities.h:63`) |
| `initialValues[27]` (`:95`) | 0x3cc + 0x6c = 0x438 | `initialOff[0]` (`Priorities.h:64`) |
| `initialOff[27]` (`:100`/`:104`) | 0x438 + 0x6c = 0x4a4 | **`prioritiesGlobs.count` itself** (`Priorities.h:65`) |

**Concrete failure.** A level cfg lists 28 priority lines. On the 28th iteration
`initialOff[27] = TRUE` writes `1` into `count`; `count++` at `:106` makes it `2`. The 29th
entry writes `buttonTypes[2]`, silently overwriting the third real priority. The loop never
escapes the struct, never faults, and returns `TRUE`. The player gets a priority bar with wrong
icons in wrong slots and a `Priorities_Reset` (`Priorities.cpp:72-79`) that restores the wrong
values. Nothing is logged.

Per `docs/address-map.json`, if the layout were different this would walk into
`LegoRR::s_ScrollInfo_BOOL_005023c0` (`ScrollInfo.cpp:14`) — `0x00501f00 + 0x4c0 = 0x005023c0`,
**zero slack**.

**OLD** (`Priorities.cpp:91`):
```
		if (AIPriority_GetType(Gods98::Config_GetItemName(prop), &locAiPro)) {
```
**NEW:**
```
		if (AIPriority_GetType(Gods98::Config_GetItemName(prop), &locAiPro)) {

			/// DEEPCORE: every array indexed below is [AI_Priority_Count] == 27
			/// (GameCommon.h:266), welded to Priorities_Globs' 0x4c0 layout
			/// (Priorities.h:71), overlaid at 0x00501f00 (Priorities.cpp:31). Index 27
			/// of each aliases a LATER FIELD of the same struct -- initialOff[27] IS
			/// `count`, so overflowing here rewrites the loop counter and scrambles the
			/// table back onto itself without ever leaving the struct or faulting.
			/// AIPriority_GetType accepts duplicate names, so 28 lines is all it takes.
			/// Unconditional: the path being replaced is an out-of-bounds write.
			if (prioritiesGlobs.count >= (uint32)AI_Priority_Count) {
				Config_WarnItemF(true, prop,
					"More than %i entries in the level Priorities block; \"%s\" and "
					"everything after it is ignored.",
					(sint32)AI_Priority_Count, Gods98::Config_GetItemName(prop));
				break;
			}
```

**Gate: unconditional.** **Risk:** none — 27 is the total number of distinct priority names, so
no non-duplicating config can reach the cap.

---

### 8. F-03 — `Priorities_LoadImages`: three defects in five lines

**Status: CONFIRMED at `game/interface/Priorities.cpp:43-69`. Severity: CORRUPTION. Reach: R2.**
OURS (hooked, `interop.cpp:3862`, `0x0045d080`). Indent: **TAB**.

1. **`:54`** — `std::strcpy(buff, Gods98::Config_GetDataString(prop))` into `char buff[512]`
   (`:48`) from an arbitrary-length config value. Stack smash at 512 bytes. Also an unchecked
   `nullptr`: `Config_GetDataString` is the raw field (`Config.h:218`), and `Config_Load2`
   leaves it `nullptr` for a trailing key with no value, warning about it at
   `Config.cpp:210-215`.
2. **`:55,:56`** — unbounded `Util_Tokenise` into `stringParts[10]` / `stringPartsSub[10]`
   (`:46,47`). Eleven colons overruns `stringParts`; at index 10 it hits `stringPartsSub`, and
   past that the saved frame pointer.
3. **`:64-66`** — `stringParts[1]`, `[2]`, `[3]` are read with **no token-count check**.

**Concrete failure.** A modder writes `Crystal  Crystal:crystal.bmp` (two fields instead of
four). `stringParts[2]` and `stringParts[3]` are uninitialised stack slots handed to
`Image_LoadBMPScaled` as filenames. Best case an access violation; worst case a readable stack
address producing a corrupt image handle stored in `prioritiesGlobs.priorityOffImage[]` for the
rest of the session.

**OLD** (`Priorities.cpp:52-67`):
```
		if (AIPriority_GetType(prop->itemName, &priorityType)) {
			// -FC, grab strings and assign images for press, unpress, and disabled
			std::strcpy(buff, Gods98::Config_GetDataString(prop));
			Gods98::Util_Tokenise(buff, stringParts, ":");
			int numSubParts = Gods98::Util_Tokenise(stringParts[0], stringPartsSub, "|");
			AI_Priority priorityTypeLoc = priorityType;
			prioritiesGlobs.langPriorityName[priorityTypeLoc] = Gods98::Util_StrCpy(stringPartsSub[0]);

			if (numSubParts == 2) {
				SFX_GetType(stringPartsSub[1], &prioritiesGlobs.sfxPriorityName[priorityTypeLoc]);;
			}

			prioritiesGlobs.priorityImage[priorityType] =      Gods98::Image_LoadBMPScaled(stringParts[1], 0, 0);
			prioritiesGlobs.priorityPressImage[priorityType] = Gods98::Image_LoadBMPScaled(stringParts[2], 0, 0);
			prioritiesGlobs.priorityOffImage[priorityType] =   Gods98::Image_LoadBMPScaled(stringParts[3], 0, 0);
		}
```

**NEW:**
```
		if (AIPriority_GetType(prop->itemName, &priorityType)) {
			// -FC, grab strings and assign images for press, unpress, and disabled

			/// DEEPCORE: three defects here, all replaced.
			///  1. strcpy into char buff[512] (Priorities.cpp:48) from an arbitrary-length
			///     config value -- stack smash at 512 bytes.
			///  2. Config_GetDataString is the raw dataString field (Config.h:218) and is
			///     nullptr for a trailing key with no value (Config.cpp:210-215).
			///  3. Util_Tokenise is the UNBOUNDED variant (Utils.cpp:41-44) writing into
			///     stringParts[10], and stringParts[1..3] were then read with no count
			///     check -- a short line handed uninitialised stack pointers straight to
			///     Image_LoadBMPScaled as filenames.
			/// Util_StrCpy also removes the fixed 512-byte ceiling; Mem_Free matches the
			/// ownership convention already used at Priorities.cpp:128.
			const char* dataStr = Gods98::Config_GetDataString(prop);
			if (dataStr == nullptr) {
				Config_WarnItemF(true, prop, "PriorityImages \"%s\" has no value; skipped.",
								 Gods98::Config_GetItemName(prop));
				continue;
			}

			char* value = Gods98::Util_StrCpy(dataStr);
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
			AI_Priority priorityTypeLoc = priorityType;
			prioritiesGlobs.langPriorityName[priorityTypeLoc] = Gods98::Util_StrCpy(stringPartsSub[0]);

			if (numSubParts == 2) {
				SFX_GetType(stringPartsSub[1], &prioritiesGlobs.sfxPriorityName[priorityTypeLoc]);
			}

			prioritiesGlobs.priorityImage[priorityType] =      Gods98::Image_LoadBMPScaled(stringParts[1], 0, 0);
			prioritiesGlobs.priorityPressImage[priorityType] = Gods98::Image_LoadBMPScaled(stringParts[2], 0, 0);
			prioritiesGlobs.priorityOffImage[priorityType] =   Gods98::Image_LoadBMPScaled(stringParts[3], 0, 0);

			Gods98::Mem_Free(value);
		}
```

`char buff[512]` at `Priorities.cpp:48` becomes unused — **delete it**, otherwise MSVC C4101
(unreferenced local variable) adds a 45th warning and breaks the build contract.

**OLD** (`Priorities.cpp:48`):
```
	char buff[512];
```
**NEW:** *(remove the line entirely)*

**Note on the warning count:** `Priorities.cpp` already contributes **3** of the contracted 44
warnings (`docs/HANDOFF-2026-07-30.md:10`). Confirm after building that the count is still
exactly 44, not 45 or 41.

**Gate: unconditional** for the `TokeniseSafe` swap and the `nullptr` guard (both replace UB).
The `continue` on a short line changes control flow, but the behaviour it replaces is "load a
BMP from an uninitialised stack pointer", which is not behaviour worth preserving.

**Risk:** a `PriorityImages` line with only 2 or 3 fields is now skipped entirely rather than
partially applied. For that to be safe, no shipping `Lego.cfg` may use a short-form
`PriorityImages` line. **UNDETERMINED** — this project has never read a real `Lego.cfg`. The
usage comment at `Priorities.h:106` documents all four fields as mandatory, and the old path on
such a line is undefined behaviour, so skipping is the honest reading. If it turns out short
lines are common, downgrade the `continue` to "load what is present, warn about the rest".

---

### 9. A-04 — `Pointer_Load`: `strcpy` from config into `buff[1024]`, plus fatal-fallthrough

**Status: NEW (missed by the audit). Severity: CORRUPTION. Reach: R2.**
**Location:** `game/interface/Pointers.cpp:116-119` and `:139-150`. OURS (hooked,
`interop.cpp:3836`, `0x0045cd30`). Indent: **TAB**.

Same shape as F-03 defect 1: `std::strcpy(buff, Gods98::Config_GetDataString(item))` into
`char buff[1024]` (`:116`) from an arbitrary-length config value, with no `nullptr` check.
The tokenise at `:119` is already `Util_TokeniseSafe(…, 3)` — correct — but the copy is not.

Second defect at `:139`: `Error_Fatal((argc < 3), "Not enough commas for flic pointer
definition.")` followed unconditionally by `std::atoi(argv[1])` at `:150`. `Error_Fatal` expands
to `Error_FatalF2` (`Errors.h:118`), which is a **no-op when `fatalVisible` is off**
(`Errors.h:111`) — execution then falls through and reads `argv[1]`, which
`Util_TokeniseSafe(…, 3)` did not write when `argc < 3`. Identical class to F-16.

**What gets clobbered.** `buff` is a stack local of `Pointer_Load`. Worth noting from
`docs/address-map.json`: `LegoRR::pointerGlobs` (`0x00501a98`, 0x468) ends at exactly
`0x00501f00`, which is `LegoRR::prioritiesGlobs` — **zero slack**. That does not apply to this
stack overflow, but it means any future `pointerGlobs` index overflow lands on the priority
table, so treat that struct as adjacency-critical.

**Concrete failure.** A `Pointers` block value with a long absolute path (a real thing in
hand-edited cfgs) overruns 1024 bytes of stack inside a function whose name never appears in the
resulting crash.

**OLD** (`Pointers.cpp:115-119`):
```
			char* argv[3];
			char buff[1024];
			std::strcpy(buff, Gods98::Config_GetDataString(item));

			const uint32 argc = Gods98::Util_TokeniseSafe(buff, argv, ",", 3);
```
**NEW:**
```
			char* argv[3];

			/// DEEPCORE: was strcpy into a 1024-byte stack buffer from an
			/// arbitrary-length config value, with no nullptr check.
			/// Config_GetDataString is the raw dataString field (Config.h:218) and is
			/// nullptr for a trailing key with no value (Config.cpp:210-215).
			const char* dataStr = Gods98::Config_GetDataString(item);
			if (dataStr == nullptr) {
				Config_WarnItemF(true, item, "Pointer \"%s\" has no value; skipped.", pointerName);
				continue;
			}
			char* buff = Gods98::Util_StrCpy(dataStr);

			const uint32 argc = Gods98::Util_TokeniseSafe(buff, argv, ",", _countof(argv));
```

**OLD** (`Pointers.cpp:138-139`):
```
			else { // <PointerType>    <flicPath>,<xOff>,<yOff>
				Error_Fatal((argc < 3), "Not enough commas for flic pointer definition.");
```
**NEW:**
```
			else { // <PointerType>    <flicPath>,<xOff>,<yOff>
				/// DEEPCORE: Error_Fatal is a no-op when fatalVisible is off (Errors.h:111,118),
				/// and execution then fell through to atoi(argv[1]) -- a slot Util_TokeniseSafe
				/// never wrote when argc < 3. Warn and skip instead of relying on the fatal.
				if (argc < 3) {
					Config_WarnItemF(true, item,
						"Flic pointer \"%s\" needs 3 comma-separated fields (flic,xOff,yOff), "
						"found %i; skipped.", pointerName, (sint32)argc);
					Gods98::Mem_Free(buff);
					continue;
				}
```

A matching `Gods98::Mem_Free(buff);` must be added on **every** remaining exit path of the
`if (!reduced && Pointer_GetType(...))` block — read `Pointers.cpp:114-160` in full before
applying, since the block has several `Error_FatalF` early exits.

**Gate: unconditional.**

**Risk:** the `continue` on a short flic line replaces a fatal (or, with fatals off, undefined
behaviour). Safe as long as no cfg relies on the fatal firing. The ownership change from stack
`buff` to heap `buff` is the risky half of this patch — **do not apply it without reading every
exit path in that block.**

---

### 10. A-05 — `Effect_Load_RockFallStylesAll`: `strcpy` from config into `buff[1024]`

**Status: NEW (missed by the audit). Severity: CORRUPTION. Reach: R2.**
**Location:** `game/effects/Effects.cpp:143-144`. OURS (hooked, `interop.cpp:2046`,
`0x0040bf00`). Indent: **TAB**.

The surrounding code is otherwise the *best-written* loader in the tree — it caps
`rockFallStyleCount` at `:136`, uses `Util_TokeniseSafe` with `_countof` at `:148`, and checks
`argc >= 2` at `:149`. The `strcpy` at `:144` is the one unguarded thing left, and it has no
`nullptr` check either.

**OLD** (`Effects.cpp:143-148`):
```
		char buff[1024];
		std::strcpy(buff, Gods98::Config_GetDataString(item));

		// usage: <RockFallStyle>    <TumbleNullName>,<3SidesRockFallType>,<OutsideCornerRockFallType>,<VTunnelRockFallType>
		char* stringParts[(uint32)ROCKFALL_TYPE_COUNT + 1 + 1]; // +1 extra for overflow but still-accurate parsing.
		const uint32 argc = Gods98::Util_TokeniseSafe(buff, stringParts, ",", _countof(stringParts));
```
**NEW:**
```
		/// DEEPCORE: was strcpy into a 1024-byte stack buffer from an arbitrary-length
		/// config value, with no nullptr check. Config_GetDataString is the raw
		/// dataString field (Config.h:218), nullptr for a trailing key with no value
		/// (Config.cpp:210-215). Everything else in this loop is already bounded.
		const char* dataStr = Gods98::Config_GetDataString(item);
		if (dataStr == nullptr) {
			Config_WarnItem(true, item, "RockFallStyles entry has no value; skipped.");
			effectGlobs.rockFallStyleCount++;
			continue;
		}
		char* buff = Gods98::Util_StrCpy(dataStr);

		// usage: <RockFallStyle>    <TumbleNullName>,<3SidesRockFallType>,<OutsideCornerRockFallType>,<VTunnelRockFallType>
		char* stringParts[(uint32)ROCKFALL_TYPE_COUNT + 1 + 1]; // +1 extra for overflow but still-accurate parsing.
		const uint32 argc = Gods98::Util_TokeniseSafe(buff, stringParts, ",", _countof(stringParts));
```

And free it at the end of the loop body — **OLD** (`Effects.cpp:161`):
```
		effectGlobs.rockFallStyleCount++;
```
**NEW:**
```
		Gods98::Mem_Free(buff);
		effectGlobs.rockFallStyleCount++;
```

**Gate: unconditional.**

**Risk:** the `rockFallStyleCount++` on the skip path preserves the existing index alignment
between `rockFallStyleName[]` (written at `:141`, before the copy) and the style table. Confirm
that is what you want — the alternative is to skip the name write too. **Read `Effects.cpp:130-163`
in full before applying.**

---

### 11. F-09 — `Front_Menu_LoadMenuImage`: `strcpy` into `buff[1024]`, unbounded tokenise into a 4-slot array

**Status: CONFIRMED at `game/front/FrontEnd.cpp:778-799`. Severity: CORRUPTION (stack). Reach: R2.**
OURS (hooked, `interop.cpp:2260`, `0x00410ee0`). Indent: **TAB**.

Five colon-separated fields is enough to write `stringParts[4]`. The subsequent **reads** at
`:792-799` *are* guarded (`numParts >= 3`, `numParts == 4`) — this is a pure write overflow,
which is exactly what makes it easy to miss: the author checked the count where it was obvious
and missed it where it was implicit.

**Concrete failure.** A Windows path with a drive letter in a cfg that expects a relative one:
`C:\LRR\Data\menu.bmp:10:20:1` tokenises to five parts (`C`, `\LRR\...`, `10`, `20`, `1`) and
silently smashes the stack.

**OLD** (`FrontEnd.cpp:786-790`):
```
	std::strcpy(buff, filename);

	
	// cfg: filename.bmp[:xPos:yPos[:trans=0/1]]
	uint32 numParts = Gods98::Util_Tokenise(buff, stringParts, ":");
```
**NEW:**
```
	/// DEEPCORE: strcpy into char buff[1024] (FrontEnd.cpp:781) from a config value of
	/// arbitrary length, then UNBOUNDED Util_Tokenise (Utils.cpp:41-44) into a FOUR-slot
	/// array (FrontEnd.cpp:780). Five colon-separated fields is all it takes -- and a
	/// Windows absolute path in a cfg that expects a relative one produces exactly five.
	/// The reads below are already count-checked; only the write side was unguarded.
	const size_t filenameLen = std::strlen(filename);
	Error_WarnF2((filenameLen + 1 > _countof(buff)),
				 "Front: menu image spec is longer than %i characters and was truncated.\n",
				 (sint32)_countof(buff) - 1);
	std::strncpy(buff, filename, _countof(buff) - 1);
	buff[_countof(buff) - 1] = '\0';

	// cfg: filename.bmp[:xPos:yPos[:trans=0/1]]
	uint32 numParts = Gods98::Util_TokeniseSafe(buff, stringParts, ":", _countof(stringParts));
```

**Gate: unconditional.**

**Build-contract note:** `std::strncpy` may trip C4996 — see the note under item 5. If so, use
`::lstrcpynA(buff, filename, (int)_countof(buff))`. `FrontEnd.cpp` already includes
`platform/windows.h` transitively via `Front.h`; **verify before pasting.**

**Risk:** a menu image spec with 5+ colon-separated fields now loses the extra fields instead of
overflowing. `Front_Menu_LoadMenuImage` reads at most 4, so nothing legitimate is lost.

---

### 12. A-06 — `Front_LevelSet_LoadLevelLinks`: `linkNames[15]` overflows on **both** write and read

**Status: NEW (the audit listed the site but not the read-side defect). Severity: CORRUPTION. Reach: R2.**
**Location:** `game/front/FrontEnd.cpp:4316-4336`. OURS. Indent: **TAB**.

`char* linkNames[15]` at `:4317` with the comment "Maximum of 15 linked levels". `:4327` uses
the **unbounded** `Util_Tokenise`, so a 16th comma writes past the array. Worse, `numParts` is
then used at `:4331` to size a `Mem_Alloc` and at `:4333-4334` to **read** `linkNames[i]` for
`i < numParts` — so a 20-link level both writes 5 slots past the array *and* reads 5 slots past
it, feeding stack garbage into `Front_LevelSet_LoadLevelLinks` recursively.

This is strictly worse than F-09, and reachable from any `LevelLinks` property in `Lego.cfg`.

**OLD** (`FrontEnd.cpp:4327`):
```
		else if ((numParts = Gods98::Util_Tokenise(levelLinksStr, linkNames, ",")) != 0) {
```
**NEW:**
```
		/// DEEPCORE: was UNBOUNDED Util_Tokenise (Utils.cpp:41-44) into linkNames[15]
		/// (FrontEnd.cpp:4317). numParts is then used BOTH as the Mem_Alloc size at
		/// :4331 AND as the read bound at :4333, so a 16th comma overflowed the array on
		/// the write side and then read the overflowed slots back as level names.
		else if ((numParts = Gods98::Util_TokeniseSafe(levelLinksStr, linkNames, ",",
													   _countof(linkNames))) != 0) {
			Error_WarnF2((numParts >= _countof(linkNames)),
						 "Front: level \"%s\" declares more than %i LevelLinks; the rest are ignored.\n",
						 levelName, (sint32)_countof(linkNames) - 1);
```

**Gate: unconditional.**

**Risk:** a level declaring more than 15 links now loses links past 15 instead of corrupting the
stack. The array comment already asserts 15 is the maximum. Safe.

---

### 13. A-07 — `Front_MenuItem_AddSelectItem`: unbounded tokenise into `stringParts[3]`, then a fatal that falls through

**Status: NEW (the audit listed the site only). Severity: CORRUPTION (stack). Reach: R2.**
**Location:** `game/front/FrontEnd.cpp:391-396`. OURS. Indent: **TAB**.

`char* stringParts[3]` at `:391`, unbounded `Util_Tokenise` at `:392`. Four commas overruns it.
`Error_Fatal(numParts != 3, …)` at `:393` is a no-op when `fatalVisible` is off (`Errors.h:111,118`),
after which `:396` reads `stringParts[i]` for `i < MenuItem_SelectImage_Count`.

**OLD** (`FrontEnd.cpp:391-393`):
```
			char* stringParts[3];
			uint32 numParts = Gods98::Util_Tokenise(bannerOrBMPName, stringParts, ",");
			Error_Fatal(numParts != 3, "Must have exactly 3 comma-separated bmp images for select item");
```
**NEW:**
```
			/// DEEPCORE: unbounded Util_Tokenise (Utils.cpp:41-44) into a 3-slot array,
			/// then an Error_Fatal that is a NO-OP when fatalVisible is off
			/// (Errors.h:111,118) -- after which stringParts[0..2] are read regardless.
			char* stringParts[3] = { nullptr };
			uint32 numParts = Gods98::Util_TokeniseSafe(bannerOrBMPName, stringParts, ",",
														_countof(stringParts));
			if (numParts != 3) {
				Error_WarnF2(true, "Front: select item needs exactly 3 comma-separated bmp "
								   "images, found %i; item skipped.\n", (sint32)numParts);
				return nullptr;
			}
```

**Gate: unconditional.**

**Risk:** `return nullptr` on a malformed select item. **Read `FrontEnd.cpp:360-410` in full
first** — confirm `nullptr` is the function's existing failure return and that the caller
handles it. If the function returns `bool32` or has already mutated `selItem`, adapt the early
exit to match; do not paste blindly.

---

### 14. F-05 — SFX volume parser: unbounded copy into `volBuff[64]`, unbounded read past the token

**Status: CONFIRMED at `game/audio/SFX.cpp:164-175`. Severity: CORRUPTION. Reach: R2.**
OURS (hooked, `interop.cpp:4031`, `0x00464fc0`). Indent: **4 SPACES**.

Two unbounded operations in one loop:

- **Write:** no check that `v` stays inside `volBuff[64]` (`SFX.cpp:142`). The `*v = '\0'` at
  `:172` can itself be the 65th byte.
- **Read:** with no closing `#`, `s` walks off the end of the token — and because
  `Util_Tokenise` replaced the separators with `'\0'` **in place** (`Utils.cpp:60`) inside the
  config's own `fileData` buffer (`Config.cpp:87`), it walks through the *rest of the config
  file* until it finds a `#` or faults.

**Concrete failure.** `SFX_Drill  #80drill.wav` — a missing closing `#`, an easy typo. The loop
copies `80drill.wav\0` and keeps going through every subsequent byte of `Lego.cfg` into a
64-byte stack buffer. Stack smash within microseconds, in a function whose name never appears
in the crash context.

**OLD** (`SFX.cpp:164-175`):
```
        if (*s == '#') { // volume modifier: #-vol#
            s++; // skip opening '#'
            char* v = volBuff;
            while (*s != '#') {
                *v++ = *s++; // Copy number between #...#
            }
            s++; // skip closing '#'
            /// FIX APPLY: null-terminate volume buffer
            *v = '\0';

            volume = std::atoi(volBuff);
        }
```
**NEW:**
```
        if (*s == '#') { // volume modifier: #-vol#
            s++; // skip opening '#'
            /// DEEPCORE: this loop was unbounded in BOTH directions. Nothing kept `v`
            /// inside volBuff[64] (SFX.cpp:142), and with no closing '#' the read walked
            /// off the end of the token -- Util_Tokenise replaced the separators with
            /// '\0' IN PLACE (Utils.cpp:60) inside the config's own fileData buffer
            /// (Config.cpp:87), so an unterminated modifier scanned the entire config
            /// file into a 64-byte stack array.
            char* v = volBuff;
            char* const vEnd = volBuff + _countof(volBuff) - 1; // reserve the terminator
            while (*s != '#' && *s != '\0' && v < vEnd) {
                *v++ = *s++; // Copy number between #...#
            }
            *v = '\0';

            Error_WarnF2((*s != '#'), "SFX: unterminated '#volume#' modifier in \"%s\"; "
                                      "treating the volume as %i.\n",
                         sampleNames[i], std::atoi(volBuff));
            if (*s == '#') s++; // skip closing '#'

            volume = std::atoi(volBuff);
        }
```

**Gate: unconditional.**

**Risk:** a volume field longer than 63 digits is now truncated. No legitimate volume value is
longer than 4 characters.

---

### 15. F-18 — `CollBox` reads `argv[1]` unchecked, from a **freed** allocation

**Status: CONFIRMED at `game/object/Stats.cpp:329-341`. Severity: CORRUPTION (use-after-free read). Reach: R1.**
OURS. Indent: **4 SPACES** (8 for the `else if`, 12 inside).

`Gods98::Util_Tokenise(str, argv, ",")` at `:331` **discards its return value**; `argv[1]` is
read at `:333` regardless. `argv` is the function-scope `char* argv[32]` (`Stats.cpp:132`),
reused by every tokenise in the loop body **and across loop iterations**, and every earlier
block frees its string — `Mem_Free(str)` at `:207`, `:235`, and so on. So a single-value
`CollBox` leaves `argv[1]` holding a pointer into a heap block freed during the `RouteSpeed`
parse of *this or an earlier object*.

**Concrete failure.** `CollBox  12.0` (one value where two are expected). `std::atof` reads
freed heap. With MSVC's debug allocator the freed block is `0xDDDDDDDD` fill, so `atof` returns
garbage and `collBox.height` becomes 0 — `collRadius` is then `max(12.0, 0.0) * 0.5 = 6.0`
instead of the intended value, and the unit's collision box is silently wrong for the whole
game. In Release the block may have been reused, producing a different wrong number each run.
Nothing is printed.

**OLD** (`Stats.cpp:329-341`):
```
        else if ((str = Config_GetStringValue(config, Stats_ID("CollBox")))) {
            // Only use CollBox if CollRadius is missing or 0.0f
            Gods98::Util_Tokenise(str, argv, ",");
            collBox.width  = (real32)std::atof(argv[0]);
            collBox.height = (real32)std::atof(argv[1]);
                
            //local_12c = std::max(local_f0, local_ec) * 0.5f;
            collRadius = std::max(collBox.width, collBox.height) * 0.5f;
            flags1 |= StatsFlags1::STATS1_COLLBOX;

            /// FIX APPLY: Missing memory cleanup
            Gods98::Mem_Free(str);
        }
```
**NEW:**
```
        else if ((str = Config_GetStringValue(config, Stats_ID("CollBox")))) {
            // Only use CollBox if CollRadius is missing or 0.0f

            /// DEEPCORE: the tokenise return value was DISCARDED and argv[1] read
            /// unconditionally. argv is function-scope (Stats.cpp:132), reused by every
            /// tokenise in this loop AND across iterations, and every earlier block
            /// Mem_Free()s its string (e.g. Stats.cpp:207) -- so a one-value CollBox read
            /// a dangling pointer into a freed heap block and silently produced a wrong
            /// collision radius for the rest of the game. Also swaps to the bounded
            /// tokeniser (see F-08).
            const uint32 argcBox = Gods98::Util_TokeniseSafe(str, argv, ",", _countof(argv));
            if (argcBox < 2) {
                Config_WarnLastF(true, config, "Stats CollBox needs 2 comma-separated values, "
                                               "found %i; CollBox ignored.", (sint32)argcBox);
            }
            else {
                collBox.width  = (real32)std::atof(argv[0]);
                collBox.height = (real32)std::atof(argv[1]);

                //local_12c = std::max(local_f0, local_ec) * 0.5f;
                collRadius = std::max(collBox.width, collBox.height) * 0.5f;
                flags1 |= StatsFlags1::STATS1_COLLBOX;
            }

            /// FIX APPLY: Missing memory cleanup
            Gods98::Mem_Free(str);
        }
```

**Gate: unconditional.**

**Risk:** a one-value `CollBox` now leaves `collRadius` at 0 and `STATS1_COLLBOX` unset, instead
of setting a radius derived from freed memory. For this to be safe, no shipping cfg may use a
one-value `CollBox` expecting it to mean "square". **UNDETERMINED.** If that turns out to be a
real idiom, change the `argcBox < 2` branch to `collBox.height = collBox.width;` plus a warning
rather than skipping.

---

### 16. F-19 — three more "read fixed token slots, never check the count" sites

**Status: CONFIRMED. Severity: CRASH / SILENT-WRONG. Reach: R2.** All OURS.

| Location | Reads | Array | Hooked at |
|---|---|---|---|
| `Advisor.cpp:138` → `:140-146` | `parts[0..2]` | `char* parts[3]` (`:116`) | `interop.cpp:1649`, `0x00401270` |
| `Advisor.cpp:205` → `:207-214` | `parts[0..5]` | `char* parts[6]` (`:186`) | `interop.cpp:1656`, `0x004014a0` |
| `Priorities.cpp:125` → `:126-127` | `parts[0..1]` | `char* parts[16]` (`:116`) | `interop.cpp:3868`, `0x0045d320` |

In each case the tokenise is the **unbounded** variant and the return value is discarded.
`Advisor.cpp:140` hands `parts[0]` to `Container_Load`; `:208-214` hand `parts[1]`, `[2]`, `[5]`
to `_stricmp` / `Text_GetTextType` / `SFX_GetType` / `Panel_GetPanelType`. A short line means
`_stricmp` on an uninitialised stack pointer. Indent: **TAB** in both files.

**OLD** (`Advisor.cpp:138`):
```
			Gods98::Util_Tokenise(value, parts, ",");
```
**NEW:**
```
			/// DEEPCORE: unbounded tokenise into char* parts[3] (Advisor.cpp:116), return
			/// value discarded, parts[0..2] then read regardless. A short Advisor line
			/// handed uninitialised stack pointers to Container_Load and atof.
			const uint32 argcAnim = Gods98::Util_TokeniseSafe(value, parts, ",", _countof(parts));
			if (argcAnim < 3) {
				Error_WarnF2(true, "Advisor: anim \"%s\" needs 3 comma-separated fields "
								   "(file,loopStart,loopEnd), found %i; skipped.\n",
							 animName, (sint32)argcAnim);
				Gods98::Mem_Free(value);
				continue;
			}
```

**OLD** (`Advisor.cpp:205`):
```
			Gods98::Util_Tokenise(value, parts, ",");
```
**NEW:**
```
			/// DEEPCORE: unbounded tokenise into char* parts[6] (Advisor.cpp:186), return
			/// value discarded, parts[0..5] then read regardless -- including _stricmp on
			/// parts[1] and parts[5], i.e. strcmp against an uninitialised stack pointer.
			const uint32 argcPos = Gods98::Util_TokeniseSafe(value, parts, ",", _countof(parts));
			if (argcPos < 6) {
				Config_WarnItemF(true, prop,
					"AdvisorPositions \"%s\" needs 6 comma-separated fields "
					"(anim,text,sfx,x,y,panel), found %i; skipped.",
					Gods98::Config_GetItemName(prop), (sint32)argcPos);
				Gods98::Mem_Free(value);
				continue;
			}
```

**OLD** (`Priorities.cpp:124-127`):
```
			/// FIX APPLY: Don't pass GetTempStringValue to Util_Tokenise
			Gods98::Util_Tokenise(positionStr, parts, ",");
			prioritiesGlobs.buttonPoints[i].x = (float)std::atoi(parts[0]);
			prioritiesGlobs.buttonPoints[i].y = (float)std::atoi(parts[1]);
```
**NEW:**
```
			/// FIX APPLY: Don't pass GetTempStringValue to Util_Tokenise
			/// DEEPCORE: unbounded tokenise into char* parts[16] (Priorities.cpp:116),
			/// return value discarded, parts[1] read regardless.
			const uint32 argcPos = Gods98::Util_TokeniseSafe(positionStr, parts, ",", _countof(parts));
			if (argcPos < 2) {
				Error_WarnF2(true, "Priorities: image position %i needs 2 comma-separated "
								   "values, found %i; position left at its default.\n",
							 (sint32)i + 1, (sint32)argcPos);
			}
			else {
				prioritiesGlobs.buttonPoints[i].x = (float)std::atoi(parts[0]);
				prioritiesGlobs.buttonPoints[i].y = (float)std::atoi(parts[1]);
			}
```

**Gate: unconditional.**

**Risk:** the two `continue`s in `Advisor.cpp` skip an entry that previously read uninitialised
memory. `Advisor.cpp:138`'s loop is `for (uint32 i = 0; i < Advisor_Anim_Count; i++)` — the
`continue` is correct there. `Advisor.cpp:205` is inside a `for (prop = …)` config walk — the
`continue` is correct there too. **Both verified by reading the enclosing loops; neither
verified by running.**

---

### 17. A-08 — `Text_SetMessageWithImage`: `sprintf` into a 32-byte field of an **overlaid** struct

**Status: NEW (missed by the audit). Severity: CORRUPTION. Reach: R2.**
**Location:** `game/interface/TextMessages.cpp:168`. OURS (hooked, `interop.cpp:4186`,
`0x0046ae70`). Indent: **TAB**.

```cpp
std::sprintf(textGlobs.textImagesSFX[textType], "%s", sfxName);
```

`textImagesSFX` is `char[Text_Type_Count][32]` at offset **0x138**, size 0x340, inside
`Text_Globs` — 0x4dc, `assert_sizeof` at `TextMessages.h:81`, overlaid at `0x00504190`
(`TextMessages.cpp:24`). `Text_Type_Count` is 26 (`GameCommon.h:621`; 0x340 / 32 = 26).

The **only** caller is exe code — `interop.cpp:4182` records `Lego_LoadTextMessages` as the
user, and there is no in-tree call site. So `sfxName` comes straight out of the exe's
`Lego.cfg` text-message parsing, unbounded.

**What gets clobbered.** Per `docs/address-map.json`:

| Overflow | Struct offset | Lands on |
|---|---|---|
| `textImagesSFX[n]` bytes 33..64 | 0x138 + 32n + 32 | `textImagesSFX[n+1]` — **the next text type's SFX name** |
| `textImagesSFX[25]` bytes 33+ | ≥ 0x478 | `currType`, `textCount`, `jankCounter`, `currText` (`TextMessages.h:56-64`) — `currText` is a `const char*` that `Text_Update` dereferences |
| far overflow | ≥ 0x4dc | past `textGlobs`. `0x00504190 + 0x4dc = 0x0050466c`; next sized region is `weaponGlobs` at `0x00504870` — **516 bytes of slack**, contents unmapped by our address map |

**Concrete failure.** A `TextMessages` entry whose SFX name is 40 characters. The tail
overwrites the *next* text type's SFX name, so an unrelated message plays the wrong sound —
or, for the last text type, `currText` becomes a pointer built out of ASCII and the game faults
in `Text_Update` on the next message.

**OLD** (`TextMessages.cpp:168`):
```
	std::sprintf(textGlobs.textImagesSFX[textType], "%s", sfxName);
```
**NEW:**
```
	/// DEEPCORE: textImagesSFX is char[Text_Type_Count][32] at offset 0x138 inside
	/// Text_Globs (TextMessages.h:55), which is 0x4dc pinned by assert_sizeof
	/// (TextMessages.h:81) and overlaid on the executable at 0x00504190
	/// (TextMessages.cpp:24). sfxName arrives from the exe's own Lego.cfg parsing
	/// (this function's only caller is Lego_LoadTextMessages, interop.cpp:4182) and
	/// was never length-checked: a 40-character name overwrote the NEXT text type's
	/// SFX name, and for the last text type it walks into currType / textCount /
	/// jankCounter / currText (TextMessages.h:56-64) -- currText being a pointer that
	/// Text_Update dereferences. Bounded, and bounds-check textType while we are here.
	if ((uint32)textType < (uint32)Text_Type_Count) {
		char* const dst = textGlobs.textImagesSFX[textType];
		const size_t cap = _countof(textGlobs.textImagesSFX[textType]);
		const size_t len = (sfxName != nullptr ? std::strlen(sfxName) : 0);
		Error_WarnF2((len + 1 > cap), "Text: SFX name \"%s\" exceeds %i characters and was "
									  "truncated.\n", (sfxName ? sfxName : "(null)"), (sint32)cap - 1);
		if (sfxName != nullptr) {
			const size_t n = std::min(len, cap - 1);
			std::memcpy(dst, sfxName, n);
			dst[n] = '\0';
		}
		else {
			dst[0] = '\0';
		}
	}
```

Note `textImages[textType]` at `:170` and `Text_SetMessage(textType, …)` at `:167` are indexed
with the same unchecked `textType`; the `if` above covers `:168` only. Extending it to wrap
`:167-173` entirely is the fuller fix — **UNDETERMINED** whether the exe ever passes an
out-of-range `textType`, so the minimal patch is proposed here.

**Gate: unconditional.**

**Risk:** SFX names longer than 31 characters are now truncated rather than corrupting the
struct. A truncated name will fail to match in `SFX_GetType` and the sound will not play —
a visible, logged degradation instead of an invisible one.

---

### 18. F-12 — `Weapon_GetWeaponIDByName` returns an out-of-range sentinel every caller uses as an index

**Status: CONFIRMED at `game/object/Weapons.cpp:216-227`. Severity: CRASH. Reach: R3.**
OURS (hooked, `interop.cpp:4346`, `0x0046f390`). Indent: **TAB**.

The comment's own `??` at `:226` is the tell. The sentinel is `weaponGlobs.weaponCount + 1` —
**two elements past the end** of `weaponStatsList` (`Weapons.cpp:57`, `weaponCount * 0x4b68`
bytes). Not one caller checks it. Re-verified call sites:

| Caller | What it does with the sentinel |
|---|---|
| `Weapons.cpp:1365` → `:1369` | `Weapon_GetRechargeTime(weaponID)` → `weaponStatsList[weaponID].rechargeTime` (`:232`) |
| `Weapons.cpp:1365` → `:1375` | `Weapon_GetWeaponRange(weaponID)` (`:244`) |
| `Weapons.cpp:1365` → `:1384,:1389` | `Weapon_FireLazer`, `Weapon_GenericDamageObject` (`:266`) |
| `ElectricFence.cpp:754` → `:755` | `Weapon_GetRechargeTime` for `"FenceSpark"` |
| `ElectricFence.cpp:781` → `:782` | `Weapon_GenericDamageObject` → `Weapon_GetDamageForObject` → `objectCoefs[…]` (`:256`) |
| `Object.cpp:5478` → `:5479` | `Weapon_GetDamageForObject` for `"BatAttack"` |

`Weapon_GetDamageForObject` reads at `weaponStatsList[weaponCount+1].objectCoefs[type][id][level]`
— roughly `2 × 0x4b68` = 38,608 bytes past a valid heap buffer, plus the per-object offset.
`Weapon_GetRechargeTime` reads `+0x4b0c` of that same phantom element.

**Concrete failure.** A modder's `Lego.cfg` renames `FenceSpark` to `FenceZap`. Nothing
complains at load. The first time a monster touches an electric fence,
`ElectricFence_SparkObject` (`ElectricFence.cpp:781`) calls
`Weapon_GenericDamageObject(liveObj, weaponCount+1, …)`, which reads `isSlowDeath` and a damage
coefficient from unrelated or unmapped heap. Either an access violation mid-combat, or — worse —
a plausible float that makes fences do random damage. The log is empty.

**(a) The sentinel.** Add to `game/object/Weapons.h`, next to `WEAPON_MAXWEAPONS`:
```
/// CUSTOM: Unmistakable "no such weapon" result from Weapon_GetWeaponIDByName.
/// The original returned weaponCount+1, which is TWO valid-looking elements past the end
/// of weaponStatsList (Weapons.cpp:57) and which every caller used directly as an index.
#define WEAPON_ID_INVALID			((uint32)-1)
```

**OLD** (`Weapons.cpp:216-227`):
```
// On failure, returns g_WeaponTypes_COUNT
// Weapon IDs are 1-indexed it seems...
// <LegoRR.exe @0046f390>
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
**NEW:**
```
// On failure, returns WEAPON_ID_INVALID.
// <LegoRR.exe @0046f390>
uint32 __cdecl LegoRR::Weapon_GetWeaponIDByName(const char* weaponName)
{
	for (uint32 weaponID = 0; weaponID < weaponGlobs.weaponCount; weaponID++) {
		if (::_stricmp(weaponGlobs.weaponNameList[weaponID], weaponName) == 0) {
			return weaponID;
		}
	}
	/// DEEPCORE: the original returned weaponCount+1 -- an index TWO elements past the
	/// end of weaponStatsList (Weapons.cpp:57) that every caller then used directly, with
	/// no caller checking it. Return an unmistakable sentinel instead, and report the
	/// missing weapon ONCE per name: this runs from a per-frame update (Weapons.cpp:1365),
	/// so a raw warning here would spam every frame. Once-only bookkeeping lives DLL-side
	/// (DeepCore.cpp), following the _waterOverflowWarned precedent at DeepCore.cpp:374 --
	/// Weapon_Globs is assert_sizeof(Weapon_Globs, 0x1b90) (Weapons.h:140) and MUST NOT grow.
	DeepCore::WarnOnce_UnknownWeapon(weaponName);
	return WEAPON_ID_INVALID;
}
```

Add to `game/DeepCore.hpp` (already included by `Weapons.cpp:17`), next to
`WarnOnce_DebugWaterKeyDisabled`:
```
/// Report, once per distinct name, that a weapon named in a config does not exist in the
/// loaded WeaponTypes block. Called from Weapon_GetWeaponIDByName, which runs inside a
/// per-frame update, so this must not warn more than once per name. Storage is DLL-side;
/// Weapon_Globs carries assert_sizeof and must not grow.
void WarnOnce_UnknownWeapon(const char* weaponName);
```

Add to `game/DeepCore.cpp` (already includes `<map>` at `:6` and `<string>` transitively via
`DeepCore.hpp:26`), next to `WaterOverflow`:
```
/// Weapon names already reported as missing, so each warns exactly once per run.
static std::map<std::string, bool> _unknownWeaponWarned;


void DeepCore::WarnOnce_UnknownWeapon(const char* weaponName)
{
	const std::string key = (weaponName != nullptr ? weaponName : "?");
	if (!_unknownWeaponWarned[key]) {
		_unknownWeaponWarned[key] = true;
		DeepCore_WarnF(true, "weapon type \"%s\" is not defined in the WeaponTypes block; "
			"anything that fires it will do no damage.", key.c_str());
	}
}
```

**(b) The accessor guards.** Six one-line guards so an unchecked caller degrades to a defined
value instead of reading off the heap.

**OLD** (`Weapons.cpp:230-262`):
```
real32 __cdecl LegoRR::Weapon_GetRechargeTime(uint32 weaponID)
{
	return weaponGlobs.weaponStatsList[weaponID].rechargeTime;
}

// <LegoRR.exe @0046f400>
real32 __cdecl LegoRR::Weapon_GetDischargeRate(uint32 weaponID)
{
	return weaponGlobs.weaponStatsList[weaponID].dischargeRate;
}

// <LegoRR.exe @0046f430>
real32 __cdecl LegoRR::Weapon_GetWeaponRange(uint32 weaponID)
{
	return weaponGlobs.weaponStatsList[weaponID].weaponRange;
}

// <LegoRR.exe @0046f460>
real32 __cdecl LegoRR::Weapon_GetWallDestroyTime(uint32 weaponID, Lego_SurfaceType surfaceType)
{
	return weaponGlobs.weaponStatsList[weaponID].wallDestroyTimes[surfaceType];
}

// <LegoRR.exe @0046f490>
real32 __cdecl LegoRR::Weapon_GetDamageForObject(uint32 weaponID, LegoObject* liveObj)
{
	const real32 coef = weaponGlobs.weaponStatsList[weaponID].objectCoefs[liveObj->type][liveObj->id][liveObj->objLevel];
	if (coef == -1.0f) {
		// Damage for this specific object type not defined, return default damage.
		return weaponGlobs.weaponStatsList[weaponID].damage;
	}
	return coef;
}
```
**NEW:**
```
real32 __cdecl LegoRR::Weapon_GetRechargeTime(uint32 weaponID)
{
	/// DEEPCORE: weaponStatsList holds exactly weaponCount elements of 0x4b68 bytes
	/// (Weapons.cpp:56-57). Callers pass the result of Weapon_GetWeaponIDByName without
	/// checking it; before the sentinel change that was weaponCount+1, i.e. ~38KB past
	/// the end of the allocation. Degrade to a defined value instead.
	if (weaponID >= weaponGlobs.weaponCount) return 0.0f;
	return weaponGlobs.weaponStatsList[weaponID].rechargeTime;
}

// <LegoRR.exe @0046f400>
real32 __cdecl LegoRR::Weapon_GetDischargeRate(uint32 weaponID)
{
	if (weaponID >= weaponGlobs.weaponCount) return 0.0f; /// DEEPCORE: see Weapon_GetRechargeTime
	return weaponGlobs.weaponStatsList[weaponID].dischargeRate;
}

// <LegoRR.exe @0046f430>
real32 __cdecl LegoRR::Weapon_GetWeaponRange(uint32 weaponID)
{
	if (weaponID >= weaponGlobs.weaponCount) return 0.0f; /// DEEPCORE: see Weapon_GetRechargeTime
	return weaponGlobs.weaponStatsList[weaponID].weaponRange;
}

// <LegoRR.exe @0046f460>
real32 __cdecl LegoRR::Weapon_GetWallDestroyTime(uint32 weaponID, Lego_SurfaceType surfaceType)
{
	/// DEEPCORE: surfaceType indexes wallDestroyTimes[Lego_SurfaceType_Count] at offset
	/// 0x4b20 of the 0x4b68 WeaponStats (Weapons.h:92,95) -- the LAST field, so an
	/// out-of-range surface walks into the next element.
	if (weaponID >= weaponGlobs.weaponCount) return 0.0f; /// DEEPCORE: see Weapon_GetRechargeTime
	if ((uint32)surfaceType >= (uint32)Lego_SurfaceType_Count) return 0.0f;
	return weaponGlobs.weaponStatsList[weaponID].wallDestroyTimes[surfaceType];
}

// <LegoRR.exe @0046f490>
real32 __cdecl LegoRR::Weapon_GetDamageForObject(uint32 weaponID, LegoObject* liveObj)
{
	/// DEEPCORE: see Weapon_GetRechargeTime. liveObj->type/id also index objectCoefs
	/// [20][15][16] (Weapons.h:83), so bound them too -- an ID-15 RockMonster otherwise
	/// reads Building ID 0's row (docs/HANDOFF-2026-07-30.md:69-72).
	if (weaponID >= weaponGlobs.weaponCount) return 0.0f;
	if ((uint32)liveObj->type   >= (uint32)LegoObject_Type_Count ||
		(uint32)liveObj->id     >= (uint32)LegoObject_ID_Count ||
		(uint32)liveObj->objLevel >= (uint32)OBJECT_MAXLEVELS) {
		return weaponGlobs.weaponStatsList[weaponID].damage;
	}
	const real32 coef = weaponGlobs.weaponStatsList[weaponID].objectCoefs[liveObj->type][liveObj->id][liveObj->objLevel];
	if (coef == -1.0f) {
		// Damage for this specific object type not defined, return default damage.
		return weaponGlobs.weaponStatsList[weaponID].damage;
	}
	return coef;
}
```

And in `Weapon_GenericDamageObject` — **OLD** (`Weapons.cpp:273`):
```
		const WeaponStats* weaponStats = &weaponGlobs.weaponStatsList[weaponID];
```
**NEW:**
```
		/// DEEPCORE: see Weapon_GetRechargeTime. This one reads isSlowDeath (offset
		/// 0x4b00, Weapons.h:84); with the old weaponCount+1 sentinel that was a
		/// garbage float bit-pattern, silently turning the weapon into a
		/// damage-over-time weapon with a garbage duration.
		if (weaponID >= weaponGlobs.weaponCount) return;
		const WeaponStats* weaponStats = &weaponGlobs.weaponStatsList[weaponID];
```

**Gate: unconditional.** Changing the sentinel from "an out-of-bounds index" to "an
out-of-bounds index that is obviously out of bounds" preserves no defined behaviour, and the
accessor guards only alter the path that currently reads unmapped memory.

**Risk — the one real one in this plan.** `Weapons.cpp:1446` passes a `Weapon_KnownType` enum
value directly to `Weapon_GetWeaponRange`, and `Weapons.cpp:733` passes a `weaponID` of
unverified origin to `Weapon_GetWallDestroyTime`. If `Weapon_KnownType` values are *not*
valid `weaponStatsList` indices, the new guard changes those calls from "read garbage" to
"return 0.0f" — a **behaviour change on a path that may currently work by accident**. For this
to be safe, `Weapon_KnownType` must be an index into the loaded `WeaponTypes` list.
**UNDETERMINED — read `Weapons.cpp:1420-1460` and the `Weapon_KnownType` definition before
applying part (b).** Part (a) has no such exposure and can land alone.

---

### 19. F-17 — `atof` / `atoi` / `strlen` applied directly to a `Config_GetDataString` that can be `nullptr`

**Status: CONFIRMED. Severity: CRASH. Reach: R1.**
**Locations (all `Weapon_Initialise`, all OURS, indent TAB):** `Weapons.cpp:111, 122, 125, 128,
132, 135, 142, 191`.

`Config_GetDataString` is the raw struct field (`Config.h:218`), not a checked accessor.
`Config_Load2` leaves it `nullptr` for a trailing property whose value is missing and says so
explicitly at `Config.cpp:210-215`:

> `"%s (%i): Warning: Last property \"%s\" missing value string.\n"`

`Util_StrCpy` (`Utils.cpp:100-107`) calls `std::strlen(str)` with no null check, so
`Weapons.cpp:111` and `:191` fault on the same input; `std::atof(nullptr)` faults at the other six.

**Concrete failure.** A `Lego.cfg` whose final line is `RechargeTime` with the value
accidentally deleted. The config loads with a warning; `Weapon_Initialise` then faults during
startup, several subsystems after that warning was printed.

**(a)** Add a checked accessor to `engine/core/Config.h`, alongside `Config_GetIntValue2` /
`Config_GetRealValue2` (near `Config.h:325-333`):
```
/// CUSTOM: Config_GetDataString is a raw field read (Config.h:218) and is nullptr for a
/// trailing key with no value (Config.cpp:210-215). Everything downstream assumes a
/// string, so atof/atoi/strlen on it faults. Use this where "" is an acceptable value.
#define Config_GetDataStringOrEmpty(conf)	(Gods98::Config_GetDataString((conf)) ? Gods98::Config_GetDataString((conf)) : "")
```

**(b)** Apply at the six `atof`/`atoi` sites. Example — **OLD** (`Weapons.cpp:122`):
```
					weaponStats->rechargeTime = (real32)std::atof(Gods98::Config_GetDataString(stat));
```
**NEW:**
```
					weaponStats->rechargeTime = (real32)std::atof(Config_GetDataStringOrEmpty(stat)); /// DEEPCORE: nullptr-safe
```
Identically at `:125` (`damage`), `:128` (`dischargeRate`), `:132` (`ammo`, `std::atoi`),
`:135` (`weaponRange`), `:142` (`wallDestroyTimes[surfaceType]`).

**(c)** At the two `Util_StrCpy` sites, warn and skip instead. **OLD** (`Weapons.cpp:190-191`):
```
							/// FIX APPLY: Don't modify the config strings. BAD! NO TOUCH!
							char* str = Gods98::Util_StrCpy(Gods98::Config_GetDataString(stat));
```
**NEW:**
```
							/// FIX APPLY: Don't modify the config strings. BAD! NO TOUCH!
							/// DEEPCORE: Util_StrCpy calls strlen with no null check
							/// (Utils.cpp:100-107), and Config_GetDataString is nullptr for a
							/// trailing key with no value (Config.cpp:210-215).
							const char* coefStr = Gods98::Config_GetDataString(stat);
							if (coefStr == nullptr) {
								Config_WarnItemF(true, stat, "WeaponTypes \"%s\" has no value; ignored.",
												 Gods98::Config_GetItemName(stat));
								continue;
							}
							char* str = Gods98::Util_StrCpy(coefStr);
```
The `SlowDeath` site at `:111` is covered by item 20 (F-16) below — do not patch it twice.

**Gate: unconditional.** **Risk:** none — `""` parses to 0 through `atof`/`atoi`, which is what
a missing value already meant everywhere else in the loader.

---

### 20. F-16 — `SlowDeath`: `Config_FatalItem` on a short value, **and** falls through to an unwritten slot

**Status: CONFIRMED at `game/object/Weapons.cpp:109-119`. Severity: CRASH + latent UB. Reach: R1.**
OURS. Indent: **TAB**.

This site is simultaneously "fatal where a skip would do" **and** "silently continues where it
must not". `Config_FatalItem` → `Error_FatalF2` is a **no-op when `fatalVisible` is off**
(`Errors.h:111,118`), and execution then falls through to `stringParts[1]` at `:117` — a slot
`Util_TokeniseSafe(str, stringParts, ":", 2)` never wrote when `argc < 2`. It also leaks `str`
on the fatal path. Both halves need fixing together.

**OLD** (`Weapons.cpp:110-119`):
```
					/// FIX APPLY: Don't modify the config strings. BAD! NO TOUCH!
					char* str = Gods98::Util_StrCpy(Gods98::Config_GetDataString(stat));
					const uint32 argc = Gods98::Util_TokeniseSafe(str, stringParts, ":", 2);
					Config_FatalItem(argc < 2, stat, "Not enough parts in WeaponTypes SlowDeath");

					weaponStats->isSlowDeath = true;
					weaponStats->slowDeathInitialCoef = (real32)std::atof(stringParts[0]);
					weaponStats->slowDeathDuration = (real32)std::atof(stringParts[1]);

					Gods98::Mem_Free(str);
```
**NEW:**
```
					/// FIX APPLY: Don't modify the config strings. BAD! NO TOUCH!
					/// DEEPCORE: two defects here.
					///  1. Util_StrCpy calls strlen with no null check (Utils.cpp:100-107)
					///     and Config_GetDataString is nullptr for a trailing key with no
					///     value (Config.cpp:210-215).
					///  2. Config_FatalItem both (a) closed the game over one cfg line and
					///     (b) is a NO-OP when fatalVisible is off (Errors.h:111,118), after
					///     which execution fell through to stringParts[1] -- a slot
					///     Util_TokeniseSafe never wrote. It also leaked `str`.
					const char* slowDeathStr = Gods98::Config_GetDataString(stat);
					if (slowDeathStr == nullptr) {
						Config_WarnItem(true, stat, "WeaponTypes SlowDeath has no value; ignored.");
						continue;
					}
					char* str = Gods98::Util_StrCpy(slowDeathStr);
					const uint32 argc = Gods98::Util_TokeniseSafe(str, stringParts, ":", 2);
					if (argc < 2) {
						Config_WarnItemF(true, stat,
							"WeaponTypes SlowDeath needs 2 colon-separated parts "
							"(initialCoef:duration), found %i; ignored.", (sint32)argc);
						Gods98::Mem_Free(str);
						continue;
					}

					weaponStats->isSlowDeath = true;
					weaponStats->slowDeathInitialCoef = (real32)std::atof(stringParts[0]);
					weaponStats->slowDeathDuration = (real32)std::atof(stringParts[1]);

					Gods98::Mem_Free(str);
```

**Gate: unconditional.** **Risk:** a one-part `SlowDeath` line is now ignored instead of
terminating the process (or, with fatals off, setting `slowDeathDuration` from an uninitialised
pointer). Strictly better on both branches.

---

### 21. F-07B — `conf->depth--` underflows to `0xFFFFFFFF` and silently blanks the whole config

**Status: CONFIRMED at `engine/core/Config.cpp:163-165`. Severity: SILENT-WRONG (total). Reach: R1.**
OURS. Indent: **TAB**.

The warning fires and **the decrement happens anyway**. `depth` is `uint32` (`Config.h:110`), so
a stray `}` at depth 0 sets it to `0xFFFFFFFF`. `Config_Create` then copies that value into
every subsequent node (`Config.cpp:668`).

**Concrete failure — verified end to end.** A hand-edited `Lego.cfg` has one unbalanced `}`
before the first `{`. Every property after it carries `depth == 0xFFFFFFFF`. `Config_FindItem`
compares `item->depth != count - 1` at `Config.cpp:758` and never matches, so **every lookup in
the entire file silently returns null**. The game boots with zero stats, zero weapons, zero
priorities, and the only message in the log is a single "close brace used at depth 0" warning
near the top. `Config_GetParentItem` (`Config.cpp:279-285`) also mis-walks, since
`conf->depth - 1` is `0xFFFFFFFE`.

This is the audit's "warns but continues into undefined behaviour" case: the diagnostic exists
and is useless, because the code proceeds to do the damaging thing regardless.

**OLD** (`Config.cpp:162-165`):
```
					// Treat as non-fatal for when we introduce hot-reloading settings.
					Error_WarnF2((conf->depth == 0), "%s (%i): Warning: Config close brace \"%s\" used at depth 0.\n",
								 filename, lineNumber, CONFIG_CLOSEBLOCK);
					conf->depth--;
```
**NEW:**
```
					// Treat as non-fatal for when we introduce hot-reloading settings.
					/// DEEPCORE: the warning fired and the decrement happened ANYWAY. depth is
					/// uint32 (Config.h:110) and is copied into every subsequent node by
					/// Config_Create (Config.cpp:668), so one stray '}' before the first '{'
					/// poisoned the whole file with 0xFFFFFFFF -- after which Config_FindItem's
					/// `item->depth != count - 1` test (Config.cpp:758) never matches and EVERY
					/// lookup in the file silently returns null. The game then boots with no
					/// stats, no weapons and no priorities, and one warning at the top of the log.
					Error_WarnF2((conf->depth == 0), "%s (%i): Warning: Config close brace \"%s\" used at depth 0; ignoring it.\n",
								 filename, lineNumber, CONFIG_CLOSEBLOCK);
					if (conf->depth > 0) conf->depth--;
```

**Gate: unconditional.**

**Risk:** an unbalanced-`}` config that previously loaded as an empty tree now loads with the
brace ignored — i.e. it starts working. That is a behaviour change, but the old behaviour was
"nothing loads and nothing says why". Safe.

**Not proposed:** a matching cap on the `{` path at `Config.cpp:180`. The audit recommended one
to protect `hierarchy[CONFIG_MAXDEPTH]` in `Config_GetStringID` — but that function is dead
code (§0.3), and `Config_FindItem` already caps its own tokenise at `CONFIG_MAXDEPTH`. A 100+
deep item simply never matches. Adding a cap would change which items resolve; not worth it.

---

### 22. F-15a — a missing `Levels` key silently means "no stats at all"

**Status: CONFIRMED at `game/object/Stats.cpp:184`. Severity: SILENT-WRONG. Reach: R1.**
OURS. Indent: **8 SPACES**.

`Config_GetIntValue` is `Config_GetIntValue2` (`Config.h:325` / `Config.cpp:851-858`), which
returns **0** when the key is absent. A `Stats` block with no `Levels` key therefore yields
`levels == 0` — no fatal, no warning, and an object with an allocated but entirely zeroed stats
block. Every write loop below (`:204`, `:230`, `:344`, …) becomes a no-op.

**OLD** (`Stats.cpp:184`):
```
        uint32 levels = (uint32)Config_GetIntValue(config, Stats_ID("Levels"));
```
**NEW:**
```
        uint32 levels = (uint32)Config_GetIntValue(config, Stats_ID("Levels"));
        /// DEEPCORE: Config_GetIntValue2 returns 0 for an ABSENT key (Config.cpp:851-858),
        /// so a Stats block with no Levels line is indistinguishable from "Levels 0".
        /// Either way every write loop below is a no-op and the object ends up existing
        /// with an allocated, entirely zeroed stats block -- previously with no diagnostic.
        Config_WarnLast(levels == 0, config, "Stats entry has no Levels (or Levels 0); "
                                             "this object will have no stats at all");
```

**Gate: unconditional** — adds a message, cannot change control flow.

---

### 23. F-15b — `Config_FatalLast` on `Levels > OBJECT_MAXLEVELS` kills the process for a typo

**Status: CONFIRMED at `game/object/Stats.cpp:185`. Severity: CRASH. Reach: R1.**
OURS. Indent: **8 SPACES**. **THIS IS THE ONLY GATED ITEM IN THE PLAN.**

The check is correct and load-bearing: `levels` is the loop bound for every write into the
16-element `objectStats[type][id]` block (`Stats.cpp:192-196, 204, 230, 344`), and the
`(uint32)` cast means a negative `atoi` result becomes huge and is caught. But the consequence
of one mistyped digit is the game closing.

Unlike A-01 and A-02, softening this one **changes defined behaviour**: a config that currently
terminates the process would begin to load. That is exactly what the `surviveWaterOverflow`
gate precedent exists for (`DeepCore.cpp:377-391`).

**OLD** (`Stats.cpp:185`):
```
        Config_FatalLast(levels > OBJECT_MAXLEVELS, config, "Cannot have levels greater than maximum in Stats");
```
**NEW:**
```
        /// DEEPCORE: GATED, unlike every other fix in this file. Softening this fatal
        /// changes DEFINED behaviour -- a config that currently terminates the process
        /// would start to load -- which is precisely the case DeepCore::settings gates are
        /// for (see surviveWaterOverflow, DeepCore.cpp:377-391). Default is false = vanilla.
        if (levels > OBJECT_MAXLEVELS) {
            if (DeepCore::settings.surviveConfigErrors) {
                Config_WarnLastF(true, config, "Stats Levels %i exceeds the maximum of %i; clamped.",
                                 (sint32)levels, (sint32)OBJECT_MAXLEVELS);
                levels = OBJECT_MAXLEVELS;
            }
            else {
                Config_FatalLast(true, config, "Cannot have levels greater than maximum in Stats");
            }
        }
```

Add to `game/DeepCore.hpp`, immediately after `relocateWaterTables` (`DeepCore.hpp:213`),
inside `struct Settings`:
```
	/// Degrade instead of dying when a config value exceeds a fixed engine limit.
	///
	/// Distinct from every other correctness fix in this project, which are unconditional
	/// because the behaviour they replace is memory corruption. This one is gated because
	/// the behaviour it replaces is DEFINED: Config_FatalLast at Stats.cpp:185 terminates
	/// the process, and a config that currently terminates would begin to load. Modelled
	/// exactly on surviveWaterOverflow above.
	bool surviveConfigErrors = false;
```

Add to `DeepCore::Load()` in `game/DeepCore.cpp`, next to the `surviveWaterOverflow` read
(`DeepCore.cpp:583`):
```
	settings.surviveConfigErrors = Config_GetBoolOrFalse(config, DeepCore_ID("SurviveConfigErrors"));
```
…and to the startup log next to `DeepCore.cpp:619`:
```
		DeepCore_LogF("  SurviveConfigErrors = %s", settings.surviveConfigErrors ? "true" : "false");
```
…and to the `IsAnyFeatureEnabled()` disjunction at `DeepCore.cpp:60`:
```
		|| settings.surviveConfigErrors
```

`Stats.cpp` must include `../DeepCore.hpp` for this — **check its include block first;**
`Weapons.cpp:17` has it, `Stats.cpp` may not.

**Gate: `DeepCore::settings.surviveConfigErrors`, default `false` = vanilla.**

**Risk:** with the gate on, a config declaring 30 levels loads with 16. Silently different
gameplay rather than a refusal. That is the whole point of the gate defaulting to off.

---

### 24. F-13 — skipped objects are never reported as statless downstream

**Status: CONFIRMED at `game/object/Stats.cpp:138-141`. Severity: SILENT-WRONG. Reach: R1.**
OURS. Indent: **4 SPACES** at the insertion point.

`Stats.cpp:139` *does* warn, so it is not silent at the point of failure. The silence is
downstream: nothing ever notices the object has no stats. `statsGlobs.objectLevels` stays 0 for
it, so `Stats_GetLevels` (`Stats.cpp:1176-1179`) returns 0, so `StatsObject_SetObjectLevel`
(`Stats.cpp:968`) always fails, so the object exists in the world with `liveObj->stats` pointing
at whatever it was initialised to — which every `StatsObject_Get*` accessor
(`Stats.cpp:995-1362`) then dereferences unconditionally. The same applies to the A-01 guard and
to the F-01 guard in `Weapons.cpp`.

Insert after the `for (prop = …)` config walk closes and before
`Stats_AddToolTaskType(LegoObject_ToolType_Drill, …)` at `Stats.cpp:876`:

**NEW** (insert at `Stats.cpp:875`, i.e. immediately before line 876):
```
    /// DEEPCORE: message-only sweep. An object with zero levels is not a crash, but every
    /// StatsObject_* accessor (Stats.cpp:995-1362) will happily dereference whatever
    /// liveObj->stats points at, so name the gap ONCE at load instead of never. This
    /// catches unresolved names (Stats.cpp:139), out-of-range indices (Stats.cpp:165) and
    /// missing Levels keys (Stats.cpp:184) in one place.
    /// Lego_GetObjectTypeIDCount is an EXE ADDRESS MACRO (Game.h:1519, 0x0042ee70) and is
    /// unbounded on our side, hence the std::min -- Object.cpp:253 already trusts it raw.
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

**Gate: unconditional** — adds a message, cannot change control flow.

**Risk:** noise. If a stock `Lego.cfg` legitimately leaves many type/ID slots empty, this
prints one line per slot at every startup. **UNDETERMINED — we have never read a real
`Lego.cfg`.** If it turns out noisy, aggregate into a single count line instead. Consider
landing this one last, behind whatever the noise level turns out to be.

---

### 25. F-21 — drill-time warnings name the object instead of the stat

**Status: CONFIRMED at `game/object/Stats.cpp:228`. Severity: diagnostic. Reach: R1.**
OURS. Indent: **4 TABS**.

`prop` is the **object** item (e.g. `LargeDigger`), not the drill-time key. All six drill-time
stats therefore report `"Not enough levels in Stats LargeDigger"`, giving the reader no way to
tell whether `SoilDrillTime` or `HardDrillTime` was short. The sibling warnings at `:202`,
`:381`, `:393` correctly hardcode the stat name. `drillTimeNames[i]` is in scope
(`Stats.cpp:211-218`).

**OLD** (`Stats.cpp:228`):
```
				Config_WarnLastF(argcLevels < levels, config, "Not enough levels in Stats %s", Gods98::Config_GetItemName(prop));
```
**NEW:**
```
				/// DEEPCORE: `prop` is the OBJECT item, not the drill-time key, so all six
				/// drill-time stats printed the same message. drillTimeNames[i] is in scope
				/// (Stats.cpp:211-218) and is the thing the reader actually needs.
				Config_WarnLastF(argcLevels < levels, config, "Not enough levels in Stats %s for %s",
								 drillTimeNames[i], Gods98::Config_GetItemName(prop));
```

**Gate: unconditional** — message text only.

---

### 26. F-20 — `Config_FindItem`'s max-depth guard is dead code

**Status: CONFIRMED at `engine/core/Config.cpp:740-748`. Severity: SILENT-WRONG. Reach: R1.**
OURS (hooked, `interop.cpp:395`, `0x004795a0`). Indent: **TAB**.

`Util_TokeniseSafe` increments `index` only while `index < count` (`Utils.cpp:57,61`) and
returns `index`, so its return value **can never exceed** the `count` argument. The condition
`count > CONFIG_MAXDEPTH` at `:744` is therefore always false: the warning never fires, and a
string ID with more than 100 `::` segments is silently truncated to its first 100 and matched
against the wrong hierarchy.

**OLD** (`Config.cpp:740-748`):
```
	char* argv[CONFIG_MAXDEPTH];
	uint32 hashv[CONFIG_MAXDEPTH];
	char* tempstring = Util_StrCpy(stringID);
	const uint32 count = Util_TokeniseSafe(tempstring, argv, CONFIG_SEPARATOR, CONFIG_MAXDEPTH);
	if (count > CONFIG_MAXDEPTH) {
		Mem_Free(tempstring);
		Error_WarnF2(true, "%s: Warning: StringID exceeds max depth of %i \"%s\".\n", Config_GetFileName(conf), CONFIG_MAXDEPTH, stringID);
		return nullptr;
	}
```
**NEW:**
```
	/// DEEPCORE: Util_TokeniseSafe caps its own return value at the `count` argument
	/// (Utils.cpp:57,61), so the original `count > CONFIG_MAXDEPTH` test could NEVER be
	/// true -- the warning never fired and an over-deep ID was silently truncated to its
	/// first 100 segments and matched against the wrong hierarchy. Tokenise with one slot
	/// of headroom so the overflow is actually observable. Note the arrays grew by one
	/// element each; both are stack locals of this function, not overlaid storage.
	char* argv[CONFIG_MAXDEPTH + 1];
	uint32 hashv[CONFIG_MAXDEPTH + 1];
	char* tempstring = Util_StrCpy(stringID);
	const uint32 count = Util_TokeniseSafe(tempstring, argv, CONFIG_SEPARATOR, CONFIG_MAXDEPTH + 1);
	if (count > CONFIG_MAXDEPTH) {
		Mem_Free(tempstring);
		Error_WarnF2(true, "%s: Warning: StringID exceeds max depth of %i \"%s\".\n", Config_GetFileName(conf), CONFIG_MAXDEPTH, stringID);
		return nullptr;
	}
```

**Gate: unconditional.**

**Risk:** an over-deep ID now returns `nullptr` with a warning instead of silently matching the
wrong item. Both arrays are stack locals of `Config_FindItem` — growing them by one element
each (4 bytes + 4 bytes) touches no `assert_sizeof` struct and no overlaid region. **Confirmed
against `docs/address-map.json`: `Config_FindItem` has no overlaid storage.**

---

### 27. F-10 — `Stats_AddToolTaskType`: `taskTypes[3]` **is** `taskCount`

**Status: CONFIRMED at `game/object/Stats.cpp:885-890`. Severity: CORRUPTION. Reach: R4 (latent).**
OURS. Indent: **TAB**.

`taskTypes` is `[STATS_MAXTOOLAITASKS]` = 3 (`Stats.h:30`, `Stats.h:218`). `taskCount` is at
offset 0x0c of the 0x10-byte `ToolStats` (`Stats.h:219`, `assert_sizeof` at `Stats.h:222`) — so
**`taskTypes[3]` *is* `taskCount`**. `toolType` is likewise unchecked against
`LegoObject_ToolType_Count` (11, `GameCommon.h:1016`).

Neighbour, from `docs/address-map.json` and `Stats.h:229-230`: `toolStats[11]` occupies
`statsGlobs + 0x500 .. 0x5b0`, and `0x5b0` is the **end of `Stats_Globs`**. `statsGlobs` is at
`0x00503bd8` and `0x00503bd8 + 0x5b0 = 0x00504188`, which is
`LegoRR::g_Teleporter_BOOL_00504188` (`Teleporter.cpp:17`), **zero slack**, and `textGlobs`
8 bytes on. So an out-of-range `toolType` writes a live Teleporter flag.

**Why it is latent:** `Stats_Initialise` is called exactly once, from `GameState.cpp:566`, and
the four calls at `Stats.cpp:876-879` use four *different* tool types, so each `taskCount`
reaches 1. **Why it still matters:** the file's own `_modifiedStatsListSet.Initialise()` at
`Stats.cpp:126` and `Config_Load2`'s "when we introduce hot-reloading settings" comment
(`Config.cpp:162`) both anticipate re-initialisation. A fourth call to `Stats_Initialise` in one
process would write `taskTypes[3]`, i.e. set `taskCount` to `AITask_Type_Dig`.
`Stats_FindToolFromTaskType` (`Stats.cpp:1266-1278`) then iterates `j < toolStats->taskCount`
over a 3-element array with a garbage bound.

**OLD** (`Stats.cpp:885-890`):
```
void __cdecl LegoRR::Stats_AddToolTaskType(LegoRR::LegoObject_ToolType toolType, LegoRR::AITask_Type taskType)
{
	ToolStats* toolStats = &statsGlobs.toolStats[toolType];

	toolStats->taskTypes[toolStats->taskCount++] = taskType;
}
```
**NEW:**
```
void __cdecl LegoRR::Stats_AddToolTaskType(LegoRR::LegoObject_ToolType toolType, LegoRR::AITask_Type taskType)
{
	/// DEEPCORE: taskTypes is [STATS_MAXTOOLAITASKS] == 3 and taskCount lives at offset
	/// 0x0c of the 0x10-byte ToolStats (Stats.h:218-222) -- writing taskTypes[3] IS
	/// writing taskCount. toolStats[11] also ends at exactly the end of Stats_Globs
	/// (Stats.h:229-230, 0x500..0x5b0), and statsGlobs 0x00503bd8 + 0x5b0 == 0x00504188 ==
	/// g_Teleporter_BOOL_00504188 (Teleporter.cpp:17) with ZERO slack, so an out-of-range
	/// toolType writes a live Teleporter flag. Latent today -- Stats_Initialise runs once
	/// (GameState.cpp:566) with four distinct tool types -- but fatal the moment stats are
	/// reloaded, which Stats.cpp:126 and Config.cpp:162 both anticipate.
	if ((uint32)toolType >= (uint32)LegoObject_ToolType_Count) {
		Error_WarnF2(true, "Stats_AddToolTaskType: tool type %i out of range (max %i).\n",
					 (sint32)toolType, (sint32)LegoObject_ToolType_Count);
		return;
	}

	ToolStats* toolStats = &statsGlobs.toolStats[toolType];

	if (toolStats->taskCount >= STATS_MAXTOOLAITASKS) {
		Error_WarnF2(true, "Stats_AddToolTaskType: tool %i already has %i task types; "
						   "task type %i ignored.\n",
					 (sint32)toolType, (sint32)STATS_MAXTOOLAITASKS, (sint32)taskType);
		return;
	}

	toolStats->taskTypes[toolStats->taskCount++] = taskType;
}
```

There is also a plain **leak** in the same re-entry scenario, noted but *not* patched here:
`Stats.cpp:174-181` only allocates `objectStats[type]` when it is `nullptr`, but `Stats.cpp:195`
re-allocates `objectStats[type][id]` unconditionally, orphaning the previous 16-level block on
every re-run. Fixing that properly means deciding the re-initialisation contract, which is a
design question, not a patch.

**Gate: unconditional.** **Risk:** none — unreachable today.

---

### 28. F-11 — write index and allocation size come from two separate walks

**Status: CONFIRMED at `game/object/Weapons.cpp:52-54` (count) and `:98-99` (index). Severity: defensive. Reach: R1.**
OURS. Indent: **TAB**.

The two walks are over the same list with the same stepping function, so **today `i` cannot
exceed `weaponCount - 1`**. But the allocation size (`Weapons.cpp:56-57`) and the write index
come from *textually separate* traversals, which is exactly the coupling that breaks the day
someone adds a filter to one loop and not the other. One compare.

**OLD** (`Weapons.cpp:98-99`):
```
		for (uint32 i = 0; item != nullptr; i++, item = Gods98::Config_GetNextItem(item)) {
			WeaponStats* weaponStats = &weaponGlobs.weaponStatsList[i];
```
**NEW:**
```
		for (uint32 i = 0; item != nullptr; i++, item = Gods98::Config_GetNextItem(item)) {
			/// DEEPCORE: weaponStatsList holds exactly weaponCount elements of 0x4b68
			/// (Weapons.cpp:56-57), and that count comes from a SEPARATE walk of the same
			/// list at Weapons.cpp:52-54. The two cannot diverge today; this compare exists
			/// so they cannot diverge silently tomorrow, because the failure mode is a
			/// heap overflow with no diagnostic.
			if (i >= weaponGlobs.weaponCount) {
				Config_WarnItemF(true, item, "WeaponTypes list grew during load at \"%s\"; ignored.",
								 Gods98::Config_GetItemName(item));
				break;
			}
			WeaponStats* weaponStats = &weaponGlobs.weaponStatsList[i];
```

**Gate: unconditional.** **Risk:** none — the guard cannot fire in the current call graph.

---

### 29. F-07A — `Config_GetStringID` is dead code: **do not fix**

`Gods98::Config_GetStringID` (`Config.cpp:254-274`) has **zero callers**. Its
`hierarchy[CONFIG_MAXDEPTH]` write at `:261`, its unbounded `strcpy`/`strcat` into
`s_stringID[1024]` at `:266-271`, and its `strcat(s_stringID, hierarchy[i])` on a possibly-null
`hierarchy[i]` are all unreachable.

**Recommendation: leave it.** Patching unreachable code adds compile risk against the 44-warning
contract for zero benefit. If it ever acquires a caller, it needs the same treatment as F-06
plus a null check on `hierarchy[i]` — record that as a precondition on any future use.

---

### 30. F-22 — zero-means-default conflated with zero-means-absent: **no change**

`Stats.cpp:287-288` and `:778-846`, ~25 sites. `Config_GetRealValue2` returns 0.0 both when the
key is absent **and** when the modder explicitly wrote `0`, so `RubbleCoef 0` silently becomes
`1.0` and `Flocks_Turn 0` silently becomes `0.06`.

This is faithful to the 1999 behaviour and changing it would alter defined behaviour for every
existing mod. **Documented here so it is not re-discovered as a bug.** The project already
models the better approach for *new* keys — `DeepCore.cpp:322-342` uses a `Config_FindItem`
presence test — and any future DeepCore stat wanting a real "explicit zero" should do the same.

---

## 3. Landings

Each landing is independently compilable and independently reviewable. Build both
configurations and confirm exactly 44 warnings after each.

**Landing 1 — undo the self-inflicted crashes and finish the guard that shipped today.**
Items **1 (A-01), 2 (A-02), 3 (A-03)**. Three files touched, ~30 lines. Without this, the very
configs the two new guards exist to tolerate close the game at load.

**Landing 2 — the infrastructure everything else sits on.**
Items **4 (F-06), 5 (F-23), 6 (F-08), 26 (F-20), 21 (F-07B)**. F-23 must land **before** any
landing that adds diagnostics, so the new messages cannot themselves overflow.

**Landing 3 — the config-load overflows.**
Items **7-17** (`F-02`, `F-03`, `A-04`, `A-05`, `F-09`, `A-06`, `A-07`, `F-05`, `F-18`, `F-19`,
`A-08`). The largest landing; split it if the warning count moves.

**Landing 4 — the weapon lookup and the malformed-value crashes.**
Items **18 (F-12), 19 (F-17), 20 (F-16)**. Land F-12 part (a) alone first; part (b) only after
resolving the `Weapon_KnownType` question in §2 item 18.

**Landing 5 — diagnostics, the latent guard, and the one gate.**
Items **22 (F-15a), 24 (F-13), 25 (F-21), 27 (F-10), 28 (F-11)**, then **23 (F-15b)** with the
new `DeepCore::settings.surviveConfigErrors`. F-15b is the only item in this document that
touches `DeepCore.cfg`, and the only one that is gated.

**Explicitly not doing:** items **29 (F-07A)** and **30 (F-22)**; anything in
`Dependencies.cpp`, `Upgrade.cpp`, or `LegoObject_LoadObjTtsSFX` (all still exe address macros —
`Dependencies.cpp:24-55`, `Upgrade.cpp:13-47`, `Object.h:1628` / `0x0044af80`); and any change
to a struct carrying `assert_sizeof`. All new storage in this plan is stack-local, heap, or
DLL-side.

---

## 4. Gating summary

| Gate | Items |
|---|---|
| **Unconditional** — guards against UB, or message-only | A-01, A-02, A-03, A-04, A-05, A-06, A-07, A-08, F-02, F-03, F-05, F-06, F-07B, F-08, F-09, F-10, F-11, F-12, F-13, F-15a, F-16, F-17, F-18, F-19, F-20, F-21, F-23 |
| **Behind `DeepCore::settings.surviveConfigErrors`** (new, default `false`) | **F-15b only** |
| **No change** | F-07A (dead code), F-22 (defined vanilla behaviour) |

The ratio is the point, and it matches the rule in the brief: **a guard that prevents a
corrupting write is unconditional, because the path it replaces is already broken.** The
project's gate precedent (`surviveWaterOverflow`, `DeepCore.cpp:377-391`) is for cases where the
vanilla behaviour is *defined* and we choose to deviate. Out-of-bounds writes are not defined
behaviour, so there is nothing to preserve and nothing to bisect.

---

## 5. Neighbour reference (from `docs/address-map.json`)

Every overlaid region touched or cited above, with its actual next neighbour:

| Region | Start + size = end | Next region | Slack |
|---|---|---|---|
| `LegoRR::pointerGlobs` | `0x00501a98 + 0x468 = 0x00501f00` | `LegoRR::prioritiesGlobs` | **0** |
| `LegoRR::prioritiesGlobs` | `0x00501f00 + 0x4c0 = 0x005023c0` | `LegoRR::s_ScrollInfo_BOOL_005023c0` | **0** |
| `LegoRR::sfxGlobs` | `0x00502468 + 0x1770 = 0x00503bd8` | `LegoRR::statsGlobs` | **0** |
| `LegoRR::statsGlobs` | `0x00503bd8 + 0x5b0 = 0x00504188` | `LegoRR::g_Teleporter_BOOL_00504188` | **0** |
| `LegoRR::textGlobs` | `0x00504190 + 0x4dc = 0x0050466c` | `LegoRR::weaponGlobs` | 516 |
| `LegoRR::weaponGlobs` | `0x00504870 + 0x1b90 = 0x00506400` | `Gods98::mainGlobs` | 1024 |
| `Gods98::configGlobs` | `0x00507098 + 0x48c = 0x00507524` | `Gods98::fontGlobs` | 4 |
| `LegoRR::advisorGlobs` | `0x004b3db0 + 0x410 = 0x004b41c0` | `LegoRR::aiGlobs` | 8 |
| `LegoRR::effectGlobs` | `0x004c8180 + 0xc78 = 0x004c8df8` | `LegoRR::efenceGlobs` | **0** |

Four zero-slack pairs, not one. The audit named only `sfxGlobs`→`statsGlobs`.

---

## 6. What could not be determined

- **Whether `Lego_GetObjectByName` can actually return an out-of-range ID.** EXE ADDRESS MACRO
  (`Game.h:1511`, `0x0042e7e0`) with no disassembly in-tree. The counters that size its tables
  (`Game.h:553-556`) are plain `uint32` filled from config by exe code with no ceiling on our
  side. Every guard above is written to be correct whether or not the exe already clamps — a
  guard that never fires costs one compare.
- **Whether `Lego_GetObjectTypeIDCount` (`Game.h:1519`, `0x0042ee70`) clamps to 15.**
  `Object.cpp:253` already trusts it raw. F-13's sweep wraps it in `std::min` regardless.
- **Whether `Weapon_KnownType` values are valid `weaponStatsList` indices** (`Weapons.cpp:1446`,
  `:733`). This gates F-12 part (b). Must be resolved by reading the enum and its producers
  before applying.
- **Whether any shipping `Lego.cfg` uses short-form `PriorityImages` (F-03), one-value `CollBox`
  (F-18), or >15 `LevelLinks` (A-06).** This project has never read a real `Lego.cfg`. In each
  case the path being replaced is undefined behaviour, so skipping is the honest reading, but a
  real cfg could prove otherwise.
- **Whether F-13's sweep is noisy.** Depends entirely on how sparse a stock `Lego.cfg`'s
  type/ID matrix is. Unknown.
- **Warning-count impact of every patch here.** The contract is 0 errors / exactly 44 warnings
  (Debug|x86 and Release|x86, v142; currently `TextMessages.cpp` 22, `Maths.cpp` 14,
  `Loader.cpp` 4, `Priorities.cpp` 3, `Roof.cpp` 1 — `docs/HANDOFF-2026-07-30.md:9-11`). Every
  patch uses explicit casts and `_countof` to avoid signed/unsigned and truncation warnings, and
  §2 items 5, 8 and 11 flag the specific C4996/C4101 risks. **That is design intent, not a
  measurement. None of these patches has been compiled.**
- **Runtime confirmation of any scenario above.** We cannot run the game. Every failure
  scenario is derived from source and `docs/address-map.json`, never observed. Nothing here is
  play-tested, and nothing in this document should be read as implying it was.

---

## 7. DECISION

Apply in this order. Landings 1 and 2 are not optional — landing 1 undoes a crash the last two
commits introduced, and landing 2's F-23 has to precede every diagnostic added afterwards.

```
Landing 1   A-01   Stats.cpp:165        FatalItemF -> WarnItemF
            A-02   Weapons.cpp:178      FatalItemF -> WarnItemF
            A-03   Weapons.cpp:188,197  clamp objLevelCount; guard argc-1

Landing 2   F-06   Config.cpp:225-245   bounded Config_BuildStringID (+ va_end fix)
            F-23   Errors.cpp:159,174,192,285,308   vsprintf -> vsnprintf; cap the 512B view
            F-08   Stats.cpp x20 + sweep table      Util_Tokenise -> Util_TokeniseSafe
            F-20   Config.cpp:740-748   make the dead max-depth guard live
            F-07B  Config.cpp:165       clamp depth-- instead of underflowing

Landing 3   F-02   Priorities.cpp:91    cap prioritiesGlobs.count at 27
            F-03   Priorities.cpp:48-67 heap copy, TokeniseSafe, token-count check
            A-04   Pointers.cpp:116,139 heap copy; fatal -> warn+skip
            A-05   Effects.cpp:143,161  heap copy + free
            F-09   FrontEnd.cpp:786,790 bounded copy + TokeniseSafe
            A-06   FrontEnd.cpp:4327    TokeniseSafe (fixes write AND read overflow)
            A-07   FrontEnd.cpp:391-393 TokeniseSafe; fatal -> warn+return
            F-05   SFX.cpp:164-175      bound the volume parser both ways
            F-18   Stats.cpp:329-341    check the CollBox token count
            F-19   Advisor.cpp:138,205; Priorities.cpp:125   check token counts
            A-08   TextMessages.cpp:168 bound the 32-byte overlaid SFX-name field

Landing 4   F-12a  Weapons.cpp:216-227 + Weapons.h + DeepCore.{hpp,cpp}   WEAPON_ID_INVALID
            F-12b  Weapons.cpp:230-273  six accessor guards  [BLOCKED on Weapon_KnownType]
            F-17   Config.h + Weapons.cpp:122,125,128,132,135,142,191     nullptr-safe
            F-16   Weapons.cpp:110-119  SlowDeath: nullptr check, fatal -> warn+skip

Landing 5   F-15a  Stats.cpp:184        warn on missing/zero Levels
            F-21   Stats.cpp:228        name the stat, not the object
            F-11   Weapons.cpp:98       defensive index invariant
            F-10   Stats.cpp:885-890    cap taskCount and toolType
            F-13   Stats.cpp:875        post-load "0 levels" sweep  [noise risk]
            F-15b  Stats.cpp:185 + DeepCore.{hpp,cpp}   GATED: surviveConfigErrors

Not doing   F-07A  Config.cpp:254-274   dead code, zero callers
            F-22   Stats.cpp:287,778-846   defined vanilla behaviour
```

One gate is added in total: `DeepCore::settings.surviveConfigErrors`, default `false` = vanilla,
used by exactly one item (F-15b). Everything else is unconditional, because everything else
replaces an out-of-bounds write, an uninitialised read, or a message.
