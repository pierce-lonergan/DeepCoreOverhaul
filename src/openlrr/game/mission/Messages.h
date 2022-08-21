// Messages.h : 
//

#pragma once

#include "../GameCommon.h"


namespace LegoRR
{; // !<---

/**********************************************************************************
 ******** Forward Declarations
 **********************************************************************************/

#pragma region Forward Declarations

struct LegoObject;

#pragma endregion

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define MESSAGE_MAXTYPES		65
//#define MESSAGE_MAXSELECTED		100
#define MESSAGE_MAXHOTKEYS		10
#define MESSAGE_MAXEVENTS		2048

#define MESSAGE_NAMEPREFIX		"Message_"

#define MESSAGE_ARGUMENT_NONE	Message_Argument()

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

/// CUSTOM: Special enum for internal use in Message_ReduceSelectedUnits.
///         This is exposed in the case that we want to configure how to reduce selections in the future.
// Reduce type priority, with lower values having higher priority (excluding None, which is default).
// This states what a selection is reduced *to*. Meaning all non-matching objects for the type are removed from selection.
enum Message_ReduceType : sint32
{
	//Message_ReduceType_None,            // No valid selectable objects found, clear selected units.

	Message_ReduceType_MiniFigure,      // MiniFigure type, that is not a driver.
	Message_ReduceType_LandVehicle,     // Drivable with driver -or- Vehicle type (self-driving), that can cross land -or- has flags4 0x40.
	Message_ReduceType_WaterVehicle,    // Drivable with driver -or- Vehicle type (self-driving), that can't cross land, and doesn't have flags4 0x40.
	Message_ReduceType_UnmannedVehicle, // Drivable without driver.
	// All types below must not be drivable for the reduceType to be chosen (but drivable isn't checked during the reduce loop).
	Message_ReduceType_Building,        // Building type.
	Message_ReduceType_ElectricFence,   // ElectricFence type, that is placed (active).

	/// NEW: Additional selection types for debugging. Requires DebugComplete + AllowEditMode (which is no longer enabled by DebugComplete).
	Message_ReduceType_Monster,         // RockMonster type.
	// Disabled due to unknown behaviour:
	Message_ReduceType_SpiderWeb,       // SpiderWeb type.
	// Disabled due to crashes with AI:
	Message_ReduceType_Resource,        // PowerCrystal type -or- Ore type.
	// Disabled due to unknown behaviour (probably crashes with AI):
	Message_ReduceType_Equipment,		// Dynamite type -or- Barrier type -or- OohScary type, or ElectricFence type that isn't placed (inactive).


	Message_ReduceType_Count,           // No valid selectable objects found, clear selected units.
};

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

// Second argument for Message_Event that can be many types, and depends on the message.
union Message_Argument
{
	// (cameraFrame): Message_FirstPerson
	uint32 uinteger;

	// (noDoubleSelect): Message_Select
	// (reevalTasks): Message_UserGoto
	bool32 boolean;

	// (sfxID): Message_PlaySample       
	SFX_ID sfxID;

	// (refineryObj): Message_CollectCrystalComplete, Message_CrystalToRefinery
	// (attackObj): Message_GatherRockComplete, Message_AttackBuilding, Message_FollowAttack
	LegoObject* object;

	// (referrerTask): This field is assigned to AITask, but unused.
	// Message_Goto, Message_RockMonsterGoto, Message_Dig, Message_Repair, Message_Reinforce, Message_CollectCrystal,
	// Message_Clear, Message_CollectTool, Message_BuildPath, Message_Train, Message_Upgrade
	AITask* aiTask;

	// (wallDirection, isWallConnection): Message_DigComplete, Message_RockFall
	struct Message_WallLowordHiword
	{
		/*0,2*/	uint16 direction;		// LOWORD, Direction
		/*2,2*/	bool16 isConnection;	// HIWORD, bool32
	} wall;

	inline Message_Argument() : uinteger(0) {}
	inline Message_Argument(sint32 integer) : uinteger(static_cast<uint32>(integer)) {} // Keep this to prevent ambiguity with bool constructor.
	inline Message_Argument(uint32 uinteger) : uinteger(uinteger) {}
	inline Message_Argument(bool boolean) : boolean(boolean) {}
	inline Message_Argument(SFX_ID sfxID) : sfxID(sfxID) {}
	inline Message_Argument(LegoObject* object) : object(object) {}
	inline Message_Argument(AITask* aiTask) : aiTask(aiTask) {}
	inline Message_Argument(Direction wallDirection, bool32 isWallConnection) : wall({ (uint16)wallDirection, (bool16)isWallConnection }) {}
};
assert_sizeof(Message_Argument, 0x4);


struct Message_Event // [LegoRR/Message.c|struct:0x14]
{
	/*00,4*/	Message_Type type;
	/*04,4*/	LegoObject* argumentObj; // (this argument is only used for live objects, but it's not necessarily true that it's the same in source)
	/*08,4*/	Message_Argument argument2; // ()
	/*0c,8*/	Point2I blockPos;
	/*14*/
};
assert_sizeof(Message_Event, 0x14);


struct Message_Globs // [LegoRR/Message.c|struct:0x14380|tags:GLOBS]
{
	/*00000,14000*/	Message_Event eventLists[2][MESSAGE_MAXEVENTS];
	/*14000,8*/	    uint32 eventCounts[2];
	/*14008,4*/	    bool32 eventAB; // Whether to use index 0 or 1 of eventLists, eventCounts.
	/*1400c,190*/	LegoObject* selectedUnitList[LEGO_MAXMULTISELECT];
	/*1419c,a*/	    Gods98::Keys hotKeyKeyList[MESSAGE_MAXHOTKEYS];
	/*141a6,2*/	    undefined2 padding1;
	/*141a8,c8*/	Message_Event hotKeyEventList[MESSAGE_MAXHOTKEYS];
	/*14270,104*/	const char* messageName[Message_Type_Count];
	/*14374,4*/	    undefined4 reserved1;
	/*14378,4*/	    uint32 selectedUnitCount;
	/*1437c,4*/	    uint32 hotKeyCount;
	/*14380*/
};
assert_sizeof(Message_Globs, 0x14380);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004ebf20>
extern Message_Globs & messageGlobs;

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define Message_RegisterName(n)		(messageGlobs.messageName[n]=#n)

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @00451f90>
void __cdecl Message_Initialise(void);

// <LegoRR.exe @00452220>
void __cdecl Message_RemoveEventsWithObject(LegoObject* deadObj);

// <LegoRR.exe @00452290>
Message_Type __cdecl Message_ParsePTLName(const char* ptlName);

// <LegoRR.exe @004522d0>
void __cdecl Message_RegisterHotKeyEvent(Gods98::Keys key, Message_Type messageType, LegoObject* argument1Obj, Message_Argument argument2, OPTIONAL const Point2I* blockPos);

// <LegoRR.exe @00452320>
void __cdecl Message_PostEvent(Message_Type messageType, LegoObject* argument1Obj, Message_Argument argument2, OPTIONAL const Point2I* blockPos);

// <LegoRR.exe @00452390>
//#define Message_Update ((void (__cdecl* )(void))0x00452390)
void __cdecl Message_Update(void);

// <LegoRR.exe @004526f0>
//#define Message_PickRandomFloorBlock ((void (__cdecl* )(LegoObject* liveObj))0x004526f0)
void __cdecl Message_PickRandomFloorBlock(LegoObject* liveObj);

// Remove object from selection.
// Cancel first-person view if this is the objectFP.
// And clear the camera tracker reference to this object.
// <LegoRR.exe @00452770>
//#define Message_RemoveObjectReference ((void (__cdecl* )(LegoObject* deadObj))0x00452770)
void __cdecl Message_RemoveObjectReference(LegoObject* deadObj);

// Allocates and outputs a copied list of all currently selected units.
// <LegoRR.exe @004527e0>
bool32 __cdecl Message_CopySelectedUnits(OUT LegoObject*** unitsList, OUT uint32* unitsCount);

// <LegoRR.exe @00452840>
LegoObject** __cdecl Message_GetSelectedUnits2(OPTIONAL OUT uint32* count);

// <LegoRR.exe @00452870>
bool32 __cdecl Message_AnyUnitSelected(void);

// <LegoRR.exe @00452880>
LegoObject* __cdecl Message_GetPrimarySelectedUnit(void);

// <LegoRR.exe @004528a0>
LegoObject** __cdecl Message_GetSelectedUnits(void);

// <LegoRR.exe @004528b0>
uint32 __cdecl Message_GetNumSelectedUnits(void);

// Should only be used during Level_Free, because normally extra logic is needed when deselecting an object (i.e. unsetting flags).
// <LegoRR.exe @004528c0>
void __cdecl Message_CleanupSelectedUnitsCount(void);

// <LegoRR.exe @004528d0>
//#define Message_IsUnitSelected ((bool32 (__cdecl* )(LegoObject* liveObj, OUT uint32* index))0x004528d0)
bool32 __cdecl Message_IsUnitSelected(LegoObject* liveObj, OUT uint32* index);

// <LegoRR.exe @00452910>
bool32 __cdecl Message_FindIndexOfObject(LegoObject** objsTable, uint32 objsCount, LegoObject* thisObj, OPTIONAL OUT uint32* index);

// Used to check for double-selection of unit to enter laser tracker mode.
// <LegoRR.exe @00452950>
//#define Message_IsObjectDoubleSelectable ((bool32 (__cdecl* )(LegoObject* liveObj))0x00452950)
bool32 __cdecl Message_IsObjectDoubleSelectable(LegoObject* liveObj);

// Used to check for double-selection of unit to enter laser tracker mode.
// <LegoRR.exe @00452980>
//#define Message_SelectObject ((bool32 (__cdecl* )(LegoObject* liveObj, bool32 noDoubleSelect))0x00452980)
bool32 __cdecl Message_SelectObject(LegoObject* liveObj, bool32 noDoubleSelect);

// When noDoubleSelect is false, it's possible to double-click and enter laser tracker mode.
// <LegoRR.exe @004529a0>
//#define Message_SelectObject2 ((bool32 (__cdecl* )(LegoObject* liveObj, bool32 noDoubleSelect, bool32 interrupt))0x004529a0)
bool32 __cdecl Message_SelectObject2(LegoObject* liveObj, bool32 noDoubleSelect, bool32 interrupt);

// <LegoRR.exe @00452b30>
//#define Message_IsObjectSelectable ((bool32 (__cdecl* )(LegoObject* liveObj))0x00452b30)
bool32 __cdecl Message_IsObjectSelectable(LegoObject* liveObj);

// <LegoRR.exe @00452b80>
//#define Message_ReduceSelectedUnits ((void (__cdecl* )(void))0x00452b80)
void __cdecl Message_ReduceSelectedUnits(void);

// <LegoRR.exe @00452ea0>
//#define Message_ClearSelectedUnits ((void (__cdecl* )(void))0x00452ea0)
void __cdecl Message_ClearSelectedUnits(void);

// <LegoRR.exe @00452f10>
//#define Message_DeselectObject ((bool32 (__cdecl* )(LegoObject* liveObj))0x00452f10)
bool32 __cdecl Message_DeselectObject(LegoObject* liveObj);

// <LegoRR.exe @00452f80>
//#define Message_Debug_DestroySelectedUnits ((uint32 (__cdecl* )(void))0x00452f80)
uint32 __cdecl Message_Debug_DestroySelectedUnits(void);

// <LegoRR.exe @00453020>
//#define Message_EnterFirstPersonView ((bool32 (__cdecl* )(uint32 cameraFrame))0x00453020)
bool32 __cdecl Message_EnterFirstPersonView(uint32 cameraFrame);

#pragma endregion

}
