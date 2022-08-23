// Object.cpp : 
//

#include "../Game.h"
#include "../audio/SFX.h"
#include "../interface/hud/Bubbles.h"
#include "../interface/Encyclopedia.h"
#include "../interface/InfoMessages.h"
#include "../mission/Messages.h"
#include "AITask.h"
#include "Stats.h"
#include "Object.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004df790>
LegoRR::LegoObject_Globs & LegoRR::objectGlobs = *(LegoRR::LegoObject_Globs*)0x004df790;

LegoRR::LegoObject_ListSet LegoRR::objectListSet = LegoRR::LegoObject_ListSet(LegoRR::objectGlobs);

#pragma endregion

/**********************************************************************************
 ******** Class Functions
 **********************************************************************************/

#pragma region Class Functions

bool LegoRR::LegoObject_ListSet::FilterSkipUpgradeParts(const LegoObject* liveObj)
{
	return ListSet::IsAlive(liveObj) && !(liveObj->flags3 & LIVEOBJ3_UPGRADEPART);
}

LegoRR::LegoObject_ListSet::enumerable<LegoRR::LegoObject_ListSet::FilterSkipUpgradeParts> LegoRR::LegoObject_ListSet::EnumerateSkipUpgradeParts()
{
	return this->EnumerateWhere<FilterSkipUpgradeParts>();
}

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @00436ee0>
void __cdecl LegoRR::LegoObject_Initialise(void)
{
	std::memset(&objectGlobs, 0, 0xc644); // Exclude end of struct holding non-glob fields. // sizeof(objectGlobs));

	Activity_RegisterName(Activity_Stand);
	Activity_RegisterName(Activity_Route);
	Activity_RegisterName(Activity_RouteRubble);
	Activity_RegisterName(Activity_RunPanic);
	Activity_RegisterName(Activity_Drill);
	Activity_RegisterName(Activity_Teleport);
	Activity_RegisterName(Activity_Walk);
	Activity_RegisterName(Activity_Reinforce);
	Activity_RegisterName(Activity_Reverse);
	Activity_RegisterName(Activity_TurnLeft);
	Activity_RegisterName(Activity_TurnRight);
	Activity_RegisterName(Activity_CantDo);
	Activity_RegisterName(Activity_Emerge);
	Activity_RegisterName(Activity_Enter);
	Activity_RegisterName(Activity_EnterRein);
	Activity_RegisterName(Activity_Collect);
	Activity_RegisterName(Activity_Gather);
	Activity_RegisterName(Activity_Carry);
	Activity_RegisterName(Activity_CarryRubble);
	Activity_RegisterName(Activity_Throw);
	Activity_RegisterName(Activity_CarryTurnLeft);
	Activity_RegisterName(Activity_CarryTurnRight);
	Activity_RegisterName(Activity_CarryStand);
	Activity_RegisterName(Activity_HitLeft);
	Activity_RegisterName(Activity_HitRight);
	Activity_RegisterName(Activity_HitFront);
	Activity_RegisterName(Activity_HitBack);
	Activity_RegisterName(Activity_HitHard);
	Activity_RegisterName(Activity_Dynamite);
	Activity_RegisterName(Activity_Deposit);
	Activity_RegisterName(Activity_Clear);
	Activity_RegisterName(Activity_Place);
	Activity_RegisterName(Activity_Repair);
	Activity_RegisterName(Activity_Slip);
	Activity_RegisterName(Activity_Rest);
	Activity_RegisterName(Activity_Eat);
	Activity_RegisterName(Activity_Stamp);
	Activity_RegisterName(Activity_ThrowMan);
	Activity_RegisterName(Activity_ThrownByRockMonster);
	Activity_RegisterName(Activity_GetUp);
	Activity_RegisterName(Activity_BuildPath);
	Activity_RegisterName(Activity_Upgrade);
	Activity_RegisterName(Activity_Explode);
	Activity_RegisterName(Activity_Unpowered);
	Activity_RegisterName(Activity_FireLaser);
	Activity_RegisterName(Activity_Freezed);
	Activity_RegisterName(Activity_FreezeStart);
	Activity_RegisterName(Activity_FreezeMelt);
	Activity_RegisterName(Activity_Recharge);
	Activity_RegisterName(Activity_WakeUp);
	Activity_RegisterName(Activity_Train);
	Activity_RegisterName(Activity_FloatOn);
	Activity_RegisterName(Activity_FloatOff);
	Activity_RegisterName(Activity_Opening);
	Activity_RegisterName(Activity_Closing);
	Activity_RegisterName(Activity_Open);
	Activity_RegisterName(Activity_Waiting1);
	Activity_RegisterName(Activity_Waiting2);
	Activity_RegisterName(Activity_Waiting3);
	Activity_RegisterName(Activity_Waiting4);
	Activity_RegisterName(Activity_Waiting5);
	Activity_RegisterName(Activity_Waiting6);
	Activity_RegisterName(Activity_Waiting7);
	Activity_RegisterName(Activity_Waiting8);
	Activity_RegisterName(Activity_Waiting9);
	Activity_RegisterName(Activity_Waiting10);
	Activity_RegisterName(Activity_Waiting11);
	Activity_RegisterName(Activity_Waiting12);
	Activity_RegisterName(Activity_Waiting13);
	Activity_RegisterName(Activity_Waiting14);
	Activity_RegisterName(Activity_Waiting15);
	Activity_RegisterName(Activity_Waiting16);
	Activity_RegisterName(Activity_Waiting17);
	Activity_RegisterName(Activity_Waiting18);
	Activity_RegisterName(Activity_Waiting19);
	Activity_RegisterName(Activity_Waiting20);
	Activity_RegisterName(Activity_Waiting21);
	Activity_RegisterName(Activity_Crumble);
	Activity_RegisterName(Activity_TeleportIn);

	AbilityType_RegisterName(LegoObject_AbilityType_Pilot);
	AbilityType_RegisterName(LegoObject_AbilityType_Sailor);
	AbilityType_RegisterName(LegoObject_AbilityType_Driver);
	AbilityType_RegisterName(LegoObject_AbilityType_Dynamite);
	AbilityType_RegisterName(LegoObject_AbilityType_Repair);
	AbilityType_RegisterName(LegoObject_AbilityType_Scanner);

	objectGlobs.toolNullIndex[LegoObject_ToolType_Drill]      = 0;
	objectGlobs.toolNullIndex[LegoObject_ToolType_Spade]      = 0;
	objectGlobs.toolNullIndex[LegoObject_ToolType_Hammer]     = 1;
	objectGlobs.toolNullIndex[LegoObject_ToolType_Spanner]    = 1;
	objectGlobs.toolNullIndex[LegoObject_ToolType_Laser]      = 4;
	objectGlobs.toolNullIndex[LegoObject_ToolType_PusherGun]  = 4;
	objectGlobs.toolNullIndex[LegoObject_ToolType_BirdScarer] = 4;
	objectGlobs.toolNullIndex[LegoObject_ToolType_FreezerGun] = 4;
	objectGlobs.toolNullIndex[LegoObject_ToolType_Barrier]    = 3;
	objectGlobs.toolNullIndex[LegoObject_ToolType_Dynamite]   = 2;
	objectGlobs.toolNullIndex[LegoObject_ToolType_CryOre]     = 4;

	ToolType_RegisterName(LegoObject_ToolType_Drill);
	ToolType_RegisterName(LegoObject_ToolType_Spade);
	ToolType_RegisterName(LegoObject_ToolType_Hammer);
	ToolType_RegisterName(LegoObject_ToolType_Spanner);
	ToolType_RegisterName(LegoObject_ToolType_Laser);
	ToolType_RegisterName(LegoObject_ToolType_PusherGun);
	ToolType_RegisterName(LegoObject_ToolType_BirdScarer);
	ToolType_RegisterName(LegoObject_ToolType_FreezerGun);

	/// NOTE: Special tools that DO NOT have their names registered.
	//ToolType_RegisterName(LegoObject_ToolType_Barrier);
	//ToolType_RegisterName(LegoObject_ToolType_Dynamite);
	//ToolType_RegisterName(LegoObject_ToolType_CryOre);

	objectListSet.Initialise();
	objectGlobs.flags = LegoObject_GlobFlags::OBJECT_GLOB_FLAG_INITIALISED;
}

// <LegoRR.exe @00437310>
void __cdecl LegoRR::LegoObject_Shutdown(void)
{
	LegoObject_RemoveAll();

	if (objectGlobs.UnkSurfaceGrid_1_TABLE != nullptr) {
		Gods98::Mem_Free(objectGlobs.UnkSurfaceGrid_1_TABLE);
	}

	/// REFACTOR: Moved below freeing `UnkSurfaceGrid_1_TABLE`.
	objectListSet.Shutdown();
	objectGlobs.flags = LegoObject_GlobFlags::OBJECT_GLOB_FLAG_NONE;

	// Yup, this assignment really exists after the above one (originally) next to objectListSet.Shutdown()....
	// It has no effect, so we can keep it commented out.
	//objectGlobs.flags &= ~LegoObject_GlobFlags::OBJECT_GLOB_FLAG_INITIALISED;
}


// <LegoRR.exe @00437700>
void __cdecl LegoRR::LegoObject_CleanupObjectLevels(void)
{
	std::memset(objectGlobs.objectPrevLevels, 0, sizeof(objectGlobs.objectPrevLevels));
	std::memset(objectGlobs.objectTotalLevels, 0, sizeof(objectGlobs.objectTotalLevels));
}

// <LegoRR.exe @00437720>
uint32 __cdecl LegoRR::LegoObject_GetLevelObjectsBuilt(LegoObject_Type objType, LegoObject_ID objID, uint32 objLevel, bool32 currentCount)
{
	uint32 levelsCount = objectGlobs.objectTotalLevels[objType][objID][objLevel];
	if (currentCount) {
		// Subtract objects that have been removed from the total.
		levelsCount -= objectGlobs.objectPrevLevels[objType][objID][objLevel];
	}
	return levelsCount;
}

// <LegoRR.exe @00437760>
uint32 __cdecl LegoRR::LegoObject_GetPreviousLevelObjectsBuilt(LegoObject_Type objType, LegoObject_ID objID, uint32 objLevel)
{
	return objectGlobs.objectPrevLevels[objType][objID][objLevel];
}

// <LegoRR.exe @00437790>
void __cdecl LegoRR::LegoObject_IncLevelPathsBuilt(bool32 incCurrent)
{
	if (incCurrent) {
		objectGlobs.objectTotalLevels[LegoObject_Path][0][0]++;
	}
	else {
		objectGlobs.objectPrevLevels[LegoObject_Path][0][0]++;
	}
}

// Removes all route-to references that match the specified object.
// Does nothing if routeToObj is a Boulder type.
// <LegoRR.exe @004377b0>
void __cdecl LegoRR::LegoObject_RemoveRouteToReferences(LegoObject* routeToObj)
{
	if (routeToObj->type != LegoObject_Boulder) {
		for (LegoObject* obj : objectListSet.EnumerateSkipUpgradeParts()) {
			LegoObject_Callback_RemoveRouteToReference(obj, routeToObj);
		}
		//LegoObject_RunThroughListsSkipUpgradeParts(LegoObject_Callback_RemoveRouteToReference, routeToObj);
	}
}

// Removes the route-to reference if it matches the specified object.
// DATA: LegoObject* routeToObj
// <LegoRR.exe @004377d0>
bool32 __cdecl LegoRR::LegoObject_Callback_RemoveRouteToReference(LegoObject* liveObj, void* pRouteToObj)
{
	const LegoObject* routeToObj = (LegoObject*)pRouteToObj;
	if (liveObj->routeToObject == routeToObj) {
		LegoObject_Route_End(liveObj, false);
		liveObj->routeToObject = nullptr;
	}
	return false;
}


// <LegoRR.exe @00437800>
bool32 __cdecl LegoRR::LegoObject_Remove(LegoObject* deadObj)
{
	// Increment number of this object type/id/levels that were removed from the level.
	objectGlobs.objectPrevLevels[deadObj->type][deadObj->id][deadObj->objLevel]++;

	// Stop currently-playing sound handles.
	if (deadObj->flags4 & LIVEOBJ4_DRILLSOUNDPLAYING) {
		SFX_Sound3D_StopSound(deadObj->drillSoundHandle);
	}
	if (deadObj->flags4 & LIVEOBJ4_ENGINESOUNDPLAYING) {
		SFX_Sound3D_StopSound(deadObj->engineSoundHandle);
	}

	// Clear all projectileObject fields referencing this object.
	for (LegoObject* obj : objectListSet.EnumerateSkipUpgradeParts()) {
		// Always returns false to enumerate the full list.
		Weapon_Callback_RemoveProjectileReference(obj, deadObj);
	}
	//LegoObject_RunThroughListsSkipUpgradeParts(Weapon_Callback_RemoveProjectileReference, deadObj);

	if (deadObj->type == LegoObject_RockMonster) {
		AITask_RemoveAttackRockMonsterReferences(deadObj);
	}

	if (deadObj->contMiniTeleportUp != nullptr) {
		Gods98::Container_Remove(deadObj->contMiniTeleportUp);
	}

	if (deadObj->carryingThisObject != nullptr) {
		LegoObject_DropCarriedObject(deadObj->carryingThisObject, false);
	}

	// This function has no side effects with deadObj, we can safely reorganize the LIVEOBJ4_DOCKOCCUPIED cleanup around this call.
	LegoObject_WaterVehicle_Unregister(deadObj);

	// Exit laser tracker mode.
	// Note that this mode does not treat the object as selected, so no extra handling seems to be needed.
	if (deadObj->flags4 & LIVEOBJ4_LASERTRACKERMODE) {
		legoGlobs.flags1 &= ~GAME1_LASERTRACKER;
	}

	// Clear flags that link routeToObjects to one another.
	/// TODO: Check if a single routeToObject can be referenced by multiple objects at once.
	///       If that's the case, then is clearing these flags causing problems?
	/// REFACTOR: First block moved from above `LegoObject_WaterVehicle_Unregister`.
	if (deadObj->flags4 & LIVEOBJ4_DOCKOCCUPIED) {
		deadObj->flags4 &= ~LIVEOBJ4_DOCKOCCUPIED;
		if (deadObj->routeToObject != nullptr) {
			deadObj->routeToObject->flags4 &= ~LIVEOBJ4_DOCKOCCUPIED;
		}
		/// TODO: Should routeToObject be cleared in this scenario too?
	}
	if (deadObj->flags4 & LIVEOBJ4_ENTRANCEOCCUPIED) {
		if (deadObj->routeToObject != nullptr) {
			deadObj->routeToObject->flags4 &= ~LIVEOBJ4_ENTRANCEOCCUPIED;
		}
		deadObj->routeToObject = nullptr;
	}

	LegoObject_Route_End(deadObj, false);
	LegoObject_DoBuildingsCallback_AttackByBoulder(deadObj);
	LegoObject_DropCarriedObject(deadObj, false); // false : don't put away.

	AITask_RemoveTargetObjectReferences(deadObj);

	// Clear first teleportDownObject field referencing this object.
	LegoObject_RemoveTeleportDownReference(deadObj);

	// Remove encyclopedia reference to this object.
	Encyclopedia_RemoveCurrentReference(deadObj);

	if ((StatsObject_GetStatsFlags1(deadObj) & STATS1_PROCESSCRYSTAL) ||
		(StatsObject_GetStatsFlags1(deadObj) & STATS1_PROCESSORE))
	{
		// Probably to remove static task saying: "Hey! if you need processed resources, then come on down to <buildingNameHere>!"
		AITask_LiveObject_FUN_00403940(deadObj);
	}

	// Remove object from selection.
	// Cancel first-person view if this is the objectFP.
	// And clear the camera tracker reference to this object.
	Message_RemoveObjectReference(deadObj);

	// Removes info messages referencing this object.
	Info_RemoveObjectReferences(deadObj);
	Bubble_RemoveObjectReferences(deadObj);

	// Clear routeToObject fields referencing this object.
	LegoObject_RemoveRouteToReferences(deadObj);

	Lego_RemoveRecordObject(deadObj);


	/// FIX APPLY: Remove object references created by K register/get in vehicle debug key.
	if (gamectrlGlobs.dbgGetInVehicle == deadObj) gamectrlGlobs.dbgGetInVehicle = nullptr;
	

	// Remove the model, and other object type-specific cleanup.
	switch (deadObj->type) {
	case LegoObject_Vehicle:
		Vehicle_Remove(deadObj->vehicle);
		Gods98::Mem_Free(deadObj->vehicle);
		break;

	case LegoObject_MiniFigure:
		Creature_Remove(deadObj->miniFigure);
		Gods98::Mem_Free(deadObj->miniFigure);
		break;

	case LegoObject_RockMonster:
		if (StatsObject_GetStatsFlags1(deadObj) & STATS1_FLOCKS) {
			LegoObject_FlocksDestroy(deadObj);
		}
		Creature_Remove(deadObj->rockMonster);
		Gods98::Mem_Free(deadObj->rockMonster);
		break;

	case LegoObject_Building:
		Building_Remove(deadObj->building);
		Gods98::Mem_Free(deadObj->building);
		break;

	case LegoObject_UpgradePart:
		/// REFACTOR: This case was checked after flag `LIVEOBJ3_SIMPLEOBJECT`. But these cases are mutually exclusive.
		Upgrade_Part_Remove(deadObj->upgradePart);
		Gods98::Mem_Free(deadObj->upgradePart);
		break;

	default: // Simple object types:
		if (deadObj->flags3 & LIVEOBJ3_SIMPLEOBJECT) {
			Gods98::Container_Remove(deadObj->other);
		}
		break;
	}


	objectListSet.Remove(deadObj);

	return false;
}

// <LegoRR.exe @00437a70>
bool32 __cdecl LegoRR::LegoObject_RunThroughListsSkipUpgradeParts(LegoObject_RunThroughListsCallback callback, void* data)
{
	for (LegoObject* obj : objectListSet.EnumerateSkipUpgradeParts()) {

		if (callback(obj, data))
			return true; // terminate run through listSet
	}
	return false;
	//return LegoObject_RunThroughLists(callback, search, true);
}

// <LegoRR.exe @00437a90>
bool32 __cdecl LegoRR::LegoObject_RunThroughLists(LegoObject_RunThroughListsCallback callback, void* data, bool32 skipUpgradeParts)
{
	for (LegoObject* obj : objectListSet.EnumerateAlive()) {
		// Secondary filter
		if (!skipUpgradeParts || !(obj->flags3 & LIVEOBJ3_UPGRADEPART)) {

			if (callback(obj, data))
				return true; // terminate run through listSet
		}
	}
	return false;
}


// <LegoRR.exe @00437fc0>
LegoRR::LegoObject* __cdecl LegoRR::LegoObject_Create(ObjectModel* objModel, LegoObject_Type objType, LegoObject_ID objID)
{
	LegoObject* liveObj = LegoObject_Create_internal();
	if (!liveObj) return nullptr;

	const uint32 objLevel = 0; // Keep this here if we every want to extend creating an object that isn't at level 0.

	liveObj->type = objType;
	liveObj->id = objID;
	liveObj->objLevel = objLevel; // We *would* need to assign this beforehand so that StatsObject_SetObjectLevel doesn't change the prevCount for Lv0.

	liveObj->flags3 = LIVEOBJ3_NONE;

	liveObj->health = 100.0f;
	liveObj->energy = 100.0f;

	// Disable sound temporarily to avoid anything that would play during initial activity setup(?)
	Gods98::Container_EnableSoundTriggers(false);
	const bool32 isSoundOn = SFX_IsSoundOn();
	SFX_SetSoundOn(false, false);

	switch (liveObj->type) {
	case LegoObject_Vehicle:
		{
			VehicleModel* srcVehicleModel = (VehicleModel*)objModel;

			liveObj->vehicle = (VehicleModel*)Gods98::Mem_Alloc(sizeof(VehicleModel));
			Vehicle_Clone(srcVehicleModel, liveObj->vehicle);
			Vehicle_SetOwnerObject(liveObj->vehicle, liveObj);

			liveObj->carryNullFrames = Vehicle_GetCarryNullFrames(liveObj->vehicle);

			liveObj->abilityFlags = ABILITY_FLAG_SCANNER; // Do vehicles always have a scanner by default??
			liveObj->flags3 |= (LIVEOBJ3_CANFIRSTPERSON|LIVEOBJ3_CANYESSIR|LIVEOBJ3_CANSELECT|
								LIVEOBJ3_CENTERBLOCKIDLE|LIVEOBJ3_CANDAMAGE|LIVEOBJ3_AITASK_UNK_400000);

			// Set upgrade parts used in model.
			Vehicle_SetUpgradeLevel(liveObj->vehicle, objLevel);
			// Assign normal stats.
			StatsObject_SetObjectLevel(liveObj, objLevel);

			if (srcVehicleModel->drillNullName != nullptr) {
				liveObj->flags3 |= LIVEOBJ3_CANDIG;
			}

			if (srcVehicleModel->carryNullName != nullptr) {
				liveObj->flags3 |= LIVEOBJ3_CANCARRY;

				const StatsFlags1 sflags1 = StatsObject_GetStatsFlags1(liveObj);
				if (sflags1 & STATS1_CROSSLAND) {
					liveObj->flags3 |= LIVEOBJ3_CANPICKUP;
				}
			}
		}
		break;
	case LegoObject_MiniFigure:
		{
			CreatureModel* srcCreatureModel = (CreatureModel*)objModel;

			liveObj->miniFigure = (CreatureModel*)Gods98::Mem_Alloc(sizeof(CreatureModel));
			Creature_Clone(srcCreatureModel, liveObj->miniFigure);
			Creature_SetOwnerObject(liveObj->miniFigure, liveObj);

			liveObj->carryNullFrames = Creature_GetCarryNullFrames(liveObj->miniFigure);

			liveObj->flags3 |= (LIVEOBJ3_UNK_1|LIVEOBJ3_CANREINFORCE|LIVEOBJ3_CANFIRSTPERSON|
								LIVEOBJ3_CANCARRY|LIVEOBJ3_CANPICKUP|LIVEOBJ3_CANYESSIR|
								LIVEOBJ3_CANSELECT|LIVEOBJ3_CANDAMAGE|LIVEOBJ3_AITASK_UNK_400000);

			if (srcCreatureModel->drillNullName != nullptr) {
				liveObj->flags3 |= LIVEOBJ3_CANDIG;
			}

			liveObj->flags3 |= LIVEOBJ3_CANDYNAMITE;

			// Assign normal stats.
			StatsObject_SetObjectLevel(liveObj, objLevel);

			/// FIXME: Allow modding to change which tools a unit automatically starts with.
			LegoObject_MiniFigure_EquipTool(liveObj, LegoObject_ToolType_Drill);
		}
		break;
	case LegoObject_RockMonster:
		{
			CreatureModel* srcCreatureModel = (CreatureModel*)objModel;

			liveObj->rockMonster = (CreatureModel*)Gods98::Mem_Alloc(sizeof(CreatureModel));
			Creature_Clone(srcCreatureModel, liveObj->rockMonster);
			Creature_SetOwnerObject(liveObj->rockMonster, liveObj);

			liveObj->carryNullFrames = Creature_GetCarryNullFrames(liveObj->rockMonster);

			liveObj->flags3 |= (LIVEOBJ3_UNK_200|LIVEOBJ3_CANDAMAGE|LIVEOBJ3_AITASK_UNK_400000|LIVEOBJ3_MONSTER_UNK_8000000);

			if (srcCreatureModel->drillNullName != nullptr) {
				liveObj->flags3 |= LIVEOBJ3_CANDIG;
			}

			// Assign normal stats.
			StatsObject_SetObjectLevel(liveObj, objLevel);

			const StatsFlags1 sflags1 = StatsObject_GetStatsFlags1(liveObj);
			if (!(sflags1 & STATS1_FLOCKS)) {
				liveObj->flocks = nullptr;
			}
			else {
				LegoObject_Flocks_Initialise(liveObj);
				if (!(sflags1 & STATS1_FLOCKS_DEBUG)) {
					LegoObject_Hide(liveObj, true);
					// CANSELECT flag was (definitely) present in debug builds for monsters, but it's never set in release.
					liveObj->flags3 &= ~LIVEOBJ3_CANSELECT;
				}
			}

			if (LegoObject_CanShootObject(liveObj)) {
				// Register as a shootable target(?)
				AITask_DoAttackRockMonster_Target(liveObj);
			}

			liveObj->flags3 |= LIVEOBJ3_POWEROFF;
		}
		break;
	case LegoObject_Building:
		{
			BuildingModel* srcBuildingModel = (BuildingModel*)objModel;

			liveObj->building = (BuildingModel*)Gods98::Mem_Alloc(sizeof(BuildingModel));
			Building_Clone(srcBuildingModel, liveObj->building);
			Building_SetOwnerObject(liveObj->building, liveObj);

			liveObj->carryNullFrames = Building_GetCarryNullFrames(liveObj->building);

			liveObj->flags3 |= (LIVEOBJ3_CANSELECT|LIVEOBJ3_CANDAMAGE);

			// Set upgrade parts used in model.
			Building_SetUpgradeLevel(liveObj->building, objLevel);
			// Assign normal stats.
			StatsObject_SetObjectLevel(liveObj, objLevel);

			const StatsFlags1 sflags1 = StatsObject_GetStatsFlags1(liveObj);
			// Not entirely sure, but register this new building as the primary teleporter for what it supports(??)
			if (sflags1 & STATS1_SMALLTELEPORTER) {
				legoGlobs.placeDestSmallTeleporter = liveObj;
			}
			if (sflags1 & STATS1_BIGTELEPORTER) {
				legoGlobs.placeDestBigTeleporter = liveObj;
			}
			if (sflags1 & STATS1_WATERTELEPORTER) {
				legoGlobs.placeDestWaterTeleporter = liveObj;
			}
			if (sflags1 & STATS1_SNAXULIKE) {
				// Register this location as a valid *slow* food joint for Rock Raiders to hang out and loiter at(?)
				AITask_DoGotoEat(liveObj);
			}

			// Register object as a repairable target(?)
			AITask_DoRepair(liveObj);

			liveObj->abilityFlags = ABILITY_FLAG_SCANNER; // Do buildings always have a scanner by default??

			// The power grid needs to be updated for this building's consumption/or output.
			LegoObject_RequestPowerGridUpdate();
		}
		break;
	case LegoObject_UpgradePart:
		{
			Upgrade_PartModel* srcUpgradePartModel = (Upgrade_PartModel*)objModel;

			liveObj->upgradePart = (Upgrade_PartModel*)Gods98::Mem_Alloc(sizeof(Upgrade_PartModel));
			Upgrade_Part_Clone(srcUpgradePartModel, liveObj->upgradePart);

			liveObj->flags3 |= LIVEOBJ3_UPGRADEPART; // This makes 99% of all object enumerations skip this object (see: EnumerateSkipUpgradeParts).

			// Assign special stats.
			liveObj->stats = &c_ObjectStats_Upgrade;
		}
		break;
	default:
		{
			Gods98::Container* srcContainer = (Gods98::Container*)objModel;

			liveObj->other = Gods98::Container_Clone(srcContainer);
			Gods98::Container_SetUserData(liveObj->other, liveObj);

			liveObj->flags3 |= LIVEOBJ3_SIMPLEOBJECT;

			// Assign flags depending on the object type.
			switch (liveObj->type) {
			case LegoObject_PowerCrystal:
			case LegoObject_Ore:
				liveObj->flags3 |= (LIVEOBJ3_CANSELECT|LIVEOBJ3_ALLOWCULLING_UNK);
				break;
			case LegoObject_ElectricFence:
				liveObj->flags3 |= (LIVEOBJ3_CANSELECT|LIVEOBJ3_CANDAMAGE);
				break;
			case LegoObject_Dynamite:
			case LegoObject_OohScary:
				liveObj->flags3 |= LIVEOBJ3_CANDAMAGE; // Uhhh... these can be damaged!? OR WAIT! MAYBE ITS THE COUNTDOWN NUMBERS! AHAHHHH!!!!
				break;
			}

			// Only does anything when liveObj->type == LegoObject_PowerCrystal,
			//  but leave it outside the switch/if-else check if we want to extend this.
			LegoObject_SetCrystalPoweredColour(liveObj, true);

			// Assign special or normal stats depending on the object type.
			// Note that these special stats types are all identical. They have 0x30 for the first field, and the rest is all zeroes.
			switch (liveObj->type) {
			case LegoObject_Dynamite:
				// Assign special stats.
				liveObj->stats = &c_ObjectStats_Dynamite;
				break;
			case LegoObject_Barrier:
				// Assign special stats.
				liveObj->stats = &c_ObjectStats_Barrier;
				break;
			case LegoObject_SpiderWeb:
			case LegoObject_OohScary:
			case LegoObject_ElectricFenceStud:
			case LegoObject_Pusher:
			case LegoObject_Freezer:
			case LegoObject_IceCube:
			case LegoObject_LaserShot:
				// Assign special stats.
				liveObj->stats = &c_ObjectStats_Other;
				break;
			case LegoObject_Boulder:
			case LegoObject_PowerCrystal:
			case LegoObject_Ore:
			case LegoObject_ElectricFence:
			//case LegoObject_Path: // Not a real object type
			default:
				// Assign normal stats.
				StatsObject_SetObjectLevel(liveObj, objLevel);
				break;
			}
		}
		break;
	}

	// Go through a few activities and confirm if our activity model supports them.
	// Note that activityName2 needs to be different for the UpdateAcivityChange function to succeed.
	liveObj->activityName2 = objectGlobs.activityName[Activity_Stand];

	liveObj->activityName1 = objectGlobs.activityName[Activity_TurnLeft];
	if (LegoObject_UpdateActivityChange(liveObj)) {
		liveObj->flags3 |= LIVEOBJ3_CANTURN;
	}

	liveObj->activityName1 = objectGlobs.activityName[Activity_Gather];
	if (LegoObject_UpdateActivityChange(liveObj)) {
		liveObj->flags3 |= LIVEOBJ3_CANGATHER;
	}

	liveObj->activityName1 = objectGlobs.activityName[Activity_RouteRubble];
	if (LegoObject_UpdateActivityChange(liveObj)) {
		liveObj->flags3 |= LIVEOBJ3_CANROUTERUBBLE;
	}

	// Set back to default activity. Note, this only works because the previous calls to
	//  LegoObject_UpdateActivityChange automatically assigned activityName2, even on failure.
	liveObj->activityName1 = objectGlobs.activityName[Activity_Stand];
	LegoObject_UpdateActivityChange(liveObj);

	AITask_LiveObject_Unk_UpdateAITask_AnimationWait(liveObj);

	// Restore sound functionality now that our object creation is finished.
	Gods98::Container_EnableSoundTriggers(true);
	if (isSoundOn) SFX_SetSoundOn(true, false);

	LegoObject_UpdatePowerConsumption(liveObj);

	// Track the number of objects that ever existed in the level.
	/// NOTE: We ALWAYS want to start by incrementing the initial level, since StatsObject_SetObjectLevel
	///       only either updates total+prevLevels, or neither if the level hasn't changed.
	objectGlobs.objectTotalLevels[liveObj->type][liveObj->id][liveObj->objLevel]++;

	// Respect the unselectable stats flag. This stats flag is ONLY used for initialisation HERE.
	const StatsFlags2 sflags2 = StatsObject_GetStatsFlags2(liveObj);
	if (sflags2 & STATS2_UNSELECTABLE) {
		liveObj->flags3 &= ~LIVEOBJ3_CANSELECT;
	}

	return liveObj;
}

// <LegoRR.exe @00438580>
LegoRR::LegoObject* __cdecl LegoRR::LegoObject_Create_internal(void)
{
	LegoObject* newLiveObj = objectListSet.Add();
	ListSet::MemZero(newLiveObj);

	newLiveObj->point_2f4.y = -1.0f;
	newLiveObj->point_2f4.x = -1.0f;

	return newLiveObj;
}

// <LegoRR.exe @004385d0>
void __cdecl LegoRR::LegoObject_AddList(void)
{
	// NOTE: This function is no longer called, objectListSet.Add already does so.
	objectListSet.AddList();
}




// <LegoRR.exe @00439c50>
void __cdecl LegoRR::LegoObject_RequestPowerGridUpdate(void)
{
	if (objectGlobs.flags & LegoObject_GlobFlags::OBJECT_GLOB_FLAG_UPDATING) {
		// We're in the update loop. Recalculating power now is dangerous when only some of the
		// objects have been update while others are awaiting an update.
		// Delay recalculation until the end of the update loop.
		objectGlobs.flags |= LegoObject_GlobFlags::OBJECT_GLOB_FLAG_POWERNEEDSUPDATE;
	}
	else {
		// Not in the update loop, go straight to power grid recalculation mode.
		objectGlobs.flags |= LegoObject_GlobFlags::OBJECT_GLOB_FLAG_POWERUPDATING;

		if (legoGlobs.currLevel != nullptr) {
			// We need to reset this before PowerGrid recalculation.
			legoGlobs.currLevel->crystalsDrained = 0;
		}
	}
}



// <LegoRR.exe @0043b530>
void __cdecl LegoRR::LegoObject_UpdateAll(real32 elapsedGame)
{
	// Were we updating our power grid after the update loop last tick?
	if (objectGlobs.flags & LegoObject_GlobFlags::OBJECT_GLOB_FLAG_POWERUPDATING) {
		Level_PowerGrid_ClearBlockPowered_100_Points28C();
	}


	objectGlobs.flags |= LegoObject_GlobFlags::OBJECT_GLOB_FLAG_UPDATING;

	for (LegoObject* obj : objectListSet.EnumerateAlive()) {
		LegoObject_Callback_Update(obj, &elapsedGame);
	}
	//LegoObject_RunThroughLists(LegoObject_Callback_Update, &elapsedGame, false);
	
	objectGlobs.flags &= ~LegoObject_GlobFlags::OBJECT_GLOB_FLAG_UPDATING;
	

	// Were we updating our power grid after the update loop last tick?
	if (objectGlobs.flags & LegoObject_GlobFlags::OBJECT_GLOB_FLAG_POWERUPDATING) {
		Level_PowerGrid_UpdateLevelBlocks_PointsAAC();
	}

	// Stop updating power grid if a recalculation is no longer needed.
	if (!(objectGlobs.flags & LegoObject_GlobFlags::OBJECT_GLOB_FLAG_POWERNEEDSUPDATE)) {
		objectGlobs.flags &= ~LegoObject_GlobFlags::OBJECT_GLOB_FLAG_POWERUPDATING;
	}
	else {
		// Otherwise, restart the request for a power grid update (turns on POWERUPDATING flag).
		LegoObject_RequestPowerGridUpdate();
		objectGlobs.flags &= ~LegoObject_GlobFlags::OBJECT_GLOB_FLAG_POWERNEEDSUPDATE;
	}

	LegoObject_Flocks_Update_FUN_0044c1c0(&elapsedGame);

	objectGlobs.LiveManager_TimerUnk -= elapsedGame;
	if (objectGlobs.LiveManager_TimerUnk <= 0.0f) {
		objectGlobs.LiveManager_TimerUnk = 125.0f;
		Lego_UpdateAll3DSounds(true);
	}
}

// <LegoRR.exe @0043b5e0>
void __cdecl LegoRR::LegoObject_RemoveAll(void)
{
	objectGlobs.flags |= LegoObject_GlobFlags::OBJECT_GLOB_FLAG_REMOVING;

	/// TODO: WHY IS SKIPUPGRADEPARTS BEING USED??? Shouldn't we be trying to remove all objects?
	///       Or was that intended to happen after this loop, but was forgotten?
	///       It's also possible upgrade parts are just handled in a wildly different way.
	for (LegoObject* obj : objectListSet.EnumerateSkipUpgradeParts()) {
		// LegoObject_Callback_Remove is just a wrapper around LegoObject_Remove. *something something, with extra steps.*
		LegoObject_Remove(obj);
		//LegoObject_Callback_Remove(obj, nullptr);
	}
	//LegoObject_RunThroughListsSkipUpgradeParts(LegoObject_Callback_Remove, nullptr);

	objectGlobs.flags &= ~LegoObject_GlobFlags::OBJECT_GLOB_FLAG_REMOVING;
}

// RunThroughLists wrapper for LegoObject_Remove.
// <LegoRR.exe @0043b610>
bool32 __cdecl LegoRR::LegoObject_Callback_Remove(LegoObject* liveObj, void* unused)
{
	// ALWAYS returns false, and even if it didn't we still wouldn't want its return value.
	/*return*/ LegoObject_Remove(liveObj);
	return false;
}




// <LegoRR.exe @00449ec0>
void __cdecl LegoRR::LegoObject_HideAllCertainObjects(void)
{
	for (LegoObject* obj : objectListSet.EnumerateAlive()) {
		LegoObject_Callback_HideCertainObjects(obj, nullptr);
	}
	//LegoObject_RunThroughLists(LegoObject_Callback_HideCertainObjects, nullptr, false);
}



// <LegoRR.exe @0044b080>
void __cdecl LegoRR::LegoObject_SetLevelEnding(bool32 ending)
{
	if (ending) objectGlobs.flags |= LegoObject_GlobFlags::OBJECT_GLOB_FLAG_LEVELENDING;
	else        objectGlobs.flags &= ~LegoObject_GlobFlags::OBJECT_GLOB_FLAG_LEVELENDING;
}

// <LegoRR.exe @0044b0a0>
void __cdecl LegoRR::LegoObject_FUN_0044b0a0(LegoObject* liveObj)
{
	if (!(objectGlobs.flags & LegoObject_GlobFlags::OBJECT_GLOB_FLAG_LEVELENDING) &&
		(liveObj->type == LegoObject_Vehicle) && !(liveObj->flags4 & LIVEOBJ4_CRYORECOSTDROPPED))
	{
		LegoObject* routeToObj = ((liveObj->flags4 & LIVEOBJ4_DOCKOCCUPIED) ? liveObj->routeToObject : nullptr);

		LegoObject* spawnTargetObj = (routeToObj != nullptr ? routeToObj : liveObj);

		uint32 crystalCost = Stats_GetCostCrystal(LegoObject_Vehicle, liveObj->id, 0);
		LegoObject_SpawnDropCrystals_FUN_0044b110(spawnTargetObj, crystalCost, (routeToObj != nullptr));
		liveObj->flags4 |= LIVEOBJ4_CRYORECOSTDROPPED;
	}
}




#pragma endregion
