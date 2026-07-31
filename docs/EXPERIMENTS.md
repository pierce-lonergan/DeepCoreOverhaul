<!-- Permanent, user-facing document. This is the register of every open question in this
     project that can only be closed by somebody who has an installation of the original
     game. Nothing in this file has been run. Every procedure is derived from source that
     is cited inline; every citation is file:line and is marked OURS (implemented C++ in
     this tree) or EXE (a raw address macro -- still 1999 machine code).

     Naming rule, absolute: the "L" in "LRR" is never expanded, anywhere. -->

# EXPERIMENTS — the register of tests that need the game installed

This project has never run. Not once. `docs/HANDOFF-2026-07-30.md:3` states the ceiling
plainly: *"Everything below is compile-verified and none of it is play-tested."* Across four
handoffs and eleven research documents the phrase "the highest-value thing someone with the
install could do" has been written a dozen times, in a dozen places, and never collected.

This file is that collection. It is the single place a person with a legally-obtained copy
of the game should look, and the single place an answer should come back to.

**What an "answer" is here.** An answer is an observation plus the evidence that produced
it, committed back into the document named in the entry's *WHAT TO RECORD* row. An answer is
not "it seemed to work". §3 gives the reporting contract.

**Why the failure modes are inside the procedures.** Several of these tests produce **no
error at all** when the hypothesis fails. `Stats_Initialise` warns and `continue`s on a name
it cannot resolve (`game/object/Stats.cpp:138-141`, OURS). A stereo WAV loads as handle `0`
and is inaudible forever (`engine/audio/3DSound.cpp:1130`, OURS). A tester who does not know
that will look at a quiet screen and report a false negative. So the failure mode is written
into the procedure, at the step where it bites, not appended as a footnote.

---

## 0. Standing setup — read this before running anything

Six facts about this build. Getting any of them wrong turns a real result into a false one.

### 0.1 The log is a console window. There is no log file.

`Gods98::Error_Out` (`engine/core/Errors.cpp:166`, OURS) writes to `errorGlobs.dumpFile` if
one is open, and otherwise falls through to a plain `printf`
(`engine/core/Errors.cpp:206`, because `LOG_CONSOLEWINDOW` is defined at
`engine/core/Errors.cpp:14`).

**`Error_SetDumpFile` (`engine/core/Errors.cpp:92`) has zero callers in this tree.** Nothing
in the DLL ever opens a dump file. So every diagnostic in this document arrives on stdout,
and stdout is the console window the DLL allocates for itself:

```
src/openlrr/OpenLRR.cpp:1081    if (Gods98::mainOptions.log.value_or(true))
src/openlrr/OpenLRR.cpp:1081        openlrrGlobs.conout = MakeConsole();
src/openlrr/OpenLRR.cpp:47      FILE* MakeConsole(void)  ->  ::AllocConsole() + freopen_s("CONOUT$")
```

The console is on by default; `-nolog` suppresses it and `-log` forces it
(`engine/Main.cpp:1310-1311`). **Never pass `-nolog` while running an experiment** — it
removes the only output channel this project has.

> To keep a transcript, run the launcher from a `cmd`/PowerShell window and redirect, or
> screenshot the console before it closes. The console dies with the process.

### 0.2 Debug-level messages are OFF by default. This is the single most common way to get a false negative.

```
engine/core/Errors.cpp:26   Gods98::Error_LogLevels Gods98::errorLogLevels
                              = { false, true, true, true, true };
                              //  Debug  Trace Info  Warn  Fatal
```

`debugVisible` is **false**. And `DeepCore_LogF`, the macro behind every single
`VerboseStartup` line, is `Error_DebugF2`:

```
game/DeepCore.cpp:26   #define DeepCore_LogF(s, ...)  Error_DebugF2("DeepCore: %s\n", ...)
game/DeepCoreAudio.cpp:19   #define Audio_LogF(s, ...)  Error_DebugF2("DeepCore/Audio: %s\n", ...)
```

⚠ **Therefore `VerboseStartup TRUE` on its own prints nothing.** You must also pass
`-loglevels debug` (`engine/Main.cpp:1337-1356`). A tester who sets `VerboseStartup TRUE`,
sees an empty console and concludes "the config was not found" has reported the wrong
answer to X-00.

*Warnings are visible by default*, so `DeepCore_WarnF` and `Config_WarnItem` lines appear
without any flag. Every entry below states which level its evidence arrives at.

### 0.3 "FATAL" is a log level, not a stop

`Config_FatalItemF` → `Error_FatalF2` → `Error_Out(true, …)` → `Error_TerminateProgram`, and
the whole chain is gated on `errorLogLevels.fatalVisible` (`engine/core/Errors.h:111`,
default `true` at `Errors.cpp:26`). Turning the level off turns off the *termination*, not
just the message. There are places in the tree that log fatal and carry on with invalid
state regardless — `engine/audio/3DSound.cpp:300` and `:523` are the documented pair
(`docs/research/audio-pipeline-truth.md:892-899`). **Do not read the word "Fatal" in the
console as proof the process stopped, and do not read the absence of a crash as proof
nothing went wrong.**

### 0.4 A log line containing `%` may garble or crash the log line itself

`Error_Out` formats into `achBuffer` with `vsprintf` (`engine/core/Errors.cpp:192`) and then
passes that already-formatted buffer to `printf` as a **format string**
(`engine/core/Errors.cpp:206`). A config item name or file path containing `%` is therefore
expanded twice. **Never use `%` in a test name, a cue name, or a monster type name you add
for an experiment.** If a diagnostic line looks truncated or nonsensical, check the input
for a `%` before reporting a bug.

### 0.5 Debug keys and the debug overlay need to be switched on

`Lego_IsAllowDebugKeys()` (`game/Game.h:889`) reads `GAME2_ALLOWDEBUGKEYS`, set at
`game/GameState.cpp:408-410` when **either** `AllowDebugKeys TRUE` appears in the `Main`
block of your `Lego.cfg`, **or** `-programmer` is on the command line
(`engine/Main.cpp:1150-1155`). Without it:

- the **[F8] debug overlay** does not appear (`game/GameState.cpp:1771`), and
- `Lego_HandleWorldDebugKeys` keys such as **[E] emerge a monster at the cursor**
  (`game/Game.cpp:2192-2221`, OURS, hooked at `interop.cpp:2782`) do nothing.

Several procedures below depend on both.

### 0.6 The standard command line for every experiment in this file

```
OpenLRR.exe -programmer -loglevels debug -datafirst
```

- `-programmer` — enables debug keys and the F8 overlay (§0.5).
- `-loglevels debug` — makes `VerboseStartup` output visible (§0.2).
- `-datafirst` — loose files under `Data\` beat the WADs
  (`engine/core/Files.cpp:877-893`, and the default is `false` at `Main.cpp:800`). Needed
  only when you are *shadowing* a filename that already exists in a WAD; a brand-new
  filename needs no flag at all (`docs/research/audio-pipeline-truth.md:368-373`).

Add `-nointro -novideo` to shorten the loop. Do not add `-nowad` unless an entry says so.

### 0.7 One variable at a time, and everything starts vanilla

Every feature gate in `data/Settings/DeepCore.cfg` defaults **FALSE = vanilla**
(`data/Settings/DeepCore.cfg`, header block). Deleting the file entirely gives stock
OpenLRR. Change exactly one gate per run. If two are on and something breaks, the run is
worthless and has to be repeated.

**Back up your `Lego.cfg` and your `Saves\` directory before starting.** X-13 blanks save
files by design, and several entries edit `Lego.cfg` directly.

---

## 1. The register

Ranked by what each unlocks. **X-01 and X-02 lead by a wide margin**: X-01 gates the whole
roster question, and X-02 converts an entire inferred column of `docs/NERPS-LANGUAGE.md`
into measured fact. X-00 is not ranked because it is a precondition for everything else.

| # | Experiment | Unlocks | Fails silently? |
| --- | --- | --- | --- |
| **X-00** | Does `DeepCore.cfg` load at all? | *every other entry* | **YES** — §0.2 |
| **X-01** | Does the exe's `RockMonsterTypes` loader accept an unseen name? | the entire roster plan | **YES** |
| **X-02** | Dump `c_nerpsFunctions[294]` — names + arity | the NERPs assembler, all of §4 of the language reference | no (it prints or it does not) |
| **X-03** | Does the Debug configuration single-step after the `/Od` fix? | all future in-situ debugging | no |
| **X-04** | Do the 19 shipped audio cues resolve and play? | the threat-audio layer, the whole audio pipeline spec | **YES** |
| **X-05** | Does `RelocateWaterTables` hold on a map that broke the old caps? | 25 years of community maps that crash on load | **partly** |
| **X-06** | Does an 11th building icon fit? | +1 buildable building | **YES** |
| **X-07** | Wave-director behaviours that only a running game can settle | tuning the one shipped gameplay feature | mixed — see entry |
| **X-08** | Do `Spider`, `Snake`, `Scorpion` resolve? | 3 of the 11 species identities in the stats overhaul | **YES** |
| **X-09** | Recover the authoritative `Lego.cfg` schema | every Stage-2 claim in `OVERHAUL-PLAN.md` | n/a — tool does not exist |
| **X-10** | Is the slug `targetBlockPos` corruption reachable? | severity of a shipped fix | **YES** |
| **X-11** | What occupies sound-table index 0? | a documented "never put your cue first" rule | **YES** |
| **X-12** | Do the two objective gates really kill a mission? | the whole built-in objective system | **YES** |
| **X-13** | Does changing the mission list blank every save? | campaign work | no — it is loud and destructive |
| **X-14** | Is a wide viewport Hor+ or stretched? | widescreen support | n/a — code not written |
| **X-15** | Is the panel config block really `Panels<W>x<H>`? | the HUD layout plan | **YES** |
| **X-16** | What does raising `WaveMaxAlive` cost in frame time? | the quadratic paths in `PERFORMANCE.md` | no |
| **X-17** | Do creature variants and beam styles look like anything? | two shipped cosmetic features | **YES** |
| **X-18** | Does the `[W]` key decline cleanly under relocation? | closes the one undecompiled blocker | no |

---

### X-00 — PRECONDITION: does `Settings\DeepCore.cfg` get found and parsed?

**What it settles.** Whether anything this project has ever written is running at all.
Nothing in this fork has been observed loading, once.

**WHY IT MATTERS.** Every other entry in this register assumes the settings layer works. If
`DeepCore::Load()` returns `false` because the file was not found, *every gate is at its
default*, every feature is off, and every subsequent experiment silently measures stock
OpenLRR. Run this first. Do not skip it because a later test "seems to work" — the later
tests mostly cannot tell the difference.

**PROCEDURE.**

1. Confirm the file is beside the executable, not in `Data\`. It is loaded with
   `FILE_FLAG_EXEDIR | FILE_FLAG_NOCD` (`game/DeepCore.cpp:461-462`, OURS), the same flags
   Shortcuts uses:

   ```
   <game folder>\Settings\DeepCore.cfg
   ```

   `DEEPCORE_FILENAME` is `"Settings\\DeepCore.cfg"` (`game/DeepCore.hpp`).

2. Edit it and set exactly one key:

   ```
   VerboseStartup                  TRUE
   ```

3. Launch with **both** flags:

   ```
   OpenLRR.exe -programmer -loglevels debug
   ```

4. Read the console.

⚠ **FAILS SILENTLY, in two distinct ways, and they look identical.**

- *A missing file is not an error.* `Config_Load2` returning `nullptr` makes
  `DeepCore::Load()` return `false` with no message whatsoever
  (`game/DeepCore.cpp:466-470`, OURS) — the comment there says so explicitly: "Not an error.
  No config means behave exactly like upstream OpenLRR."
- *Forgetting `-loglevels debug` produces the same empty console*, because the echo block is
  `DeepCore_LogF` → `Error_DebugF2` and `debugVisible` is `false` by default (§0.2).

You cannot distinguish these two by looking at the screen. **Run step 3 twice — once with
`-loglevels debug` and once without — and report both.** If both are empty, the file was not
found; if the second is empty and the first is not, everything is fine.

**EXPECTED RESULT.** Nine lines on the console, exactly this shape
(`game/DeepCore.cpp:676-686`, OURS):

```
DeepCore: settings loaded from Settings\DeepCore.cfg
DeepCore:   MultiSpeciesEmerge  = false
DeepCore:   WaveDirector        = false
DeepCore:   WaveIntervalSeconds = 150.000000
DeepCore:   WaveMaxAlive        = 6
DeepCore:   CreatureVariants    = false
DeepCore:   SurviveWaterOverflow= false
DeepCore:   RelocateWaterTables = false
DeepCore:   WaterMaxPools       = 4096
DeepCore:   WaterMaxPoolBlocks  = 65536
```

**FAILURE MODE.** Empty console under `-loglevels debug` ⇒ the file is not being found.
Check the path, then check that `legoGlobs.gameName` is what the config's root block says —
keys resolve as `<gameName>::DeepCore::<name>` (`game/DeepCore.cpp:29`), so a `Lego*` root
block in `DeepCore.cfg` must match the root block in your `Lego.cfg`.

**WHAT TO RECORD.** The verbatim console block (or its absence, both runs), the exact
command line, the game folder layout, and your `Lego.cfg`'s root block name. → updates
`docs/HANDOFF-2026-07-30.md` §6 and `docs/WORKLOG.md`.

---

### X-01 — Does the exe's `RockMonsterTypes` loader accept a name it has never seen?

**What it settles.** Whether the ~4 unused RockMonster ID slots below the hard ceiling of 15
can be filled with new creature names at all — which is the single question the entire
roster plan hangs on.

**WHY IT MATTERS.** `LegoObject_ID_Count` is 15 and cannot be raised: ID 15 does not run off
the end of the `[20][15]` tables, it **aliases Building ID 0** — the Tool Store in stock data
(`docs/WORKLOG.md:97-103`; `game/GameCommon.h:1075` for `LegoObject_Building == 4`). That
door is closed and is not re-litigated here. But roughly eleven of the fifteen slots are
spoken for (`game/GameCommon.h:137-150`), which leaves about four. Whether those four are
usable depends entirely on `Lego_LoadRockMonsterTypes` — **EXE**, `0x0042d030`
(`game/Game.h:1474`) — accepting a name that was not in the 1999 data, and on
`Lego_GetObjectByName` — **EXE**, `0x0042e7e0` (`game/Game.h:1511`) — resolving it
afterwards.

Everything downstream is blocked on the answer: `EmergeSpeciesPool` and `WaveSpeciesPool`
(`data/Settings/DeepCore.cfg`), the `Variants` table, the eleven species identities in
`docs/research/stats-overhaul.md` §3, and X-08. `docs/OVERHAUL-PLAN.md:252` calls this "the
first experiment of Stage 2". It still is.

**PROCEDURE.**

1. **Back up `Lego.cfg`.** Work on a copy.

2. Find the `RockMonsterTypes` block. It is keyed from `legoGlobs.rockMonsterData`
   (`game/Game.h:533`, comment `(cfg: RockMonsterTypes)`) and its count from
   `legoGlobs.rockMonsterCount` (`game/Game.h:555`). The loader takes no config parameter,
   so **the block's exact nesting path cannot be derived from this tree** — search your own
   `Lego.cfg` for `RockMonsterTypes` and record the full path you find, because that answer
   is itself a deliverable.

3. Duplicate an existing entry under a new, previously-unused name. Use a *real, existing*
   model directory so the only new thing in the test is the name. Add exactly one entry —
   a 12th:

   ```
   RockMonsterTypes {
       ...the existing 11 entries, untouched...
       DeepCoreProbe   Creatures\RMonster        ; same path as the stock RockMonster entry
   }
   ```

   ⚠ No `%` anywhere in the name (§0.4). No spaces.

4. Give it a `Stats` entry, because that is the loudest available detector:

   ```
   Stats {
       DeepCoreProbe {
           Levels          1
           RouteSpeed      1.0:
           ...copy the stock RockMonster block verbatim...
       }
   }
   ```

5. Launch: `OpenLRR.exe -programmer -loglevels debug -datafirst`, load any mission.

6. Read the console **at level load**, not during play.

⚠ **THIS FAILS SILENTLY. THE FAILURE IS A WARNING, NOT A CRASH.**

If the exe's loader ignores the 12th entry, or loads it but `Lego_GetObjectByName` will not
resolve it, `Stats_Initialise` prints one warning and **`continue`s** — the game runs on,
the creature simply does not exist and nothing anywhere says so again:

```
game/object/Stats.cpp:138    if (!Lego_GetObjectByName(Config_GetItemName(prop), &type, &id, nullptr)) {
game/object/Stats.cpp:139        Config_WarnItem(true, prop, "Object name in Stats not found");
game/object/Stats.cpp:140        continue;
```

That line arrives at **Warn** level, which is visible without any flag, and it carries the
config file name and line number (`engine/core/Config.h:342`). So the exact string to search
the console for is:

```
Object name in Stats not found
```

**Its presence means the experiment FAILED. Its absence means the experiment PASSED.**
A tester who looks at the screen instead of the console, sees nothing unusual, and reports
"seems fine" has reported the failure case as a pass.

There is a second, different warning that means something else entirely. If the name *did*
resolve but resolved to an out-of-range index, the DeepCore guard fires instead
(`game/object/Stats.cpp:163-172`, OURS, unconditional and deliberately not gated):

```
Stats entry "DeepCoreProbe" resolved to out-of-range indices (type %i, max %i; id %i, max %i).
```

That is a *third* outcome — "loader accepted it, but handed back an index that would corrupt
the executable's data segment" — and it is worth more than either of the other two. Report it
verbatim if you see it.

7. If no warning appeared, confirm positively rather than by absence. Load a level, enable
   debug keys, hover a wall and press **[E]** (`Debug_EmergeMonster`,
   `game/Shortcuts.cpp:156`; handler `game/Game.cpp:2192-2221`, OURS). That path emerges
   `legoGlobs.currLevel->EmergeCreature`, so set that level's `EmergeCreature` to
   `DeepCoreProbe` first. Alternatively set `MultiSpeciesEmerge TRUE` and
   `EmergeSpeciesPool DeepCoreProbe` in `DeepCore.cfg` and watch for
   `DeepCore: EmergeSpeciesPool resolved 1 of 1 names` (`game/DeepCore.cpp:133-136`, OURS,
   **Debug** level — needs `-loglevels debug`).

**EXPECTED RESULT IF THE HYPOTHESIS HOLDS.** No `Object name in Stats not found` for
`DeepCoreProbe`; `EmergeSpeciesPool resolved 1 of 1 names`; and pressing [E] on a suitable
wall produces a creature. The roster question opens: about four new RockMonster identities
become authorable from data alone, with no code change, because both consumers are
count-driven (`docs/WORKLOG.md:112-117`).

**FAILURE MODE.** Two of them, and they need to be told apart:
- **Warning printed, no crash** ⇒ the loader or the name lookup rejected it. The roster is
  closed at eleven and `docs/research/stats-overhaul.md` §3's last three identities are dead.
- **The whole game fails to reach a mission.** `Lego_LoadRockMonsterTypes()` returns `bool32`
  and sits inside an `&&` chain at `game/GameState.cpp:512-514`; if it returns false, none of
  `Stats_Initialise`, `Weapon_Initialise`, `Encyclopedia_Initialise` or the camera setup run
  (`game/GameState.cpp:566-571`). That is loud, and it means the loader actively rejected the
  block rather than ignoring the extra row. **Revert `Lego.cfg` immediately from your backup
  if this happens.**

**WHAT TO RECORD.**
- The full config path where you found `RockMonsterTypes` (this is a deliverable in itself).
- The exact block you added, verbatim.
- The complete console output from launch to first mission, verbatim.
- Whether `[E]` produced a creature, and a screenshot if it did.
- `legoGlobs.rockMonsterCount` if you can read it (see X-02's method — the same debugger
  session can read `legoGlobs` at `Game.cpp`'s bound address).

→ updates `docs/HANDOFF-2026-07-30.md` §6, `docs/OVERHAUL-PLAN.md` §1 and §6,
`docs/research/type-loader-reimplementation.md` §4.5 and §7,
`docs/research/stats-overhaul.md` §3.12.

---

### X-02 — Dump `c_nerpsFunctions[294]`: names and arity, measured

**What it settles.** The name and the *authoritative arity* of all 294 slots in the exe's
NERPs built-in table, converting `docs/NERPS-LANGUAGE.md` §4's inferred-arity column — 293
rows of it — from inference into fact, and settling check **C7**
(`docs/NERPS-LANGUAGE.md:1460`), the one UNDETERMINED fact the whole assembler design rests on.

**WHY IT MATTERS.** `docs/NERPS-LANGUAGE.md` §8 decides to build an in-DLL text assembler and
ranks the table dumper first: *"~10 lines, zero risk … the cheapest possible way to falsify
this document"* (`docs/NERPS-LANGUAGE.md:1487-1491`). Everything the assembler must do — turn
a name into a slot index, know how many words to consume after a call, know whether a call is
legal in argument position — is already in memory at `0x004a6948`. Nobody has ever looked.

The table is **EXE data**, bound by us:

```
game/mission/NERPsFile.cpp:34   c_nerpsFunctions = *(NERPsFunctionSignature(*)[294])0x004a6948;
game/mission/NERPsFile.cpp:37   c_nerpsOperators = *(const char*(*)[11])        0x004a7710;
game/mission/NERPsFile.h:222    struct NERPsFunctionSignature { const char* name;  // +0
                                                                NERPsFunction function; // +4
                                                                NERPsFunctionArgs arguments; }; // +8
game/mission/NERPsFile.h:229    assert_sizeof(NERPsFunctionSignature, 0xc);
```

Arity encoding (`game/mission/NERPsFile.h:85-95`):
`0 ARGS_0`, `1 ARGS_1`, `2 ARGS_2`, `3 ARGS_0_NORETURN`, `4 ARGS_1_NORETURN`,
`5 ARGS_2_NORETURN`, `6 ARGS_3_NORETURN`, `7 END_OF_LIST`.

**PROCEDURE — method A: a debugger, no code change, no rebuild.**

This is the fastest route and it needs nothing from this repository.

1. Launch the game normally and get to the main menu (the table is exe *static data*, so it
   is valid from process start; being in-game is not required, but it guarantees the DLL has
   patched its hooks in, which is what makes the `function` column interesting).
2. Attach any debugger — Visual Studio *Debug ▸ Attach to Process*, x64dbg, WinDbg.
3. Dump `294 × 0xc = 0xdc8` bytes from `0x004a6948`. In x64dbg: `dump 0x004a6948`, then
   right-click ▸ *Copy ▸ Selection*, or `savedata "nerps.bin", 0x004a6948, 0xdc8`.
4. Decode. Each 12-byte record is three little-endian `uint32`: a `char*`, a function
   pointer, and the arity. Resolve each name pointer by reading the null-terminated string at
   that address (all of them point into the exe's read-only data).

   A worked decode, given `nerps.bin` and a full process memory image, is a ten-line script;
   in x64dbg the script console does it directly:

   ```
   // x64dbg script -- prints "index<TAB>name<TAB>arity" for all 294 slots
   mov $base, 0x004a6948
   mov $i, 0
   loop:
     mov $rec,  $base + $i * 0xc
     mov $name, [$rec]
     mov $args, [$rec + 8]
     log "{d:$i}\t{s:$name}\t{d:$args}"
     inc $i
     cmp $i, 294
     jb loop
   ```

   Save the log to a text file.

**PROCEDURE — method B: a ten-line debug keybind in the DLL.**

Preferable if you can build, because the output lands in the same console as everything else
and can be produced by anyone without a debugger. It is a **new** shortcut, so it needs three
edits; none of them touches an exe-overlaid struct, adds a hook, or grows anything with an
`assert_sizeof`.

1. `game/Shortcuts.hpp` — one enum entry before `ShortcutID::Count`
   (`game/Shortcuts.hpp:310`), e.g. `Debug_DumpNERPsTable,`.
2. `game/Shortcuts.cpp` — one registration beside the others at `:108-127`, e.g.
   `Shortcut_Register(Debug_DumpNERPsTable, "KEY_LEFTSHIFT+KEY_F7");` — pick a chord that
   does not collide; `Shortcuts.cfg` makes it rebindable for free.
3. `game/Game.cpp`, inside `Lego_HandleWorldDebugKeys` (`game/Game.cpp:2173`, OURS, hooked at
   `interop.cpp:2782`):

   ```cpp
   if (Shortcut_IsPressed(ShortcutID::Debug_DumpNERPsTable)) {
       for (uint32 i = 0; i < _countof(LegoRR::c_nerpsFunctions); i++) {
           const char* n = LegoRR::c_nerpsFunctions[i].name;
           Error_InfoF("NERPs[%3i] %-32s args=%i", (sint32)i,
                       (n != nullptr ? n : "<null>"),
                       (sint32)LegoRR::c_nerpsFunctions[i].arguments);
       }
   }
   ```

   Use `Error_InfoF` (Info level is **on** by default, `Errors.cpp:26`) rather than
   `Error_DebugF`, so the dump appears without `-loglevels debug`. **Null-check `name`** —
   `NERPs_HookFunction` does not (`game/mission/NERPsFile.cpp:83`) and would fault on a
   malformed table; this is check **C1** (`docs/NERPS-LANGUAGE.md:1454`).

4. Build both configurations. **The build contract is 0 errors and exactly 44 warnings on a
   full `-t:Rebuild`** (`docs/HANDOFF-2026-07-30.md:8-11`). Re-verify the tally; watch for
   `sint32`/`uint32` conversion warnings at `/W3` and cast explicitly
   (`docs/research/wave-director.md:1232-1237`).
5. Run with `-programmer` (§0.5), load any mission, press the key, capture the console.

**EXPECTED RESULT IF THE HYPOTHESIS HOLDS.** 294 lines. Three of them are independently
predicted from our own source and are the self-test:

| Slot | Expected | Source of the prediction |
| ---: | --- | --- |
| 0 | name `Stop`, arity `0` or `3` (`ARGS_0` / `ARGS_0_NORETURN`) | `game/mission/NERPsFile.h:47` (`NERPS_FUNCID_STOP == 0`); the arity is check **C7** and is the point of the whole test |
| 27 (`0x1b`) | name `SetR2` | the interpreter's worked example at `game/mission/NERPsFile.cpp:392` |
| 293 | name `**End Of List**`, arity `7` (`NERPS_END_OF_LIST`) | installed by name at `interop.cpp:3432`; check **C3** |

Also expected: no duplicate names (check **C6**), no arity above 7 (check **C5**), and 11
non-null operator strings at `0x004a7710` (check **C2**).

**FAILURE MODE. It does not fail silently — it prints or it does not.** The realistic bad
outcomes are:

- **A crash inside the loop.** Means a `name` pointer is null or garbage, i.e. the table is
  not what `NERPsFile.cpp:34` claims. That is a *finding*, not a mistake — record the index
  it died at. (Method B's null check prevents the common case; method A cannot crash the game
  at all, which is one reason to prefer it for the first attempt.)
- **Slot 0 is not `Stop`, or slot 293 is not the sentinel.** The table's base address or its
  stride is wrong for your build of the executable. Report the first sixteen records as raw
  hex so the layout can be re-derived.
- **Arity of slot 0 consumes arguments.** Then §2.8 and §5.7 of `docs/NERPS-LANGUAGE.md` are
  wrong and the assembler's `Stop` fall-through is unsafe. This is exactly the outcome the
  document says it cannot rule out (`docs/NERPS-LANGUAGE.md:372`, `:1460`). Report it loudly.

**WHAT TO RECORD.** The full 294-line dump as a plain text file, one `index<TAB>name<TAB>arity`
per line, plus which method produced it and the exe build it came from. Attach the file; do
not paste 294 lines into a message. → replaces the inferred-arity column throughout
`docs/NERPS-LANGUAGE.md` §4, discharges checks C1–C7 in §7.3, and unblocks items 1–3 of
§8's ranked plan (dumper → disassembler → assembler).

**Bonus, same session, ~zero extra cost.** While the debugger is attached, also dump
`c_nerpsOperators` (11 pointers at `0x004a7710`) and confirm the arithmetic that says neither
table can be extended in place:
`0x004a6948 + 294*0xc = 0x004a7710` and `0x004a7710 + 11*4 = 0x004a773c ==
&nerpsHasNextButton` (`game/mission/NERPsFile.cpp:37-40`;
`docs/NERPS-LANGUAGE.md:401-412`).

---

### X-03 — Does the Debug configuration actually single-step now?

**What it settles.** Whether `Debug|Win32` produces steppable code after the optimisation fix,
i.e. whether every experiment in this file that would benefit from a breakpoint is now
practical.

**WHY IT MATTERS.** `docs/PERFORMANCE.md:26-32` records a defect and its fix: **both**
configurations were silently inheriting MSBuild's default `Optimization=MaxSpeed`, so a
configuration named Debug was emitting inlined frames and optimised-away locals — useless to
the one audience it exists for, which is a person who can run the game. Both are now explicit
in `src/openlrr/openlrr.vcxproj`: `<Optimization>Disabled</Optimization>` at `:110` for
Debug, `<Optimization>MaxSpeed</Optimization>` at `:140` for Release. That fix has been
verified by re-reading the MSBuild task parameters. **It has never been verified by
stepping.** X-02 method B, X-10 and X-16 all get much cheaper if this holds.

**PROCEDURE.**

1. Build `Debug|Win32`, v142. Confirm 0 errors / exactly 44 warnings on `-t:Rebuild`.
2. Verify the switch really reached the compiler, not just the project file — this is the
   Tier-3 method `docs/PERFORMANCE.md:864-872` makes binding:

   ```
   msbuild src/openlrr/openlrr.vcxproj -t:ClCompile -p:Configuration=Debug -p:Platform=x86 -v:diagnostic
   ```

   Search the output for `Task Parameter:Optimization`. It must read `Disabled`.
3. Attach Visual Studio to the running game (*Debug ▸ Attach to Process*), or launch under
   the debugger.
4. Set a breakpoint on a small function with locals that a real optimiser would fold away.
   `DeepCore::Logic::WaveInterval` (called from `game/WaveDirector.cpp:404`) or
   `IsFairSpawnBlock` (`game/WaveDirector.cpp:135`) are good targets: both have named locals,
   both are hit on a predictable schedule with `WaveDirector TRUE`.
5. Step through with F10. Inspect `bx`, `by`, `f1`, `f2`, `adjacentWall`.

**EXPECTED RESULT.** The breakpoint binds; stepping advances one source line at a time in
source order; every named local shows a plausible value rather than
`<optimized away>`/`<undefined>`; the call stack shows `IsFairSpawnBlock` as its own frame
rather than inlined into `GatherCandidates`.

**FAILURE MODE. Does not fail silently — but it fails *confusingly*.** If stepping still
jumps around, or locals read as optimised away, the likely causes in order are: the PDB does
not match the loaded DLL; you attached to the launcher rather than the process hosting the
DLL; or the `-p:Configuration` you built is not the one deployed. Check the modules window
for the DLL's path and PDB status before concluding the fix did not land.

⚠ **One thing this experiment does NOT establish, and must not be reported as establishing.**
The Debug configuration is **not a checked build**. `UseDebugLibraries` is `false` in both
configurations (`openlrr.vcxproj:31`), so `_DEBUG` is undefined, `_ITERATOR_DEBUG_LEVEL` is 0,
and there are **no checked iterators and no `std::vector` bounds assertions anywhere**;
`BasicRuntimeChecks` is `Default` (`:111`), so there is no `/RTC` either
(`docs/PERFORMANCE.md:510-539`). That is deliberate — a second CRT in a 1999 process means
two heaps — but it means "it ran clean in Debug" proves nothing about bounds.

**WHAT TO RECORD.** The `-v:diagnostic` line showing `Optimization=Disabled`, a screenshot of
the locals window at a breakpoint, and whether the call stack showed inlining.
→ updates `docs/PERFORMANCE.md` §7 and `docs/WORKLOG.md`.

---

### X-04 — Do the 19 shipped audio cues resolve, and do they play?

**What it settles.** Whether `docs/research/audio-pipeline-truth.md`'s spec is right end to
end: whether the `Samples` block lives where the document infers, whether a brand-new cue name
can be created with zero C++, and whether the 19 generated WAVs in `assets/audio/` are
actually audible.

**WHY IT MATTERS.** `ThreatAudio` is one of the shipped features and it is the only thing in
this project that turns wave-director state into something a player perceives. It is also the
best available test of the whole audio pipeline: the loader is 100% OURS
(`docs/research/audio-pipeline-truth.md:6-11`), so a failure isolates cleanly to either the
config path (EXE, `Lego_LoadSamples` at `game/Game.h:1914`) or the file format. And
`docs/research/audio-pipeline-truth.md:991` lists resolving the `Samples` block path as the
**one blocking item** between that document and a fully specified audio generator.

**The format is already correct and does not need testing.** All 19 shipped files were
verified as 22050 Hz / 16-bit / **mono** — which is mandatory, because `DSBCAPS_CTRL3D` is set
unconditionally on every secondary buffer (`engine/audio/3DSound.cpp:1123`) and
DirectSound3D buffers are mono-only. This experiment tests the *plumbing*, not the assets.

**PROCEDURE.**

1. Copy the shipped WAVs into your data directory. The path in the config is relative to
   `<game folder>\Data\` and the `.wav` extension is appended by the loader
   (`engine/audio/3DSound.cpp:280`, OURS):

   ```
   assets\audio\threat\*.wav   ->   <game folder>\Data\Sounds\DeepCore\*.wav
   ```

2. Find the `Samples` block in your `Lego.cfg` and add five lines. ⚠ **The block path is
   UNDETERMINED** — `docs/research/audio-pipeline-truth.md:518-524` infers
   `Lego*::Main::Samples` from three converging pieces of indirect evidence and says so.
   **Search your own `Lego.cfg` for `Samples` and record the real path; that answer is the
   deliverable.** Then append:

   ```
   Samples {
       ...existing entries, untouched...
       dc_threat_telegraph         Sounds\DeepCore\dc_threat_telegraph
       dc_threat_telegraph_heavy   Sounds\DeepCore\dc_threat_telegraph_heavy
       dc_sting_arrival            Sounds\DeepCore\dc_sting_arrival
       dc_threat_escalate          Sounds\DeepCore\dc_threat_escalate
       dc_threat_cleared           Sounds\DeepCore\dc_threat_cleared
   }
   ```

   ⚠ **Never put a space after a comma and never make your cue the first line of the block.**
   `Util_Tokenise` does not trim (`engine/core/Utils.cpp:41`), so `a, b` yields a filename
   `" b.wav"` that fails. And sample-table index `0` is unreachable through every `SFX_*`
   play path, all of which guard `if (handle > 0)` (`game/audio/SFX.cpp:350`, `:390`, `:401`,
   `:431`, `:476`) — the first WAV loaded in the whole session is silent through the game
   layer. See X-11.

3. In `Settings\DeepCore.cfg` set exactly these, and nothing else:

   ```
   VerboseStartup                  TRUE
   WaveDirector                    TRUE
   ThreatAudio                     TRUE
   WaveIntervalSeconds             30.0
   WaveTelegraphSeconds            6.0
   ```

   (The short interval is only to shorten the test loop.) The five `Cue*` keys already
   default to exactly these names (`game/DeepCore.hpp:145-149`).

4. Launch `OpenLRR.exe -programmer -loglevels debug -datafirst`, start a mission with at
   least one Rock Raider alive, and wait ~30 seconds of mission time.

⚠ **THREE INDEPENDENT SILENT FAILURES ON THIS PATH. Know all three before you judge.**

- **(a) The cue name does not resolve.** Reported once per cue per level, at **Warn** level
  (visible without flags) — `game/DeepCoreAudio.cpp:84-88`, OURS:

  ```
  DeepCore/Audio: cue "dc_threat_telegraph" is not declared in this installation's config,
  so it will stay silent. Add it to the game's Samples block and drop the matching WAV
  alongside it. This is reported once per cue per level.
  ```

  **It is reported when the cue is first *played*, not at startup.** If the wave director
  never fires a wave, you will see nothing at all and learn nothing. Confirm a wave actually
  telegraphed first (step 5).

- **(b) The name resolved but the WAV never loaded.** Then the cue is permanently silent with
  no further complaint: `SFX_LoadSampleProperty` sets `success = false` and deliberately
  does **not** break (`game/audio/SFX.cpp:192-195`), and the cue keeps the zeroed
  `sound3DHandle` of `0`, which every play path rejects. The only evidence is a *load-time*
  line, which you must have already captured:

  ```
  Cannot load sound "..."          engine/audio/3DSound.cpp:344
  Cannot create sound buffer.      engine/audio/3DSound.cpp:1130
  Error loading sample.            engine/audio/3DSound.cpp:1100
  ```

  Search the console for these **before** blaming the cue layer. `Cannot create sound buffer.`
  with a mono 22050/16 file means something else is wrong; with a stereo file it means
  exactly the `DSBCAPS_CTRL3D` constraint above and the file is at fault.

- **(c) A WAD copy shadowed your loose file.** By default the WAD wins
  (`engine/core/Files.cpp:877-893`). It cannot happen for these filenames, which are new —
  but it will happen the moment you reuse a stock name. `-datafirst` is in the standard
  command line for this reason.

5. Confirm the director actually ran, using the **Debug**-level wave log
   (`game/WaveDirector.cpp:428-431`, OURS — needs `-loglevels debug`):

   ```
   DeepCore/Waves: wave 1 telegraphed: 1 creature(s), N candidate block(s), 6.0s warning
   DeepCore/Waves: wave 1 landed: 1 creature(s), M alive, 1 spawned this level
   ```

   No `telegraphed` line at all ⇒ this is an X-07 problem, not an audio problem. Stop and go
   there.

6. On a successful resolution you also get, at Debug level
   (`game/DeepCoreAudio.cpp:79-81`):

   ```
   DeepCore/Audio: cue "dc_threat_telegraph" resolved to SFX id NNN
   ```

**EXPECTED RESULT IF THE HYPOTHESIS HOLDS.** `resolved to SFX id NNN` for each of the five
cues; a warning sound six seconds before each wave; a heavier warning when the wave is ≥ 3
creatures (`ThreatHeavyWaveSize`, `game/DeepCore.hpp:139`); an arrival sting when creatures
land; an all-clear when the last one dies. And, most importantly for the documentation: the
`Samples` block path confirmed, the "zero C++ for a new cue" claim confirmed
(`docs/research/audio-pipeline-truth.md:494-524`), and item 8 of that document's ranked plan
(`:991`) closed.

**FAILURE MODE.** Distinguish carefully — the whole point of the three cases above is that
"I heard nothing" is not a result. Report which of (a), (b), (c) the console showed. If none
of them appeared *and* nothing was audible, check the master gate: `SFX_IsSoundOn`
(`game/audio/SFX.cpp:596`) and the `-nosound` flag.

**WHAT TO RECORD.** The `Samples` block's true config path; the complete console from launch
through two waves; which of the five cues resolved; whether each was audible; and — because
`Sound3DPlay::Normal` applies a fixed −800 mB duck (`engine/audio/3DSound.cpp:103-121`) —
whether the level was appropriate. → updates `docs/research/audio-pipeline-truth.md` §4.4,
§8.2 and §8.4, and `docs/research/audio-and-juice.md` §5.7.

---

### X-05 — Does `RelocateWaterTables` hold on a map that broke the old caps?

**What it settles.** Whether the water-table relocation — moving pool storage out of the
exe's data segment and into the DLL — actually loads and simulates a map that the original
engine kills on sight.

**WHY IT MATTERS.** The vanilla engine calls `Error_Fatal` — process termination — above 10
pools, 100 blocks in one pool, or 10 drains in one pool (`game/world/Water.h:26-28`).
Community maps have been dying on this for twenty-five years
(`docs/OVERHAUL-PLAN.md:70`). Two independent mitigations shipped:
`SurviveWaterOverflow` (warn and skip) and `RelocateWaterTables` (the real fix — no cap at
all, configurable sanity ceilings of 4096 pools / 65536 blocks). The relocation is a subsystem
rewrite that has **never been run**, and `docs/research/water-relocation.md:669-671` says so:
*"Everything runtime. We cannot run the game."*

**PROCEDURE.**

1. Obtain a map that is known to crash on load with a water error, or build one: a level whose
   surface map contains more than 10 disjoint bodies of water, or one lake larger than 100
   tiles.

2. **Run A — establish the baseline.** All DeepCore water gates FALSE:

   ```
   SurviveWaterOverflow            FALSE
   RelocateWaterTables             FALSE
   ```

   Load the map. Expect termination with one of these
   (`game/world/Water.cpp:412`, `:551`, `:620`, `:1062`):

   ```
   Ran out of water pools
   Ran out of water pool drains for pool at index %i
   Ran out of water merge pairs
   ```

   **If the map loads fine here, it is not an over-cap map and the experiment is void.**
   Find a different map. This baseline run is not optional — without it a "pass" in run C
   proves nothing.

3. **Run B — the stopgap.** `SurviveWaterOverflow TRUE`, `RelocateWaterTables FALSE`.
   Expect the level to load, with one Warn line per distinct overflow kind
   (`game/DeepCore.cpp:375-378`, OURS):

   ```
   DeepCore: Warning: map exceeds the engine's fixed limit for water pools. The excess is
   being skipped so the level can still load; some water will not be simulated. ...
   ```

   ⚠ **This is where the silence is.** The message is emitted **once per kind per run**
   (`_waterOverflowWarned`, `game/DeepCore.cpp:373-374`). After that first line, every further
   skipped pool is invisible. So *some water is missing and the log will not tell you how
   much.* Look at the map, not the console: the pools that were dropped simply are not there.

4. **Run C — the real fix.** `SurviveWaterOverflow FALSE`, `RelocateWaterTables TRUE`,
   `VerboseStartup TRUE`. Load the same map.

5. Verify with the **[F8] water overlay** (`game/GameState.cpp:1818-1860`, OURS). Press F8
   three times to cycle ListSets → NERPs → Water. It reads through
   `Water_GetPoolView` / `Water_GetPoolDrain` / `Water_GetPoolBlock`, which are the
   relocation-aware accessors, so it works with the gate on:

   ```
   [Water]
   Water Level 0.0/4.0
   Drain (12,7) Level 1.0  [0.0, 0.0]  'R' (Active)
   ```

   `No pools of water in this level` while standing next to visible water is the **failure**
   signature for the relocated path.

6. Watch water rise and fall for at least a minute. The overlay only shows pool 0; the visual
   check covers the rest.

⚠ **PARTIAL SILENT FAILURE.** With relocation on, `waterGlobs` — the exe-overlaid struct at
`0x0054a520` — is deliberately left **zeroed** (`docs/research/water-relocation.md:660-662`).
That is the design's safety net: any undecompiled exe code that still reads it sees
`poolCount == 0` and does nothing. **The failure mode of the unknown is "water inert", not
"corruption".** So if some water-related thing quietly stops happening, that is the expected
shape of a partial failure, and it will not announce itself. Look for water that renders but
never moves, drains that never activate, and levels whose water objective never completes.

**EXPECTED RESULT.** Run A terminates; run B loads with visibly missing water and exactly one
warning per kind; run C loads with **all** the water present, simulating, rising and draining,
and the F8 overlay showing a live pool.

**FAILURE MODE.**
- **Crash under run C** — capture the console tail and, if possible, a debugger call stack.
  The known hazards are enumerated at `docs/research/water-relocation.md` §6.1-6.7 (pointer
  arithmetic, `memset` over a pool, `qsort` over the pool array, pointers escaping the module,
  lifetime versus level teardown). Naming which one you hit is worth as much as the crash.
- **Water present but frozen** — the inert-unknown case above.
- **The `[W]` debug key does nothing** — that is *correct and intended*. See X-18.

**WHAT TO RECORD.** The map (or its surface-map dimensions and water layout), all three runs'
console output, an F8 water-overlay screenshot from run C, and a before/after screenshot of
the water itself. → updates `docs/research/water-relocation.md` §10 and `docs/WORKLOG.md`.

---

### X-06 — Does an 11th building icon fit?

**What it settles.** Whether the build menu can display an eleventh entry, and therefore
whether the "+1 buildable building" in `docs/OVERHAUL-PLAN.md:37` is real.

**WHY IT MATTERS.** Ten building IDs are used of fifteen (`game/GameCommon.h:185-194`), but
all buildings share one menu (`Interface_Menu_BuildBuilding == 19`,
`game/GameCommon.h:647`) whose icon-layout tables are sized `INTERFACE_MENU_MAXICONS == 11`
(`game/interface/Interface.h:30`; the arrays at `Interface.h:88-94` —
`iconPanelImages[11]`, `iconPanelIconOffsets[11]`, `iconPanelNoBackImages[11]`,
`iconPanelNoBackIconOffsets[11]`, `iconPanelBackButtonOffsets[11]`). Ten buildings plus a
Back button (`game/GameCommon.h:660`) is exactly 11. So the eleventh buildable building has
nowhere to sit — *unless* the Back button is not counted, or the menu has a no-Back variant
that frees a slot, which is what the parallel `iconPanelNoBack*` arrays hint at.

`docs/OVERHAUL-PLAN.md:107` marks this "unprovable from source" and notes community-shipped
precedent for an eleventh-slot building. `Interface.h` is 3 implemented against 96 live exe
macros (`docs/DIRECTORY.md:495`), so nothing on our side can answer it.

**PROCEDURE.**

1. Back up `Lego.cfg`.
2. Add an 11th buildable entry to the `BuildingTypes` block, duplicating an existing one under
   a new name against the same model directory (same shape as X-01 step 3). Give it an icon
   triple — normal, unavailable, pressed (`game/interface/Interface.h:106-111`).
3. Ensure it is genuinely *buildable* — the dependency block must permit it, or it will show
   greyed or not at all for reasons unrelated to the icon count.
4. Launch `-programmer -loglevels debug -datafirst`, start a level with enough resources, open
   the build-building menu.
5. Count the icons and the Back button. Then click each one, including the last.

⚠ **FAILS SILENTLY, and in more than one way.**

- The eleventh icon may simply **not be drawn**, with no message anywhere. The menu is exe
  code; there is no bounds warning to look for.
- Worse, the eleventh icon may be drawn **at a garbage offset** — the layout arrays are
  `Point2F[11]` inside `interfaceGlobs`, an exe-overlaid struct at `0x004ddd58`
  (`game/interface/Interface.cpp:45`). Reading offset `[11]` reads whatever is next in the
  struct. It will look like a position bug, not an overflow.
- Worst, the **hit test and the drawing may disagree**: an icon you can see but not click, or
  a click that activates the wrong building. **Click every icon and confirm each builds the
  thing it depicts.** Counting icons is not sufficient evidence.

**EXPECTED RESULT.** Eleven building icons plus a working Back button, all eleven clickable,
each building the correct structure. That would establish `+1 buildable building` and would
mean the free building IDs above ten can only ever be map/NERPs/teleport-spawned — which is
what `docs/OVERHAUL-PLAN.md:37` already says.

**FAILURE MODE.** Missing icon, misplaced icon, or a click/draw mismatch — all three are
"no". Also record whether removing the Back button (a menu that uses the `NoBack` layout)
changes the answer, because that distinguishes "11 slots total" from "10 slots plus Back".

**WHAT TO RECORD.** A screenshot of the open menu with the icon count visible; a note on
whether each icon was clickable and correct; and the config you added.
→ updates `docs/OVERHAUL-PLAN.md` §1 (the headroom table), §3 Stage 2, and §6.

---

### X-07 — Wave-director behaviours that only a running game can settle

**What it settles.** Six open questions about the one substantial gameplay feature this
project ships. `docs/research/wave-director.md` §9 lists them and closes none.

**WHY IT MATTERS.** The director is the reason this project exists in gameplay terms — it
adds pressure the map does not know about. It ships behind `WaveDirector FALSE` and has never
produced a single monster. Until these are answered, nobody can tune it, and
`docs/PERFORMANCE.md:211-214` warns that raising `WaveMaxAlive` pushes directly on a
quadratic path (see X-16).

**PROCEDURE — common setup.** In `Settings\DeepCore.cfg`:

```
VerboseStartup                  TRUE
WaveDirector                    TRUE
WaveIntervalSeconds             30.0
WaveMinIntervalSeconds          20.0
WaveTelegraphSeconds            6.0
WaveSize                        1
WaveMaxAlive                    4
WaveMinDistanceFromBase         8
WaveShakeIntensity              0.35
```

Launch `-programmer -loglevels debug`. Play a large, open level with a built-out base.

**X-07a — Does a wave spawn at all, and where?**
Watch for the Debug-level pair (`game/WaveDirector.cpp:428`, `:369`):

```
DeepCore/Waves: wave 1 telegraphed: 1 creature(s), N candidate block(s), 6.0s warning
DeepCore/Waves: wave 1 landed: 1 creature(s), M alive, 1 spawned this level
```

⚠ **`N candidate block(s)` is the number that matters and its zero case is silent by
design.** If `GatherCandidates` finds nothing, `Waves::Update` returns with **no message at
all** (`game/WaveDirector.cpp:412-416`, OURS — the comment says so: *"Not an error: a sealed
or fully-built-out map legitimately has nowhere fair"*). So **no `telegraphed` line does not
mean the director is broken.** It may mean the map has no block that satisfies
`IsFairSpawnBlock` (`game/WaveDirector.cpp:135-176`): uncovered floor, not hidden, not path,
not foundation, not building-solid, not busy, not inside the Tool Store's block, at least
`WaveMinDistanceFromBase` from every building, **and adjacent to a visible wall**. On a
small or heavily-built map that set is easily empty. Lower `WaveMinDistanceFromBase` to 3 and
retry before reporting a bug.

**X-07b — Does the creature look like it emerged from a wall?**
The director spawns on a *floor* block adjacent to a visible wall
(`game/WaveDirector.cpp:163-175`), because whether vanilla emerge points sit on wall blocks
or floor blocks is **UNDETERMINED** — the exe's `LegoObject_TryGenerateRMonster`
(`0x0043b1f0`, `game/object/Object.h:974`) is undecompiled
(`docs/research/wave-director.md:1264-1266`). Compare side by side with a map-triggered
emerge (drill the wall that a level's own emerge trigger watches) and say whether they read
the same. **This is a judgement, and it is the right kind of judgement to ask a player for.**

**X-07c — Is the telegraph legible?**
`Telegraph` (`game/WaveDirector.cpp:264-282`, OURS) does two things: an `Info_Send` panel
message carrying the block position, and a camera shake.

⚠ **`Info_Send` is EXE** — `InfoMessages` is 0 implemented of 38
(`docs/research/audio-and-juice.md:90-110`) — and it needs `Info_RockMonster` to be configured
in *your* `Lego.cfg`. If it is not, **there is no message and no diagnostic**
(`docs/research/wave-director.md:1279-1281`). So: did a panel message appear? Could you click
it to jump to the location? Did the camera shake? Report each separately — a missing message
with a working shake is a config answer, not a code bug.

**X-07d — Is 6 seconds of warning enough?**
Pure judgement, and it is the fairness dial (`data/Settings/DeepCore.cfg`, the
`WaveTelegraphSeconds` comment). Try 3, 6 and 10. Say which one makes the wave feel like
difficulty rather than a coin flip.

**X-07e — The `12.75f` magic number.**
`LegoObject_TryGenerateSlugAtBlock` offsets a spawn by `sin/cos(heading) * 12.75f`
(`game/object/Object.cpp:1625-1626`). It is not derivable from any constant in this tree;
`Map3D_BlockSize` is an exe macro (`game/world/Map3D.h:345`) and `BlockSize` is per-level
(`game/Game.h:398`). If it is really `BlockSize * 0.31875`, the director should compute it
(`docs/research/wave-director.md:1260-1263`). **Measurable:** spawn on a known block and
observe whether the creature appears centred or offset, and by roughly what fraction of a
tile. Report the fraction, not the pixels.

**X-07f — Does `WaveMaxAlive` really count map-spawned monsters too?**
It is supposed to: `CountLiveMonsters` walks the whole object list, not just director spawns
(`game/WaveDirector.cpp:92`). Drill a map emerge trigger to get 4 monsters out with
`WaveMaxAlive 4`, then confirm the director sends no wave until one dies. Silent when
correct (the budget check returns with no message, `game/WaveDirector.cpp:407-409`), so the
evidence is the *absence* of a `telegraphed` line while monsters are alive, followed by its
*appearance* after one dies. **Watch for both halves.**

**EXPECTED RESULT.** Waves telegraph on schedule, creatures arrive at the announced place
six seconds later, the interval tightens as the mission runs on, and the alive budget holds
against map-spawned monsters as well as director ones.

**FAILURE MODE.** The dangerous one is a wave that arrives *without* a telegraph — a message
that did not appear, or a shake set to 0. That converts the feature from difficulty into
noise and is the one outcome the design explicitly refuses. Report it first.

**WHAT TO RECORD.** Console transcript across at least four waves; the map used and its
size; screenshots of a telegraph and the matching arrival; and your subjective answers to
X-07b, X-07c and X-07d in plain words. → updates `docs/research/wave-director.md` §9 and
`data/Settings/DeepCore.cfg`'s documented defaults.

---

### X-08 — Do `Spider`, `Snake` and `Scorpion` resolve?

**What it settles.** Whether three of the eleven enemy identities designed in
`docs/research/stats-overhaul.md` §3.9-3.11 exist in a real installation at all.

**WHY IT MATTERS.** Those three are names in `game/GameCommon.h:148-150`. They are **not**
guaranteed entries in a real `Lego.cfg RockMonsterTypes` block, and the base game is
described as not using them (`data/Settings/DeepCore.cfg`, the `EmergeSpeciesPool` comment).
If they do not resolve, three of the eleven designed identities — "the ambusher", "the
vandal", "the one your enemies fear" — are dead, and the stats overhaul's roster table
(`docs/research/stats-overhaul.md:585-597`) shrinks to eight.

This is a strictly cheaper subset of X-01 and should be run in the same session.

**PROCEDURE.**

1. In `Settings\DeepCore.cfg`:

   ```
   VerboseStartup                  TRUE
   MultiSpeciesEmerge              TRUE
   EmergeSpeciesPool               RockMonster IceMonster LavaMonster TinyRM TinyIM Bat Slug SmallSpider Spider Snake Scorpion
   ```

2. Launch `-programmer -loglevels debug`, load any mission.

3. Read the console.

⚠ **The miss is a Warn line, one per unresolved name, and then it never mentions it again** —
`_ResolveSpeciesPool` caches failures deliberately (`game/DeepCore.cpp:73-76`, OURS):

```
DeepCore: Warning: EmergeSpeciesPool: no object named "Scorpion"; skipping it.
DeepCore: Warning: EmergeSpeciesPool: "X" is not a RockMonster type; skipping it.
DeepCore: Warning: EmergeSpeciesPool: "X" resolved to out-of-range ID N (loaded L, ceiling 15); skipping it.
```

The three messages mean three different things and the third is the interesting one — it
means the exe's loader handed back an index past the loaded table or past the hard ceiling.

The summary line is at **Debug** level (`game/DeepCore.cpp:133-136`), so without
`-loglevels debug` you get the warnings but not the count:

```
DeepCore: EmergeSpeciesPool resolved 8 of 11 names
```

**EXPECTED RESULT.** `resolved 11 of 11 names` would be a surprise and would mean all eleven
species are authorable today. `resolved 8 of 11` with three `no object named` warnings is the
predicted outcome and confirms `docs/research/stats-overhaul.md:602-607`.

**FAILURE MODE.** Not really a failure — every outcome is informative. The one thing to avoid
is running it without `-loglevels debug` and reporting only "some warnings appeared": the
count line is what makes it a measurement.

**WHAT TO RECORD.** The exact list of which names resolved and which did not, verbatim
console, and the `RockMonsterTypes` block from your `Lego.cfg` (names only — no need to paste
paths). → updates `docs/research/stats-overhaul.md` §3.12 and `data/Settings/DeepCore.cfg`'s
species comment.

---

### X-09 — Recover the authoritative `Lego.cfg` schema

**What it settles.** Every Stage-2 claim in `docs/OVERHAUL-PLAN.md`, all of which are
currently inferred from parsers rather than read from data.

**WHY IT MATTERS.** `docs/OVERHAUL-PLAN.md:216` is blunt: *"Every Stage-2 claim in this plan
is currently inferred from parsers, not read from data."* At least four separate documents
carry an UNDETERMINED that a single config dump would close: the `Samples` block path
(`docs/research/audio-pipeline-truth.md:518`), the `Panels<W>x<H>` block name
(`docs/research/widescreen-plan.md:1152`), whether any shipping config uses short-form
`PriorityImages`, one-value `CollBox` or more than 15 `LevelLinks`
(`docs/research/silent-failures-fixplan.md:2275-2278`), and the `RockMonsterTypes` path in
X-01.

**⚠ STATUS: BLOCKED. The tool does not exist.** `-cfgdump` is workstream 1.10 of
`docs/OVERHAUL-PLAN.md` (`:211-216`) and has not been written. It is listed here so the
register is complete and so nobody is asked to run something that is not there.

**PROCEDURE (interim, no code).** Until `-cfgdump` exists, the honest substitute is manual:
copy your `Lego.cfg` and any `-cfgadd` files verbatim and attach them, with any personal
paths redacted. That single attachment closes more UNDETERMINEDs than any other artefact in
this register. **Do not paste it into a message body — attach it as a file.**

**PROCEDURE (once the tool exists).** Add `-cfgdump <file>` beside `-cfgadd` in
`engine/Main.cpp:~1371`, walk the loaded config tree after `Config_AppendConfig`
(`game/GameState.cpp:189-196`) and emit `path : value` with `Config_GetLineNumber` and
`Config_GetFileNameOf` provenance (`engine/core/Config.h:209`), marking every definition
shadowed by a later wildcard match. Then:

```
OpenLRR.exe -cfgdump schema.txt -nointro
```

**FAILS SILENTLY?** N/A today. Once built, the risk is that a dump taken *before*
`Config_AppendConfig` misses the `-cfgadd` layer entirely and looks complete — dump after,
and mark the provenance, or the output is worse than nothing.

**WHAT TO RECORD.** The `Lego.cfg` itself (attached), the list of WADs present, and the
game's language/localisation. → updates `docs/OVERHAUL-PLAN.md` §3 Stage 2,
`docs/research/audio-pipeline-truth.md` §8.4, `docs/research/widescreen-plan.md` §6.

---

### X-10 — Is the slug `targetBlockPos` corruption actually reachable?

**What it settles.** The severity of a fix that already shipped: whether the negative-index
write it prevents was ever happening.

**WHY IT MATTERS.** New objects are created with `targetBlockPos = (-1,-1)`
(`game/object/Object.cpp:1163-1164`). The slug spawner set `LIVEOBJ1_EXPANDING` without ever
replacing it, and the completion branch for that flag casts the value to a `Point2I` and hands
it to `Level_Block_SetBusy` (`game/object/Object.cpp:3214-3227`). At `(-1,-1)` that is index
`-(width + 1)` into an array of `0x48`-byte structs — a negative-index write. The fix landed
(`game/object/Object.cpp:1655-1656`, OURS, unconditional). But whether the branch is
*reachable* depends on `LegoObject_IsRockMonsterCanGather`, which is an **EXE** macro at
`0x004439b0` (`game/object/Object.h:1299`), so the fix is "inert if not, correct if so"
(`docs/HANDOFF-2026-07-30.md:137-139`; `docs/research/wave-director.md:1267-1270`).

**PROCEDURE.** Requires a debugger and a build (X-03 first).

1. Build `Debug|Win32`, attach.
2. Breakpoint on the `LIVEOBJ1_EXPANDING` completion branch at
   `game/object/Object.cpp:3214`.
3. In-game, find a slug hole and press **[E]** over it (`Debug_EmergeMonster`,
   `game/Game.cpp:2192-2221`) — that path calls `LegoObject_TryGenerateSlugAtBlock` when the
   level defines a `Slug` (`game/Game.cpp:2199-2210`).
4. When the breakpoint hits, inspect `liveObj->targetBlockPos`.

⚠ **FAILS SILENTLY IN BOTH DIRECTIONS.**
- If the branch never runs, you will sit at an unhit breakpoint and learn *nothing* about
  whether it is unreachable in general or merely unreachable on this map with this slug. Try
  at least three levels before concluding "unreachable".
- If it does run and `targetBlockPos` is now `(bx, by)` rather than `(-1,-1)`, that proves the
  fix is *live* — but the pre-fix behaviour is not observable from a fixed build. To measure
  the original severity you would have to temporarily revert `Object.cpp:1655-1656`, which
  reintroduces a negative-index write. **Do not do that on a machine you care about, and if
  you do it, do it in a throwaway install.**

**EXPECTED RESULT.** Breakpoint hits with `targetBlockPos == (bx, by)` ⇒ the branch is
reachable and the shipped fix was preventing a real corruption. Breakpoint never hits across
several levels ⇒ probably inert, and the fix stays as cheap insurance.

**FAILURE MODE.** Inconclusive is the most likely outcome and is an acceptable answer. Say so
rather than guessing.

**WHAT TO RECORD.** Which levels you tried, whether the breakpoint hit, and the observed
`targetBlockPos` and `blockPos` values. → updates
`docs/HANDOFF-2026-07-30.md` §4 and `docs/research/wave-director.md` §9 item 3.

---

### X-11 — What occupies sound-table index 0?

**What it settles.** Whether stock data already parks a throwaway sample at sample-table index
0, and therefore whether "never make your new cue the first line of the `Samples` block" is a
real rule or a theoretical one.

**WHY IT MATTERS.** Every `SFX_*` play path guards `if (handle > 0)`
(`game/audio/SFX.cpp:350`, `:390`, `:401`, `:431`, `:476`), where `handle` is a
*sample-table index* returned by `Sound3D_Load` (`engine/audio/3DSound.cpp:338`). `0` is a
perfectly valid index — the one handed to the **first** WAV loaded in the whole session — so
that sample is permanently silent through the game layer. The guard cannot simply be removed,
because `0` is also the zeroed "never set" value from `SFX_Initialise`'s `memset`
(`game/audio/SFX.cpp:35`). Whether stock data already wastes index 0 on something harmless is
**UNDETERMINED** (`docs/research/audio-pipeline-truth.md:888-891`).

**PROCEDURE.** Runs for free inside X-09 or X-04.

1. From your `Lego.cfg`, read the **first** line of the `Samples` block. Record its cue name
   and WAV path.
2. In game, trigger that cue by whatever means it normally fires.
3. Listen.

⚠ **FAILS SILENTLY BY DEFINITION.** The symptom of "index 0 is wasted" is *a sound that never
plays*, with no message anywhere. And a cue can be inaudible for four unrelated reasons
(missing file, stereo file, master mute, and this). Rule the other three out first — check the
console for `Cannot load sound` and `Cannot create sound buffer.` per X-04 — before
attributing silence to index 0.

**EXPECTED RESULT.** If the first `Samples` line is something inconsequential, the rule stands
as documented and costs nothing. If the first line is a cue you can hear, then index 0 is
*not* what the analysis assumed, and `docs/research/audio-pipeline-truth.md` §7.4 item 2 needs
correcting.

**WHAT TO RECORD.** The first three lines of your `Samples` block verbatim, and whether the
first cue is audible in game. → updates `docs/research/audio-pipeline-truth.md` §7.4 and §8.4.

---

### X-12 — Do the two objective gates really make a mission unwinnable?

**What it settles.** Whether the built-in objective system is as vestigial as
`docs/research/campaign-and-story.md` §2.4 concludes.

**WHY IT MATTERS.** Two findings, both from OURS code, both marked UNDETERMINED against the
original binary (`docs/research/campaign-and-story.md:249-253`):

- **Gate 1.** `Objective_CheckCompleted` returns `false` outright when
  `OBJECTIVE_GLOB_FLAG_SHOWBRIEFINGADVISOR` is set (`mission/Objective.cpp:1086-1088`), and
  that flag is set unless `DontShowObjectiveAdvisor` is true (`:388-390`) **and** is
  force-set as the fallback when nothing else was assigned (`:576-577`). ⇒ any level showing
  a briefing advisor can never complete through the built-in evaluator.
- **Gate 2.** `Objective_SetStatus` returns early when `OBJECTIVE_GLOB_FLAG_CRYSTAL` is set
  (`mission/Objective.cpp:649-651`), and `SetStatus` is the *only* entry point to the briefing
  and both end screens. ⇒ a level with `CrystalObjective` can never show a briefing or an end
  screen.

If both hold, the entire 1999 campaign must have been driven by NERPs, which is exactly where
the content lever is (X-02). If either does *not* hold, three documents need rewriting.

**PROCEDURE.**

1. Pick a stock level that has a `CrystalObjective` and confirm it shows a briefing. If it
   does, **Gate 2 is refuted immediately** and you can stop.
2. Author a minimal test level (or copy one) with:
   ```
   DontShowObjectiveAdvisor        FALSE
   TimerObjective                  60
   ```
   Play past 60 seconds. Does it complete?
3. Change only `DontShowObjectiveAdvisor` to `TRUE`. Replay. Does it complete now?

⚠ **THIS FAILS SILENTLY AND IT LOOKS LIKE A DESIGN CHOICE.** An unwinnable level does not
error. It just never ends. A tester who plays for two minutes, gets bored and reports
"objective didn't fire, maybe I did it wrong" has produced the exact observation the
experiment wants but has not labelled it as a result. **Play well past the objective
condition, and state the elapsed time you waited.**

**EXPECTED RESULT.** Step 2 never completes; step 3 completes. That confirms both readings
and confirms `docs/research/campaign-and-story.md:290-294`'s conclusion that only
`TimerObjective`, `BlockObjective` and `ConstructionObjective` can end a level through this
path, and only with the briefing advisor disabled.

**FAILURE MODE.** Step 2 completing means the decompiled early-return does not match the
original's behaviour, or a NERPs script in that level completed it instead. **Rule out NERPs
first**: press [F12] (`Debug_ToggleNoNERPs`, `game/Shortcuts.cpp:93`) to disable scripts and
replay. A result obtained without checking that is not usable.

**WHAT TO RECORD.** The level config used, elapsed time waited in each run, whether [F12] was
on, and the outcome. → updates `docs/research/campaign-and-story.md` §2.4 and §7.2,
`docs/NERPS-LANGUAGE.md` §6.1 and §6.2.

---

### X-13 — Does adding one mission blank every save file?

**What it settles.** The save-format trap that gates all campaign work.

**WHY IT MATTERS.** `Front_Save_ReadSaveFile` (`front/FrontEnd.cpp:4717`, OURS) counts
reachable mission links and compares against the count stored in the save; on mismatch it
**rewrites the save blank and memsets the in-memory copy** (`:4738-4748`).
`Front_Save_LoadAllSaveFiles` (`:4853`) runs that check over all six slots at startup. So
adding or removing a single mission is predicted to destroy every existing save
(`docs/research/campaign-and-story.md:826-839`). Three further constraints follow — reordering
`NextLevel` silently remaps completion flags, every level in the chain must be reachable
through `LevelLinks`, and a ninth tutorial's completion is never recorded because
`front/FrontEnd.cpp:4910-4915` hard-codes eight.

**PROCEDURE.**

⚠ **THIS EXPERIMENT IS DESTRUCTIVE BY DESIGN. Copy your entire `Saves\` directory somewhere
outside the game folder before you start.** That is not a precaution, it is step 0.

1. Play far enough to have real progress in at least two save slots. Note exactly which levels
   show as completed in each.
2. Copy `Saves\` to a backup.
3. Add one level to the `NextLevel` chain and make it reachable from `LevelLinks`.
4. Launch. Go to the load-game screen.

**EXPECTED RESULT.** All six slots read as empty. The `.sav` files on disk have been rewritten
to zeros (compare sizes and contents against your backup).

**FAILURE MODE — and this is the dangerous one.** If the count check *passes* (because your
added level was not link-reachable, so `missionsCount` did not change) the saves survive but
`setIndex` may now be remapped, and the completion flags shown will be **wrong rather than
absent**. That is the silent case. **Do not report "saves survived" without checking which
levels are marked complete against the list you wrote down in step 1.**

**WHAT TO RECORD.** The completed-level list before and after, whether the `.sav` files
changed on disk, and the exact edit you made to `NextLevel`/`LevelLinks`. → updates
`docs/research/campaign-and-story.md` §4.5 and justifies (or retires) the campaign-scoped
save-directory proposal at `:866-877`.

---

### X-14 — Is a wide viewport Hor+ or stretched?

**What it settles.** Whether a 16:9 viewport shows *more world* horizontally or the same world
squashed — the one question the whole widescreen plan is blocked on.

**⚠ STATUS: BLOCKED. The code does not exist.** `docs/research/widescreen-plan.md` is a plan;
no `Display` block, no `AspectMode`, and no `DeepCore::Display` namespace exists in
`game/DeepCore.hpp`. It is registered here because the plan already contains the exact
measurement, ready to run the day the layer lands.

**WHY IT MATTERS.** `docs/research/widescreen-plan.md:1043-1044` refuses to assert Hor+ and
says so: it is *"a plausible guess, correctly flagged there as UNDETERMINED, and it stays that
way here."* The plan's key contribution is that **the renderer can answer it at runtime**,
with ordinary C++ that compiles on the authoring machine.

**PROCEDURE (once `AspectMode` exists).** The measurement is `_MeasurePixelSkew`
(`docs/research/widescreen-plan.md:1065-1084`): un-project three screen points forming a small
right-angled cross at screen centre via `Viewport_InverseTransform`
(`engine/gfx/Viewports.cpp:344-361`, the same idiom `interface/RadarMap.cpp:482-500` already
uses) and compare the world-space lengths of the two arms.

1. Set `AspectMode Auto` and run at 1280×720.
2. Read the Info-level line (visible without flags):

   ```
   DeepCore: measured projection pixel skew 1.0000 at 1280x720 (1.0 == square pixels);
   field correction 1.0000
   ```

**EXPECTED RESULT.** Skew ≈ 1.0 ⇒ square pixels ⇒ **Hor+**; a 16:9 viewport already shows more
world and nothing needs doing. Skew ≈ (16/9)/(4/3) ≈ 1.333 ⇒ the projection normalised x by
the aspect ⇒ **stretch**, and the field must be corrected.

⚠ **FAILS SILENTLY when the assumption behind the measurement breaks.** The probe assumes
`z = 1.0` behaves as the far plane on your device. If it does not, the number comes back
plausible-looking and wrong. `docs/research/widescreen-plan.md:1120-1123` states the clamp:
**if the result is above 4 or below 0.25, treat it as invalid, fall back to 1.0 and log.**
A tester should report the raw number, not the correction the game applied.

**WHAT TO RECORD.** The raw skew at three resolutions (1024×768, 1280×720, 1920×1080), plus a
screenshot at each showing the same in-game location, so the "more world vs squashed" question
can also be judged by eye. → updates `docs/research/widescreen-plan.md` §5.2, §5.3 and §6.

---

### X-15 — Is the panel config block really named `Panels<W>x<H>`?

**What it settles.** Risk #1 of the widescreen plan
(`docs/research/widescreen-plan.md:1141`), and with it whether the entire HUD-relayout
approach is even addressable.

**WHY IT MATTERS.** `Panels.h` is 0 implemented of 52. The `Panels<W>x<H>` block name is an
*inference* from `Lego_LoadPanels`'s `(config, screenWidth, screenHeight)` signature
(`game/Game.h:1902`, EXE) and from eight readable `%ix%i` construction sites. If the inference
is wrong, the plan's central change does nothing and the panels are simply absent.

**PROCEDURE.** This is answerable today, with no new code, from X-09's artefact: **search your
`Lego.cfg` for a block whose name contains `640x480`.** If `Panels640x480` exists, the
inference is confirmed. If the panel definitions live under some other key, record the real
name.

The runtime confirmation, once the Display layer exists, is a `PANEL_FLAG_HASIMAGE` count
after `Lego_LoadPanels` returns, logged under `VerboseStartup`
(`docs/research/widescreen-plan.md:1141` — "one line, and it is a real test the first person
with the game can run").

⚠ **FAILS SILENTLY.** If `Lego_LoadPanels` does not find the block it is looking for, the HUD
is drawn with zeroed panel data and the symptom is "the HUD looks wrong", not an error. That
is indistinguishable from a dozen other causes.

**WHAT TO RECORD.** The exact name of the block your `Lego.cfg` uses for panel layout, and
whether any `<W>x<H>` suffix appears in it. → updates `docs/research/widescreen-plan.md` §6
UNDETERMINED item 1.

---

### X-16 — What does raising `WaveMaxAlive` cost in frame time?

**What it settles.** The only wall-clock question in `docs/PERFORMANCE.md`, every instance of
which is currently marked UNDETERMINED and must stay that way until measured.

**WHY IT MATTERS.** `docs/PERFORMANCE.md:207-214` names the collision precisely: two paths are
genuinely O(N²) over live objects, both are ours, and one of them —
`LegoObject_UpdateSlipAndScare` (`game/object/Object.cpp:5313`), which runs a full object
enumeration *for every RockMonster* — is quadratic in exactly the quantity the wave director
exists to raise. *"Raising `waveMaxAlive` without landing A2 first is spending frame time on a
curve that bends the wrong way"* (`:960-961`). Nobody knows whether that is a real problem at
`WaveMaxAlive 12` or a purely theoretical one.

There is a second, separate cost: `GatherCandidates` does O(B · Bld) work in a single frame
every time a wave is scheduled — on a 64×64 map with 20 buildings, roughly 82,000 distance
tests plus 25,000 block reads in one frame (`docs/PERFORMANCE.md:283-287`). That is a *hitch*,
and hitches are more visible than throughput.

**PROCEDURE.**

1. Build `Release|Win32` — not Debug. Confirm `Optimization=MaxSpeed` via
   `-v:diagnostic` (X-03 step 2). Release compiles `/O2 /Oi /Oy /Gy /GL`
   (`docs/PERFORMANCE.md:14-24`).
2. Use one large map (64×64 or bigger) with a built-out base, and one fixed camera position.
   Same save, same starting state, every run.
3. Run a ladder, changing **only** `WaveMaxAlive`: 4, 8, 12, 16, 20. Keep
   `WaveIntervalSeconds 30.0`, `WaveSizeMax 4`.
4. Record frame time. Use an external overlay (any frame-time counter that hooks the
   swap/present path) or the in-game `-fpscap`/`-fpslock` behaviour as a coarse proxy. Note
   which tool you used — that matters for interpreting the number.
5. Separately, watch for the **wave-scheduling hitch**: a single long frame at the moment the
   `telegraphed` line appears in the console. That is `GatherCandidates`. A stutter that
   correlates exactly with the log line is the observation.

⚠ **Does not fail silently — but it is very easy to measure the wrong thing.** Most of the
frame is not ours: 36 of the 159 callees named in `Lego_MainLoop` are raw exe address macros
(`docs/PERFORMANCE.md:56-66`), and `Viewport_Render` and the `Draw_*` family bottom out in
Direct3D Retained Mode and dominate the frame. **A frame-time number is only meaningful as a
*delta across the ladder*, at a fixed camera and a fixed scene.** An absolute FPS figure from
one run tells nobody anything.

**EXPECTED RESULT.** If the quadratic bites, frame time grows super-linearly with
`WaveMaxAlive` — noticeably worse from 12 upward. If it does not, the two O(N²) paths are
real but immaterial at achievable monster counts, and fixes A1/A2 in `docs/PERFORMANCE.md` §5
drop down the ranking.

**FAILURE MODE.** Reporting an absolute FPS number instead of a delta; changing the camera
between runs; or using the Debug build. Any of those makes the result unusable.
`docs/PERFORMANCE.md:907-920` is binding here: **no wall-clock number may be stated for the
running game without the recipe that produced it.**

**WHAT TO RECORD.** A five-row table (`WaveMaxAlive`, median frame time, worst frame time),
the map, the camera position, the measurement tool, and a note on whether a hitch coincided
with each `telegraphed` line. → updates `docs/PERFORMANCE.md` §1.2, §1.4 and §7, and re-ranks
fixes A1, A2 and A4.

---

### X-17 — Do creature variants and beam styles look like anything?

**What it settles.** Whether two shipped cosmetic features actually render, and whether they
deliver the "apparent roster growth with no new art" the plan claims.

**WHY IT MATTERS.** `docs/OVERHAUL-PLAN.md:63-67` argues the felt-variety lever is density and
per-instance differentiation, not roster count. `CreatureVariants` (scale + tint, applied
after `Creature_Clone` so the shared template is never mutated,
`game/object/Object.cpp:954`) and `WeaponBeamStyles` (`game/object/Weapons.cpp:817-864`) are
that argument made concrete. Both ship default-off with **empty tables**, because the names
they need live in the user's `Lego.cfg` (`docs/HANDOFF-2026-07-30.md:140-142`).

**PROCEDURE.**

1. In `Settings\DeepCore.cfg`:

   ```
   VerboseStartup                  TRUE
   CreatureVariants                TRUE

   Variants {
       Brute       RockMonster 1.45 0.55:0.30:0.30
       Runt        RockMonster 0.70 0.90:0.90:1.00
   }
   ```

2. Launch `-programmer -loglevels debug`, spawn several Rock Monsters (press **[E]** on
   emerge-capable walls, or turn on the wave director).

3. Expect, at **Debug** level (`game/DeepCore.cpp:302-305`, OURS):

   ```
   DeepCore: variant "Brute" applied to species ID N (instance 0)
   DeepCore: variant "Runt" applied to species ID N (instance 1)
   ```

   Instances **cycle** through the rows in order rather than rolling dice, so the first
   monster is a Brute, the second a Runt, the third a Brute — deliberately, so every variant
   is actually seen and a bug report reproduces.

4. For beam styles, set `WeaponBeamStyles TRUE` and fill the `BeamStyles` block with weapon
   names from *your* `Lego.cfg WeaponTypes` block, then fire each weapon.
   `GetBeamStyle` logs once per weapon at Debug level (`game/DeepCore.cpp:346-350`):

   ```
   DeepCore: beam style for weapon 0 ("Lazer"): matched
   DeepCore: beam style for weapon 1 ("Pusher"): stock
   ```

⚠ **BOTH FAIL SILENTLY, AND THERE IS A THIRD TRAP THAT IS NOT A LOG PROBLEM.**

- An unresolvable species or weapon name is reported once and skipped
  (`game/DeepCore.cpp:107-127`; `:346-350` reports `stock`). With an empty or all-unresolved
  table, both features are exact no-ops and the game looks completely normal.
- ⚠ **Scaling the mesh does not scale the hitbox.** `CollRadius` and `PickSphere` are separate
  stats (`docs/OVERHAUL-PLAN.md:162`). A 1.45× Brute is punched through by projectiles that
  visually hit it, and a 0.70× Runt has a hitbox larger than its model. **Shoot at every
  variant and say whether the hits land where they look like they should.** That is the real
  finding of this experiment and no log line will tell you.

**EXPECTED RESULT.** Visibly different creature sizes and colours from one species and one
model; visibly different beams per weapon; both `matched` in the log.

**FAILURE MODE.** Everything identical with `matched` in the log ⇒ the scale/tint is being
applied to the wrong container or is being overwritten downstream, which is a real bug worth
a report. Everything identical with `stock`/`skipping it` in the log ⇒ a name problem, which
is a config answer.

**WHAT TO RECORD.** Screenshots of two variants of the same species side by side and of two
different beams; the console lines; and an explicit statement about whether shots landed
correctly on the scaled variants. → updates `docs/HANDOFF-2026-07-30.md` §4 and
`data/Settings/DeepCore.cfg`'s variant documentation.

---

### X-18 — Does the `[W]` debug key decline cleanly under relocation?

**What it settles.** Whether the one genuinely undecompiled thing standing in the water
relocation's way is handled safely.

**WHY IT MATTERS.** `Level_Debug_WKey_NeedsBlockFlags1_8_FUN_004303a0` at `0x004303a0`
(`game/Game.h:1557`, EXE) is the **only** consumer of a `Water_Pool*` outside this DLL. Its
behaviour is commented "unknown" — upstream did not work it out either
(`docs/research/water-relocation.md:663-665`). With the tables relocated there is no
executable-resident pool for it to read, so the key is **declined with a warning rather than
emulated** (`game/Game.cpp:2225-2236`, OURS). Guessing at undecompiled behaviour to preserve a
debug key would be a bad trade (`docs/HANDOFF-2026-07-30.md:127-131`).

**PROCEDURE.** Runs inside X-05 run C, at no extra cost.

1. `RelocateWaterTables TRUE`, `-programmer` (the key needs debug keys enabled, §0.5).
2. Hover the mouse over a map block and press **[W]** (`game/Shortcuts.cpp:117`).
3. Read the console. Expect exactly one Warn line, **once per run**
   (`game/DeepCore.cpp:393-396`, OURS):

   ```
   DeepCore: Warning: the [W] debug keybind is disabled while RelocateWaterTables is on.
   That key calls straight into original 1999 machine code which expects to find a water
   pool inside the executable's own data segment, and with the tables relocated there is
   no such pool. What that code does was never worked out, so it is declined rather than
   guessed at.
   ```

4. Now set `RelocateWaterTables FALSE` and press **[W]** again — this time the exe call goes
   through (`game/Game.cpp:2229`). **Describe what visibly happens.** Nobody in this project
   knows. Anything you can say about it — a surface change, a flood, nothing at all — is a new
   fact.

**FAILURE MODE.** Does not fail silently in the relocated case: the message prints or it does
not. In the **non**-relocated case, whatever the key does is by definition unknown, so record
observations rather than judging them.

**WHAT TO RECORD.** Whether the decline message appeared exactly once; and a description (plus
before/after screenshots) of what the key does with relocation off. → updates
`docs/research/water-relocation.md` §6.9 item 2 and §7.

---

## 2. Standing hazards that apply to more than one experiment

Collected here so they are not repeated in every entry. Each is a way to produce a confident
wrong answer.

| # | Hazard | Where it bites | Cite |
| --- | --- | --- | --- |
| H1 | `VerboseStartup TRUE` prints nothing without `-loglevels debug` | X-00, X-04, X-07, X-08, X-17 | `engine/core/Errors.cpp:26`; `game/DeepCore.cpp:26` |
| H2 | A missing `DeepCore.cfg` is not an error and produces no message | all | `game/DeepCore.cpp:466-470` |
| H3 | An unresolvable name warns once and is then cached as a failure forever | X-01, X-04, X-08, X-17 | `game/DeepCore.cpp:73-76`; `game/DeepCoreAudio.cpp:28-30` |
| H4 | "Fatal" is a log level; the process may or may not stop | all | `engine/core/Errors.h:111`; `engine/core/Errors.cpp:26` |
| H5 | A WAD copy silently beats a loose file of the same name | X-04 | `engine/core/Files.cpp:877-893` |
| H6 | A stereo WAV on a non-`@` cue loads as handle `0` and is inaudible forever | X-04 | `engine/audio/3DSound.cpp:1123`, `:1130`; `game/audio/SFX.cpp:192-195` |
| H7 | No checked iterators, no bounds asserts, no `/RTC` in **either** configuration | X-03, X-10, X-16 | `openlrr.vcxproj:31`, `:111`; `docs/PERFORMANCE.md:510-539` |
| H8 | There is no hook trampoline; a half-reimplemented function is a crash, not a degradation | anything requiring a code change | `hook.cpp:36`, restore paths at `:30-32`, `:47-49`; 1515 installations, zero backup buffers |
| H9 | A `%` in a config name is expanded twice on the way to the console | X-01, X-04 | `engine/core/Errors.cpp:192`, `:206` |
| H10 | Growing any `assert_sizeof` type overwrites its neighbour; never edit a size to make a test pass | any code change | `docs/ADDRESS-MAP.md` — 113 regions, 0 overlaps; `tools/addrlint/addrlint.py` |

---

## 3. How to report results so they land as evidence

An answer that cannot be checked is an anecdote, and this project has spent a lot of effort
making sure its documents are evidence rather than memory. Please match that standard.

### 3.1 Every report carries these five things

1. **The experiment ID.** `X-01`, `X-07c`. One report per experiment. Do not bundle.
2. **The exact command line**, copy-pasted, including flags you thought were irrelevant.
3. **The exact gate settings** — paste the changed lines of `Settings\DeepCore.cfg`, not a
   summary of them. "I turned the wave director on" is not a setting.
4. **The console output, verbatim, as text.** From launch to the moment of interest. Not a
   screenshot of text, not a paraphrase, not "there was a warning about names". Warnings carry
   the config file and line number (`engine/core/Config.h:342`) and those are load-bearing.
5. **The build you ran** — Debug or Release, and the DLL's build date. If you changed code,
   the diff, plus the `-t:Rebuild` warning count for both configurations.

### 3.2 What to attach rather than paste

- The 294-line NERPs table dump (X-02) — as a `.txt` file.
- Your `Lego.cfg`, with personal paths redacted (X-09) — as a file.
- Screenshots for anything visual (X-06, X-07, X-14, X-17, X-18).
- A crash dump or debugger call stack if anything terminated.

### 3.3 Say which of these your answer is

Borrowed from `docs/PERFORMANCE.md` §6, which is binding on performance claims and is the
right discipline for all of these:

- **OBSERVED** — you saw it happen, and the console output above shows it.
- **NOT OBSERVED** — you looked, under the stated conditions, and it did not happen. State
  how long you waited and how many attempts you made. This is a real result and is often the
  more valuable one.
- **INCONCLUSIVE** — the run did not distinguish the hypotheses. Say why. This is an
  acceptable and honest answer; a guess dressed as an observation is not.

⚠ **Never report "it works" or "it doesn't work".** Every entry above has at least two
distinguishable failure modes and several have three. The distinction *is* the finding.

### 3.4 Which document each answer updates

| Experiment | Primary document to update | Also |
| --- | --- | --- |
| X-00 | `docs/HANDOFF-2026-07-30.md` §6 | `docs/WORKLOG.md` |
| X-01 | `docs/OVERHAUL-PLAN.md` §1, §6 | `docs/research/type-loader-reimplementation.md` §4.5, §7; `docs/research/stats-overhaul.md` §3.12 |
| X-02 | `docs/NERPS-LANGUAGE.md` §4 (the whole arity column), §7.3 | §8 ranked plan |
| X-03 | `docs/PERFORMANCE.md` §7 | `docs/WORKLOG.md` |
| X-04 | `docs/research/audio-pipeline-truth.md` §4.4, §8.2, §8.4 | `docs/research/audio-and-juice.md` §5.7 |
| X-05 | `docs/research/water-relocation.md` §10 | `data/Settings/DeepCore.cfg` |
| X-06 | `docs/OVERHAUL-PLAN.md` §1, §3, §6 | — |
| X-07 | `docs/research/wave-director.md` §9 | `data/Settings/DeepCore.cfg` defaults |
| X-08 | `docs/research/stats-overhaul.md` §3.12 | `data/Settings/DeepCore.cfg` |
| X-09 | `docs/OVERHAUL-PLAN.md` §3 Stage 2 | `docs/research/audio-pipeline-truth.md`; `docs/research/widescreen-plan.md` |
| X-10 | `docs/HANDOFF-2026-07-30.md` §4 | `docs/research/wave-director.md` §9 |
| X-11 | `docs/research/audio-pipeline-truth.md` §7.4, §8.4 | — |
| X-12 | `docs/research/campaign-and-story.md` §2.4, §7.2 | `docs/NERPS-LANGUAGE.md` §6 |
| X-13 | `docs/research/campaign-and-story.md` §4.5 | — |
| X-14 | `docs/research/widescreen-plan.md` §5.2, §5.3, §6 | — |
| X-15 | `docs/research/widescreen-plan.md` §6 | `docs/OVERHAUL-PLAN.md` |
| X-16 | `docs/PERFORMANCE.md` §1.2, §1.4, §7 | re-ranks fixes A1, A2, A4 |
| X-17 | `docs/HANDOFF-2026-07-30.md` §4 | `data/Settings/DeepCore.cfg` |
| X-18 | `docs/research/water-relocation.md` §6.9, §7 | — |

### 3.5 How the answer gets committed

An answer replaces a marker; it does not get appended beside one.

1. Delete the **UNDETERMINED** marker in the target document and write the measured fact in
   its place, with the date and the reporter.
2. Add a `docs/WORKLOG.md` entry stating what was run, what was observed, and what remains
   unproven — the same three-part shape every entry there already uses.
3. Add a row to §1's ranking table above marking the experiment ANSWERED, with the date.
   **Do not delete the entry.** A closed experiment with its procedure intact is how the
   answer gets re-checked on a different installation.
4. Run `python tools/docsaudit/docsaudit.py --check` before committing. It fails CI on any
   load-bearing number that drifts between documents (`.github/workflows/build_artifacts.yml:41`),
   which is exactly the failure mode a scattered answer causes.
5. If you touched any code, run `python tools/addrlint/addrlint.py --check` too and confirm
   `docs/ADDRESS-MAP.md` is unchanged.

---

## 4. DECISION

**Run X-00 first, then X-01 and X-02, in that order, and stop.** Report those three before
starting anything else.

The reasoning is not that the others are unimportant — it is that those three change what the
rest of the register *is*.

- **X-00** is the only entry whose failure invalidates every other result in this file. If
  `DeepCore.cfg` is not being found, X-04 through X-18 are all measuring stock OpenLRR and
  will all come back "no", correctly and uselessly. It costs one launch.
- **X-01** is the largest single fork in the project. A "yes" opens roughly four new creature
  identities, revives three of the eleven designs in `docs/research/stats-overhaul.md` §3, and
  makes `EmergeSpeciesPool`, `WaveSpeciesPool` and the `Variants` table worth filling in. A
  "no" closes the roster at eleven permanently and redirects the whole effort onto density,
  per-instance differentiation and NERPs-authored content — which is where
  `docs/OVERHAUL-PLAN.md:61-72` already argues the value is. Either answer is worth having;
  not having it is what is expensive, because two documents are currently written to cover
  both branches.
- **X-02** costs one debugger session with no code change and no rebuild, cannot corrupt
  anything, cannot fail ambiguously, and converts 293 rows of inferred arity into measured
  fact in a single artefact. It is the highest evidence-per-minute item in this repository.
  `docs/NERPS-LANGUAGE.md:1487-1491` calls it "the cheapest possible way to falsify this
  document"; that is exactly why it should be run early rather than saved for when the
  assembler is being written.

After those three, the ordering is X-04 (audio, because it also settles the `Samples` block
path that a generator is blocked on), X-05 (water, because it is the only entry that fixes a
twenty-five-year-old crash for real users), then X-07 and X-16 together, since tuning the wave
director and measuring what it costs are the same session.

**X-09 and X-14 are blocked on code that does not exist and must not be assigned to a tester
as written.** They are registered so the blocker is visible, not so somebody tries.

And the standing rule, unchanged: **compile-verified is the ceiling on this machine.** Every
row of this register exists because that ceiling is real. An answer from a running game is
worth more than any amount of further reading, and it is the only thing that can move a line
in these documents from UNDETERMINED to fact.
