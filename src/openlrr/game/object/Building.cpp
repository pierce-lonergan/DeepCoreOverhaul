// Building.cpp : 
//

#include "Object.h"
#include "Upgrade.h"
#include "Weapons.h"

#include "Building.h"


/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

/// CUSTOM: Properly cleanup Container reference items.
void LegoRR::_Building_RemoveNulls(BuildingModel* building)
{
	/// FIXME: Properly remove Container references that were being leaked.
	if (building->depositNull) {
		Gods98::Container_RemoveReference(building->depositNull);
		building->depositNull = nullptr;
	}
	if (building->entranceNull) {
		Gods98::Container_RemoveReference(building->entranceNull);
		building->entranceNull = nullptr;
	}

	for (uint32 i = 0; i < BUILDING_MAXCARRYS; i++) {
		if (building->carryNulls[i]) {
			Gods98::Container_RemoveReference(building->carryNulls[i]);
			building->carryNulls[i] = nullptr;
		}
	}
	for (uint32 i = 0; i < BUILDING_MAXCAMERAS; i++) {
		if (building->cameraNulls[i]) {
			Gods98::Container_RemoveReference(building->cameraNulls[i]);
			building->cameraNulls[i] = nullptr;
		}
	}
	for (uint32 i = 0; i < BUILDING_MAXTOOLS; i++) {
		if (building->toolNulls[i]) {
			Gods98::Container_RemoveReference(building->toolNulls[i]);
			building->toolNulls[i] = nullptr;
		}
	}
}

/// CUSTOM: Properly cleanup Container references to nulls.
void LegoRR::_Building_RemoveWeaponNulls(BuildingModel* building)
{
	/// FIXME: Properly remove Container references that were being leaked.
	/// CODE DUPLICATION: Identical to _Vehicle_RemoveWeaponNulls (assuming we use WeaponsModel* as the parameter).
	for (uint32 i = 0; i < WEAPON_MAXWEAPONS; i++) {
		for (uint32 j = 0; j < WEAPON_MAXFIRES; j++) {
			if (building->weapons.fireNullPairs[i][j]) {
				Gods98::Container_RemoveReference(building->weapons.fireNullPairs[i][j]);
				building->weapons.fireNullPairs[i][j] = nullptr;
			}
		}

		if (building->weapons.xPivotNulls[i]) {
			Gods98::Container_RemoveReference(building->weapons.xPivotNulls[i]);
			building->weapons.xPivotNulls[i] = nullptr;
		}
		if (building->weapons.yPivotNulls[i]) {
			Gods98::Container_RemoveReference(building->weapons.yPivotNulls[i]);
			building->weapons.yPivotNulls[i] = nullptr;
		}
	}
}



// Merged function: Object_GetActivityContainer
// <LegoRR.exe @00406d60>
Gods98::Container* __cdecl LegoRR::Building_GetActivityContainer(BuildingModel* building)
{
	return building->contAct;
}

// Merged function: Object_FindNull
// <LegoRR.exe @00406e80>
Gods98::Container* __cdecl LegoRR::Building_FindNull(BuildingModel* building, const char* name, uint32 frameNo)
{
	const char* partName = Gods98::Container_FormatPartName(building->contAct, name, &frameNo);
	return Gods98::Container_SearchTree(building->contAct, partName, Gods98::Container_SearchMode::FirstMatch, nullptr);
}

// Merged function: Object_SetOwnerObject
// <LegoRR.exe @004082b0>
void __cdecl LegoRR::Building_SetOwnerObject(BuildingModel* building, LegoObject* liveObj)
{
	Gods98::Container_SetUserData(building->contAct, liveObj);
}

// Merged function: Object_IsHidden
// <LegoRR.exe @004085d0>
bool32 __cdecl LegoRR::Building_IsHidden(BuildingModel* building)
{
	return Gods98::Container_IsHidden(building->contAct);
}



// <LegoRR.exe @00407c90>
//bool32 __cdecl LegoRR::Building_Load(OUT BuildingModel* building, LegoObject_ID objID, Gods98::Container* root, const char* filename, const char* gameName);

// <LegoRR.exe @00408210>
void __cdecl LegoRR::Building_ChangePowerLevel(BuildingModel* building, bool32 increment)
{
	// Exponential curve, with the largest increment speed starting at the lowest time:
	//                     ______
	//   |            ____/      
	// S |        ___/           
	// p |     __/               
	// e |   _/                  
	// e |  /                    
	// d | /                     
	//   |/______________________
	//            T i m e        

	// P.Level -> Increment
	//     0.0 -> +0.10
	//     0.1 -> +0.09
	//     0.2 -> +0.08
	//     ...
	//     0.9 -> +0.01
	//     1.0 -> +0.00

	// The PowerLevelScene is only updated during Processing of crystals by a building carrying them.
	// Aka, this function only triggers with the PowerStation once per processed crystal (times carried during processing).
	// However, the PowerStation does not define a PowerLevelScene.

	// This was likely intended to visually represent a power quantity.
	// But the current code doesn't count crystals stored before processing, and doesn't decrement ever.
	// Not to mention this is on a per-building basis...

	if (!increment) {
		// Inverse function of increment to get back to where level was one increment ago.
		building->powerLevel -= (1.0f - building->powerLevel) / 9.0f;
	}
	else {
		building->powerLevel += (1.0f - building->powerLevel) / 10.0f;
	}
	/// SANITY: Bounds check 1.0f too, not just 0.0f.
	building->powerLevel = std::clamp(building->powerLevel, 0.0f, 1.0f);

	building->flags |= BUILDING_FLAG_UPDATEPOWERLEVEL; // A call to SetAnimationTime is required.
}

// <LegoRR.exe @00408290>
const Point2I* __cdecl LegoRR::Building_GetShapePoints(BuildingModel* building, OPTIONAL OUT uint32* shapeCount)
{
	if (shapeCount) *shapeCount = building->shapeCount;
	return building->shapePoints;
}

// <LegoRR.exe @004082d0>
void __cdecl LegoRR::Building_SetUpgradeActivity(BuildingModel* building, OPTIONAL const char* activityName)
{
	/// FIX APPLY: Remove Container references that were being leaked after every SetUpgradeActivity.
	_Building_RemoveWeaponNulls(building);

	for (uint32 i = 0; i < WEAPON_MAXWEAPONS; i++) {
		//for (uint32 j = 0; j < WEAPON_MAXFIRES; j++) {
		//	building->weapons.fireNullPairs[i][j] = nullptr;
		//}
		//building->weapons.xPivotNulls[i] = nullptr;
		//building->weapons.yPivotNulls[i] = nullptr;

		building->weapons.weaponParts[i] = nullptr;
	}

	building->weapons.weaponCount = 0;

	for (LegoObject* upg = building->upgrades.firstObject; upg != nullptr; upg = upg->upgradePart->nextObject) {

		Upgrade_PartModel* upgradePart = upg->upgradePart;

		Gods98::Container* partParentCont = Building_FindNull(building, upgradePart->partInfo->nullObjectName, upgradePart->partInfo->nullFrameNo);
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
			const uint32 i = building->weapons.weaponCount;

			Error_FatalF(i == WEAPON_MAXWEAPONS,
						 "Building (ID %i) has too many upgrade parts with weapons", static_cast<sint32>(building->objID));

			building->weapons.fireNullPairFrames[i] = 0;

			for (uint32 j = 0; j < WEAPON_MAXFIRES; j++) {
				building->weapons.fireNullPairs[i][j] = nullptr;

				const char* firePartName = Gods98::Container_FormatPartName(building->contAct, building->fireNullName, &j);
				building->weapons.fireNullPairs[i][j] = Gods98::Container_SearchTree(partChildCont, firePartName, Gods98::Container_SearchMode::FirstMatch, nullptr);
				
				/// TODO: Yes, this is assigned before the null check. But is that what *should* happen?
				building->weapons.fireNullPairFrames[i]++;
				if (building->weapons.fireNullPairs[i][j] == nullptr) {
					break;
				}
			}

			const uint32 zero = 0;

			const char* xPartName = Gods98::Container_FormatPartName(building->contAct, building->xPivot, &zero);
			building->weapons.xPivotNulls[i] = Gods98::Container_SearchTree(partChildCont, xPartName, Gods98::Container_SearchMode::FirstMatch, nullptr);

			const char* yPartName = Gods98::Container_FormatPartName(building->contAct, building->yPivot, &zero);
			building->weapons.yPivotNulls[i] = Gods98::Container_SearchTree(partChildCont, yPartName, Gods98::Container_SearchMode::FirstMatch, nullptr);

			building->weapons.weaponParts[i] = upgradePart->partInfo;
			building->weapons.weaponCount++;
		}
	}
}

// <LegoRR.exe @004084a0>
bool32 __cdecl LegoRR::Building_SetActivity(BuildingModel* building, const char* activityName, real32 elapsed)
{
	/// FIX APPLY: Remove Container references that were being leaked after every SetActivity.
	/// FIX APPLY: Also remove tool null references, which were originally excluded from being cleared.
	_Building_RemoveNulls(building);
	_Building_RemoveWeaponNulls(building);

	bool success = Gods98::Container_SetActivity(building->contAct, activityName);
	Gods98::Container_SetAnimationTime(building->contAct, elapsed);
	Building_SetUpgradeActivity(building, activityName);
	return success;
}

// <LegoRR.exe @00408520>
Gods98::Container* __cdecl LegoRR::Building_GetCameraNull(BuildingModel* building, uint32 frameNo)
{
	if (building->cameraNullName != nullptr) {
		if (building->cameraNulls[frameNo] == nullptr) {
			building->cameraNulls[frameNo] = Building_FindNull(building, building->cameraNullName, frameNo);
		}
		return building->cameraNulls[frameNo];
	}
	return nullptr;
}

// <LegoRR.exe @00408550>
void __cdecl LegoRR::Building_Clone(IN BuildingModel* srcBuilding, OUT BuildingModel* destBuilding)
{
	// Directly copy data from source model.
	*destBuilding = *srcBuilding;

	// Dissociate nulls from source model.
	/// FIX APPLY: We don't want to be referencing the nulls of another container.
	destBuilding->depositNull = nullptr;
	destBuilding->entranceNull = nullptr;

	for (uint32 i = 0; i < BUILDING_MAXCARRYS; i++) {
		destBuilding->carryNulls[i] = nullptr;
	}
	for (uint32 i = 0; i < BUILDING_MAXCAMERAS; i++) {
		destBuilding->cameraNulls[i] = nullptr;
	}
	for (uint32 i = 0; i < BUILDING_MAXTOOLS; i++) {
		destBuilding->toolNulls[i] = nullptr;
	}

	for (uint32 i = 0; i < WEAPON_MAXWEAPONS; i++) {
		for (uint32 j = 0; j < WEAPON_MAXFIRES; j++) {
			destBuilding->weapons.fireNullPairs[i][j] = nullptr;
		}
		destBuilding->weapons.xPivotNulls[i] = nullptr;
		destBuilding->weapons.yPivotNulls[i] = nullptr;
	}

	// This is a cloned model, and doesn't own string memory allocations.
	/// FIXME: Building_Load doesn't set SOURCE flag.
	destBuilding->flags &= ~BUILDING_FLAG_SOURCE;

	destBuilding->contAct = Gods98::Container_Clone(srcBuilding->contAct);

	if (srcBuilding->contPowerLevel != nullptr) {
		destBuilding->contPowerLevel = Gods98::Container_Clone(srcBuilding->contPowerLevel);
	}
}

// <LegoRR.exe @004085a0>
void __cdecl LegoRR::Building_Hide(BuildingModel* building, bool32 hide)
{
	Gods98::Container_Hide(building->contAct, hide);

	if (building->contPowerLevel != nullptr) {
		Gods98::Container_Hide(building->contPowerLevel, hide);
	}
}


// <LegoRR.exe @004085f0>
void __cdecl LegoRR::Building_SetOrientation(BuildingModel* building, real32 xDir, real32 yDir)
{
	Gods98::Container_SetOrientation(building->contAct, nullptr, xDir, yDir, 0.0f, 0.0f, 0.0f, -1.0f);

	if (building->contPowerLevel != nullptr) {
		Gods98::Container_SetOrientation(building->contPowerLevel, nullptr, xDir, yDir, 0.0f, 0.0f, 0.0f, -1.0f);
	}
}

// <LegoRR.exe @00408640>
void __cdecl LegoRR::Building_SetPosition(BuildingModel* building, real32 xPos, real32 yPos, GetWorldZCallback zCallback, Map3D* map)
{
	const real32 zPos = zCallback(xPos, yPos, map);
	Gods98::Container_SetPosition(building->contAct, nullptr, xPos, yPos, zPos);

	if (building->contPowerLevel != nullptr) {
		Gods98::Container_SetPosition(building->contPowerLevel, nullptr, xPos, yPos, zPos);
	}
}

// <LegoRR.exe @004086a0>
Gods98::Container* __cdecl LegoRR::Building_GetCarryNull(BuildingModel* building, uint32 frameNo)
{
	if (building->carryNullName != nullptr) {
		if (building->carryNulls[frameNo] == nullptr) {
			building->carryNulls[frameNo] = Building_FindNull(building, building->carryNullName, frameNo);
		}
		return building->carryNulls[frameNo];
	}
	return nullptr;
}

// <LegoRR.exe @004086e0>
Gods98::Container* __cdecl LegoRR::Building_GetDepositNull(BuildingModel* building)
{
	if (building->depositNullName != nullptr) {
		if (building->depositNull == nullptr) {
			building->depositNull = Building_FindNull(building, building->depositNullName, 0);
		}
		return building->depositNull;
	}
	return nullptr;
}

// <LegoRR.exe @00408710>
Gods98::Container* __cdecl LegoRR::Building_GetEntranceNull(BuildingModel* building)
{
	if (building->entranceNullName != nullptr) {
		if (building->entranceNull == nullptr) {
			building->entranceNull = Building_FindNull(building, building->entranceNullName, 0);
		}
		return building->entranceNull;
	}
	return nullptr;
}

// <LegoRR.exe @00408740>
Gods98::Container* __cdecl LegoRR::Building_GetToolNull(BuildingModel* building, uint32 frameNo)
{
	if (building->toolNullName != nullptr) {
		if (building->toolNulls[frameNo] == nullptr) {
			building->toolNulls[frameNo] = Building_FindNull(building, building->toolNullName, frameNo);
		}
		return building->toolNulls[frameNo];
	}
	return nullptr;
}

// <LegoRR.exe @00408780>
uint32 __cdecl LegoRR::Building_GetCarryNullFrames(BuildingModel* building)
{
	return building->carryNullFrames;
}

// <missing>
uint32 __cdecl LegoRR::Building_GetCameraNullFrames(BuildingModel* building)
{
	return building->cameraNullFrames;
}

// <missing>
uint32 __cdecl LegoRR::Building_GetToolNullFrames(BuildingModel* building)
{
	return building->toolNullFrames;
}

// <LegoRR.exe @00408790>
real32 __cdecl LegoRR::Building_MoveAnimation(BuildingModel* building, real32 elapsed, uint32 repeatCount)
{
	real32 overrun = Gods98::Container_MoveAnimation(building->contAct, elapsed);
	if (repeatCount > 1 && overrun != 0.0f) {
		const uint32 animFrames = Gods98::Container_GetAnimationFrames(building->contAct);
		overrun -= static_cast<real32>(animFrames * (repeatCount - 1));
	}

	if (building->flags & BUILDING_FLAG_UPDATEPOWERLEVEL) {
		// Update the visual representation of the power level. This isn't a time-based animation,
		//  but rather the animation time specifies the quantity that the scene intends to show.
		if (building->contPowerLevel != nullptr) {
			const uint32 animFrames = Gods98::Container_GetAnimationFrames(building->contPowerLevel);
			const real32 animTime = static_cast<real32>(animFrames) * building->powerLevel;
			Gods98::Container_SetAnimationTime(building->contPowerLevel, animTime);
		}
		building->flags &= ~BUILDING_FLAG_UPDATEPOWERLEVEL;
	}

	return overrun;
}

// <LegoRR.exe @00408860>
real32 __cdecl LegoRR::Building_GetTransCoef(BuildingModel* building)
{
	return 0.0f;
}

// <LegoRR.exe @00408870>
void __cdecl LegoRR::Building_Remove(BuildingModel* building)
{
	// Remove upgrade model part objects.
	Building_SetUpgradeLevel(building, -1);

	// Remove source string allocations.
	/// FIXME: Building_Load doesn't set SOURCE flag.
	/// FIX APPLY: Setup building for freeing source memory, once flag is fixed in Building_Load.
	if (building->flags & BUILDING_FLAG_SOURCE) {
		if (building->carryNullName) {
			Gods98::Mem_Free(building->carryNullName);
			building->carryNullName = nullptr;
		}
		if (building->cameraNullName) {
			Gods98::Mem_Free(building->cameraNullName);
			building->cameraNullName = nullptr;
		}

		if (building->depositNullName) {
			Gods98::Mem_Free(building->depositNullName);
			building->depositNullName = nullptr;
		}
		if (building->entranceNullName) {
			Gods98::Mem_Free(building->entranceNullName);
			building->entranceNullName = nullptr;
		}
		if (building->toolNullName) {
			Gods98::Mem_Free(building->toolNullName);
			building->toolNullName = nullptr;
		}

		if (building->fireNullName) {
			Gods98::Mem_Free(building->fireNullName);
			building->fireNullName = nullptr;
		}

		if (building->yPivot) {
			Gods98::Mem_Free(building->yPivot);
			building->yPivot = nullptr;
		}
		if (building->xPivot) {
			Gods98::Mem_Free(building->xPivot);
			building->xPivot = nullptr;
		}
	}

	// Remove null containers.
	/// FIX APPLY: Properly remove Container references that were being leaked.
	_Building_RemoveNulls(building);
	_Building_RemoveWeaponNulls(building);

	// Remove activity containers.
	if (building->contPowerLevel != nullptr) {
		Gods98::Container_Remove(building->contPowerLevel);
		building->contPowerLevel = nullptr;
	}

	Gods98::Container_Remove(building->contAct);
	building->contAct = nullptr;
}

// When `current` is true, the function returns whether the object can currently get this upgrade.
//  (AKA, has the object not gotten the upgrade yet? And does it support the upgrade type?)
// Otherwise the function returns whether the object type supports the upgrade at all.
// <LegoRR.exe @004088a0>
bool32 __cdecl LegoRR::Building_CanUpgradeType(BuildingModel* building, LegoObject_UpgradeType upgradeType, bool32 current)
{
	const LegoObject_UpgradeFlags mask = LegoObject_UpgradeToFlag(upgradeType);

	/// FIX APPLY: Mask against currentLevel instead of levelFlags when using current.
	if (current && (Building_GetUpgradeLevel(building) & mask)) {
		// This type is currently upgraded, can't upgrade again.
		return false;
	}
	// Fallback to checking upgrade capabilities, if not currently upgraded.
	return (building->upgrades.levelFlags & mask);
}

// <missing>
uint32 __cdecl LegoRR::Building_GetUpgradeLevel(BuildingModel* building)
{
	return building->upgrades.currentLevel;
}

// <LegoRR.exe @004088d0>
void __cdecl LegoRR::Building_SetUpgradeLevel(BuildingModel* building, uint32 objLevel)
{
	/// FIX APPLY: Remove nulls before they can be changed by Upgrade_SetUpgradeLevel.
	_Building_RemoveNulls(building);
	_Building_RemoveWeaponNulls(building);

	Upgrade_SetUpgradeLevel(&building->upgrades, objLevel);
	Building_SetUpgradeActivity(building, nullptr);
}


#pragma endregion
