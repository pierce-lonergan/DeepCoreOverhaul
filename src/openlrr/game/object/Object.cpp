// Object.cpp : 
//

#include "../../engine/core/Maths.h"
#include "../../engine/core/Utils.h"
#include "../../engine/input/Input.h"

#include "../audio/SFX.h"
#include "../interface/hud/Bubbles.h"
#include "../interface/Encyclopedia.h"
#include "../interface/HelpWindow.h"
#include "../interface/InfoMessages.h"
#include "../interface/Interface.h"
#include "../interface/Pointers.h"
#include "../interface/RadarMap.h"
#include "../interface/TextMessages.h"
#include "../mission/Messages.h"
#include "../mission/NERPsFile.h"
#include "../world/Construction.h"
#include "../world/ElectricFence.h"
#include "../Debug.h"
#include "../Game.h"
#include "AITask.h"
#include "Stats.h"
#include "Object.h"



/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

// Logic for how often survey scan is performed, and speed at which dots extend out on radar map.
#define OBJECT_SURVEY_CYCLEDURATION		(STANDARD_FRAMERATE * 1.0f)

#pragma endregion

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

// <LegoRR.exe @00437370>
void __cdecl LegoRR::Object_Save_CopyStruct18(OUT SaveStruct_18* saveStruct18)
{
	*saveStruct18 = objectGlobs.savestruct18_c01c;
}

// <LegoRR.exe @00437390>
void __cdecl LegoRR::Object_Save_OverwriteStruct18(OPTIONAL const SaveStruct_18* saveStruct18)
{
	if (saveStruct18 != nullptr) {
		objectGlobs.savestruct18_c01c = *saveStruct18;
	}
	else {
		std::memset(&objectGlobs.savestruct18_c01c, 0, sizeof(SaveStruct_18));
	}
}

// <LegoRR.exe @004373c0>
uint32 __cdecl LegoRR::LegoObject_GetObjectsBuilt(LegoObject_Type objType, bool32 excludeToolStore)
{
	uint32 builtCount = 0;

	/// FIX APPLY: Don't enumerate through object IDs that don't exist!
	const uint32 objIDCount = Lego_GetObjectTypeIDCount(objType); // LegoObject_ID_Count;
	for (uint32 objID = 0; objID < objIDCount; objID++) {

		/// REFACTOR: Move loop inside check, since it doesn't require objLevel.
		if (!excludeToolStore || !(Stats_GetStatsFlags1(objType, (LegoObject_ID)objID) & STATS1_STOREOBJECTS)) {

			// Still enumerate through max levels for objects in-case there's any jank with non-standard object types.
			for (uint32 objLevel = 0; objLevel < OBJECT_MAXLEVELS; objLevel++) {
				builtCount += LegoObject_GetLevelObjectsBuilt(objType, (LegoObject_ID)objID, objLevel, true);
			}
		}
	}
	return builtCount;
}

// <LegoRR.exe @00437410>
void __cdecl LegoRR::Object_LoadToolTipIcons(const Gods98::Config* config)
{
	const Gods98::Config* arrayFirst = Gods98::Config_FindArray(config, Lego_ID("ToolTipIcons"));

	for (const Gods98::Config* prop = arrayFirst; prop != nullptr; prop = Gods98::Config_GetNextItem(prop)) {
		bool found = false;

		for (uint32 toolType = 0; !found && toolType < LegoObject_ToolType_Count; toolType++) {
			const char* toolName = legoGlobs.toolName[toolType];
			if (toolName != nullptr && ::_stricmp(toolName, Gods98::Config_GetItemName(prop)) == 0) {
				objectGlobs.ToolTipIcons_Tools[toolType] = Gods98::Image_LoadBMP(Gods98::Config_GetDataString(prop));
				found = true;
			}
		}

		for (uint32 abilityType = 0; !found && abilityType < LegoObject_AbilityType_Count; abilityType++) {
			const char* abilityName = objectGlobs.abilityName[abilityType];
			if (abilityName != nullptr && ::_stricmp(abilityName, Gods98::Config_GetItemName(prop)) == 0) {
				objectGlobs.ToolTipIcons_Abilities[abilityType] = Gods98::Image_LoadBMP(Gods98::Config_GetDataString(prop));
				found = true;
			}
		}

		if (!found) {
			if (::_stricmp(Gods98::Config_GetItemName(prop), "Blank") == 0) {
				objectGlobs.ToolTipIcon_Blank = Gods98::Image_LoadBMP(Gods98::Config_GetDataString(prop));
				found = true;
			}
			else if (::_stricmp(Gods98::Config_GetItemName(prop), "Ore") == 0) {
				objectGlobs.ToolTipIcon_Ore = Gods98::Image_LoadBMP(Gods98::Config_GetDataString(prop));
				found = true;
			}
			else {
				Config_WarnItemF(true, prop, "Unknown ToolTipIcon %s", Gods98::Config_GetItemName(prop));
			}
		}
	}

	if (objectGlobs.ToolTipIcon_Ore != nullptr) {
		Gods98::Image_SetupTrans(objectGlobs.ToolTipIcon_Ore, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	}
}

// <LegoRR.exe @00437560>
void __cdecl LegoRR::LegoObject_CleanupLevel(void)
{
	LegoObject_RemoveAll();
	LegoObject_CleanupObjectLevels();
	HiddenObject_RemoveAll();

	std::memset(objectGlobs.slugHoleBlocks, 0, sizeof(objectGlobs.slugHoleBlocks));
	std::memset(objectGlobs.rechargeSeamBlocks, 0, sizeof(objectGlobs.rechargeSeamBlocks));

	objectGlobs.slugHoleCount = 0;
	objectGlobs.rechargeSeamCount = 0;
	objectGlobs.dischargeBuildup = 0.0f;

	std::memset(objectGlobs.liveObjArray100_c43c, 0, sizeof(objectGlobs.liveObjArray100_c43c));

	objectGlobs.uintCount_c5cc = 0;

	LegoObject_SetNumBuildingsTeleported(0);
}

// Used for consuming and producing unpowered crystals after weapon discharge.
// <LegoRR.exe @004375c0>
void __cdecl LegoRR::LegoObject_Weapon_FUN_004375c0(LegoObject* liveObj, sint32 weaponID, real32 coef)
{
	real32 rate = coef;
	if (weaponID != -1) {
		rate *= Weapon_GetDischargeRate(weaponID);
	}
	objectGlobs.dischargeBuildup += rate;

	Point2I blockPos = { 0 }; // dummy init
	LegoObject_GetBlockPos(liveObj, &blockPos.x, &blockPos.y);

	while (objectGlobs.dischargeBuildup >= 1.0f) {
		LegoObject* crystalObj = LegoObject_FUN_00438d20(&blockPos, LegoObject_PowerCrystal, (LegoObject_ID)0, 0);
		if (crystalObj == nullptr) break;

		crystalObj = LegoObject_SpawnCarryableObject(crystalObj, LegoObject_PowerCrystal, (LegoObject_ID)0, 0);
		if (crystalObj == nullptr) break;

		crystalObj->flags3 |= LIVEOBJ3_POWEROFF;
		LegoObject_SetCrystalPoweredColour(crystalObj, false);
		objectGlobs.dischargeBuildup -= 1.0;
	}
}

// <LegoRR.exe @00437690>
bool32 __cdecl LegoRR::LegoObject_DoOpeningClosing(LegoObject* liveObj, bool32 open)
{
	if (open && !(liveObj->flags4 & LIVEOBJ4_UNK_2000)) {
		liveObj->flags4 |= LIVEOBJ4_UNK_1000;
		LegoObject_SetActivity(liveObj, Activity_Opening, 0);
	}
	else if (!open && (liveObj->flags4 & LIVEOBJ4_UNK_2000)) {
		liveObj->flags4 &= ~LIVEOBJ4_UNK_2000;
		liveObj->flags4 |= LIVEOBJ4_UNK_4000;
		LegoObject_SetActivity(liveObj, Activity_Closing, 0);
	}

	return LegoObject_UpdateActivityChange(liveObj);
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
	/// CUSTOM: Cleanup routing path lines.
	Debug_RouteVisual_Remove(deadObj);

	/// CUSTOM: Cleanup disposable stats (if it is overridden).
	StatsObject_RestoreModified(deadObj);

	/// CUSTOM: Cleanup follow reference.
	if (Lego_GetFollowUnit() == deadObj) {
		Lego_SetFollowUnit(nullptr);
	}


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

	/// FIX APPLY: Cleanup freeze object references.
	if (deadObj->freezeObject != nullptr) {
		if (deadObj->flags2 & LIVEOBJ2_FROZEN) {
			// We're not the ice cube. Perform cleanup to remove the ice cube too.
			deadObj->flags2 &= ~LIVEOBJ2_FROZEN;

			Error_InfoF("Object @ %i with ice cube @ %i removed", (uint32)objectListSet.IndexOf(deadObj), (uint32)objectListSet.IndexOf(deadObj->freezeObject));

			// Set ice cube to final stage of melting, where it won't interact with freezeObject.
			deadObj->freezeObject->health = -1.0f;
			deadObj->freezeObject->flags1 &= ~LIVEOBJ1_TELEPORTINGDOWN;
			//deadObj->freezeObject->flags3 &= ~LIVEOBJ3_REMOVING; // Mark the ice cube for melting.
			deadObj->freezeObject->flags3 |= LIVEOBJ3_REMOVING; // Mark the ice cube for melting.

			// Link the ice cube object to itself, so that it can die in peace (and not reference a destroyed object)...
			deadObj->freezeObject->freezeObject = deadObj->freezeObject;
		}
		else {
			// This is the ice cube.
			Error_InfoF("Ice cube @ %i removed", (uint32)objectListSet.IndexOf(deadObj));

			// Make sure our object is unfrozen.
			deadObj->freezeObject->flags2 &= ~LIVEOBJ2_FROZEN;
			deadObj->freezeObject->freezeObject = nullptr;
		}
		// Unlink freeze object (one-way).
		deadObj->freezeObject = nullptr;
	}

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

// <LegoRR.exe @00437b40>
void __cdecl LegoRR::LegoObject_SetCustomName(LegoObject* liveObj, OPTIONAL const char* newCustomName)
{
	if (newCustomName != nullptr && *newCustomName != '\0') {
		// Add or Set custom name.
		/// FIX APPLY: Actually allocate 12 chars instead of allocating 11 and writing to index 11!!!
		///            The only reason this worked before is because the malloc implementation allocates in units of 16 for smaller sizes.
		if (liveObj->customName == nullptr) {
			liveObj->customName = (char*)Gods98::Mem_Alloc(OBJECT_CUSTOMNAMELENGTH);
		}

		// Copy up to Length-1. strncpy null-terminates all chars beyond the string up to excluding Length-1.
		// So we'll need to null-terminate the final char ourselves.
		std::strncpy(liveObj->customName, newCustomName, OBJECT_MAXCUSTOMNAMECHARS);
		liveObj->customName[OBJECT_MAXCUSTOMNAMECHARS] = '\0'; // Null-terminate final char, not handled by strncpy.
	}
	else {
		// Remove custom name.
		if (liveObj->customName != nullptr) {
			Gods98::Mem_Free(liveObj->customName);
			liveObj->customName = nullptr;
		}
	}
}

// <LegoRR.exe @00437ba0>
void __cdecl LegoRR::HiddenObject_RemoveAll(void)
{
	for (uint32 i = 0; i < objectGlobs.hiddenObjectCount; i++) {
		HiddenObject* hiddenObj = &objectGlobs.hiddenObjects[i];
		
		if (hiddenObj->olistID != nullptr) {
			Gods98::Mem_Free(hiddenObj->olistID);
		}

		if (hiddenObj->olistDrivingID != nullptr) {
			Gods98::Mem_Free(hiddenObj->olistDrivingID);
		}
	}
	objectGlobs.hiddenObjectCount = 0;

	std::memset(objectGlobs.hiddenObjects, 0, sizeof(objectGlobs.hiddenObjects));

	NERPsRuntime_ClearHiddenObjectsFound();
}

// <LegoRR.exe @00437c00>
void __cdecl LegoRR::HiddenObject_ExposeBlock(const Point2I* blockPos)
{
	LegoObject* exposedObjs[OBJECT_MAXHIDDENOBJECTS]      = { nullptr }; // apLStack2400
	char* exposedOListIDs[OBJECT_MAXHIDDENOBJECTS]        = { nullptr }; // apcStack800
	char* exposedOListDrivingIDs[OBJECT_MAXHIDDENOBJECTS] = { nullptr }; // local_640
	uint32 exposedCount = 0; // local_968

	// Find exposed OL objects.
	for (uint32 i = 0; i < objectGlobs.hiddenObjectCount; i++) {
		HiddenObject* hiddenObj = &objectGlobs.hiddenObjects[i];
		if (hiddenObj->blockPos.x == blockPos->x && hiddenObj->blockPos.y == blockPos->y) {

			LegoObject* liveObj;
			if (hiddenObj->type == LegoObject_Building) {
				// Convert radians to range [0,8], then wrap to a valid direction [0,3].
				// This uses slices (like a pie chart) around each cardinal direction.
				// So a value of 340deg or 41deg would still be closest to 0deg, not 270deg or 90deg.
				//  (This is stored in radians, degrees are just for the example)
				const sint32 rotationTemp = (sint32)(hiddenObj->heading * (8.0f / (M_PI * 2))); // (0.1591549f * 8.0f));
				// Positive modulus of (rotation + 1). Use +1 to align pie slices for modulus operation.
				const Direction rotation = (Direction)(((((rotationTemp + 1) / 2) % DIRECTION__COUNT) + DIRECTION__COUNT) % DIRECTION__COUNT);

				uint32 shapeCount = 0; // dummy init
				const Point2I* shapePoints = Building_GetShapePoints(&legoGlobs.buildingData[hiddenObj->id], &shapeCount);
				const Point2I* finalShapePoints = SelectPlace_TransformShapePoints(&hiddenObj->blockPos, shapePoints, shapeCount, rotation);

				liveObj = Construction_SpawnBuilding(hiddenObj->id, &hiddenObj->blockPos, rotation, finalShapePoints, shapeCount, false);
			}
			else {
				liveObj = LegoObject_Create(hiddenObj->model, hiddenObj->type, hiddenObj->id);
			}

			if (liveObj != nullptr) {
				if (liveObj->type != LegoObject_Building) {
					LegoObject_SetPositionAndHeading(liveObj, hiddenObj->worldPos.x, hiddenObj->worldPos.y, hiddenObj->heading, true);
				}

				if (liveObj->type == LegoObject_PowerCrystal || liveObj->type == LegoObject_Ore ||
					liveObj->type == LegoObject_Dynamite || liveObj->type == LegoObject_Barrier)
				{
					AITask_DoCollect(liveObj, 0.0f);
				}

				liveObj->health = hiddenObj->health;

				if (liveObj->type == LegoObject_MiniFigure) {
					NERPsRuntime_IncHiddenObjectsFound(LegoObject_MiniFigure);
					Info_Send(Info_FoundMinifigure, nullptr, liveObj, nullptr);
				}

				liveObj->flags4 |= LIVEOBJ4_DONTSHOWHELPWINDOW; // Don't spam the help window when discovering hidden units.

				LegoObject_FUN_00438720(liveObj);

				if (liveObj->type == LegoObject_ElectricFence) {
					ElectricFence_CreateFence(liveObj);
				}

				exposedObjs[exposedCount]            = liveObj;
				exposedOListIDs[exposedCount]        = hiddenObj->olistID;
				exposedOListDrivingIDs[exposedCount] = hiddenObj->olistDrivingID;
				exposedCount++;

				/// TODO: Should we also nullify olistDrivingID?
				hiddenObj->olistID = nullptr;
			}

			// Replace this exposed object with the hidden object at the end of the list.
			objectGlobs.hiddenObjects[i] = objectGlobs.hiddenObjects[objectGlobs.hiddenObjectCount - 1];
			objectGlobs.hiddenObjectCount--;
			i--;
		}
	}

	// Connect exposed OL drivers.
	/// NOTE: This requires that both drivers and vehicle OL objects be defined in the same blockPos.
	for (uint32 i = 0; i < exposedCount; i++) {
		char* olistDrivingID = exposedOListDrivingIDs[i];
		if (olistDrivingID != nullptr) {

			// Find the the exposed OL object of the same olistID as olistDrivingID.
			for (uint32 j = 0; j < exposedCount; j++) {
				if (i != j && ::_stricmp(exposedOListIDs[j], olistDrivingID) == 0) {

					LegoObject* driverObj = exposedObjs[i];
					LegoObject* vehicleObj = exposedObjs[j];

					driverObj->routeToObject = vehicleObj;
					driverObj->flags2 |= LIVEOBJ2_UNK_4; // OL object flag? Related to driving? Route to object and get in?

					LegoObject_AbilityFlags trainFlags;

					const StatsFlags3 sflags3 = StatsObject_GetStatsFlags3(vehicleObj);
					if (sflags3 & STATS3_NEEDSPILOT) {
						trainFlags = ABILITY_FLAG_PILOT;
					}
					else {
						const StatsFlags1 sflags1 = StatsObject_GetStatsFlags1(vehicleObj);
						if (!(sflags1 & STATS1_CROSSWATER)) {
							trainFlags = ABILITY_FLAG_DRIVER;
						}
						else if (!(sflags1 & STATS1_CROSSLAND)) {
							trainFlags = ABILITY_FLAG_SAILOR;
						}
						else {
							trainFlags = ABILITY_FLAG_PILOT;
						}
					}
					LegoObject_TrainMiniFigure_instantunk(driverObj, trainFlags);
					LegoObject_ClearFlags4_40_AndSameForObject2FC(driverObj, vehicleObj);
				}
			}
		}
	}

	// Cleanup exposed OL string IDs that were owned by HiddenObjects.
	for (uint32 i = 0; i < exposedCount; i++) {
		if (exposedOListIDs[i] != nullptr) {
			Gods98::Mem_Free(exposedOListIDs[i]);
		}

		if (exposedOListDrivingIDs[i] != nullptr) {
			Gods98::Mem_Free(exposedOListDrivingIDs[i]);
		}
	}
}

// <LegoRR.exe @00437ee0>
void __cdecl LegoRR::HiddenObject_Add(ObjectModel* objModel, LegoObject_Type objType, LegoObject_ID objID, const Point2F* worldPos, real32 heading, real32 health, const char* olistID, const char* olistDrivingID)
{
	/// SANITY: Error check bounds.
	if (objectGlobs.hiddenObjectCount >= OBJECT_MAXHIDDENOBJECTS) {
		Error_Fatal(true, "Too many hidden objects.");
	}

	HiddenObject* hiddenObj = &objectGlobs.hiddenObjects[objectGlobs.hiddenObjectCount];
	objectGlobs.hiddenObjectCount++;

	std::memset(hiddenObj, 0, sizeof(HiddenObject));

	// Get blockPos
	Map3D_WorldToBlockPos_NoZ(Lego_GetMap(), worldPos->x, worldPos->y, &hiddenObj->blockPos.x, &hiddenObj->blockPos.y);

	hiddenObj->worldPos = *worldPos;
	hiddenObj->heading = heading;
	hiddenObj->model = objModel;
	hiddenObj->type = objType;
	hiddenObj->id = objID;
	hiddenObj->health = health;

	if (olistID != nullptr) {
		hiddenObj->olistID = Gods98::Util_StrCpy(olistID);
	}

	if (olistDrivingID != nullptr) {
		hiddenObj->olistDrivingID = Gods98::Util_StrCpy(olistDrivingID);
	}
}

// <LegoRR.exe @00437f80>
bool32 __cdecl LegoRR::LegoObject_CanShootObject(LegoObject* liveObj)
{
	const StatsFlags2 sflags2 = StatsObject_GetStatsFlags2(liveObj);
	if (sflags2 & STATS2_CANBESHOTAT) {
		if (liveObj->health > StatsObject_GetPainThreshold(liveObj)) {
			return true;
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

	newLiveObj->targetBlockPos.y = -1.0f;
	newLiveObj->targetBlockPos.x = -1.0f;

	return newLiveObj;
}

// <LegoRR.exe @004385d0>
void __cdecl LegoRR::LegoObject_AddList(void)
{
	// NOTE: This function is no longer called, objectListSet.Add already does so.
	objectListSet.AddList();
}

// <LegoRR.exe @00438650>
uint32 __cdecl LegoRR::LegoObject_GetNumBuildingsTeleported(void)
{
	return objectGlobs.BuildingsTeleported;
}

// <LegoRR.exe @00438660>
void __cdecl LegoRR::LegoObject_SetNumBuildingsTeleported(uint32 numTeleported)
{
	objectGlobs.BuildingsTeleported = numTeleported;
}

// <LegoRR.exe @00438670>
void __cdecl LegoRR::LegoObject_SetCrystalPoweredColour(LegoObject* liveObj, bool32 powered)
{
	if (liveObj->type == LegoObject_PowerCrystal) {
		const ColourRGBF colour = (powered ? legoGlobs.PowerCrystalRGB : legoGlobs.UnpoweredCrystalRGB);

		for (uint32 groupID = 0; groupID < Gods98::Container_Mesh_GetGroupCount(liveObj->other); groupID++) {

			Gods98::Container_Mesh_SetColourAlpha(liveObj->other, groupID, colour.red, colour.green, colour.blue, 1.0f);
			Gods98::Container_Mesh_SetEmissive(liveObj->other, groupID, colour.red, colour.green, colour.blue);
		}
	}
}

// <LegoRR.exe @00438720>
void __cdecl LegoRR::LegoObject_FUN_00438720(LegoObject* liveObj)
{
	if (liveObj->type == LegoObject_Building) {
		LegoObject_UpdateActivityChange(liveObj);

		liveObj->flags3 |= LIVEOBJ3_CANCARRY; // Temporarily set flag, not sure if other cases unset it.

		const StatsFlags1 sflags1 = StatsObject_GetStatsFlags1(liveObj);
		if (sflags1 & STATS1_STOREOBJECTS) {
			AITask_LiveObject_FUN_004038d0(liveObj);
		}
		else if (sflags1 & STATS1_PROCESSORE) {
			AITask_DoDeposit_ObjectType(liveObj, LegoObject_Ore, (LegoObject_ID)0, 0);
		}
		else if (sflags1 & STATS1_PROCESSCRYSTAL) {
			AITask_DoDeposit_ObjectType(liveObj, LegoObject_PowerCrystal, (LegoObject_ID)0, 0);
		}
		else {
			liveObj->flags3 &= ~LIVEOBJ3_CANCARRY; // Unset temporary flag.
		}

		LegoObject_RequestPowerGridUpdate();
		liveObj->flags4 |= LIVEOBJ4_UNK_200000;
	}
	else if (liveObj->type == LegoObject_Vehicle) {
		Vehicle_HideWheels(liveObj->vehicle, false);
	}

	if (liveObj->type == LegoObject_MiniFigure || liveObj->type == LegoObject_Vehicle) {
		LegoObject_RemoveTeleportDownReference(liveObj);
	}

	HelpWindow_RecallDependencies(liveObj->type, liveObj->id, 0, (liveObj->flags4 & LIVEOBJ4_DONTSHOWHELPWINDOW));

	if (liveObj->type == LegoObject_Building) {
		objectGlobs.BuildingsTeleported++;
	}
	if (liveObj->type == LegoObject_MiniFigure || liveObj->type == LegoObject_Vehicle) {
		AITask_DoAttackObject(liveObj);
	}
	if (liveObj->type == LegoObject_Vehicle || liveObj->type == LegoObject_Building) {
		liveObj->flags4 |= LIVEOBJ4_UNK_40000;
	}
}

// <LegoRR.exe @00438840>
void __cdecl LegoRR::LegoObject_SetPowerOn(LegoObject* liveObj, bool32 on)
{
	if (!on) {
		liveObj->flags3 |= LIVEOBJ3_POWEROFF;
	}
	else {
		liveObj->flags3 &= ~LIVEOBJ3_POWEROFF;
	}
	/// TODO: Can we choose to skip this when the flag is unchanged?
	LegoObject_RequestPowerGridUpdate();
}

// <LegoRR.exe @00438870>
bool32 __cdecl LegoRR::LegoObject_IsActive(LegoObject* liveObj, bool32 ignoreUnpowered)
{
	if (!(liveObj->flags1 & (LIVEOBJ1_EXPANDING|LIVEOBJ1_CRUMBLING|LIVEOBJ1_TELEPORTINGDOWN|LIVEOBJ1_TELEPORTINGUP)) &&
		!(liveObj->flags1 & LIVEOBJ1_ENTERING_WALLHOLE) && !(liveObj->flags2 & LIVEOBJ2_UNK_100000) &&
		liveObj->health >= 0.0f &&
		(ignoreUnpowered || ((liveObj->flags3 & LIVEOBJ3_HASPOWER) && !(liveObj->flags3 & LIVEOBJ3_POWEROFF))))
	{
		return true;
	}
	return false;
}

// <LegoRR.exe @004388d0>
LegoRR::LegoObject* __cdecl LegoRR::LegoObject_CreateInWorld(ObjectModel* objModel, LegoObject_Type objType, LegoObject_ID objID, uint32 objLevel, real32 xPos, real32 yPos, real32 heading)
{
	LegoObject* liveObj = LegoObject_Create(objModel, objType, objID);
	if (liveObj != nullptr) {
		LegoObject_SetPositionAndHeading(liveObj, xPos, yPos, heading, true);
		if (liveObj->type == LegoObject_PowerCrystal || liveObj->type == LegoObject_Ore) {
			LegoObject_SetLevel(liveObj, objLevel);
		}
		return liveObj;
	}
	return nullptr;
}

// <LegoRR.exe @00438930>
LegoRR::LegoObject* __cdecl LegoRR::LegoObject_FindPoweredBuildingAtBlockPos(const Point2I* blockPos)
{
	SearchObjectBlockXY_c search = { nullptr }; // dummy init

	search.resultObj = nullptr;
	search.bx = blockPos->x;
	search.by = blockPos->y;

	for (LegoObject* obj : objectListSet.EnumerateSkipUpgradeParts()) {
		if (LegoObject_Callback_FindPoweredBuildingAtBlockPos(obj, &search))
			break;
	}
	//LegoObject_RunThroughListsSkipUpgradeParts(LegoObject_Callback_FindPoweredBuildingAtBlockPos, &search);
	return search.resultObj;
}

// DATA: SearchObjectBlockXY_c* search
// <LegoRR.exe @00438970>
bool32 __cdecl LegoRR::LegoObject_Callback_FindPoweredBuildingAtBlockPos(LegoObject* liveObj, void* pSearch)
{
	SearchObjectBlockXY_c* search = (SearchObjectBlockXY_c*)pSearch;

	if (liveObj->type == LegoObject_Building) {
		const StatsFlags2 sflags2 = StatsObject_GetStatsFlags2(liveObj);
		if (sflags2 & STATS2_GENERATEPOWER) {
			if (LegoObject_IsActive(liveObj, false)) {
				sint32 bx = 0, by = 0; // dummy inits
				LegoObject_GetBlockPos(liveObj, &bx, &by);
				if (bx == search->bx && by == search->by) {
					search->resultObj = liveObj;
					return true;
				}
			}
		}
	}
	return false;
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

		// We need to reset this before PowerGrid recalculation.
		Level_ResetCrystalsDrained();
	}
}


// <LegoRR.exe @0043b010>
LegoRR::LegoObject* __cdecl LegoRR::LegoObject_TryGenerateSlug(LegoObject* originObj, LegoObject_ID objID)
{
	if (objectGlobs.slugHoleCount != 0) {
		Point2I blockPos = { 0 }; // dummy init
		if (originObj == nullptr) {
			const uint32 holeIndex = ((uint32)Gods98::Maths_Rand() % objectGlobs.slugHoleCount);
			blockPos = objectGlobs.slugHoleBlocks[holeIndex];
		}
		else {
			LegoObject_FindNearestSlugHole(originObj, &blockPos);
		}
		return LegoObject_TryGenerateSlugAtBlock(&legoGlobs.rockMonsterData[objID], LegoObject_RockMonster, objID,
												 blockPos.x, blockPos.y, 0.0f, false);
	}
	return nullptr;
}

/// CUSTOM: Generation for slug with a specific block pos already specified.
LegoRR::LegoObject* LegoRR::LegoObject_TryGenerateSlugAtBlock(ObjectModel* objModel, LegoObject_Type objType, LegoObject_ID objID, uint32 bx, uint32 by, real32 heading, bool assignHeading)
{
	if (objType != LegoObject_RockMonster)
		return nullptr;

	bool isSlugHole = false;
	for (uint32 i = 0; i < objectGlobs.slugHoleCount; i++) {
		if (objectGlobs.slugHoleBlocks[i].x == bx && objectGlobs.slugHoleBlocks[i].y == by) {
			isSlugHole = true;
			break;
		}
	}
	if (!isSlugHole)
		return nullptr;

	// If no heading supplied, generate a random one.
	if (!assignHeading) {
		heading = Gods98::Maths_RandRange(0.0f, M_PI*2.0f);
	}

	Point2F wPos = { 0.0f }; // dummy init
	Map3D_BlockToWorldPos(Lego_GetMap(), bx, by, &wPos.x, &wPos.y);
	wPos.x += Maths_Sin(heading) * 12.75f;
	wPos.y += Maths_Cos(heading) * 12.75f;

	LegoObject* slugObj = LegoObject_CreateInWorld(objModel, objType, objID, 0, wPos.x, wPos.y, heading);
	if (slugObj != nullptr) {
		slugObj->flags1 |= LIVEOBJ1_EXPANDING;
		slugObj->flags3 &= ~LIVEOBJ3_POWEROFF;

		LegoObject_SetActivity(slugObj, Activity_Emerge, 0);
		LegoObject_UpdateActivityChange(slugObj);
		AITask_DoAnimationWait(slugObj);
		Info_Send(Info_SlugEmerge, nullptr, slugObj, nullptr);
	}
	return slugObj;
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

	objectGlobs.s_sound3DUpdateTimer -= elapsedGame;
	if (objectGlobs.s_sound3DUpdateTimer <= 0.0f) {
		objectGlobs.s_sound3DUpdateTimer = 5.0f * STANDARD_FRAMERATE; // Every 5 seconds
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


// DRAW MODE: Only Draw API drawing calls can be used within this function.
// <LegoRR.exe @0043c570>
void __cdecl LegoRR::LegoObject_UpdateAllRadarSurvey(real32 elapsedGame, bool32 inRadarMapView)
{
	objectGlobs.radarSurveyCycleTimer += elapsedGame;
	// Survey dots have a one second duration.
	if (objectGlobs.radarSurveyCycleTimer > OBJECT_SURVEY_CYCLEDURATION) {
		objectGlobs.radarSurveyCycleTimer = 0.0f;
	}

	for (LegoObject* obj : objectListSet.EnumerateSkipUpgradeParts()) {
		LegoObject_Callback_UpdateRadarSurvey(obj, &inRadarMapView);
	}
	//LegoObject_RunThroughListsSkipUpgradeParts(LegoObject_Callback_UpdateRadarSurvey, &inRadarMapView);
}

// DRAW MODE: Only Draw API drawing calls can be used within this function.
// DATA: bool32* inRadarMapView
// <LegoRR.exe @0043c5b0>
bool32 __cdecl LegoRR::LegoObject_Callback_UpdateRadarSurvey(LegoObject* liveObj, void* pInRadarMapView)
{
	const bool32 inRadarMapView = *static_cast<bool32*>(pInRadarMapView);

	if (liveObj->abilityFlags & ABILITY_FLAG_SCANNER) {
		const uint32 surveyRadius = StatsObject_GetSurveyRadius(liveObj);
		if (surveyRadius <= 0)
			return false; // No distance to survey.

		if (liveObj->type == LegoObject_Building && !LegoObject_IsActive(liveObj, false))
			return false; // Building is not currently surveying.

		if (objectGlobs.radarSurveyCycleTimer == 0.0f) {
			// Start of next survey cycle. Perform survey checks now.
			// Also this means the survey dots are at their center, so we don't draw them.
			Point2I blockPos = { 0 }; // dummy init
			LegoObject_GetBlockPos(liveObj, &blockPos.x, &blockPos.y);
			Level_Block_UpdateSurveyRadius_FUN_00434f40(&blockPos, surveyRadius);
		}
		else if (inRadarMapView) {
			Point2F centerPos = { 0.0f }; // dummy init
			LegoObject_GetPosition(liveObj, &centerPos.x, &centerPos.y);

			// Range from [0.0,1.0].
			const real32 range = (objectGlobs.radarSurveyCycleTimer / OBJECT_SURVEY_CYCLEDURATION);

			const real32 blockSize = Map3D_BlockSize(Lego_GetMap());
			// Dots draw starting from the center of the unit and expand out to `surveyRadius` blocks.
			const real32 radius = (blockSize * range * static_cast<real32>(surveyRadius));
			// Dots get darker the further out they're drawn.
			const real32 brightness = 1.0f - range;

			RadarMap_DrawSurveyDotCircle(Lego_GetRadarMap(), &centerPos, radius, brightness);
		}
	}
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


// <LegoRR.exe @0044abd0>
bool32 __cdecl LegoRR::LegoObject_UpdateBuildingPlacement(LegoObject_Type objType, LegoObject_ID objID, bool32 leftReleased, bool32 rightReleased,
															OPTIONAL const Point2F* mouseWorldPos, uint32 mouseBlockX, uint32 mouseBlockY,
															bool32 execute, SelectPlace* selectPlace)
{
	const Point2I mouseBlockPos = {
		static_cast<sint32>(mouseBlockX),
		static_cast<sint32>(mouseBlockY),
	};

	if (objType != LegoObject_Building)
		return true; // Default return value.

	if (!execute) {
		SelectPlace_Hide(selectPlace, true);
		return true; // Default return value.
	}

	if (!Level_Block_IsGround(mouseBlockX, mouseBlockY)) {
		if (!rightReleased) {
			Pointer_SetCurrent_IfTimerFinished(Pointer_CannotBuild);
		}
		if (!rightReleased && leftReleased) {
			Lego_SetPointerSFX(PointerSFX_NotOkay);
		}

		SelectPlace_Hide(selectPlace, true);
	}
	else {
		uint32 shapeCount = 0; // dummy init
		const Point2I* shapePoints = Building_GetShapePoints(&legoGlobs.buildingData[objID], &shapeCount);
		const uint32 waterEntrances = Stats_GetWaterEntrances(LegoObject_Building, objID, 0);
		SelectPlace_Hide(selectPlace, false);

		Point2F wPos = { 0.0f }; // dummy init
		Map3D_BlockToWorldPos(Lego_GetMap(), mouseBlockX, mouseBlockY, &wPos.x, &wPos.y);

		Point2F wOff; // Offset from where the mouse is to the center of the block.
		Gods98::Maths_Vector2DSubtract(&wOff, mouseWorldPos, &wPos);

		// Try to snap to a new default valid rotation, note that this may be replaced during the direction loop below this.
		if (wOff.x > 0.0f && wOff.x > std::abs(wOff.y)) {
			legoGlobs.placeObjDirection = DIRECTION_RIGHT;
		}
		else if (wOff.x < 0.0f && std::abs(wOff.x) > std::abs(wOff.y)) {
			legoGlobs.placeObjDirection = DIRECTION_LEFT;
		}
		else if (wOff.y > 0.0f && wOff.y > std::abs(wOff.x)) {
			// Wouldn't this actually be down? If we're following conventions for left/right...
			legoGlobs.placeObjDirection = DIRECTION_UP;
		}
		else if (wOff.y < 0.0f && std::abs(wOff.y) > std::abs(wOff.x)) {
			legoGlobs.placeObjDirection = DIRECTION_DOWN;
		}
		else {
			/// FIXME: No handling for this scenario???
			//legoGlobs.placeObjDirection = ???;
		}

		// FIVE!!???
		for (uint32 d = 0; d < 5; d++) {
			const Direction direction = DirectionWrap(legoGlobs.placeObjDirection + d);

			const Point2I* shapeBlocks = SelectPlace_CheckAndUpdate(selectPlace, &mouseBlockPos, shapePoints, shapeCount,
																	direction, Lego_GetMap(), waterEntrances);
			// I think this may return null if the shape is outside the world bounds, or doesn't meet OSHA regulations.
			if (shapeBlocks == nullptr)
				continue;

			if (!rightReleased) {
				Pointer_SetCurrent_IfTimerFinished(Pointer_CanBuild);
			}
			if (!rightReleased && leftReleased) {
				const uint32 constructHandle = Construction_Zone_StartBuilding(objID, &mouseBlockPos, direction,
																			   shapeBlocks, shapeCount);
				// Request Barriers:
				//const StatsFlags1 sflags1 = Stats_GetStatsFlags1(LegoObject_Building, objID);
				//if (!(sflags1 & STATS1_STOREOBJECTS)) {
				if (Stats_GetRequiresConstructionBarriers(LegoObject_Building, objID)) {
					// Only non-ToolStore objects require barriers.
					Construction_Zone_RequestBarriers(&mouseBlockPos, shapeBlocks, shapeCount);
				}

				// Request Crystals:
				const uint32 crystalsCost = Stats_GetCostCrystal(LegoObject_Building, objID, 0);
				Construction_Zone_RequestResource(&mouseBlockPos, LegoObject_PowerCrystal, (LegoObject_ID)0, 0, crystalsCost);

				// Request Ore / Studs:
				// Studs will only be used if a processing building is available, and if the studs cost is non-zero.
				const LegoObject* oreProcessingObj = LegoObject_FindResourceProcessingBuilding(nullptr, &mouseBlockPos, LegoObject_Ore, 0);
				const uint32 oreCost = Stats_GetCostOre(LegoObject_Building, objID, 0);
				const uint32 studsCost = Stats_GetCostRefinedOre(LegoObject_Building, objID, 0);

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
				Construction_Zone_RequestResource(&mouseBlockPos, LegoObject_Ore, oreObjID, 0, oreTypeCost);

				// Track this info to properly handle Construction_Zone_NeedsMoreOfResource.
				Construction_Zone_SetNoBuildCost(constructHandle, Cheat_IsNoBuildCosts());

				// Dummy call to check condition of having all resources (needed for buildings like toolstore).
				Construction_Zone_PlaceResource(constructHandle, nullptr);

				SelectPlace_Hide(selectPlace, true);
				Interface_BackToMain();
				Lego_SetPointerSFX(PointerSFX_Okay);
				return false; // End selectPlace mode??
			}
			else {
				// Found a valid direction, but didn't place.
				// Store this direction as the last valid direction.
				legoGlobs.placeObjDirection = direction;
				//break;
				return true;
			}
		}

		if (!rightReleased) {
			Pointer_SetCurrent_IfTimerFinished(Pointer_CannotBuild);
		}
		if (!rightReleased && leftReleased) {
			Lego_SetPointerSFX(PointerSFX_NotOkay);
		}
	}
	Text_DisplayMessage(Text_CannotPlaceBuilding, false, false);
	return true;
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


// <LegoRR.exe @0044c2f0>
bool32 __cdecl LegoRR::LegoObject_Freeze(LegoObject* liveObj, real32 freezerTime)
{
	/// CHANGE: Allow other units to be frozen, this is a pointless restriction when CANFREEZE is a stat.
	if (!(liveObj->flags2 & LIVEOBJ2_FROZEN) /*&& liveObj->type == LegoObject_RockMonster*/) {
		liveObj->flags2 |= LIVEOBJ2_FROZEN;
		liveObj->freezeTimer = freezerTime * STANDARD_FRAMERATE;

		Point2F wPos = { 0.0f }; // dummy init
		LegoObject_GetPosition(liveObj, &wPos.x, &wPos.y);
		const real32 heading = LegoObject_GetHeading(liveObj);
		
		LegoObject* iceCubeObj = LegoObject_CreateInWorld(legoGlobs.contIceCube, LegoObject_IceCube, (LegoObject_ID)0, 0, wPos.x, wPos.y, heading);
		Gods98::Container_SetActivity(iceCubeObj->other, "Start");
		Gods98::Container_SetAnimationTime(iceCubeObj->other, 0.0f);
		iceCubeObj->flags1 |= LIVEOBJ1_EXPANDING;

		liveObj->freezeObject = iceCubeObj;
		iceCubeObj->freezeObject = liveObj;

		return true;
	}
	return false;
}


// <LegoRR.exe @0044c810>
void __cdecl LegoRR::LegoObject_CameraCycleUnits(void)
{
	// Cycle-able units have now been validated since level-start (once the function exits).
	objectGlobs.flags |= LegoObject_GlobFlags::OBJECT_GLOB_FLAG_CYCLEUNITS;

	bool unitsExist = false; // Track if we've found the unit type to cycle through.

	// Start by trying to cycle through buildings.
	//bool32 noBuildings = false;
	LegoObject_TypeFlags objTypeFlags = OBJECT_TYPE_FLAG_BUILDING;
	if (LegoObject_CameraCycleFindNextUnitByFlags(objTypeFlags, &unitsExist))
		return; // New building added to cycle list and camera moved. We're done.


	// If no buildings are constructed, then cycle through minifigure unit types instead.
	if (!unitsExist) {//objectGlobs.cycleBuildingCount == 0) {
		// Change cycle unit type to minifigures.
		//noBuildings = true;
		objTypeFlags = OBJECT_TYPE_FLAG_MINIFIGURE;
		if (LegoObject_CameraCycleFindNextUnitByFlags(objTypeFlags, &unitsExist))
			return; // New minifigure added to cycle list and camera moved. We're done.
	}

	// If cycle-able units exist, then all valid units were already in the list.
	if (unitsExist) {//objectGlobs.cycleUnitCount != 0) {
		// Reset the cycle list, then jump to the first available unit of our cycle unit type (building or minifigure).

		// This behavior is used so that the cycle list is guaranteed to not repeat itself until all units
		// have been visited once. After all units have been visited, the list is started anew.
		//  (This effectively functions like a music shuffle, without the intention of shuffling.)

		// Note that the order of units may change if new valid units were created in the middle of a cycle,
		// This is because the order of the cycle is first and foremost based on where the object is in the listSet,
		// And the listSet is not intended to be ordered based on when a unit is added, as freed up slots earlier in
		// the list may be used instead.
		objectGlobs.cycleUnitCount = 0;
		objectGlobs.cycleBuildingCount = 0;

		// Find first unit to cycle to.
		if (LegoObject_CameraCycleFindNextUnitByFlags(objTypeFlags, nullptr))
			return; // First building/minifigure added to cycle list and camera moved. We're done.
	}
}

/// CUSTOM: Wrapper around LegoObject_Callback_CameraCycleFindUnitByFlags with extra limit handling.
bool LegoRR::LegoObject_CameraCycleFindNextUnitByFlags(LegoObject_TypeFlags objTypeFlags, OPTIONAL OUT bool* unitsExist)
{
	/// FIX APPLY: Handle cycleUnitCount limit here instead of in callback.
	///            This allows us to restart the cycle immediately, instead of after one use.
	if (objectGlobs.cycleUnitCount >= OBJECT_MAXCYCLEUNITS) {
		// If we've reached the maximum possible units to cycle through then restart the list.
		// We want to do this here so it's as early as possible in LegoObject_CameraCycleUnits,
		//  to allow checking for all unit types.
		objectGlobs.cycleUnitCount = 0;
		objectGlobs.cycleBuildingCount = 0;
	}

	if (unitsExist) *unitsExist = false;

	for (LegoObject* obj : objectListSet.EnumerateSkipUpgradeParts()) {
		if (LegoObject_Callback_CameraCycleFindUnitByFlags(obj, objTypeFlags, unitsExist))
			return true; // New unit added to cycle list and camera moved.
	}
	return false;
}

/// CUSTOM: Extension of LegoObject_Callback_CameraCycleFindUnit so that we can customize what objects are included in the cycle.
bool LegoRR::LegoObject_Callback_CameraCycleFindUnitByFlags(LegoObject* liveObj, LegoObject_TypeFlags objTypeFlags, OPTIONAL IN OUT bool* unitsExist)
{
	// Only check for units of a specific object type:
	if (!LegoObject_TypeFlagsHasType(objTypeFlags, liveObj->type))
		return false;

	// Don't check for units already in cycleUnits:
	for (uint32 i = 0; i < objectGlobs.cycleUnitCount; i++) {
		if (objectGlobs.cycleUnits[i] == liveObj) {
			if (unitsExist) *unitsExist = true;
			return false;
		}
	}

	if (objectGlobs.cycleUnitCount < OBJECT_MAXCYCLEUNITS) {
		if (unitsExist) *unitsExist = true;

		objectGlobs.cycleUnits[objectGlobs.cycleUnitCount++] = liveObj;
		if (liveObj->type == LegoObject_Building)
			objectGlobs.cycleBuildingCount++;

		Lego_Goto(liveObj, nullptr, false);

		// Move the topdown spotlight. Our cursor is over the panel interface, meaning it won't update the light movement.
		uint32 bx, by; // dummy outputs

		Vector3F wPos;
		if (Map3D_GetIntersections(legoGlobs.currLevel->map, legoGlobs.viewMain, Gods98::msx(), Gods98::msy(), &bx, &by, &wPos)) {
			Gods98::Container_SetPosition(legoGlobs.rootLight, nullptr, wPos.x, wPos.y, wPos.z - 250.0f);
			return true; // Object found and camera moved successfully.
		}
		/// TODO: Should we still return true on failure when an new unit is found?
	}
	/// FIX APPLY: Failure on limit hit/failed camera move. Part of handling for reaching the limit fix.
	return false;

	/*else {
		objectGlobs.cycleUnitCount = 0;
		objectGlobs.cycleBuildingCount = 0;
	}
	return true; // Is returning true here intentional?
	             // So cycle functionality breaks for one use after 256 building or minifigure units I guess...?
	*/
}

// <LegoRR.exe @0044c8b0>
bool32 __cdecl LegoRR::LegoObject_Callback_CameraCycleFindUnit(LegoObject* liveObj, OPTIONAL void* pNoBuildings)
{
	const bool32 noBuildings = (pNoBuildings != nullptr ? *(bool32*)pNoBuildings : false); // Default to false when NULL.

	LegoObject_TypeFlags objTypeFlags;
	if (noBuildings) objTypeFlags = OBJECT_TYPE_FLAG_MINIFIGURE;
	else             objTypeFlags = OBJECT_TYPE_FLAG_BUILDING;

	//if (noBuildings) {
	//	if (liveObj->type != LegoObject_MiniFigure) return false;
	//}
	//else {
	//	if (liveObj->type != LegoObject_Building) return false;
	//}

	return LegoObject_Callback_CameraCycleFindUnitByFlags(liveObj, objTypeFlags, nullptr);
}



/// CUSTOM: Starts the tickdown for dynamite or sonic blaster.
void LegoRR::LegoObject_StartTickDown(LegoObject* liveObj, bool showInfoMessage)
{
	if (liveObj->type == LegoObject_Dynamite || liveObj->type == LegoObject_OohScary) {
		//liveObj->flags1 &= ~LIVEOBJ1_PLACING;
		liveObj->flags3 |= LIVEOBJ3_UNK_10000;

		Gods98::Container_SetActivity(liveObj->other, "TickDown");
		Gods98::Container_SetAnimationTime(liveObj->other, 0.0f);

		Point2I blockPos = { 0 }; // dummy init
		LegoObject_GetBlockPos(liveObj, &blockPos.x, &blockPos.y);
		Level_Block_SetBusyFloor(&blockPos, true);
		
		if (showInfoMessage) {
			Info_Send(Info_DynamitePlaced, nullptr, liveObj, nullptr);
		}
	}
}

#pragma endregion
