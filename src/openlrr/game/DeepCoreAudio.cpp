// DeepCoreAudio.cpp : Plays the threat layer's cues.
//

#include <cstring>
#include <map>
#include <string>

#include "../engine/core/Errors.h"

#include "audio/SFX.h"
#include "Game.h"
#include "DeepCoreLogic.hpp"
#include "DeepCore.hpp"
#include "DeepCoreAudio.hpp"


#pragma region Macros

#define Audio_WarnF(b, s, ...)	Error_WarnF2( (b), "DeepCore/Audio: %s\n", Gods98::Error_Format((s), __VA_ARGS__))
#define Audio_LogF(s, ...)		Error_InfoF2("DeepCore/Audio: %s\n", Gods98::Error_Format((s), __VA_ARGS__))

#pragma endregion


namespace
{

/// Resolved SFX ids, cached per cue including failures. -1 means "looked it up, not there;
/// stop asking". Without caching the miss, a level whose config lacks these cues would
/// re-hash and re-warn on every single wave.
std::map<int, int> _resolved;


/// The configured sample name for a cue, or nullptr if the cue has no name configured.
const char* CueName(DeepCore::Logic::ThreatCue cue)
{
	using DeepCore::Logic::ThreatCue;
	const DeepCore::Settings& s = DeepCore::settings;

	switch (cue) {
	case ThreatCue::Telegraph:      return s.cueTelegraph.empty()      ? nullptr : s.cueTelegraph.c_str();
	case ThreatCue::TelegraphHeavy: return s.cueTelegraphHeavy.empty() ? nullptr : s.cueTelegraphHeavy.c_str();
	case ThreatCue::Arrival:        return s.cueArrival.empty()        ? nullptr : s.cueArrival.c_str();
	case ThreatCue::Escalate:       return s.cueEscalate.empty()       ? nullptr : s.cueEscalate.c_str();
	case ThreatCue::Cleared:        return s.cueCleared.empty()        ? nullptr : s.cueCleared.c_str();
	default:                        return nullptr;
	}
}

} // namespace


void DeepCore::Audio::InvalidateCueCache(void)
{
	_resolved.clear();
}


void DeepCore::Audio::PlayThreatCue(Logic::ThreatCue cue)
{
	using namespace LegoRR; // required: SFX_ID and friends, and the exe macros nearby

	if (cue == Logic::ThreatCue::None)  return;
	if (!DeepCore::settings.threatAudio) return;

	const char* name = CueName(cue);
	if (name == nullptr) return; // deliberately unconfigured

	const int key = (int)cue;
	auto found = _resolved.find(key);

	if (found == _resolved.end()) {
		SFX_ID sfxID = (SFX_ID)0;

		// NOT in populate mode. Registering a name here would create a cue with no samples
		// bound to it, which is worse than not resolving: it would look present and be
		// permanently silent, and it would consume one of the finite hash slots.
		if (SFX_GetType(name, &sfxID)) {
			_resolved[key] = (int)sfxID;
			if (DeepCore::settings.verboseStartup) {
				Audio_LogF("cue \"%s\" resolved to SFX id %i", name, (sint32)sfxID);
			}
		}
		else {
			_resolved[key] = -1;
			Audio_WarnF(true, "cue \"%s\" is not declared in this installation's config, so it "
				"will stay silent. Add it to the game's Samples block and drop the matching "
				"WAV alongside it. This is reported once per cue per level.", name);
		}
		found = _resolved.find(key);
	}

	if (found->second < 0) return; // known-missing

	// Non-positional on purpose. These are mission-control lines and global stings, not
	// world sounds -- a warning that pans away when the camera moves is worse than useless,
	// and the wave's LOCATION is already communicated by the Info_Send message the director
	// sends with a block position.
	SFX_Random_PlaySoundNormal((SFX_ID)found->second, false);
}


void DeepCore::Audio::RegisterCues(void)
{
	using namespace LegoRR;

	if (!DeepCore::settings.threatAudio) return;

	InvalidateCueCache();

	// Populate mode is what lets SFX_GetType CREATE a name rather than only find one
	// (SFX.cpp:125-133). It is the same switch the game's own loader uses, and it must be
	// turned back off afterwards or every later lookup of a typo'd name would silently
	// invent a cue instead of reporting it.
	SFX_SetSamplePopulateMode(true);

	const struct { Logic::ThreatCue cue; const std::string* name; } kCues[] = {
		{ Logic::ThreatCue::Telegraph,      &DeepCore::settings.cueTelegraph      },
		{ Logic::ThreatCue::TelegraphHeavy, &DeepCore::settings.cueTelegraphHeavy },
		{ Logic::ThreatCue::Arrival,        &DeepCore::settings.cueArrival        },
		{ Logic::ThreatCue::Escalate,       &DeepCore::settings.cueEscalate       },
		{ Logic::ThreatCue::Cleared,        &DeepCore::settings.cueCleared        },
	};

	sint32 registered = 0;
	for (const auto& entry : kCues) {
		if (entry.name->empty()) continue;

		SFX_ID sfxID = (SFX_ID)0;
		if (!SFX_GetType(entry.name->c_str(), &sfxID)) {
			// Only reachable when the name table is full; SFX_GetType creates otherwise.
			Audio_WarnF(true, "could not register cue \"%s\" -- the engine's sample name "
				"table is full.", entry.name->c_str());
			continue;
		}

		// SFX_LoadSampleProperty tokenises its argument IN PLACE (SFX.cpp:152), so it must
		// be handed writable storage. Passing a string literal or c_str() would be a write
		// to read-only memory.
		char value[260];
		const std::string path = DeepCore::settings.cueSampleDir.empty()
			? *entry.name
			: (DeepCore::settings.cueSampleDir + "\\" + *entry.name);

		if (path.size() + 1 > sizeof(value)) {
			Audio_WarnF(true, "cue path for \"%s\" is too long (%i chars); skipped.",
				entry.name->c_str(), (sint32)path.size());
			continue;
		}
		std::memcpy(value, path.c_str(), path.size() + 1);

		if (SFX_LoadSampleProperty(value, sfxID)) {
			registered++;
			if (DeepCore::settings.verboseStartup) {
				Audio_LogF("registered cue \"%s\" -> %s.wav (id %i)",
					entry.name->c_str(), path.c_str(), (sint32)sfxID);
			}
		}
		else {
			// The usual cause is simply that the WAV is not on disk yet. Not fatal: the cue
			// exists, has no sample bound, and PlayThreatCue will find no positive handle
			// and stay silent.
			Audio_WarnF(true, "cue \"%s\" registered but no sample loaded from \"%s.wav\" -- "
				"copy assets/audio/threat/ into your game's Data\\%s\\ directory.",
				entry.name->c_str(), path.c_str(), DeepCore::settings.cueSampleDir.c_str());
		}
	}

	SFX_SetSamplePopulateMode(false);

	if (DeepCore::settings.verboseStartup) {
		Audio_LogF("threat cue registration complete: %i cue(s)", registered);
	}
}
