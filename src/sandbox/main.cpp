// main.cpp : the DeepCore sandbox.
//
// Runs DeepCoreOverhaul's own systems with no game, no 1999 executable, and no copyrighted
// asset of any kind. The cavern is procedurally generated; the decisions are the real ones
// from src/openlrr/game/DeepCoreLogic.hpp, the same header the shipping DLL calls.
//
// THE CLAIM THIS SUPPORTS, EXACTLY
//   compile-verified  -- it builds
//   sandbox-verified  -- our code executed, on real data, and did the expected thing
//   play-tested       -- nobody can say this, and this program does not change that
//
// Sandbox-verified says NOTHING about the 1999 executable, nothing about Direct3D Retained
// Mode, and nothing about how any of this feels in an actual mission.
//
//   sandbox            headless run, human-readable summary
//   sandbox --trace    JSONL event stream on stdout (this is what CI asserts on)
//   sandbox --view     animated terminal view
//   sandbox --map      print the generated cavern and exit
//   sandbox --selftest run assertions over a batch of seeds, exit non-zero on failure
//

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#include "Simulation.hpp"

#if defined(_WIN32)
#  include <windows.h>
#endif

using namespace Sandbox;
using namespace DeepCore::Logic;

namespace
{

void EnableAnsi()
{
#if defined(_WIN32)
	HANDLE h = ::GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode = 0;
	if (::GetConsoleMode(h, &mode)) {
		::SetConsoleMode(h, mode | 0x0004 /*ENABLE_VIRTUAL_TERMINAL_PROCESSING*/);
	}
#endif
}

SimConfig DefaultConfig(std::uint32_t seed)
{
	SimConfig cfg;
	cfg.seed = seed;
	cfg.level.seed = seed;
	cfg.level.width = 56;
	cfg.level.height = 28;
	cfg.level.caverns = 6;
	// Deliberately more water than the ORIGINAL engine's 10-pool / 100-block cap allowed,
	// so the generated map is one the relocated tables were built for.
	cfg.level.waterFraction = 0.08f;

	cfg.wave.intervalSeconds = 60.0f;
	cfg.wave.rampSeconds = 300.0f;
	cfg.wave.minIntervalSeconds = 20.0f;
	cfg.wave.size = 1;
	cfg.wave.sizeMax = 4;
	cfg.wave.maxAlive = 6;
	cfg.telegraphSeconds = 6.0f;
	cfg.durationSeconds = 900.0f;
	return cfg;
}

int RunHeadless(SimConfig cfg)
{
	Simulation sim;
	sim.Init(cfg);
	while (!sim.Done()) sim.Step();

	if (cfg.traceJson) return 0;

	std::printf("\nDeepCore sandbox -- headless run\n");
	std::printf("  seed              %u\n", cfg.seed);
	std::printf("  simulated         %.0f s of mission time\n", cfg.durationSeconds);
	std::printf("  waves landed      %d\n", sim.TotalWaves());
	std::printf("  creatures spawned %d\n", sim.TotalSpawned());
	std::printf("  peak alive        %d  (budget %d)\n", sim.PeakAlive(), cfg.wave.maxAlive);
	std::printf("  cues: telegraph %d  heavy %d  arrival %d  escalate %d  cleared %d\n",
				sim.CuesFired(ThreatCue::Telegraph), sim.CuesFired(ThreatCue::TelegraphHeavy),
				sim.CuesFired(ThreatCue::Arrival), sim.CuesFired(ThreatCue::Escalate),
				sim.CuesFired(ThreatCue::Cleared));
	std::printf("  trace events      %zu\n", sim.Trace().size());
	std::printf("\n  SANDBOX-VERIFIED: this executed. It is NOT play-tested and says nothing\n"
				"  about the 1999 executable or how a wave feels in a real mission.\n");
	return 0;
}

int RunView(SimConfig cfg)
{
	EnableAnsi();
	Simulation sim;
	sim.Init(cfg);

	std::printf("\x1b[2J");
	while (!sim.Done()) {
		for (int i = 0; i < 5; i++) { if (!sim.Done()) sim.Step(); }

		std::printf("\x1b[H");
		std::printf("DeepCore sandbox -- terminal view   (no game, no exe, synthetic cavern)\n");
		std::printf("%s\n", sim.StatusLine().c_str());
		std::printf("legend  #rock  %%wall  .floor  ~water  O ore  C crystal  T store  "
					"\x1b[1;31m!\x1b[0m incoming  \x1b[1;31mM\x1b[0m monster\n\n");
		std::printf("%s", sim.RenderAscii(true).c_str());
		std::fflush(stdout);

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	std::printf("\nDone. %d waves, %d spawned, peak alive %d.\n",
				sim.TotalWaves(), sim.TotalSpawned(), sim.PeakAlive());
	return 0;
}

/// Assertions that must hold for ANY seed. These are what make the sandbox a test rather
/// than a demo, and they are exactly the invariants that were previously only claimed in
/// comments beside code nobody could run.
int RunSelfTest()
{
	int failures = 0;
	auto fail = [&](std::uint32_t seed, const char* what) {
		std::printf("  FAIL seed %-4u  %s\n", seed, what);
		failures++;
	};

	std::printf("DeepCore sandbox -- self test over 40 seeds\n\n");

	for (std::uint32_t seed = 1; seed <= 40; seed++) {
		SimConfig cfg = DefaultConfig(seed);
		Simulation sim;
		sim.Init(cfg);

		int maxAliveSeen = 0;
		while (!sim.Done()) {
			sim.Step();
			if (sim.AliveCount() > maxAliveSeen) maxAliveSeen = sim.AliveCount();

			// THE invariant: the budget is a ceiling on total creature pressure, and a
			// director that exceeds it makes a level unwinnable.
			if (sim.AliveCount() > cfg.wave.maxAlive) {
				fail(seed, "alive count exceeded maxAlive");
				break;
			}
			// Every pending spawn must still be a fair block at the moment it was chosen.
			for (const auto& p : sim.PendingBlocks()) {
				const Block& b = sim.Map().At(p.first, p.second);
				if (b.Has(BLOCK_TOOLSTORE) || b.Has(BLOCK_BUILDING)) {
					fail(seed, "telegraphed a spawn inside the player's base");
					break;
				}
			}
		}

		// A 900-second mission with a 60s opening interval must produce waves. Zero would
		// mean the director never fired, which is the failure mode a comment cannot catch.
		if (sim.TotalWaves() == 0) fail(seed, "no waves in 900 simulated seconds");

		// Escalation must actually escalate: the last interval must be shorter than the
		// first, or the ramp is not doing anything.
		const float first = WaveInterval(cfg.wave, 0.0f);
		const float last  = WaveInterval(cfg.wave, cfg.durationSeconds);
		if (!(last < first)) fail(seed, "interval did not decrease over the mission");

		// Every arrival must have been preceded by a telegraph. An untelegraphed wave is
		// the single thing the design forbids.
		const int telegraphs = sim.CuesFired(ThreatCue::Telegraph)
							 + sim.CuesFired(ThreatCue::TelegraphHeavy);
		if (sim.CuesFired(ThreatCue::Arrival) > telegraphs) {
			fail(seed, "an arrival cue fired without a preceding telegraph");
		}
	}

	std::printf("\n%s  (%d failure(s))\n",
				failures == 0 ? "ALL SEEDS PASS" : "FAILURES PRESENT", failures);
	std::printf("\nSANDBOX-VERIFIED. Not play-tested.\n");
	return failures == 0 ? 0 : 1;
}

} // namespace


int main(int argc, char** argv)
{
	std::uint32_t seed = 1u;
	bool view = false, trace = false, mapOnly = false, selftest = false;
	float duration = 900.0f;

	for (int i = 1; i < argc; i++) {
		if (!std::strcmp(argv[i], "--view")) view = true;
		else if (!std::strcmp(argv[i], "--trace")) trace = true;
		else if (!std::strcmp(argv[i], "--map")) mapOnly = true;
		else if (!std::strcmp(argv[i], "--selftest")) selftest = true;
		else if (!std::strcmp(argv[i], "--seed") && i + 1 < argc) seed = (std::uint32_t)std::atoi(argv[++i]);
		else if (!std::strcmp(argv[i], "--seconds") && i + 1 < argc) duration = (float)std::atof(argv[++i]);
		else if (!std::strcmp(argv[i], "--help")) {
			std::printf("DeepCore sandbox\n"
						"  --view      animated terminal view\n"
						"  --trace     JSONL event stream\n"
						"  --map       print the generated cavern and exit\n"
						"  --selftest  invariant assertions over 40 seeds\n"
						"  --seed N    generator seed (default 1)\n"
						"  --seconds N simulated mission seconds (default 900)\n");
			return 0;
		}
	}

	if (selftest) return RunSelfTest();

	SimConfig cfg = DefaultConfig(seed);
	cfg.durationSeconds = duration;
	cfg.traceJson = trace;

	if (mapOnly) {
		Level lvl;
		lvl.Generate(cfg.level);
		std::printf("%s", lvl.ToText().c_str());
		return 0;
	}

	return view ? RunView(cfg) : RunHeadless(cfg);
}
