// DeepCore3D.cpp : the subterranean mining game, in 3D.
//
// WHAT THIS IS
// ------------
// A real 3D game -- perspective camera you orbit and zoom, lit voxel terrain, models built
// from geometry, mouse picking through an unprojected ray. It is driven by the SAME
// DeepCoreLogic.hpp that the OpenLRR modification calls, so the wave cadence, the alive
// budget, the spawn-fairness rules and the all-clear edge detector are literally the
// shipping code, not a reimplementation of them.
//
// WHAT IT IS NOT, and this matters
// --------------------------------
// It is not LEGO Rock Raiders. That is a 1999 commercial product and nothing here contains
// or reproduces any part of it.
//
// It is also not Manic Miners, and will not look like it. Manic Miners is four-plus years
// of solo development in Unreal Engine with authored art; this is procedural geometry and
// flat colours. If you want a faithful, finished Rock Raiders remake, play that -- it is
// free and it is excellent. What this offers instead is a game whose systems you can read,
// change, and watch change, because every one of them lives in this repository.
//
// DEPENDENCIES: none beyond the Windows SDK. OpenGL 1.x through opengl32/glu32, which ship
// with Windows, so it builds with the same v142 toolset as everything else.
//
//   Left drag        orbit the camera
//   Right click      order the selection: walk, or drill a wall
//   Left click       select a miner
//   Wheel            zoom
//   WASD             pan
//   Space pause   R new cavern   Esc quit
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "../openlrr/game/DeepCoreLogic.hpp"
#include "../sandbox/SyntheticLevel.hpp"
#include "Anim.hpp"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "winmm.lib")

using namespace DeepCore::Logic;
using namespace Sandbox;

namespace
{

const float DRILL_TIME  = 2.2f;
const float MINER_SPEED = 3.0f;
const float MON_SPEED   = 1.6f;
const float MON_DPS     = 14.0f;
const float MINER_DPS   = 24.0f;
const int   CRYSTAL_GOAL = 12;

struct V3 { float x = 0, y = 0, z = 0; };

float Dist2D(const V3& a, const V3& b)
{
	const float dx = a.x - b.x, dz = a.z - b.z;
	return std::sqrt(dx * dx + dz * dz);
}

struct Miner
{
	V3 pos, target;
	bool selected = false, hasTarget = false, drilling = false;
	int drillX = 0, drillZ = 0;
	float drillProgress = 0.0f, health = 100.0f, facing = 0.0f;

	// --- animation state ---
	float gait = 0.0f;          ///< walk-cycle phase, advanced by DISTANCE not by time, so
	                            ///  the feet cannot skate however fast the miner moves
	float speed = 0.0f;         ///< smoothed, drives the walk/idle blend
	float lifeT = 0.0f;         ///< personal clock, offset per miner so nobody syncs
	Anim::Spring headYaw;       ///< lags the body, then settles
	Anim::Spring lean;          ///< leans into acceleration
	Anim::Spring toolRaise;     ///< the drill comes up and down with weight
};

struct Monster
{
	V3 pos, wander;
	int species = 0;
	float health = 100.0f, scale = 1.0f, emerge = 0.0f;

	float gait = 0.0f, speed = 0.0f, lifeT = 0.0f;
	float attackT = -1.0f;      ///< >=0 while an attack is playing; negative when idle
	float hitFlash = 0.0f;      ///< recoil timer, so a hit is felt rather than just counted
	Anim::Spring headYaw;
	Anim::Spring bodyTilt;
};

struct Marker { int x = 0, z = 0; float remaining = 0.0f; };

struct Game
{
	Level level;
	std::vector<Miner> miners;
	std::vector<Monster> monsters;
	std::vector<Marker> markers;

	WaveTuning wave;
	ThreatAudioTuning audioCfg;
	QuietDetector quiet;
	Rng rng{ 1 };

	float time = 0, waveTimer = 0;
	int waveNumber = 0, crystals = 0;
	bool paused = false, won = false, lost = false;

	// Camera: orbit around a focus point on the cavern floor.
	float camYaw = 0.7f, camPitch = 0.95f, camDist = 34.0f;
	V3 camFocus;

	std::string banner;
	float bannerT = 0;

	void Say(const std::string& s, float t = 3.5f) { banner = s; bannerT = t; }

	bool Walkable(int x, int z) const
	{
		if (!level.InBounds(x, z)) return false;
		const Block& b = level.At(x, z);
		return b.Has(BLOCK_FLOOR) && !b.Has(BLOCK_WATER);
	}
	bool Solid(int x, int z) const
	{
		if (!level.InBounds(x, z)) return true;
		const Block& b = level.At(x, z);
		return !b.Has(BLOCK_FLOOR) && !b.Has(BLOCK_WATER);
	}

	SpawnBlockInfo Describe(int x, int z) const
	{
		SpawnBlockInfo i;
		const Block& b = level.At(x, z);
		i.isFloor     = b.Has(BLOCK_FLOOR) && !b.Has(BLOCK_WATER);
		i.isHidden    = b.Has(BLOCK_HIDDEN);
		i.isToolStore = b.Has(BLOCK_TOOLSTORE);
		i.isBuilding  = b.Has(BLOCK_BUILDING);
		i.isPath      = b.Has(BLOCK_PATH);
		static const int DX[4] = { 1,-1,0,0 }, DZ[4] = { 0,0,1,-1 };
		for (int d = 0; d < 4; d++) {
			const int nx = x + DX[d], nz = z + DZ[d];
			if (!level.InBounds(nx, nz)) continue;
			const Block& n = level.At(nx, nz);
			if (n.Has(BLOCK_WALL) && !n.Has(BLOCK_HIDDEN)) { i.hasAdjacentExposedWall = true; break; }
		}
		int best = 1 << 30;
		for (const Miner& m : miners) {
			const int dx = x - (int)m.pos.x, dz = z - (int)m.pos.z;
			const int d2 = dx * dx + dz * dz;
			if (d2 < best) best = d2;
		}
		i.distSqToNearestBuilding = best;
		return i;
	}

	void NewLevel(std::uint32_t seed);
	void Update(float dt);
	void CommandTo(int tx, int tz);
};

Game g;


void Game::NewLevel(std::uint32_t seed)
{
	rng = Rng(seed);
	LevelDesc d;
	d.seed = seed;
	d.width = 40; d.height = 40;
	d.caverns = 8;
	d.waterFraction = 0.05f;
	d.oreSeams = 14;
	d.crystalSeams = 22;
	level.Generate(d);

	miners.clear(); monsters.clear(); markers.clear();
	crystals = 0; time = waveTimer = 0; waveNumber = 0;
	won = lost = false; quiet.Reset();

	int sx = 0, sz = 0;
	for (int z = 0; z < level.Height() && !sz; z++)
		for (int x = 0; x < level.Width(); x++)
			if (level.At(x, z).Has(BLOCK_TOOLSTORE)) { sx = x; sz = z; break; }

	for (int i = 0; i < 5; i++) {
		Miner m;
		m.pos = { (float)sx + (float)(i % 3) - 1.0f, 0, (float)sz + (float)(i / 3) - 0.5f };
		m.lifeT = (float)i * 1.37f;   // offset so the crew never breathes in unison
		miners.push_back(m);
	}

	wave.intervalSeconds = 45.0f;
	wave.rampSeconds = 240.0f;
	wave.minIntervalSeconds = 15.0f;
	wave.size = 1; wave.sizeMax = 5; wave.maxAlive = 7;

	camFocus = { (float)sx, 0, (float)sz };
	Say("Drill the walls. Purple seams are energy crystals -- bring back " +
		std::to_string(CRYSTAL_GOAL) + ".", 7.0f);
}


void Game::CommandTo(int tx, int tz)
{
	if (!level.InBounds(tx, tz)) return;
	const bool wall = Solid(tx, tz) && !level.At(tx, tz).Has(BLOCK_HIDDEN);
	for (Miner& m : miners) {
		if (!m.selected) continue;
		if (wall) {
			m.drilling = true; m.drillX = tx; m.drillZ = tz;
			m.drillProgress = 0; m.hasTarget = true;
			m.target = { (float)tx, 0, (float)tz };
			static const int DX[4] = { 1,-1,0,0 }, DZ[4] = { 0,0,1,-1 };
			for (int d = 0; d < 4; d++)
				if (Walkable(tx + DX[d], tz + DZ[d])) {
					m.target = { (float)(tx + DX[d]), 0, (float)(tz + DZ[d]) }; break;
				}
		}
		else if (Walkable(tx, tz)) {
			m.drilling = false; m.hasTarget = true;
			m.target = { (float)tx, 0, (float)tz };
		}
	}
}


void Game::Update(float dt)
{
	if (paused || won || lost) return;
	time += dt;
	if (bannerT > 0) bannerT -= dt;

	for (Miner& m : miners) {
		m.lifeT += dt;
		const V3 was = m.pos;
		const float d = Dist2D(m.pos, m.target);
		if (m.hasTarget && d > 0.08f) {
			m.facing = std::atan2(m.target.x - m.pos.x, m.target.z - m.pos.z);
			const float s = MINER_SPEED * dt;
			m.pos.x += (m.target.x - m.pos.x) / d * s;
			m.pos.z += (m.target.z - m.pos.z) / d * s;
		}
		else if (m.hasTarget && m.drilling) {
			m.drillProgress += dt;
			if (m.drillProgress >= DRILL_TIME) {
				Block& b = level.At(m.drillX, m.drillZ);
				const bool crystal = b.Has(BLOCK_CRYSTAL_SEAM);
				const bool ore = b.Has(BLOCK_ORE_SEAM);
				b.flags = BLOCK_FLOOR;
				level.Discover(m.drillX, m.drillZ, 3);
				level.RecomputeWalls();
				if (crystal) {
					crystals++;
					Say("Crystal recovered: " + std::to_string(crystals) + " / " +
						std::to_string(CRYSTAL_GOAL), 2.5f);
					::Beep(1200, 60);
					if (crystals >= CRYSTAL_GOAL) { won = true; Say("ALL CRYSTALS RECOVERED.", 999.0f); }
				}
				else if (ore) Say("Ore seam cleared.", 1.5f);
				m.drilling = false; m.hasTarget = false;
			}
		}
		else if (m.hasTarget) m.hasTarget = false;

		for (Monster& mo : monsters) {
			if (mo.emerge < 1.0f) continue;
			if (Dist2D(m.pos, mo.pos) < 0.9f) {
				mo.health -= MINER_DPS * dt;
				m.health  -= MON_DPS * dt;
				mo.hitFlash = 0.18f;
				if (mo.attackT < 0.0f) mo.attackT = 0.0f;    // wind up before it lands
			}
		}

		// --- animation drive ---
		//
		// The gait advances by DISTANCE TRAVELLED, not by elapsed time. That is the whole
		// trick behind feet that do not skate: however fast the miner moves, one stride of
		// cycle corresponds to one stride of ground, so a planted foot stays planted.
		const float moved = Dist2D(was, m.pos);
		const float instSpeed = (dt > 0.0f) ? moved / dt : 0.0f;
		m.speed += (instSpeed - m.speed) * (1.0f - std::exp(-8.0f * dt));
		const float STRIDE = 0.85f;
		m.gait = Anim::Wrap01(m.gait + moved / STRIDE);

		// Head lags the body and settles, rather than snapping with it.
		m.headYaw.stiffness = 55.0f; m.headYaw.damping = 11.0f;
		m.headYaw.Step(m.facing, dt);

		// Lean into acceleration; upright when idle.
		m.lean.stiffness = 40.0f; m.lean.damping = 9.0f;
		m.lean.Step(m.speed * 0.045f, dt);

		// The drill rises with weight and drops when the work stops.
		m.toolRaise.stiffness = 70.0f; m.toolRaise.damping = 10.0f;
		m.toolRaise.Step(m.drilling && !m.hasTarget ? 1.0f : (m.drilling ? 0.4f : 0.0f), dt);
	}

	for (std::size_t i = 0; i < miners.size(); ) {
		if (miners[i].health <= 0) { miners.erase(miners.begin() + (long)i); Say("We lost one.", 3.0f); }
		else i++;
	}
	if (miners.empty()) { lost = true; Say("THE CREW IS GONE.", 999.0f); }

	for (Monster& mo : monsters) {
		mo.lifeT += dt;
		if (mo.hitFlash > 0.0f) mo.hitFlash -= dt;
		if (mo.attackT >= 0.0f) { mo.attackT += dt / 0.75f; if (mo.attackT > 1.0f) mo.attackT = -1.0f; }
		const V3 wasM = mo.pos;
		if (mo.emerge < 1.0f) { mo.emerge += dt / 1.6f; continue; }

		const Miner* best = nullptr; float bd = 1e9f;
		for (const Miner& m : miners) { const float d = Dist2D(m.pos, mo.pos); if (d < bd) { bd = d; best = &m; } }

		V3 want = mo.wander;
		if (best && bd < 15.0f) want = best->pos;
		else if (rng.Chance(0.8f * dt))
			mo.wander = { mo.pos.x + rng.Unit() * 8 - 4, 0, mo.pos.z + rng.Unit() * 8 - 4 };

		const float d = Dist2D(mo.pos, want);
		if (d > 0.1f) {
			const float s = MON_SPEED * (mo.species == 2 ? 1.3f : 1.0f) * dt;
			const float nx = mo.pos.x + (want.x - mo.pos.x) / d * s;
			const float nz = mo.pos.z + (want.z - mo.pos.z) / d * s;
			if (Walkable((int)nx, (int)mo.pos.z)) mo.pos.x = nx;
			if (Walkable((int)mo.pos.x, (int)nz)) mo.pos.z = nz;
		}

		const float movedM = Dist2D(wasM, mo.pos);
		mo.speed += ((dt > 0.0f ? movedM / dt : 0.0f) - mo.speed) * (1.0f - std::exp(-7.0f * dt));
		mo.gait = Anim::Wrap01(mo.gait + movedM / (0.72f * mo.scale));

		const float faceTo = std::atan2(want.x - mo.pos.x, want.z - mo.pos.z);
		mo.headYaw.stiffness = 34.0f; mo.headYaw.damping = 9.0f;
		mo.headYaw.Step(faceTo, dt);
		mo.bodyTilt.stiffness = 26.0f; mo.bodyTilt.damping = 7.0f;
		mo.bodyTilt.Step(mo.speed * 0.10f, dt);
	}
	for (std::size_t i = 0; i < monsters.size(); ) {
		if (monsters[i].health <= 0) monsters.erase(monsters.begin() + (long)i); else i++;
	}

	for (std::size_t i = 0; i < markers.size(); ) {
		markers[i].remaining -= dt;
		if (markers[i].remaining <= 0) {
			Monster mo;
			mo.pos = { (float)markers[i].x, 0, (float)markers[i].z };
			mo.wander = mo.pos;
			mo.species = (int)RotationIndex(waveNumber + (int)i, 3);
			mo.scale = 0.75f + rng.Unit() * 0.8f;   // the variant system, in 3D
			monsters.push_back(mo);
			markers.erase(markers.begin() + (long)i);
			::Beep(70, 90);
		} else i++;
	}

	if (quiet.Update((int)monsters.size()) == ThreatCue::Cleared)
		Say("Cavern's quiet again.", 3.0f);

	waveTimer += dt;
	if (waveTimer >= WaveInterval(wave, time) && markers.empty()) {
		waveTimer = 0;
		const int size = WaveSize(wave, waveNumber, (int)monsters.size());
		if (size > 0) {
			std::vector<std::pair<int,int>> cand;
			for (int z = 1; z < level.Height() - 1; z++)
				for (int x = 1; x < level.Width() - 1; x++)
					if (IsFairSpawn(Describe(x, z), 5)) cand.push_back({ x, z });
			if (!cand.empty()) {
				for (int i = 0; i < size; i++) {
					const auto& p = cand[rng.Below((std::uint32_t)cand.size())];
					markers.push_back({ p.first, p.second, 5.0f });
				}
				waveNumber++;
				Say(size >= audioCfg.heavyWaveSize ? "MULTIPLE CONTACTS INBOUND."
												   : "Movement in the rock.", 4.0f);
				::Beep(200, 130);
			}
		}
	}
}


// --------------------------------------------------------------------------
// Rendering
// --------------------------------------------------------------------------

void Cube(float cx, float cy, float cz, float sx, float sy, float sz,
		  float r, float gr, float b)
{
	const float x0 = cx - sx, x1 = cx + sx;
	const float y0 = cy,      y1 = cy + sy * 2;
	const float z0 = cz - sz, z1 = cz + sz;

	// Per-face shading fakes a light without needing normals or a light rig, and keeps
	// every face readable no matter how the camera is turned.
	struct F { float n[3]; float v[4][3]; float k; };
	const F faces[6] = {
		{ {0,1,0},  {{x0,y1,z0},{x1,y1,z0},{x1,y1,z1},{x0,y1,z1}}, 1.00f }, // top
		{ {0,-1,0}, {{x0,y0,z1},{x1,y0,z1},{x1,y0,z0},{x0,y0,z0}}, 0.35f },
		{ {0,0,-1}, {{x0,y0,z0},{x1,y0,z0},{x1,y1,z0},{x0,y1,z0}}, 0.72f },
		{ {0,0,1},  {{x1,y0,z1},{x0,y0,z1},{x0,y1,z1},{x1,y1,z1}}, 0.60f },
		{ {-1,0,0}, {{x0,y0,z1},{x0,y0,z0},{x0,y1,z0},{x0,y1,z1}}, 0.82f },
		{ {1,0,0},  {{x1,y0,z0},{x1,y0,z1},{x1,y1,z1},{x1,y1,z0}}, 0.50f },
	};
	glBegin(GL_QUADS);
	for (const F& f : faces) {
		glColor3f(r * f.k, gr * f.k, b * f.k);
		for (int i = 0; i < 4; i++) glVertex3fv(f.v[i]);
	}
	glEnd();
}

void DrawMiner(const Miner& m, float t)
{
	// Blend idle against walk on smoothed speed, so starting and stopping is a transition
	// rather than a switch between two states.
	const float w = Anim::Clamp(m.speed / MINER_SPEED, 0.0f, 1.0f);

	const Anim::LegPose L = Anim::WalkLeg(m.gait);
	const Anim::LegPose R = Anim::WalkLeg(m.gait + 0.5f);   // half a cycle out of phase
	const float bob   = Anim::WalkBob(m.gait) * w + Anim::IdleBreath(m.lifeT) * (1.0f - w);
	const float sway  = Anim::WalkSway(m.gait) * w;
	const float armL  =  Anim::WalkArm(m.gait) * w;
	const float armR  = -Anim::WalkArm(m.gait) * w;
	const float shift = Anim::IdleShift(m.lifeT) * (1.0f - w) * 3.0f;

	glPushMatrix();
	glTranslatef(m.pos.x, 0, m.pos.z);
	glRotatef(m.facing * 57.2958f, 0, 1, 0);

	if (m.selected) {
		glColor3f(0.25f, 0.95f, 0.4f);
		glBegin(GL_LINE_LOOP);
		for (int i = 0; i < 28; i++) {
			const float a = (float)i / 28.0f * Anim::TAU;
			glVertex3f(std::cos(a) * 0.55f, 0.03f, std::sin(a) * 0.55f);
		}
		glEnd();
	}

	glTranslatef(sway, bob, 0.0f);
	glRotatef(m.lean.value * 57.2958f, 1, 0, 0);   // lean into travel
	glRotatef(shift, 0, 0, 1);                      // idle weight shift

	// Legs. Hip rotation applies at the hip and the knee bend BELOW it, so the shin follows
	// the thigh instead of both pivoting from the body.
	for (int side = 0; side < 2; side++) {
		const Anim::LegPose& P = side ? R : L;
		glPushMatrix();
		glTranslatef(side ? 0.10f : -0.10f, 0.30f, 0.0f);
		glRotatef(P.hip, 1, 0, 0);
		Cube(0, -0.15f, 0, 0.055f, 0.075f, 0.055f, 0.20f, 0.22f, 0.28f);
		glTranslatef(0, -0.15f, 0);
		glRotatef(-P.knee, 1, 0, 0);
		Cube(0, -0.15f, 0, 0.05f, 0.075f, 0.05f, 0.17f, 0.19f, 0.24f);
		glTranslatef(0, -0.15f, 0);
		Cube(0, -0.03f + P.lift, 0.02f, 0.06f, 0.025f, 0.075f, 0.12f, 0.12f, 0.14f);
		glPopMatrix();
	}

	Cube(0, 0.30f, 0, 0.155f, 0.145f, 0.11f, 0.24f, 0.46f, 0.86f);
	Cube(0, 0.29f, 0.09f, 0.10f, 0.075f, 0.03f, 0.92f, 0.74f, 0.20f);

	for (int side = 0; side < 2; side++) {
		glPushMatrix();
		glTranslatef(side ? 0.20f : -0.20f, 0.53f, 0.0f);
		glRotatef(side ? armR : armL, 1, 0, 0);
		glRotatef(side ? -8.0f : 8.0f, 0, 0, 1);
		Cube(0, -0.13f, 0, 0.045f, 0.11f, 0.045f, 0.24f, 0.46f, 0.86f);
		Cube(0, -0.26f, 0, 0.05f, 0.035f, 0.05f, 0.90f, 0.72f, 0.52f);
		glPopMatrix();
	}

	// The head lags the torso through its spring, which is what stops it feeling welded on.
	glPushMatrix();
	glTranslatef(0, 0.60f, 0);
	glRotatef((m.headYaw.value - m.facing) * 57.2958f, 0, 1, 0);
	Cube(0, 0.0f, 0, 0.10f, 0.085f, 0.095f, 0.92f, 0.76f, 0.56f);
	Cube(0, 0.15f, 0, 0.125f, 0.045f, 0.115f, 0.98f, 0.80f, 0.14f);
	Cube(0, 0.16f, 0.10f, 0.10f, 0.035f, 0.03f, 0.98f, 0.80f, 0.14f);
	Cube(0, 0.11f, 0.115f, 0.035f, 0.028f, 0.025f, 1.0f, 1.0f, 0.90f);

	// A visible cone from the helmet lamp. In a dark cave this does more for readability
	// than any amount of extra geometry.
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(1.0f, 0.97f, 0.75f, 0.22f);
	glVertex3f(0, 0.11f, 0.14f);
	glColor4f(1.0f, 0.95f, 0.65f, 0.0f);
	for (int i = 0; i <= 12; i++) {
		const float a = (float)i / 12.0f * Anim::TAU;
		glVertex3f(std::cos(a) * 0.36f, 0.11f + std::sin(a) * 0.28f, 1.35f);
	}
	glEnd();
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glPopMatrix();

	// Drill, raised on a spring while working and pumping into the face.
	if (m.toolRaise.value > 0.02f) {
		glPushMatrix();
		glTranslatef(0.0f, 0.42f, 0.20f);
		glRotatef(-62.0f * m.toolRaise.value, 1, 0, 0);
		const float pump = (m.drilling && !m.hasTarget) ? std::sin(m.lifeT * 26.0f) * 0.045f : 0.0f;
		glTranslatef(0, 0, pump);
		Cube(0, 0.0f, 0.14f, 0.045f, 0.035f, 0.16f, 0.42f, 0.44f, 0.48f);
		Cube(0, 0.0f, 0.34f, 0.028f, 0.022f, 0.10f, 0.85f, 0.85f, 0.90f);
		glPopMatrix();
	}

	if (m.health < 100.0f) {
		glDisable(GL_DEPTH_TEST);
		glColor3f(0.2f, 0.9f, 0.3f);
		glBegin(GL_QUADS);
		const float bw = 0.4f * (m.health / 100.0f);
		glVertex3f(-0.2f, 1.02f, 0); glVertex3f(-0.2f + bw, 1.02f, 0);
		glVertex3f(-0.2f + bw, 1.09f, 0); glVertex3f(-0.2f, 1.09f, 0);
		glEnd();
		glEnable(GL_DEPTH_TEST);
	}
	glPopMatrix();
	(void)t;
}

void DrawMonster(const Monster& mo)
{
	static const float base[3][3] = { {0.72f,0.28f,0.20f}, {0.45f,0.80f,0.92f}, {1.0f,0.45f,0.12f} };
	float c[3] = { base[mo.species % 3][0], base[mo.species % 3][1], base[mo.species % 3][2] };

	// A hit whitens the whole creature briefly, so damage is FELT rather than merely counted.
	if (mo.hitFlash > 0.0f) {
		const float f = mo.hitFlash / 0.18f;
		c[0] = Anim::Lerp(c[0], 1.0f, f);
		c[1] = Anim::Lerp(c[1], 1.0f, f);
		c[2] = Anim::Lerp(c[2], 1.0f, f);
	}

	// Emerging strains upward then bursts through, rather than rising linearly.
	const float e = Anim::EmergeCurve(mo.emerge);
	const float sink = (1.0f - e) * 1.05f;
	const float s = mo.scale;

	// Anticipation: pull BACK before the strike. Same principle as the wave director's
	// telegraph, applied to a single animation -- the player gets a moment to read it.
	const float atk = (mo.attackT >= 0.0f) ? Anim::AttackCurve(mo.attackT) : 0.0f;

	const float w = Anim::Clamp(mo.speed / MON_SPEED, 0.0f, 1.0f);
	const Anim::LegPose L = Anim::WalkLeg(mo.gait, 34.0f);
	const Anim::LegPose R = Anim::WalkLeg(mo.gait + 0.5f, 34.0f);
	const float bob = Anim::WalkBob(mo.gait, 0.07f) * w + Anim::IdleBreath(mo.lifeT, 0.03f) * (1.0f - w);

	glPushMatrix();
	glTranslatef(mo.pos.x, -sink, mo.pos.z);
	glRotatef(mo.headYaw.value * 57.2958f, 0, 1, 0);
	glTranslatef(0, bob, atk * 0.22f * s);
	glRotatef(-mo.bodyTilt.value * 57.2958f + atk * 16.0f, 1, 0, 0);

	for (int side = 0; side < 2; side++) {
		const Anim::LegPose& P = side ? R : L;
		glPushMatrix();
		glTranslatef(side ? 0.20f * s : -0.20f * s, 0.26f * s, 0);
		glRotatef(P.hip * 0.8f, 1, 0, 0);
		Cube(0, -0.12f * s, 0, 0.075f * s, 0.07f * s, 0.075f * s, c[0]*0.7f, c[1]*0.7f, c[2]*0.7f);
		glTranslatef(0, -0.14f * s, 0);
		glRotatef(-P.knee * 0.7f, 1, 0, 0);
		Cube(0, -0.06f * s + P.lift * s, 0.02f * s, 0.085f * s, 0.05f * s, 0.10f * s,
			 c[0]*0.55f, c[1]*0.55f, c[2]*0.55f);
		glPopMatrix();
	}

	Cube(0, 0.28f * s, 0, 0.34f * s, 0.24f * s, 0.28f * s, c[0], c[1], c[2]);

	// Jagged back plates. Silhouette is what distinguishes a creature from a box at
	// distance, and costs three cubes.
	for (int i = 0; i < 3; i++) {
		const float o = -0.14f * s + (float)i * 0.14f * s;
		const float hgt = 0.10f * s - (float)((i == 1) ? 0 : 1) * 0.03f * s;
		Cube(o, 0.72f * s, -0.06f * s, 0.05f * s, hgt, 0.05f * s, c[0]*1.2f, c[1]*1.2f, c[2]*1.2f);
	}

	glPushMatrix();
	glTranslatef(0, 0.66f * s, 0.14f * s);
	glRotatef(atk * -14.0f, 1, 0, 0);
	Cube(0, 0, 0, 0.20f * s, 0.15f * s, 0.19f * s, c[0]*1.12f, c[1]*1.12f, c[2]*1.12f);
	Cube(-0.10f * s, 0.10f * s, 0.17f * s, 0.045f * s, 0.035f * s, 0.03f * s, 1.0f, 0.93f, 0.28f);
	Cube( 0.10f * s, 0.10f * s, 0.17f * s, 0.045f * s, 0.035f * s, 0.03f * s, 1.0f, 0.93f, 0.28f);
	Cube(0, -0.13f * s - atk * 0.05f * s, 0.14f * s, 0.14f * s, 0.035f * s, 0.10f * s,
		 c[0]*0.6f, c[1]*0.6f, c[2]*0.6f);
	glPopMatrix();

	for (int side = 0; side < 2; side++) {
		glPushMatrix();
		glTranslatef(side ? 0.40f * s : -0.40f * s, 0.42f * s, 0);
		glRotatef((side ? -1.0f : 1.0f) * Anim::WalkArm(mo.gait, 22.0f) * w - atk * 45.0f, 1, 0, 0);
		Cube(0, -0.16f * s, 0, 0.085f * s, 0.14f * s, 0.085f * s, c[0]*0.85f, c[1]*0.85f, c[2]*0.85f);
		Cube(0, -0.32f * s, 0.03f * s, 0.10f * s, 0.06f * s, 0.09f * s, c[0]*0.65f, c[1]*0.65f, c[2]*0.65f);
		glPopMatrix();
	}

	if (mo.health < 100.0f) {
		glDisable(GL_DEPTH_TEST);
		glColor3f(0.95f, 0.35f, 0.3f);
		glBegin(GL_QUADS);
		const float bw = 0.5f * (mo.health / 100.0f);
		const float hy = 1.15f * s;
		glVertex3f(-0.25f, hy, 0); glVertex3f(-0.25f + bw, hy, 0);
		glVertex3f(-0.25f + bw, hy + 0.07f, 0); glVertex3f(-0.25f, hy + 0.07f, 0);
		glEnd();
		glEnable(GL_DEPTH_TEST);
	}
	glPopMatrix();
}

void RenderWorld()
{
	const Level& L = g.level;

	for (int z = 0; z < L.Height(); z++) {
		for (int x = 0; x < L.Width(); x++) {
			const Block& b = L.At(x, z);
			const bool solid = !b.Has(BLOCK_FLOOR) && !b.Has(BLOCK_WATER);

			if (b.Has(BLOCK_HIDDEN) && solid) {
				Cube((float)x, 0, (float)z, 0.5f, 0.62f, 0.5f, 0.13f, 0.115f, 0.10f);
				continue;
			}
			if (b.Has(BLOCK_CRYSTAL_SEAM)) {
				Cube((float)x, 0, (float)z, 0.5f, 0.62f, 0.5f, 0.30f, 0.20f, 0.34f);
				const float p = 0.65f + 0.35f * std::sin(g.time * 3.5f + (float)(x * 3 + z));
				Cube((float)x, 0.85f, (float)z, 0.20f, 0.22f, 0.20f, 0.75f * p, 0.35f * p, 1.0f * p);
				continue;
			}
			if (b.Has(BLOCK_ORE_SEAM)) {
				Cube((float)x, 0, (float)z, 0.5f, 0.62f, 0.5f, 0.34f, 0.24f, 0.15f);
				Cube((float)x, 0.85f, (float)z, 0.18f, 0.14f, 0.18f, 0.62f, 0.44f, 0.22f);
				continue;
			}
			if (solid) {                     // exposed wall face
				Cube((float)x, 0, (float)z, 0.5f, 0.62f, 0.5f, 0.34f, 0.29f, 0.25f);
				continue;
			}
			if (b.Has(BLOCK_WATER)) {
				Cube((float)x, -0.16f, (float)z, 0.5f, 0.10f, 0.5f, 0.10f, 0.34f, 0.60f);
				continue;
			}
			if (b.Has(BLOCK_TOOLSTORE)) {
				Cube((float)x, -0.14f, (float)z, 0.5f, 0.06f, 0.5f, 0.26f, 0.24f, 0.22f);
				Cube((float)x, 0, (float)z, 0.42f, 0.34f, 0.42f, 0.82f, 0.62f, 0.16f);
				continue;
			}
			Cube((float)x, -0.14f, (float)z, 0.5f, 0.06f, 0.5f, 0.26f, 0.24f, 0.22f); // floor
		}
	}

	// Telegraph markers: exactly where a wave will arrive, five seconds early.
	for (const Marker& mk : g.markers) {
		const float p = 0.4f + 0.6f * std::sin(g.time * 12.0f);
		Cube((float)mk.x, -0.06f, (float)mk.z, 0.46f, 0.03f, 0.46f, 0.95f * p, 0.15f * p, 0.12f * p);
		glDisable(GL_DEPTH_TEST);
		glColor3f(1.0f * p, 0.2f, 0.15f);
		glBegin(GL_LINES);
		glVertex3f((float)mk.x, 0.0f, (float)mk.z);
		glVertex3f((float)mk.x, 2.2f, (float)mk.z);
		glEnd();
		glEnable(GL_DEPTH_TEST);
	}

	for (const Miner& m : g.miners)  DrawMiner(m, g.time);
	for (const Monster& mo : g.monsters) DrawMonster(mo);

	// Drill progress bars
	for (const Miner& m : g.miners) {
		if (!m.drilling) continue;
		glDisable(GL_DEPTH_TEST);
		glColor3f(1.0f, 0.82f, 0.25f);
		glBegin(GL_QUADS);
		const float w = 0.8f * (m.drillProgress / DRILL_TIME);
		const float bx = (float)m.drillX - 0.4f, bz = (float)m.drillZ;
		glVertex3f(bx, 1.35f, bz); glVertex3f(bx + w, 1.35f, bz);
		glVertex3f(bx + w, 1.45f, bz); glVertex3f(bx, 1.45f, bz);
		glEnd();
		glEnable(GL_DEPTH_TEST);
	}
}

// Mouse -> world, by unprojecting through the depth buffer.
bool PickTile(int mx, int my, int& outX, int& outZ)
{
	GLint vp[4]; GLdouble mv[16], pr[16];
	glGetIntegerv(GL_VIEWPORT, vp);
	glGetDoublev(GL_MODELVIEW_MATRIX, mv);
	glGetDoublev(GL_PROJECTION_MATRIX, pr);

	const int wy = vp[3] - my - 1;
	float depth = 0;
	glReadPixels(mx, wy, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
	if (depth >= 1.0f) return false;

	GLdouble ox, oy, oz;
	if (!gluUnProject((GLdouble)mx, (GLdouble)wy, depth, mv, pr, vp, &ox, &oy, &oz)) return false;
	outX = (int)std::floor(ox + 0.5);
	outZ = (int)std::floor(oz + 0.5);
	return true;
}

// --------------------------------------------------------------------------
// Win32
// --------------------------------------------------------------------------
HWND g_hwnd = nullptr;
bool g_orbit = false;
POINT g_last{};
bool g_keys[256] = { false };

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	switch (m) {
	case WM_DESTROY: ::PostQuitMessage(0); return 0;
	case WM_KEYDOWN:
		g_keys[w & 0xFF] = true;
		if (w == VK_ESCAPE) ::PostQuitMessage(0);
		else if (w == VK_SPACE) g.paused = !g.paused;
		else if (w == 'R') g.NewLevel(::GetTickCount());
		return 0;
	case WM_KEYUP: g_keys[w & 0xFF] = false; return 0;

	case WM_MBUTTONDOWN:
	case WM_LBUTTONDOWN:
		g_last = { LOWORD(l), HIWORD(l) };
		if (m == WM_MBUTTONDOWN) { g_orbit = true; ::SetCapture(h); }
		return 0;

	case WM_LBUTTONUP: {
		const int mx = LOWORD(l), my = HIWORD(l);
		int tx, tz;
		bool hit = PickTile(mx, my, tx, tz);
		float bestD = 1.6f; int pick = -1;
		if (hit) {
			for (std::size_t i = 0; i < g.miners.size(); i++) {
				const float d = std::sqrt((g.miners[i].pos.x - tx) * (g.miners[i].pos.x - tx) +
										  (g.miners[i].pos.z - tz) * (g.miners[i].pos.z - tz));
				if (d < bestD) { bestD = d; pick = (int)i; }
			}
		}
		for (std::size_t i = 0; i < g.miners.size(); i++)
			g.miners[i].selected = ((int)i == pick) || (pick < 0 && ::GetKeyState(VK_SHIFT) < 0 && g.miners[i].selected);
		if (pick < 0 && ::GetKeyState(VK_SHIFT) >= 0)
			for (Miner& mm : g.miners) mm.selected = true;   // click empty ground = select all
		return 0;
	}

	case WM_MBUTTONUP: g_orbit = false; ::ReleaseCapture(); return 0;

	case WM_RBUTTONDOWN: {
		int tx, tz;
		if (PickTile(LOWORD(l), HIWORD(l), tx, tz)) g.CommandTo(tx, tz);
		return 0;
	}

	case WM_MOUSEMOVE: {
		const POINT p{ LOWORD(l), HIWORD(l) };
		if (g_orbit || (w & MK_LBUTTON && ::GetKeyState(VK_MENU) < 0)) {
			g.camYaw   += (p.x - g_last.x) * 0.008f;
			g.camPitch += (p.y - g_last.y) * 0.006f;
			if (g.camPitch < 0.25f) g.camPitch = 0.25f;
			if (g.camPitch > 1.45f) g.camPitch = 1.45f;
		}
		g_last = p;
		return 0;
	}

	case WM_MOUSEWHEEL:
		g.camDist -= (float)GET_WHEEL_DELTA_WPARAM(w) / 120.0f * 2.5f;
		if (g.camDist < 8.0f) g.camDist = 8.0f;
		if (g.camDist > 70.0f) g.camDist = 70.0f;
		return 0;
	}
	return ::DefWindowProcA(h, m, w, l);
}

void DrawText2D(HDC dc, int x, int y, const char* s, COLORREF c)
{
	::SetBkMode(dc, TRANSPARENT);
	::SetTextColor(dc, c);
	::TextOutA(dc, x, y, s, (int)strlen(s));
}

} // namespace


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
	WNDCLASSA wc{};
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInst;
	wc.lpszClassName = "DeepCore3D";
	wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	::RegisterClassA(&wc);

	g_hwnd = ::CreateWindowA("DeepCore3D",
		"DeepCore 3D  -  a subterranean mining game  (not Rock Raiders, not Manic Miners)",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 820,
		nullptr, nullptr, hInst, nullptr);
	::ShowWindow(g_hwnd, SW_SHOW);

	HDC dc = ::GetDC(g_hwnd);
	PIXELFORMATDESCRIPTOR pfd{};
	pfd.nSize = sizeof(pfd); pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32; pfd.cDepthBits = 24;
	const int pf = ::ChoosePixelFormat(dc, &pfd);
	::SetPixelFormat(dc, pf, &pfd);
	HGLRC rc = ::wglCreateContext(dc);
	::wglMakeCurrent(dc, rc);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClearColor(0.03f, 0.035f, 0.05f, 1.0f);
	glEnable(GL_FOG);
	const float fogc[4] = { 0.03f, 0.035f, 0.05f, 1.0f };
	glFogfv(GL_FOG_COLOR, fogc);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, 26.0f);
	glFogf(GL_FOG_END, 62.0f);

	HFONT font = ::CreateFontA(16, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, "Consolas");

	g.NewLevel(::GetTickCount());

	DWORD last = ::GetTickCount();
	MSG msg; bool run = true;

	while (run) {
		while (::PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) { run = false; break; }
			::TranslateMessage(&msg); ::DispatchMessageA(&msg);
		}
		if (!run) break;

		const DWORD now = ::GetTickCount();
		float dt = (float)(now - last) / 1000.0f; last = now;
		if (dt > 0.1f) dt = 0.1f;

		// WASD pans the focus in camera space.
		const float pan = 12.0f * dt;
		const float cs = std::cos(g.camYaw), sn = std::sin(g.camYaw);
		if (g_keys['W']) { g.camFocus.x -= sn * pan; g.camFocus.z -= cs * pan; }
		if (g_keys['S']) { g.camFocus.x += sn * pan; g.camFocus.z += cs * pan; }
		if (g_keys['A']) { g.camFocus.x -= cs * pan; g.camFocus.z += sn * pan; }
		if (g_keys['D']) { g.camFocus.x += cs * pan; g.camFocus.z -= sn * pan; }

		g.Update(dt);

		RECT r; ::GetClientRect(g_hwnd, &r);
		const int W = r.right, H = r.bottom;
		glViewport(0, 0, W, H);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION); glLoadIdentity();
		gluPerspective(52.0, (double)W / (double)(H ? H : 1), 0.4, 200.0);

		glMatrixMode(GL_MODELVIEW); glLoadIdentity();
		const float ex = g.camFocus.x + std::sin(g.camYaw) * std::cos(g.camPitch) * g.camDist;
		const float ey = std::sin(g.camPitch) * g.camDist;
		const float ez = g.camFocus.z + std::cos(g.camYaw) * std::cos(g.camPitch) * g.camDist;
		gluLookAt(ex, ey, ez, g.camFocus.x, 0.0f, g.camFocus.z, 0, 1, 0);

		RenderWorld();

		// HUD via GDI on top of the GL surface.
		::wglMakeCurrent(dc, rc);
		HDC hud = dc;
		::SelectObject(hud, font);
		char line[300];
		std::snprintf(line, sizeof(line),
			" Crystals %d/%d   Crew %d   Monsters %d   Wave %d   next %.0fs %s",
			g.crystals, CRYSTAL_GOAL, (int)g.miners.size(), (int)g.monsters.size(),
			g.waveNumber, (WaveInterval(g.wave, g.time) - g.waveTimer) > 0
							? (WaveInterval(g.wave, g.time) - g.waveTimer) : 0.0f,
			g.paused ? "  [PAUSED]" : "");
		DrawText2D(hud, 8, 8, line, RGB(240, 240, 245));
		DrawText2D(hud, 8, H - 24,
			" LMB select / select-all   RMB move or drill   MMB-drag orbit   wheel zoom   WASD pan   R new   Esc quit",
			RGB(150, 150, 165));
		if (g.bannerT > 0 && !g.banner.empty())
			DrawText2D(hud, 8, 30, g.banner.c_str(), RGB(255, 220, 130));
		if (g.won)  DrawText2D(hud, W / 2 - 170, H / 2, "MISSION COMPLETE  -  R for a new cavern", RGB(130, 255, 150));
		if (g.lost) DrawText2D(hud, W / 2 - 150, H / 2, "CREW LOST  -  R to try again", RGB(255, 120, 120));

		::SwapBuffers(dc);
		::Sleep(8);
	}

	::DeleteObject(font);
	::wglMakeCurrent(nullptr, nullptr);
	::wglDeleteContext(rc);
	::ReleaseDC(g_hwnd, dc);
	return 0;
}
