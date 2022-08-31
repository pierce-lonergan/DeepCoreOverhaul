// NERPsFunctions.cpp : 
//

#include "../../engine/drawing/DirectDraw.h"
#include "../../engine/input/Input.h"
#include "../../engine/input/Keys.h"

#include "../interface/Advisor.h"
#include "../interface/Panels.h"
#include "../interface/TextMessages.h"
#include "../Game.h"
#include "../Shortcuts.hpp"

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


// MERGED FUNCTION
// <LegoRR.exe @00484e50>
sint32 __cdecl LegoRR::NERPFunc__True(sint32* stack)
{
	return true;
}


#pragma endregion
