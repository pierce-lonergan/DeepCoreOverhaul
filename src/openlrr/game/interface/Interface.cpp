// Interface.cpp : 
//

#include "../../engine/input/Input.h"
#include "../../engine/input/Keys.h"

#include "Interface.h"


using Gods98::Keys;


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004a3b58>
real32 & LegoRR::g_Interface_TIMER_004a3b58 = *(real32*)0x004a3b58;

// <LegoRR.exe @004a3b5c>
uint32 & LegoRR::g_Interface_UINT_004a3b5c = *(uint32*)0x004a3b5c;

// <LegoRR.exe @004ddd58>
LegoRR::Interface_Globs & LegoRR::interfaceGlobs = *(LegoRR::Interface_Globs*)0x004ddd58;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0041c370>
void __cdecl LegoRR::Interface_DoF2InterfaceKeyAction(void)
{
	/// KEYBIND: [F2]+(any)  "F2 configurable interface actions."
	if ((interfaceGlobs.flags & INTERFACE_GLOB_FLAG_UNK_4) &&
		(interfaceGlobs.flags & INTERFACE_GLOB_FLAG_UNK_80) &&
		Input_IsKeyDown(Keys::KEY_F2))
	{
		Interface_FUN_0041b5b0(interfaceGlobs.currMenuType, Interface_CallbackDoMenuIconKeyAction, nullptr);
	}
}

#pragma endregion
