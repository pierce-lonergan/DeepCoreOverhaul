// MouseButtons.h : 
//

#pragma once

#include "../../common.h"


namespace Gods98
{; // !<---

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

enum MouseButtons : uint8
{
    MOUSE_NONE     = 0,   // Not a real MOUSE BUTTON enum name
	MOUSE_LEFT     = 1,
	MOUSE_MIDDLE   = 2,
	MOUSE_RIGHT    = 3,
	MOUSE_XBUTTON1 = 4,
	MOUSE_XBUTTON2 = 5,
};

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct MouseButton_Globs
{
	const char* buttonName[6];
};

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

extern MouseButton_Globs mouseButtonGlobs;

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define MouseButton_RegisterName(k)		(mouseButtonGlobs.buttonName[k]=#k)

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

void __cdecl MouseButtons_Initialise(void);

bool32 __cdecl MouseButton_Find(const char* name, OUT MouseButtons* mouseButtonID);

const char* __cdecl MouseButton_GetName(MouseButtons mouseButton);

#pragma endregion

}

