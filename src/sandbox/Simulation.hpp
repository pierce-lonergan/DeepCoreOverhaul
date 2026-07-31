// Simulation.hpp : the sandbox tick loop.
//
// Runs the wave director's real decision logic -- the same DeepCoreLogic.hpp the shipping
// DLL calls -- against a synthetic cavern, with no game, no exe, and no copyrighted asset.
//
// WHAT THIS DOES AND DOES NOT PROVE
// ---------------------------------
// SANDBOX-VERIFIED means: our code executed, on real data, and did the expected thing.
// That is a genuine upgrade over compile-verified and it is the first time any of this has
// run at all.
//
// It is NOT play-tested. This says nothing about the 1999 executable, nothing about
// Direct3D Retained Mode, nothing about how a wave feels in an actual mission, and nothing
// about whether the engine's spawn call succeeds. It validates the decisions, not the game.
//

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../openlrr/game/DeepCoreLogic.hpp"
#include "SyntheticLevel.hpp"


namespace Sandbox
{

/// A creature the sandbox is tracking. Not a LegoObject -- that is 1036 bytes of engine
/// state we cannot construct here. This is only what the director's decisions depend on.
struct Creature
{
	int x = 0, y = 0;
	int speciesId = 0;
	float health = 100.0f;
	float ageSeconds = 0.0f;
	bool emerging = true;      ///< mirrors LIVEOBJ1_EXPANDING: alive, but not yet active
};

struct SimConfig
{
	LevelDesc level;
	DeepCore::Logic::WaveTuning wave;
	DeepCore::Logic::ThreatAudioTuning audio;

	int   minDistanceFromBase = 8;
	float telegraphSeconds = 6.0f;
	float durationSeconds = 900.0f;
	float tickSeconds = 0.1f;

	/// Creatures wander and are killed off at this rate, standing in for the player
	/// fighting back. Without it the alive budget saturates immediately and every wave
	/// after the first is suppressed -- which is itself a finding worth being able to see.
	float killPerCreaturePerSecond = 0.012f;

	/// Simulated exploration: the player opens a new chamber every so often, which changes
	/// which blocks are legal spawns.
	float discoverEverySeconds = 120.0f;

	std::uint32_t seed = 1u;
	bool traceJson = false;
};

/// One structured trace record. Emitted as JSONL so a run is greppable and diffable, and so
/// a regression shows up as a diff rather than as someone's recollection.
struct TraceEvent
{
	float t = 0.0f;
	std::string kind;
	std::string detail;
	int alive = 0;
	int waveNumber = 0;
};

class Simulation
{
public:
	void Init(const SimConfig& cfg);
	void Step();
	bool Done() const { return m_time >= m_cfg.durationSeconds; }

	float Time() const { return m_time; }
	int   AliveCount() const { return (int)m_creatures.size(); }
	int   WaveNumber() const { return m_waveNumber; }
	bool  Telegraphing() const { return m_phase == Phase::Telegraph; }
	float PhaseTimer() const { return m_phaseTimer; }
	const std::vector<Creature>& Creatures() const { return m_creatures; }
	const std::vector<std::pair<int,int>>& PendingBlocks() const { return m_pending; }
	const Level& Map() const { return m_level; }
	const std::vector<TraceEvent>& Trace() const { return m_trace; }

	/// Totals, for the end-of-run summary and for CI assertions.
	int TotalSpawned() const { return m_totalSpawned; }
	int TotalWaves() const   { return m_waveNumber; }
	int PeakAlive() const    { return m_peakAlive; }
	int CuesFired(DeepCore::Logic::ThreatCue c) const;

	/// Render the map as ASCII for the terminal view.
	std::string RenderAscii(bool colour) const;
	std::string StatusLine() const;

private:
	enum class Phase { Waiting, Telegraph };

	void Emit(const char* kind, const std::string& detail);
	void GatherCandidates(std::vector<std::pair<int,int>>& out) const;
	DeepCore::Logic::SpawnBlockInfo Describe(int x, int y) const;
	void FireCue(DeepCore::Logic::ThreatCue c);

	SimConfig m_cfg;
	Level m_level;
	Rng m_rng;

	float m_time = 0.0f;
	float m_phaseTimer = 0.0f;
	float m_sinceDiscover = 0.0f;
	Phase m_phase = Phase::Waiting;
	int m_waveNumber = 0;
	int m_totalSpawned = 0;
	int m_peakAlive = 0;

	std::vector<Creature> m_creatures;
	std::vector<std::pair<int,int>> m_pending;
	std::vector<std::pair<int,int>> m_buildings;
	DeepCore::Logic::QuietDetector m_quiet;
	std::vector<TraceEvent> m_trace;
	int m_cueCounts[8] = { 0 };
};

} // namespace Sandbox
