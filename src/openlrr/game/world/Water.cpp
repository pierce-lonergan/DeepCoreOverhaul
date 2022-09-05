// Water.cpp : 
//

#include "../../engine/core/Maths.h"

#include "../Game.h"

#include "Water.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @0054a520>
LegoRR::Water_Globs & LegoRR::waterGlobs = *(LegoRR::Water_Globs*)0x0054a520;

/// CUSTOM: Used for East West flood fill algorithm.
static std::vector<bool> _filledPoolBlocks;

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define WATER_USENEWFLOODFILL	true

#define poolIndex(pool)			((uint32)((pool) - waterGlobs.poolList) / sizeof(Water_Pool))

#define blockCanFill(bx, by)	(blockValue(level,(bx),(by)).terrain==Lego_SurfaceType_Water && \
								 !_filledPoolBlocks[blockIndex(level,(bx),(by))])

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
	Error_Fatal(waterGlobs.poolCount >= WATER_MAXPOOLS, "Ran out of water pools");

	Water_Pool* pool = &waterGlobs.poolList[waterGlobs.poolCount++];
	std::memset(pool, 0, sizeof(Water_Pool));


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
			Water_AddPoolRowBlocks(pool, by, bxStart, bxEnd);

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
	std::memset(&waterGlobs, 0, sizeof(waterGlobs));

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
					Water_Pool* pool = Water_FindPoolAndMergeRows(by, rowStart, rowEnd);
					if (pool == nullptr) {
						Water_AddPool(by, rowStart, rowEnd);
					}
					else {
						Water_AddPoolRowBlocks(pool, by, rowStart, rowEnd);
					}
				}
				// End of continuous row.
				rowEnd = 0;
				rowStart = 0;
			}
		}
	}

	// Merge pools that were found to be connected next to each other on the y axis.
	for (uint32 i = 0; i < waterGlobs.mergeCount; i++) {
		Water_Pool* main  = waterGlobs.mergeList[i].mainPool;
		Water_Pool* removed = waterGlobs.mergeList[i].removedPool;

		if (removed->blockCount != 0) {
			Error_Fatal(main->blockCount + removed->blockCount > WATER_MAXPOOLBLOCKS,
						"Can't merge water pools because there isn't enough blocks");
			// Some form of overlapped copying...?
			std::memcpy(&main->blocks[main->blockCount], &removed->blocks[0], sizeof(Point2F) * removed->blockCount);

			main->blockCount += removed->blockCount;
			removed->blockCount = 0; // Mark the second pool for removal.
		}
	}
	
	// Sort by blockCount, highest to lowest.
	std::qsort(&waterGlobs.poolList, waterGlobs.poolCount, sizeof(Water_Pool), Water_QsortComparePools);

	// Remove water pools that were merged into another main pool.
	for (uint32 i = 0; i < waterGlobs.poolCount; i++) {
		if (waterGlobs.poolList[i].blockCount == 0) {
			waterGlobs.poolCount = i;
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
	for (uint32 i = 0; i < waterGlobs.poolCount; i++) {
		Water_Pool* pool = &waterGlobs.poolList[i];

		Gods98::Container_Remove(pool->contMeshTrans);
		pool->contMeshTrans = nullptr;
	}

	std::memset(&waterGlobs, 0, sizeof(waterGlobs));

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

	for (uint32 i = 0; i < waterGlobs.poolCount; i++) {
		Water_Pool* pool = &waterGlobs.poolList[i];

		pool->drainCount = 0;

		for (uint32 j = 0; j < pool->blockCount; j++) {

			for (uint32 dir = 0; dir < 4; dir++) {
				/// FIXME: REAL TO INT COORDINATES
				const sint32 bx = static_cast<sint32>(pool->blocks[j].x + DIRECTIONS[dir].x);
				const sint32 by = static_cast<sint32>(pool->blocks[j].y + DIRECTIONS[dir].y);
				
				if (blockInBounds(level, bx, by) &&
					(blockValue(level, bx, by).terrain != Lego_SurfaceType_Water) &&
					(blockValue(level, bx, by).terrain != Lego_SurfaceType_Immovable))
				{
					Error_FatalF(pool->drainCount >= WATER_MAXPOOLDRAINS, "Ran out of water pool drains for pool at index %i", i);
					
					Error_DebugF("Adding drain (%i,%i) %s\n", bx, by, legoGlobs.surfaceName[blockValue(level, bx, by).terrain]);
					
					/// TODO: Should we really be breaking when other surrounding blocks may be candidates?
					pool->drainList[pool->drainCount].blockIndex = j; // Pool blocks index
					pool->drainList[pool->drainCount].direction = (Direction)dir; // direction index
					pool->drainCount++;
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

	for (uint32 i = 0; i < waterGlobs.poolCount; i++) {
		Water_Pool* pool = &waterGlobs.poolList[i];

		pool->highWaterLevel = -10000.0f;

		for (uint32 j = 0; j < pool->blockCount; j++) {
			Vector3F vertPoses[4];

			/// FIXME: REAL TO INT COORDINATES
			const uint32 bx = static_cast<uint32>(pool->blocks[j].x);
			const uint32 by = static_cast<uint32>(pool->blocks[j].y);
			Map3D_GetBlockVertexPositions(level->map, bx, by, vertPoses);

			for (uint32 dir = 0; dir < 4; dir++) {
				/// FIXME: REAL TO INT COORDINATES
				const uint32 bxOff = static_cast<uint32>(pool->blocks[j].x + DIRECTIONS[dir].x);
				const uint32 byOff = static_cast<uint32>(pool->blocks[j].y + DIRECTIONS[dir].y);

				/// CHANGE: Properly check for exposed walls (predug won't tell us that info).
				const Point2I blockOff = Point2I { static_cast<sint32>(bxOff), static_cast<sint32>(byOff) };
				const bool exposed = Level_Block_IsExposed(&blockOff);

				if ((exposed  && Level_Block_IsWall(bxOff, byOff)) ||
					(!exposed && blockValue(level, bxOff, byOff).predug == Lego_PredugType_Wall))
				{
					pool->highWaterLevel = std::max(pool->highWaterLevel, vertPoses[dir].z);
					//if (pool->highWaterLevel < vertPoses[k].z)
					//	pool->highWaterLevel = vertPoses[k].z;
				}
				else {
					pool->flags |= WATER_FLAG_VISIBLE;
				}
			}
		}

		pool->currWaterLevel = pool->highWaterLevel; // Assign highest z to z used during lowering maybe?

		pool->contMeshTrans = Gods98::Container_MakeMesh2(contRoot, Gods98::Container_MeshType::Transparent);
		Gods98::Container_Hide(pool->contMeshTrans, !(pool->flags & WATER_FLAG_VISIBLE));

		for (uint32 j = 0; j < pool->blockCount; j++) {
			const uint32 groupID = Gods98::Container_Mesh_AddGroup(pool->contMeshTrans, 4, 2, 3, FACEDATA);

			const ColourRGBAF rgba = WATER_COLOURRGBA;
			Gods98::Container_Mesh_SetColourAlpha(pool->contMeshTrans, groupID, rgba.red, rgba.green, rgba.blue, rgba.alpha);

			Vector3F vertPoses[4];
			Vertex vertices[4] = { 0.0f }; // dummy init

			/// FIXME: REAL TO INT COORDINATES
			const uint32 bx = static_cast<uint32>(pool->blocks[j].x);
			const uint32 by = static_cast<uint32>(pool->blocks[j].y);
			Map3D_GetBlockVertexPositions(level->map, bx, by, vertPoses);

			for (uint32 k = 0; k < 4; k++) {
				vertices[k].position.x = vertPoses[k].x;
				vertices[k].position.y = vertPoses[k].y;
				vertices[k].position.z = pool->highWaterLevel;
				vertices[k].normal.x =  0.0f;
				vertices[k].normal.y =  0.0f;
				vertices[k].normal.z = -1.0f;
				vertices[k].tuv = TUV_COORDS[k];
				vertices[k].colour = 0; // Black
			}

			Gods98::Container_Mesh_SetVertices(pool->contMeshTrans, j, 0, 4, vertices);
		}
	}
}

// <LegoRR.exe @0046e480>
void __cdecl LegoRR::Water_ChangeViewMode_removed(bool32 isFPViewMode)
{
	for (uint32 i = 0; i < waterGlobs.poolCount; i++) {
		Gods98::Container* cont = waterGlobs.poolList[i].contMeshTrans;
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
	Water_Pool* pool = Water_FindPoolDrain(bx, by, &drainIndex);
	if (pool != nullptr) {
		pool->flags &= ~WATER_FLAG_SETTLED; // Not filling?
		pool->flags |= WATER_FLAG_VISIBLE;

		Gods98::Container_Hide(pool->contMeshTrans, false);

		Water_PoolDrain* drain = &pool->drainList[drainIndex];
		drain->active = true;
		drain->elapsedUp_c = 0.0f;
		drain->elapsedDown_10 = 0.0f;

		/// FIXME: REAL TO INT COORDINATES
		const uint32 bx = static_cast<uint32>(pool->blocks[drain->blockIndex].x);
		const uint32 by = static_cast<uint32>(pool->blocks[drain->blockIndex].y);
		Map3D_GetBlockVertexPositions(level->map, bx, by, vertPoses);

		const Direction dir = drain->direction;
		drain->drainWaterLevel = std::max(vertPoses[VERT_OFFSETS[dir][0]].z,
										  vertPoses[VERT_OFFSETS[dir][1]].z);
	}
}

// <LegoRR.exe @0046e5f0>
void __cdecl LegoRR::Water_DamBlock(uint32 bx, uint32 by)
{
	uint32 drainIndex;
	Water_Pool* pool = Water_FindPoolDrain(bx, by, &drainIndex);

	if (pool != nullptr) {
		pool->flags &= ~WATER_FLAG_FULL; // Not draining?
		pool->flags |= WATER_FLAG_VISIBLE;

		Gods98::Container_Hide(pool->contMeshTrans, false);
		pool->drainList[drainIndex].active = false;
	}
}

// <LegoRR.exe @0046e650>
void __cdecl LegoRR::Water_Update(Lego_Level* level, real32 elapsedGame)
{
	uint32 numActiveDrains = 0;

	static uint32 flagsCounter = 0;
	static WaterFlags flagsLast = WATER_FLAG_NONE;// waterGlobs.poolList[0].flags;

	for (uint32 i = 0; i < waterGlobs.poolCount; i++) {
		Water_Pool* pool = &waterGlobs.poolList[i];

		if (i == 0) {
			// What flags have changed since last update?
			bool V = (pool->flags & WATER_FLAG_VISIBLE);
			bool A = (pool->flags & WATER_FLAG_FULL);
			bool B = (pool->flags & WATER_FLAG_SETTLED);
			bool v = ((flagsLast ^ pool->flags) & WATER_FLAG_VISIBLE);
			bool a = ((flagsLast ^ pool->flags) & WATER_FLAG_FULL);
			bool b = ((flagsLast ^ pool->flags) & WATER_FLAG_SETTLED);
			if (v || a || b) {
				Error_InfoF("[%i] Visible=%s%s  A=%s%s  B=%s%s",
							flagsCounter++,
							(V ? "On " : "Off"), (v ? "*" : " "),
							(A ? "On " : "Off"), (a ? "*" : " "),
							(B ? "On " : "Off"), (b ? "*" : " "));
			}
			flagsLast = pool->flags;
		}

		bool lowering = false;
		bool rising   = !(pool->flags & WATER_FLAG_FULL); // Rise if we're not already full.
		real32 lowestWaterLevel = pool->highWaterLevel;

		for (uint32 j = 0; j < pool->drainCount; j++) {
			Water_PoolDrain* drain = &pool->drainList[j];

			// Is this actively draining water?
			if (drain->active && pool->currWaterLevel <= drain->drainWaterLevel) {

				lowestWaterLevel = std::max(lowestWaterLevel, drain->drainWaterLevel);
				//if (lowestWaterLevel < drain->drainWaterLevel)
				//	lowestWaterLevel = drain->drainWaterLevel;

				numActiveDrains++;
				lowering = !(pool->flags & WATER_FLAG_SETTLED); // Lower if we're not already settled.
				rising   = false;
			}

			// These values are used to assign NOTHOT flags to blocks some distance away. They make no sense...
			drain->elapsedUp_c += elapsedGame * 1.5f;
			if (!drain->active || pool->currWaterLevel >= drain->drainWaterLevel)
				drain->elapsedDown_10 += elapsedGame * 0.5f; // Draining faster(?)

			// Cap spill out distance(?)
			drain->elapsedUp_c    = std::min(drain->elapsedUp_c,    240.0f); //  6.4 seconds to reach.
			drain->elapsedDown_10 = std::min(drain->elapsedDown_10, 240.0f); // 19.2 seconds to reach.
			//if (drain->elapsedUp_c > 240.0f)
			//	drain->elapsedUp_c = 240.0f;

			//if (drain->elapsedDown_10 > 240.0f)
			//	drain->elapsedDown_10 = 240.0f;
		}


		if (lowering || rising) {
			const real32 waterLevelDiff = (pool->currWaterLevel - pool->highWaterLevel) / level->DigDepth;

			if (lowering) { // Water is lowering (draining).
				pool->flags &= ~WATER_FLAG_FULL;
				pool->currWaterLevel += (static_cast<real32>(numActiveDrains) * elapsedGame * 0.3f) / (waterLevelDiff + 1.0f);
				if (pool->currWaterLevel >= lowestWaterLevel) {
					pool->currWaterLevel = lowestWaterLevel;
					pool->flags |= WATER_FLAG_SETTLED;
				}
			}
			else { // Water is rising (filling).
				pool->flags &= ~WATER_FLAG_SETTLED;
				pool->currWaterLevel -= elapsedGame * 0.05f;
				if (pool->currWaterLevel <= lowestWaterLevel) {
					pool->currWaterLevel = lowestWaterLevel;
					pool->flags |= WATER_FLAG_FULL;
				}
			}

			const ColourRGBAF rgba = WATER_COLOURRGBA;
			const real32 alpha = std::max(0.0f, rgba.alpha - waterLevelDiff * 0.4f);
			Gods98::Container_SetColourAlpha(pool->contMeshTrans, rgba.red, rgba.green, rgba.blue, alpha);

			for (uint32 j = 0; j < pool->blockCount; j++) {

				Vertex vertices[4];
				Gods98::Container_Mesh_GetVertices(pool->contMeshTrans, j, 0, 4, vertices);

				// Copy water level z to vertex positions.
				for (uint32 k = 0; k < 4; k++) {
					vertices[k].position.z = pool->currWaterLevel;
				}
				Gods98::Container_Mesh_SetVertices(pool->contMeshTrans, j, 0, 4, vertices);
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

	for (uint32 i = 0; i < waterGlobs.poolCount; i++) {
		const Water_Pool* pool = &waterGlobs.poolList[i];
		for (uint32 j = 0; j < pool->drainCount; j++) {
			const Water_PoolDrain* drain = &pool->drainList[j];

			/// CHANGE: All of the following was done twice for both the elapseUp and elapseDown states, we can safely avoid code duplication.
			/// FIXME: REAL TO INT COORDINATES
			const uint32 bx = static_cast<uint32>(pool->blocks[drain->blockIndex].x);
			const uint32 by = static_cast<uint32>(pool->blocks[drain->blockIndex].y);
			const Direction dir = drain->direction;

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
			Gods98::Maths_RayEndPoint(&wPos, &basePos, &DIRECTIONS[dir], drain->elapsedUp_c);
			if (Map3D_WorldToBlockPos_NoZ(level->map, wPos.x, wPos.y, &bxW, &byW))
				Level_Block_SetNotHot(level, bxW, byW, true);

			// And set 'HOT' for elapsedDown blocks (HOT only applies to lava/eroding blocks).
			Gods98::Maths_RayEndPoint(&wPos, &basePos, &DIRECTIONS[dir], drain->elapsedDown_10);
			if (Map3D_WorldToBlockPos_NoZ(level->map, wPos.x, wPos.y, &bxW, &byW))
				Level_Block_SetNotHot(level, bxW, byW, false);
		}
	}
}

// <LegoRR.exe @0046eb60>
LegoRR::Water_Pool* __cdecl LegoRR::Water_FindPoolDrain(uint32 bx, uint32 by, OPTIONAL OUT uint32* drainIndex)
{
	const Point2F DIRECTIONS[4] = {
		{  0.0f, -1.0f },
		{  1.0f,  0.0f },
		{  0.0f,  1.0f },
		{ -1.0f,  0.0f },
	};

	for (uint32 i = 0; i < waterGlobs.poolCount; i++) {
		Water_Pool* pool = &waterGlobs.poolList[i];
		for (uint32 j = 0; j < pool->drainCount; j++) {
			const Water_PoolDrain* drain = &pool->drainList[j];

			/// FIXME: REAL TO INT COORDINATES
			if ((bx == static_cast<uint32>(pool->blocks[drain->blockIndex].x + DIRECTIONS[drain->direction].x)) &&
				(by == static_cast<uint32>(pool->blocks[drain->blockIndex].y + DIRECTIONS[drain->direction].y)))
			{
				if (drainIndex) *drainIndex = j;

				return pool;
			}
		}
	}
	return nullptr;
}

// DATA: const Water_Pool*
// <LegoRR.exe @0046ec60>
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

// <LegoRR.exe @0046ec90>
LegoRR::Water_Pool* __cdecl LegoRR::Water_FindPoolAndMergeRows(uint32 by, uint32 bxRowStart, uint32 bxRowEnd)
{
	if (by == 0) // Can't check for pools in the previous column, this is the first column.
		return nullptr;

	Water_Pool* mainPool = nullptr;
	for (uint32 bx = bxRowStart; bx < bxRowEnd; bx++) {

		for (uint32 i = 0; i < waterGlobs.poolCount; i++) {
			Water_Pool* pool = &waterGlobs.poolList[i];

			for (uint32 j = 0; j < pool->blockCount; j++) {

				// Check for pools connected in the previous column (y - 1).
				/// FIXME: REAL TO INT COORDINATES
				if (static_cast<real32>(bx)     == pool->blocks[j].x &&
					static_cast<real32>(by - 1) == pool->blocks[j].y)
				{
					if (mainPool == nullptr) {
						// This is our first-found pool within the block row.
						mainPool = pool;
					}
					else if (mainPool != pool) {
						Error_Fatal(waterGlobs.mergeCount >= WATER_MAXMERGEPOOLS, "Ran out of water merge pairs");

						// Register this pool to be merged with mainPool at the end of Initialise.
						waterGlobs.mergeList[waterGlobs.mergeCount].mainPool = mainPool;
						waterGlobs.mergeList[waterGlobs.mergeCount].removedPool = pool;
						waterGlobs.mergeCount++;
					}
				}
			}
		}
	}
	return mainPool;
}

// <LegoRR.exe @0046ed90>
void __cdecl LegoRR::Water_AddPoolRowBlocks(Water_Pool* pool, uint32 by, uint32 bxRowStart, uint32 bxRowEnd)
{
	for (uint32 bx = bxRowStart; bx < bxRowEnd; bx++) {
		Error_FatalF(pool->blockCount >= WATER_MAXPOOLBLOCKS, "Ran out of water pool blocks for pool at index %i",
					 poolIndex(pool));

		/// FIXME: REAL TO INT COORDINATES
		pool->blocks[pool->blockCount].x = static_cast<real32>(bx);
		pool->blocks[pool->blockCount].y = static_cast<real32>(by);
		pool->blockCount++;
	}
}

// <LegoRR.exe @0046edf0>
void __cdecl LegoRR::Water_AddPool(uint32 by, uint32 bxRowStart, uint32 bxRowEnd)
{
	Error_Fatal(waterGlobs.poolCount >= WATER_MAXPOOLS, "Ran out of pool entries");

	Water_Pool* pool = &waterGlobs.poolList[waterGlobs.poolCount];
	pool->blockCount = 0;
	Water_AddPoolRowBlocks(pool, by, bxRowStart, bxRowEnd);

	waterGlobs.poolCount++;
}

#pragma endregion
