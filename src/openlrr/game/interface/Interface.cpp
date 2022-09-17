// Interface.cpp : 
//

#include "../../engine/drawing/Draw.h"
#include "../../engine/input/Input.h"
#include "../../engine/input/Keys.h"

#include "../Game.h"
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


// <LegoRR.exe @0041cc60>
void __cdecl LegoRR::Interface_DrawHoverOutline(const Area2F* area)
{
	if (!(legoGlobs.flags1 & GAME1_FREEZEINTERFACE)) {
		Point2F start = {
			area->x,
			area->y,
		};
		Point2F end = {
			(area->x + area->width),
			(area->y + area->height),
		};

		// Draw for rect expanded by 1 pixel.
		// Then draw for rect expanded by 2 pixels, for a 2-pixel-width rectangle outline.

#if true
		/// REFACTOR: Draw all sets of lines in one Draw call instead of two.

		Point2F rectLinesFrom[8] = { 0.0f }; // dummy inits
		Point2F rectLinesTo[8] = { 0.0f };
		
		for (uint32 i = 0; i < 2; i++) {
			start.x -= 1.0f;
			start.y -= 1.0f;
			end.x   += 1.0f;
			end.y   += 1.0f;

			// 0 ____ 1
			//  |    |
			//  |____|
			// 3      2

			rectLinesFrom[(i*4) + 0] = Point2F { start.x, start.y };
			rectLinesFrom[(i*4) + 1] = Point2F {   end.x, start.y };
			rectLinesFrom[(i*4) + 2] = Point2F {   end.x,   end.y };
			rectLinesFrom[(i*4) + 3] = Point2F { start.x,   end.y };
			for (uint32 j = 0; j < 4; j++) {
				rectLinesTo[(i*4) + j] = rectLinesFrom[(i*4) + ((j+1)%4)];
			}
		}
		Gods98::Draw_LineListEx(rectLinesFrom, rectLinesTo, 8, 0.0f, 1.0f, 0.0f, Gods98::DrawEffect::None);

#else
		Point2F rectLines[5] = { 0.0f }; // dummy init

		for (uint32 i = 0; i < 2; i++) {
			start.x -= 1.0f;
			start.y -= 1.0f;
			end.x   += 2.0f;
			end.y   += 2.0f;

			// 0 ____ 1
			//  |    |
			//  |____|
			// 3      2

			rectLines[0] = Point2F { start.x, start.y };
			rectLines[1] = Point2F {   end.x, start.y };
			rectLines[2] = Point2F {   end.x,   end.y };
			rectLines[3] = Point2F { start.x,   end.y };
			rectLines[4] = rectLines[0];

			Gods98::Draw_LineListEx(rectLines, rectLines + 1, 4, 0.0f, 1.0f, 0.0f, Gods98::DrawEffect::None);
		}
#endif

	}
	return;
}

#pragma endregion
