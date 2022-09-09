// Objective.h : 
//

#pragma once

#include "../GameCommon.h"


namespace LegoRR
{; // !<---

/**********************************************************************************
 ******** Forward Declarations
 **********************************************************************************/

#pragma region Forward Declarations

struct ObjectiveData;

#pragma endregion

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

enum Objective_GlobFlags : uint32 // [LegoRR/Objective.c|flags:0x4|type:uint] STATUSREADY means next status has been set, but has not been "updated" yet? HITTIMEFAIL is unused and replaced by SHOWACHIEVEDADVISOR.
{
	OBJECTIVE_GLOB_FLAG_NONE                = 0,
	OBJECTIVE_GLOB_FLAG_BRIEFING            = 0x1,
	OBJECTIVE_GLOB_FLAG_COMPLETED           = 0x2,
	OBJECTIVE_GLOB_FLAG_FAILED              = 0x4,
	OBJECTIVE_GLOB_FLAG_STATUSREADY         = 0x8,
	OBJECTIVE_GLOB_FLAG_SHOWBRIEFINGADVISOR = 0x10,
	OBJECTIVE_GLOB_FLAG_SHOWACHIEVEDADVISOR = 0x20,
	OBJECTIVE_GLOB_FLAG_HITTIMEFAIL         = 0x40, // Missing flag that's assumed to be used instead of OBJECTIVE_GLOB_FLAG_SHOWACHIEVEDADVISOR.
	OBJECTIVE_GLOB_FLAG_SHOWFAILEDADVISOR   = 0x80,
	OBJECTIVE_GLOB_FLAG_CRYSTAL             = 0x100,
	OBJECTIVE_GLOB_FLAG_ORE                 = 0x200,
	OBJECTIVE_GLOB_FLAG_BLOCK               = 0x400,
	OBJECTIVE_GLOB_FLAG_TIMER               = 0x800,
	OBJECTIVE_GLOB_FLAG_CONSTRUCTION        = 0x1000,
};
flags_end(Objective_GlobFlags, 0x4);

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct Objective_Globs // [LegoRR/Objective.c|struct:0x28c|tags:GLOBS] Globals for objective messages (Chief briefing, etc).
{
	/*000,4*/	Objective_GlobFlags flags;
	/*004,4*/	Gods98::File* file;
	/*008,80*/	char filename[128];
	/*088,10*/	char* messages[4]; // [Briefing,Completion,Failure,CrystalFailure] Strings containing text of entire status message (pages are separated with '\a').
	/*098,180*/	undefined reserved1[384]; // (possibly unused array of char[3][128])
	/*218,10*/	uint32 currentPages[4]; // (1-indexed) Current page number of the displayed status type.
	/*228,10*/	uint32 currentPageStates[4]; // (1-indexed) State tracking for page to switch to (this is only used to check if the above field needs to trigger an update).
	/*238,10*/	uint32 pageCounts[4]; // Number of pages for the specific status.
	/*248,10*/	Gods98::TextWindow* textWindows[4]; // Text windows for the specific status.
	/*258,c*/	Gods98::TextWindow* pageTextWindows[3];
	/*264,4*/	Gods98::TextWindow* beginTextWindow; // Unknown usage, only worked with when line "[BEGIN]" is found.
	/*268,4*/	undefined4 reserved2;
	/*26c,4*/	bool32 hasBeginText; // True when text has been assigned to beginTextWindow.
	/*270,4*/	bool32 achieved; // True if the level was has ended successfully.
	/*274,4*/	bool32 objectiveSwitch; // (see: NERPFunc__SetObjectiveSwitch)
	/*278,4*/	char* soundName;
	/*27c,4*/	sint32 soundHandle; // (init: -1 when unused)
	/*280,4*/	real32 soundTimer; // (init: (playTime - 1.5f) * 25.0f)
	/*284,4*/	bool32 showing; // True when an objective is currently being shown or changed to(?)
	/*288,4*/	bool32 endTeleportEnabled; // (cfg: ! DisableEndTeleport, default: false (enabled))
	/*28c*/
};

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @00500bc0>
extern Objective_Globs & objectiveGlobs;

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @004577a0>
#define Objective_LoadObjectiveText ((void (__cdecl* )(const Gods98::Config* config, const char* gameName, const char* levelName, Lego_Level* level, const char* filename))0x004577a0)

// <LegoRR.exe @00458000>
#define Objective_LoadLevel ((void (__cdecl* )(const Gods98::Config* config, const char* gameName, const char* levelName, Lego_Level* level, uint32 screenWidth, uint32 screenHeight))0x00458000)

// <LegoRR.exe @00458840>
//#define Objective_SetCryOreObjectives ((void (__cdecl* )(Lego_Level* level, uint32 crystals, uint32 ore))0x00458840)
void __cdecl Objective_SetCryOreObjectives(Lego_Level* level, uint32 crystals, uint32 ore);

// <LegoRR.exe @00458880>
//#define Objective_SetBlockObjective ((void (__cdecl* )(Lego_Level* level, const Point2I* blockPos))0x00458880)
void __cdecl Objective_SetBlockObjective(Lego_Level* level, const Point2I* blockPos);

// <LegoRR.exe @004588b0>
//#define Objective_SetTimerObjective ((void (__cdecl* )(Lego_Level* level, real32 timer, bool32 hitTimeFail))0x004588b0)
void __cdecl Objective_SetTimerObjective(Lego_Level* level, real32 timer, bool32 hitTimeFail);

// <LegoRR.exe @004588e0>
//#define Objective_SetConstructionObjective ((void (__cdecl* )(Lego_Level* level, LegoObject_Type objType, LegoObject_ID objID))0x004588e0)
void __cdecl Objective_SetConstructionObjective(Lego_Level* level, LegoObject_Type objType, LegoObject_ID objID);

// <LegoRR.exe @00458910>
//#define Objective_IsObjectiveAchieved ((bool32 (__cdecl* )(void))0x00458910)
bool32 __cdecl Objective_IsObjectiveAchieved(void);

// <LegoRR.exe @00458920>
//#define Objective_SetEndTeleportEnabled ((void (__cdecl* )(bool32 on))0x00458920)
void __cdecl Objective_SetEndTeleportEnabled(bool32 on);

// <LegoRR.exe @00458930>
//#define Objective_SetStatus ((void (__cdecl* )(LevelStatus status))0x00458930)
void __cdecl Objective_SetStatus(LevelStatus status);

// <LegoRR.exe @00458ba0>
//#define Objective_StopShowing ((void (__cdecl* )(void))0x00458ba0)
void __cdecl Objective_StopShowing(void);

// Returns true if the mission briefing or failure/complete window is showing.
// <LegoRR.exe @00458c60>
//#define Objective_IsShowing ((bool32 (__cdecl* )(void))0x00458c60)
bool32 __cdecl Objective_IsShowing(void);

// <LegoRR.exe @00458c80>
//#define Objective_HandleKeys ((bool32 (__cdecl* )(bool32 nextKeyPressed, bool32 leftButtonReleased, OUT bool32* exitGame))0x00458c80)
bool32 __cdecl Objective_HandleKeys(bool32 nextKeyPressed, bool32 leftButtonReleased, OUT bool32* exitGame);

// <LegoRR.exe @00458ea0>
//#define Objective_Update ((void (__cdecl* )(Gods98::TextWindow* textWnd, Lego_Level* level, real32 elapsedGame, real32 elapsedAbs))0x00458ea0)
void __cdecl Objective_Update(Gods98::TextWindow* textWnd, Lego_Level* level, real32 elapsedGame, real32 elapsedAbs);

// timerStillRunning is set to false when time has run out.
// <LegoRR.exe @00459310>
//#define Objective_CheckCompleted ((bool32 (__cdecl* )(Lego_Level* level, OUT bool32* timerStillRunning, real32 elapsed))0x00459310)
bool32 __cdecl Objective_CheckCompleted(Lego_Level* level, OUT bool32* timerStillRunning, real32 elapsed);

// DATA: ObjectiveData* objective
// <LegoRR.exe @004593c0>
//#define Objective_Callback_CheckCompletedObject ((bool32 (__cdecl* )(LegoObject* liveObj, ObjectiveData* objective))0x004593c0)
bool32 __cdecl Objective_Callback_CheckCompletedObject(LegoObject* liveObj, void* pObjective);

#pragma endregion

}
