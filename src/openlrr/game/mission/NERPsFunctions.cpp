// NERPsFunctions.cpp : 
//

#include "../../engine/drawing/DirectDraw.h"
#include "../../engine/input/Input.h"
#include "../../engine/input/Keys.h"

#include "../front/Reward.h"
#include "../interface/Advisor.h"
#include "../interface/Interface.h"
#include "../interface/Panels.h"
#include "../interface/TextMessages.h"
#include "../object/AITask.h"
#include "../world/Teleporter.h"
#include "../Game.h"
#include "../Shortcuts.hpp"

#include "Messages.h"
#include "Objective.h"
#include "NERPsFunctions.h"


using Gods98::Keys;
using Shortcuts::ShortcutID;



/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define return_DUMMY()				return 0

#define return_NOT_IMPLEMENTED()	return 0

#define return_VOID(r)				return r

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @00453b60>
sint32 __cdecl LegoRR::NERPFunc__SetGameSpeed(sint32* stack)
{
	if (stack[0] == 0 && Lego_GetLevel()->status != LEVELSTATUS_INCOMPLETE) {
		// Prevent freezing the game speed during outro.
	}

	Lego_LockGameSpeed(stack[1]);
	Lego_SetGameSpeed(static_cast<uint32>(stack[0]) * 0.01f); // Percent to range [0,1].
	Lego_LockGameSpeed(stack[1]);
	return_VOID(0);
}


// <LegoRR.exe @00453cb0>
sint32 __cdecl LegoRR::NERPFunc__GetMessagesAreUpToDate(sint32* stack)
{
	return (nerpsfileGlobs.uint_a8 >= nerpsfileGlobs.lineCount);
}

// <LegoRR.exe @00453cd0>
sint32 __cdecl LegoRR::NERPFunc__SupressArrow(sint32* stack)
{
	nerpsruntimeGlobs.supressArrow = stack[0];
	if (nerpsfileGlobs.int_a4 == 0) {
		Lego_SetFlags2_40_And_2_unkCamera(!nerpsruntimeGlobs.supressArrow, true);
	}
	else {
		Lego_SetFlags2_40_And_2_unkCamera(true, true);
	}
	return_VOID(0);
}

// <LegoRR.exe @00453d10>
sint32 __cdecl LegoRR::NERPFunc__AdvanceMessage(sint32* stack)
{
	if (nerpsfileGlobs.int_a4 > 1) {
		nerpsfileGlobs.int_a4 = 1;
	}
	NERPsRuntime_AdvanceMessage();
	return_VOID(0);
}

// <LegoRR.exe @00453d30>
sint32 __cdecl LegoRR::NERPFunc__AllowCameraMovement(sint32* stack)
{
	/// JANK: Change argument behaviour to behave the same for all non-zero values.
	///       STACK[0] IS ASSUMED TO BE A BOOL IN THE RANGE 0-1.
	
	nerpsruntimeGlobs.allowCameraMovement = stack[0] << 12; // (0x1000) This is used to negative TUTORIAL_FLAG_NOCAMERA.
	return_VOID(0);
}

// <LegoRR.exe @00453d50>
sint32 __cdecl LegoRR::NERPFunc__ClickOnlyObjects(sint32* stack)
{
	/// JANK: Treat allowCameraMovement as a bool instead of a flag to subtract.
	TutorialFlags tflags = (TUTORIAL_FLAGS_ALL & ~(TUTORIAL_FLAG_NOMAP|TUTORIAL_FLAG_NOOBJECTS));

	//if (nerpsruntimeGlobs.allowCameraMovement) tflags &= ~TUTORIAL_FLAG_NOCAMERA;
	tflags = (TutorialFlags)(static_cast<uint32>(tflags) - nerpsruntimeGlobs.allowCameraMovement);

	NERPs_SetTutorialFlags(tflags);
	return_VOID(0);
}

// <LegoRR.exe @00453d80>
sint32 __cdecl LegoRR::NERPFunc__ClickOnlyMap(sint32* stack)
{
	/// JANK: Treat allowCameraMovement as a bool instead of a flag to subtract.
	TutorialFlags tflags = (TUTORIAL_FLAGS_ALL & ~(TUTORIAL_FLAG_NOMAP));

	//if (nerpsruntimeGlobs.allowCameraMovement) tflags &= ~TUTORIAL_FLAG_NOCAMERA;
	tflags = (TutorialFlags)(static_cast<uint32>(tflags) - nerpsruntimeGlobs.allowCameraMovement);

	NERPs_SetTutorialFlags(tflags);
	return_VOID(0);
}

// <LegoRR.exe @00453db0>
sint32 __cdecl LegoRR::NERPFunc__ClickOnlyIcon(sint32* stack)
{
	/// JANK: Treat allowCameraMovement as a bool instead of a flag to subtract.
	TutorialFlags tflags = (TUTORIAL_FLAGS_ALL & ~(TUTORIAL_FLAG_NOICONS));

	//if (nerpsruntimeGlobs.allowCameraMovement) tflags &= ~TUTORIAL_FLAG_NOCAMERA;
	tflags = (TutorialFlags)(static_cast<uint32>(tflags) - nerpsruntimeGlobs.allowCameraMovement);

	NERPs_SetTutorialFlags(tflags);
	return_VOID(0);
}

// <LegoRR.exe @00453de0>
sint32 __cdecl LegoRR::NERPFunc__ClickOnlyCalltoarms(sint32* stack)
{
	/// JANK: Treat allowCameraMovement as a bool instead of a flag to subtract.
	TutorialFlags tflags = (TUTORIAL_FLAGS_ALL & ~(TUTORIAL_FLAG_NOCALLTOARMS));

	//if (nerpsruntimeGlobs.allowCameraMovement) tflags &= ~TUTORIAL_FLAG_NOCAMERA;
	tflags = (TutorialFlags)(static_cast<uint32>(tflags) - nerpsruntimeGlobs.allowCameraMovement);

	NERPs_SetTutorialFlags(tflags);
	return_VOID(0);
}

// <LegoRR.exe @00453e10>
sint32 __cdecl LegoRR::NERPFunc__DisallowAll(sint32* stack)
{
	/// JANK: Treat allowCameraMovement as a bool instead of a flag to subtract.
	TutorialFlags tflags = TUTORIAL_FLAGS_ALL;

	//if (nerpsruntimeGlobs.allowCameraMovement) tflags &= ~TUTORIAL_FLAG_NOCAMERA;
	tflags = (TutorialFlags)(static_cast<uint32>(tflags) - nerpsruntimeGlobs.allowCameraMovement);

	NERPs_SetTutorialFlags(tflags);
	return_VOID(0);
}

// <LegoRR.exe @00453e40>
sint32 __cdecl LegoRR::NERPFunc__FlashCallToArmsIcon(sint32* stack)
{
	if (stack[0]) {
		Panel_SetCurrentAdvisorFromButton(Panel_TopPanel, PanelButton_TopPanel_CallToArms, true);
	}
	else {
		Advisor_End();
	}
	return_VOID(0);
}


// <LegoRR.exe @00453f60>
sint32 __cdecl LegoRR::NERPFunc__GetTimer0(sint32* stack)
{
	return static_cast<sint32>(nerpsruntimeGlobs.timers[0]);
}

// <LegoRR.exe @00453f70>
sint32 __cdecl LegoRR::NERPFunc__GetTimer1(sint32* stack)
{
	return static_cast<sint32>(nerpsruntimeGlobs.timers[1]);
}

// <LegoRR.exe @00453f80>
sint32 __cdecl LegoRR::NERPFunc__GetTimer2(sint32* stack)
{
	return static_cast<sint32>(nerpsruntimeGlobs.timers[2]);
}

// <LegoRR.exe @00453f90>
sint32 __cdecl LegoRR::NERPFunc__GetTimer3(sint32* stack)
{
	return static_cast<sint32>(nerpsruntimeGlobs.timers[3]);
}


// <LegoRR.exe @00453fa0>
sint32 __cdecl LegoRR::NERPFunc__SetTimer0(sint32* stack)
{
	nerpsruntimeGlobs.timers[0] = static_cast<real32>(static_cast<uint32>(stack[0]));
	return_VOID(1);
}

// <LegoRR.exe @00453fd0>
sint32 __cdecl LegoRR::NERPFunc__SetTimer1(sint32* stack)
{
	nerpsruntimeGlobs.timers[1] = static_cast<real32>(static_cast<uint32>(stack[0]));
	return_VOID(1);
}

// <LegoRR.exe @00454000>
sint32 __cdecl LegoRR::NERPFunc__SetTimer2(sint32* stack)
{
	nerpsruntimeGlobs.timers[2] = static_cast<real32>(static_cast<uint32>(stack[0]));
	return_VOID(1);
}

// <LegoRR.exe @00454030>
sint32 __cdecl LegoRR::NERPFunc__SetTimer3(sint32* stack)
{
	nerpsruntimeGlobs.timers[3] = static_cast<real32>(static_cast<uint32>(stack[0]));
	return_VOID(1);
}


// <LegoRR.exe @004542a0>
sint32 __cdecl LegoRR::NERPFunc__CameraLockOnMonster(sint32* stack)
{
	if (stack[0]) {
		for (auto obj : objectListSet.EnumerateSkipUpgradeParts()) {
			if (NERPsRuntime_LiveObject_GetIfRockMonsterAnd_FUN_004542e0(obj, &nerpsfileGlobs.camLockOnObject))
				break;
		}
		//LegoObject_RunThroughListsSkipUpgradeParts(NERPsRuntime_LiveObject_GetIfRockMonsterAnd_FUN_004542e0, &nerpsfileGlobs.camLockOnObject);
		nerpsfileGlobs.camIsLockedOn = true;
	}
	else {
		nerpsfileGlobs.camIsLockedOn = false;
		nerpsfileGlobs.camLockOnObject = nullptr;
	}
	return_VOID(0);
}


// <LegoRR.exe @00454310>
sint32 __cdecl LegoRR::NERPFunc__CameraLockOnObject(sint32* stack)
{
	if (stack[0]) { // 0 is 'no record object', so do nothing.
		nerpsfileGlobs.camLockOnRecord = (stack[0] - 1); // To 0-indexed.
		nerpsfileGlobs.camIsLockedOn = true;
	}
	return_VOID(0);
}

// <LegoRR.exe @00454330>
sint32 __cdecl LegoRR::NERPFunc__CameraUnlock(sint32* stack)
{
	nerpsfileGlobs.camIsLockedOn = false;
	nerpsfileGlobs.camLockOnRecord = static_cast<uint32>(-1);
	nerpsfileGlobs.camLockOnObject = nullptr;
	return_VOID(0);
}

// <LegoRR.exe @00454350>
sint32 __cdecl LegoRR::NERPFunc__CameraZoomIn(sint32* stack)
{
	// Flush remaining zoom amount.
	if (nerpsfileGlobs.camZoomMoved != 0.0f) {
		/// NOTE: No special handling needed for negative zoom amounts.
		const real32 zoomLeft = nerpsfileGlobs.camZoomTotal - nerpsfileGlobs.camZoomMoved;

		Camera_AddZoom(legoGlobs.cameraMain, zoomLeft);
		nerpsfileGlobs.camZoomMoved = 0.0f;
	}

	// Negative zoom total.
	nerpsfileGlobs.camZoomTotal = -static_cast<real32>(static_cast<uint32>(stack[0]));
	if (nerpsfileGlobs.camZoomTotal != 0.0f) {
		nerpsfileGlobs.camIsZooming = true;
	}
	return_VOID(0);
}

// <LegoRR.exe @004543d0>
sint32 __cdecl LegoRR::NERPFunc__CameraZoomOut(sint32* stack)
{
	// Flush remaining zoom amount.
	if (nerpsfileGlobs.camZoomMoved != 0.0f) {
		/// NOTE: No special handling needed for negative zoom amounts.
		const real32 zoomLeft = nerpsfileGlobs.camZoomTotal - nerpsfileGlobs.camZoomMoved;

		Camera_AddZoom(legoGlobs.cameraMain, zoomLeft);
		/// FIX APPLY: Clear camZoomMoved like in CameraZoomIn.
		nerpsfileGlobs.camZoomMoved = 0.0f;
	}

	// Positive zoom total.
	nerpsfileGlobs.camZoomTotal = static_cast<real32>(static_cast<uint32>(stack[0]));
	if (nerpsfileGlobs.camZoomTotal != 0.0f) {
		nerpsfileGlobs.camIsZooming = true;
	}
	return_VOID(0);
}

// <LegoRR.exe @00454440>
sint32 __cdecl LegoRR::NERPFunc__CameraRotate(sint32* stack)
{
	// Flush remaining rotation amount.
	if (nerpsfileGlobs.camRotMoved != 0.0f) {
		/// NOTE: No special handling needed for negative rotation amounts.
		const real32 rotateLeft = nerpsfileGlobs.camRotTotal - nerpsfileGlobs.camRotMoved;

		Camera_AddRotation(legoGlobs.cameraMain, rotateLeft * (M_PI / 180.0f)); // Degrees to radians.
		nerpsfileGlobs.camRotMoved = 0.0;
	}

	nerpsfileGlobs.camRotTotal = static_cast<real32>(static_cast<uint32>(stack[0]));
	if (nerpsfileGlobs.camRotTotal > 180.0f) {
		nerpsfileGlobs.camRotTotal -= 360.0f; // Rotate counter-clockwise.
	}
	if (nerpsfileGlobs.camRotTotal != 0.0f) {
		nerpsfileGlobs.camIsRotating = true;
	}
	return_VOID(0);
}

// <LegoRR.exe @004544e0>
sint32 __cdecl LegoRR::NERPFunc__GetSelectedRecordObject(sint32* stack)
{
	for (uint32 i = 0; i < OBJECT_MAXRECORDOBJECTS; i++) {
		LegoObject* recordObj = nullptr;
		if (Lego_GetRecordObject(i, &recordObj)) {

			if (Message_IsUnitSelected(recordObj, nullptr)) {
				return (i + 1); // (1-indexed) record object pointer.
			}
		}
	}
	return 0;
}

// <LegoRR.exe @00454530>
sint32 __cdecl LegoRR::NERPFunc__SetCrystalPriority(sint32* stack)
{
	AITask_Game_SetPriorityOff(AI_Priority_Crystal, !stack[0]);
	return_VOID(0);
}

// <LegoRR.exe @00454550>
sint32 __cdecl LegoRR::NERPFunc__SetMonsterAttackPowerstation(sint32* stack)
{
	// if ( (A & sflags2) && (A != -1)) FAIL
	// if (!(B & sflags2) && (B != -1)) FAIL
	// It's impossible for A to conflict with B, despite what it looks like.

	SearchNERPsSetMonsterAttack search = { (StatsFlags2)0 };
	/// FIXME: EXACT BOOLEAN
	search.stopAttacking = (stack[0] == TRUE);
	if (search.stopAttacking) {
		// Stop attacking NON-power-generating buildings (AKA Power Station).
		search.sflags2Exclude = STATS2_GENERATEPOWER; // Exclude power-generating buildings.
		search.sflags2Include = STATS2_FLAGS_ALL;
	}
	else {
		// Start attacking any building indiscriminately. (same as NERPFunc__SetMonsterAttackNowt)
		search.sflags2Exclude = STATS2_NONE;
		search.sflags2Include = STATS2_FLAGS_ALL;
	}

	for (auto obj : objectListSet.EnumerateSkipUpgradeParts()) {
		NERPs_LegoObject_Callback_SetMonsterAttack(obj, &search);
	}
	return_VOID(0);
}


// <LegoRR.exe @00454660>
sint32 __cdecl LegoRR::NERPFunc__SetMonsterAttackNowt(sint32* stack)
{
	SearchNERPsSetMonsterAttack search = { (StatsFlags2)0 };
	/// FIXME: EXACT BOOLEAN
	search.stopAttacking = (stack[0] == TRUE);
	// Start/stop attacking any building indiscriminately.
	search.sflags2Exclude = STATS2_NONE;
	search.sflags2Include = STATS2_FLAGS_ALL;

	for (auto obj : objectListSet.EnumerateSkipUpgradeParts()) {
		NERPs_LegoObject_Callback_SetMonsterAttack(obj, &search);
	}
	return_VOID(0);
}


// <LegoRR.exe @004547f0>
sint32 __cdecl LegoRR::NERPFunc__GetRecordObjectAtTutorial(sint32* stack)
{
	for (uint32 i = 0; i < OBJECT_MAXRECORDOBJECTS; i++) {
		LegoObject* recordObj = nullptr;
		if (Lego_GetRecordObject(i, &recordObj)) {

			SearchNERPsCountRecordObjects search = { 0 };
			search.count = 0;
			search.recordObj = recordObj;
			NERPsRuntime_EnumerateBlockPointers(stack[0], NERPs_LiveObject_Callback_CountRecordObjectsAtBlock, &search);
			if (search.count > 0) {
				return (i + 1); // (1-indexed) record object pointer.
			}
		}
	}
	return 0;
}

// <LegoRR.exe @00454860>
sint32 __cdecl LegoRR::NERPFunc__GetRecordObjectAmountAtTutorial(sint32* stack)
{
	SearchNERPsCountRecordObjects search = { 0 };
	search.count = 0;

	for (uint32 i = 0; i < OBJECT_MAXRECORDOBJECTS; i++) {
		LegoObject* recordObj = nullptr;
		if (Lego_GetRecordObject(i, &recordObj)) {

			search.recordObj = recordObj;
			NERPsRuntime_EnumerateBlockPointers(stack[0], NERPs_LiveObject_Callback_CountRecordObjectsAtBlock, &search);
		}
	}
	return search.count;
}


// <LegoRR.exe @004549c0>
sint32 __cdecl LegoRR::NERPFunc__SetRecordObjectPointer(sint32* stack)
{
	nerpsfileGlobs.RecordObjectPointer = stack[0];
	return_VOID(stack[0]);
}


// <LegoRR.exe @004549e0>
sint32 __cdecl LegoRR::NERPFunc__GetOxygenLevel(sint32* stack)
{
	// The original logic for this function is so messed up...
	//if (oxygen < 1.0f) oxygen = 0.0f;
	//if (oxygen == 0.0f) {
	//	if (!Teleporter_ServiceAll(OBJECT_TYPE_FLAGS_TELEPORTED)) {
	//		oxygen = 1.0f;
	//	}
	//}
	//if (oxygen < 1.0f) oxygen = 1.0f;

	real32 oxygen = Lego_GetLevel()->oxygenLevel;
	if (oxygen < 1.0f) {
		// WHY ARE WE TRIGGERING THE LEVEL FAILURE TELEPORTING HERE!??
		Teleporter_ServiceAll(OBJECT_TYPE_FLAGS_TELEPORTED);
		oxygen = 1.0f;
	}
	return static_cast<sint32>(oxygen);
}

// <LegoRR.exe @00454a60>
sint32 __cdecl LegoRR::NERPFunc__GenerateSlug(sint32* stack)
{
	/// TODO: Actually reference level's SlugID if assigned...
	///       Don't change this yet in-case level designers abuse this for normal slug spawns.
	LegoObject_Type objType = LegoObject_None;
	LegoObject_ID objID = (LegoObject_ID)0;
	ObjectModel* objModel = nullptr;
	/// FIX APPLY: Check return value for success.
	if (Lego_GetObjectByName(OBJECT_NAME_SLUG, &objType, &objID, &objModel)) {
		/// FIX APPLY: Ensure object type is rock monster.
		if (objType == LegoObject_RockMonster) {
			LegoObject_TryGenerateSlug(nullptr, objID);
		}
	}
	return_VOID(0);
}

// <LegoRR.exe @00454aa0>
sint32 __cdecl LegoRR::NERPFunc__SetAttackDefer(sint32* stack)
{
	Lego_SetAttackDefer(stack[0]);
	return_VOID(0);
}

// <LegoRR.exe @00454ac0>
sint32 __cdecl LegoRR::NERPFunc__SetCallToArms(sint32* stack)
{
	Lego_SetCallToArmsOn(stack[0]);
	return_VOID(0);
}

// <LegoRR.exe @00454ae0>
sint32 __cdecl LegoRR::NERPFunc__GetCallToArmsButtonClicked(sint32* stack)
{
	return (legoGlobs.flags2 & GAME2_CALLTOARMS);
}

// <LegoRR.exe @00454af0>
sint32 __cdecl LegoRR::NERPFunc__SetRockMonster(sint32* stack)
{
	LegoObject_ID objID = Lego_GetEmergeCreatureID();
	/// CHANGE: Handle invalid object ID (is this used for emerge?)
	if (objID != LegoObject_ID_Invalid) {
		uint32 bx = static_cast<uint32>(stack[0] - 1);
		uint32 by = static_cast<uint32>(stack[1] - 1);
		LegoObject_TryGenerateRMonster(&legoGlobs.rockMonsterData[objID], LegoObject_RockMonster, objID, bx, by);
	}
	return_VOID(1);
}

// <LegoRR.exe @00454b30>
sint32 __cdecl LegoRR::NERPFunc__GetRockMonstersDestroyed(sint32* stack)
{
	return rewardGlobs.current.items[Reward_RockMonsters].numDestroyed;
}


// <LegoRR.exe @00454b60>
sint32 __cdecl LegoRR::NERPFunc__GetHiddenObjectsFound(sint32* stack)
{
	return nerpsruntimeGlobs.hiddenObjectsFound;
}

// <LegoRR.exe @00454b70>
sint32 __cdecl LegoRR::NERPFunc__SetHiddenObjectsFound(sint32* stack)
{
	nerpsruntimeGlobs.hiddenObjectsFound = stack[0];
	return_VOID(stack[0]);
}

// <LegoRR.exe @00454b80>
sint32 __cdecl LegoRR::NERPFunc__SetUpgradeBuildingIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_UpgradeBuilding, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00454ba0>
sint32 __cdecl LegoRR::NERPFunc__GetUpgradeBuildingIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_UpgradeBuilding);
}

// <LegoRR.exe @00454bc0>
sint32 __cdecl LegoRR::NERPFunc__FlashUpgradeBuildingIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_UpgradeBuilding, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00454be0>
sint32 __cdecl LegoRR::NERPFunc__SetGoBackIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_Back, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00454c00>
sint32 __cdecl LegoRR::NERPFunc__GetGoBackIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_Back);
}

// <LegoRR.exe @00454c20>
sint32 __cdecl LegoRR::NERPFunc__FlashGoBackIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_Back, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00454c40>
sint32 __cdecl LegoRR::NERPFunc__GetRockMonsterRunningAway(sint32* stack)
{
	SearchNERPsSetObjectHealthPain search = { (SearchNERPsSetObjectHealthPain::Type)0 };
	search.type = SearchNERPsSetObjectHealthPain::Type::GetRunningAway; // 0;
	search.runningAwayCount = 0;
	for (auto obj : objectListSet.EnumerateSkipUpgradeParts()) {
		NERPs_LiveObject_Callback_SetRockMonsterHealthType(obj, &search);
	}
	//LegoObject_RunThroughListsSkipUpgradeParts(NERPs_LiveObject_Callback_SetRockMonsterHealthType, &search);
	return search.runningAwayCount;
}


// <LegoRR.exe @00454d20>
sint32 __cdecl LegoRR::NERPFunc__SetRockMonsterPainThreshold(sint32* stack)
{
	SearchNERPsSetObjectHealthPain search = { (SearchNERPsSetObjectHealthPain::Type)0 };
	search.type = SearchNERPsSetObjectHealthPain::Type::SetPainThreshold; // 1;
	search.painHealthValue = static_cast<real32>(static_cast<uint32>(stack[0]));
	for (auto obj : objectListSet.EnumerateSkipUpgradeParts()) {
		NERPs_LiveObject_Callback_SetRockMonsterHealthType(obj, &search);
	}
	//LegoObject_RunThroughListsSkipUpgradeParts(NERPs_LiveObject_Callback_SetRockMonsterHealthType, &search);
	return_VOID(0);
}

// <LegoRR.exe @00454d60>
sint32 __cdecl LegoRR::NERPFunc__SetRockMonsterHealth(sint32* stack)
{
	SearchNERPsSetObjectHealthPain search = { (SearchNERPsSetObjectHealthPain::Type)0 };
	search.type = SearchNERPsSetObjectHealthPain::Type::SetHealth; // 2;
	search.painHealthValue = static_cast<real32>(static_cast<uint32>(stack[0]));
	for (auto obj : objectListSet.EnumerateSkipUpgradeParts()) {
		NERPs_LiveObject_Callback_SetRockMonsterHealthType(obj, &search);
	}
	//LegoObject_RunThroughListsSkipUpgradeParts(NERPs_LiveObject_Callback_SetRockMonsterHealthType, &search);
	return_VOID(0);
}

// <LegoRR.exe @00454da0>
sint32 __cdecl LegoRR::NERPFunc__SetPauseGame(sint32* stack)
{
	Lego_SetPaused(false, stack[0]);
	return_VOID(1);
}

// <LegoRR.exe @00454dc0>
sint32 __cdecl LegoRR::NERPFunc__GetAnyKeyPressed(sint32* stack)
{
	return Gods98::Input_AnyKeyPressed();
}

// <LegoRR.exe @00454dd0>
sint32 __cdecl LegoRR::NERPFunc__SetIconPos(sint32* stack)
{
	// (global)
	nerpsIconPos = Point2F {
		static_cast<real32>(static_cast<uint32>(stack[0])),
		static_cast<real32>(static_cast<uint32>(stack[1])),
	};
	return_VOID(0);
}

// <LegoRR.exe @00454e10>
sint32 __cdecl LegoRR::NERPFunc__SetIconSpace(sint32* stack)
{
	// (global)
	nerpsIconSpace = stack[0];
	return_VOID(0);
}

// <LegoRR.exe @00454e20>
sint32 __cdecl LegoRR::NERPFunc__SetIconWidth(sint32* stack)
{
	// (global)
	nerpsIconWidth = stack[0];
	return_VOID(0);
}

// <LegoRR.exe @00454e30>
sint32 __cdecl LegoRR::NERPFunc__SetLevelCompleted(sint32* stack)
{
	Objective_SetStatus(LEVELSTATUS_COMPLETE);
	return_VOID(1);
}

// ALIAS: NERPFunc__SetLevelCompleted
// <LegoRR.exe @00454e30>
sint32 __cdecl LegoRR::NERPFunc__SetGameCompleted(sint32* stack)
{
	if (!(legoGlobs.flags1 & GAME1_LEVELENDING)) {
		Objective_SetStatus(LEVELSTATUS_FAILED);
	}
	return_VOID(1);
}

// <LegoRR.exe @00454e40>
sint32 __cdecl LegoRR::NERPFunc__SetLevelFail(sint32* stack)
{
	Objective_SetStatus(LEVELSTATUS_FAILED);
	return_VOID(1);
}

// <LegoRR.exe @00454e60>
sint32 __cdecl LegoRR::NERPFunc__SetGameFail(sint32* stack)
{
	Objective_SetStatus(LEVELSTATUS_FAILED);
	return_VOID(1);
}

// <LegoRR.exe @00454e70>
sint32 __cdecl LegoRR::NERPFunc__SetTutorialPointer(sint32* stack)
{
	NERPsRuntime_SetTutorialPointer(stack[0], stack[1]);
	return_VOID(1);
}


// [NERPsRuntime...]


// <LegoRR.exe @00454f40>
sint32 __cdecl LegoRR::NERPFunc__SetTutorialBlockClicks(sint32* stack)
{
	sint32 clicks = stack[1];
	NERPsRuntime_EnumerateBlockPointers(stack[0], NERPsRuntime_Enumerate_SetTutorialBlockClicks, &clicks);
	return_VOID(1);
}


// [NERPsRuntime...]


// <LegoRR.exe @00454fd0>
sint32 __cdecl LegoRR::NERPFunc__SetTutorialCrystals(sint32* stack)
{
	sint32 count = stack[1];
	NERPsRuntime_EnumerateBlockPointers(stack[0], NERPsRuntime_Enumerate_SetTutorialCrystals, &count);
	return_VOID(1);
}


// [NERPsRuntime...]


// <LegoRR.exe @00455050>
sint32 __cdecl LegoRR::NERPFunc__SetOreAtIconPositions(sint32* stack)
{
	sint32 count = stack[1];
	NERPsRuntime_EnumerateBlockPointers(stack[0], NERPs_SetOreAtBlock, &count);
	return_VOID(1);
}


// [NERPsRuntime...]


// <LegoRR.exe @004550d0>
sint32 __cdecl LegoRR::NERPFunc__GetMiniFigureSelected(sint32* stack)
{
	return NERPsRuntime_CountSelectedUnits_ByObjectName(OBJECT_NAME_PILOT);
}

// <LegoRR.exe @004550e0>
sint32 __cdecl LegoRR::NERPFunc__GetSmallTruckSelected(sint32* stack)
{
	const LegoObject* unit = Interface_GetPrimarySelectedUnit();
	if (unit != nullptr) {
		LegoObject_Type objType = LegoObject_None;
		LegoObject_ID objID = (LegoObject_ID)0;
		/// FIX APPLY: Check return value for success.
		if (Lego_GetObjectByName(OBJECT_NAME_SMALLTRUCK, &objType, &objID, nullptr)) {
			return (unit->type == objType && unit->id == objID);
		}
	}
	return false;
}

// <LegoRR.exe @00455140>
sint32 __cdecl LegoRR::NERPFunc__GetSmallDiggerSelected(sint32* stack)
{

	const LegoObject* unit = Interface_GetPrimarySelectedUnit();
	if (unit != nullptr) {
		LegoObject_Type objType = LegoObject_None;
		LegoObject_ID objID = (LegoObject_ID)0;
		/// FIX APPLY: Check return value for success.
		if (Lego_GetObjectByName(OBJECT_NAME_SMALLDIGGER, &objType, &objID, nullptr)) {
			return (unit->type == objType && unit->id == objID);
		}
	}
	return false;
}

// <LegoRR.exe @004551a0>
sint32 __cdecl LegoRR::NERPFunc__GetRapidRiderSelected(sint32* stack)
{
	const LegoObject* unit = Interface_GetPrimarySelectedUnit();
	if (unit != nullptr) {
		LegoObject_Type objType = LegoObject_None;
		LegoObject_ID objID = (LegoObject_ID)0;
		/// FIX APPLY: Check return value for success.
		if (Lego_GetObjectByName(OBJECT_NAME_RAPIDRIDER, &objType, &objID, nullptr)) {
			return (unit->type == objType && unit->id == objID);
		}
	}
	return false;
}

// <LegoRR.exe @00455200>
sint32 __cdecl LegoRR::NERPFunc__GetSmallHelicopterSelected(sint32* stack)
{
	const LegoObject* unit = Interface_GetPrimarySelectedUnit();
	if (unit != nullptr) {
		LegoObject_Type objType = LegoObject_None;
		LegoObject_ID objID = (LegoObject_ID)0;
		/// FIX APPLY: Check return value for success.
		if (Lego_GetObjectByName(OBJECT_NAME_TUNNELSCOUT, &objType, &objID, nullptr)) {
			return (unit->type == objType && unit->id == objID);
		}
	}
	return false;
}

// <LegoRR.exe @00455260>
sint32 __cdecl LegoRR::NERPFunc__GetGraniteGrinderSelected(sint32* stack)
{
	const LegoObject* unit = Interface_GetPrimarySelectedUnit();
	if (unit != nullptr) {
		LegoObject_Type objType = LegoObject_None;
		LegoObject_ID objID = (LegoObject_ID)0;
		/// FIX APPLY: Check return value for success.
		if (Lego_GetObjectByName(OBJECT_NAME_GRANITEGRINDER, &objType, &objID, nullptr)) {
			return (unit->type == objType && unit->id == objID);
		}
	}
	return false;
}

// <LegoRR.exe @004552c0>
sint32 __cdecl LegoRR::NERPFunc__GetChromeCrusherSelected(sint32* stack)
{
	const LegoObject* unit = Interface_GetPrimarySelectedUnit();
	if (unit != nullptr) {
		LegoObject_Type objType = LegoObject_None;
		LegoObject_ID objID = (LegoObject_ID)0;
		/// FIX APPLY: Check return value for success.
		if (Lego_GetObjectByName(OBJECT_NAME_CHROMECRUSHER, &objType, &objID, nullptr)) {
			return (unit->type == objType && unit->id == objID);
		}
	}
	return false;
}


// [NERPsRuntime...]


// <LegoRR.exe @00455390>
sint32 __cdecl LegoRR::NERPFunc__AddPoweredCrystals(sint32* stack)
{
	/// CHANGE: Call new 'Add' function.
	Level_AddCrystalsStored(stack[0]);
	//for (uint32 i = 0; i < static_cast<uint32>(stack[0]); i++) {
	//	Level_IncCrystalsStored();
	//}
	LegoObject_RequestPowerGridUpdate();
	return_VOID(stack[0]);
}

// <LegoRR.exe @004553c0>
sint32 __cdecl LegoRR::NERPFunc__AddStoredOre(sint32* stack)
{
	/// CHANGE: Call new 'Add' function.
	Level_AddOreStored(false, stack[0]);
	//for (uint32 i = 0; i < static_cast<uint32>(stack[0]); i++) {
	//	Level_IncOreStored(false);
	//}
	return_VOID(stack[0]);
}


// [NERPsRuntime...]


// <LegoRR.exe @00455420>
sint32 __cdecl LegoRR::NERPFunc__GetTutorialCrystals(sint32* stack)
{
	sint32 crystals = 0;
	NERPsRuntime_EnumerateBlockPointers(stack[0], NERPsRuntime_Callback_GetTutorialCrystals, &crystals);
	return crystals;
}


// [NERPsRuntime...]


// <LegoRR.exe @00455480>
sint32 __cdecl LegoRR::NERPFunc__GetTutorialBlockClicks(sint32* stack)
{
	sint32 clicks = 0;
	NERPsRuntime_EnumerateBlockPointers(stack[0], NERPsRuntime_Callback_GetTutorialBlockClicks, &clicks);
	return clicks;
}


// [NERPsRuntime...]


// <LegoRR.exe @00455680>
sint32 __cdecl LegoRR::NERPFunc__GetMiniFigureinGraniteGrinder(sint32* stack)
{
	return NERPs_Game_DoCallbacks_ByObjectName(OBJECT_NAME_GRANITEGRINDER);
}

// <LegoRR.exe @00455690>
sint32 __cdecl LegoRR::NERPFunc__GetMiniFigureinChromeCrusher(sint32* stack)
{
	return NERPs_Game_DoCallbacks_ByObjectName(OBJECT_NAME_CHROMECRUSHER);
}

// <LegoRR.exe @004556a0>
sint32 __cdecl LegoRR::NERPFunc__GetMiniFigureinSmallDigger(sint32* stack)
{
	return NERPs_Game_DoCallbacks_ByObjectName(OBJECT_NAME_SMALLDIGGER);
}

// <LegoRR.exe @004556b0>
sint32 __cdecl LegoRR::NERPFunc__GetMiniFigureinRapidRider(sint32* stack)
{
	return NERPs_Game_DoCallbacks_ByObjectName(OBJECT_NAME_RAPIDRIDER);
}

// <LegoRR.exe @004556c0>
sint32 __cdecl LegoRR::NERPFunc__GetMiniFigureinSmallTruck(sint32* stack)
{
	return NERPs_Game_DoCallbacks_ByObjectName(OBJECT_NAME_SMALLTRUCK);
}

// <LegoRR.exe @004556d0>
sint32 __cdecl LegoRR::NERPFunc__GetMiniFigureinSmallHelicopter(sint32* stack)
{
	return NERPs_Game_DoCallbacks_ByObjectName(OBJECT_NAME_TUNNELSCOUT);
}

// <LegoRR.exe @004556e0>
sint32 __cdecl LegoRR::NERPFunc__SetBarracksLevel(sint32* stack)
{
	NERPs_SetObjectsLevel(OBJECT_NAME_BARRACKS, stack[0]);
	return_VOID(0);
}

// <LegoRR.exe @00455700>
sint32 __cdecl LegoRR::NERPFunc__SetDocksLevel(sint32* stack)
{
	NERPs_SetObjectsLevel(OBJECT_NAME_DOCKS, stack[0]);
	return_VOID(0);
}

// <LegoRR.exe @00455720>
sint32 __cdecl LegoRR::NERPFunc__SetGeoDomeLevel(sint32* stack)
{
	NERPs_SetObjectsLevel(OBJECT_NAME_GEODOME, stack[0]);
	return_VOID(0);
}

// <LegoRR.exe @00455740>
sint32 __cdecl LegoRR::NERPFunc__SetPowerstationLevel(sint32* stack)
{
	NERPs_SetObjectsLevel(OBJECT_NAME_POWERSTATION, stack[0]);
	return_VOID(0);
}

// <LegoRR.exe @00455760>
sint32 __cdecl LegoRR::NERPFunc__SetToolStoreLevel(sint32* stack)
{
	NERPs_SetObjectsLevel(OBJECT_NAME_TOOLSTORE, stack[0]);
	return_VOID(0);
}

// <LegoRR.exe @00455780>
sint32 __cdecl LegoRR::NERPFunc__SetGunstationLevel(sint32* stack)
{
	NERPs_SetObjectsLevel(OBJECT_NAME_GUNSTATION, stack[0]);
	return_VOID(0);
}

// <LegoRR.exe @004557a0>
sint32 __cdecl LegoRR::NERPFunc__SetTeleportPadLevel(sint32* stack)
{
	NERPs_SetObjectsLevel(OBJECT_NAME_TELEPORTPAD, stack[0]);
	return_VOID(0);
}

// <LegoRR.exe @004557c0>
sint32 __cdecl LegoRR::NERPFunc__SetSuperTeleportLevel(sint32* stack)
{
	NERPs_SetObjectsLevel(OBJECT_NAME_SUPERTELEPORT, stack[0]);
	return_VOID(0);
}

// <LegoRR.exe @004557e0>
sint32 __cdecl LegoRR::NERPFunc__SetUpgradeStationLevel(sint32* stack)
{
	NERPs_SetObjectsLevel(OBJECT_NAME_UPGRADESTATION, stack[0]);
	return_VOID(0);
}

// <LegoRR.exe @00455800>
sint32 __cdecl LegoRR::NERPFunc__GetBarracksSelected(sint32* stack)
{
	return NERPsRuntime_CountSelectedUnits_ByObjectName(OBJECT_NAME_BARRACKS);
}

// <LegoRR.exe @00455810>
sint32 __cdecl LegoRR::NERPFunc__GetDocksSelected(sint32* stack)
{
	return NERPsRuntime_CountSelectedUnits_ByObjectName(OBJECT_NAME_DOCKS);
}

// <LegoRR.exe @00455820>
sint32 __cdecl LegoRR::NERPFunc__GetGeoDomeSelected(sint32* stack)
{
	return NERPsRuntime_CountSelectedUnits_ByObjectName(OBJECT_NAME_GEODOME);
}

// <LegoRR.exe @00455830>
sint32 __cdecl LegoRR::NERPFunc__GetPowerstationSelected(sint32* stack)
{
	return NERPsRuntime_CountSelectedUnits_ByObjectName(OBJECT_NAME_POWERSTATION);
}

// <LegoRR.exe @00455840>
sint32 __cdecl LegoRR::NERPFunc__GetToolStoreSelected(sint32* stack)
{
	return NERPsRuntime_CountSelectedUnits_ByObjectName(OBJECT_NAME_TOOLSTORE);
}

// <LegoRR.exe @00455850>
sint32 __cdecl LegoRR::NERPFunc__GetGunstationSelected(sint32* stack)
{
	return NERPsRuntime_CountSelectedUnits_ByObjectName(OBJECT_NAME_GUNSTATION);
}

// <LegoRR.exe @00455860>
sint32 __cdecl LegoRR::NERPFunc__GetTeleportPadSelected(sint32* stack)
{
	return NERPsRuntime_CountSelectedUnits_ByObjectName(OBJECT_NAME_TELEPORTPAD);
}

// <LegoRR.exe @00455870>
sint32 __cdecl LegoRR::NERPFunc__GetSuperTeleportSelected(sint32* stack)
{
	return NERPsRuntime_CountSelectedUnits_ByObjectName(OBJECT_NAME_SUPERTELEPORT);
}

// <LegoRR.exe @00455880>
sint32 __cdecl LegoRR::NERPFunc__GetUpgradeStationSelected(sint32* stack)
{
	return NERPsRuntime_CountSelectedUnits_ByObjectName(OBJECT_NAME_UPGRADESTATION);
}

// <LegoRR.exe @00455890>
sint32 __cdecl LegoRR::NERPFunc__GetPoweredBarracksBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_BARRACKS, NERPS_GETLEVELS_POWERED);
}

// <LegoRR.exe @004558a0>
sint32 __cdecl LegoRR::NERPFunc__GetPoweredDocksBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_DOCKS, NERPS_GETLEVELS_POWERED);
}

// <LegoRR.exe @004558b0>
sint32 __cdecl LegoRR::NERPFunc__GetPoweredGeodomeBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_GEODOME, NERPS_GETLEVELS_POWERED);
}

// <LegoRR.exe @004558c0>
sint32 __cdecl LegoRR::NERPFunc__GetPoweredPowerstationsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_POWERSTATION, NERPS_GETLEVELS_POWERED);
}

// <LegoRR.exe @004558d0>
sint32 __cdecl LegoRR::NERPFunc__GetPoweredToolStoresBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_TOOLSTORE, NERPS_GETLEVELS_POWERED);
}

// <LegoRR.exe @004558e0>
sint32 __cdecl LegoRR::NERPFunc__GetPoweredGunstationsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_GUNSTATION, NERPS_GETLEVELS_POWERED);
}

// <LegoRR.exe @004558f0>
sint32 __cdecl LegoRR::NERPFunc__GetPoweredTeleportsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_TELEPORTPAD, NERPS_GETLEVELS_POWERED);
}

// <LegoRR.exe @00455900>
sint32 __cdecl LegoRR::NERPFunc__GetPoweredVehicleTeleportsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_SUPERTELEPORT, NERPS_GETLEVELS_POWERED);
}

// <LegoRR.exe @00455910>
sint32 __cdecl LegoRR::NERPFunc__GetPoweredUpgradeStationsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_UPGRADESTATION, NERPS_GETLEVELS_POWERED);
}

// <LegoRR.exe @00455920>
sint32 __cdecl LegoRR::NERPFunc__GetBarracksBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_BARRACKS, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455930>
sint32 __cdecl LegoRR::NERPFunc__GetDocksBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_DOCKS, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455940>
sint32 __cdecl LegoRR::NERPFunc__GetGeodomeBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_GEODOME, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455950>
sint32 __cdecl LegoRR::NERPFunc__GetPowerstationsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_POWERSTATION, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455960>
sint32 __cdecl LegoRR::NERPFunc__GetToolStoresBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_TOOLSTORE, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455970>
sint32 __cdecl LegoRR::NERPFunc__GetGunstationsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_GUNSTATION, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455980>
sint32 __cdecl LegoRR::NERPFunc__GetTeleportsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_TELEPORTPAD, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455990>
sint32 __cdecl LegoRR::NERPFunc__GetVehicleTeleportsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_SUPERTELEPORT, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @004559a0>
sint32 __cdecl LegoRR::NERPFunc__GetUpgradeStationsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_UPGRADESTATION, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @004559b0>
sint32 __cdecl LegoRR::NERPFunc__GetLevel1BarracksBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_BARRACKS, 1);
}

// <LegoRR.exe @004559c0>
sint32 __cdecl LegoRR::NERPFunc__GetLevel1DocksBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_DOCKS, 1);
}

// <LegoRR.exe @004559d0>
sint32 __cdecl LegoRR::NERPFunc__GetLevel1GeodomeBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_GEODOME, 1);
}

// <LegoRR.exe @004559e0>
sint32 __cdecl LegoRR::NERPFunc__GetLevel1PowerstationsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_POWERSTATION, 1);
}

// <LegoRR.exe @004559f0>
sint32 __cdecl LegoRR::NERPFunc__GetLevel1ToolStoresBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_TOOLSTORE, 1);
}

// <LegoRR.exe @00455a00>
sint32 __cdecl LegoRR::NERPFunc__GetLevel1GunstationsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_GUNSTATION, 1);
}

// <LegoRR.exe @00455a10>
sint32 __cdecl LegoRR::NERPFunc__GetLevel1TeleportsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_TELEPORTPAD, 1);
}

// <LegoRR.exe @00455a20>
sint32 __cdecl LegoRR::NERPFunc__GetLevel1VehicleTeleportsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_SUPERTELEPORT, 1);
}

// <LegoRR.exe @00455a30>
sint32 __cdecl LegoRR::NERPFunc__GetLevel1UpgradeStationsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_UPGRADESTATION, 1);
}

// <LegoRR.exe @00455a40>
sint32 __cdecl LegoRR::NERPFunc__GetLevel2BarracksBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_BARRACKS, 2);
}

// <LegoRR.exe @00455a50>
sint32 __cdecl LegoRR::NERPFunc__GetLevel2DocksBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_DOCKS, 2);
}

// <LegoRR.exe @00455a60>
sint32 __cdecl LegoRR::NERPFunc__GetLevel2GeodomeBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_GEODOME, 2);
}

// <LegoRR.exe @00455a70>
sint32 __cdecl LegoRR::NERPFunc__GetLevel2PowerstationsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_POWERSTATION, 2);
}

// <LegoRR.exe @00455a80>
sint32 __cdecl LegoRR::NERPFunc__GetLevel2ToolStoresBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_TOOLSTORE, 2);
}

// <LegoRR.exe @00455a90>
sint32 __cdecl LegoRR::NERPFunc__GetLevel2GunstationsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_GUNSTATION, 2);
}

// <LegoRR.exe @00455aa0>
sint32 __cdecl LegoRR::NERPFunc__GetLevel2TeleportsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_TELEPORTPAD, 2);
}

// <LegoRR.exe @00455ab0>
sint32 __cdecl LegoRR::NERPFunc__GetLevel2VehicleTeleportsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_SUPERTELEPORT, 2);
}

// <LegoRR.exe @00455ac0>
sint32 __cdecl LegoRR::NERPFunc__GetLevel2UpgradeStationsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_UPGRADESTATION, 2);
}

// <LegoRR.exe @00455ad0>
sint32 __cdecl LegoRR::NERPFunc__GetBarracksIconClicked(sint32* stack)
{
	return NERPs_SubMenu_GetBuildingVehicleIcon_ByObjectName(OBJECT_NAME_BARRACKS);

}

// <LegoRR.exe @00455ae0>
sint32 __cdecl LegoRR::NERPFunc__GetGeodomeIconClicked(sint32* stack)
{
	return NERPs_SubMenu_GetBuildingVehicleIcon_ByObjectName(OBJECT_NAME_GEODOME);

}

// <LegoRR.exe @00455af0>
sint32 __cdecl LegoRR::NERPFunc__GetPowerstationIconClicked(sint32* stack)
{
	return NERPs_SubMenu_GetBuildingVehicleIcon_ByObjectName(OBJECT_NAME_POWERSTATION);

}

// <LegoRR.exe @00455b00>
sint32 __cdecl LegoRR::NERPFunc__GetToolStoreIconClicked(sint32* stack)
{
	return NERPs_SubMenu_GetBuildingVehicleIcon_ByObjectName(OBJECT_NAME_TOOLSTORE);

}

// <LegoRR.exe @00455b10>
sint32 __cdecl LegoRR::NERPFunc__GetGunstationIconClicked(sint32* stack)
{
	return NERPs_SubMenu_GetBuildingVehicleIcon_ByObjectName(OBJECT_NAME_GUNSTATION);

}

// <LegoRR.exe @00455b20>
sint32 __cdecl LegoRR::NERPFunc__GetTeleportPadIconClicked(sint32* stack)
{
	return NERPs_SubMenu_GetBuildingVehicleIcon_ByObjectName(OBJECT_NAME_TELEPORTPAD);
}

// <LegoRR.exe @00455b30>
sint32 __cdecl LegoRR::NERPFunc__GetVehicleTransportIconClicked(sint32* stack)
{
	return NERPs_SubMenu_GetBuildingVehicleIcon_ByObjectName(OBJECT_NAME_SUPERTELEPORT);
}

// <LegoRR.exe @00455b40>
sint32 __cdecl LegoRR::NERPFunc__GetUpgradeStationIconClicked(sint32* stack)
{
	return NERPs_SubMenu_GetBuildingVehicleIcon_ByObjectName(OBJECT_NAME_UPGRADESTATION);
}

// <LegoRR.exe @00455b50>
sint32 __cdecl LegoRR::NERPFunc__SetBarracksIconClicked(sint32* stack)
{
	auto r = NERPsRuntime_SetSubmenuIconClicked(OBJECT_NAME_BARRACKS, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00455b70>
sint32 __cdecl LegoRR::NERPFunc__SetGeodomeIconClicked(sint32* stack)
{
	auto r = NERPsRuntime_SetSubmenuIconClicked(OBJECT_NAME_GEODOME, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00455b90>
sint32 __cdecl LegoRR::NERPFunc__SetPowerstationIconClicked(sint32* stack)
{
	auto r = NERPsRuntime_SetSubmenuIconClicked(OBJECT_NAME_POWERSTATION, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00455bb0>
sint32 __cdecl LegoRR::NERPFunc__SetToolStoreIconClicked(sint32* stack)
{
	auto r = NERPsRuntime_SetSubmenuIconClicked(OBJECT_NAME_TOOLSTORE, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00455bd0>
sint32 __cdecl LegoRR::NERPFunc__SetGunstationIconClicked(sint32* stack)
{
	auto r = NERPsRuntime_SetSubmenuIconClicked(OBJECT_NAME_GUNSTATION, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00455bf0>
sint32 __cdecl LegoRR::NERPFunc__SetTeleportPadIconClicked(sint32* stack)
{
	auto r = NERPsRuntime_SetSubmenuIconClicked(OBJECT_NAME_TELEPORTPAD, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00455c10>
sint32 __cdecl LegoRR::NERPFunc__SetVehicleTransportIconClicked(sint32* stack)
{
	auto r = NERPsRuntime_SetSubmenuIconClicked(OBJECT_NAME_SUPERTELEPORT, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00455c30>
sint32 __cdecl LegoRR::NERPFunc__SetUpgradeStationIconClicked(sint32* stack)
{
	auto r = NERPsRuntime_SetSubmenuIconClicked(OBJECT_NAME_UPGRADESTATION, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00455c50>
sint32 __cdecl LegoRR::NERPFunc__FlashBarracksIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashSubmenuIcon(OBJECT_NAME_BARRACKS, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00455c70>
sint32 __cdecl LegoRR::NERPFunc__FlashGeodomeIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashSubmenuIcon(OBJECT_NAME_GEODOME, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00455c90>
sint32 __cdecl LegoRR::NERPFunc__FlashPowerStationIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashSubmenuIcon(OBJECT_NAME_POWERSTATION, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00455cb0>
sint32 __cdecl LegoRR::NERPFunc__FlashToolStoreIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashSubmenuIcon(OBJECT_NAME_TOOLSTORE, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00455cd0>
sint32 __cdecl LegoRR::NERPFunc__FlashGunstationIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashSubmenuIcon(OBJECT_NAME_GUNSTATION, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00455cf0>
sint32 __cdecl LegoRR::NERPFunc__FlashTeleportPadIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashSubmenuIcon(OBJECT_NAME_TELEPORTPAD, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00455d10>
sint32 __cdecl LegoRR::NERPFunc__FlashVehicleTransportIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashSubmenuIcon(OBJECT_NAME_SUPERTELEPORT, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00455d30>
sint32 __cdecl LegoRR::NERPFunc__FlashUpgradeStationIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashSubmenuIcon(OBJECT_NAME_UPGRADESTATION, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00455d50>
sint32 __cdecl LegoRR::NERPFunc__GetPathsBuilt(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_PATH, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455d60>
sint32 __cdecl LegoRR::NERPFunc__GetStudCount(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_STUD, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455d70>
sint32 __cdecl LegoRR::NERPFunc__GetSmallHelicoptersOnLevel(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_TUNNELSCOUT, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455d80>
sint32 __cdecl LegoRR::NERPFunc__GetGraniteGrindersOnLevel(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_GRANITEGRINDER, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455d90>
sint32 __cdecl LegoRR::NERPFunc__GetRapidRidersOnLevel(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_RAPIDRIDER, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455da0>
sint32 __cdecl LegoRR::NERPFunc__GetSmallDiggersOnLevel(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_SMALLDIGGER, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455db0>
sint32 __cdecl LegoRR::NERPFunc__GetSlugsOnLevel(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_SLUG, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455dc0>
sint32 __cdecl LegoRR::NERPFunc__GetMiniFiguresOnLevel(sint32* stack)
{
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_PILOT, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455dd0>
sint32 __cdecl LegoRR::NERPFunc__GetOreRefineriesBuilt(sint32* stack)
{
	/// FIXME: This references an object name that is not used by the game: "Refinery".
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_LEGACY_NAME_OREREFINERY, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455de0>
sint32 __cdecl LegoRR::NERPFunc__GetCrystalRefineriesBuilt(sint32* stack)
{
	/// TODO: This references an object name that is not used by the game: "CrystalRefinery".
	return NERPsRuntime_GetLevelObjectsBuilt(OBJECT_LEGACY_NAME_CRYSTALREFINERY, NERPS_GETLEVELS_ALL);
}

// <LegoRR.exe @00455df0>
sint32 __cdecl LegoRR::NERPFunc__GetTeleportIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_TeleportMan);
}

// <LegoRR.exe @00455e10>
sint32 __cdecl LegoRR::NERPFunc__GetDynamiteClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_Dynamite);
}

// <LegoRR.exe @00455e30>
sint32 __cdecl LegoRR::NERPFunc__GetMountIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_GetIn);
}

// <LegoRR.exe @00455e50>
sint32 __cdecl LegoRR::NERPFunc__GetTrainIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_TrainSkill);
}

// <LegoRR.exe @00455e70>
sint32 __cdecl LegoRR::NERPFunc__GetDropSonicBlasterIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_DropBirdScarer);
}

// <LegoRR.exe @00455e90>
sint32 __cdecl LegoRR::NERPFunc__GetGetToolIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_GetTool);
}

// <LegoRR.exe @00455eb0>
sint32 __cdecl LegoRR::NERPFunc__GetGetPusherIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_GetPusherGun);
}

// <LegoRR.exe @00455ed0>
sint32 __cdecl LegoRR::NERPFunc__GetGetSonicBlasterIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_GetBirdScarer);
}

// <LegoRR.exe @00455ef0>
sint32 __cdecl LegoRR::NERPFunc__GetTrainSailorIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_TrainSailor);
}

// <LegoRR.exe @00455f10>
sint32 __cdecl LegoRR::NERPFunc__GetTrainPilotIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_TrainPilot);
}

// <LegoRR.exe @00455f30>
sint32 __cdecl LegoRR::NERPFunc__GetTrainDriverIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_TrainDriver);
}

// <LegoRR.exe @00455f50>
sint32 __cdecl LegoRR::NERPFunc__GetGetLaserIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_GetLaser);
}

// <LegoRR.exe @00455f70>
sint32 __cdecl LegoRR::NERPFunc__GetDismountIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_GetOut);
}

// <LegoRR.exe @00455f90>
sint32 __cdecl LegoRR::NERPFunc__GetDigIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_Dig);
}

// <LegoRR.exe @00455fb0>
sint32 __cdecl LegoRR::NERPFunc__GetBuildIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_BuildBuilding);
}

// <LegoRR.exe @00455fd0>
sint32 __cdecl LegoRR::NERPFunc__GetLayPathIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_LayPath);
}

// <LegoRR.exe @00455ff0>
sint32 __cdecl LegoRR::NERPFunc__GetPlaceFenceIconClicked(sint32* stack)
{
	return Interface_GetIconClicked(Interface_MenuItem_PlaceFence);
}

// <LegoRR.exe @00456010>
sint32 __cdecl LegoRR::NERPFunc__SetTeleportIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_TeleportMan, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456030>
sint32 __cdecl LegoRR::NERPFunc__SetDynamiteClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_Dynamite, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456050>
sint32 __cdecl LegoRR::NERPFunc__SetTrainIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_TrainSkill, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456070>
sint32 __cdecl LegoRR::NERPFunc__SetTrainDriverIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_TrainDriver, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456090>
sint32 __cdecl LegoRR::NERPFunc__SetTrainSailorIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_TrainSailor, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @004560b0>
sint32 __cdecl LegoRR::NERPFunc__SetGetToolIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_GetTool, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @004560d0>
sint32 __cdecl LegoRR::NERPFunc__SetDropSonicBlasterIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_DropBirdScarer, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @004560f0>
sint32 __cdecl LegoRR::NERPFunc__SetGetLaserIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_GetLaser, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456110>
sint32 __cdecl LegoRR::NERPFunc__SetGetPusherIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_GetPusherGun, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456130>
sint32 __cdecl LegoRR::NERPFunc__SetGetSonicBlasterIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_GetBirdScarer, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456150>
sint32 __cdecl LegoRR::NERPFunc__SetDismountIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_GetOut, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456170>
sint32 __cdecl LegoRR::NERPFunc__SetTrainPilotIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_TrainPilot, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456190>
sint32 __cdecl LegoRR::NERPFunc__SetMountIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_GetIn, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @004561b0>
sint32 __cdecl LegoRR::NERPFunc__SetDigIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_Dig, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @004561d0>
sint32 __cdecl LegoRR::NERPFunc__SetBuildIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_BuildBuilding, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @004561f0>
sint32 __cdecl LegoRR::NERPFunc__SetLayPathIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_LayPath, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456210>
sint32 __cdecl LegoRR::NERPFunc__SetPlaceFenceIconClicked(sint32* stack)
{
	auto r = Interface_SetIconClicked(Interface_MenuItem_PlaceFence, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456230>
sint32 __cdecl LegoRR::NERPFunc__FlashTeleportIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_TeleportMan, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456250>
sint32 __cdecl LegoRR::NERPFunc__FlashDynamiteIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_Dynamite, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456270>
sint32 __cdecl LegoRR::NERPFunc__FlashMountIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_GetIn, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456290>
sint32 __cdecl LegoRR::NERPFunc__FlashTrainIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_TrainSkill, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @004562b0>
sint32 __cdecl LegoRR::NERPFunc__FlashTrainDriverIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_TrainDriver, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @004562d0>
sint32 __cdecl LegoRR::NERPFunc__FlashTrainPilotIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_TrainPilot, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @004562f0>
sint32 __cdecl LegoRR::NERPFunc__FlashTrainSailorIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_TrainSailor, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456310>
sint32 __cdecl LegoRR::NERPFunc__FlashDismountIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_GetOut, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456330>
sint32 __cdecl LegoRR::NERPFunc__FlashGetToolIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_GetTool, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456350>
sint32 __cdecl LegoRR::NERPFunc__FlashDropSonicBlasterIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_DropBirdScarer, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456370>
sint32 __cdecl LegoRR::NERPFunc__FlashGetLaserIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_GetLaser, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456390>
sint32 __cdecl LegoRR::NERPFunc__FlashGetPusherIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_GetPusherGun, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @004563b0>
sint32 __cdecl LegoRR::NERPFunc__FlashGetSonicBlasterIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_GetBirdScarer, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @004563d0>
sint32 __cdecl LegoRR::NERPFunc__FlashDigIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_Dig, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @004563f0>
sint32 __cdecl LegoRR::NERPFunc__FlashBuildIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_BuildBuilding, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456410>
sint32 __cdecl LegoRR::NERPFunc__FlashLayPathIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_LayPath, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456430>
sint32 __cdecl LegoRR::NERPFunc__FlashPlaceFenceIcon(sint32* stack)
{
	auto r = NERPsRuntime_FlashIcon(Interface_MenuItem_PlaceFence, stack[0]);
	return_VOID(r);
}

// <LegoRR.exe @00456450>
sint32 __cdecl LegoRR::NERPFunc__GetRandom(sint32* stack)
{
	return Gods98::Maths_Rand() & 0xfff;
}

// <LegoRR.exe @00456460>
sint32 __cdecl LegoRR::NERPFunc__GetRandomTrueFalse(sint32* stack)
{
	return static_cast<bool>(Gods98::Maths_Rand() & 0x1);
}

// <LegoRR.exe @00456470>
sint32 __cdecl LegoRR::NERPFunc__GetRandom10(sint32* stack)
{
	return static_cast<uint32>(Gods98::Maths_Rand()) % 10;
}

// <LegoRR.exe @00456490>
sint32 __cdecl LegoRR::NERPFunc__GetRandom100(sint32* stack)
{
	return static_cast<uint32>(Gods98::Maths_Rand()) % 100;
}

// <LegoRR.exe @004564b0>
sint32 __cdecl LegoRR::NERPFunc__GetCrystalsPickedUp(sint32* stack)
{
	return Lego_GetLevel()->crystalsPickedUp;
}

// <LegoRR.exe @004564c0>
sint32 __cdecl LegoRR::NERPFunc__GetCrystalsCurrentlyStored(sint32* stack)
{
	return Level_GetCrystalCount(true); // Total number of crystals
}


// MERGED FUNCTION
// <LegoRR.exe @004564d0>
sint32 __cdecl LegoRR::NERPFunc__False(sint32* stack)
{
	return false;
}

// ALIAS: NERPFunc__False
// <LegoRR.exe @004564d0>
sint32 __cdecl LegoRR::NERPFunc__Null(sint32* stack)
{
	return 0;
}

// ALIAS: NERPFunc__False (function behaviour is hardcoded, and is never actually called)
// <LegoRR.exe @004564d0>
sint32 __cdecl LegoRR::NERPFunc__Stop(sint32* stack)
{
	return_DUMMY(); // Dummy function, never called directly.
}

// ALIAS: NERPFunc__False (dummy function used at the end-of-list entry for NERPs functions)
// <LegoRR.exe @004564d0>
sint32 __cdecl LegoRR::NERPFunc__End_Of_List(sint32* stack)
{
	return_DUMMY(); // Dummy function, not supposed to be called.
}

// ALIAS: NERPFunc__False (function is unimplemented)
// <LegoRR.exe @004564d0>
sint32 __cdecl LegoRR::NERPFunc__GetCrystalsUsed(sint32* stack)
{
	return_NOT_IMPLEMENTED();
}

// ALIAS: NERPFunc__False (function is unimplemented)
// <LegoRR.exe @004564d0>
sint32 __cdecl LegoRR::NERPFunc__GetCrystalsStolen(sint32* stack)
{
	return_NOT_IMPLEMENTED();
}

// ALIAS: NERPFunc__False (function is unimplemented)
// <LegoRR.exe @004564d0>
sint32 __cdecl LegoRR::NERPFunc__GetOreUsed(sint32* stack)
{
	return_NOT_IMPLEMENTED();
}

// ALIAS: NERPFunc__False (function is unimplemented)
// <LegoRR.exe @004564d0>
sint32 __cdecl LegoRR::NERPFunc__GetOreStolen(sint32* stack)
{
	return_NOT_IMPLEMENTED();
}


// <LegoRR.exe @004564e0>
sint32 __cdecl LegoRR::NERPFunc__GetOrePickedUp(sint32* stack)
{
	return Lego_GetLevel()->orePickedUp;
}

// <LegoRR.exe @004564f0>
sint32 __cdecl LegoRR::NERPFunc__GetOreCurrentlyStored(sint32* stack)
{
	return Lego_GetLevel()->ore;
}

// <LegoRR.exe @00456500>
sint32 __cdecl LegoRR::NERPFunc__GetTutorialFlags(sint32* stack)
{
	return static_cast<sint32>(nerpsruntimeGlobs.tutorialFlags);
}


// <LegoRR.exe @00456510>
sint32 __cdecl LegoRR::NERPFunc__GetR0(sint32* stack)
{
	return nerpsruntimeGlobs.registers[0];
}

// <LegoRR.exe @00456520>
sint32 __cdecl LegoRR::NERPFunc__GetR1(sint32* stack)
{
	return nerpsruntimeGlobs.registers[1];
}

// <LegoRR.exe @00456530>
sint32 __cdecl LegoRR::NERPFunc__GetR2(sint32* stack)
{
	return nerpsruntimeGlobs.registers[2];
}

// <LegoRR.exe @00456540>
sint32 __cdecl LegoRR::NERPFunc__GetR3(sint32* stack)
{
	return nerpsruntimeGlobs.registers[3];
}

// <LegoRR.exe @00456550>
sint32 __cdecl LegoRR::NERPFunc__GetR4(sint32* stack)
{
	return nerpsruntimeGlobs.registers[4];
}

// <LegoRR.exe @00456560>
sint32 __cdecl LegoRR::NERPFunc__GetR5(sint32* stack)
{
	return nerpsruntimeGlobs.registers[5];
}

// <LegoRR.exe @00456570>
sint32 __cdecl LegoRR::NERPFunc__GetR6(sint32* stack)
{
	return nerpsruntimeGlobs.registers[6];
}

// <LegoRR.exe @00456580>
sint32 __cdecl LegoRR::NERPFunc__GetR7(sint32* stack)
{
	return nerpsruntimeGlobs.registers[7];
}


// <LegoRR.exe @00456590>
sint32 __cdecl LegoRR::NERPFunc__AddR0(sint32* stack)
{
	nerpsruntimeGlobs.registers[0] += stack[0];
	return_VOID(nerpsruntimeGlobs.registers[0]);
}

// <LegoRR.exe @004565b0>
sint32 __cdecl LegoRR::NERPFunc__AddR1(sint32* stack)
{
	nerpsruntimeGlobs.registers[1] += stack[0];
	return_VOID(nerpsruntimeGlobs.registers[1]);
}

// <LegoRR.exe @004565d0>
sint32 __cdecl LegoRR::NERPFunc__AddR2(sint32* stack)
{
	nerpsruntimeGlobs.registers[2] += stack[0];
	return_VOID(nerpsruntimeGlobs.registers[2]);
}

// <LegoRR.exe @004565f0>
sint32 __cdecl LegoRR::NERPFunc__AddR3(sint32* stack)
{
	nerpsruntimeGlobs.registers[3] += stack[0];
	return_VOID(nerpsruntimeGlobs.registers[3]);
}

// <LegoRR.exe @00456610>
sint32 __cdecl LegoRR::NERPFunc__AddR4(sint32* stack)
{
	nerpsruntimeGlobs.registers[4] += stack[0];
	return_VOID(nerpsruntimeGlobs.registers[4]);
}

// <LegoRR.exe @00456630>
sint32 __cdecl LegoRR::NERPFunc__AddR5(sint32* stack)
{
	nerpsruntimeGlobs.registers[5] += stack[0];
	return_VOID(nerpsruntimeGlobs.registers[5]);
}

// <LegoRR.exe @00456650>
sint32 __cdecl LegoRR::NERPFunc__AddR6(sint32* stack)
{
	nerpsruntimeGlobs.registers[6] += stack[0];
	return_VOID(nerpsruntimeGlobs.registers[6]);
}

// <LegoRR.exe @00456670>
sint32 __cdecl LegoRR::NERPFunc__AddR7(sint32* stack)
{
	nerpsruntimeGlobs.registers[7] += stack[0];
	return_VOID(nerpsruntimeGlobs.registers[7]);
}


// <LegoRR.exe @00456690>
sint32 __cdecl LegoRR::NERPFunc__SubR0(sint32* stack)
{
	nerpsruntimeGlobs.registers[0] -= stack[0];
	return_VOID(nerpsruntimeGlobs.registers[0]);
}

// <LegoRR.exe @004566b0>
sint32 __cdecl LegoRR::NERPFunc__SubR1(sint32* stack)
{
	nerpsruntimeGlobs.registers[1] -= stack[0];
	return_VOID(nerpsruntimeGlobs.registers[1]);
}

// <LegoRR.exe @004566d0>
sint32 __cdecl LegoRR::NERPFunc__SubR2(sint32* stack)
{
	nerpsruntimeGlobs.registers[2] -= stack[0];
	return_VOID(nerpsruntimeGlobs.registers[2]);
}

// <LegoRR.exe @004566f0>
sint32 __cdecl LegoRR::NERPFunc__SubR3(sint32* stack)
{
	nerpsruntimeGlobs.registers[3] -= stack[0];
	return_VOID(nerpsruntimeGlobs.registers[3]);
}

// <LegoRR.exe @00456710>
sint32 __cdecl LegoRR::NERPFunc__SubR4(sint32* stack)
{
	nerpsruntimeGlobs.registers[4] -= stack[0];
	return_VOID(nerpsruntimeGlobs.registers[4]);
}

// <LegoRR.exe @00456730>
sint32 __cdecl LegoRR::NERPFunc__SubR5(sint32* stack)
{
	nerpsruntimeGlobs.registers[5] -= stack[0];
	return_VOID(nerpsruntimeGlobs.registers[5]);
}

// <LegoRR.exe @00456750>
sint32 __cdecl LegoRR::NERPFunc__SubR6(sint32* stack)
{
	nerpsruntimeGlobs.registers[6] -= stack[0];
	return_VOID(nerpsruntimeGlobs.registers[6]);
}

// <LegoRR.exe @00456770>
sint32 __cdecl LegoRR::NERPFunc__SubR7(sint32* stack)
{
	nerpsruntimeGlobs.registers[7] -= stack[0];
	return_VOID(nerpsruntimeGlobs.registers[7]);
}


// <LegoRR.exe @00456790>
sint32 __cdecl LegoRR::NERPFunc__SetR0(sint32* stack)
{
	nerpsruntimeGlobs.registers[0] = stack[0];
	return_VOID(nerpsruntimeGlobs.registers[0]);
}

// <LegoRR.exe @004567a0>
sint32 __cdecl LegoRR::NERPFunc__SetR1(sint32* stack)
{
	nerpsruntimeGlobs.registers[1] = stack[0];
	return_VOID(nerpsruntimeGlobs.registers[1]);
}

// <LegoRR.exe @004567b0>
sint32 __cdecl LegoRR::NERPFunc__SetR2(sint32* stack)
{
	nerpsruntimeGlobs.registers[2] = stack[0];
	return_VOID(nerpsruntimeGlobs.registers[2]);
}

// <LegoRR.exe @004567c0>
sint32 __cdecl LegoRR::NERPFunc__SetR3(sint32* stack)
{
	nerpsruntimeGlobs.registers[3] = stack[0];
	return_VOID(nerpsruntimeGlobs.registers[3]);
}

// <LegoRR.exe @004567d0>
sint32 __cdecl LegoRR::NERPFunc__SetR4(sint32* stack)
{
	nerpsruntimeGlobs.registers[4] = stack[0];
	return_VOID(nerpsruntimeGlobs.registers[4]);
}

// <LegoRR.exe @004567e0>
sint32 __cdecl LegoRR::NERPFunc__SetR5(sint32* stack)
{
	nerpsruntimeGlobs.registers[5] = stack[0];
	return_VOID(nerpsruntimeGlobs.registers[5]);
}

// <LegoRR.exe @004567f0>
sint32 __cdecl LegoRR::NERPFunc__SetR6(sint32* stack)
{
	nerpsruntimeGlobs.registers[6] = stack[0];
	return_VOID(nerpsruntimeGlobs.registers[6]);
}

// <LegoRR.exe @00456800>
sint32 __cdecl LegoRR::NERPFunc__SetR7(sint32* stack)
{
	nerpsruntimeGlobs.registers[7] = stack[0];
	return_VOID(nerpsruntimeGlobs.registers[7]);
}


// <LegoRR.exe @00456810>
sint32 __cdecl LegoRR::NERPFunc__SetTutorialFlags(sint32* stack)
{
	NERPs_SetTutorialFlags(static_cast<TutorialFlags>(stack[0]));
	return_VOID(stack[0]);
}

// <LegoRR.exe @00456820>
sint32 __cdecl LegoRR::NERPFunc__GetTrainFlags(sint32* stack)
{
	return static_cast<sint32>(objectGlobs.NERPs_TrainFlags);
}

// <LegoRR.exe @00456830>
sint32 __cdecl LegoRR::NERPFunc__SetTrainFlags(sint32* stack)
{
	objectGlobs.NERPs_TrainFlags = static_cast<LegoObject_AbilityFlags>(stack[0]);
	return_VOID(stack[0]);
}

// <LegoRR.exe @00456840>
sint32 __cdecl LegoRR::NERPFunc__GetMonstersOnLevel(sint32* stack)
{
	/// HARDCODED: Monster types.
	/// ALT: Maybe fix by using the 'CanGather' check for rock monsters.
	const sint32 lavaMons = NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_LAVAMONSTER, 0);
	const sint32 rockMons = NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_ROCKMONSTER, 0);
	const sint32 iceMons  = NERPsRuntime_GetLevelObjectsBuilt(OBJECT_NAME_ICEMONSTER, 0);
	return lavaMons + rockMons + iceMons;
}

// <LegoRR.exe @00456880>
sint32 __cdecl LegoRR::NERPFunc__GetBuildingsTeleported(sint32* stack)
{
	// (inlined)
	return LegoObject_GetNumBuildingsTeleported();
	//return objectGlobs.BuildingsTeleported;
}

// <LegoRR.exe @00456890>
sint32 __cdecl LegoRR::NERPFunc__SetBuildingsTeleported(sint32* stack)
{
	LegoObject_SetNumBuildingsTeleported(stack[0]);
	return_VOID(0);
}

// <LegoRR.exe @004568b0>
sint32 __cdecl LegoRR::NERPFunc__SetMessagePermit(sint32* stack)
{
	nerpsruntimeGlobs.messagePermit = stack[0];
	/// FIXME: Exact boolean.
	if (stack[0] == TRUE) {
		Lego_SetFlags2_40_And_2_unkCamera(false, true);
		Lego_SetFlags2_80(false);
	}
	textGlobs.jankCounter = 0;
	textGlobs.currText = nullptr;
	return_VOID(stack[0]);
}


// <LegoRR.exe @00456920>
sint32 __cdecl LegoRR::NERPFunc__SetMessageWait(sint32* stack)
{
	nerpsruntimeGlobs.messageWait = stack[0];
	return_VOID(stack[0]);
}

// <LegoRR.exe @00456930>
sint32 __cdecl LegoRR::NERPFunc__SetMessageTimerValues(sint32* stack)
{
	// Set wait times (sample length multiplier, time added after sample, time for no sample).
	nerpsMessageTimerValues[0] = static_cast<real32>(static_cast<uint32>(stack[0]));
	nerpsMessageTimerValues[1] = static_cast<real32>(static_cast<uint32>(stack[1]));
	nerpsMessageTimerValues[2] = static_cast<real32>(static_cast<uint32>(stack[2]));
	return_VOID(1);
}

// <LegoRR.exe @00456980>
sint32 __cdecl LegoRR::NERPFunc__GetMessageTimer(sint32* stack)
{
	return static_cast<sint32>(nerpsruntimeGlobs.messageTimer);
}

// <LegoRR.exe @00456990>
sint32 __cdecl LegoRR::NERPFunc__SetMessage(sint32* stack)
{
	if (NERPsRuntime_IsMessagePermit()) {
		return_VOID(0);
	}

	uint32 lineIndex = stack[0];
	if (lineIndex != 0) {
		lineIndex--; // 1-indexed, unless someone messed up and used 0-indexed...
	}

	const char* text = NERPsFile_GetMessageLine(lineIndex);

	if (lineIndex != nerpsfileGlobs.lineIndexArray_7c[nerpsfileGlobs.uint_a0]) {
		if (nerpsfileGlobs.uint_a0 == 8) {
			nerpsfileGlobs.lineIndexArray_7c[1] = nerpsfileGlobs.lineIndexArray_7c[5];
			nerpsfileGlobs.lineIndexArray_7c[2] = nerpsfileGlobs.lineIndexArray_7c[6];
			nerpsfileGlobs.lineIndexArray_7c[3] = nerpsfileGlobs.lineIndexArray_7c[7];
			nerpsfileGlobs.lineIndexArray_7c[4] = nerpsfileGlobs.lineIndexArray_7c[8];
			nerpsfileGlobs.uint_a0 = 4;
		}

		nerpsfileGlobs.lineIndexArray_7c[nerpsfileGlobs.uint_a0 + 1] = lineIndex;
		nerpsfileGlobs.uint_a0++;

		/// TODO: Is this check impossible to hit???
		if (nerpsfileGlobs.uint_a0 != 0) {
			Lego_SetFlags2_80(true);
		}
		if (nerpsfileGlobs.int_a4 != 0) {
			nerpsfileGlobs.int_a4++;
		}
	}

	const sint32 jankUnusedCounter = stack[1];

	Text_SetNERPsMessage(text, jankUnusedCounter); // Non-zero for unused counter.
	nerpsruntimeGlobs.nextArrowDisabled = jankUnusedCounter; // Non-zero for next arrow disabled (probably was how long to disable(?)).
	nerpsBOOL_004a7740 = true;

	if (NERPsRuntime_GetMessageWait()) {
		// Ensure the screen has rendered before going into the endless wait loop.
		Gods98::DirectDraw_Flip();

		// ...What is going on here? An infinite loop until the user presses enter??
		// ...This is really bad.

		//while (!Input_IsKeyPressed(Keys::KEY_RETURN)) {
		while (!Shortcut_IsPressed(ShortcutID::NERPsEndMessageWait)) {
			/// CHANGE: Switch to full-blown update loop for proper mouse input support,
			///         and not freezing the entire dang application.
			// Use extension of Main_LoopUpdate that disables graphics updates, since we don't need that.
			Gods98::Main_LoopUpdate2(false, false);
			//Gods98::Input_ReadKeys();

			Shortcuts::shortcutManager.Update(0.0f); // Dummy no elapsed time.
		}
	}

	return_VOID(stack[0]);
}

// <LegoRR.exe @00456a80>
sint32 __cdecl LegoRR::NERPFunc__SetObjectiveSwitch(sint32* stack)
{
	nerpsruntimeGlobs.objectiveSwitch = stack[0];
	return_VOID(stack[0]);
}

// <LegoRR.exe @00456a90>
sint32 __cdecl LegoRR::NERPFunc__GetObjectiveSwitch(sint32* stack)
{
	/// JANK: This function is poorly named, it's intended to be a fire once state for the moment the objective switches from showing.
	if (nerpsruntimeGlobs.objectiveSwitch) {
		nerpsruntimeGlobs.objectiveSwitch = false;
		return true;
	}
	return false;
}

// <LegoRR.exe @00456ab0>
sint32 __cdecl LegoRR::NERPFunc__GetObjectiveShowing(sint32* stack)
{
	return static_cast<bool>(Objective_IsShowing());
}



// [NERPsRuntime...]


// <LegoRR.exe @004573f0>
sint32 __cdecl LegoRR::NERPFunc__MakeSomeoneOnThisBlockPickUpSomethingOnThisBlock(sint32* stack)
{
	SearchNERPsTutorialAction search = { (NERPsTutorialAction)0 };
	search.action = NERPS_TUTORIAL_MAKESOMEONEPICKUP;
	search.blockPointerIdx = stack[0];
	search.result = 0;
	NERPsRuntime_EnumerateBlockPointers(search.blockPointerIdx, c_NERPsRuntime_TutorialActionCallback, &search);
	return_VOID(search.result);
}


// [NERPsRuntime...]


// <LegoRR.exe @00457520>
sint32 __cdecl LegoRR::NERPFunc__SetCongregationAtTutorial(sint32* stack)
{
	SearchNERPsTutorialAction search = { (NERPsTutorialAction)0 };
	if (stack[1])
		search.action = NERPS_TUTORIAL_SETCONGREGATIONATTUTORIAL_START;
	else
		search.action = NERPS_TUTORIAL_SETCONGREGATIONATTUTORIAL_STOP;
	search.blockPointerIdx = stack[0];
	search.result = 0;
	NERPsRuntime_EnumerateBlockPointers(search.blockPointerIdx, c_NERPsRuntime_TutorialActionCallback, &search);
	return_VOID(search.result);
}

// <LegoRR.exe @00457560>
sint32 __cdecl LegoRR::NERPFunc__SetRockMonsterAtTutorial(sint32* stack)
{
	SearchNERPsTutorialAction search = { (NERPsTutorialAction)0 };
	search.action = NERPS_TUTORIAL_SETROCKMONSTERATTUTORIAL;
	search.blockPointerIdx = stack[0];
	search.result = 0;
	NERPsRuntime_EnumerateBlockPointers(search.blockPointerIdx, c_NERPsRuntime_TutorialActionCallback, &search);
	return_VOID(search.result);
}

// <LegoRR.exe @004575a0>
sint32 __cdecl LegoRR::NERPFunc__SetCameraGotoTutorial(sint32* stack)
{
	SearchNERPsTutorialAction search = { (NERPsTutorialAction)0 };
	search.action = NERPS_TUTORIAL_SETCAMERAGOTOTUTORIAL;
	search.blockPointerIdx = stack[0];
	search.result = 0;
	NERPsRuntime_EnumerateBlockPointers(search.blockPointerIdx, c_NERPsRuntime_TutorialActionCallback, &search);
	return_VOID(search.result);
}

// <LegoRR.exe @004575e0>
sint32 __cdecl LegoRR::NERPFunc__GetCameraAtTutorial(sint32* stack)
{
	SearchNERPsTutorialAction search = { (NERPsTutorialAction)0 };
	search.action = NERPS_TUTORIAL_GETCAMERAATTUTORIAL;
	search.blockPointerIdx = stack[0];
	search.result = 0;
	NERPsRuntime_EnumerateBlockPointers(search.blockPointerIdx, c_NERPsRuntime_TutorialActionCallback, &search);
	return search.result;
}

// <LegoRR.exe @00457620>
sint32 __cdecl LegoRR::NERPFunc__GetTutorialBlockIsGround(sint32* stack)
{
	SearchNERPsTutorialAction search = { (NERPsTutorialAction)0 };
	search.action = NERPS_TUTORIAL_GETBLOCKISGROUND;
	search.blockPointerIdx = stack[0];
	search.result = 0;
	NERPsRuntime_EnumerateBlockPointers(search.blockPointerIdx, c_NERPsRuntime_TutorialActionCallback, &search);
	return search.result;
}

// <LegoRR.exe @00457660>
sint32 __cdecl LegoRR::NERPFunc__GetTutorialBlockIsPath(sint32* stack)
{
	SearchNERPsTutorialAction search = { (NERPsTutorialAction)0 };
	search.action = NERPS_TUTORIAL_GETBLOCKISPATH;
	search.blockPointerIdx = stack[0];
	search.result = 0;
	NERPsRuntime_EnumerateBlockPointers(search.blockPointerIdx, c_NERPsRuntime_TutorialActionCallback, &search);
	return search.result;
}

// <LegoRR.exe @004576a0>
sint32 __cdecl LegoRR::NERPFunc__SetTutorialBlockIsGround(sint32* stack)
{
	SearchNERPsTutorialAction search = { (NERPsTutorialAction)0 };
	search.action = NERPS_TUTORIAL_SETBLOCKISGROUND;
	search.blockPointerIdx = stack[0];
	search.result = 0;
	NERPsRuntime_EnumerateBlockPointers(search.blockPointerIdx, c_NERPsRuntime_TutorialActionCallback, &search);
	return_VOID(search.result);
}

// <LegoRR.exe @004576e0>
sint32 __cdecl LegoRR::NERPFunc__SetTutorialBlockIsPath(sint32* stack)
{
	SearchNERPsTutorialAction search = { (NERPsTutorialAction)0 };
	search.action = NERPS_TUTORIAL_SETBLOCKISPATH;
	search.blockPointerIdx = stack[0];
	search.result = 0;
	NERPsRuntime_EnumerateBlockPointers(search.blockPointerIdx, c_NERPsRuntime_TutorialActionCallback, &search);
	return_VOID(search.result);
}

// <LegoRR.exe @00457720>
sint32 __cdecl LegoRR::NERPFunc__GetUnitAtBlock(sint32* stack)
{
	SearchNERPsTutorialAction search = { (NERPsTutorialAction)0 };
	search.action = NERPS_TUTORIAL_GETUNITATBLOCK;
	search.blockPointerIdx = stack[0];
	search.result = 0;
	NERPsRuntime_EnumerateBlockPointers(search.blockPointerIdx, c_NERPsRuntime_TutorialActionCallback, &search);
	return search.result;
}

// <LegoRR.exe @00457760>
sint32 __cdecl LegoRR::NERPFunc__GetMonsterAtTutorial(sint32* stack)
{
	SearchNERPsTutorialAction search = { (NERPsTutorialAction)0 };
	search.action = NERPS_TUTORIAL_GETMONSTERATTUTORIAL;
	search.blockPointerIdx = stack[0];
	search.result = 0;
	NERPsRuntime_EnumerateBlockPointers(search.blockPointerIdx, c_NERPsRuntime_TutorialActionCallback, &search);
	return search.result;
}


// MERGED FUNCTION
// <LegoRR.exe @00484e50>
sint32 __cdecl LegoRR::NERPFunc__True(sint32* stack)
{
	return true;
}


#pragma endregion
