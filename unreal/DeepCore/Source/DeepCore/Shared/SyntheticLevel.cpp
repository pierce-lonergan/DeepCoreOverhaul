// SyntheticLevel.cpp
//

#include <cstdio>
#include <cstring>

#include "SyntheticLevel.hpp"

namespace Sandbox
{

namespace
{

/// One character per block. Chosen so a map is readable at a glance in a terminal and in a
/// diff: solid rock is dense, floor is empty, and the things that matter stand out.
char BlockToChar(const Block& b)
{
	if (b.Has(BLOCK_TOOLSTORE))    return 'T';
	if (b.Has(BLOCK_BUILDING))     return 'B';
	if (b.Has(BLOCK_PATH))         return '=';
	if (b.Has(BLOCK_WATER))        return '~';
	if (b.Has(BLOCK_CRYSTAL_SEAM)) return 'C';
	if (b.Has(BLOCK_ORE_SEAM))     return 'O';
	if (b.Has(BLOCK_FLOOR))        return '.';
	if (b.Has(BLOCK_HIDDEN))       return '#';   // undiscovered solid
	if (b.Has(BLOCK_WALL))         return '%';   // exposed wall face
	return '#';
}

void CharToBlock(char c, Block& b)
{
	b.flags = BLOCK_NONE;
	switch (c) {
	case 'T': b.Set(BLOCK_TOOLSTORE | BLOCK_FLOOR | BLOCK_BUILDING); break;
	case 'B': b.Set(BLOCK_BUILDING | BLOCK_FLOOR); break;
	case '=': b.Set(BLOCK_PATH | BLOCK_FLOOR); break;
	case '~': b.Set(BLOCK_WATER); break;
	case 'C': b.Set(BLOCK_CRYSTAL_SEAM); break;
	case 'O': b.Set(BLOCK_ORE_SEAM); break;
	case '.': b.Set(BLOCK_FLOOR); break;
	case '%': b.Set(BLOCK_WALL); break;
	case '#': default: b.Set(BLOCK_HIDDEN); break;
	}
}

} // namespace


void Level::Generate(const LevelDesc& desc)
{
	m_width = desc.width;
	m_height = desc.height;
	m_blocks.assign((std::size_t)m_width * m_height, Block{});

	// Everything starts as undiscovered solid rock, and caverns are carved out of it. That
	// is the right direction: a real map is mostly rock, and the interesting question for
	// the wave director is which of the few open tiles are legal spawns.
	for (Block& b : m_blocks) b.Set(BLOCK_HIDDEN);

	Rng rng(desc.seed);

	// Carve chambers, then connect them with corridors so the map is one reachable space.
	struct Room { int cx, cy, r; };
	std::vector<Room> rooms;

	for (int i = 0; i < desc.caverns; i++) {
		const int r = 2 + (int)rng.Below(3);
		const int cx = r + 1 + (int)rng.Below((std::uint32_t)(m_width  - 2 * r - 2));
		const int cy = r + 1 + (int)rng.Below((std::uint32_t)(m_height - 2 * r - 2));
		rooms.push_back({ cx, cy, r });

		for (int y = cy - r; y <= cy + r; y++) {
			for (int x = cx - r; x <= cx + r; x++) {
				if (!InBounds(x, y)) continue;
				const int dx = x - cx, dy = y - cy;
				if (dx * dx + dy * dy <= r * r) {
					At(x, y).flags = BLOCK_FLOOR;
				}
			}
		}
	}

	for (std::size_t i = 1; i < rooms.size(); i++) {
		int x = rooms[i - 1].cx, y = rooms[i - 1].cy;
		const int tx = rooms[i].cx, ty = rooms[i].cy;
		while (x != tx || y != ty) {
			if (x != tx) x += (tx > x) ? 1 : -1;
			else if (y != ty) y += (ty > y) ? 1 : -1;
			if (InBounds(x, y)) At(x, y).flags = BLOCK_FLOOR;
		}
	}

	// Seams live in the rock at the edges of open space, which is where a player finds them.
	auto sprinkleSeam = [&](std::uint32_t flag, int count) {
		int placed = 0, guard = 0;
		while (placed < count && guard++ < 10000) {
			const int x = 1 + (int)rng.Below((std::uint32_t)(m_width - 2));
			const int y = 1 + (int)rng.Below((std::uint32_t)(m_height - 2));
			Block& b = At(x, y);
			if (!b.Has(BLOCK_HIDDEN)) continue;
			bool nextToFloor = false;
			if (At(x + 1, y).Has(BLOCK_FLOOR) || At(x - 1, y).Has(BLOCK_FLOOR) ||
				At(x, y + 1).Has(BLOCK_FLOOR) || At(x, y - 1).Has(BLOCK_FLOOR)) nextToFloor = true;
			if (!nextToFloor) continue;
			b.flags = flag;
			placed++;
		}
	};
	sprinkleSeam(BLOCK_ORE_SEAM, desc.oreSeams);
	sprinkleSeam(BLOCK_CRYSTAL_SEAM, desc.crystalSeams);

	// Water pools. The count matters: the relocated water tables raised the engine's caps
	// from 10 pools / 100 blocks to 4096 / 65536, and a sandbox map can deliberately exceed
	// the old limits to exercise the path that used to terminate the process.
	//
	// BUT the fraction is of the OPEN FLOOR, not of the whole map. Taking it from the map
	// area was a real bug: a 56x28 map at 0.08 asked for 125 water tiles when the carved
	// caverns only contained about 220, so water ate more than half the walkable space and
	// the director had almost nowhere legal to put anything. Hard-capped at a third of the
	// floor as well, so a careless fraction cannot reproduce that.
	const int floorTiles = (int)CountFlag(BLOCK_FLOOR);
	int waterTiles = (int)(desc.waterFraction * (float)floorTiles);
	if (waterTiles > floorTiles / 3) waterTiles = floorTiles / 3;

	int placedWater = 0, guard = 0;
	while (placedWater < waterTiles && guard++ < 20000) {
		const int x = 1 + (int)rng.Below((std::uint32_t)(m_width - 2));
		const int y = 1 + (int)rng.Below((std::uint32_t)(m_height - 2));
		Block& b = At(x, y);
		if (!b.Has(BLOCK_FLOOR)) continue;
		b.flags = BLOCK_WATER;
		placedWater++;
	}

	// The Tool Store, in the first chamber. The wave director's standoff rule is measured
	// from the player's buildings, so there must be one.
	if (!rooms.empty()) {
		At(rooms[0].cx, rooms[0].cy).flags = BLOCK_TOOLSTORE | BLOCK_FLOOR | BLOCK_BUILDING;
	}

	// Open the map up over time, but not from nothing.
	//
	// Revealing ONLY the starting chamber was the second half of the same bug: the base
	// standoff distance is measured from the Tool Store, and a starting room of radius 2-4
	// lies entirely inside it, so every discovered tile was rejected and the director had
	// zero candidates for the first two minutes of every run. That is arguably correct
	// behaviour -- an unexplored map genuinely has nowhere fair -- but it modelled a player
	// who had done nothing, which is not the interesting case and made the sandbox look
	// broken.
	//
	// Two chambers plus the corridor between them is the smallest state that is both
	// plausible and testable: it puts legal ground outside the standoff radius immediately,
	// while leaving most of the map to be discovered as the run proceeds.
	for (Block& b : m_blocks) {
		if (b.Has(BLOCK_FLOOR) || b.Has(BLOCK_WATER)) b.Set(BLOCK_HIDDEN);
	}
	for (std::size_t i = 0; i < rooms.size() && i < 2; i++) {
		Discover(rooms[i].cx, rooms[i].cy, rooms[i].r + 3);
	}

	RecomputeWalls();
}


void Level::Discover(int cx, int cy, int radius)
{
	// Flood outward through open space only, so discovering a chamber does not reveal
	// whatever happens to be within a radius on the far side of a wall.
	std::vector<std::pair<int,int>> stack;
	stack.push_back({ cx, cy });

	while (!stack.empty()) {
		const auto p = stack.back();
		stack.pop_back();
		const int x = p.first, y = p.second;
		if (!InBounds(x, y)) continue;

		Block& b = At(x, y);
		const bool open = b.Has(BLOCK_FLOOR) || b.Has(BLOCK_WATER);
		if (!open) { b.Clear(BLOCK_HIDDEN); continue; }   // reveal the wall face itself
		if (!b.Has(BLOCK_HIDDEN)) continue;

		b.Clear(BLOCK_HIDDEN);
		const int dx = x - cx, dy = y - cy;
		if (dx * dx + dy * dy > radius * radius) continue;

		stack.push_back({ x + 1, y });
		stack.push_back({ x - 1, y });
		stack.push_back({ x, y + 1 });
		stack.push_back({ x, y - 1 });
	}
	RecomputeWalls();
}


void Level::RecomputeWalls()
{
	for (int y = 0; y < m_height; y++) {
		for (int x = 0; x < m_width; x++) {
			Block& b = At(x, y);
			const bool solid = !b.Has(BLOCK_FLOOR) && !b.Has(BLOCK_WATER);
			if (!solid) { b.Clear(BLOCK_WALL); continue; }

			bool exposed = false;
			static const int DX[4] = { 1, -1, 0, 0 };
			static const int DY[4] = { 0, 0, 1, -1 };
			for (int d = 0; d < 4; d++) {
				const int nx = x + DX[d], ny = y + DY[d];
				if (!InBounds(nx, ny)) continue;
				const Block& n = At(nx, ny);
				if ((n.Has(BLOCK_FLOOR) || n.Has(BLOCK_WATER)) && !n.Has(BLOCK_HIDDEN)) {
					exposed = true;
					break;
				}
			}
			if (exposed) b.Set(BLOCK_WALL);
			else         b.Clear(BLOCK_WALL);
		}
	}
}


std::size_t Level::CountFlag(std::uint32_t f) const
{
	std::size_t n = 0;
	for (const Block& b : m_blocks) if (b.Has(f)) n++;
	return n;
}


std::string Level::ToText() const
{
	std::string out;
	out += "# DeepCore sandbox level\n";
	out += "# legend: '#' rock/undiscovered  '%' exposed wall  '.' floor  '~' water\n";
	out += "#         'O' ore seam  'C' crystal seam  'T' tool store  'B' building  '=' path\n";
	char header[64];
	std::snprintf(header, sizeof(header), "size %d %d\n", m_width, m_height);
	out += header;

	for (int y = 0; y < m_height; y++) {
		for (int x = 0; x < m_width; x++) out += BlockToChar(At(x, y));
		out += '\n';
	}
	return out;
}


bool Level::FromText(const std::string& text)
{
	std::vector<std::string> rows;
	int w = 0, h = 0;

	std::size_t i = 0;
	while (i < text.size()) {
		std::size_t e = text.find('\n', i);
		if (e == std::string::npos) e = text.size();
		std::string line = text.substr(i, e - i);
		i = e + 1;

		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (line.empty() || line[0] == '#') continue;
		if (line.compare(0, 5, "size ") == 0) {
			// sscanf is flagged as unsafe by MSVC, but the concern there is unbounded %s into
			// a fixed buffer. This reads two ints from a NUL-terminated std::string and checks
			// the conversion count, so there is nothing to overrun. The warning is suppressed
			// at the call site rather than project-wide, because a project-wide suppression
			// would also hide the cases that genuinely are unsafe.
#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4996)
#endif
			if (std::sscanf(line.c_str(), "size %d %d", &w, &h) != 2) return false;
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif
			continue;
		}
		rows.push_back(line);
	}

	if (w <= 0 || h <= 0 || (int)rows.size() < h) return false;

	m_width = w;
	m_height = h;
	m_blocks.assign((std::size_t)w * h, Block{});
	for (int y = 0; y < h; y++) {
		const std::string& r = rows[(std::size_t)y];
		for (int x = 0; x < w; x++) {
			CharToBlock(x < (int)r.size() ? r[(std::size_t)x] : '#', At(x, y));
		}
	}
	RecomputeWalls();
	return true;
}

} // namespace Sandbox
