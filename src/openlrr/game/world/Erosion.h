// Erosion.h : 
//

#pragma once

#include "../GameCommon.h"


namespace LegoRR
{; // !<---

/**********************************************************************************
 ******** Forward Declarations
 **********************************************************************************/

#pragma region Forward Declarations

#pragma endregion

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define EROSION_MAXLAVABLOCKS				2000
#define EROSION_MAXERODEBLOCKS				2000
#define EROSION_MAXLOCKEDBLOCKS				1000

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

struct Erosion_Globs // [LegoRR/Erode.c|struct:0xfa14|tags:GLOBS]
{
	/*0000,3e80*/	Point2I lavaBlockList[EROSION_MAXLAVABLOCKS];
	/*3e80,4*/	uint32 lavaBlockCount;
	/*3e84,3e80*/	Point2I activeBlocks[EROSION_MAXERODEBLOCKS];  // (orig: erodeBlock) First-unused table
	/*7d04,1f40*/	bool32 activeUsed[EROSION_MAXERODEBLOCKS];     // (orig: erodeUsed)
	/*9c44,1f40*/	real32 activeTimers[EROSION_MAXERODEBLOCKS];   // (orig: erodeDone) Countdown timers to advance erosion level.
	/*bb84,1f40*/	Point2I lockedBlocks[EROSION_MAXLOCKEDBLOCKS]; // (orig: erodeLocked) First-unused table
	/*dac4,fa0*/	real32 lockedTimers[EROSION_MAXLOCKEDBLOCKS];  // (orig: erodeLockedTime) Countdown timers to unlock.
	/*ea64,fa0*/	bool32 lockedUsed[EROSION_MAXLOCKEDBLOCKS];    // (orig: erodeLockedUsed)
	/*fa04,4*/	real32 updateTime; // Count-up timer (to triggerTime) to start a new actively-eroding block.
	/*fa08,4*/	real32 triggerTime; // (cfg: Level::ErodeTriggerTime, mult: seconds to standard units)
	/*fa0c,4*/	real32 erodeTime; // (cfg: Level::ErodeErodeTime, mult: seconds to standard units * 0.2)
	/*fa10,4*/	real32 lockTime; // (cfg: Level::ErodeLockTime, mult: seconds to standard units)
	/*fa14*/
};
assert_sizeof(Erosion_Globs, 0xfa14);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004c8eb0>
extern Erosion_Globs & erosionGlobs;

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

// <LegoRR.exe @0040e860>
//#define Erosion_Initialise ((void (__cdecl* )(real32 triggerTime, real32 erodeTime, real32 lockTime))0x0040e860)
void __cdecl Erosion_Initialise(real32 triggerTime, real32 erodeTime, real32 lockTime);

// <LegoRR.exe @0040e8c0>
//#define Erosion_GetFreeActiveIndex ((bool32 (__cdecl* )(OUT uint32* index))0x0040e8c0)
bool32 __cdecl Erosion_GetFreeActiveIndex(OUT uint32* index);

// <LegoRR.exe @0040e8f0>
//#define Erosion_GetBlockErodeRate ((real32 (__cdecl* )(const Point2I* blockPos))0x0040e8f0)
real32 __cdecl Erosion_GetBlockErodeRate(const Point2I* blockPos);

// <LegoRR.exe @0040e940>
//#define Erosion_AddActiveBlock ((void (__cdecl* )(const Point2I* blockPos, sint32 erodeStage))0x0040e940)
void __cdecl Erosion_AddActiveBlock(const Point2I* blockPos, sint32 erodeStage);

// <LegoRR.exe @0040e9e0>
//#define Erosion_Update ((void (__cdecl* )(real32 elapsedWorld))0x0040e9e0)
void __cdecl Erosion_Update(real32 elapsedWorld);

// <LegoRR.exe @0040ed30>
//#define Erosion_AddLockedBlock ((void (__cdecl* )(const Point2I* blockPos))0x0040ed30)
void __cdecl Erosion_AddLockedBlock(const Point2I* blockPos);

// <LegoRR.exe @0040ed80>
//#define Erosion_AddOrRemoveLavaBlock ((void (__cdecl* )(const Point2I* blockPos, bool32 add))0x0040ed80)
void __cdecl Erosion_AddOrRemoveLavaBlock(const Point2I* blockPos, bool32 add);

// <LegoRR.exe @0040eee0>
//#define Erosion_IsBlockLocked ((bool32 (__cdecl* )(const Point2I* blockPos))0x0040eee0)
bool32 __cdecl Erosion_IsBlockLocked(const Point2I* blockPos);

// <LegoRR.exe @0040ef30>
//#define Erosion_FindAdjacentBlockPos ((bool32 (__cdecl* )(const Point2I* blockPos, OUT Point2I* adjacentblockPos))0x0040ef30)
bool32 __cdecl Erosion_FindAdjacentBlockPos(const Point2I* blockPos, OUT Point2I* adjacentblockPos);

#pragma endregion

}
