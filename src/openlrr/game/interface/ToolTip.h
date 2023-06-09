// ToolTip.h : 
//

#pragma once

#include "../GameCommon.h"


namespace LegoRR
{; // !<---

/**********************************************************************************
 ******** Forward Declarations
 **********************************************************************************/

#pragma region Forward Declarations

#pragma endregion

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define TOOLTIP_MAXICONS			20
#define TOOLTIP_BUFFERSIZE			512

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

enum ToolTipFlags : uint32 // [LegoRR/ToolTip.c|flags:0x4|type:uint]
{
	TOOLTIP_FLAG_NONE         = 0,
	TOOLTIP_FLAG_ENABLED1     = 0x1, // This and 0x2 are always used together, so their meaning can't be fully deciphered.
	TOOLTIP_FLAG_ENABLED2     = 0x2, // The only thing we know is that only one of these two flags is required for a tooltip to update.
	TOOLTIP_FLAG_ACTIVE       = 0x4, // ToolTip timer should increment and then display when it reaches hoverTime.
	TOOLTIP_FLAG_IMAGECONTENT = 0x8, // ToolTip will draw using an image as the body content.
	TOOLTIP_FLAG_RIGHTALIGN   = 0x10,
};
flags_end(ToolTipFlags, 0x4);

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct ToolTip // [LegoRR/ToolTip.c|struct:0x27c]
{
	/*000,4*/	real32 timer;
	/*004,200*/	char textBuffer[TOOLTIP_BUFFERSIZE];
	/*204,4*/	uint32 textWidth;
	/*208,4*/	uint32 textLineCount;
	/*20c,50*/	Gods98::Image* iconList[TOOLTIP_MAXICONS];
	/*25c,4*/	uint32 iconCount;
	/*260,4*/	sint32 iconsY;
	/*264,4*/	sint32 iconsHeight;
	/*268,4*/	sint32 iconsX;
	/*26c,4*/	sint32 iconsWidth;
	/*270,4*/	Gods98::Image* image; // Supplied with SetText prefixed with an @ sign.
	/*274,4*/	SFX_ID sfxType;
	/*278,4*/	ToolTipFlags flags; // (flag 0x8 IMAGE is not for "iconList")
	/*27c*/
};
assert_sizeof(ToolTip, 0x27c);


struct ToolTip_Globs // [LegoRR/ToolTip.c|struct:0x61c4|tags:GLOBS]
{
	/*0000,4*/	Gods98::Font* font; // (init only)
	/*0004,4*/	uint32 fontHeight; // (init only)
	/*0008,4*/	uint32 borderThickness; // (init only)
	/*000c,4*/	uint32 paddingThickness; // (init only)
	/*0010,4*/	uint32 appWidth; // (init only) Likely the bounds where tooltips are allowed to display.
	/*0014,4*/	uint32 appHeight; // (init only)
	/*0018,4*/	sint32 offsetY; // (init only) // Y offset from cursor?
	/*001c,4*/	real32 hoverTime; // Duration before showing tooltip  (init only)
	/*0020,24*/	real32 rgbFloats[3][3]; // [r:g:b(3)][norm:hi:lo(3)]  (init only)
	/*0044,9c*/	const char* toolTipName[ToolTip_Type_Count]; // (init only)
	/*00e0,60e4*/	ToolTip toolTips[ToolTip_Type_Count];
	/*61c4*/
};
assert_sizeof(ToolTip_Globs, 0x61c4);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004ab64c>
extern bool32 & s_ToolTip_sfxStopped;

// <LegoRR.exe @0054cf20>
extern ToolTip_Globs & toolTipGlobs;

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define ToolTip_RegisterName(n)		(toolTipGlobs.toolTipName[n]=#n)

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0046b490>
//#define ToolTip_Initialise ((void (__cdecl* )(Gods98::Font* font, uint32 borderThickness, uint32 paddingThickness, real32 delaySeconds, uint32 appWidth, uint32 appHeight, sint32 offsetY, real32 red, real32 green, real32 blue))0x0046b490)
void __cdecl ToolTip_Initialise(Gods98::Font* font, uint32 borderThickness, uint32 paddingThickness, real32 delaySeconds, uint32 appWidth, uint32 appHeight, sint32 offsetY, real32 red, real32 green, real32 blue);

// <LegoRR.exe @0046b790>
//#define ToolTip_GetType ((bool32 (__cdecl* )(const char* toolTipName, OUT ToolTip_Type* toolTipType))0x0046b790)
bool32 __cdecl ToolTip_GetType(const char* toolTipName, OUT ToolTip_Type* toolTipType);

// <LegoRR.exe @0046b7e0>
//#define ToolTip_SetContent ((void (__cdecl* )(ToolTip_Type toolTipType, const char* msg, ...))0x0046b7e0)
void __cdecl ToolTip_SetContent(ToolTip_Type toolTipType, const char* msg, ...);

// Pass nullptr to start a new row of icons.
// <LegoRR.exe @0046b920>
//#define ToolTip_AddIcon ((void (__cdecl* )(ToolTip_Type toolTipType, Gods98::Image* image))0x0046b920)
void __cdecl ToolTip_AddIcon(ToolTip_Type toolTipType, OPTIONAL Gods98::Image* image);

// <LegoRR.exe @0046b9c0>
//#define ToolTip_SetSFX ((void (__cdecl* )(ToolTip_Type toolTipType, SFX_ID sfxType))0x0046b9c0)
void __cdecl ToolTip_SetSFX(ToolTip_Type toolTipType, SFX_ID sfxType);

// <LegoRR.exe @0046b9f0>
//#define ToolTip_SetRightAlign ((void (__cdecl* )(ToolTip_Type toolTipType, bool32 on))0x0046b9f0)
void __cdecl ToolTip_SetRightAlign(ToolTip_Type toolTipType, bool32 on);

// <LegoRR.exe @0046ba30>
//#define ToolTip_Activate ((void (__cdecl* )(ToolTip_Type toolTipType))0x0046ba30)
void __cdecl ToolTip_Activate(ToolTip_Type toolTipType);

// <LegoRR.exe @0046ba60>
//#define ToolTip_ShowInstant ((void (__cdecl* )(ToolTip_Type toolTipType))0x0046ba60)
void __cdecl ToolTip_ShowInstant(ToolTip_Type toolTipType);

// <LegoRR.exe @0046ba80>
//#define ToolTip_Update ((void (__cdecl* )(sint32 mouseX, sint32 mouseY, real32 elapsedAbs))0x0046ba80)
void __cdecl ToolTip_Update(sint32 mouseX, sint32 mouseY, real32 elapsedAbs);

// For some weird reason, Area2F is passed BY VALUE here.
// The only reason this was even determined was due to the weird compiler behavior when calling this function.
// <LegoRR.exe @0046bb70>
//#define ToolTip_DrawBox ((void (__cdecl* )(Area2F valueRect, real32 rcRed, real32 rcGreen, real32 rcBlue, real32 ln1Red, real32 ln1Green, real32 ln1Blue, real32 ln2Red, real32 ln2Green, real32 ln2Blue, bool32 halfTrans))0x0046bb70)
void __cdecl ToolTip_DrawBox(Area2F valueRect, real32 rcRed, real32 rcGreen, real32 rcBlue, real32 ln1Red, real32 ln1Green, real32 ln1Blue, real32 ln2Red, real32 ln2Green, real32 ln2Blue, bool32 halfTrans);

// <LegoRR.exe @0046bef0>
//#define ToolTip_Draw ((void (__cdecl* )(ToolTip* toolTip, sint32 mouseX, sint32 mouseY))0x0046bef0)
void __cdecl ToolTip_Draw(ToolTip* toolTip, sint32 mouseX, sint32 mouseY);

#pragma endregion

}
