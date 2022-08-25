// MouseButtons.cpp : 
//

#include "MouseButtons.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

Gods98::MouseButton_Globs Gods98::mouseButtonGlobs = { 0 };

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

void __cdecl Gods98::MouseButtons_Initialise(void)
{
	log_firstcall();

	for (uint32 loop = 0; loop < _countof(mouseButtonGlobs.buttonName); loop++) {
		mouseButtonGlobs.buttonName[loop] = nullptr;
	}

	//MouseButton_RegisterName(MOUSE_NONE);
	MouseButton_RegisterName(MOUSE_LEFT);
	MouseButton_RegisterName(MOUSE_MIDDLE);
	MouseButton_RegisterName(MOUSE_RIGHT);
	MouseButton_RegisterName(MOUSE_XBUTTON1);
	MouseButton_RegisterName(MOUSE_XBUTTON2);
}

bool32 __cdecl Gods98::MouseButton_Find(const char* name, OUT MouseButtons* mouseButtonID)
{
	log_firstcall();

	for (uint32 loop = 0; loop < _countof(mouseButtonGlobs.buttonName); loop++) {
		if (mouseButtonGlobs.buttonName[loop] && ::_stricmp(mouseButtonGlobs.buttonName[loop], name) == 0) {
			*mouseButtonID = (MouseButtons)loop;
			return true;
		}
	}

	return false;
}

const char* __cdecl Gods98::MouseButton_GetName(MouseButtons mouseButton)
{
	return mouseButtonGlobs.buttonName[mouseButton];
}

#pragma endregion
