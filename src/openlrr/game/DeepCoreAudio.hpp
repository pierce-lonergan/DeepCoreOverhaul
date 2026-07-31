// DeepCoreAudio.hpp : Plays the threat layer's cues.
//
// This is the THIN half. Every decision about which cue belongs to which moment lives in
// DeepCoreLogic.hpp so the harness can exercise it; this file only turns a decision into
// a sound.
//
// HOW A CUE RESOLVES, AND WHY IT MIGHT NOT
// ----------------------------------------
// SFX cues are addressed by name through SFX_GetType (SFX.cpp:112), which hashes the name
// and searches sfxGlobs.hashNameList. The table is populated from the game's own config at
// load time, so a cue only exists if the user's Lego.cfg declares it. If it does not,
// SFX_GetType returns false and there is nothing to play.
//
// That is expected, not exceptional: this project ships WAV files, but it cannot ship or
// edit the user's Lego.cfg. So every cue resolves lazily, caches the answer including the
// failure, warns exactly once, and is then silently skipped forever. A missing cue must
// never become a per-frame warning storm or a reason for anything else to stop working.
//
// Even a resolved cue can be silent: SFX_Random_PlaySoundNormal only plays when
// SFX_Random_GetSound3DHandle returns a positive handle (SFX.cpp:350-352), which requires
// samples actually bound to that cue. That path already degrades quietly, so we let it.
//
// See docs/research/audio-pipeline-truth.md, and note its open item: the literal config
// path for the Samples block is UNDETERMINED because Lego_LoadSamples is still an exe
// address macro. Until that is settled, the honest position is that these cues work when
// a user wires them up and stay silent otherwise.
//

#pragma once

#include "DeepCoreLogic.hpp"


namespace DeepCore
{; // !<---

namespace Audio
{; // !<---

/// Play a cue, if it exists. ThreatCue::None is a no-op, as is any cue whose name does not
/// resolve. Safe to call from a per-frame path: resolution happens once per cue per level.
void PlayThreatCue(Logic::ThreatCue cue);

/// Drop cached name->ID resolutions. Called on level teardown, because the SFX table is
/// rebuilt per level and a cached ID from the previous one would be meaningless.
void InvalidateCueCache(void);

} // namespace Audio

} // namespace DeepCore
