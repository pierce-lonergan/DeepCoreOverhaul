// ToolTip.cpp : 
//

#include "../../engine/drawing/Draw.h"

#include "ToolTip.h"


/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define TOOLTIP_PRERENDER_BOXES		true

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004ab64c>
bool32 & LegoRR::g_ToolTipIsSFXPlaying = *(bool32*)0x004ab64c;

// <LegoRR.exe @0054cf20>
LegoRR::ToolTip_Globs & LegoRR::toolTipGlobs = *(LegoRR::ToolTip_Globs*)0x0054cf20;

/// CUSTOM: Cached image for the last-drawn tooltip.
static Gods98::Image* _toolTipImageCache = nullptr;
static Size2I _toolTipImageCacheSize = { 0, 0 };
static ColourRGBF _toolTipImageCacheRc = { 0.0f };
static ColourRGBF _toolTipImageCacheLn1 = { 0.0f };
static ColourRGBF _toolTipImageCacheLn2 = { 0.0f };
static ColourRGBF _toolTipImageCacheTrans = { 0.0f };
static uint32 _toolTipImageCacheThickness = 0;
// We can't cache half-trans drawing.
//static bool _toolTipImageCacheHalfTrans = false;

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
	/// OPTIMISE: Draw pre-rendered tooltips on smaller surfaces to avoid locking the back surface every frame.

	const Point2F destPos = valueRect.point;

	Gods98::Image* prerendered = nullptr;

	// Note: Change define to false to disable pre-rendered drawing.
	#if TOOLTIP_PRERENDER_BOXES
	if (!halfTrans) {
		// Can only pre-render when not using halfTrans.
		Size2I size = { static_cast<sint32>(valueRect.width), static_cast<sint32>(valueRect.height) };
		const ColourRGBF rc  = { rcRed,  rcGreen,  rcBlue };
		const ColourRGBF ln1 = { ln1Red, ln1Green, ln1Blue };
		const ColourRGBF ln2 = { ln2Red, ln2Green, ln2Blue };

		if (_toolTipImageCache != nullptr &&
			(size.width == _toolTipImageCacheSize.width && size.height == _toolTipImageCacheSize.height) &&
			(rc.red  == _toolTipImageCacheRc.red  && rc.green  == _toolTipImageCacheRc.green  && rc.blue  == _toolTipImageCacheRc.blue) &&
			(ln1.red == _toolTipImageCacheLn1.red && ln1.green == _toolTipImageCacheLn1.green && ln1.blue == _toolTipImageCacheLn1.blue) &&
			(ln2.red == _toolTipImageCacheLn2.red && ln2.green == _toolTipImageCacheLn2.green && ln2.blue == _toolTipImageCacheLn2.blue) &&
			toolTipGlobs.borderThickness == _toolTipImageCacheThickness)
		{
			// This is the same tooltip image as last time.
			Gods98::Image_Display(_toolTipImageCache, &destPos);
			return;
		}

		// We need to render a new tooltip image.
		bool needsClear = false;

		if (_toolTipImageCache != nullptr &&
			size.width  <= Gods98::Image_GetWidth(_toolTipImageCache) &&
			size.height <= Gods98::Image_GetHeight(_toolTipImageCache))
		{
			// The image size is large enough to fit our tooltip, no need to create a new one.
			prerendered = _toolTipImageCache;

			if (size.width < _toolTipImageCacheSize.width || size.height < _toolTipImageCacheSize.height) {
				// At least one dimension is smaller, so we need to clear the image.
				needsClear = true;
			}
		}
		else {
			needsClear = true;
			if (_toolTipImageCache != nullptr) {
				// The existing image size is too small, dispose of it and make a larger image.
				size.width  = std::max(size.width,  Gods98::Image_GetWidth(_toolTipImageCache));
				size.height = std::max(size.height, Gods98::Image_GetHeight(_toolTipImageCache));
				Gods98::Image_Remove(_toolTipImageCache);
			}
			prerendered = Gods98::Image_CreateNew(size.width, size.height);
		}

		// Pick a transparent colour that isn't rc, ln1, or ln2.
		const ColourRGBF colours[3] = { rc, ln1, ln2 };
		ColourRGBF trans;
		Gods98::Image_FindTransColour(prerendered, colours, _countof(colours), &trans);
		if (needsClear ||
			(trans.red   != _toolTipImageCacheTrans.red) ||
			(trans.green != _toolTipImageCacheTrans.green) ||
			(trans.blue  != _toolTipImageCacheTrans.blue))
		{
			Gods98::Image_ClearRGBF(prerendered, nullptr, trans.red, trans.green, trans.blue);
		}
		Gods98::Image_SetupTrans2(prerendered, trans.red, trans.green, trans.blue,
									trans.red, trans.green, trans.blue, false); // Don't truncate to 16-bit colour.

		_toolTipImageCache = prerendered;
		_toolTipImageCacheSize  = size;
		_toolTipImageCacheRc    = rc;
		_toolTipImageCacheLn1   = ln1;
		_toolTipImageCacheLn2   = ln2;
		_toolTipImageCacheTrans = trans;
		_toolTipImageCacheThickness = toolTipGlobs.borderThickness;
	}
	#endif

	// Setup outline rects.
	uint32 lineCount = 0;
	Point2F ln1ListFrom[10] = { 0.0f }; // dummy inits
	Point2F ln1ListTo[10]   = { 0.0f };
	Point2F ln2ListFrom[10] = { 0.0f };
	Point2F ln2ListTo[10]   = { 0.0f };

	// This uses the same border drawing method as: ObjInfo_DrawHealthBar
	// int2_8 is probably outline thickness.
	for (uint32 i = 0; i < toolTipGlobs.borderThickness; i++) {
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

	Gods98::Draw_Begin(prerendered); // Start of only Draw calls for tooltip box.
	if (prerendered != nullptr) {
		Gods98::Draw_SetTranslation(Point2F { -destPos.x, -destPos.y });
	}

	// Draw fill.
	Gods98::Draw_RectListEx(&valueRect, 1, rcRed, rcGreen, rcBlue, effect);
	// Draw outline.
	if (lineCount > 0) {
		Gods98::Draw_LineListEx(ln1ListFrom, ln1ListTo, lineCount, ln1Red, ln1Green, ln1Blue, effect);
		Gods98::Draw_LineListEx(ln2ListFrom, ln2ListTo, lineCount, ln2Red, ln2Green, ln2Blue, effect);
	}

	Gods98::Draw_End(); // End of only Draw calls for tooltip box.

	if (prerendered != nullptr) {
		Gods98::Image_Display(prerendered, &destPos);
	}
}

#pragma endregion
