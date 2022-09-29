// Config.h : 
//
/// APIS: -
/// DEPENDENCIES: Files, Keys, Maths, Utils, (Errors, Memory)
/// DEPENDENTS: Containers, Advisor, Creature, Bubble, Building, Dependencies, Effects,
///             Encyclopedia, FrontEnd, HelpWindow, Lego, LightEffects, Objective,
///             ObjInfo, Panel, Priorities, PTL, Rewards, Stats, Upgrade, Vehicle, Weapons

#pragma once

#include "../../common.h"

#include "../colour.h"
#include "../geometry.h"

//#include "../input/Keys.h"
#include "ListSet.hpp"


namespace Gods98
{; // !<---

/**********************************************************************************
 ******** Forward Declarations
 **********************************************************************************/

#pragma region Forward Declarations

enum Keys : uint8; // from `engine/input/Keys.h`

enum_scoped_forward(FileFlags) : uint32;
enum_scoped_forward_end(FileFlags);

#pragma endregion

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define CONFIG_MAXDEPTH				100
#define CONFIG_BINARYFILEID			"GBCF"

#define CONFIG_CONVERTIONINCPC		200

#define CONFIG_MAXSTRINGID			1024

#define CONFIG_MAXLISTS				32			// 2^32 - 1 possible configs...

#define CONFIG_SEPARATOR			"::"

#define CONFIG_ILLEGALCOMMENT		"//"		// Often seen as "// ROCK FOG", this comment type is NOT SUPPORTED!

#define CONFIG_COMMENTCHAR			';'
#define CONFIG_COMMENT				";"

#define CONFIG_WILDCARDCHAR			'*'
#define CONFIG_WILDCARD				"*"

#define CONFIG_OPENBLOCKCHAR		'{'
#define CONFIG_OPENBLOCK			"{"

#define CONFIG_CLOSEBLOCKCHAR		'}'
#define CONFIG_CLOSEBLOCK			"}"

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

enum Config_GlobFlags : uint32
{
	CONFIG_GLOB_FLAG_NONE          = 0,

	CONFIG_GLOB_FLAG_INITIALISED   = 0x1,
	CONFIG_GLOB_FLAG_LOADINGCONFIG = 0x2,
};
flags_end(Config_GlobFlags, 0x4);


enum class Config_ReadStage : sint32
{
	PreName = 0,
	Name,
	PreData,
	Data,

	End,
};
assert_sizeof(Config_ReadStage, 0x4);

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

/// TODO: Consider name changes for `itemName`, `dataString` fields to `key`, `value`.
struct Config
{
	/*00,4*/ char* fileData; // held exclusively by root Config node, holds string used for all `itemName`, `dataString` fields.
	/*04,4*/ const char* itemName;   // (key)
	/*08,4*/ const char* dataString; // (value)
	/*0c,4*/ uint32 depth;
	/*10,4*/ uint32 itemHashCode; // (unused field)
	/*14,4*/ Config* linkNext;
	/*18,4*/ Config* linkPrev;
	/*1c,4*/ Config* nextFree; // (listSet field)
	/*20*/
	/// EXPANSION:
	uint32 lineNumber; // 1-indexed
	char* fileName;
};
//assert_sizeof(Config, 0x20);


struct Config_Globs
{
	/*000,400*/ char s_JoinPath_string[CONFIG_MAXSTRINGID]; // (not part of Globs, static array in BuildStringID func)
	/*400,80*/ Config* listSet[CONFIG_MAXLISTS];
	/*480,4*/ Config* freeList;
	/*484,4*/ uint32 listCount;
	/*488,4*/ Config_GlobFlags flags;
	/*48c*/
};
assert_sizeof(Config_Globs, 0x48c);


using Config_ListSet = ListSet::WrapperCollection<Config_Globs>;

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @00507098>
extern Config_Globs & configGlobs;

extern Config_ListSet configListSet;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

#ifdef DEBUG
	#define Config_CheckInit()			if (!(configGlobs.flags & Config_GlobFlags::CONFIG_GLOB_FLAG_INITIALISED)) Error_Fatal(true, "Error: Config_Intitialise() Has Not Been Called");
#else
	#define Config_CheckInit()
#endif

// <LegoRR.exe @004790b0>
void __cdecl Config_Initialise(void);

// <LegoRR.exe @004790e0>
void __cdecl Config_Shutdown(void);

/*void Config_SetCharacterTable(const char* fname);
void Config_SetCharacterConvertFile(const char* fname);
void Config_ReadCharacterTable(const char* fname);
char __cdecl Config_ConvertCharacter(char c);
void __cdecl Config_SetLanguageDatabase(const char* langFile);
void __cdecl Config_ReadLanguageDatabase(const char* langFile);
char* __cdecl Config_ConvertString(const char* s, const char* sectionName, uint32* size, sint32 spaceToUnderscore);
void __cdecl Config_DumpUnknownPhrases(const char* fname);
void* __cdecl Config_LoadConvertedText(const char* fname, uint32* fileSize);*/

// <LegoRR.exe @00479120>
Config* __cdecl Config_Load(const char* filename);

/// CUSTOM: Loads a configuration file, with additional flags specifying where and what checks are used to open it.
Config* __cdecl Config_Load2(const char* filename, FileFlags fileFlags);

// Always use the Config_ID macro, and never call this directly.
// Null must always be passed to terminate the arguments list, which is automatically handled by Config_ID.
// Calling this again will invalidate the previous result.
// <LegoRR.exe @00479210>
const char* __cdecl Config_BuildStringID(const char* s, ...);

/// CUSTOM: Returns the last result of Config_BuildStringID or Config_ID.
const char* Config_LastStringID();

/// CUSTOM: Builds and returns a string ID for the config property. Calling this again will invalidate the previous result.
const char* Config_GetStringID(const Config* conf);

/// CUSTOM: Gets the parent property of the config property. Returns null if this is a root property.
const Config* Config_GetParentItem(const Config* conf);

/// CUSTOM: Gets the line number of the config property with the given string ID. Returns 0 on failure.
uint32 Config_GetLineNumberOf(const Config* root, const char* stringID);

/// CUSTOM: Gets the line number of the config property.
uint32 Config_GetLineNumber(const Config* conf);

/// CUSTOM: Gets the filename of the config this property with the given string ID was loaded from. Returns "<Config>" on failure.
///         This is useful for merged configs, where a definition may be in a different file from the root config.
const char* Config_GetFileNameOf(const Config* root, const char* stringID);

/// CUSTOM: Gets the filename of the config this property was loaded from. Returns "<Config>" on failure.
const char* Config_GetFileName(OPTIONAL const Config* conf);

// <inlined>
__inline const char* Config_GetItemName(const Config* conf) { return conf->itemName; }

// <inlined>
__inline const char* Config_GetDataString(const Config* conf) { return conf->dataString; }

// <LegoRR.exe @004792b0>
const Config* __cdecl Config_FindArray(const Config* root, const char* name);

// <LegoRR.exe @004792e0>
const Config* __cdecl Config_GetNextItem(const Config* start);

// <LegoRR.exe @00479310>
char* Config_GetStringValue(const Config* root, const char* stringID);

// <LegoRR.exe @00479370>
const char* __cdecl Config_GetTempStringValue(const Config* root, const char* stringID);

// <LegoRR.exe @00479390>
BoolTri __cdecl Config_GetBoolValue(const Config* root, const char* stringID);

// <LegoRR.exe @004793d0>
real32 __cdecl Config_GetAngle(const Config* root, const char* stringID);

// <LegoRR.exe @00479430>
bool32 __cdecl Config_GetRGBValue(const Config* root, const char* stringID, OUT real32* r, OUT real32* g, OUT real32* b);

/// CUSTOM:
bool Config_GetColourRGBF(const Config* root, const char* stringID, OUT ColourRGBF* colour);

// <missing>
bool32 __cdecl Config_GetCoord(const Config* root, const char* stringID, OUT real32* x, OUT real32* y, OPTIONAL OUT real32* z);


bool Config_GetIntValues(const Config* root, const char* stringID, const char* sep, OUT sint32* values, uint32 count);

bool Config_GetRealValues(const Config* root, const char* stringID, const char* sep, OUT real32* values, uint32 count);


bool Config_GetPoint2I(const Config* root, const char* stringID, const char* sep, OUT Point2I* point);

bool Config_GetSize2I(const Config* root, const char* stringID, const char* sep, OUT Size2I* size);

bool Config_GetRange2I(const Config* root, const char* stringID, const char* sep, OUT Range2I* range);

bool Config_GetArea2I(const Config* root, const char* stringID, const char* sep, OUT Area2I* area);

bool Config_GetRect2I(const Config* root, const char* stringID, const char* sep, OUT Rect2I* rect);


bool Config_GetPoint2F(const Config* root, const char* stringID, const char* sep, OUT Point2F* point);

bool Config_GetSize2F(const Config* root, const char* stringID, const char* sep, OUT Size2F* size);

bool Config_GetRange2F(const Config* root, const char* stringID, const char* sep, OUT Range2F* range);

bool Config_GetArea2F(const Config* root, const char* stringID, const char* sep, OUT Area2F* area);

bool Config_GetRect2F(const Config* root, const char* stringID, const char* sep, OUT Rect2F* rect);

bool Config_GetVector3F(const Config* root, const char* stringID, const char* sep, OUT Vector3F* vector);

bool Config_GetVector4F(const Config* root, const char* stringID, const char* sep, OUT Vector4F* vector);


// <missing>
bool32 __cdecl Config_GetKey(const Config* root, const char* stringID, OUT Keys* key);

// <LegoRR.exe @00479500>
void __cdecl Config_Free(Config* root);


// Used by Error_TerminateProgram
// <debug>
const char* __cdecl Config_Debug_GetLastFind(void);


// <LegoRR.exe @00479530>
Config* __cdecl Config_Create(Config* prev);

// <LegoRR.exe @00479580>
void __cdecl Config_Remove(Config* dead);

/// CUSTOM:
uint32 Config_HashItemName(const char* name);

/// CUSTOM: Subfunction of Config_FindItem.
bool Config_MatchItemName(const Gods98::Config* conf, const char* name, OPTIONAL uint32 hashCode, OPTIONAL OUT bool* wildcard);

// <LegoRR.exe @004795a0>
const Config* __cdecl Config_FindItem(const Config* conf, const char* stringID);

/// LEGACY:
// <LegoRR.exe @00479750>
void __cdecl Config_AddList(void);

/// CUSTOM: Count number of items in an array.
uint32 Config_CountItems(const Config* arrayItem);

/// CUSTOM: Merge configs together by appending one to another.
void Config_AppendConfig(Config* root, Config* config);

/// CUSTOM: Future replacement for Config_GetIntValue macro. Name will have 2 removed once replaced.
sint32 Config_GetIntValue2(const Config* root, const char* stringID);

/// CUSTOM: Future replacement for Config_GetRealValue macro. Name will have 2 removed once replaced.
real32 Config_GetRealValue2(const Config* root, const char* stringID);


#define Config_GetIntValue(c,s)		Gods98::Config_GetIntValue2((c),(s))
#define Config_GetRealValue(c,s)	Gods98::Config_GetRealValue2((c),(s))
//#define Config_GetIntValue(c,s)		std::atoi(Gods98::Config_GetTempStringValue((c),(s))?Gods98::Config_GetTempStringValue((c),(s)):"")
//#define Config_GetRealValue(c,s)	(real32)std::atof(Gods98::Config_GetTempStringValue((c),(s))?Gods98::Config_GetTempStringValue((c),(s)):"")
#define Config_Get3DCoord(c,s,v)	Gods98::Config_GetCoord((c),(s),&((v)->x),&((v)->y),&((v)->z))
#define Config_Get2DCoord(c,s,x,y)	Gods98::Config_GetCoord((c),(s),(x),(y),nullptr)

// Returns true only if the value is found, and is a valid TRUE constant.
#define Config_GetBoolOrFalse(c,s)	(Gods98::Config_GetBoolValue((c),(s)) == BoolTri::BOOL3_TRUE)
// Returns false only if the value is found, and IS a valid FALSE constant.
#define Config_GetBoolOrTrue(c,s)	(Gods98::Config_GetBoolValue((c),(s)) != BoolTri::BOOL3_FALSE)

/// CUSTOM:
#define Config_ID(s, ...) Gods98::Config_BuildStringID(s, __VA_ARGS__, nullptr)

// Error reporting for the passed config item.

#define Config_DebugItem(conf, s)				Error_DebugF2("%s (%i): %s",   Gods98::Config_GetFileName((conf)), Gods98::Config_GetLineNumber((conf)), (s))
#define Config_InfoItem(conf, s)				Error_InfoF2( "%s (%i): %s\n", Gods98::Config_GetFileName((conf)), Gods98::Config_GetLineNumber((conf)), (s))
#define Config_WarnItem(b, conf, s)				Error_WarnF2( (b), "%s (%i): Warning: %s\n", Gods98::Config_GetFileName((conf)), Gods98::Config_GetLineNumber((conf)), (s))
#define Config_FatalItem(b, conf, s)			Error_FatalF2((b), "%s (%i): Fatal: %s\n",   Gods98::Config_GetFileName((conf)), Gods98::Config_GetLineNumber((conf)), (s))

#define Config_DebugItemF(conf, s, ...)			Config_DebugItem((conf), Gods98::Error_Format((s), __VA_ARGS__))
#define Config_InfoItemF(conf, s, ...)			Config_InfoItem( (conf), Gods98::Error_Format((s), __VA_ARGS__))
#define Config_WarnItemF(b, conf, s, ...)		Config_WarnItem( (b), (conf), Gods98::Error_Format((s), __VA_ARGS__))
#define Config_FatalItemF(b, conf, s, ...)		Config_FatalItem((b), (conf), Gods98::Error_Format((s), __VA_ARGS__))

// Error reporting for the last config item looked up with Config_BuildStringID / Config_ID.

// Defaults to root when last item wasn't found.
#define _Config_LastStringIDItem(root)			(Gods98::Config_FindItem((root), Gods98::Config_LastStringID()) \
													? Gods98::Config_FindItem((root), Gods98::Config_LastStringID()) \
													: (root))

#define Config_DebugLast(root, s)				Config_DebugItem(_Config_LastStringIDItem((root)), (s))
#define Config_InfoLast(root, s)				Config_InfoItem( _Config_LastStringIDItem((root)), (s))
#define Config_WarnLast(b, root, s)				Config_WarnItem( (b), _Config_LastStringIDItem((root)), (s))
#define Config_FatalLast(b, root, s)			Config_FatalItem((b), _Config_LastStringIDItem((root)), (s))

#define Config_DebugLastF(root, s, ...)			Config_DebugItemF(_Config_LastStringIDItem((root)), (s), __VA_ARGS__)
#define Config_InfoLastF(root, s, ...)			Config_InfoItemF( _Config_LastStringIDItem((root)), (s), __VA_ARGS__)
#define Config_WarnLastF(b, root, s, ...)		Config_WarnItemF( (b), _Config_LastStringIDItem((root)), (s), __VA_ARGS__)
#define Config_FatalLastF(b, root, s, ...)		Config_FatalItemF((b), _Config_LastStringIDItem((root)), (s), __VA_ARGS__)

#pragma endregion

}
