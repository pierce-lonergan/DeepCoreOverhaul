// DeepCore.cpp : Settings layer and feature gates for DeepCoreOverhaul.
//

#include "../engine/core/Errors.h"
#include "../engine/core/Files.h"

#include "Game.h"
#include "DeepCore.hpp"


/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define DeepCore_WarnF(b, s, ...)	Error_WarnF2( (b), "DeepCore: Warning: %s\n", Gods98::Error_Format((s), __VA_ARGS__))
#define DeepCore_LogF(s, ...)		Error_DebugF2("DeepCore: %s\n", Gods98::Error_Format((s), __VA_ARGS__))

// Config keys live at "<gameName>::DeepCore::<name>", mirroring how Shortcuts.cfg
// nests its KeyBinds block under the game name.
#define DeepCore_ID(name)			Config_ID(LegoRR::legoGlobs.gameName, DEEPCORE_BLOCKNAME, name)

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

DeepCore::Settings DeepCore::settings = DeepCore::Settings{};

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

void DeepCore::Reset(void)
{
	settings = Settings{};
}


bool DeepCore::IsAnyFeatureEnabled(void)
{
	return (settings.multiSpeciesEmerge
		|| settings.waveDirector
		|| settings.creatureVariants
		|| settings.unlimitWater);
}


bool DeepCore::Load(void)
{
	Reset();

	// Load using exeDir, and never from the CD. Same flags as Shortcuts.
	Gods98::Config* config = Gods98::Config_Load2(DEEPCORE_FILENAME,
		Gods98::FileFlags::FILE_FLAG_EXEDIR|Gods98::FileFlags::FILE_FLAG_NOCD);

	if (config == nullptr) {
		// Not an error. No config means "behave exactly like upstream OpenLRR",
		// which is the correct outcome for anyone who just wants the base project.
		return false;
	}

	// ---- Diagnostics ----------------------------------------------------------

	settings.verboseStartup   = Config_GetBoolOrFalse(config, DeepCore_ID("VerboseStartup"));

	// ---- Monster density ------------------------------------------------------

	settings.multiSpeciesEmerge = Config_GetBoolOrFalse(config, DeepCore_ID("MultiSpeciesEmerge"));
	settings.waveDirector       = Config_GetBoolOrFalse(config, DeepCore_ID("WaveDirector"));

	// Only override the numeric defaults when the key is actually present, so a
	// partial config file cannot silently zero a tuning value.
	if (Gods98::Config_FindItem(config, DeepCore_ID("WaveIntervalSeconds")) != nullptr) {
		const real32 value = Config_GetRealValue(config, DeepCore_ID("WaveIntervalSeconds"));
		if (value > 0.0f) {
			settings.waveIntervalSeconds = value;
		}
		else {
			DeepCore_WarnF(true, "WaveIntervalSeconds must be > 0 (got %f), keeping default %f",
				value, settings.waveIntervalSeconds);
		}
	}

	if (Gods98::Config_FindItem(config, DeepCore_ID("WaveMaxAlive")) != nullptr) {
		const sint32 value = Config_GetIntValue(config, DeepCore_ID("WaveMaxAlive"));
		if (value > 0) {
			settings.waveMaxAlive = value;
		}
		else {
			DeepCore_WarnF(true, "WaveMaxAlive must be > 0 (got %i), keeping default %i",
				value, settings.waveMaxAlive);
		}
	}

	// ---- Creature variants ----------------------------------------------------

	settings.creatureVariants = Config_GetBoolOrFalse(config, DeepCore_ID("CreatureVariants"));

	// ---- Stability ------------------------------------------------------------

	settings.unlimitWater     = Config_GetBoolOrFalse(config, DeepCore_ID("UnlimitWater"));

	Gods98::Config_Free(config);

	if (settings.verboseStartup) {
		DeepCore_LogF("%s", "settings loaded from " DEEPCORE_FILENAME);
		DeepCore_LogF("  MultiSpeciesEmerge  = %s", settings.multiSpeciesEmerge ? "true" : "false");
		DeepCore_LogF("  WaveDirector        = %s", settings.waveDirector ? "true" : "false");
		DeepCore_LogF("  WaveIntervalSeconds = %f", settings.waveIntervalSeconds);
		DeepCore_LogF("  WaveMaxAlive        = %i", settings.waveMaxAlive);
		DeepCore_LogF("  CreatureVariants    = %s", settings.creatureVariants ? "true" : "false");
		DeepCore_LogF("  UnlimitWater        = %s", settings.unlimitWater ? "true" : "false");
	}

	return true;
}

#pragma endregion
