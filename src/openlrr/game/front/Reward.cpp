// Reward.cpp : 
//

#include "../../engine/input/Input.h"
#include "../../engine/Main.h"

#include "../audio/SFX.h"
#include "../interface/Pointers.h"
#include "../interface/ToolTip.h"
#include "../mission/Objective.h"
#include "../Game.h"

#include "FrontEnd.h"
#include "Reward.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @00553980>
LegoRR::Reward_Globs & LegoRR::rewardGlobs = *(LegoRR::Reward_Globs*)0x00553980;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

/// CUSTOM: Show tooltips to highlight what a reward counter value is describing.
void LegoRR::Reward_HandleItemToolTip(const Area2F* box, RewardLevelItem* rewardItem)
{
	if (Gods98::msx() >= box->x && Gods98::msx() < box->x + box->width &&
		Gods98::msy() >= box->y && Gods98::msy() < box->y + box->height)
	{
		/// POINTER MATH: Cast to ptrdiff_t before subtraction to avoid C++ pointer math conversion.
		///               RewardLevelItems don't store their type, so grab that from the memory address.
		const Reward_Type itemType = static_cast<Reward_Type>(((ptrdiff_t)rewardItem - (ptrdiff_t)rewardGlobs.level->itemPtr) / sizeof(RewardLevelItem));

		// We don't want to display tooltips for score, since it's labeled right on the clipboard.
		if (itemType != Reward_Score && itemType < Reward_Type_Count) {

			// Hijack one of the non-predefined tooltip types that won't conflict and use that here.
			ToolTip_SetContent(ToolTip_UnitSelect, "%s", rewardItem->Text);
			ToolTip_SetSFX(ToolTip_UnitSelect, SFX_NULL);
			ToolTip_Activate(ToolTip_UnitSelect);
		}
	}
}


// <LegoRR.exe @00461a50>
void __cdecl LegoRR::Reward_DrawItem(RewardLevelItem* rewardItem, RewardItemFlags flags, Reward_Type rewardType)
{
#if false
	// Test NAMETEXT boxes:
	// The chief animation includes sections of the BoxImages due to drawing under them,
	//  So it's not a bug that they're partially visible at the very end.
	flags &= ~REWARDITEM_FLAG_BOXIMAGES;

	if (flags & REWARDITEM_FLAG_VALUETEXT) {
		/// POINTER MATH: rewardItems don't store their type, so grab that from the memory address.
		const Reward_Type itemType = static_cast<Reward_Type>(((ptrdiff_t)rewardItem - (ptrdiff_t)rewardGlobs.level->itemPtr) / sizeof(RewardLevelItem));
		if (itemType != Reward_Score && itemType < Reward_Type_Count) {
			flags &= ~REWARDITEM_FLAG_VALUETEXT;
			flags |= REWARDITEM_FLAG_NAMETEXT_HALFTRANS;
		}
	}
#endif

	/// NOTE: Throughout this function, we check if both rewardItem->flags and parameter flags have the same values.
	///       So we're using a variable that's a bitwise AND of both flags to simplify the comparisons.

	const RewardItemFlags bothFlags = (rewardItem->flags & flags);


	// 1-INDEXED!!!????
	uint32 rewardIdx;
	if (rewardType == 0)
		rewardIdx = 0;
	else
		rewardIdx = rewardType - 1;

	if (rewardGlobs.displayImages) {
		if (bothFlags & REWARDITEM_FLAG_IMAGES) {
			const Point2F destPos = {
				static_cast<real32>(rewardItem->ImagePosition.x),
				static_cast<real32>(rewardItem->ImagePosition.y),
			};
			Gods98::Image_Display(rewardItem->Image, &destPos);
		}

		if (bothFlags & REWARDITEM_FLAG_BOXIMAGES) {
			const Point2F destPos = {
				static_cast<real32>(rewardItem->BoxImagePosition.x),
				static_cast<real32>(rewardItem->BoxImagePosition.y),
			};
			Gods98::Image_Display(rewardItem->BoxImage, &destPos);

			/// CUSTOM: Show tooltips for what reward value the mouse is hovering over.
			const Area2F destArea = {
				destPos.x,
				destPos.y,
				static_cast<real32>(Gods98::Image_GetWidth(rewardItem->BoxImage)),
				static_cast<real32>(Gods98::Image_GetHeight(rewardItem->BoxImage)),
			};
			Reward_HandleItemToolTip(&destArea, rewardItem);
		}
	}

	if (rewardGlobs.displayFlics && (bothFlags & REWARDITEM_FLAG_FLIC)) {
		// Lazy loading for Flics.
		if (rewardItem->Flic == nullptr) {
			Gods98::Flic_Setup(rewardItem->FlicName, &rewardItem->Flic, Gods98::FlicUserFlags::FLICDISK);
		}

		const Area2F destArea = {
			static_cast<real32>(rewardItem->FlicPosition.x),
			static_cast<real32>(rewardItem->FlicPosition.y),
			static_cast<real32>(rewardItem->FlicSize.width),
			static_cast<real32>(rewardItem->FlicSize.height),
		};

		/// ALTERATION: Draw animation at 1.5x speed, because it looks terribly slow in modern day.
		const bool advance = (rewardItem->Flic->currentframe < 35);
		Gods98::Flic_AnimateDeltaTime(rewardItem->Flic, &destArea, advance, false,
									  Gods98::Main_GetDeltaTime() * (37.5f / STANDARD_FRAMERATE));
	}

	if (rewardGlobs.displayText) {
		if (bothFlags & REWARDITEM_FLAG_VALUETEXT) {

			Gods98::Font_PrintFCenter(rewardItem->Font, rewardItem->TextPosition.x, rewardItem->TextPosition.y, rewardItem->valueText);
			if (rewardType < Reward_Type_Count) {
				const char* text = rewardGlobs.level->itemPtr[rewardIdx].Text;

				Gods98::Font_PrintFCenter(rewardGlobs.titleFont, rewardGlobs.textPos.x, rewardGlobs.textPos.y, text);
			}
		}

		if ((bothFlags & REWARDITEM_FLAG_NAMETEXT) ||
			(bothFlags & REWARDITEM_FLAG_NAMETEXT_HALFTRANS))
		{
			const ColourRGBF rc = {
				(30.0f / 255.0f),
				(30.0f / 255.0f),
				(30.0f / 255.0f),
			};
			const ColourRGBF ln1 = {
				(60.0f / 255.0f),
				(60.0f / 255.0f),
				(60.0f / 255.0f),
			};
			const ColourRGBF ln2 = {
				0.0f,
				0.0f,
				0.0f,
			};

			{
				const uint32 width = Gods98::Font_GetStringWidth(rewardItem->Font, rewardItem->valueText);
				const uint32 boxWidth = width + 20;

				const Area2F destArea = {
					static_cast<real32>(rewardItem->TextPosition.x - (boxWidth/2)),
					static_cast<real32>(rewardItem->TextPosition.y),
					static_cast<real32>(boxWidth),
					static_cast<real32>(Gods98::Font_GetHeight(rewardItem->Font)),
				};

				const bool halfTrans = (flags & REWARDITEM_FLAG_NAMETEXT_HALFTRANS);

				ToolTip_DrawBox(destArea, rc.red, rc.green, rc.blue, ln1.red, ln1.green, ln1.blue,
								ln2.red, ln2.green, ln2.blue, halfTrans);

				Gods98::Font_PrintF(rewardItem->Font, rewardItem->TextPosition.x - (width/2), rewardItem->TextPosition.y, rewardItem->valueText);

				/// CUSTOM: Show tooltips for what reward value the mouse is hovering over.
				Reward_HandleItemToolTip(&destArea, rewardItem);
			}

			if (rewardType < Reward_Type_Count) {
				const char* text = rewardGlobs.level->itemPtr[rewardIdx].Text;

				const uint32 width = Gods98::Font_GetStringWidth(rewardGlobs.titleFont, text);
				const uint32 boxWidth = width + 20;

				const Area2F destArea = {
					static_cast<real32>(rewardGlobs.textPos.x - (boxWidth/2)),
					static_cast<real32>(rewardGlobs.textPos.y),
					static_cast<real32>(boxWidth),
					static_cast<real32>(Gods98::Font_GetHeight(rewardGlobs.titleFont)),
				};

				ToolTip_DrawBox(destArea, rc.red, rc.green, rc.blue, ln1.red, ln1.green, ln1.blue,
								ln2.red, ln2.green, ln2.blue, false);

				Gods98::Font_PrintF(rewardGlobs.titleFont, rewardGlobs.textPos.x - (width/2), rewardGlobs.textPos.y, text);
			}
		}
	}
}


// <LegoRR.exe @004629c0>
void __cdecl LegoRR::Reward_LoopUpdate(real32 elapsedSeconds)
{
	/// FIX APPLY: Seconds to standard framerate.
	const real32 elapsed = (elapsedSeconds * STANDARD_FRAMERATE);

	/// FIX APPLY: Properly pass elapsed time to Pointer_Update
	Pointer_Update(elapsed);
	Pointer_SetCurrent_IfTimerFinished(Pointer_Standard);
	Pointer_DrawPointer(Gods98::msx(), Gods98::msy());

	Lego_UpdateGameCtrlLeftButtonLast();
	Gods98::INPUT.lClicked = false;

	SFX_Update(elapsed);

	// Why is it that ToolTips are the biggest thing NOT using standard framerate internally??
	ToolTip_Update(Gods98::msx(), Gods98::msy(), elapsed);

	Gods98::Main_LoopUpdate(true);
}


// <LegoRR.exe @004622f0>
//#define Reward_GotoSaveMenu ((void (__cdecl* )(void))0x004622f0)
void __cdecl LegoRR::Reward_GotoSaveMenu(void)
{
	/// REMOVE: Function call with no effect.
	//Front_IsMissionSelected();

	Front_SaveOptionParameters();

	if (Front_IsFrontEndEnabled() && Objective_IsObjectiveAchieved() && !Front_IsTutorialSelected()) {
		const sint32 levelIndex = Front_LevelSet_IndexOf(Front_Levels_GetTutoOrMissions(), Lego_GetLevel()->name);
		/// REMOVE: Function call with no effect.
		//Front_Save_GetRewardLevel(levelIndex);

		const sint32 lastSaveNumber = Front_Save_GetSaveNumber();
		SaveData* lastSave = Front_Save_GetCurrentSaveData();
		SaveStruct_18 saveStruct18;
		SaveData copySave;
		// This allocates missionsTable, so we need to either call Front_Save_SetSaveData or Front_Save_FreeSaveData (but not both).
		Front_Save_CopySaveData(&copySave);
		RewardLevel* rewardLevel = GetRewardLevel();

		do {
			Front_RunScreenMenuType(Menu_Screen_Save);

		} while (frontGlobs.saveMenuKeepOpen);

		// True when leaving the save menu without using the back button.
		if (frontGlobs.saveMenuHasSaved == TRUE) {
			// This boolean is only ever true when running Menu_Screen_Save, and bool_85c was true.
			// Solve bool_85c to solve bool_540.
			if (Front_Save_GetHasNoSaveData()) {
				Front_Save_WriteEmptySaveFiles();
				Front_Save_SetHasNoSaveData(false);
			}

			SaveData* currSave = Front_Save_GetCurrentSaveData();
			if (currSave != nullptr) {
				if (currSave != lastSave && lastSave != nullptr) {
					Front_Save_SetSaveData(&copySave);
				}
				else {
					/// FIX APPLY: Free SaveData when Front_Save_SetSaveData is not called.
					Front_Save_FreeSaveData(&copySave);
				}

				Front_Save_SetLevelCompleted(levelIndex);
				Front_Save_SetRewardLevel(levelIndex, rewardLevel);

				Object_Save_CopyStruct18(&saveStruct18);
				Front_Save_SetSaveStruct18(&saveStruct18);

				Front_Save_WriteCurrentSaveFiles();
			}
			else {
				/// FIX APPLY: Free SaveData when Front_Save_SetSaveData is not called.
				Front_Save_FreeSaveData(&copySave);

				/// REMOVE: Level index lookup with no usage.
			}
		}
		else {
			/// FIX APPLY: Free SaveData when Front_Save_SetSaveData is not called.
			Front_Save_FreeSaveData(&copySave);

			Front_Save_SetSaveNumber(lastSaveNumber);

			// This should be the same as lastSave.
			SaveData* currSave = Front_Save_GetCurrentSaveData();
			if (currSave != nullptr) {
				Front_Save_SetLevelCompleted(levelIndex);
				Front_Save_SetRewardLevel(levelIndex, rewardLevel);

				Object_Save_CopyStruct18(&saveStruct18);
				Front_Save_SetSaveStruct18(&saveStruct18);
			}
		}
	}
}

// <LegoRR.exe @00462530>
//#define Reward_GotoAdvance ((void (__cdecl* )(void))0x00462530)
void __cdecl LegoRR::Reward_GotoAdvance(void)
{
	const sint32 levelIndex = Front_LevelSet_IndexOf(Front_Levels_GetTutoOrMissions(), Lego_GetLevel()->name);

	const sint32 lastSaveNumber = Front_Save_GetSaveNumber();
	if (lastSaveNumber == -1) {
		// Change to the local save if no save is active.
		Front_Save_SetSaveNumber(5);
	}

	if (Lego_GetLevel()->status == LEVELSTATUS_COMPLETE) {
		Front_Save_SetLevelCompleted((uint32)levelIndex);
	}
	Front_Save_SetRewardLevel(levelIndex, GetRewardLevel());

	if (lastSaveNumber == -1) {
		// This is the first time the local save has been used (or after clearing). So clear all missions first.
		SaveData* localSave = Front_Save_GetSaveDataAt(5);
		if (localSave != nullptr) {
			/// TODO: Why are we starting at one??? Does this have something to do
			///        with that > count bounds check originally in Front_Save_SetLevelCompleted?
			///       Or is this because the first tutorial mission is always unlocked?
			std::memset(localSave->missionsTable + 1, 0, ((localSave->missionsCount - 1) * sizeof(SaveReward)));
			//for (uint32 i = 1; i < localSave->missionsCount; i++) {
			//
			//}

			// Apply the changes again because we just overwrote them with memset.
			if (Lego_GetLevel()->status == LEVELSTATUS_COMPLETE) {
				Front_Save_SetLevelCompleted((uint32)levelIndex);
			}
			Front_Save_SetRewardLevel(levelIndex, GetRewardLevel());
		}

		// Write the local save file.
		Front_Save_WriteCurrentSaveFiles();

		/// FIX APPLY: We need to free missionsTable before reading over it!
		Front_Save_FreeSaveData(&frontGlobs.saveData[5]);

		// Then reload the local save file for whatever reason (to confirm the save file is accurate maybe?)
		Front_Save_ReadSaveFile(5, &frontGlobs.saveData[5], false);
	}

	MenuItem_SelectData* select = frontGlobs.mainMenuSet->menus[1]->items[1]->itemData.select;
	SaveData* currSave = Front_Save_GetCurrentSaveData();
	Front_Levels_UpdateAvailable(frontGlobs.startMissionLink, currSave->missionsTable, &frontGlobs.missionLevels, select, false);
	
	Front_Save_SetShouldClearUnlockedLevels(false);
}


#pragma endregion
