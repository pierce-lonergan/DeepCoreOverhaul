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


struct Error_Globs2
{
	bool debugVisible;
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

extern Error_Globs2 errorGlobs2;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

//#ifndef _RELEASE

#define Error_Debug(s)						{ if (Gods98::errorGlobs2.debugVisible) { Gods98::Error_Out(false, "%s\n", (s)); } }
#define Error_Info(s)						{ if (Gods98::errorGlobs2.infoVisible) { Gods98::Error_Out(false, "%s\n", (s)); } }
#define Error_Warn(b, s)					{ if (Gods98::errorGlobs2.warnVisible && (b)) { Gods98::Error_Out(false, "%s(%i): Warning: %s\n", __FILE__, __LINE__, (s)); Gods98::Error_SetWarn(); } }
#define Error_Fatal(b, s)					{ if (Gods98::errorGlobs2.fatalVisible && (b)) { Gods98::Error_Out(true, "%s(%i): Fatal: %s\n", __FILE__, __LINE__, (s)); } }

#define Error_DebugF(s, ...)				Error_Debug(Gods98::Error_Format((s), __VA_ARGS__))
#define Error_InfoF(s, ...)					Error_Info(Gods98::Error_Format((s), __VA_ARGS__))
#define Error_WarnF(b, s, ...)				Error_Warn((b), Gods98::Error_Format((s), __VA_ARGS__))
#define Error_FatalF(b, s, ...)				Error_Fatal((b), Gods98::Error_Format((s), __VA_ARGS__))

#define Error_LogLoad(b, s)					{ Gods98::Error_Log(errorGlobs.loadLogFile, (b), "%s\n", (s)); }
#define Error_LogLoadError(b, s)			{ Gods98::Error_Log(errorGlobs.loadErrorLogFile, (b), "%s\n", (s)); }
#define Error_LogRedundantFile(b, s)		{ Gods98::Error_Log(errorGlobs.redundantLogFile, (b), "%s\n", (s)); }

/*#else

#define Error_Warn(b,s)			(b)
#define Error_Fatal(b,s)		(b)
#define Error_Debug(s)
__inline void Error_CheckWarn(bool32 check) {  }
#define Error_LogLoad(b,s)					(b)
#define Error_LogLoadError(b,s)				(b)
#define Error_LogRedundantFile(b,s)			(b)

#endif*/

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


__inline void Error_SetWarn(void) { errorGlobs.warnCalled = true; }
__inline void Error_CheckWarn(bool32 check) { if (!check) errorGlobs.warnCalled = false; else if (errorGlobs.warnCalled) Error_TerminateProgram("Check warning message log"); }


inline bool Error_IsDebugVisible()					{ return errorGlobs2.debugVisible; }
inline void Error_SetDebugVisible(bool visible)		{ errorGlobs2.debugVisible = visible; }

inline bool Error_IsInfoVisible()					{ return errorGlobs2.infoVisible; }
inline void Error_SetInfoVisible(bool visible)		{ errorGlobs2.infoVisible = visible; }

inline bool Error_IsWarnVisible()					{ return errorGlobs2.warnVisible; }
inline void Error_SetWarnVisible(bool visible)		{ errorGlobs2.warnVisible = visible; }

inline bool Error_IsFatalVisible()					{ return errorGlobs2.fatalVisible; }
inline void Error_SetFatalVisible(bool visible)		{ errorGlobs2.fatalVisible = visible; }

#pragma endregion

}
