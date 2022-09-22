// Weapons.h : 
//

#pragma once

#include "../GameCommon.h"
#include "BezierCurve.h"
#include "Upgrade.h"


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

#define WEAPON_MAXWEAPONS			3
#define WEAPON_MAXFIRES				2

#define WEAPON_MAXLAZERS			10
#define WEAPON_MAXPROJECTILES		10

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

struct SearchWeapons18_2 // [LegoRR/search.c|struct:0x18]
{
	/*00,4*/	LegoObject* foundObject;
	/*04,4*/	real32 ref_float_4;
	/*08,4*/	const Vector3F* fromPos;
	/*0c,4*/	const Vector3F* fromToDistance;
	/*10,4*/	bool32 success;
	/*14,4*/	LegoObject* ignoreObject;
	/*18*/
};
assert_sizeof(SearchWeapons18_2, 0x18);


struct WeaponsModel // [LegoRR/Weapons.c|struct:0xa8]
{
	/*00,18*/	Gods98::Container* fireNullPairs[WEAPON_MAXWEAPONS][WEAPON_MAXFIRES];
	/*18,c*/	Gods98::Container* xPivotNulls[WEAPON_MAXWEAPONS];
	/*24,c*/	Gods98::Container* yPivotNulls[WEAPON_MAXWEAPONS];
	/*30,4*/	real32 pivotMaxZ;			// (ae: PivotMaxZ)
	/*34,4*/	real32 pivotMinZ;			// (ae: PivotMinZ)
	/*38,c*/	uint32 fireNullPairFrames[WEAPON_MAXWEAPONS];	// (valid: [0,1])
	/*44,48*/	Vector3F fireDirections[WEAPON_MAXWEAPONS];
	/*68,48*/	Vector3F targetWorldPos[WEAPON_MAXWEAPONS];
	/*8c,c*/	Upgrade_PartInfo* weaponParts[WEAPON_MAXWEAPONS];
	/*98,c*/	real32 timers[WEAPON_MAXWEAPONS];
	/*a4,4*/	uint32 weaponCount;			// (valid: [0,3])
	/*a8*/
};
assert_sizeof(WeaponsModel, 0xa8);


struct WeaponStats // [LegoRR/Weapons.c|struct:0x4b68]
{
	/*0000,4b00*/	real32 objectCoefs[LegoObject_Type_Count][LegoObject_ID_Count][OBJECT_MAXLEVELS]; // (cfg: [object] l:e:v:e:l:s, inits: -1.0f)
	/*4b00,4*/	bool32 isSlowDeath;          // (cfg: SlowDeath exists)
	/*4b04,4*/	real32 slowDeathInitialCoef; // (cfg: SlowDeath[0]) Initial damage multiplier.
	/*4b08,4*/	real32 slowDeathDuration;    // (cfg: SlowDeath[1])
	/*4b0c,4*/	real32 rechargeTime;         // (cfg: RechargeTime, default: 0.0f)
	/*4b10,4*/	real32 damage;               // (cfg: DefaultDamage, default: 0.0f)
	/*4b14,4*/	real32 dischargeRate;        // (cfg: DischargeRate)
	/*4b18,4*/	sint32 ammo;                 // (cfg: Ammo, unused)
	/*4b1c,4*/	real32 weaponRange;          // (cfg: WeaponRange, default: 150.0f)
	/*4b20,48*/	real32 wallDestroyTimes[Lego_SurfaceType_Count]; // (cfg: WallDestroyTime_[surface], defaults: 5.0f)
	/*4b68*/
};
assert_sizeof(WeaponStats, 0x4b68);


struct Weapon_Lazer // [LegoRR/???|struct:0x10]
{
	/*00,4*/	Gods98::Mesh* innerMesh; // Inner bright beam.
	/*04,4*/	Gods98::Mesh* outerMesh; // Outer blue-ish beam glow.
	/*08,4*/	Gods98::Container* cont;
	/*0c,4*/	real32 timer; // Countdown duration in frames.
	/*10*/
};
assert_sizeof(Weapon_Lazer, 0x10);


struct Weapon_Projectile // [LegoRR/Weapons.c|struct:0x2b0] Seen in a table of [10]. This is likely an extension of the Weapons module.
{
	/*000,4*/	bool32 isCurvedPath;
	/*004,25c*/	BezierCurve curve;
	/*260,4*/	real32 totalDistance; // Curved paths only.
	/*264,4*/	real32 currDistance; // Curved paths only.
	/*268,c*/	Vector3F initialPos;
	/*274,c*/	Vector3F currPos;
	/*280,c*/	Vector3F lastPos;
	/*28c,4*/	real32 speed;
	/*290,c*/	Vector3F dir;
	/*29c,4*/	undefined4 unused_29c; // assigned to 0 but unused
	/*2a0,4*/	LegoObject* shooterObject; // field_2a0;
	/*2a4,4*/	LegoObject* projectileObject;
	/*2a8,4*/	sint32 weaponID;
	/*2ac,4*/	Weapon_KnownType knownWeapon;
	/*2b0*/
};
assert_sizeof(Weapon_Projectile, 0x2b0);


struct Weapon_Globs // [LegoRR/Weapons.c|struct:0x1b90|tags:GLOBS]
{
	/*0000,4*/	uint32 weaponCount;
	/*0004,4*/	char** weaponNameList;
	/*0008,4*/	WeaponStats* weaponStatsList;
	/*000c,a0*/	Weapon_Lazer lazerList[WEAPON_MAXLAZERS];
	/*00ac,1ae0*/	Weapon_Projectile projectileList[WEAPON_MAXPROJECTILES];
	/*1b8c,4*/	const Gods98::Config* config;
	/*1b90*/
};
assert_sizeof(Weapon_Globs, 0x1b90);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @00504870>
extern Weapon_Globs & weaponGlobs;

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

// <LegoRR.exe @0046ee40>
//#define Weapon_Initialise ((bool32 (__cdecl* )(const Gods98::Config* config, const char* gameName))0x0046ee40)
bool32 __cdecl Weapon_Initialise(const Gods98::Config* config, const char* gameName);

// On failure, returns weaponGlobs.weaponCount
// Weapon IDs are 1-indexed it seems...(?)
// <LegoRR.exe @0046f390>
//#define Weapon_GetWeaponIDByName ((uint32 (__cdecl* )(const char* weaponName))0x0046f390)
uint32 __cdecl Weapon_GetWeaponIDByName(const char* weaponName);

// <LegoRR.exe @0046f3d0>
//#define Weapon_GetRechargeTime ((real32 (__cdecl* )(uint32 weaponID))0x0046f3d0)
real32 __cdecl Weapon_GetRechargeTime(uint32 weaponID);

// <LegoRR.exe @0046f400>
//#define Weapon_GetDischargeRate ((real32 (__cdecl* )(uint32 weaponID))0x0046f400)
real32 __cdecl Weapon_GetDischargeRate(uint32 weaponID);

// <LegoRR.exe @0046f430>
//#define Weapon_GetWeaponRange ((real32 (__cdecl* )(uint32 weaponID))0x0046f430)
real32 __cdecl Weapon_GetWeaponRange(uint32 weaponID);

// <LegoRR.exe @0046f460>
//#define Weapon_GetWallDestroyTime ((real32 (__cdecl* )(uint32 weaponID, Lego_SurfaceType surfaceType))0x0046f460)
real32 __cdecl Weapon_GetWallDestroyTime(uint32 weaponID, Lego_SurfaceType surfaceType);

// <LegoRR.exe @0046f490>
//#define Weapon_GetDamageForObject ((real32 (__cdecl* )(uint32 weaponID, LegoObject* liveObj))0x0046f490)
real32 __cdecl Weapon_GetDamageForObject(uint32 weaponID, LegoObject* liveObj);

// elapsed is only passed a non-zero value when unkSlowDeathBool is FALSE. But this may be coincidence.
// <LegoRR.exe @0046f530>
//#define Weapon_GenericDamageObject ((void (__cdecl* )(LegoObject* liveObj, uint32 weaponID, bool32 unkSlowDeathBool, real32 elapsed, const Point2F* dir))0x0046f530)
void __cdecl Weapon_GenericDamageObject(LegoObject* liveObj, uint32 weaponID, bool32 hit, real32 elapsed, const Point2F* dir);

// <LegoRR.exe @0046f640>
//#define Weapon_GunDamageObject ((void (__cdecl* )(LegoObject* liveObj, real32 damage, bool32 param_3))0x0046f640)
void __cdecl Weapon_GunDamageObject(LegoObject* liveObj, real32 damage, bool32 reactToHit);

// Probably projectile hitting an object and the projectile object being removed.
// Along with effects of weapon hitting.
// <LegoRR.exe @0046f670>
//#define Weapon_Projectile_FUN_0046f670 ((void (__cdecl* )(Weapon_Projectile* projectile))0x0046f670)
void __cdecl Weapon_Projectile_FUN_0046f670(Weapon_Projectile* projectile);

// Update Projectile and Lazer shots.
// <LegoRR.exe @0046f810>
//#define Weapon_Update ((void (__cdecl* )(real32 elapsedGame))0x0046f810)
void __cdecl Weapon_Update(real32 elapsedGame);

// <LegoRR.exe @0046f8d0>
//#define Weapon_LegoObject_Callback_UpdateObject ((bool32 (__cdecl* )(LegoObject* liveObj, real32* pElapsed))0x0046f8d0)
bool32 __cdecl Weapon_LegoObject_Callback_UpdateObject(LegoObject* liveObj, void* pElapsed);

// effectSpawnPos is required if LegoObject_IsRockMonsterCanGather(liveObj).
// <LegoRR.exe @0046fa30>
//#define Weapon_GunHitObject ((void (__cdecl* )(LegoObject* liveObj, const Vector3F* dirVec2D, OPTIONAL const Vector3F* effectSpawnPos, uint32 weaponID, Weapon_KnownType knownWeapon))0x0046fa30)
void __cdecl Weapon_GunHitObject(LegoObject* liveObj, const Vector3F* dir, OPTIONAL const Vector3F* effectSpawnPos, uint32 weaponID, Weapon_KnownType knownWeapon);

// <LegoRR.exe @0046fbe0>
//#define Weapon_Projectile_UpdatePath ((void (__cdecl* )(Weapon_Projectile* projectile, real32 elapsed))0x0046fbe0)
void __cdecl Weapon_Projectile_UpdatePath(Weapon_Projectile* projectile, real32 elapsed);

// <LegoRR.exe @0046fdb0>
//#define Weapon_Projectile_AddStraightPath ((bool32 (__cdecl* )(LegoObject* liveObj, const Vector3F* fromPos, const Vector3F* fromToDistance, uint32 weaponID, Weapon_KnownType knownWeapon))0x0046fdb0)
bool32 __cdecl Weapon_Projectile_AddStraightPath(LegoObject* liveObj, const Vector3F* fromPos, const Vector3F* fromToDistance, sint32 weaponID, Weapon_KnownType knownWeapon);

// <LegoRR.exe @0046ff30>
//#define Weapon_Projectile_AddCurvedPath ((bool32 (__cdecl* )(LegoObject* liveObj, const Vector3F* fromPos, const Vector3F* toPos, sint32 weaponID, Weapon_KnownType knownWeapon))0x0046ff30)
bool32 __cdecl Weapon_Projectile_AddCurvedPath(LegoObject* liveObj, const Vector3F* fromPos, const Vector3F* toPos, sint32 weaponID, Weapon_KnownType knownWeapon);

// <LegoRR.exe @004701b0>
//#define Weapon_GetObjectTypeAndID_ByKnownWeaponType ((void (__cdecl* )(Weapon_KnownType knownWeapon, OUT LegoObject_Type* objType, OUT LegoObject_ID* objID))0x004701b0)
void __cdecl Weapon_GetObjectTypeAndID_ByKnownWeaponType(Weapon_KnownType knownWeapon, OUT LegoObject_Type* objType, OUT LegoObject_ID* objID);

// <LegoRR.exe @00470230>
//#define Weapon_Projectile_GetNextAvailable ((sint32 (__cdecl* )(void))0x00470230)
sint32 __cdecl Weapon_Projectile_GetNextAvailable(void);

// <LegoRR.exe @00470250>
//#define Weapon_Lazer_GetNextAvailable ((sint32 (__cdecl* )(void))0x00470250)
sint32 __cdecl Weapon_Lazer_GetNextAvailable(void);

// <LegoRR.exe @00470270>
//#define Weapon_FireLazer ((LegoObject* (__cdecl* )(const Vector3F* fromPos, const Vector3F* dir, LegoObject* liveObj, real32 elapsed, real32 weaponRange, sint32 weaponID, real32 coef))0x00470270)
LegoObject* __cdecl Weapon_FireLazer(const Vector3F* fromPos, const Vector3F* dir, LegoObject* liveObj, real32 elapsed, real32 weaponRange, sint32 weaponID, real32 coef);

// <LegoRR.exe @00470520>
//#define Weapon_LegoObject_Collision_FUN_00470520 ((bool32 (__cdecl* )(LegoObject* liveObj, const Point2F* fromPos, const Point2F* faceDir, bool32 doTestCollision))0x00470520)
bool32 __cdecl Weapon_LegoObject_Collision_FUN_00470520(LegoObject* liveObj, const Point2F* fromPos, const Point2F* faceDir, bool32 doTestCollision);

// <LegoRR.exe @00470570>
//#define Weapon_LegoObject_CollisionBox_FUN_00470570 ((bool32 (__cdecl* )(LegoObject* liveObj, const Point2F* fromPos, const Point2F* faceDir, OPTIONAL OUT Point2F* intersection, bool32 doTestCollision))0x00470570)
bool32 __cdecl Weapon_LegoObject_CollisionBox_FUN_00470570(LegoObject* liveObj, const Point2F* fromPos, const Point2F* faceDir, OPTIONAL OUT Point2F* intersection, bool32 doTestCollision);

// <LegoRR.exe @00470800>
//#define Weapon_LegoObject_CollisionRadius_FUN_00470800 ((bool32 (__cdecl* )(LegoObject* liveObj, const Point2F* fromPos, const Point2F* faceDir, bool32 doTestCollision))0x00470800)
bool32 __cdecl Weapon_LegoObject_CollisionRadius_FUN_00470800(LegoObject* liveObj, const Point2F* fromPos, const Point2F* faceDir, bool32 doTestCollision);

// <LegoRR.exe @004708f0>
//#define Weapon_LegoObject_TestCollision_FUN_004708f0 ((bool32 (__cdecl* )(LegoObject* liveObj, const Point2F* fromPos, const Point2F* faceDir))0x004708f0)
bool32 __cdecl Weapon_LegoObject_TestCollision_FUN_004708f0(LegoObject* liveObj, const Point2F* fromPos, const Point2F* faceDir);

// <LegoRR.exe @00470950>
//#define Weapon_Lazer_Add ((void (__cdecl* )(Gods98::Container* cont, const Vector3F* fromPos, const Vector3F* toPos))0x00470950)
void __cdecl Weapon_Lazer_Add(Gods98::Container* cont, const Vector3F* fromPos, const Vector3F* toPos);

// <LegoRR.exe @00470a20>
//#define Weapon_Lazer_InitMesh ((void (__cdecl* )(Gods98::Mesh* mesh, real32 thickness, const Vector3F* fromPos, const Vector3F* toPos, real32 red, real32 green, real32 blue, real32 alpha))0x00470a20)
void __cdecl Weapon_Lazer_InitMesh(Gods98::Mesh* mesh, real32 thickness, const Vector3F* fromPos, const Vector3F* toPos, real32 red, real32 green, real32 blue, real32 alpha);

// <LegoRR.exe @00471580>
//#define Weapon_LegoObject_GetWeaponsModel ((WeaponsModel* (__cdecl* )(LegoObject* liveObj))0x00471580)
WeaponsModel* __cdecl Weapon_LegoObject_GetWeaponsModel(LegoObject* liveObj);

// <LegoRR.exe @004715b0>
//#define Weapon_LegoObject_GetWeaponTimer ((real32 (__cdecl* )(LegoObject* liveObj, uint32 frameNo))0x004715b0)
real32 __cdecl Weapon_LegoObject_GetWeaponTimer(LegoObject* liveObj, uint32 frameNo);

// <LegoRR.exe @004715d0>
//#define Weapon_LegoObject_SetWeaponTimer ((void (__cdecl* )(LegoObject* liveObj, real32 timer, uint32 frameNo))0x004715d0)
void __cdecl Weapon_LegoObject_SetWeaponTimer(LegoObject* liveObj, real32 timer, uint32 frameNo);

// Only Z value of the two vector parameters is used.
// <LegoRR.exe @004715f0>
//#define Weapon_MathUnk_CheckVectorsZScalar_InRange ((bool32 (__cdecl* )(const Vector3F* vectorPos, const Vector3F* vectorScaled, real32 minZ, real32 maxZ, real32 scalar))0x004715f0)
bool32 __cdecl Weapon_MathUnk_CheckVectorsZScalar_InRange(const Vector3F* vectorPos, const Vector3F* vectorScaled, real32 minZ, real32 maxZ, real32 scalar);

// DATA: SearchWeapons18_2*
// <LegoRR.exe @00471630>
//#define Weapon_LegoObject_Callback_FUN_00471630 ((bool32 (__cdecl* )(LegoObject* liveObj, void* search))0x00471630)
bool32 __cdecl Weapon_LegoObject_Callback_FUN_00471630(LegoObject* liveObj, void* search);

// <LegoRR.exe @004718f0>
//#define Weapon_LegoObject_FUN_004718f0 ((void (__cdecl* )(SearchWeapons18_2* search))0x004718f0)
void __cdecl Weapon_LegoObject_FUN_004718f0(SearchWeapons18_2* search);

// <LegoRR.exe @00471b20>
//#define Weapon_DoCallbacksSearch_FUN_00471b20 ((bool32 (__cdecl* )(const Vector3F* vecCector, const Vector3F* vecDistance, OUT LegoObject** liveObj, IN OUT real32* float_4, OPTIONAL LegoObject* ignoreObj))0x00471b20)
bool32 __cdecl Weapon_DoCallbacksSearch_FUN_00471b20(const Vector3F* vecCector, const Vector3F* vecDistance, OUT LegoObject** liveObj, IN OUT real32* float_4, OPTIONAL LegoObject* ignoreObj);

// <LegoRR.exe @00471b90>
//#define Weapon_LegoObject_DoCallbacksSearch_FUN_00471b90 ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* otherObj))0x00471b90)
bool32 __cdecl Weapon_LegoObject_DoCallbacksSearch_FUN_00471b90(LegoObject* liveObj, LegoObject* otherObj);

// <LegoRR.exe @00471c20>
//#define Weapon_LegoObject_SeeThroughWalls_FUN_00471c20 ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* otherObj))0x00471c20)
bool32 __cdecl Weapon_LegoObject_SeeThroughWalls_FUN_00471c20(LegoObject* liveObj, LegoObject* otherObj);

// <LegoRR.exe @00471ce0>
//#define Weapon_GetFireNull ((Gods98::Container* (__cdecl* )(WeaponsModel* weapons, uint32 frameNo, uint32 pairNo))0x00471ce0)
Gods98::Container* __cdecl Weapon_GetFireNull(const WeaponsModel* weapons, uint32 frameNo, uint32 pairNo);

// <LegoRR.exe @00471d00>
//#define Weapon_GetXPivotNull ((Gods98::Container* (__cdecl* )(WeaponsModel* weapons, uint32 frameNo))0x00471d00)
Gods98::Container* __cdecl Weapon_GetXPivotNull(const WeaponsModel* weapons, uint32 frameNo);

// <LegoRR.exe @00471d10>
//#define Weapon_GetYPivotNull ((Gods98::Container* (__cdecl* )(WeaponsModel* weapons, uint32 frameNo))0x00471d10)
Gods98::Container* __cdecl Weapon_GetYPivotNull(const WeaponsModel* weapons, uint32 frameNo);

// targetWorldPos is required if assignTarget is true.
// <LegoRR.exe @00471d20>
//#define Weapon_PivotTracker ((void (__cdecl* )(WeaponsModel* weapons, const Vector3F* targetWorldPos, real32 elapsed, bool32 assignTarget, uint32 frameNo))0x00471d20)
void __cdecl Weapon_PivotTracker(WeaponsModel* weapons, OPTIONAL const Vector3F* targetWorldPos, real32 elapsed, bool32 assignTarget, uint32 frameNo);

// <LegoRR.exe @00471f30>
//#define Weapon_GetFireDirection ((void (__cdecl* )(WeaponsModel* weapons, OUT Vector3F* dir, uint32 frameNo))0x00471f30)
void __cdecl Weapon_GetFireDirection(const WeaponsModel* weapons, OUT Vector3F* dir, uint32 frameNo);

// <LegoRR.exe @00471f60>
//#define Weapon_LegoObject_GetCollCenterPosition ((bool32 (__cdecl* )(LegoObject* liveObj, OUT Vector3F* vector))0x00471f60)
bool32 __cdecl Weapon_LegoObject_GetCollCenterPosition(LegoObject* liveObj, OUT Vector3F* vector);

// Returns true if the object is active, and if the object stats specifies it has a tracker
//  (not if the tracker itself is active).
// <LegoRR.exe @00471fa0>
//#define Weapon_LegoObject_IsActiveWithTracker ((bool32 (__cdecl* )(LegoObject* liveObj))0x00471fa0)
bool32 __cdecl Weapon_LegoObject_IsActiveWithTracker(LegoObject* liveObj);

// Updates the mounted laser tracker on an object when in tracker mode.
// The tracker will pivot towards the direction of the mouse, and fire when mslb is down.
// <LegoRR.exe @00471fe0>
//#define Weapon_LegoObject_UpdateSelectedTracker ((bool32 (__cdecl* )(LegoObject* liveObj, real32 elapsed, uint32 frameNo))0x00471fe0)
bool32 __cdecl Weapon_LegoObject_UpdateSelectedTracker(LegoObject* liveObj, real32 elapsed, uint32 frameNo);

// Updates the mounted laser tracker on an object when not in tracker mode.
// The tracker will pivot towards the direction of the vehicle/building.
// <LegoRR.exe @004721c0>
//#define Weapon_LegoObject_UpdateUnselectedTracker ((bool32 (__cdecl* )(LegoObject* liveObj, real32 elapsed, uint32 frameNo))0x004721c0)
bool32 __cdecl Weapon_LegoObject_UpdateUnselectedTracker(LegoObject* liveObj, real32 elapsed, uint32 frameNo);

// <LegoRR.exe @00472280>
//#define Weapon_LegoObject_UpdateTracker ((bool32 (__cdecl* )(LegoObject* liveObj, real32 elapsed))0x00472280)
bool32 __cdecl Weapon_LegoObject_UpdateTracker(LegoObject* liveObj, real32 elapsed);

// Removes the projectile reference if it matches the specified object.
// DATA: LegoObject* projectileObj
// <LegoRR.exe @00472320>
//#define Weapon_Callback_RemoveProjectileReference ((bool32 (__cdecl* )(LegoObject* liveObj, void* projectileObj))0x00472320)
bool32 __cdecl Weapon_Callback_RemoveProjectileReference(LegoObject* liveObj, LegoObject* projectileObj);

// <LegoRR.exe @00472340>
//#define Weapon_LegoObject_WithinWeaponRange ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* otherObj))0x00472340)
bool32 __cdecl Weapon_LegoObject_WithinWeaponRange(LegoObject* liveObj, LegoObject* otherObj);

// <LegoRR.exe @004723f0>
//#define Weapon_LegoObject_WithinAwarenessRange ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* otherObj))0x004723f0)
bool32 __cdecl Weapon_LegoObject_WithinAwarenessRange(LegoObject* liveObj, LegoObject* otherObj);

#pragma endregion

}
