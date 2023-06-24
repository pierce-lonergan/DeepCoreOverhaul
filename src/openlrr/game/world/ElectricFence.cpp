// ElectricFence.cpp : 
//

#include "../../engine/core/Maths.h"
#include "../../engine/core/Memory.h"

#include "../effects/Effects.h"
#include "../object/Object.h"
#include "../object/Stats.h"
#include "../object/Weapons.h"
#include "../Game.h"
#include "ElectricFence.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004c8df8>
LegoRR::ElectricFence_Globs & LegoRR::efenceGlobs = *(LegoRR::ElectricFence_Globs*)0x004c8df8;

LegoRR::ElectricFence_ListSet LegoRR::efenceListSet = LegoRR::ElectricFence_ListSet(LegoRR::efenceGlobs);

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0040ccf0>
void __cdecl LegoRR::ElectricFence_Initialise(Lego_Level* level)
{
	/// FIXME: This code seems fishy, grid lookup is always done with multiples of 0xc, not 0x14.
	///        The wrong block type `sizeof(ElectricFence_Block)` may have accidentically been used here.
	const uint32 gridSize = level->width * level->height * 0x14; // sizeof(ElectricFence_GridBlock);

	efenceGlobs.fenceGrid = (ElectricFence_GridBlock*)Gods98::Mem_Alloc(gridSize);
	if (efenceGlobs.fenceGrid != nullptr) {
		std::memset(efenceGlobs.fenceGrid, 0, gridSize);
	}

	efenceGlobs.level = level;

	efenceListSet.Initialise();
}

// <LegoRR.exe @0040cd60>
void __cdecl LegoRR::ElectricFence_Shutdown(void)
{
	if (efenceGlobs.fenceGrid != nullptr) {
		Gods98::Mem_Free(efenceGlobs.fenceGrid);
		efenceGlobs.fenceGrid = nullptr;
	}

	efenceListSet.Shutdown();
}

// <LegoRR.exe @0040cdb0>
void __cdecl LegoRR::ElectricFence_Restart(Lego_Level* level)
{
	ElectricFence_Shutdown();
	ElectricFence_Initialise(level);
}

// <LegoRR.exe @0040cdd0>
void __cdecl LegoRR::ElectricFence_UpdateConnectionEStuds(sint32 bx, sint32 by)
{
	// Going counter-clockwise instead of the standard clockwise.
	const Point2I DIRECTIONS[DIRECTION__COUNT] = {
		{  0,  1 },
		{  1,  0 },
		{  0, -1 },
		{ -1,  0 },
	};

	// The Level_BlockUpdateSurface function will handle adding and removing EStuds.

	Level_BlockUpdateSurface(efenceGlobs.level, bx, by, 0);

	for (uint32 i = 0; i < _countof(DIRECTIONS); i++) {
		const Point2I blockOffPos = {
			bx + DIRECTIONS[i].x,
			by + DIRECTIONS[i].y,
		};

		if (Map3D_IsInsideDimensions(efenceGlobs.level->map, blockOffPos.x, blockOffPos.y)) {
			Level_BlockUpdateSurface(efenceGlobs.level, blockOffPos.x, blockOffPos.y, 0);
		}
	}
}

// Public creation function for electric fences.
// <LegoRR.exe @0040ce80>
LegoRR::ElectricFence_Block* __cdecl LegoRR::ElectricFence_CreateFence(LegoObject* liveObj)
{
	Point2I blockPos = { 0, 0 }; // dummy init
	LegoObject_GetBlockPos(liveObj, &blockPos.x, &blockPos.y);

	return ElectricFence_Create(liveObj, blockPos.x, blockPos.y);
}

// <LegoRR.exe @0040ceb0>
LegoRR::ElectricFence_Block* __cdecl LegoRR::ElectricFence_Create(LegoObject* liveObj, sint32 bx, sint32 by)
{
	ElectricFence_Block* newEFence = efenceListSet.Add();
	//ListSet::MemZero(newEFence); // No need to memzero, all fields are assigned.

	newEFence->efenceObject = liveObj;
	newEFence->blockPos.x = bx;
	newEFence->blockPos.y = by;
	newEFence->timer = 0.0f;

	efenceBlockValue(bx, by).flags = FENCEGRID_FLAG_NONE;
	efenceBlockValue(bx, by).efence = newEFence;

	liveObj->flags2 |= LIVEOBJ2_ACTIVEELECTRICFENCE;

	// Add electric fence studs to created connections.
	ElectricFence_UpdateConnectionEStuds(bx, by);
	return newEFence;
}

// <LegoRR.exe @0040cf60>
void __cdecl LegoRR::ElectricFence_AddList(void)
{
	// NOTE: This function is no longer called, efenceListSet.Add already does so.
	efenceListSet.AddList();
}

// Public removal function for electric fences.
// <LegoRR.exe @0040cfd0>
void __cdecl LegoRR::ElectricFence_RemoveFence(LegoObject* liveObj)
{
	if (liveObj->flags2 & LIVEOBJ2_ACTIVEELECTRICFENCE) {

		sint32 bx = 0, by = 0; // dummy init
		LegoObject_GetBlockPos(liveObj, &bx, &by);

		if (efenceBlockValue(bx, by).efence != nullptr) {
			ElectricFence_Remove(efenceBlockValue(bx, by).efence, bx, by);
		}
	}
}

// <LegoRR.exe @0040d030>
void __cdecl LegoRR::ElectricFence_Remove(ElectricFence_Block* dead, sint32 bx, sint32 by)
{
	efenceListSet.Remove(dead);

	efenceBlockValue(bx, by).flags = FENCEGRID_FLAG_NONE;
	efenceBlockValue(bx, by).efence = nullptr;

	// Remove electric fence studs from removed connections.
	ElectricFence_UpdateConnectionEStuds(bx, by);
}

// <LegoRR.exe @0040d0a0>
bool32 __cdecl LegoRR::ElectricFence_Debug_PlaceFence(sint32 bx, sint32 by)
{
	if (ElectricFence_CanPlaceFenceAtBlock(bx, by)) {
		Point2F wPos2D = { 0.0f, 0.0f }; // dummy init
		Map3D_BlockToWorldPos(efenceGlobs.level->map, bx, by, &wPos2D.x, &wPos2D.y);

		LegoObject* efenceObj = LegoObject_CreateInWorld(legoGlobs.contElectricFence,
														 LegoObject_ElectricFence, (LegoObject_ID)0, 0,
														 wPos2D.x, wPos2D.y, 0.0f);
		ElectricFence_Create(efenceObj, bx, by);
		return true;
	}
	return false;
}

// <LegoRR.exe @0040d120>
bool32 __cdecl LegoRR::ElectricFence_Debug_RemoveFence(sint32 bx, sint32 by)
{
	ElectricFence_Block* efence = efenceBlockValue(bx, by).efence;
	if (efence != nullptr) {
		LegoObject* efenceObj = efence->efenceObject;

		ElectricFence_RemoveFence(efenceObj);
		LegoObject_Remove(efenceObj);
		return true;
	}
	return false;
}

// <LegoRR.exe @0040d170>
bool32 __cdecl LegoRR::ElectricFence_CanPlaceFenceAtBlock(sint32 bx, sint32 by)
{
	// Going counter-clockwise instead of the standard clockwise.
	//const Point2I DIRECTIONS_SHORT[DIRECTION__COUNT];
	//const Point2I DIRECTIONS_LONG[DIRECTION__COUNT];
	const Point2I DIRECTIONS[DIRECTION__COUNT * 2] = {
		{  0,  1 },
		{  1,  0 },
		{  0, -1 },
		{ -1,  0 },

		{  0,  2 },
		{  2,  0 },
		{  0, -2 },
		{ -2,  0 },
	};

	// TODO: The check for lake surface type is missing.
	Lego_Block* block = &blockValue(efenceGlobs.level, bx, by);
	if (!(block->flags1 & BLOCK1_FLOOR) ||          // Not a floor.
		block->terrain == Lego_SurfaceType_Lava ||  // Invalid surface types.
		//block->terrain == Lego_SurfaceType_Lake ||
		block->terrain == Lego_SurfaceType_Water ||
		ElectricFence_BlockHasFence(bx, by) ||      // Fence already exists.
		ElectricFence_BlockHasBuilding(efenceGlobs.level, bx, by, false)) // Occupied by building.
	{
		return false;
	}

	/// REFACTOR: Instead of checking short and long distances in a single loop,
	///            just check long distances after short distances.
	for (uint32 i = 0; i < _countof(DIRECTIONS); i++) {
		const Point2I blockOffPos = {
			bx + DIRECTIONS[i].x,
			by + DIRECTIONS[i].y,
		};

		if (Map3D_IsInsideDimensions(efenceGlobs.level->map, blockOffPos.x, blockOffPos.y)) {
			if (ElectricFence_BlockHasFence(blockOffPos.x, blockOffPos.y)) {
				return true; // A fence is nearby, we can connect to that.
			}
			if (ElectricFence_BlockHasBuilding(efenceGlobs.level, blockOffPos.x, blockOffPos.y, false)) {
				return true; // A building is nearby, we can connect to that.
			}
		}
	}
	return false;
}

// <LegoRR.exe @0040d320>
bool32 __cdecl LegoRR::ElectricFence_BlockHasBuilding(Lego_Level* level, sint32 bx, sint32 by, bool32 checkPowered)
{
	/// FIXME: This function check is very lazy, and doesn't account for secondary building blocks.

	// Level parameter isn't even used...
	const Point2I blockPos = { bx, by };
	return (Level_Block_IsSolidBuilding(bx, by, true) && (!checkPowered || Level_Block_IsPowered(&blockPos)));
}

// <LegoRR.exe @0040d380>
void __cdecl LegoRR::ElectricFence_UpdateAll(real32 elapsedWorld)
{
	// Reset visited nodes for our upcoming recursive function.
	// Also update the recharge timer of fences.
	for (auto efence : efenceListSet.EnumerateAlive()) {
		ElectricFence_Callback_ResetVisitedAndUpdateTimer(efence, &elapsedWorld);
	}
	//ElectricFence_RunThroughLists(ElectricFence_Callback_ResetVisitedAndUpdateTimer, &elapsedWorld);
	for (auto obj : objectListSet.EnumerateSkipUpgradeParts()) {
		ElectricFence_Callback_ResetBuildingVisited(obj, nullptr);
	}
	//LegoObject_RunThroughListsSkipUpgradeParts(ElectricFence_Callback_ResetBuildingVisited, nullptr);

	// Update which fence and fence stud blocks are currently powered.
	// Chain beams originating from buildings.
	for (auto obj : objectListSet.EnumerateSkipUpgradeParts()) {
		ElectricFence_Callback_ChainBeamsFromBuilding(obj, &elapsedWorld);
	}
	//LegoObject_RunThroughListsSkipUpgradeParts(ElectricFence_Callback_ChainBeamsFromBuilding, &elapsedWorld);
}

// <LegoRR.exe @0040d3c0>
void __cdecl LegoRR::ElectricFence_RunThroughLists(ElectricFence_RunThroughListsCallback callback, void* data)
{
	// ElectricFence has the only RunThroughLists function without a conditional stop.
	for (auto efence : efenceListSet.EnumerateAlive()) {
		callback(efence, data);
	}
}

// <LegoRR.exe @0040d420>
bool32 __cdecl LegoRR::ElectricFence_ChainBeamsFromBuildingOrBlock(OPTIONAL LegoObject* liveObj, uint32 bx, uint32 by)
{
	for (auto efence : efenceListSet.EnumerateAlive()) {
		ElectricFence_Callback_ResetVisitedAndUpdateTimer(efence, nullptr);
	}
	//ElectricFence_RunThroughLists(ElectricFence_Callback_ResetVisitedAndUpdateTimer, nullptr);

	for (auto obj : objectListSet.EnumerateSkipUpgradeParts()) {
		ElectricFence_Callback_ResetBuildingVisited(obj, nullptr);
	}
	//LegoObject_RunThroughListsSkipUpgradeParts(ElectricFence_Callback_ResetBuildingVisited, nullptr);

	if (liveObj == nullptr) {
		if (ElectricFence_BlockHasFence(bx, by) ||
			ElectricFence_BlockHasBuilding(efenceGlobs.level, bx, by, false))
		{
			ElectricFence_ChainBeams_Recurse(bx, by, true, 0.0f);
			return true;
		}
	}
	else {
		/// FIXME: This function doesn't account for secondary building blocks.

		Point2F wPos2D = { 0.0f, 0.0f }; // dummy init
		LegoObject_GetPosition(liveObj, &wPos2D.x, &wPos2D.y);

		if ((StatsObject_GetStatsFlags2(liveObj) & STATS2_SELFPOWERED) ||
			(liveObj->flags3 & LIVEOBJ3_HASPOWER))
		{
			// Don't use bx/by, use the object's blockPos.
			Point2I objBlockPos = { 0, 0 }; // dummy init
			Map3D_WorldToBlockPos_NoZ(efenceGlobs.level->map, wPos2D.x, wPos2D.y, &objBlockPos.x, &objBlockPos.y);
			ElectricFence_ChainBeams_Recurse(objBlockPos.x, objBlockPos.y, true, 0.0f);
			return true;
		}
	}
	return false;
}

// DATA: real32* pElapsedWorld
// <LegoRR.exe @0040d510>
bool32 __cdecl LegoRR::ElectricFence_Callback_ChainBeamsFromBuilding(LegoObject* liveObj, void* pElapsedWorld)
{
	const real32 elapsedWorld = *reinterpret_cast<real32*>(pElapsedWorld);

	if (liveObj->type == LegoObject_Building) {
		Point2I blockPos = { 0, 0 }; // dummy init
		LegoObject_GetBlockPos(liveObj, &blockPos.x, &blockPos.y);

		// The building at this block is powered.
		if (ElectricFence_BlockHasBuilding(efenceGlobs.level, blockPos.x, blockPos.y, true)) {

			// Get rotation for building shape.
			Point2F faceDir;
			LegoObject_GetFaceDirection(liveObj, &faceDir);
			Direction direction;
			if (std::abs(faceDir.y) >= std::abs(faceDir.x)) {
				// Note that world coordinate y is flipped from block coordinate y. So -1 y block pos is +1 y world pos.
				if (faceDir.y > 0.0f)
					direction = DIRECTION_UP;
				else
					direction = DIRECTION_DOWN;
			}
			else {
				if (faceDir.x > 0.0f)
					direction = DIRECTION_RIGHT;
				else
					direction = DIRECTION_LEFT;
			}

			// Transform shape for building.
			uint32 shapeCount;
			const Point2I* shapePoints = Building_GetShapePoints(liveObj->building, &shapeCount);
			const Point2I* shapeBlocks = SelectPlace_TransformShapePoints(&blockPos, shapePoints, shapeCount, direction);

			// Act on solid building shape blocks.
			for (uint32 i = 0; i < shapeCount; i++) {
				const Point2I shapeBlock = shapeBlocks[i];

				if ((i == (shapeCount - 1)) ||
					(shapeBlock.x != shapeBlocks[i + 1].x) ||
					(shapeBlock.y != shapeBlocks[i + 1].y))
				{
					// This is a solid building block.
					ElectricFence_ChainBeams_Recurse(shapeBlock.x, shapeBlock.y, false, elapsedWorld);
				}
				else {
					i++; // Skip the next shape point.
				}
			}
		}
	}

	return false;
}

// <LegoRR.exe @0040d650>
bool32 __cdecl LegoRR::ElectricFence_Callback_ResetBuildingVisited(LegoObject* liveObj, void* unused)
{
	if (liveObj->type == LegoObject_Building) {
		Point2I blockPos = { 0, 0 }; // dummy init
		LegoObject_GetBlockPos(liveObj, &blockPos.x, &blockPos.y);
		efenceBlockValue(blockPos.x, blockPos.y).flags = FENCEGRID_FLAG_NONE;
	}
	return false;
}

// DATA: real32* pElapsedWorld
// <LegoRR.exe @0040d6a0>
void __cdecl LegoRR::ElectricFence_Callback_ResetVisitedAndUpdateTimer(ElectricFence_Block* efence, OPTIONAL void* pElapsedWorld)
{
	const Point2I DIRECTIONS[DIRECTION__COUNT] = {
		{  0,  1 },
		{  1,  0 },
		{  0, -1 },
		{ -1,  0 },
	};

	efenceBlockValue(efence->blockPos.x, efence->blockPos.y).flags = FENCEGRID_FLAG_NONE;

	for (uint32 i = 0; i < _countof(DIRECTIONS); i++) {
		const Point2I blockOffPos = {
			efence->blockPos.x + DIRECTIONS[i].x,
			efence->blockPos.y + DIRECTIONS[i].y,
		};

		if (Map3D_IsInsideDimensions(efenceGlobs.level->map, blockOffPos.x, blockOffPos.y)) {
			efenceBlockValue(blockOffPos.x, blockOffPos.y).flags = FENCEGRID_FLAG_NONE;
		}
	}

	if (pElapsedWorld != nullptr) {
		const real32 elapsedWorld = *reinterpret_cast<real32*>(pElapsedWorld);

		if (efence->timer > 0.0f) {
			efence->timer -= elapsedWorld;
		}
	}
}

// <LegoRR.exe @0040d780>
void __cdecl LegoRR::ElectricFence_ChainBeams_Recurse(uint32 bx, uint32 by, bool32 forceSpawnBeam, real32 elapsedWorld)
{
	const Point2I DIRECTIONS_SHORT[DIRECTION__COUNT] = {
		{  0,  1 },
		{  1,  0 },
		{  0, -1 },
		{ -1,  0 },
	};
	const Point2I DIRECTIONS_LONG[DIRECTION__COUNT] = {
		{  0,  2 },
		{  2,  0 },
		{  0, -2 },
		{ -2,  0 },
	};
	static_assert(_countof(DIRECTIONS_SHORT) == _countof(DIRECTIONS_LONG), "ElectricFence direction array lengths don't match");


	efenceBlockValue(bx, by).flags |= FENCEGRID_FLAG_POWERED; // Mark as visited and powered.

	for (uint32 i = 0; i < _countof(DIRECTIONS_SHORT); i++) {
		/// REFACTOR: Swap shortBeam bool to match Effect_Spawn_ElectricFenceBeam input.
		bool connectionFound = false;
		bool longBeam = false; // dummy init
		uint32 bxFound = 0; // dummy init
		uint32 byFound = 0; // dummy init

		const uint32 bxS = bx + DIRECTIONS_SHORT[i].x;
		const uint32 byS = by + DIRECTIONS_SHORT[i].y;

		const uint32 bxL = bx + DIRECTIONS_LONG[i].x;
		const uint32 byL = by + DIRECTIONS_LONG[i].y;

		if (Map3D_IsInsideDimensions(efenceGlobs.level->map, bxS, byS)) {
			if (ElectricFence_BlockHasFence(bxS, byS) || (ElectricFence_BlockHasBuilding(efenceGlobs.level, bxS, byS, true) &&
														  !ElectricFence_BlockHasBuilding(efenceGlobs.level, bx, by, false)))
			{
				connectionFound = true;
				longBeam = false;
				bxFound = bxS;
				byFound = byS;
			}
		}

		if (!connectionFound && Map3D_IsInsideDimensions(efenceGlobs.level->map, bxL, byL)) {
			if (!ElectricFence_BlockHasBuilding(efenceGlobs.level, bxS, byS, false) ||
				!ElectricFence_BlockHasBuilding(efenceGlobs.level, bx, by, false))
			{
				if (ElectricFence_BlockHasFence(bxL, byL) || (ElectricFence_BlockHasBuilding(efenceGlobs.level, bxL, byL, true) &&
															  !ElectricFence_BlockHasBuilding(efenceGlobs.level, bx, by, false)))
				{
					connectionFound = true;
					longBeam = true;
					bxFound = bxL;
					byFound = byL;
				}
			}
		}

		if (connectionFound) {
			if (longBeam) {
				// Mark gap between fences/buildings as visited and powered.
				efenceBlockValue(bxS, byS).flags |= FENCEGRID_FLAG_POWERED;
			}

			if (efenceBlockValue(bxFound, byFound).efence == nullptr ||
				efenceBlockValue(bxFound, byFound).efence->timer <= 0.0f)
			{
				// Check if a beam has been spawned by bxFound,byFound to bx,by. If so, then don't try to spawn another.
				if (!(efenceBlockValue(bxFound, byFound).flags & DirectionToFlag(i))) {
					// We tried to spawn a beam from bx,by to bxFound,byFound.
					efenceBlockValue(bx, by).flags |= (ElectricFence_GridFlags)DirectionToFlag(DirectionFlip(i)); // Rotate 180.

					if (!forceSpawnBeam) {
						/// SANITY: Check if elapsedWorld is > 0.0f instead of != 0.0f.
						if (elapsedWorld > 0.0f) {
							const uint32 rng = (uint32)Gods98::Maths_Rand();
							const uint32 chance = std::max(1u, (uint32)(200.0f / std::min(200.0f, elapsedWorld)));
							if ((rng % chance) == 0) {
								ElectricFence_TrySpawnBeamBetweenBlocks(bx, by, bxFound, byFound);
							}
						}
					}
					else {
						ElectricFence_TrySpawnBeamBetweenBlocks(bx, by, bxFound, byFound);
					}
				}

				// Only recurse over non-visited, non-building blocks.
				// Apparently you can't recurse through fences that are recharging...  this intentional?
				if (!ElectricFence_BlockHasBuilding(efenceGlobs.level, bxFound, byFound, false) &&
					!(efenceBlockValue(bxFound, byFound).flags & FENCEGRID_FLAG_POWERED))
				{
					// Recursion...
					ElectricFence_ChainBeams_Recurse(bxFound, byFound, forceSpawnBeam, elapsedWorld);
				}
			}
		}
	}
}

// <LegoRR.exe @0040db50>
bool32 __cdecl LegoRR::ElectricFence_TrySpawnBeamBetweenBlocks(uint32 bxFrom, uint32 byFrom, uint32 bxTo, uint32 byTo)
{
	const Point2I DIRECTIONS_SHORT[DIRECTION__COUNT] = {
		{  0,  1 },
		{  1,  0 },
		{  0, -1 },
		{ -1,  0 },
	};
	const Point2I DIRECTIONS_LONG[DIRECTION__COUNT] = {
		{  0,  2 },
		{  2,  0 },
		{  0, -2 },
		{ -2,  0 },
	};
	static_assert(_countof(DIRECTIONS_SHORT) == _countof(DIRECTIONS_LONG), "ElectricFence direction array lengths don't match");

	/// REFACTOR: Swap shortBeam bool to match Effect_Spawn_ElectricFenceBeam input.
	bool connectionFound = false;
	bool longBeam = false; // dummy init

	for (uint32 i = 0; i < _countof(DIRECTIONS_SHORT); i++) {
		if (bxTo == (bxFrom + DIRECTIONS_SHORT[i].x) && byTo == (byFrom + DIRECTIONS_SHORT[i].y)) {
			connectionFound = true;
			longBeam = false;
			break;
		}
		if (bxTo == (bxFrom + DIRECTIONS_LONG[i].x) && byTo == (byFrom + DIRECTIONS_LONG[i].y)) {
			connectionFound = true;
			longBeam = true;
			break;
		}
	}

	if (connectionFound) {
		Vector3F dir;
		ElectricFence_GetDirectionBetweenBlocks(bxFrom, byFrom, bxTo, byTo, &dir);

		Vector3F wPos = { 0.0f, 0.0f, 0.0f }; // dummy init
		Map3D_BlockToWorldPos(efenceGlobs.level->map, bxFrom, byFrom, &wPos.x, &wPos.y);
		wPos.z = Map3D_GetWorldZ(efenceGlobs.level->map, wPos.x, wPos.y);

		Effect_Spawn_ElectricFenceBeam(longBeam, wPos.x, wPos.y, wPos.z, dir.x, dir.y, dir.z);
		return true;
	}
	return false;
}

// returns parameter: dir
// <LegoRR.exe @0040dcc0>
Vector3F* __cdecl LegoRR::ElectricFence_GetDirectionBetweenBlocks(uint32 bxFrom, uint32 byFrom, uint32 bxTo, uint32 byTo, OUT Vector3F* dir)
{
	Vector3F wPosFrom = { 0.0f, 0.0f, 0.0f }; // dummy init
	Map3D_BlockToWorldPos(efenceGlobs.level->map, bxFrom, byFrom, &wPosFrom.x, &wPosFrom.y);
	wPosFrom.z = Map3D_GetWorldZ(efenceGlobs.level->map, wPosFrom.x, wPosFrom.y);

	Vector3F wPosTo = { 0.0f, 0.0f, 0.0f }; // dummy init
	Map3D_BlockToWorldPos(efenceGlobs.level->map, bxTo, byTo, &wPosTo.x, &wPosTo.y);
	wPosTo.z = Map3D_GetWorldZ(efenceGlobs.level->map, wPosTo.x, wPosTo.y);

	return Gods98::Maths_Vector3DSubtract(dir, &wPosTo, &wPosFrom);
}

// <LegoRR.exe @0040dd70>
bool32 __cdecl LegoRR::ElectricFence_TrySparkObject(LegoObject* liveObj)
{
	const Point2I DIRECTIONS[DIRECTION__COUNT] = {
		{  0,  1 },
		{  1,  0 },
		{  0, -1 },
		{ -1,  0 },
	};

	Point2F wObjectPos = { 0.0f, 0.0f }; // dummy init
	LegoObject_GetPosition(liveObj, &wObjectPos.x, &wObjectPos.y);

	Point2I blockPos = { 0, 0 }; // dummy init
	Map3D_WorldToBlockPos_NoZ(efenceGlobs.level->map, wObjectPos.x, wObjectPos.y, &blockPos.x, &blockPos.y);

	// Is this block guarded by a powered fence?
	if ((efenceBlockValue(blockPos.x, blockPos.y).flags & FENCEGRID_FLAG_POWERED) &&
		!ElectricFence_BlockHasBuilding(efenceGlobs.level, blockPos.x, blockPos.y, false))
	{
		bool fenceFound = false;
		Point2I fenceBlockPos = blockPos;

		if (efenceBlockValue(blockPos.x, blockPos.y).efence != nullptr) {
			// Fence on object's block exists, use its recharge timer.
			if (efenceBlockValue(blockPos.x, blockPos.y).efence->timer <= 0.0f) {
				// Fence has finished recharging.
				fenceFound = true;
				fenceBlockPos = blockPos;
			}
		}
		else {
			real32 minDist = 0.0f;
			//uint32 dirTarget = 0; // dummy init

			for (uint32 i = 0; i < _countof(DIRECTIONS); i++) {
				const Point2I blockOffPos = {
					blockPos.x + DIRECTIONS[i].x,
					blockPos.y + DIRECTIONS[i].y,
				};

				/// FIX APPLY: Check bounds.
				if (Map3D_IsInsideDimensions(efenceGlobs.level->map, blockOffPos.x, blockOffPos.y)) {
					ElectricFence_Block* efence = efenceBlockValue(blockOffPos.x, blockOffPos.y).efence;
					if (efence != nullptr && efence->timer <= 0.0f) {
						// Fence adjacent to object's block exists and has finished recharging.

						Point2F wBlockPos = { 0.0f, 0.0f }; // dummy init
						Map3D_BlockToWorldPos(efenceGlobs.level->map, blockOffPos.x, blockOffPos.y, &wBlockPos.x, &wBlockPos.y);

						const real32 dist = Gods98::Maths_Vector2DDistance(&wObjectPos, &wBlockPos);
						if (dist < minDist || minDist == 0.0f) {
							minDist = dist;
							/// REFACTOR: Just assign fenceBlockPos here instead of after loop.
							fenceFound = true;
							fenceBlockPos = blockOffPos;
							//dirTarget = i;

							/// SANITY: Break if we have a distance of zero, because otherwise checks won't account for it.
							if (dist == 0.0f)
								break;
						}
					}
				}
			}
			/*if (fenceFound) {
				fenceBlockPos = Point2I {
					blockPos.x + DIRECTIONS[dirTarget].x,
					blockPos.y + DIRECTIONS[dirTarget].y,
				};
			}*/
		}

		if (fenceFound) {
			ElectricFence_SparkObject(liveObj, &wObjectPos, blockPos.x, blockPos.y);
			ElectricFence_ChainBeamsFromBuildingOrBlock(nullptr, fenceBlockPos.x, fenceBlockPos.y);

			const uint32 weaponID = Weapon_GetWeaponIDByName("FenceSpark");
			const real32 rechargeTime = Weapon_GetRechargeTime(weaponID);
			efenceBlockValue(fenceBlockPos.x, fenceBlockPos.y).efence->timer = rechargeTime;
			return true;
		}
	}
	return false;
}

// <LegoRR.exe @0040dff0>
void __cdecl LegoRR::ElectricFence_SparkObject(LegoObject* liveObj, const Point2F* objWorldPos, uint32 bx, uint32 by)
{
	Point2F wPos2D = { 0.0f, 0.0f }; // dummy init
	Map3D_BlockToWorldPos(efenceGlobs.level->map, bx, by, &wPos2D.x, &wPos2D.y);
	const real32 zPos = Map3D_GetWorldZ(efenceGlobs.level->map, wPos2D.x, wPos2D.y);
	const real32 zObj = Map3D_GetWorldZ(efenceGlobs.level->map, objWorldPos->x, objWorldPos->y);

	const real32 collHeight = StatsObject_GetCollHeight(liveObj);

	Point2F dir;
	Gods98::Maths_Vector2DSubtract(&dir, objWorldPos, &wPos2D);
	const real32 zDir = ((zObj - (collHeight / 2.0f)) - zPos);

	// Create a short beam that points towards the object from the guarded block it touched.
	// Note that this will create beams even if the block is just an electric fence stud.
	Effect_Spawn_ElectricFenceBeam(false, wPos2D.x, wPos2D.y, zPos, dir.x, dir.y, zDir);

	const uint32 weaponID = Weapon_GetWeaponIDByName("FenceSpark");
	Weapon_GenericDamageObject(liveObj, weaponID, true, 0.0f, &dir);
	LegoObject_StartCrumbling(liveObj);
}

// <LegoRR.exe @0040e110>
bool32 __cdecl LegoRR::ElectricFence_IsBlockBetweenConnection(Lego_Level* level, uint32 bx, uint32 by)
{
	const Point2I DIRECTIONS[DIRECTION__COUNT] = {
		{  0,  1 },
		{  1,  0 },
		{  0, -1 },
		{ -1,  0 },
	};

	if (efenceGlobs.fenceGrid == nullptr || ElectricFence_BlockHasFence(bx, by) ||
		ElectricFence_BlockHasBuilding(level, bx, by, false))
	{
		return false;
	}

	for (uint32 i = 0; i < _countof(DIRECTIONS); i++) {
		const Point2I blockOffPos = {
			(sint32)bx + DIRECTIONS[i].x,
			(sint32)by + DIRECTIONS[i].y,
		};

		/// FIX APPLY: Check bounds.
		if (Map3D_IsInsideDimensions(efenceGlobs.level->map, blockOffPos.x, blockOffPos.y) &&
			(ElectricFence_BlockHasFence(blockOffPos.x, blockOffPos.y) ||
			 ElectricFence_BlockHasBuilding(level, blockOffPos.x, blockOffPos.y, false)))
		{
			const Point2I blockOffPos2 = { // Rotate 180.
				(sint32)bx + DIRECTIONS[(uint32)DirectionFlip(i)].x,
				(sint32)by + DIRECTIONS[(uint32)DirectionFlip(i)].y,
			};

			/// FIX APPLY: Check bounds.
			if (Map3D_IsInsideDimensions(efenceGlobs.level->map, blockOffPos2.x, blockOffPos2.y) &&
				(ElectricFence_BlockHasFence(blockOffPos2.x, blockOffPos2.y) ||
				 (ElectricFence_BlockHasBuilding(level, blockOffPos2.x, blockOffPos2.y, false) &&
				  !ElectricFence_BlockHasBuilding(level, blockOffPos.x, blockOffPos.y, false))))
			{
				return true;
			}
		}
	}
	return false;
}

// <LegoRR.exe @0040e280>
void __cdecl LegoRR::ElectricFence_AddOrRemoveEStud(Lego_Level* level, uint32 bx, uint32 by, bool32 addNew)
{
	if (efenceGlobs.fenceGrid != nullptr) {
		ElectricFence_GridBlock* efenceBlock = &efenceBlockValue(bx, by);
		if (!addNew) {
			if (efenceBlock->estudObject != nullptr) {
				LegoObject_Remove(efenceBlock->estudObject);
				efenceBlock->estudObject = nullptr;
				efenceBlock->flags = FENCEGRID_FLAG_NONE;
			}
		}
		else {
			if (efenceBlock->estudObject == nullptr) {
				Point2F wPos2D = { 0.0f, 0.0f }; // dummy init
				Map3D_BlockToWorldPos(level->map, bx, by, &wPos2D.x, &wPos2D.y);

				LegoObject* estudObj = LegoObject_CreateInWorld(legoGlobs.contElectricFenceStud,
																LegoObject_ElectricFenceStud, (LegoObject_ID)0, 0,
																wPos2D.x, wPos2D.y, 0.0f);
				efenceBlock->estudObject = estudObj;
			}
		}
	}
}

// <LegoRR.exe @0040e390>
bool32 __cdecl LegoRR::ElectricFence_BlockHasFence(sint32 bx, sint32 by)
{
	return (efenceBlockValue(bx, by).efence != nullptr);
}

#pragma endregion
