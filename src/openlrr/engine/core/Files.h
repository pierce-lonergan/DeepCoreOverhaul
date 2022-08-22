// Files.h : 
//
/// APIS: -
/// DEPENDENCIES: Errors, Memory, Registry, Utils, Wad
/// DEPENDENTS: Containers, Advisor, Creature, Bubble, Building, Dependencies, Effects,
///             Encyclopedia, FrontEnd, HelpWindow, Lego, LightEffects, Objective,
///             ObjInfo, Panel, Priorities, PTL, Rewards, Stats, Upgrade, Vehicle, Weapons

#pragma once

#include "../../common.h"


namespace Gods98
{; // !<---

/**********************************************************************************
 ******** Function Typedefs
 **********************************************************************************/

#pragma region Function Typedefs

typedef void (__cdecl* FileLoadCallback)(const char* filename, uint32 fileSize, void* data);

#pragma endregion

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define MAX_WAD_BASE_PATH				1024

#define FILE_DATADIRNAME				"Data"
#define FILE_KEYFILENAME				"cd.key"
#define FILE_DELMEFILENAME				"delme.dat"
#define FILE_CONFIGFILENAME				"lego.cfg"
#define FILE_MAXPATH					260

#define FILE_DEBUG_RLISTSIZE			2000

#ifndef WAD_ERROR
#define WAD_ERROR						(static_cast<Wad_HandleValue>(-1))
#endif

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

enum class SeekOrigin : sint32
{
	Set     = 0, // SEEK_SET
	Current = 1, // SEEK_CUR
	End     = 2, // SEEK_END
};
assert_sizeof(SeekOrigin, 0x4);


enum class FileSys : sint32
{
	Wad      = 0, // Wad file
	Standard = 1, // Standard C Libraries
	Error    = 2, // Error code
};
assert_sizeof(FileSys, 0x4);


// Flags used to specify how and where a file is opened.
flags_scoped(FileFlags) : uint32
{
	FILE_FLAG_DATADIR      = 0x0,  // File path will have "<dataDir>\\" prepended.
	FILE_FLAG_EXEDIR       = 0x1,  // File path will have "<exeDir>\\" (current dir) prepended.
	FILE_FLAG_BASEDIR      = FILE_FLAG_EXEDIR, // Same as: FILE_FLAG_EXEDIR.
	FILE_FLAG_ABSOLUTEPATH = 0x2,  // File path will be used as-is.
								   // DO NOT USE THIS FLAG FOR USER-PROVIDED CONTENT.

	FILE_FLAGS_PATHMASK    = (FILE_FLAG_DATADIR|FILE_FLAG_EXEDIR|FILE_FLAG_ABSOLUTEPATH),

	FILE_FLAG_NOSTD        = 0x10, // Exclude looking in the standard file system.
	FILE_FLAG_NOWAD        = 0x20, // Exclude looking in WAD files.
								   // Used with: FILE_FLAG_DATADIR
	FILE_FLAG_NOCD         = 0x40, // Exclude looking in the CD file system.
								   // Used with: FILE_FLAG_DATADIR, FILE_FLAG_EXEDIR (FILE_FLAG_BASEDIR)

	//FILE_FLAG_DATAPRIORITY = 0x80, // Files will attempt to load from the dataDir before checking WAD files. (excludes CD files)

	FILE_FLAG_NOVERIFY     = 0x100,  // File path will not be checked for rogue ".\\" or "..\\" path parts.
								     // DO NOT USE THIS FLAG FOR USER-PROVIDED CONTENT.
								     // Used with: FILE_FLAG_DATADIR, FILE_FLAG_EXEDIR (FILE_FLAG_BASEDIR)


	FILE_FLAGS_DEFAULT     = (FILE_FLAG_DATADIR), // Default flags used when not using File_<Function>2 calls.
};
flags_scoped_end(FileFlags, 0x4);

#pragma endregion

/**********************************************************************************
 ******** Typedefs
 **********************************************************************************/

#pragma region Typedefs

#ifndef _WAD_HANDLEVALUE_TYPEDEF_
#define _WAD_HANDLEVALUE_TYPEDEF_
typedef sint32 Wad_HandleValue;
#endif

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

///////////////////////////////////////////////////////////////////////////////////////////
// Types and structures
//		FileSys - the file system used ie. wad or normal
//		WADFILE - structure to represent a wad file
//		File    - strucrure to represent a file (of either type)
///////////////////////////////////////////////////////////////////////////////////////////

struct WADFILE
{
	/*0,4*/ sint32 hFile;			// Handle to file in the wad
	/*4,4*/ sint32 streamPos;		// Position in stream of the file.  Indexes are from 0-length-1 inclusive.
	/*8,4*/ bool32 eof;				// Has the file hit EOF.
	/*c*/
};
assert_sizeof(WADFILE, 0xc);


struct File
{
	/*0,4*/ FileSys type;
	/*4,4*/ union {
		FILE* std; // stored as `void*` in Gods98
		WADFILE* wad;
	};
	/*8*/
};
assert_sizeof(File, 0x8);


struct File_Globs
{
	/*000,400*/ char wadBasePath[MAX_WAD_BASE_PATH];
	/*400,400*/ char s_GetWadName_wadedName[MAX_WAD_BASE_PATH];
	/*800,104*/ char s_VerifyFilename_full[FILE_MAXPATH];
	/*904,4*/ FILE* s_ErrorFile_f;		// (address not known)
	// [globs: start]
	/*908,104*/ char dataDir[FILE_MAXPATH];
	/*a0c,4*/ FileLoadCallback loadCallback;
	/*a10,4*/ void* loadCallbackData;
	/*a14,1*/ char cdLetter;
	// [globs: end]
	/*a15,3*/ uint8 padding1[3]; // padding not needed, but included for safety
	/*a18,4*/ bool32 basePathSet;
	/*a1c,4*/ bool32 fileLogFileAccess;		// (address not known)
	/*a20*/
};
assert_sizeof(File_Globs, 0xa20);


struct FileCheck_Globs
{
	/*000000,1f4000*/ char loadedList[FILE_DEBUG_RLISTSIZE][1024]; // MAYBE USE LINKED LIST (...maybe!???)
	/*1f4000,4*/ uint32 numInList;
	/*1f4004*/
};
assert_sizeof(FileCheck_Globs, 0x1f4004);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004abf00>
extern sint32 & currWadHandle; // = -1;

// <LegoRR.exe @005349a0>
extern File_Globs & fileGlobs;

// <LegoRR.exe @005779e0>
extern FileCheck_Globs & checkGlobs;

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define StdFile(f) ((FILE*)((f)->std))
#define WadFile(f) ((WADFILE*)((f)->wad))

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// multiple-uses (logf_removed)
// <LegoRR.exe @00484f50>
void __cdecl File_ErrorFile(const char* msg, ...);


// <LegoRR.exe @0047f3f0>
void __cdecl File_Initialise(const char* programName, bool32 insistOnCD, const char* registryLocation);

// <LegoRR.exe @0047f7b0>
bool32 __cdecl File_FindDataCD(void);

// This allows the translation from File paths into Wad names.
// <LegoRR.exe @0047f850>
bool32 __cdecl File_SetBaseSearchPath(const char* basePath);

// <LegoRR.exe @0047f8c0>
void __cdecl File_Error(const char* msg, ...);

// Extra functions for using with Wads.
// <LegoRR.exe @0047f900>
sint32 __cdecl File_LoadWad(const char* fName);

// <missing>
void __cdecl File_CloseWad(sint32 hWad);

// <missing>
void __cdecl File_SetActiveWad(sint32 hWad);

// <LegoRR.exe @0047f920>
bool32 __cdecl File_GetCDFilePath(IN OUT char* path, const char* fname);

// C library implementation

// <LegoRR.exe @0047f960>
void __cdecl File_MakeDir(const char* path);

/// CUSTOM: Creates a directory, with additional flags specifying where to create it.
void File_MakeDir2(const char* path, FileFlags fileFlags);

/// CUSTOM: Subfunction of File_Open
File* File_FromStandardFile(FILE* stdfile);

// <LegoRR.exe @0047f9a0>
File* __cdecl File_Open(const char* fName, const char* mode);

/// CUSTOM: Opens a file, with additional flags specifying where and what checks are used to open it.
File* File_Open2(const char* fName, const char* mode, FileFlags fileFlags);

// <LegoRR.exe @0047fb10>
sint32 __cdecl File_Seek(File* f, sint32 pos, SeekOrigin mode);

// <LegoRR.exe @0047fc40>
sint32 __cdecl File_Read(OUT void* buffer, sint32 size, sint32 count, File* f);

// <LegoRR.exe @0047fd10>
sint32 __cdecl File_Write(const void* buffer, sint32 size, sint32 count, File* f);

// <LegoRR.exe @0047fd80>
sint32 __cdecl File_Close(File* f);

// <missing>
sint32 __cdecl File_EOF(File* f);

// <LegoRR.exe @0047fdd0>
sint32 __cdecl File_Tell(File* f);

// <missing>
sint32 __cdecl File_Flush(File* f);

// <LegoRR.exe @0047fe20>
bool32 __cdecl File_Exists(const char* fName);

/// CUSTOM: Checks for a file's existence, with additional flags specifying where and what checks are used.
bool32 File_Exists2(const char* fName, FileFlags fileFlags);

// <LegoRR.exe @0047fee0>
sint32 __cdecl File_GetC(File* f);

// <LegoRR.exe @0047ff60>
sint32 __cdecl File_Length(File* f);

// <LegoRR.exe @0047ffa0>
char* __cdecl File_InternalFGetS(OUT char* fgetsBuffer, sint32 num, File* f);

// <LegoRR.exe @00480000>
char* __cdecl File_GetS(OUT char* fgetsBuffer, sint32 num, File* f);

// <LegoRR.exe @00480070>
sint32 __cdecl File_PrintF(File* f, const char* msg, ...);

// <missing>
uint32 __cdecl File_VPrintF(File* f, const char* msg, std::va_list args);

// <missing>
sint32 __cdecl File_ScanF(File* f, const char* msg, ...);


// <LegoRR.exe @004800e0>
FileSys __cdecl _File_GetSystem(File* f);

// <LegoRR.exe @004800f0>
FileSys __cdecl _File_CheckSystem(const char* fName, const char* mode);

/// CUSTOM: Checks which file system a file is found in, with additional flags specifying where to check.
FileSys _File_CheckSystem2(const char* fName, const char* mode, FileFlags fileFlags);

// <LegoRR.exe @00480160>
bool32 __cdecl _File_OpenWad(WADFILE* wad, const char* fName);

// <LegoRR.exe @00480190>
File* __cdecl _File_Alloc(FileSys fType);

// <LegoRR.exe @004801f0>
void* __cdecl _File_Malloc(uint32 size);

// <LegoRR.exe @00480200>
void __cdecl _File_Free(void* ptr);

// <LegoRR.exe @00480210>
void __cdecl _File_Dealloc(File* file);

// <LegoRR.exe @00480280>
const char* __cdecl _File_GetWadName(const char* fName);

// <LegoRR.exe @00480310>
char* __cdecl File_GetLine(OUT char* buffer, uint32 size, File* file);

// <LegoRR.exe @00480360>
void* __cdecl File_LoadBinary(const char* filename, OPTIONAL OUT uint32* sizeptr);

// WARNING: Does not null-terminate data!
// <missing>
void* __cdecl File_LoadASCII(const char* filename, OPTIONAL OUT uint32* sizeptr);

// <LegoRR.exe @00480380>
void* __cdecl File_Load(const char* filename, OPTIONAL OUT uint32* sizeptr, bool32 binary);


/// CUSTOM: Extension of File_LoadBinary that allocates one extra byte and null-terminates the end of the buffer.
///         Extra byte IS NOT included in the value returned by sizeptr.
void* __cdecl File_LoadBinaryString(const char* filename, OPTIONAL OUT uint32* sizeptr);

/// CUSTOM: Extension of File_LoadBinary that allocates one extra byte and null-terminates the end of the buffer.
///         Extra byte IS NOT included in the value returned by sizeptr.
///         Includes additional flags specifying where and what checks are used to load.
void* File_LoadBinaryString2(const char* filename, OPTIONAL OUT uint32* sizeptr, FileFlags fileFlags);

/// CUSTOM: Extension of File_Load to allow terminating the end of the buffer with extra bytes.
///         extraSize IS NOT included in the value returned by sizeptr.
///         Includes additional flags specifying where and what checks are used to load.
void* File_Load2(const char* filename, OPTIONAL OUT uint32* sizeptr, bool32 binary, uint32 extraSize, FileFlags fileFlags);


// <LegoRR.exe @00480430>
uint32 __cdecl File_LoadBinaryHandle(const char* filename, OPTIONAL OUT uint32* sizeptr);


/// CUSTOM: Extension of File_LoadBinaryHandle to allow ASCII and terminating the end of the buffer with extra bytes.
///         extraSize IS NOT included in the value returned by sizeptr.
///         Includes additional flags specifying where and what checks are used to load.
uint32 File_LoadHandle2(const char* filename, OPTIONAL OUT uint32* sizeptr, bool32 binary, uint32 extraSize, FileFlags fileFlags);


// <LegoRR.exe @004804e0>
const char* __cdecl File_VerifyFilename(const char* filename);

/// CUSTOM: Creates a full path filename, with additional flags specifying where the base directory is and whether to verify or not.
const char* File_VerifyFilename2(const char* filename, FileFlags fileFlags);

// <LegoRR.exe @00480570>
void __cdecl File_SetLoadCallback(FileLoadCallback Callback, void* data);

// <LegoRR.exe @00480590>
void __cdecl File_CheckRedundantFiles(const char* logName);

// <LegoRR.exe @00480650>
void __cdecl File_CheckDirectory(const char* dirName);

// <LegoRR.exe @00480830>
void __cdecl File_CheckFile(const char* fileName);


/// CUSTOM:
std::string File_GetWorkingDir();

/// CUSTOM:
std::string File_GetDataDir();

/// CUSTOM:
std::string File_GetExeDir();

// <missing>
//void File_SetDataDir(const char* newDataDir);

/// CUSTOM:
//void File_SetExeDir(const char* newExeDir);


/// CUSTOM: Sets the Data directory used for loose files lookup.
///         The argument will be converted to a full path.
///         This must be called before File_Initialise.
void File_SetDataDir(const std::string& initDataDir);

/// CUSTOM: Sets the directory location to use when searching for WAD files.
///         This must be called before File_Initialise.
void File_SetWadDir(const std::string& initWadDir);


/// CUSTOM: Gets if files should be looked for in the Data directory before WAD files.
bool File_IsDataPriority();

/// CUSTOM: Sets if files should be looked for in the Data directory before WAD files.
void File_SetDataPriority(bool dataFirst);

/// CUSTOM: Gets if the WAD files should be used for file lookup.
bool File_IsWadsEnabled();

/// CUSTOM: Sets if the WAD files should be used for file lookup.
void File_SetWadsEnabled(bool useWads);

/// CUSTOM: Gets if the CD should be used for file lookup.
bool File_IsCDEnabled();

/// CUSTOM: Sets if the CD should be used for file lookup.
void File_SetCDEnabled(bool useCD);

#pragma endregion

}

