// DeepCoreLog.cpp : A log file that actually exists.
//

#include <cstdarg>
#include <cstdio>
#include <ctime>

#include "../platform/windows.h"

#include "../engine/core/Errors.h"

#include "Game.h"
#include "DeepCore.hpp"
#include "DeepCoreLog.hpp"


namespace
{

std::FILE* _file = nullptr;
bool _tried = false;

const char* LevelName(DeepCore::Log::Level l)
{
	switch (l) {
	case DeepCore::Log::Level::Warn:  return "WARN ";
	case DeepCore::Log::Level::Error: return "ERROR";
	default:                          return "info ";
	}
}

/// Resolve a path next to the running module rather than the working directory.
/// An injected process inherits whatever working directory the launcher had, which is not
/// something this project controls and is frequently not the game folder.
void ResolveBesideModule(const char* filename, char* out, size_t outSize)
{
	char modulePath[MAX_PATH] = { 0 };

	// nullptr means the module that started the process. That is the launcher/exe, which
	// is where a user will look for a log -- next to the thing they double-clicked.
	if (::GetModuleFileNameA(nullptr, modulePath, MAX_PATH) == 0) {
		std::snprintf(out, outSize, "%s", filename);
		return;
	}

	char* lastSlash = nullptr;
	for (char* p = modulePath; *p; p++) {
		if (*p == '\\' || *p == '/') lastSlash = p;
	}
	if (lastSlash != nullptr) {
		*(lastSlash + 1) = '\0';
		std::snprintf(out, outSize, "%s%s", modulePath, filename);
	}
	else {
		std::snprintf(out, outSize, "%s", filename);
	}
}

} // namespace


void DeepCore::Log::Open(const char* filename)
{
	if (_tried) return;
	_tried = true;

	char path[MAX_PATH * 2] = { 0 };
	ResolveBesideModule(filename, path, sizeof(path));

	_file = std::fopen(path, "w");
	if (_file == nullptr) {
		// Not fatal, and deliberately not loud beyond one line: a read-only install
		// directory is a real situation, and losing the log must never stop the game.
		Error_WarnF2(true, "DeepCore: could not open log file \"%s\"; console only.\n", path);
		return;
	}

	std::fprintf(_file, "DeepCoreOverhaul log\n");
	std::fprintf(_file, "file: %s\n", path);
	std::fflush(_file);
}


void DeepCore::Log::Close(void)
{
	if (_file != nullptr) {
		std::fclose(_file);
		_file = nullptr;
	}
}


void DeepCore::Log::Write(Level level, const char* fmt, ...)
{
	char body[1024];

	std::va_list args;
	va_start(args, fmt);
	std::vsnprintf(body, sizeof(body), fmt, args);
	va_end(args);

	// Console/debugger output, so someone watching a terminal sees it live. INFO level,
	// because debugVisible defaults false (Errors.cpp:26) and a diagnostic nobody can see
	// is not a diagnostic.
	Error_InfoF2("DeepCore: %s\n", body);

	if (_file == nullptr) return;

	std::time_t now = std::time(nullptr);
	std::tm tmv;
#if defined(_WIN32)
	::localtime_s(&tmv, &now);
#else
	tmv = *std::localtime(&now);
#endif

	std::fprintf(_file, "[%02d:%02d:%02d] %s %s\n",
				 tmv.tm_hour, tmv.tm_min, tmv.tm_sec, LevelName(level), body);

	// Flushed every line on purpose. A log that loses its last thirty lines to a crash is
	// useless precisely when it matters most, and this is not a hot path.
	std::fflush(_file);
}


void DeepCore::Log::AssetChecked(const char* what, const char* path, bool found)
{
	Write(found ? Level::Info : Level::Warn,
		  "asset %-22s %-46s %s", what, path, found ? "FOUND" : "MISSING");
}


void DeepCore::Log::Banner(void)
{
	const Settings& s = DeepCore::settings;

	Write(Level::Info, "%s", "----------------------------------------------------------------");
	Write(Level::Info, "DeepCoreOverhaul  build %s  (%s)",
#if defined(NDEBUG)
		  "Release",
#else
		  "Debug",
#endif
		  __DATE__);
	Write(Level::Info, "%s", "This is a fan fork of OpenLRR. See NOTICE.md.");
	Write(Level::Info, "%s", "----------------------------------------------------------------");

	// Every gate, resolved. If a feature did not do what someone expected, the first
	// question is always whether it was actually on, and this answers it without asking
	// them to turn on a separate diagnostic setting.
	Write(Level::Info, "settings resolved from %s:", DEEPCORE_FILENAME);
	Write(Level::Info, "  MultiSpeciesEmerge    %-6s  (pool: %i name(s))",
		  s.multiSpeciesEmerge ? "TRUE" : "FALSE", (sint32)s.emergeSpeciesNames.size());
	Write(Level::Info, "  WaveDirector          %-6s  interval %.0fs, ramp %.0fs, max alive %i",
		  s.waveDirector ? "TRUE" : "FALSE", s.waveIntervalSeconds, s.waveRampSeconds,
		  (sint32)s.waveMaxAlive);
	Write(Level::Info, "  ThreatAudio           %-6s  (dir: %s)",
		  s.threatAudio ? "TRUE" : "FALSE", s.cueSampleDir.c_str());
	Write(Level::Info, "  CreatureVariants      %-6s  (%i variant(s))",
		  s.creatureVariants ? "TRUE" : "FALSE", (sint32)s.variants.size());
	Write(Level::Info, "  WeaponBeamStyles      %-6s  (%i style(s))",
		  s.weaponBeamStyles ? "TRUE" : "FALSE", (sint32)s.beamStyles.size());
	Write(Level::Info, "  SurviveWaterOverflow  %s", s.surviveWaterOverflow ? "TRUE" : "FALSE");
	Write(Level::Info, "  RelocateWaterTables   %-6s  caps %u pools / %u blocks",
		  s.relocateWaterTables ? "TRUE" : "FALSE",
		  (unsigned)s.waterMaxPools, (unsigned)s.waterMaxPoolBlocks);

	if (!DeepCore::IsAnyFeatureEnabled()) {
		Write(Level::Info, "%s",
			  "NOTE: every DeepCore feature is OFF, so this build behaves as stock OpenLRR. "
			  "That is the default. Edit Settings\\DeepCore.cfg to turn something on.");
	}
	Write(Level::Info, "%s", "----------------------------------------------------------------");
}
