// WaveDirector.cpp : Timed monster waves, independent of a map's own emerge triggers.
//

#include <vector>

#include "../engine/core/Errors.h"
#include "../engine/core/Maths.h"

#include "object/Object.h"
#include "object/AITask.h"
#include "world/Camera.h"
#include "interface/InfoMessages.h"
#include "Game.h"
#include "DeepCoreLogic.hpp"
#include "DeepCoreAudio.hpp"
#include "DeepCore.hpp"
#include "WaveDirector.hpp"


/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define Waves_WarnF(b, s, ...)	Error_WarnF2( (b), "DeepCore/Waves: %s\n", Gods98::Error_Format((s), __VA_ARGS__))
#define Waves_LogF(s, ...)		Error_DebugF2("DeepCore/Waves: %s\n", Gods98::Error_Format((s), __VA_ARGS__))

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

namespace
{

/// What the director is currently doing.
enum class Phase
{
	Waiting,	///< counting down to the next wave
	Telegraph,	///< the player has been warned; creatures land when this expires
};

struct State
{
	Phase  phase          = Phase::Waiting;
	real32 timer          = 0.0f;	///< world-seconds accumulated in the current phase
	real32 missionTime    = 0.0f;	///< world-seconds since the level started
	sint32 waveNumber     = 0;		///< how many waves have landed this level
	sint32 spawnedTotal   = 0;		///< creatures this director has spawned this level

	/// Blocks chosen for the pending wave, fixed at telegraph time so the warning and
	/// the arrival agree. A player who is told where to look and then ambushed
	/// somewhere else has been lied to.
	std::vector<Point2I> pendingBlocks;

	/// Edge detector for "the cavern just went quiet", so the all-clear plays once on
	/// the transition rather than every frame that nothing is alive.
	DeepCore::Logic::QuietDetector quiet;
};

State _s;

} // namespace

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

void DeepCore::Waves::Reset(void)
{
	_s = State{};
}


sint32 DeepCore::Waves::SpawnedThisLevel(void)
{
	return _s.spawnedTotal;
}


namespace
{

/// Count creatures that are alive at all, INCLUDING ones still climbing out of a wall.
///
/// Deliberately not LegoObject_IsActive: that returns false for the whole duration of
/// LIVEOBJ1_EXPANDING (Object.cpp:1283-1284), i.e. the entire emerge animation. A budget
/// built on it would let the director fire a second wave while the first is still
/// emerging, which is exactly how "hard but fair" becomes "unplayable".
sint32 CountLiveMonsters(void)
{
	using namespace LegoRR;

	sint32 count = 0;
	for (LegoObject* obj : objectListSet.EnumerateSkipUpgradeParts()) {
		if (obj->type != LegoObject_RockMonster) continue;
		if (obj->health < 0.0f)                  continue;
		if (obj->flags1 & LIVEOBJ1_CRUMBLING)    continue; // already dying
		count++;
	}
	return count;
}


/// Are there any Rock Raiders left? If not, the level is already lost and piling on is
/// just noise.
sint32 CountMiniFigures(void)
{
	using namespace LegoRR;

	sint32 count = 0;
	for (LegoObject* obj : objectListSet.EnumerateSkipUpgradeParts()) {
		if (obj->type != LegoObject_MiniFigure) continue;
		if (obj->health < 0.0f)                 continue;
		count++;
	}
	return count;
}


/// Is this a block a creature may legally and fairly arrive on?
///
/// Reads block flags directly rather than going through the Level_Block_* family. Those
/// are almost all still exe address macros (Game.h:1741-1829); the flag tests are visible
/// in our own source, reason-about-able, and avoid the macro/namespace trap at every call
/// site. The codebase already documents the equivalence inline at Game.cpp:3749 and :3767.
bool IsFairSpawnBlock(LegoRR::Lego_Level* level, sint32 bx, sint32 by)
{
	using namespace LegoRR;

	if (bx <= 0 || by <= 0 || bx >= (sint32)level->width - 1 || by >= (sint32)level->height - 1) {
		return false; // keep a one-tile margin so neighbour checks never leave the map
	}

	const BlockFlags1 f1 = blockValue(level, bx, by).flags1;
	const BlockFlags2 f2 = blockValue(level, bx, by).flags2;

	// Must be walkable ground the player has actually uncovered.
	if (!(f1 & BLOCK1_FLOOR))    return false;
	if (f1 & BLOCK1_HIDDEN)      return false; // inside an unopened cavern

	// Never inside the player's base. Being ambushed on top of your own Tool Store is
	// not difficulty, it is a broken promise -- the base is the one place a player is
	// entitled to feel safe until something walks there.
	if (f2 & BLOCK2_TOOLSTORE)          return false;
	if (f1 & BLOCK1_BUILDINGSOLID)      return false;
	if (f1 & BLOCK1_FOUNDATION)         return false;
	if (f1 & BLOCK1_BUILDINGPATH)       return false;
	if (f1 & (BLOCK1_PATH|BLOCK1_LAYEDPATH)) return false;

	// Do not drop a creature on a tile something is already working on.
	if (f1 & (BLOCK1_BUSY_FLOOR|BLOCK1_BUSY_WALL)) return false;

	// Arrive at a wall face, the way the game's own emerges do -- a creature that
	// materialises in open floor reads as a bug rather than a threat.
	bool adjacentWall = false;
	static const sint32 DX[4] = { 1, -1, 0, 0 };
	static const sint32 DY[4] = { 0, 0, 1, -1 };
	for (uint32 d = 0; d < 4; d++) {
		const BlockFlags1 nf1 = blockValue(level, bx + DX[d], by + DY[d]).flags1;
		if ((nf1 & BLOCK1_WALL) && !(nf1 & BLOCK1_HIDDEN)) {
			adjacentWall = true;
			break;
		}
	}
	return adjacentWall;
}


/// Squared distance in blocks, to avoid a sqrt in a scan loop.
sint32 DistSq(sint32 ax, sint32 ay, sint32 bx, sint32 by)
{
	const sint32 dx = ax - bx, dy = ay - by;
	return dx * dx + dy * dy;
}


/// Collect every legal spawn block that is far enough from the player's buildings.
void GatherCandidates(LegoRR::Lego_Level* level, sint32 minDistBlocks, std::vector<Point2I>& out)
{
	using namespace LegoRR;

	out.clear();

	// Positions of the player's buildings, so we can enforce a standoff distance.
	std::vector<Point2I> bases;
	for (LegoObject* obj : objectListSet.EnumerateSkipUpgradeParts()) {
		if (obj->type != LegoObject_Building) continue;
		if (obj->health < 0.0f)               continue;
		Point2I bp = { 0, 0 };
		LegoObject_GetBlockPos(obj, &bp.x, &bp.y);
		bases.push_back(bp);
	}

	const sint32 minSq = minDistBlocks * minDistBlocks;

	for (sint32 by = 1; by < (sint32)level->height - 1; by++) {
		for (sint32 bx = 1; bx < (sint32)level->width - 1; bx++) {
			if (!IsFairSpawnBlock(level, bx, by)) continue;

			bool tooClose = false;
			for (const Point2I& b : bases) {
				if (DistSq(bx, by, b.x, b.y) < minSq) { tooClose = true; break; }
			}
			if (tooClose) continue;

			out.push_back(Point2I{ bx, by });
		}
	}
}


/// Spawn one creature at a block, mirroring the proven from-scratch path in
/// LegoObject_TryGenerateSlugAtBlock (Object.cpp:1602-1639) minus its slug-hole gate.
bool SpawnMonsterAtBlock(sint32 bx, sint32 by, sint32 objID)
{
	using namespace LegoRR;

	if ((uint32)objID >= legoGlobs.rockMonsterCount ||
		(uint32)objID >= (uint32)LegoObject_ID_Count)
	{
		return false; // never index rockMonsterData out of range
	}

	ObjectModel* objModel = (ObjectModel*)&legoGlobs.rockMonsterData[objID];

	const real32 heading = Gods98::Maths_RandRange(0.0f, M_PI * 2.0f);

	Point2F wPos = { 0.0f, 0.0f };
	Map3D_BlockToWorldPos(Lego_GetMap(), bx, by, &wPos.x, &wPos.y);

	LegoObject* obj = LegoObject_CreateInWorld(objModel, LegoObject_RockMonster,
											   (LegoObject_ID)objID, 0,
											   wPos.x, wPos.y, heading);
	if (obj == nullptr) {
		return false;
	}

	obj->flags1 |= LIVEOBJ1_EXPANDING;
	obj->flags3 &= ~LIVEOBJ3_POWEROFF;

	/// Set a real target block. Objects are created with targetBlockPos == (-1,-1)
	/// (Object.cpp:1163-1164), and the emerge-completion branch feeds that value to
	/// Level_Block_SetBusy (Object.cpp:3214-3227) -- at (-1,-1) that is a negative index
	/// into the block array. Same defect fixed for the slug path in Object.cpp:1632.
	obj->targetBlockPos.x = (real32)bx;
	obj->targetBlockPos.y = (real32)by;

	LegoObject_SetActivity(obj, Activity_Emerge, 0);
	LegoObject_UpdateActivityChange(obj);
	AITask_DoAnimationWait(obj);

	return true;
}


/// Warn the player, at the blocks the wave will actually arrive on.
void Telegraph(const std::vector<Point2I>& blocks)
{
	using namespace LegoRR;

	if (blocks.empty()) return;

	// A locatable panel message: Info_Send takes a block position, so the player can
	// click through to where it is rather than being told "something, somewhere".
	Point2I where = blocks[0];
	Info_Send(Info_RockMonster, nullptr, nullptr, &where);

	if (DeepCore::settings.waveShakeIntensity > 0.0f) {
		Camera_Shake(legoGlobs.cameraMain,
					 DeepCore::settings.waveShakeIntensity,
					 DeepCore::settings.waveShakeDuration);
	}
}

} // namespace


/// Snapshot the live settings into the pure tuning struct the scheduling maths uses.
/// Keeping this the ONLY place settings are read for scheduling means the harness tests
/// exactly the arithmetic that ships.
static DeepCore::Logic::ThreatAudioTuning CurrentAudioTuning(void)
{
	DeepCore::Logic::ThreatAudioTuning t;
	t.heavyWaveSize        = DeepCore::settings.threatHeavyWaveSize;
	t.escalateEveryNWaves  = DeepCore::settings.threatEscalateEveryNWaves;
	return t;
}


static DeepCore::Logic::WaveTuning CurrentTuning(void)
{
	DeepCore::Logic::WaveTuning t;
	t.intervalSeconds    = DeepCore::settings.waveIntervalSeconds;
	t.rampSeconds        = DeepCore::settings.waveRampSeconds;
	t.minIntervalSeconds = DeepCore::settings.waveMinIntervalSeconds;
	t.size               = DeepCore::settings.waveSize;
	t.sizeMax            = DeepCore::settings.waveSizeMax;
	t.maxAlive           = DeepCore::settings.waveMaxAlive;
	return t;
}


void DeepCore::Waves::Update(real32 elapsedReal)
{
	using namespace LegoRR;

	if (!settings.waveDirector) return;

	// Lego_IsInLevel covers all four hazards at once: engine not initialised, no current
	// level, the level's first tick not yet processed, and the level ending
	// (Game.h:812). Without it the director would tick during front-end menus, which
	// Lego_EndLevel runs from inside Lego_MainLoop.
	if (!Lego_IsInLevel()) return;

	// Paused means paused. Advancing on raw real time would keep waves coming while the
	// player sits in a menu.
	if (legoGlobs.flags1 & GAME1_FREEZEINTERFACE) return;

	// World time, not interface time. Note the standing complaint at GameState.cpp:955
	// that the game's OWN emerge triggers use interface time; this deliberately differs,
	// and differs in the better direction -- a slowed game should threaten more slowly.
	const real32 elapsedWorld = elapsedReal * Lego_GetGameSpeed();

	// Standard units: 25.0 == one second (STANDARD_FRAMERATE, common.h). The engine
	// already hard-caps elapsed at 3.0 per tick (Main.cpp:438-443), so a long stall
	// cannot dump several waves at once and the director needs no clamp of its own.
	const real32 seconds = elapsedWorld / STANDARD_FRAMERATE;

	_s.missionTime += seconds;
	_s.timer       += seconds;

	// Feed the quiet detector every tick so the all-clear fires on the EDGE. This reuses
	// the same enumeration the budget needs; see PERFORMANCE.md on ListSet cost, which is
	// why this is one walk rather than two.
	if (settings.threatAudio) {
		DeepCore::Audio::PlayThreatCue(_s.quiet.Update(CountLiveMonsters()));
	}

	if (_s.phase == Phase::Telegraph) {
		if (_s.timer < settings.waveTelegraphSeconds) return;

		sint32 landed = 0;
		for (const Point2I& b : _s.pendingBlocks) {
			const sint32 objID = PickWaveSpecies(_s.waveNumber, landed);
			if (objID >= 0 && SpawnMonsterAtBlock(b.x, b.y, objID)) {
				landed++;
			}
		}

		if (landed > 0) {
			DeepCore::Audio::PlayThreatCue(DeepCore::Logic::ThreatCue::Arrival);
		}

		_s.spawnedTotal += landed;
		_s.waveNumber++;
		_s.pendingBlocks.clear();
		_s.phase = Phase::Waiting;
		_s.timer = 0.0f;

		if (settings.verboseStartup) {
			Waves_LogF("wave %i landed: %i creature(s), %i alive, %i spawned this level",
					   _s.waveNumber, landed, CountLiveMonsters(), _s.spawnedTotal);
		}
		return;
	}

	// ---- Waiting -------------------------------------------------------------

	// Waves come faster as the mission runs on, floored so it never becomes a stream.
	// The arithmetic lives in DeepCoreLogic.hpp so tools/harness/ can exercise it without
	// the game -- this file only reads state and applies the answer.
	const DeepCore::Logic::WaveTuning tuning = CurrentTuning();
	const real32 interval = DeepCore::Logic::WaveInterval(tuning, _s.missionTime);

	if (_s.timer < interval) return;

	_s.timer = 0.0f;

	// Do not kick a level that is already over.
	if (CountMiniFigures() == 0) return;

	// The budget is a ceiling on TOTAL creature pressure, not on director-spawned
	// creatures, so the director never stacks on top of a map that is already hostile.
	const sint32 alive = CountLiveMonsters();
	if (alive >= settings.waveMaxAlive) return;

	std::vector<Point2I> candidates;
	GatherCandidates(legoGlobs.currLevel, settings.waveMinDistanceFromBase, candidates);
	if (candidates.empty()) {
		// Not an error: a sealed or fully-built-out map legitimately has nowhere fair.
		return;
	}

	// Wave size grows with the wave number, clipped by the remaining alive budget and by
	// the per-wave ceiling. Also in DeepCoreLogic.hpp, and covered by the harness.
	const sint32 size = DeepCore::Logic::WaveSize(tuning, _s.waveNumber, alive);
	if (size < 1) return;

	_s.pendingBlocks.clear();
	for (sint32 i = 0; i < size; i++) {
		const sint32 pick = (sint32)Gods98::Maths_RandRange(0.0f, (real32)candidates.size());
		_s.pendingBlocks.push_back(candidates[(size_t)pick % candidates.size()]);
	}

	Telegraph(_s.pendingBlocks);

	/// DEEPCORE: the audio layer. Which cue belongs to this moment is decided in
	/// DeepCoreLogic.hpp so the harness can test it; this only plays the answer.
	{
		const DeepCore::Logic::ThreatAudioTuning at = CurrentAudioTuning();
		DeepCore::Audio::PlayThreatCue(DeepCore::Logic::TelegraphCueFor(at, size));
		if (DeepCore::Logic::ShouldEscalate(at, _s.waveNumber)) {
			DeepCore::Audio::PlayThreatCue(DeepCore::Logic::ThreatCue::Escalate);
		}
	}

	_s.phase = Phase::Telegraph;

	if (settings.verboseStartup) {
		Waves_LogF("wave %i telegraphed: %i creature(s), %i candidate block(s), %.1fs warning",
				   _s.waveNumber + 1, (sint32)_s.pendingBlocks.size(),
				   (sint32)candidates.size(), settings.waveTelegraphSeconds);
	}
}

#pragma endregion
