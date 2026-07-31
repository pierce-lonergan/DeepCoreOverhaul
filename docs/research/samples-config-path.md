# The `Samples` Config Path — Settled

**Question asked:** `docs/research/audio-pipeline-truth.md` §4.4 / §8.2 / §8.4 mark the literal config
path of the `Samples` block **UNDETERMINED**, because `Lego_LoadSamples` is an exe address macro.
The shipped threat-audio layer (`src/openlrr/game/DeepCoreAudio.cpp`) resolves cue names through
`SFX_GetType`, so if the name is not declared in the user's config the cue is silent. Settle the
path, or prove it unsettleable.

**Verdict, up front:**

> ## `Lego*::Samples` — a DIRECT CHILD of the root block, a **sibling of `Main`**, not a child of it.
>
> The repository's own inference (`Lego*::Main::Samples`) was **WRONG**. `Main` contains only
> scalars. `Samples` sits at depth 1 alongside `TextMessages`, `Stats`, `WeaponTypes`, `Levels`.
>
> Settled by three independent lines of evidence that agree exactly: a published decompilation of
> the very function we cannot read, two published `Lego.cfg` files, and the RRU knowledge base.
> Confidence: **as high as anything in this project that has not been run.**

Everything marked **OURS** below is implemented C++ in this repository and is quoted from it.
Everything marked **EXE MACRO** is still 1999 machine code behind an address macro. Everything
marked **EXTERNAL** is community data or a third-party decompile, cited by URL — it is corroborating
evidence, not something this repo compiles. **We still cannot run the game.**

---

## 0. Answers, compressed

| # | Question | Answer |
|---|---|---|
| 1 | Call order | `SFX_Initialise` (`GameState.cpp:308`) → `Lego_LoadSamples` (`GameState.cpp:438`, EXE MACRO) which brackets its own parse with `SFX_SetSamplePopulateMode(TRUE/FALSE)` and calls `SFX_GetType` + `SFX_LoadSampleProperty` per line → `Lws_Initialise` (`GameState.cpp:452`) wires `SFX_GetType` for scene triggers. Populate mode is on **only** inside `Lego_LoadSamples`. |
| 2 | Value grammar | `[*][#mB#][@]path[,path…]` — prefixes in that fixed order, per token, no spaces, no extension (`SFX.cpp:154-185`). Plus a **`!` prefix on the KEY** (not the value), which the repo had never documented. |
| 3 | Key path | `<gameName>::Samples`, i.e. `Lego*::Samples` as written in the file, `LegoRR::Samples` as looked up. **Settled.** |
| 4 | External proof | Decompiled `Lego_LoadSamples`; two published `Lego.cfg` files; RRU KB structure page; RRU syntax thread. All four agree. URLs in §4. |
| 5 | User instructions | §5. One block edit, one WAV per cue, `-cfgfirst` **required** if you keep `Lego.cfg` loose. |
| 6 | Zero-config registration | **Viable, and recommended.** All three functions are OURS and hooked; the insertion point is one line of our own C++ at `GameState.cpp:439`. Design and hazards in §6. |

**Two corrections to `audio-pipeline-truth.md` fall out of this and matter more than the path itself:**

- Free `hashNameList` slots in a real installation are **~35, not 450** (§7.1). The vanilla `Samples`
  block has **446 entries**.
- The `Samples` block is **not re-read per level**, and `SFX_Initialise` runs **once per process**
  (`GameState.cpp:308` is the only call site). `DeepCoreAudio.hpp`'s comment that "the SFX table is
  rebuilt per level" is wrong; `InvalidateCueCache` is harmless but unnecessary.

---

## 1. Every call site, and the startup order

### 1.1 The four functions, and who owns them

| Function | Owner | Where | Hooked at |
|---|---|---|---|
| `Lego_LoadSamples(const Config*, bool32 noReduceSamples)` | **EXE MACRO** `0x00434980` | `Game.h:1914` (declaration commented out at `Game.h:1915`, `Game.cpp:4125`) | not hooked — no reimplementation exists |
| `SFX_SetSamplePopulateMode(bool32 on)` | **OURS** | `SFX.cpp:105-109`, decl `SFX.h:153` | `interop.cpp:4027` → `0x00464f10` |
| `SFX_GetType(const char*, OUT SFX_ID*)` | **OURS** | `SFX.cpp:112-137`, decl `SFX.h:157` | `interop.cpp:4028` → `0x00464f30` |
| `SFX_LoadSampleProperty(char* value, SFX_ID)` | **OURS** | `SFX.cpp:140-245`, decl `SFX.h:168` | `interop.cpp:4031` → `0x00464fc0` |

Because the latter three are hooked, the exe's own `Lego_LoadSamples` calls **our** code for all
three. So the grammar in §2 is what actually runs, not an approximation of it.

### 1.2 Complete reference list (whole tree, source only)

`Lego_LoadSamples` — 4 references, 1 of them a call:

- `src/openlrr/game/Game.h:1914` — the address macro.
- `src/openlrr/game/Game.h:1915` — commented-out prototype.
- `src/openlrr/game/Game.cpp:4125` — commented-out definition stub.
- **`src/openlrr/game/GameState.cpp:438`** — the only call:
  ```cpp
  Lego_LoadSamples(legoConfig, !Gods98::Graphics_IsReduceSamples());
  ```
  `Graphics_IsReduceSamples()` is `mainGlobs.flags & MAIN_FLAG_REDUCESAMPLES` (`Graphics.h:203`),
  set by `-reducesamples` (`Main.cpp:1041`, `:1410`).

`SFX_SetSamplePopulateMode` — 2 references: definition `SFX.cpp:105`, declaration `SFX.h:153`.
**Nothing in this repository ever calls it.** Its only caller is the exe's `Lego_LoadSamples`.

`SFX_LoadSampleProperty` — 2 references: definition `SFX.cpp:140`, declaration `SFX.h:168`.
**Nothing in this repository ever calls it either.** Same single exe caller.

`SFX_GetType` — 20 call sites, all OURS:

| Caller | Name source | Citation |
|---|---|---|
| `SFX_Container_SoundTriggerCallback` | activity `SAMPLE` key | `SFX.cpp:98` |
| `SFX_Callback_FindSFXIDFunc` (LWS trigger) | `.lws` null-object name | `SFX.h:162`, wired `GameState.cpp:452` |
| **`DeepCore::Audio::PlayThreatCue`** | `DeepCore.cfg` cue name | **`DeepCoreAudio.cpp:77`** |
| FrontEnd menu items / two menu SFX helpers | menu config | `FrontEnd.cpp:2855`, `:3864`, `:3874` |
| Low-oxygen heartbeat (3 sites) | literal `"SND_AirBeat"` | `GameState.cpp:1715`, `:1725`, `:1729` |
| Advisor | advisor config token 2 | `Advisor.cpp:209` |
| Priority buttons | priority config token 1 | `Priorities.cpp:61` |
| Text message panel | `textGlobs.textImagesSFX[…]` | `TextMessages.cpp:207` |
| Objectives | `objectiveGlobs.soundName`, built as `"Stream_Objective%s_%s"` | `Objective.cpp:1006`, name built at `:744-752` |
| Raider hurt | literal `"SND_Hurt"` | `Object.cpp:4049` |
| Object stats | `DrillSound` / `DrillFadeSound` / `EngineSound` | `Stats.cpp:298`, `:301`, `:305` |

Every one of those runs **after** `Lego_LoadSamples`, i.e. with populate mode **off**. They are pure
lookups; an unknown name returns `false`.

### 1.3 The startup order, reconstructed

All line numbers are inside `LegoRR::Lego_Initialise` (**OURS**, `GameState.cpp:95`), which is the
whole story — the ordering is in our source, not the exe.

```
 Main_WinMain                                   Main.cpp:826
   Sound_Initialise(-nosound)                   Sound.cpp:71   -> Sound3D_Initialise  3DSound.cpp:126
 Gods_Go(programName)                           OpenLRR.cpp:1016
   legoProgramName := "LegoRR" (or -name)       OpenLRR.cpp:1020-1039
 Lego_Initialise()                              GameState.cpp:95
   :146   DeepCore::Load()                      <- reads OUR DeepCore.cfg (cue NAMES land here)
   :178   rootConfigName := -cfgfile or "Lego.cfg"        GameCommon.h:71
   :181-183 +FILE_FLAG_DATAPRIORITY if -cfgfirst          Main.h:450
   :185   legoGlobs.config := Config_Load2(...)
   :195-201 -cfgadd files appended                        Config.cpp:834
   :308   SFX_Initialise()                      SFX.cpp:32   45 preload names, table alloc
   ...
   :438   Lego_LoadSamples(legoConfig, !reduceSamples)     <-- EXE MACRO, THE PARSE
   :440   Lego_LoadSurfaceTypeDescriptions_sound(...)
   :442-446 Priorities / ToolTips / ToolNames / PanelRotation
   :452   Lws_Initialise(..., SFX_Callback_FindSFXIDFunc, ...)   scene triggers now resolvable
   :566   Stats_Initialise(...)                 <- DrillSound etc. resolve here
```

Note `SFX_Initialise` appears **exactly once** in the tree (`GameState.cpp:308`; the only other hits
are the definition `SFX.cpp:32` and the hook `interop.cpp:4024`). The sample table is therefore
built once per **process**, not per level.

### 1.4 What `Lego_LoadSamples` does — the body we could not read

Recovered from an independent decompilation project, `ProjectReversio/LegoRockRaiders`,
`src/LegoRR/lego.c:3080-3104` (**EXTERNAL**,
<https://github.com/ProjectReversio/LegoRockRaiders/blob/master/src/LegoRR/lego.c>):

```c
void Lego_LoadSamples(lpConfig config, B32 noReduceSamples)
{
    SFX_ID sfxType;

    lpConfig prop = Config_FindArray(config, Config_BuildStringID(legoGlobs.gameName, "Samples", 0));
    if (!prop)
        return;

    SFX_SetSamplePopulateMode(TRUE);
    do
    {
        const char* name = prop->itemName;
        B32 reduced = FALSE;
        if ((*name == '!') && (name++, noReduceSamples == 0))
            reduced = TRUE;
        if (!reduced)
        {
            if (SFX_GetType(name, &sfxType))
                SFX_LoadSampleProperty(prop->dataString, sfxType);
        }
        prop = Config_GetNextItem(prop);
    } while (prop != NULL);
    SFX_SetSamplePopulateMode(FALSE);
}
```

Six things fall out, every one of which is checkable against **our** code:

1. **The key path is `Config_BuildStringID(legoGlobs.gameName, "Samples", 0)`** — two components,
   not three. `Config_BuildStringID` joins with `CONFIG_SEPARATOR` `"::"` (`Config.h:51`,
   `Config.cpp:225-245`), so the lookup string is `LegoRR::Samples`. This is `Lego_ID("Samples")`
   in our macro vocabulary (`Game.h:793`), **not** `Main_ID("Samples")` (`Game.h:794`).
2. **`Config_FindArray` + `Config_GetNextItem`** is the flat-array idiom (`Config.cpp:337-364`,
   both **OURS**): find the block, take its first child, walk siblings at the same depth, stop when
   depth drops. It confirms `Samples` is a block of key→value lines with **no nesting**.
3. **`!` on the KEY is the reduce marker.** `name++` is inside the comma expression, so the `!` is
   stripped from the registered name whenever it is present; the entry is *skipped* only when
   `noReduceSamples == 0`, i.e. when `-reducesamples` is on. This prefix appears **nowhere** in this
   repository's source or docs and is the single biggest gap the audit closes after the path itself.
4. **Populate mode brackets exactly the parse** — matching `SFX.cpp:126-134` and confirming
   `audio-pipeline-truth.md` §4.2's claim about the registration window.
5. **`prop->dataString` is passed straight to `SFX_LoadSampleProperty`**, whose parameter is
   `char*` (`SFX.cpp:140`) and whose `Util_Tokenise` is `IN OUT` and writes `'\0'` over each comma
   in place (`Utils.cpp:41-67`). So the parse is **destructive**: after `Lego_LoadSamples`, reading
   `LegoRR::Samples::<name>` back through the config API returns only the first alternate.
6. **A missing `Samples` block is a silent no-op** (`if (!prop) return;`) — no warning, no fatal.

> **How much weight does a third-party decompile carry?** On its own, moderate. But its
> `Lego_Initialise` body (`lego.c:348-366`) is line-for-line the same sequence as our
> `GameState.cpp:438-366`… including calling `Lws_Initialize` with `SFX_GetType` and reading
> `"Main", "SharedObjects"` via `Config_BuildStringID(legoGlobs.gameName, "Main", "SharedObjects", 0)`
> — the exact shape of our `Main_ID("SharedObjects")` at `GameState.cpp:451`. It is describing the
> same binary we are. And §4 corroborates the conclusion from two data sources that never saw it.

---

## 2. The VALUE grammar — reconstructed from `SFX_LoadSampleProperty` (OURS, certain)

This is the half that was always determinable, and it is worth stating exhaustively because two
layers of tokenising stack on top of each other: the config parser's, then the sample parser's.

### 2.1 Layer 1 — what the config parser hands over

`Config_Load2` (**OURS**, `Config.cpp:103-128`) converts **every** space, tab, CR and LF to `'\0'`,
and blanks everything from a `;` to end of line:

```cpp
// Config.cpp:125
if (commentMode || (c == '\t' || c == '\n' || c == '\r' || c == ' ')) *s = '\0';
```

Then `Config.cpp:146-191` walks the resulting NUL-separated tokens, taking them strictly alternately
as `itemName`, `dataString`, `itemName`, `dataString`… So:

- **The key is the first whitespace-delimited token; the value is the second.** Nothing else on the
  line is part of the value.
- **A value can never contain a space or tab.** It cannot be quoted either — there is no quoting.
- **Trailing tabs are harmless** (they became NULs). The vanilla block relies on this:
  `!SFX_Ambientloop  Sounds\iambloop<TAB>` parses as `Sounds\iambloop`.
- **A space inside a comma list is corrupting, not merely wrong.** `A  x,<space>y` yields
  `itemName=A`, `dataString="x,"`, and then `y` becomes the **next itemName**, which swallows the
  following line's key as its value, and the whole rest of the block shifts by one. Every entry
  after the mistake is silently mis-bound. This is worse than the "second token is `" b"`" failure
  described in `audio-pipeline-truth.md` §4.4 gotcha 1 — that failure mode is unreachable *from a
  config file*; the cascade is what actually happens.
- A `;` starts a comment anywhere on the line (`Config.cpp:106`), so `;` cannot appear in a path.

### 2.2 Layer 2 — `SFX_LoadSampleProperty`, line by line

```cpp
// SFX.cpp:140-152
bool32 __cdecl LegoRR::SFX_LoadSampleProperty(char* value, SFX_ID sfxID)
{
    char volBuff[64] = { 0 };
    char* sampleNames[100];                       // :143  UNGUARDED
    SFX_Property* lastItem = nullptr;
    SFX_Property* curItem  = &sfxGlobs.samplePropTable[sfxID];
    sfxGlobs.samplePropTable[sfxID].next = nullptr;
    bool32 stream  = false;                       // :149  DECLARED OUTSIDE THE LOOP
    bool32 success = true;
    uint32 count = Gods98::Util_Tokenise(value, sampleNames, ",");   // :152  destructive
```

then per token (`SFX.cpp:154-185`):

```cpp
const char* s = sampleNames[i];
bool32 simultaneous = false;                      // :156  per-token
sint32 volume       = 0;                          // :157  per-token

if (*s == '*') { s++; simultaneous = true; }                       // :159-162
if (*s == '#') { s++; while (*s != '#') *v++ = *s++; s++; *v='\0';  // :164-173
                 volume = std::atoi(volBuff); }                    // :174
if (*s == '@') { s++; stream = true; }                             // :177-180
sint32 sound3DHandle = Gods98::Sound3D_Load(s, stream, simultaneous, volume);  // :185
```

### 2.3 The grammar, stated

```
<samples-line>  ::= [ "!" ] <cue-name> <WS+> <value>
<value>         ::= <token> { "," <token> }
<token>         ::= [ "*" ] [ "#" <int> "#" ] [ "@" ] <path>
<path>          ::= data-dir-relative path, backslashes, NO ".wav" extension
```

| Element | Meaning | Enforced at |
|---|---|---|
| `!` on the **key** | drop this entry when `-reducesamples` is on; stripped from the registered name either way | `lego.c:3093` (EXTERNAL), flag read at `Graphics.h:203` |
| `*` | "multi" — allocate `SOUND3D_MAXSIMULTANEOUS` = 3 duplicate buffers so the sample can overlap itself; without it a new play *restarts* the one buffer | `SFX.cpp:159-162` → `3DSound.cpp:1137-1148`, `3DSound.h:56` |
| `#N#` | buffer volume in **millibels**, `-10000` (silent) … `0` (full). Out-of-range is warned and replaced with `0` | `SFX.cpp:164-175` → `3DSound.cpp:330-333` |
| `@` | stream from disk instead of loading to memory; **loose file or CD only, never a WAD**; may be stereo; cannot be positional or simultaneous | `SFX.cpp:177-180` → `3DSound.cpp:282-296` |
| `,` | alternates — one is picked at random per play; alternates 2..N consume `sampleGroupTable` entries | `SFX.cpp:152`, `:198-241`, `SFX_Random_GetSound3DHandle` `SFX.cpp:249-278` |
| path | `.wav` appended by the loader (`3DSound.cpp:280`), resolved under `<game>\Data\` (`Files.cpp:1184-1205`) | — |

**Order is fixed and positional.** `#-600#*Sounds\x` does not parse as a multi sample: the `*` test
already ran and failed, so `*` becomes part of the filename.

### 2.4 A well-formed line

```
        !SND_MyCue          *#-600#Sounds\DeepCore\mycue
```

reads: droppable under `-reducesamples`; cue name `SND_MyCue`; 3 overlapping voices; −6 dB;
file `<game>\Data\Sounds\DeepCore\mycue.wav`.

Real published examples, from the vanilla block (**EXTERNAL**, §4.2):

```
        SFX_Bodge                   Sounds\Minifigure\screw1
        !SFX_Drip                   Sounds\drip1,Sounds\drip2,Sounds\drip3,Sounds\dripsA,Sounds\dripsB,Sounds\dripsC
        SFX_Drill                   *Sounds\drtdrillc
        !SFX_Lava                   *Sounds\New\lavbub
        SND_AirBeat                 Sounds\New_Sfx\Heart
        Stream_Objective_Levels::Level01    @Sounds\Streamed\Objectiv\MisObj01
        !FrontSFX_RockMonster       @Sounds\Streamed\FronRock
```

### 2.5 Seven gotchas in the grammar, all verified in our code

1. **`stream` leaks across tokens.** `bool32 stream = false;` is at `SFX.cpp:149`, outside the loop,
   while `simultaneous` and `volume` are inside (`:156-157`). One `@` makes every *subsequent*
   alternate on that line streamed too. Every `@` line in the vanilla block is single-token, so
   stock data never exercises the bug — which is exactly why it survived.
2. **`char* sampleNames[100]` is unguarded** (`SFX.cpp:143`) and `Util_Tokenise` passes `uint32` max
   as the limit (`Utils.cpp:43`). ≥100 commas smashes the stack. Observed maxima in real data: 6
   tokens (vanilla), 13 (a large mod). `Util_TokeniseSafe` (`Utils.cpp:47`) is the drop-in fix.
3. **`#` with no closing `#` runs off the end of the string** — `while (*s != '#') *v++ = *s++;`
   (`SFX.cpp:167-169`) into a 64-byte `volBuff`. The missing null terminator was fixed
   (`SFX.cpp:172`); the scan bound was not. **No stock line uses `#` at all** — zero occurrences in
   either published cfg — so this is a modder-only landmine.
4. **A trailing comma creates an empty token.** `Util_TokeniseSafe` pushes `s + len` after the final
   separator (`Utils.cpp:61`), giving `""` → `Sound3D_Load("")` → `".wav"` → load fails → `-1` →
   `success = false`. Harmless but it warns.
5. **Failure does not stop the line.** `if (sound3DHandle == -1) { success = false; /*no break*/ }`
   (`SFX.cpp:192-195`). The cue keeps `sound3DHandle == 0` from `SFX_Initialise`'s memset
   (`SFX.cpp:35`), and `0` is rejected by every play guard `if (handle > 0)` (`SFX.cpp:350`, `:431`).
   **A cue that fails to load is indistinguishable from one that loaded into table slot 0.**
6. **The value buffer is destroyed by parsing** (`Utils.cpp:60`). Anything that re-reads a `Samples`
   value after startup sees only the first alternate.
7. **Names are matched by 32-bit hash only** — `Util_HashString(name, false, true)` (`Utils.cpp:180`,
   upper-cased, whitespace significant), compared without a `strcmp` (`SFX.cpp:119`). Cue names are
   therefore **case-insensitive**, and two names that collide alias silently.

---

## 3. The KEY path — the evidence chain, and why the old inference was wrong

### 3.1 What the repository can prove on its own

- `legoGlobs.gameName` is the program name forced to start with `Lego` (`OpenLRR.cpp:1020-1038`),
  defaulting to the literal `"LegoRR"` (`OpenLRR.cpp:1029`), overridable with `-name`/`-gamename`
  (`Main.cpp:1239`, `:1250`, `Main.h:299`).
- `Lego_ID(...)` = `Config_ID(gameName, ...)` (`Game.h:793`); `Main_ID(...)` =
  `Config_ID(gameName, "Main", ...)` (`Game.h:794`); `Config_ID` = `Config_BuildStringID(..., nullptr)`
  (`Config.h:336`) joining with `"::"` (`Config.h:51`).
- **The `*` wildcard lives on the config-file side, at depth 0 only** (`Config.cpp:704-729`):
  ```cpp
  if (!fullMatch && conf->depth == 0) { ... if (*s == CONFIG_WILDCARDCHAR)
        wildcardMatch = (::_strnicmp(conf->itemName, name, wildcardLength) == 0); }
  ```
  So a file whose root block is literally `Lego* {` answers a lookup for `LegoRR::…`. That is why
  this project's own `data/Settings/DeepCore.cfg:20` and `data/Settings/Shortcuts.cfg:72` open with
  `Lego* {`.
- **Every depth-1 block loader we own uses `Config_ID(gameName, "<Block>")`, never `Main`:**
  `Config_ID(gameName, "PriorityImages")` (`Priorities.cpp:50`),
  `Config_ID(gameName, "PrioritiesImagePositions", …)` (`:155`),
  `Config_ID(gameName, "Stats")` (`Stats.cpp:129`),
  `Lego_ID("WeaponTypes")` (`Weapons.cpp:47`),
  `Lego_ID("Bubbles")` (`Bubbles.cpp:102`),
  `Lego_ID("Pointers")` (`GameState.cpp:682`),
  `Lego_ID("ToolTipIcons")` (`Object.cpp:271`),
  `Config_ID(gameName, "RockFallStyles")` (`Effects.cpp:132`),
  `Config_ID(gameName, "Advisor", animName)` (`Advisor.cpp:123`),
  `Config_ID(gameName, "Encyclopedia", name)` (`Encyclopedia.cpp:53`),
  `Config_ID(gameName, "MiscObjects", …)` (`Effects.cpp:29`, `Game.cpp:183`).
- **All 79 `Main_ID(...)` uses in the tree are scalars** — `LoadScreen`, `SharedObjects`,
  `TextureUsage`, `CameraSpeed`, `PowerCrystalRGB`, `NextButton%ix%i` … There is **not one**
  `Config_FindArray(config, Main_ID(...))` anywhere. `Main` holds no sub-blocks.

That last point alone should have falsified `Lego*::Main::Samples` before this audit. The old
inference rested on "every other `Main`-level key goes through `Main_ID`" — true, but it is
reasoning about the wrong set: `Samples` is a *block*, and no block is under `Main`.

### 3.2 What the repository cannot prove

The literal string `"Samples"` appears **nowhere** in `src/` as a config key. The only in-tree hits
are prose: the comment at `SFX.cpp:205` ("every `Samples` line in the config"), the module note at
`src/openlrr/game/README.md:8`, the inventory line `docs/DIRECTORY.md:470`, our own warning text at
`DeepCoreAudio.cpp:86`, and the modder note at `data/Settings/DeepCore.cfg:135`. None of them is
evidence of *depth*. **From this repository alone the path is genuinely unsettleable** — the old
UNDETERMINED marker was honest. It is settled only because external evidence exists.

### 3.3 The chain that settles it

| Source | Kind | What it says |
|---|---|---|
| `ProjectReversio/LegoRockRaiders`, `src/LegoRR/lego.c:3084` | decompiled code | `Config_BuildStringID(legoGlobs.gameName, "Samples", 0)` — **two** components |
| `miningmanna/LRR-remake`, `Data/Lego.cfg` line 1268 | published data | `Samples {` at **depth 1**, between `TextMessagesWithImages` (1252) and `InfoMessages` (1799) |
| `ShadowOfNight/Rock-Raiders-Reloaded`, `Data/Lego.cfg` line 1541 | published data | `Samples {` at **depth 1**, between `UpgradeNames` (1526) and `BuildingTypes` (2053) |
| RRU knowledge base, `Lego.cfg` | community doc | outline lists `Samples { }` as a sibling of `Main { }` under `Lego*` |
| RRU forum, "In-depth Look at the CFG Syntax" | community doc | wildcard is root-level only; lookups are `LegoRR::Block::Path::To::Key` |

Four sources, three of them independent of each other, zero disagreement. **Confidence: settled.**
The only residual uncertainty is the one this project can never remove — nobody here has run it.

---

## 4. External evidence, in full

### 4.1 The decompiled loader

- <https://github.com/ProjectReversio/LegoRockRaiders> — reverse-engineering project.
  `src/LegoRR/lego.c:3080-3104` is `Lego_LoadSamples`, quoted in full in §1.4;
  `src/LegoRR/lego.c:348` is its call site, matching our `GameState.cpp:438`;
  `src/LegoRR/sfx.c` carries `SFX_LoadSampleProperty` with the same signature as our `SFX.cpp:140`.

### 4.2 Two published `Lego.cfg` files

- <https://github.com/miningmanna/LRR-remake/blob/master/Data/Lego.cfg> — 6814 lines, appears to be
  a straight extraction (it still contains `SFX_Radar`, which the mod below removes).
- <https://github.com/ShadowOfNight/Rock-Raiders-Reloaded/blob/master/Data/Lego.cfg> — 7436 lines, a
  large overhaul mod, structurally identical.

Structure recovered by brace-counting both files (`Samples` marked):

```
Lego* {                            (line 39 / 6)
    Main { … }                     (44 / 8)          scalars only
    …
    TextMessages { … }
    TextMessagesWithImages { … }
    Samples { … }                  (1268 / 1541)     <-- DEPTH 1
    InfoMessages { … }
    …
    Stats { … }  WeaponTypes { … }  Levels { … }
}
LegoRR {                           (6796 / 7428)     "settings for the final version"
    Main { … }
}
```

Measured contents of the vanilla-style `Samples` block (`LRR-remake`, lines 1268–1798):

| Measure | Value | Consequence |
|---|---|---|
| entries | **446**, all unique names, **no nesting** | flat `Config_GetNextItem` walk is valid |
| entries carrying `!` | 242 | most of the game goes quiet under `-reducesamples` |
| names matching a preload `SFX_ID` | 31 of 45 | 415 names are registered dynamically |
| WAV tokens total | 480 of `SOUND3D_MAXSAMPLES` 600 | **120 free sound-table slots** |
| group entries needed | 34 of `SFX_MAXSAMPLEGROUPS` 200 | comfortable |
| max tokens on one line | 6 (13 in the mod) | far below the unguarded 100 |
| lines using `#volume#` | **0** | the volume grammar is unexercised by stock data |
| lines using `@` | 147 | streamed speech and front-end sounds |
| lines using `*` | 13 | |
| **first entry in the block** | `SFX_Bodge  Sounds\Minifigure\screw1` | see §7.2 |

### 4.3 Community documentation

- <https://kb.rockraidersunited.com/Lego.cfg> — structural outline; `Samples { }` listed as a
  sibling of `Main { }`, plus the `LegoRR { Main { } }` override block at the end.
- <https://rockraidersunited.com/topic/9048-in-depth-look-at-the-cfg-syntax/> — documents all four
  prefixes independently of our source: `!` = "removed with `-reduce____` command-line arguments";
  `*` = "a sound can have multiple instances playing at once… if this prefix is not present… it will
  cut off any currently playing instance"; `@` = streamed; `#-####` = volume reduction, "-10000 to
  0". Also: the asterisk "is used as a wildcard in block/property key names (at the root level only)"
  and "Rock Raiders uses the executable name (LegoRR.exe) → `LegoRR::Block::Path::To::PropertyKey`".
- <https://kb.rockraidersunited.com/Config_documentation> — the `Samples` section is an empty
  heading; the page is explicitly unfinished. Cited for completeness; it settles nothing.

---

## 5. User instructions — making the shipped cues audible

The five shipped threat cues and their default names (`DeepCore.hpp:147-152`,
`data/Settings/DeepCore.cfg:153-157`), with the WAVs that ship in `assets/audio/threat/`:

| `DeepCore.cfg` key | Default cue name | Shipped WAV |
|---|---|---|
| `CueTelegraph` | `dc_threat_telegraph` | `assets/audio/threat/dc_threat_telegraph.wav` |
| `CueTelegraphHeavy` | `dc_threat_telegraph_heavy` | `assets/audio/threat/dc_threat_telegraph_heavy.wav` |
| `CueArrival` | `dc_sting_arrival` | `assets/audio/threat/dc_sting_arrival.wav` |
| `CueEscalate` | `dc_threat_escalate` | `assets/audio/threat/dc_threat_escalate.wav` |
| `CueCleared` | `dc_threat_cleared` | `assets/audio/threat/dc_threat_cleared.wav` |

### Step 1 — put the WAVs where the config will point

```
<game folder>\Data\Sounds\DeepCore\dc_threat_telegraph.wav
<game folder>\Data\Sounds\DeepCore\dc_threat_telegraph_heavy.wav
<game folder>\Data\Sounds\DeepCore\dc_sting_arrival.wav
<game folder>\Data\Sounds\DeepCore\dc_threat_escalate.wav
<game folder>\Data\Sounds\DeepCore\dc_threat_cleared.wav
```

These are new filenames that exist in no WAD, so the loose copies are used with **no command-line
flag at all** (`Files.cpp:886-891`: not in a WAD → `FileSys::Standard`). The 19 shipped assets are
already 22050 Hz / 16-bit / mono, which is mandatory — see `audio-pipeline-truth.md` §2.

### Step 2 — add five lines to the `Samples` block of your `Lego.cfg`

The block is `Lego* { … Samples { … } … }`. It is a **sibling of `Main`**, roughly two-thirds of the
way down the file, right after `TextMessagesWithImages` (vanilla) and before `InfoMessages`.

```
Lego* {

    Samples {

        ; ... the game's own 446 entries, leave every one of them alone ...

        ; ---- DeepCoreOverhaul threat layer ----
        dc_threat_telegraph         *#-600#Sounds\DeepCore\dc_threat_telegraph
        dc_threat_telegraph_heavy   *#-400#Sounds\DeepCore\dc_threat_telegraph_heavy
        dc_sting_arrival            *#-300#Sounds\DeepCore\dc_sting_arrival
        dc_threat_escalate          *#-500#Sounds\DeepCore\dc_threat_escalate
        dc_threat_cleared           *#-600#Sounds\DeepCore\dc_threat_cleared
    }
}
```

Rules, each of which will bite if broken:

- **Add them at the END of the existing block. Do not create a second `Samples` block, and do not
  make your line the first line of the block** (§7.2).
- **No spaces inside a value**, ever — not after a comma, not anywhere (§2.1).
- **No `.wav`** in the path; the loader appends it (`3DSound.cpp:280`).
- **Do not put a `!` on these keys** unless you want them dropped under `-reducesamples`.
- Volume is optional; `#-600#` is about −6 dB. `*` is optional but recommended so two warnings can
  overlap instead of cutting each other off.
- `Sounds\DeepCore\…` is a suggestion; any data-dir-relative path works, `..\` does not
  (`Files.cpp:1205`).

### Step 3 — make the game read *your* `Lego.cfg`

`Lego.cfg` ships **inside a WAD**, and by default the WAD copy wins (`Files.cpp:886-889`,
`Main.cpp:800`). Three routes, in order of preference:

| Route | Command line | Notes |
|---|---|---|
| Loose `<game>\Data\Lego.cfg` | **`-cfgfirst`** | narrowest: `FILE_FLAG_DATAPRIORITY` on the root config only (`GameState.cpp:181-183`, `Main.cpp:1360-1361`, `Main.h:450`). **Recommended.** |
| Loose `<game>\Data\Lego.cfg` | `-datafirst` | global loose-over-WAD (`Main.cpp:1304`, `Files.cpp:878-884`). Broader than needed. |
| Repack the WAD | none | edit the copy inside `LegoRR1.wad`, or ship a higher-numbered `LegoRR2.wad` — WADs are searched backwards (`Wad.cpp:328`, `:422`). |

`-cfgfile <name>` (`Main.cpp:1365-1369`) points at a differently-named root config if you prefer.

### Step 4 — do NOT try to do this with `-cfgadd`

`-cfgadd` (`Main.cpp:1371-1384`) appends whole config files to the tail of the list
(`Config_AppendConfig`, `Config.cpp:834-848`). `Config_FindItem` records a wildcard match and
**keeps searching**, overwriting it, breaking only on a full match (`Config.cpp:787-792`). So:

- an appended `Lego* { Samples { … } }` is a **later** wildcard match and **replaces** the stock
  block entirely — you would lose all 446 game sounds;
- an appended `LegoRR { Samples { … } }` is a **full** match and beats the wildcard — same outcome.

Arrays replace, they never merge. This is the same hazard already recorded for `Stats`. **There is
no additive path through `-cfgadd`.**

### Step 5 — turn the layer on and verify

In `Data\Settings\DeepCore.cfg`: `ThreatAudio TRUE` (`data/Settings/DeepCore.cfg:139`), and
`VerboseStartup TRUE` to get the per-cue resolution line from `DeepCoreAudio.cpp:80`.

The log is the only instrument available:

| Log line | Meaning | Source |
|---|---|---|
| `DeepCore/Audio: cue "…" resolved to SFX id N` | name found — config edit worked | `DeepCoreAudio.cpp:80` |
| `DeepCore/Audio: cue "…" is not declared…` | name **not** in the block — wrong block, wrong depth, or typo | `DeepCoreAudio.cpp:85-87` |
| `Cannot load sound "…"` | name found, **file** missing/unreadable | `3DSound.cpp:344` |
| `Cannot create sound buffer.` | file found but rejected — almost always stereo | `3DSound.cpp:1130` |

The first two distinguish a config-path mistake from a file-path mistake in one run. If you get
"resolved" plus `Cannot load sound`, the block path is right and the WAV path is wrong.

### If you would rather not trust §3 at all — the one-run discriminating experiment

Add the **same** cue name twice, at the two candidate depths, pointing at two **different** WAVs:

```
Lego* {
    Main {
        dc_probe_main     Sounds\DeepCore\probe_a      ; candidate B: Lego*::Main::Samples ... as a Main scalar
        Samples { dc_probe_main2   Sounds\DeepCore\probe_a }
    }
    Samples {
        dc_probe_top      Sounds\DeepCore\probe_b      ; candidate A: Lego*::Samples
    }
}
```

Run once with `VerboseStartup TRUE` and set `CueTelegraph` to `dc_probe_top`, then to
`dc_probe_main2`. Exactly one resolves. Predicted result: **`dc_probe_top` resolves,
`dc_probe_main2` does not.** (Under candidate A the nested `Samples` inside `Main` is at depth 2 and
`Config_FindItem` only matches items at `depth == count-1`, `Config.cpp:758`.)

---

## 6. Registering a cue WITHOUT touching the user's config

**Verdict: viable, low-risk, and it removes the blocker completely. It is the better answer than §5.**

### 6.1 Ownership — all three pieces are ours

| Needed | Owner | Callable from DLL C++? |
|---|---|---|
| `SFX_SetSamplePopulateMode(bool32)` | **OURS**, `SFX.cpp:105`, hooked `interop.cpp:4027` | yes |
| `SFX_GetType(const char*, SFX_ID*)` | **OURS**, `SFX.cpp:112`, hooked `interop.cpp:4028` | yes |
| `SFX_LoadSampleProperty(char*, SFX_ID)` | **OURS**, `SFX.cpp:140`, hooked `interop.cpp:4031` | yes |
| `sfxGlobs` (to roll back a failed registration) | **OURS binding**, `SFX.cpp:21` → `0x00502468` | yes — existing fields only |

Nothing here needs the exe. Note that `SFX_SetSamplePopulateMode` and `SFX_LoadSampleProperty`
currently have **zero callers in this repository** — we would be their first.

### 6.2 The insertion point

`Lego_Initialise` is **OURS** (`GameState.cpp:95`). The call to `Lego_LoadSamples` is one line of
our own source (`GameState.cpp:438`). A registration pass goes on the next line:

```cpp
    Lego_LoadSamples(legoConfig, !Gods98::Graphics_IsReduceSamples());   // :438  unchanged
    DeepCore::Audio::RegisterCues();                                     // :439  NEW, ours
```

Everything the ordering requires is satisfied there:

| Requirement | Satisfied because |
|---|---|
| `SFX_Initialise` has run (table allocated, 45 names present) | `GameState.cpp:308` |
| DirectSound is up — `Sound3D_Load` needs `lpDSnd()` | `Sound_Initialise` → `Sound3D_Initialise` at `Main.cpp:826`, long before `Gods_Go` |
| Populate mode is off and owned by nobody | `Lego_LoadSamples` sets it `FALSE` on exit (`lego.c:3103`) |
| The user's own names are already registered, so we can detect collisions | the parse at `:438` completed |
| Registration precedes every consumer | `Lws_Initialise` `:452`, `Stats_Initialise` `:566`, all level loads |
| Our settings are loaded | `DeepCore::Load()` at `:146` |
| Survives the whole session | `SFX_Initialise` runs once per process (§1.3) |

### 6.3 The shape it has to have

```cpp
// Sketch. Not code to land as-is; the design constraints are what matter.
static bool RegisterOne(const char* cueName, const std::string& value)
{
    using namespace LegoRR;
    if (cueName == nullptr || *cueName == '\0' || value.empty()) return false;

    SFX_ID id = (SFX_ID)0;

    // 1. Populate mode OFF: does the user's own config already declare this name?
    if (SFX_GetType(cueName, &id))
        return true;                       // theirs wins; touch nothing

    // 2. Room? hashNameList is SFX_MAXSAMPLES(495) and SFX.cpp:129 is UNGUARDED.
    if (sfxGlobs.hashNameCount + (uint32)SFX_ID::SFX_Preload_Count >= SFX_MAXSAMPLES)
        return false;                      // warn and skip

    const uint32 before = sfxGlobs.hashNameCount;

    // 3. Populate mode ON for exactly one name.
    SFX_SetSamplePopulateMode(true);
    const bool got = SFX_GetType(cueName, &id);
    SFX_SetSamplePopulateMode(false);
    if (!got) return false;

    // 4. MUTABLE buffer: Util_Tokenise writes NULs into it (Utils.cpp:60).
    char buf[FILE_MAXPATH * 2];
    std::strncpy(buf, value.c_str(), sizeof(buf) - 1); buf[sizeof(buf)-1] = '\0';

    if (!SFX_LoadSampleProperty(buf, id)) {
        // 5. Roll back: our name is the LAST slot written (SFX.cpp:127-132), so this is exact.
        if (sfxGlobs.hashNameCount == before + 1) {
            sfxGlobs.hashNameList[before + (uint32)SFX_ID::SFX_Preload_Count] = 0;
            sfxGlobs.hashNameCount = before;
        }
        return false;                      // warn once, cue stays unregistered
    }
    return true;
}
```

Five constraints, each with a reason:

1. **Check first with populate OFF.** If the user already declared the name, `SFX_LoadSampleProperty`
   would reset `samplePropTable[id].next = nullptr` (`SFX.cpp:147`) and re-bind the handle,
   silently discarding their sound-group. User config must win.
2. **Bound the table ourselves.** `SFX.cpp:129` writes `hashNameList[totalCount]` with **no** bounds
   check, and a stock install leaves only ~35 free slots (§7.1). This is item 1 of
   `audio-pipeline-truth.md` §8.3's ranked plan and it becomes a prerequisite, not a nice-to-have.
3. **Populate mode on for the shortest possible window,** one name at a time, never spanning a call
   into anything else. `SFX_GetType` is the *only* consumer of the flag (`SFX.cpp:126`), so the
   blast radius is exactly the calls we make.
4. **Pass a writable buffer.** `SFX_LoadSampleProperty` takes `char*` and tokenises in place. A
   string literal or `.c_str()` would be undefined behaviour.
5. **Roll back on failure.** Otherwise a missing WAV leaves a registered name with
   `sound3DHandle == 0`, which looks present, is permanently silent, and burns a slot — precisely
   the failure `DeepCoreAudio.cpp:74-76` refuses to create. Rollback is exact because our name is
   always the most recently appended entry.

### 6.4 What we would need from the user instead

Only the WAV files, and a path. Since `DeepCore.cfg` is **our** file, the value string is ours to
define, e.g. alongside the existing cue-name keys:

```
CueTelegraph            dc_threat_telegraph
CueTelegraphFile        *#-600#Sounds\DeepCore\dc_threat_telegraph
```

with a sensible default so a user who simply copies `assets/audio/threat/*.wav` into
`Data\Sounds\DeepCore\` gets sound with **zero** config editing. Their `Lego.cfg` is never opened,
never parsed, never rewritten.

### 6.5 What could break, honestly

| Risk | Severity | Mitigation |
|---|---|---|
| `hashNameList` overflow at `SFX.cpp:129` | **high** — 4-byte heap overflow | the guard in step 2; land the unconditional guard in `SFX_GetType` first |
| Stomping a user cue of the same name | high | step 1 |
| Registered-but-silent name after a missing WAV | medium | step 5 rollback |
| `soundTable` exhaustion → `Error_Fatal(true, "Run out of samples…")`, `3DSound.cpp:365` | low — 120 free in stock data, we need 5 | count before loading; skip if `< n` free |
| Hash collision with an existing name | low | step 1 detects it as "already present" and we skip — silent but safe |
| A user *wants* to override our cue from `Lego.cfg` | none | step 1 makes their declaration win automatically |
| Our sounds occupy sound-table slot 0 | none | vanilla's 446 entries already claimed it (§7.2) |
| `-reducesamples` should drop our cues too | cosmetic | mirror the `!` semantics: skip when `Graphics_IsReduceSamples()` |
| No trampoline / cannot call the original | none | we add a call, we replace nothing (`docs/HOOK-ARCHITECTURE.md`, handoff §2) |
| CARDINAL address rule | none | `sfxGlobs` fields are written, never grown; no new overlaid state |

The honest residual: **this cannot be verified without running the game.** But it is verifiable
*more cheaply* than the config route, because a failure prints a specific line we already own, and
because it removes the user's ability to get it subtly wrong.

### 6.6 Why this is better than §5

The config route asks a user to hand-edit a 6800-line file that lives inside a WAD, at a depth they
must get exactly right, with a value grammar that has no whitespace tolerance and whose single most
likely mistake (a space after a comma) silently mis-binds *every subsequent entry in the block*. The
code route asks them to copy five files into a folder. Both remain available; §5 stays the documented
escape hatch and the override mechanism.

---

## 7. Corrections this audit forces on `audio-pipeline-truth.md`

Listed because that document is cited as ground truth elsewhere in the tree.

### 7.1 "450 free" name slots is wrong — it is ~35

`audio-pipeline-truth.md` §4.3 reports `hashNameList` as 495 slots, 45 used at boot, **450 free**.
That is true at the instant `SFX_Initialise` returns and false forever after. Measured against a real
`Samples` block (§4.2): 446 entries, 31 of which match a preload name, so **415 new names** are
registered, giving `45 + 415 = 460` of 495 used and **35 free**. The overhaul mod measures 455 used /
40 free. Adding the five DeepCore cues fits — with a margin of thirty, not four hundred and fifty.

`sampleGroupTable` (200) and `soundTable` (600) are genuinely comfortable: 34 and 480 used.

### 7.2 Sound-table index 0 — resolved, and it is not a problem

§7.4 item 2 and §8.4 leave open whether stock data parks something harmless at sample-table index 0
(which is unreachable through every `SFX_*` play path because of `if (handle > 0)`, `SFX.cpp:350`).
**It does.** The first line of the vanilla block is:

```
        SFX_Bodge           Sounds\Minifigure\screw1
                    ; ^^^ THIS LINE IS AN UTTER BODGE
                    ; TO STOP DRILL SOUND WHEN CLAPPING
```

— a deliberate throwaway, with the shipping comment saying so. The advice "never make your new cue
the first line of the block" stands, but for a stock install it is already impossible to hit.

### 7.3 The `!` key prefix was entirely missing

242 of 446 vanilla entries carry it. Any tool that reads or rewrites a `Samples` block and does not
know that `!` is part of the syntax rather than part of the name will corrupt the file.

### 7.4 "The SFX table is rebuilt per level" is wrong

`DeepCoreAudio.hpp` (header comment) and `DeepCoreAudio.cpp:52` justify `InvalidateCueCache` on that
basis. `SFX_Initialise` has exactly one call site (`GameState.cpp:308`, inside `Lego_Initialise`) and
`Lego_LoadSamples` exactly one (`:438`). Both run once per process. The cache invalidation is
harmless — it just re-resolves — but the stated reason is not real. (NERPs *does* tear down its own
streamed sounds per level, `NERPsFile.cpp:312-321`, but those never enter `sfxGlobs`.)

### 7.5 The whitespace gotcha is worse than described

§4.4 gotcha 1 says `a, b` yields a token `" b"` that becomes `" b.wav"` and fails. From a config file
it cannot get that far: `Config_Load2` turns the space into `'\0'` first (`Config.cpp:125`), so `b`
becomes the **next item name** and the whole remainder of the block shifts by one entry (§2.1). The
failure is a cascade, not one dead sample.

---

## 8. DECISION

**SETTLED.** The `Samples` block is at **`<gameName>::Samples`** — written `Lego* { Samples { … } }`
in the file, looked up as `LegoRR::Samples`. It is a **direct child of the root block and a sibling
of `Main`**, at depth 1. The repository's prior inference `Lego*::Main::Samples` is **refuted**: no
config *block* anywhere in this game lives under `Main`, all 79 in-tree `Main_ID` uses are scalars,
and every depth-1 block loader we own uses `Config_ID(gameName, "<Block>")`.

This was **not** settleable from this repository alone — `Lego_LoadSamples` is still an exe address
macro at `0x00434980` (`Game.h:1914`) and the literal `"Samples"` appears in `src/` only in prose. It
is settled by external evidence: a decompilation of that exact function showing
`Config_BuildStringID(legoGlobs.gameName, "Samples", 0)`, two published `Lego.cfg` files that both
place `Samples {` at depth 1, and two RRU pages that agree. Four sources, no disagreement.

**The value grammar is fully determined from our own code** and is
`[!]<name> <WS> [*][#millibels#][@]<path>[,<more>…]` — with `!` on the key (reduce-samples marker,
newly documented here), no whitespace anywhere in the value, and no `.wav` extension.

**Recommended action, in order:**

1. **Land the `hashNameList` bound at `SFX.cpp:129`** before anything else registers a name. A stock
   install has ~35 free slots, not 450, and the write is unguarded. This was already item 1 of the
   ranked plan; §7.1 upgrades it from theoretical to near-term.
2. **Implement `DeepCore::Audio::RegisterCues()` at `GameState.cpp:439`** to the design in §6.
   All three required functions are OURS and hooked, the insertion point is our own source, the
   ordering constraints are all satisfied, user declarations keep priority, and failure rolls back.
   This removes the blocker instead of documenting it, and it is the recommendation.
3. **Keep §5 as the documented manual route** — it is the override mechanism and the escape hatch,
   and it is now correct rather than "believed, UNDETERMINED".
4. **Amend `audio-pipeline-truth.md`**: delete the UNDETERMINED markers in §4.4, §8.2 item 2 and
   §8.4 bullet 1; fix "450 free" (§7.1 here); resolve §8.4 bullet 2 (§7.2 here); add the `!` prefix
   to the grammar table; correct the whitespace gotcha. Amend `DeepCoreAudio.hpp`'s "the literal
   config path … is UNDETERMINED" note and its "rebuilt per level" claim.

**Still UNDETERMINED, and unchanged by this audit:** everything audible. Nothing here has been run.
The `!` semantics, the depth-1 path and the 35-slot headroom are all read from code and data, not
observed. Confidence that the path is right: very high. Confidence that a user following §5 will
hear a sound: unverifiable from this machine.
