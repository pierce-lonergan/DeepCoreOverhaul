// Config.cpp : 
//

#include "../input/Keys.h"
#include "Files.h"
#include "Errors.h"
#include "Maths.h"         // defines
#include "Memory.h"
#include "Utils.h"

#include "Config.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @00507098>
Gods98::Config_Globs & Gods98::configGlobs = *(Gods98::Config_Globs*)0x00507098; // = { nullptr };

Gods98::Config_ListSet Gods98::configListSet = Gods98::Config_ListSet(Gods98::configGlobs);

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @004790b0>
void __cdecl Gods98::Config_Initialise(void)
{
	log_firstcall();

	configListSet.Initialise();
	configGlobs.flags = Config_GlobFlags::CONFIG_GLOB_FLAG_INITIALISED;
}

// <LegoRR.exe @004790e0>
void __cdecl Gods98::Config_Shutdown(void)
{
	log_firstcall();

	configListSet.Shutdown();
	configGlobs.flags = Config_GlobFlags::CONFIG_GLOB_FLAG_NONE;
}

/*void __cdecl Gods98::Config_SetCharacterTable(const char* fname);
void __cdecl Gods98::Config_SetCharacterConvertFile(const char* fname);
void __cdecl Gods98::Config_ReadCharacterTable(const char* fname);
char __cdecl Gods98::Config_ConvertCharacter(char c);
void __cdecl Gods98::Config_SetLanguageDatabase(const char* langFile);
void __cdecl Gods98::Config_ReadLanguageDatabase(const char* langFile);
char* __cdecl Gods98::Config_ConvertString(const char* s, const char* sectionName, uint32* size, sint32 spaceToUnderscore);
void __cdecl Gods98::Config_DumpUnknownPhrases(const char* fname);
void* __cdecl Gods98::Config_LoadConvertedText(const char* fname, uint32* fileSize);*/


// <LegoRR.exe @00479120>
Gods98::Config* __cdecl Gods98::Config_Load(const char* filename)
{
	return Config_Load2(filename, FileFlags::FILE_FLAGS_DEFAULT);
}

/// CUSTOM: Loads a configuration file, with additional flags specifying where and what checks are used to open it.
Gods98::Config* __cdecl Gods98::Config_Load2(const char* filename, FileFlags fileFlags)
{
	log_firstcall();

	Config* rootConf = nullptr;
	uint32 fileSize, loop;
	char* s;
	char* fdata;

	configGlobs.flags |= Config_GlobFlags::CONFIG_GLOB_FLAG_LOADINGCONFIG;

	/// FIX APPLY: Use extension of File_LoadBinary function to null-terminate buffer.
	if (fdata = (char*)File_LoadBinaryString2(filename, &fileSize, fileFlags)) {

		rootConf = Config_Create(nullptr);
		rootConf->fileData = fdata;

		// Change any return/tab/blah/blah characters to zero...
		// Clear anything after a semi-colon until the next return character.

		bool commentMode = false;
		for (s = rootConf->fileData, loop = 0; loop < fileSize; loop++) {
			const char c = *s;

			if (c == CONFIG_COMMENTCHAR) commentMode = true;
			else if (c == '\n') commentMode = false;

			if (commentMode || (c == '\t' || c == '\n' || c == '\r' || c == ' ')) *s = '\0';

			s++;
		}

		// Replace the semi-colons that were removed by the language converter...
		//for (loop = 0; loop < fileSize; loop++) {
		//	if (rootConf->fileData[loop] == FONT_LASTCHARACTER + 1)
		//		rootConf->fileData[loop] = CONFIG_COMMENTCHAR;
		//}

		// Run through the file data and point in the config structures

		Config* conf = rootConf;
		for (s = rootConf->fileData, loop = 0; loop < fileSize; loop++) {
			const char c = *s;

			if (c != '\0') {
				const char cnext = s[1];

				if (c == CONFIG_CLOSEBLOCKCHAR && cnext == '\0') {
					// Close block.
					Error_WarnF((conf->itemName!=nullptr), "Config close brace \"%s\" used between item name and value.", CONFIG_CLOSEBLOCK);
					Error_FatalF((conf->depth == 0), "Config close brace \"%s\" used at depth 0.", CONFIG_CLOSEBLOCK);
					conf->depth--;
				}
				else if (conf->itemName == nullptr) {
					// Assign item key.
					Error_WarnF((c==CONFIG_OPENBLOCKCHAR && cnext=='\0'), "Config open brace \"%s\" used for item name.", CONFIG_OPENBLOCK);
					conf->itemName = s;
				}
				else {
					// Assign item value.
					conf->dataString = s;
					conf = Config_Create(conf);
					// Open block.
					if (c==CONFIG_OPENBLOCKCHAR && cnext=='\0') conf->depth++;
					Error_WarnF((c==CONFIG_CLOSEBLOCKCHAR && cnext=='\0'), "Config close brace \"%s\" used for item value.", CONFIG_CLOSEBLOCK);
				}

				// Skip whitespace.
				for ( ; loop < fileSize; loop++) if (*(s++) == '\0') break;
			}
			else {
				s++;
			}
		}

	}

	configGlobs.flags &= ~Config_GlobFlags::CONFIG_GLOB_FLAG_LOADINGCONFIG;

	return rootConf;
}

// <LegoRR.exe @00479210>
const char* __cdecl Gods98::Config_BuildStringID(const char* s, ...)
{
	log_firstcall();

	std::va_list args;
	const char* curr;

	//static char s_JoinPath_string[1024];
	std::strcpy(configGlobs.s_JoinPath_string, s);

	va_start(args, s);
	while (curr = va_arg(args, const char*)) {
		std::strcat(configGlobs.s_JoinPath_string, CONFIG_SEPARATOR);
		std::strcat(configGlobs.s_JoinPath_string, curr);
	}
	va_end(s);

	return configGlobs.s_JoinPath_string;
}

/*
// <inlined>
__inline const char* Gods98::Config_GetItemName(const Config* conf)
{
	return conf->itemName;
}

// <inlined>
__inline const char* Gods98::Config_GetDataString(const Config* conf)
{
	return conf->dataString;
}
*/

// <LegoRR.exe @004792b0>
const Gods98::Config* __cdecl Gods98::Config_FindArray(const Config* root, const char* name)
{
	log_firstcall();

	const Config* conf;
	if (conf = Config_FindItem(root, name)) {
		/// FIX LEGORR: Ensure next linked item is not null before accessing.
		if (conf->linkNext != nullptr && conf->depth < conf->linkNext->depth) {
			return conf->linkNext;
		}
	}
	return nullptr;
}

// <LegoRR.exe @004792e0>
const Gods98::Config* __cdecl Gods98::Config_GetNextItem(const Config* start)
{
	log_firstcall();

	uint32 level = start->depth;
	const Config* conf = start;

	while (conf = conf->linkNext) {
		if (conf->depth < level) return nullptr;
		if (conf->depth == level) return conf;
	}
	return nullptr;
}

// <LegoRR.exe @00479310>
char* Gods98::Config_GetStringValue(const Config* root, const char* stringID)
{
	log_firstcall();

	const Config* conf;
	if (conf = Config_FindItem(root, stringID)) {
		if (conf->dataString) {
			return Util_StrCpy(conf->dataString);
		}
	}
	return nullptr;
}

// <LegoRR.exe @00479370>
const char* __cdecl Gods98::Config_GetTempStringValue(const Config* root, const char* stringID)
{
	log_firstcall();

	const Config* conf;
	if (conf = Config_FindItem(root, stringID)) {
		return conf->dataString;
	}
	return nullptr;
}

// <LegoRR.exe @00479390>
BoolTri __cdecl Gods98::Config_GetBoolValue(const Config* root, const char* stringID)
{
	log_firstcall();

	/// FIX LEGORR: Don't bother allocating a new string,
	///              Util_GetBoolFromString won't modify it.
	const char* str;
	if (str = Config_GetTempStringValue(root, stringID)) {
		return Util_GetBoolFromString(str);
	}
	return BoolTri::BOOL3_ERROR;
}

// <LegoRR.exe @004793d0>
real32 __cdecl Gods98::Config_GetAngle(const Config* root, const char* stringID)
{
	log_firstcall();

	real32 degrees;
	if (degrees = Config_GetRealValue(root, stringID)) {
		return (degrees / 360.0f) * (2.0f * M_PI); // degrees to radians
	}
	return 0.0f;
}

// <LegoRR.exe @00479430>
bool32 __cdecl Gods98::Config_GetRGBValue(const Config* root, const char* stringID, OUT real32* r, OUT real32* g, OUT real32* b)
{
	log_firstcall();

	char* argv[100];
	bool res = false;

	char* str;
	if (str = Config_GetStringValue(root, stringID)) {
		if (Util_Tokenise(str, argv, ":") == 3) {
			*r = std::atoi(argv[0]) / 255.0f;
			*g = std::atoi(argv[1]) / 255.0f;
			*b = std::atoi(argv[2]) / 255.0f;

			res = true;
		}
		else Error_Warn(true, Error_Format("Invalid RBG entry '%s'", Config_GetTempStringValue(root, stringID)));

		Mem_Free(str);
	}

	return res;
}


// <missing>
bool32 __cdecl Gods98::Config_GetCoord(const Config* root, const char* stringID, OUT real32* x, OUT real32* y, OPTIONAL OUT real32* z)
{
	Error_Fatal(!x || !y, "Null passed as x or y");

	char* argv[100];
	bool res = false;

	char* str;
	if (str = Config_GetStringValue(root, stringID)) {
		uint32 argc = Util_Tokenise(str, argv, ",");

		if (z == nullptr) { // FORMAT: x,y
			if (argc == 2) {
				*x = (real32)std::atof(argv[0]);
				*y = (real32)std::atof(argv[1]);

				res = true;
			}
			else Error_Warn(true, Error_Format("Invalid 2D Coordinate entry '%s'", str));
		}
		else { // FORMAT: x,y,z
			if (argc == 3) {
				*x = (real32)std::atof(argv[0]);
				*y = (real32)std::atof(argv[1]);
				*z = (real32)std::atof(argv[2]);

				res = true;
			}
			else Error_Warn(true, Error_Format("Invalid 3D Coordinate entry '%s'", str));
		}

		Mem_Free(str);
	}

	return res;
}


// <missing>
bool32 __cdecl Gods98::Config_GetKey(const Config* root, const char* stringID, OUT Keys* key)
{
	const char* str;
	if (str = Config_GetTempStringValue(root, stringID)) {
		return Key_Find(str, key);
	}
	return false;
}


// <LegoRR.exe @00479500>
void __cdecl Gods98::Config_Free(Config* root)
{
	log_firstcall();

	Error_Fatal(root->fileData == nullptr, "Only pass the root (loaded) config structure to Config_Free()");

	while (root) {
		/// CHANGE: Allow other config properties to store string data allocations.
		if (root->fileData) {
			Mem_Free(root->fileData);
			root->fileData = nullptr;
		}

		Config* next = const_cast<Config*>(root->linkNext);
		Config_Remove(root);
		root = next;
	}
}


// Used by Error_TerminateProgram
// <debug>
const char* __cdecl Gods98::Config_Debug_GetLastFind(void)
{
	return "<not implemented>";
}


// <LegoRR.exe @00479530>
Gods98::Config* __cdecl Gods98::Config_Create(Config* prev)
{
	log_firstcall();

	Config_CheckInit();

	Config* newConfig = configListSet.Add();

	newConfig->fileData = nullptr;
	newConfig->itemName = nullptr;
	newConfig->dataString = nullptr;
	//newConfig->itemHashCode = 0; // unused field
	newConfig->linkNext = nullptr;

	if (prev != nullptr) { // Non-root config node
		prev->linkNext = newConfig;
		newConfig->linkPrev = prev;
		newConfig->depth = prev->depth;
	}
	else { // Root config node
		newConfig->linkPrev = nullptr;
		newConfig->depth = 0;
	}

//#ifdef _DEBUG_2
//	newConfig->accessed = false;
//#endif // _DEBUG_2

	return newConfig;
}

// <LegoRR.exe @00479580>
void __cdecl Gods98::Config_Remove(Config* dead)
{
	log_firstcall();

	Config_CheckInit();

//#ifdef _DEBUG_2
//	Error_Fatal(!dead, "NULL passed to Config_Remove()");
//	if (configGlobs.debugLastFind == dead) configGlobs.debugLastFind = NULL;
//#endif // _DEBUG_2

	configListSet.Remove(dead);
}

// <LegoRR.exe @004795a0>
const Gods98::Config* __cdecl Gods98::Config_FindItem(const Config* conf, const char* stringID)
{
	log_firstcall();

	// Search the list for the given item.

	const Config* foundConf = nullptr;

	char* argv[CONFIG_MAXDEPTH];
	char* tempstring = Util_StrCpy(stringID);
	uint32 count = Util_Tokenise(tempstring, argv, CONFIG_SEPARATOR);

	// First find anything that matches the depth of the request
	// then see if the hierarchy matches the request.

	while (conf) {
		if (conf->depth == count - 1) {

			bool wildcard = false;

			if (count == 1) {
				const char* s;
				uint32 index = 0;
				for (s = conf->itemName; *s != '\0'; s++) {
					if (*s == CONFIG_WILDCARDCHAR) break;
					index++;
				}
				if (*s == CONFIG_WILDCARDCHAR) {
					wildcard = (::_strnicmp(argv[count - 1], conf->itemName, index) == 0);
				}
			}

			if (wildcard || ::_stricmp(argv[count - 1], conf->itemName) == 0) {

				wildcard = false;

				// Look backwards down the list to check the hierarchy.
				uint32 currDepth = count - 1;
				const Config* backConf = conf;
				while (backConf) {
					if (backConf->depth == currDepth - 1) {
						if (currDepth == 1) {
							const char* s;
							uint32 index = 0;
							for (s = backConf->itemName; *s != '\0'; s++) {
								if (*s == CONFIG_WILDCARDCHAR) break;
								index++;
							}
							if (*s == CONFIG_WILDCARDCHAR) {
								wildcard = (::_strnicmp(argv[currDepth - 1], backConf->itemName, index) == 0);
							}
						}

						if (wildcard || ::_stricmp(argv[currDepth - 1], backConf->itemName) == 0) {
							currDepth--;
						}
						else break;
					}
					backConf = backConf->linkPrev;
				}
				if (currDepth == 0) {
					foundConf = conf;
					if (!wildcard) break;
				}
			}
		}
		conf = conf->linkNext;
	}

	Mem_Free(tempstring);

//#ifdef _DEBUG_2
//	if (foundConf) {
//		configGlobs.debugLastFind = foundConf;
//		foundConf->accessed = true;
//	}
//	else {
//		foundConf = nullptr;
//	}
//#endif // _DEBUG_2

	return foundConf;
}

// <LegoRR.exe @00479750>
void __cdecl Gods98::Config_AddList(void)
{
	// NOTE: This function is no longer called, configListSet.Add already handles this.
	configListSet.AddList();
}




uint32 Gods98::Config_CountItems(const Config* arrayItem)
{
	uint32 count = 0;
	while (arrayItem != nullptr) {
		count++;
		arrayItem = Config_GetNextItem(arrayItem);
	}
	return count;
}

void Gods98::Config_AppendConfig(Config* root, Config* config)
{
	Error_Fatal((root->depth != 0), "Cannot append to a config that does not start at depth 0.");
	Error_Fatal((config->depth != 0), "Cannot append a new config that does not start at depth 0.");
	Error_Fatal((config->linkPrev != nullptr), "Appended config is not the root.");

	Config* next = root;
	do {
		root = next;
		next = const_cast<Config*>(root->linkNext);
	} while (next != nullptr);

	root->linkNext = config;
	config->linkPrev = root;
}

#pragma endregion
