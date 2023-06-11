// Vehicle.cpp : 
//

#include "../../engine/core/Config.h"
#include "../../engine/core/Maths.h"
#include "../../engine/core/Utils.h"
#include "../../engine/gfx/Activities.h"
#include "MeshLOD.h"
#include "Object.h"
#include "Upgrade.h"
#include "Weapons.h"

#include "Vehicle.h"


/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

 /// CUSTOM: Properly cleanup Container references to nulls.
void LegoRR::_Vehicle_RemoveNulls(VehicleModel* vehicle)
{
	log_firstcall();

	/// FIXME: Properly remove Container references that were being leaked.
	if (vehicle->drillNull) {
		Gods98::Container_RemoveReference(vehicle->drillNull);
		vehicle->drillNull = nullptr;
	}
	if (vehicle->depositNull) {
		Gods98::Container_RemoveReference(vehicle->depositNull);
		vehicle->depositNull = nullptr;
	}
	if (vehicle->driverNull) {
		Gods98::Container_RemoveReference(vehicle->driverNull);
		vehicle->driverNull = nullptr;
	}

	for (uint32 i = 0; i < VEHICLE_MAXCARRYS; i++) {
		if (vehicle->carryNulls[i]) {
			Gods98::Container_RemoveReference(vehicle->carryNulls[i]);
			vehicle->carryNulls[i] = nullptr;
		}
	}
	for (uint32 i = 0; i < VEHICLE_MAXCAMERAS; i++) {
		if (vehicle->cameraNulls[i]) {
			Gods98::Container_RemoveReference(vehicle->cameraNulls[i]);
			vehicle->cameraNulls[i] = nullptr;
		}
	}
}

/// CUSTOM: Properly cleanup Container references to nulls.
void LegoRR::_Vehicle_RemoveWeaponNulls(VehicleModel* vehicle)
{
	log_firstcall();

	/// FIXME: Properly remove Container references that were being leaked.
	/// CODE DUPLICATION: Identical to _Building_RemoveWeaponNulls (assuming we use WeaponsModel* as the parameter).
	for (uint32 i = 0; i < WEAPON_MAXWEAPONS; i++) {
		for (uint32 j = 0; j < WEAPON_MAXFIRES; j++) {
			if (vehicle->weapons.fireNullPairs[i][j]) {
				Gods98::Container_RemoveReference(vehicle->weapons.fireNullPairs[i][j]);
				vehicle->weapons.fireNullPairs[i][j] = nullptr;
			}
		}

		if (vehicle->weapons.xPivotNulls[i]) {
			Gods98::Container_RemoveReference(vehicle->weapons.xPivotNulls[i]);
			vehicle->weapons.xPivotNulls[i] = nullptr;
		}
		if (vehicle->weapons.yPivotNulls[i]) {
			Gods98::Container_RemoveReference(vehicle->weapons.yPivotNulls[i]);
			vehicle->weapons.yPivotNulls[i] = nullptr;
		}
	}
}

/// CUSTOM: Properly cleanup Container references to nulls.
void LegoRR::_Vehicle_RemoveWheelNulls(VehicleModel* vehicle)
{
	log_firstcall();

	/// FIXME: Properly remove Container references that were being leaked.
	for (uint32 i = 0; i < VEHICLE_MAXWHEELS; i++) {
		if (vehicle->wheelNulls[i]) {
			Gods98::Container_RemoveReference(vehicle->wheelNulls[i]);
			vehicle->wheelNulls[i] = nullptr;
		}
	}
}

// <missing>
uint32 __cdecl LegoRR::Vehicle_GetCameraNullFrames(VehicleModel* vehicle)
{
	return vehicle->cameraNullFrames;
}

// <missing>
uint32 __cdecl LegoRR::Vehicle_GetWheelNullFrames(VehicleModel* vehicle)
{
	return vehicle->wheelNullFrames;
}



// <LegoRR.exe @0046c690>
bool32 __cdecl LegoRR::Vehicle_IsCameraFlipDir(VehicleModel* vehicle)
{
	log_firstcall();

	return vehicle->cameraFlipDir == BoolTri::BOOL3_TRUE;
}

// <LegoRR.exe @0046c6b0>
bool32 __cdecl LegoRR::Vehicle_SetActivity(VehicleModel* vehicle, const char* activityName, real32 elapsed)
{
	log_firstcall();

	/// FIX APPLY: Remove Container references that were being leaked after every SetActivity.
	_Vehicle_RemoveNulls(vehicle);
	_Vehicle_RemoveWeaponNulls(vehicle);
	_Vehicle_RemoveWheelNulls(vehicle);

	MeshLOD_RemoveTargets(vehicle->polyMedium1);
	/// FIXME: Should we also be calling RemoveTargets for polyMedium2??
	if (vehicle->polyMedium2 != nullptr) {
		MeshLOD_RemoveTargets(vehicle->polyMedium2);
	}

	bool success2 = false;
	bool success1 = Gods98::Container_SetActivity(vehicle->contAct1, activityName);
	vehicle->flags &= ~VEHICLE_FLAG_NOACTIVITY1;

	if (!success1 && (vehicle->flags & VEHICLE_FLAG_HOLDMISSING)) {
		if (!Gods98::Container_IsCurrentActivity(vehicle->contAct1, activityName)) {
			vehicle->flags |= VEHICLE_FLAG_NOACTIVITY1;
		}
	}
	if (vehicle->contAct2 != nullptr) {
		success2 = Gods98::Container_SetActivity(vehicle->contAct2, activityName);
	}

	if (!(vehicle->flags & VEHICLE_FLAG_NOACTIVITY1)) {
		Gods98::Container_SetAnimationTime(vehicle->contAct1, elapsed);
	}
	if (vehicle->contAct2 != nullptr) {
		Gods98::Container_SetAnimationTime(vehicle->contAct2, elapsed);
	}
	Vehicle_PopulateWheelNulls(vehicle);
	Vehicle_SetUpgradeActivity(vehicle, activityName);
	return (success1 || success2);

}

// <LegoRR.exe @0046c7d0>
void __cdecl LegoRR::Vehicle_SetUpgradeActivity(VehicleModel* vehicle, const char* activityName)
{
	/// CODE DUPLICATION: Almost identical to _Building_SetUpgradeActivity.

	log_firstcall();

	/// FIX APPLY: Remove Container references that were being leaked after every SetUpgradeActivity.
	_Vehicle_RemoveWeaponNulls(vehicle);

	for (uint32 i = 0; i < WEAPON_MAXWEAPONS; i++) {
		//for (uint32 j = 0; j < WEAPON_MAXFIRES; j++) {
		//	vehicle->weapons.fireNullPairs[i][j] = nullptr;
		//}
		//vehicle->weapons.xPivotNulls[i] = nullptr;
		//vehicle->weapons.yPivotNulls[i] = nullptr;

		vehicle->weapons.weaponParts[i] = nullptr;
	}

	vehicle->weapons.weaponCount = 0;

	for (LegoObject* upg = vehicle->upgrades.firstObject; upg != nullptr; upg = upg->upgradePart->nextObject) {

		Upgrade_PartModel* upgradePart = upg->upgradePart;

		Gods98::Container* partParentCont = Vehicle_FindNull(vehicle, upgradePart->partInfo->nullObjectName, upgradePart->partInfo->nullFrameNo);
		Gods98::Container* partChildCont = Upgrade_Part_GetActivityContainer(upgradePart);
		Gods98::Container_SetParent(partChildCont, partParentCont);
		Gods98::Container_ClearTransform(partChildCont);
		if (activityName == nullptr) {
			// Sort-of inlined.
			//Gods98::Container_SetActivity(upgradePart->cont, objectGlobs.activityName[Activity_Stand]);
			Upgrade_Part_SetActivity(upgradePart, objectGlobs.activityName[Activity_Stand]);
		}
		else {
			Upgrade_Part_SetActivity(upgradePart, activityName);
		}

		if (upgradePart->partInfo->weaponName != nullptr) {
			// Shorthand to reduce line length. Sort-of a loop variable, it's just that
			//  the loop isn't guaranteed to be incremented each cycle in the outer loop.
			const uint32 i = vehicle->weapons.weaponCount;

			Error_FatalF(i == WEAPON_MAXWEAPONS,
						 "Vehicle (ID %i) has too many upgrade parts with weapons", static_cast<sint32>(vehicle->objID));

			vehicle->weapons.fireNullPairFrames[i] = 0;

			// Unlike Vehicle_FindNull, these always format the part name based on vehicle->contAct1, but maybe both names would be the same(?)

			for (uint32 j = 0; j < WEAPON_MAXFIRES; j++) {
				vehicle->weapons.fireNullPairs[i][j] = nullptr;

				const char* firePartName = Gods98::Container_FormatPartName(vehicle->contAct1, vehicle->fireNullName, &j);
				vehicle->weapons.fireNullPairs[i][j] = Gods98::Container_SearchTree(partChildCont, firePartName, Gods98::Container_SearchMode::FirstMatch, nullptr);

				/// TODO: Yes, this is assigned before the null check. But is that what *should* happen?
				vehicle->weapons.fireNullPairFrames[i]++;
				if (vehicle->weapons.fireNullPairs[i][j] == nullptr) {
					break;
				}
			}

			const uint32 zero = 0;

			const char* xPartName = Gods98::Container_FormatPartName(vehicle->contAct1, vehicle->xPivot, &zero);
			vehicle->weapons.xPivotNulls[i] = Gods98::Container_SearchTree(partChildCont, xPartName, Gods98::Container_SearchMode::FirstMatch, nullptr);

			const char* yPartName = Gods98::Container_FormatPartName(vehicle->contAct1, vehicle->yPivot, &zero);
			vehicle->weapons.yPivotNulls[i] = Gods98::Container_SearchTree(partChildCont, yPartName, Gods98::Container_SearchMode::FirstMatch, nullptr);

			vehicle->weapons.weaponParts[i] = upgradePart->partInfo;
			vehicle->weapons.weaponCount++;
		}
	}
}

// <LegoRR.exe @0046c9b0>
bool32 __cdecl LegoRR::Vehicle_Load(OUT VehicleModel* vehicle, LegoObject_ID objID, Gods98::Container* root, const char* filename, const char* gameName)
{
	std::memset(vehicle, 0, sizeof(VehicleModel));

	//char* pathParts[100];
	//char baseDir[1024];
	char fullNameBuff[1024];

	char* actFileNames[2 + 1]; // +1 for proper error handling of more than the expected amount of commas.
	char inputBuff[1024];
	std::strcpy(inputBuff, filename); // Copy filename to a buffer so we can modify the string.
	uint32 numParts = Gods98::Util_TokeniseSafe(inputBuff, actFileNames, ",", _countof(actFileNames));

	vehicle->contAct1 = Gods98::Container_Load(root, actFileNames[0], "ACT", true);
	if (vehicle->contAct1 != nullptr) {
		if (numParts == 2) {
			vehicle->contAct2 = Gods98::Container_Load(vehicle->contAct1, actFileNames[1], "ACT", true);
		}
		else {
			vehicle->contAct2 = nullptr;
			Error_Warn((numParts != 1), "Vehicle definition must have either 0 or 1 commas!");
		}

		// Get the filename from the path so we can build our desired path: e.g. "path\to\myObject\myObject.ae"
		// Note that Container_Load already does this for "ACT" type files,
		//  but we need the path for things not handled by Container.
		const char* name = actFileNames[0];
		for (const char* s = name; *s != '\0'; s++) {
			if (*s == '\\') name = s + 1;
		}
		std::sprintf(fullNameBuff, "%s\\%s.%s", actFileNames[0], name, ACTIVITY_FILESUFFIX);
		/// REFACTOR: We don't need all this extra logic just to find the filename.
		//std::strcpy(baseDir, actFileNames[0]);
		//numParts = Gods98::Util_TokeniseSafe(baseDir, pathParts, "\\", _countof(pathParts));
		//for (uint32 i = 1; i < numParts; i++) {
		//	pathParts[i][-1] = '\\'; // Restore separator between parts
		//}
		//std::sprintf(fullNameBuff, "%s\\%s.%s", baseDir, pathParts[numParts - 1], ACTIVITY_FILESUFFIX);

		Gods98::Config* config1 = Gods98::Config_Load(fullNameBuff);
		if (config1 != nullptr) {

			vehicle->cameraNullName = Gods98::Config_GetStringValue(config1, Config_ID(gameName, "CameraNullName"));
			if (vehicle->cameraNullName == nullptr) {
				vehicle->cameraNullFrames = 0;
			}
			else {
				vehicle->cameraNullFrames = Config_GetIntValue(config1, Config_ID(gameName, "CameraNullFrames"));
				vehicle->cameraFlipDir = Gods98::Config_GetBoolValue(config1, Config_ID(gameName, "CameraFlipDir"));
			}

			vehicle->carryNullName = Gods98::Config_GetStringValue(config1, Config_ID(gameName, "CarryNullName"));
			if (vehicle->carryNullName == nullptr) {
				vehicle->carryNullFrames = 0;
			}
			else {
				vehicle->carryNullFrames = Config_GetIntValue(config1, Config_ID(gameName, "CarryNullFrames"));
			}

			vehicle->drillNullName = Gods98::Config_GetStringValue(config1, Config_ID(gameName, "DrillNullName"));
			vehicle->depositNullName = Gods98::Config_GetStringValue(config1, Config_ID(gameName, "DepositNullName"));
			vehicle->driverNullName = Gods98::Config_GetStringValue(config1, Config_ID(gameName, "DriverNullName"));
			vehicle->fireNullName = Gods98::Config_GetStringValue(config1, Config_ID(gameName, "FireNullName"));

			vehicle->yPivot = Gods98::Config_GetStringValue(config1, Config_ID(gameName, "yPivot"));
			vehicle->xPivot = Gods98::Config_GetStringValue(config1, Config_ID(gameName, "xPivot"));

			if (Gods98::Config_GetTempStringValue(config1, Config_ID(gameName, "PivotMaxZ")) == nullptr) {
				// 0.0f is a valid value, so we need to properly check for the property's existence.
				vehicle->weapons.pivotMaxZ = 1.0f;
			}
			else {
				vehicle->weapons.pivotMaxZ = Config_GetRealValue(config1, Config_ID(gameName, "PivotMaxZ"));
			}
			if (Gods98::Config_GetTempStringValue(config1, Config_ID(gameName, "PivotMinZ")) == nullptr) {
				// 0.0f is a valid value, so we need to properly check for the property's existence.
				vehicle->weapons.pivotMinZ = -1.0f;
			}
			else {
				vehicle->weapons.pivotMinZ = Config_GetRealValue(config1, Config_ID(gameName, "PivotMinZ"));
			}
			
			Upgrade_Load(&vehicle->upgrades, config1, gameName);

			// The game expects all vehicles to have wheels defined for them, even if none are visible.
			const char* wheelMeshName = Gods98::Config_GetTempStringValue(config1, Config_ID(gameName, "WheelMesh"));
			if (wheelMeshName != nullptr) {
				std::sprintf(fullNameBuff, "%s\\%s", actFileNames[0], wheelMeshName);
				//std::sprintf(fullNameBuff, "%s\\%s", baseDir, wheelMeshName);

				vehicle->wheelNullName = Gods98::Config_GetStringValue(config1, Config_ID(gameName, "WheelNullName"));
				if (vehicle->wheelNullName != nullptr) {
					for (uint32 i = 0; i < VEHICLE_MAXWHEELS; i++) {
						vehicle->contWheels[i] = nullptr;
						vehicle->wheelNulls[i] = nullptr;
					}

					vehicle->wheelNullFrames = 1;
					vehicle->contWheels[0] = Gods98::Container_Load(vehicle->contAct1, fullNameBuff, "LWO", false);
					if (vehicle->contWheels[0] == nullptr) {
						vehicle->wheelRadius = 0.0f;
					}
					else {
						vehicle->wheelRadius = Config_GetRealValue(config1, Config_ID(gameName, "WheelRadius"));
						if (vehicle->wheelRadius == 0.0f) {
							if (Gods98::Container_GetType(vehicle->contWheels[0]) == Gods98::Container_Type::Mesh) {
								// Calculate wheel radius using the mesh.
								Box3F box = {}; // dummy init
								Gods98::Container_Mesh_GetBox(vehicle->contWheels[0], &box);
								vehicle->wheelRadius = (box.max.y - box.min.y) / 2.0f; // Divide by 2 since radius is half of diameter.

								/// TODO: Should SetQuality really only be called when WheelRadius isn't defined???
								Gods98::Container_Mesh_SetQuality(vehicle->contWheels[0], 0, Gods98::Container_Quality::Gouraud);
							}
							else {
								// Use a default wheel radius value.
								vehicle->wheelRadius = 3.0f;
							}
						}
					}

					vehicle->polyMedium1 = LegoObject_LoadMeshLOD(config1, gameName, actFileNames[0], LOD_MediumPoly, 1);

					if (vehicle->contAct2 != nullptr) {
						// Get the filename from the path so we can build our desired path: e.g. "path\to\myObject\myObject.ae"
						// Note that Container_Load already does this for "ACT" type files,
						//  but we need the path for things not handled by Container.
						name = actFileNames[1];
						for (const char* s = name; *s != '\0'; s++) {
							if (*s == '\\') name = s + 1;
						}
						std::sprintf(fullNameBuff, "%s\\%s.%s", actFileNames[1], name, ACTIVITY_FILESUFFIX);
						/// REFACTOR: We don't need all this extra logic just to find the filename.
						//std::strcpy(baseDir, actFileNames[1]);
						//numParts = Gods98::Util_TokeniseSafe(baseDir, pathParts, "\\", _countof(pathParts));
						//for (uint32 i = 1; i < numParts; i++) {
						//	pathParts[i][-1] = '\\'; // Restore separator between parts
						//}
						//std::sprintf(fullNameBuff, "%s\\%s.%s", baseDir, pathParts[numParts - 1], ACTIVITY_FILESUFFIX);

						Gods98::Config* config2 = Gods98::Config_Load(fullNameBuff);
						if (config2 != nullptr) {
							vehicle->polyMedium2 = LegoObject_LoadMeshLOD(config2, gameName, actFileNames[1], LOD_MediumPoly, 1);

							/// FIX APPLY: Free activity2 config file!!!
							Gods98::Config_Free(config2);
						}
					}

					vehicle->flags = VEHICLE_FLAG_SOURCE;
					if (Config_GetBoolOrFalse(config1, Config_ID(gameName, "HoldMissing"))) {
						vehicle->flags |= VEHICLE_FLAG_HOLDMISSING;
					}

					vehicle->wheelLastUp = Vector3F { 0.0f, 0.0f, -1.0f };
					vehicle->objID = objID;
					Gods98::Config_Free(config1);
					return true;
				}
				/// TODO: Free WheelNullName allocated string on failure.
			}
			/// TODO: Free all allocated strings on failure.
			/// TODO: We need a function to free the upgrade model on failure.
			Gods98::Config_Free(config1);
		}
		/// TODO: Remove container on failure.
	}
	return false;
}

// Similar to `Vehicle_Load`, this does not free the passed pointer.
// <LegoRR.exe @0046d0d0>
void __cdecl LegoRR::Vehicle_Remove(VehicleModel* vehicle)
{
	log_firstcall();

	// Remove upgrade model part objects.
	Vehicle_SetUpgradeLevel(vehicle, -1);

	// Remove MeshLODs.
	/// FIX APPLY: Free MeshLODs.
	MeshLOD_Free(vehicle->polyMedium1);
	vehicle->polyMedium1 = nullptr;

	if (vehicle->polyMedium2 != nullptr) {
		MeshLOD_Free(vehicle->polyMedium2);
		vehicle->polyMedium2 = nullptr;
	}

	// Remove source string allocations.
	if (vehicle->flags & VEHICLE_FLAG_SOURCE) {
		/// FIX APPLY: Free depositNullName, driverNullName, yPivot, xPivot, and carryNullName.
		if (vehicle->wheelNullName) {
			Gods98::Mem_Free(vehicle->wheelNullName);
			vehicle->wheelNullName = nullptr;
		}

		if (vehicle->drillNullName) {
			Gods98::Mem_Free(vehicle->drillNullName);
			vehicle->drillNullName = nullptr;
		}
		if (vehicle->depositNullName) {
			Gods98::Mem_Free(vehicle->depositNullName);
			vehicle->depositNullName = nullptr;
		}
		if (vehicle->fireNullName) {
			Gods98::Mem_Free(vehicle->fireNullName);
			vehicle->fireNullName = nullptr;
		}
		if (vehicle->driverNullName) {
			Gods98::Mem_Free(vehicle->driverNullName);
			vehicle->driverNullName = nullptr;
		}

		if (vehicle->yPivot) {
			Gods98::Mem_Free(vehicle->yPivot);
			vehicle->yPivot = nullptr;
		}
		if (vehicle->xPivot) {
			Gods98::Mem_Free(vehicle->xPivot);
			vehicle->xPivot = nullptr;
		}

		if (vehicle->carryNullName) {
			Gods98::Mem_Free(vehicle->carryNullName);
			vehicle->carryNullName = nullptr;
		}
		if (vehicle->cameraNullName) {
			Gods98::Mem_Free(vehicle->cameraNullName);
			vehicle->cameraNullName = nullptr;
		}
	}

	// Remove null containers.
	/// FIX APPLY: Properly remove Container references that were being leaked.
	_Vehicle_RemoveNulls(vehicle);
	_Vehicle_RemoveWeaponNulls(vehicle);
	_Vehicle_RemoveWheelNulls(vehicle);

	// Remove wheel mesh containers.
	for (uint32 i = 0; i < VEHICLE_MAXWHEELS; i++) {
		// Note: Removing wheel nulls handled by _Vehicle_RemoveWheelNulls.
		/// CHANGE: Should probably be removing wheel nulls before the container they're likely referencing.
		//if (vehicle->wheelNulls[i]) {
		//	Gods98::Container_Remove(vehicle->wheelNulls[i]);
		//	vehicle->wheelNulls[i] = nullptr;
		//}

		if (vehicle->contWheels[i]) {
			Gods98::Container_Remove(vehicle->contWheels[i]);
			vehicle->contWheels[i] = nullptr;
		}
	}

	// Remove activity containers.
	Gods98::Container_Remove(vehicle->contAct1);
	vehicle->contAct1 = nullptr;

	if (vehicle->contAct2 != nullptr) {
		Gods98::Container_Remove(vehicle->contAct2);
		vehicle->contAct2 = nullptr;
	}
}

// <LegoRR.exe @0046d190>
void __cdecl LegoRR::Vehicle_SwapPolyMedium(VehicleModel* vehicle, bool32 swap)
{
	log_firstcall();

	MeshLOD_SwapTarget(vehicle->polyMedium1, vehicle->contAct1, !swap, 0);
	if (vehicle->contAct2 != nullptr && vehicle->polyMedium2 != nullptr)
	{
		MeshLOD_SwapTarget(vehicle->polyMedium2, vehicle->contAct2, !swap, 0);
	}
}

// <LegoRR.exe @0046d1e0>
void __cdecl LegoRR::Vehicle_SwapPolyHigh(VehicleModel* vehicle, bool32 swap)
{
	log_firstcall();

	Vehicle_SwapPolyMedium(vehicle, swap);
}

// <LegoRR.exe @0046d200>
bool32 __cdecl LegoRR::Vehicle_CanUpgradeType(VehicleModel* vehicle, LegoObject_UpgradeType upgradeType, bool32 current)
{
	log_firstcall();

	const LegoObject_UpgradeFlags mask = LegoObject_UpgradeToFlag(upgradeType);

	if (current && (Vehicle_GetUpgradeLevel(vehicle) & mask)) {
		// This type is currently upgraded, can't upgrade again.
		return false;
	}
	// Fallback to checking upgrade capabilities, if not currently upgraded.
	return (vehicle->upgrades.levelFlags & mask);
}

// <LegoRR.exe @0046d240>
uint32 __cdecl LegoRR::Vehicle_GetUpgradeLevel(VehicleModel* vehicle)
{
	log_firstcall();

	return vehicle->upgrades.currentLevel;
}

// <LegoRR.exe @0046d250>
void __cdecl LegoRR::Vehicle_SetUpgradeLevel(VehicleModel* vehicle, uint32 objLevel)
{
	log_firstcall();

	/// FIX APPLY: Remove nulls before they can be changed by Upgrade_SetUpgradeLevel.
	_Vehicle_RemoveNulls(vehicle);
	_Vehicle_RemoveWeaponNulls(vehicle);
	// Don't remove wheels, bad!
	//_Vehicle_RemoveWheelNulls(vehicle);

	Upgrade_SetUpgradeLevel(&vehicle->upgrades, objLevel);
	Vehicle_SetUpgradeActivity(vehicle, nullptr);
}

// <LegoRR.exe @0046d280>
void __cdecl LegoRR::Vehicle_HideWheels(VehicleModel* vehicle, bool32 hide)
{
	log_firstcall();

	for (uint32 i = 0; i < VEHICLE_MAXWHEELS; i++) {
		Gods98::Container_Hide(vehicle->contWheels[i], hide);
	}
}

// <LegoRR.exe @0046d2b0>
bool32 __cdecl LegoRR::Vehicle_Clone(IN VehicleModel* srcVehicle, OUT VehicleModel* destVehicle)
{
	log_firstcall();

	// Directly copy data from source model.
	*destVehicle = *srcVehicle;

	// Dissociate nulls from source model.
	/// FIX APPLY: We don't want to be referencing the nulls of another container.
	destVehicle->drillNull   = nullptr;
	destVehicle->depositNull = nullptr;
	destVehicle->driverNull  = nullptr;

	for (uint32 i = 0; i < VEHICLE_MAXCARRYS; i++) {
		destVehicle->carryNulls[i] = nullptr;
	}
	for (uint32 i = 0; i < VEHICLE_MAXCAMERAS; i++) {
		destVehicle->cameraNulls[i] = nullptr;
	}

	for (uint32 i = 0; i < WEAPON_MAXWEAPONS; i++) {
		for (uint32 j = 0; j < WEAPON_MAXFIRES; j++) {
			destVehicle->weapons.fireNullPairs[i][j] = nullptr;
		}
		destVehicle->weapons.xPivotNulls[i] = nullptr;
		destVehicle->weapons.yPivotNulls[i] = nullptr;

		destVehicle->weapons.weaponParts[i] = nullptr;
	}

	// This is a cloned model, and doesn't own string memory allocations.
	destVehicle->flags &= ~VEHICLE_FLAG_SOURCE;

	// Clone activity containers.
	destVehicle->contAct1 = Gods98::Container_Clone(srcVehicle->contAct1);
	if (destVehicle->contAct1 != nullptr) {
		// Clone and parent second activity container.
		if (srcVehicle->contAct2 != nullptr) {
			destVehicle->contAct2 = Gods98::Container_Clone(srcVehicle->contAct2);
			Gods98::Container_SetParent(destVehicle->contAct2, destVehicle->contAct1);
			Gods98::Container_SetPosition(destVehicle->contAct2, destVehicle->contAct1, 0.0f, 0.0f, 0.0f);
			Gods98::Container_SetOrientation(destVehicle->contAct2, destVehicle->contAct1, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
			Gods98::Container_SetPerspectiveCorrection(destVehicle->contAct2, true);
		}
		/// REFACTOR: Moved above MeshLOD_Clone.
		Gods98::Container_SetPerspectiveCorrection(destVehicle->contAct1, true);

		// Clone MeshLODs.
		destVehicle->polyMedium1 = MeshLOD_Clone(srcVehicle->polyMedium1);
		if (srcVehicle->polyMedium2 != nullptr) {
			destVehicle->polyMedium2 = MeshLOD_Clone(srcVehicle->polyMedium2);
		}

		// Clone wheel mesh containers.
		Vehicle_PopulateWheelNulls(destVehicle);
		/// TODO: Should we be cloning and parenting contWheels before populating Nulls!??

		for (uint32 i = 0; i < VEHICLE_MAXWHEELS; i++) {
			destVehicle->contWheels[i] = nullptr;
		}
		/// JANK: Source vehicle models can never be used directly because only the first wheel is ever assigned.
		///       This is also why cloning is passing srcVehicle->contWheels[0], but assigning to index i.
		if (srcVehicle->contWheels[0] != nullptr) {
			/// TODO: Does cloning more than the first contWheel have any effect?
			for (uint32 i = 0; i < destVehicle->wheelNullFrames; i++) {
				destVehicle->contWheels[i] = Gods98::Container_Clone(srcVehicle->contWheels[0]);
				if (destVehicle->contWheels[i] != nullptr) {
					Gods98::Container_SetParent(destVehicle->contWheels[i], destVehicle->contAct1);
					Gods98::Container_SetPerspectiveCorrection(destVehicle->contWheels[i], true);
				}
			}
		}

		return true;
	}
	return false;
}

// <LegoRR.exe @0046d400>
void __cdecl LegoRR::Vehicle_SetOwnerObject(VehicleModel* vehicle, LegoObject* liveObj)
{
	log_firstcall();

	Gods98::Container_SetUserData(vehicle->contAct1, liveObj);
	if (vehicle->contAct2 != nullptr)
	{
		Gods98::Container_SetUserData(vehicle->contAct2, liveObj);
	}

	for (uint32 i = 0; i < vehicle->wheelNullFrames; i++)
	{
		if (vehicle->contWheels[i] != nullptr)
		{
			Gods98::Container_SetUserData(vehicle->contWheels[i], liveObj);
		}
	}
}

// <LegoRR.exe @0046d460>
real32 __cdecl LegoRR::Vehicle_GetAnimationTime(VehicleModel* vehicle)
{
	log_firstcall();

	return Gods98::Container_GetAnimationTime(vehicle->contAct1);
}

// <LegoRR.exe @0046d480>
real32 __cdecl LegoRR::Vehicle_MoveAnimation(VehicleModel* vehicle, real32 elapsed1, real32 elapsed2, uint32 repeatCount)
{
	log_firstcall();

	real32 overrun = 0.0f;

	if (!(vehicle->flags & VehicleFlags::VEHICLE_FLAG_NOACTIVITY1))
	{
		overrun = Gods98::Container_MoveAnimation(vehicle->contAct1, elapsed1);
	}

	if (vehicle->contAct2 != nullptr)
	{
		overrun = Gods98::Container_MoveAnimation(vehicle->contAct2, elapsed2);
	}

	if (repeatCount > 1 && overrun != 0.0f)
	{
		/// TODO: Should we only be getting the anim frame count from contAct1?
		///       What if VEHICLE_FLAG_NOACTIVITY1 is set? 
		const uint32 animFrames = Gods98::Container_GetAnimationFrames(vehicle->contAct1);
		overrun -= static_cast<real32>(animFrames * (repeatCount - 1));
	}

	return overrun;
}

// <LegoRR.exe @0046d520>
void __cdecl LegoRR::Vehicle_PopulateWheelNulls(VehicleModel* vehicle)
{
	log_firstcall();

	_Vehicle_RemoveWheelNulls(vehicle);

	vehicle->wheelNullFrames = 0;

	for (uint32 i = 0; i < VEHICLE_MAXWHEELS; i++) {
		const char* partName = Gods98::Container_FormatPartName(vehicle->contAct1, vehicle->wheelNullName, &i);
		vehicle->wheelNulls[i] = Gods98::Container_SearchTree(vehicle->contAct1, partName, Gods98::Container_SearchMode::FirstMatch, nullptr);
		if (vehicle->wheelNulls[i] != nullptr) {
			vehicle->wheelNullFrames++;
		}
	}
}

// <LegoRR.exe @0046d580>
void __cdecl LegoRR::Vehicle_Hide(VehicleModel* vehicle, bool32 hide)
{
	log_firstcall();

	if (hide)
	{
		vehicle->flags |= VehicleFlags::VEHICLE_FLAG_HIDDEN;
	}
	else
	{
		vehicle->flags &= ~VehicleFlags::VEHICLE_FLAG_HIDDEN;
	}

	Gods98::Container_Hide(vehicle->contAct1, hide);
	if (vehicle->contAct2 != nullptr)
	{
		Gods98::Container_Hide(vehicle->contAct2, hide);
	}

	for (uint32 i = 0; i < vehicle->wheelNullFrames; i++)
	{
		if (vehicle->contWheels[i] != nullptr)
		{
			Gods98::Container_Hide(vehicle->contWheels[i], hide);
		}
	}
}

// <LegoRR.exe @0046d5f0>
bool32 __cdecl LegoRR::Vehicle_IsHidden(VehicleModel* vehicle)
{
	log_firstcall();

	return (bool)(vehicle->flags & VehicleFlags::VEHICLE_FLAG_HIDDEN);
}

// <LegoRR.exe @0046d610>
void __cdecl LegoRR::Vehicle_SetOrientation(VehicleModel* vehicle, real32 xDir, real32 yDir, real32 zDir)
{
	log_firstcall();

	/// TODO: Should we be setting the up argument to negative wheelLastUp? (if wheelNullFrames != 0)
	Gods98::Container_SetOrientation(vehicle->contAct1, nullptr, xDir, yDir, zDir, 0.0f, 0.0f, -1.0f);
}

// <LegoRR.exe @0046d640>
void __cdecl LegoRR::Vehicle_SetPosition(VehicleModel* vehicle, real32 xPos, real32 yPos, GetWorldZCallback zCallback, Map3D* map)
{
	log_firstcall();

	Gods98::Container_SetPosition(vehicle->contAct1, nullptr, xPos, yPos, zCallback(xPos, yPos, map));

	Vector3F direction;
	Gods98::Container_GetOrientation(vehicle->contAct1, nullptr, &direction, nullptr);
	Gods98::Container_SetOrientation(vehicle->contAct1, nullptr, direction.x, direction.y, direction.z, 0.0f, 0.0f, -1.0f);

	if (vehicle->wheelNullFrames == 0)
	{
		return; // No wheels, no more positioning to do.
	}

	std::array<Vector3F, VEHICLE_MAXWHEELS> wheelPositions {}; // dummy init
	for (uint32 i = 0; i < vehicle->wheelNullFrames; i++)
	{
		Gods98::Container_GetPosition(vehicle->wheelNulls[i], nullptr, &wheelPositions[i]);
		wheelPositions[i].z = zCallback(wheelPositions[i].x, wheelPositions[i].y, map);
	}

	// Update vehicle upwards orientation based on wheel positions.

	/// TODO: Consider supporting vehicles with only 2 (or even 3?) wheels.
	///       Currently the 4 outer wheels are required to calculate the new up orientation.

	Vector3F planeA, planeB, up;
	//Vector3F tempA, tempB;
	//Gods98::Maths_Vector3DSubtract(&tempA, &wheelPositions[1], &wheelPositions[0]);
	//Gods98::Maths_Vector3DSubtract(&tempB, &wheelPositions[1], &wheelPositions[2]);
	//Gods98::Maths_Vector3DCrossProduct(&planeA, &tempA, &tempB);
	//Gods98::Maths_Vector3DNormalize(&planeA);
	//Gods98::Maths_Vector3DSubtract(&tempB, &wheelPositions[3], &wheelPositions[0]);
	//Gods98::Maths_Vector3DSubtract(&tempA, &wheelPositions[3], &wheelPositions[2]);
	//Gods98::Maths_Vector3DCrossProduct(&planeB, &tempA, &tempB);
	//Gods98::Maths_Vector3DNormalize(&planeB);

	// The above Vector3D calculations can be substituted with these PlaneNormal functions.
	// Note that the above calculations perform subtraction like so:
	//   Subtract(p2, p1)
	//   Subtract(p2, p3)
	// However PlaneNormal performs subtraction like so:
	//   Subtract(p2, p1)
	//   Subtract(p3, p2)
	// Interestingly, all that needs to be changed for the function to give the same output
	//  is to swap the p1 and p2 arguments. Note that this does cause a slight discrepancy
	//  in the final floating point values, as is normal for floats.
	Gods98::Maths_PlaneNormal(&planeA, &wheelPositions[1], &wheelPositions[0], &wheelPositions[2]);
	Gods98::Maths_PlaneNormal(&planeB, &wheelPositions[3], &wheelPositions[2], &wheelPositions[0]);

	Gods98::Maths_Vector3DAdd(&up, &planeA, &planeB);
	Gods98::Maths_Vector3DNormalize(&up);

	// Transition between the last up orientation (weight x0.4) and the new up orientation (weight x1.0).
	if (!std::isfinite(vehicle->wheelLastUp.x) || !std::isfinite(vehicle->wheelLastUp.y) || !std::isfinite(vehicle->wheelLastUp.z))
	{
		// Last wheel up orientation is invalid. Instantly change to new up orientation instead.
		vehicle->wheelLastUp = up;
	}

	Gods98::Maths_RayEndPoint(&up, &up, &vehicle->wheelLastUp, 0.4f);

	Gods98::Maths_Vector3DNormalize(&up);
	vehicle->wheelLastUp = up;

	const real32 dot = Gods98::Maths_Vector3DDotProduct(&direction, &up);
	Gods98::Maths_RayEndPoint(&direction, &direction, &up, -dot);

	Gods98::Container_SetOrientation(vehicle->contAct1, nullptr, direction.x, direction.y, direction.z, -up.x, -up.y, -up.z);


	// Update wheel positions (suspension) and rotations (turning).
	for (uint32 i = 0; i < vehicle->wheelNullFrames; i++)
	{
		if (vehicle->contWheels[i] != nullptr)
		{
			// Center the wheel's z position so that the bottom of the wheel is touching the ground.
			Gods98::Container_GetPosition(vehicle->wheelNulls[i], nullptr, &wheelPositions[i]);
			const real32 zWheel = zCallback(wheelPositions[i].x, wheelPositions[i].y, map);
			const real32 yTranslate = -(zWheel - wheelPositions[i].z - vehicle->wheelRadius);
			Gods98::Container_AddTranslation(vehicle->wheelNulls[i], Gods98::Container_Combine::After, 0.0f, yTranslate, 0.0f);
			Gods98::Container_SetPosition(vehicle->contWheels[i], vehicle->wheelNulls[i], 0.0f, 0.0f, 0.0f);

			// Calculate how much the wheel should turn based on the change from wheelLastPositions and wheelRadius.
			Vector3F temp;
			Gods98::Container_GetPosition(vehicle->contWheels[i], nullptr, &wheelPositions[i]);
			Gods98::Maths_Vector3DSubtract(&temp, &wheelPositions[i], &vehicle->wheelLastPositions[i]);
			real32 angle = Gods98::Maths_Vector3DModulus(&temp) / vehicle->wheelRadius;
			if (Gods98::Maths_Vector3DDotProduct(&temp, &direction) < 0.0f)
			{
				angle = -angle;
			}
			Gods98::Container_AddRotation(vehicle->contWheels[i], Gods98::Container_Combine::Before, 1.0f, 0.0f, 0.0f, angle);
			Gods98::Container_GetPosition(vehicle->contWheels[i], nullptr, &vehicle->wheelLastPositions[i]);
		}
	}
}

// <LegoRR.exe @0046dca0>
Gods98::Container* __cdecl LegoRR::Vehicle_GetActivityContainer(VehicleModel* vehicle)
{
	log_firstcall();

	return vehicle->contAct1;
}

// <LegoRR.exe @0046dcb0>
Gods98::Container* __cdecl LegoRR::Vehicle_FindNull(VehicleModel* vehicle, char* name, uint32 frameNo)
{
	log_firstcall();

	Gods98::Container* container = nullptr;
	if (vehicle->contAct2 != nullptr)
	{
		char const *partName = Gods98::Container_FormatPartName(vehicle->contAct2, name, &frameNo);
		container = Gods98::Container_SearchTree(vehicle->contAct2, partName, Gods98::Container_SearchMode::FirstMatch, nullptr);
	}

	if (container == nullptr)
	{
		char const* partName = Gods98::Container_FormatPartName(vehicle->contAct1, name, &frameNo);
		container = Gods98::Container_SearchTree(vehicle->contAct1, partName, Gods98::Container_SearchMode::FirstMatch, nullptr);
	}

	return container;
}

// <LegoRR.exe @0046dd10>
Gods98::Container* __cdecl LegoRR::Vehicle_GetCameraNull(VehicleModel* vehicle, uint32 frameNo)
{
	log_firstcall();

	if (vehicle->cameraNullName != nullptr) {
		if (vehicle->cameraNulls[frameNo] == nullptr) {
			vehicle->cameraNulls[frameNo] = Vehicle_FindNull(vehicle, vehicle->cameraNullName, frameNo);
		}
		return vehicle->cameraNulls[frameNo];
	}
	return nullptr;
}

// <LegoRR.exe @0046dd50>
Gods98::Container* __cdecl LegoRR::Vehicle_GetDrillNull(VehicleModel* vehicle)
{
	log_firstcall();

	if (vehicle->drillNullName != nullptr) {
		if (vehicle->drillNull == nullptr) {
			vehicle->drillNull = Vehicle_FindNull(vehicle, vehicle->drillNullName, 0);
		}
		return vehicle->drillNull;
	}
	return nullptr;
}

// <LegoRR.exe @0046dd80>
Gods98::Container* __cdecl LegoRR::Vehicle_GetDepositNull(VehicleModel* vehicle)
{
	log_firstcall();

	if (vehicle->depositNullName != nullptr) {
		if (vehicle->depositNull == nullptr) {
			vehicle->depositNull = Vehicle_FindNull(vehicle, vehicle->depositNullName, 0);
		}
		return vehicle->depositNull;
	}
	return nullptr;
}

// <LegoRR.exe @0046ddb0>
Gods98::Container* __cdecl LegoRR::Vehicle_GetDriverNull(VehicleModel* vehicle)
{
	log_firstcall();

	if (vehicle->driverNullName != nullptr) {
		if (vehicle->driverNull == nullptr) {
			vehicle->driverNull = Vehicle_FindNull(vehicle, vehicle->driverNullName, 0);
		}
		return vehicle->driverNull;
	}
	return nullptr;
}

// <LegoRR.exe @0046dde0>
Gods98::Container* __cdecl LegoRR::Vehicle_GetCarryNull(VehicleModel* vehicle, uint32 frameNo)
{
	log_firstcall();

	if (vehicle->carryNullName != nullptr) {
		if (vehicle->carryNulls[frameNo] == nullptr) {
			vehicle->carryNulls[frameNo] = Vehicle_FindNull(vehicle, vehicle->carryNullName, frameNo);
		}
		return vehicle->carryNulls[frameNo];
	}
	return nullptr;
}

// <LegoRR.exe @0046de20>
uint32 __cdecl LegoRR::Vehicle_GetCarryNullFrames(VehicleModel* vehicle)
{
	log_firstcall();

	return vehicle->carryNullFrames;
}

// <LegoRR.exe @0046de30>
real32 __cdecl LegoRR::Vehicle_GetTransCoef(VehicleModel* vehicle)
{
	log_firstcall();

	return Gods98::Container_GetTransCoef(vehicle->contAct1);
}

#pragma endregion
