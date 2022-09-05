// Pointers.h : 
//

#pragma once

#include "../../engine/drawing/Flic.h"
#include "../../engine/drawing/Images.h"

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

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct Pointer_Globs // [LegoRR/Pointer.c|struct:0x468|tags:GLOBS]
{
	/*000,4*/	Pointer_Type currType;
	/*004,e0*/	Gods98::Image_Flic images[Pointer_Type_Count]; // (each item is either an Image or Flic)
	/*0e4,e0*/	bool32 imageIsFlic[Pointer_Type_Count];
	/*1c4,1c0*/	Point2I flicOffsets[Pointer_Type_Count];
	/*384,e0*/	const char* pointerName[Pointer_Type_Count];
	/*464,4*/	real32 timer; // Countdown timer to change pointer(?)
	/*468*/
};
assert_sizeof(Pointer_Globs, 0x468);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @00501a98>
extern Pointer_Globs & pointerGlobs;

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define Pointer_RegisterName(n)				(pointerGlobs.pointerName[n]=#n)

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0045caf0>
//#define Pointer_Initialise ((void (__cdecl* )(void))0x0045caf0)
void __cdecl Pointer_Initialise(void);

// Load all Pointers in the CFG block (prop is the first child in the block)
//  USAGE: <PointerType>    <bmpPath>
//  USAGE: <PointerType>    <flicPath>,<xOff>,<yOff>
// 
// NOTE: config is the first item in the array of pointers!!
// <LegoRR.exe @0045cd30>
//#define Pointer_Load ((void (__cdecl* )(const Gods98::Config* config))0x0045cd30)
void __cdecl Pointer_Load(const Gods98::Config* config);

// <LegoRR.exe @0045ce90>
//#define Pointer_GetType ((bool32 (__cdecl* )(const char* name, OUT Pointer_Type* pointerType))0x0045ce90)
bool32 __cdecl Pointer_GetType(const char* name, OUT Pointer_Type* pointerType);

// <LegoRR.exe @0045ced0>
//#define Pointer_GetImage ((Gods98::Image* (__cdecl* )(Pointer_Type pointerType))0x0045ced0)
Gods98::Image_Flic __cdecl Pointer_GetImage(Pointer_Type pointerType);

// <LegoRR.exe @0045cee0>
//#define Pointer_SetCurrent_IfTimerFinished ((void (__cdecl* )(Pointer_Type pointerType))0x0045cee0)
void __cdecl Pointer_SetCurrent_IfTimerFinished(Pointer_Type pointerType);

// <LegoRR.exe @0045cf00>
//#define Pointer_SetCurrent ((void (__cdecl* )(Pointer_Type pointerType, real32 timer))0x0045cf00)
void __cdecl Pointer_SetCurrent(Pointer_Type pointerType, real32 timer);

// <LegoRR.exe @0045cf20>
//#define Pointer_GetCurrentType ((Pointer_Type (__cdecl* )(void))0x0045cf20)
Pointer_Type __cdecl Pointer_GetCurrentType(void);

// <LegoRR.exe @0045cf30>
//#define Pointer_DrawPointer ((void (__cdecl* )(uint32 mouseX, uint32 mouseY))0x0045cf30)
void __cdecl Pointer_DrawPointer(uint32 mouseX, uint32 mouseY);

// <LegoRR.exe @0045d050>
//#define Pointer_Update ((void (__cdecl* )(real32 elapsedReal))0x0045d050)
void __cdecl Pointer_Update(real32 elapsedReal);

#pragma endregion

}
