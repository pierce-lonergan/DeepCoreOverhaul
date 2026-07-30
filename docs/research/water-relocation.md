# Water table relocation — full DLL-side storage for `Water_Globs`

**Status:** design study. Nothing in this document has been implemented.
**Scope:** replace the shipped graceful-degradation patch (`DeepCore::WaterOverflow`,
gated by `surviveWaterOverflow`) with a real capacity fix that keeps
`LegoRR::Water_Globs` byte-identical.
**Honest ceiling:** we cannot run the game. Every claim below is source-derived or
compile-verifiable. Nothing here has been play-tested and nothing here ever will be
until someone with a game installation runs it.

---

## 0. Executive summary

The water module is the single best relocation candidate in the tree, for three
reasons that no other capped table currently satisfies together:

1. **All 13 original `Water_*` functions are already ours.** `Water.h` contains
   thirteen exe address macros and *every one of them is commented out*
   (`Water.h:137, 144, 148, 152, 156, 161, 165, 169, 173, 178, 182, 186, 190`), with a
   real C++ declaration beneath each. All thirteen have bodies in `Water.cpp`, and all
   thirteen are hooked over the exe in one contiguous block
   (`interop.cpp:4312–4336`). There is no water code left in the original executable
   that we do not own.
2. **`Water_Globs` is a module-private globs struct.** Its whole reason to exist is
   `Water.c`; the only two reads outside `world/Water.cpp` in the entire tree are ours
   (`GameState.cpp:1832` and `GameState.cpp:1837`, the `[Water]` debug overlay).
3. **We do not have to touch the struct at all.** `Water_Globs` stays exactly
   `0x29ec` bytes at `0x0054a520`; the relocation is additive DLL-side storage plus an
   indirection layer, which is the pattern already proven in-tree by the PowerGrid
   vectors at `Game.cpp:169–172`.

**There is exactly one blocker, and it is small and closable.** `Water_FindPoolDrain`
returns a `Water_Pool*`, and one of its three callers is still original 1999 machine
code: `Level_Debug_WKey_NeedsBlockFlags1_8_FUN_004303a0` at `0x004303a0`
(`interop.cpp:4327`; still a live address macro at `Game.h:1557`). That is the only
place a `Water_Pool*` escapes this DLL. It is reachable from exactly one call site —
our own `[W]` debug keybind at `Game.cpp:2222–2224` — which means we can close it
without decompiling anything. Section 7 gives two independent closures.

**Decision (Section 9): full relocation is safe to do today**, conditional on
implementing closure C1 (gate the `[W]` debug key) — which is a five-line edit.

---

## 1. `Water_Globs` layout — every field, offset, size

### 1.1 Binding and total size

```cpp
// src/openlrr/game/world/Water.cpp:19
LegoRR::Water_Globs & LegoRR::waterGlobs = *(LegoRR::Water_Globs*)0x0054a520;
```

```cpp
// src/openlrr/game/world/Water.h:104
assert_sizeof(Water_Globs, 0x29ec);
```

Both confirmed as stated in the task. Independently confirmed by the address-space
linter, which derives its rows from the source rather than from this document:

| source | claim |
|---|---|
| `docs/address-map.json:715–722` | `LegoRR::waterGlobs`, `Water_Globs`, `addr` 5547296 = `0x0054a520`, `size` 10732 = `0x29ec`, `size_src: "assert_sizeof"`, `where: src/openlrr/game/world/Water.cpp:19` |
| `docs/ADDRESS-MAP.md:149` | start `0x0054a520`, end `0x0054cf0c`, 10732 bytes |
| `docs/address-map.json:1360–1368` | predecessor `Gods98::dxbugGlobs` (slack 1120 before), successor `LegoRR::toolTipGlobs` (slack **20** after) |

`0x0054a520 + 0x29ec = 0x0054cf0c`. `toolTipGlobs` begins at 5558048 = `0x0054cf20`.
Twenty bytes of raw slack follow the struct. **This is not an invitation.** Twenty
bytes buys one extra `Water_PoolDrain` and change; the relocation below does not use
a single byte of it, and the linter's "113 sized overlaid regions, 0 overlaps" result
is therefore unchanged by this work — a fact worth citing in the eventual commit.

### 1.2 `Water_Globs` field table

`Water.h:95–104`.

| offset | size | field | notes |
|---:|---:|---|---|
| `0x0000` | `0x2990` (10664) | `Water_Pool poolList[WATER_MAXPOOLS]` | **fixed table.** `10 × 0x428` |
| `0x2990` | `0x4` | `uint32 poolCount` | live element count of `poolList` |
| `0x2994` | `0x50` (80) | `Water_PoolMergePair mergeList[WATER_MAXMERGEPOOLS]` | **fixed table.** `10 × 0x8` |
| `0x29e4` | `0x4` | `uint32 mergeCount` | live element count of `mergeList` |
| `0x29e8` | `0x4` | `real32 digDepth` | assigned at `Water.cpp:131`, never read anywhere |
| `0x29ec` | | *(end)* | |

### 1.3 `Water_Pool` field table

`Water.h:71–83`, `assert_sizeof(Water_Pool, 0x428)`.

| offset | size | field | notes |
|---:|---:|---|---|
| `0x000` | `0x320` (800) | `Point2F blocks[WATER_MAXPOOLBLOCKS]` | **fixed table.** `100 × 8` |
| `0x320` | `0x4` | `uint32 blockCount` | |
| `0x324` | `0xf0` (240) | `Water_PoolDrain drainList[WATER_MAXPOOLDRAINS]` | **fixed table.** `10 × 0x18` |
| `0x414` | `0x4` | `uint32 drainCount` | |
| `0x418` | `0x4` | `real32 highWaterLevel` | init `-10000.0f` at `Water.cpp:307` |
| `0x41c` | `0x4` | `real32 currWaterLevel` | |
| `0x420` | `0x4` | `Gods98::Container* contMeshTrans` | the D3DRM water plane mesh |
| `0x424` | `0x4` | `WaterFlags flags` | |
| `0x428` | | *(end)* | |

### 1.4 `Water_PoolDrain` and `Water_PoolMergePair`

`Water_PoolDrain` (`Water.h:57–67`, `assert_sizeof(..., 0x18)`):
`blockIndex` `0x00`, `direction` `0x04`, `drainWaterLevel` `0x08`, `elapsedUp_c` `0x0c`,
`elapsedDown_10` `0x10`, `active` `0x14`.

`blockIndex` is an index **into the owning pool's `blocks[]`**, not a map index
(`Water.cpp:272`, consumed at `Water.cpp:429–430, 583–584, 633–634`,
`GameState.cpp:1842–1843`). This matters: relocating `blocks[]` to a `std::vector`
does not invalidate any drain, because drains address blocks by index and never by
pointer. That is a lucky and load-bearing property of the original design.

`Water_PoolMergePair` (`Water.h:86–92`, `assert_sizeof(..., 0x8)`): two raw
`Water_Pool*`. **This is the one struct in the module that stores pointers to pools**,
and it is therefore the pointer-stability hazard (Section 6.2).

### 1.5 The four fixed tables, restated

| table | declared at | capacity | element size | total |
|---|---|---:|---:|---:|
| `Water_Globs::poolList[]` | `Water.h:97` (`WATER_MAXPOOLS`, `Water.h:28`) | 10 | `0x428` | `0x2990` |
| `Water_Pool::blocks[]` | `Water.h:73` (`WATER_MAXPOOLBLOCKS`, `Water.h:26`) | 100 | `8` | `0x320` |
| `Water_Pool::drainList[]` | `Water.h:75` (`WATER_MAXPOOLDRAINS`, `Water.h:27`) | 10 | `0x18` | `0xf0` |
| `Water_Globs::mergeList[]` | `Water.h:99` (`WATER_MAXMERGEPOOLS`, `Water.h:29`) | 10 | `0x8` | `0x50` |

100 blocks per pool is the cap that actually kills community maps: it is *400 map
tiles* of water surface in total across the whole level (10 pools × 100 tiles at the
absolute best, and only if the map's water splits into exactly ten equal bodies).

---

## 2. Ownership audit — is any of this still the exe's?

### 2.1 All 13 functions are ours

Counting live vs commented-out address macros in `Water.h`:

| function | exe address | macro in `Water.h` | our body | hooked |
|---|---|---|---|---|
| `Water_Initialise` | `0x0046de50` | commented, `:137` | `Water.cpp:124` | `interop.cpp:4312` |
| `Water_InitPoolDrains` | `0x0046dfd0` | commented, `:144` | `Water.cpp:239` | `interop.cpp:4314` |
| `Water_InitVertices` | `0x0046e140` | commented, `:148` | `Water.cpp:283` | `interop.cpp:4316` |
| `Water_ChangeViewMode_removed` | `0x0046e480` | commented, `:152` | `Water.cpp:375` | `interop.cpp:4318` |
| `Water_DestroyWallComplete` | `0x0046e4e0` | commented, `:156` | `Water.cpp:397` | `interop.cpp:4320` |
| `Water_DamBlock` | `0x0046e5f0` | commented, `:161` | `Water.cpp:440` | `interop.cpp:4322` |
| `Water_Update` | `0x0046e650` | commented, `:165` | `Water.cpp:455` | `interop.cpp:4324` |
| `Water_UpdateNotHotBlocks` | `0x0046e8d0` | commented, `:169` | `Water.cpp:560` | `interop.cpp:4326` |
| `Water_FindPoolDrain` | `0x0046eb60` | commented, `:173` | `Water.cpp:618` | `interop.cpp:4328` |
| `Water_QsortComparePools` | `0x0046ec60` | commented, `:178` | `Water.cpp:647` | `interop.cpp:4330` |
| `Water_FindPoolAndMergeRows` | `0x0046ec90` | commented, `:182` | `Water.cpp:660` | `interop.cpp:4332` |
| `Water_AddPoolRowBlocks` | `0x0046ed90` | commented, `:186` | `Water.cpp:702` | `interop.cpp:4334` |
| `Water_AddPool` | `0x0046edf0` | commented, `:190` | `Water.cpp:720` | `interop.cpp:4336` |

**Live address macros in `Water.h`: zero. Commented-out: 13. Implemented: 13/13.
Hooked: 13/13.**

Two further functions exist in the module and are **CUSTOM** (no exe counterpart, not
hooked, not callable from the exe):

- `Water_AddPoolFloodFill` (`Water.h:134`, `Water.cpp:48`) — the East-West flood fill
  that replaces the original row-scan-plus-merge algorithm.
- `Water_RemoveAll` (`Water.h:141`, `Water.cpp:222`) — end-of-level cleanup, called
  from `Game.cpp:3351`.

**Address-range coverage.** The hooked functions run `0x0046de50` → `0x0046edf0`. The
next symbol the tree knows about is `Weapon_Initialise` at `0x0046ee40`
(`interop.cpp:4345`, itself commented out). The `0x50` bytes between `Water_AddPool`'s
entry and `0x0046ee40` are `Water_AddPool`'s own body. **There is no gap in which an
unaccounted-for `Water.c` function could hide.** This is the strongest available
evidence that the exe's water module is 100% covered.

### 2.2 Who reads `waterGlobs`?

Whole-tree grep for `waterGlobs`. Results outside `world/Water.cpp`:

| file:line | access | ours? |
|---|---|---|
| `GameState.cpp:1832` | `waterGlobs.poolCount == 0` | **ours** (debug overlay `DEBUGOVERLAY_WATER`, `GameState.cpp:1818`) |
| `GameState.cpp:1837` | `auto pool = &waterGlobs.poolList[0]` | **ours** (same overlay; then reads `currWaterLevel`, `highWaterLevel`, `drainCount`, `drainList[j]`, `blocks[...]` at `:1838–1850`) |
| `Water.h:115` | `extern Water_Globs & waterGlobs;` | declaration |
| `docs/*`, `DeepCore.hpp:130/136` | prose | not code |

That is the complete list. **No other module in the tree touches `waterGlobs`.**
`RadarMap.cpp:60` and `:917` match the grep on `Water_` only because of the unrelated
`Radar_Colour::Water_Hidden` enumerator — not a water-module reference.

### 2.3 Who calls into the water module?

From `interop.cpp`'s `// used by:` annotations (`4311–4336`) plus a call-site grep:

| water function | callers | caller status |
|---|---|---|
| `Water_Initialise` | `Lego_LoadMapSet` | **exe** — live macro `Game.h:1398`, declaration commented at `Game.cpp:2978` |
| `Water_InitPoolDrains`, `Water_InitVertices` | `Water_Initialise` | ours |
| `Water_ChangeViewMode_removed` | `Lego_SetViewMode` | ours (hooked, `interop.cpp:2790`); also called directly `Game.cpp:2863, 2885` |
| `Water_DestroyWallComplete` | `Level_DestroyWall` | **exe** — live macro `Game.h:1561`, declaration commented at `Game.cpp:3419` |
| `Water_DamBlock` | `Level_Debug_WKey_..._004303a0` | **exe** — live macro `Game.h:1557` |
| `Water_Update` | `Lego_MainLoop` | ours — called at `GameState.cpp:1134` |
| `Water_UpdateNotHotBlocks` | `Water_Update` | ours |
| `Water_FindPoolDrain` | `Level_Debug_WKey_..._004303a0`, `Water_DestroyWallComplete`, `Water_DamBlock` | **one exe, two ours** |
| `Water_QsortComparePools`, `Water_FindPoolAndMergeRows`, `Water_AddPool` | `Water_Initialise` | ours (legacy branch only — see 2.4) |
| `Water_AddPoolRowBlocks` | `Water_Initialise`, `Water_AddPool` | ours |
| `Water_RemoveAll` (CUSTOM) | `Game.cpp:3351` | ours |
| `Water_AddPoolFloodFill` (CUSTOM) | `Water.cpp:144` | ours |

**Why exe callers are harmless.** `hook_write_jmpret` overwrites the *callee's*
prologue (`docs/HOOK-ARCHITECTURE.md:26, 30`): 6 bytes, `E9 rel32` + `C3`, destructive,
no trampoline. An exe `CALL 0x0046de50` therefore lands on our `Water_Initialise`
regardless of who issued it. The original water bodies are unreachable dead bytes.
The exe callers being un-decompiled (`Lego_LoadMapSet`, `Level_DestroyWall`,
`0x004303a0`) does **not** re-expose the exe-side tables — with one exception, which is
about a *return value*, not about who calls whom. See Section 7.

### 2.4 A dead branch that still matters

`Water.cpp:32` sets `#define WATER_USENEWFLOODFILL true`. Under that,
`Water.cpp:148–215` — the whole original row-scan + `mergeList` + `qsort` +
compaction algorithm — is `#if`'d out. So **in the shipping build, `mergeList`,
`mergeCount`, `Water_FindPoolAndMergeRows`, `Water_AddPool` and
`Water_QsortComparePools` are never executed.** They still compile (the last three are
defined unconditionally at `Water.cpp:647, 660, 720` and remain hooked).

This is good news for relocation risk, and bad news for anyone who flips
`WATER_USENEWFLOODFILL` back to `false` after a naive relocation: that branch contains
three of the five genuine hazards (Section 6). The design below relocates the dead
branch correctly rather than leaving it as a landmine.

---

## 3. What is wrong with what we shipped

`DeepCore::WaterOverflow` (`DeepCore.hpp:206`, `DeepCore.cpp:248–262`) converts six
`Error_Fatal` sites into a warn-once-and-skip. Gated by `surviveWaterOverflow`
(`DeepCore.hpp:142`, parsed `DeepCore.cpp:408`). The six sites are
`Water.cpp:56/58` (pools, flood-fill path), `192/195` (merged-pool blocks),
`265/267` (drains), `684/687` (merge pairs), `707/709` (blocks), `723/725` (pools,
legacy path).

It is honest and it stops the crash. But it is a **truncation**, and the failure mode
it produces is worse than it sounds:

- Blocks skipped past the 100 cap (`Water.cpp:707`) are silently not part of the pool.
  `Water_InitPoolDrains` then computes drains only from surviving blocks
  (`Water.cpp:253`), and `Water_InitVertices` builds a mesh only over surviving blocks
  (`Water.cpp:344`). A large lake renders as its first 100 tiles and the rest of the
  water is invisible, un-dammable, and not drained.
- Pools skipped past the 10 cap (`Water.cpp:56`) simply do not exist. The map's water
  is there in the terrain data and does nothing.
- The header comment we shipped says the arrays "CANNOT be grown"
  (`DeepCore.hpp:130–131, 135–138`). That sentence is true about the *exe struct* and
  false about the *feature*. This document is the correction.

---

## 4. The relocation design

### 4.1 Principles

1. **`Water_Globs`, `Water_Pool`, `Water_PoolDrain`, `Water_PoolMergePair` are not
   edited. Not one byte, not one field, not one `assert_sizeof`.** After this change
   `tools/addrlint/addrlint.py` must still report 113 regions, 0 overlaps, and
   `docs/ADDRESS-MAP.md:149` must be unchanged. That is the acceptance test.
2. **Feature-gated, default off**, per `DeepCore.hpp:3–9`. Gate off ⇒ every code path
   is bit-for-bit the current build.
3. **Index-addressed, not pointer-addressed.** All internal water code moves from
   `Water_Pool*` to `uint32 poolIndex`. This is what actually makes relocation safe;
   see Section 6.
4. **The 13 hooked ABIs are preserved exactly.** `interop.cpp` is not edited at all.
   Any function whose signature mentions `Water_Pool*` keeps that signature and becomes
   a thin wrapper over an index-form twin.

### 4.2 Storage

New DLL-side storage in `world/Water.cpp`, directly modelled on the PowerGrid
precedent at `Game.cpp:169–172`:

```cpp
// Game.cpp:169-172, for reference — the sanctioned pattern:
//   // PowerGrid replacements for infinite grid sizes.
//   static std::vector<Point2I> _drainPowerBlockList;
//   static std::vector<Point2I> _poweredBlockList;
//   static std::vector<Point2I> _unpoweredBlockList;

/// CUSTOM (DeepCore): DLL-side replacement for Water_Globs::poolList, with no
/// capacity limit. Water_Globs itself is left EXACTLY as-is -- it is a reference
/// overlaid on the exe's data segment (Water.cpp:19) pinned by
/// assert_sizeof(Water_Globs, 0x29ec) (Water.h:104) -- and is simply left zeroed
/// while this feature is on.
struct DeepCorePool
{
	std::vector<Point2F>                blocks;    // was Water_Pool::blocks[100]
	std::vector<LegoRR::Water_PoolDrain> drains;   // was Water_Pool::drainList[10]
	real32 highWaterLevel               = -10000.0f;
	real32 currWaterLevel               = 0.0f;
	Gods98::Container* contMeshTrans    = nullptr;
	LegoRR::WaterFlags flags            = LegoRR::WATER_FLAG_NONE;
};

/// unique_ptr, NOT DeepCorePool by value. Two reasons, both load-bearing:
///  1. push_back on a vector<DeepCorePool> reallocates and invalidates every
///     outstanding DeepCorePool* -- and the legacy merge algorithm holds exactly
///     such pointers across pool creation (Water.cpp:690-691).
///  2. Sorting by blockCount (Water.cpp:206) must move pointers, never objects.
static std::vector<std::unique_ptr<DeepCorePool>> _dcPools;

/// CUSTOM: index-form replacement for Water_Globs::mergeList (pairs of pool indices,
/// never pointers). Only used when WATER_USENEWFLOODFILL is false.
static std::vector<std::pair<uint32, uint32>> _dcMergeList;

static bool WaterRelocated(void) { return DeepCore::settings.relocateWaterTables; }
```

`<vector>`, `<memory>`, `<algorithm>`, `<utility>` are already in scope: `common.h:34,
42, 45` (and `<queue>` at `common.h:35`, which is how `Water.cpp:68`'s `std::queue`
already compiles without a local include). **No new `#include` is required**, which
matters for the 44-warning contract.

### 4.3 The indirection layer

One static accessor per field, branching on the gate. Every one is a two-line inline;
the ternary's untaken branch is never evaluated, so the exe-side expression is not
touched when relocated (and vice versa).

```cpp
#define WATER_POOL_INVALID	((uint32)-1)

static uint32 Pool_Count(void)
{
	return WaterRelocated() ? (uint32)_dcPools.size() : waterGlobs.poolCount;
}

static uint32 Pool_BlockCount(uint32 pi)
{
	return WaterRelocated() ? (uint32)_dcPools[pi]->blocks.size()
	                        : waterGlobs.poolList[pi].blockCount;
}

static Point2F& Pool_Block(uint32 pi, uint32 bi)
{
	return WaterRelocated() ? _dcPools[pi]->blocks[bi]
	                        : waterGlobs.poolList[pi].blocks[bi];
}

static uint32 Pool_DrainCount(uint32 pi)
{
	return WaterRelocated() ? (uint32)_dcPools[pi]->drains.size()
	                        : waterGlobs.poolList[pi].drainCount;
}

static LegoRR::Water_PoolDrain& Pool_Drain(uint32 pi, uint32 di)
{
	return WaterRelocated() ? _dcPools[pi]->drains[di]
	                        : waterGlobs.poolList[pi].drainList[di];
}

static real32&              Pool_HighLevel(uint32 pi);   // ... same shape
static real32&              Pool_CurrLevel(uint32 pi);
static Gods98::Container*&  Pool_Mesh(uint32 pi);
static LegoRR::WaterFlags&  Pool_Flags(uint32 pi);

/// Create a pool. Returns WATER_POOL_INVALID if the (vanilla or sanity) cap is hit.
static uint32 Pool_New(void)
{
	if (WaterRelocated()) {
		if (_dcPools.size() >= DeepCore::settings.waterMaxPools)
			return WATER_POOL_INVALID;
		_dcPools.push_back(std::make_unique<DeepCorePool>());
		return (uint32)(_dcPools.size() - 1);
	}
	if (waterGlobs.poolCount >= WATER_MAXPOOLS)
		return WATER_POOL_INVALID;
	const uint32 pi = waterGlobs.poolCount++;
	std::memset(&waterGlobs.poolList[pi], 0, sizeof(Water_Pool)); // Water.cpp:61, vanilla
	return pi;
}

/// Append one block. False == capped.
static bool Pool_AddBlock(uint32 pi, real32 bx, real32 by)
{
	if (WaterRelocated()) {
		if (_dcPools[pi]->blocks.size() >= DeepCore::settings.waterMaxPoolBlocks)
			return false;
		_dcPools[pi]->blocks.push_back(Point2F { bx, by });
		return true;
	}
	Water_Pool* pool = &waterGlobs.poolList[pi];
	if (pool->blockCount >= WATER_MAXPOOLBLOCKS)
		return false;
	pool->blocks[pool->blockCount].x = bx;
	pool->blocks[pool->blockCount].y = by;
	pool->blockCount++;
	return true;
}

static bool Pool_AddDrain(uint32 pi, uint32 blockIndex, LegoRR::Direction dir);   // same shape
static void Pool_ResetAll(void);   // _dcPools.clear(); _dcMergeList.clear();
                                   // + memset(&waterGlobs, 0, sizeof(waterGlobs));
```

`DeepCore::settings.waterMaxPools` / `waterMaxPoolBlocks` are **sanity ceilings, not
engine limits** — defaults chosen high enough to be irrelevant to real maps
(say 4096 / 65536) so that a corrupt surface map cannot make the loader allocate
without bound. Same reasoning as `waveMaxAlive` at `DeepCore.hpp:95`.

`_dcPools[pi]` is `operator[]`, not `.at()`: bounds are the caller's contract exactly
as they are today with the raw arrays, and `.at()` would drag exceptions into a
`__cdecl` boundary that the exe calls through.

### 4.4 Loop rewrite shape

Every loop over `waterGlobs.poolCount` becomes:

```cpp
	// before (Water.cpp:248-249)
	for (uint32 i = 0; i < waterGlobs.poolCount; i++) {
		Water_Pool* pool = &waterGlobs.poolList[i];
		...
		for (uint32 j = 0; j < pool->blockCount; j++) { ... pool->blocks[j].x ... }

	// after
	for (uint32 i = 0; i < Pool_Count(); i++) {
		...
		for (uint32 j = 0; j < Pool_BlockCount(i); j++) { ... Pool_Block(i, j).x ... }
```

No `Water_Pool*` local survives anywhere in the module after the rewrite, except
inside the two ABI wrappers of Section 5.

---

## 5. The two ABI wrappers

### 5.1 `Water_FindPoolDrain` — the only pointer that escapes

```cpp
/// CUSTOM: index form. This is the only form the module itself should use.
bool LegoRR::Water_FindPoolDrainIndex(uint32 bx, uint32 by,
                                      OUT uint32* poolIndex, OPTIONAL OUT uint32* drainIndex)
{
	const Point2F DIRECTIONS[4] = { {0.0f,-1.0f}, {1.0f,0.0f}, {0.0f,1.0f}, {-1.0f,0.0f} };

	for (uint32 i = 0; i < Pool_Count(); i++) {
		for (uint32 j = 0; j < Pool_DrainCount(i); j++) {
			const Water_PoolDrain& drain = Pool_Drain(i, j);
			const Point2F& block = Pool_Block(i, drain.blockIndex);
			/// FIXME: REAL TO INT COORDINATES  (unchanged from Water.cpp:633-634)
			if (bx == static_cast<uint32>(block.x + DIRECTIONS[drain.direction].x) &&
			    by == static_cast<uint32>(block.y + DIRECTIONS[drain.direction].y))
			{
				*poolIndex = i;
				if (drainIndex) *drainIndex = j;
				return true;
			}
		}
	}
	return false;
}

// <LegoRR.exe @0046eb60>
// ABI-preserving wrapper. MAY BE ENTERED FROM ORIGINAL EXE CODE
// (Level_Debug_WKey_NeedsBlockFlags1_8_FUN_004303a0 @0x004303a0, interop.cpp:4327),
// which will dereference the result at Water_Pool offsets.
LegoRR::Water_Pool* __cdecl LegoRR::Water_FindPoolDrain(uint32 bx, uint32 by,
                                                        OPTIONAL OUT uint32* drainIndex)
{
	uint32 pi = WATER_POOL_INVALID;
	if (!Water_FindPoolDrainIndex(bx, by, &pi, drainIndex))
		return nullptr;

	if (!WaterRelocated())
		return &waterGlobs.poolList[pi];   // exact vanilla

	/// DEEPCORE: with the tables relocated there is no Water_Pool to point at. Publish
	/// a byte-exact legacy snapshot into the now-unused exe slot 0 and return THAT, so
	/// a 1999 caller reading blocks[]/blockCount/drainList[]/drainCount/levels/flags at
	/// their historic offsets reads something valid and self-consistent.
	/// waterGlobs.poolCount deliberately stays 0, so any exe loop `for (i < poolCount)`
	/// still finds nothing -- the snapshot is reachable only through this return value.
	/// Writes the exe makes to the snapshot are discarded; see Section 7.
	Water_Pool& shadow = waterGlobs.poolList[0];
	Water_PublishLegacySnapshot(pi, &shadow);   // truncates to 100 blocks / 10 drains
	return &shadow;
}
```

`Water_PublishLegacySnapshot` is a new static: `memset` the slot, copy
`min(blockCount, WATER_MAXPOOLBLOCKS)` blocks and `min(drainCount, WATER_MAXPOOLDRAINS)`
drains, copy the four scalars. It writes only inside `poolList[0]`, i.e. inside the
struct's own first `0x428` bytes — no neighbour is touched.

### 5.2 `Water_AddPoolRowBlocks`

```cpp
/// CUSTOM: index form.
void LegoRR::Water_AddPoolRowBlocksIndex(uint32 poolIndex, uint32 by,
                                         uint32 bxRowStart, uint32 bxRowEnd)
{
	for (uint32 bx = bxRowStart; bx < bxRowEnd; bx++) {
		if (!Pool_AddBlock(poolIndex, static_cast<real32>(bx), static_cast<real32>(by))) {
			/// DEEPCORE: stop growing this pool rather than terminating.
			if (DeepCore::WaterOverflow("blocks in one water pool"))
				break;
			Error_FatalF(true, "Ran out of water pool blocks for pool at index %i", poolIndex);
		}
	}
}

// <LegoRR.exe @0046ed90>
// ABI-preserving wrapper. Every known caller is C++ (interop.cpp:4333: Water_Initialise,
// Water_AddPool), and both call the index form directly; this exists only so an
// unknown exe CALL to 0x0046ed90 keeps vanilla semantics on the exe-side arrays.
void __cdecl LegoRR::Water_AddPoolRowBlocks(Water_Pool* pool, uint32 by,
                                            uint32 bxRowStart, uint32 bxRowEnd)
{
	Water_AddPoolRowBlocksIndex(poolIndex(pool), by, bxRowStart, bxRowEnd);
}
```

Note `poolIndex(pool)` (`Water.cpp:35`) is *correct* here and only here: the parameter
can only be an exe-side pointer, because that is the only thing a 1999 caller could
have. The macro's definition gains a comment saying exactly that.

---

## 6. Hazards — complete enumeration

### 6.1 `poolIndex()` pointer arithmetic — `Water.cpp:35`, used `Water.cpp:710`

```cpp
#define poolIndex(pool)  ((uint32)((ptrdiff_t)(pool) - (ptrdiff_t)waterGlobs.poolList) / sizeof(Water_Pool))
```

Under relocation `pool` points into the heap and `waterGlobs.poolList` into the exe's
data segment. The subtraction of pointers from unrelated objects is undefined
behaviour, and the practical result is a nonsense index printed into an
`Error_FatalF` message — the crash message would be garbage at exactly the moment
someone needs it. **Only use site is `Water.cpp:710`**, inside the fatal path of
`Water_AddPoolRowBlocks`. Fixed by the index-form rewrite (5.2); the macro survives
only for the ABI wrapper.

### 6.2 `mergeList` holds raw `Water_Pool*` — `Water.cpp:690–691`, read `186–187`

```cpp
	waterGlobs.mergeList[waterGlobs.mergeCount].mainPool = mainPool;
	waterGlobs.mergeList[waterGlobs.mergeCount].removedPool = pool;
```

These pointers are stored during the row scan and dereferenced *after the scan
completes* (`Water.cpp:186–201`), i.e. **held across arbitrarily many pool creations**.
In vanilla that is safe because `poolList` is a fixed array. With a naive
`std::vector<DeepCorePool>` every `push_back` past capacity reallocates and turns all
of them into dangling pointers — a textbook use-after-free that would corrupt the heap
during level load and be blamed on something else entirely.

Two independent mitigations, both applied: store **indices** in `_dcMergeList`, and
hold pools through `std::unique_ptr` so addresses are stable even if someone
reintroduces a pointer later.

Also note `Water_FindPoolAndMergeRows` returns `Water_Pool* mainPool`
(`Water.cpp:665, 698`) which the caller holds at `Water.cpp:169` and passes on at
`:174` — same class of hazard, internal only, same fix (return a `uint32` index and
`WATER_POOL_INVALID` for "none").

Both are on the `#if !WATER_USENEWFLOODFILL` branch and therefore currently dead
(Section 2.4). They must still be converted, or the flag becomes a booby trap.

### 6.3 `std::memset` over a pool — `Water.cpp:61`

```cpp
	Water_Pool* pool = &waterGlobs.poolList[waterGlobs.poolCount++];
	std::memset(pool, 0, sizeof(Water_Pool));
```

Perfectly fine on a POD `Water_Pool`; catastrophic on a `DeepCorePool`, whose first
two members are `std::vector`s — zeroing their internals leaks the buffers and then
double-frees on destruction. Replaced by `Pool_New()` (4.3), which memsets in the
vanilla branch and constructs in the relocated branch.

`Water.cpp:126` and `Water.cpp:231` memset `&waterGlobs` itself; those stay (the exe
struct must be zeroed either way) and gain a companion `_dcPools.clear()`.

### 6.4 `std::qsort` over the pool array — `Water.cpp:206`

```cpp
	std::qsort(&waterGlobs.poolList, waterGlobs.poolCount, sizeof(Water_Pool), Water_QsortComparePools);
```

`qsort` swaps by raw bytes. Byte-swapping non-trivially-copyable objects is undefined
behaviour and in practice corrupts `std::vector` internals. Replacement in the
relocated branch:

```cpp
	std::sort(_dcPools.begin(), _dcPools.end(),
	          [](const std::unique_ptr<DeepCorePool>& a, const std::unique_ptr<DeepCorePool>& b) {
	              return a->blocks.size() > b->blocks.size();   // highest first, as Water.cpp:652-656
	          });
```

Because the container holds `unique_ptr`s, the sort moves pointers and every
`DeepCorePool` keeps its address. `Water_QsortComparePools` itself remains defined
(`Water.cpp:647`) and hooked (`interop.cpp:4330`) and simply stops being called.

### 6.5 `std::memcpy` for the merge — `Water.cpp:198`

```cpp
	std::memcpy(&main->blocks[main->blockCount], &removed->blocks[0], sizeof(Point2F) * removed->blockCount);
```

Becomes, in the relocated branch, `mainBlocks.insert(mainBlocks.end(),
removedBlocks.begin(), removedBlocks.end())`, and the `WATER_MAXPOOLBLOCKS` check at
`Water.cpp:192` disappears with it. Legacy branch only.

### 6.6 Pool pointers escaping the module

Complete list of every place a `Water_Pool*` (or `Water_PoolDrain*`) leaves a
function:

| site | what escapes | destination | verdict |
|---|---|---|---|
| `Water.cpp:638` (`return pool`) | `Water_Pool*` | `Water.cpp:416`, `Water.cpp:443` (**ours**) and `0x004303a0` (**exe**) | **the blocker** — Section 7 |
| `Water.cpp:698` (`return mainPool`) | `Water_Pool*` | `Water.cpp:169` (ours) | internal; index form |
| `Water.cpp:690–691` | `Water_Pool*` × 2 | stored in `waterGlobs.mergeList` | 6.2 |
| `Water.cpp:60, 727` | `Water_Pool*` | local, passed to `Water_AddPoolRowBlocks` | index form |
| `Water.cpp:423, 488, 579, 630` | `Water_PoolDrain*` | local only, never stored or returned | safe |
| `GameState.cpp:1837, 1841` | `Water_Pool*`, `Water_PoolDrain*` | local to the debug overlay (**ours**) | redirect to accessor |

Nothing else. No `Water_Pool*` is stored in any other struct, in any `Lego_Level`
field, in any object, or in any save state.

### 6.7 Lifetime versus level teardown

`Water_RemoveAll` (`Water.cpp:222–236`, called `Game.cpp:3351`) is the only teardown.
It must clear `_dcPools` *after* removing the containers, exactly mirroring the
current order. The PowerGrid precedent does the same thing 18 lines earlier
(`Game.cpp:3333–3335`), which is a useful thing to point at in review.

### 6.8 Build-contract hazards

- **44 warnings, exactly.** `_dcPools.size()` is `size_t`, which on `x86` is 32-bit, so
  `(uint32)` casts are same-width and cannot produce C4267/C4244. They are written
  explicitly anyway.
- The ternaries returning `T&` have identical types in both arms; no C4172, no
  temporaries.
- No new `#include` (4.2), so no new header pulls in anything warning-noisy.
- `Water_QsortComparePools` becoming call-free does not warn: it is a namespace-scope
  non-static function whose address is still taken at `interop.cpp:4330`.

### 6.9 What I could not determine

1. **Whether any original exe code reads `0x0054a520` directly**, i.e. outside the 13
   hooked functions. There is no `LegoRR.exe` in the tree to disassemble (`./bin`
   contains only `OpenLRR.exe`, `OpenLRR-d.exe`, `OpenLRR-MakeExe*.exe` — the injector
   and its build, not the 1999 game), and no `.idb`/listing artifacts beyond MSVC's own
   `vc142.idb`. The evidence is indirect but converging: the struct is a per-module
   `GLOBS` struct (`Water.h:95`, tag `tags:GLOBS`), the module's entire code range is
   covered with no gaps (2.1), and upstream OpenLRR named it `waterGlobs` after
   decompiling `Water.c` as a unit. **Design mitigation:** the exe-side struct is left
   *zeroed*, so an unknown reader sees `poolCount == 0` and does nothing. The failure
   mode of the unknown is "water feature inert", not "corruption".
2. **What `Level_Debug_WKey_NeedsBlockFlags1_8_FUN_004303a0` does with the
   `Water_Pool*`.** Its own name records that upstream did not work it out either. It
   may only null-check. Section 7 assumes the worst.
3. **Whether the 20 bytes of slack after `waterGlobs` are genuinely unclaimed.**
   Irrelevant here — we use none of them — but worth recording so a future reader does
   not mistake this document for a licence to expand into them.
4. **Everything runtime.** We cannot run the game. The deliverable of this work is
   "compiles clean at the contracted warning count and is argued correct", nothing more.

---

## 7. The blocker, and its closures

**Blocker.** `Water_FindPoolDrain` (`Water.cpp:618`, hooked `interop.cpp:4328`) returns
a `Water_Pool*` to `Level_Debug_WKey_NeedsBlockFlags1_8_FUN_004303a0` at `0x004303a0`
(`interop.cpp:4327`), which is **still original machine code**: `Game.h:1557` is a live
address macro and the C++ declaration beneath it (`Game.h:1558`, `Game.cpp:3416`) is
commented out. With the tables relocated, a pointer that is not a `Water_Pool` would be
read at `Water_Pool` offsets.

**Reachability.** Exactly one call site in the whole tree:

```cpp
// Game.cpp:2221-2224, inside our Lego_HandleWorldDebugKeys (Game.cpp:2172)
	/// DEBUG KEYBIND: [W]  "Performs unknown behaviour with the unfinished 'flood water' surface."
	if (Shortcut_IsDown(ShortcutID::Debug_Unknown_Water)) {
		Level_Debug_WKey_NeedsBlockFlags1_8_FUN_004303a0(legoGlobs.currLevel, 0, mbx, mby);
	}
```

Bound to `KEY_W` (`Shortcuts.cpp:117`, enum `Shortcuts.hpp:137`). It is a debug
keybind in a debug-key handler that is **our own C++**. That is what makes this
closable without decompiling anything.

**C1 — gate the call site (primary, required).**

```cpp
	if (Shortcut_IsDown(ShortcutID::Debug_Unknown_Water)) {
		/// DEEPCORE: 0x004303a0 is still original 1999 machine code and is the only
		/// consumer of a Water_Pool* outside this DLL (interop.cpp:4327). With the water
		/// tables relocated DLL-side there is no exe-resident Water_Pool for it to read.
		/// This debug keybind's exact behaviour was never decompiled ("unknown"), so we
		/// do not attempt to emulate it -- we decline it and say so, once.
		if (!DeepCore::settings.relocateWaterTables) {
			Level_Debug_WKey_NeedsBlockFlags1_8_FUN_004303a0(legoGlobs.currLevel, 0, mbx, mby);
		}
		else {
			DeepCore::WarnOnce_DebugWaterKeyDisabled();
		}
	}
```

Cost: one debug keybind, in a build where the user has explicitly opted into
relocation. Note `Game.cpp:2223` already needs `using namespace LegoRR;` in scope for
the macro to parse — it is inside a `LegoRR::` function so this is already satisfied;
do not try to qualify the macro (macros are not namespace members).

**C2 — the legacy shadow slot (defence in depth, cheap, keep it).** Section 5.1. Even
with C1 in place, `Water_FindPoolDrain` never returns a non-`Water_Pool` pointer to
anyone. If some undiscovered exe path reaches it, it reads a valid truncated snapshot
rather than heap garbage. C2 costs ~15 lines and one `memcpy` on a keypress-rate path.

**Explicitly rejected:** implementing `Level_Debug_WKey_NeedsBlockFlags1_8_FUN_004303a0`
ourselves. Upstream's name for it says its behaviour is unknown; a guessed
reimplementation installed by `hook_write_jmpret` is irreversible
(`HOOK-ARCHITECTURE.md:34` — destructive, no trampoline, no way to call the original)
and would be a much larger risk than losing a debug key.

---

## 8. Complete redirection table

Every access site that must change. `P` = pool index, `B` = block index, `D` = drain
index. "vanilla-only" = the site stays as-is because it is inside the `!WaterRelocated()`
branch of an accessor.

### 8.1 `src/openlrr/game/world/Water.cpp`

| line | current expression | replacement |
|---:|---|---|
| 35 | `#define poolIndex(pool)` | keep; comment "exe-side pointers only, ABI wrapper use" |
| 56, 58 | `waterGlobs.poolCount >= WATER_MAXPOOLS` | `Pool_New()` returning `WATER_POOL_INVALID` |
| 60 | `&waterGlobs.poolList[waterGlobs.poolCount++]` | `const uint32 pi = Pool_New();` |
| 61 | `std::memset(pool, 0, sizeof(Water_Pool))` | folded into `Pool_New()` (6.3) |
| 88 | `Water_AddPoolRowBlocks(pool, by, bxStart, bxEnd)` | `Water_AddPoolRowBlocksIndex(pi, …)` |
| 126 | `std::memset(&waterGlobs, 0, sizeof(waterGlobs))` | keep; add `Pool_ResetAll()` |
| 128, 129 | `waterGlobs.poolCount = 0; waterGlobs.mergeCount = 0;` | keep (exe view stays zeroed) |
| 131 | `waterGlobs.digDepth = level->DigDepth` | keep (write-only field, `Water.h:101`) |
| 169 | `Water_Pool* pool = Water_FindPoolAndMergeRows(...)` | `uint32 pi = Water_FindPoolAndMergeRowsIndex(...)` |
| 171 | `Water_AddPool(by, rowStart, rowEnd)` | unchanged (void) |
| 174 | `Water_AddPoolRowBlocks(pool, …)` | `Water_AddPoolRowBlocksIndex(pi, …)` |
| 185 | `waterGlobs.mergeCount` | `Merge_Count()` |
| 186, 187 | `waterGlobs.mergeList[i].mainPool / .removedPool` | `Merge_Main(i) / Merge_Removed(i)` → indices |
| 189 | `removed->blockCount` | `Pool_BlockCount(rem)` |
| 192, 195 | `main->blockCount + removed->blockCount > WATER_MAXPOOLBLOCKS` | vanilla-only; relocated branch has no cap |
| 198 | `std::memcpy(&main->blocks[...], …)` | `insert()` in relocated branch (6.5) |
| 200, 201 | `main->blockCount += …; removed->blockCount = 0;` | `Pool_AppendBlocks(main, rem); Pool_ClearBlocks(rem);` |
| 206 | `std::qsort(&waterGlobs.poolList, …)` | `std::sort` on `_dcPools` (6.4) |
| 209–213 | `waterGlobs.poolCount`, `poolList[i].blockCount` | `Pool_Count()`, `Pool_BlockCount(i)`, `Pool_Truncate(i)` |
| 217, 218 | `Water_InitPoolDrains / InitVertices` | unchanged |
| 224 | `waterGlobs.poolCount` | `Pool_Count()` |
| 225 | `Water_Pool* pool = &waterGlobs.poolList[i]` | delete |
| 227, 228 | `pool->contMeshTrans` | `Pool_Mesh(i)` |
| 231 | `std::memset(&waterGlobs, …)` | keep; add `Pool_ResetAll()` |
| 248 | `waterGlobs.poolCount` | `Pool_Count()` |
| 249 | `Water_Pool* pool = &waterGlobs.poolList[i]` | delete |
| 251 | `pool->drainCount = 0` | `Pool_ClearDrains(i)` |
| 253 | `pool->blockCount` | `Pool_BlockCount(i)` |
| 257, 258 | `pool->blocks[j].x / .y` | `Pool_Block(i, j).x / .y` |
| 265, 267 | `pool->drainCount >= WATER_MAXPOOLDRAINS` | `Pool_AddDrain` returning false |
| 272, 273 | `pool->drainList[pool->drainCount].blockIndex / .direction` | folded into `Pool_AddDrain(i, j, dir)` |
| 274 | `pool->drainCount++` | folded into `Pool_AddDrain` |
| 269 | `blockValue(level, bx, by)` in `Error_DebugF` | unchanged; keep `using namespace LegoRR;` semantics |
| 304 | `waterGlobs.poolCount` | `Pool_Count()` |
| 305 | `Water_Pool* pool = …` | delete |
| 307 | `pool->highWaterLevel = -10000.0f` | `Pool_HighLevel(i) = -10000.0f` |
| 309 | `pool->blockCount` | `Pool_BlockCount(i)` |
| 313, 314 | `pool->blocks[j].x / .y` | `Pool_Block(i, j)` |
| 319, 320 | `pool->blocks[j].x + DIRECTIONS[dir].x` | `Pool_Block(i, j)` |
| 329 | `pool->highWaterLevel = std::max(...)` | `Pool_HighLevel(i)` |
| 334 | `pool->flags \|= WATER_FLAG_VISIBLE` | `Pool_Flags(i)` |
| 339 | `pool->currWaterLevel = pool->highWaterLevel` | `Pool_CurrLevel(i) = Pool_HighLevel(i)` |
| 341, 342 | `pool->contMeshTrans`, `pool->flags` | `Pool_Mesh(i)`, `Pool_Flags(i)` |
| 344 | `pool->blockCount` | `Pool_BlockCount(i)` |
| 345, 348 | `pool->contMeshTrans` | `Pool_Mesh(i)` |
| 354, 355 | `pool->blocks[j].x / .y` | `Pool_Block(i, j)` |
| 361 | `pool->highWaterLevel` | `Pool_HighLevel(i)` |
| 369 | `pool->contMeshTrans` | `Pool_Mesh(i)` |
| 377 | `waterGlobs.poolCount` | `Pool_Count()` |
| 378 | `waterGlobs.poolList[i].contMeshTrans` | `Pool_Mesh(i)` |
| 416 | `Water_Pool* pool = Water_FindPoolDrain(bx, by, &drainIndex)` | `Water_FindPoolDrainIndex(bx, by, &pi, &di)` |
| 418, 419 | `pool->flags` | `Pool_Flags(pi)` |
| 421 | `pool->contMeshTrans` | `Pool_Mesh(pi)` |
| 423 | `Water_PoolDrain* drain = &pool->drainList[drainIndex]` | `Water_PoolDrain& drain = Pool_Drain(pi, di)` |
| 424–426 | `drain->active / elapsedUp_c / elapsedDown_10` | `drain.` |
| 429, 430 | `pool->blocks[drain->blockIndex]` | `Pool_Block(pi, drain.blockIndex)` |
| 433, 434 | `drain->direction`, `drain->drainWaterLevel` | `drain.` |
| 443 | `Water_Pool* pool = Water_FindPoolDrain(...)` | `Water_FindPoolDrainIndex(...)` |
| 446, 447 | `pool->flags` | `Pool_Flags(pi)` |
| 449 | `pool->contMeshTrans` | `Pool_Mesh(pi)` |
| 450 | `pool->drainList[drainIndex].active = false` | `Pool_Drain(pi, di).active = false` |
| 462 | `waterGlobs.poolCount` | `Pool_Count()` |
| 463 | `Water_Pool* pool = …` | delete |
| 467–472, 480 | `pool->flags` | `Pool_Flags(i)` |
| 484 | `pool->flags & WATER_FLAG_FULL` | `Pool_Flags(i)` |
| 485 | `pool->highWaterLevel` | `Pool_HighLevel(i)` |
| 487 | `pool->drainCount` | `Pool_DrainCount(i)` |
| 488 | `Water_PoolDrain* drain = &pool->drainList[j]` | `Water_PoolDrain& drain = Pool_Drain(i, j)` |
| 491, 504 | `pool->currWaterLevel` | `Pool_CurrLevel(i)` |
| 493, 503–514 | `drain->…` | `drain.…` |
| 498 | `pool->flags` | `Pool_Flags(i)` |
| 519 | `pool->currWaterLevel - pool->highWaterLevel` | `Pool_CurrLevel(i) - Pool_HighLevel(i)` |
| 522–535 | `pool->flags`, `pool->currWaterLevel` | `Pool_Flags(i)`, `Pool_CurrLevel(i)` |
| 540 | `pool->contMeshTrans` | `Pool_Mesh(i)` |
| 542 | `pool->blockCount` | `Pool_BlockCount(i)` |
| 545, 551 | `pool->contMeshTrans` | `Pool_Mesh(i)` |
| 549 | `pool->currWaterLevel` | `Pool_CurrLevel(i)` |
| 576 | `waterGlobs.poolCount` | `Pool_Count()` |
| 577 | `const Water_Pool* pool = …` | delete |
| 578 | `pool->drainCount` | `Pool_DrainCount(i)` |
| 579 | `const Water_PoolDrain* drain = &pool->drainList[j]` | `const Water_PoolDrain& drain = Pool_Drain(i, j)` |
| 583, 584 | `pool->blocks[drain->blockIndex]` | `Pool_Block(i, drain.blockIndex)` |
| 585, 605, 610 | `drain->direction / elapsedUp_c / elapsedDown_10` | `drain.` |
| 627 | `waterGlobs.poolCount` | `Pool_Count()` |
| 628 | `Water_Pool* pool = …` | delete |
| 629, 630 | `pool->drainCount`, `&pool->drainList[j]` | `Pool_DrainCount(i)`, `Pool_Drain(i, j)` |
| 633, 634 | `pool->blocks[drain->blockIndex]` | `Pool_Block(i, drain.blockIndex)` |
| 638 | `return pool` | `*poolIndex = i; return true;` (index form) + ABI wrapper (5.1) |
| 647–657 | `Water_QsortComparePools` | keep verbatim (still hooked, no longer called) |
| 665 | `Water_Pool* mainPool = nullptr` | `uint32 mainPool = WATER_POOL_INVALID` |
| 668 | `waterGlobs.poolCount` | `Pool_Count()` |
| 669 | `Water_Pool* pool = …` | delete |
| 671 | `pool->blockCount` | `Pool_BlockCount(i)` |
| 675, 676 | `pool->blocks[j].x / .y` | `Pool_Block(i, j)` |
| 678–682 | `mainPool == nullptr`, `mainPool != pool` | `== WATER_POOL_INVALID`, `!= i` |
| 684, 687 | `waterGlobs.mergeCount >= WATER_MAXMERGEPOOLS` | `Merge_Add()` returning false |
| 690–692 | `waterGlobs.mergeList[...] = pointers` | `Merge_Add(mainPool, i)` — **indices** (6.2) |
| 698 | `return mainPool` | returns `uint32` index |
| 707, 709 | `pool->blockCount >= WATER_MAXPOOLBLOCKS` | `Pool_AddBlock` returning false |
| 710 | `poolIndex(pool)` in the fatal message | `poolIndex` parameter (6.1) |
| 713–715 | `pool->blocks[pool->blockCount] = …; blockCount++` | `Pool_AddBlock(pi, bx, by)` |
| 723, 725 | `waterGlobs.poolCount >= WATER_MAXPOOLS` | `Pool_New()` |
| 727 | `Water_Pool* pool = &waterGlobs.poolList[waterGlobs.poolCount]` | `const uint32 pi = Pool_New()` |
| 728 | `pool->blockCount = 0` | folded into `Pool_New()` |
| 729 | `Water_AddPoolRowBlocks(pool, …)` | `Water_AddPoolRowBlocksIndex(pi, …)` |
| 731 | `waterGlobs.poolCount++` | folded into `Pool_New()` |

### 8.2 `src/openlrr/game/GameState.cpp` (debug overlay)

| line | current | replacement |
|---:|---|---|
| 1832 | `waterGlobs.poolCount == 0` | `Water_GetPoolCount() == 0` |
| 1837 | `auto pool = &waterGlobs.poolList[0]` | `Water_PoolView view; Water_GetPoolView(0, &view);` |
| 1838 | `pool->currWaterLevel`, `pool->highWaterLevel` | `view.currWaterLevel`, `view.highWaterLevel` |
| 1840 | `pool->drainCount` | `view.drainCount` |
| 1841 | `auto drain = &pool->drainList[j]` | `Water_PoolDrain drain; Water_GetPoolDrain(0, j, &drain);` |
| 1842, 1843 | `pool->blocks[drain->blockIndex]` | `Point2F blk; Water_GetPoolBlock(0, drain.blockIndex, &blk);` |
| 1844–1850 | `drain->…` | `drain.…` |

### 8.3 `src/openlrr/game/Game.cpp`

| line | current | replacement |
|---:|---|---|
| 2222–2224 | `[W]` debug key → `0x004303a0` | gate on `!relocateWaterTables` (closure C1, Section 7) |
| 3351 | `Water_RemoveAll()` | unchanged — already the right teardown hook |

### 8.4 `src/openlrr/interop.cpp`

**No change.** All 13 hooks stay exactly as they are (`interop.cpp:4312–4336`). This
is deliberate and is the reason the design keeps the ABI wrappers.

---

## 9. Implementation plan, file by file

**Step 0 — baseline.** Record `Debug|x86` and `Release|x86` (v142) at 0 errors / 44
warnings, and `python tools/addrlint/addrlint.py` at 113 regions / 0 overlaps. These are
the before-and-after contract.

**Step 1 — `src/openlrr/game/DeepCore.hpp`.** Add to `struct Settings`, next to
`surviveWaterOverflow` (`:142`):

```cpp
	/// Relocate the water tables to DLL-side storage, removing the caps entirely.
	///
	/// Supersedes surviveWaterOverflow: instead of skipping the water a map cannot
	/// fit into WATER_MAXPOOLS/WATER_MAXPOOLBLOCKS, all of it is simulated. Water_Globs
	/// itself is UNCHANGED -- still 0x29ec bytes at 0x0054a520 (Water.cpp:19,
	/// Water.h:104) -- and is simply left zeroed while this is on, so any original exe
	/// code that reads it sees "no pools" and does nothing.
	///
	/// Side effect: the [W] debug keybind (Game.cpp:2222) is disabled while this is on,
	/// because it enters original 1999 machine code at 0x004303a0 that would read a
	/// Water_Pool out of the exe's data segment. See docs/research/water-relocation.md.
	bool relocateWaterTables = false;

	/// Sanity ceilings, NOT engine limits. They exist only so a corrupt surface map
	/// cannot make the loader allocate without bound. Ignored unless relocateWaterTables.
	uint32 waterMaxPools      = 4096;
	uint32 waterMaxPoolBlocks = 65536;
```

Also declare `void WarnOnce_DebugWaterKeyDisabled(void);` beside `WaterOverflow`
(`:206`).

**Step 2 — `src/openlrr/game/DeepCore.cpp`.** Parse the three keys beside
`SurviveWaterOverflow` (`:408`), log them in the `verboseStartup` block (`:412–420`),
include `relocateWaterTables` in `IsAnyFeatureEnabled()` (`:58`), and implement the
warn-once helper against the existing `_waterOverflowWarned` map pattern (`:255–260`).

**Step 3 — `data/Settings/DeepCore.cfg`.** Document `RelocateWaterTables`,
`WaterMaxPools`, `WaterMaxPoolBlocks`; note that it supersedes `SurviveWaterOverflow`
and that `[W]` is disabled while on.

**Step 4 — `src/openlrr/game/world/Water.h`.** Add **only** CUSTOM declarations. Do
not touch any struct, any `assert_sizeof`, or any constant:

```cpp
/// CUSTOM: read-only snapshot of one pool, for code outside this module.
/// NOT an overlaid struct -- DLL-side only, no assert_sizeof, safe to change.
struct Water_PoolView
{
	uint32     blockCount;
	uint32     drainCount;
	real32     highWaterLevel;
	real32     currWaterLevel;
	WaterFlags flags;
};

/// CUSTOM: pool accessors. Valid whether or not the tables are relocated.
uint32 Water_GetPoolCount(void);
bool   Water_GetPoolView (uint32 poolIndex, OUT Water_PoolView* view);
bool   Water_GetPoolBlock(uint32 poolIndex, uint32 blockIndex, OUT Point2F* block);
bool   Water_GetPoolDrain(uint32 poolIndex, uint32 drainIndex, OUT Water_PoolDrain* drain);

/// CUSTOM: index forms of the two functions whose ABI carries a Water_Pool*.
bool Water_FindPoolDrainIndex(uint32 bx, uint32 by, OUT uint32* poolIndex,
                              OPTIONAL OUT uint32* drainIndex);
void Water_AddPoolRowBlocksIndex(uint32 poolIndex, uint32 by,
                                 uint32 bxRowStart, uint32 bxRowEnd);
uint32 Water_FindPoolAndMergeRowsIndex(uint32 by, uint32 bxRowStart, uint32 bxRowEnd);
```

**Step 5 — `src/openlrr/game/world/Water.cpp`.** The bulk. In order:
storage + gate (4.2) → accessors (4.3) → index-form twins (5.1, 5.2) → mechanical
rewrite of all 13 bodies per the table in 8.1 → `Water_PublishLegacySnapshot` →
public accessors from Step 4. Compile after the accessors and again after each
function group; the rewrite is mechanical and a mid-file compile catches a mistyped
index immediately.

**Step 6 — `src/openlrr/game/GameState.cpp:1832–1850`.** Swap the overlay onto the
public accessors (8.2). This also removes the last `waterGlobs` reference outside the
module, which is worth stating in the commit message.

**Step 7 — `src/openlrr/game/Game.cpp:2222–2224`.** Closure C1 (Section 7).

**Step 8 — verify.**
- `Debug|x86` and `Release|x86`, v142: 0 errors, **exactly 44 warnings**.
- `python tools/addrlint/addrlint.py`: 113 regions, 0 overlaps; `git diff` on
  `docs/ADDRESS-MAP.md` and `docs/address-map.json` must be **empty**.
- `git diff src/openlrr/interop.cpp` must be **empty**.
- `grep -n "assert_sizeof" src/openlrr/game/world/Water.h` must be unchanged (4 lines:
  `:67, :83, :92, :104`).
- `grep -rn "waterGlobs" src/` must show hits only in `world/Water.{h,cpp}`.
- Gate off: `git diff` should show no behavioural path change — verify by reading, since
  we cannot run.

**Step 9 — retire the old patch (later, separate commit).** Once relocation is
confidence-tested by someone who can actually run the game, `surviveWaterOverflow`
becomes redundant. Keep both for now; they are independent gates and the old one is the
fallback if relocation misbehaves in the field.

---

## 10. DECISION

**Full relocation is safe to do today, conditional on closure C1.**

The conditions the task set out are met:

- `Water_Globs` stays byte-identical: the design adds storage, never widens a struct.
  `assert_sizeof(Water_Globs, 0x29ec)` (`Water.h:104`) and the `0x0054a520` binding
  (`Water.cpp:19`) are untouched, and `docs/address-map.json` must diff empty as the
  acceptance test.
- All 13 exe water functions are ours and hooked (`Water.h`: 13 commented macros, 0
  live; `interop.cpp:4312–4336`: 13 hooks). No original water code is reachable.
- `waterGlobs` has exactly two readers outside the module, both ours
  (`GameState.cpp:1832, 1837`).
- `interop.cpp` needs no edit, because the 13 ABIs are preserved by wrappers.

**The single blocker is named precisely:** `Water_FindPoolDrain` (`Water.cpp:638`)
returns a `Water_Pool*` into original 1999 machine code at `0x004303a0`
(`interop.cpp:4327`, live macro `Game.h:1557`). It is reachable from exactly one call
site, our own `[W]` debug keybind at `Game.cpp:2222–2224`. Gate that call site
(closure C1) and the pointer never escapes; add the legacy shadow slot (closure C2) and
even an undiscovered escape reads a valid truncated `Water_Pool` instead of garbage.

**If C1 is judged too aggressive** — someone wants `[W]` to keep working — the largest
safe subset is: relocate `Water_Pool::blocks[]` and `Water_Pool::drainList[]` only,
keeping `poolList[10]` on the exe side, and keeping the legacy shadow slot. `Water_Pool*`
values then remain exe-resident and stay valid for `0x004303a0`; the 100-block and
10-drain caps (the ones that actually kill community maps) are gone; only the 10-pool
cap survives, still handled by the existing `WaterOverflow` skip. That variant is
strictly smaller, needs no `Game.cpp` edit, and every hazard in Section 6 except 6.2's
`mergeList` still applies to it.

**Recommendation: do the full relocation with C1 + C2.** The half-measure keeps a cap
alive to protect a debug keybind whose behaviour upstream never worked out, and the
full version is the marquee demonstration that a fixed 1999 table can be lifted into
the DLL without moving a single byte of the exe's data segment.
