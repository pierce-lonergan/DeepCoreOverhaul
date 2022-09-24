// Panels.cpp : 
//

#include "../Game.h"

#include "Panels.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @005010e0>
LegoRR::Panel_Globs & LegoRR::panelGlobs = *(LegoRR::Panel_Globs*)0x005010e0;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

/// CUSTOM: Hook function to overwrite Panel maximum radarmap zoom level.
void LegoRR::Panel_RadarMap_ZoomIn(real32 elapsedInterface)
{
	const real32 maxZoom = 30.0f; // OLD: 20.0f;
	legoGlobs.radarZoom = std::min(maxZoom, (legoGlobs.radarZoom + elapsedInterface));
}

/// CUSTOM: Hook function to overwrite Panel minimum radarmap zoom level.
void LegoRR::Panel_RadarMap_ZoomOut(real32 elapsedInterface)
{
	const real32 minZoom = 4.0f; // OLD: 10.0f;
	legoGlobs.radarZoom = std::max(minZoom, (legoGlobs.radarZoom - elapsedInterface));
}

#pragma endregion
