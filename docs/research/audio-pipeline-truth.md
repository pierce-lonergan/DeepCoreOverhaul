# The Audio Pipeline: Ground Truth

**Question asked:** what exactly must a generated audio file *be*, and what exactly must a modder
*do*, for this engine to load and play it? Assume nothing; read the loader.

**Scope of evidence.** Everything below is read from source in this repository. The loader itself
(`Sound3D_Load`, `Sound3D_LoadSample`, `WaveOpenFile`, `Sound3D_CreateSoundBuffer`) is **OURS** —
fully reimplemented C++ in the DLL — so the container/format answers are certain. The *config block
parser* that feeds it (`Lego_LoadSamples`) is an **EXE address macro** and is still 1999 machine
code; where that matters the claim is marked **UNDETERMINED** and a verification route is given.
We cannot run the game, so nothing here is play-tested.

---

## 0. The seven answers, compressed

| # | Question | Answer |
|---|---|---|
| 1 | Container | RIFF/WAVE. Chunks parsed: `RIFF`+form `WAVE`, `fmt `, `data`. **No check at all** on sample rate, bit depth or channel count. |
| 2 | Mono vs stereo | **Every non-streamed sample MUST be mono.** All secondary buffers are created with `DSBCAPS_CTRL3D` (`3DSound.cpp:1123`) — including the ones used for "2D" UI cues. Only `@`-prefixed *streamed* samples may be stereo. |
| 3 | Location | `<game dir>\Data\<config path>.wav`. `.wav` is appended by the loader. WAD wins over loose files by default; `-datafirst` flips it; new filenames need no flag. **Streamed samples never look in a WAD at all.** |
| 4 | Registry | Names are hash-registered on first sight while `POPULATEMODE` is on. 495 slots, 45 pre-registered, **450 free**. A new cue needs **zero C++** — one config line and one WAV. |
| 5 | LWS trigger | A null object named `SFX,<CueName>,<frames>` — confirmed, `Lws.cpp:105-110`, `:350-377`, `:536-565`. |
| 6 | What we can call | 4 game-level play functions + `Sound3D_Play2` and its 4 macros. Only `SFX_Random_SetAndPlayGlobalSample` is safe to call unguarded per-frame. |
| 7 | Caps | 600 WAV slots (fatal on overflow), 3 simultaneous voices per sample, 20 position-tracked frames, 200 group entries, 1 looping + 1 non-looping stream, 100 comma-tokens per config line (**unguarded**). |

**The one sentence that matters most:** *if you generate a stereo WAV for anything other than an
`@`-streamed sample, `CreateSoundBuffer` fails, the cue loads as handle `-1`, and the sound is
silently absent for the entire session with only a `Warn`-level line in the log.*

---

## 1. Container and format

### 1.1 The call chain

```
  Lego_LoadSamples(config, ...)                 EXE MACRO   Game.h:1914   (0x00434980)
    -> SFX_GetType(<key>, &sfxID)               OURS        SFX.cpp:112
    -> SFX_LoadSampleProperty(<value>, sfxID)   OURS        SFX.cpp:140
         -> Gods98::Sound3D_Load(name, stream, simultaneous, volume)   OURS  3DSound.cpp:265
              -> Sound3D_LoadSample(...)                               OURS  3DSound.cpp:1043
                   -> Gods98::File_LoadBinary(fName, &fileSize)        OURS  Files.cpp:1023
                   -> Gods98::WaveOpenFile(...)                        OURS  Sound.cpp:186
                   -> Gods98::WaveStartDataRead(...)                   OURS  Sound.cpp:515
                   -> Gods98::WaveReadFile(...)                        OURS  Sound.cpp:544
                   -> Sound3D_CreateSoundBuffer(...)                   OURS  3DSound.cpp:1112
                   -> Sound3D_SendSoundToBuffer(...)                   OURS  3DSound.cpp:1154
```

Every step below the exe macro is ours. The extension is not optional and not configurable —
`Sound3D_Load` builds the filename itself (`3DSound.cpp:280`):

```cpp
std::sprintf(buffer, "%s.wav", fName);
```

So a config value of `Sounds\DeepCore\tension1` becomes `Sounds\DeepCore\tension1.wav`, always,
lowercase in the string (Windows and the WAD lookup are both case-insensitive — the WAD compare is
`::_stricmp`, `Wad.cpp:346`).

### 1.2 What `WaveOpenFile` actually parses

`Gods98::WaveOpenFile` (**OURS**, `Sound.cpp:186`) is the in-memory parser used for all
non-streamed samples. It is the classic MMIO wave reader. The whole of its validation is:

```cpp
// Sound.cpp:203-208 -- open the already-loaded bytes as an MMIO memory file
mmioInfo.fccIOProc = FOURCC_MEM;
mmioInfo.pchBuffer = (HPSTR)fileData;
mmioInfo.cchBuffer = fileSize;
if ((hmmioIn = ::mmioOpenA(nullptr, &mmioInfo, MMIO_ALLOCBUF | MMIO_READ)) == nullptr) { ... }

// Sound.cpp:214 -- descend into the outermost chunk
if ((nError = (sint32)::mmioDescend(hmmioIn, pckInRIFF, nullptr, 0)) != 0) { ... }

// Sound.cpp:220-224 -- CHECK 1: must be RIFF, form type must be WAVE
if ((pckInRIFF->ckid != FOURCC_RIFF) || (pckInRIFF->fccType != mmioFOURCC('W','A','V','E')))
{
    nError = ER_NOTWAVEFILE;
    goto ERROR_READING_WAVE;
}

// Sound.cpp:227-231 -- CHECK 2: a 'fmt ' chunk must exist somewhere inside the form
ckIn.ckid = mmioFOURCC('f','m','t',' ');
if ((nError = (sint32)::mmioDescend(hmmioIn, &ckIn, pckInRIFF, MMIO_FINDCHUNK)) != 0) { ... }

// Sound.cpp:236-240 -- CHECK 3: 'fmt ' must be at least sizeof(PCMWAVEFORMAT) == 16 bytes
if (ckIn.cksize < (sint32) sizeof(PCMWAVEFORMAT))
{
    nError = ER_NOTWAVEFILE;
    goto ERROR_READING_WAVE;
}

// Sound.cpp:243 -- read exactly 16 bytes into a PCMWAVEFORMAT
if (::mmioRead(hmmioIn, (HPSTR)&pcmWaveFormat, sizeof(pcmWaveFormat)) != sizeof(pcmWaveFormat)) { ... }

// Sound.cpp:253-267 -- if the tag is NOT WAVE_FORMAT_PCM, read cbSize and allocate the extra bytes
if (pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM) cbExtraAlloc = 0;
else { /* read a uint16 cbSize, allocate that many extra bytes */ }
```

That is the **entire** format validation for a non-streamed sample. Note what is *not* there:

- **No sample-rate check.** `nSamplesPerSec` is copied into the `WAVEFORMATEX` and handed straight
  to DirectSound (`3DSound.cpp:1126`).
- **No bit-depth check.** `wBitsPerSample` is likewise never inspected on the load path. (The
  *streaming* path reads it, but only to pick the silence-fill byte — `3DSound.cpp:1416` and
  `:1515` use `128` for 8-bit and `0` otherwise.)
- **No channel-count check.** Nothing in `Sound.cpp` or `3DSound.cpp` ever reads `nChannels` on a
  loaded sample. This is exactly why §2 is a landmine: the constraint is real but it is enforced by
  DirectSound, not by any code we can grep for.
- **No `WAVE_FORMAT_PCM` requirement on the non-streamed path.** The `else` branch at
  `Sound.cpp:258` cheerfully builds a `WAVEFORMATEX` with extra bytes for a compressed or
  extensible format. It will parse. It will then fail in `CreateSoundBuffer`, because DirectSound
  secondary buffers take PCM (or `WAVEFORMATEXTENSIBLE` wrapping PCM) only.

The **streamed** path is the only one with an explicit format gate (`3DSound.cpp:1308-1313`):

```cpp
if (streamData->wiWave.pwfx->wFormatTag != WAVE_FORMAT_PCM)
{
    Error_Warn(true, "Wave file not PCM.");
    WaveCloseReadFile(&streamData->wiWave.hmmio, &streamData->wiWave.pwfx);
    return false;
}
```

### 1.3 Finding the audio

`Gods98::WaveStartDataRead` (**OURS**, `Sound.cpp:515`) locates the samples:

```cpp
// Sound.cpp:522 -- rewind to just past the 'WAVE' form type
if ((nError = ::mmioSeek(*phmmioIn, pckInRIFF->dwDataOffset + sizeof(FOURCC), SEEK_SET)) == -1)
    nError = 1;
nError = 0;                                    // <-- Sound.cpp:527, the seek error is discarded

// Sound.cpp:529-530 -- scan forward for 'data'
pckIn->ckid = mmioFOURCC('d','a','t','a');
if ((nError = ::mmioDescend(*phmmioIn, pckIn, pckInRIFF, MMIO_FINDCHUNK)) != 0) { }
```

Because the scan restarts from the top of the form, **extra chunks are tolerated**: `LIST`/`INFO`,
`fact`, `cue `, `id3 ` and anything else between `fmt ` and `data` are skipped by `MMIO_FINDCHUNK`.
Chunk order beyond "`fmt ` and `data` both exist inside the `RIFF`/`WAVE` form" does not matter.

`Sound3D_LoadSample` then sizes the staging buffer from the `data` chunk header and byte-copies it
(`3DSound.cpp:1066-1075`):

```cpp
if ((sound->data = (uint8*)::GlobalAlloc(GMEM_FIXED, ckIn.cksize)) == nullptr) { nError = ER_MEM; goto err; }
if ((nError = WaveReadFile(hmmioIn, ckIn.cksize, sound->data, &ckIn, &cbActualRead)) != 0) goto err;
sound->size = cbActualRead;
```

`sound->size` becomes `dwBufferBytes` at `3DSound.cpp:1125`. A `data` chunk of size 0 therefore
asks DirectSound for a 0-byte secondary buffer, which is invalid (`DSBSIZE_MIN`) — the sample fails
to load. A truncated `data` chunk (header claims more bytes than the file holds) makes
`WaveReadFile` return `ER_CORRUPTWAVEFILE` (`Sound.cpp:580`) and the load fails.

### 1.4 The exact file a generator must emit

To load without complaint on the **normal (non-streamed) path**:

| Property | Required value | Why |
|---|---|---|
| Container | `RIFF....WAVE` | `Sound.cpp:220` |
| `fmt ` chunk | present, `cksize >= 16` | `Sound.cpp:227,236` |
| `wFormatTag` | `1` (`WAVE_FORMAT_PCM`) | not checked by us; required by `CreateSoundBuffer` |
| `nChannels` | **`1` (mono) — mandatory** | `DSBCAPS_CTRL3D`, `3DSound.cpp:1123`; see §2 |
| `wBitsPerSample` | 8 or 16 (**16 recommended**) | unchecked; 16 matches the primary buffer, `3DSound.cpp:155` |
| `nSamplesPerSec` | any (**22050 recommended**) | unchecked; the primary mixer is set to 22050, `3DSound.cpp:156` |
| `nBlockAlign` | `nChannels * wBitsPerSample / 8` | consumed verbatim by DirectSound; also drives stream notify sizing, `3DSound.cpp:1325-1329` |
| `nAvgBytesPerSec` | `nSamplesPerSec * nBlockAlign` | **must be correct** — it is the divisor in `Sound3D_GetSamplePlayTime`, `3DSound.cpp:1224` |
| `data` chunk | present, non-empty, length honest | `Sound.cpp:529`, `3DSound.cpp:1066-1075` |
| Extra chunks | allowed anywhere | `MMIO_FINDCHUNK` rescans |

A canonical 44-byte-header PCM WAV is exactly right. Do **not** emit `WAVE_FORMAT_EXTENSIBLE`
(`0xFFFE`), floating-point PCM (`0x0003`), ADPCM or anything else: nothing in our code rejects it,
so the failure surfaces as a generic `"Cannot create sound buffer."` warning three frames deep.

> A wrong `nAvgBytesPerSec` does not break playback — DirectSound uses `nSamplesPerSec` and
> `nBlockAlign` — but it does corrupt every duration the game computes. That duration is what gates
> `SFX_Random_SetAndPlayGlobalSample` (`SFX.cpp:322`, `globalSampleDuration = playTime * 25.0`), so
> a bad header can wedge the one-at-a-time stinger channel for minutes.

---

## 2. Mono vs stereo — the answer that matters

### 2.1 Yes, this is a DirectSound3D path, and it is not opt-in

Every non-streamed sample gets its buffer from `Sound3D_CreateSoundBuffer` (**OURS**,
`3DSound.cpp:1112`), and there is exactly one flag set in the whole engine:

```cpp
// 3DSound.cpp:1120-1128
std::memset(&dsbuf, 0, sizeof(DSBUFFERDESC));
dsbuf.dwSize = sizeof(DSBUFFERDESC);
// FIX APPLY: Use `DSBCAPS_GLOBALFOCUS` so that non-streamed sounds can play without window focus.
dsbuf.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRL3D |
                DSBCAPS_STATIC | DSBCAPS_MUTE3DATMAXDISTANCE | DSBCAPS_GLOBALFOCUS;
dsbuf.dwBufferBytes = sound->size;
dsbuf.lpwfxFormat   = sound->pwf;

if (lpDSnd()->CreateSoundBuffer(&dsbuf, &sound->lpDsb3D[0], nullptr) != DS_OK)
{
    Error_Warn(true, "Cannot create sound buffer.");
    return false;
}
```

There is no non-3D branch. There is no per-sample flag. `DSBCAPS_CTRL3D` is unconditional for
every sample loaded without the `@` modifier, and the field it lands in is even named
`lpDsb3D[SOUND3D_MAXSIMULTANEOUS]` (`3DSound.h:176`).

The 3D interface is then queried on **every** play, before the play mode is even considered
(`3DSound.cpp:549`):

```cpp
soundBuff->QueryInterface(IID_IDirectSound3DBuffer, (void**)&sound3DBuff);
```

### 2.2 The trap: "2D" cues use the same 3D buffer

`Sound3DPlay::Normal` — the mode behind `SFX_Random_PlaySoundNormal`, i.e. every menu click, every
advisor line, every non-positional stinger — does **not** use a different buffer. It uses the same
`DSBCAPS_CTRL3D` buffer and merely switches the 3D processing off (`3DSound.cpp:583-589`):

```cpp
else if (Sound3DPlay::Normal == play) {
    sound3DBuff->SetMode(DS3DMODE_DISABLE, DS3D_DEFERRED);
    Sound3D_CheckAlreadyExists(nullptr, sound3DBuff);
    Sound3D_AddSoundRecord(nullptr, soundBuff, sound3DBuff);
}
```

`DS3DMODE_DISABLE` turns off *spatialisation at play time*. It does nothing about the *buffer
creation* constraint, which was already applied at load time. **There is no such thing as a 2D
sample in this engine.** The distinction between "positional" and "non-positional" is a per-play
decision made against a buffer that is always 3D.

### 2.3 The constraint itself

DirectSound's documented rule is that applications "must supply monaural sound sources when using
the 3D capabilities" — attempting `DSBCAPS_CTRL3D` with a multi-channel WAV format is an error
([DirectSound 3D Buffers, Microsoft Learn](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee416765(v=vs.85))).

Combined with §2.1 and §2.2 this yields the rule:

> **Every WAV named in the `Samples` block without an `@` prefix must be single-channel.**
> This includes UI beeps, advisor speech, tutorial stingers and music-style loops — anything the
> game plays through an `SFX_*` cue.
>
> **Only `@`-streamed samples may be stereo.** Their buffer is created at
> `3DSound.cpp:1337-1338` with `DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY |
> DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS` — **no `DSBCAPS_CTRL3D`** — so the mono
> restriction does not apply to them.

### 2.4 What a stereo mistake looks like

The failure is quiet and it happens at level load, far from the sound that goes missing:

1. `CreateSoundBuffer` returns non-`DS_OK`; `Error_Warn(true, "Cannot create sound buffer.")`,
   `3DSound.cpp:1130`. **`Error_Warn` is a log level, not a stop.** It returns `false`.
2. `Sound3D_LoadSample` jumps to `err:` and logs `"Error loading sample."` (`3DSound.cpp:1100`).
   Note the two leaks on this path: `fileData` is only `Mem_Free`d on the success path
   (`3DSound.cpp:1094`), and `sound->pwf` is only released by `Sound3D_Remove` (`3DSound.cpp:383`).
3. `Sound3D_Load` falls through to `Error_WarnF(true, "Cannot load sound \"%s\".", fName)`
   (`3DSound.cpp:344`) and returns `-1` (`:347`).
4. `SFX_LoadSampleProperty` sets `success = false` **and keeps going** — there is deliberately no
   `break` (`SFX.cpp:192-195`, and the comment "surprisingly there is no break on failure").
   The cue's `sound3DHandle` keeps whatever it had, which after `SFX_Initialise`'s
   `std::memset(&sfxGlobs, 0, sizeof(sfxGlobs))` (`SFX.cpp:35`) is **`0`**.
5. `0` is then rejected by every play path's `if (handle > 0)` guard (`SFX.cpp:350`, `:431`).
   The cue exists, resolves by name, and is permanently silent.

There is no in-game symptom other than the absence of a sound. Check the log for
`Cannot load sound` before suspecting anything else.

### 2.5 A note on `DSBCAPS_CTRLPAN | DSBCAPS_CTRL3D`

`3DSound.cpp:1123` sets both. Some DirectX-era documentation states these are mutually exclusive;
the archived `DSBCAPS` flag table
([Microsoft Learn](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee416818(v=vs.85)))
does not say so. Empirically the combination is accepted — the 1999 build shipped this exact flag
set and had working sound, and if it were rejected *every* sample would fail to load. Nothing here
depends on resolving it. `SetPan` is never called anywhere in the codebase (grep: zero hits), so
`DSBCAPS_CTRLPAN` is dead weight either way. **UNDETERMINED**, and deliberately left alone.

---

## 3. File layout and the loose-file override

### 3.1 Where the engine looks

The path in the config is *relative to the data directory*, and the extension is appended for you.

```cpp
// 3DSound.cpp:280
std::sprintf(buffer, "%s.wav", fName);
// 3DSound.cpp:1057  (non-streamed)
if (fileData = File_LoadBinary(fName, &fileSize))
```

`File_LoadBinary` → `File_Load2` → `File_Open2(filename, "rb", FILE_FLAGS_DEFAULT)`
(`Files.cpp:1023, 1079, 1084`). `FILE_FLAGS_DEFAULT` is `FILE_FLAG_DATADIR | FILE_FLAG_DEFAULTPRIORITY`
(`Files.h:104`), so `File_VerifyFilename2` prepends the data directory and refuses anything that
escapes it (`Files.cpp:1184-1188`, and the containment check at `:1205`):

```cpp
case FileFlags::FILE_FLAG_DATADIR:
    fileDir = fileGlobs.dataDir;
    if (*temp == '\\') temp++;
    std::sprintf(part, "%s\\%s", fileDir, temp);
    break;
...
if (noVerify || std::strncmp(fileGlobs.s_VerifyFilename_full, fileDir, std::strlen(fileDir)) == 0)
    return fileGlobs.s_VerifyFilename_full;
```

`fileGlobs.dataDir` defaults to `<current working directory>\Data` (`Files.cpp:1397-1399`,
`FILE_DATADIRNAME` is `"Data"`, `Files.h:35`), overridable with `-datadir` (`Main.cpp:1219-1222`).

> **The exact directory.** A config value of `Sounds\DeepCore\tension1` resolves to
> `<game folder>\Data\Sounds\DeepCore\tension1.wav`.
> Backslashes. No extension in the config. No leading slash needed (one is stripped).
> `..\` will be rejected by the verify check.

### 3.2 WAD versus loose — the real priority order

`Files.cpp:31-32` is the comment in question, and the default is the opposite of what the comment
alone suggests:

```cpp
// Data directory has priority over loading from WAD files.
static bool _dataFirst = false;
```

The comment describes what the flag *means when true*; it ships **false**, confirmed at
`Main.cpp:800`:

```cpp
File_SetDataPriority(mainOptions.dataFirst.value_or(false));
```

The decision is made in `_File_CheckSystem2` (**OURS**, `Files.cpp:838`):

```cpp
// Files.cpp:877-893
/// COMMANDLINE OPTION: Data files have precedence over WAD files.
if (dataFirst && useStd && useWad) {            // only when -datafirst
    FILE* f;
    if (f = std::fopen(fName, mode)) { std::fclose(f); return FileSys::Standard; }
}

if (useWad && Wad_IsFileInWad(_File_GetWadName(fName), currWadHandle) != WAD_ERROR) {
    return FileSys::Wad;                        // <-- DEFAULT: the WAD copy wins
}
else if (useStd || useCD) {
    return FileSys::Standard;                   // <-- not in any WAD: loose file is used
}
```

with the same fast path repeated in `File_Open2` (`Files.cpp:391-400`). So:

| Situation | What happens | Flag needed |
|---|---|---|
| **New** filename, not present in any WAD | Loose file under `Data\` is loaded | **none** |
| Filename that also exists in a WAD | **WAD copy wins** and your loose file is ignored | `-datafirst` |
| You want to shadow a stock sound | | `-datafirst` (or `-nowad`, `Main.cpp:1297`) |
| Streamed (`@`) sample | Loose file / CD only, never a WAD | **none** (see §3.3) |

WADs are loaded as `<wadDir>\LegoRR0.wad` … `LegoRR9.wad` (`Files.cpp:96`, `MAX_WADS` is 10,
`Wad.h:35`) and searched **backwards** — `Wad_IsFileInWad` and `Wad_FileOpen` both loop
`for (i = MAX_WADS-1; i >= 0; i--)` (`Wad.cpp:328`, `:422`). A higher-numbered WAD therefore
shadows a lower one, which is a second, flagless override route: ship `LegoRR2.wad`.

`_File_GetWadName` (`Files.cpp:976`) strips the data-directory prefix set by
`File_SetBaseSearchPath(fileGlobs.dataDir)` (`Files.cpp:107`), so WAD entry names are the same
`Sounds\...\name.wav` relative paths. Matching is `_stricmp` (`Wad.cpp:346`).

### 3.3 Streamed samples bypass the file system entirely

This is not obvious and it bites. Both the load-time existence check and the play-time open use
raw `std::fopen`, not `File_Open`:

```cpp
// 3DSound.cpp:284-296  (Sound3D_Load, stream branch)
const char* hdFileName = File_VerifyFilename(buffer);
const char* useFile = nullptr;
char cdFileName[FILE_MAXPATH];
FILE *mfp;

if (mfp = std::fopen(hdFileName, "r")) { useFile = hdFileName; std::fclose(mfp); }
else { if (File_GetCDFilePath(cdFileName, buffer)) { useFile = cdFileName; } }
```

and again at play time (`3DSound.cpp:519-531`). `File_VerifyFilename` only builds and validates the
path string; it does not consult a WAD. **A sample marked `@` must exist as a loose file under
`Data\` (or on the CD).** Putting it in a WAD makes it fail to load, with
`Cannot load sound "..."` as the only clue.

Also note the mode is `"r"`, not `"rb"` (`3DSound.cpp:289`, `:303`), and the size used for duration
is the *whole file* including its RIFF header (`3DSound.cpp:305-308`), so streamed durations are
slightly over-reported.

---

## 4. The SFX registry

### 4.1 Registration

`SFX_Initialise` (**OURS**, `SFX.cpp:32`, hooked at `interop.cpp:4024`) zeroes the globals, allocates
the name table, and pre-registers 45 names:

```cpp
std::memset(&sfxGlobs, 0, sizeof(sfxGlobs));            // SFX.cpp:35
sfxGlobs.globalSampleSoundHandle = -1;                   // SFX.cpp:36
uint32 arraySize = SFX_MAXSAMPLES * sizeof(uint32);      // 495 * 4 = 1980   SFX.cpp:39
sfxGlobs.hashNameList = (uint32*)Gods98::Mem_Alloc(arraySize);
std::memset(sfxGlobs.hashNameList, 0, arraySize);        // SFX.cpp:43
SFX_RegisterName(SFX_NULL);                              // SFX.cpp:45
...
SFX_RegisterName(SFX_AmbientMusicLoop);                  // SFX.cpp:89   -- the 45th
Gods98::Container_SetSoundTriggerCallback(SFX_Container_SoundTriggerCallback, nullptr);  // SFX.cpp:91
```

`SFX_RegisterName` is a stringising macro (`SFX.h:136`):

```cpp
#define SFX_RegisterName(n)   (sfxGlobs.hashNameList[n] = Gods98::Util_HashString(#n, false, true))
```

The stored value is a hash of the literal identifier text — `"SFX_Drill"` etc. — and the array index
*is* the `SFX_ID` (`GameCommon.h:1369-1422`, `SFX_Preload_Count` = 45 at `:1421`).

`Util_HashString(str, bIgnoreBlanks=false, upperCase=true)` (**OURS**, `Utils.cpp:180`) upper-cases
each character before folding it in, so **cue names are case-insensitive** but whitespace is
significant.

### 4.2 Lookup, and dynamic registration

`SFX_GetType` (**OURS**, `SFX.cpp:112`) is both the resolver and the registrar:

```cpp
uint32 hashValue  = Gods98::Util_HashString(sfxName, false, true);
uint32 totalCount = sfxGlobs.hashNameCount + (uint32)SFX_ID::SFX_Preload_Count;   // + 45
for (uint32 i = 0; i < totalCount; i++) {
    if (hashValue == sfxGlobs.hashNameList[i]) { *sfxID = (SFX_ID)i; return true; }
}
if (sfxGlobs.flags & SFX_GlobFlags::SFX_GLOB_FLAG_POPULATEMODE) {
    *sfxID = (SFX_ID)totalCount;
    sfxGlobs.hashNameList[totalCount] = hashValue;    // SFX.cpp:129 -- unbounded write
    sfxGlobs.hashNameCount++;
    return true;
}
return false;
```

- **`POPULATEMODE` off** — pure lookup; unknown name returns `false`.
- **`POPULATEMODE` on** — an unknown name is *appended* and gets the next free ID.

`POPULATEMODE` is toggled only by `SFX_SetSamplePopulateMode` (**OURS**, `SFX.cpp:105`), which is
called only by `Lego_LoadSamples` — an **EXE address macro** (`Game.h:1914`), invoked from
`GameState.cpp:438`. So the window in which new names can be born is exactly the parse of the
`Samples` config block.

Matching is **hash equality only**; the name string is never compared. Two cue names that collide
in this 32-bit sum-of-products hash silently alias to the same ID. For hand-written names this is
a non-issue; for **machine-generated** cue names it is a real risk, and there is no diagnostic.

### 4.3 Slot budget

| Table | Size | Used at boot | Free | Citation |
|---|---|---|---|---|
| `hashNameList` | 495 (`SFX_MAXSAMPLES`) | 45 | **450** | `SFX.h:34`, `SFX.cpp:39-43` |
| `samplePropTable[]` | 495, indexed by `SFX_ID` | 45 | 450 | `SFX.h:96` |
| `sampleGroupTable[]` | 200 (`SFX_MAXSAMPLEGROUPS`) | 0 | 200 | `SFX.h:35,97` |
| `sound3DGlobs.soundTable[]` | 600 (`SOUND3D_MAXSAMPLES`) | 0 | 600 | `3DSound.h:54`, `:199` |

A cue consumes **one** name slot and **one** prop slot. It consumes **one `soundTable` slot per
comma-separated WAV**, and **one `sampleGroupTable` slot per WAV beyond the first**. A 5-way random
group is 1 name + 1 prop + 5 sound-table + 4 group entries.

The `sampleGroupTable` overflow is guarded (unconditionally) at `SFX.cpp:223-229`; the reasoning —
including that `sfxGlobs` ends exactly where `statsGlobs` begins — is in the comment block at
`SFX.cpp:202-222` and independently in `docs/ADDRESS-MAP.md:54,131`. **`hashNameList` is not
guarded**: name 451 is a 4-byte heap overflow at `SFX.cpp:129`. `soundTable` exhaustion is
`Error_Fatal(true, "Run out of samples - SOUND3D_MAXSAMPLES too small.")` at `3DSound.cpp:365`,
which terminates.

### 4.4 Can a new cue be added with zero C++?

**Yes.** The name is created by the act of naming it in the config, and every consumer resolves by
name at load time.

```
Lego* {
    Main {
        Samples {
            ; <CueName>       <wav path, no extension>[,<more paths>...]
            SND_Tension1      *#-600#Sounds\DeepCore\tension1
            SND_MonsterNear   *Sounds\DeepCore\near_a,*Sounds\DeepCore\near_b,*Sounds\DeepCore\near_c
            SND_AllClear      Sounds\DeepCore\allclear
            SND_LongAmbience  @Sounds\DeepCore\bed_stereo
        }
    }
}
```

Config syntax is the one documented in `data/Settings/DeepCore.cfg:1-5`: whitespace-separated
name/value, `{ }` nesting, `;` comments, case-insensitive. The `::`-joined string ID path comes from
`Config_ID` / `CONFIG_SEPARATOR` (`Config.h:336`, `:51`) and `Main_ID` is
`Config_ID(legoGlobs.gameName, "Main", ...)` (`Game.h:794`).

> **UNDETERMINED:** the exact block path `Lego*::Main::Samples` cannot be proven from this
> repository — `Lego_LoadSamples` is exe code (`Game.h:1914`) and no source file contains the
> literal `"Samples"` as a config key. The evidence for it is: (a) every other `Main`-level key
> goes through `Main_ID` (`GameState.cpp:400-451`); (b) our own comment at `SFX.cpp:205` speaks of
> "every `Samples` line in the config"; (c) `src/openlrr/game/README.md:8` describes `SFX.cpp` as
> handling "loading of `Samples` config block". Verify against the stock `Lego.cfg` before shipping
> a generator that writes this block.

**Value grammar** — parsed by `SFX_LoadSampleProperty` (**OURS**, `SFX.cpp:140`), which *is* ours,
so this part is certain:

| Prefix | Meaning | Citation |
|---|---|---|
| `*` | simultaneous: 3 duplicate buffers so the sample can overlap itself | `SFX.cpp:159-162`, `3DSound.cpp:1137-1148` |
| `#N#` | volume in millibels, `-10000 … 0`; anything else is clamped to `0` | `SFX.cpp:164-175`, `3DSound.cpp:330-333` |
| `@` | stream from disk instead of loading into memory | `SFX.cpp:177-180`, `3DSound.cpp:282` |
| `,` | separates alternates; one is chosen at random per play | `SFX.cpp:152`, `SFX.cpp:249-278` |

Order is fixed by the parser: `*` then `#N#` then `@` then the path.

**Four gotchas in that grammar, all verified in our code:**

1. **No whitespace trimming.** `Util_Tokenise` (`Utils.cpp:41`) splits on the literal `,` and does
   not trim. `a, b` yields a second token of `" b"`, which becomes the filename `" b.wav"` and
   fails. **Never put a space after a comma.**
2. **`stream` leaks across tokens.** `bool32 stream = false;` is declared *outside* the loop
   (`SFX.cpp:149`) while `simultaneous` and `volume` are declared inside (`:156-157`). One `@`
   anywhere in a line makes every *subsequent* alternate on that line streamed too.
3. **`char* sampleNames[100]` is unguarded** (`SFX.cpp:143`) and `Util_Tokenise` has no limit
   (`Utils.cpp:43` passes `uint32` max). 100 or more commas on one line smashes the stack before any
   bounds check runs. `Util_TokeniseSafe` exists (`Utils.cpp:47`) and is the drop-in fix.
4. **`#` with no closing `#` runs off the end** — `while (*s != '#') { *v++ = *s++; }` (`SFX.cpp:167-169`)
   into a 64-byte `volBuff`. The missing null terminator was already fixed (`SFX.cpp:172`); the
   scan bound was not.

**Zero-C++ consumers that resolve a cue by name** (all call `SFX_GetType`, all **OURS**):

| Consumer | Where the name comes from | Citation |
|---|---|---|
| LWS animation null node | the `.lws` scene file (§5) | `Lws.cpp:356` |
| Container activity `SAMPLE` key | an activity `.ae` block | `Containers.cpp:587`, `Activities.h:36`, fired at `Containers.cpp:2569-2575` |
| Object stats | `DrillSound`, `DrillFadeSound`, `EngineSound` | `Stats.cpp:298,301,305` |
| Priority buttons | priority config line, token 1 | `Priorities.cpp:61` |
| Advisor | advisor position config, token 2 | `Advisor.cpp:209` |
| Objectives | `objectiveGlobs.soundName` | `Objective.cpp:1006` |
| Text message panel | `textGlobs.textImagesSFX[...]` | `TextMessages.cpp:207` |
| Front end / menus | menu item config | `FrontEnd.cpp:2855, 3864, 3874` |
| **Optional hardcoded names** | `"SND_AirBeat"`, `"SND_Hurt"` — looked up by literal string and *skipped if absent* | `GameState.cpp:1715,1725,1729`; `Object.cpp:4049` |

That last row is worth calling out: `SND_AirBeat` (low-oxygen heartbeat) and `SND_Hurt` are
already wired into the game and cost **nothing but a `Samples` line and a WAV** to enable.

---

## 5. The LWS trigger

Confirmed, and it is entirely data-driven.

### 5.1 Detection, at scene parse

`Lws_Load` (**OURS**, `Lws.cpp`) flags any null object whose name begins with `SFX,`
(`Lws.cpp:102-110`):

```cpp
if (std::strcmp("AddNullObject", argv[0]) == 0) {
    currNode->flags |= Lws_NodeFlags::LWSNODE_FLAG_NULL;
    currNode->name = Util_StrCpy(&line[std::strlen("AddNullObject") + 1]);
    if (::_strnicmp(currNode->name, LWS_SOUNDTRIGGERPREFIX, std::strlen(LWS_SOUNDTRIGGERPREFIX)) == 0 &&
        ::_strnicmp(&currNode->name[std::strlen(LWS_SOUNDTRIGGERPREFIX)], LWS_SOUNDTRIGGERSEPERATOR, std::strlen(LWS_SOUNDTRIGGERSEPERATOR)) == 0) {
        currNode->flags |= Lws_NodeFlags::LWSNODE_FLAG_SOUNDTRIGGER;
        Error_Fatal(scene->triggerCount == 256, "Too many sound trigger frames");
        scene->triggerCount++;
    }
}
```

`LWS_SOUNDTRIGGERPREFIX` is `"SFX"` and `LWS_SOUNDTRIGGERSEPERATOR` is `","` (`Lws.h:59-60`). The
prefix test is `_strnicmp` — **case-insensitive**, so `sfx,` works too.

### 5.2 Parsing, at `Lws_SetupSoundTriggers`

`Lws_SetupSoundTriggers` (**OURS**, `Lws.cpp:329`) splits the node name on commas
(`Lws.cpp:349-377`):

```cpp
std::strcpy(line, node->name);
uint32 argc = Util_Tokenise(line, argv, LWS_SOUNDTRIGGERSEPERATOR);
st->count = (uint16)argc - 2;
Error_Fatal(st->count >= LWS_MAXTRIGGERKEYS, "LWS_MAXTRIGGERKEYS too small");
Error_Fatal(st->count == 0, "No trigger frames specified");

bool32 result = lwsGlobs.FindSFXIDFunc(argv[1], &st->sfxID);      // -> SFX_GetType
if (Graphics_IsReduceSamples()) {
    Error_WarnF(!result, "Cannot match sound with %s", argv[1]);
    if (!result) st->sfxID = 0;
} else {
    Error_FatalF(!result, "Cannot match sound with %s", argv[1]);  // <-- terminates
}

for (uint32 index = 0; index < st->count; index++) {
    const char* end = std::strstr(argv[index+2], "-");
    st->frameStartList[index] = std::atoi(argv[index+2]);
    if (end) st->frameEndList[index] = std::atoi(&end[1]);
    else     st->frameEndList[index] = st->frameStartList[index];
    st->loopUID[index] = -1;
}
```

`lwsGlobs.FindSFXIDFunc` is wired to `SFX_Callback_FindSFXIDFunc` → `SFX_GetType`
(`GameState.cpp:452`, `SFX.h:160`). Since `POPULATEMODE` is off by the time scenes load, **the cue
name must already exist in the `Samples` block** — otherwise `Error_FatalF` terminates the program
(unless `-reducesamples` is on, `Main.cpp:1041`, which downgrades it to a warning and substitutes
`SFX_NULL`).

### 5.3 Firing, per animation frame

`Lws_HandleTrigger` (**OURS**, `Lws.cpp:536`) is called for every node from `Lws_SetTime`
(`Lws.cpp:499`):

```cpp
for (uint16 loop = 0; loop < st->count; loop++) {
    bool32 loopMode = (st->frameStartList[loop] != st->frameEndList[loop]);

    if (loopMode && Lws_KeyPassed(scene, st->frameEndList[loop])) {
        Sound3D_StopSound(st->loopUID[loop]);
    }
    if (Lws_KeyPassedExclusive(scene, st->frameStartList[loop])) {
        if (lwsGlobs.SoundEnabledFunc()) {
            st->loopUID[loop] = lwsGlobs.PlaySample3DFunc(frame, st->sfxID, loopMode, true, nullptr);
        }
    }
}
```

`PlaySample3DFunc` is `SFX_Callback_PlaySample3DFunc` → `SFX_Random_PlaySound3DOnFrame(frame, id,
loop, onFrame = true, nullptr)` (`SFX.h:222`), i.e. `Sound3DPlay::OnFrame` — the sound is attached
to the animated node's frame and follows it (§6.4).

### 5.4 How a modder uses it

In the LightWave scene, add a **null object** whose name is:

```
SFX,<CueName>,<frame>[,<frame>...]
```

- `<CueName>` — a name already present in the `Samples` block (case-insensitive).
- `<frame>` — a single integer fires a **one-shot** at that frame.
- `<start>-<end>` — a range starts a **looping** play at `start` and stops it at `end`
  (`Lws.cpp:546`, `:549-551`).
- Between 1 and 24 frame entries per node (`LWS_MAXTRIGGERKEYS` is 25, `Lws.h:67`; the guard is
  `>= 25`).
- At most 256 trigger nodes per scene (`Lws.cpp:108`).

The resulting `.lws` line looks like `AddNullObject SFX,SND_Tension1,12` — the name is taken from
everything after `AddNullObject ` (`Lws.cpp:104`), so **no spaces anywhere in the trigger name**.

There is a second, parallel data-driven trigger with no LWS involvement: an activity's `SAMPLE` key
(`Containers.cpp:587`, `Activities.h:36`) fires `SFX_Container_SoundTriggerCallback` once when the
activity is set (`Containers.cpp:782` sets the flag, `:2569-2575` fires it), which plays the cue
positionally on the container (`SFX.cpp:95-102`).

---

## 6. What we can call

Every function in this section is **OURS** (implemented in this repo). Hook state is noted because
an unhooked function is still callable from DLL code — it simply means the exe's own call sites
still run 1999 code.

### 6.1 Game level — `LegoRR`, `game/audio/SFX.h`

| Signature | Impl | Hooked | Notes |
|---|---|---|---|
| `sint32 SFX_Random_PlaySoundNormal(SFX_ID sfxID, bool32 loop)` | `SFX.cpp:343` | `interop.cpp:4038` | **Non-positional.** Returns a *play UID* or `-1`. |
| `sint32 SFX_Random_PlaySound3DOnFrame(IDirect3DRMFrame3* frame, SFX_ID sfxID, bool32 loop, bool32 onFrame, OPTIONAL const Vector3F* wPos)` | `SFX.cpp:424` | `interop.cpp:4042` | **Positional.** `onFrame=true` follows the frame; `onFrame=false` plays at `wPos`. Returns `0` on the failure paths — *not* `-1`. |
| `sint32 SFX_Random_PlaySound3DOnContainer(Gods98::Container* cont, SFX_ID sfxID, bool32 loop, bool32 onCont, OPTIONAL const Vector3F* wPos)` | `SFX.cpp:413` | `interop.cpp:4041` | Wrapper; `cont` may be `nullptr` when using `wPos`. This is the ergonomic one. |
| `bool32 SFX_Random_SetAndPlayGlobalSample(SFX_ID sfxID, OPTIONAL OUT sint32* handle)` | `SFX.cpp:314` | `interop.cpp:4036` | **The self-gating stinger channel.** Refuses while `globalSampleDuration > 0`. |
| `void SFX_AddToQueue(SFX_ID sfxID, Gods98::SoundMode mode)` | `SFX.cpp:333` | `interop.cpp:4037` | Defers to the next `SFX_Update`. Capacity 10 (`SFX.cpp:335`). |
| `void SFX_StopGlobalSample(void)` | `SFX.cpp:293` | `interop.cpp:4034` | Cancels the stinger. |
| `void SFX_Sound3D_StopSound(sint32 handle)` | `SFX.cpp:483` | `interop.cpp:4044` | Takes a **play UID**. |
| `void SFX_Random_SetBufferVolume(SFX_ID sfxID, sint32 volume)` | `SFX.cpp:386` | `interop.cpp:4039` | Millibels `[-10000, 0]`. Affects **all** voices of that sample. |
| `sint32 SFX_Random_GetBufferVolume(SFX_ID sfxID)` | `SFX.cpp:397` | `interop.cpp:4040` | Returns `Sound3D_MinVolume()` (`-10000`) if the handle is bad. |
| `real32 SFX_Random_GetSamplePlayTime(SFX_ID sfxID)` | `SFX.cpp:473` | `interop.cpp:4043` | Seconds. `0.0f` if unresolved. |
| `bool32 SFX_GetType(OPTIONAL const char* sfxName, OUT SFX_ID* sfxID)` | `SFX.cpp:112` | `interop.cpp:4028` | Name → ID. |
| `bool32 SFX_IsSoundOn(void)` / `void SFX_SetSoundOn(bool32, bool32 stopAll)` | `SFX.cpp:596` / `:533` | `interop.cpp:4052` / `:4047` | Master gate. |
| `void SFX_Update(real32 elapsed)` | `SFX.cpp:489` | `interop.cpp:4045` | Already driven by the game loop; do not call again. |

### 6.2 Engine level — `Gods98`, `engine/audio/3DSound.h`

| Signature | Impl | Hooked |
|---|---|---|
| `sint32 Sound3D_Play2(Sound3DPlay play, IDirect3DRMFrame3* frame, sint32 soundTableIndex, bool32 loop, OPTIONAL const Vector3F* wPos)` | `3DSound.cpp:502` | `interop.cpp:212` |
| `sint32 Sound3D_Load(const char* fName, bool32 stream, bool32 simultaneous, sint32 volume)` | `3DSound.cpp:265` | `interop.cpp:193` |
| `bool32 Sound3D_Remove(sint32 soundTableIndex)` | `3DSound.cpp:371` | `interop.cpp:199` |
| `void Sound3D_StopSound(sint32 playUID)` / `void Sound3D_StopAllSounds(void)` | `3DSound.cpp:690` / `:704` | `interop.cpp:226` / `:230` |
| `void Sound3D_SetBufferVolume(sint32 soundTableIndex, sint32 mB)` / `Sound3D_GetBufferVolume` | `3DSound.cpp:628` / `:668` | `interop.cpp:218` / `:220` |
| `void Sound3D_SetGlobalVolume(sint32 mB)` / `…Prescaled(sint32 0..10)` / `…Real(real32 0..1)` | `3DSound.cpp:971` / `:1006` / `3DSound.h:390` | (`Prescaled` at `interop.cpp:254`) |
| `real32 Sound3D_GetSamplePlayTime(sint32 soundTableIndex)` | `3DSound.cpp:1212` | `interop.cpp:263` |
| `bool32 Sound3D_Stream_Play(const char* fullPath, bool32 loop, sint32 volume)` / `Sound3D_Stream_Stop(bool32 looping)` | `3DSound.cpp:1228` / `:1262` | — / `interop.cpp:268` |
| `void Sound3D_SetMinDistForAtten(real32)` / `Sound3D_SetMaxDist(real32)` / `Sound3D_SetRollOffFactor(real32)` | `3DSound.cpp:937` / `:954` / `:1584` | `interop.cpp:246` / `:248` / `:277` |
| `void Sound3D_Update(void)` | `3DSound.cpp:883` | `interop.cpp:240` |
| `bool32 Sound_PlayCDTrack(uint32 track, SoundMode mode, SoundCDStopCallback)` / `Sound_StopCD()` | `Sound.cpp:105` / `:133` | `interop.cpp:1444` / `:1445` |

Convenience macros (`3DSound.h:455-458`), all `Sound3D_Play2` in disguise:

```cpp
#define Sound3D_PlayOnFrame(frame, sound, loop)  Sound3D_Play2(Gods98::Sound3DPlay::OnFrame, (frame), (sound), (loop), nullptr)
#define Sound3D_PlayOnCont(cont, sound, loop)    Sound3D_Play2(Gods98::Sound3DPlay::OnFrame, (cont)->masterFrame, (sound), (loop), nullptr)
#define Sound3D_PlayOnPos(sound, loop, wPos)     Sound3D_Play2(Gods98::Sound3DPlay::OnPos,  nullptr, (sound), (loop), (wPos))
#define Sound3D_PlayNormal(sound, loop)          Sound3D_Play2(Gods98::Sound3DPlay::Normal, nullptr, (sound), (loop), nullptr)
```

### 6.3 Two handle spaces with identical-looking parameters

This is the sharpest trap in the API and it is not documented anywhere else in the tree.

| Handle | Meaning | Produced by | Consumed by |
|---|---|---|---|
| **sample handle** (table index) | index into `sound3DGlobs.soundTable[600]` | `Sound3D_Load` (`3DSound.cpp:338`), `SFX_Current_GetSound3DHandle` (`SFX.cpp:282`) | `Sound3D_SetBufferVolume` (`:632`), `Sound3D_GetBufferVolume` (`:672`), `Sound3D_GetSamplePlayTime` (`:1219`), `Sound3D_Remove` (`:377`), `Sound3D_Play2`'s 3rd argument (`:514`) |
| **play UID** | `soundTableIndex * 3 + voice` | `Sound3D_Play2` return (`3DSound.cpp:602`), `SFX_Random_Play*` returns | `Sound3D_StopSound` → `Sound3D_GetSoundBuffer` (`:684-686`) |

```cpp
// 3DSound.cpp:602  -- the encoding
return ((soundTableIndex * SOUND3D_MAXSIMULTANEOUS) + sound->voice);

// 3DSound.cpp:684-686  -- the decoding
uint32 remainder = soundHandle % SOUND3D_MAXSIMULTANEOUS /*3*/;
return sound3DGlobs.soundTable[(soundHandle - remainder) / SOUND3D_MAXSIMULTANEOUS].lpDsb3D[remainder];
```

Passing a play UID to `Sound3D_SetBufferVolume` indexes the wrong sample (and, above 199, walks
past `soundTable` into `intialised` / `windowsVolume` / `updateFrameList`). Passing a sample handle
to `Sound3D_StopSound` stops a different sample's voice. The `SFX_*` wrappers get this right; new
code must too.

### 6.4 Per-frame safety

| Call | Per-frame? | Why |
|---|---|---|
| `SFX_Random_SetAndPlayGlobalSample` | **YES** | Self-gating: refuses while `globalSampleDuration > 0` (`SFX.cpp:316`), and `SFX_Update` decrements it (`SFX.cpp:493`). This is the built-in anti-spam channel. |
| `SFX_Random_SetBufferVolume` / `GetBufferVolume` | **YES** | Three `SetVolume` calls at most (`3DSound.cpp:654-663`). No allocation. |
| `SFX_Random_GetSamplePlayTime`, `SFX_IsSoundOn`, `SFX_GetType` | **YES** | Pure reads (`SFX_GetType` walks ≤495 uint32s). |
| `SFX_Sound3D_StopSound` | **YES** | One `IDirectSoundBuffer::Stop` (`3DSound.cpp:699`). |
| `SFX_AddToQueue` | **Yes, but** | Silently drops past 10 entries per tick (`SFX.cpp:335`). |
| **`SFX_Random_PlaySoundNormal` / `PlaySound3DOn*`** | **NO — gate them** | See below. |

Three independent reasons the play calls are not per-frame safe:

1. **Allocation per play.** `Sound3D_AddSoundRecord` `Mem_Alloc`s a `Sound3D_SoundRecord`
   (`3DSound.cpp:614`) on every play. It is freed only when `Sound3D_Update` observes the buffer
   has stopped (`3DSound.cpp:897-899`). For a **looping** sound that never happens — the list grows
   without bound.
2. **Restart, not overlap.** A non-`*` sample has one buffer; `Sound3D_Play2` does
   `soundBuff->SetCurrentPosition(0)` then `Play` (`3DSound.cpp:594-598`). Calling it every frame
   restarts the sample every frame: you hear a click, not a sound. Even a `*` sample only has
   `SOUND3D_MAXSIMULTANEOUS = 3` voices (`3DSound.h:56`) and round-robins over them
   (`3DSound.cpp:542-544`), stealing a live voice on the 4th call.
3. **COM ref churn.** Each play does `QueryInterface(IID_IDirectSound3DBuffer, …)`
   (`3DSound.cpp:549`); the matching `Release` happens in `Sound3D_RemoveSound` (`:780`), which is
   again only reached once the buffer stops.

**The correct shape for a looping sound owned by our code** (the pattern used at `Smoke.cpp:373`):

```cpp
// somewhere DLL-side; NOT in the exe's data segment (CARDINAL RULE)
static sint32 _bedPlayUID = -1;   // play UID space, see §6.3

void MyFeature_Update(bool wantBed, const Vector3F* at)
{
    using namespace LegoRR;   // needed if any exe address macro is used nearby

    if (wantBed && _bedPlayUID <= 0) {
        SFX_ID id;
        if (SFX_GetType("SND_Tension1", &id)) {            // SFX.cpp:112
            // onCont=false + wPos => Sound3DPlay::OnPos, no container required
            const sint32 uid = SFX_Random_PlaySound3DOnContainer(nullptr, id, /*loop*/true,
                                                                 /*onCont*/false, at);  // SFX.cpp:413
            if (uid > 0) _bedPlayUID = uid;                 // 0 and -1 are both "failed"
        }
    }
    else if (!wantBed && _bedPlayUID > 0) {
        SFX_Sound3D_StopSound(_bedPlayUID);                 // SFX.cpp:483, play-UID space
        _bedPlayUID = -1;
    }
}
```

Note `uid > 0`, not `uid != -1`: `SFX_Random_PlaySound3DOnFrame` returns `0` on several failure
paths (`SFX.cpp:468`) while `SFX_Random_PlaySoundNormal` returns `-1` (`SFX.cpp:381`).

**How to play a named cue, positionally and non-positionally:**

```cpp
SFX_ID id;
if (SFX_GetType("SND_AllClear", &id)) {
    // non-positional (UI / stinger):
    SFX_Random_PlaySoundNormal(id, false);                                   // SFX.cpp:343
    // positional at a world point:
    SFX_Random_PlaySound3DOnContainer(nullptr, id, false, false, &wPos);     // SFX.cpp:413
    // positional, following an object:
    SFX_Random_PlaySound3DOnContainer(cont, id, false, true, nullptr);       // SFX.cpp:413
    // self-gating stinger (safe to spam):
    SFX_Random_SetAndPlayGlobalSample(id, nullptr);                          // SFX.cpp:314
}
```

---

## 7. Limits and hazards

### 7.1 Hard caps

| Cap | Value | Behaviour at the limit | Citation |
|---|---|---|---|
| WAV files loaded | 600 | **`Error_Fatal` → terminate** | `3DSound.h:54`, `3DSound.cpp:355-366` |
| Cue names | 495 total, 45 preloaded | **unguarded 4-byte heap overflow at name 451** | `SFX.h:34`, `SFX.cpp:39,129` |
| Group entries | 200 | guarded: warns, ignores extras, `success=false` | `SFX.h:35`, `SFX.cpp:223-229` |
| Voices per sample | 3 (`*` only; 1 otherwise) | oldest voice is stolen, silently | `3DSound.h:56`, `3DSound.cpp:540-547` |
| Position-tracked frames | 20 | **silently dropped** — the 21st `OnFrame` sound will not follow its object | `3DSound.h:58`, `3DSound.cpp:721-730` |
| Concurrent streams | 1 looping + 1 non-looping, **globally** | a new stream stops the old one | `3DSound.cpp:1234-1241` |
| Sound queue | 10 per tick | silently dropped | `SFX.cpp:335`, `SFX.h:102` |
| SFX instance queue | 10 | silently dropped | `SFX.cpp:359,445` |
| LWS trigger nodes | 256 per scene | `Error_Fatal` | `Lws.cpp:108` |
| LWS frame keys | 24 per node | `Error_Fatal` | `Lws.h:67`, `Lws.cpp:352` |
| Comma tokens per `Samples` line | **100, unguarded** | stack overflow | `SFX.cpp:143`, `Utils.cpp:43` |

### 7.2 Streaming versus in-memory

| | In-memory (default) | Streamed (`@`) |
|---|---|---|
| Buffer | one `DSBCAPS_STATIC` buffer sized to the whole `data` chunk | 3-second circular buffer in 4 notification blocks |
| Flags | includes `DSBCAPS_CTRL3D` → **mono only** | no `CTRL3D` → **stereo allowed** |
| Positional | yes (`OnFrame`, `OnPos`) | **no** — `Error_Fatal` then plays non-positionally anyway |
| Simultaneous (`*`) | yes, 3 voices | **no** — `Error_Fatal` then loads as a stream anyway |
| File source | WAD, loose `Data\`, or CD | **loose `Data\` or CD only** |
| Concurrency | up to 600 loaded, 3 voices each | 1 looping + 1 non-looping, globally |
| Citations | `3DSound.cpp:1112-1151` | `3DSound.cpp:282-316`, `:1290-1376`, `:1447-1575` |

Stream buffer sizing (`3DSound.cpp:1325-1330`) is `nSamplesPerSec * 3 * nBlockAlign`, rounded up to
a multiple of `nBlockAlign`, divided into `SOUND3D_NUMOFPLAYNOTIFICATIONS = 4` blocks
(`3DSound.h:62`). Refill is **polled**, not notified — `Sound3D_Stream_CheckPosition` runs from
`Sound3D_Update` (`3DSound.cpp:904-905`), which runs from `SFX_Sound3D_Update` ← `SFX_Update`. A
long frame hitch can therefore under-run a stream.

### 7.3 Volume and pan ranges

- **Volume is millibels**, `DSBVOLUME_MIN = -10000` … `DSBVOLUME_MAX = 0`, asserted at
  `3DSound.cpp:1607` and `:1617`. Out-of-range values are warned and replaced with `0` (max)
  at load (`3DSound.cpp:330-333`) or with `0` at runtime (`3DSound.cpp:633`).
- Millibels are **not linear**; use the conversion helpers `Sound3D_VolumeToReal` /
  `Sound3D_VolumeFromReal` (`3DSound.cpp:62`, `:75`) or the `…Real` wrappers (`3DSound.h:322-325`,
  `:390`) if you want a 0…1 knob.
- **Global volume** scales every sample (`Sound3D_SetGlobalVolume`, `3DSound.cpp:971`, which
  re-walks all 600 slots) and both streams. `Sound3D_SetGlobalVolumePrescaled` takes an integer
  0…10 (`3DSound.cpp:1006-1018`).
- `Sound3DPlay::Normal` applies a **fixed −800 mB duck** to non-positional plays
  (`Sound3D_GetFinalBufferVolume`, `3DSound.cpp:103-121`). Budget for it: a "2D" cue is ~8 dB
  quieter than the same sample played positionally.
- **Pan is unavailable.** `DSBCAPS_CTRLPAN` is requested but `SetPan` is never called anywhere.
- Default global volume is **max**, not the vanilla `-300` (`3DSound.cpp:1027-1031`).

### 7.4 Hazards a generator or a new cue can trip

1. **Stereo on a non-`@` sample** — §2.4. The single most likely generator bug.
2. **Sample-table index 0 is unreachable through `SFX_*`.** Every SFX play path guards with
   `if (handle > 0)` (`SFX.cpp:350`, `:390`, `:401`, `:431`, `:476`) where `handle` is a
   *sample-table index* returned by `Sound3D_Load` — and `0` is a perfectly valid index, handed to
   the **first** WAV loaded in the whole session. That first sample is therefore silent through the
   game layer (it still works through `Sound3D_Play2` directly). The guard exists because `0` is
   also the zeroed-out "never set" value from `SFX_Initialise` (`SFX.cpp:35`), so it cannot simply
   be removed. **Practical rule: never make your new cue the first line of the `Samples` block.**
   Whether stock data already parks something harmless at index 0 is **UNDETERMINED** — the block
   order lives in `Lego.cfg`, which is not in this repository.
3. **`Error_Fatal` does not stop anything here.** Two places log fatal and then carry on with the
   invalid state: `3DSound.cpp:300` (`"Cannot have a multi streaming sound!"` — then loads the
   stream and ignores `simultaneous`) and `3DSound.cpp:523`
   (`"Can only play a streaming sound noramlly, not 3D."` — then plays the stream anyway,
   hijacking the single global stream buffer). An LWS trigger pointing at an `@` cue will therefore
   *seem* to work while stomping whatever stream was playing. Per the house rule
   (`Error_FatalF2`, `Errors.h:111`, gated on `errorLogLevels.fatalVisible` which defaults `true`,
   `Errors.cpp:26`), these are reports, not guarantees.
4. **Hash collisions alias cue names silently** — §4.2. Only the 32-bit hash is compared.
5. **`Sound3D_Remove` recycles slots.** It clears `SOUND3D_FLAG_USED` but leaves
   `SOUND3D_FLAG_ACTIVE` set (`3DSound.cpp:379`), so a stale sample handle kept across a level
   transition can address a *different* sample later. NERPs does exactly this teardown per level
   (`NERPsFile.cpp:312-321`).
6. **Failed loads leak.** `fileData` (`Mem_Alloc`) and `sound->pwf` (`GlobalAlloc`) are not freed on
   the `err:` path of `Sound3D_LoadSample` (`3DSound.cpp:1099-1108`). A sound pack with many broken
   WAVs leaks proportionally at load time.
7. **A second name registry exists.** NERPs message files use `$key  Path\To\file` and
   `#key#` references (`NERPsFile.cpp:211-251`, `:1051-1095`). These bypass `sfxGlobs` entirely and
   are **always loaded with `stream = true`** (`NERPsFile.cpp:236`), so those WAVs must be loose
   files — and, being streamed, they may be stereo.
8. **The primary mixer runs at 22050 Hz / 16-bit / stereo** (`3DSound.cpp:152-161`), despite the
   comment saying 44.1 kHz. Source WAVs at other rates are resampled by DirectSound; matching 22050
   avoids that.

---

## 8. DECISION

### 8.1 The spec a generated audio file must meet

**Normal cues — anything a `Samples` line names without `@`:**

```
Container ......... RIFF / WAVE
Format tag ........ 1  (WAVE_FORMAT_PCM)  -- not 0xFFFE, not float, not ADPCM
Channels .......... 1  (MONO)             -- MANDATORY, non-negotiable
Bit depth ......... 16-bit signed LE      (8-bit unsigned also loads)
Sample rate ....... 22050 Hz              (any rate loads; 22050 matches the mixer)
nBlockAlign ....... 2                     (= channels * bits/8)
nAvgBytesPerSec ... 44100                 (= rate * blockAlign; MUST be correct)
fmt chunk ......... >= 16 bytes, standard 16-byte PCM fmt is ideal
data chunk ........ present, non-empty, length must match the bytes actually there
Extra chunks ...... permitted anywhere (LIST/INFO, fact, cue ... are skipped)
Filename .......... <name>.wav            -- extension appended by the loader, do not
                                             put it in the config
Directory ......... <game folder>\Data\<the exact path written in the config>\<name>.wav
Naming ............ no spaces, no commas, no leading '*', '#' or '@' in the path itself
```

**Streamed cues — a `Samples` line prefixed `@`, and every NERPs `$key` sound:**

```
Same as above EXCEPT:
Channels .......... 1 or 2 (stereo permitted -- no DSBCAPS_CTRL3D on this buffer)
Format tag ........ 1 (WAVE_FORMAT_PCM) -- explicitly enforced, 3DSound.cpp:1308
Location .......... MUST be a loose file under Data\ (or on the CD). Never a WAD.
Restrictions ...... cannot be positional, cannot be simultaneous, one looping + one
                    non-looping stream at a time in the whole game
```

Concretely, for a generator: **`ffmpeg -i in.wav -ac 1 -ar 22050 -sample_fmt s16 -f wav out.wav`**
for a normal cue; drop `-ac 1` only for `@` cues.

### 8.2 The exact steps to register a new cue

1. **Write the WAV** to the spec above, at
   `<game folder>\Data\Sounds\<YourPack>\<name>.wav`.
   (Or into a `LegoRR2.wad`+ — higher-numbered WADs shadow lower ones, `Wad.cpp:328`.)
2. **Add one line to the `Samples` block** — believed `Lego*::Main::Samples`, **UNDETERMINED**,
   verify against the stock `Lego.cfg`:
   ```
   SND_MyCue     *#-600#Sounds\YourPack\name
   ```
   `*` = 3 overlapping voices, `#-600#` = −6 dB, path without extension, **no spaces**.
   Do not make it the first line in the block (§7.4 item 2).
3. **Do not** shadow an existing filename unless you also pass `-datafirst` (or `-nowad`) —
   by default a WAD copy of the same relative path wins (`Files.cpp:886-889`).
4. **Trigger it**, choosing one, all zero-C++:
   - **animation-driven:** add a null object named `SFX,SND_MyCue,<frame>` (or
     `SFX,SND_MyCue,<start>-<end>` to loop) to the `.lws` scene;
   - **activity-driven:** add `SAMPLE  SND_MyCue` to the activity block in the `.ae`;
   - **stats-driven:** `DrillSound` / `DrillFadeSound` / `EngineSound` on an object;
   - **already-wired optional names:** call it `SND_AirBeat` or `SND_Hurt` and it fires with no
     further wiring at all (`GameState.cpp:1715`, `Object.cpp:4049`);
   - **or** from our own C++: `SFX_GetType("SND_MyCue", &id)` then one of §6.4's calls.
5. **Verify** in the log. Absence of `Cannot load sound "…"` and `Cannot create sound buffer.`
   is the only confirmation available without running the game.

### 8.3 Ranked plan — what to do about what this audit found

| # | Item | Severity | Cost | Risk | Files (all **OURS**) |
|---|---|---|---|---|---|
| 1 | **Bound `hashNameList`.** `SFX.cpp:129` writes `hashNameList[totalCount]` with no check; name 451 is a heap overflow. Mirror the existing `sampleGroupTable` guard exactly (`SFX.cpp:223-229`): warn, return `false`, let the caller's existing `!result` paths handle it. | High | ~10 lines | Low — callers already handle `false` (`Stats.cpp:298-305`, `Lws.cpp:356-365`) | `game/audio/SFX.cpp` |
| 2 | **Bound the `Samples` token list.** `SFX.cpp:152` → `Util_TokeniseSafe(value, sampleNames, ",", _countof(sampleNames))` (`Utils.cpp:47`). One-word change; removes a stack smash. | High | 1 line | None | `game/audio/SFX.cpp` |
| 3 | **Detect stereo at load and say so.** Add, in `Sound3D_CreateSoundBuffer` before `CreateSoundBuffer`, an explicit `sound->pwf->nChannels != 1` warning naming the file. Turns the single most likely modder/generator error from a mute mystery into a named log line. | High | ~6 lines | None — it is a diagnostic, control flow unchanged | `engine/audio/3DSound.cpp` |
| 4 | **Bound the `#N#` volume scan.** `SFX.cpp:167` runs to the end of the string if the closing `#` is missing, into a 64-byte buffer. Add `&& *s != '\0' && v < volBuff + sizeof(volBuff) - 1`. | Medium | 1 line | None | `game/audio/SFX.cpp` |
| 5 | **Hoist `stream` into the token loop.** `SFX.cpp:149` — declaring it beside `simultaneous`/`volume` (`:156-157`) makes `@` per-alternate instead of sticky. This *changes behaviour*, so it needs a decision, not just a fix. | Medium | 1 line | Behavioural — a stock line relying on the sticky bug would change | `game/audio/SFX.cpp` |
| 6 | **Document the two handle spaces** at the declarations (`3DSound.h:313,319,329,332,409`) — rename the parameters to `soundTableIndex` / `playUID` to match `Sound3D_Play2`'s existing naming. Pure comment/parameter-name change. | Medium | ~10 lines | None | `engine/audio/3DSound.h` |
| 7 | **Free `fileData`/`pwf` on the `err:` path** of `Sound3D_LoadSample` (`3DSound.cpp:1099-1108`). | Low | ~5 lines | Low | `engine/audio/3DSound.cpp` |
| 8 | **Resolve the `Samples` block path** against a real `Lego.cfg` and delete the UNDETERMINED marker in §4.4 / §8.2. Cannot be done from this repository. | Blocking for a generator | — | — | (data, not code) |

Items 1–4 and 7 add no warnings and change no behaviour that anything can legitimately depend on;
they are the same class of unconditional guard already accepted for `sampleGroupTable`
(`docs/WORKLOG.md:138-143`). Item 5 is the only one that alters semantics and should be Pierce's
call. Item 8 is the only thing standing between this document and a fully specified generator.

### 8.4 Explicitly UNDETERMINED

- The literal config path of the `Samples` block (`Lego*::Main::Samples` is inference, §4.4).
- Whether the vanilla `Samples` block already parks a throwaway sample at sound-table index 0
  (§7.4 item 2).
- Whether `DSBCAPS_CTRLPAN | DSBCAPS_CTRL3D` is formally legal; empirically it is accepted (§2.5).
- Everything about actual audible behaviour. Compile-verified is the ceiling; we cannot run the
  game.

---

*Sources for the DirectSound constraints in §2:
[DirectSound 3D Buffers](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee416765(v=vs.85)),
[DSBCAPS Structure](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee416818(v=vs.85)),
[DSBUFFERDESC Structure](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee416820(v=vs.85)).*
