// DeepCore.cpp : Settings layer and feature gates for DeepCoreOverhaul.
//

#include <cstdlib>
#include <map>

#include "../engine/core/Errors.h"
#include "../engine/gfx/Containers.h"
#include "../engine/core/Files.h"

#include "object/Creature.h"
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
		|| settings.surviveWaterOverflow);
}


/// Cached name->ID resolution for the emerge species pool.
/// -1 means "resolution attempted and failed"; the entry is then skipped forever
/// rather than re-warned every time a wall is drilled.
static std::vector<sint32> _resolvedSpecies;
static bool _speciesResolved = false;


void DeepCore::InvalidateSpeciesCache(void)
{
	_resolvedSpecies.clear();
	_speciesResolved = false;
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


bool DeepCore::Load(void)
{
	InvalidateSpeciesCache();
	_waterOverflowWarned.clear();
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

	// Species pool: whitespace- or comma-separated monster type names.
	{
		const char* pool = Gods98::Config_GetTempStringValue(config, DeepCore_ID("EmergeSpeciesPool"));
		if (pool != nullptr) {
			std::string current;
			for (const char* c = pool; ; c++) {
				if (*c == '\0' || *c == ' ' || *c == '\t' || *c == ',' || *c == ':') {
					if (!current.empty()) {
						settings.emergeSpeciesNames.push_back(current);
						current.clear();
					}
					if (*c == '\0') break;
				}
				else {
					current.push_back(*c);
				}
			}
		}

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
		{
			std::string current;
			for (const char* c = value; ; c++) {
				if (*c == '\0' || *c == ' ' || *c == '\t' || *c == ':' || *c == ',') {
					if (!current.empty()) { fields.push_back(current); current.clear(); }
					if (*c == '\0') break;
				}
				else current.push_back(*c);
			}
		}

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

	// ---- Stability ------------------------------------------------------------

	settings.surviveWaterOverflow = Config_GetBoolOrFalse(config, DeepCore_ID("SurviveWaterOverflow"));

	Gods98::Config_Free(config);

	if (settings.verboseStartup) {
		DeepCore_LogF("%s", "settings loaded from " DEEPCORE_FILENAME);
		DeepCore_LogF("  MultiSpeciesEmerge  = %s", settings.multiSpeciesEmerge ? "true" : "false");
		DeepCore_LogF("  WaveDirector        = %s", settings.waveDirector ? "true" : "false");
		DeepCore_LogF("  WaveIntervalSeconds = %f", settings.waveIntervalSeconds);
		DeepCore_LogF("  WaveMaxAlive        = %i", settings.waveMaxAlive);
		DeepCore_LogF("  CreatureVariants    = %s", settings.creatureVariants ? "true" : "false");
		DeepCore_LogF("  SurviveWaterOverflow= %s", settings.surviveWaterOverflow ? "true" : "false");
	}

	return true;
}

#pragma endregion
