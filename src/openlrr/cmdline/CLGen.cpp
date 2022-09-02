// CLGen.cpp : 
//

#include "../platform/windows.h"
#include "../../../resources/resource.h"

#include "../engine/core/Utils.h"
#include "../engine/util/Registry.h"

#include "CLGen.h"


/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define CLGEN_CAPACITYINCREASE	10

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <CLGen.exe @0040aa30>
CLGen::CLGen_Globs CLGen::clgenGlobs = { 0 };

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

uint32 CLGen::CLGen_GetPresetCount()
{
	return clgenGlobs.count;
}

const CLGen::CLGen_Preset* CLGen::CLGen_GetPreset(uint32 index)
{
	if (index < clgenGlobs.count) {
		return &clgenGlobs.presetList[index];
	}
	return nullptr;
}


// <CLGen.exe @00401000>
sint32 __stdcall CLGen::CLGen_WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
									  _In_ LPSTR     lpCmdLine, _In_     sint32 nCmdShow)
{
	if (CLGen_Open(CLGEN_FILENAME)) {
		clgenGlobs.selIndex = static_cast<sint32>(clgenGlobs.count);
		
		INT_PTR result = ::DialogBoxParamA(hInstance, MAKEINTRESOURCEA(IDD_CLGEN_PRESETDIALOG), nullptr, CLGen_DialogProc, 0);
		if (result == IDOK) {
			if (clgenGlobs.selIndex != clgenGlobs.count && clgenGlobs.selIndex != -1) {

				CLGen_ApplyRegistryChanges(&clgenGlobs.presetList[clgenGlobs.selIndex]);
				return 0;
			}
		}
	}
	else {
		char errorMessage[1024];
		std::sprintf(errorMessage, "Cannot find data file \"%s\"", CLGEN_FILENAME);
		::MessageBoxA(nullptr, errorMessage, "Error", MB_OK);
	}

	return 0;
}

// <CLGen.exe @004010b0>
bool32 __cdecl CLGen::CLGen_Open(const char* filename)
{
	CLGen_Close();

	FILE* file = std::fopen(filename, "rb");
	if (file != nullptr) {
		// Determine file size, then read all data into buffer.
		std::fseek(file, 0, SEEK_END);
		clgenGlobs.fileSize = std::ftell(file);
		std::fseek(file, 0, SEEK_SET);

		clgenGlobs.buffer = (char*)std::malloc(clgenGlobs.fileSize + 1); // +1 for null-terminator
		if (clgenGlobs.buffer == nullptr) {
			std::fclose(file);
			CLGen_Close();
			return false;
		}
		std::memset(clgenGlobs.buffer, 0, clgenGlobs.fileSize + 1);

		size_t readCount = std::fread(clgenGlobs.buffer, 1, clgenGlobs.fileSize, file);
		std::fclose(file);
		if (readCount != clgenGlobs.fileSize) {
			CLGen_Close();
			return false;
		}

		// Tokenise file by lines.
		for (uint32 i = 0; i < clgenGlobs.fileSize; i++) {
			if (clgenGlobs.buffer[i] == '\r' || clgenGlobs.buffer[i] == '\n') {
				clgenGlobs.buffer[i] = '\0';
			}
		}

		// Read commands by line.
		bool isEOL = true; // EOL signifies we can start reading the next command (start true to ignore empty first line).
		char* lineStart = clgenGlobs.buffer;
		uint32 lineNum = 0;

		for (uint32 i = 0; i < clgenGlobs.fileSize; i++) {
			const char c = clgenGlobs.buffer[i];

			if (isEOL) {
				if (c != '\0') {
					// Set start of next line.
					isEOL = false;
					lineStart = clgenGlobs.buffer + i;
				}
			}
			else if (c == '\0') {
				// End of current line found, now parse the command.
				char* argv[128];
				uint32 argc = Gods98::Util_Tokenise(lineStart, argv, "|");
				const char* message = CLGen_ParseCommand(argc, argv);

				if (message != nullptr) {
					char errorMessage[1024];
					std::sprintf(errorMessage, "Error in \"%s\":\n%s", filename, message);
					::MessageBoxA(nullptr, errorMessage, "Error", MB_OK);

					CLGen_Close();
					return false;
				}
				isEOL = true;
			}
		}

		return true;
	}
	return false;
}

void CLGen::CLGen_Close()
{
	if (clgenGlobs.buffer) std::free(clgenGlobs.buffer);
	if (clgenGlobs.presetList) std::free(clgenGlobs.presetList);
	std::memset(&clgenGlobs, 0, sizeof(clgenGlobs));
}

// <CLGen.exe @00401220>
const char* __cdecl CLGen::CLGen_ParseCommand(sint32 argc, char** argv)
{
	if (::_stricmp(argv[0], "TITLE") == 0) {
		if (argc == 2) {
			clgenGlobs.title = argv[1];
			return nullptr;
		}
		return "Usage TITLE|<Window title text>";
	}
	else if (::_stricmp(argv[0], "INSTRUCTION") == 0) {
		if (argc == 2) {
			clgenGlobs.instruction = argv[1];
			return nullptr;
		}
		return "Usage INSTRUCTION|<Instruction message text>";
	}
	else if (::_stricmp(argv[0], "ADDITEM") == 0) {
		if (argc == 3) {
			if (CLGen_AddPresetItem(argv[1], argv[2])) {
				return nullptr;
			}
			return "Failed to allocate space for preset list";
		}
		return "Usage ADDITEM|<Mode Name>|<Options>";
	}
	else if (::_stricmp(argv[0], "ACTION") == 0) {
		if (::_stricmp(argv[1], "WRITEKEY") == 0) {
			if (argc == 3) {
				clgenGlobs.writeKey = argv[2];
				return nullptr;
			}
			return "Usage ACTION|WRITEKEY|<registry key>";
		}
		return "Unknown action";
	}
	else {
		return argv[0];
	}
}

// <CLGen.exe @00401320>
CLGen::CLGen_Preset* __cdecl CLGen::CLGen_AddPresetItem(const char* presetName, const char* options)
{
	// Exand list if needed.
	if (clgenGlobs.count == clgenGlobs.capacity) {
		// Allocate more space for table
		clgenGlobs.capacity += CLGEN_CAPACITYINCREASE; // 10;
		// `realloc` behaves as `malloc` for the first call, since clgenGlobs.presetList is NULL
		CLGen_Preset* newPresetList = (CLGen_Preset*)std::realloc(clgenGlobs.presetList, clgenGlobs.capacity * sizeof(CLGen_Preset));
		if (newPresetList == nullptr) {
			return nullptr;
		}
		clgenGlobs.presetList = newPresetList;

		// Zero out all unused items.
		for (uint32 i = clgenGlobs.count; i < clgenGlobs.capacity; i++) {
			std::memset(&clgenGlobs.presetList[i], 0, sizeof(CLGen_Preset));
		}
	}

	CLGen_Preset* preset = &clgenGlobs.presetList[clgenGlobs.count++];
	preset->displayName = presetName;
	preset->options = options;
	return preset;
}

// <CLGen.exe @00401380>
void __cdecl CLGen::CLGen_ApplyRegistryChanges(const CLGen_Preset* preset)
{

	if (clgenGlobs.writeKey != nullptr) {
		// Split "HKEY_LOCAL_MACHINE", subkey, and value name
		char* valueName = nullptr;
		char* regSubKey = nullptr; // dummy init

		char buff[1024];
		std::strcpy(buff, clgenGlobs.writeKey);

		for (uint32 i = 0; buff[i] != '\0'; i++) {
			if (buff[i] == '\\') {
				if (valueName == nullptr) {
					buff[i] = '\0';
					regSubKey = &buff[i] + 1;
				}
				valueName = &buff[i] + 1;
			}
			//buff[0] = buff[i + 1]; // why...??? oh... Ghidra just being really dump :P
		}

		if (valueName != nullptr) {
			valueName[-1] = '\0'; // split subkey from value name

			if (::_stricmp(buff, "HKEY_LOCAL_MACHINE") == 0) {
				Gods98::Registry_SetValue(regSubKey, valueName, Gods98::RegistryValue::String, preset->options, std::strlen(preset->options));
			}
			else {
				::MessageBoxA(nullptr, "Can only write to HKEY_LOCAL_MACHINE", "Error", MB_OK);
			}
		}
	}
}


// WINDOWS MESSAGES:
//
//  WM_CLOSE        - close the application without applying changes
//  WM_INITDIALOG   - set the label text and populate the combobox
//  WM_COMMAND      - update currently selected mode and handle OK/Cancel buttons
//
// <CLGen.exe @004014b0>
BOOL __stdcall CLGen::CLGen_DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CLOSE:
        ::EndDialog(hDlg, IDCANCEL);
        return true;

    case WM_INITDIALOG:
        return CLGen_Dialog_HandleInitDialog(hDlg, wParam, lParam);

    case WM_COMMAND:
        return CLGen_Dialog_HandleCommand(hDlg, wParam, lParam);

    default:
        return false;
    }
}

// <CLGen.exe @00401520>
bool32 __cdecl CLGen::CLGen_Dialog_HandleInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    HWND hCtrlCombobox = ::GetDlgItem(hDlg, IDC_CLGEN_SELECT);
    HWND hCtrlLabel = ::GetDlgItem(hDlg, IDC_CLGEN_SELECTTEXT);

	::SendMessageA(hDlg, WM_SETTEXT, 0, (LPARAM)clgenGlobs.title);
	::SendMessageA(hCtrlLabel, WM_SETTEXT, 0, (LPARAM)clgenGlobs.instruction);

    // remove all items from the combobox
    while (::SendMessageA(hCtrlCombobox, CB_DELETESTRING, 0, 0) != -1);

    // populate the combobox with CLGen modes
    for (uint32 i = 0; i < clgenGlobs.count; i++) {
		::SendMessageA(hCtrlCombobox, CB_ADDSTRING, 0, (LPARAM)clgenGlobs.presetList[i].displayName);
    }

	::SendMessageA(hCtrlCombobox, CB_SETCURSEL, 0, 0);
	return false;
}

// <CLGen.exe @004015c0>
bool32 __cdecl CLGen::CLGen_Dialog_HandleCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    WORD msgSource = HIWORD(wParam);
    WORD idCaller = LOWORD(wParam);

    switch (msgSource) {
    case 0: // message from Menu
        return CLGen_Dialog_HandleMenuCommand(hDlg, wParam);
    case 1: // message from Accelerator
        return CLGen_Dialog_HandleAcceleratorCommand(hDlg, idCaller, (HWND)lParam);
    default:
        return false;
    }
}

// <CLGen.exe @00401600>
bool32 __cdecl CLGen::CLGen_Dialog_HandleMenuCommand(HWND hDlg, WPARAM wParam)
{
    WORD idCaller = LOWORD(wParam);
    if (idCaller == IDOK || idCaller == IDCANCEL) {
        ::EndDialog(hDlg, (INT_PTR)idCaller);
        return true;
    }
    return false;
}

// <CLGen.exe @00401630>
bool32 __cdecl CLGen::CLGen_Dialog_HandleAcceleratorCommand(HWND hDlg, WORD idCaller, HWND hCtrl)
{
    if (idCaller == IDC_CLGEN_SELECT) {
        clgenGlobs.selIndex = ::SendMessageA(hCtrl, CB_GETCURSEL, 0, 0);
        return true;
    }
    return false;
}

#pragma endregion
