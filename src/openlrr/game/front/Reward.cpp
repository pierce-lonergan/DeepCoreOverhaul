// Reward.cpp : 
//

#include "../../engine/input/Input.h"
#include "../../engine/Main.h"

#include "../audio/SFX.h"
#include "../interface/Pointers.h"
#include "../interface/ToolTip.h"
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

#pragma endregion
