// Water.h : 
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

#define WATER_MAXPOOLBLOCKS			100
#define WATER_MAXPOOLDRAINS			10				// This number is not related to WATER_MAXPOOLS.
#define WATER_MAXPOOLS				10
#define WATER_MAXMERGEPOOLS			WATER_MAXPOOLS	// This number might be related to WATER_MAXPOOLS.
#define WATER_COLOURRGBA			(ColourRGBAF { 0.0f, 0.3f, 0.8f, 0.6f }) // rgba(  0, 76,204,153) #004CCC99

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

enum WaterFlags : uint32 // [LegoRR/Water.c|flags:0x4|type:uint]
{
	WATER_FLAG_NONE    = 0,
	WATER_FLAG_VISIBLE = 0x1,
	WATER_FLAG_FULL    = 0x2, // Water is at the highest level, and isn't rising or lowering.
	WATER_FLAG_SETTLED = 0x4, // Water has reached the lowest level isn't rising or lowering.
};
flags_end(WaterFlags, 0x4);

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct Water_PoolDrain // [LegoRR/Water.c|struct:0x18]
{
	/*00,4*/	uint32 blockIndex; // The index of the block in the Water_Pool that this drain is connected to.
	/*04,4*/	Direction direction; // The direction water drains in.
	/*08,4*/	real32 drainWaterLevel;
	/*0c,4*/	real32 elapsedUp_c; // elapsed count-up timer
	/*10,4*/	real32 elapsedDown_10; // elapsed count-down timer
	/*14,4*/	bool32 active; // Actively draining pool down to drainWaterLevel.
	/*18*/
};
assert_sizeof(Water_PoolDrain, 0x18);


/// ALT NAME: Water_Reservoir (but that's a pain to spell).
struct Water_Pool // [LegoRR/Water.c|struct:0x428]
{
	/*000,320*/	Point2F blocks[WATER_MAXPOOLBLOCKS]; // Blocks are coordinates on the map that are part of this pool.
	/*320,4*/	uint32 blockCount;
	/*324,f0*/	Water_PoolDrain drainList[WATER_MAXPOOLDRAINS]; // Drains are sources where water can flow away into.
	/*414,4*/	uint32 drainCount;
	/*418,4*/	real32 highWaterLevel; // (init: -10000.0, or vertPos if PREDUG)
	/*41c,4*/	real32 currWaterLevel;
	/*420,4*/	Gods98::Container* contMeshTrans; // Water plane mesh.
	/*424,4*/	WaterFlags flags;
	/*428*/
};
assert_sizeof(Water_Pool, 0x428);


struct Water_PoolMergePair // [LegoRR/Water.c|struct:0x8] This struct seems to be used solely for qsort ordering by pointsCount(?)
{
	/*0,4*/	Water_Pool* mainPool; // Pool to take on the extra blocks of otherPool.
	/*4,4*/	Water_Pool* removedPool; // Pool to be merged into mainPool and then erased.
	/*8*/
};
assert_sizeof(Water_PoolMergePair, 0x8);


struct Water_Globs // [LegoRR/Water.c|struct:0x29ec|tags:GLOBS] Module globals for the unfinished "Water flooding" feature.
{
	/*0000,2990*/	Water_Pool poolList[WATER_MAXPOOLS]; // Table of isolated pools (reservoirs) of water.
	/*2990,4*/	uint32 poolCount;
	/*2994,50*/	Water_PoolMergePair mergeList[WATER_MAXMERGEPOOLS]; // Pools that need to be merged at the end of Initialise. (This can be retired by using a better flood-fill algorithm).
	/*29e4,4*/	uint32 mergeCount;
	/*29e8,4*/	real32 digDepth; // (assigned, but never used)
	/*29ec*/
};
assert_sizeof(Water_Globs, 0x29ec);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @0054a520>
extern Water_Globs & waterGlobs;

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

/// CUSTOM: Use the East West algorithm to flood fill without needing to rely on mergeList.
void Water_AddPoolFloodFill(Lego_Level* level, uint32 bxStart, uint32 byStart);

// <LegoRR.exe @0046de50>
//#define Water_Initialise ((void (__cdecl* )(Gods98::Container* contRoot, Lego_Level* level))0x0046de50)
void __cdecl Water_Initialise(Gods98::Container* contRoot, Lego_Level* level);

/// CUSTOM: Force water to cleanup at the end of the level.
void Water_RemoveAll();

// <LegoRR.exe @0046dfd0>
//#define Water_InitPoolDrains ((void (__cdecl* )(Gods98::Container* contRoot, Lego_Level* level))0x0046dfd0)
void __cdecl Water_InitPoolDrains(Gods98::Container* contRoot, Lego_Level* level);

// <LegoRR.exe @0046e140>
//#define Water_InitVertices ((void (__cdecl* )(Gods98::Container* contRoot, Lego_Level* level))0x0046e140)
void __cdecl Water_InitVertices(Gods98::Container* contRoot, Lego_Level* level);

// <LegoRR.exe @0046e480>
//#define Water_ChangeViewMode_removed ((void (__cdecl* )(bool32 isFPViewMode))0x0046e480)
void __cdecl Water_ChangeViewMode_removed(bool32 isFPViewMode);

// <LegoRR.exe @0046e4e0>
//#define Water_DestroyWallComplete ((void (__cdecl* )(Lego_Level* level, uint32 bx, uint32 by))0x0046e4e0)
void __cdecl Water_DestroyWallComplete(Lego_Level* level, uint32 bx, uint32 by);

// Dams up a drain block, causing a pool to start filling (as long as no other drains are undammed).
// <LegoRR.exe @0046e5f0>
//#define Water_DamBlock ((void (__cdecl* )(uint32 bx, uint32 by))0x0046e5f0)
void __cdecl Water_DamBlock(uint32 bx, uint32 by);

// <LegoRR.exe @0046e650>
//#define Water_Update ((void (__cdecl* )(Lego_Level* level, real32 elapsedGame))0x0046e650)
void __cdecl Water_Update(Lego_Level* level, real32 elapsedGame);

// <LegoRR.exe @0046e8d0>
//#define Water_UpdateNotHotBlocks ((void (__cdecl* )(Lego_Level* level))0x0046e8d0)
void __cdecl Water_UpdateNotHotBlocks(Lego_Level* level);

// <LegoRR.exe @0046eb60>
//#define Water_FindPoolDrain ((Water_Pool* (__cdecl* )(uint32 bx, uint32 by, OUT uint32* drainIndex))0x0046eb60)
Water_Pool* __cdecl Water_FindPoolDrain(uint32 bx, uint32 by, OPTIONAL OUT uint32* drainIndex);

// DATA: const Water_Pool*
// <LegoRR.exe @0046ec60>
//#define Water_QsortComparePools ((sint32 (__cdecl* )(const void* a, const void* b))0x0046ec60)
sint32 __cdecl Water_QsortComparePools(const void* a, const void* b);

// <LegoRR.exe @0046ec90>
//#define Water_FindPoolAndMergeRows ((Water_Pool* (__cdecl* )(uint32 by, uint32 bxRowStart, uint32 bxRowEnd))0x0046ec90)
Water_Pool* __cdecl Water_FindPoolAndMergeRows(uint32 by, uint32 bxRowStart, uint32 bxRowEnd);

// <LegoRR.exe @0046ed90>
//#define Water_AddPoolRowBlocks ((void (__cdecl* )(Water_Pool* pool, uint32 by, uint32 bxRowStart, uint32 bxRowEnd))0x0046ed90)
void __cdecl Water_AddPoolRowBlocks(Water_Pool* pool, uint32 by, uint32 bxRowStart, uint32 bxRowEnd);

// <LegoRR.exe @0046edf0>
//#define Water_AddPool ((void (__cdecl* )(uint32 by, uint32 bxRowStart, uint32 bxRowEnd))0x0046edf0)
void __cdecl Water_AddPool(uint32 by, uint32 bxRowStart, uint32 bxRowEnd);

#pragma endregion

}
