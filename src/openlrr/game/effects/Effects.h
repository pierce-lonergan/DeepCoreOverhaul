// Effects.h : 
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

#define EFFECT_ROCKFALL_MAXSTYLES				4
#define EFFECT_ROCKFALL_MAXCONTAINERS			4

#define EFFECT_MISC_MAXCONTAINERS				10

#define EFFECT_EROSION_MAXTYPES					4

#define EFFECT_BOULDEREXPLODE_MAXCONTAINERS		4

#define EFFECT_SMASHPATH_MAXCONTAINERS			4

#define EFFECT_ELECTRICFENCE_MAXTYPES			2
#define EFFECT_ELECTRICFENCE_MAXCONTAINERS		30

#define EFFECT_EXPLOSION_MAXCONTAINERS			4

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

enum MiscEffectType : sint32 // [LegoRR/Effects.c|enum:0x4|type:int] Need a better name for this.
{
	MISCOBJECT_LAZERHIT          = 0,
	MISCOBJECT_PUSHERHIT         = 1,
	MISCOBJECT_FREEZERHIT        = 2,
	MISCOBJECT_PATHDUST          = 3,
	MISCOBJECT_LAVAEROSIONSMOKE1 = 4,
	MISCOBJECT_LAVAEROSIONSMOKE2 = 5,
	MISCOBJECT_LAVAEROSIONSMOKE3 = 6,
	MISCOBJECT_LAVAEROSIONSMOKE4 = 7,
	MISCOBJECT_BIRDSCARER        = 8,
	MISCOBJECT_UPGRADEEFFECT     = 9,
};
assert_sizeof(MiscEffectType, 0x4);


enum RockFallType : uint32 // [LegoRR/Effects.c|enum:0x4|type:uint]
{
	ROCKFALL_3SIDES        = 0,
	ROCKFALL_OUTSIDECORNER = 1,
	ROCKFALL_VTUNNEL       = 2,

	ROCKFALL_TYPE_COUNT    = 3,
};
assert_sizeof(RockFallType, 0x4);

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct EffectElectricFenceBeam // [LegoRR/Effects.c|struct:0xf4]
{
	/*00,4*/	Gods98::Container* cont; // LWS:true
	/*04,78*/	Gods98::Container* contTable[EFFECT_ELECTRICFENCE_MAXCONTAINERS];
	/*7c,78*/	bool32 finishedTable[EFFECT_ELECTRICFENCE_MAXCONTAINERS];
	/*f4*/
};
assert_sizeof(EffectElectricFenceBeam, 0xf4);


struct EffectMisc // [LegoRR/Effects.c|struct:0x38]
{
	/*00,28*/	Gods98::Container* contTable[EFFECT_MISC_MAXCONTAINERS];
	/*28,4*/	uint32 count; // (max of 10)
	/*2c,4*/	real32 birdScarerTimer;
	/*30,4*/	Gods98::Container* cont; // LWS,true | LWO,true
	/*34,4*/	undefined4 field_34;
	/*38*/
};
assert_sizeof(EffectMisc, 0x38);


// See Effect_GetTumbleNull_RockFall for known information on tumbleNull fields.
struct EffectRockFall // [LegoRR/Effects.c|struct:0xa4]
{
	/*00,4*/	Gods98::Container* cont; // LWS:true, or ANIM:true
	/*04,10*/	Gods98::Container* contTable[EFFECT_ROCKFALL_MAXCONTAINERS];
	/*14,10*/	uint32 xBlockPosTable[EFFECT_ROCKFALL_MAXCONTAINERS];
	/*24,10*/	uint32 yBlockPosTable[EFFECT_ROCKFALL_MAXCONTAINERS];
	/*34,10*/	bool32 finishedTable[EFFECT_ROCKFALL_MAXCONTAINERS];
	/*44,4*/	char* tumbleNullName; // (temporary stack buffer, BROKEN)
	/*48,4*/	uint32 tumbleNullFrames;
	/*4c,48*/	undefined field_0x4c_0x93[72];
	/*94,10*/	uint32 tumbleNullUsedTable[EFFECT_ROCKFALL_MAXCONTAINERS]; // Bitfield for each frame.
	/*a4*/
};
assert_sizeof(EffectRockFall, 0xa4);


struct Effect_Globs // [LegoRR/Effects.c|struct:0xc78|tags:GLOBS]
{
	/*000,30*/	RockFallType rockFallCompletedTypes[(uint32)ROCKFALL_TYPE_COUNT * EFFECT_ROCKFALL_MAXCONTAINERS]; // (size not related to [style:4])
	/*030,30*/	uint32 rockFallCompletedIndexes[(uint32)ROCKFALL_TYPE_COUNT * EFFECT_ROCKFALL_MAXCONTAINERS]; // (size not related to [style:4])
	/*060,7b0*/	EffectRockFall rockFallEffects[EFFECT_ROCKFALL_MAXSTYLES][(uint32)ROCKFALL_TYPE_COUNT]; // [style:4][type:3] LWS:true, or ANIM:true
	/*810,10*/	char* rockFallStyleName[EFFECT_ROCKFALL_MAXSTYLES];
	/*820,4*/	uint32 rockFallStyleCount;
	/*824,4*/	uint32 rockFallStyleIndex;
	/*828,38*/	EffectMisc lazerHitEffect; // LWS:true, or LWO:true
	/*860,38*/	EffectMisc pusherHitEffect; // LWS:true, or LWO:true
	/*898,38*/	EffectMisc freezerHitEffect; // LWS:true, or LWO:true
	/*8d0,38*/	EffectMisc pathDustEffect; // LWS:true, or LWO:true
	/*908,e0*/	EffectMisc lavaErosionSmokeEffects[EFFECT_EROSION_MAXTYPES]; // [1-4] LWS:true, or LWO:true
	/*9e8,38*/	EffectMisc birdScarerEffect; // LWS:true, or LWO:true
	/*a20,38*/	EffectMisc upgradeEffect; // LWS:true, or LWO:true
	/*a58,10*/	Gods98::Container* boulderExplodeContTable[EFFECT_BOULDEREXPLODE_MAXCONTAINERS]; // source: legoGlobs.contBoulderExplode / legoGlobs.contBoulderExplodeIce
	/*a68,10*/	Gods98::Container* smashPathContTable[EFFECT_SMASHPATH_MAXCONTAINERS]; // source: legoGlobs.contSmashPath
	/*a78,1e8*/	EffectElectricFenceBeam efenceEffects[EFFECT_ELECTRICFENCE_MAXTYPES]; // [shortBeam,longBeam]
	/*c60,4*/	Gods98::Container* explosionCont; // LWS:true
	/*c64,10*/	Gods98::Container* explosionContTable[EFFECT_EXPLOSION_MAXCONTAINERS];
	/*c74,4*/	uint32 explosionCount;
	/*c78*/
};
assert_sizeof(Effect_Globs, 0xc78);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004c8180>
extern Effect_Globs & effectGlobs;

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

// <LegoRR.exe @0040bcf0>
//#define Effect_StopAll ((void (__cdecl* )(void))0x0040bcf0)
void __cdecl Effect_StopAll(void);

// This CAN ONLY be used with a non-activity-based object type:
// i.e. anything but Vehicles, MiniFigures, RockMonsters, Vehicles, or UpgradeParts.
// <LegoRR.exe @0040bd10>
//#define Effect_Spawn_BoulderExplode_AtSimpleObject ((void (__cdecl* )(LegoObject* simpleObj))0x0040bd10)
void __cdecl Effect_Spawn_BoulderExplode_AtSimpleObject(LegoObject* simpleObj);

// <LegoRR.exe @0040bd40>
//#define Effect_Spawn_BoulderExplode ((void (__cdecl* )(const Vector3F* wPos))0x0040bd40)
void __cdecl Effect_Spawn_BoulderExplode(const Vector3F* wPos);

// <LegoRR.exe @0040bde0>
//#define Effect_Spawn_SmashPath ((void (__cdecl* )(OPTIONAL LegoObject* liveObj, OPTIONAL const Vector3F* wPos))0x0040bde0)
void __cdecl Effect_Spawn_SmashPath(OPTIONAL LegoObject* liveObj, OPTIONAL const Vector3F* wPos);

// <LegoRR.exe @0040bea0>
//#define Effect_FindRockFallStyle ((bool32 (__cdecl* )(const char* name, OUT uint32* index))0x0040bea0)
bool32 __cdecl Effect_FindRockFallStyle(const char* name, OUT uint32* index);

// <LegoRR.exe @0040bef0>
//#define Effect_SetRockFallStyle ((void (__cdecl* )(uint32 rockFallStyleIndex))0x0040bef0)
void __cdecl Effect_SetRockFallStyle(uint32 rockFallStyleIndex);

// <LegoRR.exe @0040bf00>
//#define Effect_Load_RockFallStylesAll ((void (__cdecl* )(const Gods98::Config* config, const char* gameName, Gods98::Container* contRoot))0x0040bf00)
void __cdecl Effect_Load_RockFallStylesAll(const Gods98::Config* config, const char* gameName, Gods98::Container* contRoot);

// <LegoRR.exe @0040c000>
//#define Effect_Load_RockFallStyle ((bool32 (__cdecl* )(Gods98::Container* contRoot, const char* filename, uint32 rockFallStyle, RockFallType rockFallType, const char* tumbleNullName))0x0040c000)
bool32 __cdecl Effect_Load_RockFallStyle(Gods98::Container* contRoot, const char* filename, uint32 rockFallStyle, RockFallType rockFallType, const char* tumbleNullName);

// <LegoRR.exe @0040c0e0>
//#define Effect_Load_ElectricFenceBeam ((bool32 (__cdecl* )(Gods98::Container* contRoot, const char* filename, bool32 longBeam))0x0040c0e0)
bool32 __cdecl Effect_Load_ElectricFenceBeam(Gods98::Container* contRoot, const char* filename, bool32 longBeam);

// <LegoRR.exe @0040c160>
//#define Effect_Spawn_RockFall ((bool32 (__cdecl* )(RockFallType rockFallType, sint32 bx, sint32 by, real32 xPos, real32 yPos, real32 zPos, real32 xDir, real32 yDir))0x0040c160)
bool32 __cdecl Effect_Spawn_RockFall(RockFallType rockFallType, sint32 bx, sint32 by, real32 xPos, real32 yPos, real32 zPos, real32 xDir, real32 yDir);

// <LegoRR.exe @0040c220>
//#define Effect_Spawn_ElectricFenceBeam ((bool32 (__cdecl* )(bool32 longBeam, real32 xPos, real32 yPos, real32 zPos, real32 xDir, real32 yDir, real32 zDir))0x0040c220)
bool32 __cdecl Effect_Spawn_ElectricFenceBeam(bool32 longBeam, real32 xPos, real32 yPos, real32 zPos, real32 xDir, real32 yDir, real32 zDir);

// <LegoRR.exe @0040c2d0>
//#define Effect_UpdateAll ((uint32 (__cdecl* )(real32 elapsedWorld, OUT RockFallType** rockFallTypes, OUT uint32** rockFallIndexes))0x0040c2d0)
uint32 __cdecl Effect_UpdateAll(real32 elapsedWorld, OUT RockFallType** rockFallTypes, OUT uint32** rockFallIndexes);

// <LegoRR.exe @0040c400>
//#define Effect_Update_BoulderExplode ((void (__cdecl* )(real32 elapsedWorld))0x0040c400)
void __cdecl Effect_Update_BoulderExplode(real32 elapsedWorld);

// <LegoRR.exe @0040c450>
//#define Effect_Update_SmashPath ((void (__cdecl* )(real32 elapsedWorld))0x0040c450)
void __cdecl Effect_Update_SmashPath(real32 elapsedWorld);

// <LegoRR.exe @0040c4a0>
//#define Effect_GetBlockPos_RockFall ((void (__cdecl* )(RockFallType rockFallType, uint32 index, OUT sint32* bx, OUT sint32* by))0x0040c4a0)
void __cdecl Effect_GetBlockPos_RockFall(RockFallType rockFallType, uint32 index, OUT sint32* bx, OUT sint32* by);

// So this is a very weird behaviour for rockfall effects.
// Each rockfall animation defines Item_Null (tumbleNullName) objects,
//  and LegoObject_UpdateWorldStickyPosition will try to get a camera null if the object being updated is a simple object and not a barrier.
// Each time a camera null is obtained while the effect is in-play, that returned camera null will be marked as used,
//  and then the next camera null frame will be obtained until none are left.
// 
// <LegoRR.exe @0040c4e0>
//#define Effect_GetTumbleNull_RockFall ((Gods98::Container* (__cdecl* )(uint32 bx, uint32 by))0x0040c4e0)
Gods98::Container* __cdecl Effect_GetTumbleNull_RockFall(uint32 bx, uint32 by);

// <LegoRR.exe @0040c5c0>
//#define Effect_RemoveAll_BoulderExplode ((void (__cdecl* )(void))0x0040c5c0)
void __cdecl Effect_RemoveAll_BoulderExplode(void);

// <LegoRR.exe @0040c5f0>
//#define Effect_RemoveAll_RockFall ((void (__cdecl* )(void))0x0040c5f0)
void __cdecl Effect_RemoveAll_RockFall(void);

// <LegoRR.exe @0040c650>
//#define Effect_Load_Explosion ((void (__cdecl* )(Gods98::Container* contRoot, const char* filename))0x0040c650)
void __cdecl Effect_Load_Explosion(Gods98::Container* contRoot, const char* filename);

// <LegoRR.exe @0040c680>
//#define Effect_Spawn_Explosion ((void (__cdecl* )(OPTIONAL LegoObject* liveObj, OPTIONAL const Point2F* wPos))0x0040c680)
void __cdecl Effect_Spawn_Explosion(OPTIONAL LegoObject* liveObj, OPTIONAL const Point2F* wPos);

// <LegoRR.exe @0040c760>
//#define Effect_Update_Explosion ((void (__cdecl* )(real32 elapsedWorld))0x0040c760)
void __cdecl Effect_Update_Explosion(real32 elapsedWorld);

// <LegoRR.exe @0040c7d0>
//#define Effect_GetMiscEffectData ((EffectMisc* (__cdecl* )(MiscEffectType miscEffectType))0x0040c7d0)
EffectMisc* __cdecl Effect_GetMiscEffectData(MiscEffectType miscEffectType);

// <LegoRR.exe @0040c850>
//#define Effect_Load_Misc ((bool32 (__cdecl* )(EffectMisc* effectMisc, Gods98::Container* contRoot, const char* filename))0x0040c850)
bool32 __cdecl Effect_Load_Misc(EffectMisc* effectMisc, Gods98::Container* contRoot, const char* filename);

// <LegoRR.exe @0040c8c0>
//#define Effect_Initialise ((void (__cdecl* )(const Gods98::Config* config, const char* gameName, Gods98::Container* contRoot))0x0040c8c0)
void __cdecl Effect_Initialise(const Gods98::Config* config, const char* gameName, Gods98::Container* contRoot);

// <LegoRR.exe @0040caa0>
//#define Effect_Update_MiscEffect ((void (__cdecl* )(EffectMisc* miscEffect, real32 elapsedWorld))0x0040caa0)
void __cdecl Effect_Update_MiscEffect(EffectMisc* miscEffect, real32 elapsedWorld);

// <LegoRR.exe @0040cb70>
//#define Effect_Update_MiscEffectsAll ((void (__cdecl* )(real32 elapsedWorld))0x0040cb70)
void __cdecl Effect_Update_MiscEffectsAll(real32 elapsedWorld);

// <LegoRR.exe @0040cc10>
//#define Effect_Spawn_Particle ((bool32 (__cdecl* )(MiscEffectType miscEffectType, const Vector3F* wPos, OPTIONAL const Vector3F* dir))0x0040cc10)
bool32 __cdecl Effect_Spawn_Particle(MiscEffectType miscEffectType, const Vector3F* wPos, OPTIONAL const Vector3F* dir);

#pragma endregion

}
