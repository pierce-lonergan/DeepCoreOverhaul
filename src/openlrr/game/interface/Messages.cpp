// Messages.cpp : 
//

#include "../../engine/core/Errors.h"
#include "../../engine/core/Memory.h"
#include "../../engine/core/Utils.h"

#include "../object/Object.h"
#include "../object/Stats.h"
#include "../Game.h"

#include "Interface.h"
#include "Messages.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004ebf20>
LegoRR::Message_Globs & LegoRR::messageGlobs = *(LegoRR::Message_Globs*)0x004ebf20;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

using namespace Gods98;


// <LegoRR.exe @00451f90>
void __cdecl LegoRR::Message_Initialise(void)
{
    Message_RegisterName(Message_Null);
    Message_RegisterName(Message_Select);
    Message_RegisterName(Message_Selected);
    Message_RegisterName(Message_ClearSelection);
    Message_RegisterName(Message_Deselect);
    Message_RegisterName(Message_Goto);
    Message_RegisterName(Message_RockMonsterGoto);
    Message_RegisterName(Message_RockMonsterGotoComplete);
    Message_RegisterName(Message_UserGoto);
    Message_RegisterName(Message_FirstPerson);
    Message_RegisterName(Message_TrackObject);
    Message_RegisterName(Message_TopView);
    Message_RegisterName(Message_PlaySample);
    Message_RegisterName(Message_Dig);
    Message_RegisterName(Message_DigComplete);
    Message_RegisterName(Message_Repair);
    Message_RegisterName(Message_RepairComplete);
    Message_RegisterName(Message_Reinforce);
    Message_RegisterName(Message_ReinforceComplete);
    Message_RegisterName(Message_RockFall);
    Message_RegisterName(Message_RockFallComplete);
    Message_RegisterName(Message_GenerateCrystal);
    Message_RegisterName(Message_GenerateCrystalComplete);
    Message_RegisterName(Message_CollectCrystal);
    Message_RegisterName(Message_CollectCrystalComplete);
    Message_RegisterName(Message_CrystalToRefinery);
    Message_RegisterName(Message_CrystalToRefineryComplete);
    Message_RegisterName(Message_GenerateOre);
    Message_RegisterName(Message_GenerateOreComplete);
    Message_RegisterName(Message_CollectOre);
    Message_RegisterName(Message_CollectOreComplete);
    Message_RegisterName(Message_GenerateRockMonster);
    Message_RegisterName(Message_GenerateRockMonsterComplete);
    Message_RegisterName(Message_GatherRock);
    Message_RegisterName(Message_GatherRockComplete);
    Message_RegisterName(Message_PickRandomFloor);
    Message_RegisterName(Message_PickRandomFloorComplete);
    Message_RegisterName(Message_AttackBuilding);
    Message_RegisterName(Message_AttackBuildingComplete);
    Message_RegisterName(Message_Clear);
    Message_RegisterName(Message_ClearComplete);
    Message_RegisterName(Message_GetIn);
    Message_RegisterName(Message_ManSelectedAndMonsterClicked);
    Message_RegisterName(Message_FollowAttack);
    Message_RegisterName(Message_CollectTool);
    Message_RegisterName(Message_ReduceSelection);
    Message_RegisterName(Message_ClearFallIn);
    Message_RegisterName(Message_ClearFallInComplete);
    Message_RegisterName(Message_BuildPath);
    Message_RegisterName(Message_BuildPathComplete);
    Message_RegisterName(Message_Train);
    Message_RegisterName(Message_TrainComplete);
    Message_RegisterName(Message_GenerateCrystalAndOre);
    Message_RegisterName(Message_GenerateCrystalAndOreComplete);
    Message_RegisterName(Message_GenerateFromCryOre);
    Message_RegisterName(Message_GenerateFromCryOreComplete);
    Message_RegisterName(Message_Upgrade);
    Message_RegisterName(Message_UpgradeComplete);
    Message_RegisterName(Message_ClearBuilding);
    Message_RegisterName(Message_ClearBuildingComplete);
    Message_RegisterName(Message_ClearInitial);
    Message_RegisterName(Message_ClearInitialComplete);
    Message_RegisterName(Message_ClearRemovePath);
    Message_RegisterName(Message_ClearRemovePathComplete);
    Message_RegisterName(Message_Debug_DestroyAll);
}

// <LegoRR.exe @00452220>
void __cdecl LegoRR::Message_RemoveLiveObject(LegoObject* liveObj)
{
    //uint32 newCount;
    bool32 bx2 = messageGlobs.messageX2Bool;
    uint32 count = messageGlobs.messageCountX2[bx2];
    for (uint32 i = 0; i < count; i++) {
        if (liveObj == messageGlobs.messageTableX2[bx2][i].argumentObj) {
            //const MessageAction* lastAction = &messageGlobs.messageTableX2[bx2][--count];
            //MessageAction* curAction = &messageGlobs.messageTableX2[bx2][i];

            messageGlobs.messageTableX2[bx2][i] = messageGlobs.messageTableX2[bx2][--count];


            /// ADDED: Is this necessary to avoid memory issues?
            //if (count > 0)
            //*curAction = *lastAction;

            messageGlobs.messageCountX2[bx2] = count;
        }
    }
}

// <LegoRR.exe @00452290>
LegoRR::Message_Type __cdecl LegoRR::Message_LookupPTLEventIndex(const char* ptlName)
{
    for (uint32 i = 0; i < MESSAGE_MAXTYPES; i++) {
        // Skip "Message_" in name
        if (::_stricmp(messageGlobs.messageName[i] + (sizeof(MESSAGE_NAMEPREFIX)-1), ptlName) == 0) {
            return (Message_Type)i;
        }
    }

    /// TODO: Actually return an invalid result on failure
    Error_Warn(true, "Invalid message name in PTL");
    return Message_Type::Message_Null; // safest possible return
    //return (Message_Type)-1;
}

// <LegoRR.exe @004522d0>
void __cdecl LegoRR::Message_Debug_RegisterSelectedUnitHotkey(Gods98::Keys key, Message_Type messageType, LegoObject* argumentObj, undefined4 argument2, OUT Point2I* position)
{
    messageGlobs.hotkeyList[messageGlobs.hotkeyCount] = key;
    messageGlobs.hotkeyMessages[messageGlobs.hotkeyCount].event = messageType;
    messageGlobs.hotkeyMessages[messageGlobs.hotkeyCount].argumentObj = argumentObj;
    messageGlobs.hotkeyMessages[messageGlobs.hotkeyCount].argument2 = argument2;
    if (position) {
        messageGlobs.hotkeyMessages[messageGlobs.hotkeyCount].position = *position;
    }
    messageGlobs.hotkeyCount++;
}

// <LegoRR.exe @00452320>
void __cdecl LegoRR::Message_AddMessageAction(Message_Type messageType, LegoObject* argument1Obj, undefined4 argument2, OPTIONAL Point2I* blockPos)
{
    bool32 bx2 = messageGlobs.messageX2Bool;
    uint32 index = messageGlobs.messageCountX2[bx2];
    if (index < MESSAGE_MAXACTIONS) {
        messageGlobs.messageCountX2[bx2]++;

        std::memset(&messageGlobs.messageTableX2[bx2][index], 0, sizeof(MessageAction));
        // Some leftover default constructor (or memset?)
        /*messageGlobs.messageTableX2[bx2][index].event = Message_Null;
        messageGlobs.messageTableX2[bx2][index].argumentObj = nullptr;
        messageGlobs.messageTableX2[bx2][index].argument2 = 0;
        messageGlobs.messageTableX2[bx2][index].position.x = 0;
        messageGlobs.messageTableX2[bx2][index].position.y = 0;*/

        messageGlobs.messageTableX2[bx2][index].event = messageType;
        messageGlobs.messageTableX2[bx2][index].argumentObj = argument1Obj;
        messageGlobs.messageTableX2[bx2][index].argument2 = argument2;
        if (blockPos) {
            messageGlobs.messageTableX2[bx2][index].position = *blockPos;
        }
    }
}

// <LegoRR.exe @00452390>
//void __cdecl LegoRR::Message_PTL_Update(void);

// <LegoRR.exe @004526f0>
//void __cdecl LegoRR::Message_PTL_PickRandomFloor(LegoObject* liveObj);

// <LegoRR.exe @00452770>
//void __cdecl LegoRR::Message_RemoveObjectReference(LegoObject* liveObj);

// Allocates and outputs a copied list of all currently selected units.
// <LegoRR.exe @004527e0>
bool32 __cdecl LegoRR::Message_CopySelectedUnits(OUT LegoObject*** unitsList, OUT uint32* unitsCount)
{
    if (messageGlobs.selectedUnitCount != 0) {
        uint32 arraySize = messageGlobs.selectedUnitCount * sizeof(LegoObject*);
        *unitsCount = messageGlobs.selectedUnitCount;
        *unitsList = (LegoObject**)Gods98::Mem_Alloc(arraySize);
        std::memcpy(*unitsList, messageGlobs.selectedUnitList, arraySize);
        return true;
    }
    return false;
}

// <LegoRR.exe @00452840>
LegoRR::LegoObject** __cdecl LegoRR::Message_GetSelectedUnits2(OPTIONAL OUT uint32* count)
{
    if (count) *count = messageGlobs.selectedUnitCount;
    if (messageGlobs.selectedUnitCount != 0)
        return messageGlobs.selectedUnitList;
    else
        return nullptr;
}

// <LegoRR.exe @00452870>
bool32 __cdecl LegoRR::Message_AnyUnitSelected(void)
{
    return (messageGlobs.selectedUnitCount != 0);
}

// <LegoRR.exe @00452880>
LegoRR::LegoObject* __cdecl LegoRR::Message_GetPrimarySelectedUnit(void)
{
    if (messageGlobs.selectedUnitCount != 0)
        return messageGlobs.selectedUnitList[0];
    else
        return nullptr;
}

// <LegoRR.exe @004528a0>
LegoRR::LegoObject** __cdecl LegoRR::Message_GetSelectedUnits(void)
{
    return messageGlobs.selectedUnitList;
}

// <LegoRR.exe @004528b0>
uint32 __cdecl LegoRR::Message_GetNumSelectedUnits(void)
{
    return messageGlobs.selectedUnitCount;
}

// <LegoRR.exe @004528c0>
void __cdecl LegoRR::Message_CleanupSelectedUnitsCount(void)
{
    messageGlobs.selectedUnitCount = 0;
}

// <LegoRR.exe @004528d0>
//bool32 __cdecl LegoRR::Message_LiveObject_Check_IsSelected_OrFlags3_200000(LegoObject* liveObj, OUT uint32* index);

// <LegoRR.exe @00452910>
bool32 __cdecl LegoRR::Message_FindIndexOfObject(LegoObject** objsTable, uint32 objsCount, LegoObject* thisObj, OPTIONAL OUT uint32* index)
{
    uint32 i = 0;
    for (uint32 i = 0; i < objsCount; i++) {
        if (thisObj == objsTable[i]) {
            if (index) *index = i;
            return true;
        }
    }
    return false;
}

// <LegoRR.exe @00452950>
//bool32 __cdecl LegoRR::Message_LiveObject_Check_FUN_00452950(LegoObject* liveObj);

// <LegoRR.exe @00452980>
//bool32 __cdecl LegoRR::Message_PTL_Select_LiveObject(LegoObject* liveObj, bool32 noDoubleSelect);

// <LegoRR.exe @004529a0>
//bool32 __cdecl LegoRR::Message_LiveObject_DoSelect_FUN_004529a0(LegoObject* liveObj, bool32 noDoubleSelect, bool32 interrupt);

// <LegoRR.exe @00452b30>
//bool32 __cdecl LegoRR::Message_LiveObject_Check_FUN_00452b30(LegoObject* liveObj);

// <LegoRR.exe @00452b80>
void __cdecl LegoRR::Message_PTL_ReduceSelection(void)
{
	/// DECOMPILER WARNING: The decompiled code for this function was VERY BROKEN. It's flooded with gotos, and the logic is
	///                     straight-up wrong in same cases (like unmanned vehicles being reduced when there's no driver).

	// Disabled for initial implementation commit.
	//const bool allowSpecial = false; // (Gods98::Main_IsDebugComplete() && Lego_IsAllowEditMode());

	// All these reduceType bools will be *reduced* to just one true, after the loop.
	// Listed by highest to lowest priority.
	bool hasMiniFigure      = false; // MiniFigure type, that is not a driver.
	bool hasLandVehicle     = false; // Drivable with driver -or- Vehicle type (self-driving), that can cross land -or- has flags4 0x40.
	bool hasWaterVehicle    = false; // Drivable with driver -or- Vehicle type (self-driving), that can't cross land, and doesn't have flags4 0x40.
	bool hasUnmannedVehicle = false; // Drivable without driver.
	// All types below must not be drivable for the reduceType to be chosen (but drivable isn't checked during the reduce loop).
	bool hasBuilding        = false; // Building type.
	bool hasElectricFence   = false; // ElectricFence type, that is placed (active).
	/// NEW: Additional selection types for debugging. Requires DebugComplete and AllowEditMode.
	bool hasMonster         = false; // RockMonster type.
	bool hasResource        = false; // PowerCrystal type -or- Ore type.
	// Not set in loop, but set if all above are false.
	// This is the main interface menu for when no units are selected.
	bool hasMain            = false;

	// Find all possible reduceTypes from the list of selected units.
	for (uint32 i = 0; i < messageGlobs.selectedUnitCount; i++) {
		LegoObject* unit = messageGlobs.selectedUnitList[i];

		if (unit->type == LegoObject_MiniFigure && unit->driveObject == nullptr) {
			hasMiniFigure = true;
			break;
		}

		const StatsFlags1 sflags1 = StatsObject_GetStatsFlags1(unit);
		if (!(sflags1 & STATS1_CANBEDRIVEN)) {
			if (unit->type == LegoObject_Vehicle) {
				// Self-driving vehicles:
				if ((sflags1 & STATS1_CROSSLAND) || (unit->flags4 & LIVEOBJ4_UNK_40)) {
					hasLandVehicle = true;
				}
				else {
					hasWaterVehicle = true;
				}
			}
			else if (unit->type == LegoObject_Building) {
				hasBuilding = true;
			}
			else if (unit->type == LegoObject_ElectricFence && (unit->flags2 & LIVEOBJ2_ACTIVEELECTRICFENCE)) {
				hasElectricFence = true;
			}
			//else if (allowSpecial && unit->type == LegoObject_RockMonster) {
			//	hasMonster = true;
			//}
			// Can cause crashes with the AI, so disable this for now.
			//else if (allowSpecial && (unit->type == LegoObject_PowerCrystal || unit->type == LegoObject_Ore)) {
			//	hasResource = true;
			//}
		}
		else {
			// Drivable vehicles:
			if (unit->driveObject == nullptr) {
				hasUnmannedVehicle = true;
			}
			else if ((sflags1 & STATS1_CROSSLAND) || (unit->flags4 & LIVEOBJ4_UNK_40)) {
				hasLandVehicle = true;
			}
			else {
				hasWaterVehicle = true;
			}
		}
	}


	// From all of our possible reduceTypes, pick the reduceType with the highest priority and disable all other reduceTypes.
	if (hasMiniFigure) {
		Interface_OpenMenu_FUN_0041b200(Interface_Menu_LegoMan, nullptr);
		hasLandVehicle = false;
		hasWaterVehicle = false;
		hasUnmannedVehicle = false;
		hasBuilding = false;
		hasElectricFence = false;

		hasMonster = false;
		hasResource = false;
	}
	else if (hasLandVehicle) {
		Interface_OpenMenu_FUN_0041b200(Interface_Menu_LandVehicle, nullptr);
		hasWaterVehicle = false;
		hasUnmannedVehicle = false;
		hasBuilding = false;
		hasElectricFence = false;

		hasMonster = false;
		hasResource = false;
	}
	else if (hasWaterVehicle) {
		Interface_OpenMenu_FUN_0041b200(Interface_Menu_WaterVehicle, nullptr);
		hasUnmannedVehicle = false;
		hasBuilding = false;
		hasElectricFence = false;

		hasMonster = false;
		hasResource = false;
	}
	else if (hasUnmannedVehicle) {
		Interface_OpenMenu_FUN_0041b200(Interface_Menu_UnmannedVehicle, nullptr);
		hasBuilding = false;
		hasElectricFence = false;

		hasMonster = false;
		hasResource = false;
	}
	else if (hasBuilding) {
		Interface_OpenMenu_FUN_0041b200(Interface_Menu_Building, nullptr);
		hasElectricFence = false;

		hasMonster = false;
		hasResource = false;
	}
	else if (hasElectricFence) {
		Interface_OpenMenu_FUN_0041b200(Interface_Menu_ElectricFence, nullptr);

		hasMonster = false;
		hasResource = false;
	}
	else if (hasMonster) {
		// Crashes:
		//Interface_OpenMenu_FUN_0041b200(Interface_Menu_LegoMan, nullptr);
		//Interface_OpenMenu_FUN_0041b200(Interface_Menu_Building, nullptr);

		// Works: Even the tele' up button functions (for "standard" monster types).
		Interface_OpenMenu_FUN_0041b200(Interface_Menu_ElectricFence, nullptr);
		//Interface_OpenMenu_FUN_0041b200(Interface_Menu_Main, nullptr);
		hasResource = false;
	}
	else if (hasResource) {
		// Works: Even the tele' up button functions.
		Interface_OpenMenu_FUN_0041b200(Interface_Menu_ElectricFence, nullptr);
	}
	else {
		hasMain = true; // Fallback to main interface if no matching units found.
		Interface_OpenMenu_FUN_0041b200(Interface_Menu_Main, nullptr);
	}


	// Go through the list of selected units, and remove units that are excluded by our reduceType.
	for (uint32 i = 0; i < messageGlobs.selectedUnitCount; i++) {
		LegoObject* unit = messageGlobs.selectedUnitList[i];
		const StatsFlags1 sflags1 = StatsObject_GetStatsFlags1(unit);
		bool reduce = false;

		if (hasMiniFigure) {
			if (unit->type != LegoObject_MiniFigure || unit->driveObject != nullptr) {
				// 1. This is not a mini-figure.
				// 2. This is a driver (select the vehicle instead).
				reduce = true;
			}
		}
		else if (hasLandVehicle) {
			if (!(sflags1 & STATS1_CANBEDRIVEN)) {
				/// FIXME: Self-driving vehicles don't properly differentiate between land and water!
				if (unit->type != LegoObject_Vehicle) {
					// 1. This isn't a self-driving vehicle.
					reduce = true;
				}
			}
			else if ((!(sflags1 & STATS1_CROSSLAND) && !(unit->flags4 & LIVEOBJ4_UNK_40)) || unit->driveObject == nullptr) {
				// 1. This is a water vehicle. No mystery land vehicle flag.
				// 2. This is an unmanned vehicle.
				reduce = true;
			}
		}
		else if (hasWaterVehicle) {
			if (!(sflags1 & STATS1_CANBEDRIVEN)) {
				/// FIXME: Self-driving vehicles don't properly differentiate between land and water!
				if (unit->type != LegoObject_Vehicle) {
					// 1. This isn't a self-driving vehicle.
					reduce = true;
				}
			}
			else if (((sflags1 & STATS1_CROSSLAND) || (unit->flags4 & LIVEOBJ4_UNK_40)) || unit->driveObject == nullptr) {
				// 1. This is a land vehicle. Has mystery land vehicle flag.
				// 2. This is an unmanned vehicle.
				reduce = true;
			}
		}
		else if (hasUnmannedVehicle) {
			/// DECOMPILER FIX: THE DECOMPILER LOGIC HERE *WAS* WRONG!!!
			///                 A vehicle *was* considered unmanned if it was drivable WITH A DRIVER (not without).
			if (!(sflags1 & STATS1_CANBEDRIVEN) || unit->driveObject != nullptr) {
				// 1. This is a self-driving vehicle.
				// 2. This is not an unmanned vehicle.
				reduce = true;
			}
		}
		else if (hasBuilding) {
			if (unit->type != LegoObject_Building) {
				reduce = true;
			}
		}
		else if (hasElectricFence) {
			if (unit->type != LegoObject_ElectricFence || !(unit->flags2 & LIVEOBJ2_ACTIVEELECTRICFENCE)) {
				// 1. This is not an electric fence.
				// 2. This electric fence is not active (placed).
				reduce = true;
			}
		}
		else if (hasMonster) {
			if (unit->type != LegoObject_RockMonster) {
				reduce = true;
			}
		}
		else if (hasResource) {
			if (unit->type != LegoObject_PowerCrystal && unit->type != LegoObject_Ore) {
				reduce = true;
			}
		}
		else { //if (hasMain) {
			reduce = true;
		}

		if (reduce || hasMain) { // hasMain check is redundant here, but wasn't in original decompiled function.

			// Replace this reduced object with the object at the end of the list.
			messageGlobs.selectedUnitList[i] = messageGlobs.selectedUnitList[messageGlobs.selectedUnitCount - 1];
			messageGlobs.selectedUnitCount--;
			i--;

			unit->flags3 &= ~LIVEOBJ3_SELECTED;
			/// FIXME: Should we also remove LIVEOBJ4_LASERTRACKERMODE, like with ClearSelection?
			//unit->flags4 &= ~LIVEOBJ4_LASERTRACKERMODE;
		}
	}
}

// <LegoRR.exe @00452ea0>
//void __cdecl LegoRR::Message_PTL_ClearSelection(void);

// <LegoRR.exe @00452f10>
//bool32 __cdecl LegoRR::Message_PTL_Deselect_LiveObject(LegoObject* liveObj);

// <LegoRR.exe @00452f80>
//uint32 __cdecl LegoRR::Message_PTL_Debug_DestroyAll(void);

// <LegoRR.exe @00453020>
//bool32 __cdecl LegoRR::Message_PTL_FirstPerson(uint32 cameraFrame);

#pragma endregion
