# ListSet creator/destroyer audit — the precondition for PERFORMANCE item A1

<!-- Scope of "verified": every claim below is either (a) a line of source in this tree, cited
     file:line, or (b) a mechanical count taken from the tree with a stated command. Nothing
     here was observed in a running game; there is no installation of the original game on this
     machine and no LegoRR.exe in this repository, so nothing in the 1999 binary was
     disassembled. Where a claim depends on the binary it is marked UNDETERMINED and stays that
     way. See §7 for exactly what that leaves unproven and why it does not block the decision. -->

Status: **research note**, answering the question the source itself asks at
`src/openlrr/game/GameState.cpp:1792-1795`:

> `CountAlive()` can only be used when we are SURE we've hooked
> and replaced all functions creating and removing listset items.

That doubt gates `docs/PERFORMANCE.md` §5 item **A1** — a DLL-side dense index of live pointers
maintained in `Add`/`Remove`, replacing O(capacity) enumeration with O(alive). A1 is the root
fix in that document: eight-plus full walks per frame collapse from `2^listCount − 1` slots at a
1036-byte stride to the live count.

**Headline: the audit passes on hooks and fails on lifecycle.** Every creator and destroyer of
every listSet item in this tree is implemented C++, and for eight of the nine exe-backed
listSets the creator/destroyer pair is reachable from 1999 machine code *only* through a hooked
address. But that was never the whole precondition. `Initialise` and `Shutdown` bypass
`Remove` **by documented design** (`ListSet.hpp:680-682`) and neither resets `m_aliveCount`
(`ListSet.hpp:669-695`), so `CountAlive()` is **already wrong today** on at least three
listSets, for reasons that have nothing to do with hooking. The comment at `GameState.cpp:1792`
asked the right question and would have got a misleading "yes".

---

## 0. What was audited, and the method

### 0.1 The tree of listSets — enumerated, not assumed

A listSet container is defined structurally by `ListSet.hpp:12-23`: a type with
`TItem* listSet[MAXLISTS]`, `TItem* freeList`, `uint32 listCount`, whose item type carries
`TItem* nextFree`. Enumerating every such type mechanically:

```
grep -rn "WrapperCollection\|ListSet::Collection" --include=*.h --include=*.hpp --include=*.cpp src/
grep -rn "freeList\|nextFree" --include=*.h --include=*.hpp --include=*.cpp src/
```

yields **exactly ten**, nine wrapping exe-overlaid globs and one wholly DLL-side. Both greps
agree, and the ten match the nine shown by the `[F8]` debug overlay
(`GameState.cpp:1788-1801`) plus `Stats.cpp`'s private one.

| # | listSet | Container type | Glob binding | `MAXLISTS` | Capacity ceiling |
| ---: | --- | --- | --- | ---: | ---: |
| 1 | `objectListSet` | `LegoObject_Globs` | `Object.cpp:56` @ `0x004df790` | 32 (`Object.h:48`) | 2³²−1 |
| 2 | `aiListSet` | `AITask_Globs` | `AITask.cpp:17` @ `0x004b41c8` | 12 (`AITask.h:30`) | 4095 |
| 3 | `efenceListSet` | `ElectricFence_Globs` | `ElectricFence.cpp:22` @ `0x004c8df8` | 32 (`ElectricFence.h:30`) | 2³²−1 |
| 4 | `configListSet` | `Config_Globs` | `Config.cpp:21` @ `0x00507098` | 32 (`Config.h:49`) | 2³²−1 |
| 5 | `containerListSet` | `Container_Globs` | `Containers.cpp:44` @ `0x0076bd80` | 20 (`Containers.h:85`) | 1,048,575 |
| 6 | `fontListSet` | `Font_Globs` | `Fonts.cpp:19` @ `0x00507528` | 32 (`Fonts.h:45`) | 2³²−1 |
| 7 | `imageListSet` | `Image_Globs` | `Images.cpp:26` @ `0x00534908` | 32 (`Images.h:61`) | 2³²−1 |
| 8 | `meshListSet` | `Mesh_Globs` | `Mesh.cpp:29` @ `0x005353c0` | 20 (`Mesh.h:92`) | 1,048,575 |
| 9 | `viewportListSet` | `Viewport_Globs` | `Viewports.cpp:25` @ `0x0076bce0` | 32 (`Viewports.h:46`) | 2³²−1 |
| 10 | `_modifiedStatsListSet` | `ListSet::BasicContainer` | `Stats.cpp:58` — **DLL-side, no exe address** | 32 (`Stats.cpp:30`) | 2³²−1 |

### 0.2 Why hooking the entry point is the decisive test, not enumerating callers

The brief asks for every caller of every `Add`/`Remove`. That list is worth having and §2 gives
it — but it is not what makes a listSet safe, and reasoning from it alone would give the wrong
answer in both directions.

`hook_write_jmpret` (`hook.cpp:36-50`) overwrites a function's prologue with `E9 rel32` + `C3`.
Every `CALL` to that address — from our C++, from 1999 machine code, from a function pointer
stored in exe data — lands in the C++ body. There is no trampoline: every restore path is
commented out (`hook.cpp:30-32`, `:47-49`) and the handoff counts 1515 installations with zero
backup buffers (`docs/HANDOFF-2026-07-30.md` §2). So:

> **If the function that calls `listSet.Add()` / `listSet.Remove()` is itself hooked at its exe
> address, the caller set is irrelevant.** Every possible caller, ours or exe, executes our C++.

Conversely, a creator that is *not* hooked is safe only if every exe function that can reach it
is hooked — which requires knowing the exe's call graph, which requires the binary, which this
repository does not contain (`bin/` holds only `OpenLRR*.exe` and `harness.exe`; the
`No_game_binaries` CI gate forbids tracking any game binary — `docs/HANDOFF-2026-07-30.md` §1).

So the audit runs in three tiers, in this order:

- **Tier 1 — hooked creator/destroyer.** Proof-grade. Nothing else needs checking.
- **Tier 2 — unhooked creator/destroyer, all C++ callers hooked.** Depends on the
  `// used by:` cross-reference comments in `interop.cpp` being a complete xref of the 1999
  binary. Those comments are the OpenLRR decompilation's own record; they are good evidence and
  they are not proof, and we cannot upgrade them here.
- **Tier 3 — direct manipulation of `listSet` / `freeList` / `nextFree` outside `ListSet.hpp`.**
  Checked by grep; result in §3.

---

## 1. Tier-3 result first: direct field manipulation

```
grep -rn "\.listCount\|->listCount\|\.listSet\[\|->listSet\[\|listSet =" \
     --include=*.cpp --include=*.h --include=*.hpp src/ | grep -v ListSet.hpp
   -> (no matches)

grep -rn "freeList\|nextFree" --include=*.cpp --include=*.h --include=*.hpp src/ | grep -v ListSet.hpp
   -> 24 matches: 18 are struct field declarations, 3 are comments,
      1 is a commented-out line (Images.cpp:787),
      2 are ListSet::MemCopy notes (Mesh.cpp:187, Viewports.cpp:100),
      1 is a live assignment (Images.cpp:792).
```

**No C++ in this tree writes `listSet[]`, `listCount` or `freeList` outside `ListSet.hpp`.**
That is a clean result and it is the strongest single fact in this document.

There is **exactly one** live write to `nextFree` outside `ListSet.hpp`:

```cpp
// engine/drawing/Images.cpp:779-800  — Gods98::Image_InitFromSurface, OURS
	// This serves no purpose, as only existing images (or images stored outside the listSet) would be passed here.
	//if (imageGlobs.freeList == nullptr) imageListSet.AddList();

	/// FIXME: Figure out how best to handle re-assignment of nextFree here.
	///        Are there any checks where this could cause issues if left as is or if changed?
	///        How should items not stored in the listSet have this field assigned?
	newImage->nextFree = newImage;                    // <-- Images.cpp:792
```

`nextFree == self` is precisely the liveness marker (`ListSet.hpp:180-183`). This line therefore
**marks an item alive without incrementing `m_aliveCount`** — the exact divergence class A1
must not have. Assessed:

- Its only caller is `Image_GetScreenshot` (`Images.cpp:775`), which is hooked
  (`interop.cpp:986` @ `0x0047e5c0`). `Image_InitFromSurface` itself is *not* hooked
  (`interop.cpp:989`).
- Its only call site in the game passes `&rewardGlobs.current.saveCaptureImage`
  (`Objective.cpp:685`) — an `Image` **embedded by value in a glob struct**, not a listSet slot.

So today it flips liveness on an object that is not in `imageListSet`, and `CountAlive()` is
unaffected. But it is a loaded gun: the source's own `FIXME` says nobody knows what should
happen if a listSet item is ever passed here. **A dense index must therefore key on slot
membership (`WrapperCollection::Contains`, `ListSet.hpp:631-634`), never assume an alive-marked
`Image` is indexed.** Noted as a permanent hazard, not a current defect.

---

## 2. Per-listSet: entry points, callers, verdicts

Legend: **OURS** = implemented C++ in this tree. **EXE** = raw address macro, 1999 machine code.
**HOOKED** = `hook_write_jmpret` installed at the exe address, so all exe callers land in OURS.

### 2.1 `objectListSet` — the one that matters

| Role | Function | Owner | Exe addr | Hook |
| --- | --- | --- | --- | --- |
| Add | `LegoObject_Create_internal` (`Object.cpp:1157-1166`, `Add()` at `:1160`) | OURS | `0x00438580` | **HOOKED** `interop.cpp:3572` |
| Add (public) | `LegoObject_Create` (`Object.h:665`) | OURS | `0x00437fc0` | **HOOKED** `interop.cpp:3570` |
| Remove | `LegoObject_Remove` (`Object.cpp:438-618`, `Remove()` at `:615`) | OURS | `0x00437800` | **HOOKED** `interop.cpp:3550` |
| Remove (bulk) | `LegoObject_RemoveAll` (`Object.cpp:1716-1731`) | OURS | `0x0043b5e0` | **HOOKED** `interop.cpp:3615` |
| Remove (callback) | `LegoObject_Callback_Remove` (`Object.cpp:1735-1740`) | OURS | `0x0043b610` | not hooked; body is a one-line wrapper around the hooked `LegoObject_Remove` (`Object.cpp:1738`) |
| AddList | `LegoObject_AddList` (`Object.cpp:1170-1174`) | OURS | `0x004385d0` | **not hooked** (`interop.cpp:3575`, "internal") |
| Initialise | `LegoObject_Initialise` (`Object.cpp:208`) | OURS | `0x00436ee0` | **HOOKED** `interop.cpp:3517` |
| Shutdown | `LegoObject_Shutdown` (`Object.cpp:213-227`) | OURS | `0x00437310` | **HOOKED** `interop.cpp:3520` |
| Level cleanup | `LegoObject_CleanupLevel` (`Object.cpp:317-`) | OURS | `0x00437560` | **HOOKED** `interop.cpp:3531` |

There is **exactly one** `objectListSet.Add()` call in the tree (`Object.cpp:1160`) and
**exactly one** `objectListSet.Remove()` (`Object.cpp:615`). Both sit inside hooked functions.
Every creation route named in `interop.cpp:3565-3568` and `:3592-3593` —
`Lego_HandleWorldDebugKeys`, `Lego_LoadLevel`, `Lego_LoadOLObjectList`,
`HiddenObject_ExposeBlock`, `LegoObject_CreateInWorld`, `LegoObject_PTL_GatherRock`,
`Upgrade_SetUpgradeLevel`, `Construction_Zone_CompleteBuilding`, `Level_GenerateCrystal`,
`Level_GenerateOre`, `SpiderWeb_SpawnAt`, `Weapon_Projectile_Add*Path` — funnels into
`LegoObject_Create`, and it does not matter whether any of them is exe: the funnel is hooked.

`Object.cpp` is the biggest exe frontier in the project (**196 exe tags live against 90
implemented bodies, 31%** — `docs/DIRECTORY.md:531`). That statistic is what makes this listSet
look dangerous, and it is the wrong statistic. The 196 unimplemented functions are *consumers*
of objects; the only two functions in the module that touch the free list are both ours and both
hooked.

**Verdict: SAFE**, at Tier 1, with the residue named in §7.

### 2.2 `aiListSet`

| Role | Function | Owner | Exe addr | Hook |
| --- | --- | --- | --- | --- |
| Add | `AITask_Create` (`AITask.cpp:494-504`, `Add()` at `:497`) | OURS | `0x00406370` | **HOOKED** `interop.cpp:1710` |
| Add (clone) | `AITask_Clone` (`AITask.cpp:486`, via `MemCopy`) | OURS | `0x00406330` | **HOOKED** `interop.cpp:1709` |
| Remove | `AITask_Remove` (`AITask.cpp:507-521`, `Remove()` at `:519`) | OURS | `0x004063b0` | **HOOKED** `interop.cpp:1711` |
| AddList | `AITask_AddList` (`AITask.cpp:524-528`) | OURS | `0x004063f0` | **not hooked** (`interop.cpp:1714`) |
| Initialise | `AITask_Initialise` (`AITask.cpp:150`) | OURS | `0x00401c30` | **HOOKED** `interop.cpp:1698` |
| Shutdown | `AITask_Shutdown` (`AITask.cpp:169-173`) | OURS | `0x00402000` | **HOOKED** `interop.cpp:1702` |

`AITask.cpp` is **8% implemented — 96 exe tags against 9 bodies** (`docs/DIRECTORY.md:532`),
easily the most exe-dominated module holding a listSet. It is nevertheless Tier-1 safe on
creation and destruction for exactly the reason in §0.2: the two free-list functions are among
the nine that *are* implemented, and both are hooked. `AITask_Clone` uses `ListSet::MemCopy`
(`AITask.cpp:486`), which preserves the destination's `nextFree` (`ListSet.hpp:146-152`) — the
copy cannot clobber liveness.

**Verdict on creation/destruction: SAFE.** **Verdict on `CountAlive()`: UNSAFE** — see §3.1;
`AITask_Shutdown` (`AITask.cpp:172`) calls `aiListSet.Shutdown()` without removing live tasks.

### 2.3 `efenceListSet`

| Role | Function | Owner | Exe addr | Hook |
| --- | --- | --- | --- | --- |
| Add | `ElectricFence_Create` (`ElectricFence.cpp:106-127`, `Add()` at `:109`) | OURS | `0x0040ceb0` | **HOOKED** `interop.cpp:2646` |
| Remove | `ElectricFence_Remove` (`ElectricFence.cpp:150-159`, `Remove()` at `:152`) | OURS | `0x0040d030` | **not hooked** (`interop.cpp:2652`) |
| Remove (public) | `ElectricFence_RemoveFence` (`ElectricFence.cpp:139-148`) | OURS | `0x0040cfd0` | **HOOKED** `interop.cpp:2650` |
| AddList | `ElectricFence_AddList` (`ElectricFence.cpp:129-133`) | OURS | `0x0040cf60` | **not hooked** (`interop.cpp:2648`) |
| Initialise / Shutdown | `ElectricFence.cpp:48`, `:59` | OURS | `0x0040ccf0`, `0x0040cd60` | **not hooked** (`interop.cpp:2635-2636`) |
| Restart | `ElectricFence_Restart` (`ElectricFence.cpp:62-66`) | OURS | `0x0040cdb0` | **HOOKED** `interop.cpp:2639` |

This is the one listSet whose **destroyer is not hooked**, so it is Tier 2, not Tier 1.
`interop.cpp:2651` records a single exe caller of `0x0040d030` — `ElectricFence_RemoveFence` —
and that one *is* hooked. `ElectricFence.cpp` is **fully implemented (0 exe tags, 27 bodies —
`docs/DIRECTORY.md:552`)**, which is the strongest possible circumstantial support: there is no
unimplemented electric-fence code left to hide a second destroyer in. Initialise/Shutdown are
likewise unhooked but reachable only through the hooked `ElectricFence_Restart`
(`ElectricFence.cpp:64-65`), whose exe callers are `Lego_LoadLevel` / `Lego_LoadMapSet`.

**Verdict on creation/destruction: SAFE (Tier 2).** Hooking `0x0040d030` would raise it to
Tier 1 for one line of code and is the cheapest correctness upgrade in this document.
**Verdict on `CountAlive()`: UNSAFE** — `ElectricFence_Shutdown` (`ElectricFence.cpp:59`) drops
live fences without `Remove`, and it runs on **every level load** via `Restart`.

### 2.4 `configListSet`

| Role | Function | Owner | Exe addr | Hook |
| --- | --- | --- | --- | --- |
| Add | `Config_Create` (`Config.cpp:648-678`, `Add()` at `:656`) | OURS | `0x00479530` | **not hooked** (`interop.cpp:392`) |
| Remove | `Config_Remove` (`Config.cpp:681-695`, `Remove()` at `:694`) | OURS | `0x00479580` | **not hooked** (`interop.cpp:393`) |
| AddList | `Config_AddList` (`Config.cpp:813-817`) | OURS | `0x00479750` | **not hooked** (`interop.cpp:398`) |
| Initialise / Shutdown | `Config.cpp:38`, `:50` | OURS | `0x004790b0`, `0x004790e0` | **HOOKED** `interop.cpp:376-377` |

Tier 2. C++ callers, complete:

- `Config_Create` ← `Config_Load` (`Config.cpp:86`, `:178`) — `Config_Load` **HOOKED**
  (`interop.cpp:379` @ `0x00479120`).
- `Config_Remove` ← `Config_Load` (`Config.cpp:206`) and `Config_Free` (`Config.cpp:635`, inside
  the `while (root)` link-walk at `:622-636`) — `Config_Free` **HOOKED** (`interop.cpp:389` @
  `0x00479500`).

Both unhooked entry points are reachable only from two hooked functions. `Config.cpp` is fully
ours (`docs/DIRECTORY.md:393`).

**Verdict on creation/destruction: SAFE (Tier 2).**
**Verdict on `CountAlive()`: UNSAFE** — `Config_Shutdown` (`Config.cpp:46-52`) calls
`configListSet.Shutdown()` with **no `Config_Free` / RemoveAll pass first**, unlike every other
Gods98 module. Any config still loaded at shutdown leaks its count. (It also leaks its
`fileData`/`fileName` heap blocks, which is a separate, real defect this audit did not go
looking for.)

### 2.5 `containerListSet`

| Role | Function | Owner | Exe addr | Hook |
| --- | --- | --- | --- | --- |
| Add | `Container_Create` (`Containers.cpp:252-`, `Add()` at `:264`) | OURS | `0x00472c10` | **HOOKED** `interop.cpp:422` |
| Remove | `Container_Remove2` (`Containers.cpp:313-510`, `Remove()` at `:509`) | OURS | `0x00472d10` | **not hooked** (`interop.cpp:426`) |
| Remove (public) | `Container_Remove` (`Containers.cpp:307-310`) | OURS | `0x00472d00` | **HOOKED** `interop.cpp:423` |
| AddList | `Containers.cpp:3100` | OURS | — | — |
| Initialise / Shutdown | `Containers.cpp:156`, `:192` | OURS | `0x004729d0`, `0x00472ac0` | **HOOKED** `interop.cpp:407`, `:409` |

Tier 2 on removal. C++ callers of `Container_Remove2`: `Containers.cpp:82` (inside
`Container_Callback_RemoveChildReference`, a D3DRM frame-walk callback), `:189`
(`Container_Shutdown`, hooked) and `:309` (`Container_Remove`, hooked). `interop.cpp:332` names
`Container_Remove2` as the sole consumer of the hook at that site. Fully ours
(`docs/DIRECTORY.md:419`).

Two notes specific to this listSet:

- `Container_Shutdown` **removes while enumerating** (`Containers.cpp:186-190`):
  `for (Container* cont : containerListSet.EnumerateAlive()) { … Container_Remove2(cont, true); }`.
  Safe against the current slot-walk iterator (§5.2); **not** safe against a packed-array
  iterator.
- `Containers.cpp:506` trashes the whole struct — `Mem_DebugTrash(dead, CONTAINER_TRASHVALUE,
  sizeof(Container))` — three lines *before* `containerListSet.Remove(dead)` at `:509`. Trashing
  overwrites `nextFree`, so `ListSet::IsAlive(dead)` would be false and the
  `Error_Fatal(!IsAlive(dead), "Dead item passed to ListSet::Remove")` guard
  (`ListSet.hpp:729`) would fire — i.e. `Error_TerminateProgram`. It does not fire today because
  `Mem_DebugTrash` is defined as **nothing**: `#define Mem_DebugTrash(a,v,s)`
  (`Memory.h:106`). Anyone who ever gives that macro a body must reorder those two statements.

**Verdict on creation/destruction: SAFE (Tier 2).** **`CountAlive()`: SAFE** — `Container_Shutdown`
removes every live item before `Shutdown()` (`Containers.cpp:186-192`).

### 2.6 `fontListSet`

| Role | Function | Owner | Exe addr | Hook |
| --- | --- | --- | --- | --- |
| Add | `Font_Create` (`Fonts.cpp:346-357`, `Add()` at `:353`) | OURS | `0x0047a840` | **not hooked** (`interop.cpp:934`) |
| Remove | `Font_Remove` (`Fonts.cpp:334-344`, `Remove()` at `:343`) | OURS | `0x0047a810` | **HOOKED** `interop.cpp:931` |
| AddList | `Fonts.cpp:365` | OURS | `0x0047a880` | not hooked (`interop.cpp:935`) |
| Initialise / Shutdown | `Fonts.cpp:52`, `:63` | OURS | **`<missing>`** — no exe address at all | n/a |

`Font_Create`'s only caller is `Font_Load` (`Fonts.cpp:102`), **HOOKED** (`interop.cpp:909` @
`0x0047a1a0`). Fully ours (`docs/DIRECTORY.md:411`).

`Font_Initialise` / `Font_Shutdown` are marked `<missing>` in the source (`Fonts.cpp:47`, `:56`):
they do not exist in the 1999 binary, so no exe code can call them. `Font_Initialise` is called
once from `Main.cpp:816`; **`Font_Shutdown` has no caller anywhere in the tree**, so
`fontListSet.Shutdown()` never runs — no count leak, but the lists are never freed either.

**Verdict on creation/destruction: SAFE (Tier 2). `CountAlive()`: SAFE** (vacuously — the only
path that could corrupt it is dead code).

### 2.7 `imageListSet`

| Role | Function | Owner | Exe addr | Hook |
| --- | --- | --- | --- | --- |
| Add | `Image_Create` (`Images.cpp:678-696`, `Add()` at `:685`) | OURS | `0x0047e310` | **not hooked** (`interop.cpp:979`) |
| Remove | `Image_Remove` (`Images.cpp:77-88`, `Remove()` at `:86`) | OURS | `0x0047d730` | **HOOKED** `interop.cpp:950` |
| Remove (bulk) | `Image_RemoveAll` (`Images.cpp:800-807`) | OURS | `0x0047e3f0` | not hooked (`interop.cpp:981`) |
| AddList | `Images.cpp:702` | OURS | `0x0047e380` | not hooked (`interop.cpp:980`) |
| Initialise / Shutdown | `Images.cpp:61`, `:72` | OURS | `0x0047d6d0`, `0x0047d6f0` | **HOOKED** `interop.cpp:947-948` |
| **liveness write** | `Image_InitFromSurface` (`Images.cpp:792`) | OURS | `0x0047e6a0` | **not hooked** (`interop.cpp:989`) |

`Image_Create`'s only callers are `Images.cpp:181` and `:219`, both inside
`Image_LoadBMPScaled`, **HOOKED** (`interop.cpp:960` @ `0x0047dc90`); `Image_LoadBMP` is a macro
onto it (`Images.h:358`). `Image_RemoveAll`'s only caller is `Image_Shutdown` (`Images.cpp:70`),
hooked. Fully ours (`docs/DIRECTORY.md:409`).

**Verdict on creation/destruction: SAFE (Tier 2), with the `Image_InitFromSurface` caveat of §1
attached permanently.** `CountAlive()`: SAFE — `Image_Shutdown` removes all first.

### 2.8 `meshListSet`

| Role | Function | Owner | Exe addr | Hook |
| --- | --- | --- | --- | --- |
| Add | `Mesh_ObtainFromList` (`Mesh.cpp:124-130`, `Add()` at `:127`) | OURS | `0x00480a60` | **not hooked** (`interop.cpp:1285`) |
| Remove | `Mesh_ReturnToList` (`Mesh.cpp:133-137`, `Remove()` at `:136`) | OURS | `0x00480a90` | **not hooked** (`interop.cpp:1286`) |
| AddList | `Mesh.cpp:143` | OURS | `0x00480ab0` | not hooked (`interop.cpp:1287`) |
| Initialise | `Mesh.cpp:59` | OURS | `0x00480870` | **HOOKED** `interop.cpp:1280` |
| Shutdown | **does not exist** | — | — | — |

Tier 2 on both sides. Complete C++ caller sets:

- `Mesh_ObtainFromList` ← `Mesh_CreateOnFrame` (`Mesh.cpp:150`), **HOOKED** (`interop.cpp:1290`);
  `Mesh_Clone` (`Mesh.cpp:182`), **HOOKED** (`interop.cpp:1292`).
- `Mesh_ReturnToList` ← `Mesh_Remove` only, twice (`Mesh.cpp:983`, `:1007` — the cloned-mesh
  branch and the last-reference branch), **HOOKED** (`interop.cpp:1309`).

`Mesh_Clone` uses `ListSet::MemCopy` (`Mesh.cpp:187-188`, carrying the note *"FIX APPLY: Copy
mesh without overwriting nextFree field"*) — a bug already found and fixed once in exactly this
class. Fully ours (`docs/DIRECTORY.md:420`).

There is **no `meshListSet.Shutdown()` anywhere in the tree**, so the lists are never freed and
`m_aliveCount` is never reset — the same shape as `fontListSet`.

**Verdict on creation/destruction: SAFE (Tier 2). `CountAlive()`: SAFE** (vacuously).

### 2.9 `viewportListSet`

| Role | Function | Owner | Exe addr | Hook |
| --- | --- | --- | --- | --- |
| Add | `Viewport_CreatePixel` (`Viewports.cpp:101`) | OURS | `0x00477110` | **HOOKED** `interop.cpp:1521` |
| Add (public) | `Viewport_Create` (`Viewports.cpp:58-73`) | OURS | `0x00477080` | **HOOKED** `interop.cpp:1520` |
| Remove | `Viewport_Remove` (`Viewports.cpp:307`) | OURS | `0x004774e0` | **HOOKED** `interop.cpp:1544` |
| Remove (failure path) | `Viewports.cpp:115` — inside the hooked `Viewport_CreatePixel` | OURS | — | — |
| Remove (bulk) | `Viewport_RemoveAll` | OURS | `0x004776a0` | not hooked (`interop.cpp:1563`); sole caller is `Viewport_Shutdown` (`Viewports.cpp:51`), **HOOKED** `interop.cpp:1518` |
| AddList | `Viewports.cpp:415` | OURS | — | — |

Tier 1 on both creation and destruction. `Viewports.cpp:100` carries the note *"CHANGE:
newViewport now has its nextFree field assigned to itself, like all other listSets"* — a
divergence in this exact class, already found and normalised.

**Verdict: SAFE. `CountAlive()`: SAFE** — `Viewport_Shutdown` removes all first
(`Viewports.cpp:51-53`).

### 2.10 `_modifiedStatsListSet` (DLL-side)

| Role | Site | Owner |
| --- | --- | --- |
| Add | `StatsObject_MakeModified` (`Stats.cpp:92`) | OURS |
| Remove | `StatsObject_RestoreModified` (`Stats.cpp:108`) | OURS |
| Initialise | `Stats_Initialise` (`Stats.cpp:126`) | OURS, hooked `interop.cpp:4096` @ `0x00466aa0` |
| Shutdown | `Stats_RemoveAllModified` (`Stats.cpp:70`) | OURS; sole caller `Game.cpp:3356` |

`_modifiedStatsListSet` is `static` at file scope (`Stats.cpp:58`) with **no exe address**. It
did not exist in 1999; no machine code can reach it. This is the only listSet in the project
that is unconditionally, structurally SAFE.

Two defects found in passing, both about lifecycle rather than hooks:

- `Stats_RemoveAllModified` (`Stats.cpp:68-71`) calls `Shutdown()` without `Remove` —
  `m_aliveCount` leak, same class as §3.1.
- `Stats_Initialise` (`Stats.cpp:126`) calls `Initialise()`, which **nulls `listSet[]` without
  freeing it** (`ListSet.hpp:669-676`). If `Stats_Initialise` ever runs without a preceding
  `Stats_RemoveAllModified`, every allocated list leaks outright. Whether the ordering at
  `Game.cpp:3356` always precedes it is **UNDETERMINED** — it depends on exe-side level-load
  sequencing.

**Verdict: SAFE.**

---

## 3. The divergence that is not about hooks

The audit the source asked for was "have we hooked all creators and destroyers". Answering it
turns up a second, independent way for `CountAlive()` to be wrong, and it is *already firing*.

### 3.1 `Initialise` and `Shutdown` bypass `Remove` by design, and neither resets `m_aliveCount`

```cpp
// engine/core/ListSet.hpp:669-676
void Initialise()
{
	for (size_t i = 0; i < this->MaxLists(); i++) m_cont.listSet[i] = nullptr;
	m_cont.listCount = 0;
	m_cont.freeList  = nullptr;
}                                   // <-- m_aliveCount untouched

// engine/core/ListSet.hpp:678-695
/** … Note that the user should handle cleanup of individual list items beforehand
 *      (but `Remove` does not need to be called). */
void Shutdown()
{
	for (…) if (m_cont.listSet[i]) { Gods98::Mem_Free(m_cont.listSet[i]); … }
	m_cont.listCount = 0;
	m_cont.freeList  = nullptr;
}                                   // <-- m_aliveCount untouched
```

`m_aliveCount` is declared at `ListSet.hpp:850` and zeroed **only** in the constructor
(`ListSet.hpp:590`). It is incremented at `:717` and decremented at `:731`. Since the container
objects are namespace-scope globals constructed once per process, `m_aliveCount` is zeroed
exactly once, at DLL load.

Consequence, provable from the cited lines alone:

> For any listSet whose `Shutdown` runs while items are still alive, `CountAlive()` is
> permanently inflated by that many, for the rest of the process.

Which ones do that:

| listSet | Shutdown path | Removes first? |
| --- | --- | --- |
| `configListSet` | `Config_Shutdown` `Config.cpp:46-52` | **NO** |
| `aiListSet` | `AITask_Shutdown` `AITask.cpp:169-173` | **NO** |
| `efenceListSet` | `ElectricFence_Shutdown` `ElectricFence.cpp:51-60` | **NO** |
| `_modifiedStatsListSet` | `Stats_RemoveAllModified` `Stats.cpp:68-71` | **NO** |
| `objectListSet` | `LegoObject_Shutdown` `Object.cpp:213-227` → `LegoObject_RemoveAll` `:215` | yes |
| `containerListSet` | `Container_Shutdown` `Containers.cpp:186-192` | yes |
| `imageListSet` | `Image_Shutdown` `Images.cpp:65-74` → `Image_RemoveAll` `:70` | yes |
| `viewportListSet` | `Viewport_Shutdown` `Viewports.cpp:47-55` → `Viewport_RemoveAll` `:51` | yes |
| `fontListSet` | `Font_Shutdown` `Fonts.cpp:56-65` → `Font_RemoveAll` `:60` | yes — but never called |
| `meshListSet` | — | no Shutdown exists |

`efenceListSet` is the sharp one: `ElectricFence_Restart` (`ElectricFence.cpp:62-66`) calls
`Shutdown` then `Initialise`, and it is hooked as the handler for `Lego_LoadLevel` /
`Lego_LoadMapSet` (`interop.cpp:2638-2639`). **So `efenceListSet.CountAlive()` gains the fence
count of every completed level, permanently.** The `[F8]` overlay already prints
`efenceListSet` through `EnumerateAlive().Count()` (`GameState.cpp:1790`) rather than
`CountAlive()`, so the bug is invisible on screen — which is exactly why it survived.

`aiListSet` and `configListSet` only lose on `Lego_Shutdown_Full`, so the practical damage is
smaller; the defect is identical.

**This is the real answer to `GameState.cpp:1792-1795`.** The hooking question passes. The
lifecycle question does not, and no amount of hooking would have fixed it.

**Fix, three lines, no address-map impact:** set `m_aliveCount = 0` in both `Initialise`
(`ListSet.hpp:674`) and `Shutdown` (`ListSet.hpp:693`), and have the dense index `Clear()` in
the same place. Both functions are ours, header-only, and no exe-overlaid struct changes:
`m_aliveCount` is a member of `WrapperCollection`, a DLL-side object, not of the pinned
`*_Globs` (`ListSet.hpp:850` vs `Object.h:452-453`). The CARDINAL RULE is not engaged.

### 3.2 `LegoObject_RemoveAll` deliberately skips upgrade parts

`Object.cpp:1721-1726` iterates `EnumerateSkipUpgradeParts()`, carrying the source's own
`/// TODO: WHY IS SKIPUPGRADEPARTS BEING USED???`. So `LegoObject_CleanupLevel`
(`Object.cpp:318-320`) leaves every `LIVEOBJ3_UPGRADEPART` object alive across a level
transition. This is **not** a divergence — the items really are alive and `m_aliveCount` really
does count them — but it means a dense index will faithfully carry stale upgrade parts into the
next level, and anyone debugging "the index has entries before the level loaded" will chase the
wrong thing. Recorded so they do not.

---

## 4. ListSet's own hazards for a dense index

The brief asks six specific questions. Answered from source.

### 4.1 Does `Remove` leave holes? **Yes, and that is the whole problem.**

`Remove` (`ListSet.hpp:726-736`) splices the item onto `freeList` and does nothing else. The
slot stays allocated and the walk still visits it — `BaseIterator::operator++`
(`ListSet.hpp:327-361`) steps every index of every allocated list and calls the predicate on
each. `AliveFilter` (`ListSet.hpp:230`) then rejects it. Cost of rejecting: `nextFree` at
`Object.h:444` offset `0x408`, one cache line, no useful payload.

### 4.2 Does `PopList` ever run? **No — and it cannot, because it does not compile.**

```
grep -rn "ShrinkToFit\|PopList" --include=*.cpp --include=*.h --include=*.hpp src/
  -> ListSet.hpp:771, 773, 836, 842   (definition and self-reference only)
```

`PopList` is `protected` (`ListSet.hpp:767`) and its only caller is `ShrinkToFit`
(`ListSet.hpp:842`), which has **zero callers in the tree**. That is the usual argument, and
there is a stronger one: `ShrinkToFit` → `IsListAlive` (`ListSet.hpp:814-828`) contains

```cpp
if (ListSet::IsAlive(list[i]))          // ListSet.hpp:822
```

where `IsAlive` takes `const TItem*` (`ListSet.hpp:180`) and `list[i]` is an object, not a
pointer. Template argument deduction fails; this is a **compile error**, latent only because
these are templates and the path is never instantiated. `IsListAlive` also sizes its loop with
`CountOfList(m_cont.listCount)` instead of `CountOfList(listIndex)` (`ListSet.hpp:819`) — the
wrong list's length, which would over-run every list below the last.

So: **capacity is monotonic, provably, for the life of the process.** Confirms
`docs/PERFORMANCE.md` §1.1. It also means `ShrinkToFit` must not be "just turned on" as a
memory fix — it is two bugs deep and would invalidate every held item pointer if it worked.

### 4.3 Is capacity monotonic? **Yes**, per §4.2. Growth is append-only.

`AddList` (`ListSet.hpp:741-764`) writes `listSet[listCount]` then increments `listCount`. Lists
below the new one are never touched, so **`IndexOfInListSet(listIndex, itemIndex)`
(`ListSet.hpp:103-107`) is stable across capacity growth** — an existing item's slot id never
changes. A dense index keyed by slot id must therefore *extend* `m_where` on growth, not renumber.

`DeepCoreDenseIndex.hpp:76-85` currently discards everything and marks dirty on any capacity
change. That is safe and, given growth happens at most `MAXLISTS` times per level, cheap. It is
also stricter than necessary; note it as a deliberate conservatism, not a bug.

### 4.4 Are item pointers stable across `Add`? **Yes.**

Each list is an independent `Gods98::Mem_Alloc` (`ListSet.hpp:747`) that is never reallocated.
`Add` (`ListSet.hpp:701-720`) only unlinks from `freeList`. Nothing moves an item. The only
function that frees a list is `PopList` (`ListSet.hpp:803`), which per §4.2 cannot run.

**An index of raw `TItem*` is therefore valid for the life of the process**, which is the
property A1 needs and the one that would have killed it had it been false.

### 4.5 Mutation during iteration — **the hazard the brief did not ask about, and the one most likely to bite**

The current iterator is *incidentally* remove-safe: it walks by index, so an item dying under
the cursor changes only a predicate result, and an item born in an already-passed slot is simply
missed this pass. A packed-pointer array with swap-with-last removal has neither property —
removing the current element moves an unvisited element into the cursor's position and the walk
skips it.

Mutation during a walk is **routine**, not exceptional, on `objectListSet`:

| Site | What happens |
| --- | --- |
| `Object.cpp:1683` `LegoObject_UpdateAll` → `LegoObject_Callback_Update` | reaches `LegoObject_Remove(liveObj)` at `Object.cpp:1829` and `:3633` — **removal mid-walk** |
| same path | `LegoObject_DestroyBoulder_AndCreateExplode` (`Object.cpp:1824`), weapon spawns, monster emerges — **creation mid-walk** |
| `Object.cpp:1723-1726` `LegoObject_RemoveAll` | removes every item while enumerating |
| `Containers.cpp:186-190` `Container_Shutdown` | removes every item while enumerating |
| `Images.cpp:801-804` `Image_RemoveAll`, `Viewports.cpp` `Viewport_RemoveAll` | same shape |

`objectGlobs.flags` already carries `OBJECT_GLOB_FLAG_UPDATING` (`Object.cpp:1681`) and
`OBJECT_GLOB_FLAG_REMOVING` (`Object.cpp:1718`), so the codebase knows it is inside a walk. Any
dense-index enumeration must therefore iterate **a snapshot** of the live array, not the live
array itself, and must define "created during the walk" as "not visited this pass" — which is
what the current iterator does for slots behind the cursor and *not* what it does for slots
ahead of it. That behavioural difference is small, real, and must be written down before A1
lands, not discovered afterwards.

### 4.6 Cost of obtaining the slot id in `Add`/`Remove`

The index is keyed by slot, but `Add` returns only a pointer. `WrapperCollection::IndexOf`
(`ListSet.hpp:616-624`) → `ListIndexOf` (`ListSet.hpp:600-609`) is a linear scan of at most
`listCount` ≤ 32 range checks. Paid once per creation and once per destruction — not per frame,
and not inside any of the eight per-frame walks. Acceptable. Alternatively `Add` can compute the
slot directly, since it already knows which list it drew from, but the simple version is fine.

---

## 5. The dirty-flag rebuild design — and the fact that it already exists

The brief asks whether the index can be rebuilt lazily so correctness never depends on the audit
being perfect, and to evaluate that seriously as possibly *strictly better* than relying on an
audit. It is, for three reasons:

1. **The worst case is exactly today's cost.** A dirty index costs one full O(capacity) walk to
   resync. That is precisely the walk being performed today, eight-plus times per frame. A
   dirty-by-default index is therefore never slower than the status quo by more than one walk
   per dirty event, and is faster the rest of the time.
2. **It converts a correctness gamble into a performance ledger.** Under audit-only, a missed
   creator silently skips live objects — monsters that do not update, projectiles that never
   land. Under dirty-flag, a missed creator costs one rebuild. Each subsequent audit result then
   *retires* a dirty flag, and each retirement is a measurable speedup rather than a bet.
3. **It survives the parts of the audit that cannot be closed here.** §7 lists residues that
   require the 1999 binary. Those can be handled by marking dirty at the boundary — e.g. after
   any hook whose installation returned false — instead of by assertion.

**This is already built.** `src/openlrr/game/DeepCoreDenseIndex.hpp` implements
`DeepCore::Detail::DenseLiveIndex<TItem>`: packed `m_live` pointer array, `m_slotOf` parallel
array, `m_where` slot→position map, `OnAdd`/`OnRemove` with swap-with-last,
`RebuildBegin`/`RebuildAdd`/`RebuildEnd`, `MarkDirty`, and an out-of-range `OnAdd` that marks
dirty rather than corrupting (`DeepCoreDenseIndex.hpp:100-104`). Its header comment states the
design contract in the same terms as this audit, including "CORRECTNESS DOES NOT DEPEND ON AN
AUDIT" (`DeepCoreDenseIndex.hpp:37-48`).

It is tested headlessly in `tools/harness/harness.cpp:496-676`: eight named tests plus a 40,000-step
fuzz (`fuzz_dense_index_always_agrees_with_the_full_walk`, `harness.cpp:652`) asserting that
after any sequence of adds and removes the index's live set equals what the O(capacity) walk
reports, over a `FakeItem` deliberately padded to 1036 bytes so the benchmark cannot flatter the
new path (`harness.cpp:501-506`).

Two facts about its current state, both verifiable by grep:

- **It is not referenced by any other file.** `grep -rn DeepCoreDenseIndex src/` returns only its
  own first line. `ListSet.hpp` does not know it exists.
- **It is not in `src/openlrr/openlrr.vcxproj`.** It is compiled only by
  `tools/harness/harness.vcxproj`.

So A1 is, today, a tested component with no consumer. What is missing is the wiring — and the
wiring is what this audit was supposed to license.

Two gaps in the existing design that this audit exposes and the harness does not cover:

- **No `Clear()` on `Initialise`/`Shutdown`.** Per §3.1 those bypass `Remove` entirely. The index
  must be cleared there or it inherits the exact `m_aliveCount` bug.
- **No snapshot semantics for mutation-during-iteration.** Per §4.5. `Live()` returns a
  `const std::vector<TItem*>&` (`DeepCoreDenseIndex.hpp:97`); handing that reference straight to
  a range-for whose body can call `Remove` is undefined behaviour on the vector, never mind the
  index. The enumerator must copy, or iterate by index over a size captured up front and tolerate
  swap-with-last, or the walk must be flagged so `OnRemove` defers.

---

## 6. Per-listSet verdict table

Two columns, because §3.1 proved they are different questions. **Creation/destruction** = can an
item come into being or cease to be without passing through implemented C++.
**`CountAlive()`/index integrity** = is the running live-count trustworthy today.

| listSet | Creator (hooked?) | Destroyer (hooked?) | Creation/destruction | `CountAlive()` today | Dense index? |
| --- | --- | --- | --- | --- | --- |
| `objectListSet` | `LegoObject_Create_internal` **HOOKED** | `LegoObject_Remove` **HOOKED** | **SAFE** (Tier 1) | SAFE | **YES** |
| `aiListSet` | `AITask_Create` **HOOKED** | `AITask_Remove` **HOOKED** | **SAFE** (Tier 1) | **UNSAFE** — `AITask.cpp:172` | yes, after §3.1 fix |
| `viewportListSet` | `Viewport_CreatePixel` **HOOKED** | `Viewport_Remove` **HOOKED** | **SAFE** (Tier 1) | SAFE | yes (no benefit) |
| `containerListSet` | `Container_Create` **HOOKED** | `Container_Remove2` unhooked; sole exe caller hooked | **SAFE** (Tier 2) | SAFE | yes; §4.5 applies |
| `efenceListSet` | `ElectricFence_Create` **HOOKED** | `ElectricFence_Remove` **unhooked**; sole exe caller `ElectricFence_RemoveFence` **HOOKED** | **SAFE** (Tier 2) | **UNSAFE** — `ElectricFence.cpp:59` via `Restart`, every level | yes, after §3.1 fix |
| `configListSet` | `Config_Create` unhooked; callers `Config_Load` **HOOKED** | `Config_Remove` unhooked; callers `Config_Load`/`Config_Free` **HOOKED** | **SAFE** (Tier 2) | **UNSAFE** — `Config.cpp:50` | yes (no benefit) |
| `imageListSet` | `Image_Create` unhooked; sole caller `Image_LoadBMPScaled` **HOOKED** | `Image_Remove` **HOOKED** | **SAFE** (Tier 2), caveat §1 | SAFE | yes (no benefit) |
| `fontListSet` | `Font_Create` unhooked; sole caller `Font_Load` **HOOKED** | `Font_Remove` **HOOKED** | **SAFE** (Tier 2) | SAFE (vacuous) | yes (no benefit) |
| `meshListSet` | `Mesh_ObtainFromList` unhooked; callers `Mesh_CreateOnFrame`/`Mesh_Clone` **HOOKED** | `Mesh_ReturnToList` unhooked; sole caller `Mesh_Remove` **HOOKED** | **SAFE** (Tier 2) | SAFE (vacuous) | yes (no benefit) |
| `_modifiedStatsListSet` | `StatsObject_MakeModified` | `StatsObject_RestoreModified` | **SAFE** (no exe address exists) | **UNSAFE** — `Stats.cpp:70` | n/a |

No listSet is UNSAFE on creation/destruction. Four are UNSAFE on `CountAlive()`, all from the
same three-line defect in §3.1, none of them from an unhooked creator.

---

## 7. What could not be established, and why

Stated plainly, because the value of this document depends on its residue being honest.

1. **The `// used by:` comments in `interop.cpp` are unverified against the binary.** Every
   Tier-2 verdict above rests on them being a complete cross-reference of the 1999 exe. They are
   the OpenLRR decompilation's own record and they are internally consistent, but this repository
   contains no `LegoRR.exe` (the `No_game_binaries` CI gate forbids one) so no xref was
   independently recomputed. **UNDETERMINED**, permanently, on this machine.
2. **The nine unhooked `*_AddList` functions.** `LegoObject_AddList` `0x004385d0`,
   `AITask_AddList` `0x004063f0`, `ElectricFence_AddList` `0x0040cf60`, `Config_AddList`
   `0x00479750`, `Image_AddList` `0x0047e380`, `Font_AddList` `0x0047a880`, `Mesh_AddList`
   `0x00480ab0`, plus the container and viewport equivalents. Each is asserted "internal" in
   `interop.cpp` and each C++ body carries the note *"This function is no longer called, `Add`
   already handles this"*. If 1999 code calls one directly it would allocate a list and splice
   `freeList` behind our back. Per §4.3 that cannot renumber existing slots — capacity growth is
   append-only — so the damage is bounded to a stale `Reserve()`, which `DenseLiveIndex` already
   handles by marking dirty (`DeepCoreDenseIndex.hpp:76-85`). **UNDETERMINED but contained.**
3. **Hook installation failure is silent.** `return_interop` (`interop.h:89`) prints
   `"<fn> failed"` and returns; `interop_hook_all` (`interop.cpp:4579`) ANDs the results and the
   caller at `OpenLRR.cpp:1102` **ignores the return value**. A `VirtualProtect` failure would
   leave a creator unhooked with nothing but a `printf` to say so. Every Tier-1 verdict above is
   conditional on the hook actually installing. This is the strongest single argument for the
   dirty-flag design over an audit: `interop_hook_all()` returning false should
   `MarkDirty()` every index, and today it cannot, because nobody looks at it.
4. **Whether `Stats_RemoveAllModified` always precedes `Stats_Initialise`** (§2.10) depends on
   exe-side level-load ordering. **UNDETERMINED.**
5. **Nothing here was run.** Consistent with the project mandate: compile-verification is the
   ceiling, and this document did not even compile anything — no build was run, per the research-only
   constraint. The one component that *is* executable, `DenseLiveIndex`, is exercised by
   `tools/harness/harness.cpp` and its results are cited above as prior work, not re-run here.

---

## 8. DECISION

**A1 may proceed on `objectListSet`.** It is the only listSet that is Tier-1 safe on both
creation and destruction *and* has a correct `CountAlive()` today *and* is the target of the
eight-plus per-frame walks. Its single `Add` site (`Object.cpp:1160`) and single `Remove` site
(`Object.cpp:615`) both sit inside functions hooked at their exe addresses
(`interop.cpp:3572`, `:3550`), so no 1999 code path can reach the free list without executing
our C++. No C++ anywhere in the tree writes `listSet[]`, `listCount` or `freeList` outside
`ListSet.hpp`. Item pointers are stable for the life of the process (§4.4) and capacity growth
never renumbers a slot (§4.3), which are the two structural properties an index of pointers
requires.

**It must proceed as the dirty-flag rebuildable index, not as an audit-backed one.** Not because
the audit failed — it passed on every listSet — but because three of its supports are
unverifiable on this machine (§7 items 1–3) and one of them, silent hook-installation failure,
is a live hole in the program rather than a hypothetical. The dirty-flag design bounds every one
of those to a single full walk, which is what the code does today anyway. That makes it strictly
better than relying on the audit, and it is already written and fuzz-tested
(`DeepCoreDenseIndex.hpp`, `tools/harness/harness.cpp:496-676`).

**Three things must land before or with the wiring**, in this order:

1. **Reset `m_aliveCount` in `Initialise` (`ListSet.hpp:669-676`) and `Shutdown`
   (`ListSet.hpp:683-695`), and `Clear()` the index in both.** This is a live bug today,
   independent of A1: `efenceListSet.CountAlive()` inflates on every level load (§3.1). Three
   lines, DLL-side only, `ADDRESS-MAP.md` unaffected, and it fixes the very counter the F8
   overlay was written to display.
2. **Define and enforce snapshot semantics for enumeration** (§4.5). Removal and creation
   mid-walk are routine on `objectListSet` (`Object.cpp:1683` → `:1829`), the current
   slot-walking iterator tolerates both by accident, and a packed array does not. `Live()`
   must not be handed to a range-for whose body can mutate.
3. **Hook `ElectricFence_Remove` at `0x0040d030`** (uncomment `interop.cpp:2652`). One line,
   raises the only unhooked destroyer in the project from Tier 2 to Tier 1, and removes the last
   place where a creator/destroyer verdict rests on a decompilation comment.

**Do not extend A1 to the other listSets on performance grounds.** `aiListSet`, `configListSet`,
`containerListSet`, `fontListSet`, `imageListSet`, `meshListSet`, `viewportListSet` and
`efenceListSet` are all safe enough to index, but none of them is walked eight times a frame and
`efenceListSet` already has a cheaper fix available (A5: early-out on `CountAlive() == 0`,
`ElectricFence.cpp:250-268`) which becomes correct the moment fix 1 above lands. Indexing them
would add maintenance surface for no measured gain — and per the project's own standard, gain
here cannot be measured at all outside the harness.

**Finally: `GameState.cpp:1792-1795` should be updated, not deleted.** Its stated worry —
unhooked creators — is resolved. The reason its advice was right is different from the reason it
gave, and four of the nine `CountAlive()` calls it guards are wrong today for a reason it did not
anticipate.
