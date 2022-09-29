// Errors.h : 
//

#pragma once

#include "../../common.h"


namespace Gods98
{; // !<---

/**********************************************************************************
 ******** Forward Declarations
 **********************************************************************************/

#pragma region Forward Declarations

struct File; // from `engine/core/Files.h`

#pragma endregion

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define ERROR_LOADLOG			"Errors\\loadLog.dmp"
#define ERROR_LOADERRORLOG		"Errors\\loadErrorLog.dmp"
#define ERROR_REDUNDANTLOG		"Errors\\redundantFiles.dmp"

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

enum class Error_LoadError : sint32
{
	InvalidFName,
	UnableToOpen,
	UnableToOpenForWrite,
	UnableToVerifyName,
	RMTexture,

	Count,
};
assert_sizeof(Error_LoadError, 0x4);

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct Error_Globs
{
	// [globs: start]
	/*000,4*/ File* dumpFile;
	/*004,4*/ File* loadLogFile;
	/*008,4*/ File* loadErrorLogFile;
	/*00c,4*/ File* redundantLogFile;
	/*010,400*/ char loadLogName[1024];
	/*410,400*/ char redundantLogName[1024];
	/*810,4*/ bool32 warnCalled;
	/*814,4*/ bool32 fullScreen;
	// [globs: end]
	/*818*/
};
assert_sizeof(Error_Globs, 0x818);


struct Error_LogLevels
{
	bool debugVisible;
	bool traceVisible;
	bool infoVisible;
	bool warnVisible;
	bool fatalVisible;
};

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @00576ce0>
extern Error_Globs & errorGlobs;

extern Error_LogLevels errorLogLevels;

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define Error_DebugF2(s, ...)				do { if (Gods98::Error_IsDebugVisible()) { Gods98::Error_Out(false, (s), __VA_ARGS__); } } while (0)
#define Error_TraceF2(s, ...)				do { if (Gods98::Error_IsTraceVisible()) { Gods98::Error_Out(false, (s), __VA_ARGS__); } } while (0)
#define Error_InfoF2(s, ...)				do { if (Gods98::Error_IsInfoVisible())  { Gods98::Error_Out(false, (s), __VA_ARGS__); } } while (0)
#define Error_WarnF2(b, s, ...)				do { if (Gods98::Error_IsWarnVisible()  && (b)) { Gods98::Error_Out(false, (s), __VA_ARGS__); Gods98::Error_SetWarn(); } } while (0)
#define Error_FatalF2(b, s, ...)			do { if (Gods98::Error_IsFatalVisible() && (b)) { Gods98::Error_Out(true,  (s), __VA_ARGS__); } } while (0)

// Debug is used to log information without the "file(line):" prefix, and without automatically ending the line.
#define Error_Debug(s)						Error_DebugF2("%s",   (s))
#define Error_Trace(s)						Error_TraceF2("%s\n", (s))
#define Error_Info(s)						Error_InfoF2( "%s\n", (s))
#define Error_Warn(b, s)					Error_WarnF2( (b), "%s(%i): Warning: %s\n", __FILE__, __LINE__, (s))
#define Error_Fatal(b, s)					Error_FatalF2((b), "%s(%i): Fatal: %s\n",   __FILE__, __LINE__, (s))

#define Error_DebugF(s, ...)				Error_Debug(Gods98::Error_Format((s), __VA_ARGS__))
#define Error_TraceF(s, ...)				Error_Trace(Gods98::Error_Format((s), __VA_ARGS__))
#define Error_InfoF(s, ...)					Error_Info( Gods98::Error_Format((s), __VA_ARGS__))
#define Error_WarnF(b, s, ...)				Error_Warn( (b), Gods98::Error_Format((s), __VA_ARGS__))
#define Error_FatalF(b, s, ...)				Error_Fatal((b), Gods98::Error_Format((s), __VA_ARGS__))

#define Error_LogLoad(b, s)					do { Gods98::Error_Log(errorGlobs.loadLogFile,      (b), "%s\n", (s)); } while (0)
#define Error_LogLoadError(b, s)			do { Gods98::Error_Log(errorGlobs.loadErrorLogFile, (b), "%s\n", (s)); } while (0)
#define Error_LogRedundantFile(b, s)		do { Gods98::Error_Log(errorGlobs.redundantLogFile, (b), "%s\n", (s)); } while (0)

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0048b520>
void __cdecl Error_Initialise(void);

// <LegoRR.exe @0048b540>
void __cdecl Error_FullScreen(bool32 on);

// <LegoRR.exe @0048b550>
void __cdecl Error_CloseLog(void);

// <LegoRR.exe @0048b5b0>
void __cdecl Error_Shutdown(void);

// <missing>
bool32 __cdecl Error_SetDumpFile(const char* errors, const char* loadLog, const char* loadErrorLog, const char* rendundantLog);

// <missing>
void __cdecl Error_DebugBreak(void);

// <missing>
void __cdecl Error_TerminateProgram(const char* msg);

// <missing>
const char* __cdecl Error_Format(const char* msg, ...);

// <missing>
void __cdecl Error_Out(bool32 ErrFatal, const char* lpOutputString, ...);

// <missing>
void __cdecl Error_Log(File* logFile, bool32 log, const char* lpOutputString, ...);

// <missing>
void Error_CheckWarn(bool32 check);


__inline void Error_SetWarn(void) { errorGlobs.warnCalled = true; }


inline bool Error_IsDebugVisible()					{ return errorLogLevels.debugVisible; }
inline void Error_SetDebugVisible(bool visible)		{ errorLogLevels.debugVisible = visible; }

inline bool Error_IsTraceVisible()					{ return errorLogLevels.traceVisible; }
inline void Error_SetTraceVisible(bool visible)		{ errorLogLevels.traceVisible = visible; }

inline bool Error_IsInfoVisible()					{ return errorLogLevels.infoVisible; }
inline void Error_SetInfoVisible(bool visible)		{ errorLogLevels.infoVisible = visible; }

inline bool Error_IsWarnVisible()					{ return errorLogLevels.warnVisible; }
inline void Error_SetWarnVisible(bool visible)		{ errorLogLevels.warnVisible = visible; }

inline bool Error_IsFatalVisible()					{ return errorLogLevels.fatalVisible; }
inline void Error_SetFatalVisible(bool visible)		{ errorLogLevels.fatalVisible = visible; }

#pragma endregion

}
