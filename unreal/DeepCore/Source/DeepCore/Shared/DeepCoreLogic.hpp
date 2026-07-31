// DeepCoreLogic.hpp : The parts of DeepCore that are pure logic.
//
// WHY THIS FILE EXISTS
// --------------------
// This project cannot run the game. Everything else in game/ is entangled with globals
// that are C++ references overlaid onto the original executable's data segment -- reading
// one outside the injected process reads unmapped memory. So none of it can be exercised
// by a test.
//
// This header is the exception, and it is deliberately kept that way. It depends on
// nothing but the C++ standard library: no Gods98, no LegoRR, no Windows, no d3drm, no
// exe address macros. That makes it linkable into tools/harness/, a standalone x86 console
// binary that runs on a machine with no copy of the game -- which is the closest thing to
// running that this project has.
//
// THE RULE FOR THIS FILE
// ----------------------
// If a function here ever needs a LegoRR or Gods98 type, it does not belong here. Move the
// engine-touching part to the caller and keep the decision itself pure. Every arithmetic
// decision the overhaul makes -- when a wave fires, how big it is, whether a config value
// is sane, how a config line splits -- should live here where it can be tested, and the
// engine-facing code should only apply the answer.
//
// Types are plain float/int rather than the engine's real32/sint32, because common.h pulls
// in Windows headers and that would defeat the entire purpose.
//

#pragma once

#include <string>
#include <vector>


namespace DeepCore
{; // !<---

namespace Logic
{; // !<---

/**********************************************************************************
 ******** Config value parsing
 **********************************************************************************/

#pragma region Parsing

/// Split a config value into fields on whitespace, ':' and ','.
///
/// One splitter for every list-shaped value in DeepCore.cfg, so "1.5 0.6:0.3:0.3" and
/// "1.5 0.6 0.3 0.3" always mean the same thing regardless of which key they appear under.
/// ':' is accepted because the engine's own config values use it as a level separator, so
/// accepting it keeps our file looking like the rest of the game's data.
///
/// Empty fields are dropped, so runs of separators and leading/trailing separators are all
/// harmless. A null input yields an empty result rather than a crash -- config lookups
/// return null for absent keys and every caller would otherwise have to check.
inline void SplitFields(const char* value, std::vector<std::string>& out)
{
	out.clear();
	if (value == nullptr) return;

	std::string current;
	for (const char* c = value; ; c++) {
		if (*c == '\0' || *c == ' ' || *c == '\t' || *c == ':' || *c == ',') {
			if (!current.empty()) {
				out.push_back(current);
				current.clear();
			}
			if (*c == '\0') break;
		}
		else {
			current.push_back(*c);
		}
	}
}

#pragma endregion

/**********************************************************************************
 ******** Wave director scheduling
 **********************************************************************************/

#pragma region Waves

/// Everything the director needs to decide when the next wave fires and how big it is.
/// Deliberately a plain struct of numbers: the caller reads these off the live settings
/// and game state, and the decisions below are then testable without either.
struct WaveTuning
{
	float intervalSeconds    = 150.0f; ///< spacing between waves at mission start
	float rampSeconds        = 600.0f; ///< mission-seconds over which the interval halves; 0 = flat
	float minIntervalSeconds = 45.0f;  ///< floor on the ramped interval
	int   size               = 1;      ///< creatures in the first wave
	int   sizeMax            = 4;      ///< ceiling on any single wave
	int   maxAlive           = 6;      ///< ceiling on TOTAL live creatures, map-spawned included
};

/// Seconds to wait before the next wave, given how long the mission has run.
///
/// The interval decays hyperbolically: interval / (1 + missionTime/ramp). At exactly one
/// ramp period it is halved, at two it is a third, and so on -- it approaches the floor
/// without ever stepping. A linear decay would hit zero and need clamping to stay sane;
/// this shape degrades gracefully on its own and the floor is a guarantee rather than a
/// correction.
///
/// A non-positive ramp disables escalation entirely and yields a flat cadence, which is
/// what a player who just wants steady pressure would configure.
inline float WaveInterval(const WaveTuning& t, float missionSeconds)
{
	if (t.rampSeconds <= 0.0f) return t.intervalSeconds;
	if (missionSeconds < 0.0f) missionSeconds = 0.0f;

	const float ramp = missionSeconds / t.rampSeconds;
	const float interval = t.intervalSeconds / (1.0f + ramp);
	return (interval < t.minIntervalSeconds) ? t.minIntervalSeconds : interval;
}

/// How many creatures the next wave should contain, or 0 for "do not fire".
///
/// Three ceilings apply in order, and the order matters:
///   1. growth      -- one more creature every three waves
///   2. alive budget -- never exceed maxAlive counting what is ALREADY alive, which is
///                      what stops the director stacking on top of a hostile map
///   3. sizeMax     -- a single wave is never overwhelming on its own, however long the
///                      mission has run
/// Returning 0 rather than a negative number means the caller can treat this as a plain
/// count and never has to special-case a "skip" sentinel.
inline int WaveSize(const WaveTuning& t, int waveNumber, int aliveNow)
{
	if (waveNumber < 0) waveNumber = 0;
	if (aliveNow < 0)   aliveNow = 0;

	int size = t.size + (waveNumber / 3);

	const int room = t.maxAlive - aliveNow;
	if (size > room)      size = room;
	if (size > t.sizeMax) size = t.sizeMax;

	return (size > 0) ? size : 0;
}

/// Index into a rotation, for any non-negative step. Used for species pools and cosmetic
/// variants, where cycling is preferred over randomness so a player can learn the pattern
/// and a bug report reproduces.
///
/// Returns 0 for an empty rotation so the caller can index unconditionally after checking
/// emptiness once.
inline std::size_t RotationIndex(int step, std::size_t count)
{
	if (count == 0) return 0;
	if (step < 0) step = -step;
	return (std::size_t)step % count;
}

#pragma endregion

/**********************************************************************************
 ******** Spawn fairness
 **********************************************************************************/

#pragma region SpawnFairness

/// A candidate spawn block, described in terms of what the rules care about rather than in
/// terms of engine flag bits.
///
/// The normalisation is the point. The shipping code fills this from BlockFlags1/BlockFlags2
/// (Game.h:182-246); the sandbox fills it from its own synthetic map. Both then run the SAME
/// predicate, so the fairness rules are exercised for real against a generated cavern
/// instead of only being asserted in a comment next to code nobody can execute.
struct SpawnBlockInfo
{
	bool isFloor = false;          ///< walkable ground
	bool isHidden = false;         ///< inside an unopened cavern
	bool isToolStore = false;
	bool isBuilding = false;       ///< building footprint or foundation
	bool isPath = false;
	bool isBusy = false;           ///< something is already working here
	bool hasAdjacentExposedWall = false;
	int  distSqToNearestBuilding = 1 << 30;  ///< squared, in blocks; huge when none exist
};

/// May a creature fairly arrive here?
///
/// Every clause is a promise to the player, not a technical constraint:
///  - discovered floor only, because arriving inside unexplored rock is not a threat, it is
///    a bug the player cannot see
///  - never in the base, because that is the one place they are entitled to feel safe until
///    something walks there
///  - never on a busy tile, because displacing a working unit reads as the game cheating
///  - adjacent to an exposed wall, because a creature that materialises in open floor looks
///    like an error rather than something that came from somewhere
inline bool IsFairSpawn(const SpawnBlockInfo& b, int minDistanceFromBase)
{
	if (!b.isFloor)   return false;
	if (b.isHidden)   return false;
	if (b.isToolStore || b.isBuilding || b.isPath) return false;
	if (b.isBusy)     return false;
	if (!b.hasAdjacentExposedWall) return false;

	if (minDistanceFromBase > 0) {
		const int minSq = minDistanceFromBase * minDistanceFromBase;
		if (b.distSqToNearestBuilding < minSq) return false;
	}
	return true;
}

#pragma endregion

/**********************************************************************************
 ******** Threat audio
 **********************************************************************************/

#pragma region ThreatAudio

/// A cue the threat layer wants played. Which sample that maps to is the shipping
/// layer's problem; choosing it is a decision, and decisions live here.
enum class ThreatCue
{
	None = 0,
	Telegraph,       ///< a wave has been announced
	TelegraphHeavy,  ///< ...and it is a big one
	Arrival,         ///< the wave is landing now
	Escalate,        ///< periodic "the pressure is rising" line
	Cleared,         ///< the last live creature just died
};

struct ThreatAudioTuning
{
	/// A wave of at least this many creatures gets the heavier warning. Below it, the
	/// ordinary one. Two distinct warnings is the point: a player who cannot tell a
	/// skirmish from a serious wave has been given noise, not information.
	int heavyWaveSize = 3;

	/// Play the escalation line every N waves. 0 disables it. Escalation lines are
	/// flavour, and flavour on every wave stops being flavour.
	int escalateEveryNWaves = 3;
};

/// Which warning a wave of this size deserves.
inline ThreatCue TelegraphCueFor(const ThreatAudioTuning& t, int waveSize)
{
	if (waveSize <= 0) return ThreatCue::None;
	return (waveSize >= t.heavyWaveSize) ? ThreatCue::TelegraphHeavy : ThreatCue::Telegraph;
}

/// True when this wave should also carry the escalation line.
///
/// waveNumber is 0-based, and wave 0 never escalates -- the first wave of a mission is
/// the player's introduction to the mechanic, not the moment to tell them it is getting
/// worse.
inline bool ShouldEscalate(const ThreatAudioTuning& t, int waveNumber)
{
	if (t.escalateEveryNWaves <= 0) return false;
	if (waveNumber <= 0) return false;
	return (waveNumber % t.escalateEveryNWaves) == 0;
}

/// Edge detector for "the cavern just went quiet".
///
/// Deliberately an explicit little state machine rather than a check inlined at the call
/// site, because the interesting case is the EDGE and edges are exactly what gets written
/// wrong: firing on every frame while alive == 0, or firing once at mission start before
/// anything has ever spawned. Both are covered by harness tests.
struct QuietDetector
{
	int  lastAlive   = 0;
	bool everHadLife = false;

	/// Feed the current live-creature count; returns Cleared exactly on the transition
	/// from "something alive" to "nothing alive", and None otherwise.
	ThreatCue Update(int aliveNow)
	{
		if (aliveNow < 0) aliveNow = 0;

		ThreatCue cue = ThreatCue::None;
		if (aliveNow > 0) {
			everHadLife = true;
		}
		else if (everHadLife && lastAlive > 0) {
			cue = ThreatCue::Cleared;
		}
		lastAlive = aliveNow;
		return cue;
	}

	void Reset() { lastAlive = 0; everHadLife = false; }
};

#pragma endregion

/**********************************************************************************
 ******** Bounds and clamping
 **********************************************************************************/

#pragma region Bounds

/// True if an index is safe to use against a fixed-size table.
///
/// Exists so the intent reads the same at every call site. Every fixed-size array indexed
/// by a config-derived value in this codebase is a corruption site until proven otherwise,
/// and several were: an object ID of 15 aliases another object type's row, and a levels
/// count above the maximum overruns every per-level table.
inline bool IndexInRange(long long index, long long count)
{
	return index >= 0 && index < count;
}

/// Clamp a count to a maximum, reporting whether it had to.
///
/// The bool return is the point: the caller warns only when something was actually wrong,
/// so a correct config stays silent and a broken one is loud exactly once.
inline bool ClampCount(unsigned int& value, unsigned int maximum)
{
	if (value <= maximum) return false;
	value = maximum;
	return true;
}

#pragma endregion

} // namespace Logic

} // namespace DeepCore
