// CLGen.h : 
//

#pragma once

#include "../platform/windows.h"

#include "../common.h"


namespace CLGen
{; // !<---

/**********************************************************************************
 ******** Forward Declarations
 **********************************************************************************/

#pragma region Forward Declarations

#pragma endregion

/**********************************************************************************
 ******** Function Typedefs
 **********************************************************************************/

#pragma region Function Typedefs

#pragma endregion

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define CLGEN_FILENAME					"CLGen.dat"


#define CLGEN_CMD_TITLE					"TITLE"
#define CLGEN_USAGE_TITLE				"Usage " CLGEN_CMD_TITLE "|<Window title text>"

#define CLGEN_CMD_INSTRUCTION			"INSTRUCTION"
#define CLGEN_USAGE_INSTRUCTION			"Usage " CLGEN_CMD_INSTRUCTION "|<Instruction message text>"
#define CLGEN_CMD_ADDITEM				"ADDITEM"
#define CLGEN_USAGE_ADDITEM				"Usage " CLGEN_CMD_ADDITEM "|<Mode Name>|<Options>"
#define CLGEN_CMD_ACTION				"ACTION"
#define CLGEN_ACTION_WRITEKEY			"WRITEKEY"
#define CLGEN_USAGE_ACTION_WRITEKEY		"Usage " CLGEN_CMD_ACTION  "|" CLGEN_ACTION_WRITEKEY  "|<registry key>"
#define CLGEN_MSG_UNKNOWN_ACTION		"Unknown action"

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct CLGen_Preset
{
	const char* displayName;
	const char* options;
};
assert_sizeof(CLGen_Preset, 0x8);


struct CLGen_Globs
{
	/*0,4*/		char* buffer;
	/*4,4*/		uint32 fileSize;
	/*8,4*/		const char* title;
	/*c,4*/		const char* instruction;
	/*10,4*/	const char* writeKey;
	/*14,4*/	CLGen_Preset* presetList;
	/*18,4*/	uint32 count;
	/*1c,4*/	uint32 capacity;
	/*20,4*/	sint32 selIndex;
};
assert_sizeof(CLGen_Globs, 0x24);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <CLGen.exe @0040aa30>
extern CLGen_Globs clgenGlobs;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

uint32 CLGen_GetPresetCount();

const CLGen_Preset* CLGen_GetPreset(uint32 index);



// <CLGen.exe @00401000>
sint32 __stdcall CLGen_WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
							   _In_ LPSTR     lpCmdLine, _In_     sint32    nCmdShow);

// Open "CLGen.dat" file and parse commands
// <CLGen.exe @004010b0>
bool32 __cdecl CLGen_Open(const char* filename);

void CLGen_Close();

// Parse a single "CLGen.dat" line command
// <CLGen.exe @00401220>
const char* __cdecl CLGen_ParseCommand(sint32 argc, char** argv);

// Apply changes to Registry key using the passed mode's options
// <CLGen.exe @00401320>
void __cdecl CLGen_ApplyRegistryChanges(const CLGen_Preset* preset);

// Add a CLGen mode name and associated options to table
// <CLGen.exe @00401380>
CLGen_Preset* __cdecl CLGen_AddPresetItem(const char* presetName, const char* options);

// <CLGen.exe @004014b0>
BOOL __stdcall CLGen_DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// <CLGen.exe @00401520>
bool32 __cdecl CLGen_Dialog_HandleInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam);

// <CLGen.exe @004015c0>
bool32 __cdecl CLGen_Dialog_HandleCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);

// <CLGen.exe @00401600>
bool32 __cdecl CLGen_Dialog_HandleMenuCommand(HWND hDlg, WPARAM wParam);

// <CLGen.exe @00401630>
bool32 __cdecl CLGen_Dialog_HandleAcceleratorCommand(HWND hDlg, WORD sourceId, HWND hCtrl);

#pragma endregion

}
