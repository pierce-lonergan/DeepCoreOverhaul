// Construction.cpp : 
//

#include "../../engine/core/Utils.h"

#include "../object/AITask.h"
#include "../object/Object.h"
#include "../object/Stats.h"
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


// <LegoRR.exe @004092e0>
void __cdecl LegoRR::Construction_PowerGrid_PowerAdjacentBlocks(const Point2I* blockPos)
{
	const Point2I DIRECTIONS[DIRECTION__COUNT] = {
		{  0, -1 },
		{  1,  0 },
		{  0,  1 },
		{ -1,  0 },
	};

	for (uint32 dir = 0; dir < DIRECTION__COUNT; dir++) {
		const Point2I blockOffPos = {
			blockPos->x + DIRECTIONS[dir].x,
			blockPos->y + DIRECTIONS[dir].y,
		};

		if (Level_Block_IsPath(&blockOffPos) || Level_Block_IsSolidBuilding(blockOffPos.x, blockOffPos.y, true)) {

			if (!Level_Block_IsPowered(&blockOffPos)) {
				// Add to list, and mark as visited, so that we don't get stuck in an infinite loop.
				Level_PowerGrid_AddPoweredBlock(&blockOffPos);
				/// RECURSION:
				Construction_PowerGrid_PowerAdjacentBlocks(&blockOffPos);
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

	for (uint32 dir = 0; dir < DIRECTION__COUNT; dir++) {
		const Point2I blockOffPos = {
			blockPos->x + DIRECTIONS[dir].x,
			blockPos->y + DIRECTIONS[dir].y,
		};

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

				/// RECURSION:
				if (Construction_PowerGrid_DrainAdjacentBlocks_Recurse(&blockOffPos, crystalDrainedAmount)) {
					return true;
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


#pragma endregion
