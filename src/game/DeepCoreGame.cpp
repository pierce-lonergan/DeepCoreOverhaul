// DeepCoreGame.cpp : a playable subterranean mining game.
//
// WHAT THIS IS, AND IS NOT
// ------------------------
// This is NOT LEGO Rock Raiders, and it never can be. That game is a 1999 commercial
// product; this project's other half is a modification that injects into its executable,
// which you must own. Nothing here contains, reproduces, or requires any part of it -- the
// terrain, the units, the creatures and every sound are original work or procedurally
// generated.
//
// What it IS: a game in the same genre, driven by the systems this project actually built.
// The cavern comes from our procedural generator. The monster waves come from the SAME
// DeepCoreLogic.hpp the modification uses -- escalating intervals, an alive budget that
// counts everything on the map, and a telegraph before every wave, because a threat you
// could not have seen coming is a coin flip rather than difficulty.
//
// If you want to play a faithful Rock Raiders remake, play Manic Miners. It is free,
// complete, standalone, and excellent. This is not trying to be that.
//
// DEPENDENCIES: none. Win32 and GDI only, so it builds with the same v142 toolset as the
// rest of the project and needs nothing installed.
//
//   Left click        select a miner (or drag a box to select several)
//   Right click       order the selection: walk to floor, drill a wall
//   Space             pause
//   R                 new cavern
//   Esc               quit
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "../openlrr/game/DeepCoreLogic.hpp"
#include "../sandbox/SyntheticLevel.hpp"

#pragma comment(lib, "winmm.lib")

using namespace DeepCore::Logic;
using namespace Sandbox;

namespace
{

// --------------------------------------------------------------------------
// Tunables
// --------------------------------------------------------------------------
const int   TILE          = 22;
const float MINER_SPEED   = 3.2f;    // tiles/sec
const float DRILL_TIME    = 2.2f;    // seconds to break a wall
const float MONSTER_SPEED = 1.7f;
const float MONSTER_DPS   = 14.0f;
const float MINER_DPS     = 22.0f;   // a miner fights back when cornered
const int   CRYSTAL_GOAL  = 15;

struct Vec2 { float x = 0, y = 0; };

float Dist(const Vec2& a, const Vec2& b)
{
	const float dx = a.x - b.x, dy = a.y - b.y;
	return std::sqrt(dx * dx + dy * dy);
}

// --------------------------------------------------------------------------
// Entities
// --------------------------------------------------------------------------
struct Miner
{
	Vec2 pos;
	Vec2 target;
	bool selected = false;
	bool hasTarget = false;
	bool drilling = false;
	int  drillX = 0, drillY = 0;
	float drillProgress = 0.0f;
	float health = 100.0f;
	float bob = 0.0f;              // little idle animation phase
};

struct Monster
{
	Vec2 pos;
	int  species = 0;              // 0 rock, 1 ice, 2 lava -- colour and speed differ
	float health = 100.0f;
	float scale = 1.0f;            // the variant system, made visible
	float emergeT = 0.0f;          // rises out of the wall over ~1.2s
	Vec2 wander;
};

struct Telegraph
{
	int x = 0, y = 0;
	float remaining = 0.0f;
};

// --------------------------------------------------------------------------
// Game
// --------------------------------------------------------------------------
struct Game
{
	Level level;
	std::vector<Miner> miners;
	std::vector<Monster> monsters;
	std::vector<Telegraph> telegraphs;

	WaveTuning wave;
	ThreatAudioTuning audioCfg;
	QuietDetector quiet;

	float time = 0.0f;
	float waveTimer = 0.0f;
	int   waveNumber = 0;
	int   crystals = 0;
	bool  paused = false;
	bool  won = false, lost = false;
	std::uint32_t seed = 1;
	Rng rng{ 1 };

	std::string banner;
	float bannerT = 0.0f;

	int camX = 0, camY = 0;

	void Say(const std::string& s, float secs = 3.5f)
	{
		banner = s;
		bannerT = secs;
	}

	void NewLevel(std::uint32_t s, int viewW, int viewH)
	{
		seed = s;
		rng = Rng(s);

		LevelDesc d;
		d.seed = s;
		d.width  = 46;
		d.height = 34;
		d.caverns = 8;
		d.waterFraction = 0.05f;
		d.oreSeams = 16;
		d.crystalSeams = 26;
		level.Generate(d);

		// Reveal a starting area and drop the crew in it.
		miners.clear();
		monsters.clear();
		telegraphs.clear();
		crystals = 0;
		time = waveTimer = 0.0f;
		waveNumber = 0;
		won = lost = false;
		quiet.Reset();

		int sx = 0, sy = 0;
		for (int y = 0; y < level.Height() && !sy; y++)
			for (int x = 0; x < level.Width(); x++)
				if (level.At(x, y).Has(BLOCK_TOOLSTORE)) { sx = x; sy = y; break; }

		for (int i = 0; i < 5; i++) {
			Miner m;
			m.pos = { (float)sx + (float)(i % 3) - 1.0f, (float)sy + (float)(i / 3) - 0.5f };
			m.bob = (float)i * 0.7f;
			miners.push_back(m);
		}

		wave.intervalSeconds = 45.0f;
		wave.rampSeconds = 240.0f;
		wave.minIntervalSeconds = 14.0f;
		wave.size = 1;
		wave.sizeMax = 5;
		wave.maxAlive = 7;

		camX = sx * TILE - viewW / 2;
		camY = sy * TILE - viewH / 2;

		Say("Get the crew drilling. Energy crystals are the shiny ones. " +
			std::to_string(CRYSTAL_GOAL) + " will do it.", 7.0f);
	}

	bool Walkable(int x, int y) const
	{
		if (!level.InBounds(x, y)) return false;
		const Block& b = level.At(x, y);
		return b.Has(BLOCK_FLOOR) && !b.Has(BLOCK_WATER);
	}

	bool Solid(int x, int y) const
	{
		if (!level.InBounds(x, y)) return true;
		const Block& b = level.At(x, y);
		return !b.Has(BLOCK_FLOOR) && !b.Has(BLOCK_WATER);
	}

	SpawnBlockInfo Describe(int x, int y) const
	{
		SpawnBlockInfo i;
		const Block& b = level.At(x, y);
		i.isFloor      = b.Has(BLOCK_FLOOR) && !b.Has(BLOCK_WATER);
		i.isHidden     = b.Has(BLOCK_HIDDEN);
		i.isToolStore  = b.Has(BLOCK_TOOLSTORE);
		i.isBuilding   = b.Has(BLOCK_BUILDING);
		i.isPath       = b.Has(BLOCK_PATH);

		static const int DX[4] = { 1, -1, 0, 0 };
		static const int DY[4] = { 0, 0, 1, -1 };
		for (int d = 0; d < 4; d++) {
			const int nx = x + DX[d], ny = y + DY[d];
			if (!level.InBounds(nx, ny)) continue;
			const Block& n = level.At(nx, ny);
			if (n.Has(BLOCK_WALL) && !n.Has(BLOCK_HIDDEN)) { i.hasAdjacentExposedWall = true; break; }
		}

		int best = 1 << 30;
		for (const Miner& m : miners) {
			const int dx = x - (int)m.pos.x, dy = y - (int)m.pos.y;
			const int d2 = dx * dx + dy * dy;
			if (d2 < best) best = d2;
		}
		i.distSqToNearestBuilding = best;
		return i;
	}

	void Update(float dt);
	void CommandTo(int tx, int ty);
};


void Game::CommandTo(int tx, int ty)
{
	if (!level.InBounds(tx, ty)) return;
	const bool wall = Solid(tx, ty) && !level.At(tx, ty).Has(BLOCK_HIDDEN);

	int commanded = 0;
	for (Miner& m : miners) {
		if (!m.selected) continue;
		commanded++;
		if (wall) {
			m.drilling = true;
			m.drillX = tx; m.drillY = ty;
			m.drillProgress = 0.0f;
			m.hasTarget = true;
			// Stand on an adjacent floor tile to work the face.
			static const int DX[4] = { 1, -1, 0, 0 };
			static const int DY[4] = { 0, 0, 1, -1 };
			m.target = { (float)tx, (float)ty };
			for (int d = 0; d < 4; d++) {
				if (Walkable(tx + DX[d], ty + DY[d])) {
					m.target = { (float)(tx + DX[d]), (float)(ty + DY[d]) };
					break;
				}
			}
		}
		else if (Walkable(tx, ty)) {
			m.drilling = false;
			m.hasTarget = true;
			m.target = { (float)tx, (float)ty };
		}
	}
	if (commanded && wall) Say("Drilling.", 1.2f);
}


void Game::Update(float dt)
{
	if (paused || won || lost) return;
	time += dt;
	if (bannerT > 0.0f) bannerT -= dt;

	// ---- miners ----
	for (Miner& m : miners) {
		m.bob += dt * 4.0f;
		if (m.health <= 0.0f) continue;

		if (m.hasTarget) {
			const float d = Dist(m.pos, m.target);
			if (d > 0.08f) {
				const float step = MINER_SPEED * dt;
				m.pos.x += (m.target.x - m.pos.x) / d * step;
				m.pos.y += (m.target.y - m.pos.y) / d * step;
			}
			else if (m.drilling) {
				m.drillProgress += dt;
				if (m.drillProgress >= DRILL_TIME) {
					Block& b = level.At(m.drillX, m.drillY);
					const bool wasCrystal = b.Has(BLOCK_CRYSTAL_SEAM);
					const bool wasOre = b.Has(BLOCK_ORE_SEAM);
					b.flags = BLOCK_FLOOR;
					level.Discover(m.drillX, m.drillY, 3);
					level.RecomputeWalls();
					if (wasCrystal) {
						crystals++;
						Say("Energy crystal recovered. " + std::to_string(crystals) +
							" of " + std::to_string(CRYSTAL_GOAL) + ".", 2.5f);
						if (crystals >= CRYSTAL_GOAL) { won = true; Say("ALL CRYSTALS RECOVERED. Well done.", 99.0f); }
					}
					else if (wasOre) {
						Say("Ore seam cleared.", 1.5f);
					}
					m.drilling = false;
					m.hasTarget = false;
				}
			}
			else {
				m.hasTarget = false;
			}
		}

		// Fight anything standing on top of you.
		for (Monster& mo : monsters) {
			if (mo.emergeT < 1.0f) continue;
			if (Dist(m.pos, mo.pos) < 0.9f) {
				mo.health -= MINER_DPS * dt;
				m.health  -= MONSTER_DPS * dt;
			}
		}
	}

	for (std::size_t i = 0; i < miners.size(); ) {
		if (miners[i].health <= 0.0f) {
			miners.erase(miners.begin() + (long)i);
			Say("We lost one.", 3.0f);
		} else i++;
	}
	if (miners.empty()) { lost = true; Say("THE CREW IS GONE.", 99.0f); }

	// ---- monsters ----
	for (Monster& mo : monsters) {
		if (mo.emergeT < 1.0f) { mo.emergeT += dt / 1.2f; continue; }

		// Head for the nearest miner; wander if there is none in reach.
		const Miner* best = nullptr;
		float bestD = 1e9f;
		for (const Miner& m : miners) {
			const float d = Dist(m.pos, mo.pos);
			if (d < bestD) { bestD = d; best = &m; }
		}

		Vec2 want = mo.pos;
		if (best && bestD < 14.0f) { want = best->pos; }
		else {
			if (rng.Chance(0.9f * dt)) mo.wander = { mo.pos.x + rng.Unit() * 6 - 3, mo.pos.y + rng.Unit() * 6 - 3 };
			want = mo.wander;
		}

		const float d = Dist(mo.pos, want);
		if (d > 0.1f) {
			const float sp = MONSTER_SPEED * (mo.species == 2 ? 1.25f : 1.0f) * dt;
			const float nx = mo.pos.x + (want.x - mo.pos.x) / d * sp;
			const float ny = mo.pos.y + (want.y - mo.pos.y) / d * sp;
			if (Walkable((int)nx, (int)mo.pos.y)) mo.pos.x = nx;
			if (Walkable((int)mo.pos.x, (int)ny)) mo.pos.y = ny;
		}
	}
	for (std::size_t i = 0; i < monsters.size(); ) {
		if (monsters[i].health <= 0.0f) monsters.erase(monsters.begin() + (long)i);
		else i++;
	}

	// ---- telegraphs resolve into monsters ----
	for (std::size_t i = 0; i < telegraphs.size(); ) {
		telegraphs[i].remaining -= dt;
		if (telegraphs[i].remaining <= 0.0f) {
			Monster mo;
			mo.pos = { (float)telegraphs[i].x + 0.5f, (float)telegraphs[i].y + 0.5f };
			mo.species = (int)RotationIndex(waveNumber + (int)i, 3);
			// The variant system, visible: individuals differ in size.
			mo.scale = 0.78f + rng.Unit() * 0.75f;
			mo.wander = mo.pos;
			monsters.push_back(mo);
			telegraphs.erase(telegraphs.begin() + (long)i);
			::PlaySoundA(nullptr, nullptr, SND_PURGE);
			::Beep(90, 70);
		} else i++;
	}

	// ---- the wave director: the project's real logic ----
	const ThreatCue q = quiet.Update((int)monsters.size());
	if (q == ThreatCue::Cleared) Say("Cavern's quiet again. Use it.", 3.0f);

	waveTimer += dt;
	const float interval = WaveInterval(wave, time);
	if (waveTimer >= interval && telegraphs.empty()) {
		waveTimer = 0.0f;
		const int size = WaveSize(wave, waveNumber, (int)monsters.size());
		if (size > 0) {
			std::vector<std::pair<int,int>> cand;
			for (int y = 1; y < level.Height() - 1; y++)
				for (int x = 1; x < level.Width() - 1; x++)
					if (IsFairSpawn(Describe(x, y), 5)) cand.push_back({ x, y });

			if (!cand.empty()) {
				for (int i = 0; i < size; i++) {
					const auto& p = cand[rng.Below((std::uint32_t)cand.size())];
					telegraphs.push_back({ p.first, p.second, 5.0f });
				}
				waveNumber++;
				Say(size >= audioCfg.heavyWaveSize
					? "MULTIPLE CONTACTS. They're coming through the walls."
					: "Movement in the rock. Something's coming through.", 4.0f);
				::Beep(180, 120);
			}
		}
	}
}

// --------------------------------------------------------------------------
// Rendering
// --------------------------------------------------------------------------
Game g;
HWND g_hwnd = nullptr;
bool g_dragging = false;
POINT g_dragStart{}, g_dragNow{};

COLORREF Shade(COLORREF c, float f)
{
	int r = (int)(GetRValue(c) * f), gg = (int)(GetGValue(c) * f), b = (int)(GetBValue(c) * f);
	if (r > 255) r = 255; if (gg > 255) gg = 255; if (b > 255) b = 255;
	return RGB(r, gg, b);
}

void FillRc(HDC dc, int x, int y, int w, int h, COLORREF c)
{
	RECT r{ x, y, x + w, y + h };
	HBRUSH br = ::CreateSolidBrush(c);
	::FillRect(dc, &r, br);
	::DeleteObject(br);
}

void Render(HDC dc, int W, int H)
{
	FillRc(dc, 0, 0, W, H, RGB(8, 8, 12));

	const int x0 = g.camX / TILE - 1, y0 = g.camY / TILE - 1;
	const int x1 = (g.camX + W) / TILE + 1, y1 = (g.camY + H) / TILE + 1;

	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++) {
			if (!g.level.InBounds(x, y)) continue;
			const Block& b = g.level.At(x, y);
			const int sx = x * TILE - g.camX, sy = y * TILE - g.camY;

			COLORREF c;
			if (b.Has(BLOCK_HIDDEN) && !b.Has(BLOCK_FLOOR))      c = RGB(26, 22, 20);
			else if (b.Has(BLOCK_TOOLSTORE))                      c = RGB(200, 150, 40);
			else if (b.Has(BLOCK_WATER))                          c = RGB(30, 70, 120);
			else if (b.Has(BLOCK_CRYSTAL_SEAM))                   c = RGB(150, 60, 190);
			else if (b.Has(BLOCK_ORE_SEAM))                       c = RGB(120, 80, 50);
			else if (b.Has(BLOCK_FLOOR))                          c = RGB(58, 50, 44);
			else                                                  c = RGB(78, 66, 58);

			FillRc(dc, sx, sy, TILE - 1, TILE - 1, c);

			// Seams glitter a little so they read as valuable.
			if (b.Has(BLOCK_CRYSTAL_SEAM)) {
				const float p = 0.6f + 0.4f * std::sin(g.time * 3.0f + (float)(x * 7 + y * 3));
				FillRc(dc, sx + TILE / 3, sy + TILE / 3, TILE / 3, TILE / 3, Shade(RGB(230, 140, 255), p));
			}
		}
	}

	// Telegraph markers: where the next wave WILL arrive.
	for (const Telegraph& t : g.telegraphs) {
		const int sx = t.x * TILE - g.camX, sy = t.y * TILE - g.camY;
		const float pulse = 0.45f + 0.55f * std::sin(g.time * 12.0f);
		FillRc(dc, sx, sy, TILE - 1, TILE - 1, Shade(RGB(200, 40, 40), pulse));
	}

	// Monsters. Species sets the colour; scale is the variant system made visible.
	for (const Monster& mo : g.monsters) {
		static const COLORREF kSp[3] = { RGB(190, 70, 50), RGB(120, 200, 235), RGB(255, 140, 40) };
		const int size = (int)(TILE * 0.85f * mo.scale * (0.35f + 0.65f * mo.emergeT));
		const int sx = (int)(mo.pos.x * TILE) - g.camX - size / 2;
		const int sy = (int)(mo.pos.y * TILE) - g.camY - size / 2;
		FillRc(dc, sx, sy, size, size, kSp[mo.species % 3]);
		FillRc(dc, sx + size / 4, sy + size / 4, size / 5, size / 5, RGB(255, 240, 120)); // eye
	}

	// Miners
	for (const Miner& m : g.miners) {
		const int sx = (int)(m.pos.x * TILE) - g.camX, sy = (int)(m.pos.y * TILE) - g.camY;
		const int bob = (int)(std::sin(m.bob) * 1.5f);
		if (m.selected) FillRc(dc, sx - 9, sy - 9 + bob, 18, 18, RGB(60, 200, 90));
		FillRc(dc, sx - 6, sy - 6 + bob, 12, 12, RGB(240, 200, 60));
		FillRc(dc, sx - 3, sy - 8 + bob, 6, 4, RGB(250, 250, 250));      // helmet light
		// health
		if (m.health < 100.0f)
			FillRc(dc, sx - 8, sy - 12 + bob, (int)(16 * m.health / 100.0f), 2, RGB(80, 230, 80));
		if (m.drilling) {
			const int dx = m.drillX * TILE - g.camX, dy = m.drillY * TILE - g.camY;
			FillRc(dc, dx, dy + TILE - 4, (int)((TILE - 1) * (m.drillProgress / DRILL_TIME)), 3, RGB(255, 210, 60));
		}
	}

	// Selection box
	if (g_dragging) {
		const int l = min(g_dragStart.x, g_dragNow.x), t = min(g_dragStart.y, g_dragNow.y);
		const int r = max(g_dragStart.x, g_dragNow.x), b = max(g_dragStart.y, g_dragNow.y);
		FillRc(dc, l, t, r - l, 1, RGB(60, 220, 90));
		FillRc(dc, l, b, r - l, 1, RGB(60, 220, 90));
		FillRc(dc, l, t, 1, b - t, RGB(60, 220, 90));
		FillRc(dc, r, t, 1, b - t, RGB(60, 220, 90));
	}

	// ---- HUD ----
	FillRc(dc, 0, 0, W, 30, RGB(18, 18, 24));
	::SetBkMode(dc, TRANSPARENT);
	::SetTextColor(dc, RGB(235, 235, 240));

	char hud[320];
	std::snprintf(hud, sizeof(hud),
		"  Crystals %d/%d     Crew %d     Monsters %d     Wave %d     next in %.0fs%s",
		g.crystals, CRYSTAL_GOAL, (int)g.miners.size(), (int)g.monsters.size(),
		g.waveNumber, max(0.0f, WaveInterval(g.wave, g.time) - g.waveTimer),
		g.paused ? "     [PAUSED]" : "");
	::TextOutA(dc, 4, 7, hud, (int)strlen(hud));

	if (g.bannerT > 0.0f && !g.banner.empty()) {
		FillRc(dc, 0, H - 58, W, 26, RGB(18, 18, 24));
		::SetTextColor(dc, RGB(255, 220, 120));
		::TextOutA(dc, 10, H - 52, g.banner.c_str(), (int)g.banner.size());
	}

	FillRc(dc, 0, H - 30, W, 30, RGB(14, 14, 18));
	::SetTextColor(dc, RGB(150, 150, 160));
	const char* help = "  Left-click / drag: select crew    Right-click: move or drill    "
					   "Space: pause    R: new cavern    Esc: quit";
	::TextOutA(dc, 4, H - 22, help, (int)strlen(help));

	if (g.won || g.lost) {
		::SetTextColor(dc, g.won ? RGB(120, 255, 140) : RGB(255, 110, 110));
		const char* msg = g.won ? "MISSION COMPLETE  -  press R for a new cavern"
								: "CREW LOST  -  press R to try again";
		::TextOutA(dc, W / 2 - 190, H / 2, msg, (int)strlen(msg));
	}
}

LRESULT CALLBACK WndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_DESTROY: ::PostQuitMessage(0); return 0;

	case WM_KEYDOWN:
		if (wp == VK_ESCAPE) ::PostQuitMessage(0);
		else if (wp == VK_SPACE) g.paused = !g.paused;
		else if (wp == 'R') { RECT rc; ::GetClientRect(h, &rc); g.NewLevel(::GetTickCount(), rc.right, rc.bottom); }
		return 0;

	case WM_LBUTTONDOWN:
		g_dragging = true;
		g_dragStart = { LOWORD(lp), HIWORD(lp) };
		g_dragNow = g_dragStart;
		return 0;

	case WM_MOUSEMOVE:
		if (g_dragging) g_dragNow = { LOWORD(lp), HIWORD(lp) };
		return 0;

	case WM_LBUTTONUP: {
		g_dragging = false;
		const int l = min(g_dragStart.x, g_dragNow.x), t = min(g_dragStart.y, g_dragNow.y);
		const int r = max(g_dragStart.x, g_dragNow.x), b = max(g_dragStart.y, g_dragNow.y);
		const bool box = (r - l > 4 || b - t > 4);

		for (Miner& m : g.miners) {
			const int sx = (int)(m.pos.x * TILE) - g.camX, sy = (int)(m.pos.y * TILE) - g.camY;
			m.selected = box ? (sx >= l && sx <= r && sy >= t && sy <= b)
							 : (std::abs(sx - (int)g_dragNow.x) < 14 && std::abs(sy - (int)g_dragNow.y) < 14);
		}
		return 0;
	}

	case WM_RBUTTONDOWN: {
		const int tx = (LOWORD(lp) + g.camX) / TILE;
		const int ty = (HIWORD(lp) + g.camY) / TILE;
		g.CommandTo(tx, ty);
		return 0;
	}
	}
	return ::DefWindowProcA(h, msg, wp, lp);
}

} // namespace


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
	WNDCLASSA wc{};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInst;
	wc.lpszClassName = "DeepCoreGame";
	wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	::RegisterClassA(&wc);

	const int W = 1100, H = 760;
	g_hwnd = ::CreateWindowA("DeepCoreGame",
		"DeepCore  -  a subterranean mining game  (not Rock Raiders; see NOTICE.md)",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, W, H,
		nullptr, nullptr, hInst, nullptr);
	::ShowWindow(g_hwnd, SW_SHOW);

	g.NewLevel(::GetTickCount(), W, H);

	// Double-buffered so nothing flickers.
	HDC wdc = ::GetDC(g_hwnd);
	HDC bdc = ::CreateCompatibleDC(wdc);
	HBITMAP bmp = ::CreateCompatibleBitmap(wdc, 2560, 1440);
	::SelectObject(bdc, bmp);
	HFONT font = ::CreateFontA(15, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, "Consolas");
	::SelectObject(bdc, font);

	DWORD last = ::GetTickCount();
	MSG msg;
	bool running = true;

	while (running) {
		while (::PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) { running = false; break; }
			::TranslateMessage(&msg);
			::DispatchMessageA(&msg);
		}
		if (!running) break;

		const DWORD now = ::GetTickCount();
		float dt = (float)(now - last) / 1000.0f;
		last = now;
		if (dt > 0.1f) dt = 0.1f;

		// Camera follows the crew's centre of mass, gently.
		if (!g.miners.empty()) {
			float cx = 0, cy = 0;
			for (const Miner& m : g.miners) { cx += m.pos.x; cy += m.pos.y; }
			cx /= (float)g.miners.size(); cy /= (float)g.miners.size();
			RECT rc; ::GetClientRect(g_hwnd, &rc);
			const int wantX = (int)(cx * TILE) - rc.right / 2;
			const int wantY = (int)(cy * TILE) - rc.bottom / 2;
			g.camX += (int)((wantX - g.camX) * 2.5f * dt);
			g.camY += (int)((wantY - g.camY) * 2.5f * dt);
		}

		g.Update(dt);

		RECT rc; ::GetClientRect(g_hwnd, &rc);
		Render(bdc, rc.right, rc.bottom);
		::BitBlt(wdc, 0, 0, rc.right, rc.bottom, bdc, 0, 0, SRCCOPY);

		::Sleep(12);
	}

	::DeleteObject(font);
	::DeleteObject(bmp);
	::DeleteDC(bdc);
	::ReleaseDC(g_hwnd, wdc);
	return 0;
}
