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


// <LegoRR.exe @0045f2f0>
//bool32 __cdecl LegoRR::Reward_Initialise(void);

// <LegoRR.exe @0045f4b0>
//void __cdecl LegoRR::Reward_LoadItemSounds(RewardLevelItem* rewardItem);

// <LegoRR.exe @0045f4f0>
//void __cdecl LegoRR::Reward_LoadItemFonts(RewardLevelItem* rewardItem);

// <LegoRR.exe @0045f550>
//void __cdecl LegoRR::Reward_LoadItemBoxImages(RewardLevelItem* rewardItem);

// <LegoRR.exe @0045f6a0>
//void __cdecl LegoRR::Reward_LoadItemImages(RewardLevelItem* rewardItem);

// <LegoRR.exe @0045f7f0>
//void __cdecl LegoRR::Reward_LoadItemText(RewardLevelItem* rewardItem);

// <LegoRR.exe @0045f8b0>
//void __cdecl LegoRR::Reward_LoadItemFlics(RewardLevelItem* rewardItem);

// <LegoRR.exe @0045fa10>
//void __cdecl LegoRR::Reward_Shutdown(void);

// <LegoRR.exe @0045fa70>
//void __cdecl LegoRR::Reward_FreeItemImage(RewardLevelItem* rewardItem);

// <LegoRR.exe @0045fa90>
//void __cdecl LegoRR::Reward_FreeItemFont(RewardLevelItem* rewardItem);

// <LegoRR.exe @0045fab0>
//bool32 __cdecl LegoRR::Reward_LoadGraphics(const Gods98::Config* config, const char* gameName);

// <LegoRR.exe @0045fdb0>
//bool32 __cdecl LegoRR::Reward_LoadButtons(const Gods98::Config* config, const char* gameName);

// <LegoRR.exe @00460060>
//bool32 __cdecl LegoRR::Reward_CreateLevel(void);

// <LegoRR.exe @00460360>
//void __cdecl LegoRR::Reward_LoadLevelItemImportance(RewardLevelItem* rewardItem);

// <LegoRR.exe @00460400>
//void __cdecl LegoRR::Reward_LoadLevelItemQuota(RewardLevelItem* rewardItem);

// <LegoRR.exe @004604a0>
//void __cdecl LegoRR::Reward_FreeLevel(void);

// <LegoRR.exe @004604b0>
//LegoRR::RewardLevel* __cdecl LegoRR::GetRewardLevel2(undefined4 unused_rewardID);

// <LegoRR.exe @004604c0>
//LegoRR::RewardLevel* __cdecl LegoRR::GetRewardLevel(void);

// <LegoRR.exe @004604e0>
//void __cdecl LegoRR::RewardQuota_CountUnits(void);

// <LegoRR.exe @00460550>
//LegoRR::RewardBuildingCounts* __cdecl LegoRR::RewardQuota_CountBuildings(OUT RewardBuildingCounts* buildingCounts);

// <LegoRR.exe @004605d0>
//bool32 __cdecl LegoRR::RewardQuota_LiveObjectCallback_CountBuildings(LegoObject* liveObj, void* search);

// <LegoRR.exe @00460620>
//bool32 __cdecl LegoRR::Reward_Prepare(void);

// <LegoRR.exe @00460bd0>
//uint32 __cdecl LegoRR::Reward_GetLevelObjectsBuilt(const char* objName, sint32 objLevel, bool32 currentLevel);

// <LegoRR.exe @00460c10>
//bool32 __cdecl LegoRR::Reward_PrepareCalculate(void);

// <LegoRR.exe @004611c0>
//bool32 __cdecl LegoRR::Reward_PrepareScroll(void);

// <LegoRR.exe @00461330>
//bool32 __cdecl LegoRR::Reward_PrepareValueText(void);

// <LegoRR.exe @004616d0>
//bool32 __cdecl LegoRR::Reward_Show(void);

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

				ToolTip_DrawBox(destArea, rc.r, rc.g, rc.b, ln1.r, ln1.g, ln1.b,
								ln2.r, ln2.g, ln2.b, halfTrans);

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

				ToolTip_DrawBox(destArea, rc.r, rc.g, rc.b, ln1.r, ln1.g, ln1.b,
								ln2.r, ln2.g, ln2.b, false);

				Gods98::Font_PrintF(rewardGlobs.titleFont, rewardGlobs.textPos.x - (width/2), rewardGlobs.textPos.y, text);
			}
		}
	}
}

// <LegoRR.exe @00461f50>
//void __cdecl LegoRR::Reward_DrawScore(Reward_Type rewardType);

// <LegoRR.exe @00462090>
//void __cdecl LegoRR::Reward_HandleButtons(OUT RewardUIState* state, OUT bool32* saved);

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

// <LegoRR.exe @00462650>
//void __cdecl LegoRR::Reward_HandleDebugKeys(IN OUT Reward_Type* rewardType, IN OUT RewardUIState* state, IN OUT bool32* finished, IN OUT real32* timer);

// <LegoRR.exe @00462720>
//void __cdecl LegoRR::Reward_PlayFlic(RewardLevelItem* rewardItem);

// <LegoRR.exe @00462760>
//void __cdecl LegoRR::Reward_UpdateState(IN OUT Reward_Type* rewardType, IN OUT RewardUIState* state, IN OUT real32* timer);

// <LegoRR.exe @004628c0>
//void __cdecl LegoRR::Reward_DrawAllValues(IN Reward_Type* rewardType, OUT bool32* finished);

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

// <LegoRR.exe @00462a40>
//bool32 __cdecl LegoRR::Reward_LoopBegin(void);

// <LegoRR.exe @00462ac0>
//void __cdecl LegoRR::RewardQuota_UpdateTimers(real32 elapsedGame);

// <LegoRR.exe @00462af0>
//void __cdecl LegoRR::RewardQuota_WallDestroyed(void);

// <LegoRR.exe @00462b10>
//void __cdecl LegoRR::RewardQuota_CavernDiscovered(void);

// <LegoRR.exe @00462b30>
//void __cdecl LegoRR::RewardQuota_RockMonsterGenerated(void);

// <LegoRR.exe @00462b40>
//void __cdecl LegoRR::RewardQuota_RockMonsterDestroyed(void);

// <LegoRR.exe @00462b50>
//void __cdecl LegoRR::RewardQuota_RockMonsterAttacked(void);

// <LegoRR.exe @00462b60>
//void __cdecl LegoRR::RewardQuota_RockMonsterDamageDealt(real32 damage);

// <LegoRR.exe @00462b80>
//void __cdecl LegoRR::RewardQuota_MiniFigureDamageTaken(real32 damage);

// <LegoRR.exe @00462ba0>
//LegoRR::RewardScroll* __cdecl LegoRR::RewardScroll_Create(OUT RewardScroll** scroll, real32 zero, real32 heightDiv20, real32 width, real32 bottomSubDiv3pt5, real32 scrollSpeed);

// <LegoRR.exe @00462c20>
//bool32 __cdecl LegoRR::RewardScroll_Free(IN OUT RewardScroll** scroll);

// <LegoRR.exe @00462c90>
//LegoRR::RewardScrollLabel* __cdecl LegoRR::RewardScroll_AddLabel(RewardScroll* scroll, const char* text, Gods98::Font* font, real32 xPos, real32 yPos, RewardScrollLabelFlags labelFlags);

// <LegoRR.exe @00462d70>
//void __cdecl LegoRR::RewardScroll_SetDelay_Unk(RewardScroll* scroll, real32 curScrollY);

// <LegoRR.exe @00462d80>
//void __cdecl LegoRR::RewardScroll_AddFlags(RewardScroll* scroll, RewardScrollFlags flags);

// <LegoRR.exe @00462d90>
//bool32 __cdecl LegoRR::RewardScroll_DrawLabels(RewardScroll* scroll);

#pragma endregion
