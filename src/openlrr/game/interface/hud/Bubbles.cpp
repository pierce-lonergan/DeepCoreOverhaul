// Bubbles.cpp : 
//

#include "../../../engine/drawing/Draw.h"
#include "../../../engine/Graphics.h"

#include "../../object/AITask.h"
#include "../../object/Object.h"
#include "../../object/Stats.h"
#include "../../Game.h"
#include "ObjInfo.h"

#include "Bubbles.h"


/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define BUBBLE_SHOWPOWEROFF_LOOPDURATION	(STANDARD_FRAMERATE * 1.0f)         // Toggle flash every half a second.
#define BUBBLE_SHOWPOWEROFF_SHOWDURATION	(BUBBLE_SHOWPOWEROFF_LOOPDURATION * 0.5f) // Show flash every other half a second.

#define BUBBLE_SHOWBUBBLE_DURATION			(STANDARD_FRAMERATE * 0.5f)         // Half a second (12.5f)

#define BUBBLE_SHOWCALLTOARMS_DURATION		(STANDARD_FRAMERATE * 60 * 60 * 25) // 25 HOURS (2250000.0f).

#define BUBBLE_SHOWHEALTHBAR_DURATION		(STANDARD_FRAMERATE * 1.5f)         // 1.5 seconds (37.5f)

#pragma endregion
/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004b9a18>
real32 & LegoRR::s_Bubble_PowerOffFlashTimer = *(real32*)0x004b9a18;

// <LegoRR.exe @00558860>
LegoRR::Bubble_Globs & LegoRR::bubbleGlobs = *(LegoRR::Bubble_Globs*)0x00558860;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @00406fe0>
void __cdecl LegoRR::Bubble_Initialise(void)
{
	/// SANITY: Clear memory before use.
	std::memset(&bubbleGlobs, 0, sizeof(bubbleGlobs));

	Bubble_RegisterName(Bubble_CantDo);
	Bubble_RegisterName(Bubble_Idle);
	Bubble_RegisterName(Bubble_CollectCrystal);
	Bubble_RegisterName(Bubble_CollectOre);
	Bubble_RegisterName(Bubble_CollectStud);
	Bubble_RegisterName(Bubble_CollectDynamite);
	Bubble_RegisterName(Bubble_CollectBarrier);
	Bubble_RegisterName(Bubble_CollectElecFence);
	Bubble_RegisterName(Bubble_CollectDrill);
	Bubble_RegisterName(Bubble_CollectSpade);
	Bubble_RegisterName(Bubble_CollectHammer);
	Bubble_RegisterName(Bubble_CollectSpanner);
	Bubble_RegisterName(Bubble_CollectLaser);
	Bubble_RegisterName(Bubble_CollectPusher);
	Bubble_RegisterName(Bubble_CollectFreezer);
	Bubble_RegisterName(Bubble_CollectBirdScarer);
	Bubble_RegisterName(Bubble_CarryCrystal);
	Bubble_RegisterName(Bubble_CarryOre);
	Bubble_RegisterName(Bubble_CarryStud);
	Bubble_RegisterName(Bubble_CarryDynamite);
	Bubble_RegisterName(Bubble_CarryBarrier);
	Bubble_RegisterName(Bubble_CarryElecFence);
	Bubble_RegisterName(Bubble_Goto);
	Bubble_RegisterName(Bubble_Dynamite);
	Bubble_RegisterName(Bubble_Reinforce);
	Bubble_RegisterName(Bubble_Drill);
	Bubble_RegisterName(Bubble_Repair);
	Bubble_RegisterName(Bubble_Dig);
	Bubble_RegisterName(Bubble_Flee);
	Bubble_RegisterName(Bubble_PowerOff);
	Bubble_RegisterName(Bubble_CallToArms);
	Bubble_RegisterName(Bubble_ElecFence);
	Bubble_RegisterName(Bubble_Eat);
	Bubble_RegisterName(Bubble_Drive);
	Bubble_RegisterName(Bubble_Upgrade);
	Bubble_RegisterName(Bubble_BuildPath);
	Bubble_RegisterName(Bubble_Train);
	Bubble_RegisterName(Bubble_Recharge);
	Bubble_RegisterName(Bubble_Request);
}

// <LegoRR.exe @00407170>
void __cdecl LegoRR::Bubble_LoadBubbles(const Gods98::Config* config)
{
	const Gods98::Config* arrayFirst = Gods98::Config_FindArray(config, Lego_ID("Bubbles"));

	for (const Gods98::Config* item = arrayFirst; item != nullptr; item = Gods98::Config_GetNextItem(item)) {

		const char* bubbleName = Gods98::Config_GetItemName(item);

		bool noReduce = true;
		if (std::strlen(bubbleName) > 0 && bubbleName[0] == '!') {
			if (!Gods98::Graphics_IsReduceImages()) {
				bubbleName += 1; // Skip past '!' character.
			}
			else {
				noReduce = false;
			}
		}

		if (noReduce) {
			const Bubble_Type bubbleType = Bubble_GetBubbleType(bubbleName);
			if (bubbleType != Bubble_Type_Count) {
				bubbleGlobs.bubbleImage[bubbleType] = Gods98::Image_LoadBMP(Gods98::Config_GetDataString(item));
				if (bubbleGlobs.bubbleImage[bubbleType] != nullptr) {
					Gods98::Image_SetupTrans(bubbleGlobs.bubbleImage[bubbleType], 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
				}
			}
			else {
				Error_WarnF(true, "Invalid Bubble name %s", bubbleName);
			}
		}
	}
}

// <LegoRR.exe @00407230>
LegoRR::Bubble_Type __cdecl LegoRR::Bubble_GetBubbleType(const char* bubbleName)
{
	for (uint32 i = 0; i < Bubble_Type_Count; i++) {
		if (::_stricmp(bubbleGlobs.bubbleName[i], bubbleName) == 0)
			return static_cast<Bubble_Type>(i);
	}
	return Bubble_Type_Count;
}

// <LegoRR.exe @00407270>
void __cdecl LegoRR::Bubble_ToggleObjectUIsAlwaysVisible(void)
{
	bubbleGlobs.alwaysVisible = !bubbleGlobs.alwaysVisible;
}

// <LegoRR.exe @00407290>
bool32 __cdecl LegoRR::Bubble_GetObjectUIsAlwaysVisible(void)
{
	return bubbleGlobs.alwaysVisible;
}

// <LegoRR.exe @004072a0>
void __cdecl LegoRR::Bubble_ResetObjectBubbleImage(LegoObject* liveObj)
{
	if (bubbleGlobs.alwaysVisible) {
		liveObj->bubbleTimer = 0.0f;
		Bubble_EvaluateObjectBubbleImage(liveObj, &liveObj->bubbleImage);
	}
}

// Remove references from the bubble tables that match the specified object.
// <LegoRR.exe @004072d0>
void __cdecl LegoRR::Bubble_RemoveObjectReferences(LegoObject* deadObj)
{
	Bubble_HideHealthBar(deadObj);
	Bubble_HideBubble(deadObj);
	Bubble_HidePowerOff(deadObj);
	Bubble_HideCallToArms(deadObj);

	/*for (uint32 i = 0; i < BUBBLE_MAXSHOWHEALTHBARS; i++) {
		if (bubbleGlobs.healthBarList[i].object == deadObj) {
			bubbleGlobs.healthBarList[i].object = nullptr; // Free up this bubble.
		}
	}

	for (uint32 i = 0; i < BUBBLE_MAXSHOWBUBBLES; i++) {
		if (bubbleGlobs.bubbleList[i].object == deadObj) {
			bubbleGlobs.bubbleList[i].object = nullptr; // Free up this bubble.
		}
	}

	for (uint32 i = 0; i < BUBBLE_MAXSHOWPOWEROFFS; i++) {
		if (bubbleGlobs.powerOffList[i].object == deadObj) {
			bubbleGlobs.powerOffList[i].object = nullptr; // Free up this bubble.
		}
	}

	for (uint32 i = 0; i < BUBBLE_MAXSHOWCALLTOARMS; i++) {
		if (bubbleGlobs.callToArmsList[i].object == deadObj) {
			bubbleGlobs.callToArmsList[i].object = nullptr; // Free up this bubble.
		}
	}*/
}

// Does not set timer, as Power-Off is a state that Bubble automatically handles removing.
// Only allows Building object types.
// <LegoRR.exe @00407340>
void __cdecl LegoRR::Bubble_ShowPowerOff(LegoObject* liveObj)
{
	if (liveObj->type != LegoObject_Building)
		return; // Only allowed for Buildings.

	for (uint32 i = 0; i < BUBBLE_MAXSHOWPOWEROFFS; i++) {
		if (bubbleGlobs.powerOffList[i].object == liveObj) { // Already in list.
			// Timer is not used by Power-Off bubbles.
			return;
		}
	}

	for (uint32 i = 0; i < BUBBLE_MAXSHOWPOWEROFFS; i++) {
		if (bubbleGlobs.powerOffList[i].object == nullptr) { // Add to list.
			bubbleGlobs.powerOffList[i].object = liveObj;
			// Timer is not used by Power-Off bubbles.
			break;
		}
	}
}

// Sets timer to 0.5 seconds. That's pretty low...
// Only allows MiniFigure object types.
// <LegoRR.exe @00407380>
void __cdecl LegoRR::Bubble_ShowBubble(LegoObject* liveObj)
{
	if (liveObj->type != LegoObject_MiniFigure)
		return; // Only allowed for Mini-Figures.

	for (uint32 i = 0; i < BUBBLE_MAXSHOWBUBBLES; i++) {
		if (bubbleGlobs.bubbleList[i].object == liveObj) { // Already in list.
			bubbleGlobs.bubbleList[i].remainingTimer = BUBBLE_SHOWBUBBLE_DURATION;
			return;
		}
	}
	for (uint32 i = 0; i < BUBBLE_MAXSHOWBUBBLES; i++) {
		if (bubbleGlobs.bubbleList[i].object == nullptr) { // Add to list.
			bubbleGlobs.bubbleList[i].object = liveObj;
			bubbleGlobs.bubbleList[i].remainingTimer = BUBBLE_SHOWBUBBLE_DURATION;
			break;
		}
	}
}

// Only allows MiniFigure object types.
// Sets timer 25 hours, but the timer is never decremented for Call to Arms, so it doesn't matter.
// <LegoRR.exe @004073e0>
void __cdecl LegoRR::Bubble_ShowCallToArms(LegoObject* liveObj)
{
	if (liveObj->type != LegoObject_MiniFigure)
		return; // Only allowed for Mini-Figures.

	for (uint32 i = 0; i < BUBBLE_MAXSHOWCALLTOARMS; i++) {
		if (bubbleGlobs.callToArmsList[i].object == liveObj) { // Already in list.
			bubbleGlobs.callToArmsList[i].remainingTimer = BUBBLE_SHOWCALLTOARMS_DURATION;
			return;
		}
	}
	for (uint32 i = 0; i < BUBBLE_MAXSHOWCALLTOARMS; i++) {
		if (bubbleGlobs.callToArmsList[i].object == nullptr) { // Add to list.
			bubbleGlobs.callToArmsList[i].object = liveObj;
			bubbleGlobs.callToArmsList[i].remainingTimer = BUBBLE_SHOWCALLTOARMS_DURATION;
			break;
		}
	}
}

// Only allows MiniFigure object types.
// Setting timer to 0.0f or lower will stop this object from showing Call to Arms.
// <LegoRR.exe @00407440>
void __cdecl LegoRR::Bubble_SetCallToArmsTimer(LegoObject* liveObj, real32 timer)
{
	if (liveObj->type != LegoObject_MiniFigure)
		return; // Only allowed for Mini-Figures.

	/// OLD CODE: "Bubble_SetAllCallToArmsTimers"
	//for (uint32 i = 0; i < BUBBLE_MAXSHOWCALLTOARMS; i++) {
	//	if (bubbleGlobs.callToArmsList[i].object != nullptr) {
	//		bubbleGlobs.callToArmsList[i].remainingTimer = 0.0f;
	//	}
	//}

	/// FIX APPLY: Change this function to how it was supposed to work.
	///            The way it behaves makes no sense based on the arguments, and this is still called for all alive units.
	for (uint32 i = 0; i < BUBBLE_MAXSHOWCALLTOARMS; i++) {
		if (bubbleGlobs.callToArmsList[i].object == liveObj) {
			bubbleGlobs.callToArmsList[i].remainingTimer = timer;
			/// SANITY: Don't break, for the sake of sanity in-case the object managed to worm its way into the list twice.
			//break;
		}
	}
}

// Sets timer to 1.5 seconds.
// Only allows objects with stats flag STATS3_SHOWHEALTHBAR.
// <LegoRR.exe @00407470>
void __cdecl LegoRR::Bubble_ShowHealthBar(LegoObject* liveObj)
{
	const StatsFlags3 sflags3 = StatsObject_GetStatsFlags3(liveObj);
	if (!(sflags3 & STATS3_SHOWHEALTHBAR))
		return; // This object does not allow showing health bar.

	for (uint32 i = 0; i < BUBBLE_MAXSHOWHEALTHBARS; i++) {
		if (bubbleGlobs.healthBarList[i].object == liveObj) { // Already in list.
			bubbleGlobs.healthBarList[i].remainingTimer = BUBBLE_SHOWHEALTHBAR_DURATION;
			return;
		}
	}
	for (uint32 i = 0; i < BUBBLE_MAXSHOWHEALTHBARS; i++) {
		if (bubbleGlobs.healthBarList[i].object == nullptr) { // Add to list.
			bubbleGlobs.healthBarList[i].object = liveObj;
			bubbleGlobs.healthBarList[i].remainingTimer = BUBBLE_SHOWHEALTHBAR_DURATION;
			break;
		}
	}
}

/// CUSTOM:
void LegoRR::Bubble_HidePowerOff(LegoObject* liveObj)
{
	for (uint32 i = 0; i < BUBBLE_MAXSHOWPOWEROFFS; i++) {
		if (bubbleGlobs.powerOffList[i].object == liveObj) {
			bubbleGlobs.powerOffList[i].object = nullptr; // Free up this bubble.
		}
	}
}

/// CUSTOM:
void LegoRR::Bubble_HideBubble(LegoObject* liveObj)
{
	for (uint32 i = 0; i < BUBBLE_MAXSHOWBUBBLES; i++) {
		if (bubbleGlobs.bubbleList[i].object == liveObj) {
			bubbleGlobs.bubbleList[i].object = nullptr; // Free up this bubble.
		}
	}
}

/// CUSTOM:
void LegoRR::Bubble_HideCallToArms(LegoObject* liveObj)
{
	for (uint32 i = 0; i < BUBBLE_MAXSHOWCALLTOARMS; i++) {
		if (bubbleGlobs.callToArmsList[i].object == liveObj) {
			bubbleGlobs.callToArmsList[i].object = nullptr; // Free up this bubble.
		}
	}
}

/// CUSTOM:
void LegoRR::Bubble_HideHealthBar(LegoObject* liveObj)
{
	for (uint32 i = 0; i < BUBBLE_MAXSHOWHEALTHBARS; i++) {
		if (bubbleGlobs.healthBarList[i].object == liveObj) {
			bubbleGlobs.healthBarList[i].object = nullptr; // Free up this bubble.
		}
	}
}

// <LegoRR.exe @004074d0>
void __cdecl LegoRR::Bubble_DrawAllObjInfos(real32 elapsedAbs)
{
	// Draw info for all Mini-Figure units (health bar, hunger image, bubble image).
	// And skip drawing for Mini-Figure units in the lists below.
	if (bubbleGlobs.alwaysVisible && !Lego_IsFirstPersonView()) {
		for (LegoObject* obj : objectListSet.EnumerateSkipUpgradeParts()) {
			Bubble_Callback_DrawObjInfo(obj, &elapsedAbs);
		}
		//LegoObject_RunThroughListsSkipUpgradeParts(Bubble_Callback_DrawObjInfo, &elapsedAbs);
	}

	// Draw health bars over units for a period of time.
	for (uint32 i = 0; i < BUBBLE_MAXSHOWHEALTHBARS; i++) {
		Bubble* bubble = &bubbleGlobs.healthBarList[i];
		if (bubble->object == nullptr)
			continue; // Bubble is unused.

		// Other units are allowed to show health bars, but are not handled by alwaysVisible drawing.
		// Unlike Power-Off, we want to check Lego_IsFirstPersonView here to handle updating/removing logic.
		if ((bubble->object->type != LegoObject_MiniFigure || !bubbleGlobs.alwaysVisible) &&
			!Lego_IsFirstPersonView() && !(bubble->object->flags1 & (LIVEOBJ1_TELEPORTINGDOWN|LIVEOBJ1_TELEPORTINGUP)))
		{
			Vector3F wPos = { 0.0f }; // dummy init
			Gods98::Container* cont = LegoObject_GetActivityContainer(bubble->object);
			Gods98::Container_GetPosition(cont, nullptr, &wPos);
			wPos.z -= StatsObject_GetCollHeight(bubble->object); // Raise Z to top of collision box.

			Point2F screenPt = { 0.0f }; // dummy init
			Gods98::Viewport_WorldToScreen(legoGlobs.viewMain, &screenPt, &wPos);

			ObjInfo_DrawHealthBar(bubble->object, static_cast<sint32>(screenPt.x), static_cast<sint32>(screenPt.y));
		}

		bubble->remainingTimer -= elapsedAbs;
		if (bubble->remainingTimer < 0.0f) {
			bubble->object = nullptr; // Free up this bubble.
		}
	}

	// Draw flashing power-off thunderbolt icons over units.
	if (!Lego_IsFirstPersonView()) {
		// This flashes on/off every half-second.
		if (s_Bubble_PowerOffFlashTimer < BUBBLE_SHOWPOWEROFF_SHOWDURATION) {
			Gods98::Image* powerOffImage = bubbleGlobs.bubbleImage[Bubble_PowerOff];

			for (uint32 i = 0; i < BUBBLE_MAXSHOWPOWEROFFS; i++) {
				Bubble* bubble = &bubbleGlobs.powerOffList[i];
				if (bubble->object == nullptr)
					continue; // Bubble is unused.

				// Confirm if the unit still has no power.
				if (!(bubble->object->flags3 & LIVEOBJ3_HASPOWER)) {
					Vector3F wPos = { 0.0f }; // dummy init
					Gods98::Container* cont = LegoObject_GetActivityContainer(bubble->object);
					Gods98::Container_GetPosition(cont, nullptr, &wPos);
					wPos.z -= StatsObject_GetCollHeight(bubble->object); // Raise Z to top of collision box.

					Point2F screenPt = { 0.0f }; // dummy init
					Gods98::Viewport_WorldToScreen(legoGlobs.viewMain, &screenPt, &wPos);

					/// SANITY: Null check powerOffImage.
					///         We still want to handle the outside logic in-case bubbles have more use than just visuals.
					if (powerOffImage != nullptr) {
						screenPt.x -= static_cast<real32>(Gods98::Image_GetWidth(powerOffImage)) / 2.0f;
						screenPt.y -= static_cast<real32>(Gods98::Image_GetHeight(powerOffImage));
						Gods98::Image_Display(powerOffImage, &screenPt);
					}
				}
				else {
					bubble->object = nullptr; // Free up this bubble.
				}
			}
		}
		else if (s_Bubble_PowerOffFlashTimer >= BUBBLE_SHOWPOWEROFF_LOOPDURATION) {
			/// SANITY: Ensure proper wrapping using fmod, not just subtraction.
			s_Bubble_PowerOffFlashTimer = std::fmod(s_Bubble_PowerOffFlashTimer, BUBBLE_SHOWPOWEROFF_LOOPDURATION);
			//s_Bubble_PowerOffFlashTimer -= BUBBLE_SHOWPOWEROFF_LOOPDURATION;
		}

		s_Bubble_PowerOffFlashTimer += elapsedAbs;
	}

	// Draw standard bubbles for units for a period of time.
	for (uint32 i = 0; i < BUBBLE_MAXSHOWBUBBLES; i++) {
		Bubble* bubble = &bubbleGlobs.bubbleList[i];
		if (bubble->object == nullptr)
			continue; // Bubble is unused.

		// Other units are allowed to show bubbles, but are not handled by alwaysVisible drawing.
		// Unlike Power-Off, we want to check Lego_IsFirstPersonView here to handle updating/removing logic.
		if ((bubble->object->type != LegoObject_MiniFigure || !bubbleGlobs.alwaysVisible) &&
			!Lego_IsFirstPersonView() && !(bubble->object->flags1 & (LIVEOBJ1_TELEPORTINGDOWN|LIVEOBJ1_TELEPORTINGUP)))
		{
			Gods98::Image* bubbleImage = nullptr;
			Point2F screenPt = { 0.0f };
			Bubble_UpdateAndGetBubbleImage(bubble->object, elapsedAbs, &bubbleImage, &screenPt);

			// Bubble_Idle is only drawn during alwaysVisible.
			if (bubbleImage != nullptr && bubbleImage != bubbleGlobs.bubbleImage[Bubble_Idle]) {
				ObjInfo_DrawBubbleImage(bubbleImage, static_cast<sint32>(screenPt.x), static_cast<sint32>(screenPt.y));
			}
		}

		bubble->remainingTimer -= elapsedAbs;
		if (bubble->remainingTimer < 0.0f) {
			bubble->object = nullptr; // Free up this bubble.
		}
	}

	// Draw Call to Arms bubbles for units on alert.
	for (uint32 i = 0; i < BUBBLE_MAXSHOWCALLTOARMS; i++) {
		Bubble* bubble = &bubbleGlobs.callToArmsList[i];
		if (bubble->object == nullptr)
			continue; // Bubble is unused.

		// Only Mini-Figures show the Call To Arms bubble, so no need to check object type.
		// Unlike Power-Off, we want to check Lego_IsFirstPersonView here to handle updating/removing logic.
		if (!bubbleGlobs.alwaysVisible &&
			!Lego_IsFirstPersonView() && !(bubble->object->flags1 & (LIVEOBJ1_TELEPORTINGDOWN|LIVEOBJ1_TELEPORTINGUP)))
		{
			Gods98::Image* bubbleImage = nullptr;
			Point2F screenPt = { 0.0f };
			Bubble_UpdateAndGetBubbleImage(bubble->object, elapsedAbs, &bubbleImage, &screenPt);

			if (bubbleImage != nullptr) {
				ObjInfo_DrawBubbleImage(bubbleImage, static_cast<sint32>(screenPt.x), static_cast<sint32>(screenPt.y));
			}
		}

		/// TODO: Why is this the only comparison that's `<=` rather than `<`, and without a timer decrement??
		if (bubble->remainingTimer <= 0.0f) {
			bubble->object = nullptr; // Free up this bubble.
		}
	}
}

// <LegoRR.exe @004077f0>
void __cdecl LegoRR::Bubble_UpdateAndGetBubbleImage(LegoObject* liveObj, real32 elapsedAbs, OUT Gods98::Image** bubbleImage, OUT Point2F* screenPt)
{
	Vector3F wPos = { 0.0f }; // dummy init
	Gods98::Container* cont = LegoObject_GetActivityContainer(liveObj);
	Gods98::Container_GetPosition(cont, nullptr, &wPos);
	wPos.z -= StatsObject_GetCollHeight(liveObj); // Raise Z to top of collision box.

	Gods98::Viewport_WorldToScreen(legoGlobs.viewMain, screenPt, &wPos);

	liveObj->bubbleTimer -= elapsedAbs;
	if (liveObj->bubbleTimer > 0.0f) {
		*bubbleImage = liveObj->bubbleImage;
	}
	else {
		// Re-evaluate bubble image.
		Bubble_EvaluateObjectBubbleImage(liveObj, bubbleImage);
	}

	if (*bubbleImage == nullptr) {
		*bubbleImage = bubbleGlobs.bubbleImage[Bubble_Idle]; // Default bubble.
	}
}

// DATA: real32* elapsedAbs
// <LegoRR.exe @00407890>
bool32 __cdecl LegoRR::Bubble_Callback_DrawObjInfo(LegoObject* liveObj, void* pElapsedAbs)
{
	const real32 elapsedAbs = *static_cast<real32*>(pElapsedAbs);

	if (liveObj->type == LegoObject_MiniFigure && !(liveObj->flags1 & (LIVEOBJ1_TELEPORTINGDOWN|LIVEOBJ1_TELEPORTINGUP))) {
		Gods98::Image* bubbleImage = nullptr;
		Point2F screenPt = { 0.0f };
		Bubble_UpdateAndGetBubbleImage(liveObj, elapsedAbs, &bubbleImage, &screenPt);

		const sint32 x = static_cast<sint32>(screenPt.x);
		const sint32 y = static_cast<sint32>(screenPt.y);

		ObjInfo_DrawHealthBar(liveObj, x, y);
		ObjInfo_DrawHungerImage(liveObj, x, y);

		if (bubbleImage != nullptr) {
			ObjInfo_DrawBubbleImage(bubbleImage, x, y);
		}
	}
	return false;
}

// <LegoRR.exe @00407940>
void __cdecl LegoRR::Bubble_EvaluateObjectBubbleImage(LegoObject* liveObj, OUT Gods98::Image** bubbleImage)
{
	// First check some actions that aren't a task type.
	if (liveObj->flags4 & LIVEOBJ4_CALLTOARMS) {
		*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CallToArms];
	}
	else if (liveObj->flags1 & LIVEOBJ1_RUNNINGAWAY) {
		*bubbleImage = bubbleGlobs.bubbleImage[Bubble_Flee];
	}
	else if (liveObj->flags1 & LIVEOBJ1_CANTDO) {
		*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CantDo];
	}
	else if (liveObj->aiTask != nullptr) {
		// Follow the linked list to find the unit's upcoming action.
		AITask* aiTask = liveObj->aiTask;
		while (aiTask->taskType == AITask_Type_AnimationWait && aiTask->next != nullptr) {
			aiTask = aiTask->next;
		}

		switch (aiTask->taskType) {
		case AITask_Type_Goto:
			*bubbleImage = bubbleGlobs.bubbleImage[Bubble_Goto];
			return;
		case AITask_Type_Collect:
			switch (aiTask->targetObject->type) {
			case LegoObject_PowerCrystal:
				*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CollectCrystal];
				return;
			case LegoObject_Ore:
				if (aiTask->targetObject->id == LegoObject_ID_Ore)
					*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CollectOre];
				else
					*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CollectStud];
				return;
			case LegoObject_Dynamite:
				*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CollectDynamite];
				return;
			case LegoObject_Barrier:
				*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CollectBarrier];
				return;
			case LegoObject_ElectricFence:
				*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CollectElecFence];
				return;
			}
			break;
		case AITask_Type_Deposit:
			/// FIX APPLY: Ensure carried object is valid by checking count.
			if (liveObj->numCarriedObjects >= 1 && liveObj->carriedObjects[0] != nullptr) {
				switch (liveObj->carriedObjects[0]->type) {
				case LegoObject_PowerCrystal:
					*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CarryCrystal];
					return;
				case LegoObject_Ore:
					if (liveObj->carriedObjects[0]->id == LegoObject_ID_Ore)
						*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CarryOre];
					else
						*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CarryStud];
					return;
				case LegoObject_Dynamite:
					*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CarryDynamite];
					return;
				case LegoObject_Barrier:
					*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CarryBarrier];
					return;
				case LegoObject_ElectricFence:
					*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CarryElecFence];
					return;
				}
			}
			break;
		case AITask_Type_Request:
			*bubbleImage = bubbleGlobs.bubbleImage[Bubble_Request];
			break;
		case AITask_Type_Dig:
			*bubbleImage = bubbleGlobs.bubbleImage[Bubble_Drill];
			return;
		case AITask_Type_Dynamite:
			*bubbleImage = bubbleGlobs.bubbleImage[Bubble_Dynamite];
			return;
		case AITask_Type_Repair:
			*bubbleImage = bubbleGlobs.bubbleImage[Bubble_Repair];
			return;
		case AITask_Type_Reinforce:
			*bubbleImage = bubbleGlobs.bubbleImage[Bubble_Reinforce];
			return;
		case AITask_Type_Clear:
			*bubbleImage = bubbleGlobs.bubbleImage[Bubble_Dig];
			return;
		case AITask_Type_ElecFence:
			*bubbleImage = bubbleGlobs.bubbleImage[Bubble_ElecFence];
			return;
		case AITask_Type_Eat:
		case AITask_Type_GotoEat:
			// Shared bubble for tasks.
			*bubbleImage = bubbleGlobs.bubbleImage[Bubble_Eat];
			return;
		case AITask_Type_FindDriver:
			*bubbleImage = bubbleGlobs.bubbleImage[Bubble_Drive];
			return;
		case AITask_Type_GetTool:
			switch (aiTask->toolType) {
			case LegoObject_ToolType_Drill:
				*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CollectDrill];
				return;
			case LegoObject_ToolType_Spade:
				*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CollectSpade];
				return;
			case LegoObject_ToolType_Hammer:
				*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CollectHammer];
				return;
			case LegoObject_ToolType_Spanner:
				*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CollectSpanner];
				return;
			case LegoObject_ToolType_Laser:
				*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CollectLaser];
				return;
			case LegoObject_ToolType_PusherGun:
				*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CollectPusher];
				return;
			case LegoObject_ToolType_FreezerGun:
				*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CollectFreezer];
				return;
			case LegoObject_ToolType_BirdScarer:
				*bubbleImage = bubbleGlobs.bubbleImage[Bubble_CollectBirdScarer];
				return;
			}
			break;
		case AITask_Type_Upgrade:
			*bubbleImage = bubbleGlobs.bubbleImage[Bubble_Upgrade];
			return;
		case AITask_Type_BuildPath:
			*bubbleImage = bubbleGlobs.bubbleImage[Bubble_BuildPath];
			return;
		case AITask_Type_Train:
			*bubbleImage = bubbleGlobs.bubbleImage[Bubble_Train];
			return;
		case AITask_Type_Recharge:
			*bubbleImage = bubbleGlobs.bubbleImage[Bubble_Recharge];
			return;
		}
	}
}

#pragma endregion
