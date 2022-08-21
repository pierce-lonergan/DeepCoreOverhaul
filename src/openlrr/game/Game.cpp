// Game.cpp : 
//

#include "../engine/audio/3DSound.h"
#include "../engine/core/Maths.h"
#include "../engine/core/Utils.h"
#include "../engine/gfx/Viewports.h"
#include "../engine/input/Input.h"
#include "../engine/input/Keys.h"
#include "../engine/Main.h"

#include "audio/SFX.h"
#include "effects/DamageText.h"
#include "effects/Effects.h"
#include "effects/LightEffects.h"
#include "effects/Smoke.h"
#include "front/FrontEnd.h"
#include "interface/Advisor.h"
#include "interface/HelpWindow.h"
#include "interface/InfoMessages.h"
#include "interface/Interface.h"
#include "interface/Panels.h"
#include "interface/RadarMap.h"
#include "interface/TextMessages.h"
#include "interface/ToolTip.h"
#include "mission/Messages.h"
#include "mission/NERPsFile.h"
#include "mission/NERPsFunctions.h"
#include "object/AITask.h"
#include "object/Dependencies.h"
#include "object/Object.h"
#include "object/Stats.h"
#include "world/Construction.h"
#include "world/Detail.h"
#include "world/ElectricFence.h"
#include "world/Erode.h"
#include "world/Fallin.h"
#include "world/Map3D.h"
#include "world/Roof.h"
#include "world/SpiderWeb.h"
#include "Debug.h"
#include "Game.h"


using Gods98::Keys;


/**********************************************************************************
 ******** Game Entry point
 **********************************************************************************/

#pragma region Entry point

void __cdecl LegoRR::Lego_Gods_Go_Setup(const char* programName, OUT Gods98::Main_State* mainState)
{
	std::memset(&LegoRR::legoGlobs, 0, 0xef8); //sizeof(legoGlobs);
	LegoRR::legoGlobs.gameName = programName;

	
	/*Gods98::Main_State mainState = {
		Lego_Initialise,
		Lego_MainLoop,
		Lego_Shutdown_Full, // proper shutdown, with cleanup
	};*/

	mainState->Initialise = Lego_Initialise;
	mainState->MainLoop = Lego_MainLoop;
	mainState->Shutdown = Lego_Shutdown_Full; // shutdown with cleanup

	if (Gods98::Main_ProgrammerMode() != 10) { // PROGRAMMER_MODE_10
		mainState->Shutdown = Lego_Shutdown_Quick; // immediate shutdown, no cleanup
	}
}

// This is the GAME entry point as called by WinMain,
//  this should hook the Main_State loop functions and only perform basic initial setup.
// (this can return bool32, but does not)
// <LegoRR.exe @0041f950>
void __cdecl LegoRR::Lego_Gods_Go(const char* programName)
{
	log_firstcall();

	Gods98::Main_State mainState = { nullptr }; // dummy init
	Lego_Gods_Go_Setup(programName, &mainState);

	Gods98::Main_SetTitle(programName);
	Gods98::Main_SetState(&mainState);

	/*std::memset(&LegoRR::legoGlobs, 0, 0xef8); //sizeof(legoGlobs);
	LegoRR::legoGlobs.gameName = programName;

	/// FLUFF OPENLRR: Wrap the program name in parenthesis and start with "OpenLRR"
	char buff[1024];
	if (std::strcmp(programName, "OpenLRR") != 0) {
		std::sprintf(buff, "%s (%s)", "OpenLRR", programName);
		programName = buff;
	}
	Gods98::Main_SetTitle(programName);
	
	Gods98::Main_State mainState = {
		Lego_Initialise,
		Lego_MainLoop,
		Lego_Shutdown_Full, // proper shutdown, with cleanup
	};
	if (Gods98::Main_ProgrammerMode() != 10) { // PROGRAMMER_MODE_10
		mainState.Shutdown = Lego_Shutdown_Quick; // immediate shutdown, no cleanup
	}

	Gods98::Main_SetState(&mainState);*/
}

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004a4558>
LegoRR::LegoUpdate_Globs & LegoRR::updateGlobs = *(LegoRR::LegoUpdate_Globs*)0x004a4558;

// Only used by Lego_ShowBlockToolTip
// <LegoRR.exe @004df208>
Point2I & LegoRR::s_ShowBlockToolTip_MousePos = *(Point2I*)0x004df208;

// <LegoRR.exe @004df410>
LegoRR::GameControl_Globs & LegoRR::gamectrlGlobs = *(LegoRR::GameControl_Globs*)0x004df410;

// <LegoRR.exe @005570c0>
LegoRR::Lego_Globs & LegoRR::legoGlobs = *(LegoRR::Lego_Globs*)0x005570c0;

LegoRR::Lego_Globs2 LegoRR::legoGlobs2 = { 0 };

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions


/// CUSTOM: cfg: Main::ShowDebugToolTips
void LegoRR::Lego_SetShowDebugToolTips(bool on)
{
	if (on) legoGlobs.flags2 |= GAME2_SHOWDEBUGTOOLTIPS;
	else    legoGlobs.flags2 &= ~GAME2_SHOWDEBUGTOOLTIPS;
}

/// CUSTOM: cfg: Main::AllowDebugKeys
void LegoRR::Lego_SetAllowDebugKeys(bool on)
{
	if (on) legoGlobs.flags2 |= GAME2_ALLOWDEBUGKEYS;
	else    legoGlobs.flags2 &= ~GAME2_ALLOWDEBUGKEYS;
}

/// CUSTOM: cfg: Main::AllowEditMode
void LegoRR::Lego_SetAllowEditMode(bool on)
{
	if (on) legoGlobs.flags2 |= GAME2_ALLOWEDITMODE;
	else    legoGlobs.flags2 &= ~GAME2_ALLOWEDITMODE;
}

/// REQUIRES: Replacing `Lego_HandleKeys`, since this is always checked by CFG lookup.
/// CUSTOM: cfg: Main::LoseFocusAndPause
bool LegoRR::Lego_IsLoseFocusAndPause()
{
	return legoGlobs2.loseFocusAndPause;

	//return Config_GetBoolOrFalse(legoGlobs.config, Main_ID("LoseFocusAndPause"));
}
void LegoRR::Lego_SetLoseFocusAndPause(bool on)
{
	legoGlobs2.loseFocusAndPause = on;

	/*Gods98::Config* cfgFocus = const_cast<Gods98::Config*>(Gods98::Config_FindItem(legoGlobs.config, Main_ID("LoseFocusAndPause")));
	if (cfgFocus == nullptr) {
		// Hoooh boy, this is gonna be a pain. The game reads the cfg value AS NEEDED!!!

		// Instead of doing anything fancy with the existing tree,
		//  we'll append a new tree with the exact path to the property we need.

		// Find tail of config linked list.
		Gods98::Config* tail = legoGlobs.config;
		while (tail->linkNext != nullptr) tail = tail->linkNext;

		Gods98::Config* cfgGame = Gods98::Config_Create(tail);
		Gods98::Config* cfgMain = Gods98::Config_Create(cfgGame);
		cfgFocus = Gods98::Config_Create(cfgMain);

		// Lego*::
		cfgGame->depth = 0;
		cfgGame->itemName = legoGlobs.gameName;
		cfgGame->dataString = "{";
		// Lego*::Main::
		cfgMain->depth = cfgGame->depth + 1;
		cfgMain->itemName = "Main";
		cfgMain->dataString = "{";
		// Lego*::Main::LoseFocusAndPause
		cfgFocus->depth = cfgMain->depth + 1;
		cfgFocus->itemName = "LoseFocusAndPause";
		cfgFocus->dataString = "FALSE"; // default value when not found
	}
	cfgFocus->dataString = (on ? "TRUE" : "FALSE");*/
}

/// REQUIRES: Replacing `Weapon_LegoObject_SeeThroughWalls_FUN_00471c20`, since this is always checked by CFG lookup.
/// CUSTOM: cfg: Level::SeeThroughWalls
bool LegoRR::Lego_IsSeeThroughWalls()
{
	if (Lego_IsInLevel()) {
		return legoGlobs2.seeThroughWalls;
	}
	return true; // Default value when none exists, doesn't really matter when outside of a level.

	//return Config_GetBoolOrTrue(legoGlobs.config, Lego_ID(levelName, "SeeThroughWalls"));
}
void LegoRR::Lego_SetSeeThroughWalls(bool on)
{
	if (Lego_IsInLevel()) {
		legoGlobs2.seeThroughWalls = on;
	}

	/*/// FIXME: This is a bad temporary implementation, because it permenantly changes the level for this game session!!!
	Gods98::Config* cfgSee = const_cast<Gods98::Config*>(Gods98::Config_FindItem(legoGlobs.config, Lego_ID(levelName, "SeeThroughWalls")));
	if (cfgSee == nullptr) {

		// We know this property has to exist if this is a valid level.
		// Lego*::Levels::<levelName>
		Gods98::Config* cfgLevel = const_cast<Gods98::Config*>(Gods98::Config_FindItem(legoGlobs.config, Lego_ID(levelName)));

		if (cfgLevel == nullptr)
			return; // Failed to find level with this name.

		// Find tail of level block linked list.
		Gods98::Config* tail = cfgLevel;
		while (tail->linkNext != nullptr) tail = tail->linkNext;

		// Create as child of cfgLevel.
		cfgSee = Gods98::Config_Create(cfgLevel);

		// Lego*::Levels::<levelName>::SeeThroughWalls
		cfgSee->depth = cfgLevel->depth + 1;
		cfgSee->itemName = "SeeThroughWalls";
		cfgSee->dataString = "TRUE"; // default value when not found
	}
	cfgSee->dataString = (on ? "TRUE" : "FALSE");*/
}

/// CUSTOM: cfg: Main::DisableToolTipSound
void LegoRR::Lego_SetDisableToolTipSound(bool disabled)
{
	if (disabled) legoGlobs.flags2 |= GAME2_DISABLETOOLTIPSOUND;
	else          legoGlobs.flags2 &= ~GAME2_DISABLETOOLTIPSOUND;
}

/// CUSTOM: cfg: Main::Panels
void LegoRR::Lego_SetRenderPanels(bool on)
{
	if (on) legoGlobs.flags1 |= GAME1_RENDERPANELS;
	else    legoGlobs.flags1 &= ~GAME1_RENDERPANELS;
}

/// CUSTOM: cfg: Main::Clear
void LegoRR::Lego_SetDDrawClear(bool on)
{
	if (on) legoGlobs.flags1 |= GAME1_DDRAWCLEAR;
	else    legoGlobs.flags1 &= ~GAME1_DDRAWCLEAR;
}

// <inlined>
void LegoRR::Lego_SetDetailOn(bool on)
{
	if (on) legoGlobs.flags1 |= GAME1_USEDETAIL;
	else    legoGlobs.flags1 &= ~GAME1_USEDETAIL;
}

// <inlined>
void LegoRR::Lego_SetDynamicPM(bool on)
{
	if (on) legoGlobs.flags1 |= GAME1_DYNAMICPM;
	else    legoGlobs.flags1 &= ~GAME1_DYNAMICPM;
}

// <inlined>
void LegoRR::Lego_SetFPSMonitorOn(bool on)
{
	if (on) legoGlobs.flags1 |= GAME1_SHOWFPS;
	else    legoGlobs.flags1 &= ~GAME1_SHOWFPS;
}

// <inlined>
void LegoRR::Lego_SetMemoryMonitorOn(bool on)
{
	if (on) legoGlobs.flags1 |= GAME1_SHOWMEMORY;
	else    legoGlobs.flags1 &= ~GAME1_SHOWMEMORY;
}

// <inlined>
void LegoRR::Lego_SetNoNERPs(bool on)
{
	if (on) legoGlobs.flags1 |= GAME1_DEBUG_NONERPS;
	else    legoGlobs.flags1 &= ~GAME1_DEBUG_NONERPS;
}

// <inlined>
void LegoRR::Lego_SetOnlyBuildOnPaths(bool on)
{
	if (on) legoGlobs.flags1 |= GAME1_ONLYBUILDONPATHS;
	else    legoGlobs.flags1 &= ~GAME1_ONLYBUILDONPATHS;
}

// <inlined>
void LegoRR::Lego_SetNoclipOn(bool on)
{
	if (on) legoGlobs.flags1 |= GAME1_DEBUG_NOCLIP_FPS;
	else    legoGlobs.flags1 &= ~GAME1_DEBUG_NOCLIP_FPS;
}

// <inlined>
void LegoRR::Lego_SetAlwaysRockFall(bool on)
{
	if (on) legoGlobs.flags1 |= GAME1_ALWAYSROCKFALL;
	else    legoGlobs.flags1 &= ~GAME1_ALWAYSROCKFALL;
}

// <inlined>
void LegoRR::Lego_SetAllowRename(bool on)
{
	if (on) legoGlobs.flags2 |= GAME2_ALLOWRENAME;
	else    legoGlobs.flags2 &= ~GAME2_ALLOWRENAME;
}

// <inlined>
void LegoRR::Lego_SetGenerateSpiders(bool on)
{
	if (on) legoGlobs.flags2 |= GAME2_GENERATESPIDERS;
	else    legoGlobs.flags2 &= ~GAME2_GENERATESPIDERS;
}

// <inlined>
void LegoRR::Lego_SetNoAutoEat(bool on)
{
	if (on) legoGlobs.flags2 |= GAME2_NOAUTOEAT;
	else    legoGlobs.flags2 &= ~GAME2_NOAUTOEAT;
}

// <inlined>
void LegoRR::Lego_SetNoMultiSelect(bool on)
{
	if (on) legoGlobs.flags2 |= GAME2_NOMULTISELECT;
	else    legoGlobs.flags2 &= ~GAME2_NOMULTISELECT;
}





// <LegoRR.exe @00424660>
void __cdecl LegoRR::Lego_UpdateSceneFog(bool32 fogEnabled, real32 elapsed)
{
	if (legoGlobs.flags1 & GAME1_FOGCOLOURRGB) {
		Gods98::Container_EnableFog(fogEnabled);

		/// NOTE: Fog is already set to `FogColourRGB` during Lego_LoadLevel,
		///        so we only need to change it when we have `HighFogColourRGB`.
		if (fogEnabled && (legoGlobs.flags1 & GAME1_HIGHFOGCOLOURRGB)) {

			// (M_PI*2) float hex: 0x40c90fdb
			gamectrlGlobs.sceneFogDelta += (((M_PI*2.0f) / legoGlobs.FogRate) * elapsed);

			const real32 waveDelta = std::sin(gamectrlGlobs.sceneFogDelta); // ASM FSIN

			ColourRGBF fogRGB = legoGlobs.FogColourRGB; // Base channel values before range is added.
			for (uint32 i = 0; i < 3; i++) {
				const real32 range = (legoGlobs.HighFogColourRGB.channels[i] - legoGlobs.FogColourRGB.channels[i]) * 0.5f;
				fogRGB.channels[i] += range * waveDelta;
			}
			Gods98::Container_SetFogColour(fogRGB.red, fogRGB.green, fogRGB.blue);
		}
	}
}

LegoRR::ToolTip_Type LegoRR::Lego_PrepareObjectToolTip(LegoObject* liveObj)
{
	/// FIX APPLY: Increase horribly small buffer sizes
	char buffVal[TOOLTIP_BUFFERSIZE]; //[128];
	char buffText[TOOLTIP_BUFFERSIZE * 4]; //[256]; // x4 so that we can safely cap it at TOOLTIP_BUFFERSIZE before calling ToolTip_SetText
	buffText[0] = '\0'; // Sanity init


	const bool debugToolTips = (legoGlobs.flags2 & GAME2_SHOWDEBUGTOOLTIPS);


	const char* langName = LegoObject_GetLangName(liveObj);
	if (debugToolTips && (langName == nullptr || langName[0] == '\0')) {
		// (Debug only) If no language name is defined (or name is empty),
		//               then get the internal object Type/ID name.
		langName = Debug_GetObjectIDName(liveObj->type, liveObj->id);
	}

	// Only show a tooltip if we have a name to display for this object.
	if (langName != nullptr && langName[0] != '\0') {

		// Object name:
		if (liveObj->customName != nullptr && liveObj->customName[0] == '\0') {
			// Remove empty (but defined) custom names ahead of time.
			LegoObject_SetCustomName(liveObj, nullptr);
		}
		// Use custom name instead of langName, if we have one.
		const char* objName = ((liveObj->customName != nullptr) ? liveObj->customName : langName);
		std::strcpy(buffText, objName);


		// Upgrade level:
		const char* upgradeName = legoGlobs.langUpgradeLevel_name[liveObj->objLevel];
		if (debugToolTips || (liveObj->type == LegoObject_Building && upgradeName != nullptr)) {
			if (liveObj->type == LegoObject_Vehicle) {
				// (Debug only) Show level as bits for vehicles (1 bit per upgrade type).
				//              But don't show level 0.
				if (liveObj->objLevel != 0) {
					std::sprintf(buffVal, " (UPG%i%i%i%i)",
								 ((liveObj->objLevel & UPGRADE_FLAG_CARRY) ? 1 : 0),
								 ((liveObj->objLevel & UPGRADE_FLAG_SCAN)  ? 1 : 0),
								 ((liveObj->objLevel & UPGRADE_FLAG_SPEED) ? 1 : 0),
								 ((liveObj->objLevel & UPGRADE_FLAG_DRILL) ? 1 : 0));
					std::strcat(buffText, buffVal);
				}
			}
			else if (upgradeName == nullptr) {
				// (Debug only) Show generic upgrade text, when none is defined.
				//              But don't show level 0.
				if (liveObj->objLevel != 0) {
					std::sprintf(buffVal, " (UP%i)", liveObj->objLevel);
					std::strcat(buffText, buffVal);
				}
			}
			else {
				// Only buildings openly display their upgrade level: (UP1), (UP2), etc...
				std::sprintf(buffVal, " (%s)", upgradeName);
				std::strcat(buffText, buffVal);
			}
		}

		// World position:
		if (debugToolTips) {
			real32 xPos, yPos, zPos;
			LegoObject_GetPosition(liveObj, &xPos, &yPos);
			if (liveObj->type == LegoObject_MiniFigure) {
				zPos = LegoObject_GetWorldZCallback_Lake(xPos, yPos, Lego_GetMap());
			}
			else {
				zPos = LegoObject_GetWorldZCallback(xPos, yPos, Lego_GetMap());
			}
			std::sprintf(buffVal, " (%0.1f,%0.1f,%0.1f)", xPos, yPos, zPos);
			std::strcat(buffText, buffVal);
		}

		// Health and Energy:
		if (liveObj->flags3 & LIVEOBJ3_UNK_40000) {
			const uint32 health = ((liveObj->health >= 0.0f) ? (uint32)liveObj->health : 0);
			const uint32 energy = ((liveObj->energy >= 0.0f) ? (uint32)liveObj->energy : 0);

			if (debugToolTips) {
				// Under non-debug circumstances, the game would run fine without this text being defined.
				const char* healthToolTip = legoGlobs.langHealth_toolTip;
				if (healthToolTip == nullptr) healthToolTip = "Health";

				// (Debug only) Always show health and energy.
				std::sprintf(buffVal, "\n%s: %i", healthToolTip, health);
				std::strcat(buffText, buffVal);

				std::sprintf(buffVal, "\n%s: %i", legoGlobs.langEnergy_toolTip, energy);
				std::strcat(buffText, buffVal);
			}
			else if (DamageFont_LiveObject_CheckCanShowDamage_Unk(liveObj) &&
					 (liveObj->type != LegoObject_MiniFigure && liveObj->type != LegoObject_RockMonster))
			{
				// Vehicles, buildings, etc. show "Energy" as their health.
				std::sprintf(buffVal, "\n%s: %i", legoGlobs.langEnergy_toolTip, health);
				std::strcat(buffText, buffVal);
			}
		}

		// Cargo count:
		if (debugToolTips && liveObj->numCarriedObjects != 0) {
			std::sprintf(buffVal, "\nCargo: %i", liveObj->numCarriedObjects);
			std::strcat(buffText, buffVal);
		}

		// Driver custom name:
		if (liveObj->type == LegoObject_Vehicle && liveObj->driveObject != nullptr &&
			(liveObj->driveObject->customName != nullptr && liveObj->driveObject->customName[0] != '\0') &&
			legoGlobs.langDrivenBy_toolTip != nullptr)
		{
			// Only show "Driven by:" text for drivers with custom names.
			std::sprintf(buffVal, "\n%s: %s", legoGlobs.langDrivenBy_toolTip, liveObj->driveObject->customName);
			std::strcat(buffText, buffVal);
		}

		// Task and Activity:
		if (debugToolTips) {
			if (liveObj->aiTask != nullptr) {
				//const char* taskName = AITask_GetTaskName(liveObj->aiTask);
				const char* taskName = aiGlobs.taskName[liveObj->aiTask->taskType];
				const char* prioName = aiGlobs.priorityName[liveObj->aiTask->priorityType];
				
				std::sprintf(buffVal, "\nTask: %s", taskName + std::strlen("AITask_Type_"));
				std::strcat(buffText, buffVal);
				
				std::sprintf(buffVal, "\nPriority: %s", prioName + std::strlen("AI_Priority_"));
				std::strcat(buffText, buffVal);
			}
			if (liveObj->activityName1 != nullptr) {
				std::sprintf(buffVal, "\nActivity: %s", liveObj->activityName1 + std::strlen("Activity_"));
				std::strcat(buffText, buffVal);
			}
		}

		// Routing blocks:
		if (debugToolTips && liveObj->routeBlocks != nullptr && liveObj->routeBlocksTotal != 0) {
			std::sprintf(buffVal, "\nRouting: %i/%i", liveObj->routeBlocksCurrent, liveObj->routeBlocksTotal);
			std::strcat(buffText, buffVal);
		}

		// Record object ID:
		if (debugToolTips) {
			for (uint32 i = 0; i < legoGlobs.recordObjsCount; i++) {
				LegoObject* recordObj = legoGlobs.recordObjs[i];
				if (recordObj == liveObj) {
					std::sprintf(buffVal, "\nRecord: %i", (uint32)i);
					std::strcat(buffText, buffVal);
					break;
				}
			}
		}

		// Object flags: (consider another setting to remove this verbose info)
		if (debugToolTips) {
			std::sprintf(buffVal, "\nFlags1: %08x\nFlags2: %08x\nFlags3: %08x\nFlags4: %08x",
						 (uint32)liveObj->flags1,
						 (uint32)liveObj->flags2,
						 (uint32)liveObj->flags3,
						 (uint32)liveObj->flags4);
			std::strcat(buffText, buffVal);
		}


		/// SANITY: Until we increase the tooltip buffer size, ensure we don't exceed 511 characters.
		if (std::strlen(buffText) >= TOOLTIP_BUFFERSIZE) {
			std::strcpy(&buffText[TOOLTIP_BUFFERSIZE-4], "...");
		}
		ToolTip_SetText(ToolTip_UnitSelect, buffText);

		const SFX_ID objTtSFX = LegoObject_GetObjTtSFX(liveObj);
		ToolTip_SetSFX(ToolTip_UnitSelect, objTtSFX);


		// Carried tools and Trained abilities icons:
		if (liveObj->type == LegoObject_MiniFigure) {
			// Only minifigures have tools and abilities to display.

			// Carried tools:
			{
				uint32 i = 0;
				// Draw equipped tool icons:
				for (; i < liveObj->numCarriedTools; i++) {
					const LegoObject_ToolType toolType = liveObj->carriedTools[i];
					if (objectGlobs.ToolTipIcons_Tools[toolType] != nullptr) {
						ToolTip_AddIcon(ToolTip_UnitSelect, objectGlobs.ToolTipIcons_Tools[toolType]);
					}
				}
				// Draw blank icons for the remaining space the unit has available.
				for (; i < StatsObject_GetNumOfToolsCanCarry(liveObj); i++) {
					if (objectGlobs.ToolTipIcon_Blank != nullptr) {
						ToolTip_AddIcon(ToolTip_UnitSelect, objectGlobs.ToolTipIcon_Blank);
					}
				}
			}

			// Trained abilities:
			if (liveObj->abilityFlags != ABILITY_FLAG_NONE) {
				// Show ability icons, only when unit has been trained in at least one.

				// I think this nullptr is used here to add a new row (to show abilities below tool icons).
				ToolTip_AddIcon(ToolTip_UnitSelect, nullptr);

				for (uint32 i = 0; i < LegoObject_AbilityType_Count; i++) {
					Gods98::Image* abilityIcon;
					if (liveObj->abilityFlags & (0x1<<i))
						abilityIcon = objectGlobs.ToolTipIcons_Abilities[i];
					else
						abilityIcon = objectGlobs.ToolTipIcon_Blank;

					if (abilityIcon != nullptr) {
						ToolTip_AddIcon(ToolTip_UnitSelect, abilityIcon);
					}
				}
			}
		}

		// Something like flushing the tooltip buffer, now that its ready(?)
		ToolTip_AddFlag4(ToolTip_UnitSelect);

		return ToolTip_UnitSelect;
	}
	return ToolTip_Type_Count; // invalid
}


// <LegoRR.exe @00427f50>
void __cdecl LegoRR::Lego_ShowObjectToolTip(LegoObject* liveObj)
{
	const bool debugToolTips = (legoGlobs.flags2 & GAME2_SHOWDEBUGTOOLTIPS);


	const char* langName = LegoObject_GetLangName(liveObj);
	if (debugToolTips && (langName == nullptr || langName[0] == '\0')) {
		// (Debug only) If no language name is defined (or name is empty),
		//               then get the internal object Type/ID name.
		langName = Debug_GetObjectIDName(liveObj->type, liveObj->id);
	}

	// Only show a tooltip if we have a name to display for this object.
	if (langName != nullptr && langName[0] != '\0') {

		// Not the current toolTip object, abort, and let the next update loop handle this object.
		if (gamectrlGlobs.toolTipObject != liveObj) {
			gamectrlGlobs.toolTipObject = liveObj;
			return;
		}

		Lego_PrepareObjectToolTip(liveObj);
	}
}

LegoRR::ToolTip_Type LegoRR::Lego_PrepareConstructionToolTip(const Point2I* blockPos, bool32 showConstruction)
{
	/// FIX APPLY: Increase horribly small buffer sizes
	char buffVal[TOOLTIP_BUFFERSIZE]; //[128];
	char buffText[TOOLTIP_BUFFERSIZE * 4]; //[128]; // x4 so that we can safely cap it at TOOLTIP_BUFFERSIZE before calling ToolTip_SetText
	buffText[0] = '\0'; // Sanity init


	const bool debugToolTips = (legoGlobs.flags2 & GAME2_SHOWDEBUGTOOLTIPS);


	const Lego_Block* block = &blockValue(Lego_GetLevel(), blockPos->x, blockPos->y);
	Construction_Zone* construct = block->construct;

	// Building name:
	const char* objName = Object_GetLangName(LegoObject_Building, construct->objID);
	std::strcat(buffText, objName);


	// Resource progress:
	if (showConstruction) {

		const uint32 crystals = Construction_Zone_CountOfResourcePlaced(construct, LegoObject_PowerCrystal, (LegoObject_ID)0);
		const uint32 crystalsCost = Stats_GetCostCrystal(LegoObject_Building, construct->objID, 0);

		const uint32 ore   = Construction_Zone_CountOfResourcePlaced(construct, LegoObject_Ore, LegoObject_ID_Ore);
		const uint32 studs = Construction_Zone_CountOfResourcePlaced(construct, LegoObject_Ore, LegoObject_ID_ProcessedOre);
		const uint32 oreCost   = Stats_GetCostOre(LegoObject_Building, construct->objID, 0);
		const uint32 studsCost = Stats_GetCostRefinedOre(LegoObject_Building, construct->objID, 0);

		// Crystals progress:
		if (crystalsCost != 0) {
			std::sprintf(buffVal, "\n%s: %i/%i", legoGlobs.langCrystals_toolTip, crystals, crystalsCost);
			std::strcat(buffText, buffVal);
		}

		const bool useStuds = (construct->flags & CONSTRUCTION_FLAG_USESTUDS);

		const char* oreTypeToolTip = (useStuds ? legoGlobs.langStuds_toolTip : legoGlobs.langOre_toolTip);
		const uint32 oreType       = (useStuds ? studs     : ore);
		const uint32 oreTypeCost   = (useStuds ? studsCost : oreCost);

		// Ore progress:
		if (oreTypeCost != 0) {
			std::sprintf(buffVal, "\n%s: %i/%i", oreTypeToolTip, oreType, oreTypeCost);
			std::strcat(buffText, buffVal);
		}

		// (Debug only) Barriers progress:
		if (debugToolTips) {
			Construction_Zone_CountOfResourcePlaced(construct, LegoObject_Barrier, (LegoObject_ID)0);
			// Currently number of barriers requested is not counted,
			//  we can hack it thanks to the assumption that all non-cryore costs are barriers.
			const sint32 barriers = (sint32)construct->placedCount - (sint32)(crystals + oreType);
			const sint32 barriersCost = (sint32)construct->requestCount - (sint32)(crystalsCost + oreTypeCost);
			if (barriers >= 0 && barriersCost > 0) {
				const char* barrierName = Object_GetLangName(LegoObject_Barrier, (LegoObject_ID)0);
				if (barrierName == nullptr || barrierName[0] == '\0') {
					barrierName = Debug_GetObjectIDName(LegoObject_Barrier, (LegoObject_ID)0);
				}

				if (barrierName != nullptr && barrierName[0] != '\0') {
					std::sprintf(buffVal, "\n%s: %i/%i", barrierName, barriers, barriersCost);
					std::strcat(buffText, buffVal);
				}
			}
		}

		// (Debug only) Foreign objects obstructing:
		if (debugToolTips) {
			bool obstructed = !Construction_Zone_NoForeignObjectsInside(construct);
			std::sprintf(buffVal, "\nObstructed: %s", (obstructed ? "Yes": "No"));
			std::strcat(buffText, buffVal);
		}
	}


	/// SANITY: Until we increase the tooltip buffer size, ensure we don't exceed 511 characters.
	if (std::strlen(buffText) >= TOOLTIP_BUFFERSIZE) {
		std::strcpy(&buffText[TOOLTIP_BUFFERSIZE-4], "...");
	}
	ToolTip_SetText(ToolTip_Construction, buffText);

	ToolTip_SetSFX(ToolTip_Construction, SFX_NULL);
	ToolTip_AddFlag4(ToolTip_Construction);
	return ToolTip_Construction;
}

LegoRR::ToolTip_Type LegoRR::Lego_PrepareMapBlockToolTip(const Point2I* blockPos, bool32 playSound, bool32 showCavern)
{
	/// FIX APPLY: Increase horribly small buffer sizes
	// Originally these buffers were only used for Construction.
	char buffVal[TOOLTIP_BUFFERSIZE]; //[128];
	char buffText[TOOLTIP_BUFFERSIZE * 4]; //[128]; // x4 so that we can safely cap it at TOOLTIP_BUFFERSIZE before calling ToolTip_SetText
	buffText[0] = '\0'; // Sanity init


	const bool debugToolTips = (legoGlobs.flags2 & GAME2_SHOWDEBUGTOOLTIPS);


	const Lego_Block* block = &blockValue(Lego_GetLevel(), blockPos->x, blockPos->y);

	Lego_SurfaceType surfType; // = Lego_SurfaceType_Tunnel; // dummy init (0)
	bool showToolTip = true; // default behaviour (only changed for solid building tiles)


	if (!(block->flags1 & BLOCK1_FLOOR)) {
		// Wall types:
		if (!(block->flags1 & BLOCK1_WALL) && (block->flags1 & BLOCK1_HIDDEN)) {
			surfType = Lego_SurfaceType_Undiscovered; // Undiscovered cavern (not necessarily a wall type)
		}
		else if (block->flags1 & BLOCK1_REINFORCED) {
			/// UNINLINED: Level_Block_IsReinforced(blockPos->x, blockPos->y)
			surfType = Lego_SurfaceType_Reinforcement; // Reinforced wall (of any type).
		}
		else {
			surfType = (Lego_SurfaceType)block->terrain; // All other wall types
		}
	}
	else {
		// Floor types:
		if (block->terrain == Lego_SurfaceType_Lake || block->terrain == Lego_SurfaceType_Lava) {
			surfType = (Lego_SurfaceType)block->terrain; // Lake ("Water") and Lava (overrides all other floor type flags).
		}
		else if (block->flags1 & BLOCK1_UNK_80000000) {
			// Tunnel: (what exactly is this flag? Considering we have a second case for Tunnel below)
			//         Although possibly just an optimization. Flow for the second case jumps to here.
			surfType = Lego_SurfaceType_Tunnel;
		}
		else if (!(block->flags1 & BLOCK1_CLEARED_UNK)) {
			/// UNINLINED: Level_Block_GetRubbleLayers(blockPos) != 0
			surfType = Lego_SurfaceType_Rubble; // Uncleared rubble
		}
		else if (block->flags2 & BLOCK2_SLUGHOLE_EXPOSED) {
			surfType = Lego_SurfaceType_SlugHole; // Slimy slug hole
		}
		else if (Level_Block_IsPath(blockPos) || Level_Block_IsPathBuilding(blockPos)) {
			/// INLINED: (block->flags1 & (BLOCK1_PATH|BLOCK1_BUILDINGPATH))
			surfType = Lego_SurfaceType_Path; // Power path
		}
		else if (Level_Block_IsSolidBuilding(blockPos->x, blockPos->y, true) || (block->flags1 & BLOCK1_FOUNDATION)) {
			/// INLINED: (block->flags1 & (BLOCK1_BUILDINGSOLID|BLOCK1_FOUNDATION)) || (block->flags2 & BLOCK2_TOOLSTORE)
			showToolTip = false; // Core building tile: DON'T SHOW TOOLTIP!
		}
		else if (showCavern) {
			surfType = Lego_SurfaceType_Cavern; // Catch all for Ground
		}
		else {
			surfType = Lego_SurfaceType_Tunnel; // Catch all for Roof(?)
		}
	}
	//if (showCavern == 0) goto LAB_004285a8;


	if (showToolTip) {
		const char* surfName = Lego_GetSurfaceTypeDescription(surfType);
		SFX_ID surfSFX = Lego_GetSurfaceTypeSFX(surfType);

		// Block surface name:
		std::strcat(buffText, surfName);

		if (debugToolTips) {
			// Block coordinates:
			std::sprintf(buffVal, " (%i,%i)", blockPos->x, blockPos->y);
			std::strcat(buffText, buffVal);


			// Texture hex ID (and direction):
			static constexpr const std::array<char, 4> directionChars = { 'U', 'R', 'D', 'L' };
			std::sprintf(buffVal, "\nTex: %02x, Dir: %c", (uint32)block->texture, directionChars[block->direction % directionChars.size()]);
			std::strcat(buffText, buffVal);
			
			// Wall damage:
			if (Level_Block_IsWall(blockPos->x, blockPos->y)) {
				const uint32 damage = ((block->damage >= 0.0f) ? (uint32)(block->damage*100) : 0);

				std::sprintf(buffVal, "\nDamage: %i%%", damage); // Percentage version
				//std::sprintf(buffVal, "\nnDamage: %0.2f", block->damage); // Ratio version
				std::strcat(buffText, buffVal);
			}

			// Rubble layers:
			const uint32 rubbleLayers = Level_Block_GetRubbleLayers(blockPos);
			if (!Level_Block_IsWall(blockPos->x, blockPos->y) && rubbleLayers != 0) {
				std::sprintf(buffVal, "\nLayers: %i/%i", rubbleLayers, 4); // 4 == Max rubble layers
				std::strcat(buffText, buffVal);
			}

			// Clicks:
			if (block->clickCount != 0) {
				std::sprintf(buffVal, "\nClicks: %i", (uint32)block->clickCount);
				std::strcat(buffText, buffVal);
			}

			// Landslides:
			if (block->numLandSlides != 0) {
				std::sprintf(buffVal, "\nLandslides: %i", (uint32)block->numLandSlides);
				std::strcat(buffText, buffVal);
			}

			// CryOre:
			// Sanity check for valid cryore count.
			// It's entirely possible to "safely" have this value out of range,
			//  but never have the count accessed because of not being in a wall block).
			uint32 cryOre[2][2] = { { 0 } }; // [cry,ore][lv0,lv1] dummy init
			if (block->cryOre != Lego_CryOreType_None && block->cryOre <= Lego_CryOreType_Ore_Lv1_25 &&
				Lego_GetBlockCryOre(blockPos, &cryOre[0][0], &cryOre[0][1], &cryOre[1][0], &cryOre[1][1]))
			{
				for (uint32 i = 0; i < 2; i++) { // Type
					const LegoObject_Type cryOreType = (i == 1 ? LegoObject_Ore : LegoObject_PowerCrystal);

					const char* cryOreToolTip = (cryOreType == LegoObject_Ore ? legoGlobs.langOre_toolTip : legoGlobs.langCrystals_toolTip);

					if (cryOreToolTip == nullptr || cryOreToolTip[0] == '\0') {
						cryOreToolTip = Object_GetLangName(cryOreType, (LegoObject_ID)0);
						if (cryOreToolTip == nullptr || cryOreToolTip[0] == '\0') {
							cryOreToolTip = Debug_GetObjectIDName(cryOreType, (LegoObject_ID)0);
						}
					}

					for (uint32 j = 0; j < 2; j++) { // Level
						const uint32 cryOreLevel = j;
						const uint32 cryOreCount = cryOre[i][j];
						
						if (cryOreCount != 0 && cryOreToolTip != nullptr && cryOreToolTip[0] != '\0') {
							std::sprintf(buffVal, "\n%s: %i", cryOreToolTip, cryOreCount);
							std::strcat(buffText, buffVal);
							if (cryOreLevel != 0) {
								std::sprintf(buffVal, " (L%i)", cryOreLevel);
								std::strcat(buffText, buffVal);
							}
						}
					}
				}
			}
			
			// Erosion:
			if (block->erodeSpeed != Lego_ErodeType_None) {// && block->terrain != Lego_SurfaceType_Lava) {
				bool erodeActive = false;
				for (uint32 i = 0; i < _countof(erodeGlobs.activeBlocks); i++) {
					const Point2I& activePos = erodeGlobs.activeBlocks[i];
					if (erodeGlobs.activeStates[i] && activePos.x == blockPos->x && activePos.y == blockPos->y) {
						erodeActive = true;
						break;
					}
				}
				// Include "None" as dummy to keep array index lookup the same as erodeSpeed.
				static constexpr const std::array<const char*, 6> erodeSpeedNames = { "None", "VSlow", "Slow", "Med", "Fast", "VFast" };
				const char* erodeActiveName = (erodeActive ? "Active" : "Dormant");
				// Casts are vital here, since values are stored in `uint8`.
				if (block->erodeSpeed >= erodeSpeedNames.size()) {
					std::sprintf(buffVal, "\nErode: %i", (uint32)block->erodeSpeed);
					std::strcat(buffText, buffVal);
				}
				else {
					std::sprintf(buffVal, "\nErode: %s", erodeSpeedNames[block->erodeSpeed]);
					std::strcat(buffText, buffVal);
				}
				std::sprintf(buffVal, ", %s, %i/%i", erodeActiveName, (uint32)block->erodeLevel, 4); // Progress 4 is lava
				std::strcat(buffText, buffVal);
			}

			// Emerge:
			if (Lego_GetLevel()->emergeTriggers != nullptr) {
				for (uint32 i = 0; i < Lego_GetLevel()->emergeCount; i++) {
					auto& emergeTrigger = Lego_GetLevel()->emergeTriggers[i];
					if (emergeTrigger.blockPos.x == blockPos->x && emergeTrigger.blockPos.y == blockPos->y) {
						std::sprintf(buffVal, "\nEmerge: %i, Trigger", i);
						std::strcat(buffText, buffVal);
						break;
					}
					else {
						bool found = false;
						for (uint32 j = 0; j < _countof(emergeTrigger.emergePoints); j++) {
							auto& emergeBlock = emergeTrigger.emergePoints[j];
							if (emergeBlock.used && emergeBlock.blockPos.x == blockPos->x && emergeBlock.blockPos.y == blockPos->y) {
								found = true;
								std::sprintf(buffVal, "\nEmerge: %i, Point %i", i, j+1);
								std::strcat(buffText, buffVal);
								break;
							}
						}
						if (found) {
							break;
						}
					}
				}
			}

			// Block pointer ID:
			if (block->blockpointer != 0) {
				std::sprintf(buffVal, "\nPointer: %i", (uint32)block->blockpointer);
				std::strcat(buffText, buffVal);
			}

			// Block flags: (consider another setting to remove this verbose info)
			{
				if (block->field_44 != 0 || block->short_22 != 0) {
					std::sprintf(buffVal, "\nField44: %08x\nShort22: %04x",
									 (uint32)block->field_44,
									 (uint32)block->short_22);
				}

				std::sprintf(buffVal, "\nFlags1: %08x\nFlags2: %08x",
								(uint32)block->flags1,
								(uint32)block->flags2);
				std::strcat(buffText, buffVal);
			}
		}


		/// SANITY: Until we increase the tooltip buffer size, ensure we don't exceed 511 characters.
		if (std::strlen(buffText) >= TOOLTIP_BUFFERSIZE) {
			std::strcpy(&buffText[TOOLTIP_BUFFERSIZE-4], "...");
		}
		ToolTip_SetText(ToolTip_MapBlock, "%s", buffText);

		if (!playSound) surfSFX = SFX_NULL;
		ToolTip_SetSFX(ToolTip_MapBlock, surfSFX);
		ToolTip_AddFlag4(ToolTip_MapBlock);
		return ToolTip_MapBlock;
	}
	return ToolTip_Type_Count; // invalid
}

LegoRR::ToolTip_Type LegoRR::Lego_PrepareBlockToolTip(const Point2I* blockPos, bool32 showConstruction, bool32 playSound, bool32 showCavern)
{
	const Lego_Block* block = &blockValue(Lego_GetLevel(), blockPos->x, blockPos->y);

	if ((block->flags1 & BLOCK1_FOUNDATION) && !Level_Block_IsSolidBuilding(blockPos->x, blockPos->y, true) &&
		!Level_Block_IsPathBuilding(blockPos))
	{
		/// INLINED: (block->flags1 & BLOCK1_FOUNDATION) && !(block->flags1 & (BLOCK1_BUILDINGSOLID|BLOCK1_BUILDINGPATH)) &&
		///          !(block->flags2 & BLOCK2_TOOLSTORE)

		// Construction barrier-ed area:
		return Lego_PrepareConstructionToolTip(blockPos, showConstruction);
	}
	else {
		// Map block:
		return Lego_PrepareMapBlockToolTip(blockPos, playSound, showCavern);
	}
}

// <LegoRR.exe @00428260>
void __cdecl LegoRR::Lego_ShowBlockToolTip(const Point2I* blockPos, bool32 showConstruction, bool32 playSound, bool32 showCavern)
{
	// Not the current toolTip block, abort, and let the next update loop handle this block.
	if (s_ShowBlockToolTip_MousePos.x != blockPos->x || s_ShowBlockToolTip_MousePos.y != blockPos->y) {
		s_ShowBlockToolTip_MousePos = *blockPos;
		return;
	}

	Lego_PrepareBlockToolTip(blockPos, showConstruction, playSound, showCavern);
}


// mbx,mby : mouse-over block position.
// mouseOverObj: mouse-over object.
// <LegoRR.exe @00428810>
void __cdecl LegoRR::Lego_HandleWorldDebugKeys(sint32 mbx, sint32 mby, LegoObject* mouseOverObj, real32 noMouseButtonsElapsed)
{
	const Point2I mouseBlockPos = { mbx, mby };

	/// DEBUG KEYBIND: [A]  "Creates a landslide at mousepoint."
	// The DIRECTION_FLAG_N here is probably why the landslide debug key is so finicky.
	if (Input_IsKeyPressed(Keys::KEY_A)) {
		Fallin_Block_FUN_0040f260(&mouseBlockPos, DIRECTION_FLAG_N, true);
	}
	/// DEBUG KEYBIND: [A]  "Causes unit at mousepoint to slip."
	if (mouseOverObj != nullptr && Input_IsKeyPressed(Keys::KEY_A)) {
		LegoObject_DoSlip(mouseOverObj);
	}

	/// DEBUG KEYBIND: [End]  "Toggles power Off/On for currently selected building."
	if (Message_AnyUnitSelected() && Input_IsKeyPressed(Keys::KEY_END)) {
		StatsObject_Debug_ToggleObjectPower(Message_GetPrimarySelectedUnit());
	}

	/// DEBUG KEYBIND: [E]  "Makes a monster emerge from a diggable (valid) wall at mousepoint."
	if (Input_IsKeyPressed(Keys::KEY_E)) {
		LegoObject_TryGenerateRMonster(&legoGlobs.rockMonsterData[legoGlobs.currLevel->EmergeCreature],
									   LegoObject_RockMonster, legoGlobs.currLevel->EmergeCreature, mbx, mby);
	}

	/// DEBUG KEYBIND: [W]  "Performs unknown behaviour with the unfinished 'flood water' surface."
	if (Input_IsKeyDown(Keys::KEY_W)) {
		Level_Debug_WKey_NeedsBlockFlags1_8_FUN_004303a0(legoGlobs.currLevel, 0, mbx, mby); // 0 = unused parameter
	}

	/// DEBUG KEYBIND: [C]  "Tell selected unit carrying dynamite drop it where they are and set it off."
	if (Message_AnyUnitSelected() && Input_IsKeyPressed(Keys::KEY_C)) {
		LegoObject_Debug_DropActivateDynamite(Message_GetPrimarySelectedUnit());

		// Simply calling the same debug [C] key function again... no idea why.
		if (Message_AnyUnitSelected()) {
			LegoObject_Debug_DropActivateDynamite(Message_GetPrimarySelectedUnit());
		}
	}

	/// DEBUG KEYBIND: [F12]  "Disables all NERPs functions (toggle)."
	if (Input_IsKeyPressed(Keys::KEY_F12)) {
		if (!(legoGlobs.flags1 & GAME1_DEBUG_NONERPS)) {
			legoGlobs.flags1 |= GAME1_DEBUG_NONERPS;
			// Clear tutorial flags.
			TutorialFlags tutFlags = TUTORIAL_FLAG_NONE;
			NERPFunc__SetTutorialFlags((sint32*)&tutFlags);
		}
		else {
			legoGlobs.flags1 &= ~GAME1_DEBUG_NONERPS;
		}
	}

	/// DEBUG KEYBIND: [F11]  "Disables all building and vehicle prerequisites."
	if (Input_IsKeyPressed(Keys::KEY_F11)) {
		Dependencies_SetEnabled(!Dependencies_IsEnabled());
	}

	/// DEBUG KEYBIND: [F10]  "Inverts the direction of lighting."
	if (Input_IsKeyPressed(Keys::KEY_F10)) {
		gamectrlGlobs.dbgF10InvertLighting = !gamectrlGlobs.dbgF10InvertLighting;
		real32 dirZ;
		if (!gamectrlGlobs.dbgF10InvertLighting) {
			Gods98::Container_SetPosition(legoGlobs.spotlightTop, (legoGlobs.cameraMain)->cont3, 200.0f, 140.0f, -130.0f);
			dirZ = 0.75f;
		}
		else {
			Gods98::Container_SetPosition(legoGlobs.spotlightTop, (legoGlobs.cameraMain)->cont3, 250.0f, 190.0f, 20.0f);
			dirZ = 0.0f;
		}
		Gods98::Container_SetOrientation(legoGlobs.spotlightTop, (legoGlobs.cameraMain)->cont3, -1.0f, -0.8f, dirZ, 0.0f, 1.0f, 0.0f);
		LightEffects_InvalidatePosition();
	}

	/// DEBUG KEYBIND: [F9]  "Toggle spotlight effects."
	if (Input_IsKeyPressed(Keys::KEY_F9)) {
		gamectrlGlobs.dbgF9DisableLightEffects = !gamectrlGlobs.dbgF9DisableLightEffects;
		LightEffects_SetDisabled(gamectrlGlobs.dbgF9DisableLightEffects);
	}

	/// DEBUG KEYBIND: [5]  "Change selected unit visual upgrade parts (Carry level bit)"
	/// DEBUG KEYBIND: [6]  "Change selected unit visual upgrade parts (Scan  level bit)"
	/// DEBUG KEYBIND: [7]  "Change selected unit visual upgrade parts (Speed level bit)"
	/// DEBUG KEYBIND: [8]  "Change selected unit visual upgrade parts (Drill level bit)"
	// Evaluate if we can perform an upgrade before checking keys (for future hotkey implementation).
	if (Message_AnyUnitSelected() && 
		(Message_GetPrimarySelectedUnit()->type == LegoObject_Building ||
		 Message_GetPrimarySelectedUnit()->type == LegoObject_Vehicle))
	{
		bool hotkeyUpgCarry = Input_IsKeyPressed(Keys::KEY_FIVE);
		bool hotkeyUpgScan  = Input_IsKeyPressed(Keys::KEY_SIX);
		bool hotkeyUpgSpeed = Input_IsKeyPressed(Keys::KEY_SEVEN);
		bool hotkeyUpgDrill = Input_IsKeyPressed(Keys::KEY_EIGHT);
		if (hotkeyUpgCarry || hotkeyUpgScan || hotkeyUpgSpeed || hotkeyUpgDrill) {

			bool canUpgCarry = false, canUpgScan = false, canUpgSpeed = false, canUpgDrill = false; // dummy inits
			UpgradesModel* upgrades = nullptr; // Can't perform upgrades unless a check below for object type passes.

			LegoObject* primaryObj = Message_GetPrimarySelectedUnit();
			switch (primaryObj->type) {
			case LegoObject_Building:
				upgrades = &primaryObj->building->upgrades;

				canUpgCarry = Building_CanUpgradeType(primaryObj->building, LegoObject_UpgradeType_Carry, false);
				canUpgScan  = Building_CanUpgradeType(primaryObj->building, LegoObject_UpgradeType_Scan,  false);
				canUpgSpeed = Building_CanUpgradeType(primaryObj->building, LegoObject_UpgradeType_Speed, false);
				canUpgDrill = Building_CanUpgradeType(primaryObj->building, LegoObject_UpgradeType_Drill, false);
				break;

			case LegoObject_Vehicle:
				upgrades = &primaryObj->vehicle->upgrades;

				canUpgCarry = Vehicle_CanUpgradeType(primaryObj->vehicle, LegoObject_UpgradeType_Carry, false);
				canUpgScan  = Vehicle_CanUpgradeType(primaryObj->vehicle, LegoObject_UpgradeType_Scan,  false);
				canUpgSpeed = Vehicle_CanUpgradeType(primaryObj->vehicle, LegoObject_UpgradeType_Speed, false);
				canUpgDrill = Vehicle_CanUpgradeType(primaryObj->vehicle, LegoObject_UpgradeType_Drill, false);
				break;
			}

			if (upgrades != nullptr) {
				const LegoObject_UpgradeFlags oldUpgradeLvl = (LegoObject_UpgradeFlags)upgrades->currentLevel;
				LegoObject_UpgradeFlags newUpgradeLvl = oldUpgradeLvl;

				/// FIX APPLY: Upgrade hotkeys now check the correct 'CanUpgradeType'.
				///            Original order was reversed: canUpgDrill, canUpgSpeed, canUpgScan, canUpgCarry
				if (hotkeyUpgCarry && canUpgCarry) newUpgradeLvl ^= UPGRADE_FLAG_CARRY;
				if (hotkeyUpgScan  && canUpgScan)  newUpgradeLvl ^= UPGRADE_FLAG_SCAN;
				if (hotkeyUpgSpeed && canUpgSpeed) newUpgradeLvl ^= UPGRADE_FLAG_SPEED;
				if (hotkeyUpgDrill && canUpgDrill) newUpgradeLvl ^= UPGRADE_FLAG_DRILL;

				if (newUpgradeLvl != oldUpgradeLvl) {
					switch (primaryObj->type) {
					case LegoObject_Building:
						Building_SetUpgradeLevel(primaryObj->building, newUpgradeLvl);
						break;
					case LegoObject_Vehicle:
						Vehicle_SetUpgradeLevel(primaryObj->vehicle, newUpgradeLvl);
						break;
					}
				}
			}
		}
	}

	/// DEBUG KEYBIND: [Backspace]  ???
	if (Input_IsKeyPressed(Keys::KEY_BACKSPACE)) {
		AITask_DoGather_Count(5);
	}

	/// DEBUG KEYBIND: [Numpad Del]  "Destroys any walls at mousepoint, except border rock"
	if (Input_IsKeyPressed(Keys::KEYPAD_DELETE)) {
		Level_DestroyWall(legoGlobs.currLevel, mbx, mby, false);
	}

	/// DEBUG KEYBIND: [Numpad 3]  "Destroys connections between any walls at mousepoint, except border rock."
	if (Input_IsKeyDown(Keys::KEYPAD_3)) {
		Level_DestroyWallConnection(legoGlobs.currLevel, mbx, mby);
	}

	/// DEBUG KEYBIND: [W]  "Tell selected monster to immediately carry a created boulder."
	if (Message_AnyUnitSelected() && Message_GetPrimarySelectedUnit()->type == LegoObject_RockMonster &&
		Input_IsKeyPressed(Keys::KEY_W))
	{
		LegoObject* primaryObj = Message_GetPrimarySelectedUnit();
		sint32 nearest_bx = 0, nearest_by = 0; // dummy inits
		LegoObject_FindNearestWall(primaryObj, &nearest_bx, &nearest_by, true, false, false);
		LegoObject* boulderObj = LegoObject_Create(legoGlobs.contBoulder, LegoObject_Boulder, (LegoObject_ID)0);
		LegoObject_Hide(boulderObj, true);
		LegoObject_RoutingNoCarry_FUN_00447470(primaryObj, nearest_bx, nearest_by, boulderObj);
		Gods98::Container_Texture* contTexture = Detail_GetTexture(legoGlobs.currLevel->textureSet, blockValue(legoGlobs.currLevel, nearest_bx, nearest_by).texture);
		LegoObject_InitBoulderMesh_FUN_00440eb0(boulderObj, contTexture);
	}

	/// DEBUG KEYBIND: [N]  ???
	if (Message_AnyUnitSelected() && Input_IsKeyPressed(Keys::KEY_N)) {
		LegoObject_Route_End(Message_GetPrimarySelectedUnit(), false);
	}

	/// DEBUG KEYBIND: [LShift]+[A]  "Tells any Rock Raider to get a Sonic Blaster from Tool Store and place at mousepoint."
	if (Input_IsKeyDown(Keys::KEY_LEFTSHIFT) && Input_IsKeyPressed(Keys::KEY_A)) {
		AITask_DoBirdScarer_AtPosition(&mouseBlockPos);
	}

	/// DEBUG KEYBIND: [B]  "Pushes (bumps) any Rock Raider or Monster at mousepoint east-northeast."
	if (mouseOverObj != nullptr && Input_IsKeyPressed(Keys::KEY_B)) {
		const Point2F pushVec2D = { 2.0f, 1.0f };
		LegoObject_Push(mouseOverObj, &pushVec2D, 40.0f);
	}

	/// DEBUG KEYBIND: [H]  "Creates a spider web at mousepoint."
	if (Input_IsKeyPressed(Keys::KEY_H)) {
		SpiderWeb_SpawnAt(mbx, mby);
	}

	/// DEBUG KEYBIND: [F]  "Take 40 health points off all units selected."
	if (Message_AnyUnitSelected() && Input_IsKeyPressed(Keys::KEY_F)) {
		const uint32 numSelected = Message_GetNumSelectedUnits();
		LegoObject** selectedUnits = Message_GetSelectedUnits();
		for (uint32 i = 0; i < numSelected; i++) {
			LegoObject_AddDamage2(selectedUnits[i], 40.0f, true, noMouseButtonsElapsed);
		}
	}

	/// DEBUG KEYBIND: [H]  ???
	if (Message_AnyUnitSelected() && Message_GetPrimarySelectedUnit()->type == LegoObject_Building &&
		Input_IsKeyPressed(Keys::KEY_H))
	{
		ElectricFence_FUN_0040d420(Message_GetPrimarySelectedUnit(), 0, 0);
	}

	/// DEBUG KEYBIND: [J]  "Place an electric fence at mousepoint."
	if (Input_IsKeyPressed(Keys::KEY_J)) {
		if (!ElectricFence_Block_IsFence(mbx, mby)) {
			ElectricFence_Debug_PlaceFence(mbx, mby);
		}
		else {
			ElectricFence_Debug_RemoveFence(mbx, mby);
		}
	}

	/// DEBUG KEYBIND: [Y]  "Triggers the CrystalFound InfoMessage."
	if (Input_IsKeyPressed(Keys::KEY_Y)) {
		const Point2I infoBlockPos = { 1, 1 };
		Info_Send(Info_CrystalFound, nullptr, nullptr, &infoBlockPos);
	}


	/// DEBUG KEYBIND: (no LShift)+[U]  "Ends Advisor_Anim_Point_N."
	if (Input_IsKeyUp(Keys::KEY_LEFTSHIFT) && Input_IsKeyPressed(Keys::KEY_U)) {
		Advisor_End();
	}
	/// DEBUG KEYBIND: [LShift]+[U]  "Begins Advisor_Anim_Point_N."
	if (Input_IsKeyDown(Keys::KEY_LEFTSHIFT) && Input_IsKeyPressed(Keys::KEY_U)) {
		Panel_SetCurrentAdvisorFromButton(Panel_Radar, PanelButton_Radar_Toggle, true);
	}

	/// DEBUG KEYBIND: [K]  "Pt.1 Registers a selected vehicle as a get-in target."
	if (Message_AnyUnitSelected() && Message_GetPrimarySelectedUnit()->type == LegoObject_Vehicle &&
		Input_IsKeyPressed(Keys::KEY_K))
	{
		// Register vehicle for Get-in.
		gamectrlGlobs.dbgGetInVehicle = Message_GetPrimarySelectedUnit();
	}
	/// DEBUG KEYBIND: [K]  "Pt.2 Tells a selected minifigure to get in a registered vehicle (training not required)."
	if (Message_AnyUnitSelected() && Message_GetPrimarySelectedUnit()->type == LegoObject_MiniFigure &&
		 Input_IsKeyPressed(Keys::KEY_K))
	{
		// Tell unit to Get-in registered vehicle.
		/// FIX APPLY: Ensure our registered vehicle is valid/hasn't been teleported up/etc.
		///            Note that this is also ensured in LegoObject_Remove.
		if (!ListSet::IsNullOrDead(gamectrlGlobs.dbgGetInVehicle) &&
			gamectrlGlobs.dbgGetInVehicle->type == LegoObject_Vehicle)
		{
			LegoObject* registeredVehicle = gamectrlGlobs.dbgGetInVehicle;
			LegoObject_TryFindDriver_FUN_00440690(Message_GetPrimarySelectedUnit(), registeredVehicle);
		}
	}
}


// Move functionality from here into Lego_LoadLevel once that's implemented.
/// CUSTOM: Extended version of `Lego_LoadLevel` that also handles storing the SeeThroughWalls property.
/// CHANGE: Always allocate new memory for the level name, because its taken from various places that we can't rely on.
bool32 __cdecl LegoRR::Lego_LoadLevel2(const char* tempLevelName)
{
	/// NEW: Allocate memory for level name (because name was being taken from inconsistent sources).
	char* levelName = Gods98::Util_StrCpy(tempLevelName);

	/// NEW: Store SeeThroughWalls property so that we don't need to look it up on-demand.
	legoGlobs2.seeThroughWalls = Config_GetBoolOrTrue(legoGlobs.config, Lego_ID(levelName, "SeeThroughWalls"));

	if (!Lego_LoadLevel(levelName)) {
		/// NEW: Failed to load level, so cleanup allocated levelName string.
		Gods98::Mem_Free(levelName);
		return false;
	}

	return true;
}


// <LegoRR.exe @0042c260>
bool32 __cdecl LegoRR::Level_HandleEmergeTriggers(Lego_Level* level, const Point2I* blockPos, OUT Point2I* emergeBlockPos)
{
    /// DEBUG: (testing) Enable multiple emerges at once :)
//#define DEBUG_MULTIPLE_EMERGE


#ifdef DEBUG_MULTIPLE_EMERGE
    uint32 emergedCount = 0;
#endif

    for (uint32 i = 0; i < level->emergeCount; i++) {
        EmergeTrigger* trigger = &level->emergeTriggers[i];
        const Point2I* triggerPos = &trigger->blockPos;
        if ((trigger->blockPos.x == blockPos->x && trigger->blockPos.y == blockPos->y) &&
            trigger->timeout == 0.0f)
        {
            for (uint32 j = 0; j < EMERGE_MAXPOINTS; j++) {
                const EmergeBlock* emergePt = &trigger->emergePoints[j];
                if (emergePt->used) {

                    LegoObject* legoObj = LegoObject_TryGenerateRMonster(
                        &legoGlobs.rockMonsterData[level->EmergeCreature],
                        LegoObject_RockMonster, level->EmergeCreature,
                        emergePt->blockPos.x, emergePt->blockPos.y);
                    
                    if (legoObj != nullptr) {

                        #ifdef DEBUG_MULTIPLE_EMERGE
                        // Everything after is just setting the output parameter and returning,
                        //  which we don't need with this preprocessor after the first emerge.
                        if (emergedCount++ > 0)
                            continue;  
                        #endif

                        trigger->timeout = level->EmergeTimeOut;
                        if (emergeBlockPos != nullptr) {
                            *emergeBlockPos = emergePt->blockPos;
                        }

                        #ifndef DEBUG_MULTIPLE_EMERGE
                        return true;
                        #endif
                    }
                }
            }
        }
    }

    #ifdef DEBUG_MULTIPLE_EMERGE
    /// CUSTOM: Spawn all the things!
    return (emergedCount != 0);

    #else
    return false;

    #endif
}



// <LegoRR.exe @0042eff0>
const char* __cdecl LegoRR::Level_Free(void)
{
	Lego_Level* level = legoGlobs.currLevel;
	const char* nextLevelName = nullptr;

	if (legoGlobs.currLevel != nullptr) {
		Gods98::Mem_Free((legoGlobs.currLevel)->emergeTriggers);
		if (legoGlobs.EndGameAVI1 != nullptr) {
			Gods98::Mem_Free(legoGlobs.EndGameAVI1);
		}
		if (legoGlobs.EndGameAVI2 != nullptr) {
			Gods98::Mem_Free(legoGlobs.EndGameAVI2);
		}
		Gods98::Mem_Free(level->FullName);

		legoGlobs.flags1 &= ~(GAME1_CAMERAGOTO|GAME1_LEVELENDING);
		objectGlobs.flags &= ~LegoObject_GlobFlags::OBJECT_GLOB_FLAG_CYCLEUNITS;
		legoGlobs.flags2 &= ~(GAME2_LEVELEXITING|GAME2_NOMULTISELECT);

		legoGlobs.gotoSmooth = false;
		legoGlobs.objTeleportQueue_COUNT = 0;
		objectGlobs.minifigureObj_9cb8 = nullptr;
		objectGlobs.cycleUnitCount = 0;
		objectGlobs.cycleBuildingCount = 0;

		Lego_SetCallToArmsOn(false);

		legoGlobs.flags1 &= ~GAME1_LASERTRACKER;
		legoGlobs.flags2 &= ~(GAME2_ATTACKDEFER|GAME2_UNK_40|GAME2_UNK_80|GAME2_MENU_HASNEXT);

		legoGlobs.pointsCount2_dcc[0] = 0;
		legoGlobs.pointsCount2_dcc[1] = 0;

		Camera_Shake(legoGlobs.cameraMain, 0.0f, 0.0f);
		Camera_SetZoom(legoGlobs.cameraMain, 200.0f);

		Smoke_RemoveAll();
		Lego_ClearSomeFlags3_FUN_00435950();

		nextLevelName = level->nextLevelID;

		Text_Clear();
		HelpWindow_ClearFlag1();
		Dependencies_Reset_ClearAllLevelFlags_10c();
		NERPs_FreeBlockPointers();

		AITask_CleanupLevel(false);
		LegoObject_CleanupLevel();
		Construction_RemoveAll();
		Effect_StopAll();
		AITask_CleanupLevel(true);

		Info_SetFlag4(false);
		Roof_Shutdown();
		Camera_TrackObject(legoGlobs.cameraTrack, nullptr, 0.0f, 0.0f, 0.0f, 0.0f);
		Camera_SetFPObject(legoGlobs.cameraFP, nullptr, 0);
		Message_CleanupSelectedUnitsCount();
		NERPsFile_Free();
		RadarMap_Free_UnwindMultiUse(level->radarMap);
		Map3D_Remove(level->map);
		Lego_FreeDetailMeshes(level);
		Detail_FreeTextureSet(level->textureSet);
		Level_BlockActivity_RemoveAll(level);
		Effect_RemoveAll_BoulderExplode();

		/// NEW: Free memory allocated for level name (because name was being taken from inconsistent sources).
		Gods98::Mem_Free(level->name);

		Gods98::Mem_Free(level->blocks);
		Gods98::Mem_Free(level);

		Info_ClearAllMessages();
		Interface_ClearStates();
		Lego_SetGameSpeed(1.0f);
		Panel_ResetAll();
		Advisor_Stop();
		Gods98::Sound3D_StopAllSounds();
		DamageFont_Cleanup();
	}
	legoGlobs.currLevel = nullptr;
	return nextLevelName;
}


// <LegoRR.exe @00435870>
bool32 __cdecl LegoRR::Lego_EndLevel(void)
{
	const bool32 isMissionLevelSel = Front_IsMissionSelected();
	const bool32 isTutorialLevelSel = Front_IsTutorialSelected();

	Gods98::Sound3D_StopAllSounds();

	if (Front_IsFrontEndEnabled()) {
		Reward_CreateLevel();
		Reward_Prepare();
		Reward_Show();
		Reward_FreeLevel();
	}
	Lego_SetViewMode(ViewMode_Top, nullptr, 0);

	Gods98::TextWindow_Clear(legoGlobs.textWnd_80);

	// Default to using the next level specified in the level links. Otherwise ask the FrontEnd.
	const char* nextLevelID = Level_Free();

	if (Front_IsFrontEndEnabled()) {
		// Start FrontEnd loop to select the next level (this runs all FrontEnd menus).
		if (isMissionLevelSel) {
			Front_RunScreenMenuType(Menu_Screen_Missions);
			if (Front_IsTriggerAppQuit()) {
				return false;
			}
			nextLevelID = Front_GetSelectedLevel();
		}
		else if (isTutorialLevelSel) {
			Front_RunScreenMenuType(Menu_Screen_Training);
			if (Front_IsTriggerAppQuit()) {
				return false;
			}
			nextLevelID = Front_GetSelectedLevel();
		}
	}

	Front_LoadOptionParameters(true, false);
	if (nextLevelID == nullptr) {
		return false; // No next level selected, must have quit or something...
	}

	// Override Lego_LoadLevel to store SeeThroughWalls property.
	if (!Lego_LoadLevel2(nextLevelID)) {
		return false; // Failed to load level.
	}

	/// FIX APPLY: Don't free memory that wasn't allocated for this. BAD! NO TOUCH!
	// Another memory leak? Only freed on LoadLevel success...
	// I'm not even sure how this bit of memory is supposed to be managed.
	//Gods98::Mem_Free(nextLevelID);

	return true;
}

#pragma endregion
