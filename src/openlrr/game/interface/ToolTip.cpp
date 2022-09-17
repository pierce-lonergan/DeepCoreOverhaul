// ToolTip.cpp : 
//

#include "../../engine/drawing/Draw.h"

#include "ToolTip.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004ab64c>
bool32 & LegoRR::g_ToolTipIsSFXPlaying = *(bool32*)0x004ab64c;

// <LegoRR.exe @0054cf20>
LegoRR::ToolTip_Globs & LegoRR::toolTipGlobs = *(LegoRR::ToolTip_Globs*)0x0054cf20;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// For some weird reason, Area2F is passed BY VALUE here.
// The only reason this was even determined was due to the weird compiler behavior when calling this function.
// <LegoRR.exe @0046bb70>
void __cdecl LegoRR::ToolTip_DrawBox(Area2F valueRect, real32 rcRed, real32 rcGreen, real32 rcBlue, real32 ln1Red, real32 ln1Green, real32 ln1Blue, real32 ln2Red, real32 ln2Green, real32 ln2Blue, bool32 halfTrans)
{
	// Setup outline rects.
	uint32 lineCount = 0;
	Point2F ln1ListFrom[10] = { 0.0f }; // dummy inits
	Point2F ln1ListTo[10]   = { 0.0f };
	Point2F ln2ListFrom[10] = { 0.0f };
	Point2F ln2ListTo[10]   = { 0.0f };

	// int2_8 is probably outline thickness.
	for (uint32 i = 0; i < toolTipGlobs.int2_8; i++) {
		const Point2F start = {
			valueRect.x + static_cast<real32>(i),
			valueRect.y + static_cast<real32>(i),
		};
		const Point2F end = {
			((valueRect.x + valueRect.width)  - 1.0f) - static_cast<real32>(i),
			((valueRect.y + valueRect.height) - 1.0f) - static_cast<real32>(i),
		};

		// l10 <----------l11
		//  | 011111111111
		//  | 0          B ^
		//  | 0          B |
		//  v 0          B |
		//    AAAAAAAAAAAB |
		// l20----------> l21

		ln1ListFrom[lineCount + 0] = Point2F { start.x,   start.y };
		ln1ListTo[  lineCount + 0] = Point2F { start.x,   end.y-1 }; // -1 to skip overlapping pixel.
		ln1ListFrom[lineCount + 1] = Point2F {   end.x,   start.y };
		ln1ListTo[  lineCount + 1] = Point2F { start.x+1, start.y }; // +1 to skip overlapping pixel.

		ln2ListFrom[lineCount + 0] = Point2F { start.x,   end.y };
		ln2ListTo[  lineCount + 0] = Point2F { end.x-1,   end.y }; // -1 to skip overlapping pixel.
		ln2ListFrom[lineCount + 1] = Point2F { end.x,     end.y };
		ln2ListTo[  lineCount + 1] = Point2F { end.x, start.y+1 }; // +1 to skip overlapping pixel.

		lineCount += 2;
	}

	const Gods98::DrawEffect effect = (halfTrans ? Gods98::DrawEffect::HalfTrans : Gods98::DrawEffect::None);
	
	// Draw fill.
	Gods98::Draw_RectListEx(&valueRect, 1, rcRed, rcGreen, rcBlue, effect);
	// Draw outline.
	if (lineCount > 0) {
		Gods98::Draw_LineListEx(ln1ListFrom, ln1ListTo, lineCount, ln1Red, ln1Green, ln1Blue, effect);
		Gods98::Draw_LineListEx(ln2ListFrom, ln2ListTo, lineCount, ln2Red, ln2Green, ln2Blue, effect);
	}
}

#pragma endregion
