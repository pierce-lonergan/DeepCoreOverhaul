// Interface.cpp : 
//

#include "../../engine/input/Input.h"
#include "../../engine/input/Keys.h"

#include "../Shortcuts.hpp"

#include "Interface.h"


using Gods98::Keys;
using Shortcuts::ShortcutID;


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
		Shortcut_IsDown(ShortcutID::InterfaceActionModifier))
	{
		Interface_FUN_0041b5b0(interfaceGlobs.currMenuType, Interface_CallbackDoMenuIconKeyAction, nullptr);
	}
}

// <LegoRR.exe @0041c3a0>
bool32 __cdecl LegoRR::Interface_CallbackDoMenuIconKeyAction(Interface_MenuItemType menuIcon, LegoObject_Type objType, LegoObject_ID objID)
{
	Keys keyID = Keys::KEY_NONE;
	if (menuIcon == Interface_MenuItem_Build) {
		if (objType == LegoObject_Vehicle) {
			keyID = interfaceGlobs.vehicleItemF2Keys[objID];
		}
		else if (objType == LegoObject_Building) {
			keyID = interfaceGlobs.buildingItemF2Keys[objID];
		}
	}
	else {
		keyID = interfaceGlobs.menuItemF2keys[menuIcon];
	}

	if (Input_IsKeyPressed(keyID)) {
		Interface_SetDat_004decd8_004decdc(menuIcon, objType, objID);
		Interface_DoAction_FUN_0041dbd0(menuIcon);

		// Prevent other debug shortcuts from triggering.
		Shortcuts::shortcutManager.Disable(true);
		// Re-activate the interface modifier shortcut.
		Shortcut_Activate(ShortcutID::InterfaceActionModifier, true);
	}
	return false;
}

#pragma endregion
