// Objective.cpp : 
//

#include "../../engine/audio/3DSound.h"
#include "../../engine/input/Input.h"

#include "../audio/SFX.h"
#include "../front/Reward.h"
#include "../interface/Advisor.h"
#include "../interface/TextMessages.h"
#include "../interface/ToolTip.h"
#include "../interface/hud/Bubbles.h"
#include "../world/Teleporter.h"
#include "../Game.h"
#include "NERPsFile.h"
#include "NERPsFunctions.h"

#include "Objective.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @00500bc0>
LegoRR::Objective_Globs & LegoRR::objectiveGlobs = *(LegoRR::Objective_Globs*)0x00500bc0;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions


// <LegoRR.exe @00458ba0>
void __cdecl LegoRR::Objective_StopShowing(void)
{
	// End the current showing objective.
	if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_BRIEFING) {

		// nerpsruntimeGlobs.objectiveSwitch = objectiveGlobs.objectiveSwitch;
		//  (before we set ours to false)
		NERPFunc__SetObjectiveSwitch(&objectiveGlobs.objectiveSwitch); // This *should* be true at this point in the program.

		objectiveGlobs.flags &= ~OBJECTIVE_GLOB_FLAG_BRIEFING;
		objectiveGlobs.objectiveSwitch = false;

		// We're entering the level, so start playing music, if enabled.
		/// CHANGE: This now also checks Lego_IsMusicPlaying.
		Lego_ChangeMusicPlaying(true); // Start music.
	}
	else if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_COMPLETED) {

		objectiveGlobs.flags &= ~OBJECTIVE_GLOB_FLAG_COMPLETED;
	}
	else if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_FAILED) {

		objectiveGlobs.flags &= ~(OBJECTIVE_GLOB_FLAG_FAILED|OBJECTIVE_GLOB_FLAG_CRYSTAL); // Clear failed and potential failed reason flags.
	}

	Advisor_End();

	if (objectiveGlobs.soundHandle != -1) {
		Gods98::Sound3D_Stream_Stop(false);
		objectiveGlobs.soundHandle = -1;
	}
	Lego_SetPaused(false, false);
	SFX_SetQueueMode_AndFlush(false);

	if (!NERPs_AnyTutorialFlags()) {
		legoGlobs.flags2 &= ~GAME2_INMENU;
	}
}

// Returns true if the mission briefing or failure/complete window is showing.
// <LegoRR.exe @00458c60>
bool32 __cdecl LegoRR::Objective_IsShowing(void)
{
	return (objectiveGlobs.flags & (OBJECTIVE_GLOB_FLAG_BRIEFING|OBJECTIVE_GLOB_FLAG_COMPLETED|OBJECTIVE_GLOB_FLAG_FAILED));
}

// <LegoRR.exe @00458c80>
bool32 __cdecl LegoRR::Objective_HandleKeys(bool32 nextKeyPressed, bool32 leftButtonReleased, OUT bool32* exitGame)
{
	LevelStatus briefIdx;
	if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_BRIEFING) {
		briefIdx = LEVELSTATUS_INCOMPLETE;
	}
	else if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_COMPLETED) {
		briefIdx = LEVELSTATUS_COMPLETE;
	}
	else if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_FAILED) {
		briefIdx = (!(objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_CRYSTAL) ? LEVELSTATUS_FAILED
																		  : LEVELSTATUS_FAILED_CRYSTALS);
	}
	else {
		/// FIX APPLY: Handle LEVELSTATUS_FAILED_OTHER now to avoid reading outside bounds of the arrays.
		//briefIdx = LEVELSTATUS_FAILED_OTHER;
		return Objective_IsShowing(); // Should be false.
	}

	*exitGame = false;

	/// CHANGE: Used the button positions specified by Objective_Update (which is called at least once before Objective_HandleKeys).
	/// HARDCODED: UI button dimensions/positions for briefing.
	//const Point2I nextPos = { 470, 315 };
	//const Point2I prevPos = { 130, 315 };
	const Point2I nextPos = { (sint32)legoGlobs.menuNextPoint.x, (sint32)legoGlobs.menuNextPoint.y };
	const Point2I prevPos = { (sint32)legoGlobs.menuPrevPoint.x, (sint32)legoGlobs.menuPrevPoint.y };

	const bool overNext = (legoGlobs.NextButtonImage != nullptr && (legoGlobs.flags2 & GAME2_MENU_HASNEXT) &&
	                       (Gods98::msx() >= nextPos.x && Gods98::msx() < nextPos.x + (sint32)Gods98::Image_GetWidth(legoGlobs.NextButtonImage)) &&
	                       (Gods98::msy() >= nextPos.y && Gods98::msy() < nextPos.y + (sint32)Gods98::Image_GetHeight(legoGlobs.NextButtonImage)));
	
	const bool overPrev = (legoGlobs.BackButtonImage != nullptr && (legoGlobs.flags2 & GAME2_MENU_HASPREVIOUS) &&
	                       (Gods98::msx() >= prevPos.x && Gods98::msx() < prevPos.x + (sint32)Gods98::Image_GetWidth(legoGlobs.BackButtonImage)) &&
	                       (Gods98::msy() >= prevPos.y && Gods98::msy() < prevPos.y + (sint32)Gods98::Image_GetHeight(legoGlobs.BackButtonImage)));

	// Press detection for last page:
	// Pressing space or clicking anywhere (except over the previous button) will act as the end of the message.
	/// FIX APPLY: Change space (NextPage shortcut) behaviour to ignore overPrev.
	if (objectiveGlobs.currentPages[briefIdx] == objectiveGlobs.pageCounts[briefIdx]) {
		if (nextKeyPressed || (!overPrev && leftButtonReleased)) {

			if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_BRIEFING) {

				Objective_StopShowing();
				Lego_SetMenuNextPosition(nullptr);
				Lego_SetMenuPreviousPosition(nullptr);
				Lego_SetPaused(false, false);

				// Start the level by centering the cursor in the middle of the screen.
				Gods98::Input_SetCursorPos((Gods98::appWidth() / 2), (Gods98::appHeight() / 2));

				// Start the level with object huds off.
				/// TODO: Where is the best place to actually handle this??
				if (Bubble_GetObjectUIsAlwaysVisible()) {
					Bubble_ToggleObjectUIsAlwaysVisible();
				}
			}
			else if (objectiveGlobs.flags & (OBJECTIVE_GLOB_FLAG_COMPLETED|OBJECTIVE_GLOB_FLAG_FAILED)) {

				// Check if all teleporting units have been beamed up.
				if (Teleporter_ServiceAll(OBJECT_TYPE_FLAGS_TELEPORTED)) {
					Objective_StopShowing();

					// Exit game if there's no next level/user selects quit from the front end.
					*exitGame = !Lego_EndLevel();
				}
			}
		}
	}
	
	// Press detection for 'next page' button:
	if (objectiveGlobs.currentPages[briefIdx] < objectiveGlobs.pageCounts[briefIdx]) {
		bool nextButtonPressed = false;
		if (overNext) {
			ToolTip_AddFlag4(ToolTip_More);
			if (leftButtonReleased) {
				Lego_SetPointerSFX(PointerSFX_Okay);
				nextButtonPressed = true;
			}
		}
		if (nextKeyPressed || nextButtonPressed) {
			objectiveGlobs.currentPages[briefIdx]++;
		}
	}

	// Press detection for 'previous page' button:
	if (objectiveGlobs.currentPages[briefIdx] > 0) {
		if (overPrev) {
			ToolTip_AddFlag4(ToolTip_Back);
			if (leftButtonReleased) {
				Lego_SetPointerSFX(PointerSFX_Okay);
				objectiveGlobs.currentPages[briefIdx]--;
			}
		}
	}

	return Objective_IsShowing();
}

// <LegoRR.exe @00458ea0>
void __cdecl LegoRR::Objective_Update(Gods98::TextWindow* textWnd, Lego_Level* level, real32 elapsedGame, real32 elapsedAbs)
{
	LevelStatus briefIdx = LEVELSTATUS_FAILED_OTHER; // not showing briefing or completed/failure messages.
	bool stopMusic = false; // Set to true for completed/failure messages only.
	bool flushSFXQueue = false; // Set to true when briefing message only.

	if (Gods98::Main_ProgrammerMode() > 3) {
		Objective_StopShowing(); // Higher programmer modes disable intro/outro briefing entirely.
	}

	if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_BRIEFING) {
		briefIdx = LEVELSTATUS_INCOMPLETE; // LEVELSTATUS_INCOMPLETE;
		flushSFXQueue = true;

		Lego_SetPaused(false, true);
		
		if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_STATUSREADY) {
			objectiveGlobs.flags &= ~OBJECTIVE_GLOB_FLAG_STATUSREADY;
			if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_SHOWBRIEFINGADVISOR) {
				Advisor_Start(Advisor_Objective, true);
			}
		}
		Text_DisplayMessage(Text_SpaceToContinue, false, false);
	}
	else if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_COMPLETED) {
		if (Teleporter_ServiceAll(OBJECT_TYPE_FLAGS_TELEPORTED)) {
			briefIdx = LEVELSTATUS_COMPLETE;
			stopMusic = true;

			ObjectRecall_Save_CopyToNewObjectRecallData();
			Lego_SetPaused(false, true);
			if (!level->objective.achievedVideoPlayed) {
				Point2F* optVideoPos = nullptr;
				if (!level->objective.noAchievedVideo) {
					optVideoPos = &level->objective.achievedVideoPosition;
				}
				Lego_PlayMovie_old(level->objective.achievedVideoName, optVideoPos);
				level->objective.achievedVideoPlayed = true;
			}

			if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_STATUSREADY) {
				objectiveGlobs.flags &= ~OBJECTIVE_GLOB_FLAG_STATUSREADY;
				if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_SHOWACHIEVEDADVISOR) {
					Advisor_Start(Advisor_Objective, true);
				}
			}
			Text_DisplayMessage(Text_SpaceToContinue, false, false);
		}
	}
	else if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_FAILED) {
		if (Teleporter_ServiceAll(OBJECT_TYPE_FLAGS_TELEPORTED)) {
			briefIdx = (!(objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_CRYSTAL) ? LEVELSTATUS_FAILED
																			  : LEVELSTATUS_FAILED_CRYSTALS);
			stopMusic = true;

			ObjectRecall_Save_CreateNewObjectRecall();
			Lego_SetPaused(false, true);

			if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_STATUSREADY) {
				objectiveGlobs.flags &= ~OBJECTIVE_GLOB_FLAG_STATUSREADY;
				if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_SHOWFAILEDADVISOR) {
					Advisor_Start(Advisor_Objective, true);
				}
			}
			Text_DisplayMessage(Text_SpaceToContinue, false, false);
		}
	}
	else {
		bool32 timerStillRunning;
		if (Objective_CheckCompleted(level, &timerStillRunning, elapsedGame)) {
			Objective_SetStatus(timerStillRunning ? LEVELSTATUS_COMPLETE : LEVELSTATUS_FAILED);
		}
		else {
			RewardQuota_UpdateTimers(elapsedGame);
		}
	}

	if (objectiveGlobs.soundHandle != -1) {
		objectiveGlobs.soundTimer -= elapsedAbs;
		if (objectiveGlobs.soundTimer <= 0.0f) {
			Advisor_End();
			objectiveGlobs.soundHandle = -1;
		}
	}

	if (stopMusic) {
		/// CHANGE: No longer check the GAME2_MUSICREADY flag, since that's only necessary to start playing.
		Lego_ChangeMusicPlaying(false); // End music.
	}

	// Old showing state.
	if (objectiveGlobs.showing && objectiveGlobs.soundName != nullptr) {

		SFX_ID sfxID; // dummy init
		if (SFX_GetType(objectiveGlobs.soundName, &sfxID)) {
			// Unused function call.
			//SFX_IsQueueMode();

			SFX_SetQueueMode(false, false);
			objectiveGlobs.soundHandle = SFX_Random_PlaySoundNormal(sfxID, false);
			const real32 samplePlayTime = SFX_Random_GetSamplePlayTime(sfxID);

			objectiveGlobs.soundTimer = ((samplePlayTime - 1.5f) * STANDARD_FRAMERATE);
			SFX_SetQueueMode_AndFlush(true);
		}
		Gods98::Mem_Free(objectiveGlobs.soundName);
		objectiveGlobs.soundName = nullptr;
	}

	if (flushSFXQueue) {
		SFX_SetQueueMode_AndFlush(true);
	}

	objectiveGlobs.showing = (briefIdx != LEVELSTATUS_FAILED_OTHER);

	if (objectiveGlobs.showing) {
		/// HARDCODED: UI button dimensions/positions for briefing.
		const Point2F nextPos = Point2F { 470.0f, 315.0f };
		const Point2F prevPos = Point2F { 130.0f, 315.0f };

		// Show or hide the 'next page' button.
		if (objectiveGlobs.currentPages[briefIdx] < objectiveGlobs.pageCounts[briefIdx])
			Lego_SetMenuNextPosition(&nextPos);
		else
			Lego_SetMenuNextPosition(nullptr);

		// Show or hide the 'previous page' button.
		if (objectiveGlobs.currentPages[briefIdx] > 0)
			Lego_SetMenuPreviousPosition(&prevPos);
		else
			Lego_SetMenuPreviousPosition(nullptr);

		if (objectiveGlobs.currentPages[briefIdx] != objectiveGlobs.currentPageStates[briefIdx]) {
			Gods98::TextWindow_Clear(objectiveGlobs.textWindows[briefIdx]);
			Gods98::TextWindow_PagePrintF(objectiveGlobs.textWindows[briefIdx], objectiveGlobs.currentPages[briefIdx], "%s", objectiveGlobs.messages[briefIdx]);
			objectiveGlobs.currentPageStates[briefIdx] = objectiveGlobs.currentPages[briefIdx];
		}

		if (level->objective.panelImage != nullptr) {
			Gods98::Image_Display(level->objective.panelImage, &level->objective.panelImagePosition);
		}

		// NOTE: pageTextWindows does not have an index for LEVELSTATUS_FAILED_CRYSTALS.
		Gods98::TextWindow* pageTextWindow;
		switch (level->status) {
		case LEVELSTATUS_COMPLETE:
			pageTextWindow = objectiveGlobs.pageTextWindows[LEVELSTATUS_COMPLETE];
			break;
		case LEVELSTATUS_FAILED:
			pageTextWindow = objectiveGlobs.pageTextWindows[LEVELSTATUS_FAILED];
			break;
		case LEVELSTATUS_INCOMPLETE:
		default: // This includes two other failed cases besides incomplete!!!
			pageTextWindow = objectiveGlobs.pageTextWindows[LEVELSTATUS_INCOMPLETE];
			break;
		}

		if (pageTextWindow != nullptr) {
			Gods98::TextWindow_Update(pageTextWindow, 0, elapsedGame, nullptr);
		}
		if (objectiveGlobs.textWindows[briefIdx] != nullptr) {
			Gods98::TextWindow_Update(objectiveGlobs.textWindows[briefIdx], 0, elapsedGame, nullptr);
		}
		if (objectiveGlobs.beginTextWindow != nullptr) {
			Gods98::TextWindow_Update(objectiveGlobs.beginTextWindow, 0, elapsedGame, nullptr);
		}
	}
}


#pragma endregion
