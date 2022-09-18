// Encyclopedia.cpp : 
//

#include "../Game.h"
#include "Panels.h"

#include "Encyclopedia.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004c8e88>
LegoRR::Encyclopedia_Globs & LegoRR::encyclopediaGlobs = *(LegoRR::Encyclopedia_Globs*)0x004c8e88;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// DRAW MODE: Only Draw API drawing calls can be used within this function.
// <LegoRR.exe @0040e800>
void __cdecl LegoRR::Encyclopedia_DrawSelectBox(Gods98::Viewport* viewMain)
{
	if ((encyclopediaGlobs.flags & ENCYCLOPEDIA_GLOB_FLAG_UNK_1) &&
		(panelGlobs.panelTable[Panel_Encyclopedia].flags & PANELTYPE_FLAG_UNK_2) &&
		encyclopediaGlobs.currentObj != nullptr)
	{
		Lego_DrawObjectSelectionBox(encyclopediaGlobs.currentObj, viewMain, 0.0f, 1.0f, 0.0f); // green
	}
}

/// CUSTOM: Isolate Draw API calls from Encyclopedia_DrawSelectBox.
void __cdecl LegoRR::Encyclopedia_DrawSelectName(Gods98::Viewport* viewMain)
{
	if ((encyclopediaGlobs.flags & ENCYCLOPEDIA_GLOB_FLAG_UNK_1) &&
		(panelGlobs.panelTable[Panel_Encyclopedia].flags & PANELTYPE_FLAG_UNK_2) &&
		encyclopediaGlobs.currentObj != nullptr)
	{
		Lego_DrawObjectName(encyclopediaGlobs.currentObj, viewMain);
	}
}

#pragma endregion
