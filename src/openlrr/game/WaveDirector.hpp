// WaveDirector.hpp : Timed monster waves, independent of a map's own emerge triggers.
//
// WHY THIS EXISTS
// ---------------
// A mission's threat is entirely front-loaded into map data: emerge triggers fire when a
// player drills a specific tile, and Lego_Level::EmergeCreature names ONE species for the
// whole level (Game.h:434). Once a player learns a map, it stops being dangerous. Nothing
// escalates, nothing responds to how well they are doing.
//
// The director adds pressure that the map does not know about: waves on a timer, scaling
// with mission time, budgeted against how many creatures are already alive, and -- this is
// the part that keeps it fair -- TELEGRAPHED before anything appears. A wave the player
// could not have seen coming is not difficulty, it is a coin flip.
//
// WHAT IT DOES NOT DO
// -------------------
// It adds no object IDs and no species. The engine's ceiling is 15 IDs per category and
// roughly 11 are already spent (see docs/HANDOFF-2026-07-30.md). Everything here reuses
// creatures the level already loaded. The variety comes from WHEN and WHERE and HOW MANY,
// not from a longer roster -- and it composes with the per-instance scale/tint variants and
// per-weapon beam styles that already ship.
//
// SAFETY
// ------
// Off by default, like every DeepCore feature. Adds no hook: it runs from the existing
// post-MainLoop slot in OpenLRR_MainLoop_Wrapper (OpenLRR.cpp:977). Touches no struct
// carrying assert_sizeof -- all director state is DLL-side.
//

#pragma once

#include "../common.h"


namespace DeepCore
{; // !<---

namespace Waves
{; // !<---

/// Clear all director state. Called on level teardown AND on successful level load, so a
/// path that bypasses one is still covered. Cheap and idempotent.
void Reset(void);

/// Advance the director by one frame.
///
/// `elapsedReal` is the raw value handed to OpenLRR_MainLoop_Wrapper, in the engine's
/// standard units where 25.0 == one second (common.h STANDARD_FRAMERATE, and Main.cpp's
/// "in 25th's of a second"). The director converts to world time itself so that waves do
/// not advance while the game is paused or slowed.
///
/// Safe to call unconditionally and every frame: returns immediately when the feature is
/// off or when there is no live level.
void Update(real32 elapsedReal);

/// Number of creatures the director has spawned this level, for diagnostics.
sint32 SpawnedThisLevel(void);

} // namespace Waves

} // namespace DeepCore
