// LightEffects.h : 
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

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

enum LightEffects_GlobFlags : uint32 // [LegoRR/LightEffects.c|flags:0x4|type:uint] LightEffects_GlobFlags, Flags for LightEffectsManager global variable @004ebec8
{
	LIGHTFX_GLOB_FLAG_NONE         = 0,
	LIGHTFX_GLOB_FLAG_HASBLINK     = 0x1,
	LIGHTFX_GLOB_FLAG_HASFADE      = 0x4,
	LIGHTFX_GLOB_FLAG_FADING       = 0x8,
	LIGHTFX_GLOB_FLAG_FADE_FORWARD = 0x10,
	LIGHTFX_GLOB_FLAG_FADE_REVERSE = 0x20,
	LIGHTFX_GLOB_FLAG_HASMOVE      = 0x40,
	LIGHTFX_GLOB_FLAG_MOVING       = 0x80,
	LIGHTFX_GLOB_FLAG_DISABLED     = 0x100,
	LIGHTFX_GLOB_FLAG_DIMOUT       = 0x200,
	LIGHTFX_GLOB_FLAG_DIMIN_DONE   = 0x400,
	LIGHTFX_GLOB_FLAG_DIMOUT_DONE  = 0x1000,
};
flags_end(LightEffects_GlobFlags, 0x4);

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct LightEffects_Globs // [LegoRR/LightEffects.c|struct:0xf4|tags:GLOBS] LightEffects module globals @004ebdd8
{
	/*00,4*/	Gods98::Container* rootSpotlight; // Init
	/*04,4*/	Gods98::Container* rootLight; // Init. Used as reference for spotlight relative position.
	/*08,c*/	ColourRGBF	initialRGB;		// Init
	/*14,c*/	ColourRGBF	currentRGB;		// Update. Changed by Blink,Fade,Dimmer.
	/*20,c*/	ColourRGBF	blinkRGBMax;	// Init (cfg: BlinkRGBMax)
	/*2c,4*/	real32		blinkWait;		// Update. Time between end of blink and next blink.
	/*30,8*/	Range2F		blinkWaitRange;	// Init (cfg: RandomRangeForTimeBetweenBlinks, mult: 25.0)
	/*38,4*/	real32		blinkChange;	// Update
	/*3c,4*/	real32		blinkChangeMax;	// Init (cfg: MaxChangeAllowed, div: 255.0)
	/*40,c*/	ColourRGBF	fadeDestRGB;	// Update
	/*4c,c*/	ColourRGBF	fadeRGBMin;		// Init (cfg: FadeRGBMin)
	/*58,c*/	ColourRGBF	fadeRGBMax;		// Init (cfg: FadeRGBMax)
	/*64,4*/	real32		fadeWait;		// Update. Time between end of fade and next fade.
	/*68,8*/	Range2F		fadeWaitRange;	// Init (cfg: RandomRangeForTimeBetweenFades, mult: 25.0)
	/*70,c*/	ColourRGBF	fadeSpeedRGB;	// Update
	/*7c,8*/	Range2F		fadeSpeedRange;	// Init (cfg: RandomRangeForFadeTimeFade, mult: 25.0)
	/*84,4*/	real32		fadeHold;		// Update. Time between peak of fade before reversing.
	/*88,8*/	Range2F		fadeHoldRange;	// Init (cfg: RandomRangeForHoldTimeOfFade, mult: 25.0)
	/*90,c*/	ColourRGBF	fadePosRGB;		// Update
	/*9c,c*/	Vector3F	movePosition;	// Init (3D position of rootSpotlight relative to light)
	/*a8,c*/	Vector3F	moveLimit;		// Init (cfg: MoveLimit)
	/*b4,4*/	real32		moveWait;		// Update. Time between end of move and next move.
	/*b8,8*/	Range2F		moveWaitRange;	// Init (cfg: RandomRangeForTimeBetweenMoves, mult: 25.0)
	/*c0,4*/	real32		moveSpeed;		// Update
	/*c4,8*/	Range2F		moveSpeedRange;	// Init (cfg: RandomRangeForSpeedOfMove, mult: 25.0)
	/*cc,c*/	Vector3F	moveVector;		// Update
	/*d8,4*/	real32		moveDist;		// Update
	/*dc,8*/	Range2F		moveDistRange;	// Init (cfg: RandomRangeForDistOfMove)
	/*e4,c*/	undefined1 reserved1[12]; // Possibly an unused Vector3F/ColourRGBF.
	/*f0,4*/	LightEffects_GlobFlags flags;
	/*f4*/
};
assert_sizeof(LightEffects_Globs, 0xf4);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004ebdd8>
extern LightEffects_Globs & lightGlobs;

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0044c9d0>
//#define LightEffects_Initialise ((void (__cdecl* )(Gods98::Container* rootSpotlight, Gods98::Container* rootLight, real32 initialRed, real32 initialGreen, real32 initialBlue))0x0044c9d0)
void __cdecl LightEffects_Initialise(Gods98::Container* rootSpotlight, Gods98::Container* rootLight,
									 real32 initialRed, real32 initialGreen, real32 initialBlue);

/// CUSTOM: Reset all state values and start from the beginning.
///         Add this to Lego_LoadLevel once implemented.
void LightEffects_Restart();

// <LegoRR.exe @0044ca20>
//#define LightEffects_ResetSpotlightColour ((void (__cdecl* )(void))0x0044ca20)
void __cdecl LightEffects_ResetSpotlightColour(void);

/// CUSTOM:
void LightEffects_ResetSpotlightPosition();

// <LegoRR.exe @0044ca50>
//#define LightEffects_SetDisabled ((void (__cdecl* )(bool32 disabled))0x0044ca50)
void __cdecl LightEffects_SetDisabled(bool32 disabled);

// <LegoRR.exe @0044ca80>
//#define LightEffects_Load ((bool32 (__cdecl* )(const Gods98::Config* config, const char* gameName))0x0044ca80)
bool32 __cdecl LightEffects_Load(const Gods98::Config* config, const char* gameName);

// <LegoRR.exe @0044cab0>
//#define LightEffects_LoadBlink ((bool32 (__cdecl* )(const Gods98::Config* config, const char* gameName))0x0044cab0)
bool32 __cdecl LightEffects_LoadBlink(const Gods98::Config* config, const char* gameName);

// <LegoRR.exe @0044cc30>
//#define LightEffects_SetBlink ((void (__cdecl* )(real32 red, real32 green, real32 blue, real32 maxChange, real32 minRange, real32 maxRange))0x0044cc30)
void __cdecl LightEffects_SetBlink(real32 redMax, real32 greenMax, real32 blueMax,
								   real32 changeMax, real32 waitMin, real32 waitMax);

// <LegoRR.exe @0044cc80>
//#define LightEffects_LoadFade ((bool32 (__cdecl* )(const Gods98::Config* config, const char* gameName))0x0044cc80)
bool32 __cdecl LightEffects_LoadFade(const Gods98::Config* config, const char* gameName);

// <LegoRR.exe @0044ced0>
//#define LightEffects_SetFade ((void (__cdecl* )(real32 minRed, real32 minGreen, real32 minBlue, real32 maxRed, real32 maxGreen, real32 maxBlue, real32 minTime, real32 maxTime, real32 minFade, real32 maxFade, real32 minHold, real32 maxHold))0x0044ced0)
void __cdecl LightEffects_SetFade(real32 redMin, real32 greenMin, real32 blueMin, real32 redMax, real32 greenMax, real32 blueMax, 
								  real32 waitMin, real32 waitMax, real32 speedMin, real32 speedMax, real32 holdMin, real32 holdMax);

// <LegoRR.exe @0044cf60>
//#define LightEffects_LoadMove ((bool32 (__cdecl* )(const Gods98::Config* config, const char* gameName))0x0044cf60)
bool32 __cdecl LightEffects_LoadMove(const Gods98::Config* config, const char* gameName);

// <LegoRR.exe @0044d1b0>
//#define LightEffects_SetMove ((void (__cdecl* )(real32 minTime, real32 maxTime, real32 minSpeed, real32 maxSpeed, real32 minDist, real32 maxDist, real32 limitX, real32 limitY, real32 limitZ))0x0044d1b0)
void __cdecl LightEffects_SetMove(real32 waitMin, real32 waitMax, real32 speedMin, real32 speedMax,
								  real32 distMin, real32 distMax, real32 limitX, real32 limitY, real32 limitZ);

// <LegoRR.exe @0044d230>
//#define LightEffects_InvalidatePosition ((void (__cdecl* )(void))0x0044d230)
void __cdecl LightEffects_InvalidatePosition(void);

// <LegoRR.exe @0044d260>
//#define LightEffects_Update ((void (__cdecl* )(real32 elapsed))0x0044d260)
void __cdecl LightEffects_Update(real32 elapsed);

// <LegoRR.exe @0044d2b0>
//#define LightEffects_UpdateSpotlightColour ((void (__cdecl* )(void))0x0044d2b0)
void __cdecl LightEffects_UpdateSpotlightColour(void);

// <LegoRR.exe @0044d390>
//#define LightEffects_UpdateBlink ((void (__cdecl* )(real32 elapsed))0x0044d390)
void __cdecl LightEffects_UpdateBlink(real32 elapsed);

// <LegoRR.exe @0044d510>
//#define LightEffects_FlipSign ((void (__cdecl* )(IN OUT real32* value))0x0044d510)
void __cdecl LightEffects_FlipSign(IN OUT real32* value);

// <LegoRR.exe @0044d540>
//#define LightEffects_UpdateFade ((void (__cdecl* )(real32 elapsed))0x0044d540)
void __cdecl LightEffects_UpdateFade(real32 elapsed);

// <LegoRR.exe @0044d9d0>
//#define LightEffects_RandomizeFadeSpeedRGB ((void (__cdecl* )(void))0x0044d9d0)
void __cdecl LightEffects_RandomizeFadeSpeedRGB(void);

// <LegoRR.exe @0044da20>
//#define LightEffects_UpdateMove ((void (__cdecl* )(real32 elapsed))0x0044da20)
void __cdecl LightEffects_UpdateMove(real32 elapsed);

// <LegoRR.exe @0044dc60>
//#define LightEffects_CheckMoveLimit ((bool32 (__cdecl* )(const Vector3F* vector))0x0044dc60)
bool32 __cdecl LightEffects_CheckMoveLimit(const Vector3F* vector);

// <LegoRR.exe @0044dce0>
//#define LightEffects_SetDimmerMode ((void (__cdecl* )(bool32 isDimoutMode))0x0044dce0)
void __cdecl LightEffects_SetDimmerMode(bool32 isDimoutMode);

// <LegoRR.exe @0044dd10>
//#define LightEffects_UpdateDimmer ((void (__cdecl* )(real32 elapsed))0x0044dd10)
void __cdecl LightEffects_UpdateDimmer(real32 elapsed);



/// CUSTOM:
inline bool LightEffects_IsEnabled() { return !(lightGlobs.flags & LIGHTFX_GLOB_FLAG_DISABLED); }

/// CUSTOM:
inline void LightEffects_SetEnabled(bool enabled) { LightEffects_SetDisabled(!enabled); }

/// CUSTOM:
bool LightEffects_IsBlinkEnabled();

/// CUSTOM:
void LightEffects_SetBlinkEnabled(bool enabled);

/// CUSTOM:
bool LightEffects_IsFadeEnabled();

/// CUSTOM:
void LightEffects_SetFadeEnabled(bool enabled);

/// CUSTOM:
bool LightEffects_IsMoveEnabled();

/// CUSTOM:
void LightEffects_SetMoveEnabled(bool enabled);

#pragma endregion

}
