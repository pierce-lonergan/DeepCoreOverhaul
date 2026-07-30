# The Juice Layer — dynamic threat audio and cheap high-impact effects

**Status: research only. Nothing in this document has been built, and nothing in this
project has ever been play-tested** — there is no installation of the original game on the
authoring machine (`docs/HANDOFF-2026-07-30.md:3-6`). Every claim below was derived by
reading source. Where a claim could only be settled by running the game it is marked
**UNDETERMINED**.

Two prior claims were put to me for verification. Both are **substantially right and both
are wrong in the details**:

| Claim | Verdict |
| --- | --- |
| "`game/effects` is 57/57 implemented with zero live address macros" | **Zero live address macros: CONFIRMED.** The count is wrong: it is **69/69**, not 57/57. 57 = Effects (27) + LightEffects (21) + DamageText (9). `Smoke` — 12 more functions, all ours — was omitted entirely. See §1. |
| "SFX cue names are hash-registered dynamically with ~450 of 495 slots free" | **CONFIRMED, and the number is exactly 450.** `SFX_MAXSAMPLES` 495 (`SFX.h:34`) − `SFX_Preload_Count` 45 (`GameCommon.h:1421`) = 450. But the bound is **not enforced**: name 451 is a 4-byte heap overflow (`SFX.cpp:129`). See §2.5 — this is a *new* finding, not in `silent-failures.md`. |

The headline for the actual task: **a complete dynamic-threat-audio layer is buildable with
no new engine code, no new hook, and no exe function touched** — because the SFX name table
is string-keyed at runtime and the play/volume/stop primitives are all already ours. The
cost is entirely in DLL-side C++ we write and in WAV files the *user* supplies.

---

## 1. Ownership audit — verified by counting

Method: for each header, count declarations of the form `<type> __cdecl <Name>(`; count live
address macros (`#define X ((...)0xADDR)` at column 0) versus commented ones; count matching
definitions in the `.cpp`; count `hook_write_jmpret` installations in `interop.cpp`.

### 1.1 `src/openlrr/game/effects/` — 69 exe-derived functions, 69 implemented, 0 live macros

| File | exe-derived fns | implemented | live addr macros | commented macros | hooks live | hooks commented |
| --- | --- | --- | --- | --- | --- | --- |
| `Effects.{h,cpp}` | 27 | **27** | **0** | 27 (`Effects.h:176`–`:288`) | 27 | 0 |
| `Smoke.{h,cpp}` | 12 | **12** | **0** | 0 — *the macros were deleted outright, not commented* | 8 | 4 |
| `LightEffects.{h,cpp}` | 21 | **21** | **0** | 21 (`LightEffects.h:123`–`:214`) | 18 | 3 |
| `DamageText.{h,cpp}` | 9 | **9** | **0** | 9 (`DamageText.h:109`–`:141`) | 9 | 0 |
| **total** | **69** | **69** | **0** | 57 | **61** | **8** |

`LightEffects` additionally carries **8 non-exe custom functions** (`LightEffects.cpp:62`
`_Restart`, `:103` `_ResetSpotlightPosition`, `:624`–`:654` the four `Is*Enabled` /
`Set*Enabled` pairs) plus two inline helpers (`LightEffects.h:220,223`). Those are pure
OpenLRR additions with no exe address at all, and they are the ones that make runtime
toggling of the light effects possible without touching `lightGlobs.flags` by hand.

**The 8 commented hooks matter and are easy to misread.** A commented hook means *our C++
body exists and compiles, but the exe's original prologue was never overwritten*. So:

- **our** code calling `LegoRR::Smoke_Remove(...)` runs **our** implementation;
- **exe** code calling `0x00465c70` still runs the **1999** implementation.

Both operate on the same `smokeGlobs` at `0x00553100` (`Smoke.cpp:25`), so they interoperate,
but they are not the same code. The eight:

```
interop.cpp:2956  LightEffects_ResetSpotlightColour   0x0044ca20
interop.cpp:2982  LightEffects_FlipSign               0x0044d510
interop.cpp:2990  LightEffects_CheckMoveLimit         0x0044dc60
interop.cpp:4072  Smoke_Remove                        0x00465c70
interop.cpp:4084  Smoke_Group_Show                    0x00465f10
interop.cpp:4085  Smoke_Group_Update                  0x004660c0
interop.cpp:4086  Smoke_MeshRenderCallback            0x004661a0
interop.cpp:4087  Smoke_Group_MeshRenderCallback      0x00466200
```

Note especially `Smoke_MeshRenderCallback`: our `Smoke_CreateSmokeArea` (which **is** hooked,
`interop.cpp:4066`) installs **our** callback pointer via `Mesh_Create` (`Smoke.cpp:144`), so
any smoke *we* create renders through our C++ regardless. Nothing needs changing here.

### 1.2 `src/openlrr/game/audio/` — 26/26, zero live macros

`SFX.h` declares 26 functions (24 with exe addresses, plus the custom
`SFX_Current_GetSound3DHandle` at `SFX.h:180` / `SFX.cpp:282`, plus two `__inline` parameter
wrappers at `SFX.h:160` and `:222` used as Lws callbacks). `SFX.cpp` defines all 26.
`interop.cpp:4024`–`:4052` installs **25 hooks, none commented**. **Zero address macros in
the header.** This module is fully ours.

### 1.3 `src/openlrr/engine/audio/` — fully implemented, mostly hooked

| File | declared | defined | live addr macros | hooks live | hooks commented |
| --- | --- | --- | --- | --- | --- |
| `Sound.{h,cpp}` | 5 | 5 (+11 file-static helpers) | **0** | 5 | 0 |
| `3DSound.{h,cpp}` | 44 | 46 | **0** | 19 | 23 |

The 23 commented `Sound3D_*` hooks are all internal helpers (`Sound3D_LoadSample`,
`Sound3D_CreateSoundBuffer`, `Sound3D_AddSoundRecord`, the stream internals…). Every entry
point the game layer actually calls — `Load`, `Play2`, `StopSound`, `SetBufferVolume`,
`GetBufferVolume`, `GetSamplePlayTime`, `Update` — is hooked live.

### 1.4 The contrast that shapes the whole design: `InfoMessages` is 0/38

`src/openlrr/game/interface/InfoMessages.cpp` is 138 lines and contains **exactly one
statement**: the `infoGlobs` binding at `:14`. All 38 function bodies are commented-out
signatures (`:24`–`:136`), and `InfoMessages.h` carries **38 live address macros, 0
commented**. `Info_Send` is `InfoMessages.h:267`, an exe macro at `0x00419ab0`.

So the telegraph channel the wave director leans on hardest is the one channel we do not
own. That is fine — we only *call* it — but it means:

- we cannot change what `Info_Send` does, only when we call it;
- `Info_SetTypeSFX` (`InfoMessages.h:198`, exe macro) is called from
  `Lego_LoadInfoMessages` (`Game.h:1922`, exe macro, invoked at `GameState.cpp:797`), so the
  per-`Info_Type` sound is a **user config value we can neither read nor validate**;
- if a user's `Lego.cfg` never configured a sound for `Info_RockMonster`, the panel alert is
  silent and there is no way for us to detect it in advance. **UNDETERMINED** by inspection.

That last point is the single strongest argument for the layer proposed in §5: our own
threat audio does not depend on the user having configured `Info_*` sounds at all.

---

## 2. The SFX system, end to end

### 2.1 Registration: 45 hardcoded names, then everything else by hash

`SFX_Initialise` (**OURS**, `SFX.cpp:32`, hooked `interop.cpp:4024`) zeroes `sfxGlobs`,
heap-allocates the name table, and registers the 45 built-in names:

```cpp
uint32 arraySize = SFX_MAXSAMPLES * sizeof(uint32);          // 495 * 4 = 1980 bytes
sfxGlobs.hashNameList = (uint32*)Gods98::Mem_Alloc(arraySize);
std::memset(sfxGlobs.hashNameList, 0, arraySize);

SFX_RegisterName(SFX_NULL);      // SFX.cpp:45
...
SFX_RegisterName(SFX_AmbientMusicLoop);  // SFX.cpp:89  -- the 45th
```

`SFX_RegisterName` is a stringising macro (`SFX.h:136`):

```cpp
#define SFX_RegisterName(n)  (sfxGlobs.hashNameList[n] = Gods98::Util_HashString(#n, false, true))
```

So `hashNameList[i]` for `i < 45` holds the uppercased hash of the *enum identifier text*
(`"SFX_Drill"`, etc.), and the array index **is** the `SFX_ID` (`GameCommon.h:1369-1422`).
`Util_HashString` (`Utils.cpp:180`) is a positional multiply-accumulate over
`UTIL_LARGENUMBER = 6293815` (`Utils.h:22`), upper-cased — i.e. **cue names are
case-insensitive**.

### 2.2 Lookup and dynamic registration — `SFX_GetType`

`SFX_GetType` (**OURS**, `SFX.cpp:112`) is both the lookup *and* the registrar:

```cpp
uint32 hashValue  = Gods98::Util_HashString(sfxName, false, true);
uint32 totalCount = sfxGlobs.hashNameCount + (uint32)SFX_ID::SFX_Preload_Count;  // + 45

for (uint32 i = 0; i < totalCount; i++) {
    if (hashValue == sfxGlobs.hashNameList[i]) { *sfxID = (SFX_ID)i; return true; }
}

if (sfxGlobs.flags & SFX_GlobFlags::SFX_GLOB_FLAG_POPULATEMODE) {
    *sfxID = (SFX_ID)totalCount;
    sfxGlobs.hashNameList[totalCount] = hashValue;   // <-- SFX.cpp:129, unbounded
    sfxGlobs.hashNameCount++;
    return true;
}
return false;
```

Two modes, and the mode is the whole story:

- **`POPULATEMODE` on** (set only by `SFX_SetSamplePopulateMode`, **OURS**, `SFX.cpp:105`):
  an unknown name is *created*, gets the next free ID, and returns `true`. This is the
  registration path.
- **`POPULATEMODE` off**: an unknown name returns `false` and writes nothing. This is the
  lookup path, and it is the one every gameplay call site uses.

`POPULATEMODE` is toggled by `Lego_LoadSamples` — **an exe address macro**
(`Game.h:1914`, `0x00434980`), called once from `GameState.cpp:438`:

```cpp
Lego_LoadSamples(legoConfig, !Gods98::Graphics_IsReduceSamples());
```

We do not own the `Samples` block parser. We own everything it calls into
(`SFX_SetSamplePopulateMode`, `SFX_GetType`, `SFX_LoadSampleProperty`), which is enough to
reason about it completely but not enough to change its syntax.

### 2.3 Sample values — `SFX_LoadSampleProperty` fully documents the value grammar

**OURS**, `SFX.cpp:140`, hooked `interop.cpp:4031`. The config *value* for a cue is a
comma-separated list, each element optionally prefixed:

| Prefix | Meaning | Source |
| --- | --- | --- |
| `*` | *simultaneous* — the sample gets `SOUND3D_MAXSIMULTANEOUS` (3) duplicate buffers so it can overlap itself | `SFX.cpp:159`, `3DSound.cpp:540-547`, `3DSound.h:56` |
| `#N#` | volume in DirectSound millibels, range `[-10000, 0]` | `SFX.cpp:164-175`, `3DSound.cpp:330` |
| `@` | *stream* from disk rather than loading into memory | `SFX.cpp:177`, `3DSound.cpp:282-315` |
| (none) | resident, single-buffer | |

More than one element makes it a **sound group**: a random member is chosen on each play
(`SFX_Random_GetSound3DHandle`, `SFX.cpp:249`). Group storage comes out of
`sampleGroupTable[200]` (`SFX.h:97`), now bounds-guarded at `SFX.cpp:223-229` — the fix
landed on 2026-07-30 and the guard reads:

```cpp
if (sfxGlobs.sampleGroupCount >= SFX_MAXSAMPLEGROUPS) {
    Error_WarnF2(true, "SFX: sample group table is full (%i entries); extra "
                       "grouped samples are being ignored.\n", (sint32)SFX_MAXSAMPLEGROUPS);
    success = false;
    break;
}
```

Note the accounting: **one group slot per grouped sample beyond the first, across the entire
config**, never reset. 200 total for the whole game.

Each element resolves through `Gods98::Sound3D_Load(name, stream, simultaneous, volume)`
(`3DSound.cpp:265`), which appends `".wav"` (`3DSound.cpp:280`) and loads through
`File_LoadBinary` → `File_Load2` → `File_Open2` (`Files.cpp:1023-1094`), i.e. the standard
Gods98 file layer where **loose files override the WADs** (`HANDOFF-2026-07-30.md:171-172`).

### 2.4 Can new named cues be added with ZERO C++? — Yes for *definition*, no for *most triggers*

**Defining** a cue is pure data:

```
Lego* {
    Samples {
        ; <CueName>   <wav path, no extension>[,<more>...]
        SND_Tension1        *#-600#Sounds\DeepCore\tension1
        SND_MonsterNear     *Sounds\DeepCore\near_a,*Sounds\DeepCore\near_b
        SND_AllClear        Sounds\DeepCore\allclear
    }
}
```

with `Sounds\DeepCore\tension1.wav` etc. dropped loose in the game directory. No code.
The exact block name and nesting is **UNDETERMINED from this repo** — `Lego_LoadSamples` is
exe code and the project has no `Lego.cfg` to read (`HANDOFF-2026-07-30.md:141-142`). What
*is* certain from source is the value grammar (§2.3), the `.wav` extension, the file search
path, and case-insensitivity.

**Triggering** a cue with zero C++ is possible only through the config surfaces the engine
already reads. Every one of these calls `SFX_GetType(<name-from-config>)`:

| Trigger surface | Call site | Owner |
| --- | --- | --- |
| LWS animation null node (see §3) | `Lws.cpp:356`, via `SFX_Callback_FindSFXIDFunc` | **OURS** |
| Per-object `DrillSound` / `DrillFadeSound` / `EngineSound` | `Stats.cpp:280,283,287` | **OURS** |
| Priority-button sound | `Priorities.cpp:61` | **OURS** |
| Objective screen sound | `Objective.cpp:1006` | **OURS** |
| Advisor | `Advisor.cpp:209` | **OURS** |
| Text-message panel sound | `TextMessages.cpp:207` → `SFX_Random_SetAndPlayGlobalSample` | **OURS** |
| Front-end menu sounds | `FrontEnd.cpp:2855, 3864, 3874` | **OURS** |
| `Info_*` panel-message sound | `Info_SetTypeSFX`, `InfoMessages.h:198` | **EXE macro** |

Two engine-internal lookups also go by *name*, not by ID, and are therefore already
"modder-visible" hooks: `"SND_AirBeat"` (`GameState.cpp:1715,1725,1729`) and `"SND_Hurt"`
(`Object.cpp:4049`). These are the existence proof that config-only names work end to end.

**Conclusion: new *cues* need zero C++; new *triggers* need C++ unless they map onto an
animation frame or one of the config surfaces above.** The threat layer in §5 is exactly the
missing trigger machinery.

### 2.5 NEW FINDING — `SFX_GetType` overflows `hashNameList` at name 451

`sfxGlobs.hashNameList` is a heap block of `SFX_MAXSAMPLES * sizeof(uint32)` = 495 `uint32`s
(`SFX.cpp:39-40`). `SFX_GetType` writes `hashNameList[totalCount]` where
`totalCount = hashNameCount + 45`, **with no bound of any kind** (`SFX.cpp:129`).

- Valid indices: `0 … 494`.
- Preloads occupy `0 … 44`.
- Config names occupy `45 … 494` → **exactly 450 free slots**.
- The **451st** distinct config cue name writes `hashNameList[495]` — 4 bytes past the end of
  a `Mem_Alloc` block.

This is the same defect class as the `sampleGroupTable` overflow fixed at `SFX.cpp:223`
(worklog `docs/WORKLOG.md:138-144`), and it is **not listed in `docs/research/silent-failures.md`**
— F-04 covers `sampleGroupTable`, and the tabulation at `silent-failures.md:367` names
`hashNameList` only as the *victim* of that overflow, never as an overflow site itself.

It is less catastrophic than F-04 (heap, not the exe data segment, so it cannot land on
`statsGlobs`) but it is more likely to be *reached* by exactly the audience this document is
for: someone shipping a large sound pack. The guard is four lines and costs nothing:

```cpp
// SFX.cpp, inside SFX_GetType, replacing the POPULATEMODE branch body
if (sfxGlobs.flags & SFX_GlobFlags::SFX_GLOB_FLAG_POPULATEMODE) {

    /// DEEPCORE: hashNameList is a Mem_Alloc block of exactly SFX_MAXSAMPLES (495)
    /// uint32s (SFX.cpp:39-40, SFX.h:34). totalCount is hashNameCount + 45 preloaded
    /// names (GameCommon.h:1421) and was never checked against it, so the 451st
    /// distinct sample name in the config writes 4 bytes past the end of the heap
    /// block. Unconditional, like the other overflow guards -- the behaviour being
    /// replaced is memory corruption, so there is no vanilla semantics to preserve.
    if (totalCount >= SFX_MAXSAMPLES) {
        Error_WarnF2(true, "SFX: sample name table is full (%i names); \"%s\" and any "
                           "later sample names are being ignored.\n",
                     (sint32)SFX_MAXSAMPLES, sfxName);
        return false;
    }

    *sfxID = (SFX_ID)totalCount;
    sfxGlobs.hashNameList[totalCount] = hashValue;
    sfxGlobs.hashNameCount++;
    return true;
}
```

Returning `false` is the correct failure mode: it is exactly what an unknown name already
returns outside populate mode, and every caller in the tree already handles `false`
(`Stats.cpp:280-289` falls back to a default `SFX_ID`; `TextMessages.cpp:207` and
`Objective.cpp:1006` skip the sound).

**A second, harder ceiling sits behind it.** `Sound3D_Load` draws from
`SOUND3D_MAXSAMPLES = 600` buffer slots (`3DSound.h:54`), and `Sound3D_GetFreeSoundIndex`
calls `Error_Fatal(true, "Run out of samples - SOUND3D_MAXSAMPLES too small.")`
(`3DSound.cpp:365`) — **process termination**. Since a grouped cue consumes one slot *per
member*, a sound pack can hit 600 WAVs long before it hits 450 names. That fatal is exe-side
`Error_Fatal` in **our** C++, so it is fixable, but it is out of scope here; note it as a
known cliff.

### 2.6 The playback primitives — what each one actually is

All **OURS**, all hooked live.

| Function | Where | Semantics |
| --- | --- | --- |
| `SFX_Random_PlaySoundNormal(SFX_ID, bool32 loop)` | `SFX.cpp:343` | 2D, no attenuation. Returns a **play UID**, or `-1`. |
| `SFX_Random_PlaySound3DOnFrame(frame, id, loop, onFrame, wPos)` | `SFX.cpp:424` | 3D. With `onFrame=false` and a `wPos`, plays at a fixed world point — no container needed. |
| `SFX_Random_PlaySound3DOnContainer(cont, id, loop, onCont, wPos)` | `SFX.cpp:413` | Thin wrapper over the above. |
| `SFX_Random_SetAndPlayGlobalSample(id, OUT handle)` | `SFX.cpp:314` | **One-at-a-time stinger.** Refuses while `globalSampleDuration > 0`, and sets that duration from the sample's own play time. This is the built-in anti-spam channel; `TextMessages.cpp:207` uses it. |
| `SFX_StopGlobalSample()` | `SFX.cpp:293` | Cancels the above. |
| `SFX_Random_SetBufferVolume(id, sint32 mB)` | `SFX.cpp:386` | Sets the **sample's** volume, live, in millibels `[-10000, 0]`. Affects every voice of that sample. |
| `SFX_Random_GetBufferVolume(id)` | `SFX.cpp:397` | Reads it back; returns `Sound3D_MinVolume()` if the handle is bad. |
| `SFX_Random_GetSamplePlayTime(id)` | `SFX.cpp:473` | Seconds. |
| `SFX_Sound3D_StopSound(sint32 playUID)` | `SFX.cpp:483` | Stops **one voice**. |
| `SFX_IsSoundOn()` / `SFX_SetSoundOn` | `SFX.cpp:596` / `:533` | Master gate; everything else already honours it. |

**The unit trap, and a live bug.** `Sound3D_Play2` returns
`soundTableIndex * SOUND3D_MAXSIMULTANEOUS + voice` (`3DSound.cpp:601`) — a **play UID**.
`Sound3D_StopSound` divides by 3 to recover the buffer (`3DSound.cpp:677-687`). But
`SFX_Current_GetSound3DHandle` returns `samplePropTable[sfxID].sound3DHandle`
(`SFX.cpp:286`), which is the **table index** returned by `Sound3D_Load` (`3DSound.cpp:338`)
— a different unit.

`GameState.cpp:1717` mixes them:

```cpp
SFX_Sound3D_StopSound(SFX_Current_GetSound3DHandle(airBeatSFX));   // GameState.cpp:1717
```

A table index `H` is interpreted as a play UID, so this stops sample `H / 3`, voice `H % 3` —
**a different sample entirely** unless `H < 3`. The low-oxygen heartbeat is therefore not
reliably stopped, and an unrelated looping sound may be. `Smoke.cpp:334` and `Lws.cpp:550`
both do it correctly, passing a value that came from a play call.

Whether the audible symptom is noticeable is **UNDETERMINED** (it depends on which sample
lands at index `H/3` in the user's config). The fix is to store the value
`SFX_Random_PlaySoundNormal` returned. **The threat layer in §5 must not copy this idiom**,
and it is the reason the design below keeps its own `sint32 playUID` for every loop it owns.

**Second trap: `SFX_GetType` is not free.** It hashes the string and linear-scans up to 495
`uint32`s on every call. `GameState.cpp:1715,1725,1729` calls it three times per frame for
the same literal. Resolve once, cache the `SFX_ID`, invalidate on level load.

**Third trap: a non-`*` sample has exactly one buffer.** Starting a looping non-simultaneous
sample twice does not layer, it restarts. And a `*` sample has three
(`3DSound.cpp:541-543`): a fourth overlapping play silently steals voice 0, which may be the
loop you were relying on.

**Fourth trap: only one looping stream exists.** `Sound3D_Stream_Play` selects between
`loopStreamData` and `streamData` (`3DSound.cpp:1228-1235`) — one each, globally. So an `@`
streamed looping tension bed collides with any other streamed loop, including NERPs speech
when `StreamNERPSSpeach` is on (`GameState.cpp:400-402`). **Use resident samples for
anything the juice layer loops.**

**Music is CD audio and is not a layerable channel.** `Lego_SetMusicPlaying`
(`Game.cpp:2919-2938`) picks a random redbook track and calls `Sound_PlayCDTrack`
(`Sound.cpp:105`); the only controls are start and stop (`Lego_ChangeMusicPlaying`,
`Game.cpp:528`). There is no volume, no crossfade, no stem. **Adaptive *music* is not
available. Adaptive *sound* is.** Any "music" the juice layer wants must be a looping
resident sample it owns.

---

## 3. The LWS animation sound-trigger node — verified, and it is the best zero-code channel

**It exists exactly as described.** All of `Lws.cpp` is **OURS** (no address macros in the
file).

**Detection**, during scene parse (`Lws.cpp:102-110`):

```cpp
if (std::strcmp("AddNullObject", argv[0]) == 0) {
    currNode->flags |= Lws_NodeFlags::LWSNODE_FLAG_NULL;
    currNode->name = Util_StrCpy(&line[std::strlen("AddNullObject") + 1]);
    if (::_strnicmp(currNode->name, LWS_SOUNDTRIGGERPREFIX, ...) == 0 &&
        ::_strnicmp(&currNode->name[...], LWS_SOUNDTRIGGERSEPERATOR, ...) == 0) {
        currNode->flags |= Lws_NodeFlags::LWSNODE_FLAG_SOUNDTRIGGER;
        Error_Fatal(scene->triggerCount == 256, "Too many sound trigger frames");
        scene->triggerCount++;
    }
}
```

`LWS_SOUNDTRIGGERPREFIX` is `"SFX"` and `LWS_SOUNDTRIGGERSEPERATOR` is `","`
(`Lws.h:59-60`). The match is **case-insensitive** (`_strnicmp`) and anchored at position 0.

**Parse**, in `Lws_SetupSoundTriggers` (`Lws.cpp:329-385`):

```cpp
uint32 argc = Util_Tokenise(line, argv, LWS_SOUNDTRIGGERSEPERATOR);
st->count = (uint16)argc - 2;                                    // Lws.cpp:351
Error_Fatal(st->count >= LWS_MAXTRIGGERKEYS, "LWS_MAXTRIGGERKEYS too small");
Error_Fatal(st->count == 0, "No trigger frames specified");
bool32 result = lwsGlobs.FindSFXIDFunc(argv[1], &st->sfxID);      // Lws.cpp:356
...
st->frameStartList[index] = std::atoi(argv[index+2]);
if (end = strstr(argv[index+2], "-")) st->frameEndList[index] = std::atoi(&end[1]);
else                                  st->frameEndList[index] = st->frameStartList[index];
```

`FindSFXIDFunc` is wired to `SFX_Callback_FindSFXIDFunc` → `SFX_GetType`
(`GameState.cpp:452`, `SFX.h:160`).

**Fire**, in `Lws_HandleTrigger` (`Lws.cpp:536-565`): a key whose start == end is a one-shot;
start != end makes `loopMode` true, the sound loops from the start frame and is stopped at
the end frame. It plays via `PlaySample3DFunc` → `SFX_Random_PlaySound3DOnFrame` with
`onFrame = true`, i.e. **attached to that null's frame in the animation**, so it moves and
attenuates with the object.

### How a modder uses it

In the `.lws` scene file, add a null object whose *name* is the whole instruction:

```
AddNullObject SFX,SND_MonsterRoar,12
AddNullObject SFX,SND_DrillLoop,4-58
AddNullObject SFX,SND_Footfall,6,18,30,42
```

- field 0 — literal `SFX`
- field 1 — the cue name, resolved through `SFX_GetType`
- fields 2… — frame numbers; `A-B` means "loop from A, stop at B"

Up to **24 keys** per node (`LWS_MAXTRIGGERKEYS` 25 minus the fatal's `>=`, `Lws.h:67`) and
up to **255** trigger nodes per scene (`Lws.cpp:108`).

### The trap a modder will hit, and it kills the process

At `Lws.cpp:358-365`:

```cpp
if (Graphics_IsReduceSamples()) {
    Error_WarnF(!result, "Cannot match sound with %s", argv[1]);
    if (!result) st->sfxID = 0;
} else {
    Error_FatalF(!result, "Cannot match sound with %s", argv[1]);
}
```

`SFX_GetType` is called here **outside `POPULATEMODE`** (samples were loaded long before, at
`GameState.cpp:438`, and `Lws_Initialise` runs at `:452`). So a cue name that is not in the
`Samples` block — a typo, a missing entry, a name only present in a WAD the user replaced —
is a **fatal error at model-load time**, unless the user happens to have reduced samples
enabled. This is the loudest failure mode in the whole audio system and it should be the
first line of any modding note we ship.

**Recommendation:** the LWS route is excellent for *object-anchored* sound (a monster's roar,
a drill loop, a footfall) and is genuinely zero-code. It is **useless for threat state**,
because it is driven by animation frames, not by game state. That is precisely the gap §5
fills.

---

## 4. Every spawnable effect, with per-frame safety

All **OURS**. "Safe per-frame" means: calling it every frame from an update loop cannot leak,
cannot corrupt, and degrades gracefully.

| Entry point | Signature | Cap | Safe per-frame? |
| --- | --- | --- | --- |
| `Effect_Spawn_Explosion` | `(OPTIONAL LegoObject*, OPTIONAL const Point2F* wPos)` — `Effects.h:261`, impl `Effects.cpp:435` | 4 (`EFFECT_EXPLOSION_MAXCONTAINERS`) | **Yes with a guard.** Silently drops past 4 (`Effects.cpp:448`). **But it does not null-check `effectGlobs.explosionCont`** and `Container_Clone` dereferences its argument immediately (`Containers.cpp:969`; `Container_DebugCheckOK` is a no-op, `Containers.h:237`). Check `effectGlobs.explosionCont != nullptr` first. Note the parameter is `Point2F` — z is derived from the map at `Effects.cpp:445`. |
| `Effect_Spawn_BoulderExplode` | `(const Vector3F* wPos)` — `Effects.h:187`, impl `Effects.cpp:63` | 4 | **Yes with a guard.** Reads `Lego_GetLevel()->BoulderAnimation` (`Effects.cpp:69`) and clones `legoGlobs.contBoulderExplode` / `…Ice` — both can be null (`Game.cpp:827,831`). Guard both. |
| `Effect_Spawn_BoulderExplode_AtSimpleObject` | `(LegoObject*)` — `Effects.h:183`, impl `Effects.cpp:50` | 4 | Yes; null-checks the container itself (`Effects.cpp:55`). Header warns it is **only** valid for non-activity object types (`Effects.h:179-180`). |
| `Effect_Spawn_SmashPath` | `(OPTIONAL LegoObject*, OPTIONAL const Vector3F*)` — `Effects.h:191`, impl `Effects.cpp:87` | 4 | **Yes with a guard** on `legoGlobs.contSmashPath` (`Game.cpp:836`). |
| `Effect_Spawn_Particle` | `(MiscEffectType, const Vector3F* wPos, OPTIONAL const Vector3F* dir)` — `Effects.h:289`, impl `Effects.cpp:606` | 10 per type | **Yes, unconditionally.** The only spawn that null-checks everything (`Effects.cpp:611`) and returns `false` when full. **This is the safest visual primitive in the codebase.** Ten types, all user-configured under `MiscObjects` (`Effects.cpp:527-555`). |
| `Effect_Spawn_RockFall` | `(RockFallType, sint32 bx, sint32 by, real32 x,y,z, real32 xDir,yDir)` — `Effects.h:215`, impl `Effects.cpp:224` | 4 per type | Yes. Returns `false` when full. Only picks slots whose `finishedTable[i]` is `true`, which is only ever set by a successful load (`Effects.cpp:194`), so an unloaded style is inert rather than crashing. |
| `Effect_Spawn_ElectricFenceBeam` | `(bool32 longBeam, x,y,z, xDir,yDir,zDir)` — `Effects.h:219`, impl `Effects.cpp:246` | 30 per type | Yes, same pattern. |
| `Smoke_CreateSmokeArea` | `(OPTIONAL Container* attach, uint32 bx, uint32 by, const Vector3F* dir, r,g,b, thickness, animSpeed, Smoke_Type, uint32 randomness, SFX_ID)` — `Smoke.h:145`, impl `Smoke.cpp:68` | **NONE** | **NO.** Every call `Mem_Alloc`s a `Smoke` (`Smoke.cpp:75`) onto an unbounded linked list. You must keep the returned pointer and `Smoke_Remove` it. Also: `Error_Fatal` on a bad `Smoke_Type` (`Smoke.cpp:112`), and **`Smoke.cpp:150` computes `i % smokeGlobs.textureCount`** — an integer divide by zero if `Smoke_LoadTextures` never ran. It runs at `GameState.cpp:694` with count 3, so in practice it is set, but guard `smokeGlobs.textureCount > 0` anyway. |
| `Smoke_Remove` | `(Smoke*, Container*)` — `Smoke.h:153`, impl `Smoke.cpp:259` | — | Ours; **hook commented** (`interop.cpp:4072`), which does not matter for our own calls. Stops the smoke's SFX and frees the node. |
| `DamageText_ShowNumber` | `(LegoObject*, uint32 displayNumber)` — `DamageText.h:118`, impl `DamageText.cpp:65` | 10 (`DAMAGETEXT_MAXSHOWN`) | **Yes.** Refuses `0` and `>999` (`DamageText.cpp:77`), honours `STATS2_DONTSHOWDAMAGE` and skips Dynamite/OohScary via `DamageText_CanShow` (`DamageText.cpp:301`), and returns quietly when all 10 slots are used (`DamageText.cpp:129-139`). Colour is hardcoded by magnitude: green `<5`, yellow `<10`, orange above (`DamageText.cpp:147-155`). |
| `Camera_Shake` | `(LegoCamera*, real32 intensity, real32 duration)` — `Camera.h:167`, impl `Camera.cpp:192` | — | **NO — edge-trigger only.** It assigns `shakeTimer = 0.0f` (`Camera.cpp:196`), so calling it every frame pins the shake at full intensity forever. |
| `LightEffects_SetBlink` / `_SetFade` / `_SetMove` | `LightEffects.h:152,161,170`; impl `LightEffects.cpp:165,213,257` | — | Safe to call, but they **overwrite the level's own configured values** in `lightGlobs` and set `HASBLINK`/`HASFADE`/`HASMOVE`. Snapshot and restore (see §6.2). |
| `LightEffects_SetDisabled` / `_Restart` | `LightEffects.h:140,129`; impl `LightEffects.cpp:117,62` | — | Safe. `_Restart` is a **custom** OpenLRR addition with no exe address — it resets every sub-effect and re-applies the initial colour and position. |

### Units, stated once

`STANDARD_FRAMERATE` is `25.0f` (`common.h:109`) and **durations in this engine are in
25ths of a second**. The only in-tree `Camera_Shake` reference point is the debug keybind:

```cpp
Camera_Shake(legoGlobs.cameraMain, 5.0f, 25.0f);   // GameState.cpp:2473 -- intensity 5, ONE SECOND
```

and `Level_Free` clears it with `Camera_Shake(legoGlobs.cameraMain, 0.0f, 0.0f)`
(`Game.cpp:3351`). The falloff is linear from full intensity to zero
(`Camera.cpp:475-481`), scaled by a fresh `Maths_RandRange(0,1)` per frame, applied as a
translation on `cam->contCam`.

> **Defect in the in-flight wave director.** `DeepCore.hpp:122-123` declares
> `waveShakeIntensity = 0.35f` and `waveShakeDuration = 1.5f`, and `WaveDirector.cpp:271-275`
> passes them straight to `Camera_Shake`. `1.5` standard units is **0.06 seconds** — under
> two frames at 30 fps. Against the only calibration point in the tree, a telegraph rumble
> wants something like `1.5f` intensity over `50.0f` duration (two seconds). This should be
> fixed before anyone tunes it by feel, or they will be tuning a value whose units they have
> mis-read. (Also note `docs/research/wave-director.md:772` cites `GameState.cpp:2465` for
> that call; it is now at `:2473`, and its `SFX.cpp:311` citation for
> `SFX_Random_PlaySoundNormal` is now `SFX.cpp:343` — both shifted by the 2026-07-30 fixes.)

---

## 5. Design — the dynamic threat-audio layer

### 5.1 What this is, and why it is not part of the wave director

`src/openlrr/game/WaveDirector.{hpp,cpp}` exists in the tree **untracked and mid-build**
(`git status` shows both as `??`; `WaveDirector.cpp:315` calls `PickWaveSpecies`, which is
neither declared in `DeepCore.hpp` nor defined anywhere, and nine of its `settings.wave*`
fields have no parser in `DeepCore.cpp`). It runs from `OpenLRR.cpp:984`, inside
`OpenLRR_MainLoop_Wrapper`.

Its telegraph (`WaveDirector.cpp:260-276`) is **event** audio: one `Info_Send`, one shake,
fired once per wave. That is correct and sufficient for *warning*.

What is missing is **state** audio: a continuous signal that tracks how dangerous the
situation *is*, right now, independent of whether a wave was just announced. That is a
separate concern with a separate update cadence and separate failure modes, and it should be
a separate file. Concretely:

- it must run even when `WaveDirector` is off, because map-native emerges are also threat;
- it owns long-lived looping voices, which the director does not;
- it must survive level teardown cleanly, which means its own `Reset()`.

**Proposal: `src/openlrr/game/Juice.{hpp,cpp}`, namespace `DeepCore::Juice`.**

### 5.2 The threat model — one scalar, computed from what we can actually see

Everything needed is already ours:

| Input | How | Owner |
| --- | --- | --- |
| live monsters | `objectListSet.EnumerateSkipUpgradeParts()` (`Object.h:519`), filter `type == LegoObject_RockMonster`, `health >= 0`, not `LIVEOBJ1_CRUMBLING` | **OURS** — idiom already proven at `WaveDirector.cpp:92-104` |
| monster world position | `LegoObject_GetPosition(obj, &x, &y)` (`Object.h:1241`) | **OURS** |
| raider / building positions | same enumeration, `LegoObject_MiniFigure` / `_Building` | **OURS** |
| camera focus | `Camera_GetTopdownPosition(legoGlobs.cameraMain, &v)` (`Camera.h:218`) | **OURS** |
| in a level, not paused | `Lego_IsInLevel()` (`Game.h:812`), `legoGlobs.flags1 & GAME1_FREEZEINTERFACE` | **OURS** |
| world time | `elapsed * Lego_GetGameSpeed()` (`Game.h:1068`), `/ STANDARD_FRAMERATE` | **OURS** |

`threat` is a scalar in `[0,1]`, recomputed a few times a second (not every frame — the scan
is O(objects) and there is no reason to run it at frame rate), then **slewed** toward its
target so the audio never steps:

```
raw = clamp( monstersAlive / ThreatFullCount, 0, 1 )
    * ( 1 + ThreatProximityBoost * nearFraction )     // nearFraction: monsters within
                                                       // ThreatNearBlocks of any raider
threat += clamp(raw - threat, -ThreatFallRate * dt, +ThreatRiseRate * dt)
```

Asymmetric rates are the whole trick: rise fast (danger should feel immediate), fall slow
(relief should feel earned). Defaults: rise ≈ 1.0/s, fall ≈ 0.15/s.

### 5.3 The four channels, and exactly what each costs

**Channel A — the tension bed.** A resident (non-`@`) looping sample whose *buffer volume*
tracks `threat`. This is the `SND_AirBeat` pattern (`GameState.cpp:1709-1763`) done
correctly. Started once when `threat` first crosses `ThreatBedOnLevel`, stopped when it falls
below hysteresis. **We keep the play UID** — this is where `GameState.cpp:1717`'s bug is
avoided:

```cpp
// start
_bedPlayUID = SFX_Random_PlaySoundNormal(_bedSFX, /*loop*/ true);   // SFX.cpp:343
// per update, while running
const sint32 volMin = Gods98::Sound3D_MinVolume();   // -10000  (3DSound.cpp:1603)
const sint32 volMax = Gods98::Sound3D_MaxVolume();   //      0
const sint32 want = (sint32)Gods98::Maths_Interpolate(
                        (real32)volMin + (real32)(volMax - volMin) * settings.juiceBedFloor,
                        (real32)volMax, threat);
if (want != SFX_Random_GetBufferVolume(_bedSFX)) SFX_Random_SetBufferVolume(_bedSFX, want);
// stop
if (_bedPlayUID > 0) { SFX_Sound3D_StopSound(_bedPlayUID); _bedPlayUID = -1; }
```

Caveat, stated honestly: `SFX_Random_SetBufferVolume` sets the **sample's** volume, not the
voice's (`3DSound.cpp:628-646`). Since we own this cue exclusively that is fine — but the
layer must restore the sample's original volume on `Reset()`, or a cue reused elsewhere in
the user's config would be left ducked. Snapshot `SFX_Random_GetBufferVolume` at first use.

**Channel B — proximity stingers.** When a monster crosses inside `ThreatStingerBlocks` of a
raider or building *and* was not inside last tick, play a 3D one-shot at the monster:

```cpp
Vector3F wPos = { mx, my, Map3D_GetWorldZ(Lego_GetMap(), mx, my) };
SFX_Random_PlaySound3DOnFrame(nullptr, _stingerSFX, /*loop*/ false, /*onFrame*/ false, &wPos);
```

`onFrame = false` + a `wPos` is the "play at a world point, no container" form
(`SFX.cpp:437-439`). Rate-limited by a cooldown *and* by a per-object "already stung" flag
kept DLL-side in a `std::vector<LegoObject*>` — never on the object, which is exe-overlaid.

**Channel C — the all-clear.** When `threat` falls below `ThreatClearLevel` having been above
`ThreatBedOnLevel`, fire one non-looping cue through
`SFX_Random_SetAndPlayGlobalSample` (`SFX.cpp:314`). Using the *global sample* channel rather
than a plain play is deliberate: it self-throttles against the text-panel sounds that use the
same channel (`TextMessages.cpp:207`), so relief never stacks on top of an objective jingle.

**Channel D — the visual tell.** Tie `threat` to the cave light, via
`LightEffects_SetFade` (`LightEffects.cpp:213`) pushed toward a warmer/redder
`fadeRGBMax` at high threat and restored at low. See §6.2 for the snapshot/restore
discipline this requires.

### 5.4 What needs user-supplied audio, and what does not

| Channel | Needs a new WAV? | Fallback with stock assets only |
| --- | --- | --- |
| A — tension bed | **Yes** | None honest. A looping bed has no stock equivalent; `SFX_AmbientLoop` is the cave ambience and ducking it is not the same effect. **Ship the channel disabled unless the cue name resolves.** |
| B — proximity stinger | No | `SFX_RockMonster` / `SFX_RockMonster2` (`GameCommon.h:1382-1383`) are already loaded in every stock config. Good default. |
| C — all-clear | No | `SFX_Okay` (`GameCommon.h:1392`). Serviceable, not ideal. |
| D — light shift | No | Pure parameter change on an already-loaded system. |

So **three of four channels work out of the box** and the one that does not degrades to
silence rather than to noise. That is the right default: a cue name that does not resolve
must warn once and disable *that channel*, never fall back to an arbitrary sample.

### 5.5 Config surface

Same shape as the existing `DeepCore` block (`DeepCore.cpp:380-520`,
`data/Settings/DeepCore.cfg`). Every gate false = vanilla.

```
Lego* {
  DeepCore {

    ; ---- THE JUICE LAYER ----------------------------------------------
    ; Continuous audio/visual response to how dangerous the level currently
    ; is. Independent of WaveDirector: map-native emerges count too.
    DynamicThreatAudio        FALSE

    ; Monsters alive that count as "maximum threat".
    ThreatFullCount           4
    ; A monster this close (blocks) to a raider or building counts double.
    ThreatNearBlocks          6
    ThreatProximityBoost      0.5
    ; Threat units per second. Rise fast, fall slow -- relief is earned.
    ThreatRiseRate            1.0
    ThreatFallRate            0.15
    ; How often the object scan runs, in seconds. Not every frame.
    ThreatSampleSeconds       0.25

    ; --- Channel A: the tension bed (needs YOUR wav; silent if unset) ---
    ; A cue name from your Samples block. Looped, volume tracks threat.
    ;ThreatBedSample          SND_Tension
    ThreatBedOnLevel          0.25      ; threat at which the bed starts
    ThreatBedOffLevel         0.10      ; and stops (hysteresis: must be lower)
    ThreatBedFloorVolume      0.35      ; fraction of full volume at ThreatBedOnLevel

    ; --- Channel B: proximity stingers ---
    ThreatStingerSample       SFX_RockMonster
    ThreatStingerBlocks       5
    ThreatStingerCooldown     6.0       ; seconds, global

    ; --- Channel C: the all-clear ---
    ThreatClearSample         SFX_Okay
    ThreatClearLevel          0.05

    ; --- Channel D: the light responds ---
    ThreatLightShift          FALSE
    ThreatLightRGBMax         0.9:0.45:0.35
  }
}
```

Parsing follows `DeepCore.cpp:396-406` exactly — `Config_FindItem` first, so a partial file
cannot silently zero a tuning value, and a rejected value keeps the default *and warns*.

### 5.6 Sketch

`src/openlrr/game/Juice.hpp`:

```cpp
// Juice.hpp : Continuous audio/visual response to threat state.
//
// SAFETY: adds no hook. Runs from the existing post-MainLoop slot in
// OpenLRR_MainLoop_Wrapper (OpenLRR.cpp:984), next to DeepCore::Waves::Update.
// Touches no struct carrying assert_sizeof -- all state is DLL-side. The only
// exe-overlaid values it writes are sample buffer volumes (via SFX, which is
// ours end to end) and lightGlobs fade parameters, both snapshotted and restored.
#pragma once
#include "../common.h"

namespace DeepCore { namespace Juice {

/// Drop all cached SFX_IDs, stop any owned loop, restore any borrowed volume or
/// light parameter. Called on level teardown AND on successful level load.
void Reset(void);

/// Advance one frame. `elapsedReal` is the raw value handed to
/// OpenLRR_MainLoop_Wrapper (25.0 == one second). Returns immediately when the
/// feature is off or there is no live level.
void Update(real32 elapsedReal);

/// Current threat scalar in [0,1], for diagnostics.
real32 Threat(void);

} }
```

`src/openlrr/game/Juice.cpp`, the load-bearing parts:

```cpp
namespace {

struct State
{
    bool    resolved      = false;
    SFX_ID  bedSFX        = SFX_ID::SFX_NULL;
    SFX_ID  stingerSFX    = SFX_ID::SFX_NULL;
    SFX_ID  clearSFX      = SFX_ID::SFX_NULL;

    sint32  bedPlayUID    = -1;     ///< PLAY UID from SFX_Random_PlaySoundNormal.
                                    ///  NOT a sound-table index -- see GameState.cpp:1717
                                    ///  for what happens when those two are confused.
    sint32  bedSavedVol   = 0;      ///< sample volume before we started ducking it
    bool    bedVolSaved   = false;

    real32  threat        = 0.0f;
    real32  sampleTimer   = 0.0f;
    real32  stingerCool   = 0.0f;
    bool    bedWasOn      = false;

    std::vector<LegoRR::LegoObject*> stung;   ///< monsters already stung this approach
};
State _s;

/// Resolve a cue name once. An unresolvable name disables its channel and warns
/// exactly once -- never falls back to an arbitrary sample, because a wrong sound
/// is worse than no sound.
bool ResolveCue(const std::string& name, LegoRR::SFX_ID* out, const char* whatFor)
{
    using namespace LegoRR;   // required: SFX_GetType takes LegoRR types
    if (name.empty()) return false;
    if (!SFX_GetType(name.c_str(), out)) {
        Juice_WarnF(true, "%s: no sample named \"%s\" in the Samples block; "
                          "this channel is disabled.", whatFor, name.c_str());
        return false;
    }
    return true;
}

/// Scan once. O(objects); called at ThreatSampleSeconds, not per frame.
real32 MeasureThreat(void)
{
    using namespace LegoRR;

    std::vector<Point2F> defenders;
    sint32 monsters = 0, near = 0;
    std::vector<Point2F> monsterPos;

    for (LegoObject* obj : objectListSet.EnumerateSkipUpgradeParts()) {
        if (obj->health < 0.0f) continue;
        Point2F p = { 0.0f, 0.0f };
        if (obj->type == LegoObject_RockMonster) {
            if (obj->flags1 & LIVEOBJ1_CRUMBLING) continue;   // already dying
            LegoObject_GetPosition(obj, &p.x, &p.y);
            monsterPos.push_back(p);
            monsters++;
        }
        else if (obj->type == LegoObject_MiniFigure || obj->type == LegoObject_Building) {
            LegoObject_GetPosition(obj, &p.x, &p.y);
            defenders.push_back(p);
        }
    }
    if (monsters == 0) return 0.0f;

    const real32 nearWorld = (real32)DeepCore::settings.threatNearBlocks
                           * Lego_GetMap()->blockSize;      // blocks -> world units
    const real32 nearSq = nearWorld * nearWorld;
    for (const Point2F& m : monsterPos) {
        for (const Point2F& d : defenders) {
            const real32 dx = m.x - d.x, dy = m.y - d.y;
            if (dx*dx + dy*dy < nearSq) { near++; break; }
        }
    }

    real32 raw = (real32)monsters / (real32)DeepCore::settings.threatFullCount;
    raw *= 1.0f + DeepCore::settings.threatProximityBoost
                * ((real32)near / (real32)monsters);
    return (raw > 1.0f ? 1.0f : raw);
}

} // namespace

void DeepCore::Juice::Update(real32 elapsedReal)
{
    using namespace LegoRR;

    if (!settings.dynamicThreatAudio) return;
    if (!Lego_IsInLevel())             return;          // Game.h:812
    if (legoGlobs.flags1 & GAME1_FREEZEINTERFACE) return;
    if (!SFX_IsSoundOn())              return;          // SFX.cpp:596

    const real32 seconds = (elapsedReal * Lego_GetGameSpeed()) / STANDARD_FRAMERATE;

    if (!_s.resolved) {
        _s.resolved = true;
        ResolveCue(settings.threatBedSample,     &_s.bedSFX,     "ThreatBedSample");
        ResolveCue(settings.threatStingerSample, &_s.stingerSFX, "ThreatStingerSample");
        ResolveCue(settings.threatClearSample,   &_s.clearSFX,   "ThreatClearSample");
    }

    _s.sampleTimer += seconds;
    static real32 target = 0.0f;
    if (_s.sampleTimer >= settings.threatSampleSeconds) {
        _s.sampleTimer = 0.0f;
        target = MeasureThreat();
    }

    // Asymmetric slew: rise fast, fall slow.
    const real32 rate = (target > _s.threat ? settings.threatRiseRate
                                            : settings.threatFallRate);
    const real32 step = rate * seconds;
    real32 delta = target - _s.threat;
    if (delta >  step) delta =  step;
    if (delta < -step) delta = -step;
    _s.threat += delta;

    UpdateBed();        // channel A
    UpdateStingers();   // channel B
    UpdateClear();      // channel C
    UpdateLight();      // channel D
}
```

`Reset()` must, in order: stop `bedPlayUID` if `> 0`; restore `bedSavedVol` via
`SFX_Random_SetBufferVolume` if `bedVolSaved`; restore the light snapshot; clear `stung`;
`_s = State{}`. It is called from the same places `Waves::Reset` is.

### 5.7 Failure modes, and what each does

| # | Failure | Symptom | Mitigation |
| --- | --- | --- | --- |
| 1 | Bed cue name absent from `Samples` | channel silent | resolve once, warn once, disable that channel (§5.6 `ResolveCue`) |
| 2 | Bed cue is not `*`-simultaneous | second start restarts rather than layers | we only ever start one; harmless. Document it. |
| 3 | Bed cue is `@`-streamed | collides with the single global loop stream (`3DSound.cpp:1234`) | document "do not use `@` for `ThreatBedSample`". Cannot be detected from `SFX_ID` alone — **UNDETERMINED** whether a check is worth adding. |
| 4 | Level ends while bed is looping | loop survives into the front end | `Reset()` on teardown; also `SFX_SetSoundOn_AndStopAll` already runs on some paths (`SFX.cpp:550`) |
| 5 | Bed sample's volume left ducked | a cue the user reuses elsewhere plays quiet forever | snapshot+restore (§5.6) |
| 6 | `stung` vector holds a freed `LegoObject*` | stale-pointer compare (compare only — never deref) | clear it every measure tick; **never dereference**, only compare. Safer still: key on block position instead of pointer. |
| 7 | Threat scan cost on a large map | frame time | it is O(monsters × defenders), gated by `ThreatSampleSeconds`; with the stock 15-object-ID ceiling the counts are small |
| 8 | Feature on, `SFX_IsSoundOn()` false | dead code every frame | early-out, already in the sketch |
| 9 | Light shift left applied at level end | wrong cave colour next level | restore in `Reset()`; `LightEffects_Restart` (`LightEffects.cpp:62`) is the belt-and-braces version |

---

## 6. Cheap-but-flashy wins that need **no new assets at all**

Ranked by (impact × certainty) ÷ risk.

### 6.1 Damage numbers on monsters — one line, already capped, already ours

`DamageText_ShowNumber` (`DamageText.cpp:65`) is fully implemented, hooked, self-capping at
10, and colour-codes by magnitude. It is currently only shown where the exe calls it. Every
guard it needs is already inside it. **Zero new assets** — the digit textures load at
`DamageText_LoadTextures` (`DamageText.cpp:44`) from the stock font BMPs.

### 6.2 Threat-reactive cave light — parameter change on a live system

`LightEffects` is 29/29 ours, including a custom `Restart` and per-sub-effect enable
toggles. `LightEffects_SetFade` (`LightEffects.cpp:213`) takes min/max RGB, wait, speed and
hold ranges and sets `LIGHTFX_GLOB_FLAG_HASFADE`. Pushing `fadeRGBMax` toward red as threat
rises is a whole-screen mood change for a dozen lines and no art.

**The discipline it requires:** `lightGlobs` is exe-overlaid at `0x004ebdd8`
(`LightEffects.cpp:19`) and pinned by `assert_sizeof(LightEffects_Globs, 0xf4)`
(`LightEffects.h:93`). Copying it **by value** into DLL-side storage is fine — that is a
plain struct copy, it grows nothing:

```cpp
static LegoRR::LightEffects_Globs _lightSnapshot;   // DLL-side, by value. Grows nothing.
static bool _lightSnapshotTaken = false;

if (!_lightSnapshotTaken) { _lightSnapshot = LegoRR::lightGlobs; _lightSnapshotTaken = true; }
// ... on Reset(): LegoRR::lightGlobs = _lightSnapshot;  LegoRR::LightEffects_ResetSpotlightColour();
```

### 6.3 Impact particles on monster hits — the safest visual primitive in the tree

`Effect_Spawn_Particle` (`Effects.cpp:606`) null-checks everything, caps at 10 per type, and
returns `false` when full. Ten types already load from the user's `MiscObjects` config
(`Effects.cpp:527-555`) — `MISCOBJECT_LAZERHIT`, `_PUSHERHIT`, `_FREEZERHIT`, `_PATHDUST`,
`_UPGRADEEFFECT`… All are stock assets already in every install. Reusing `_UPGRADEEFFECT` as
a spawn flourish, or `_PATHDUST` under a heavy creature's step, costs one call.

### 6.4 Emerge dust — the wave director's telegraph, done right

`Smoke_CreateSmokeArea` (`Smoke.cpp:68`) with `Smoke_Type_Dissipate` self-terminates after
four groups (`Smoke.cpp:124-129`) — the one mode that does **not** need us to hold a pointer
forever. Textures are the stock `MiscAnims\Smoke\Smoke0..2.bmp` (`GameState.cpp:694`). Guard
`smokeGlobs.textureCount > 0` (§4) and keep the returned `Smoke*` anyway so `Reset()` can
sweep it.

### 6.5 Camera-shake vocabulary — free, and currently mis-tuned

One intensity/duration pair per event class costs nothing and is the difference between
"something happened" and "something *big* happened". The units correction in §4 is a
prerequisite; without it every value anyone picks will be an order of magnitude short.

### 6.6 Fix the air-beat stop bug — one line, real audible payoff

`GameState.cpp:1717`. Store the UID returned by the `SFX_Random_PlaySoundNormal` at
`GameState.cpp:1726` in a file-static, stop *that*. Removes a stuck loop and stops a random
unrelated sample being silenced.

### 6.7 Cache the three per-frame `SFX_GetType("SND_AirBeat")` calls

`GameState.cpp:1715,1725,1729` hash a string literal and linear-scan up to 495 entries three
times per frame. Resolve once. Trivial, and it sets the idiom the juice layer will follow.

---

## 7. DECISION — ranked plan

Ordering principle: **fix the thing that corrupts memory, then build the thing that cannot
break anything, then build the thing that is worth the most.** Items 1–3 have file-level
detail below; 4–7 are one-liners because they are one-liners.

| # | Item | Risk | New assets | Files |
| --- | --- | --- | --- | --- |
| **1** | Bound `SFX_GetType`'s `hashNameList` write | none | no | `game/audio/SFX.cpp` |
| **2** | Fix the `Camera_Shake` units in the in-flight wave director | none | no | `game/DeepCore.hpp`, `game/WaveDirector.cpp` |
| **3** | Build `DeepCore::Juice` — dynamic threat audio (§5) | low | bed only, optional | new `game/Juice.{hpp,cpp}`, `game/DeepCore.{hpp,cpp}`, `OpenLRR.cpp`, `openlrr.vcxproj`, `data/Settings/DeepCore.cfg` |
| 4 | Fix the air-beat stop-unit bug (§6.6) | none | no | `game/GameState.cpp:1717` |
| 5 | Cache the air-beat `SFX_GetType` (§6.7) | none | no | `game/GameState.cpp:1715-1729` |
| 6 | Threat-reactive light, channel D of item 3 (§6.2) | low | no | `game/Juice.cpp` |
| 7 | Ship a modder note on the `Samples` grammar and the LWS `Error_FatalF` (§2.3, §3) | none | no | new `docs/MODDING-AUDIO.md` |

**Explicitly not recommended:** adaptive *music*. It is CD redbook (`Game.cpp:2931`,
`Sound.cpp:105`) with start/stop and nothing else. Anyone promising crossfaded stems here
would be promising something the API cannot do.

**Explicitly not recommended:** reimplementing anything in `InfoMessages`. It is 0/38
(`InfoMessages.cpp`), there is no trampoline (`HANDOFF-2026-07-30.md:74-77`), and the juice
layer needs to *call* `Info_Send`, not replace it.

---

### Item 1 — `game/audio/SFX.cpp`, inside `SFX_GetType`

Replace the `POPULATEMODE` branch at `SFX.cpp:126-134` with the guarded version in §2.5.
Unconditional, not gated, for the same reason the other three overflow guards are
(`HANDOFF-2026-07-30.md:102-105`): the behaviour being replaced is memory corruption, so
there is no vanilla semantics to preserve.

- **Warning count:** `Error_WarnF2` with a `const char*` and an `sint32` is the exact form
  already used 30 lines below at `SFX.cpp:224`. No new warning.
- **Acceptance:** `tools/addrlint/addrlint.py --check` must still report 113 regions, 0
  overlaps — it will, because no struct changes. `docs/ADDRESS-MAP.md` must not change.
- **Verification available to us:** compile only. The boundary (name 451) cannot be reached
  without a game installation and a 451-cue config. **UNDETERMINED by execution.**
- **Worth doing at the same time:** the `Error_Fatal` at `3DSound.cpp:365` is the harder
  ceiling (600 buffers) and it *kills the process*. Convert it to a warn-and-return-false in
  a follow-up; `Sound3D_Load`'s caller chain already handles `-1` (`SFX.cpp:192-195`).

### Item 2 — `game/DeepCore.hpp` and `game/WaveDirector.cpp`

`DeepCore.hpp:122-123`:

```cpp
    /// Camera shake accompanying the warning. 0 disables.
    ///
    /// UNITS: both are in the engine's standard 25ths-of-a-second (common.h:109), NOT
    /// seconds. The only calibration point in the tree is the debug keybind at
    /// GameState.cpp:2473 -- Camera_Shake(cam, 5.0f, 25.0f) is "intensity 5 for one
    /// second". A telegraph rumble wants to be gentler and much longer than an impact.
    real32 waveShakeIntensity = 1.5f;
    real32 waveShakeDuration  = 50.0f;   // 2.0 seconds
```

Add the two missing parsers next to `WaveIntervalSeconds` (`DeepCore.cpp:396-406`), using the
same `Config_FindItem`-then-validate shape, and add them to the `verboseStartup` dump at
`DeepCore.cpp:552-562`. While in the file: the nine other `settings.wave*` fields declared at
`DeepCore.hpp:95-127` also have no parser, and `WaveDirector.cpp:315` calls a
`PickWaveSpecies` that does not exist. That is the in-flight author's work, not this
document's, but the shake units are a *silent* wrongness and worth flagging now rather than
after someone tunes around them.

### Item 3 — `DeepCore::Juice`

**New files.**

- `src/openlrr/game/Juice.hpp` — the four-function interface in §5.6. Header includes only
  `../common.h`, exactly like `WaveDirector.hpp:32`, so it drags in no `LegoRR` types and
  cannot trip the macro/namespace trap at include sites.
- `src/openlrr/game/Juice.cpp` — includes `object/Object.h`, `world/Camera.h`,
  `world/Map3D.h`, `audio/SFX.h`, `effects/LightEffects.h`, `Game.h`, `DeepCore.hpp`.
  **Every function that touches `LegoRR` names opens with `using namespace LegoRR;`** — not
  for style, but because `Lego_GetMap`, `Lego_IsInLevel` and friends are macros or inlines
  whose expansions name `LegoRR` types unqualified and cannot themselves be qualified.
  `WaveDirector.cpp:94` and `DeepCore.cpp:82` are the in-tree precedent.

**Edits.**

- `src/openlrr/OpenLRR.cpp:984` — one line after `DeepCore::Waves::Update(elapsed);`:
  ```cpp
  /// DEEPCORE: Continuous threat audio/visuals. Same slot, same contract as Waves:
  /// returns immediately when off or when there is no live level. See game/Juice.hpp.
  DeepCore::Juice::Update(elapsed);
  ```
  Plus `#include "game/Juice.hpp"` beside the existing `WaveDirector.hpp` include.
  **No new hook. No `interop.cpp` edit.**
- `src/openlrr/game/DeepCore.hpp` — the fields in §5.5, in a new `// ---- The juice layer ----`
  block; add `settings.dynamicThreatAudio` to `IsAnyFeatureEnabled` (`DeepCore.cpp:53-61`).
- `src/openlrr/game/DeepCore.cpp` — parsers in the `Config_FindItem`-first style; `_SplitFields`
  already exists for the RGB triple; `verboseStartup` lines.
- `src/openlrr/openlrr.vcxproj` — `<ClInclude Include="game\Juice.hpp" />` next to line 259,
  `<ClCompile Include="game\Juice.cpp" />` next to line 386. Same two-line pattern
  `WaveDirector` already uses.
- `data/Settings/DeepCore.cfg` — the block from §5.5, commented in the house style, with the
  bed sample **commented out by default** so a user without a WAV gets three working channels
  and one silent one rather than a warning on every level load.

**Wiring `Reset()`.** Wherever `DeepCore::Waves::Reset()` ends up being called from — level
teardown and successful level load, per `WaveDirector.hpp:41-43` — `Juice::Reset()` goes
beside it. `Game.cpp:3351` (`Camera_Shake(cam, 0.0f, 0.0f)` in the level-free path) is the
natural landmark; it sits next to `Smoke_RemoveAll()` at `Game.cpp:3368`.

**Build contract.** Nothing above adds a warning: no new `printf`-family format strings
beyond the `Error_WarnF2`/`Error_DebugF2` macro shapes already used in `DeepCore.cpp:23-24`,
no signed/unsigned mixing in the sketch, no unreferenced locals. **The 44-warning contract
should hold — but that is a prediction, not a measurement, and the lead engineer's build is
the only thing that settles it.**

**What "done" means, honestly.** Compile-verified with 0 errors and exactly 44 warnings, in
both configurations. That is the ceiling this project can reach
(`HANDOFF-2026-07-30.md:3-6`). Whether the tension bed actually *feels* like tension is
**UNDETERMINED and will remain so** until someone with the game runs it.
