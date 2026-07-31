// SyntheticLevel.hpp : A cavern the sandbox can simulate, with no game and no .map file.
//
// WHY THIS EXISTS
// ---------------
// This project owns no level data and never will -- .map files are game content. But the
// wave director's decisions are all about a level: which blocks are floor, which walls are
// exposed, where the player's buildings are, how far a spawn is from a base. None of that
// could ever be exercised, so all of it was asserted in comments and nothing more.
//
// A procedurally generated cavern fixes that, and it is not a throwaway: a seeded generator
// is exactly what campaign authoring needs, and it is what makes wave-director tuning
// reproducible. Same seed, same cavern, same waves, every run, on every machine.
//
// The block flags mirror the real BlockFlags1/BlockFlags2 semantics documented in
// docs/research/wave-director.md, so the fairness rules under test here are the same rules
// the shipping code applies. They are re-declared rather than included because Game.h
// drags in the entire engine and every exe-overlaid glob with it.
//
// stdlib only. Compiles and runs on Windows, Linux, and in a browser via Emscripten.
//

#pragma once

#include <cstdint>
#include <string>
#include <vector>


namespace Sandbox
{

/// Mirrors the subset of BlockFlags1 the spawn rules actually test.
enum BlockFlag : std::uint32_t
{
	BLOCK_NONE          = 0,
	BLOCK_FLOOR         = 1u << 0,  ///< walkable ground
	BLOCK_WALL          = 1u << 1,  ///< exposed wall face
	BLOCK_HIDDEN        = 1u << 2,  ///< undiscovered, inside an unopened cavern
	BLOCK_BUILDING      = 1u << 3,  ///< building footprint
	BLOCK_PATH          = 1u << 4,
	BLOCK_BUSY          = 1u << 5,  ///< something is working here
	BLOCK_TOOLSTORE     = 1u << 6,  ///< the Tool Store's own tiles
	BLOCK_WATER         = 1u << 7,
	BLOCK_ORE_SEAM      = 1u << 8,
	BLOCK_CRYSTAL_SEAM  = 1u << 9,
};

struct Block
{
	std::uint32_t flags = BLOCK_NONE;

	bool Has(std::uint32_t f) const { return (flags & f) != 0; }
	void Set(std::uint32_t f)       { flags |= f; }
	void Clear(std::uint32_t f)     { flags &= ~f; }
};

/// A deterministic little LCG. Deliberately not <random>: the same seed must produce the
/// same cavern on every platform and every standard library, and <random>'s distributions
/// are explicitly not required to.
class Rng
{
public:
	explicit Rng(std::uint32_t seed = 1u) : m_state(seed ? seed : 1u) {}

	std::uint32_t Next()
	{
		m_state = m_state * 1103515245u + 12345u;
		return (m_state >> 16) & 0x7fffu;
	}
	std::uint32_t Below(std::uint32_t n)   { return n ? (Next() % n) : 0u; }
	float Unit()                            { return (float)Next() / 32767.0f; }
	bool Chance(float p)                    { return Unit() < p; }

private:
	std::uint32_t m_state;
};

struct LevelDesc
{
	int width = 40;
	int height = 30;
	std::uint32_t seed = 1u;
	int caverns = 5;          ///< how many open chambers to carve
	float waterFraction = 0.06f;
	int oreSeams = 12;
	int crystalSeams = 8;
};

/// A simulated cavern map.
class Level
{
public:
	void Generate(const LevelDesc& desc);

	/// Round-trip text format: human-authorable, diffable, and the thing a campaign author
	/// would eventually edit. Legend is in the file itself.
	std::string ToText() const;
	bool FromText(const std::string& text);

	int Width() const  { return m_width; }
	int Height() const { return m_height; }

	bool InBounds(int x, int y) const
	{
		return x >= 0 && y >= 0 && x < m_width && y < m_height;
	}
	const Block& At(int x, int y) const { return m_blocks[(std::size_t)y * m_width + x]; }
	Block&       At(int x, int y)       { return m_blocks[(std::size_t)y * m_width + x]; }

	/// Recompute which solid blocks are exposed wall faces. A solid block adjacent to any
	/// discovered floor is a wall the player can see and a creature can emerge from; a
	/// solid block buried in rock is not.
	void RecomputeWalls();

	/// Reveal a chamber, as drilling into it would. Used by the sandbox to simulate the
	/// player opening the map up over time, which is what makes spawn candidacy change.
	void Discover(int x, int y, int radius);

	std::size_t CountFlag(std::uint32_t f) const;

private:
	int m_width = 0;
	int m_height = 0;
	std::vector<Block> m_blocks;
};

} // namespace Sandbox
