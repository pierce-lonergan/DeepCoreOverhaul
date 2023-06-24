// Construction.cpp : 
//

#include "../../engine/core/Maths.h"
#include "../../engine/core/Utils.h"

#include "../effects/Effects.h"
#include "../interface/InfoMessages.h"
#include "../interface/Interface.h"
#include "../mission/Messages.h"
#include "../object/AITask.h"
#include "../object/Object.h"
#include "../object/Stats.h"
#include "../world/ElectricFence.h"
#include "../Game.h"

#include "Construction.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004b9a20>
LegoRR::Construction_Globs & LegoRR::constructionGlobs = *(LegoRR::Construction_Globs*)0x004b9a20;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

/// CUSTOM:
bool LegoRR::Construction_Zone_IsNoBuildCost(uint32 constructHandle)
{
	Construction_Zone* construct = Construction_Zone_FindByHandleOrAtBlock(nullptr, &constructHandle);
	if (construct != nullptr) {
		return (construct->flags & CONSTRUCTION_CUSTOM_FLAG_NOBUILDCOST);
	}
	return false;
}

/// CUSTOM:
void LegoRR::Construction_Zone_SetNoBuildCost(uint32 constructHandle, bool noBuildCosts)
{
	Construction_Zone* construct = Construction_Zone_FindByHandleOrAtBlock(nullptr, &constructHandle);
	if (construct != nullptr) {
		if (noBuildCosts)
			construct->flags |= CONSTRUCTION_CUSTOM_FLAG_NOBUILDCOST;
		else
			construct->flags &= ~CONSTRUCTION_CUSTOM_FLAG_NOBUILDCOST;
	}
}


/// CUSTOM:
bool LegoRR::Construction_Zone_UsesStuds(const Construction_Zone* construct)
{
	return (construct->flags & CONSTRUCTION_FLAG_USESTUDS);
}

/// CUSTOM:
uint32 LegoRR::Construction_Zone_GetCostCrystal(Construction_Zone* construct, OPTIONAL uint32* placedCrystals)
{
	if (placedCrystals) {
		*placedCrystals = Construction_Zone_CountOfResourcePlaced(construct, LegoObject_PowerCrystal, (LegoObject_ID)0);
	}

	if (Construction_Zone_IsNoBuildCost(construct->handleValue))
		return 0;

	return Stats_GetRealCostCrystal(Construction_Zone_GetObjectType(construct), construct->objID, 0);
}

/// CUSTOM:
uint32 LegoRR::Construction_Zone_GetCostOre(Construction_Zone* construct, OPTIONAL uint32* placedOre)
{
	if (placedOre) {
		*placedOre = Construction_Zone_CountOfResourcePlaced(construct, LegoObject_Ore, LegoObject_ID_Ore);
	}

	if (Construction_Zone_IsNoBuildCost(construct->handleValue))
		return 0;
	if (Construction_Zone_UsesStuds(construct))
		return 0;

	return Stats_GetRealCostOre(Construction_Zone_GetObjectType(construct), construct->objID, 0);
}

/// CUSTOM:
uint32 LegoRR::Construction_Zone_GetCostRefinedOre(Construction_Zone* construct, OPTIONAL uint32* placedStuds)
{
	if (placedStuds) {
		*placedStuds = Construction_Zone_CountOfResourcePlaced(construct, LegoObject_Ore, LegoObject_ID_ProcessedOre);
	}

	if (Construction_Zone_IsNoBuildCost(construct->handleValue))
		return 0;
	if (!Construction_Zone_UsesStuds(construct))
		return 0;

	return Stats_GetRealCostRefinedOre(Construction_Zone_GetObjectType(construct), construct->objID, 0);
}

/// CUSTOM:
uint32 LegoRR::Construction_Zone_GetCostOreType(Construction_Zone* construct, OPTIONAL uint32* placedOreType, OPTIONAL bool* usesStuds)
{
	if (usesStuds) *usesStuds = Construction_Zone_UsesStuds(construct);

	if (Construction_Zone_UsesStuds(construct)) {
		return Construction_Zone_GetCostRefinedOre(construct, placedOreType);
	}
	else {
		return Construction_Zone_GetCostOre(construct, placedOreType);
	}
}

/// CUSTOM:
uint32 LegoRR::Construction_Zone_GetCostBarriers(Construction_Zone* construct, OPTIONAL uint32* placedBarriers)
{
	// Currently number of barriers requested is not counted,
	//  we can hack it thanks to the assumption that all non-cryore costs are barriers.

	uint32 placedCrystals, placedOreType;
	const uint32 costCrystals = Construction_Zone_GetCostCrystal(construct, &placedCrystals);
	const uint32 costOreType = Construction_Zone_GetCostOreType(construct, &placedOreType, nullptr);

	// Use max and signed integer as a failsafe for unexpected counts.

	if (placedBarriers) {
		const sint32 placed = static_cast<sint32>(construct->placedCount - (placedCrystals + placedOreType));
		*placedBarriers = static_cast<uint32>(std::max(0, placed));
	}

	const sint32 cost = static_cast<sint32>(construct->requestCount - (costCrystals + costOreType));
	return static_cast<uint32>(std::max(0, cost));
}



/// BETA: RockFall
// <LegoStripped.exe @0040b920>
void __cdecl LegoRR::Construction_LoadBuildingBases(const Gods98::Config* config, Gods98::Container* rootCont)
{
	const Gods98::Config* arrayFirst = Gods98::Config_FindArray(config, Lego_ID("Bases"));

	constructionGlobs.buildingBaseCount = 0;
	for (const Gods98::Config* item = arrayFirst; item != nullptr; item = Gods98::Config_GetNextItem(item)) {

		Error_FatalF(constructionGlobs.buildingBaseCount >= CONSTRUCTION_MAXBUILDINGBASES,
					 "Ran out of buildings bases, more than %i defined", CONSTRUCTION_MAXBUILDINGBASES);

		char* name = Gods98::Util_StrCpy(Gods98::Config_GetItemName(item));
		const char* filename = Gods98::Config_GetDataString(item);
		Gods98::Container* cont = Gods98::Container_Load(rootCont, filename, CONTAINER_LWOSTRING, true);

		Error_FatalF(cont == nullptr, "Failed to load building base file %s.", filename);

		constructionGlobs.buildingBaseName[constructionGlobs.buildingBaseCount] = name;
		constructionGlobs.buildingBaseData[constructionGlobs.buildingBaseCount] = cont;
		constructionGlobs.buildingBaseCount++;
	}
}


// Would have looked up the baseName in a list of up to 5 names, and returned the index (or buildingBaseCount on failure).
// No building bases are ever defined, so the function will always return 0 (aka buildingBaseCount).
// <LegoRR.exe @00408bb0>
//#define Construction_GetBuildingBase ((uint32 (__cdecl* )(const char* baseName))0x00408bb0)
uint32 __cdecl LegoRR::Construction_GetBuildingBase(const char* baseName)
{
	for (uint32 i = 0; i < constructionGlobs.buildingBaseCount; i++) {
		/// CHANGE: Use case-insensitive comparison instead.
		if (::_stricmp(constructionGlobs.buildingBaseName[i], baseName) == 0)
			return i;
	}
	return constructionGlobs.buildingBaseCount; // Return count on failure.
}

// <LegoRR.exe @00408c10>
void __cdecl LegoRR::Construction_Zone_PlaceResource(uint32 constructHandle, OPTIONAL LegoObject* placedObj)
{
	Construction_Zone* construct = Construction_Zone_FindByHandleOrAtBlock(nullptr, &constructHandle);
	if (construct == nullptr) {
		if (placedObj->type == LegoObject_PowerCrystal) {
			Message_PostEvent(Message_GenerateCrystalComplete, placedObj, Message_Argument(0), nullptr);
		}
		else {
			Message_PostEvent(Message_GenerateOreComplete, placedObj, Message_Argument(0), nullptr);
		}
	}
	else {
		if (placedObj != nullptr) {
			placedObj->flags4 |= LIVEOBJ4_USEDBYCONSTRUCTION;
			construct->placedObjects[construct->placedCount2] = placedObj;
			construct->placedCount2++;
			construct->placedCount++;
		}

		if (construct->requestCount == construct->placedCount) {
			// Unused float field, may have been timer for construction delay.
			construct->float_28 = (STANDARD_FRAMERATE * 5.0f); // 5 seconds.
			construct->flags |= CONSTRUCTION_FLAG_READY;
		}
	}
}

// <LegoRR.exe @00408ca0>
bool32 __cdecl LegoRR::Construction_Zone_NeedsMoreOfResource(uint32 constructHandle, LegoObject_Type objType, LegoObject_ID objID)
{
	Construction_Zone* construct = Construction_Zone_FindByHandleOrAtBlock(nullptr, &constructHandle);
	if (construct != nullptr) {
		const uint32 placed = Construction_Zone_CountOfResourcePlaced(construct, objType, objID);

		const LegoObject_Type constructObjType = Construction_Zone_GetObjectType(construct);

		uint32 cost = 0; // dummy init

		/// CHANGE: Use construction functions for Stats to always get the correct costs,
		///          in-case the user has toggled the 'No build costs' cheat since construction started.
		switch (objType) {
		case LegoObject_PowerCrystal:
			cost = Construction_Zone_GetCostCrystal(construct, nullptr);
			break;

		case LegoObject_Ore:
			if (objID == LegoObject_ID_Ore) {
				cost = Construction_Zone_GetCostOre(construct, nullptr);
			}
			else {
				cost = Construction_Zone_GetCostRefinedOre(construct, nullptr);
			}
			break;

		default:
			// Not sure what the reasoning for the return value here is. What's the use-case?
			return true;
		}

		return (placed < cost);
	}
	return false;
}

// <LegoRR.exe @00408d40>
uint32 __cdecl LegoRR::Construction_Zone_CountOfResourcePlaced(Construction_Zone* construct, LegoObject_Type objType, LegoObject_ID objID)
{
	uint32 count = 0;
	for (uint32 i = 0; i < construct->placedCount; i++) {
		if (construct->placedObjects[i]->type == objType && construct->placedObjects[i]->id == objID) {
			count++;
		}
	}
	return count;
}

// <LegoRR.exe @00408d80>
void __cdecl LegoRR::Construction_Zone_RequestResource(const Point2I* originBlockPos, LegoObject_Type objType, LegoObject_ID objID, uint32 objLevel, uint32 count)
{
	Construction_Zone* construct = Construction_Zone_FindByHandleOrAtBlock(originBlockPos, nullptr);
	if (objType == LegoObject_Ore && objID == LegoObject_ID_ProcessedOre) {
		construct->flags |= CONSTRUCTION_FLAG_USESTUDS;
	}

	for (uint32 i = 0; i < count; i++) {
		construct->requestCount++;
		AITask_DoRequest_ObjectType(objType, objID, objLevel, originBlockPos,
									construct->handleValue, nullptr, false);
	}
}

// <LegoRR.exe @00408df0>
void __cdecl LegoRR::Construction_Zone_RequestBarriers(const Point2I* originBlockPos, const Point2I* shapeBlocks, uint32 shapeCount)
{
	const Point2I DIRECTIONS[DIRECTION__COUNT] = {
		{  0, -1 },
		{  1,  0 },
		{  0,  1 },
		{ -1,  0 },
	};

	// HARDCODED CONSTANT THAT DEPENDS ON BLOCKSIZE BEING A SANE VALUE.
	const real32 topLeft = 8.8f / Map3D_BlockSize(Lego_GetMap());
	const real32 bottomRight = 1.0f - topLeft;
	// Note that BARRIER_OFFSETS rotates CCW. And Y positions are swapped from what they would be in DIRECTIONS.
	// This is because the 3D world's coordinates don't face the same way as its block coordinates.
	const Point2F BARRIER_OFFSETS[DIRECTION__COUNT] = {
		{ 0.5f, bottomRight },
		{ bottomRight, 0.5f },
		{ 0.5f,     topLeft },
		{     topLeft, 0.5f },
	};

	Construction_Zone* construct = Construction_Zone_FindByHandleOrAtBlock(originBlockPos, nullptr);

	for (uint32 i = 0; i < shapeCount; i++) {

		if ((i == (shapeCount - 1)) ||
			shapeBlocks[i].x != shapeBlocks[i + 1].x ||
			shapeBlocks[i].y != shapeBlocks[i + 1].y)
		{
			// This is a solid building block. Check all sides for other solid building blocks.
			// If a side doesn't have a solid block, then request a barrier.
			for (uint32 dir = 0; dir < _countof(DIRECTIONS); dir++) {

				bool innerConnection = false;

				for (uint32 j = 0; j < shapeCount; j++) {

					if ((j == (shapeCount - 1)) ||
						shapeBlocks[j].x != shapeBlocks[j + 1].x ||
						shapeBlocks[j].y != shapeBlocks[j + 1].y)
					{
						// This is a solid building block.
						if ((shapeBlocks[j].x == (shapeBlocks[i].x + DIRECTIONS[dir].x)) &&
							(shapeBlocks[j].y == (shapeBlocks[i].y + DIRECTIONS[dir].y)))
						{
							// This is a solid building block next to the current block we're checking.
							innerConnection = true;
							/// REFACTOR: We can break now.
							break;
						}
					}
					else {
						j++; // Skip one shape point.
					}
				}
				if (!innerConnection) {
					// This side of the current block is not connected to another solid construction tile,
					//  so we should place a barrier.
					AITask_DoRequest_ObjectType(LegoObject_Barrier, (LegoObject_ID)0, 0,
												&shapeBlocks[i], construct->handleValue,
												&BARRIER_OFFSETS[dir], true);
					construct->requestCount++;
				}
			}
		}
		else {
			i++; // Skip one shape point.
		}
	}
}

// <LegoRR.exe @00408fd0>
bool32 __cdecl LegoRR::Construction_Zone_ExistsAtBlock(const Point2I* originBlockPos)
{
	Construction_Zone* construct = Construction_Zone_FindByHandleOrAtBlock(originBlockPos, nullptr);
	return (construct != nullptr);
}

// <LegoRR.exe @00408ff0>
LegoRR::Construction_Zone* __cdecl LegoRR::Construction_Zone_FindByHandleOrAtBlock(OPTIONAL const Point2I* originBlockPos, OPTIONAL const uint32* constructHandle)
{
	Construction_Zone* construct = constructionGlobs.constructList;
	for (; construct != nullptr; construct = construct->next) {

		if (originBlockPos != nullptr) {
			if (originBlockPos->x == construct->originBlockPos.x &&
				originBlockPos->y == construct->originBlockPos.y)
			{
				return construct;
			}
		}
		else if (constructHandle != nullptr) {
			if (*constructHandle == construct->handleValue) {
				return construct;
			}
		}
	}
	return nullptr;
}

/// CUSTOM: Alternate version of Construction_Zone_FindByHandleOrAtBlock that checks all blocks that are part of a construction zone.
LegoRR::Construction_Zone* LegoRR::Construction_Zone_FindAtOccupiedBlock(const Point2I* blockPos, bool solidOnly)
{
	Construction_Zone* construct = constructionGlobs.constructList;
	for (; construct != nullptr; construct = construct->next) {

		if (Construction_Zone_OccupiesBlock(construct, blockPos, solidOnly))
			return construct;
	}
	return nullptr;
}

/// CUSTOM: Helper method to determine if the specified block is within a construction zone's shape.
bool LegoRR::Construction_Zone_OccupiesBlock(Construction_Zone* construct, const Point2I* blockPos, bool solidOnly)
{
	for (uint32 i = 0; i < construct->shapeCount; i++) {
		const Point2I shapeBlock = construct->shapeBlocks[i];
		const bool matches = (blockPos->x == shapeBlock.x && blockPos->y == shapeBlock.y);

		if ((i != (construct->shapeCount - 1)) &&
			(shapeBlock.x == construct->shapeBlocks[i + 1].x) &&
			(shapeBlock.y == construct->shapeBlocks[i + 1].y))
		{
			// Should we return true on path tiles?
			if (!solidOnly && matches) {
				return true;
			}
			i++; // Skip next shape point.
		}
		else {
			// This is a solid tile.
			if (matches) {
				return true;
			}
		}
	}
	return false;
}

// <LegoRR.exe @00409040>
void __cdecl LegoRR::Construction_Zone_CompletePath(const Point2I* originBlockPos)
{
	Construction_Zone* construct = Construction_Zone_FindByHandleOrAtBlock(originBlockPos, nullptr);
	if (construct != nullptr) {
		Interface_BackToMain_IfSelectedGroundOrConstruction_IsBlockPos(originBlockPos);
		Construction_Zone_ConsumePlacedResources(construct);
		Construction_Zone_Free(construct);
	}
}

// <LegoRR.exe @00409080>
void __cdecl LegoRR::Construction_Zone_CancelPath(const Point2I* originBlockPos)
{
	Construction_Zone* construct = Construction_Zone_FindByHandleOrAtBlock(originBlockPos, nullptr);
	/// SANITY: Null-check construct
	if (construct != nullptr) {
		Level_Block_SetLayedPath(originBlockPos, false);

		// Unreserve resource objects and remove barriers.
		for (uint32 i = 0; i < construct->placedCount2; i++) {
			LegoObject* placedObj = construct->placedObjects[i];

			/// CHANGE: Add handling for barriers for if someone wants to be evil and require barriers for paths.
			if (placedObj->type == LegoObject_Barrier) {
				placedObj->flags3 |= LIVEOBJ3_REMOVING;
				placedObj->health = -1.0f;
			}
			else {
				placedObj->flags4 &= ~LIVEOBJ4_USEDBYCONSTRUCTION;
				AITask_DoCollect(placedObj, 0.0f);
			}
		}

		Interface_BackToMain_IfSelectedGroundOrConstruction_IsBlockPos(originBlockPos);
		AITask_Construction_FUN_00403630(construct->handleValue, false, LegoObject_None, (LegoObject_ID)0);
		Construction_Zone_Free(construct);
	}
}

// <LegoRR.exe @00409110>
void __cdecl LegoRR::Construction_UpdateAll(real32 elapsedGame)
{
	Construction_Zone* next = nullptr;
	Construction_Zone* construct = constructionGlobs.constructList;
	for (; construct != nullptr; construct = next) {
		next = construct->next; // Store next here in-case we complete the construction during the loop.

		const Construction_ZoneFlags cflags = construct->flags;
		if ((cflags & CONSTRUCTION_FLAG_LAYPATH) && !(cflags & CONSTRUCTION_FLAG_REQUESTED)) {

			Construction_Zone_RequestPathResources(construct);
		}
		else if (cflags & CONSTRUCTION_FLAG_READY) {

			if (cflags & CONSTRUCTION_FLAG_LAYPATH) {
				// Ready a path construction zone.
				// Tell a unit to go and build the path, now that all resources have been placed.
				AITask_DoBuildPath_AtPosition(&construct->originBlockPos);
				// From here on out its the AI's job to handle completing the path.
				construct->flags &= ~CONSTRUCTION_FLAG_READY;
			}
			else if (Construction_Zone_NoForeignObjectsInside(construct)) {
				// Complete a building construction zone.
				Construction_Zone_ConsumePlacedResources(construct);
				Construction_Zone_CompleteBuilding(construct, true);
				Construction_Zone_Free(construct);
			}
		}
	}
}

// <LegoRR.exe @004091a0>
bool32 __cdecl LegoRR::Construction_Zone_NoForeignObjectsInside(Construction_Zone* construct)
{
	for (auto obj : objectListSet.EnumerateSkipUpgradeParts()) {
		if (Construction_Zone_ObjectCallback_IsForeignObjectInside(obj, construct))
			return false;
	}
	return true;
	//return !LegoObject_RunThroughListsSkipUpgradeParts(Construction_Zone_ObjectCallback_IsForeignObjectInside, construct);
}

// <LegoRR.exe @004091c0>
bool32 __cdecl LegoRR::Construction_Zone_ObjectCallback_IsForeignObjectInside(LegoObject* liveObj, void* pConstruct)
{
	Construction_Zone* construct = (Construction_Zone*)pConstruct;

	Point2I blockPos = { 0, 0 }; // dummy init
	LegoObject_GetBlockPos(liveObj, &blockPos.x, &blockPos.y);

	// Obstruction only happens in the origin block???
	if (blockPos.x == construct->originBlockPos.x && blockPos.y == construct->originBlockPos.y) {
		for (uint32 i = 0; i < construct->placedCount2; i++) {
			if (liveObj == construct->placedObjects[i]) {
				// Object is tracked by construction, so it must not be a foreign object.
				return false;
			}
		}
		// Object is not tracked by construction, so it must be a foreign object.
		return true;
	}
	return false;
}

// <LegoRR.exe @00409230>
bool32 __cdecl LegoRR::Construction_Zone_StartPath(const Point2I* originBlockPos)
{
	if (!Level_Block_IsPath(originBlockPos)) {
		Construction_Zone* construct = Construction_Zone_Create(originBlockPos);
		Level_Block_SetLayedPath(originBlockPos, true);
		construct->flags |= CONSTRUCTION_FLAG_LAYPATH;
		Interface_BackToMain_IfSelectedGroundOrConstruction_IsBlockPos(originBlockPos);
		return true;
	}
	return false;
}

// <LegoRR.exe @00409280>
LegoRR::Construction_Zone* __cdecl LegoRR::Construction_Zone_Create(const Point2I* originBlockPos)
{
	Construction_Zone* construct = (Construction_Zone*)Gods98::Mem_Alloc(sizeof(Construction_Zone));
	std::memset(construct, 0, sizeof(Construction_Zone));
	
	construct->placedCount = 0;
	construct->requestCount = 0;
	construct->originBlockPos = *originBlockPos;

	construct->handleValue = constructionGlobs.nextHandleValue;
	constructionGlobs.nextHandleValue++;

	construct->next = constructionGlobs.constructList;
	constructionGlobs.constructList = construct;

	return construct;
}

// <LegoRR.exe @004092e0>
void __cdecl LegoRR::Construction_PowerGrid_PowerAdjacentBlocks(const Point2I* blockPos)
{
	const Point2I DIRECTIONS[DIRECTION__COUNT] = {
		{  0, -1 },
		{  1,  0 },
		{  0,  1 },
		{ -1,  0 },
	};

	std::queue<Point2I> blocks;
	blocks.push(*blockPos);

	while (!blocks.empty()) {
		const Point2I block = blocks.front();
		blocks.pop();

		for (uint32 dir = 0; dir < DIRECTION__COUNT; dir++) {
			const Point2I blockOffPos = {
				block.x + DIRECTIONS[dir].x,
				block.y + DIRECTIONS[dir].y,
			};

			/// SANITY: Bounds check blocks.
			if (!blockInBounds(Lego_GetLevel(), blockOffPos.x, blockOffPos.y))
				continue;

			if (Level_Block_IsPath(&blockOffPos) || Level_Block_IsSolidBuilding(blockOffPos.x, blockOffPos.y, true)) {

				if (!Level_Block_IsPowered(&blockOffPos)) {
					// Add to list, and mark as visited, so that we don't get stuck in an infinite loop.
					Level_PowerGrid_AddPoweredBlock(&blockOffPos);
					
					// Handle this block next.
					blocks.push(blockOffPos);
				}
			}
		}
	}
}

// Returns false if a building cannot be powered.
// <LegoRR.exe @00409380>
bool32 __cdecl LegoRR::Construction_PowerGrid_DrainAdjacentBlocks(const Point2I* blockPos, sint32 crystalDrainedAmount)
{
	const bool32 result = Construction_PowerGrid_DrainAdjacentBlocks_Recurse(blockPos, crystalDrainedAmount);
	Level_PowerGrid_ClearDrainPowerBlocks();
	return result;
}

// <LegoRR.exe @004093a0>
bool32 __cdecl LegoRR::Construction_PowerGrid_DrainAdjacentBlocks_Recurse(const Point2I* blockPos, sint32 crystalDrainedAmount)
{
	const Point2I DIRECTIONS[DIRECTION__COUNT] = {
		{  0, -1 },
		{  1,  0 },
		{  0,  1 },
		{ -1,  0 },
	};

	std::queue<Point2I> blocks;
	blocks.push(*blockPos);

	while (!blocks.empty()) {
		const Point2I block = blocks.front();
		blocks.pop();

		for (uint32 dir = 0; dir < DIRECTION__COUNT; dir++) {
			const Point2I blockOffPos = {
				block.x + DIRECTIONS[dir].x,
				block.y + DIRECTIONS[dir].y,
			};

			/// SANITY: Bounds check blocks.
			if (!blockInBounds(Lego_GetLevel(), blockOffPos.x, blockOffPos.y))
				continue;

			// This condition does not check the temp drainpower flag, is it possible for this to be
			//  hit more than once for the same tile?
			if (Level_Block_IsGeneratePower(&blockOffPos)) {

				LegoObject* liveObj = LegoObject_FindPoweredBuildingAtBlockPos(&blockOffPos);
				if (liveObj != nullptr) {

					if (LegoObject_AddThisDrainedCrystals(liveObj, crystalDrainedAmount)) {
						return true;
					}
				}
			}

			if (Level_Block_IsPath(&blockOffPos) || Level_Block_IsSolidBuilding(blockOffPos.x, blockOffPos.y, true)) {

				if (!Level_PowerGrid_IsDrainPowerBlock(&blockOffPos)) {
					// Add to list, and mark as visited, so that we don't get stuck in an infinite loop.
					Level_PowerGrid_AddDrainPowerBlock(&blockOffPos);

					// Handle this block next.
					blocks.push(blockOffPos);
				}
			}
		}
	}
	return false;
}

// Internally used for Paths during `Construction_UpdateAll`. Does NOT use objID field.
// <LegoRR.exe @00409480>
void __cdecl LegoRR::Construction_Zone_RequestPathResources(Construction_Zone* construct)
{
	if (Level_IsBuildPathBoolUnk_true(&construct->originBlockPos)) {
		const LegoObject_ID pathObjID = (LegoObject_ID)0;

		// Request Crystals:
		const uint32 crystalsCost = Stats_GetCostCrystal(LegoObject_Path, pathObjID, 0);
		Construction_Zone_RequestResource(&construct->originBlockPos, LegoObject_PowerCrystal, pathObjID, 0, crystalsCost);

		// Request Ore / Studs:
		// Studs will only be used if a processing building is available, and if the studs cost is non-zero.
		const LegoObject* oreProcessingObj = LegoObject_FindResourceProcessingBuilding(nullptr, &construct->originBlockPos, LegoObject_Ore, 0);
		const uint32 oreCost = Stats_GetCostOre(LegoObject_Path, pathObjID, 0);
		const uint32 studsCost = Stats_GetCostRefinedOre(LegoObject_Path, pathObjID, 0);

		LegoObject_ID oreObjID;
		uint32 oreTypeCost;
		if (oreProcessingObj != nullptr && studsCost > 0) {
			// An ore processing building exists, request studs instead of ore.
			oreObjID    = LegoObject_ID_ProcessedOre;
			oreTypeCost = studsCost;
		}
		else { // Otherwise, fallback to regular ore.
			oreObjID    = LegoObject_ID_Ore;
			oreTypeCost = oreCost;
		}
		Construction_Zone_RequestResource(&construct->originBlockPos, LegoObject_Ore, oreObjID, 0, oreTypeCost);

		construct->flags |= CONSTRUCTION_FLAG_REQUESTED;

		// Track this info to properly handle Construction_Zone_NeedsMoreOfResource.
		Construction_Zone_SetNoBuildCost(construct->handleValue, Cheat_IsNoBuildCosts());

		/// FIX APPLY: Allow paths to be constructed with zero resources.
		/*if (crystalsCost == 0 && oreTypeCost == 0) {
			// Request units to go and build the path.
			construct->flags |= CONSTRUCTION_FLAG_READY;
		}*/
		// Dummy call to check condition of having all resources (needed for paths with removed resource requirements).
		Construction_Zone_PlaceResource(construct->handleValue, nullptr);
	}
}

// <LegoRR.exe @00409530>
void __cdecl LegoRR::Construction_Zone_CancelBuilding(const Point2I* occupiedBlockPos)
{
	Lego_Level* level = Lego_GetLevel();
	/// FIX APPLY: Allow cancelling construction when selecting a tile other than the origin.
	Construction_Zone* construct = Construction_Zone_FindAtOccupiedBlock(occupiedBlockPos, false);

	if (construct != nullptr && (construct->requestCount != construct->placedCount)) {
		// Restore blocks.
		for (uint32 i = 0; i < construct->shapeCount; i++) {
			const Point2I shapeBlock = construct->shapeBlocks[i];

			if ((i != (construct->shapeCount - 1)) &&
				(shapeBlock.x == construct->shapeBlocks[i + 1].x) &&
				(shapeBlock.y == construct->shapeBlocks[i + 1].y))
			{
				blockValue(level, shapeBlock.x, shapeBlock.y).flags1 &= ~BLOCK1_BUILDINGPATH;
				/// TODO: We should be able to skip the next shape point here.
				///       Like is done in Construction_Zone_StartBuilding.
				//i++; // Skip next shape point.
			}
			blockValue(level, shapeBlock.x, shapeBlock.y).flags1 &= ~BLOCK1_FOUNDATION;
			blockValue(level, shapeBlock.x, shapeBlock.y).construct = nullptr;

			Level_BlockUpdateSurface(level, shapeBlock.x, shapeBlock.y, 0);
			Interface_BackToMain_IfSelectedGroundOrConstruction_IsBlockPos(&shapeBlock);
		}

		// Unreserve resource objects and remove barriers.
		for (uint32 i = 0; i < construct->placedCount2; i++) {
			LegoObject* placedObj = construct->placedObjects[i];

			if (placedObj->type == LegoObject_Barrier) {
				placedObj->flags3 |= LIVEOBJ3_REMOVING;
				placedObj->health = -1.0f;
			}
			else {
				placedObj->flags4 &= ~LIVEOBJ4_USEDBYCONSTRUCTION;
				AITask_DoCollect(placedObj, 0.0f);
			}
		}

		AITask_Construction_FUN_00403630(construct->handleValue, false, LegoObject_None, (LegoObject_ID)0);
		Construction_Zone_Free(construct);
	}
}

// <LegoRR.exe @004096c0>
uint32 __cdecl LegoRR::Construction_Zone_StartBuilding(LegoObject_ID buildingObjID, const Point2I* originBlockPos, Direction direction, const Point2I* shapeBlocks, uint32 count)
{
	Lego_Level* level = Lego_GetLevel();
	Construction_Zone* construct = Construction_Zone_Create(originBlockPos);

	construct->objID = buildingObjID;
	construct->direction = direction;

	construct->shapeBlocks = (Point2I*)Gods98::Mem_Alloc(count * sizeof(Point2I));
	std::memcpy(construct->shapeBlocks, shapeBlocks, (count * sizeof(Point2I)));
	construct->shapeCount = count;

	const real32 mult_4_0 = 4.0f;
	Map3D_FlattenShapeVertices(Lego_GetMap(), construct->shapeBlocks, construct->shapeCount, mult_4_0);

	for (uint32 i = 0; i < construct->shapeCount; i++) {
		const Point2I shapeBlock = construct->shapeBlocks[i];

		//bool skipPoint = false;
		if ((i != (construct->shapeCount - 1)) &&
			(shapeBlock.x == construct->shapeBlocks[i + 1].x) &&
			(shapeBlock.y == construct->shapeBlocks[i + 1].y))
		{
			//skipPoint = true;
			blockValue(level, shapeBlock.x, shapeBlock.y).flags1 |= BLOCK1_BUILDINGPATH;
			i++; // Skip the next shape point.
		}
		blockValue(level, shapeBlock.x, shapeBlock.y).flags1 |= BLOCK1_FOUNDATION;
		blockValue(level, shapeBlock.x, shapeBlock.y).construct = construct;

		Level_BlockUpdateSurface(level, shapeBlock.x, shapeBlock.y, 0);
		Construction_FlattenGround(&shapeBlock);
		Interface_BackToMain_IfSelectedGroundOrConstruction_IsBlockPos(&shapeBlock);

		//if (skipPoint) {
		//	i++;
		//}
	}

	return construct->handleValue;
}

// <LegoRR.exe @00409870>
void __cdecl LegoRR::Construction_FlattenGround(const Point2I* blockPos)
{
	const Point2I ANGLES[8] = {
		{ -1, -1 },
		{  0, -1 },
		{  1, -1 },
		{  1,  0 },
		{  1,  1 },
		{  0,  1 },
		{ -1,  1 },
		{ -1,  0 },
	};

	for (uint32 i = 0; i < _countof(ANGLES); i++) {
		// I doubt this is just lowering the roof...
		Level_Block_LowerRoofVertices(Lego_GetLevel(),
									  blockPos->x + ANGLES[i].x,
									  blockPos->y + ANGLES[i].y);
	}
}

// <LegoRR.exe @00409900>
void __cdecl LegoRR::Construction_RemoveAll(void)
{
	Construction_Zone* next = nullptr;
	Construction_Zone* construct = constructionGlobs.constructList;
	for (; construct != nullptr; construct = next) {
		next = construct->next; // Store next here since we're freeing the construction zone.

		Construction_Zone_Free(construct);
	}
}

// <LegoRR.exe @00409920>
void __cdecl LegoRR::Construction_Zone_Free(Construction_Zone* construct)
{
	Construction_Zone* previous = nullptr;
	Construction_Zone* current = constructionGlobs.constructList;
	for (; current != nullptr; current = current->next) {

		if (current == construct) {
			// Cut this item out of the linked list.
			if (previous == nullptr) {
				constructionGlobs.constructList = current->next;
			}
			else {
				previous->next = current->next;
			}
			break;
		}

		previous = current;
	}

	Gods98::Mem_Free(construct->shapeBlocks);
	Gods98::Mem_Free(construct);
}

// <LegoRR.exe @00409970>
void __cdecl LegoRR::Construction_Zone_ConsumePlacedResources(Construction_Zone* construct)
{
	for (uint32 i = 0; i < construct->placedCount2; i++) {
		construct->placedObjects[i]->health = -1.0f;
		construct->placedObjects[i]->flags3 |= LIVEOBJ3_REMOVING;
	}
}

// <LegoRR.exe @004099c0>
LegoRR::LegoObject* __cdecl LegoRR::Construction_SpawnBuilding(LegoObject_ID objID, const Point2I* originBlockPos, Direction direction, const Point2I* shapeBlocks, uint32 shapeCount, bool32 teleportDown)
{
	Construction_Zone tempConstruct;
	std::memset(&tempConstruct, 0, sizeof(Construction_Zone));

	tempConstruct.objID = objID;
	tempConstruct.originBlockPos = *originBlockPos;
	tempConstruct.direction = direction;
	
	/// CHANGE: Be safe and allocate copy of shapeBlocks because this function isn't used very often.
	tempConstruct.shapeBlocks = (Point2I*)Gods98::Mem_Alloc(shapeCount * sizeof(Point2I));
	tempConstruct.shapeCount = shapeCount;
	std::memcpy(tempConstruct.shapeBlocks, shapeBlocks, (shapeCount * sizeof(Point2I)));
	// This is only safe because Construction_Zone_CompleteBuilding doesn't free shapeBlocks.
	//tempConstruct.shapeBlocks = const_cast<Point2I*>(shapeBlocks);
	//tempConstruct.shapeCount = shapeCount;

	const real32 mult_4_0 = 4.0f;
	Map3D_FlattenShapeVertices(Lego_GetMap(), shapeBlocks, shapeCount, mult_4_0);
	Construction_FlattenGround(originBlockPos);

	LegoObject* liveObj = Construction_Zone_CompleteBuilding(&tempConstruct, teleportDown);

	/// CHANGE: Free newly allocated shapeBlocks.
	Gods98::Mem_Free(tempConstruct.shapeBlocks);

	return liveObj;
}

// <LegoRR.exe @00409a60>
LegoRR::LegoObject* __cdecl LegoRR::Construction_Zone_CompleteBuilding(Construction_Zone* construct, bool32 teleportDown)
{
	Point2F wPos = { 0.0f, 0.0f }; // dummy init actually present in LRR.
	Map3D_BlockToWorldPos(Lego_GetMap(), (uint32)construct->originBlockPos.x, (uint32)construct->originBlockPos.y,
						  &wPos.x, &wPos.y);

	const real32 angle = (real32)construct->direction * (M_PI / 2.0f);
	LegoObject* liveObj = LegoObject_CreateInWorld(&legoGlobs.buildingData[construct->objID],
												   LegoObject_Building, construct->objID, 0,
												   wPos.x, wPos.y, angle);
	if (teleportDown) {
		LegoObject_TeleportDownBuilding(liveObj);
		Info_Send(Info_Constructed, nullptr, liveObj, nullptr);
	}

	for (uint32 i = 0; i < construct->shapeCount; i++) {
		const Point2I shapeBlock = construct->shapeBlocks[i];

		if ((i != (construct->shapeCount - 1)) &&
			(shapeBlock.x == construct->shapeBlocks[i + 1].x) &&
			(shapeBlock.y == construct->shapeBlocks[i + 1].y))
		{
			Level_Block_SetPathBuilding(shapeBlock.x, shapeBlock.y);
			// Register water entrances if this building has any.
			LegoObject_TryDock_AtBlockPos_FUN_004403f0(liveObj, &shapeBlock);
			i++; // Skip the next shape point.
		}
		else {
			if (StatsObject_GetStatsFlags1(liveObj) & STATS1_TOOLSTORE) {
				/// TODO: Should we be deselecting this tile in the interface??
				///       Is this logic expecting the tool store to always be an instant-build?
				Level_Block_SetToolStoreBuilding(&shapeBlock);
			}
			else {
				Interface_BackToMain_IfSelectedGroundOrConstruction_IsBlockPos(&shapeBlock);
				Level_Block_SetSolidBuilding(shapeBlock.x, shapeBlock.y);
				if (StatsObject_GetStatsFlags2(liveObj) & STATS2_GENERATEPOWER) {
					Level_Block_SetGeneratePower(&shapeBlock);
				}
				Construction_DeselectAdjacentWalls(&shapeBlock);
			}
			ElectricFence_UpdateConnectionEStuds(shapeBlock.x, shapeBlock.y);
		}
	}

	return liveObj;
}

// <LegoRR.exe @00409c00>
void __cdecl LegoRR::Construction_DeselectAdjacentWalls(const Point2I* blockPos)
{
	const Point2I DIRECTIONS[DIRECTION__COUNT] = {
		{  0, -1 },
		{  1,  0 },
		{  0,  1 },
		{ -1,  0 },
	};

	for (uint32 i = 0; i < _countof(DIRECTIONS); i++) {
		const Point2I blockOffPos = {
			blockPos->x + DIRECTIONS[i].x,
			blockPos->y + DIRECTIONS[i].y,
		};
		AITask_RemoveWallReferences(&blockOffPos, false);
	}
}

// <LegoRR.exe @00409c70>
void __cdecl LegoRR::Construction_DisableCryOreDrop(bool32 disabled)
{
	constructionGlobs.disableCryOreDrop = disabled;
}

// <LegoRR.exe @00409c80>
void __cdecl LegoRR::Construction_CleanupBuildingFoundation(LegoObject* liveObj)
{
	/// REMOVE: Unused output.
	//Point2F wPos = { 0.0f, 0.0f }; // dummy init
	//LegoObject_GetPosition(liveObj, &wPos.x, &wPos.y);

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

	Point2I blockPos = { 0, 0 }; // dummy init
	LegoObject_GetBlockPos(liveObj, &blockPos.x, &blockPos.y);

	uint32 shapeCount;
	const Point2I* shapePoints = Building_GetShapePoints(&legoGlobs.buildingData[liveObj->id], &shapeCount);
	const Point2I* shapeBlocks = SelectPlace_TransformShapePoints(&blockPos, shapePoints, shapeCount, direction);

	for (uint32 i = 0; i < shapeCount; i++) {
		const Point2I shapeBlock = shapeBlocks[i];

		//bool skipPoint = false;
		if ((i != (shapeCount - 1)) &&
			(shapeBlock.x == shapeBlocks[i + 1].x) &&
			(shapeBlock.y == shapeBlocks[i + 1].y))
		{
			//skipPoint = true;
			i++; // Skip the next shape point.
		}

		Level_Block_UnsetBuildingTile(&shapeBlock);
		Level_Block_UnsetGeneratePower(&shapeBlock);

		if (!(liveObj->flags4 & LIVEOBJ4_UNK_8)) {
			Vector3F effectPos = { 0.0f, 0.0f, 0.0f }; // dummy init
			Map3D_BlockToWorldPos(Lego_GetMap(), shapeBlock.x, shapeBlock.y, &effectPos.x, &effectPos.y);
			effectPos.z = Map3D_GetWorldZ(Lego_GetMap(), effectPos.x, effectPos.y);

			/// FIX APPLY: Ensure we have a non-zero vector after removing the z component.
			///            In the future Maths_Vector2DRandom should be rewritten to handle
			///             this just like Maths_Vector3DRandom.
			Vector3F effectDir = { 0.0f, 0.0f, 0.0f }; // dummy init
			do {
				Gods98::Maths_Vector3DRandom(&effectDir);
				effectDir.z = 0.0f;
			} while (effectDir.x == 0.0f && effectDir.y == 0.0f);
			/// TODO: Should we be normalizing the direction after setting z to 0.0f?
			//Gods98::Maths_Vector3DNormalize(&effectDir);

			Effect_Spawn_Particle(MISCOBJECT_PATHDUST, &effectPos, &effectDir);
			// Creates rubble on the block and assigns a task to clean it up.
			AITask_DoClearTypeAction(&shapeBlock, Message_ClearBuildingComplete);
		}

		ElectricFence_UpdateConnectionEStuds(shapeBlock.x, shapeBlock.y);

		//if (skipPoint) {
		//	i++;
		//}
	}

	Construction_GenerateCryOreDrop(liveObj);
}

// <LegoRR.exe @00409e50>
void __cdecl LegoRR::Construction_GenerateCryOreDrop(LegoObject* liveObj)
{
	if (!(liveObj->flags4 & LIVEOBJ4_CRYORECOSTDROPPED)) {

		if (!constructionGlobs.disableCryOreDrop) {
			/// REFACTOR: Move getters to inside !disableCryOreDrop block.
			Point2I blockPos = { 0, 0 }; // dummy init
			LegoObject_GetBlockPos(liveObj, &blockPos.x, &blockPos.y);

			const uint32 crystalsCost = Stats_GetCostCrystal(LegoObject_Building, liveObj->id, liveObj->objLevel);
			const uint32 oreCost = Stats_GetCostOre(LegoObject_Building, liveObj->id, liveObj->objLevel);
			/// REMOVE: Unused return value.
			//const uint32 studsCost = Stats_GetCostRefinedOre(LegoObject_Building, liveObj->id, liveObj->objLevel);

			for (uint32 i = 0; i < crystalsCost; i++) {
				Level_GenerateCrystal(&blockPos, 0, nullptr, false);
			}
			for (uint32 i = 0; i < oreCost; i++) {
				Level_GenerateOre(&blockPos, 0, nullptr, false);
			}
		}
		liveObj->flags4 |= LIVEOBJ4_CRYORECOSTDROPPED;
	}
}

// <LegoRR.exe @00409f20>
void __cdecl LegoRR::Construction_RemoveBuildingObject(LegoObject* liveObj)
{
	// See if this building is registered as the primary teleporter for any vehicle types.
	/// TODO: For sanity should we skip checking flags and only check the object directly?
	///       If the flags are wrong and this is the primary teleporter, then bad things
	///        could happen.
	const bool smallTeleporter = ((StatsObject_GetStatsFlags1(liveObj) & STATS1_SMALLTELEPORTER) &&
								  (legoGlobs.placeDestSmallTeleporter == liveObj));

	const bool bigTeleporter = ((StatsObject_GetStatsFlags1(liveObj) & STATS1_BIGTELEPORTER) &&
								(legoGlobs.placeDestBigTeleporter == liveObj));

	const bool waterTeleporter = ((StatsObject_GetStatsFlags1(liveObj) & STATS1_WATERTELEPORTER) &&
								  (legoGlobs.placeDestWaterTeleporter == liveObj));

	// It's expected that this function is called when health is < 0.0f.
	// Otherwise the FindTeleporter functions may return this building again
	//  due to the LegoObject_IsActive check.

	Point2F wPos = { 0.0f, 0.0f }; // dummy init
	LegoObject_GetPosition(liveObj, &wPos.x, &wPos.y);

	// Replace references to this building as the primary teleporter.
	if (smallTeleporter) {
		legoGlobs.placeDestSmallTeleporter = LegoObject_FindSmallTeleporter(&wPos);
	}
	if (bigTeleporter) {
		legoGlobs.placeDestBigTeleporter = LegoObject_FindBigTeleporter(&wPos);
	}
	if (waterTeleporter) {
		legoGlobs.placeDestWaterTeleporter = LegoObject_FindWaterTeleporter(&wPos);
	}

	LegoObject_Remove(liveObj);

	LegoObject_RequestPowerGridUpdate();
}

#pragma endregion
