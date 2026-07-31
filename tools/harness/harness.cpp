// harness.cpp : Headless tests and benchmarks for DeepCoreOverhaul's pure logic.
//
// WHY
// ---
// This project has no installation of the original game, so nothing that touches an
// exe-overlaid global can be executed at all -- reading one outside the injected process
// reads unmapped memory. That leaves compile-verification as the only check, and
// "it compiles" says nothing about whether a wave fires at the right time.
//
// This binary closes part of that gap. It links src/openlrr/game/DeepCoreLogic.hpp --
// which by construction depends on nothing but the standard library -- and exercises the
// actual arithmetic that ships. It runs anywhere, needs no game, no exe, no DirectX.
//
// It is NOT a substitute for play-testing and nothing here should ever be described as
// such. It verifies that the decisions are correct, not that the engine does what we think
// when handed them.
//
// Build:  MSBuild tools/harness/harness.vcxproj /p:Configuration=Release /p:Platform=x86
// Run:    bin/harness.exe            (all tests, then benchmarks)
//         bin/harness.exe --no-bench (tests only -- this is what CI runs)
//
// Exit code 0 = every test passed. Non-zero = the number of failures.
//

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../../src/openlrr/game/DeepCoreLogic.hpp"
#include "../../src/openlrr/game/DeepCoreDenseIndex.hpp"
#include "../../src/game3d/Anim.hpp"

using namespace DeepCore::Logic;


/**********************************************************************************
 ******** Tiny test framework
 **********************************************************************************/

namespace
{

int g_checks = 0;
int g_failures = 0;
const char* g_currentTest = "";

void Fail(const char* file, int line, const char* expr, const std::string& detail)
{
	g_failures++;
	std::printf("  FAIL  %s\n        %s:%d\n        %s\n", g_currentTest, file, line, expr);
	if (!detail.empty()) std::printf("        %s\n", detail.c_str());
}

#define CHECK(expr)                                                            \
	do {                                                                       \
		g_checks++;                                                            \
		if (!(expr)) Fail(__FILE__, __LINE__, #expr, "");                      \
	} while (0)

#define CHECK_EQ(actual, expected)                                             \
	do {                                                                       \
		g_checks++;                                                            \
		const auto _a = (actual);                                              \
		const auto _e = (expected);                                            \
		if (!(_a == _e)) {                                                     \
			char _buf[256];                                                    \
			std::snprintf(_buf, sizeof(_buf), "got %lld, expected %lld",        \
						  (long long)_a, (long long)_e);                       \
			Fail(__FILE__, __LINE__, #actual " == " #expected, _buf);          \
		}                                                                      \
	} while (0)

#define CHECK_NEAR(actual, expected, tol)                                      \
	do {                                                                       \
		g_checks++;                                                            \
		const double _a = (double)(actual);                                    \
		const double _e = (double)(expected);                                  \
		if (std::fabs(_a - _e) > (double)(tol)) {                              \
			char _buf[256];                                                    \
			std::snprintf(_buf, sizeof(_buf), "got %.6f, expected %.6f +/- %.6f", \
						  _a, _e, (double)(tol));                              \
			Fail(__FILE__, __LINE__, #actual " ~= " #expected, _buf);          \
		}                                                                      \
	} while (0)

struct TestCase { const char* name; void (*fn)(void); };
std::vector<TestCase>& Registry() { static std::vector<TestCase> r; return r; }

struct Register
{
	Register(const char* name, void (*fn)(void)) { Registry().push_back({ name, fn }); }
};

#define TEST(name)                                                             \
	static void name(void);                                                    \
	static Register _reg_##name(#name, name);                                  \
	static void name(void)


/**********************************************************************************
 ******** SplitFields
 **********************************************************************************/

std::vector<std::string> Split(const char* s)
{
	std::vector<std::string> out;
	SplitFields(s, out);
	return out;
}

TEST(split_basic_whitespace)
{
	const auto f = Split("RockMonster IceMonster LavaMonster");
	CHECK_EQ(f.size(), 3u);
	CHECK(f[0] == "RockMonster");
	CHECK(f[2] == "LavaMonster");
}

TEST(split_colon_and_space_are_equivalent)
{
	// The engine's own config values use ':' as a level separator, so "1.5 0.6:0.3:0.3"
	// and "1.5 0.6 0.3 0.3" must mean the same thing.
	CHECK(Split("1.5 0.6:0.3:0.3") == Split("1.5 0.6 0.3 0.3"));
}

TEST(split_commas_and_tabs)
{
	const auto f = Split("a,b\tc");
	CHECK_EQ(f.size(), 3u);
	CHECK(f[1] == "b");
}

TEST(split_drops_empty_fields)
{
	// Runs of separators, and leading/trailing ones, must not produce empty entries --
	// an empty species name would otherwise reach the name resolver.
	const auto f = Split("  ::a,,,b::  ");
	CHECK_EQ(f.size(), 2u);
	CHECK(f[0] == "a");
	CHECK(f[1] == "b");
}

TEST(split_null_is_empty_not_a_crash)
{
	// Config lookups return null for an absent key. Every caller would otherwise have to
	// check, and one of them would eventually forget.
	std::vector<std::string> out;
	out.push_back("stale");
	SplitFields(nullptr, out);
	CHECK_EQ(out.size(), 0u);
}

TEST(split_clears_previous_contents)
{
	std::vector<std::string> out;
	SplitFields("a b c", out);
	SplitFields("x", out);
	CHECK_EQ(out.size(), 1u);
	CHECK(out[0] == "x");
}

TEST(split_separators_only)
{
	CHECK_EQ(Split(" : , \t ").size(), 0u);
}

TEST(split_single_field_no_separators)
{
	const auto f = Split("Lazer");
	CHECK_EQ(f.size(), 1u);
	CHECK(f[0] == "Lazer");
}


/**********************************************************************************
 ******** WaveInterval
 **********************************************************************************/

TEST(interval_starts_at_configured_value)
{
	WaveTuning t;
	CHECK_NEAR(WaveInterval(t, 0.0f), t.intervalSeconds, 0.001);
}

TEST(interval_halves_after_exactly_one_ramp_period)
{
	// The documented shape: interval / (1 + missionTime/ramp). At one ramp period the
	// divisor is 2. This is the property the design comment claims, so it is pinned here.
	WaveTuning t;
	t.intervalSeconds = 150.0f;
	t.rampSeconds = 600.0f;
	t.minIntervalSeconds = 1.0f; // out of the way
	CHECK_NEAR(WaveInterval(t, 600.0f), 75.0f, 0.001);
	CHECK_NEAR(WaveInterval(t, 1200.0f), 50.0f, 0.001);
}

TEST(interval_never_falls_below_the_floor)
{
	WaveTuning t;
	t.intervalSeconds = 150.0f;
	t.rampSeconds = 60.0f;
	t.minIntervalSeconds = 45.0f;
	// An hour in, the unclamped value would be ~2.5s.
	CHECK_NEAR(WaveInterval(t, 3600.0f), 45.0f, 0.001);
}

TEST(interval_is_monotonically_non_increasing)
{
	// Escalation must never go backwards -- a mission that got easier as it ran would be
	// the opposite of the intent.
	WaveTuning t;
	float prev = WaveInterval(t, 0.0f);
	for (float s = 0.0f; s <= 3600.0f; s += 7.5f) {
		const float now = WaveInterval(t, s);
		CHECK(now <= prev + 0.0001f);
		prev = now;
	}
}

TEST(interval_zero_ramp_is_flat)
{
	WaveTuning t;
	t.rampSeconds = 0.0f;
	CHECK_NEAR(WaveInterval(t, 0.0f), t.intervalSeconds, 0.001);
	CHECK_NEAR(WaveInterval(t, 99999.0f), t.intervalSeconds, 0.001);
}

TEST(interval_negative_mission_time_is_treated_as_zero)
{
	// Defensive: a caller that ever hands us a negative accumulator must not get a
	// LARGER interval than at t=0, or a clock glitch would pause the director.
	WaveTuning t;
	CHECK_NEAR(WaveInterval(t, -100.0f), WaveInterval(t, 0.0f), 0.001);
}


/**********************************************************************************
 ******** WaveSize
 **********************************************************************************/

TEST(size_starts_at_configured_size)
{
	WaveTuning t;
	CHECK_EQ(WaveSize(t, 0, 0), t.size);
}

TEST(size_grows_every_three_waves)
{
	WaveTuning t;
	t.size = 1;
	t.sizeMax = 99;
	t.maxAlive = 99;
	CHECK_EQ(WaveSize(t, 0, 0), 1);
	CHECK_EQ(WaveSize(t, 2, 0), 1);
	CHECK_EQ(WaveSize(t, 3, 0), 2);
	CHECK_EQ(WaveSize(t, 6, 0), 3);
}

TEST(size_respects_the_alive_budget)
{
	// The budget counts creatures the MAP spawned too. This is what stops the director
	// stacking on top of an already-hostile level.
	WaveTuning t;
	t.size = 4;
	t.sizeMax = 99;
	t.maxAlive = 6;
	CHECK_EQ(WaveSize(t, 0, 4), 2);
	CHECK_EQ(WaveSize(t, 0, 6), 0);
	CHECK_EQ(WaveSize(t, 99, 6), 0);
}

TEST(size_never_negative_when_over_budget)
{
	// If the map has already exceeded our budget on its own, the answer is "do not fire",
	// not a negative count that a caller might use as a loop bound.
	WaveTuning t;
	t.maxAlive = 6;
	CHECK_EQ(WaveSize(t, 0, 20), 0);
}

TEST(size_respects_the_per_wave_ceiling)
{
	WaveTuning t;
	t.size = 1;
	t.sizeMax = 4;
	t.maxAlive = 99;
	CHECK_EQ(WaveSize(t, 300, 0), 4);
}

TEST(size_ceiling_order_alive_budget_wins_over_sizemax)
{
	// Order matters: the alive budget must bind even when sizeMax would allow more,
	// otherwise a late wave could exceed maxAlive.
	WaveTuning t;
	t.size = 10;
	t.sizeMax = 8;
	t.maxAlive = 6;
	CHECK_EQ(WaveSize(t, 0, 5), 1);
}

TEST(size_never_exceeds_remaining_budget_over_a_long_mission)
{
	// Property test: across a long mission at every alive count, size + alive must never
	// exceed maxAlive. This is the invariant that keeps a level playable.
	WaveTuning t;
	t.size = 2;
	t.sizeMax = 5;
	t.maxAlive = 7;
	for (int wave = 0; wave < 200; wave++) {
		for (int alive = 0; alive <= 12; alive++) {
			const int s = WaveSize(t, wave, alive);
			CHECK(s >= 0);
			if (s > 0) CHECK(s + alive <= t.maxAlive);
			CHECK(s <= t.sizeMax);
		}
	}
}

TEST(size_negative_inputs_are_clamped)
{
	WaveTuning t;
	CHECK(WaveSize(t, -5, -5) >= 0);
}


/**********************************************************************************
 ******** RotationIndex / bounds helpers
 **********************************************************************************/

TEST(rotation_cycles)
{
	CHECK_EQ(RotationIndex(0, 3), 0u);
	CHECK_EQ(RotationIndex(1, 3), 1u);
	CHECK_EQ(RotationIndex(3, 3), 0u);
	CHECK_EQ(RotationIndex(7, 3), 1u);
}

TEST(rotation_empty_is_zero_not_a_division_by_zero)
{
	CHECK_EQ(RotationIndex(5, 0), 0u);
}

TEST(rotation_negative_step_stays_in_range)
{
	for (int i = -50; i < 0; i++) {
		CHECK(RotationIndex(i, 4) < 4u);
	}
}

TEST(index_in_range_rejects_the_id_ceiling)
{
	// The concrete case that motivated this helper: LegoObject_ID_Count is 15, and an id
	// of 15 does not overflow -- in a [20][15] table it aliases [4][0], which is
	// LegoObject_Building ID 0, the Tool Store. 15 must be rejected.
	CHECK(IndexInRange(14, 15));
	CHECK(!IndexInRange(15, 15));
	CHECK(!IndexInRange(-1, 15));
}

TEST(clamp_count_reports_only_when_it_acted)
{
	// The bool return exists so a correct config stays silent and a broken one is loud
	// exactly once.
	unsigned int v = 10;
	CHECK(!ClampCount(v, 16));
	CHECK_EQ(v, 10u);

	v = 40;
	CHECK(ClampCount(v, 16));
	CHECK_EQ(v, 16u);

	v = 16;
	CHECK(!ClampCount(v, 16)); // boundary is allowed
	CHECK_EQ(v, 16u);
}


/**********************************************************************************
 ******** Threat audio decisions
 **********************************************************************************/

TEST(telegraph_cue_splits_on_wave_size)
{
	ThreatAudioTuning t;
	t.heavyWaveSize = 3;
	CHECK(TelegraphCueFor(t, 1) == ThreatCue::Telegraph);
	CHECK(TelegraphCueFor(t, 2) == ThreatCue::Telegraph);
	CHECK(TelegraphCueFor(t, 3) == ThreatCue::TelegraphHeavy);
	CHECK(TelegraphCueFor(t, 9) == ThreatCue::TelegraphHeavy);
}

TEST(telegraph_cue_none_for_empty_wave)
{
	// WaveSize returns 0 for "do not fire". That must not produce a warning for a wave
	// that never comes -- crying wolf is exactly the failure this layer must not have.
	ThreatAudioTuning t;
	CHECK(TelegraphCueFor(t, 0) == ThreatCue::None);
	CHECK(TelegraphCueFor(t, -1) == ThreatCue::None);
}

TEST(escalate_skips_the_first_wave)
{
	// Wave 0 is the player's introduction to the mechanic, not the moment to tell them
	// it is getting worse.
	ThreatAudioTuning t;
	t.escalateEveryNWaves = 3;
	CHECK(!ShouldEscalate(t, 0));
	CHECK(!ShouldEscalate(t, 1));
	CHECK(!ShouldEscalate(t, 2));
	CHECK(ShouldEscalate(t, 3));
	CHECK(ShouldEscalate(t, 6));
	CHECK(!ShouldEscalate(t, 7));
}

TEST(escalate_zero_disables)
{
	ThreatAudioTuning t;
	t.escalateEveryNWaves = 0;
	for (int i = 0; i < 50; i++) CHECK(!ShouldEscalate(t, i));
}

TEST(escalate_negative_period_disables_rather_than_dividing)
{
	ThreatAudioTuning t;
	t.escalateEveryNWaves = -3;
	for (int i = 0; i < 20; i++) CHECK(!ShouldEscalate(t, i));
}

TEST(quiet_fires_once_on_the_edge)
{
	QuietDetector q;
	CHECK(q.Update(2) == ThreatCue::None);
	CHECK(q.Update(1) == ThreatCue::None);
	CHECK(q.Update(0) == ThreatCue::Cleared);
	// and not again while it stays quiet -- this is the bug the detector exists to avoid
	for (int i = 0; i < 100; i++) CHECK(q.Update(0) == ThreatCue::None);
}

TEST(quiet_never_fires_before_anything_lived)
{
	// A mission opens with nothing alive. An all-clear at t=0 would be nonsense.
	QuietDetector q;
	for (int i = 0; i < 100; i++) CHECK(q.Update(0) == ThreatCue::None);
}

TEST(quiet_rearms_across_waves)
{
	QuietDetector q;
	q.Update(3);
	CHECK(q.Update(0) == ThreatCue::Cleared);
	q.Update(2);                                   // next wave
	CHECK(q.Update(0) == ThreatCue::Cleared);      // must fire again
}

TEST(quiet_reset_clears_history)
{
	// Reset happens on level teardown; the next level must not inherit an armed detector.
	QuietDetector q;
	q.Update(5);
	q.Reset();
	for (int i = 0; i < 10; i++) CHECK(q.Update(0) == ThreatCue::None);
}

TEST(quiet_negative_alive_treated_as_zero)
{
	QuietDetector q;
	q.Update(4);
	CHECK(q.Update(-7) == ThreatCue::Cleared);
}

TEST(fuzz_quiet_detector_fires_only_on_edges)
{
	// Property: over any sequence, Cleared appears exactly as often as the number of
	// positive-to-zero transitions after the first positive value.
	unsigned int seed = 2246822519u;
	auto next = [&seed]() { seed = seed * 1103515245u + 12345u; return (seed >> 16) & 0x7fff; };

	for (int trial = 0; trial < 500; trial++) {
		QuietDetector q;
		int prev = 0; bool lived = false; int expected = 0, got = 0;
		for (int step = 0; step < 200; step++) {
			const int alive = (int)(next() % 4);
			if (alive > 0) lived = true;
			else if (lived && prev > 0) expected++;
			if (q.Update(alive) == ThreatCue::Cleared) got++;
			prev = alive;
		}
		CHECK_EQ(got, expected);
	}
}


/**********************************************************************************
 ******** DenseLiveIndex -- the A1 optimisation, tested and measured here
 **********************************************************************************/

// A stand-in for LegoObject: the real one is 1036 bytes, and the SIZE is the point --
// the existing walk touches one pointer per slot at that stride and misses cache on
// essentially every one. Anything smaller would flatter the old path.
struct FakeItem
{
	FakeItem* nextFree;              // ListSet liveness: nextFree == this means DEAD
	unsigned char pad[1036 - sizeof(void*)];
};

// Mirrors ListSet's geometric layout: list i holds 2^i items, capacity 2^listCount - 1.
struct FakeListSet
{
	std::vector<std::vector<FakeItem>> lists;
	std::size_t listCount = 0;

	void Grow(std::size_t count)
	{
		listCount = count;
		lists.clear();
		for (std::size_t i = 0; i < count; i++) lists.emplace_back((std::size_t)1u << i);
		for (auto& l : lists) for (auto& it : l) it.nextFree = &it;  // all dead initially
	}
	std::size_t Capacity() const { return ((std::size_t)1u << listCount) - 1u; }

	FakeItem* At(std::size_t slot)
	{
		for (std::size_t i = 0; i < listCount; i++) {
			const std::size_t n = (std::size_t)1u << i;
			if (slot < n) return &lists[i][slot];
			slot -= n;
		}
		return nullptr;
	}
	static bool Alive(const FakeItem* it) { return it->nextFree != it; }
	void SetAlive(std::size_t slot, bool alive)
	{
		FakeItem* it = At(slot);
		it->nextFree = alive ? nullptr : it;
	}
	// The existing enumeration: full walk of every slot, liveness read per item.
	std::size_t WalkCountAlive()
	{
		std::size_t n = 0;
		for (std::size_t i = 0; i < listCount; i++)
			for (auto& it : lists[i]) if (Alive(&it)) n++;
		return n;
	}
};

using Index = DeepCore::Detail::DenseLiveIndex<FakeItem>;

static void RebuildFrom(Index& idx, FakeListSet& ls)
{
	idx.RebuildBegin();
	for (std::size_t s = 0; s < ls.Capacity(); s++)
		if (FakeListSet::Alive(ls.At(s))) idx.RebuildAdd(s, ls.At(s));
	idx.RebuildEnd();
}

TEST(dense_starts_dirty)
{
	// Dirty by default is the safety property: an index that has never been built must
	// never be trusted, so the default cannot be "clean and empty".
	Index idx;
	idx.Reserve(255);
	CHECK(idx.Dirty());
}

TEST(dense_rebuild_matches_the_full_walk)
{
	FakeListSet ls; ls.Grow(8);                 // capacity 255
	for (std::size_t s = 0; s < 255; s += 3) ls.SetAlive(s, true);
	Index idx; idx.Reserve(ls.Capacity());
	RebuildFrom(idx, ls);
	CHECK(!idx.Dirty());
	CHECK_EQ(idx.Count(), ls.WalkCountAlive());
}

TEST(dense_add_and_remove_track_the_container)
{
	FakeListSet ls; ls.Grow(6);                 // capacity 63
	Index idx; idx.Reserve(ls.Capacity());
	RebuildFrom(idx, ls);

	for (std::size_t s = 0; s < 20; s++) { ls.SetAlive(s, true); idx.OnAdd(s, ls.At(s)); }
	CHECK_EQ(idx.Count(), ls.WalkCountAlive());

	// Remove from the front, the middle and the end -- the swap-with-last path is where
	// an index like this goes wrong.
	for (std::size_t s : { (std::size_t)0, (std::size_t)9, (std::size_t)19 }) {
		ls.SetAlive(s, false); idx.OnRemove(s);
	}
	CHECK_EQ(idx.Count(), ls.WalkCountAlive());
	CHECK(!idx.IsTracked(0));
	CHECK(!idx.IsTracked(9));
	CHECK(!idx.IsTracked(19));
	CHECK(idx.IsTracked(5));
}

TEST(dense_add_is_idempotent)
{
	// A duplicate would cause every consumer to process the same object twice.
	FakeListSet ls; ls.Grow(5);
	Index idx; idx.Reserve(ls.Capacity());
	RebuildFrom(idx, ls);
	ls.SetAlive(3, true);
	idx.OnAdd(3, ls.At(3));
	idx.OnAdd(3, ls.At(3));
	idx.OnAdd(3, ls.At(3));
	CHECK_EQ(idx.Count(), 1u);
}

TEST(dense_remove_of_untracked_slot_is_harmless)
{
	FakeListSet ls; ls.Grow(5);
	Index idx; idx.Reserve(ls.Capacity());
	RebuildFrom(idx, ls);
	idx.OnRemove(7);
	idx.OnRemove(0);
	CHECK_EQ(idx.Count(), 0u);
}

TEST(dense_out_of_range_marks_dirty_rather_than_corrupting)
{
	// The safe failure. An out-of-range slot means our idea of capacity is wrong, so the
	// only honest response is to distrust the whole index.
	Index idx; idx.Reserve(16);
	idx.RebuildBegin(); idx.RebuildEnd();
	CHECK(!idx.Dirty());
	FakeItem dummy; dummy.nextFree = nullptr;
	idx.OnAdd(99, &dummy);
	CHECK(idx.Dirty());
}

TEST(dense_clear_marks_dirty)
{
	FakeListSet ls; ls.Grow(5);
	Index idx; idx.Reserve(ls.Capacity());
	RebuildFrom(idx, ls);
	idx.Clear();
	CHECK(idx.Dirty());
	CHECK_EQ(idx.Count(), 0u);
}

TEST(dense_reserve_change_invalidates)
{
	// Slot ids are only meaningful for a fixed capacity, so a capacity change must not
	// leave stale mappings behind.
	FakeListSet ls; ls.Grow(5);
	Index idx; idx.Reserve(ls.Capacity());
	RebuildFrom(idx, ls);
	idx.Reserve(1023);
	CHECK(idx.Dirty());
	CHECK_EQ(idx.Count(), 0u);
}

TEST(fuzz_dense_index_always_agrees_with_the_full_walk)
{
	// The property that matters: after ANY sequence of adds and removes, the index's live
	// set must equal what the O(capacity) walk would report. This is the test that makes
	// the optimisation safe to ship.
	unsigned int seed = 424242u;
	auto next = [&seed]() { seed = seed * 1103515245u + 12345u; return (seed >> 16) & 0x7fff; };

	FakeListSet ls; ls.Grow(7);                  // capacity 127
	Index idx; idx.Reserve(ls.Capacity());
	RebuildFrom(idx, ls);

	for (int step = 0; step < 40000; step++) {
		const std::size_t slot = next() % ls.Capacity();
		const bool alive = FakeListSet::Alive(ls.At(slot));
		if (!alive) { ls.SetAlive(slot, true);  idx.OnAdd(slot, ls.At(slot)); }
		else        { ls.SetAlive(slot, false); idx.OnRemove(slot); }

		if ((step % 977) == 0) {
			CHECK_EQ(idx.Count(), ls.WalkCountAlive());
			// every tracked pointer must actually be live in the container
			for (FakeItem* p : idx.Live()) CHECK(FakeListSet::Alive(p));
		}
	}
	CHECK_EQ(idx.Count(), ls.WalkCountAlive());
}


/**********************************************************************************
 ******** Animation -- the maths that makes a character look alive
 **********************************************************************************/

TEST(anim_walk_foot_stays_planted_through_stance)
{
	// THE test for a walk cycle. If the foot moves while it is supposed to be planted, the
	// character skates, and skating is the single most obvious tell of bad locomotion.
	// Stance is phase 0.0-0.5, and lift must be exactly zero throughout it.
	for (float p = 0.0f; p < 0.5f; p += 0.01f) {
		const Anim::LegPose l = Anim::WalkLeg(p);
		CHECK(l.lift == 0.0f);
	}
}

TEST(anim_walk_foot_lifts_during_swing)
{
	// ...and it must actually leave the ground during swing, or the foot drags.
	float maxLift = 0.0f;
	for (float p = 0.5f; p < 1.0f; p += 0.01f) {
		const Anim::LegPose l = Anim::WalkLeg(p);
		CHECK(l.lift >= 0.0f);
		if (l.lift > maxLift) maxLift = l.lift;
	}
	CHECK(maxLift > 0.05f);
}

TEST(anim_walk_hip_sweeps_backward_through_stance)
{
	// During stance the foot is fixed and the body travels over it, so the hip angle must
	// decrease monotonically. Any reversal means the foot slid.
	float prev = Anim::WalkLeg(0.0f).hip;
	for (float p = 0.01f; p < 0.5f; p += 0.01f) {
		const float h = Anim::WalkLeg(p).hip;
		CHECK(h <= prev + 0.001f);
		prev = h;
	}
}

TEST(anim_walk_cycle_is_continuous_at_the_wrap)
{
	// Phase 0.999 and phase 0.0 must nearly agree, or the character snaps once per stride.
	const Anim::LegPose a = Anim::WalkLeg(0.999f);
	const Anim::LegPose b = Anim::WalkLeg(0.0f);
	CHECK_NEAR(a.hip, b.hip, 1.5);
	CHECK_NEAR(a.lift, b.lift, 0.01);
}

TEST(anim_bob_runs_at_twice_the_leg_frequency)
{
	// The body rises once per STEP, not once per cycle. This doubling is what makes a walk
	// read as a walk, and getting it wrong is the classic hand-written-locomotion error.
	int minima = 0;
	float prev = Anim::WalkBob(0.0f), prevSlope = 0.0f;
	for (float p = 0.005f; p <= 1.0f; p += 0.005f) {
		const float v = Anim::WalkBob(p);
		const float slope = v - prev;
		if (prevSlope < 0.0f && slope >= 0.0f) minima++;
		prevSlope = slope; prev = v;
	}
	CHECK_EQ(minima, 2);
}

TEST(anim_phase_wrap_never_negative)
{
	for (float t = -5.0f; t < 5.0f; t += 0.037f) {
		const float w = Anim::Wrap01(t);
		CHECK(w >= 0.0f);
		CHECK(w < 1.0f);
	}
}

TEST(anim_easing_hits_its_endpoints)
{
	CHECK_NEAR(Anim::EaseInOut(0.0f), 0.0, 1e-5);
	CHECK_NEAR(Anim::EaseInOut(1.0f), 1.0, 1e-5);
	CHECK_NEAR(Anim::EaseIn(0.0f), 0.0, 1e-5);
	CHECK_NEAR(Anim::EaseOut(1.0f), 1.0, 1e-5);
	// and it must be non-linear in between, or it is not easing at all
	CHECK(Anim::EaseInOut(0.25f) < 0.25f);
	CHECK(Anim::EaseInOut(0.75f) > 0.75f);
}

TEST(anim_spring_settles_and_does_not_oscillate_forever)
{
	Anim::Spring s;
	s.Snap(0.0f);
	for (int i = 0; i < 600; i++) s.Step(1.0f, 1.0f / 60.0f);
	CHECK_NEAR(s.value, 1.0, 0.01);
	CHECK_NEAR(s.vel, 0.0, 0.05);
}

TEST(anim_spring_survives_an_absurd_timestep)
{
	// A dropped frame or a dragged window must not make a character fly off. dt is clamped
	// internally precisely so a visual system cannot explode.
	Anim::Spring s;
	s.Snap(0.0f);
	for (int i = 0; i < 100; i++) s.Step(1.0f, 3.0f);
	CHECK(!std::isnan(s.value));
	CHECK(!std::isinf(s.value));
	CHECK(std::fabs(s.value) < 100.0f);
}

TEST(anim_squash_preserves_volume)
{
	// Squash that does not preserve volume reads as a scaling bug rather than as weight.
	for (float v = -6.0f; v <= 6.0f; v += 0.5f) {
		const Anim::Squash s = Anim::SquashFromVelocity(v);
		const float vol = s.x * s.y * s.z;
		CHECK_NEAR(vol, 1.0, 0.02);
	}
}

TEST(anim_ik_reaches_when_it_can_and_reports_when_it_cannot)
{
	const float upper = 0.5f, lower = 0.5f;
	const Anim::IkResult mid = Anim::TwoBoneIK(0.7f, upper, lower);
	CHECK(mid.reached);
	CHECK(mid.lower > 1.0f);                 // knee is bent

	const Anim::IkResult far = Anim::TwoBoneIK(1.5f, upper, lower);
	CHECK(!far.reached);                     // honestly reports being out of reach
}

TEST(anim_ik_never_produces_nan_at_the_limits)
{
	// acos outside [-1,1] is the classic IK crash. Sweep the whole domain including beyond.
	for (float d = 0.0f; d <= 2.5f; d += 0.01f) {
		const Anim::IkResult r = Anim::TwoBoneIK(d, 0.5f, 0.5f);
		CHECK(!std::isnan(r.upper));
		CHECK(!std::isnan(r.lower));
	}
}

TEST(anim_lookat_takes_the_short_way_round)
{
	// 350 -> 10 degrees must move +20, not -340. Getting this wrong makes a head spin.
	CHECK_NEAR(Anim::AngleDelta(350.0f, 10.0f), 20.0, 0.01);
	CHECK_NEAR(Anim::AngleDelta(10.0f, 350.0f), -20.0, 0.01);
}

TEST(anim_lookat_respects_the_neck_limit)
{
	// A target exactly 180 degrees behind is genuinely ambiguous -- both directions are the
	// same distance -- so only the MAGNITUDE of the turn is meaningful there. An earlier
	// version of this test asserted +75 and failed on a perfectly correct -75, which was
	// the test being wrong rather than the code.
	CHECK_NEAR(std::fabs(Anim::LookAtYaw(0.0f, 180.0f, 75.0f)), 75.0, 0.01);

	// With an unambiguous target the direction IS meaningful, so it is checked there.
	CHECK_NEAR(Anim::LookAtYaw(0.0f, 100.0f, 75.0f), 75.0, 0.01);
	CHECK_NEAR(Anim::LookAtYaw(0.0f, -100.0f, 75.0f), -75.0, 0.01);

	// And inside the limit it must reach the target exactly rather than clamping early.
	CHECK_NEAR(Anim::LookAtYaw(0.0f, 40.0f, 75.0f), 40.0, 0.01);
}

TEST(anim_attack_anticipates_before_it_strikes)
{
	// The curve must go NEGATIVE first. A creature that strikes without winding up reads as
	// teleporting into the hit, which is exactly the unfairness the wave director's
	// telegraph exists to avoid -- the same principle at animation scale.
	bool sawWindUp = false;
	for (float t = 0.0f; t < 0.35f; t += 0.01f)
		if (Anim::AttackCurve(t) < -0.2f) sawWindUp = true;
	CHECK(sawWindUp);
	CHECK(Anim::AttackCurve(0.5f) > 0.5f);       // and then commits
}

TEST(anim_emerge_is_monotonic)
{
	// A creature climbing out of rock must never sink back down mid-emerge.
	float prev = -1.0f;
	for (float t = 0.0f; t <= 1.0f; t += 0.01f) {
		const float h = Anim::EmergeCurve(t);
		CHECK(h >= prev - 0.02f);
		prev = h;
	}
	CHECK_NEAR(Anim::EmergeCurve(1.0f), 1.0, 0.05);
}

TEST(fuzz_anim_never_produces_nan)
{
	unsigned int seed = 777u;
	auto next = [&seed]() { seed = seed * 1103515245u + 12345u; return (seed >> 16) & 0x7fff; };
	auto f = [&next](float lo, float hi) { return lo + (hi - lo) * ((float)next() / 32767.0f); };

	for (int i = 0; i < 20000; i++) {
		const float p = f(-100.0f, 100.0f);
		const Anim::LegPose l = Anim::WalkLeg(p, f(-90.0f, 90.0f));
		CHECK(!std::isnan(l.hip) && !std::isnan(l.knee) && !std::isnan(l.lift));
		CHECK(!std::isnan(Anim::WalkBob(p)));
		CHECK(!std::isnan(Anim::IdleBreath(p)));
		CHECK(!std::isnan(Anim::AttackCurve(f(-2.0f, 3.0f))));
		const Anim::Squash sq = Anim::SquashFromVelocity(f(-1000.0f, 1000.0f));
		CHECK(!std::isnan(sq.x) && !std::isnan(sq.y) && sq.y > 0.0f);
	}
}


/**********************************************************************************
 ******** Fuzz: malformed input must never crash or hang
 **********************************************************************************/

TEST(fuzz_splitfields_never_crashes)
{
	// Deterministic pseudo-random byte soup. Config files are user-editable and a modder
	// will eventually feed this something absurd; the requirement is warn-and-skip, never
	// terminate and never corrupt. There is nothing to corrupt here, so the bar is: it
	// returns, and every field it produces is non-empty.
	unsigned int seed = 12345u;
	auto next = [&seed]() { seed = seed * 1103515245u + 12345u; return (seed >> 16) & 0x7fff; };

	std::vector<std::string> out;
	for (int iter = 0; iter < 20000; iter++) {
		const int len = (int)(next() % 64);
		std::string s;
		for (int i = 0; i < len; i++) {
			// Bias towards separators and printable junk, including quotes and newlines.
			static const char alphabet[] = " \t:,;abcXYZ0129_.-\"'\n\r\\/[]{}#";
			s.push_back(alphabet[next() % (sizeof(alphabet) - 1)]);
		}
		SplitFields(s.c_str(), out);
		for (const std::string& f : out) {
			CHECK(!f.empty());
			CHECK(f.find(' ') == std::string::npos);
			CHECK(f.find(':') == std::string::npos);
			CHECK(f.find(',') == std::string::npos);
			CHECK(f.find('\t') == std::string::npos);
		}
	}
}

TEST(fuzz_wave_maths_never_produces_an_unsafe_answer)
{
	// The invariant that matters: whatever garbage a config supplies, the director must
	// never be told to spawn a negative count, never exceed the alive budget, and never
	// be handed a non-finite interval it would compare a timer against.
	unsigned int seed = 987654321u;
	auto next = [&seed]() { seed = seed * 1103515245u + 12345u; return (seed >> 16) & 0x7fff; };
	auto nextf = [&next](float lo, float hi) {
		return lo + (hi - lo) * ((float)next() / 32767.0f);
	};

	for (int iter = 0; iter < 50000; iter++) {
		WaveTuning t;
		t.intervalSeconds    = nextf(-1000.0f, 10000.0f);
		t.rampSeconds        = nextf(-1000.0f, 10000.0f);
		t.minIntervalSeconds = nextf(-1000.0f, 10000.0f);
		t.size               = (int)next() % 200 - 50;
		t.sizeMax            = (int)next() % 200 - 50;
		t.maxAlive           = (int)next() % 200 - 50;

		const float missionTime = nextf(-10000.0f, 100000.0f);
		const int alive = (int)next() % 100 - 20;

		const float interval = WaveInterval(t, missionTime);
		CHECK(!std::isnan(interval));
		CHECK(!std::isinf(interval));

		const int size = WaveSize(t, (int)next() % 1000, alive);
		CHECK(size >= 0);
		CHECK(size <= (t.sizeMax > 0 ? t.sizeMax : 0));
		if (size > 0 && alive >= 0) CHECK(size + alive <= t.maxAlive);
	}
}


/**********************************************************************************
 ******** Benchmarks
 **********************************************************************************/

double BenchMillis(void (*fn)(void), int iterations)
{
	const auto start = std::chrono::steady_clock::now();
	for (int i = 0; i < iterations; i++) fn();
	const auto end = std::chrono::steady_clock::now();
	return std::chrono::duration<double, std::milli>(end - start).count();
}

void BenchSplitOnce(void)
{
	static std::vector<std::string> out;
	SplitFields("Brute RockMonster 1.45 0.55:0.30:0.30", out);
}

void RunBenchmarks(void)
{
	std::printf("\nBenchmarks (informational -- these measure our own code, not the game)\n");

	const int N = 200000;
	const double ms = BenchMillis(BenchSplitOnce, N);
	std::printf("  SplitFields   %8d calls  %8.2f ms  %8.3f us/call\n",
				N, ms, (ms * 1000.0) / N);

	// The scheduling maths runs once per frame at most; this exists to prove it is
	// nowhere near a frame budget, not because it was ever suspected.
	const auto start = std::chrono::steady_clock::now();
	WaveTuning t;
	volatile float sink = 0.0f;
	const int M = 5000000;
	for (int i = 0; i < M; i++) sink += WaveInterval(t, (float)(i % 4000));
	const auto end = std::chrono::steady_clock::now();
	const double ms2 = std::chrono::duration<double, std::milli>(end - start).count();
	std::printf("  WaveInterval  %8d calls  %8.2f ms  %8.4f us/call\n",
				M, ms2, (ms2 * 1000.0) / M);

	// ---- A1: O(capacity) full walk vs O(alive) dense index ----
	//
	// The measurement docs/PERFORMANCE.md makes mandatory for A1. It models the real shape:
	// 1036-byte items, geometric lists, and the "peaked high, few alive now" case that
	// ListSet's monotonic capacity makes permanent for the rest of a mission.
	{
		std::printf("\n  A1 -- ListSet enumeration, 1036-byte items\n");
		std::printf("  %-32s %10s %10s %10s\n", "case", "walk ms", "index ms", "speedup");

		struct Case { const char* name; std::size_t lists; std::size_t alive; };
		const Case cases[] = {
			{ "cap 255, 200 alive",   8, 200 },
			{ "cap 255, 40 alive",    8,  40 },
			{ "cap 255, 8 alive",     8,   8 },
			{ "cap 1023, 60 alive",  10,  60 },
			{ "cap 4095, 100 alive", 12, 100 },
		};

		for (const Case& c : cases) {
			FakeListSet ls; ls.Grow(c.lists);
			unsigned int bseed = 99991u;
			auto bnext = [&bseed]() { bseed = bseed * 1103515245u + 12345u; return (bseed >> 16) & 0x7fff; };
			std::size_t placed = 0;
			while (placed < c.alive) {
				const std::size_t s2 = bnext() % ls.Capacity();
				if (!FakeListSet::Alive(ls.At(s2))) { ls.SetAlive(s2, true); placed++; }
			}
			Index idx; idx.Reserve(ls.Capacity());
			RebuildFrom(idx, ls);

			const int reps = 2000;
			volatile std::size_t benchSink = 0;

			const auto t0 = std::chrono::steady_clock::now();
			for (int i = 0; i < reps; i++) benchSink += ls.WalkCountAlive();
			const auto t1 = std::chrono::steady_clock::now();
			for (int i = 0; i < reps; i++) {
				std::size_t n = 0;
				for (FakeItem* p : idx.Live()) { if (p->nextFree != p) n++; }
				benchSink += n;
			}
			const auto t2 = std::chrono::steady_clock::now();

			const double walkMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
			const double idxMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
			std::printf("  %-32s %10.2f %10.2f %9.1fx\n", c.name, walkMs, idxMs,
						idxMs > 0.0 ? walkMs / idxMs : 0.0);
		}
	}

	std::printf("\n  NOTE: harness numbers on this machine, over a synthetic item of the same size\n"
				"        and layout as the real one. A real measurement of the ALGORITHM; it says\n"
				"        nothing about in-game frame cost, which cannot be measured from here.\n");
}

} // namespace


/**********************************************************************************
 ******** Entry point
 **********************************************************************************/

int main(int argc, char** argv)
{
	bool runBench = true;
	for (int i = 1; i < argc; i++) {
		if (std::strcmp(argv[i], "--no-bench") == 0) runBench = false;
	}

	std::printf("DeepCoreOverhaul harness -- pure-logic tests\n");
	std::printf("(compile-verified logic only; this is NOT play-testing)\n\n");

	for (const TestCase& tc : Registry()) {
		g_currentTest = tc.name;
		const int before = g_failures;
		tc.fn();
		if (g_failures == before) std::printf("  ok    %s\n", tc.name);
	}

	std::printf("\n%d checks in %d tests, %d failure(s)\n",
				g_checks, (int)Registry().size(), g_failures);

	if (runBench && g_failures == 0) RunBenchmarks();

	return g_failures;
}
