// DeepCore.hpp : Settings layer and feature gates for the DeepCoreOverhaul fan project.
//
// Every overhaul feature is gated here and DEFAULTS TO VANILLA BEHAVIOUR (disabled).
// This is deliberate and load-bearing:
//   1. A user who drops the DLL in without a DeepCore.cfg gets stock OpenLRR, not a
//      silently-altered game.
//   2. Each feature can be bisected independently when something misbehaves.
//   3. The project cannot currently run the game during development, so "off unless
//      asked for" is the only honest default.
//
// Modelled on Shortcuts.hpp/.cpp, which is the sanctioned in-tree pattern for OpenLRR
// -owned configuration that lives outside the original game's Lego.cfg (see
// Shortcuts.cpp Load(), which uses Config_Load2 with FILE_FLAG_EXEDIR|FILE_FLAG_NOCD).
//
// *** THE ONE RULE ***
// Nothing in DeepCore may change the layout of any struct carrying assert_sizeof().
// Those structs are references overlaid onto the original 1999 executable's data
// segment at fixed addresses (e.g. Stats.cpp:41 binds statsGlobs to 0x00503bd8, and
// Stats_Globs ends 8 bytes before textGlobs at 0x00504190). Growing one corrupts the
// next. When more storage is needed, add DLL-side storage instead -- the pattern
// already proven in-tree by the PowerGrid std::vector replacements at Game.cpp:168-170.
//

#pragma once

#include <string>
#include <vector>

#include "../engine/core/Config.h"


namespace DeepCore
{; // !<---

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

// Loaded relative to the exeDir, alongside Settings\Shortcuts.cfg.
#define DEEPCORE_FILENAME		"Settings\\DeepCore.cfg"

// Config block name nested under the game name, i.e. "Lego*::DeepCore::<key>".
#define DEEPCORE_BLOCKNAME		"DeepCore"

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

/// Feature gates and tuning. All bools default false == vanilla behaviour.
struct Settings
{
	// ---- Diagnostics ----------------------------------------------------------

	/// Log every setting as resolved, with its source (file value vs default).
	/// Cheap way to confirm the config was found and parsed as intended.
	bool verboseStartup = false;


	// ---- Monster density ------------------------------------------------------
	// Vanilla ships ONE monster species per mission: Lego_Level::EmergeCreature is a
	// single LegoObject_ID (Game.h:434). Eleven species exist in the RockMonster name
	// space (GameCommon.h:137-150) but a given level can only ever emerge one of them.
	// Making species a per-TRIGGER property rather than a per-LEVEL property raises
	// perceived variety without consuming any of the 15 object-ID slots and without
	// requiring a single new model.

	/// Allow different emerge triggers within one level to spawn different species.
	bool multiSpeciesEmerge = false;

	/// Monster type names eligible for multi-species emerges, in rotation order.
	///
	/// Deliberately EMPTY by default, which makes multiSpeciesEmerge a no-op. The
	/// names that actually exist live in the user's own Lego.cfg RockMonsterTypes
	/// block, which this project has never been able to read (no game installation
	/// on the authoring machine). Guessing them here and silently falling back would
	/// hide typos; requiring the user to name them means an unresolvable name gets
	/// reported instead. Vanilla names are listed in DeepCore.cfg as a starting point.
	std::vector<std::string> emergeSpeciesNames;

	/// Enable the DLL-side spawn director (waves independent of map emerge triggers).
	bool waveDirector = false;

	/// Seconds between wave-director evaluations. Ignored unless waveDirector.
	real32 waveIntervalSeconds = 90.0f;

	/// Hard ceiling on creatures the director will keep alive at once. This is a
	/// director-side budget, NOT an engine limit; it exists so a runaway config
	/// cannot flood the level.
	sint32 waveMaxAlive = 6;


	// ---- Creature variants ----------------------------------------------------
	// Uniform model scale is a data-driven property and per-group diffuse/emissive
	// colour is implemented in Containers. The base game already recolours crystals
	// by level this way. So a 1.6x dark-red brute and a 0.6x pale sprite can be the
	// same mesh -- apparent roster growth with zero new art and zero new object IDs.

	/// Apply the cosmetic variant table to spawned creatures.
	bool creatureVariants = false;

	/// One cosmetic variant: a scale and a tint applied to a creature at spawn.
	struct Variant
	{
		std::string label;			// for diagnostics only
		std::string speciesName;	// which RockMonster type this applies to
		real32 scale = 1.0f;		// uniform model scale multiplier
		real32 r = 1.0f;			// diffuse tint, 1:1:1 == untinted
		real32 g = 1.0f;
		real32 b = 1.0f;
	};

	/// Variants in declaration order. Instances of a species cycle through the
	/// variants declared for that species, so the Nth monster of a species gets
	/// variant N % count. Cycling rather than rolling dice is deliberate: it makes
	/// a bug report reproducible, and guarantees the player actually sees every
	/// variant instead of hitting an unlucky streak.
	std::vector<Variant> variants;


	// ---- Stability ------------------------------------------------------------
	// Water_Update calls Error_Fatal -- outright process termination -- when a map
	// exceeds WATER_MAXPOOLS (10) or WATER_MAXPOOLBLOCKS (100) (Water.h:26-28,
	// Water.cpp:54/186/255/671/689/702). Community maps have been dying on this for
	// twenty-five years. Water_Globs is exe-overlaid (Water.cpp:18, assert_sizeof
	// 0x29ec) so the arrays CANNOT be grown; the fix is DLL-side storage.

	/// Replace the fixed water pool/block tables with unbounded DLL-side storage.
	bool unlimitWater = false;
};

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

/// Resolved settings. Valid after Load(); safe to read before (all-defaults).
extern Settings settings;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

/// Load Settings\DeepCore.cfg from the exe directory and populate `settings`.
/// A missing file is NOT an error -- defaults are kept and the game runs as stock
/// OpenLRR. Returns true if a config file was found and read.
bool Load(void);

/// Reset `settings` to defaults (vanilla behaviour). Called by Load() before parsing.
void Reset(void);

/// True if any overhaul feature is enabled. Used to decide whether to announce the
/// overhaul at startup, so a stock run stays visually identical to upstream.
bool IsAnyFeatureEnabled(void);

/// Choose which RockMonster type ID a given emerge trigger should spawn.
///
/// Returns `fallbackId` unchanged -- i.e. exact vanilla behaviour -- whenever the
/// feature is off, the species pool is empty, or nothing in the pool resolved. The
/// caller can therefore use the result unconditionally.
///
/// `fallbackId` is the level's own Lego_Level::EmergeCreature. `triggerIndex` is the
/// index of the trigger within Lego_Level::emergeTriggers, so a given trigger always
/// yields the same species for the whole mission rather than shuffling per wall hit.
sint32 PickEmergeSpecies(sint32 fallbackId, uint32 triggerIndex);

/// Drop cached name->ID resolutions. Call when the game's object tables may have been
/// reloaded, so a stale ID can never be handed to the spawner.
void InvalidateSpeciesCache(void);

/// Apply a cosmetic variant (scale + tint) to a freshly created creature.
///
/// `creatureModel` is the per-instance CreatureModel clone owned by the live object,
/// passed as void* so this header need not pull in the LegoRR object headers.
/// `objID` is the creature's RockMonster type ID. Does nothing when the feature is
/// off or no variant is declared for that species, so it is safe to call always.
void ApplyCreatureVariant(void* creatureModel, sint32 objID);

#pragma endregion

}
