// DeepCore.cpp : Settings layer and feature gates for DeepCoreOverhaul.
//

#include <cstdlib>
#include <cstring>
#include <map>

#include "../engine/core/Errors.h"
#include "../engine/gfx/Containers.h"
#include "../engine/core/Files.h"

#include "object/Creature.h"
#include "Game.h"
#include "DeepCoreLogic.hpp"
#include "DeepCoreAudio.hpp"
#include "DeepCore.hpp"


/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

// VerboseStartup diagnostics use the INFO level, not DEBUG, and that is deliberate.
// Gods98::errorLogLevels defaults debugVisible to FALSE (Errors.cpp:26), so anything
// emitted through Error_DebugF2 prints nothing unless the user also passes
// `-loglevels debug`. A diagnostic that is silent by default is not a diagnostic -- it
// is the single most likely source of a false negative when someone is trying to work
// out whether DeepCore.cfg loaded at all. infoVisible defaults TRUE.
// Note also there is no log FILE: Error_SetDumpFile (Errors.cpp:92) has no callers
// anywhere in the tree, so output goes only to the console MakeConsole() allocates.
#define DeepCore_WarnF(b, s, ...)	Error_WarnF2( (b), "DeepCore: Warning: %s\n", Gods98::Error_Format((s), __VA_ARGS__))
#define DeepCore_LogF(s, ...)		Error_InfoF2("DeepCore: %s\n", Gods98::Error_Format((s), __VA_ARGS__))

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
		|| settings.weaponBeamStyles
		|| settings.threatAudio
		|| settings.surviveWaterOverflow
		|| settings.relocateWaterTables);
}


/// Forward declarations for the wave pool, defined with PickWaveSpecies below.
static std::vector<sint32> _resolvedWaveSpecies;
static bool _waveSpeciesResolved = false;

/// Cached name->ID resolution for the emerge species pool.
/// -1 means "resolution attempted and failed"; the entry is then skipped forever
/// rather than re-warned every time a wall is drilled.
static std::vector<sint32> _resolvedSpecies;
static bool _speciesResolved = false;


void DeepCore::InvalidateSpeciesCache(void)
{
	_resolvedSpecies.clear();
	_speciesResolved = false;
	_resolvedWaveSpecies.clear();
	_waveSpeciesResolved = false;
	DeepCore::Audio::InvalidateCueCache();
}


/// Resolve every configured species name to a RockMonster type ID, once.
static void _ResolveSpeciesPool(void)
{
	// NOTE: Lego_GetObjectByName is still an address macro into the original exe
	// (Game.h:1511), not a real function. Its expansion names LegoRR types
	// UNQUALIFIED, so it only parses with LegoRR in scope -- and it cannot be
	// namespace-qualified at the call site, because macros are not namespace members.
	using namespace LegoRR;

	_speciesResolved = true;
	_resolvedSpecies.clear();

	for (const std::string& name : DeepCore::settings.emergeSpeciesNames) {

		LegoObject_Type objType = LegoObject_Type::LegoObject_None;
		LegoObject_ID objID = (LegoObject_ID)0;

		if (!Lego_GetObjectByName(name.c_str(), &objType, &objID, nullptr)) {
			DeepCore_WarnF(true, "EmergeSpeciesPool: no object named \"%s\"; skipping it.", name.c_str());
			continue;
		}

		if (objType != LegoObject_Type::LegoObject_RockMonster) {
			DeepCore_WarnF(true, "EmergeSpeciesPool: \"%s\" is not a RockMonster type; skipping it.", name.c_str());
			continue;
		}

		// Bounds check against the loaded table AND the engine's hard ID ceiling.
		// LegoObject_ID_Count is 15 and is welded to the original executable's data
		// segment layout, so an out-of-range ID here would index past the end of
		// legoGlobs.rockMonsterData and into unrelated exe memory.
		if ((uint32)objID >= legoGlobs.rockMonsterCount ||
			(uint32)objID >= (uint32)LegoObject_ID_Count)
		{
			DeepCore_WarnF(true, "EmergeSpeciesPool: \"%s\" resolved to out-of-range ID %i "
				"(loaded %i, ceiling %i); skipping it.",
				name.c_str(), (sint32)objID,
				(sint32)legoGlobs.rockMonsterCount, (sint32)LegoObject_ID_Count);
			continue;
		}

		_resolvedSpecies.push_back((sint32)objID);
	}

	if (DeepCore::settings.verboseStartup) {
		DeepCore_LogF("EmergeSpeciesPool resolved %i of %i names",
			(sint32)_resolvedSpecies.size(), (sint32)DeepCore::settings.emergeSpeciesNames.size());
	}
}


sint32 DeepCore::PickEmergeSpecies(sint32 fallbackId, uint32 triggerIndex)
{
	if (!settings.multiSpeciesEmerge || settings.emergeSpeciesNames.empty()) {
		return fallbackId;
	}

	if (!_speciesResolved) {
		_ResolveSpeciesPool();
	}

	if (_resolvedSpecies.empty()) {
		return fallbackId;
	}

	// Deterministic by trigger index: a given cavern always produces the same
	// species for the whole mission. That is a design choice, not a limitation --
	// players learn which tunnel breeds what, which is far more interesting than
	// noise, and it keeps behaviour reproducible when diagnosing a report.
	return _resolvedSpecies[triggerIndex % _resolvedSpecies.size()];
}


sint32 DeepCore::PickWaveSpecies(sint32 waveNumber, sint32 indexInWave)
{
	using namespace LegoRR; // required: Lego_GetObjectByName is an exe address macro

	if (!_waveSpeciesResolved) {
		_waveSpeciesResolved = true;
		_resolvedWaveSpecies.clear();

		for (const std::string& name : settings.waveSpeciesNames) {
			LegoObject_Type objType = LegoObject_Type::LegoObject_None;
			LegoObject_ID objID = (LegoObject_ID)0;

			if (!Lego_GetObjectByName(name.c_str(), &objType, &objID, nullptr)) {
				DeepCore_WarnF(true, "WaveSpeciesPool: no object named \"%s\"; skipping it.", name.c_str());
				continue;
			}
			if (objType != LegoObject_Type::LegoObject_RockMonster) {
				DeepCore_WarnF(true, "WaveSpeciesPool: \"%s\" is not a RockMonster type; skipping it.", name.c_str());
				continue;
			}
			if ((uint32)objID >= legoGlobs.rockMonsterCount ||
				(uint32)objID >= (uint32)LegoObject_ID_Count)
			{
				DeepCore_WarnF(true, "WaveSpeciesPool: \"%s\" resolved to out-of-range ID %i; skipping it.",
					name.c_str(), (sint32)objID);
				continue;
			}
			_resolvedWaveSpecies.push_back((sint32)objID);
		}
	}

	if (!_resolvedWaveSpecies.empty()) {
		// Vary within a wave AND across waves, so a wave reads as a mixed group rather
		// than a squad of clones -- without ever being unpredictable enough to feel random.
		const size_t n = _resolvedWaveSpecies.size();
		return _resolvedWaveSpecies[(size_t)(waveNumber + indexInWave) % n];
	}

	// Fall back to whatever this level was built to emerge, so an unconfigured director
	// still produces something the mission was designed to contain.
	if (legoGlobs.currLevel != nullptr) {
		const sint32 id = (sint32)legoGlobs.currLevel->EmergeCreature;
		if (id >= 0 && (uint32)id < legoGlobs.rockMonsterCount &&
			(uint32)id < (uint32)LegoObject_ID_Count)
		{
			return id;
		}
	}
	return -1;
}


/// Per-species spawn counters, so successive monsters of a species cycle through
/// that species' declared variants. Indexed by RockMonster type ID.
static sint32 _speciesSpawnCount[LegoRR::LegoObject_ID_Count] = {};

/// Cache for single-name lookups made by the variant table.
static std::map<std::string, sint32> _speciesIDCache;


/// Resolve one RockMonster type name to its ID, or -1. Cached, including failures,
/// so a misspelt name warns once rather than on every single spawn.
static sint32 _ResolveSpeciesID(const std::string& name)
{
	using namespace LegoRR; // required: Lego_GetObjectByName is an exe address macro

	auto found = _speciesIDCache.find(name);
	if (found != _speciesIDCache.end()) {
		return found->second;
	}

	sint32 result = -1;

	LegoObject_Type objType = LegoObject_Type::LegoObject_None;
	LegoObject_ID objID = (LegoObject_ID)0;

	if (!Lego_GetObjectByName(name.c_str(), &objType, &objID, nullptr)) {
		DeepCore_WarnF(true, "Variants: no object named \"%s\".", name.c_str());
	}
	else if (objType != LegoObject_Type::LegoObject_RockMonster) {
		DeepCore_WarnF(true, "Variants: \"%s\" is not a RockMonster type.", name.c_str());
	}
	else if ((uint32)objID >= legoGlobs.rockMonsterCount ||
			 (uint32)objID >= (uint32)LegoObject_ID_Count) {
		DeepCore_WarnF(true, "Variants: \"%s\" resolved to out-of-range ID %i.", name.c_str(), (sint32)objID);
	}
	else {
		result = (sint32)objID;
	}

	_speciesIDCache[name] = result;
	return result;
}


void DeepCore::ApplyCreatureVariant(void* creatureModel, sint32 objID)
{
	if (!settings.creatureVariants || settings.variants.empty() || creatureModel == nullptr) {
		return;
	}
	if (objID < 0 || objID >= (sint32)LegoRR::LegoObject_ID_Count) {
		return;
	}

	if (!_speciesResolved) {
		_ResolveSpeciesPool();
	}

	// Collect the variants declared for this species. Resolution is by name, which
	// is why it happens here rather than at parse time -- the object tables are not
	// populated until the game has loaded them.
	std::vector<const Settings::Variant*> matching;
	for (const Settings::Variant& v : settings.variants) {
		if (_ResolveSpeciesID(v.speciesName) == objID) {
			matching.push_back(&v);
		}
	}
	if (matching.empty()) {
		return;
	}

	const sint32 n = _speciesSpawnCount[objID]++;
	const Settings::Variant* variant = matching[(size_t)n % matching.size()];

	Gods98::Container* cont = LegoRR::Creature_GetActivityContainer((LegoRR::CreatureModel*)creatureModel);
	if (cont == nullptr) {
		return;
	}

	if (variant->scale != 1.0f) {
		// Combine::After multiplies onto whatever the model already carries, rather
		// than discarding the .ae file's own SCALE property.
		Gods98::Container_AddScale(cont, Gods98::Container_Combine::After,
			variant->scale, variant->scale, variant->scale);
	}

	if (variant->r != 1.0f || variant->g != 1.0f || variant->b != 1.0f) {
		Gods98::Container_SetColour(cont, variant->r, variant->g, variant->b);
	}

	if (settings.verboseStartup) {
		DeepCore_LogF("variant \"%s\" applied to species ID %i (instance %i)",
			variant->label.c_str(), objID, n);
	}
}


/// Thin adapter over DeepCore::Logic::SplitFields.
///
/// The splitting itself lives in DeepCoreLogic.hpp so it can be exercised by
/// tools/harness/ without the game. This wrapper exists only so the call sites below
/// read the same as they always did.
static void _SplitFields(const char* value, std::vector<std::string>& out)
{
	DeepCore::Logic::SplitFields(value, out);
}


/// weaponID -> index into settings.beamStyles, or -1 for "no style". Resolved lazily
/// because weaponGlobs.weaponNameList does not exist until Weapon_Initialise has run.
static std::map<sint32, sint32> _beamStyleByWeapon;


const DeepCore::Settings::BeamStyle* DeepCore::GetBeamStyle(sint32 weaponID)
{
	if (!settings.weaponBeamStyles || settings.beamStyles.empty() || weaponID < 0) {
		return nullptr;
	}
	if ((uint32)weaponID >= LegoRR::weaponGlobs.weaponCount) {
		return nullptr; // never index the name list out of range
	}

	auto cached = _beamStyleByWeapon.find(weaponID);
	if (cached == _beamStyleByWeapon.end()) {
		sint32 found = -1;
		const char* name = LegoRR::weaponGlobs.weaponNameList
			? LegoRR::weaponGlobs.weaponNameList[weaponID] : nullptr;

		if (name != nullptr) {
			for (size_t i = 0; i < settings.beamStyles.size(); i++) {
				// Config identifiers are case-insensitive throughout this engine.
				if (::_stricmp(settings.beamStyles[i].weaponName.c_str(), name) == 0) {
					found = (sint32)i;
					break;
				}
			}
		}
		_beamStyleByWeapon[weaponID] = found;
		cached = _beamStyleByWeapon.find(weaponID);

		if (settings.verboseStartup) {
			DeepCore_LogF("beam style for weapon %i (\"%s\"): %s",
				weaponID, (name ? name : "?"), (found >= 0 ? "matched" : "stock"));
		}
	}

	return (cached->second >= 0) ? &settings.beamStyles[cached->second] : nullptr;
}


/// Distinct overflow kinds already reported, so each warns exactly once per run.
static std::map<std::string, bool> _waterOverflowWarned;


bool DeepCore::WaterOverflow(const char* what)
{
	if (!settings.surviveWaterOverflow) {
		return false; // fall through to the original Error_Fatal
	}

	const std::string key = (what != nullptr ? what : "?");
	if (!_waterOverflowWarned[key]) {
		_waterOverflowWarned[key] = true;
		DeepCore_WarnF(true, "map exceeds the engine's fixed limit for %s. The excess is being "
			"skipped so the level can still load; some water will not be simulated. This limit "
			"lives in the original executable's data layout and cannot be raised.", key.c_str());
	}
	return true;
}


/// Whether the declined-debug-key notice has already been printed this run.
static bool _debugWaterKeyWarned = false;


void DeepCore::WarnOnce_DebugWaterKeyDisabled(void)
{
	if (!settings.relocateWaterTables || _debugWaterKeyWarned) {
		return;
	}
	_debugWaterKeyWarned = true;
	DeepCore_WarnF(true, "%s", "the [W] debug keybind is disabled while RelocateWaterTables is on. "
		"That key calls straight into original 1999 machine code which expects to find a water pool "
		"inside the executable's own data segment, and with the tables relocated there is no such "
		"pool. What that code does was never worked out, so it is declined rather than guessed at.");
}


/// Apply a numeric config value only when the key is actually present, so a partial
/// config file cannot silently zero a tuning value. `mustBePositive` rejects <= 0 and
/// keeps the default, with a warning naming both the bad value and what was kept.
static void _ReadRealIfPresent(const Gods98::Config* config, const char* key, real32& dest, bool mustBePositive)
{
	const char* id = Config_ID(LegoRR::legoGlobs.gameName, DEEPCORE_BLOCKNAME, key);
	if (Gods98::Config_FindItem(config, id) == nullptr) return;

	const real32 value = Config_GetRealValue(config, id);
	if (mustBePositive && value <= 0.0f) {
		DeepCore_WarnF(true, "%s must be > 0 (got %f); keeping default %f", key, value, dest);
		return;
	}
	if (value < 0.0f) {
		DeepCore_WarnF(true, "%s cannot be negative (got %f); keeping default %f", key, value, dest);
		return;
	}
	dest = value;
}

/// Apply a string config value only when the key is present. An explicitly empty value is
/// meaningful -- it disables one cue without disabling the whole layer -- so emptiness is
/// preserved rather than treated as absence.
static void _ReadStringIfPresent(const Gods98::Config* config, const char* key, std::string& dest)
{
	const char* id = Config_ID(LegoRR::legoGlobs.gameName, DEEPCORE_BLOCKNAME, key);
	if (Gods98::Config_FindItem(config, id) == nullptr) return;
	const char* value = Gods98::Config_GetTempStringValue(config, id);
	dest = (value != nullptr) ? value : "";
}


static void _ReadIntIfPresent(const Gods98::Config* config, const char* key, sint32& dest, bool mustBePositive)
{
	const char* id = Config_ID(LegoRR::legoGlobs.gameName, DEEPCORE_BLOCKNAME, key);
	if (Gods98::Config_FindItem(config, id) == nullptr) return;

	const sint32 value = Config_GetIntValue(config, id);
	if (mustBePositive && value <= 0) {
		DeepCore_WarnF(true, "%s must be > 0 (got %i); keeping default %i", key, value, dest);
		return;
	}
	if (value < 0) {
		DeepCore_WarnF(true, "%s cannot be negative (got %i); keeping default %i", key, value, dest);
		return;
	}
	dest = value;
}


bool DeepCore::Load(void)
{
	InvalidateSpeciesCache();
	_waterOverflowWarned.clear();
	_debugWaterKeyWarned = false;
	for (sint32 i = 0; i < (sint32)LegoRR::LegoObject_ID_Count; i++) {
		_speciesSpawnCount[i] = 0;
	}

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

	{
		const char* wpool = Gods98::Config_GetTempStringValue(config, DeepCore_ID("WaveSpeciesPool"));
		_SplitFields(wpool, settings.waveSpeciesNames);
	}

	_ReadRealIfPresent(config, "WaveRampSeconds",        settings.waveRampSeconds,        false);
	_ReadRealIfPresent(config, "WaveMinIntervalSeconds", settings.waveMinIntervalSeconds, true);
	_ReadRealIfPresent(config, "WaveTelegraphSeconds",   settings.waveTelegraphSeconds,   false);
	_ReadRealIfPresent(config, "WaveShakeIntensity",     settings.waveShakeIntensity,     false);
	_ReadRealIfPresent(config, "WaveShakeDuration",      settings.waveShakeDuration,      false);
	_ReadIntIfPresent (config, "WaveSize",               settings.waveSize,               true);
	_ReadIntIfPresent (config, "WaveSizeMax",            settings.waveSizeMax,            true);
	_ReadIntIfPresent (config, "WaveMinDistanceFromBase",settings.waveMinDistanceFromBase,false);

	// ---- Threat audio ---------------------------------------------------------

	settings.threatAudio = Config_GetBoolOrFalse(config, DeepCore_ID("ThreatAudio"));
	_ReadIntIfPresent(config, "ThreatHeavyWaveSize",       settings.threatHeavyWaveSize,       true);
	_ReadIntIfPresent(config, "ThreatEscalateEveryNWaves", settings.threatEscalateEveryNWaves, false);
	_ReadStringIfPresent(config, "CueTelegraph",      settings.cueTelegraph);
	_ReadStringIfPresent(config, "CueTelegraphHeavy", settings.cueTelegraphHeavy);
	_ReadStringIfPresent(config, "CueArrival",        settings.cueArrival);
	_ReadStringIfPresent(config, "CueEscalate",       settings.cueEscalate);
	_ReadStringIfPresent(config, "CueCleared",        settings.cueCleared);
	_ReadStringIfPresent(config, "CueSampleDir",      settings.cueSampleDir);

	// Species pool: whitespace- or comma-separated monster type names.
	{
		const char* pool = Gods98::Config_GetTempStringValue(config, DeepCore_ID("EmergeSpeciesPool"));
		_SplitFields(pool, settings.emergeSpeciesNames);

		if (settings.multiSpeciesEmerge && settings.emergeSpeciesNames.empty()) {
			DeepCore_WarnF(true, "%s", "MultiSpeciesEmerge is TRUE but EmergeSpeciesPool is empty; "
				"emerges will behave exactly as vanilla. List monster type names from your Lego.cfg "
				"RockMonsterTypes block to enable it.");
		}
	}

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

	// Variants block: "<label>  <SpeciesName> <scale> <r>:<g>:<b>", one per line.
	// Parsed with the same Config_FindArray/Config_GetNextItem walk that Shortcuts
	// uses for its KeyBinds block.
	for (const Gods98::Config* prop = Gods98::Config_FindArray(config, DeepCore_ID("Variants"));
		 prop != nullptr;
		 prop = Gods98::Config_GetNextItem(prop))
	{
		const char* label = Gods98::Config_GetItemName(prop);
		const char* value = Gods98::Config_GetDataString(prop);
		if (label == nullptr || value == nullptr) continue;

		// Split on whitespace and ':' so both "1.5 0.6:0.3:0.3" and
		// "1.5 0.6 0.3 0.3" parse identically.
		std::vector<std::string> fields;
		_SplitFields(value, fields);

		if (fields.size() < 2) {
			DeepCore_WarnF(true, "Variants: \"%s\" needs at least a species name and a scale; skipping.", label);
			continue;
		}

		Settings::Variant variant;
		variant.label       = label;
		variant.speciesName = fields[0];
		variant.scale       = (real32)std::atof(fields[1].c_str());

		if (variant.scale <= 0.0f) {
			DeepCore_WarnF(true, "Variants: \"%s\" has non-positive scale %f; skipping.", label, variant.scale);
			continue;
		}

		if (fields.size() >= 5) {
			variant.r = (real32)std::atof(fields[2].c_str());
			variant.g = (real32)std::atof(fields[3].c_str());
			variant.b = (real32)std::atof(fields[4].c_str());
		}
		else if (fields.size() != 2) {
			DeepCore_WarnF(true, "Variants: \"%s\" has %i fields; expected 2 (species scale) or 5 "
				"(species scale r g b). Tint ignored.", label, (sint32)fields.size());
		}

		settings.variants.push_back(variant);
	}

	if (settings.creatureVariants && settings.variants.empty()) {
		DeepCore_WarnF(true, "%s", "CreatureVariants is TRUE but the Variants block is empty; "
			"creatures will look exactly as vanilla.");
	}

	// ---- Weapon beam appearance -----------------------------------------------

	settings.weaponBeamStyles = Config_GetBoolOrFalse(config, DeepCore_ID("WeaponBeamStyles"));

	// "<Label>  <WeaponName> <innerThick>:<r>:<g>:<b>:<a> <outerThick>:<r>:<g>:<b>:<a> [frames]"
	for (const Gods98::Config* prop = Gods98::Config_FindArray(config, DeepCore_ID("BeamStyles"));
		 prop != nullptr;
		 prop = Gods98::Config_GetNextItem(prop))
	{
		const char* label = Gods98::Config_GetItemName(prop);
		const char* value = Gods98::Config_GetDataString(prop);
		if (label == nullptr || value == nullptr) continue;

		std::vector<std::string> f;
		_SplitFields(value, f);

		if (f.size() < 11) {
			DeepCore_WarnF(true, "BeamStyles: \"%s\" has %i fields; needs at least 11 "
				"(weapon, then 5 inner, then 5 outer). Skipped.", label, (sint32)f.size());
			continue;
		}

		Settings::BeamStyle style;
		style.weaponName    = f[0];
		style.innerThickness= (real32)std::atof(f[1].c_str());
		style.innerR        = (real32)std::atof(f[2].c_str());
		style.innerG        = (real32)std::atof(f[3].c_str());
		style.innerB        = (real32)std::atof(f[4].c_str());
		style.innerA        = (real32)std::atof(f[5].c_str());
		style.outerThickness= (real32)std::atof(f[6].c_str());
		style.outerR        = (real32)std::atof(f[7].c_str());
		style.outerG        = (real32)std::atof(f[8].c_str());
		style.outerB        = (real32)std::atof(f[9].c_str());
		style.outerA        = (real32)std::atof(f[10].c_str());
		if (f.size() >= 12) {
			style.lifetimeFrames = (real32)std::atof(f[11].c_str());
		}

		if (style.innerThickness <= 0.0f || style.outerThickness <= 0.0f || style.lifetimeFrames <= 0.0f) {
			DeepCore_WarnF(true, "BeamStyles: \"%s\" has a non-positive thickness or lifetime; skipped.", label);
			continue;
		}

		settings.beamStyles.push_back(style);
	}

	if (settings.weaponBeamStyles && settings.beamStyles.empty()) {
		DeepCore_WarnF(true, "%s", "WeaponBeamStyles is TRUE but the BeamStyles block is empty; "
			"every laser will look stock.");
	}

	// ---- Stability ------------------------------------------------------------

	settings.surviveWaterOverflow = Config_GetBoolOrFalse(config, DeepCore_ID("SurviveWaterOverflow"));
	settings.relocateWaterTables  = Config_GetBoolOrFalse(config, DeepCore_ID("RelocateWaterTables"));

	// Sanity ceilings for the relocated tables. Only overridden when the key is
	// actually present, so a partial config file cannot silently zero them.
	if (Gods98::Config_FindItem(config, DeepCore_ID("WaterMaxPools")) != nullptr) {
		const sint32 value = Config_GetIntValue(config, DeepCore_ID("WaterMaxPools"));
		if (value > 0) {
			settings.waterMaxPools = (uint32)value;
		}
		else {
			DeepCore_WarnF(true, "WaterMaxPools must be > 0 (got %i), keeping default %i",
				value, (sint32)settings.waterMaxPools);
		}
	}

	if (Gods98::Config_FindItem(config, DeepCore_ID("WaterMaxPoolBlocks")) != nullptr) {
		const sint32 value = Config_GetIntValue(config, DeepCore_ID("WaterMaxPoolBlocks"));
		if (value > 0) {
			settings.waterMaxPoolBlocks = (uint32)value;
		}
		else {
			DeepCore_WarnF(true, "WaterMaxPoolBlocks must be > 0 (got %i), keeping default %i",
				value, (sint32)settings.waterMaxPoolBlocks);
		}
	}

	Gods98::Config_Free(config);

	if (settings.verboseStartup) {
		DeepCore_LogF("%s", "settings loaded from " DEEPCORE_FILENAME);
		DeepCore_LogF("  MultiSpeciesEmerge  = %s", settings.multiSpeciesEmerge ? "true" : "false");
		DeepCore_LogF("  WaveDirector        = %s", settings.waveDirector ? "true" : "false");
		DeepCore_LogF("  WaveIntervalSeconds = %f", settings.waveIntervalSeconds);
		DeepCore_LogF("  WaveMaxAlive        = %i", settings.waveMaxAlive);
		DeepCore_LogF("  CreatureVariants    = %s", settings.creatureVariants ? "true" : "false");
		DeepCore_LogF("  SurviveWaterOverflow= %s", settings.surviveWaterOverflow ? "true" : "false");
		DeepCore_LogF("  RelocateWaterTables = %s", settings.relocateWaterTables ? "true" : "false");
		DeepCore_LogF("  WaterMaxPools       = %i", (sint32)settings.waterMaxPools);
		DeepCore_LogF("  WaterMaxPoolBlocks  = %i", (sint32)settings.waterMaxPoolBlocks);
	}

	return true;
}

#pragma endregion
