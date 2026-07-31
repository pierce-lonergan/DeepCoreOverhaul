// Simulation.cpp
//

#include <cstdio>
#include <cstring>

#include "Simulation.hpp"

using namespace DeepCore::Logic;

namespace Sandbox
{

void Simulation::Init(const SimConfig& cfg)
{
	m_cfg = cfg;
	m_rng = Rng(cfg.seed);
	m_level.Generate(cfg.level);

	m_time = 0.0f;
	m_phaseTimer = 0.0f;
	m_sinceDiscover = 0.0f;
	m_phase = Phase::Waiting;
	m_waveNumber = 0;
	m_totalSpawned = 0;
	m_peakAlive = 0;
	m_creatures.clear();
	m_pending.clear();
	m_trace.clear();
	m_quiet.Reset();
	std::memset(m_cueCounts, 0, sizeof(m_cueCounts));

	m_buildings.clear();
	for (int y = 0; y < m_level.Height(); y++)
		for (int x = 0; x < m_level.Width(); x++)
			if (m_level.At(x, y).Has(BLOCK_BUILDING)) m_buildings.push_back({ x, y });

	char buf[256];
	std::snprintf(buf, sizeof(buf),
		"%dx%d seed=%u floor=%zu wall=%zu water=%zu buildings=%zu",
		m_level.Width(), m_level.Height(), cfg.seed,
		m_level.CountFlag(BLOCK_FLOOR), m_level.CountFlag(BLOCK_WALL),
		m_level.CountFlag(BLOCK_WATER), m_buildings.size());
	Emit("level", buf);
}


void Simulation::Emit(const char* kind, const std::string& detail)
{
	TraceEvent e;
	e.t = m_time;
	e.kind = kind;
	e.detail = detail;
	e.alive = (int)m_creatures.size();
	e.waveNumber = m_waveNumber;
	m_trace.push_back(e);

	if (m_cfg.traceJson) {
		// JSONL: one object per line, so a run diffs cleanly and greps trivially.
		std::printf("{\"t\":%.2f,\"kind\":\"%s\",\"alive\":%d,\"wave\":%d,\"detail\":\"%s\"}\n",
					e.t, kind, e.alive, e.waveNumber, detail.c_str());
	}
}


void Simulation::FireCue(ThreatCue c)
{
	if (c == ThreatCue::None) return;
	const int i = (int)c;
	if (i >= 0 && i < 8) m_cueCounts[i]++;

	static const char* kNames[] = { "none", "telegraph", "telegraph_heavy",
									"arrival", "escalate", "cleared" };
	Emit("cue", (i < 6) ? kNames[i] : "?");
}


int Simulation::CuesFired(ThreatCue c) const
{
	const int i = (int)c;
	return (i >= 0 && i < 8) ? m_cueCounts[i] : 0;
}


SpawnBlockInfo Simulation::Describe(int x, int y) const
{
	SpawnBlockInfo info;
	const Block& b = m_level.At(x, y);

	info.isFloor     = b.Has(BLOCK_FLOOR) && !b.Has(BLOCK_WATER);
	info.isHidden    = b.Has(BLOCK_HIDDEN);
	info.isToolStore = b.Has(BLOCK_TOOLSTORE);
	info.isBuilding  = b.Has(BLOCK_BUILDING);
	info.isPath      = b.Has(BLOCK_PATH);
	info.isBusy      = b.Has(BLOCK_BUSY);

	static const int DX[4] = { 1, -1, 0, 0 };
	static const int DY[4] = { 0, 0, 1, -1 };
	for (int d = 0; d < 4; d++) {
		const int nx = x + DX[d], ny = y + DY[d];
		if (!m_level.InBounds(nx, ny)) continue;
		const Block& n = m_level.At(nx, ny);
		if (n.Has(BLOCK_WALL) && !n.Has(BLOCK_HIDDEN)) { info.hasAdjacentExposedWall = true; break; }
	}

	int best = 1 << 30;
	for (const auto& bl : m_buildings) {
		const int dx = x - bl.first, dy = y - bl.second;
		const int d2 = dx * dx + dy * dy;
		if (d2 < best) best = d2;
	}
	info.distSqToNearestBuilding = best;
	return info;
}


void Simulation::GatherCandidates(std::vector<std::pair<int,int>>& out) const
{
	out.clear();
	for (int y = 1; y < m_level.Height() - 1; y++) {
		for (int x = 1; x < m_level.Width() - 1; x++) {
			// The SHIPPING predicate, not a copy of it.
			if (IsFairSpawn(Describe(x, y), m_cfg.minDistanceFromBase)) out.push_back({ x, y });
		}
	}
}


void Simulation::Step()
{
	const float dt = m_cfg.tickSeconds;
	m_time += dt;
	m_phaseTimer += dt;
	m_sinceDiscover += dt;

	// ---- creatures age, wander a little, and die ----
	for (std::size_t i = 0; i < m_creatures.size(); ) {
		Creature& c = m_creatures[i];
		c.ageSeconds += dt;
		if (c.emerging && c.ageSeconds > 2.0f) c.emerging = false;

		if (m_rng.Chance(m_cfg.killPerCreaturePerSecond * dt)) {
			m_creatures.erase(m_creatures.begin() + (long)i);
			continue;
		}
		if (m_rng.Chance(0.6f * dt)) {
			static const int DX[4] = { 1, -1, 0, 0 };
			static const int DY[4] = { 0, 0, 1, -1 };
			const int d = (int)m_rng.Below(4);
			const int nx = c.x + DX[d], ny = c.y + DY[d];
			if (m_level.InBounds(nx, ny) && m_level.At(nx, ny).Has(BLOCK_FLOOR)) { c.x = nx; c.y = ny; }
		}
		i++;
	}
	if ((int)m_creatures.size() > m_peakAlive) m_peakAlive = (int)m_creatures.size();

	// The all-clear is an EDGE, and this is the shipping detector deciding it.
	FireCue(m_quiet.Update((int)m_creatures.size()));

	// ---- the player opens up the map over time ----
	if (m_sinceDiscover >= m_cfg.discoverEverySeconds) {
		m_sinceDiscover = 0.0f;
		for (int attempt = 0; attempt < 200; attempt++) {
			const int x = (int)m_rng.Below((std::uint32_t)m_level.Width());
			const int y = (int)m_rng.Below((std::uint32_t)m_level.Height());
			if (m_level.At(x, y).Has(BLOCK_FLOOR) && m_level.At(x, y).Has(BLOCK_HIDDEN)) {
				m_level.Discover(x, y, 6);
				Emit("discover", "player opened a new chamber");
				break;
			}
		}
	}

	// ---- the director ----
	if (m_phase == Phase::Telegraph) {
		if (m_phaseTimer < m_cfg.telegraphSeconds) return;

		int landed = 0;
		for (const auto& p : m_pending) {
			Creature c;
			c.x = p.first;
			c.y = p.second;
			c.speciesId = (int)RotationIndex(m_waveNumber + landed, 3);
			m_creatures.push_back(c);
			landed++;
		}
		m_totalSpawned += landed;
		m_waveNumber++;
		m_pending.clear();
		m_phase = Phase::Waiting;
		m_phaseTimer = 0.0f;

		if (landed > 0) FireCue(ThreatCue::Arrival);

		char buf[128];
		std::snprintf(buf, sizeof(buf), "wave %d landed with %d creature(s)", m_waveNumber, landed);
		Emit("wave_land", buf);
		return;
	}

	// ---- waiting ----
	const float interval = WaveInterval(m_cfg.wave, m_time);
	if (m_phaseTimer < interval) {
		// Refresh the candidate count occasionally so the on-screen explanation reflects
		// the map as it is now, not as it was at the last wave.
		if (m_lastCandidateCount < 0 || ((int)(m_time * 10.0f) % 50) == 0) {
			std::vector<std::pair<int,int>> peek;
			GatherCandidates(peek);
			m_lastCandidateCount = (int)peek.size();
		}
		return;
	}
	m_phaseTimer = 0.0f;

	const int alive = (int)m_creatures.size();
	const int size = WaveSize(m_cfg.wave, m_waveNumber, alive);
	if (size < 1) {
		Emit("wave_suppressed", "alive budget full");
		return;
	}

	std::vector<std::pair<int,int>> candidates;
	GatherCandidates(candidates);
	m_lastCandidateCount = (int)candidates.size();
	if (candidates.empty()) {
		// Worth tracing rather than returning silently: a sealed or fully-built-out map
		// legitimately has nowhere fair, and that should be visible, not inferred.
		Emit("wave_no_candidates", "no fair spawn block available");
		return;
	}

	m_pending.clear();
	for (int i = 0; i < size; i++) {
		m_pending.push_back(candidates[m_rng.Below((std::uint32_t)candidates.size())]);
	}

	FireCue(TelegraphCueFor(m_cfg.audio, size));
	if (ShouldEscalate(m_cfg.audio, m_waveNumber)) FireCue(ThreatCue::Escalate);

	m_phase = Phase::Telegraph;

	char buf[192];
	std::snprintf(buf, sizeof(buf),
		"wave %d telegraphed: %d creature(s), %zu candidate block(s), interval %.0fs",
		m_waveNumber + 1, size, candidates.size(), interval);
	Emit("wave_telegraph", buf);
}


std::string Simulation::RenderAscii(bool colour) const
{
	std::vector<std::string> rows;
	for (int y = 0; y < m_level.Height(); y++) {
		std::string row;
		for (int x = 0; x < m_level.Width(); x++) {
			const Block& b = m_level.At(x, y);
			char ch;
			const char* col = nullptr;

			if (b.Has(BLOCK_TOOLSTORE))         { ch = 'T'; col = "\x1b[1;33m"; }
			else if (b.Has(BLOCK_BUILDING))     { ch = 'B'; col = "\x1b[33m";   }
			else if (b.Has(BLOCK_WATER))        { ch = '~'; col = "\x1b[36m";   }
			else if (b.Has(BLOCK_CRYSTAL_SEAM)) { ch = 'C'; col = "\x1b[1;35m"; }
			else if (b.Has(BLOCK_ORE_SEAM))     { ch = 'O'; col = "\x1b[35m";   }
			else if (b.Has(BLOCK_HIDDEN))       { ch = '#'; col = "\x1b[90m";   }
			else if (b.Has(BLOCK_WALL))         { ch = '%'; col = "\x1b[37m";   }
			else if (b.Has(BLOCK_FLOOR))        { ch = '.'; col = "\x1b[90m";   }
			else                                { ch = '#'; col = "\x1b[90m";   }

			row += colour && col ? std::string(col) + ch + "\x1b[0m" : std::string(1, ch);
		}
		rows.push_back(row);
	}

	// Telegraph markers go down first so a creature standing on one still shows.
	auto put = [&](int x, int y, char ch, const char* col) {
		if (!m_level.InBounds(x, y)) return;
		const std::string cell = colour ? std::string(col) + ch + "\x1b[0m" : std::string(1, ch);
		std::string& row = rows[(std::size_t)y];
		// Each cell may be multi-byte when coloured, so rebuild rather than index.
		std::string rebuilt;
		int col_i = 0;
		std::size_t i = 0;
		while (i < row.size()) {
			std::size_t start = i;
			if (row[i] == '\x1b') { while (i < row.size() && row[i] != 'm') i++; i++; }
			i++;
			if (i < row.size() && row[i] == '\x1b') { while (i < row.size() && row[i] != 'm') i++; i++; }
			rebuilt += (col_i == x) ? cell : row.substr(start, i - start);
			col_i++;
		}
		row = rebuilt;
	};

	for (const auto& p : m_pending) put(p.first, p.second, '!', "\x1b[1;31m");
	for (const Creature& c : m_creatures) put(c.x, c.y, c.emerging ? 'o' : 'M', "\x1b[1;31m");

	std::string out;
	for (const std::string& r : rows) { out += r; out += '\n'; }
	return out;
}


std::string Simulation::StatusLine() const
{
	char buf[320];
	const float interval = WaveInterval(m_cfg.wave, m_time);
	std::snprintf(buf, sizeof(buf),
		"t=%6.1fs  wave %2d  alive %2d/%-2d  peak %2d  spawned %3d  interval %5.1fs  %s%.1fs",
		m_time, m_waveNumber, (int)m_creatures.size(), m_cfg.wave.maxAlive,
		m_peakAlive, m_totalSpawned, interval,
		m_phase == Phase::Telegraph ? "INCOMING in " : "next in ",
		m_phase == Phase::Telegraph ? (m_cfg.telegraphSeconds - m_phaseTimer)
									: (interval - m_phaseTimer));
	return buf;
}


std::string Simulation::ExplainLine() const
{
	char buf[320];

	if (m_phase == Phase::Telegraph) {
		std::snprintf(buf, sizeof(buf),
			"  [1;31mWAVE INCOMING[0m at %zu marked block(s) -- the warning names where "
			"it will actually arrive", m_pending.size());
		return buf;
	}

	if ((int)m_creatures.size() >= m_cfg.wave.maxAlive) {
		std::snprintf(buf, sizeof(buf),
			"  holding: alive budget is full (%d/%d), so no wave will fire until some die",
			(int)m_creatures.size(), m_cfg.wave.maxAlive);
		return buf;
	}

	if (m_lastCandidateCount == 0) {
		// This is the state that previously looked like a hang. Say it out loud.
		std::snprintf(buf, sizeof(buf),
			"  waiting: [33mno legal spawn block[0m -- every discovered tile is inside "
			"the %d-block base standoff, or has no exposed wall", m_cfg.minDistanceFromBase);
		return buf;
	}

	std::snprintf(buf, sizeof(buf),
		"  waiting: %d legal spawn block(s) available; escalation shortens the interval as the "
		"mission runs", m_lastCandidateCount);
	return buf;
}

} // namespace Sandbox
