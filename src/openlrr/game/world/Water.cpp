// Water.cpp :
//

#include "../../engine/core/Maths.h"

#include "../Game.h"

#include "../DeepCore.hpp"
#include "Water.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @0054a520>
LegoRR::Water_Globs & LegoRR::waterGlobs = *(LegoRR::Water_Globs*)0x0054a520;

/// CUSTOM: Used for East West flood fill algorithm.
static std::vector<bool> _filledPoolBlocks;


/// CUSTOM (DeepCore): DLL-side replacement for one Water_Pool, with no capacity limit
/// on either of its two tables.
///
/// Water_Globs itself is left EXACTLY as it was. It is a reference overlaid on the
/// original executable's data segment (the binding above, address 0x0054a520) and is
/// pinned by assert_sizeof(Water_Globs, 0x29ec) in Water.h, so not one field of it,
/// nor of Water_Pool, Water_PoolDrain or Water_PoolMergePair, may move. While this
/// feature is on the exe struct is simply left zeroed, which means any original code
/// that still reads it sees "no pools" and does nothing.
struct DeepCorePool
{
	std::vector<Point2F>                 blocks;                  // was Water_Pool::blocks[WATER_MAXPOOLBLOCKS]
	std::vector<LegoRR::Water_PoolDrain> drains;                  // was Water_Pool::drainList[WATER_MAXPOOLDRAINS]
	real32                               highWaterLevel = -10000.0f;
	real32                               currWaterLevel = 0.0f;
	Gods98::Container*                   contMeshTrans  = nullptr;
	LegoRR::WaterFlags                   flags          = LegoRR::WATER_FLAG_NONE;
};

/// CUSTOM (DeepCore): replacement for Water_Globs::poolList. Empty unless
/// relocateWaterTables is on. Modelled on the PowerGrid std::vector replacements in
/// Game.cpp, which are the sanctioned in-tree pattern for DLL-side storage.
///
/// unique_ptr, NOT DeepCorePool by value, and both reasons are load-bearing:
///  1. push_back on a vector<DeepCorePool> reallocates and invalidates every
///     outstanding DeepCorePool*, and the legacy merge algorithm holds pool
///     references across arbitrarily many pool creations.
///  2. Sorting by block count must move pointers, never objects.
static std::vector<std::unique_ptr<DeepCorePool>> _dcPools;

/// CUSTOM (DeepCore): replacement for Water_Globs::mergeList. Pairs of pool INDICES,
/// never pointers -- see the hazard note on Merge_Add(). Only ever populated when
/// WATER_USENEWFLOODFILL is false.
static std::vector<std::pair<uint32, uint32>> _dcMergeList;

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define WATER_USENEWFLOODFILL	true

/// POINTER MATH: Cast to ptrdiff_t before subtraction to avoid C++ pointer math conversion.
/// EXE-SIDE POINTERS ONLY. This is pointer arithmetic against waterGlobs.poolList, so
/// it is only meaningful for a pointer that genuinely points into that array. With the
/// tables relocated, this module's pools live on the heap and subtracting them from
/// poolList is undefined behaviour, so the only surviving use is the ABI wrapper
/// Water_AddPoolRowBlocks() -- whose Water_Pool* can only have come from the original
/// executable, or from the legacy shadow slot, which IS poolList[0]. Everything inside
/// this module addresses pools by index instead.
#define poolIndex(pool)			((uint32)((ptrdiff_t)(pool) - (ptrdiff_t)LegoRR::waterGlobs.poolList) / sizeof(LegoRR::Water_Pool))

#define blockCanFill(bx, by)	(blockValue(level,(bx),(by)).terrain==Lego_SurfaceType_Water && \
								 !_filledPoolBlocks[blockIndex(level,(bx),(by))])

/// CUSTOM (DeepCore): "no such pool", as returned by Pool_New() and the index forms.
#define WATER_POOL_INVALID		((uint32)-1)

#pragma endregion

/**********************************************************************************
 ******** DeepCore table relocation
 **********************************************************************************/

#pragma region DeepCore relocation

/// CUSTOM (DeepCore): true when the water tables live in _dcPools instead of in the
/// exe-overlaid Water_Globs.
///
/// Every accessor below is a single ternary whose untaken arm is never evaluated, so
/// with the gate off each one reduces to exactly the expression it replaced. The
/// module then behaves identically to the build before this feature existed --
/// including the SurviveWaterOverflow degradation, which is still driven by the same
/// DeepCore::WaterOverflow() calls at the same six sites.
static bool WaterRelocated(void)
{
	return DeepCore::settings.relocateWaterTables;
}


static uint32 Pool_Count(void)
{
	return WaterRelocated() ? (uint32)_dcPools.size() : LegoRR::waterGlobs.poolCount;
}

/// True when another pool cannot be added. Relocated, the ceiling is a config sanity
/// limit rather than an engine limit.
static bool Pool_IsFull(void)
{
	return WaterRelocated() ? ((uint32)_dcPools.size() >= DeepCore::settings.waterMaxPools)
	                        : (LegoRR::waterGlobs.poolCount >= WATER_MAXPOOLS);
}

/// Append one empty pool and return its index. Callers test Pool_IsFull() first,
/// exactly where the original code tested its array bound.
static uint32 Pool_New(void)
{
	if (WaterRelocated()) {
		_dcPools.push_back(std::make_unique<DeepCorePool>());
		return (uint32)(_dcPools.size() - 1);
	}
	const uint32 pi = LegoRR::waterGlobs.poolCount++;
	// Zeroing a POD Water_Pool is fine. Zeroing a DeepCorePool would flatten the
	// std::vectors inside it, leaking their buffers and then double-freeing on
	// destruction -- which is exactly why this memset lives behind the gate.
	std::memset(&LegoRR::waterGlobs.poolList[pi], 0, sizeof(LegoRR::Water_Pool));
	return pi;
}

/// Drop all water storage, DLL-side and exe-side. Replaces the bare memset of
/// waterGlobs; the exe struct must still be zeroed either way.
static void Pool_ResetAll(void)
{
	_dcPools.clear();
	_dcMergeList.clear();
	std::memset(&LegoRR::waterGlobs, 0, sizeof(LegoRR::waterGlobs));
}


static uint32 Pool_BlockCount(uint32 pi)
{
	return WaterRelocated() ? (uint32)_dcPools[pi]->blocks.size()
	                        : LegoRR::waterGlobs.poolList[pi].blockCount;
}

static Point2F& Pool_Block(uint32 pi, uint32 bi)
{
	return WaterRelocated() ? _dcPools[pi]->blocks[bi]
	                        : LegoRR::waterGlobs.poolList[pi].blocks[bi];
}

static bool Pool_BlocksFull(uint32 pi)
{
	return WaterRelocated() ? ((uint32)_dcPools[pi]->blocks.size() >= DeepCore::settings.waterMaxPoolBlocks)
	                        : (LegoRR::waterGlobs.poolList[pi].blockCount >= WATER_MAXPOOLBLOCKS);
}

static void Pool_AddBlock(uint32 pi, real32 bx, real32 by)
{
	if (WaterRelocated()) {
		_dcPools[pi]->blocks.push_back(Point2F { bx, by });
		return;
	}
	LegoRR::Water_Pool* pool = &LegoRR::waterGlobs.poolList[pi];
	pool->blocks[pool->blockCount].x = bx;
	pool->blocks[pool->blockCount].y = by;
	pool->blockCount++;
}

static void Pool_ClearBlocks(uint32 pi)
{
	if (WaterRelocated()) {
		_dcPools[pi]->blocks.clear();
		return;
	}
	LegoRR::waterGlobs.poolList[pi].blockCount = 0;
}

/// True when merging remPi into mainPi would overflow the vanilla block table.
/// Always false when relocated: there is no fixed table left to overflow.
static bool Pool_MergeOverflows(uint32 mainPi, uint32 remPi)
{
	if (WaterRelocated())
		return false;
	return (LegoRR::waterGlobs.poolList[mainPi].blockCount +
			LegoRR::waterGlobs.poolList[remPi].blockCount) > WATER_MAXPOOLBLOCKS;
}

/// Append every block of remPi onto mainPi. remPi keeps its own blocks; the caller
/// clears them afterwards, exactly as the original did.
static void Pool_AppendBlocks(uint32 mainPi, uint32 remPi)
{
	if (WaterRelocated()) {
		std::vector<Point2F>& mainBlocks = _dcPools[mainPi]->blocks;
		const std::vector<Point2F>& removedBlocks = _dcPools[remPi]->blocks;
		mainBlocks.insert(mainBlocks.end(), removedBlocks.begin(), removedBlocks.end());
		return;
	}
	LegoRR::Water_Pool* main    = &LegoRR::waterGlobs.poolList[mainPi];
	LegoRR::Water_Pool* removed = &LegoRR::waterGlobs.poolList[remPi];
	// Some form of overlapped copying...?
	std::memcpy(&main->blocks[main->blockCount], &removed->blocks[0], sizeof(Point2F) * removed->blockCount);
	main->blockCount += removed->blockCount;
}

/// Sort pools by block count, highest to lowest.
static void Pool_SortByBlockCount(void)
{
	if (WaterRelocated()) {
		// qsort swaps raw bytes, which would shred the std::vectors inside a
		// DeepCorePool. std::sort over unique_ptrs moves pointers instead, so every
		// pool keeps its address and any reference held across the sort stays valid.
		std::sort(_dcPools.begin(), _dcPools.end(),
			[](const std::unique_ptr<DeepCorePool>& a, const std::unique_ptr<DeepCorePool>& b) {
				return a->blocks.size() > b->blocks.size();
			});
		return;
	}
	std::qsort(&LegoRR::waterGlobs.poolList, LegoRR::waterGlobs.poolCount,
			   sizeof(LegoRR::Water_Pool), LegoRR::Water_QsortComparePools);
}

/// Drop every pool from `count` onwards.
static void Pool_Truncate(uint32 count)
{
	if (WaterRelocated()) {
		_dcPools.resize(count);
		return;
	}
	LegoRR::waterGlobs.poolCount = count;
}


static uint32 Pool_DrainCount(uint32 pi)
{
	return WaterRelocated() ? (uint32)_dcPools[pi]->drains.size()
	                        : LegoRR::waterGlobs.poolList[pi].drainCount;
}

static LegoRR::Water_PoolDrain& Pool_Drain(uint32 pi, uint32 di)
{
	return WaterRelocated() ? _dcPools[pi]->drains[di]
	                        : LegoRR::waterGlobs.poolList[pi].drainList[di];
}

static bool Pool_DrainsFull(uint32 pi)
{
	// Relocated, a pool cannot have more drains than it has blocks, so the block
	// ceiling is the only ceiling that needs to exist.
	return WaterRelocated() ? false
	                        : (LegoRR::waterGlobs.poolList[pi].drainCount >= WATER_MAXPOOLDRAINS);
}

static void Pool_AddDrain(uint32 pi, uint32 blockIdx, Direction dir)
{
	if (WaterRelocated()) {
		LegoRR::Water_PoolDrain drain = {};
		drain.blockIndex = blockIdx; // Pool blocks index
		drain.direction  = dir;      // direction index
		_dcPools[pi]->drains.push_back(drain);
		return;
	}
	LegoRR::Water_Pool* pool = &LegoRR::waterGlobs.poolList[pi];
	pool->drainList[pool->drainCount].blockIndex = blockIdx; // Pool blocks index
	pool->drainList[pool->drainCount].direction  = dir;      // direction index
	pool->drainCount++;
}

static void Pool_ClearDrains(uint32 pi)
{
	if (WaterRelocated()) {
		_dcPools[pi]->drains.clear();
		return;
	}
	LegoRR::waterGlobs.poolList[pi].drainCount = 0;
}


static real32& Pool_HighLevel(uint32 pi)
{
	return WaterRelocated() ? _dcPools[pi]->highWaterLevel
	                        : LegoRR::waterGlobs.poolList[pi].highWaterLevel;
}

static real32& Pool_CurrLevel(uint32 pi)
{
	return WaterRelocated() ? _dcPools[pi]->currWaterLevel
	                        : LegoRR::waterGlobs.poolList[pi].currWaterLevel;
}

static Gods98::Container*& Pool_Mesh(uint32 pi)
{
	return WaterRelocated() ? _dcPools[pi]->contMeshTrans
	                        : LegoRR::waterGlobs.poolList[pi].contMeshTrans;
}

static LegoRR::WaterFlags& Pool_Flags(uint32 pi)
{
	return WaterRelocated() ? _dcPools[pi]->flags
	                        : LegoRR::waterGlobs.poolList[pi].flags;
}


static uint32 Merge_Count(void)
{
	return WaterRelocated() ? (uint32)_dcMergeList.size() : LegoRR::waterGlobs.mergeCount;
}

static bool Merge_IsFull(void)
{
	// Relocated, the merge list grows with the pool list; it can never hold more
	// pairs than there are pools to pair up, so there is no ceiling to hit.
	return WaterRelocated() ? false : (LegoRR::waterGlobs.mergeCount >= WATER_MAXMERGEPOOLS);
}

/// Register a merge to be performed at the end of Water_Initialise.
///
/// INDICES, never pointers. The original stored raw Water_Pool* here and dereferenced
/// them only after the whole row scan had finished, i.e. it held them across
/// arbitrarily many pool creations. That is safe against a fixed array and a
/// use-after-free against anything that can reallocate, which is the second reason
/// _dcPools holds unique_ptrs.
static void Merge_Add(uint32 mainPi, uint32 remPi)
{
	if (WaterRelocated()) {
		_dcMergeList.push_back(std::pair<uint32, uint32>(mainPi, remPi));
		return;
	}
	LegoRR::Water_PoolMergePair* pair = &LegoRR::waterGlobs.mergeList[LegoRR::waterGlobs.mergeCount];
	pair->mainPool    = &LegoRR::waterGlobs.poolList[mainPi];
	pair->removedPool = &LegoRR::waterGlobs.poolList[remPi];
	LegoRR::waterGlobs.mergeCount++;
}

static uint32 Merge_Main(uint32 i)
{
	return WaterRelocated() ? _dcMergeList[i].first
	                        : poolIndex(LegoRR::waterGlobs.mergeList[i].mainPool);
}

static uint32 Merge_Removed(uint32 i)
{
	return WaterRelocated() ? _dcMergeList[i].second
	                        : poolIndex(LegoRR::waterGlobs.mergeList[i].removedPool);
}


/// CUSTOM (DeepCore): publish a byte-exact legacy Water_Pool into the (otherwise
/// unused) exe-side slot poolList[0], so that an original 1999 caller which reads the
/// return value of Water_FindPoolDrain at Water_Pool offsets reads something valid and
/// self-consistent instead of a heap pointer.
///
/// The snapshot is truncated to the vanilla capacities, and any drain whose block did
/// not survive truncation is re-pointed at block 0 so a reader can never index past
/// the blocks it can see. Drains keep their index, so a drainIndex handed back
/// alongside the pointer still selects the same drain.
///
/// waterGlobs.poolCount deliberately stays 0, so any exe-side `for (i < poolCount)`
/// loop still finds nothing: the snapshot is reachable only through the returned
/// pointer. Writes the exe makes to it are discarded. This writes only inside
/// poolList[0], i.e. inside the struct's own first 0x428 bytes -- no neighbour of
/// waterGlobs is touched.
static void Water_PublishLegacySnapshot(uint32 pi, LegoRR::Water_Pool* shadow)
{
	std::memset(shadow, 0, sizeof(LegoRR::Water_Pool));

	const uint32 blockCount = std::min(Pool_BlockCount(pi), (uint32)WATER_MAXPOOLBLOCKS);
	for (uint32 i = 0; i < blockCount; i++) {
		shadow->blocks[i] = Pool_Block(pi, i);
	}
	shadow->blockCount = blockCount;

	const uint32 drainCount = std::min(Pool_DrainCount(pi), (uint32)WATER_MAXPOOLDRAINS);
	for (uint32 i = 0; i < drainCount; i++) {
		shadow->drainList[i] = Pool_Drain(pi, i);
		if (shadow->drainList[i].blockIndex >= blockCount)
			shadow->drainList[i].blockIndex = 0;
	}
	shadow->drainCount = drainCount;

	shadow->highWaterLevel = Pool_HighLevel(pi);
	shadow->currWaterLevel = Pool_CurrLevel(pi);
	shadow->contMeshTrans  = Pool_Mesh(pi);
	shadow->flags          = Pool_Flags(pi);
}

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

void LegoRR::Water_AddPoolFloodFill(Lego_Level* level, uint32 bxStart, uint32 byStart)
{
	/// SANITY: Check even though the caller checks.
	if (!blockCanFill(bxStart, byStart))
		return; // Already filled by another pool.

	// Add a new pool.
	/// DEEPCORE: skip the extra pool rather than terminating the process.
	if (Pool_IsFull() && DeepCore::WaterOverflow("water pools"))
		return;
	Error_Fatal(Pool_IsFull(), "Ran out of water pools");

	const uint32 pi = Pool_New();


	// East West algorithm: <http://en.wikipedia.org/wiki/Flood_fill>

	// Start flood fill at our initial block coordinates.
	// DO NOT mark block as filled until the inside of the loop!
	std::queue<Point2U> nodes;
	nodes.push(Point2U { bxStart, byStart });

	while (!nodes.empty()) {
		const Point2U blockPos = nodes.front();

		if (blockCanFill(blockPos.x, blockPos.y)) {
			uint32 bxStart = blockPos.x;
			uint32 bxEnd = bxStart + 1;
			const uint32 by = blockPos.y;

			// Travel as far east as possible.
			while (bxStart > 0 && blockCanFill(bxStart - 1, by)) bxStart--;
			// Travel as far west as possible.
			while (bxEnd < level->width && blockCanFill(bxEnd, by)) bxEnd++;

			// Fill all blocks in the row.
			for (uint32 bx = bxStart; bx < bxEnd; bx++) {
				_filledPoolBlocks[blockIndex(level, bx, by)] = true;
			}
			Water_AddPoolRowBlocksIndex(pi, by, bxStart, bxEnd);

			// Then continue north/south and make sure that only one node is added for every range of blocks in a row.
			bool northContinue = false, southContinue = false;
			for (uint32 bx = bxStart; bx < bxEnd; bx++) {
				if (by > 0) {
					if (blockCanFill(bx, by - 1)) {
						if (!northContinue) { // Start of row at (y - 1).
							northContinue = true;
							nodes.push(Point2U { bx, by - 1 });
						}
					}
					else { // End of row at (y - 1).
						northContinue = false;
					}
				}
				if (by + 1 < level->height) {
					if (blockCanFill(bx, by + 1)) {
						if (!southContinue) { // Start of row at (y + 1).
							southContinue = true;
							nodes.push(Point2U { bx, by + 1 });
						}
					}
					else { // End of row at (y + 1).
						southContinue = false;
					}
				}
			}

		}
		// Finally pop the current block.
		nodes.pop();
	}
}

// <LegoRR.exe @0046de50>
void __cdecl LegoRR::Water_Initialise(Gods98::Container* contRoot, Lego_Level* level)
{
	/// DEEPCORE: also drops the DLL-side pool storage. The exe struct is zeroed either
	/// way, so with the gate off this is exactly the original memset.
	Pool_ResetAll();

	waterGlobs.poolCount = 0;
	waterGlobs.mergeCount  = 0;
	// Assigned, but never used, level->DigDepth is always used instead.
	waterGlobs.digDepth = level->DigDepth;


#if WATER_USENEWFLOODFILL
	// Initialise our list that marks what blocks have been reserved for pools already.
	_filledPoolBlocks.clear();
	_filledPoolBlocks.resize(level->width * level->height, false); // Resize and assign all values to false.

	// Now go through every block in the map and perform a flood fill algorithm to create each pool.
	for (uint32 by = 0; by < level->height; by++) {
		for (uint32 bx = 0; bx < level->width; bx++) {

			if (blockCanFill(bx, by))
				Water_AddPoolFloodFill(level, bx, by);
		}
	}

#else // !WATER_USENEWFLOODFILL

	// This is a type of flood fill algorithm.
	for (uint32 by = 0; by < level->height; by++) {

		uint32 rowEnd = 0; // +1 after the last water block in the row.
		uint32 rowStart = 0;

		for (uint32 bx = 0; bx < level->width; bx++) {
			if (blockValue(level, bx, by).terrain == Lego_SurfaceType_Water) {
				// Check if this is the start of a continuous row.
				if (rowStart == rowEnd) {
					// Start a new row.
					rowStart = bx;
				}
				// Expand the row.
				rowEnd = bx + 1;
			}
			else {
				// Has a continuous row been started?
				if (rowStart != rowEnd) {
					const uint32 pi = Water_FindPoolAndMergeRowsIndex(by, rowStart, rowEnd);
					if (pi == WATER_POOL_INVALID) {
						Water_AddPool(by, rowStart, rowEnd);
					}
					else {
						Water_AddPoolRowBlocksIndex(pi, by, rowStart, rowEnd);
					}
				}
				// End of continuous row.
				rowEnd = 0;
				rowStart = 0;
			}
		}
	}

	// Merge pools that were found to be connected next to each other on the y axis.
	for (uint32 i = 0; i < Merge_Count(); i++) {
		const uint32 mainPi = Merge_Main(i);
		const uint32 remPi  = Merge_Removed(i);

		if (Pool_BlockCount(remPi) != 0) {
			/// DEEPCORE: leave the pools unmerged rather than terminating. They stay
			/// two separate bodies of water, which is cosmetically wrong but playable.
			/// Relocated, this can never trigger: there is no fixed block table.
			if (Pool_MergeOverflows(mainPi, remPi) &&
				DeepCore::WaterOverflow("blocks in a merged water pool"))
				continue;
			Error_Fatal(Pool_MergeOverflows(mainPi, remPi),
						"Can't merge water pools because there isn't enough blocks");

			Pool_AppendBlocks(mainPi, remPi);
			Pool_ClearBlocks(remPi); // Mark the second pool for removal.
		}
	}

	// Sort by blockCount, highest to lowest.
	Pool_SortByBlockCount();

	// Remove water pools that were merged into another main pool.
	for (uint32 i = 0; i < Pool_Count(); i++) {
		if (Pool_BlockCount(i) == 0) {
			Pool_Truncate(i);
			break;
		}
	}
#endif

	Water_InitPoolDrains(contRoot, level);
	Water_InitVertices(contRoot, level);
}

/// CUSTOM: Force water to cleanup at the end of the level.
void LegoRR::Water_RemoveAll()
{
	for (uint32 i = 0; i < Pool_Count(); i++) {
		Gods98::Container_Remove(Pool_Mesh(i));
		Pool_Mesh(i) = nullptr;
	}

	/// DEEPCORE: the DLL-side pools are dropped only AFTER their containers have been
	/// removed, mirroring the original teardown order exactly.
	Pool_ResetAll();

#if WATER_USENEWFLOODFILL
	_filledPoolBlocks.clear();
#endif
}

// <LegoRR.exe @0046dfd0>
void __cdecl LegoRR::Water_InitPoolDrains(Gods98::Container* contRoot, Lego_Level* level)
{
	const Point2F DIRECTIONS[4] = {
		{  0.0f, -1.0f },
		{  1.0f,  0.0f },
		{  0.0f,  1.0f },
		{ -1.0f,  0.0f },
	};

	for (uint32 i = 0; i < Pool_Count(); i++) {

		Pool_ClearDrains(i);

		for (uint32 j = 0; j < Pool_BlockCount(i); j++) {

			for (uint32 dir = 0; dir < 4; dir++) {
				/// FIXME: REAL TO INT COORDINATES
				const sint32 bx = static_cast<sint32>(Pool_Block(i, j).x + DIRECTIONS[dir].x);
				const sint32 by = static_cast<sint32>(Pool_Block(i, j).y + DIRECTIONS[dir].y);

				if (blockInBounds(level, bx, by) &&
					(blockValue(level, bx, by).terrain != Lego_SurfaceType_Water) &&
					(blockValue(level, bx, by).terrain != Lego_SurfaceType_Immovable))
				{
					/// DEEPCORE: stop adding drains for this pool rather than terminating.
					if (Pool_DrainsFull(i) && DeepCore::WaterOverflow("water pool drains"))
						break;
					Error_FatalF(Pool_DrainsFull(i), "Ran out of water pool drains for pool at index %i", i);

					Error_DebugF("Adding drain (%i,%i) %s\n", bx, by, legoGlobs.surfaceName[blockValue(level, bx, by).terrain]);

					/// TODO: Should we really be breaking when other surrounding blocks may be candidates?
					Pool_AddDrain(i, j, (Direction)dir);
					break;
				}
			}
		}
	}
}

// <LegoRR.exe @0046e140>
void __cdecl LegoRR::Water_InitVertices(Gods98::Container* contRoot, Lego_Level* level)
{
	const Point2F DIRECTIONS[4] = {
		{ 0.0f, 0.0f },
		{ 1.0f, 0.0f },
		{ 1.0f, 1.0f },
		{ 0.0f, 1.0f },
	};

	const uint32 FACEDATA[6] = {
		0, 1, 3, 1, 2, 3,
	};

	// Same as DIRECTIONS.
	const Point2F TUV_COORDS[4] = {
		{ 0.0f, 0.0f },
		{ 1.0f, 0.0f },
		{ 1.0f, 1.0f },
		{ 0.0f, 1.0f },
	};

	for (uint32 i = 0; i < Pool_Count(); i++) {

		Pool_HighLevel(i) = -10000.0f;

		for (uint32 j = 0; j < Pool_BlockCount(i); j++) {
			Vector3F vertPoses[4];

			/// FIXME: REAL TO INT COORDINATES
			const uint32 bx = static_cast<uint32>(Pool_Block(i, j).x);
			const uint32 by = static_cast<uint32>(Pool_Block(i, j).y);
			Map3D_GetBlockVertexPositions(level->map, bx, by, vertPoses);

			for (uint32 dir = 0; dir < 4; dir++) {
				/// FIXME: REAL TO INT COORDINATES
				const uint32 bxOff = static_cast<uint32>(Pool_Block(i, j).x + DIRECTIONS[dir].x);
				const uint32 byOff = static_cast<uint32>(Pool_Block(i, j).y + DIRECTIONS[dir].y);

				/// CHANGE: Properly check for exposed walls (predug won't tell us that info).
				const Point2I blockOff = Point2I { static_cast<sint32>(bxOff), static_cast<sint32>(byOff) };
				const bool exposed = Level_Block_IsInitiallyExposed(&blockOff);

				if ((exposed  && Level_Block_IsWall(bxOff, byOff)) ||
					(!exposed && blockValue(level, bxOff, byOff).predug == Lego_PredugType_Wall))
				{
					Pool_HighLevel(i) = std::max(Pool_HighLevel(i), vertPoses[dir].z);
					//if (Pool_HighLevel(i) < vertPoses[k].z)
					//	Pool_HighLevel(i) = vertPoses[k].z;
				}
				else {
					Pool_Flags(i) |= WATER_FLAG_VISIBLE;
				}
			}
		}

		Pool_CurrLevel(i) = Pool_HighLevel(i); // Assign highest z to z used during lowering maybe?

		Pool_Mesh(i) = Gods98::Container_MakeMesh2(contRoot, Gods98::Container_MeshType::Transparent);
		Gods98::Container_Hide(Pool_Mesh(i), !(Pool_Flags(i) & WATER_FLAG_VISIBLE));

		for (uint32 j = 0; j < Pool_BlockCount(i); j++) {
			const uint32 groupID = Gods98::Container_Mesh_AddGroup(Pool_Mesh(i), 4, 2, 3, FACEDATA);

			const ColourRGBAF rgba = WATER_COLOURRGBA;
			Gods98::Container_Mesh_SetColourAlpha(Pool_Mesh(i), groupID, rgba.r, rgba.g, rgba.b, rgba.a);

			Vector3F vertPoses[4];
			Vertex vertices[4] = { 0.0f }; // dummy init

			/// FIXME: REAL TO INT COORDINATES
			const uint32 bx = static_cast<uint32>(Pool_Block(i, j).x);
			const uint32 by = static_cast<uint32>(Pool_Block(i, j).y);
			Map3D_GetBlockVertexPositions(level->map, bx, by, vertPoses);

			for (uint32 k = 0; k < 4; k++) {
				vertices[k].position.x = vertPoses[k].x;
				vertices[k].position.y = vertPoses[k].y;
				vertices[k].position.z = Pool_HighLevel(i);
				vertices[k].normal.x =  0.0f;
				vertices[k].normal.y =  0.0f;
				vertices[k].normal.z = -1.0f;
				vertices[k].tuv = TUV_COORDS[k];
				vertices[k].colour = 0; // Black
			}

			Gods98::Container_Mesh_SetVertices(Pool_Mesh(i), j, 0, 4, vertices);
		}
	}
}

// <LegoRR.exe @0046e480>
void __cdecl LegoRR::Water_ChangeViewMode_removed(bool32 isFPViewMode)
{
	for (uint32 i = 0; i < Pool_Count(); i++) {
		Gods98::Container* cont = Pool_Mesh(i);
		real32 arg3, arg4;
		if (!isFPViewMode) {
			arg3 = 0.0f;
			arg4 = 0.0f;
		}
		else {
			arg3 = 0.12f;
			arg4 = 0.32f;
		}

		// Whatever this function originally was, it doesn't truly exist in LegoRR.
		// Beta builds will need to be checked.

		//logf_removed(cont, 0, arg3, arg4);
	}
}

// <LegoRR.exe @0046e4e0>
void __cdecl LegoRR::Water_DestroyWallComplete(Lego_Level* level, uint32 bx, uint32 by)
{
	// I don't think this is actually a Point2I type, just a pair of ints.
	/*const Point2I VERT_OFFSETS[4] = {
		{ 0, 1 },
		{ 1, 2 },
		{ 2, 3 },
		{ 3, 0 },
	};*/
	const uint32 VERT_OFFSETS[4][2] = {
		{ 0, 1 },
		{ 1, 2 },
		{ 2, 3 },
		{ 3, 0 },
	};

	Vector3F vertPoses[4];

	uint32 drainIndex;
	uint32 pi;
	if (Water_FindPoolDrainIndex(bx, by, &pi, &drainIndex)) {
		Pool_Flags(pi) &= ~WATER_FLAG_SETTLED; // Not filling?
		Pool_Flags(pi) |= WATER_FLAG_VISIBLE;

		Gods98::Container_Hide(Pool_Mesh(pi), false);

		Water_PoolDrain& drain = Pool_Drain(pi, drainIndex);
		drain.active = true;
		drain.elapsedUp_c = 0.0f;
		drain.elapsedDown_10 = 0.0f;

		/// FIXME: REAL TO INT COORDINATES
		const uint32 bx = static_cast<uint32>(Pool_Block(pi, drain.blockIndex).x);
		const uint32 by = static_cast<uint32>(Pool_Block(pi, drain.blockIndex).y);
		Map3D_GetBlockVertexPositions(level->map, bx, by, vertPoses);

		const Direction dir = drain.direction;
		drain.drainWaterLevel = std::max(vertPoses[VERT_OFFSETS[dir][0]].z,
										 vertPoses[VERT_OFFSETS[dir][1]].z);
	}
}

// <LegoRR.exe @0046e5f0>
void __cdecl LegoRR::Water_DamBlock(uint32 bx, uint32 by)
{
	uint32 drainIndex;
	uint32 pi;

	if (Water_FindPoolDrainIndex(bx, by, &pi, &drainIndex)) {
		Pool_Flags(pi) &= ~WATER_FLAG_FULL; // Not draining?
		Pool_Flags(pi) |= WATER_FLAG_VISIBLE;

		Gods98::Container_Hide(Pool_Mesh(pi), false);
		Pool_Drain(pi, drainIndex).active = false;
	}
}

// <LegoRR.exe @0046e650>
void __cdecl LegoRR::Water_Update(Lego_Level* level, real32 elapsedGame)
{
	uint32 numActiveDrains = 0;

	static uint32 flagsCounter = 0;
	static WaterFlags flagsLast = WATER_FLAG_NONE;// waterGlobs.poolList[0].flags;

	for (uint32 i = 0; i < Pool_Count(); i++) {

		if (i == 0) {
			// What flags have changed since last update?
			bool V = (Pool_Flags(i) & WATER_FLAG_VISIBLE);
			bool A = (Pool_Flags(i) & WATER_FLAG_FULL);
			bool B = (Pool_Flags(i) & WATER_FLAG_SETTLED);
			bool v = ((flagsLast ^ Pool_Flags(i)) & WATER_FLAG_VISIBLE);
			bool a = ((flagsLast ^ Pool_Flags(i)) & WATER_FLAG_FULL);
			bool b = ((flagsLast ^ Pool_Flags(i)) & WATER_FLAG_SETTLED);
			if (v || a || b) {
				Error_InfoF("[%i] Visible=%s%s  A=%s%s  B=%s%s",
							flagsCounter++,
							(V ? "On " : "Off"), (v ? "*" : " "),
							(A ? "On " : "Off"), (a ? "*" : " "),
							(B ? "On " : "Off"), (b ? "*" : " "));
			}
			flagsLast = Pool_Flags(i);
		}

		bool lowering = false;
		bool rising   = !(Pool_Flags(i) & WATER_FLAG_FULL); // Rise if we're not already full.
		real32 lowestWaterLevel = Pool_HighLevel(i);

		for (uint32 j = 0; j < Pool_DrainCount(i); j++) {
			Water_PoolDrain& drain = Pool_Drain(i, j);

			// Is this actively draining water?
			if (drain.active && Pool_CurrLevel(i) <= drain.drainWaterLevel) {

				lowestWaterLevel = std::max(lowestWaterLevel, drain.drainWaterLevel);
				//if (lowestWaterLevel < drain.drainWaterLevel)
				//	lowestWaterLevel = drain.drainWaterLevel;

				numActiveDrains++;
				lowering = !(Pool_Flags(i) & WATER_FLAG_SETTLED); // Lower if we're not already settled.
				rising   = false;
			}

			// These values are used to assign NOTHOT flags to blocks some distance away. They make no sense...
			drain.elapsedUp_c += elapsedGame * 1.5f;
			if (!drain.active || Pool_CurrLevel(i) >= drain.drainWaterLevel)
				drain.elapsedDown_10 += elapsedGame * 0.5f; // Draining faster(?)

			// Cap spill out distance(?)
			drain.elapsedUp_c    = std::min(drain.elapsedUp_c,    240.0f); //  6.4 seconds to reach.
			drain.elapsedDown_10 = std::min(drain.elapsedDown_10, 240.0f); // 19.2 seconds to reach.
			//if (drain.elapsedUp_c > 240.0f)
			//	drain.elapsedUp_c = 240.0f;

			//if (drain.elapsedDown_10 > 240.0f)
			//	drain.elapsedDown_10 = 240.0f;
		}


		if (lowering || rising) {
			const real32 waterLevelDiff = (Pool_CurrLevel(i) - Pool_HighLevel(i)) / level->DigDepth;

			if (lowering) { // Water is lowering (draining).
				Pool_Flags(i) &= ~WATER_FLAG_FULL;
				Pool_CurrLevel(i) += (static_cast<real32>(numActiveDrains) * elapsedGame * 0.3f) / (waterLevelDiff + 1.0f);
				if (Pool_CurrLevel(i) >= lowestWaterLevel) {
					Pool_CurrLevel(i) = lowestWaterLevel;
					Pool_Flags(i) |= WATER_FLAG_SETTLED;
				}
			}
			else { // Water is rising (filling).
				Pool_Flags(i) &= ~WATER_FLAG_SETTLED;
				Pool_CurrLevel(i) -= elapsedGame * 0.05f;
				if (Pool_CurrLevel(i) <= lowestWaterLevel) {
					Pool_CurrLevel(i) = lowestWaterLevel;
					Pool_Flags(i) |= WATER_FLAG_FULL;
				}
			}

			const ColourRGBAF rgba = WATER_COLOURRGBA;
			const real32 alpha = std::max(0.0f, rgba.a - waterLevelDiff * 0.4f);
			Gods98::Container_SetColourAlpha(Pool_Mesh(i), rgba.r, rgba.g, rgba.b, alpha);

			for (uint32 j = 0; j < Pool_BlockCount(i); j++) {

				Vertex vertices[4];
				Gods98::Container_Mesh_GetVertices(Pool_Mesh(i), j, 0, 4, vertices);

				// Copy water level z to vertex positions.
				for (uint32 k = 0; k < 4; k++) {
					vertices[k].position.z = Pool_CurrLevel(i);
				}
				Gods98::Container_Mesh_SetVertices(Pool_Mesh(i), j, 0, 4, vertices);
			}
		}

	}
	Water_UpdateNotHotBlocks(level);
}

// <LegoRR.exe @0046e8d0>
void __cdecl LegoRR::Water_UpdateNotHotBlocks(Lego_Level* level)
{
	const Vector3F DIRECTIONS[4] = {
		{  0.0f,  1.0f,  0.0f },
		{  1.0f,  0.0f,  0.0f },
		{  0.0f, -1.0f,  0.0f },
		{ -1.0f,  0.0f,  0.0f },
	};

	const uint32 VERT_OFFSETS[4][2] = {
		{ 0, 1 },
		{ 1, 2 },
		{ 2, 3 },
		{ 3, 0 },
	};

	for (uint32 i = 0; i < Pool_Count(); i++) {
		for (uint32 j = 0; j < Pool_DrainCount(i); j++) {
			const Water_PoolDrain& drain = Pool_Drain(i, j);

			/// CHANGE: All of the following was done twice for both the elapseUp and elapseDown states, we can safely avoid code duplication.
			/// FIXME: REAL TO INT COORDINATES
			const uint32 bx = static_cast<uint32>(Pool_Block(i, drain.blockIndex).x);
			const uint32 by = static_cast<uint32>(Pool_Block(i, drain.blockIndex).y);
			const Direction dir = drain.direction;

			Vector3F vertPoses[4];
			Map3D_GetBlockVertexPositions(level->map, bx, by, vertPoses);

			const Vector3F basePos = {
				(vertPoses[VERT_OFFSETS[dir][0]].x + vertPoses[VERT_OFFSETS[dir][1]].x) / 2.0f,
				(vertPoses[VERT_OFFSETS[dir][0]].y + vertPoses[VERT_OFFSETS[dir][1]].y) / 2.0f,
				0.0f,
			};

			// Maybe this is cooling off erosion???
			// I don't really understand the point of this. But note that SetNotHot updates the block surface.

			/// SANITY: Check Map3D_WorldToBlockPos_NoZ success, because Level_Block_SetNotHot is not bounds-safe.

			Vector3F wPos;
			sint32 bxW = 0, byW = 0; // dummy inits

			// Set NOTHOT for elapsedUp blocks.
			Gods98::Maths_RayEndPoint(&wPos, &basePos, &DIRECTIONS[dir], drain.elapsedUp_c);
			if (Map3D_WorldToBlockPos_NoZ(level->map, wPos.x, wPos.y, &bxW, &byW))
				Level_Block_SetNotHot(level, bxW, byW, true);

			// And set 'HOT' for elapsedDown blocks (HOT only applies to lava/eroding blocks).
			Gods98::Maths_RayEndPoint(&wPos, &basePos, &DIRECTIONS[dir], drain.elapsedDown_10);
			if (Map3D_WorldToBlockPos_NoZ(level->map, wPos.x, wPos.y, &bxW, &byW))
				Level_Block_SetNotHot(level, bxW, byW, false);
		}
	}
}

/// CUSTOM: Index form of Water_FindPoolDrain, and the ONLY form this module uses.
/// Returns false where the pointer form returns nullptr.
bool LegoRR::Water_FindPoolDrainIndex(uint32 bx, uint32 by, OUT uint32* outPoolIndex, OPTIONAL OUT uint32* drainIndex)
{
	const Point2F DIRECTIONS[4] = {
		{  0.0f, -1.0f },
		{  1.0f,  0.0f },
		{  0.0f,  1.0f },
		{ -1.0f,  0.0f },
	};

	for (uint32 i = 0; i < Pool_Count(); i++) {
		for (uint32 j = 0; j < Pool_DrainCount(i); j++) {
			const Water_PoolDrain& drain = Pool_Drain(i, j);
			const Point2F& block = Pool_Block(i, drain.blockIndex);

			/// FIXME: REAL TO INT COORDINATES
			if ((bx == static_cast<uint32>(block.x + DIRECTIONS[drain.direction].x)) &&
				(by == static_cast<uint32>(block.y + DIRECTIONS[drain.direction].y)))
			{
				if (outPoolIndex) *outPoolIndex = i;
				if (drainIndex) *drainIndex = j;

				return true;
			}
		}
	}
	return false;
}

// <LegoRR.exe @0046eb60>
/// ABI-PRESERVING WRAPPER. MAY BE ENTERED FROM ORIGINAL EXE CODE
/// (Level_Debug_WKey_NeedsBlockFlags1_8_FUN_004303a0 @0x004303a0, see interop.cpp),
/// which will dereference the result at Water_Pool offsets. Nothing inside this module
/// calls it any more -- they all use the index form above.
LegoRR::Water_Pool* __cdecl LegoRR::Water_FindPoolDrain(uint32 bx, uint32 by, OPTIONAL OUT uint32* drainIndex)
{
	uint32 pi = WATER_POOL_INVALID;
	if (!Water_FindPoolDrainIndex(bx, by, &pi, drainIndex))
		return nullptr;

	if (!WaterRelocated())
		return &waterGlobs.poolList[pi]; // exact vanilla

	/// DEEPCORE: with the tables relocated there is no exe-resident Water_Pool to point
	/// at, so publish a legacy snapshot into the now-unused exe slot 0 and return that.
	/// The one original caller of this function is additionally declined outright at its
	/// only call site (the [W] debug keybind in Game.cpp); this is defence in depth for
	/// any exe path that was never discovered.
	Water_Pool& shadow = waterGlobs.poolList[0];
	Water_PublishLegacySnapshot(pi, &shadow);
	return &shadow;
}

// DATA: const Water_Pool*
// <LegoRR.exe @0046ec60>
/// Still hooked over the exe, and still used by the non-relocated sort. The relocated
/// sort cannot use it, because qsort swaps raw bytes (see Pool_SortByBlockCount).
sint32 __cdecl LegoRR::Water_QsortComparePools(const void* a, const void* b)
{
	const Water_Pool* wa = (const Water_Pool*)a;
	const Water_Pool* wb = (const Water_Pool*)b;

	if (wa->blockCount > wb->blockCount)
		return -1;
	if (wa->blockCount < wb->blockCount)
		return 1;
	return 0;
}

/// CUSTOM: Index form of Water_FindPoolAndMergeRows. Returns WATER_POOL_INVALID where
/// the pointer form returns nullptr.
uint32 LegoRR::Water_FindPoolAndMergeRowsIndex(uint32 by, uint32 bxRowStart, uint32 bxRowEnd)
{
	if (by == 0) // Can't check for pools in the previous column, this is the first column.
		return WATER_POOL_INVALID;

	uint32 mainPool = WATER_POOL_INVALID;
	for (uint32 bx = bxRowStart; bx < bxRowEnd; bx++) {

		for (uint32 i = 0; i < Pool_Count(); i++) {

			for (uint32 j = 0; j < Pool_BlockCount(i); j++) {

				// Check for pools connected in the previous column (y - 1).
				/// FIXME: REAL TO INT COORDINATES
				if (static_cast<real32>(bx)     == Pool_Block(i, j).x &&
					static_cast<real32>(by - 1) == Pool_Block(i, j).y)
				{
					if (mainPool == WATER_POOL_INVALID) {
						// This is our first-found pool within the block row.
						mainPool = i;
					}
					else if (mainPool != i) {
						/// DEEPCORE: skip registering this merge rather than terminating.
						if (Merge_IsFull() && DeepCore::WaterOverflow("water merge pairs"))
							continue;
						Error_Fatal(Merge_IsFull(), "Ran out of water merge pairs");

						// Register this pool to be merged with mainPool at the end of Initialise.
						Merge_Add(mainPool, i);
					}
				}
			}
		}
	}
	return mainPool;
}

// <LegoRR.exe @0046ec90>
/// ABI-PRESERVING WRAPPER, kept only so that an unknown exe CALL to this address still
/// gets a Water_Pool*. Water_Initialise uses the index form above.
LegoRR::Water_Pool* __cdecl LegoRR::Water_FindPoolAndMergeRows(uint32 by, uint32 bxRowStart, uint32 bxRowEnd)
{
	const uint32 pi = Water_FindPoolAndMergeRowsIndex(by, bxRowStart, bxRowEnd);
	if (pi == WATER_POOL_INVALID)
		return nullptr;

	if (!WaterRelocated())
		return &waterGlobs.poolList[pi]; // exact vanilla

	Water_Pool& shadow = waterGlobs.poolList[0];
	Water_PublishLegacySnapshot(pi, &shadow);
	return &shadow;
}

/// CUSTOM: Index form of Water_AddPoolRowBlocks, and the ONLY form this module uses.
void LegoRR::Water_AddPoolRowBlocksIndex(uint32 pi, uint32 by, uint32 bxRowStart, uint32 bxRowEnd)
{
	for (uint32 bx = bxRowStart; bx < bxRowEnd; bx++) {
		/// DEEPCORE: stop growing this pool rather than terminating. The remaining
		/// tiles simply are not part of the simulated body of water.
		if (Pool_BlocksFull(pi) && DeepCore::WaterOverflow("blocks in one water pool"))
			break;
		Error_FatalF(Pool_BlocksFull(pi), "Ran out of water pool blocks for pool at index %i", pi);

		/// FIXME: REAL TO INT COORDINATES
		Pool_AddBlock(pi, static_cast<real32>(bx), static_cast<real32>(by));
	}
}

// <LegoRR.exe @0046ed90>
/// ABI-PRESERVING WRAPPER. Every known caller is C++ and calls the index form directly;
/// this exists only so an unknown exe CALL to 0x0046ed90 keeps vanilla semantics.
void __cdecl LegoRR::Water_AddPoolRowBlocks(Water_Pool* pool, uint32 by, uint32 bxRowStart, uint32 bxRowEnd)
{
	// poolIndex() is pointer arithmetic against the exe-side array, and it is correct
	// HERE AND ONLY HERE: a Water_Pool* arriving through this ABI can only be
	// exe-resident, because that is the only kind of pool a 1999 caller can hold.
	const uint32 pi = poolIndex(pool);

	/// DEEPCORE: relocated, the only legal exe-side pointer is the legacy shadow slot,
	/// which maps to pool 0. Anything else has no relocated pool to grow, so decline it
	/// rather than indexing _dcPools out of range.
	if (WaterRelocated() && pi >= Pool_Count())
		return;

	Water_AddPoolRowBlocksIndex(pi, by, bxRowStart, bxRowEnd);
}

// <LegoRR.exe @0046edf0>
void __cdecl LegoRR::Water_AddPool(uint32 by, uint32 bxRowStart, uint32 bxRowEnd)
{
	/// DEEPCORE: skip the extra pool rather than terminating the process.
	if (Pool_IsFull() && DeepCore::WaterOverflow("water pool entries"))
		return;
	Error_Fatal(Pool_IsFull(), "Ran out of pool entries");

	const uint32 pi = Pool_New();
	Water_AddPoolRowBlocksIndex(pi, by, bxRowStart, bxRowEnd);
}


/// CUSTOM: Read-only pool accessors for code outside this module, so that nothing else
/// has to know whether the tables are relocated. See Water.h.
uint32 LegoRR::Water_GetPoolCount(void)
{
	return Pool_Count();
}

bool LegoRR::Water_GetPoolView(uint32 pi, OUT Water_PoolView* view)
{
	if (view == nullptr || pi >= Pool_Count())
		return false;

	view->blockCount     = Pool_BlockCount(pi);
	view->drainCount     = Pool_DrainCount(pi);
	view->highWaterLevel = Pool_HighLevel(pi);
	view->currWaterLevel = Pool_CurrLevel(pi);
	view->flags          = Pool_Flags(pi);
	return true;
}

bool LegoRR::Water_GetPoolBlock(uint32 pi, uint32 bi, OUT Point2F* block)
{
	if (block == nullptr || pi >= Pool_Count() || bi >= Pool_BlockCount(pi))
		return false;

	*block = Pool_Block(pi, bi);
	return true;
}

bool LegoRR::Water_GetPoolDrain(uint32 pi, uint32 di, OUT Water_PoolDrain* drain)
{
	if (drain == nullptr || pi >= Pool_Count() || di >= Pool_DrainCount(pi))
		return false;

	*drain = Pool_Drain(pi, di);
	return true;
}

#pragma endregion
