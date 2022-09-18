// Bubbles.h : 
//

#pragma once

#include "../../GameCommon.h"


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

#define BUBBLE_MAXSHOWHEALTHBARS		20
#define BUBBLE_MAXSHOWBUBBLES			20
#define BUBBLE_MAXSHOWPOWEROFFS			20
#define BUBBLE_MAXSHOWCALLTOARMS		50

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct Bubble // [LegoRR/Bubble.c|struct:0x8]
{
	/*0,4*/	LegoObject* object;
	/*4,4*/	real32 remainingTimer;
	/*8*/
};
assert_sizeof(Bubble, 0x8);


struct Bubble_Globs // [LegoRR/Bubble.c|struct:0x4f4|tags:GLOBS]
{
	/*000,4*/	bool32 alwaysVisible; // Object Display HUD/UI over entities in-game
	/*004,9c*/	Gods98::Image* bubbleImage[Bubble_Type_Count];
	/*0a0,9c*/	const char* bubbleName[Bubble_Type_Count];
	/*13c,48*/	undefined1 reserved1[72];
	/*184,a0*/	Bubble healthBarList[BUBBLE_MAXSHOWHEALTHBARS];  // Not a bubble image, but this tracks how long to show the bar after being damaged like normal bubbles.
	/*224,a0*/	Bubble bubbleList[BUBBLE_MAXSHOWBUBBLES];        // All other types of bubbles.
	/*2c4,a0*/	Bubble powerOffList[BUBBLE_MAXSHOWPOWEROFFS];    // Flashing 'no-power' icons.
	/*364,190*/	Bubble callToArmsList[BUBBLE_MAXSHOWCALLTOARMS]; // Action stations bubbles.
	/*4f4*/
};
assert_sizeof(Bubble_Globs, 0x4f4);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004b9a18>
extern real32 & s_Bubble_PowerOffFlashTimer;

// <LegoRR.exe @00558860>
extern Bubble_Globs & bubbleGlobs;

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define Bubble_RegisterName(n)				(bubbleGlobs.bubbleName[n]=#n)

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @00406fe0>
//#define Bubble_Initialise ((void (__cdecl* )(void))0x00406fe0)
void __cdecl Bubble_Initialise(void);

// <LegoRR.exe @00407170>
//#define Bubble_LoadBubbles ((void (__cdecl* )(const Gods98::Config* config))0x00407170)
void __cdecl Bubble_LoadBubbles(const Gods98::Config* config);

// Returns Bubble_Type_Count on failure.
// <LegoRR.exe @00407230>
//#define Bubble_GetBubbleType ((Bubble_Type (__cdecl* )(const char* bubbleName))0x00407230)
Bubble_Type __cdecl Bubble_GetBubbleType(const char* bubbleName);

// <LegoRR.exe @00407270>
//#define Bubble_ToggleObjectUIsAlwaysVisible ((void (__cdecl* )(void))0x00407270)
void __cdecl Bubble_ToggleObjectUIsAlwaysVisible(void);

// <LegoRR.exe @00407290>
//#define Bubble_GetObjectUIsAlwaysVisible ((bool32 (__cdecl* )(void))0x00407290)
bool32 __cdecl Bubble_GetObjectUIsAlwaysVisible(void);

// <LegoRR.exe @004072a0>
//#define Bubble_ResetObjectBubbleImage ((void (__cdecl* )(LegoObject* liveObj))0x004072a0)
void __cdecl Bubble_ResetObjectBubbleImage(LegoObject* liveObj);

// Remove references from the bubble tables that match the specified object.
// <LegoRR.exe @004072d0>
//#define Bubble_RemoveObjectReferences ((void (__cdecl* )(LegoObject* deadObj))0x004072d0)
void __cdecl Bubble_RemoveObjectReferences(LegoObject* deadObj);

// Does not set timer, as Power-Off is a state that Bubble automatically handles removing.
// Only allows Building object types.
// <LegoRR.exe @00407340>
//#define Bubble_ShowPowerOff ((void (__cdecl* )(LegoObject* liveObj))0x00407340)
void __cdecl Bubble_ShowPowerOff(LegoObject* liveObj);

// Sets timer to 0.5 seconds. That's pretty low...
// Only allows MiniFigure object types.
// <LegoRR.exe @00407380>
//#define Bubble_ShowBubble ((void (__cdecl* )(LegoObject* liveObj))0x00407380)
void __cdecl Bubble_ShowBubble(LegoObject* liveObj);

// Only allows MiniFigure object types.
// Sets timer 25 hours, but the timer is never decremented for Call to Arms, so it doesn't matter.
// <LegoRR.exe @004073e0>
//#define Bubble_ShowCallToArms ((void (__cdecl* )(LegoObject* liveObj))0x004073e0)
void __cdecl Bubble_ShowCallToArms(LegoObject* liveObj);

/// FIX APPLY: The liveObj parameter for this makes little sense, as it's only used to confirm type, after that all active bubbles will have their timer set.
///            The function has been changed to behave similar to other Show functions.
// Only allows MiniFigure object types.
// Setting timer to 0.0f or lower will stop this object from showing Call to Arms.
// <LegoRR.exe @00407440>
//#define Bubble_SetCallToArmsTimer ((void (__cdecl* )(LegoObject* liveObj, real32 timer))0x00407440)
void __cdecl Bubble_SetCallToArmsTimer(LegoObject* liveObj, real32 timer);

// Sets timer to 1.5 seconds.
// Only allows objects with stats flag STATS3_SHOWHEALTHBAR.
// <LegoRR.exe @00407470>
//#define Bubble_ShowHealthBar ((void (__cdecl* )(LegoObject* liveObj))0x00407470)
void __cdecl Bubble_ShowHealthBar(LegoObject* liveObj);

/// CUSTOM:
void Bubble_HidePowerOff(LegoObject* liveObj);

/// CUSTOM:
void Bubble_HideBubble(LegoObject* liveObj);

/// CUSTOM:
void Bubble_HideCallToArms(LegoObject* liveObj);

/// CUSTOM:
void Bubble_HideHealthBar(LegoObject* liveObj);

// <LegoRR.exe @004074d0>
//#define Bubble_DrawAllObjInfos ((void (__cdecl* )(real32 elapsedAbs))0x004074d0)
void __cdecl Bubble_DrawAllObjInfos(real32 elapsedAbs);

// <LegoRR.exe @004077f0>
//#define Bubble_UpdateAndGetBubbleImage ((void (__cdecl* )(LegoObject* liveObj, real32 elapsedAbs, OUT Gods98::Image** bubbleImage, OUT Point2F* screenPt))0x004077f0)
void __cdecl Bubble_UpdateAndGetBubbleImage(LegoObject* liveObj, real32 elapsedAbs, OUT Gods98::Image** bubbleImage, OUT Point2F* screenPt);

// DATA: real32* elapsedAbs
// <LegoRR.exe @00407890>
//#define Bubble_Callback_DrawObjInfo ((bool32 (__cdecl* )(LegoObject* liveObj, void* pElapsedAbs))0x00407890)
bool32 __cdecl Bubble_Callback_DrawObjInfo(LegoObject* liveObj, void* pElapsedAbs);

// DRAW MODE: Only Draw API drawing calls can be used within this function.
// DATA: real32* elapsedAbs
/// CUSTOM: Split up version of Bubble_Callback_DrawObjInfo that only handles health bars.
///         For use with Draw_Begin()/Draw_End() API.
bool32 __cdecl Bubble_Callback_DrawObjInfoHealthBars(LegoObject* liveObj, void* pElapsedAbs);

// DATA: real32* elapsedAbs
/// CUSTOM: Split up version of Bubble_Callback_DrawObjInfo that only handles hunger images and bubble images.
///         For use with Draw_Begin()/Draw_End() API.
bool32 __cdecl Bubble_Callback_DrawObjInfoImages(LegoObject* liveObj, void* pElapsedAbs);

// <LegoRR.exe @00407940>
//#define Bubble_EvaluateObjectBubbleImage ((void (__cdecl* )(LegoObject* liveObj, OUT Gods98::Image** bubbleImage))0x00407940)
void __cdecl Bubble_EvaluateObjectBubbleImage(LegoObject* liveObj, OUT Gods98::Image** bubbleImage);

#pragma endregion

}
