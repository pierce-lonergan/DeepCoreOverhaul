# Campaign and story: what a fan overhaul can actually build

**Status: research only. Nothing in this document has been built, and nothing has been run.**
This project has no installation of the 1999 game, no assets and no `Lego.cfg`. Every claim below
is derived by reading source in this tree, or is explicitly attributed to an external source and
marked as such. Where a claim depends on behaviour of the original executable that cannot be
derived from source, it is marked **UNDETERMINED**. Compile-verified is the honest ceiling; no
statement here implies play-testing.

Two conventions used throughout:

* **OURS** — a real C++ body exists in this tree and is installed over the exe with
  `hook_write_jmpret` in `src/openlrr/interop.cpp`. There is no trampoline
  (`docs/HANDOFF-2026-07-30.md:74-77`), so an OURS function is a complete replacement.
* **EXE** — a live address macro of the form `#define Name ((ret (__cdecl*)(args))0xADDR)`. Calling
  it runs 1999 machine code. It cannot be partially modified.

---

## 0. Executive answer

**New campaign content is data-only.** A complete new mission — briefing prose, objectives,
scripted events, timed messages, monster waves, camera work, win and loss conditions, reward
screen, position in the campaign tree — requires **zero engine changes**, because the two systems
that carry all of it (`Objective.cpp` and the NERPs virtual machine) are already 100 % ours and
already read everything they need from `Lego.cfg` and from loose data files.

Engine work is required for exactly four things, all small, all in files we own:

1. An **in-DLL NERPs assembler**, so scripts can be authored as text instead of hand-assembled
   bytecode. This is the single highest-leverage item in the document (§3.6).
2. A **campaign-scoped save directory**, so shipping a modified mission list does not blank every
   existing save (§4.5).
3. **Per-level briefing/completion/failure titles**, which are currently global (§5.2).
4. Optional: **new objective types**, three edit sites, all in `Objective.cpp` (§2.5).

Three defects found while reading are reported in §7; one of them (`SetGameCompleted`) will make
the last mission of any campaign report failure.

---

## 1. Verifying the ownership claim

### 1.1 Method

Two independent counts per file:

* **Live exe macros** — lines matching `^\s*#define\s+\w+\s*\(\(` in the header. Each is one
  function that is still 1999 machine code.
* **Real bodies** — lines matching `^[A-Za-z_].*__cdecl LegoRR::` in the `.cpp`. Each is one
  function we own.

Commented-out `//#define Name ((...))` lines are the upstream convention for "this used to be an
address macro and has since been implemented"; they are counted as evidence of ownership, not as
live macros.

### 1.2 Results

| File | Real bodies (OURS) | Live exe macros | Verdict |
| --- | ---: | ---: | --- |
| `game/mission/Objective.cpp` / `.h` | **15** | **0** | **100 % OURS** |
| `game/mission/NERPsFunctions.cpp` / `.h` | **294** | **0** | **100 % OURS** |
| `game/mission/NERPsFile.cpp` / `.h` | 15 | 33 | **MIXED** — VM ours, services split |
| `game/mission/Messages.cpp` / `.h` | 26 | 0 | 100 % OURS |
| `game/mission/PTL.cpp` / `.h` | 2 | 0 | 100 % OURS |
| `game/mission/NERPsRuntime.cpp` / `.h` | 0 | 0 | empty stub (0 bytes) |
| `game/front/FrontEnd.cpp` / `.h` | **148** | **0** | **100 % OURS** |
| `game/front/Loader.cpp` / `.h` | 5 | 0 | 100 % OURS |
| `game/front/Credits.cpp` / `.h` | 1 | 0 | 100 % OURS |
| `game/front/Reward.cpp` / `.h` | 4 | **48** | **8 % — essentially EXE** |
| `game/interface/Encyclopedia.cpp` / `.h` | 7 | 0 | 100 % OURS |
| `game/interface/TextMessages.cpp` / `.h` | 10 | 0 | 100 % OURS |
| `game/interface/ToolTip.cpp` / `.h` | 11 | 0 | 100 % OURS |

These agree with `docs/DIRECTORY.md:515-522` and `docs/DIRECTORY.md:486`.

### 1.3 Is `Objective.cpp` really 100 % ours? — Yes.

All fifteen prototypes in `Objective.h:120-181` carry a commented-out address macro directly above
a real declaration, and all fifteen are installed:

```
interop.cpp:3775  bool interop_hook_LegoRR_Objective(void)
interop.cpp:3780    hook_write_jmpret(0x004577a0, LegoRR::Objective_LoadObjectiveText)
interop.cpp:3782    hook_write_jmpret(0x00458000, LegoRR::Objective_LoadLevel)
   ... 11 more ...
interop.cpp:3810    hook_write_jmpret(0x004593c0, LegoRR::Objective_Callback_CheckCompletedObject)
interop.cpp:4551  result &= interop_hook_LegoRR_Objective();
```

The only exe-resident thing the module touches is its own globals block,
`objectiveGlobs @0x00500bc0` (`Objective.cpp:32`), and `Lego_UnkObjective_CompleteSub_FUN_004262f0`
(called at `Objective.cpp:690`, `703`, `716`), which is still EXE.

### 1.4 Is the NERPs VM really 100 % ours? — Yes, with a precise boundary.

Separate the three layers.

**(a) The virtual machine — 6 of 6 OURS.**

| Function | Body | Installed at |
| --- | --- | --- |
| `NERPsFile_LoadScriptFile` | `NERPsFile.cpp:95` | `interop.cpp:3107` (`0x004530b0`) |
| `NERPsFile_LoadMessageFile` | `NERPsFile.cpp:115` | `interop.cpp:3109` (`0x00453130`) |
| `NERPsFile_GetMessageLine` | `NERPsFile.cpp:285` | `interop.cpp:3111` (`0x004534c0`) |
| `NERPsFile_Free` | `NERPsFile.cpp:294` | `interop.cpp:3113` (`0x004534e0`) |
| `NERPsRuntime_LoadLiteral` | `NERPsFile.cpp:333` | **not installed** — `interop.cpp:3116` is commented "internal, no need to hook these" |
| `NERPsRuntime_Execute` | `NERPsFile.cpp:346` | `interop.cpp:3119` (`0x004535e0`) |

The one gap matters and is called out again in §3.7: our `NERPsRuntime_Execute` calls our
`NERPsRuntime_LoadLiteral` directly, so the interpreter is internally consistent — but the exe's
copy at `0x004535a0` is still live, and whether anything else in the exe calls it is
**UNDETERMINED**.

**(b) The built-in function set — 294 of 294 OURS.**

`NERPsFunctions.cpp` contains 294 bodies. 293 of them are installed by writing into the exe's
function table rather than by patching code:

```
NERPsFile.h:400   #define NERPs_hook_function(name) LegoRR::NERPs_HookFunction(nameof(name), LegoRR::NERPFunc__##name)
NERPsFile.cpp:80  bool LegoRR::NERPs_HookFunction(const char* name, NERPsFunction function)
NERPsFile.cpp:82      for (uint32 i = 0; i < _countof(c_nerpsFunctions); i++)
NERPsFile.cpp:83          if (::_stricmp(c_nerpsFunctions[i].name, name) == 0) {
NERPsFile.cpp:84              c_nerpsFunctions[i].function = function;
```

`interop.cpp` makes exactly **293** `NERPs_hook_function(...)` calls. The 294th is the table
terminator, installed by name because it is not a valid C++ identifier:

```
interop.cpp:3432  result &= LegoRR::NERPs_HookFunction("**End Of List**", LegoRR::NERPFunc__End_Of_List);
```

Seven of these are *additionally* patched with `hook_write_jmpret` because exe code calls them
outside the table (`interop.cpp:3157-3177`): `SetGameSpeed`, `SetLevelCompleted`, `SetLevelFail`,
`GetTutorialFlags`, `SetTutorialFlags`, `SetMessagePermit`, `SetObjectiveSwitch`.

Set difference between implemented and table-installed names is empty in both directions except
for `End_Of_List`, which is covered by `interop.cpp:3432`. So: **294 implemented, 294 installed,
0 remaining exe built-ins.**

**(c) The runtime services the built-ins call — 33 still EXE.** This is the honest asterisk on
"the NERPs VM is 100 % ours". The functions the built-ins delegate to are split:

*Still EXE* (`NERPsFile.h`, live macros at the cited lines): the block-pointer table
(`NERPs_InitBlockPointersTable` :670, `NERPs_GetBlockPointer` :674, `NERPs_FreeBlockPointers` :678,
`NERPsRuntime_EnumerateBlockPointers` :682), the tutorial-icon renderer
(`NERPsRuntime_DrawTutorialIcon` :513), icon-click plumbing (`NERPs_GetIconClicked` :521,
`NERPsRuntime_SetSubmenuIconClicked` :550, `NERPsRuntime_FlashSubmenuIcon` :554,
`NERPsRuntime_FlashIcon` :640, `NERPs_SubMenu_GetBuildingVehicleIcon_ByObjectName` :636),
object-level setters (`NERPs_SetObjectsLevel` :495,
`NERPsRuntime_GetLevelObjectsBuilt` :623, `NERPsRuntime_GetPreviousLevelObjectsBuilt` :632),
`NERPsRuntime_SetTutorialPointer` :705, `NERPs_RestartMessageSample` :660, and ten search
callbacks.

*OURS* in the same file: `NERPsRuntime_EndExecute` (:471 / `NERPsFile.cpp:739`),
`NERPsRuntime_UpdateTimers` (:463 / :697), `NERPsRuntime_AdvanceMessage` (:455 / :658),
`NERPsRuntime_RepeatMessage` (:451 / :648), `NERPs_SetHasNextButton` (:447 / :641),
`NERPs_Level_NERPMessage_Parse` (:667), and — importantly — `NERPsRuntime_TutorialActionCallback`
(:688, installed `interop.cpp:3140`), which is the single dispatcher for every block-pointer
action a script can perform.

**Consequence for planning.** Authoring scripts, parsing them, executing them, and every one of the
293 callable operations are ours. What is not ours is some of the plumbing *underneath* particular
operations. That does not block new content; it only blocks changing *how* those specific
operations behave.

### 1.5 One correction to prior recon

Prior recon reported **20 free flag bits** in `Objective_GlobFlags`. In this tree the correct
number is **19**.

`Objective.h:38-55` defines bits `0x1` through `0x1000`. Bit `0x40` (`HITTIMEFAIL`) is commented
"Missing flag that's assumed to be used instead of `SHOWACHIEVEDADVISOR`" and is unused *in the
original*, but this fork applies the fix and uses it in two places:

```
Objective.cpp:609    /// FIX APPLY: Switch to HITTIMEFAIL flag.
Objective.cpp:610    objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_HITTIMEFAIL;
Objective.cpp:1099   if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_HITTIMEFAIL) {
```

So the free bits are `0x2000` … `0x80000000` — **19 bits, bits 13-31.** See §2.6 for why you
probably should not spend any of them.

---

## 2. The objective system

### 2.1 Storage, and the hard limit on it

Per-level objective data lives in `ObjectiveData` (`Game.h:297-317`, `assert_sizeof(…, 0x54)`),
embedded **inside** `Lego_Level` at offset `0xbc` (`Game.h:437`), and `Lego_Level` is itself pinned
at `0x284` (`Game.h:459`). Global objective state lives in `Objective_Globs`
(`Objective.h:65-91`, `assert_sizeof(…, 0x28c)`), bound to `0x00500bc0` (`Objective.cpp:32`).

**Neither struct can grow by one byte.** This is the cardinal rule. Any new per-level or
per-mission objective state must live DLL-side.

### 2.2 Every objective type that exists

Five, all parsed in `Objective_LoadLevel` (`Objective.cpp:371-579`), each keyed under
`Config_ID(gameName, levelName, …)`, i.e. `Lego*::<LevelName>::<Key>`:

| Cfg key | Format | Setter | Flag | Field |
| --- | --- | --- | --- | --- |
| `CrystalObjective` | integer | `Objective_SetCryOreObjectives` (`:582`) | `CRYSTAL` `0x100` | `objective.crystals` |
| `OreObjective` | integer | same (`:582`) | `ORE` `0x200` | `objective.ore` |
| `TimerObjective` | `seconds:HitTimeFailObjective` | `Objective_SetTimerObjective` (`:602`) | `TIMER` `0x800` (+ `HITTIMEFAIL` `0x40`) | `objective.timer` |
| `ConstructionObjective` | object name | `Objective_SetConstructionObjective` (`:615`) | `CONSTRUCTION` `0x1000` | `constructionType` / `constructionID` |
| `BlockObjective` | `bx,by` | `Objective_SetBlockObjective` (`:595`) | `BLOCK` `0x400` | `objective.blockPos` |

Parse sites: `Objective.cpp:529-531` (crystal/ore), `:535-542` (timer, multiplied by
`STANDARD_FRAMERATE`), `:546-553` (construction, resolved through the EXE
`Lego_GetObjectByName`, `Game.h:1511`), `:557-566` (block).

Three non-objective keys are parsed in the same function: `DontShowObjectiveAdvisor`,
`DontShowObjectiveAcheiveAdvisor`, `DontShowObjectiveFailedAdvisor` (`:388-396`, note the
misspelling of "Achieve" — it is load-bearing), plus the image/AVI keys `ObjectiveImage<W>x<H>`,
`ObjectiveAcheivedImage<W>x<H>`, `ObjectiveFailedImage<W>x<H>`, `ObjectiveAcheivedAVI`
(`:400-522`), and `ObjectiveText` (`:570-573`).

### 2.3 How win and loss are actually evaluated

Per frame, `Objective_Update` (`Objective.cpp:912`) is the state machine. When no briefing or
end-screen is showing it calls:

```
Objective.cpp:981   if (Objective_CheckCompleted(level, &timerStillRunning, elapsedGame)) {
Objective.cpp:982       Objective_SetStatus(timerStillRunning ? LEVELSTATUS_COMPLETE : LEVELSTATUS_FAILED);
```

`Objective_CheckCompleted` (`:1084-1125`) evaluates in this order:

1. **Bail out entirely if the briefing advisor is enabled** (`:1087-1089`).
2. Decrement the timer; if it hits zero, return `true`, with `timerStillRunning = false` only when
   `HITTIMEFAIL` is set (`:1093-1104`). So a plain `TimerObjective` *completes* the level and a
   `HitTimeFailObjective` timer *fails* it.
3. Return `false` if crystals or ore are below target (`:1107-1116`). Note: these can only ever
   **veto**, never satisfy.
4. Iterate live objects and return `true` if `Objective_Callback_CheckCompletedObject` matches
   (`:1119-1122`).

`Objective_Callback_CheckCompletedObject` (`:1129-1150`) returns `true` when a MiniFigure or
Vehicle stands on `BlockObjective`'s block, or when any live object matches
`ConstructionObjective`'s type and id.

### 2.4 Two gates that will silently kill a new mission

These are the most important practical findings in this section. Both are faithful to the
decompile as far as the surrounding comments indicate; whether the original binary behaves
identically is **UNDETERMINED**, since we cannot run it.

**Gate 1 — a mission that shows the briefing advisor can never complete.**

```
Objective.cpp:1086   // No briefing == sandbox or something... WHAT???
Objective.cpp:1087   if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_SHOWBRIEFINGADVISOR) {
Objective.cpp:1088       return false;
```

`SHOWBRIEFINGADVISOR` is set unless `DontShowObjectiveAdvisor` is true (`Objective.cpp:388-390`),
**and** it is force-set as the fallback when no other flag was assigned:

```
Objective.cpp:576   if (objectiveGlobs.flags == OBJECTIVE_GLOB_FLAG_NONE) {
Objective.cpp:577       objectiveGlobs.flags = OBJECTIVE_GLOB_FLAG_SHOWBRIEFINGADVISOR;
```

⇒ **Any new level must set `DontShowObjectiveAdvisor TRUE`** if it wants the built-in objective
evaluator to run at all. Otherwise the mission is unwinnable except through a NERPs script calling
`SetLevelCompleted`.

**Gate 2 — a level with `CrystalObjective` can never show a briefing or an end screen.**

```
Objective.cpp:646   // If already showing, then SetStatus does nothing.
Objective.cpp:647   if (Objective_IsShowing()) return;
Objective.cpp:649   // What's the purpose of the OBJECTIVE_GLOB_FLAG_CRYSTAL flag check??
Objective.cpp:650   if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_CRYSTAL)
Objective.cpp:651       return;
```

`CRYSTAL` is set by `Objective_SetCryOreObjectives` whenever `CrystalObjective > 0`
(`Objective.cpp:584-587`). `Objective_SetStatus` is the *only* entry point to the briefing and to
both end screens. So setting `CrystalObjective` makes `Objective_SetStatus` a no-op for that level,
for every status including `LEVELSTATUS_INCOMPLETE` (the briefing).

**Combined conclusion: the built-in objective system is largely vestigial.** Crystal and ore
objectives cannot satisfy a completion check (§2.3 step 3) and crystal objectives disable the
status UI outright. Only `TimerObjective`, `BlockObjective` and `ConstructionObjective` can end a
level through this path, and only when the briefing advisor is disabled. Everything else in the
1999 campaign must have been driven by NERPs — which is exactly where §3 says the content lever is.

### 2.5 Exactly where a new objective type goes

Three edit sites, all in `Objective.cpp`, all OURS, no exe struct touched:

**(1) Parse** — inside `Objective_LoadLevel`, after the `BlockObjective` block at `:566` and
**before** the `flags == NONE` fallback at `:576` if (and only if) you also set a flag bit.

**(2) Evaluate** — inside `Objective_CheckCompleted`, after the ore veto at `:1116`.

**(3) Optionally, per-object matching** — inside `Objective_Callback_CheckCompletedObject`
(`:1129`), which already receives the `ObjectiveData*`.

Worked sketch, adding `MonsterKillObjective <count>`. It stores state DLL-side and never touches
the pinned structs. Note the `return true` at the end — without it the new objective can only veto,
which is the trap crystals and ore fall into.

```cpp
// game/mission/Objective.cpp — OURS. Add near the existing Globals region (Objective.cpp:29-34).
namespace
{
    /// DLL-side objective state. ObjectiveData is pinned at 0x54 inside Lego_Level
    /// (Game.h:297-317 and Game.h:437) and MUST NOT grow — see docs/ADDRESS-MAP.md.
    struct DeepCore_ExtraObjective
    {
        bool   hasMonsterKills = false;
        uint32 monsterKills    = 0;
        bool   satisfiesOnItsOwn = false; // true => this objective can COMPLETE the level
    };
    DeepCore_ExtraObjective s_extraObjective;
}

// --- (1) inside Objective_LoadLevel, immediately after the BlockObjective block (Objective.cpp:566)
    s_extraObjective = DeepCore_ExtraObjective{};
    // usage: MonsterKillObjective    count
    const uint32 kills = Config_GetIntValue(config, Config_ID(gameName, levelName, "MonsterKillObjective"));
    if (kills > 0) {
        s_extraObjective.hasMonsterKills    = true;
        s_extraObjective.monsterKills       = kills;
        s_extraObjective.satisfiesOnItsOwn  = true;
    }

// --- (2) inside Objective_CheckCompleted, immediately after the ore veto (Objective.cpp:1116)
    if (s_extraObjective.hasMonsterKills) {
        // rewardGlobs.current.items[...] is the same counter NERPFunc__GetRockMonstersDestroyed
        // reads (NERPsFunctions.cpp:519).
        const uint32 destroyed = rewardGlobs.current.items[Reward_RockMonsters].numDestroyed;
        if (destroyed < s_extraObjective.monsterKills)
            return false;                       // veto, exactly like crystals/ore
        if (s_extraObjective.satisfiesOnItsOwn)
            return true;                        // and, unlike crystals/ore, SATISFY
    }
```

Cost: roughly 25 lines, one new cfg key, no new warning, no struct change, no exe function
reimplemented. `Reward.h` is already included by `Objective.cpp:11`, and `rewardGlobs` is already
used there at `Objective.cpp:685-687`.

### 2.6 On spending the free flag bits

You *can* add bits `0x2000`…`0x80000000` without growing anything — the field is a `uint32` at
offset 0 of a struct pinned at `0x28c`. But there are two reasons to prefer pure DLL-side state:

* `Objective.cpp:576` tests `flags == OBJECTIVE_GLOB_FLAG_NONE`. Setting a new bit during parse
  suppresses the briefing-advisor fallback. That is a behaviour change with a non-obvious cause.
* Whether any *un-decompiled* exe function reads or writes `0x00500bc0` is **UNDETERMINED**. All 15
  `Objective_*` functions are ours, but `Lego_UnkObjective_CompleteSub_FUN_004262f0`
  (`Objective.cpp:690`) is not, and neither is `Lego_LoadLevel` (`Game.h:1373`), which calls
  `Objective_LoadLevel`.

Recommendation: use the free bits only when a value genuinely must be visible to code that already
reads `objectiveGlobs.flags`; otherwise keep new state in a DLL-side struct as sketched above.

---

## 3. NERPs — the scripting language

### 3.1 What it is

A per-tick bytecode VM. The compiled script is loaded once at level load and the whole program is
re-executed **every game tick** from instruction 0; there is no program counter that persists
across ticks. State persists only in the eight registers and four timers. This is visible directly
in the interpreter:

```
NERPsFile.cpp:355   const NERPsInstruction* instructions = nerpsfileGlobs.instructions;
NERPsFile.cpp:356   const uint32 instrCount = (nerpsfileGlobs.scriptSize / sizeof(NERPsInstruction));
NERPsFile.cpp:366   for (uint32 instrIdx = 0; instrIdx < instrCount; instrIdx++, currCmp = nextCmp) {
```

Registers and timers live in `NERPsRuntime_Globs` (`NERPsFile.h:323-343`, bound at `0x00500958`,
`NERPsFile.cpp:66`): `registers[8]` at offset `0x00`, `timers[4]` at `0x48`. `NERPS_REGISTERCOUNT`
and `NERPS_TIMERCOUNT` are `NERPsFile.h:49-50`.

The RRU Knowledge Base agrees that scripts run "once per game tick (25 times per second)".

### 3.2 The bytecode format

One instruction is exactly four bytes, little-endian, and the interpreter treats the pair as a
single DWORD in places:

```
NERPsFile.h:232   struct NERPsInstruction
NERPsFile.h:234       /*0,2*/ uint16 operand;
NERPsFile.h:235       /*2,2*/ NERPsOpcode opcode;
NERPsFile.h:238   assert_sizeof(NERPsInstruction, 0x4);
```

Opcodes are a bitmask, tested in a fixed priority order (`NERPsFile.h:120-140`):

| Value | Opcode | Meaning | Handled at |
| ---: | --- | --- | --- |
| `0x0` | `Load` | push a sign-extended 16-bit literal | `NERPsFile.cpp:592-593` |
| `0x1` | `Operator` | one of eleven operators, operand = `NERPsOperator` | `NERPsFile.cpp:527-573` |
| `0x2` | `Function` | call built-in, operand = function id | `NERPsFile.cpp:380-526` |
| `0x4` | `Label` | mark a jump target; resets the expression register | `NERPsFile.cpp:576-580` |
| `0x8` | `Jump` | if the expression register is non-zero, `instrIdx = operand` | `NERPsFile.cpp:582-590` |

Operators (`NERPsFile.h:145-158`), operand values 0-10:
`+` (and), `#` (not), `/` (or), `\` (no effect), `?` (test — **no effect**),
`>`, `<`, `=`, `>=`, `<=`, `!=`.

The interpreter's own comment establishes that `?` is functionally inert
(`NERPsFile.cpp:400-403`), and there is a commented-out experiment at `NERPsFile.cpp:371-375` to
prove it.

Two constraints on function calls that any author or assembler must respect
(`NERPsFile.cpp:386-403`, verbatim structure):

* **Arguments cannot be expressions.** They must be an integer literal or a zero-argument function
  that returns a value. Anything else is used as its raw DWORD. The comment gives the worked
  example: `SetR2 SetR2` assembles to `SetR2 0x2001b`, "where `0x20000` is the opcode for functions,
  and `0x1b` is the function ID for `SetR2`" (`NERPsFile.cpp:391-392`).
* **Value-returning functions ignore the condition register**; void functions honour it. So
  `COND ? SetR0 5` works, and `COND ? GetR0` does not gate.

Argument arity is data, not syntax — it comes from the table (`NERPsFile.h:85-96`):
`ARGS_0`, `ARGS_1`, `ARGS_2` return a value; `ARGS_0_NORETURN` … `ARGS_3_NORETURN` do not;
`END_OF_LIST` terminates.

Execution halts on function id 0 (`NERPS_FUNCID_STOP`, `NERPsFile.h:47`) when the expression
register is true or no comparison is pending (`NERPsFile.cpp:382`).

**The script file is raw instructions with no header.** `NERPsFile_LoadScriptFile` does exactly one
thing:

```
NERPsFile.cpp:110   nerpsfileGlobs.instructions = (NERPsInstruction*)Gods98::File_LoadBinary(filename, &nerpsfileGlobs.scriptSize);
```

That is the entire loader. Instruction count is `fileSize / 4`. Nothing validates it.

The community documentation of the `.npl` format matches this exactly, including the opcode values
(`0x0000` constant, `0x0001` compares, `0x0002` function, `0x0004` label, `0x0008` goto).

### 3.3 The function table, and its exact size

```
NERPsFile.h:222   struct NERPsFunctionSignature   // 0xc bytes: const char* name; NERPsFunction function; NERPsFunctionArgs arguments;
NERPsFile.h:355   extern /*const*/ NERPsFunctionSignature (& c_nerpsFunctions)[294];
NERPsFile.cpp:34  ... = *(LegoRR::NERPsFunctionSignature(*)[294])0x004a6948;
NERPsFile.cpp:37  const char* (& LegoRR::c_nerpsOperators)[11] = *(const char*(*)[11])0x004a7710;
```

The table is **exactly adjacent to its neighbour with zero slack**:

```
0x004a6948 + (294 × 0xc = 0xdc8) = 0x004a7710 == &c_nerpsOperators
```

⇒ **The exe function table cannot be extended in place.** There is not one spare byte after it.
(This region is not in `docs/ADDRESS-MAP.md` because `addrlint` indexes `assert_sizeof` types and
scalar bindings, and an array-of-struct reference binding is not one; the arithmetic above is
derived by hand from `NERPsFile.cpp:34` and `NERPsFile.cpp:37`.)

The 294th slot is the terminator, named `"**End Of List**"` (`interop.cpp:3432`). So there are
**293 callable functions, ids 0-292**.

**Cross-verification of the id space.** The public assembler `jgrip/legorr` (`npl.py`, GPL-2.0,
archived) carries a name→id dictionary with 293 entries, ids 0-292, beginning
`0:"Stop", 1:"True", 2:"False", 3:"Null", 4:"GetRandom", …` and containing `27:"SetR2"`. Two
independent facts in our own source confirm it: `NERPS_FUNCID_STOP == 0` (`NERPsFile.h:47`) matches
id 0, and the interpreter comment "`0x1b` is the function ID for `SetR2`" (`NERPsFile.cpp:392`)
gives `0x1b == 27`, matching. **The public id table is correct for this executable.**

### 3.4 What a script can actually do — the useful 293, by category

Line numbers are the body in `game/mission/NERPsFunctions.cpp`. All are OURS.

**Flow, registers, arithmetic** — the entire computational substrate.
`GetR0`-`GetR7` (:1938-1980), `SetR0`-`SetR7` (:2101-2150), `AddR0`-`AddR7` (:1987-2036),
`SubR0`-`SubR7` (:2044-2093), `GetTimer0`-`3` (:180-198), `SetTimer0`-`3` (:205-226),
`True` (:2464), `False` (:1863), `Null` (:1870), `Stop` (:1877),
`GetRandom` (:1825), `GetRandomTrueFalse` (:1831), `GetRandom10` (:1837), `GetRandom100` (:1843).
Eight registers plus four timers plus randomness is enough for a real state machine.

**Mission outcome.**
`SetLevelCompleted` (:659), `SetLevelFail` (:676), `SetGameCompleted` (:667 — **see §7.1, it is
broken**), `SetGameFail` (:683), `SetObjectiveSwitch` (:2305), `GetObjectiveSwitch` (:2312),
`GetObjectiveShowing` (:2323). All route to `Objective_SetStatus`, so §2.4 Gate 2 applies to them
too: a level with `CrystalObjective` set cannot be ended by a script either.

**Messages and pacing** — the story delivery channel.
`SetMessage <line> <flag>` (:2242), `SetMessagePermit` (:2204), `SetMessageWait` (:2219),
`SetMessageTimerValues <a> <b> <c>` (:2226), `GetMessageTimer` (:2236),
`GetMessagesAreUpToDate` (:63), `AdvanceMessage` (:82), `SupressArrow` (:69).
`SetMessage` takes a **1-indexed** line number into the message file (`NERPsFunctions.cpp:2249-2252`).

**Camera.**
`CameraLockOnObject` (:253), `CameraLockOnMonster` (:234), `CameraUnlock` (:263),
`CameraZoomIn` (:272), `CameraZoomOut` (:292), `CameraRotate` (:313),
`SetCameraGotoTutorial <blockPtr>` (:2374), `GetCameraAtTutorial` (:2385),
`AllowCameraMovement` (:92). Camera state lives in `NERPsFile_Globs` `camIsLockedOn` …
`camRotMoved` (`NERPsFile.h:292-301`) and is stepped in `NERPsRuntime_EndExecute`
(`NERPsFile.cpp:742-749`).

**Spawning and threat** — the wave-director primitives, already implemented.
`SetRockMonster <bx> <by>` (:506, spawns `Lego_GetEmergeCreatureID()` at a block),
`SetRockMonsterAtTutorial <blockPtr>` (:2363), `SetCongregationAtTutorial <blockPtr> <start>` (:2349),
`GenerateSlug` (:468), `GetSlugsOnLevel` (:1459), `GetMonstersOnLevel` (:2178),
`GetRockMonstersDestroyed` (:519), `SetMonsterAttackPowerstation` (:357), `SetMonsterAttackNowt` (:385),
`SetRockMonsterHealth` (:606), `SetRockMonsterPainThreshold` (:593), `GetRockMonsterRunningAway` (:579),
`SetAttackDefer` (:486), `SetCallToArms` (:493), `GetCallToArmsButtonClicked` (:500).

**World state queries** — the condition vocabulary.
`GetCrystalsCurrentlyStored` (:1855), `GetCrystalsPickedUp` (:1849), `GetCrystalsUsed` (:1891),
`GetCrystalsStolen` (:1898), `GetOreCurrentlyStored` (:1925), `GetOrePickedUp` (:1919),
`GetOreUsed` (:1905), `GetOreStolen` (:1912), `GetStudCount` (:1429), `GetOxygenLevel` (:447),
`GetMiniFiguresOnLevel` (:1465), `GetPathsBuilt` (:1423), `GetHiddenObjectsFound` (:526),
`GetBuildingsTeleported` (:2189), plus a large, regular family:
`Get{Barracks,Docks,Geodome,Powerstations,ToolStores,Gunstations,Teleports,VehicleTeleports,UpgradeStations}Built`
(:1096-1144), the `GetPowered…Built` variants (:1042-1090), and the `GetLevel1…`/`GetLevel2…`
variants (:1150-1252). Vehicles on level: `Get{SmallHelicopters,GraniteGrinders,RapidRiders,SmallDiggers}OnLevel`
(:1435-1453).

**Resource injection.** `AddPoweredCrystals <n>` (:838), `AddStoredOre <n>` (:850). These are how a
script grants a reward or stages a setback.

**Block-pointer interaction** — the map-anchored primitives. A `blockPointerMap` assigns numeric ids
to map cells; `BlockPointer` is `{Point2I blockPos; uint32 id;}` (`NERPsFile.h:195-201`) and
`Lego_Level::blockPointers[LEGO_MAXBLOCKPOINTERS]` holds them (`Game.h:439`).
`GetUnitAtBlock` (:2440), `GetMonsterAtTutorial` (:2451), `GetTutorialBlockIsGround` (:2396),
`GetTutorialBlockIsPath` (:2407), `SetTutorialBlockIsGround` (:2418), `SetTutorialBlockIsPath` (:2429),
`GetTutorialCrystals` (:865), `SetTutorialCrystals` (:713), `GetTutorialBlockClicks` (:877),
`SetTutorialBlockClicks` (:701), `SetOreAtIconPositions` (:725), `SetTutorialPointer` (:690),
`GetRecordObjectAtTutorial` (:402), `GetRecordObjectAmountAtTutorial` (:421),
`MakeSomeoneOnThisBlockPickUpSomethingOnThisBlock` (:2334).
All thirteen `NERPsTutorialAction` verbs are enumerated at `NERPsFile.h:99-116` and dispatched
through the OURS `NERPsRuntime_TutorialActionCallback` (`NERPsFile.h:688`).

**UI restriction / tutorial framing.**
`SetTutorialFlags` (:2158) / `GetTutorialFlags` (:1931) over the 13 flags at `NERPsFile.h:63-81`
(`NOICONS`, `NOMAP`, `NOOBJECTS`, `NORADAR`, `NOOPTIONS`, `NOPRIORITIES`, `NOCALLTOARMS`, `NOINFO`,
`NOMULTISELECT`, `NOCYCLEUNITS`, `NOHELPWINDOW`, `NOCAMERA`, `NOBLOCKACTION`), plus the coarse
helpers `ClickOnlyObjects` (:102), `ClickOnlyMap` (:115), `ClickOnlyIcon` (:128),
`ClickOnlyCalltoarms` (:141), `DisallowAll` (:154), `SetPauseGame` (:619), `SetGameSpeed` (:49),
`GetAnyKeyPressed` (:626). About 60 further functions are per-icon `Get…IconClicked` /
`Set…IconClicked` / `Flash…Icon` triples (:1258-1424, :1485-1824) — these are what make guided
sequences possible.

**Building levels.** `Set{Barracks,Docks,GeoDome,Powerstation,ToolStore,Gunstation,TeleportPad,
SuperTeleport,UpgradeStation}Level` (:925-981) — a script can promote a structure's upgrade level
directly.

That is a genuinely capable mission language: state, arithmetic, randomness, timers, a rich world
query set, spawning, camera control, resource manipulation, timed narration, and full UI gating.

### 3.5 The message file — pure data, fully ours

`NERPsFile_LoadMessageFile` (`NERPsFile.cpp:115-282`) parses a plain text file, line-oriented:

* Any control character (`< ' '`) terminates a line (`NERPsFile.cpp:153-157`) — tabs included.
* **Underscores become spaces** (`NERPsFile.cpp:150`), so text cannot contain a literal underscore.
* `:key  path\to\image.bmp` defines an image (`NERPsFile.cpp:169-207`).
* `$key  path\to\sound` defines a sound, no extension (`NERPsFile.cpp:211-252`).
* Everything else is a text line, appended to `lineList` in order (`NERPsFile.cpp:253-268`).

`SetMessage n` then displays line `n` (1-indexed, `NERPsFunctions.cpp:2249-2252`). Inline markers
`#soundName#` and `<imageKey>` are stripped and resolved by `NERPs_Level_NERPMessage_Parse`
(`NERPsFile.h:663-667`), which is OURS (installed `interop.cpp:3137`).

Community documentation describes the same shape: text lines carrying a `#identifier#`, and
`$identifier  sounds\path` definitions.

**So all mission narration is authorable as a text file with no tooling whatsoever.**

### 3.6 ★ Can we author new scripts as data? — Yes, and the right answer is an in-DLL assembler

Today, no: `NERPsFile_LoadScriptFile` reads raw bytes (`NERPsFile.cpp:110`), so a script must be
pre-assembled bytecode. Community tooling exists (`jgrip/legorr`'s `npl.py`; the RRU "Npl Scripter"),
but it is external, archived, and carries its own hard-coded id table that could drift from a
modified executable.

**There is a strictly better option available to this project, and it is small.**
`NERPsFile_LoadScriptFile` is OURS and installed at `interop.cpp:3107`. And — the decisive
detail — the DLL already holds a live reference to the exe's own function table, complete with
**names and arity**:

```
NERPsFile.cpp:34  c_nerpsFunctions  → 0x004a6948, 294 × {name, function, arguments}
NERPsFile.cpp:37  c_nerpsOperators  → 0x004a7710, 11 × const char*
```

`NERPs_HookFunction` already resolves names against it by case-insensitive compare
(`NERPsFile.cpp:82-87`). Therefore an assembler living inside the DLL needs **no hard-coded id
table at all** — it reads its own instruction set out of the running executable, and it can
validate argument counts for free from the `arguments` field. It cannot drift, and it survives any
future re-ordering of the table.

Sketch. Text detection is deliberately conservative: bytecode is a multiple of 4 and its third and
fourth bytes are a valid opcode mask, so anything that fails that test and looks like ASCII is
treated as source.

```cpp
// game/mission/NERPsFile.cpp — OURS, installed over 0x004530b0 (interop.cpp:3107).

/// CUSTOM: resolve a built-in name to its table index, using the exe's own table.
///         Mirrors NERPs_HookFunction (NERPsFile.cpp:82-87).
static sint32 NERPs_FindFunctionID(const char* name)
{
    for (uint32 i = 0; i < _countof(c_nerpsFunctions); i++) {
        const char* n = c_nerpsFunctions[i].name;
        if (n != nullptr && ::_stricmp(n, name) == 0)
            return static_cast<sint32>(i);
    }
    return -1;
}

/// CUSTOM: resolve an operator spelling to its operand, using the exe's own table.
static sint32 NERPs_FindOperatorID(const char* tok)
{
    for (uint32 i = 0; i < _countof(c_nerpsOperators); i++) {
        if (c_nerpsOperators[i] != nullptr && std::strcmp(c_nerpsOperators[i], tok) == 0)
            return static_cast<sint32>(i);
    }
    return -1;
}

/// CUSTOM: assemble NERPs source text into instructions.
///  Pass 1: emit instructions, record `Name:` label definitions, leave `:Name` jump
///          operands as 0 and remember their positions.
///  Pass 2: patch jump operands to the recorded instruction indices.
///  Buffer MUST come from Gods98::Mem_Alloc — NERPsFile_Free frees it (NERPsFile.cpp:297).
static NERPsInstruction* NERPs_AssembleText(const char* src, uint32 srcSize, OUT uint32* outByteSize);
```

and the hook point itself:

```cpp
// NERPsFile.cpp:110 becomes:
    uint32 rawSize = 0;
    void* raw = Gods98::File_LoadBinary(filename, &rawSize);
    if (raw == nullptr)
        return false;

    if (DeepCore::settings.nerpsTextScripts && !NERPs_LooksLikeBytecode(raw, rawSize)) {
        uint32 codeSize = 0;
        NERPsInstruction* code = NERPs_AssembleText((const char*)raw, rawSize, &codeSize);
        Gods98::Mem_Free(raw);
        nerpsfileGlobs.instructions = code;
        nerpsfileGlobs.scriptSize   = codeSize;   // instrCount = scriptSize / 4 (NERPsFile.cpp:356)
        return (code != nullptr);
    }

    nerpsfileGlobs.instructions = (NERPsInstruction*)raw;
    nerpsfileGlobs.scriptSize   = rawSize;
    return true;
```

A grammar that covers everything the interpreter can express, in the community's established
syntax (`;` comments, `Name:` label definition, `:Name` jump target, `COND ? ACTION`):

```
; wave 1 at t = 60s, then every 45s, three waves total
SetTimer0 0
Loop:
GetTimer0 > 60000 ? SetRockMonster 12 9
GetTimer0 > 60000 ? SetTimer0 15000
GetTimer0 > 60000 ? AddR0 1
GetR0 >= 3 ? SetMessage 4 0
GetR0 >= 3 ? Stop
:Loop
```

Cost estimate: ~300 lines of C++ in `NERPsFile.cpp`, one `DeepCore` gate defaulting off (so the
vanilla path is byte-identical), no new struct, no exe function reimplemented, no new warning.
**This is the highest-value item in this document.** It converts mission scripting from "requires an
archived external toolchain and a hard-coded id table" into "edit a text file next to the level".

Two secondary benefits worth naming:

* A **disassembler** falls out of the same table for free (`funcId → c_nerpsFunctions[id].name`), so
  the stock campaign's scripts become readable — which is the fastest way to learn the idioms
  actually used by the original missions.
* A **table dumper** (iterate `c_nerpsFunctions`, print `i`, `name`, `arguments`) is ~10 lines and
  would let anyone with the game installed produce the authoritative id/arity list for this exact
  executable, confirming or refuting the public table in §3.3 empirically.

### 3.7 Extending the language beyond 293 functions

Also possible, also DLL-side, but with one caveat that must be settled first.

The exe table cannot grow (§3.3: zero slack). But `operand` is a `uint16`, so ids up to 65535 are
representable, and every consumer of `c_nerpsFunctions` in the interpreter is OURS. A DLL-side
extension table dispatched for `funcId >= 294`:

```cpp
// NERPsFile.cpp — DLL-side extension table. Does NOT touch 0x004a6948.
static std::vector<NERPsFunctionSignature> s_nerpsExtFunctions;

static const NERPsFunctionSignature& NERPs_GetSignature(uint32 funcId)
{
    if (funcId < _countof(c_nerpsFunctions))
        return c_nerpsFunctions[funcId];
    const uint32 ext = funcId - _countof(c_nerpsFunctions);
    return s_nerpsExtFunctions[ext];   // caller must range-check first
}
```

with `NERPsFile.cpp:384` (`NERPsFunctionArgs nargs = c_nerpsFunctions[funcId].arguments;`) and every
`c_nerpsFunctions[funcId].function(...)` in `NERPsRuntime_Execute` routed through it, plus the same
in `NERPsRuntime_LoadLiteral` (`NERPsFile.cpp:336-341`).

**The caveat.** `NERPsRuntime_LoadLiteral` is implemented but **not installed** —
`interop.cpp:3116` is commented out with "internal, no need to hook these". Our `Execute` calls our
`LoadLiteral` directly, so the interpreter is self-consistent; but the exe's copy at `0x004535a0`
is still live and indexes `c_nerpsFunctions[instruction->operand]` with no bound. Whether any exe
code reaches it is **UNDETERMINED**. Before shipping extended ids, either (a) uncomment
`interop.cpp:3116` so the exe copy is replaced too — a one-line change to an already-written
function — or (b) restrict extended ids to non-`ARGS_0` forms, which `LoadLiteral` ignores
(`NERPsFile.cpp:336`).

This is a *later* item. The assembler (§3.6) unlocks far more content per unit of risk, because 293
functions is already a large vocabulary.

---

## 4. What a new level needs, and what breaks

### 4.1 Config scoping

`Lego_ID(...)` expands to `Config_ID(legoGlobs.gameName, ...)` (`Game.h:793`), so every level key is
`Lego*::<LevelName>::<Key>`. Key matching is **case-insensitive** — `Config_FindItem` compares with
`::_stricmp` (`engine/core/Config.cpp:709`) — so `NERPFile` and `NerpsFile` are the same key.

### 4.2 Keys read by code we own (citable from this tree)

| Key | Read at | Purpose |
| --- | --- | --- |
| `NextLevel` | `FrontEnd.cpp:3523`, `:3554` | next level in the linear chain; also `Lego_Level::nextLevelID` (`Game.h:435`) |
| `LevelLinks` | `FrontEnd.cpp:4318` | comma-separated unlock graph, **max 15** (`FrontEnd.cpp:4317`) |
| `FullName` | `FrontEnd.cpp:3543` | display name; `_` → space (`FrontEnd.cpp:3551`) |
| `Video` | `FrontEnd.cpp:3668` | pre-level movie |
| `SeeThroughWalls` | `Game.cpp:2968` | |
| `Priorities` | `interface/Priorities.cpp:87` | per-level priority list |
| `CrystalObjective`, `OreObjective`, `TimerObjective`, `ConstructionObjective`, `BlockObjective` | `Objective.cpp:529-566` | §2.2 |
| `DontShowObjectiveAdvisor` and the two `…Acheive…`/`…Failed…` variants | `Objective.cpp:388-396` | §2.4 Gate 1 |
| `ObjectiveText` | `Objective.cpp:570` | briefing text file |
| `ObjectiveImage<W>x<H>`, `ObjectiveAcheivedImage<W>x<H>`, `ObjectiveFailedImage<W>x<H>`, `ObjectiveAcheivedAVI` | `Objective.cpp:401-499` | briefing panel art; note the resolution is baked into the key name |

### 4.3 Keys read by `Lego_LoadLevel`, which is still EXE

`Lego_LoadLevel` is an address macro (`Game.h:1373`, `0x004297c0`). Its key list therefore cannot be
derived from this tree. The RRU Knowledge Base documents the load-order list as:
`Seed, BoulderAnimation, FallinMultiplier, blocksize, digdepth, useroof, roofheight, roughlevel,
surfacemap, predugmap, terrainmap, blockPointerMap, CryOreMap, ErodeMap, EmergeMap, AIMap, PathMap,
FallinMap, textureset, EmergeTimeOut, olistfile, elistfile, selboxheight, PTLFile, NERPFile,
NERPMessageFile, EmergeCreature, SafeCaverns, IntroAVI, StartFP, NoDrain, OxygenRate, UpgradeTime,
MinifigureUpgradeTime, VehicleUpgradeTime, BuildingUpgradeTime, TrainTime, NoMultiSelect, NoAutoEat,
EndGameAVI1, EndGameAVI2, AllowRename, RecallOLObjects, GenerateSpiders, DisableToolTipSounds,
DisableEndTeleport, DragBoxRGB, BuildingTolerance, BuildingMaxVariation, MaxStolen, Slug, SlugTime,
InitialSlugTime, RockFallStyle, noFallins, FogColourRGB, HighFogColourRGB, FogRate, nextlevel`.
Treat that list as **external, unverified against this tree** — but note that many of the same
fields do appear in `Lego_Level` (`Game.h:400-457`: `RoofHeight`, `RoughLevel`, `UseRoof`,
`safeCaverns`, `EmergeCreature`, `EmergeTimeOut`, `SlugTime`, `Slug`, `MaxStolen`, `NoDrain`,
`StartFP`, `OxygenRate`, `TrainTime`, `UpgradeTimes[]`, `BuildingTolerance`,
`BuildingMaxVariation`, `BoulderAnimation`, `FullName`, `nextLevelID`, `IsStartTeleportEnabled`),
which is strong corroboration.

`PTLFile` feeds `PTL_Initialise(fname, gameName)` (`PTL.cpp:34`, OURS, installed
`interop.cpp:3899`) — a table of at most 40 `from → to` event-type remaps (`PTL.h:29`,
`PTL.cpp:44-49`) applied by `PTL_TranslateEvent` (`PTL.cpp:57`). That is the per-level input
retargeting used by tutorials, and it is pure data.

### 4.4 The briefing text file format

Parsed by `Objective_LoadObjectiveText` (`Objective.cpp:43-368`), OURS.

* The file is scanned for two section headers: `[BEGIN]` and `[<ShortLevelName>]`.
* **Short level name** is everything after the *second* colon of the level id — the parser counts
  colons and takes `s+1` after the second (`Objective.cpp:132-150`). So `Levels::Driller01` →
  `Driller01`, and the section header must be `[Driller01]`.
* Inside the level section, four prefixes are recognised, case-insensitively
  (`Objective.cpp:161-162`, matched at `:201-206`):
  `Objective:`, `Completion:`, `Failure:`, `CrystalFailure:` — index order matters, it is the
  `LevelStatus` index.
* Escapes, and **only these two**: `\n` → newline, `\a` → page break (which increments the page
  count). The backslash itself is replaced by a space (`Objective.cpp:222-240`). Uppercase `\N`
  and `\A` are **not** valid — the parser is case-sensitive here and says so
  (`Objective.cpp:224`).
* Underscores are **not** converted to spaces here (unlike NERPs message files) —
  `Objective.cpp:171`.
* The file **must use LF line endings, not CRLF** (`Objective.cpp:167`). The newline strip is a
  blind `buff[strlen(buff)-1] = '\0'` (`Objective.cpp:196`), so a CR would survive into the text.
* `[BEGIN]` consumes exactly one line, and it is only read when the level section was found
  (`Objective.cpp:186`, `:248-257`).

Text window capacity is 1024 characters per status (`Objective.cpp:112`).

### 4.5 ★ Campaign progression, and the save-format trap

**Two independent structures.**

*The linear set.* `Front_LoadLevelSet` (`FrontEnd.cpp:3513`) walks `Main::StartLevel` then
`NextLevel` repeatedly, counting first and then filling arrays (`FrontEnd.cpp:3520-3555`). The index
into this set is `setIndex` (`FrontEnd.cpp:4312-4314`). Called twice —
`Front_LoadLevelSet(config, &frontGlobs.missionLevels, "StartLevel")` and
`… "TutorialStartLevel"` (`FrontEnd.cpp:3960-3961`).

*The unlock graph.* `Front_LevelSet_LoadLevelLinks` (`FrontEnd.cpp:4292`) recursively builds a
`LevelLink` tree from the `LevelLinks` key. A link whose name is **not in the linear set** is
dropped: `Front_LevelSet_IndexOf` returns -1, the allocation is freed and `nullptr` returned
(`FrontEnd.cpp:4312`, `:4348-4350`). Unlocking: completing a level enables every level it links to
(`Front_Levels_UpdateAvailable_Recursive`, `FrontEnd.cpp:4433-4491`; the completion test is
`search->saveReward[link->setIndex].flags & SAVEREWARD_FLAG_COMPLETED`, `FrontEnd.cpp:4458`).

`LevelLink` is one of the few structs in this project that has been **deliberately grown**: declared
`0x14` in the original, now `0x1c` with `linkIndex` and `debugCompleted` appended
(`FrontEnd.h:235-248`). That is legal only because every `LevelLink` is heap-allocated by our code
(`FrontEnd.cpp:4300`) and never overlaid on the exe's data segment.

**Save invalidation — the trap.** `Front_Save_ReadSaveFile` (`FrontEnd.cpp:4717`, OURS) counts the
reachable links and compares:

```
FrontEnd.cpp:4727   Front_LevelLink_RunThroughLinks(frontGlobs.startMissionLink, Front_LevelLink_Callback_IncCount, &missionsCount);
FrontEnd.cpp:4735   Gods98::File_Read(saveData, (sizeof(SaveData) - 0x8), 1, file);
FrontEnd.cpp:4738   if (missionsCount != saveData->missionsCount && !readOnly) {
FrontEnd.cpp:4743       Front_Save_WriteSaveFiles(saveIndex, nullptr);   // blank the save
FrontEnd.cpp:4748       std::memset(saveData, 0, sizeof(SaveData));
```

⇒ **Adding or removing a single mission blanks every existing save file, all six slots**, because
`Front_Save_LoadAllSaveFiles` (`FrontEnd.cpp:4853`) runs the check over all of them at startup. The
same check guards writes (`FrontEnd.cpp:4823-4827`).

Worse, the count alone is not sufficient. Completion is recorded by **`setIndex`** — the position in
the `NextLevel` chain (`Reward.cpp:305`, `:379` → `Front_Save_SetLevelCompleted`) — while the table
is **sized by link-reachable count**. Three constraints follow:

1. **Reordering the `NextLevel` chain silently remaps completion flags** even when the count is
   unchanged. A save that survives the count check will show the wrong levels completed.
2. **Every level in the `NextLevel` chain must be reachable through `LevelLinks` from the start
   level.** If it is not, `missionsCount < LevelSet::count`, and a reachable link can hold a
   `setIndex >= missionsCount`, which `FrontEnd.cpp:4458` reads out of bounds of a
   `missionsCount`-sized array. `Front_Save_SetRewardLevel` and `Front_Save_SetLevelCompleted` are
   bounds-checked (`FrontEnd.cpp:4910`, `:4934`) and would silently drop the record; the
   `UpdateAvailable` read is not.
3. **Tutorials must stay at eight.** `Front_Save_SetLevelCompleted` hard-codes it:

```
FrontEnd.cpp:4910   if (currSave != nullptr && levelIndex < currSave->missionsCount) {
FrontEnd.cpp:4912       if (!Front_IsTutorialSelected() && levelIndex >= 8) { // NUM_TUTORIALS
FrontEnd.cpp:4913           currSave->missionsTable[levelIndex].flags |= SAVEREWARD_FLAG_COMPLETED;
FrontEnd.cpp:4915       else if (levelIndex < 8) { // NUM_TUTORIALS
```

A ninth tutorial gets `levelIndex == 8` with `Front_IsTutorialSelected()` true, so **neither branch
fires and its completion is never recorded.** Tutorials and missions also share one index space in
`missionsTable`, which is sized only from the mission link count.

**Recommended mitigation — a campaign-scoped save directory.** Both save entry points are OURS and
both build the path inline:

```
FrontEnd.cpp:4721   std::sprintf(buff, "%s\\%i.sav", "Saves", saveIndex);      // Front_Save_ReadSaveFile
FrontEnd.cpp:4780   std::sprintf(buff, "%s\\%i.sav", "Saves", saveIndex);      // Front_Save_WriteSaveFiles
FrontEnd.cpp:4842   std::sprintf(buff, "%s\\%i.osf", "Saves", saveIndex);      // ObjectRecall companion
```

Replacing the literal `"Saves"` with `DeepCore::settings.saveDirectory` (default `"Saves"`, so
vanilla is byte-identical) gives the overhaul its own save namespace and makes the mission-list
change harmless. `Gods98::File_MakeDir` is already called at `FrontEnd.cpp:4777`.

The fourth save artefact — the slot thumbnail — is **already** cfg-driven and needs no code change:
`Config_ID(gameName, "Menu::SaveImage", "Path")` then `"%s\\%d.dat"` (`FrontEnd.cpp:2463-2464` and
`:2600-2601`). An appended config can point it elsewhere on its own.

**One honest wrinkle.** The `.osf` *write* is ours (`FrontEnd.cpp:4842-4843`), but the matching
`ObjectRecall_LoadRROSFile` is called by the EXE `Lego_LoadLevel` (`interop.cpp:3770` comment
"used by: Lego_LoadLevel"), so the read path is built exe-side and would not follow the redirect.
`ObjectRecall_LoadRROSFile` itself is OURS (`object/ObjectRecall.cpp:154`), so the fix is to rewrite
the directory component of the incoming path there. Small, contained, and must be done in the same
change or object-recall saves desync.

### 4.6 The reward screen is not ours

`Reward.h` carries **48 live exe macros** against 4 bodies. Everything that draws, scores and paces
the end-of-mission screen — `Reward_CreateLevel`, `Reward_Prepare`, `Reward_Show`, `Reward_DrawScore`,
all the `RewardQuota_*` counters — is 1999 machine code (`Reward.h:272-477`). Ours are only
`Reward_GotoSaveMenu` (`Reward.cpp:297`), `Reward_GotoAdvance` (`Reward.cpp:377`) and two helpers.

Consequence: reward *content* is configurable through the same cfg data the exe already reads
(`Reward_LoadGraphics`, `Reward_LoadButtons`, `Reward_LoadLevelItemImportance`,
`Reward_LoadLevelItemQuota` all take a `const Gods98::Config*`), but reward *behaviour* cannot be
changed without a complete reimplementation, and there is no trampoline. **Do not plan work here.**

---

## 5. Text, lore and localisation

### 5.1 Where each kind of text lives

| Kind | Storage | Owner | Notes |
| --- | --- | --- | --- |
| Mission briefing / completion / failure | loose text file named by `ObjectiveText` | **OURS** (`Objective.cpp:43`) | format in §4.4 |
| In-mission narration | loose text file named by `NERPMessageFile` | **OURS** (`NERPsFile.cpp:115`) | format in §3.5 |
| Level display name | cfg `FullName` | **OURS** (`FrontEnd.cpp:3543`) | `_` → space |
| Object names | cfg `ObjectNames` → `legoGlobs.langPowerCrystal_name` … (`Game.h:513-519`) | **EXE** `Lego_LoadObjectNames` (`Game.h:1486`) | data-editable, code not ours |
| Encyclopedia entries | cfg `Encyclopedia::<TypeName>` → `.epb`-style text file | **OURS** (`Encyclopedia.cpp:53-55`) | see below |
| Engine text messages (26 fixed slots) | cfg, keyed by registered name | **EXE** `Lego_LoadTextMessages` (`Game.h:1918`); consumers OURS (`TextMessages.cpp`) | slots fixed at `Text_Type_Count` |
| Tooltips | cfg | **EXE** `Lego_LoadToolTips` (`Game.h:1926`); `ToolTip.cpp` OURS | |
| Info messages | cfg | **EXE** `Lego_LoadInfoMessages` (`Game.h:1922`) | |

**Encyclopedia is the friendliest lore surface.** `Encyclopedia_Initialise` (`Encyclopedia.cpp:60`)
opens one file per object type, keyed `Lego*::Encyclopedia::<ObjectTypeName>`:

```
Encyclopedia.cpp:53   #define Encyclopedia_ID(name)  Config_ID(gameName, "Encyclopedia", name)
Encyclopedia.cpp:70       Encyclopedia_GetFileName(fileName, Object_GetTypeName(LegoObject_Vehicle, (LegoObject_ID)i));
```

Counts come from `Lego_GetObjectTypeIDCount` (`Encyclopedia.cpp:66`, `:81`, `:96`, `:111`), so **any
object type you add to `Lego.cfg` automatically gets an encyclopedia slot with no code change.**
This module has also already been extended DLL-side without touching the pinned
`Encyclopedia_Globs` (`0x24`) — five extra file handles live as file-scope statics
(`Encyclopedia.cpp:39-43`) covering ProcessedOre, ElectricFence, Barrier, Dynamite and OohScary.
That is the pattern to copy.

**Fixed-slot text is a hard ceiling.** `Text_Globs` is pinned at `0x4dc`
(`TextMessages.h:50-81`) with every array sized `[Text_Type_Count]`, and `Text_Load`
(`TextMessages.cpp`) registers exactly 26 names. New *kinds* of engine message would grow that
struct — forbidden. New *wording* for the existing 26 is pure data.

### 5.2 A three-line engine fix worth taking: per-level briefing titles

The three title strings are read from **`Main` scope, not level scope**:

```
Objective.cpp:284   title = Gods98::Config_GetTempStringValue(config, Main_ID("MissionBriefingText"));
Objective.cpp:309   title = Gods98::Config_GetTempStringValue(config, Main_ID("MissionCompletedText"));
Objective.cpp:333   title = Gods98::Config_GetTempStringValue(config, Main_ID("MissionFailedText"));
```

`Main_ID(x)` is `Config_ID(gameName, "Main", x)` (`Game.h:794`). So every mission in the game shares
one briefing title, one completion title and one failure title. Making them level-scoped with a
`Main` fallback is three edits in a function we own:

```cpp
// Objective.cpp:284 — level-scoped with Main fallback.
title = Gods98::Config_GetTempStringValue(config, Config_ID(gameName, levelName, "MissionBriefingText"));
if (title == nullptr)
    title = Gods98::Config_GetTempStringValue(config, Main_ID("MissionBriefingText"));
```

`gameName` and `levelName` are already parameters of `Objective_LoadObjectiveText`
(`Objective.cpp:43`) and are currently unused inside it apart from the level-section search. Zero
risk to vanilla (absent key ⇒ identical behaviour), and it is the difference between "MISSION
BRIEFING" on every screen and a per-mission headline.

### 5.3 Shipping non-destructively with `-cfgadd`

The engine supports appending override configs on top of `Lego.cfg`:

```
engine/Main.h:307     std::vector<std::string> configAppends;  // -cfgadd <filename>
engine/Main.cpp:1371  // Usage: -cfgadd <filename>
engine/Main.cpp:1374  for (size_t i = 0; i < args.size(); i++)
engine/Main.cpp:1377      if (ArgumentEquals(arg, "-cfgadd") && i + 1 < args.size())
```

It is repeatable (`configAppends` is a vector) and paths are normalised
(`engine/Main.cpp:1378`). `-cfgfile <name>` replaces the base config outright
(`engine/Main.cpp:1365-1369`), and `-cfgfirst` prefers loose files over WADs
(`engine/Main.cpp:1361`).

**So yes: a campaign can ship as a single `-cfgadd DeepCore/Campaign.cfg` plus loose data files,
with the player's original `Lego.cfg` and WADs untouched.** That is the correct distribution shape.

⚠ One caveat I could not resolve from source and must mark **UNDETERMINED**: the exact merge
semantics of `configAppends` — specifically whether a `LevelLinks` value in an appended file
*replaces* or *duplicates* the base key. `Config_FindItem` returns the first match
(`engine/core/Config.cpp:709`), so append order matters. Verify before relying on override
behaviour for existing levels; adding *new* level blocks is unaffected either way.

---

## 6. What is realistically deliverable in weeks

### 6.1 The minimum viable new mission — no new map file

Every map is a loose file referenced by a cfg key (`surfacemap`, `predugmap`, `terrainmap`, …). A
new level block may point at the **same** map files as an existing level while changing everything
else. That yields a genuinely different mission with **zero new binary assets**:

```
; DeepCore/Campaign.cfg  — appended with -cfgadd
Lego* {
  Levels {
    DeepCore01 {
      FullName                    Aftershock
      ; --- reuse an existing level's geometry verbatim ---
      SurfaceMap                  Levels/GameLevels/Level01/Surf_01.map
      PreDugMap                   Levels/GameLevels/Level01/Dugg_01.map
      TerrainMap                  Levels/GameLevels/Level01/High_01.map
      CryOreMap                   Levels/GameLevels/Level01/Cror_01.map
      PathMap                     Levels/GameLevels/Level01/Path_01.map
      BlockPointerMap             Levels/GameLevels/Level01/Emrg_01.map
      OListFile                   Levels/GameLevels/Level01/Level01.ol
      TextureSet                  Rock
      ; --- everything below is new content ---
      EmergeCreature              IceMonster
      OxygenRate                  6
      NERPFile                    DeepCore/Scripts/DeepCore01.npl
      NERPMessageFile             DeepCore/Text/DeepCore01.txt
      ObjectiveText               DeepCore/Text/Objectives.txt
      DontShowObjectiveAdvisor    TRUE          ; REQUIRED — see §2.4 Gate 1
      TimerObjective              900:HitTimeFailObjective
      NextLevel                   DeepCore02
      LevelLinks                  DeepCore02
    }
  }
}
```

Reusing the map but changing `EmergeCreature`, oxygen, the objective, the script and the prose
produces a mission that plays differently. Changing only the `PreDugMap` (a small text/binary grid)
reshapes the cavern layout without any art. Neither needs the engine.

The exact map-key spellings above come from the external key list in §4.3 and are
**unverified against this tree** — `Lego_LoadLevel` is EXE. Everything below the divider is verified
(§4.2).

### 6.2 Weeks, not months

| # | Deliverable | Kind | Files | Confidence |
| --- | --- | --- | --- | --- |
| 1 | **In-DLL NERPs assembler + disassembler** | engine, ~300 lines | `mission/NERPsFile.cpp`, `DeepCore.{hpp,cpp}` | high — every consumer is ours |
| 2 | **Campaign-scoped save directory** | engine, ~30 lines | `front/FrontEnd.cpp` ×3, `object/ObjectRecall.cpp` | high |
| 3 | **Per-level briefing titles** | engine, ~9 lines | `mission/Objective.cpp` | high |
| 4 | **Fix `SetGameCompleted`** (§7.1) | engine, 3 lines | `mission/NERPsFunctions.cpp` | high |
| 5 | **A 4-6 mission side campaign** reusing stock maps | pure data | `-cfgadd` bundle | medium — depends on 1 |
| 6 | **Encyclopedia entries** for every object the overhaul adds | pure data | text files + cfg | high |
| 7 | **New objective types** (§2.5), 2-3 of them | engine, ~25 lines each | `mission/Objective.cpp` | high |
| 8 | Extended NERPs function ids (§3.7) | engine | `mission/NERPsFile.cpp` | medium — needs the `LoadLiteral` question settled |

Items 1-4 are a single focused session each and unblock everything else. Item 5 is the actual
content and is where the weeks go.

### 6.3 What is *not* deliverable, and why

* **Reward-screen behaviour** — 48 live exe macros, no trampoline (§4.6).
* **New engine text-message *kinds*** — `Text_Globs` is pinned at `0x4dc` (§5.1).
* **More than eight tutorials** — hard-coded `NUM_TUTORIALS` (§4.5 constraint 3).
* **More than 15 `LevelLinks` per level** — fixed local array (`FrontEnd.cpp:4317`).
* **New object-type ids** — settled dead end, `docs/research/type-loader-reimplementation.md` §4 and
  `docs/WORKLOG.md:93-121`.
* **Save compatibility across a campaign change** — structurally impossible without §4.5's
  redirection; the count check is by design (`FrontEnd.cpp:4738`).

---

## 7. Defects found while reading

### 7.1 `NERPFunc__SetGameCompleted` fails the level instead of completing it

`NERPsFunctions.h:296-299` declares it as an **alias** of `SetLevelCompleted`, both at
`0x00454e30`, with the aliasing `#define` commented out. But the body diverges:

```cpp
// NERPsFunctions.cpp:659
sint32 __cdecl LegoRR::NERPFunc__SetLevelCompleted(sint32* stack)
{
    Objective_SetStatus(LEVELSTATUS_COMPLETE);
    return_VOID(1);
}

// NERPsFunctions.cpp:665-673
// ALIAS: NERPFunc__SetLevelCompleted
// <LegoRR.exe @00454e30>
sint32 __cdecl LegoRR::NERPFunc__SetGameCompleted(sint32* stack)
{
    if (!(legoGlobs.flags1 & GAME1_LEVELENDING)) {
        Objective_SetStatus(LEVELSTATUS_FAILED);      // <-- FAILED, in "SetGameCompleted"
    }
    return_VOID(1);
}
```

It is installed into the table at `interop.cpp:3250` (`NERPs_hook_function(SetGameCompleted)`), so a
script calling `SetGameCompleted` — which is precisely what the *final mission of a campaign* does —
shows the failure screen. Compare the neighbours: `SetLevelFail` (`:676`) and `SetGameFail`
(`:683`) both correctly call `LEVELSTATUS_FAILED`, and the `GAME1_LEVELENDING` guard looks
copy-pasted. This is upstream OpenLRR code, not project code. **High confidence, unverified at
runtime.** Fix is three lines; recommend also keeping the guard, since re-entering `SetStatus`
during teleport-out is what it appears to protect against.

### 7.2 The built-in crystal and ore objectives cannot complete a level

Detailed in §2.3-§2.4. `CrystalObjective`/`OreObjective` only ever `return false`
(`Objective.cpp:1107-1116`) and never `return true`; and `CrystalObjective` additionally makes
`Objective_SetStatus` a no-op (`Objective.cpp:650-651`), which disables the briefing and both end
screens for that level. Both behaviours are annotated in-source as inherited puzzles
("What's the purpose of the … flag check??", `Objective.cpp:649`), so this is presumed faithful to
the original rather than a fork regression — **UNDETERMINED**, and *not* a candidate for "fixing"
blind, because stock levels may depend on it.

### 7.3 `Front_LoadLevelSet` will hang on a cyclic `NextLevel` chain

```
FrontEnd.cpp:3520   sint32 count = 0;
FrontEnd.cpp:3521   do {
FrontEnd.cpp:3522       count++;
FrontEnd.cpp:3523       nextName = Gods98::Config_GetTempStringValue(config, Config_ID(legoGlobs.gameName, nextName, "NextLevel"));
FrontEnd.cpp:3524   } while (nextName != nullptr);
```

There is no visited set and no iteration cap. A campaign author who writes
`A.NextLevel = B`, `B.NextLevel = A` gets an infinite loop at startup with no diagnostic. Low
severity (authoring error, not a runtime path) but a one-line cap with a `Config_FatalItemF` would
turn a hang into a message — worth doing at the same time as any campaign work.

---

## 8. DECISION

**New campaign and story content is DATA-ONLY. The engine work required is optional, small, and
confined to files this project already owns outright.**

The claim being tested — "the mission layer is almost entirely ours" — is **verified, with two
corrections**: the free flag-bit count is 19 and not 20 (§1.5), and "the NERPs VM is 100 % ours" is
true of the VM and all 293 built-ins but not of 33 runtime service functions underneath them
(§1.4c). Neither correction changes the conclusion.

The strategic point: **NERPs, not `Objective.cpp`, is the mission engine.** The built-in objective
types are largely non-functional (§7.2), and the 1999 campaign must have driven its objectives from
scripts. Since we own the loader, the interpreter, all 293 built-ins, and — decisively — a live
reference to the executable's own name/arity table, we can give this project a text-based mission
scripting language that needs no external toolchain and cannot drift from the binary. That is the
single biggest content lever available anywhere in the project, and it is roughly one session of
work.

### Ranked plan

**Rank 1 — In-DLL NERPs text assembler.** `mission/NERPsFile.cpp`, gated `NerpsTextScripts` in
`DeepCore.cfg`, default off. Resolve names and arity from `c_nerpsFunctions` (`NERPsFile.cpp:34`)
and operators from `c_nerpsOperators` (`NERPsFile.cpp:37`) — no hard-coded tables. Two-pass label
resolution. Allocate with `Gods98::Mem_Alloc` so `NERPsFile_Free` (`NERPsFile.cpp:297`) stays
correct. Ship the disassembler in the same change; it costs almost nothing and makes the stock
scripts readable. *Unlocks every subsequent content item.*

**Rank 2 — Fix `SetGameCompleted`** (§7.1). Three lines. Without it, no campaign can end correctly.
Do this before authoring anything.

**Rank 3 — Campaign-scoped save directory** (§4.5). Replace the three `"Saves"` literals in
`FrontEnd.cpp` (`:4721`, `:4780`, `:4842`) with a `DeepCore` setting defaulting to `"Saves"`, and
redirect the read side in `ObjectRecall_LoadRROSFile` to match. Without this, shipping a campaign
destroys the player's existing saves on first launch.

**Rank 4 — Per-level briefing titles** (§5.2). Three edits in `Objective_LoadObjectiveText`.
Highest presentation impact per line of code in the whole document.

**Rank 5 — Author the campaign.** Pure data, `-cfgadd` bundle: level blocks reusing stock maps
(§6.1), NERPs source scripts, message files, briefing text, encyclopedia entries. Constraints that
must be honoured: `DontShowObjectiveAdvisor TRUE` on every level (§2.4), no `CrystalObjective`
(§7.2), ≤ 15 `LevelLinks` per level, exactly 8 tutorials, and every chain level reachable through
links (§4.5).

**Rank 6 — New objective types** (§2.5). Three edit sites in `Objective.cpp`, DLL-side state, one
new cfg key each. Add them only where a script cannot express the goal more cheaply — after Rank 1,
most can.

**Rank 7 — `Front_LoadLevelSet` cycle guard** (§7.3). One line, turns a startup hang into a
diagnostic.

**Rank 8 — Extended NERPs function ids** (§3.7). Defer. Settle the un-hooked
`NERPsRuntime_LoadLiteral` question (`interop.cpp:3116`) first. 293 built-ins is not the binding
constraint on content; the absence of an assembler is.

**Explicitly not planned:** anything in `front/Reward.*` (§4.6), new `Text_Type` slots (§5.1), a
ninth tutorial, or new object-type ids.

---

## Sources

External claims are confined to §3.2 (bytecode/file extensions), §3.3 (the public function-id
table), §3.5 (message-file shape) and §4.3 (the exe-side level key list). Everything else is
derived from this tree.

- [NERPs — RRU Knowledge Base](https://kb.rockraidersunited.com/NERPs)
- [Writing NERP Scripts — RRU Knowledge Base](https://kb.rockraidersunited.com/Writing_NERP_Scripts)
- [Lego.cfg — RRU Knowledge Base](https://kb.rockraidersunited.com/Lego.cfg)
- [jgrip/legorr — NPL compiler/assembler/disassembler (archived, GPL-2.0)](https://github.com/jgrip/legorr)
- [Npl Scripter v2.1 — Rock Raiders United](https://rockraidersunited.com/topic/2143-npl-scripter-v21-update/)
