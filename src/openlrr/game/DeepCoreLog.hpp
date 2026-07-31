// DeepCoreLog.hpp : A log file that actually exists.
//
// WHY THIS IS NOT Error_SetDumpFile
// ---------------------------------
// The engine has a dump-file mechanism (Errors.cpp:92) and Error_Out writes to
// errorGlobs.dumpFile when it is set (Errors.cpp:168). Nothing in the entire tree ever
// calls Error_SetDumpFile, so dumpFile is always null and every diagnostic this project
// emits goes only to a console window -- which closes with the process, scrolls away, and
// is absent entirely if the game was started any way other than from a terminal.
//
// Worse, until recently every DeepCore diagnostic used Error_DebugF2, and
// Gods98::errorLogLevels defaults debugVisible = false (Errors.cpp:26). So VerboseStartup
// -- the setting a person reaches for to find out whether any of this loaded -- was the
// one thing guaranteed to tell them nothing.
//
// This module is DLL-side and owns its own file handle. It does not touch errorGlobs,
// which is a reference overlaid on the exe's data segment, and it works regardless of what
// the engine's log levels are set to. Every line also goes to the engine's normal output,
// so a console user sees the same thing.
//
// WHAT IT IS FOR
// --------------
// docs/EXPERIMENTS.md exists because this project's most valuable tests FAIL SILENTLY: an
// unresolved species name, a cue that never registered, a config file that was never found.
// A log that records what was looked for and what was found is what converts those from
// silent to observable. That is the entire justification for this file.
//

#pragma once

#include <string>


namespace DeepCore
{; // !<---

namespace Log
{; // !<---

enum class Level
{
	Info,
	Warn,
	Error,
};

/// Open the log. Safe to call more than once; the second call is ignored.
/// `filename` is resolved next to the running module, not the working directory, because
/// the working directory of an injected process is not something we control.
void Open(const char* filename);

/// Write one line. A timestamp and level are prepended. Never throws, never allocates on
/// the failure path, and silently does nothing if the file could not be opened -- a logger
/// that can break the program it is diagnosing is worse than no logger.
void Write(Level level, const char* fmt, ...);

/// Emit the startup banner: build configuration, every resolved setting, and every asset
/// path that was checked with whether it was found. This is the first thing a person reads
/// when something did not work.
void Banner(void);

/// Record an asset lookup and its outcome, so a missing file is visible rather than
/// inferred from silence.
void AssetChecked(const char* what, const char* path, bool found);

void Close(void);

#define DeepCore_Log(...)   DeepCore::Log::Write(DeepCore::Log::Level::Info,  __VA_ARGS__)
#define DeepCore_LogWarn(...) DeepCore::Log::Write(DeepCore::Log::Level::Warn, __VA_ARGS__)

} // namespace Log

} // namespace DeepCore
