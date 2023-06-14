// Object.h : 
//

#pragma once

#include "../../engine/geometry.h"
#include "../../engine/undefined.h"

#include "../../engine/core/ListSet.hpp"
#include "../../engine/gfx/Containers.h"
#include "../../engine/drawing/Images.h"

#include "../GameCommon.h"
#include "BezierCurve.h"
#include "ObjectRecall.h"				// For SaveStruct_18


namespace LegoRR
{; // !<---

/**********************************************************************************
 ******** Forward Declarations
 **********************************************************************************/

#pragma region Forward Declarations

// Moved to ObjectRecall.h, as a temporary location that both FrontEnd.h and Object.h can rely on without needing to include these heavy modules.
#if false
namespace Dummy {

	struct SaveStruct_18
	{
		/*00,18*/ undefined field_0x0_0x17[24];
		/*18*/
	};
	assert_sizeof(SaveStruct_18, 0x18);
};
#endif

#pragma endregion

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define OBJECT_MAXLISTS				32			// 2^32 - 1 possible objects...

#define OBJECT_MAXHIDDENOBJECTS		200			// Max OL objects that are not exposed on level start.

#pragma endregion

/**********************************************************************************
 ******** Function Typedefs
 **********************************************************************************/

#pragma region Function Typedefs

typedef bool32 (__cdecl* LegoObject_RunThroughListsCallback)(LegoObject* liveObj, void* data);

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

enum RouteAction : uint8 // [LegoRR/Text.c|enum:0x1|type:byte]
{
	ROUTE_ACTION_NONE        = 0,
	ROUTE_ACTION_UNK_1       = 1,
	ROUTE_ACTION_REINFORCE   = 2,
	ROUTE_ACTION_GATHERROCK  = 3,
	ROUTE_ACTION_CLEAR       = 4,
	ROUTE_ACTION_UNK_5       = 5,
	ROUTE_ACTION_REPAIRDRAIN = 6,
	ROUTE_ACTION_STORE       = 7,
	ROUTE_ACTION_DROP        = 8,
	ROUTE_ACTION_PLACE       = 9,
	ROUTE_ACTION_UNK_10      = 10,
	ROUTE_ACTION_EAT         = 11,
	ROUTE_ACTION_UNK_12      = 12,
	ROUTE_ACTION_UNK_13      = 13,
	ROUTE_ACTION_TRAIN       = 14,
	ROUTE_ACTION_UPGRADE     = 15,
	ROUTE_ACTION_UNK_16      = 16,
	ROUTE_ACTION_UNK_17      = 17,
	ROUTE_ACTION_RECHARGE    = 18,
	ROUTE_ACTION_DOCK        = 19,
	ROUTE_ACTION_ATTACK      = 20,
	ROUTE_ACTION_21          = 21,
};
assert_sizeof(RouteAction, 0x1);


enum LiveFlags1 : uint32 // [LegoRR/LegoObject.c|flags:0x4|type:uint]
{
	LIVEOBJ1_NONE              = 0,
	LIVEOBJ1_MOVING            = 0x1,
	LIVEOBJ1_LIFTING           = 0x2,
	LIVEOBJ1_TURNING           = 0x4,
	LIVEOBJ1_DRILLING          = 0x8,
	LIVEOBJ1_DRILLINGSTART     = 0x10,
	LIVEOBJ1_UNUSED_20         = 0x20, // Likely unused, but included to fill in the gaps.
	LIVEOBJ1_REINFORCING       = 0x40,
	LIVEOBJ1_TURNRIGHT         = 0x80,
	LIVEOBJ1_EXPANDING         = 0x100,
	LIVEOBJ1_GATHERINGROCK     = 0x200,
	LIVEOBJ1_CARRYING          = 0x400,
	LIVEOBJ1_UNK_800           = 0x800,
	LIVEOBJ1_GETTINGHIT        = 0x1000,
	LIVEOBJ1_STORING           = 0x2000,
	LIVEOBJ1_UNK_4000          = 0x4000,
	LIVEOBJ1_WAITING           = 0x8000,
	LIVEOBJ1_UNK_10000         = 0x10000,
	LIVEOBJ1_ENTERING_WALLHOLE = 0x20000, // This is a vague guess based on activity names... or something or other.
	LIVEOBJ1_CLEARING          = 0x40000,
	LIVEOBJ1_PLACING           = 0x80000,
	LIVEOBJ1_CRUMBLING         = 0x100000,
	LIVEOBJ1_TELEPORTINGDOWN   = 0x200000,
	LIVEOBJ1_TELEPORTINGUP     = 0x400000,
	LIVEOBJ1_RUNNINGAWAY       = 0x800000,
	LIVEOBJ1_REPAIRDRAINING    = 0x1000000, // Multi-use flag for repairing an object, or draining from an object...
	LIVEOBJ1_CAUGHTINWEB       = 0x2000000,
	LIVEOBJ1_SLIPPING          = 0x4000000,
	LIVEOBJ1_SCAREDBYPLAYER    = 0x8000000,
	LIVEOBJ1_UNUSED_10000000   = 0x10000000, // Likely unused, but included to fill in the gaps.
	LIVEOBJ1_RESTING           = 0x20000000,
	LIVEOBJ1_EATING            = 0x40000000,
	LIVEOBJ1_CANTDO            = 0x80000000, // Activity_FloatOn
};
flags_end(LiveFlags1, 0x4);


enum LiveFlags2 : uint32 // [LegoRR/LegoObject.c|flags:0x4|type:uint]
{
	LIVEOBJ2_NONE                 = 0,
	LIVEOBJ2_THROWING             = 0x1,
	LIVEOBJ2_THROWN               = 0x2,
	LIVEOBJ2_UNK_4                = 0x4, // OL object flag? Related to driving? Route to object and get in?
	LIVEOBJ2_DRIVING              = 0x8,
	LIVEOBJ2_UNK_10               = 0x10,
	LIVEOBJ2_UNK_20               = 0x20,
	LIVEOBJ2_UNK_40               = 0x40,
	LIVEOBJ2_UNK_80               = 0x80, // Observed with Flocks
	LIVEOBJ2_UNK_100              = 0x100,
	LIVEOBJ2_BUILDPATH            = 0x200,
	LIVEOBJ2_TRAINING             = 0x400,
	LIVEOBJ2_UNK_800              = 0x800,
	LIVEOBJ2_DAMAGE_UNK_1000      = 0x1000,
	LIVEOBJ2_SHOWDAMAGENUMBERS    = 0x2000,
	LIVEOBJ2_PUSHED               = 0x4000,
	LIVEOBJ2_UPGRADING            = 0x8000,
	LIVEOBJ2_TRIGGERFRAMECALLBACK = 0x10000, // What this actually does isn't clear.
	LIVEOBJ2_UNK_20000            = 0x20000,
	LIVEOBJ2_UNK_40000            = 0x40000,
	LIVEOBJ2_UNK_80000            = 0x80000,
	LIVEOBJ2_UNK_100000           = 0x100000,
	LIVEOBJ2_UNK_200000           = 0x200000,
	LIVEOBJ2_FIRINGLASER          = 0x400000,
	LIVEOBJ2_FIRINGPUSHER         = 0x800000,
	LIVEOBJ2_FIRINGFREEZER        = 0x1000000,
	LIVEOBJ2_ACTIVEELECTRICFENCE  = 0x2000000,
	LIVEOBJ2_UNK_4000000          = 0x4000000,
	LIVEOBJ2_FROZEN               = 0x8000000,
	LIVEOBJ2_RECHARGING           = 0x10000000,
	LIVEOBJ2_UNK_20000000         = 0x20000000,
	LIVEOBJ2_DAMAGESHAKING        = 0x40000000, // Shaking effect when buildings are getting hit by RMonsters/boulders(?)
	LIVEOBJ2_UNK_80000000         = 0x80000000,
};
flags_end(LiveFlags2, 0x4);


enum LiveFlags3 : uint32 // [LegoRR/LegoObject.c|flags:0x4|type:uint]
{
	LIVEOBJ3_NONE           = 0,
	LIVEOBJ3_UNK_1          = 0x1, // Set for MiniFigure types, but haven't found usage yet.
	LIVEOBJ3_CANDIG         = 0x2,
	LIVEOBJ3_CANREINFORCE   = 0x4,
	LIVEOBJ3_CANTURN        = 0x8,
	LIVEOBJ3_CANFIRSTPERSON = 0x10,
	LIVEOBJ3_CANCARRY       = 0x20, // Only checked in LegoObject_FUN_00438720 and Interface_ObjectCallback_FUN_0041f400.
	                                // LegoObject_FUN_00438720 sets this flag to true for Building types if they can store or process.
	LIVEOBJ3_CANPICKUP      = 0x40, // True for MiniFigures, and true for Vehicles with CANCARRY+CROSSLAND.
	                                // Only checked in Interface_ObjectCallback_FUN_0041f400.
	LIVEOBJ3_CANYESSIR      = 0x80, // Allows the unit to respond with SFX_YesSir.
	LIVEOBJ3_CANSELECT      = 0x100, // This isn't fool-proof. It's still near-impossible to select RMonsters,
	                                 // since they manage to interrupt the selection immediately after...
	LIVEOBJ3_UNK_200        = 0x200, // Set for RockMonster types, but haven't found usage yet.
	LIVEOBJ3_UNK_400        = 0x400,
	LIVEOBJ3_UNUSED_800     = 0x800, // Observed on Rock Raider being pummeled by RockFallin.
	LIVEOBJ3_CENTERBLOCKIDLE = 0x1000, // Used for vehicles. These units will only idle in the center of a block. Ignored if STATS1_ROUTEAVOIDANCE.
	                                   // Only used in LegoObject_Route_AllocPtr_FUN_004419c0.
	LIVEOBJ3_UNK_2000       = 0x2000,
	LIVEOBJ3_UNK_4000       = 0x4000,
	LIVEOBJ3_CANDYNAMITE    = 0x8000,
	LIVEOBJ3_UNK_10000      = 0x10000, // Seen when an object starts ticking down, but also seen in a ton of other activities.
	LIVEOBJ3_SIMPLEOBJECT   = 0x20000, // Guess at usage, for resources or other objects that don't do anything on their own.
	LIVEOBJ3_CANDAMAGE      = 0x40000,
	LIVEOBJ3_UPGRADEPART    = 0x80000, // When running through the list of all LegoObjects, this flag states to ignore this object.
	                                   // (99% of the calls to Run through lists ignore objects with this flag).
	LIVEOBJ3_ALLOWCULLING_UNK = 0x100000, // States that an object can be hidden when over certain types of non-floor blocks.
	                                      // For example, if a building with this flag is founded on blocks that are all hidden, then it'll be hidden too.
										  // This flag is only set for PowerCrystals/Ore, but is unset in a lot of places, so it's a one-time deal.
										  // Maybe CANUNCOVER or CANCULL(?)
	LIVEOBJ3_SELECTED       = 0x200000, // Object is selected (required to enter laser tracker mode, because double select).
	LIVEOBJ3_AITASK_UNK_400000 = 0x400000, // Only checked in AITask_Callback_UpdateObject.
	LIVEOBJ3_REMOVING       = 0x800000,
	LIVEOBJ3_UNK_1000000    = 0x1000000,
	LIVEOBJ3_UNK_2000000    = 0x2000000,
	LIVEOBJ3_CANGATHER      = 0x4000000,
	LIVEOBJ3_MONSTER_UNK_8000000 = 0x8000000, // Flag for RockMonster type, only checked in LegoObject_FUN_00439e90.
	LIVEOBJ3_CANROUTERUBBLE = 0x10000000,
	LIVEOBJ3_HASPOWER       = 0x20000000,
	LIVEOBJ3_UNK_40000000   = 0x40000000,
	LIVEOBJ3_POWEROFF       = 0x80000000,
};
flags_end(LiveFlags3, 0x4);


enum LiveFlags4 : uint32 // [LegoRR/LegoObject.c|flags:0x4|type:uint]
{
	LIVEOBJ4_NONE          = 0,
	LIVEOBJ4_LASERTRACKERMODE   = 0x1, // Used by the object whose mounted laser is currently being aimed (red selection box).
	LIVEOBJ4_DOUBLESELECTREADY  = 0x2, // Required to enter laser tracker mode. Set during PTL_ClearSelection if LIVEOBJ3_SELECTED, 
	LIVEOBJ4_UNK_4         = 0x4,
	LIVEOBJ4_UNK_8         = 0x8,
	LIVEOBJ4_UNK_10        = 0x10,
	LIVEOBJ4_CALLTOARMS    = 0x20, 
	LIVEOBJ4_DOCKOCCUPIED       = 0x40, // Used when a building water entrance is occupied (for both vehicle and building). Linked by routeToObject.
	LIVEOBJ4_ENTRANCEOCCUPIED   = 0x80, // Used when a building land entrance is occupied (for both vehicle and building). Linked by routeToObject.
	LIVEOBJ4_USEDBYCONSTRUCTION = 0x100, // Used when resource is placed in construction zone (reserved, and not for use?).
	LIVEOBJ4_DONTSHOWHELPWINDOW = 0x200, // Prevents showing the help window for this object. This is set for OL objects and objects exposed in a new cavern.
	LIVEOBJ4_CRYORECOSTDROPPED  = 0x800, // Used when teleporing-up a constructed object to signal that its resource costs have been returned (spawned).
	LIVEOBJ4_UNK_1000      = 0x1000,
	LIVEOBJ4_UNK_2000      = 0x2000,
	LIVEOBJ4_UNK_4000      = 0x4000,
	LIVEOBJ4_UNK_8000      = 0x8000,
	LIVEOBJ4_UNK_10000     = 0x10000,
	LIVEOBJ4_UNK_20000     = 0x20000,
	LIVEOBJ4_UNK_40000     = 0x40000,
	LIVEOBJ4_ENGINESOUNDPLAYING = 0x80000,
	LIVEOBJ4_DRILLSOUNDPLAYING  = 0x100000,
	LIVEOBJ4_UNK_200000    = 0x200000,
	LIVEOBJ4_UNK_400000    = 0x400000,
};
flags_end(LiveFlags4, 0x4);


// LiveFlags5 has been replaced by `LegoObject_AbilityFlags`.
#if false
enum LiveFlags5 : uint32 // [LegoRR/LegoObject.c|flags:0x4|type:uint] These flags contain LiveObject abilities, and are the one of the fields stored/restored with ObjectRecall.
{
	LIVEOBJ5_NONE             = 0,
	LIVEOBJ5_ABILITY_PILOT    = 0x1,
	LIVEOBJ5_ABILITY_SAILOR   = 0x2,
	LIVEOBJ5_ABILITY_DRIVER   = 0x4,
	LIVEOBJ5_ABILITY_DYNAMITE = 0x8,
	LIVEOBJ5_ABILITY_REPAIR   = 0x10,
	LIVEOBJ5_ABILITY_SCANNER  = 0x20,
};
flags_end(LiveFlags5, 0x4);
#endif


enum RouteFlags : uint8 // [LegoRR/LegoObject.c|flags:0x1|type:byte]
{
	ROUTE_FLAG_NONE         = 0,
	ROUTE_DIRECTION_MASK    = 0x3,
	ROUTE_FLAG_GOTOBUILDING = 0x4,
	ROUTE_FLAG_UNK_8        = 0x8,
	ROUTE_UNK_MASK_c        = 0xc,
	ROUTE_FLAG_UNK_10       = 0x10,
	ROUTE_FLAG_UNK_20       = 0x20,
	ROUTE_FLAG_RUNAWAY      = 0x40,
};
flags_end(RouteFlags, 0x1);


flags_scoped(LegoObject_GlobFlags) : uint32 // [LegoRR/LegoObject.c|flags:0x4|type:uint] LegoObject_GlobFlags, ReservedPool LiveObject INITFLAGS
{
	OBJECT_GLOB_FLAG_NONE             = 0,
	OBJECT_GLOB_FLAG_INITIALISED      = 0x1,
	OBJECT_GLOB_FLAG_REMOVING         = 0x2,  // Currently in the object update loop.
	OBJECT_GLOB_FLAG_POWERUPDATING    = 0x4,  // Currently in the PowerGrid update loop.
	OBJECT_GLOB_FLAG_POWERNEEDSUPDATE = 0x8,  // The PowerGrid has requested an object, but we're currently in the update loop (`OBJECT_GLOB_FLAG_UPDATING`).
	OBJECT_GLOB_FLAG_UPDATING         = 0x10, // Used by LegoObject_UpdateAll.
	OBJECT_GLOB_FLAG_LEVELENDING      = 0x20, // set/unset by LegoObject_SetGlobFlag20, which is called by:
	                                          //   set(false) by Lego_LoadLevel
										      //   set(true)  by Lego_SetLoadFlag_StartTeleporter (for objective crystals failed)
										      //   set(true)  by Objective_SetCompleteStatus
	OBJECT_GLOB_FLAG_CYCLEUNITS       = 0x40, // Units have been cycled at least once since start of level.
};
flags_scoped_end(LegoObject_GlobFlags, 0x4);

#pragma endregion

/**********************************************************************************
 ******** Typedefs
 **********************************************************************************/

#pragma region Typedefs

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct SearchObjectBlockXY_c // [LegoRR/search.c|struct:0xc]
{
	/*0,4*/	LegoObject* resultObj;
	/*4,4*/	sint32 bx;
	/*8,4*/	sint32 by;
	/*c*/
};
assert_sizeof(SearchObjectBlockXY_c, 0xc);


struct RoutingBlock // [LegoRR/Routing.c|struct:0x14]
{
	/*00,8*/	Point2I	blockPos;
	/*08,8*/	Point2F	blockOffset; // In range of [0.0,1.0], where 0.5 is the center of the block.
	/*10,1*/	RouteFlags flagsByte;
	/*11,1*/	RouteAction actionByte;
	/*12,2*/	undefined field_0x12_0x13[2];
	/*14*/
};
assert_sizeof(RoutingBlock, 0x14);


struct HiddenObject // [LegoRR/LegoObject.c|struct:0x2c] Name is only guessed
{
	/*00,8*/	Point2I blockPos;     // (ol: xPos, yPos -> blockPos)
	/*08,8*/	Point2F worldPos;     // (ol: xPos, yPos)
	/*10,4*/	real32 heading;       // (ol: heading -> radians)
	/*14,4*/	ObjectModel* model;   // (ol: type)
	/*18,4*/	LegoObject_Type type; // (ol: type)
	/*1c,4*/	LegoObject_ID id;     // (ol: type)
	/*20,4*/	real32 health;        // (ol: health)
	/*24,4*/	char* olistID;        // (ol: Object%i)
	/*28,4*/	char* olistDrivingID; // (ol: driving)
	/*2c*/
};
assert_sizeof(HiddenObject, 0x2c);


struct LegoObject // [LegoRR/LegoObject.c|struct:0x40c|tags:LISTSET]
{
	/*000,4*/       LegoObject_Type type;
	/*004,4*/       LegoObject_ID id;
	/*008,4*/       char* customName; // max size is 11 (NOT null-terminated)
	/*00c,4*/       VehicleModel* vehicle; // Model for vehicle objects only.
	/*010,4*/       CreatureModel* miniFigure; // Model for mini-figure objects only.
	/*014,4*/       CreatureModel* rockMonster; // Model for monster objects only.
	/*018,4*/       BuildingModel* building; // Model for building objects only.
	/*01c,4*/       Gods98::Container* other; // Model for simple objects only.
	/*020,4*/       Upgrade_PartModel* upgradePart; // First upgrade part model in linked list of parts.
	/*024,4*/       RoutingBlock* routeBlocks; // Unknown pointer, likely in large allocated table
	/*028,4*/       uint32 routeBlocksTotal; // total blocks to travel for current route
	/*02c,4*/       uint32 routeBlocksCurrent; // number of blocks traveled (up to routingBlocksTotal)
	/*030,25c*/		BezierCurve routingCurve; // BezierCurve/Catmull-rom spline data
	/*28c,4*/		real32 routeCurveTotalDist;
	/*290,4*/		real32 routeCurveCurrDist;
	/*294,4*/		real32 routeCurveInitialDist; // Used as spill-over when distance traveled is beyond routeCurveTotalDist.
	/*298,8*/       Point2F point_298;
	/*2a0,c*/       Vector3F tempPosition; // Last position used for finding new face direction when moving.
										   // Only ever used as a temporary variable.
	/*2ac,c*/       Vector3F faceDirection; // 1.0 to -1.0 directions that determine rotation with atan2
	/*2b8,4*/       real32 faceDirectionLength; // faceDirection length (faceDirection may be Vector4F...)
	/*2bc,4*/       sint32 strafeSignFP; // (direction sign only, does higher numbers do not affect speed)
	/*2c0,4*/       sint32 forwardSignFP; // (direction sign only, does higher numbers do not affect speed)
	/*2c4,4*/       real32 rotateSpeedFP;
	/*2c8,c*/       Vector3F dirVector_2c8; // Always (0.0f, 0.0f, 0.0f)
	/*2d4,4*/       real32 animTime;
	/*2d8,4*/       uint32 animRepeat; // Number of times an activity animation is set to repeat (i.e. number of jumping jacks/reinforce hits). Zero is default.
	/*2dc,4*/       Gods98::Container* cameraNull;
	/*2e0,4*/       uint32 cameraFrame;
	/*2e4,4*/       Gods98::Container* contMiniTeleportUp;
	/*2e8,4*/       const char* activityName1;
	/*2ec,4*/       const char* activityName2; // Seems to be used with related objects like driven, swapped with activityName1.
	/*2f0,4*/       AITask* aiTask; // Linked list of tasks (or null). Linked using the `AITask::next` field.
	/*2f4,8*/       Point2F targetBlockPos; // (init: -1.0f, -1.0f)
	/*2fc,4*/       LegoObject* routeToObject; // other half of object_300
	/*300,4*/       LegoObject* interactObject; // Used in combination with routeToObject for Upgrade station and RM boulders.
	/*304,4*/       LegoObject* carryingThisObject;
	/*308,17*/      LegoObject* carriedObjects[6]; // (includes carried vehicles)
	/*320,4*/		uint32 carryingIndex; // Index of carried object in holder's carriedObjects list.
	/*324,4*/       uint32 numCarriedObjects;
	/*328,4*/       uint32 carryNullFrames;
	/*32c,4*/       Flocks* flocks;
	/*330,4*/       uint32 objLevel;
	/*334,4*/       ObjectStats* stats;
	/*338,4*/       real32 aiTimer_338;
	/*33c,4*/       real32 carryRestTimer_33c;
	/*340,4*/       real32 health; // (init: -1.0f)
	/*344,4*/       real32 energy; // (init: -1.0f)
	/*348,4*/       sint32* stolenCrystalLevels; // (alloc: new int[6]) Each element is the count stolen for a level, index 0 only seems to be used for recovery
	/*34c,4*/       LOD_PolyLevel polyLOD;
	/*350,4*/       sint32 drillSoundHandle; // Handle returned by SFX_Play functions
	/*354,4*/       sint32 engineSoundHandle; // Handle returned by SFX_Play functions
	/*358,4*/       real32 weaponSlowDeath;
	/*35c,4*/       uint32 weaponID;
	/*360,4*/       real32 weaponRechargeTimer;
	/*364,4*/       LegoObject* freezeObject; // (bi-directional link between frozen RockMonster and IceCube)
	/*368,4*/       real32 freezeTimer;
	/*36c,4*/       LegoObject* driveObject; // (bi-directional link between driver and driven)
	/*370,14*/      LegoObject_ToolType carriedTools[5];
	/*384,4*/       uint32 numCarriedTools;
	/*388,4*/       real32 bubbleTimer;
	/*38c,4*/       Gods98::Image* bubbleImage;
	/*390,4*/       uint32 teleporter_modeFlags;
	/*394,4*/       uint32 teleporter_teleportFlags;
	/*398,4*/       TeleporterService* teleporter;
	/*39c,c*/       Vector3F beamVector_39c; // (used for unkWeaponTypes 1-3 "Lazer", "Pusher", "Freezer")
	/*3a8,c*/       Vector3F weaponVector_3a8; // (used for unkWeaponType 4 "Lazer")
	/*3b4,8*/       Point2F pushingVec2D;
	/*3bc,4*/		real32 pushingDist;
	/*3c0,4*/       LegoObject* throwObject; // (bi-directional link between thrower and thrown)
	/*3c4,4*/       LegoObject* projectileObject; // Projectile fired from weapon.
	/*3c8,4*/       undefined4 field_3c8; // (unused?)
	/*3cc,4*/       LegoObject* teleportDownObject;
	/*3d0,4*/       real32 damageNumbers; // Used to display damage text over objects.
	/*3d4,4*/       real32 elapsedTime1; // elapsed time counter 1 (for training, and something with vehicles)
	/*3d8,4*/       real32 elapsedTime2; // elapsed time counter 2 (for upgrading and one other thing)
	/*3dc,4*/       real32 eatWaitTimer; // Unit will try to find food once the timer reaches 125.
	/*3e0,4*/       LiveFlags1 flags1;
	/*3e4,4*/       LiveFlags2 flags2;
	/*3e8,4*/       LiveFlags3 flags3; // (assigned 0, flags?)
	/*3ec,4*/       LiveFlags4 flags4;
	/*3f0,4*/       LegoObject_AbilityFlags abilityFlags; // (orig: flags5) Trained ability flags, and saved in ObjectRecall.
	/*3f4,4*/       undefined4 field_3f4;
	/*3f8,4*/       bool32 bool_3f8;
	/*3fc,4*/       real32 hurtSoundTimer; // Timer resets after reaching (hurtSoundDuration * 2).
	/*400,4*/       real32 hurtSoundDuration;
	/*404,4*/       LegoObject_UpgradeType upgradingType; // New upgrade type added as mask to vehicle level when upgrade is finished.
	/*408,4*/       LegoObject* nextFree; // (for listSet)
	/*40c*/
};
assert_sizeof(LegoObject, 0x40c);


struct LegoObject_Globs // [LegoRR/LegoObject.c|struct:0xc644|tags:GLOBS]
{
	/*0000,80*/     LegoObject* listSet[OBJECT_MAXLISTS];
	/*0080,4*/      LegoObject* freeList;
	/*0084,4b0*/    SFX_ID objectTtSFX[LegoObject_Type_Count][LegoObject_ID_Count]; // [objType:20][objID:15]
	/*0534,13c*/    const char* activityName[Activity_Type_Count]; // [activityType:79]
	/*0670,4*/      void* UnkSurfaceGrid_1_TABLE;
	/*0674,4*/      void* UnkSurfaceGrid_2_TABLE;
	/*0678,4*/      uint32 UnkSurfaceGrid_COUNT;
	/*067c,4*/      real32 radarSurveyCycleTimer; // Timer for how often survey scans update.
	/*0680,4*/      uint32 listCount;
	/*0684,4*/      LegoObject_GlobFlags flags;
	/*0688,2c*/     sint32 toolNullIndex[LegoObject_ToolType_Count]; // [toolType:11]
	/*06b4,4b00*/   uint32 objectTotalLevels[LegoObject_Type_Count][LegoObject_ID_Count][OBJECT_MAXLEVELS]; // [objType:20][objID:15][objLevel:16]
	/*51b4,4b00*/   uint32 objectPrevLevels[LegoObject_Type_Count][LegoObject_ID_Count][OBJECT_MAXLEVELS]; // [objType:20][objID:15][objLevel:16]
	/*9cb4,4*/      LegoObject_AbilityFlags NERPs_TrainFlags;
	/*9cb8,4*/      LegoObject* minifigureObj_9cb8; // MINIFIGOBJ_004e9448
	/*9cbc,a0*/     Point2I slugHoleBlocks[20];
	/*9d5c,50*/     Point2I rechargeSeamBlocks[10];
	/*9dac,4*/      uint32 slugHoleCount;
	/*9db0,4*/      uint32 rechargeSeamCount;
	/*9db4,2260*/   HiddenObject hiddenObjects[OBJECT_MAXHIDDENOBJECTS];
	/*c014,4*/      uint32 hiddenObjectCount;
	/*c018,4*/      real32 dischargeBuildup; // When >= 1.0f, consume one crystal.
	/*c01c,18*/     SaveStruct_18 savestruct18_c01c;
	/*c034,400*/    LegoObject* cycleUnits[OBJECT_MAXCYCLEUNITS];
	/*c434,4*/      uint32 cycleUnitCount;
	/*c438,4*/      uint32 cycleBuildingCount;
	/*c43c,190*/    LegoObject* liveObjArray100_c43c[100]; // Used for water docking vehicles?
	/*c5cc,4*/      uint32 uintCount_c5cc; // Count for liveObjArray100_c43c
	/*c5d0,18*/     const char* abilityName[LegoObject_AbilityType_Count]; // [abilityType:6]
	/*c5e8,18*/     Gods98::Image* ToolTipIcons_Abilities[LegoObject_AbilityType_Count]; // [abilityType:6] (cfg: ToolTipIcons)
	/*c600,2c*/     Gods98::Image* ToolTipIcons_Tools[LegoObject_ToolType_Count]; // [toolType:11] (cfg: ToolTipIcons)
	/*c62c,4*/      Gods98::Image* ToolTipIcon_Blank; // (cfg: ToolTipIcons::Blank)
	/*c630,4*/      Gods98::Image* ToolTipIcon_Ore; // (cfg: ToolTipIcons::Ore)
	/*c634,4*/      uint32 BuildingsTeleported;
	/*c638,4*/      real32 s_sound3DUpdateTimer;
	/*c63c,4*/      undefined4 s_stepCounter_c63c; // (static, counter %4 for step SFX) DAT_004ebdcc
	/*c640,4*/      void** s_FlocksDestroy_c640; // (static, Struct 0x10, used in Flocks activities (QUICK_DESTROY??)) PTR_004ebdd0
	/*c644*/
};
assert_sizeof(LegoObject_Globs, 0xc644);


//using LegoObject_ListSet = ListSet::WrapperCollection<LegoObject_Globs>;

#pragma endregion

/**********************************************************************************
 ******** Classes
 **********************************************************************************/

#pragma region Classes

class LegoObject_ListSet : public ListSet::WrapperCollection<LegoObject_Globs>
{
public:
	LegoObject_ListSet(LegoObject_Globs& cont)
		: ListSet::WrapperCollection<LegoObject_Globs>(cont)
	{
	}

private:
	static bool FilterSkipUpgradeParts(const LegoObject* liveObj);
	/*{
		return ListSet::IsAlive(liveObj) && !(liveObj->flags3 & LIVEOBJ3_UPGRADEPART);
	}*/

public:
	LegoObject_ListSet::enumerable<FilterSkipUpgradeParts> EnumerateSkipUpgradeParts();
	/*{
		return this->EnumerateWhere<FilterSkipUpgradeParts>();
		//return LegoObject_ListSet::enumerable<FilterSkipUpgradeParts>(this->m_cont);
	}*/
};

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// Static variable used in LegoObject_Callback_Update.
// <LegoRR.exe @004a58b0>
extern real32 & s_objectDamageShakeTimer; // = 1.0f;

// <LegoRR.exe @004df790>
extern LegoObject_Globs & objectGlobs;

// Static variable used in LegoObject_Callback_Update. Assigned to but never used.
// <LegoRR.exe @00557090>
extern LegoObject* (& s_currentUpdateObject);

extern LegoObject_ListSet objectListSet;

#pragma endregion

/**********************************************************************************
 ******** Macros
 **********************************************************************************/

#pragma region Macros

#define Activity_RegisterName(n)		    (objectGlobs.activityName[n]=#n)
#define AbilityType_RegisterName(n)         (objectGlobs.abilityName[n]=#n)

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @00436ee0>
//#define LegoObject_Initialise ((void (__cdecl* )(void))0x00436ee0)
void __cdecl LegoObject_Initialise(void);

// <LegoRR.exe @00437310>
//#define LegoObject_Shutdown ((void (__cdecl* )(void))0x00437310)
void __cdecl LegoObject_Shutdown(void);

// <LegoRR.exe @00437370>
//#define Object_Save_CopyStruct18 ((void (__cdecl* )(OUT SaveStruct_18* saveStruct18))0x00437370)
void __cdecl Object_Save_CopyStruct18(OUT SaveStruct_18* saveStruct18);

// <LegoRR.exe @00437390>
//#define Object_Save_OverwriteStruct18 ((void (__cdecl* )(SaveStruct_18* saveStruct18))0x00437390)
void __cdecl Object_Save_OverwriteStruct18(OPTIONAL const SaveStruct_18* saveStruct18);

// <LegoRR.exe @004373c0>
//#define LegoObject_GetObjectsBuilt ((sint32 (__cdecl* )(LegoObject_Type objType, bool32 excludeToolStore))0x004373c0)
uint32 __cdecl LegoObject_GetObjectsBuilt(LegoObject_Type objType, bool32 excludeToolStore);

// <LegoRR.exe @00437410>
//#define Object_LoadToolTipIcons ((void (__cdecl* )(const Gods98::Config* config))0x00437410)
void __cdecl Object_LoadToolTipIcons(const Gods98::Config* config);

// <LegoRR.exe @00437560>
//#define LegoObject_CleanupLevel ((void (__cdecl* )(void))0x00437560)
void __cdecl LegoObject_CleanupLevel(void);

// Used for consuming and producing unpowered crystals after weapon discharge.
// <LegoRR.exe @004375c0>
//#define LegoObject_Weapon_FUN_004375c0 ((void (__cdecl* )(LegoObject* liveObj, sint32 weaponID, real32 coef))0x004375c0)
void __cdecl LegoObject_Weapon_FUN_004375c0(LegoObject* liveObj, sint32 weaponID, real32 coef);

// <LegoRR.exe @00437690>
//#define LegoObject_DoOpeningClosing ((bool32 (__cdecl* )(LegoObject* liveObj, bool32 open))0x00437690)
bool32 __cdecl LegoObject_DoOpeningClosing(LegoObject* liveObj, bool32 open);

// <LegoRR.exe @00437700>
//#define LegoObject_CleanupObjectLevels ((void (__cdecl* )(void))0x00437700)
void __cdecl LegoObject_CleanupObjectLevels(void);

// <LegoRR.exe @00437720>
//#define LegoObject_GetLevelObjectsBuilt ((uint32 (__cdecl* )(LegoObject_Type objType, LegoObject_ID objID, uint32 objLevel, bool32 currentCount))0x00437720)
uint32 __cdecl LegoObject_GetLevelObjectsBuilt(LegoObject_Type objType, LegoObject_ID objID, uint32 objLevel, bool32 currentCount);

// <LegoRR.exe @00437760>
//#define LegoObject_GetPreviousLevelObjectsBuilt ((uint32 (__cdecl* )(LegoObject_Type objType, LegoObject_ID objID, uint32 objLevel))0x00437760)
uint32 __cdecl LegoObject_GetPreviousLevelObjectsBuilt(LegoObject_Type objType, LegoObject_ID objID, uint32 objLevel);

// <LegoRR.exe @00437790>
//#define LegoObject_IncLevelPathsBuilt ((void (__cdecl* )(bool32 incCurrent))0x00437790)
void __cdecl LegoObject_IncLevelPathsBuilt(bool32 incCurrent);

// Removes all route-to references that match the specified object.
// Does nothing if routeToObj is a Boulder type.
// <LegoRR.exe @004377b0>
//#define LegoObject_RemoveRouteToReferences ((void (__cdecl* )(LegoObject* routeToObj))0x004377b0)
void __cdecl LegoObject_RemoveRouteToReferences(LegoObject* routeToObj);

// Removes the route-to reference if it matches the specified object.
// DATA: LegoObject* routeToObj
// <LegoRR.exe @004377d0>
//#define LegoObject_Callback_RemoveRouteToReference ((bool32 (__cdecl* )(LegoObject* liveObj, void* pRouteToObj))0x004377d0)
bool32 __cdecl LegoObject_Callback_RemoveRouteToReference(LegoObject* liveObj, void* pRouteToObj);

// <LegoRR.exe @00437800>
//#define LegoObject_Remove ((bool32 (__cdecl* )(LegoObject* liveObj))0x00437800)
bool32 __cdecl LegoObject_Remove(LegoObject* liveObj);

// <LegoRR.exe @00437a70>
//#define LegoObject_RunThroughListsSkipUpgradeParts ((bool32 (__cdecl* )(LegoObject_RunThroughListsCallback callback, void* data))0x00437a70)
bool32 __cdecl LegoObject_RunThroughListsSkipUpgradeParts(LegoObject_RunThroughListsCallback callback, void* data);

// <LegoRR.exe @00437a90>
//#define LegoObject_RunThroughLists ((bool32 (__cdecl* )(LegoObject_RunThroughListsCallback callback, void* data, bool32 skipIgnoreMeObjs))0x00437a90)
bool32 __cdecl LegoObject_RunThroughLists(LegoObject_RunThroughListsCallback callback, void* data, bool32 skipUpgradeParts);

// <LegoRR.exe @00437b40>
//#define LegoObject_SetCustomName ((void (__cdecl* )(LegoObject* liveObj, OPTIONAL const char* newCustomName))0x00437b40)
void __cdecl LegoObject_SetCustomName(LegoObject* liveObj, OPTIONAL const char* newCustomName);

// <LegoRR.exe @00437ba0>
//#define HiddenObject_RemoveAll ((void (__cdecl* )(void))0x00437ba0)
void __cdecl HiddenObject_RemoveAll(void);

// <LegoRR.exe @00437c00>
//#define HiddenObject_ExposeBlock ((void (__cdecl* )(const Point2I* blockPos))0x00437c00)
void __cdecl HiddenObject_ExposeBlock(const Point2I* blockPos);

// <LegoRR.exe @00437ee0>
//#define HiddenObject_Add ((void (__cdecl* )(ObjectModel* objModel, LegoObject_Type objType, LegoObject_ID objID, Point2F* worldPos, real32 heading, real32 health, const char* olistID, const char* olistDrivingID))0x00437ee0)
void __cdecl HiddenObject_Add(ObjectModel* objModel, LegoObject_Type objType, LegoObject_ID objID, const Point2F* worldPos, real32 heading, real32 health, const char* olistID, const char* olistDrivingID);

// <LegoRR.exe @00437f80>
//#define LegoObject_CanShootObject ((bool32 (__cdecl* )(LegoObject* liveObj))0x00437f80)
bool32 __cdecl LegoObject_CanShootObject(LegoObject* liveObj);

// <LegoRR.exe @00437fc0>
//#define LegoObject_Create ((LegoObject* (__cdecl* )(ObjectModel* objModel, LegoObject_Type objType, LegoObject_ID objID))0x00437fc0)
LegoObject* __cdecl LegoObject_Create(ObjectModel* objModel, LegoObject_Type objType, LegoObject_ID objID);

// <LegoRR.exe @00438580>
//#define LegoObject_Create_internal ((LegoObject* (__cdecl* )(void))0x00438580)
LegoObject* __cdecl LegoObject_Create_internal(void);

// <LegoRR.exe @004385d0>
//#define LegoObject_AddList ((void (__cdecl* )(void))0x004385d0)
void __cdecl LegoObject_AddList(void);

// <LegoRR.exe @00438650>
//#define LegoObject_GetNumBuildingsTeleported ((uint32 (__cdecl* )(void))0x00438650)
uint32 __cdecl LegoObject_GetNumBuildingsTeleported(void);

// <LegoRR.exe @00438660>
//#define LegoObject_SetNumBuildingsTeleported ((void (__cdecl* )(uint32 numTeleported))0x00438660)
void __cdecl LegoObject_SetNumBuildingsTeleported(uint32 numTeleported);

// <LegoRR.exe @00438670>
//#define LegoObject_SetCrystalPoweredColour ((void (__cdecl* )(LegoObject* liveObj, bool32 powered))0x00438670)
void __cdecl LegoObject_SetCrystalPoweredColour(LegoObject* liveObj, bool32 powered);

// <LegoRR.exe @00438720>
//#define LegoObject_FUN_00438720 ((void (__cdecl* )(LegoObject* liveObj))0x00438720)
void __cdecl LegoObject_FUN_00438720(LegoObject* liveObj);

// <LegoRR.exe @00438840>
//#define LegoObject_SetPowerOn ((void (__cdecl* )(LegoObject* liveObj, bool32 on))0x00438840)
void __cdecl LegoObject_SetPowerOn(LegoObject* liveObj, bool32 on);

// This seems to be a catch-all type function to see if an object is "active".
// 
// For buildings, it must be receiving power, and NOT be powered-off.
// Aside from that, there are many other checks based on various flags states, and also health.
// 
// When ignoreUnpowered is true, powered state will not be checked.
// 
// Old name: LegoObject_CheckCondition_AndIsPowered
// <LegoRR.exe @00438870>
//#define LegoObject_IsActive ((bool32 (__cdecl* )(LegoObject* liveObj, bool32 ignoreUnpowered))0x00438870)
bool32 __cdecl LegoObject_IsActive(LegoObject* liveObj, bool32 ignoreUnpowered);

// <LegoRR.exe @004388d0>
//#define LegoObject_CreateInWorld ((LegoObject* (__cdecl* )(ObjectModel* objModel, LegoObject_Type objType, LegoObject_ID objID, uint32 objLevel, real32 xPos, real32 yPos, real32 heading))0x004388d0)
LegoObject* __cdecl LegoObject_CreateInWorld(ObjectModel* objModel, LegoObject_Type objType, LegoObject_ID objID, uint32 objLevel, real32 xPos, real32 yPos, real32 heading);

// <LegoRR.exe @00438930>
//#define LegoObject_FindPoweredBuildingAtBlockPos ((LegoObject* (__cdecl* )(const Point2I* blockPos))0x00438930)
LegoObject* __cdecl LegoObject_FindPoweredBuildingAtBlockPos(const Point2I* blockPos);

// DATA: SearchObjectBlockXY_c* search
// <LegoRR.exe @00438970>
//#define LegoObject_Callback_FindPoweredBuildingAtBlockPos ((bool32 (__cdecl* )(LegoObject* liveObj, void* pSearch))0x00438970)
bool32 __cdecl LegoObject_Callback_FindPoweredBuildingAtBlockPos(LegoObject* liveObj, void* pSearch);

// <LegoRR.exe @004389e0>
#define LegoObject_AddThisDrainedCrystals ((bool32 (__cdecl* )(LegoObject* liveObj, sint32 crystalDrainedAmount))0x004389e0)
//bool32 __cdecl LegoObject_AddThisDrainedCrystals(LegoObject* liveObj, sint32 crystalDrainedAmount);

// <LegoRR.exe @00438a30>
#define LegoObject_GetBuildingUpgradeCost ((bool32 (__cdecl* )(LegoObject* liveObj, OPTIONAL OUT uint32* oreCost))0x00438a30)
//bool32 __cdecl LegoObject_GetBuildingUpgradeCost(LegoObject* liveObj, OPTIONAL OUT uint32* oreCost);

// <LegoRR.exe @00438ab0>
#define LegoObject_UpgradeBuilding ((void (__cdecl* )(LegoObject* liveObj))0x00438ab0)
//void __cdecl LegoObject_UpgradeBuilding(LegoObject* liveObj);

// <LegoRR.exe @00438b70>
#define LegoObject_HasEnoughOreToUpgrade ((bool32 (__cdecl* )(LegoObject* liveObj, uint32 objLevel))0x00438b70)
//bool32 __cdecl LegoObject_HasEnoughOreToUpgrade(LegoObject* liveObj, uint32 objLevel);

// <LegoRR.exe @00438c20>
#define LegoObject_Search_FUN_00438c20 ((undefined4 (__cdecl* )(LegoObject* opt_liveObj, bool32 param_2))0x00438c20)
//undefined4 __cdecl LegoObject_Search_FUN_00438c20(LegoObject* opt_liveObj, bool32 param_2);

// <LegoRR.exe @00438ca0>
#define LegoObject_Search_FUN_00438ca0 ((undefined4 (__cdecl* )(LegoObject* liveObj, bool32 param_2))0x00438ca0)
//undefined4 __cdecl LegoObject_Search_FUN_00438ca0(LegoObject* liveObj, bool32 param_2);

// <LegoRR.exe @00438d20>
#define LegoObject_FUN_00438d20 ((LegoObject* (__cdecl* )(const Point2I* blockPos, LegoObject_Type objType, LegoObject_ID objID, uint32 objLevel))0x00438d20)
//LegoObject* __cdecl LegoObject_FUN_00438d20(const Point2I* blockPos, LegoObject_Type objType, LegoObject_ID objID, uint32 objLevel);

// <LegoRR.exe @00438da0>
#define LegoObject_FindResourceProcessingBuilding ((LegoObject* (__cdecl* )(LegoObject* liveObj, const Point2I* blockPos, LegoObject_Type objType, uint32 objLevel))0x00438da0)
//LegoObject* __cdecl LegoObject_FindResourceProcessingBuilding(LegoObject* liveObj, const Point2I* blockPos, LegoObject_Type objType, uint32 objLevel);

// <LegoRR.exe @00438e40>
#define LegoObject_Search_FUN_00438e40 ((undefined4 (__cdecl* )(LegoObject* liveObj, undefined4 param_2))0x00438e40)
//undefined4 __cdecl LegoObject_Search_FUN_00438e40(LegoObject* liveObj, undefined4 param_2);

// <LegoRR.exe @00438eb0>
#define LegoObject_Search_FUN_00438eb0 ((undefined4 (__cdecl* )(LegoObject* liveObj))0x00438eb0)
//undefined4 __cdecl LegoObject_Search_FUN_00438eb0(LegoObject* liveObj);

// <LegoRR.exe @00438f20>
#define LegoObject_FindSnackServingBuilding ((LegoObject* (__cdecl* )(LegoObject* liveObj))0x00438f20)
//LegoObject* __cdecl LegoObject_FindSnackServingBuilding(LegoObject* liveObj);

// <LegoRR.exe @00438f90>
#define LegoObject_FindBigTeleporter ((LegoObject* (__cdecl* )(const Point2F* worldPos))0x00438f90)
//LegoObject* __cdecl LegoObject_FindBigTeleporter(const Point2F* worldPos);

// <LegoRR.exe @00438ff0>
#define LegoObject_FindSmallTeleporter ((LegoObject* (__cdecl* )(const Point2F* worldPos))0x00438ff0)
//LegoObject* __cdecl LegoObject_FindSmallTeleporter(const Point2F* worldPos);

// <LegoRR.exe @00439050>
#define LegoObject_FindWaterTeleporter ((LegoObject* (__cdecl* )(const Point2F* worldPos))0x00439050)
//LegoObject* __cdecl LegoObject_FindWaterTeleporter(const Point2F* worldPos);

// <LegoRR.exe @004390b0>
#define Level_GetBuildingAtPosition ((LegoObject* (__cdecl* )(const Point2F* worldPos))0x004390b0)
//LegoObject* __cdecl Level_GetBuildingAtPosition(const Point2F* worldPos);

// <LegoRR.exe @00439110>
#define LegoObject_Search_FUN_00439110 ((undefined4 (__cdecl* )(LegoObject* liveObj, OPTIONAL const Point2F* worldPos, LegoObject_AbilityFlags abilityFlags))0x00439110)
//undefined4 __cdecl LegoObject_Search_FUN_00439110(LegoObject* liveObj, OPTIONAL const Point2F* worldPos, LegoObject_AbilityFlags abilityFlags);

// <LegoRR.exe @00439190>
#define LegoObject_HasTraining ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject_AbilityFlags abilityFlags))0x00439190)
//bool32 __cdecl LegoObject_HasTraining(LegoObject* liveObj, LegoObject_AbilityFlags abilityFlags);

// <LegoRR.exe @00439220>
#define LegoObject_IsDocksBuilding_Unk ((bool32 (__cdecl* )(LegoObject* liveObj))0x00439220)
//bool32 __cdecl LegoObject_IsDocksBuilding_Unk(LegoObject* liveObj);

// <LegoRR.exe @00439270>
#define LegoObject_CallbackSearch_FUN_00439270 ((bool32 (__cdecl* )(LegoObject* liveObj, sint32** search))0x00439270)
//bool32 __cdecl LegoObject_CallbackSearch_FUN_00439270(LegoObject* liveObj, sint32** search);

// <LegoRR.exe @004394c0>
#define LegoObject_CanStoredObjectTypeBeSpawned ((bool32 (__cdecl* )(LegoObject_Type objType))0x004394c0)
//bool32 __cdecl LegoObject_CanStoredObjectTypeBeSpawned(LegoObject_Type objType);

// <LegoRR.exe @00439500>
#define LegoObject_Callback_CanSpawnStoredObjects ((bool32 (__cdecl* )(LegoObject* liveObj1, LegoObject* spawnObj))0x00439500)
//bool32 __cdecl LegoObject_Callback_CanSpawnStoredObjects(LegoObject* liveObj1, LegoObject* spawnObj);

// <LegoRR.exe @00439540>
#define LegoObject_PTL_GenerateFromCryOre ((void (__cdecl* )(const Point2I* blockPos))0x00439540)
//void __cdecl LegoObject_PTL_GenerateFromCryOre(const Point2I* blockPos);

// <LegoRR.exe @00439600>
#define LegoObject_PTL_GenerateCrystalsAndOre ((void (__cdecl* )(const Point2I* blockPos, uint32 objLevel))0x00439600)
//void __cdecl LegoObject_PTL_GenerateCrystalsAndOre(const Point2I* blockPos, uint32 objLevel);

// <LegoRR.exe @00439630>
#define Level_GenerateCrystal ((void (__cdecl* )(const Point2I* blockPos, uint32 objLevel, OPTIONAL const Point2F* worldPos, bool32 showInfoMessage))0x00439630)
//void __cdecl Level_GenerateCrystal(const Point2I* blockPos, uint32 objLevel, OPTIONAL const Point2F* worldPos, bool32 showInfoMessage);

// <LegoRR.exe @00439770>
#define Level_GenerateOre ((void (__cdecl* )(const Point2I* blockPos, uint32 objLevel, OPTIONAL const Point2F* worldPos, bool32 showInfoMessage))0x00439770)
//void __cdecl Level_GenerateOre(const Point2I* blockPos, uint32 objLevel, OPTIONAL const Point2F* worldPos, bool32 showInfoMessage);

// <LegoRR.exe @004398a0>
#define LegoObject_GetLangName ((const char* (__cdecl* )(LegoObject* liveObj))0x004398a0)
//const char* __cdecl LegoObject_GetLangName(LegoObject* liveObj);

// <LegoRR.exe @00439980>
#define Object_GetTypeName ((const char* (__cdecl* )(LegoObject_Type objType, LegoObject_ID objID))0x00439980)
//const char* __cdecl Object_GetTypeName(LegoObject_Type objType, LegoObject_ID objID);

// <LegoRR.exe @00439a50>
#define Object_GetLangTheName ((const char* (__cdecl* )(LegoObject_Type objType, LegoObject_ID objID))0x00439a50)
//const char* __cdecl Object_GetLangTheName(LegoObject_Type objType, LegoObject_ID objID);

// <LegoRR.exe @00439b20>
#define Object_GetLangName ((const char* (__cdecl* )(LegoObject_Type objType, LegoObject_ID objID))0x00439b20)
//const char* __cdecl Object_GetLangName(LegoObject_Type objType, LegoObject_ID objID);

// <LegoRR.exe @00439bf0>
#define LegoObject_GetTypeAndID ((void (__cdecl* )(LegoObject* liveObj, OUT LegoObject_Type* objType, OUT LegoObject_ID* objID))0x00439bf0)
//void __cdecl LegoObject_GetTypeAndID(LegoObject* liveObj, OUT LegoObject_Type* objType, OUT LegoObject_ID* objID);

// <LegoRR.exe @00439c10>
#define Object_LoadToolNames ((void (__cdecl* )(const Gods98::Config* config, const char* gameName))0x00439c10)
//void __cdecl Object_LoadToolNames(const Gods98::Config* config, const char* gameName);

// <LegoRR.exe @00439c50>
//#define LegoObject_RequestPowerGridUpdate ((void (__cdecl* )(void))0x00439c50)
void __cdecl LegoObject_RequestPowerGridUpdate(void);

// <LegoRR.exe @00439c80>
#define LegoObject_VehicleMaxCarryChecksTime_FUN_00439c80 ((bool32 (__cdecl* )(LegoObject* liveObj))0x00439c80)
//bool32 __cdecl LegoObject_VehicleMaxCarryChecksTime_FUN_00439c80(LegoObject* liveObj);

// <LegoRR.exe @00439ce0>
#define LegoObject_TryCollectObject ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* targetObj))0x00439ce0)
//bool32 __cdecl LegoObject_TryCollectObject(LegoObject* liveObj, LegoObject* targetObj);

// <LegoRR.exe @00439e40>
#define LegoObject_FUN_00439e40 ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* otherObj))0x00439e40)
//bool32 __cdecl LegoObject_FUN_00439e40(LegoObject* liveObj, LegoObject* otherObj);

// <LegoRR.exe @00439e90>
#define LegoObject_FUN_00439e90 ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* targetObj, bool32 param_3))0x00439e90)
//bool32 __cdecl LegoObject_FUN_00439e90(LegoObject* liveObj, LegoObject* targetObj, bool32 param_3);

// <LegoRR.exe @00439f40>
#define LegoObject_CompleteVehicleUpgrade ((void (__cdecl* )(LegoObject* liveObj))0x00439f40)
//void __cdecl LegoObject_CompleteVehicleUpgrade(LegoObject* liveObj);

// <LegoRR.exe @00439f90>
#define LegoObject_SetLevel ((bool32 (__cdecl* )(LegoObject* liveObj, uint32 newLevel))0x00439f90)
//bool32 __cdecl LegoObject_SetLevel(LegoObject* liveObj, uint32 newLevel);

// <LegoRR.exe @00439fb0>
#define LegoObject_IsSmallTeleporter ((bool32 (__cdecl* )(LegoObject* liveObj))0x00439fb0)
//bool32 __cdecl LegoObject_IsSmallTeleporter(LegoObject* liveObj);

// <LegoRR.exe @00439fd0>
#define LegoObject_IsBigTeleporter ((bool32 (__cdecl* )(LegoObject* liveObj))0x00439fd0)
//bool32 __cdecl LegoObject_IsBigTeleporter(LegoObject* liveObj);

// <LegoRR.exe @00439ff0>
#define LegoObject_IsWaterTeleporter ((bool32 (__cdecl* )(LegoObject* liveObj))0x00439ff0)
//bool32 __cdecl LegoObject_IsWaterTeleporter(LegoObject* liveObj);

// <LegoRR.exe @0043a010>
#define LegoObject_UnkGetTerrainCrossBlock_FUN_0043a010 ((bool32 (__cdecl* )(LegoObject* liveObj, OPTIONAL OUT Point2I* blockPos))0x0043a010)
//bool32 __cdecl LegoObject_UnkGetTerrainCrossBlock_FUN_0043a010(LegoObject* liveObj, OPTIONAL OUT Point2I* blockPos);

// <LegoRR.exe @0043a0d0>
#define LegoObject_UnkGetTerrainGetOutAtLandBlock_FUN_0043a0d0 ((bool32 (__cdecl* )(LegoObject* liveObj, OUT Point2I* blockPos))0x0043a0d0)
//bool32 __cdecl LegoObject_UnkGetTerrainGetOutAtLandBlock_FUN_0043a0d0(LegoObject* liveObj, OUT Point2I* blockPos);

// <LegoRR.exe @0043a100>
#define LegoObject_CheckUnkGetInAtLand_FUN_0043a100 ((bool32 (__cdecl* )(LegoObject* liveObj, const Point2I* blockPos))0x0043a100)
//bool32 __cdecl LegoObject_CheckUnkGetInAtLand_FUN_0043a100(LegoObject* liveObj, const Point2I* blockPos);

// <LegoRR.exe @0043a130>
#define LegoObject_DropCarriedObject ((void (__cdecl* )(LegoObject* liveObj, bool32 putAway))0x0043a130)
//void __cdecl LegoObject_DropCarriedObject(LegoObject* liveObj, bool32 putAway);

// <LegoRR.exe @0043a3e0>
#define LegoObject_TryRequestOrDumpCarried ((bool32 (__cdecl* )(LegoObject* liveObj, const Point2I* blockPos, const Point2F* worldPos, bool32 place, bool32 setRouteFlag8))0x0043a3e0)
//bool32 __cdecl LegoObject_TryRequestOrDumpCarried(LegoObject* liveObj, const Point2I* blockPos, const Point2F* worldPos, bool32 place, bool32 setRouteFlag8);

// <LegoRR.exe @0043a5c0>
#define LegoObject_TryDepositCarried ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* destObj))0x0043a5c0)
//bool32 __cdecl LegoObject_TryDepositCarried(LegoObject* liveObj, LegoObject* destObj);

// <LegoRR.exe @0043a8b0>
#define LegoObject_GetDepositNull ((Gods98::Container* (__cdecl* )(LegoObject* liveObj))0x0043a8b0)
//Gods98::Container* __cdecl LegoObject_GetDepositNull(LegoObject* liveObj);

// <LegoRR.exe @0043a910>
#define LegoObject_SpawnCarryableObject ((LegoObject* (__cdecl* )(LegoObject* liveObj, LegoObject_Type objType, LegoObject_ID objID, sint32 objLevel))0x0043a910)
//LegoObject* __cdecl LegoObject_SpawnCarryableObject(LegoObject* liveObj, LegoObject_Type objType, LegoObject_ID objID, sint32 objLevel);

// <LegoRR.exe @0043aa80>
#define LegoObject_CanSpawnCarryableObject ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject_Type objType, LegoObject_ID objID))0x0043aa80)
//bool32 __cdecl LegoObject_CanSpawnCarryableObject(LegoObject* liveObj, LegoObject_Type objType, LegoObject_ID objID);

// <LegoRR.exe @0043ab10>
#define LegoObject_PutAwayCarriedObject ((void (__cdecl* )(LegoObject* liveObj, LegoObject* carriedObj))0x0043ab10)
//void __cdecl LegoObject_PutAwayCarriedObject(LegoObject* liveObj, LegoObject* carriedObj);

// <LegoRR.exe @0043abb0>
#define Level_AddCrystals__unusedLegoObject ((void (__cdecl* )(LegoObject* liveObj_unused, uint32 crystalCount))0x0043abb0)
//void __cdecl Level_AddCrystals__unusedLegoObject(LegoObject* liveObj_unused, uint32 crystalCount);

// <LegoRR.exe @0043abd0>
#define Level_AddOre__unusedLegoObject ((void (__cdecl* )(LegoObject* liveObj_unused, uint32 oreCount))0x0043abd0)
//void __cdecl Level_AddOre__unusedLegoObject(LegoObject* liveObj_unused, uint32 oreCount);

// <LegoRR.exe @0043abf0>
#define LegoObject_WaterVehicle_Unregister ((void (__cdecl* )(LegoObject* liveObj))0x0043abf0)
//void __cdecl LegoObject_WaterVehicle_Unregister(LegoObject* liveObj);

// <LegoRR.exe @0043ac20>
#define LegoObject_WaterVehicle_Register ((void (__cdecl* )(LegoObject* liveObj))0x0043ac20)
//void __cdecl LegoObject_WaterVehicle_Register(LegoObject* liveObj);

// <LegoRR.exe @0043aca0>
#define LegoObject_RegisterVehicle__callsForWater ((void (__cdecl* )(LegoObject* liveObj))0x0043aca0)
//void __cdecl LegoObject_RegisterVehicle__callsForWater(LegoObject* liveObj);

// <LegoRR.exe @0043acb0>
#define LegoObject_FUN_0043acb0 ((void (__cdecl* )(LegoObject* liveObj1, LegoObject* liveObj2))0x0043acb0)
//void __cdecl LegoObject_FUN_0043acb0(LegoObject* liveObj1, LegoObject* liveObj2);

// <LegoRR.exe @0043ad70>
#define LegoObject_RockMonster_FUN_0043ad70 ((void (__cdecl* )(LegoObject* liveObj))0x0043ad70)
//void __cdecl LegoObject_RockMonster_FUN_0043ad70(LegoObject* liveObj);

// <LegoRR.exe @0043aeb0>
#define LegoObject_FUN_0043aeb0 ((void (__cdecl* )(LegoObject* liveObj))0x0043aeb0)
//void __cdecl LegoObject_FUN_0043aeb0(LegoObject* liveObj);

// <LegoRR.exe @0043af50>
#define LegoObject_Callback_TryStampMiniFigureWithCrystal ((bool32 (__cdecl* )(LegoObject* targetObj, LegoObject* stamperObj))0x0043af50)
//bool32 __cdecl LegoObject_Callback_TryStampMiniFigureWithCrystal(LegoObject* targetObj, LegoObject* stamperObj);

// <LegoRR.exe @0043b010>
//#define LegoObject_TryGenerateSlug ((LegoObject* (__cdecl* )(LegoObject* originObj, LegoObject_ID objID))0x0043b010)
LegoObject* __cdecl LegoObject_TryGenerateSlug(LegoObject* originObj, LegoObject_ID objID);

/// CUSTOM: Generation for slug with a specific block pos already specified.
// Fails if `objType != LegoObject_RockMonster`.
LegoObject* LegoObject_TryGenerateSlugAtBlock(ObjectModel* objModel, LegoObject_Type objType, LegoObject_ID objID, uint32 bx, uint32 by, real32 heading, bool assignHeading);

// <LegoRR.exe @0043b160>
#define LegoObject_TryGenerateRMonsterAtRandomBlock ((LegoObject* (__cdecl* )(void))0x0043b160)
//LegoObject* __cdecl LegoObject_TryGenerateRMonsterAtRandomBlock(void);

// Fails if `objType != LegoObject_RockMonster`.
// <LegoRR.exe @0043b1f0>
#define LegoObject_TryGenerateRMonster ((LegoObject* (__cdecl* )(CreatureModel* objModel, LegoObject_Type objType, LegoObject_ID objID, uint32 bx, uint32 by))0x0043b1f0)
//LegoObject* __cdecl LegoObject_TryGenerateRMonster(CreatureModel* objModel, LegoObject_Type objType, LegoObject_ID objID, uint32 bx, uint32 by);

// <LegoRR.exe @0043b530>
//#define LegoObject_UpdateAll ((void (__cdecl* )(real32 elapsedGame))0x0043b530)
void __cdecl LegoObject_UpdateAll(real32 elapsedGame);

// <LegoRR.exe @0043b5e0>
//#define LegoObject_RemoveAll ((void (__cdecl* )(void))0x0043b5e0)
void __cdecl LegoObject_RemoveAll(void);

// RunThroughLists wrapper for LegoObject_Remove.
// <LegoRR.exe @0043b610>
//#define LegoObject_Callback_Remove ((bool32 (__cdecl* )(LegoObject* liveObj, void* unused))0x0043b610)
bool32 __cdecl LegoObject_Callback_Remove(LegoObject* liveObj, void* unused);

// <LegoRR.exe @0043b620>
#define LegoObject_DoPickSphereSelection ((bool32 (__cdecl* )(uint32 mouseX, uint32 mouseY, LegoObject** selectedObj))0x0043b620)
//bool32 __cdecl LegoObject_DoPickSphereSelection(uint32 mouseX, uint32 mouseY, LegoObject** selectedObj);

// <LegoRR.exe @0043b670>
#define LegoObject_Callback_PickSphereSelection ((bool32 (__cdecl* )(LegoObject* liveObj, SearchObjectMouseXY_c* search))0x0043b670)
//bool32 __cdecl LegoObject_Callback_PickSphereSelection(LegoObject* liveObj, SearchObjectMouseXY_c* search);

// <LegoRR.exe @0043b980>
#define LegoObject_DoDragSelection ((void (__cdecl* )(Gods98::Viewport* view, const Point2F* dragStart, const Point2F* dragEnd))0x0043b980)
//void __cdecl LegoObject_DoDragSelection(Gods98::Viewport* view, const Point2F* dragStart, const Point2F* dragEnd);

// <LegoRR.exe @0043ba30>
#define LegoObject_CallbackDoSelection ((bool32 (__cdecl* )(LegoObject* liveObj, SearchViewportWindow_14* search))0x0043ba30)
//bool32 __cdecl LegoObject_CallbackDoSelection(LegoObject* liveObj, SearchViewportWindow_14* search);

// <LegoRR.exe @0043bae0>
#define LegoObject_SwapPolyFP ((void (__cdecl* )(LegoObject* liveObj, uint32 cameraNo, bool32 on))0x0043bae0)
//void __cdecl LegoObject_SwapPolyFP(LegoObject* liveObj, uint32 cameraNo, bool32 on);

// if onCont is true, position of cont will be used in search.
// <LegoRR.exe @0043bb10>
#define LegoObject_FP_SetRanges ((void (__cdecl* )(LegoRR::LegoObject* liveObj, Gods98::Container* cont, real32 medPolyRange, real32 highPolyRange, bool32 onCont))0x0043bb10)
//void __cdecl LegoObject_FP_SetRanges(LegoRR::LegoObject* liveObj, Gods98::Container* cont, real32 medPolyRange, real32 highPolyRange, bool32 onCont);

// <LegoRR.exe @0043bb90>
#define LegoObject_FP_Callback_SwapPolyMeshParts ((bool32 (__cdecl* )(LegoObject* liveObj, LiveObjectInfo* liveInfo))0x0043bb90)
//bool32 __cdecl LegoObject_FP_Callback_SwapPolyMeshParts(LegoObject* liveObj, LiveObjectInfo* liveInfo);

// <LegoRR.exe @0043bdb0>
#define LegoObject_Check_LotsOfFlags1AndFlags2_FUN_0043bdb0 ((bool32 (__cdecl* )(LegoObject* liveObj))0x0043bdb0)
//bool32 __cdecl LegoObject_Check_LotsOfFlags1AndFlags2_FUN_0043bdb0(LegoObject* liveObj);

// <LegoRR.exe @0043bde0>
#define LegoObject_FUN_0043bde0 ((void (__cdecl* )(LegoObject* liveObj))0x0043bde0)
//void __cdecl LegoObject_FUN_0043bde0(LegoObject* liveObj);

// <LegoRR.exe @0043be80>
#define LegoObject_FinishEnteringWallHole ((void (__cdecl* )(LegoObject* liveObj))0x0043be80)
//void __cdecl LegoObject_FinishEnteringWallHole(LegoObject* liveObj);

// Called for both unit deaths from loss of health, and manual teleports to the LMS Explorer.
// Requires the objects health to be set to <= 0.0f beforehand.
// Requires that non-Building objects are not frozen, or in the middle of a teleport-down animation.
// <LegoRR.exe @0043bf00>
//#define LegoObject_UpdateRemoval ((void (__cdecl* )(LegoObject* liveObj))0x0043bf00)
void __cdecl LegoObject_UpdateRemoval(LegoObject* liveObj);

// <LegoRR.exe @0043c4c0>
#define LegoObject_CanSupportOxygenForType ((bool32 (__cdecl* )(LegoObject_Type consumerType, LegoObject_ID consumerID, LegoObject_Type producerType, LegoObject_ID producerID))0x0043c4c0)
//bool32 __cdecl LegoObject_CanSupportOxygenForType(LegoObject_Type consumerType, LegoObject_ID consumerID, LegoObject_Type producerType, LegoObject_ID producerID);

// <LegoRR.exe @0043c540>
#define LegoObject_Callback_SumOfOxygenCoefs ((bool32 (__cdecl* )(LegoObject* liveObj, real32* oxygenCoef))0x0043c540)
//bool32 __cdecl LegoObject_Callback_SumOfOxygenCoefs(LegoObject* liveObj, real32* oxygenCoef);

// DRAW MODE: Only Draw API drawing calls can be used within this function.
// <LegoRR.exe @0043c570>
//#define LegoObject_UpdateAllRadarSurvey ((void (__cdecl* )(real32 elapsedGame, bool32 inRadarMapView))0x0043c570)
void __cdecl LegoObject_UpdateAllRadarSurvey(real32 elapsedGame, bool32 inRadarMapView);

// DRAW MODE: Only Draw API drawing calls can be used within this function.
// DATA: bool32* inRadarMapView
// <LegoRR.exe @0043c5b0>
//#define LegoObject_Callback_UpdateRadarSurvey ((bool32 (__cdecl* )(LegoObject* liveObj, void* pInRadarMapView))0x0043c5b0)
bool32 __cdecl LegoObject_Callback_UpdateRadarSurvey(LegoObject* liveObj, void* pInRadarMapView);

// <LegoRR.exe @0043c6a0>
#define LegoObject_FUN_0043c6a0 ((bool32 (__cdecl* )(LegoObject* liveObj))0x0043c6a0)
//bool32 __cdecl LegoObject_FUN_0043c6a0(LegoObject* liveObj);

// <LegoRR.exe @0043c700>
#define LegoObject_GetEquippedBeam ((Weapon_KnownType (__cdecl* )(LegoObject* liveObj))0x0043c700)
//Weapon_KnownType __cdecl LegoObject_GetEquippedBeam(LegoObject* liveObj);

// Function cannot return true unless param_3 is non-zero.
// <LegoRR.exe @0043c750>
#define LegoObject_FUN_0043c750 ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* routeToObject, Weapon_KnownType knownWeapon))0x0043c750)
//bool32 __cdecl LegoObject_FUN_0043c750(LegoObject* liveObj, LegoObject* routeToObject, Weapon_KnownType knownWeapon);

// <LegoRR.exe @0043c780>
#define LegoObject_Proc_FUN_0043c780 ((void (__cdecl* )(LegoObject* liveObj))0x0043c780)
//void __cdecl LegoObject_Proc_FUN_0043c780(LegoObject* liveObj);

// <LegoRR.exe @0043c7f0>
#define LegoObject_Proc_FUN_0043c7f0 ((void (__cdecl* )(LegoObject* liveObj))0x0043c7f0)
//void __cdecl LegoObject_Proc_FUN_0043c7f0(LegoObject* liveObj);

// <LegoRR.exe @0043c830>
//#define LegoObject_UpdatePowerConsumption ((void (__cdecl* )(LegoObject* liveObj))0x0043c830)
void __cdecl LegoObject_UpdatePowerConsumption(LegoObject* liveObj);

// <LegoRR.exe @0043c910>
//#define LegoObject_CheckCanSteal ((bool32 (__cdecl* )(LegoObject* liveObj))0x0043c910)
bool32 __cdecl LegoObject_CheckCanSteal(LegoObject* liveObj);

// <LegoRR.exe @0043c970>
//#define LegoObject_UpdateElapsedTimes ((void (__cdecl* )(LegoObject* liveObj, real32 elapsed))0x0043c970)
void __cdecl LegoObject_UpdateElapsedTimes(LegoObject* liveObj, real32 elapsed);

// DATA: real32* pElapsed
// <LegoRR.exe @0043cad0>
#define LegoObject_Callback_Update ((bool32 (__cdecl* )(LegoObject* liveObj, real32* pElapsed))0x0043cad0)
//bool32 __cdecl LegoObject_Callback_Update(LegoObject* liveObj, void* pElapsed);

// <LegoRR.exe @0043f160>
//#define LegoObject_ProccessCarriedObjects ((void (__cdecl* )(LegoObject* liveObj))0x0043f160)
void __cdecl LegoObject_ProccessCarriedObjects(LegoObject* liveObj);

// <LegoRR.exe @0043f3c0>
//#define LegoObject_ClearDockOccupiedFlag ((void (__cdecl* )(LegoObject* unused_liveObj, LegoObject* liveObj))0x0043f3c0)
void __cdecl LegoObject_ClearDockOccupiedFlag(LegoObject* unused_liveObj, LegoObject* liveObj);

// <LegoRR.exe @0043f3f0>
//#define LegoObject_TriggerFrameCallback ((void (__cdecl* )(Gods98::Container* cont, void* data))0x0043f3f0)
void __cdecl LegoObject_TriggerFrameCallback(Gods98::Container* cont, void* data);

// <LegoRR.exe @0043f410>
//#define LegoObject_QueueTeleport ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject_Type objType, LegoObject_ID objID))0x0043f410)
bool32 __cdecl LegoObject_QueueTeleport(LegoObject* liveObj, LegoObject_Type objType, LegoObject_ID objID);

// <LegoRR.exe @0043f450>
//#define LegoObject_UpdateTeleporter ((void (__cdecl* )(LegoObject* liveObj))0x0043f450)
void __cdecl LegoObject_UpdateTeleporter(LegoObject* liveObj);

// Removes the first teleport-down reference that matches the specified object.
// <LegoRR.exe @0043f820>
//#define LegoObject_RemoveTeleportDownReference ((bool32 (__cdecl* )(LegoObject* teleportDownObj))0x0043f820)
bool32 __cdecl LegoObject_RemoveTeleportDownReference(LegoObject* teleportDownObj);

// Removes the teleport-down reference if it matches the specified object. Returns true on match.
// DATA: LegoObject* teleportDownObj
// <LegoRR.exe @0043f840>
//#define LegoObject_Callback_RemoveTeleportDownReference ((bool32 (__cdecl* )(LegoObject* liveObj, void* teleportDownObj))0x0043f840)
bool32 __cdecl LegoObject_Callback_RemoveTeleportDownReference(LegoObject* liveObj, void* pTeleportDownObj);

// <LegoRR.exe @0043f870>
//#define LegoObject_TrainMiniFigure_instantunk ((void (__cdecl* )(LegoObject* liveObj, LegoObject_AbilityFlags trainFlags))0x0043f870)
void __cdecl LegoObject_TrainMiniFigure_instantunk(LegoObject* liveObj, LegoObject_AbilityFlags trainFlags);

// <LegoRR.exe @0043f960>
//#define LegoObject_AddDamage2 ((void (__cdecl* )(LegoObject* liveObj, real32 damage, bool32 showVisual, real32 elapsed))0x0043f960)
void __cdecl LegoObject_AddDamage2(LegoObject* liveObj, real32 damage, bool32 showVisual, real32 elapsed);

// <LegoRR.exe @0043fa90>
//#define LegoObject_UpdateEnergyHealthAndLavaContact ((void (__cdecl* )(LegoObject* liveObj, real32 elapsed))0x0043fa90)
void __cdecl LegoObject_UpdateEnergyHealthAndLavaContact(LegoObject* liveObj, real32 elapsed);

// <LegoRR.exe @0043fe00>
//#define LegoObject_MiniFigurePlayHurtSound ((bool32 (__cdecl* )(LegoObject* liveObj, real32 elapsed, real32 damage))0x0043fe00)
bool32 __cdecl LegoObject_MiniFigurePlayHurtSound(LegoObject* liveObj, real32 elapsed, real32 damage);

// <LegoRR.exe @0043fee0>
#define LegoObject_FUN_0043fee0 ((bool32 (__cdecl* )(LegoObject* carriedObj))0x0043fee0)
//bool32 __cdecl LegoObject_FUN_0043fee0(LegoObject* carriedObj);

// <LegoRR.exe @00440080>
#define LegoObject_UnkCarryingVehicle_FUN_00440080 ((bool32 (__cdecl* )(LegoObject* liveObj))0x00440080)
//bool32 __cdecl LegoObject_UnkCarryingVehicle_FUN_00440080(LegoObject* liveObj);

// <LegoRR.exe @00440130>
#define LegoObject_TryFindLoad_FUN_00440130 ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* targetObj))0x00440130)
//bool32 __cdecl LegoObject_TryFindLoad_FUN_00440130(LegoObject* liveObj, LegoObject* targetObj);

// <LegoRR.exe @004402b0>
#define LegoObject_TryDock_FUN_004402b0 ((bool32 (__cdecl* )(LegoObject* liveObj))0x004402b0)
//bool32 __cdecl LegoObject_TryDock_FUN_004402b0(LegoObject* liveObj);

// <LegoRR.exe @004403f0>
#define LegoObject_TryDock_AtBlockPos_FUN_004403f0 ((void (__cdecl* )(LegoObject* liveObj, const Point2I* blockPos))0x004403f0)
//void __cdecl LegoObject_TryDock_AtBlockPos_FUN_004403f0(LegoObject* liveObj, const Point2I* blockPos);

// <LegoRR.exe @00440470>
#define LegoObject_FUN_00440470 ((bool32 (__cdecl* )(LegoObject* liveObj, bool32 param_2))0x00440470)
//bool32 __cdecl LegoObject_FUN_00440470(LegoObject* liveObj, bool32 param_2);

// <LegoRR.exe @00440690>
#define LegoObject_TryFindDriver_FUN_00440690 ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* drivableObj))0x00440690)
//bool32 __cdecl LegoObject_TryFindDriver_FUN_00440690(LegoObject* liveObj, LegoObject* drivableObj);

// <LegoRR.exe @00440a70>
#define LegoObject_DoDynamiteExplosionRadiusCallbacks ((void (__cdecl* )(LegoObject* liveObj, real32 damageRadius, real32 maxDamage, real32 wakeRadius))0x00440a70)
//void __cdecl LegoObject_DoDynamiteExplosionRadiusCallbacks(LegoObject* liveObj, real32 damageRadius, real32 maxDamage, real32 wakeRadius);

// <LegoRR.exe @00440ac0>
#define LegoObject_Callback_DynamiteExplosion ((bool32 (__cdecl* )(LegoObject* liveObj, SearchDynamiteRadius* search))0x00440ac0)
//bool32 __cdecl LegoObject_Callback_DynamiteExplosion(LegoObject* liveObj, SearchDynamiteRadius* search);

// <LegoRR.exe @00440b80>
#define LegoObject_DoBirdScarerRadiusCallbacks ((void (__cdecl* )(OPTIONAL LegoObject* liveObj, OPTIONAL const Point2F* position, real32 radius))0x00440b80)
//void __cdecl LegoObject_DoBirdScarerRadiusCallbacks(OPTIONAL LegoObject* liveObj, OPTIONAL const Point2F* position, real32 radius);

// DATA: SearchDynamiteRadius*
// <LegoRR.exe @00440be0>
#define LegoObject_Callback_BirdScarer ((bool32 (__cdecl* )(LegoObject* liveObj, SearchDynamiteRadius* search))0x00440be0)
//bool32 __cdecl LegoObject_Callback_BirdScarer(LegoObject* liveObj, SearchDynamiteRadius* search);

// <LegoRR.exe @00440ca0>
//#define LegoObject_SetActivity ((void (__cdecl* )(LegoObject* liveObj, Activity_Type activityType, uint32 repeatCount))0x00440ca0)
void __cdecl LegoObject_SetActivity(LegoObject* liveObj, Activity_Type activityType, uint32 repeatCount);

// <LegoRR.exe @00440cd0>
#define LegoObject_UpdateCarrying ((void (__cdecl* )(LegoObject* liveObj))0x00440cd0)
//void __cdecl LegoObject_UpdateCarrying(LegoObject* liveObj);

// <LegoRR.exe @00440eb0>
#define LegoObject_InitBoulderMesh_FUN_00440eb0 ((void (__cdecl* )(LegoObject* liveObj, Gods98::Container_Texture* contTexture))0x00440eb0)
//void __cdecl LegoObject_InitBoulderMesh_FUN_00440eb0(LegoObject* liveObj, Gods98::Container_Texture* contTexture);

// <LegoRR.exe @00440ef0>
#define LegoObject_Route_ScoreNoCallback_FUN_00440ef0 ((bool32 (__cdecl* )(LegoObject* liveObj, uint32 bx, uint32 by, uint32 bx2, uint32 by2, sint32** out_param_6, sint32** out_param_7, sint32* out_count))0x00440ef0)
//bool32 __cdecl LegoObject_Route_ScoreNoCallback_FUN_00440ef0(LegoObject* liveObj, uint32 bx, uint32 by, uint32 bx2, uint32 by2, sint32** out_param_6, sint32** out_param_7, sint32* out_count);

// <LegoRR.exe @00440f30>
#define LegoObject_Route_ScoreSub_FUN_00440f30 ((bool32 (__cdecl* )(LegoObject* liveObj, uint32 bx, uint32 by, uint32 bx2, uint32 by2, uint32** out_param_6, uint32** out_param_7, uint32* out_count, undefined* callback, void* data))0x00440f30)
//bool32 __cdecl LegoObject_Route_ScoreSub_FUN_00440f30(LegoObject* liveObj, uint32 bx, uint32 by, uint32 bx2, uint32 by2, uint32** out_param_6, uint32** out_param_7, uint32* out_count, undefined* callback, void* data);

// <LegoRR.exe @004413b0>
#define LegoObject_Route_Score_FUN_004413b0 ((bool32 (__cdecl* )(LegoObject* liveObj, uint32 bx, uint32 by, uint32 bx2, uint32 by2, sint32** out_new_bxs, sint32** out_new_bys, sint32* out_count, void* callback, void* data))0x004413b0)
//bool32 __cdecl LegoObject_Route_Score_FUN_004413b0(LegoObject* liveObj, uint32 bx, uint32 by, uint32 bx2, uint32 by2, sint32** out_new_bxs, sint32** out_new_bys, sint32* out_count, void* callback, void* data);

// <LegoRR.exe @004419c0>
#define LegoObject_Route_AllocPtr_FUN_004419c0 ((bool32 (__cdecl* )(LegoObject* liveObj, uint32 count, const sint32* bxList, const sint32* byList, OPTIONAL const Point2F* point))0x004419c0)
//bool32 __cdecl LegoObject_Route_AllocPtr_FUN_004419c0(LegoObject* liveObj, uint32 count, const sint32* bxList, const sint32* byList, OPTIONAL const Point2F* point);

// <LegoRR.exe @00441c00>
#define LegoObject_Route_End ((void (__cdecl* )(LegoObject* liveObj, bool32 completed))0x00441c00)
//void __cdecl LegoObject_Route_End(LegoObject* liveObj, bool32 completed);

// <LegoRR.exe @00441df0>
#define LegoObject_Interrupt ((void (__cdecl* )(LegoObject* liveObj, bool32 actStand, bool32 dropCarried))0x00441df0)
//void __cdecl LegoObject_Interrupt(LegoObject* liveObj, bool32 actStand, bool32 dropCarried);

// <LegoRR.exe @00442160>
#define LegoObject_DestroyBoulder_AndCreateExplode ((void (__cdecl* )(LegoObject* liveObj))0x00442160)
//void __cdecl LegoObject_DestroyBoulder_AndCreateExplode(LegoObject* liveObj);

// <LegoRR.exe @00442190>
#define LegoObject_FireBeamWeaponAtObject ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* targetObj, Weapon_KnownType knownWeapon))0x00442190)
//bool32 __cdecl LegoObject_FireBeamWeaponAtObject(LegoObject* liveObj, LegoObject* targetObj, Weapon_KnownType knownWeapon);

// <LegoRR.exe @00442390>
#define LegoObject_CreateWeaponProjectile ((void (__cdecl* )(LegoObject* liveObj, Weapon_KnownType knownWeapon))0x00442390)
//void __cdecl LegoObject_CreateWeaponProjectile(LegoObject* liveObj, Weapon_KnownType knownWeapon);

// <LegoRR.exe @004424d0>
//#define LegoObject_StartCrumbling ((void (__cdecl* )(LegoObject* liveObj))0x004424d0)
void __cdecl LegoObject_StartCrumbling(LegoObject* liveObj);

// <LegoRR.exe @00442520>
//#define LegoObject_GetPosition ((void (__cdecl* )(LegoObject* liveObj, OUT real32* xPos, OUT real32* yPos))0x00442520)
void __cdecl LegoObject_GetPosition(LegoObject* liveObj, OUT real32* xPos, OUT real32* yPos);

// <LegoRR.exe @00442560>
//#define LegoObject_GetFaceDirection ((void (__cdecl* )(LegoObject* liveObj, OUT Point2F* dir2D))0x00442560)
void __cdecl LegoObject_GetFaceDirection(LegoObject* liveObj, OUT Point2F* dir2D);

// <LegoRR.exe @004425c0>
//#define LegoObject_SetHeadingOrDirection ((void (__cdecl* )(LegoObject* liveObj, real32 theta, OPTIONAL const Vector3F* dir))0x004425c0)
void __cdecl LegoObject_SetHeadingOrDirection(LegoObject* liveObj, real32 theta, OPTIONAL const Vector3F* dir);

// <LegoRR.exe @00442740>
//#define LegoObject_GetHeading ((real32 (__cdecl* )(LegoObject* liveObj))0x00442740)
real32 __cdecl LegoObject_GetHeading(LegoObject* liveObj);

// <LegoRR.exe @004427b0>
//#define LegoObject_GetBlockPos ((bool32 (__cdecl* )(LegoObject* liveObj, OUT sint32* bx, OUT sint32* by))0x004427b0)
bool32 __cdecl LegoObject_GetBlockPos(LegoObject* liveObj, OUT sint32* bx, OUT sint32* by);

// <LegoRR.exe @00442800>
//#define LegoObject_GetWorldZCallback ((real32 (__cdecl* )(real32 xPos, real32 yPos, Map3D* map))0x00442800)
real32 __cdecl LegoObject_GetWorldZCallback(real32 xPos, real32 yPos, Map3D* map);

// The same as `LegoObject_GetWorldZCallback`, but returns a lower Z value with over Lake terrain.
// Objects wading in a lake (aka, not sailing) will have their Z lowered a bit, and have it at the lowest near the center of a lake BLOCK.
// <LegoRR.exe @00442820>
//#define LegoObject_GetWorldZCallback_Lake ((real32 (__cdecl* )(real32 xPos, real32 yPos, Map3D* map))0x00442820)
real32 __cdecl LegoObject_GetWorldZCallback_Lake(real32 xPos, real32 yPos, Map3D* map);

// <LegoRR.exe @004428b0>
//#define LegoObject_UpdateRoutingVectors ((void (__cdecl* )(LegoObject* liveObj, real32 xPos, real32 yPos))0x004428b0)
void __cdecl LegoObject_UpdateRoutingVectors(LegoObject* liveObj, real32 xPos, real32 yPos);

// <LegoRR.exe @00442b60>
//#define LegoObject_SetPositionAndHeading ((void (__cdecl* )(LegoObject* liveObj, real32 xPos, real32 yPos, real32 theta, bool32 assignHeading))0x00442b60)
void __cdecl LegoObject_SetPositionAndHeading(LegoObject* liveObj, real32 xPos, real32 yPos, real32 theta, bool32 assignHeading);

// <LegoRR.exe @00442dd0>
#define LegoObject_FP_UpdateMovement ((sint32 (__cdecl* )(LegoObject* liveObj, real32 elapsed, OUT real32* transSpeed))0x00442dd0)
//sint32 __cdecl LegoObject_FP_UpdateMovement(LegoObject* liveObj, real32 elapsed, OUT real32* transSpeed);

// Recalculates the "sticky" position (and sometimes orientation) of an object based on the world.
// This refers to behaviours that have objects snap to certain planes and face certain directions.
// Example: All units' Z positions try to match the world's surface Z position.
// <LegoRR.exe @00443240>
#define LegoObject_UpdateWorldStickyPosition ((void (__cdecl* )(LegoObject* liveObj, real32 elapsed))0x00443240)
//void __cdecl LegoObject_UpdateWorldStickyPosition(LegoObject* liveObj, real32 elapsed);

// Updates the sticky position/orientation of a vehicle's driver to match that of the vehicle.
// REQUIRES: liveObj->vehicle != NULL and liveObj->driveObject != NULL
// <LegoRR.exe @004437d0>
#define LegoObject_UpdateDriverStickyPosition ((void (__cdecl* )(LegoObject* drivenVehicleObj))0x004437d0)
//void __cdecl LegoObject_UpdateDriverStickyPosition(LegoObject* drivenVehicleObj);

// <LegoRR.exe @00443930>
#define LegoObject_TryWaiting ((bool32 (__cdecl* )(LegoObject* liveObj))0x00443930)
//bool32 __cdecl LegoObject_TryWaiting(LegoObject* liveObj);

// <LegoRR.exe @004439b0>
#define LegoObject_IsRockMonsterCanGather ((bool32 (__cdecl* )(LegoObject* liveObj))0x004439b0)
//bool32 __cdecl LegoObject_IsRockMonsterCanGather(LegoObject* liveObj);

// <LegoRR.exe @004439d0>
#define LegoObject_FUN_004439d0 ((bool32 (__cdecl* )(LegoObject* liveObj, const Point2I* blockPos, OUT Point2I* out_blockPos, undefined4 unused))0x004439d0)
//bool32 __cdecl LegoObject_FUN_004439d0(LegoObject* liveObj, const Point2I* blockPos, OUT Point2I* out_blockPos, undefined4 unused);

// <LegoRR.exe @00443ab0>
#define LegoObject_RockMonster_DoWakeUp ((void (__cdecl* )(LegoObject* liveObj))0x00443ab0)
//void __cdecl LegoObject_RockMonster_DoWakeUp(LegoObject* liveObj);

// <LegoRR.exe @00443b00>
#define LegoObject_CheckBlock_FUN_00443b00 ((bool32 (__cdecl* )(LegoObject* liveObj, const Point2I* blockPos, bool32* pAllowWall))0x00443b00)
//bool32 __cdecl LegoObject_CheckBlock_FUN_00443b00(LegoObject* liveObj, const Point2I* blockPos, bool32* pAllowWall);

// <LegoRR.exe @00443b70>
#define LegoObject_Route_UpdateMovement ((void (__cdecl* )(LegoObject* liveObj, real32 elapsed))0x00443b70)
//void __cdecl LegoObject_Route_UpdateMovement(LegoObject* liveObj, real32 elapsed);

// <LegoRR.exe @00444360>
#define LegoObject_TryDock_AtObject2FC ((void (__cdecl* )(LegoObject* liveObj))0x00444360)
//void __cdecl LegoObject_TryDock_AtObject2FC(LegoObject* liveObj);

// <LegoRR.exe @004443b0>
#define LegoObject_UpdateCarryingEnergy ((void (__cdecl* )(LegoObject* liveObj, real32 elapsed))0x004443b0)
//void __cdecl LegoObject_UpdateCarryingEnergy(LegoObject* liveObj, real32 elapsed);

// <LegoRR.exe @00444520>
#define LegoObject_FUN_00444520 ((bool32 (__cdecl* )(LegoObject* liveObj))0x00444520)
//bool32 __cdecl LegoObject_FUN_00444520(LegoObject* liveObj);

// <LegoRR.exe @00444720>
//#define LegoObject_TryRunAway ((void (__cdecl* )(LegoObject* liveObj, const Point2F* dir))0x00444720)
void __cdecl LegoObject_TryRunAway(LegoObject* liveObj, const Point2F* dir);

// <LegoRR.exe @004448e0>
#define LegoObject_DoSlip ((void (__cdecl* )(LegoObject* liveObj))0x004448e0)
//void __cdecl LegoObject_DoSlip(LegoObject* liveObj);

// <LegoRR.exe @00444940>
#define LegoObject_RoutingUnk_FUN_00444940 ((bool32 (__cdecl* )(LegoObject* liveObj, bool32 useRoutingPos, bool32 flags3_8, bool32 notFlags1_10000))0x00444940)
//bool32 __cdecl LegoObject_RoutingUnk_FUN_00444940(LegoObject* liveObj, bool32 useRoutingPos, bool32 flags3_8, bool32 notFlags1_10000);

// <LegoRR.exe @00445270>
#define LegoObject_FaceTowardsCamera ((void (__cdecl* )(LegoObject* liveObj, const Point2F* newFaceDir))0x00445270)
//void __cdecl LegoObject_FaceTowardsCamera(LegoObject* liveObj, const Point2F* newFaceDir);

// <LegoRR.exe @004454a0>
#define LegoObject_Route_CurveSolid_FUN_004454a0 ((void (__cdecl* )(LegoObject* liveObj))0x004454a0)
//void __cdecl LegoObject_Route_CurveSolid_FUN_004454a0(LegoObject* liveObj);

// DATA: LegoObject** pOtherObj
// <LegoRR.exe @00445600>
#define LegoObject_Callback_CurveSolidCollRadius_FUN_00445600 ((bool32 (__cdecl* )(LegoObject* liveObj1, void* pOtherObj))0x00445600)
//bool32 __cdecl LegoObject_Callback_CurveSolidCollRadius_FUN_00445600(LegoObject* liveObj1, void* pOtherObj);

// <LegoRR.exe @00445860>
#define LegoObject_Route_SolidBlockCheck_FUN_00445860 ((bool32 (__cdecl* )(LegoObject* liveObj))0x00445860)
//bool32 __cdecl LegoObject_Route_SolidBlockCheck_FUN_00445860(LegoObject* liveObj);

// Update scaring?
// <LegoRR.exe @004459a0>
//#define LegoObject_UpdateSlipAndScare ((void (__cdecl* )(LegoObject* liveObj, real32 elapsed))0x004459a0)
void __cdecl LegoObject_UpdateSlipAndScare(LegoObject* liveObj, real32 elapsed);

// Only mini-figure units trained to use explosives are effected.
// <LegoRR.exe @00445a30>
//#define LegoObject_Callback_ScareTrainedMiniFiguresAwayFromTickingDynamite ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* otherObj))0x00445a30)
bool32 __cdecl LegoObject_Callback_ScareTrainedMiniFiguresAwayFromTickingDynamite(LegoObject* liveObj, void* pOtherObj);

// <LegoRR.exe @00445af0>
//#define LegoObject_Callback_SlipAndScare ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* otherObj))0x00445af0)
bool32 __cdecl LegoObject_Callback_SlipAndScare(LegoObject* liveObj, void* pOtherObj);

// <LegoRR.exe @00446030>
#define LegoObject_DoCollisionCallbacks_FUN_00446030 ((LegoObject* (__cdecl* )(LegoObject* liveObj, const Point2F* param_2, real32 param_3, bool32 param_4))0x00446030)
//LegoObject* __cdecl LegoObject_DoCollisionCallbacks_FUN_00446030(LegoObject* liveObj, const Point2F* param_2, real32 param_3, bool32 param_4);

// Data: sint32* (unsure)
// <LegoRR.exe @004460b0>
#define LegoObject_CallbackCollisionRadius_FUN_004460b0 ((bool32 (__cdecl* )(LegoObject* liveObj, void* search))0x004460b0)
//bool32 __cdecl LegoObject_CallbackCollisionRadius_FUN_004460b0(LegoObject* liveObj, void* search);

// DATA: SearchCollision_14*
// <LegoRR.exe @004463b0>
#define LegoObject_CallbackCollisionBox_FUN_004463b0 ((bool32 (__cdecl* )(LegoObject* liveObj, void* search))0x004463b0)
//bool32 __cdecl LegoObject_CallbackCollisionBox_FUN_004463b0(LegoObject* liveObj, void* search);

// <LegoRR.exe @004468d0>
#define LegoObject_CalculateSpeeds ((void (__cdecl* )(LegoObject* liveObj, real32 elapsed, OUT real32* routeSpeed, OUT real32* transSpeed))0x004468d0)
//void __cdecl LegoObject_CalculateSpeeds(LegoObject* liveObj, real32 elapsed, OUT real32* routeSpeed, OUT real32* transSpeed);

// <LegoRR.exe @00446b80>
#define LegoObject_RoutingPtr_Realloc_FUN_00446b80 ((bool32 (__cdecl* )(LegoObject* liveObj, uint32 bx, uint32 by))0x00446b80)
//bool32 __cdecl LegoObject_RoutingPtr_Realloc_FUN_00446b80(LegoObject* liveObj, uint32 bx, uint32 by);

// <LegoRR.exe @00446c80>
#define LegoObject_BlockRoute_FUN_00446c80 ((bool32 (__cdecl* )(LegoObject* liveObj, uint32 bx, uint32 by, bool32 useWideRange, OPTIONAL OUT Direction* direction, bool32 countIs8))0x00446c80)
//bool32 __cdecl LegoObject_BlockRoute_FUN_00446c80(LegoObject* liveObj, uint32 bx, uint32 by, bool32 useWideRange, OPTIONAL OUT Direction* direction, bool32 countIs8);

// <LegoRR.exe @00447100>
#define LegoObject_RouteToDig_FUN_00447100 ((bool32 (__cdecl* )(LegoObject* liveObj, uint32 bx, uint32 by, bool32 tunnelDig))0x00447100)
//bool32 __cdecl LegoObject_RouteToDig_FUN_00447100(LegoObject* liveObj, uint32 bx, uint32 by, bool32 tunnelDig);

// <LegoRR.exe @00447390>
#define LegoObject_PTL_GatherRock ((bool32 (__cdecl* )(LegoObject* liveObj))0x00447390)
//bool32 __cdecl LegoObject_PTL_GatherRock(LegoObject* liveObj);

// <LegoRR.exe @00447470>
#define LegoObject_RoutingNoCarry_FUN_00447470 ((bool32 (__cdecl* )(LegoObject* liveObj, uint32 bx, uint32 by, LegoObject* boulderObj))0x00447470)
//bool32 __cdecl LegoObject_RoutingNoCarry_FUN_00447470(LegoObject* liveObj, uint32 bx, uint32 by, LegoObject* boulderObj);

// <LegoRR.exe @004474d0>
#define LegoObject_PTL_AttackBuilding ((bool32 (__cdecl* )(LegoObject* liveObj1, LegoObject* targetObj))0x004474d0)
//bool32 __cdecl LegoObject_PTL_AttackBuilding(LegoObject* liveObj1, LegoObject* targetObj);

// <LegoRR.exe @00447670>
#define LegoObject_FUN_00447670 ((sint32 (__cdecl* )(LegoObject* liveObj, sint32 bx, sint32 by, LegoObject* liveObj2))0x00447670)
//sint32 __cdecl LegoObject_FUN_00447670(LegoObject* liveObj, sint32 bx, sint32 by, LegoObject* liveObj2);

// <LegoRR.exe @004477b0>
#define LegoObject_FUN_004477b0 ((void (__cdecl* )(LegoObject* liveObj))0x004477b0)
//void __cdecl LegoObject_FUN_004477b0(LegoObject* liveObj);

// <LegoRR.exe @00447880>
#define LegoObject_FUN_00447880 ((sint32 (__cdecl* )(LegoObject* liveObj))0x00447880)
//sint32 __cdecl LegoObject_FUN_00447880(LegoObject* liveObj);

// <LegoRR.exe @004479f0>
#define LegoObject_Add25EnergyAndSetHealth ((bool32 (__cdecl* )(LegoObject* liveObj))0x004479f0)
//bool32 __cdecl LegoObject_Add25EnergyAndSetHealth(LegoObject* liveObj);

// <LegoRR.exe @00447a40>
#define LegoObject_FUN_00447a40 ((void (__cdecl* )(LegoObject* liveObj))0x00447a40)
//void __cdecl LegoObject_FUN_00447a40(LegoObject* liveObj);

// <LegoRR.exe @00447a90>
#define LegoObject_FUN_00447a90 ((void (__cdecl* )(LegoObject* liveObj))0x00447a90)
//void __cdecl LegoObject_FUN_00447a90(LegoObject* liveObj);

// <LegoRR.exe @00447bc0>
#define LegoObject_DoBuildingsCallback_AttackByBoulder ((void (__cdecl* )(LegoObject* liveObj))0x00447bc0)
//void __cdecl LegoObject_DoBuildingsCallback_AttackByBoulder(LegoObject* liveObj);

// <LegoRR.exe @00447be0>
#define LegoObject_CallbackBoulderAttackBuilding_FUN_00447be0 ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* buildingLiveObj))0x00447be0)
//bool32 __cdecl LegoObject_CallbackBoulderAttackBuilding_FUN_00447be0(LegoObject* liveObj, LegoObject* buildingLiveObj);

// <LegoRR.exe @00447c10>
#define LegoObject_Hit ((void (__cdecl* )(LegoObject* liveObj, OPTIONAL const Point2F* dir, bool32 reactToHit))0x00447c10)
//void __cdecl LegoObject_Hit(LegoObject* liveObj, OPTIONAL const Point2F* dir, bool32 reactToHit);

// <LegoRR.exe @00447dc0>
#define LegoObject_TeleportDownBuilding ((void (__cdecl* )(LegoObject* liveObj))0x00447dc0)
//void __cdecl LegoObject_TeleportDownBuilding(LegoObject* liveObj);

// <LegoRR.exe @00447df0>
#define LegoObject_MoveAnimation ((real32 (__cdecl* )(LegoObject* liveObj, real32 elapsed))0x00447df0)
//real32 __cdecl LegoObject_MoveAnimation(LegoObject* liveObj, real32 elapsed);

// <LegoRR.exe @00447f00>
//#define LegoObject_UpdateActivityChange ((bool32 (__cdecl* )(LegoObject* liveObj))0x00447f00)
bool32 __cdecl LegoObject_UpdateActivityChange(LegoObject* liveObj);

// <LegoRR.exe @00448160>
#define LegoObject_SimpleObject_FUN_00448160 ((void (__cdecl* )(LegoObject* liveObj, real32 elapsed))0x00448160)
//void __cdecl LegoObject_SimpleObject_FUN_00448160(LegoObject* liveObj, real32 elapsed);

// <LegoRR.exe @00448a80>
#define LegoObject_Debug_DropActivateDynamite ((void (__cdecl* )(LegoObject* liveObj))0x00448a80)
//void __cdecl LegoObject_Debug_DropActivateDynamite(LegoObject* liveObj);

// <LegoRR.exe @00448ac0>
#define LegoObject_TryDynamite_FUN_00448ac0 ((bool32 (__cdecl* )(LegoObject* liveObj, const Point2I* blockPos))0x00448ac0)
//bool32 __cdecl LegoObject_TryDynamite_FUN_00448ac0(LegoObject* liveObj, const Point2I* blockPos);

// <LegoRR.exe @00448b40>
#define LegoObject_PlaceCarriedBirdScarerAt ((bool32 (__cdecl* )(LegoObject* liveObj, const Point2I* blockPos))0x00448b40)
//bool32 __cdecl LegoObject_PlaceCarriedBirdScarerAt(LegoObject* liveObj, const Point2I* blockPos);

// <LegoRR.exe @00448c60>
#define LegoObject_PlaceBirdScarer_AndTickDown ((bool32 (__cdecl* )(LegoObject* liveObj))0x00448c60)
//bool32 __cdecl LegoObject_PlaceBirdScarer_AndTickDown(LegoObject* liveObj);

// <LegoRR.exe @00448d20>
#define LegoObject_TryElecFence_FUN_00448d20 ((bool32 (__cdecl* )(LegoObject* liveObj, const Point2I* blockPos))0x00448d20)
//bool32 __cdecl LegoObject_TryElecFence_FUN_00448d20(LegoObject* liveObj, const Point2I* blockPos);

// <LegoRR.exe @00448f10>
#define LegoObject_TryBuildPath_FUN_00448f10 ((bool32 (__cdecl* )(LegoObject* liveObj))0x00448f10)
//bool32 __cdecl LegoObject_TryBuildPath_FUN_00448f10(LegoObject* liveObj);

// <LegoRR.exe @00448f50>
#define LegoObject_TryUpgrade_FUN_00448f50 ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* targetObj, sint32 targetObjLevel))0x00448f50)
//bool32 __cdecl LegoObject_TryUpgrade_FUN_00448f50(LegoObject* liveObj, LegoObject* targetObj, sint32 targetObjLevel);

// <LegoRR.exe @00449170>
#define LegoObject_TryTrain_FUN_00449170 ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* targetObj, bool32 set_0xE_or0xF))0x00449170)
//bool32 __cdecl LegoObject_TryTrain_FUN_00449170(LegoObject* liveObj, LegoObject* targetObj, bool32 set_0xE_or0xF);

// <LegoRR.exe @004492d0>
#define LegoObject_TryRechargeCarried ((bool32 (__cdecl* )(LegoObject* liveObj))0x004492d0)
//bool32 __cdecl LegoObject_TryRechargeCarried(LegoObject* liveObj);

// <LegoRR.exe @00449360>
#define LegoObject_TryRepairDrainObject ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* targetObj, bool32 setRouteFlag20, bool32 setLive4Flag400000))0x00449360)
//bool32 __cdecl LegoObject_TryRepairDrainObject(LegoObject* liveObj, LegoObject* targetObj, bool32 setRouteFlag20, bool32 setLive4Flag400000);

// <LegoRR.exe @00449500>
#define LegoObject_TryReinforceBlock ((bool32 (__cdecl* )(LegoObject* liveObj, const Point2I* blockPos))0x00449500)
//bool32 __cdecl LegoObject_TryReinforceBlock(LegoObject* liveObj, const Point2I* blockPos);

// <LegoRR.exe @00449570>
#define LegoObject_TryClear_FUN_00449570 ((bool32 (__cdecl* )(LegoObject* liveObj, const Point2I* blockPos))0x00449570)
//bool32 __cdecl LegoObject_TryClear_FUN_00449570(LegoObject* liveObj, const Point2I* blockPos);

// <LegoRR.exe @004496a0>
#define LegoObject_MiniFigureHasBeamEquipped ((bool32 (__cdecl* )(LegoObject* liveObj))0x004496a0)
//bool32 __cdecl LegoObject_MiniFigureHasBeamEquipped(LegoObject* liveObj);

// <LegoRR.exe @004496f0>
#define LegoObject_TryAttackRockMonster_FUN_004496f0 ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* targetObj))0x004496f0)
//bool32 __cdecl LegoObject_TryAttackRockMonster_FUN_004496f0(LegoObject* liveObj, LegoObject* targetObj);

// <LegoRR.exe @004497e0>
#define LegoObject_TryAttackObject_FUN_004497e0 ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject* targetObj))0x004497e0)
//bool32 __cdecl LegoObject_TryAttackObject_FUN_004497e0(LegoObject* liveObj, LegoObject* targetObj);

// <LegoRR.exe @004498d0>
#define LegoObject_TryAttackPath_FUN_004498d0 ((bool32 (__cdecl* )(LegoObject* liveObj, const Point2I* blockPos))0x004498d0)
//bool32 __cdecl LegoObject_TryAttackPath_FUN_004498d0(LegoObject* liveObj, const Point2I* blockPos);

// <LegoRR.exe @004499c0>
#define LegoObject_TryDepart_FUN_004499c0 ((bool32 (__cdecl* )(LegoObject* liveObj))0x004499c0)
//bool32 __cdecl LegoObject_TryDepart_FUN_004499c0(LegoObject* liveObj);

// <LegoRR.exe @00449b40>
#define LegoObject_RouteToFaceBlock ((bool32 (__cdecl* )(LegoObject* liveObj, uint32 bx, uint32 by, real32 distFrom))0x00449b40)
//bool32 __cdecl LegoObject_RouteToFaceBlock(LegoObject* liveObj, uint32 bx, uint32 by, real32 distFrom);

// <LegoRR.exe @00449c40>
#define LegoObject_Update_Reinforcing ((bool32 (__cdecl* )(LegoObject* liveObj, real32 elapsed, OUT bool32* finished))0x00449c40)
//bool32 __cdecl LegoObject_Update_Reinforcing(LegoObject* liveObj, real32 elapsed, OUT bool32* finished);

// <LegoRR.exe @00449d30>
#define LegoObject_GoEat_unk ((void (__cdecl* )(LegoObject* liveObj))0x00449d30)
//void __cdecl LegoObject_GoEat_unk(LegoObject* liveObj);

// <LegoRR.exe @00449d80>
#define LegoObject_TryGoEat_FUN_00449d80 ((bool32 (__cdecl* )(LegoObject* liveObj1, LegoObject* liveObj2))0x00449d80)
//bool32 __cdecl LegoObject_TryGoEat_FUN_00449d80(LegoObject* liveObj1, LegoObject* liveObj2);

// <LegoRR.exe @00449ec0>
//#define LegoObject_HideAllCertainObjects ((void (__cdecl* )(void))0x00449ec0)
void __cdecl LegoObject_HideAllCertainObjects(void);

// <LegoRR.exe @00449ee0>
#define LegoObject_FlocksCallback_FUN_00449ee0 ((void (__cdecl* )(Flocks* flockData, FlocksItem* subdata, void* data))0x00449ee0)
//void __cdecl LegoObject_FlocksCallback_FUN_00449ee0(Flocks* flockData, FlocksItem* subdata, void* data);

// <LegoRR.exe @00449f90>
#define LegoObject_Hide2 ((void (__cdecl* )(LegoObject* liveObj, bool32 hide2))0x00449f90)
//void __cdecl LegoObject_Hide2(LegoObject* liveObj, bool32 hide2);

// <LegoRR.exe @00449fb0>
#define LegoObject_Callback_HideCertainObjects ((bool32 (__cdecl* )(LegoObject* liveObj, void* unused))0x00449fb0)
//bool32 __cdecl LegoObject_Callback_HideCertainObjects(LegoObject* liveObj, void* unused);

// <LegoRR.exe @0044a210>
#define LegoObject_Hide ((void (__cdecl* )(LegoObject* liveObj, bool32 hide))0x0044a210)
//void __cdecl LegoObject_Hide(LegoObject* liveObj, bool32 hide);

// <LegoRR.exe @0044a2b0>
#define LegoObject_IsHidden ((bool32 (__cdecl* )(LegoObject* liveObj))0x0044a2b0)
//bool32 __cdecl LegoObject_IsHidden(LegoObject* liveObj);

// <LegoRR.exe @0044a330>
#define LegoObject_FP_GetPositionAndHeading ((void (__cdecl* )(LegoObject* liveObj, sint32 cameraFrame, OUT Vector3F* worldPos, OPTIONAL OUT Vector3F* dir))0x0044a330)
//void __cdecl LegoObject_FP_GetPositionAndHeading(LegoObject* liveObj, sint32 cameraFrame, OUT Vector3F* worldPos, OPTIONAL OUT Vector3F* dir);

// <LegoRR.exe @0044a4c0>
#define LegoObject_GetActivityContainer ((Gods98::Container* (__cdecl* )(LegoObject* liveObj))0x0044a4c0)
//Gods98::Container* __cdecl LegoObject_GetActivityContainer(LegoObject* liveObj);

// <LegoRR.exe @0044a560>
#define LegoObject_GetDrillNullPosition ((bool32 (__cdecl* )(LegoObject* liveObj, OUT real32* xPos, OUT real32* yPos))0x0044a560)
//bool32 __cdecl LegoObject_GetDrillNullPosition(LegoObject* liveObj, OUT real32* xPos, OUT real32* yPos);

// <LegoRR.exe @0044a5d0>
#define LegoObject_FP_Move ((void (__cdecl* )(LegoObject* liveObj, sint32 forward, sint32 strafe, real32 rotate))0x0044a5d0)
//void __cdecl LegoObject_FP_Move(LegoObject* liveObj, sint32 forward, sint32 strafe, real32 rotate);

// <LegoRR.exe @0044a650>
#define LegoObject_RegisterRechargeSeam ((void (__cdecl* )(const Point2I* blockPos))0x0044a650)
//void __cdecl LegoObject_RegisterRechargeSeam(const Point2I* blockPos);

// <LegoRR.exe @0044a690>
#define LegoObject_FindNearestRechargeSeam ((bool32 (__cdecl* )(LegoObject* liveObj, OPTIONAL OUT Point2I* blockPos))0x0044a690)
//bool32 __cdecl LegoObject_FindNearestRechargeSeam(LegoObject* liveObj, OPTIONAL OUT Point2I* blockPos);

// <LegoRR.exe @0044a770>
#define LegoObject_RegisterSlimySlugHole ((void (__cdecl* )(const Point2I* blockPos))0x0044a770)
//void __cdecl LegoObject_RegisterSlimySlugHole(const Point2I* blockPos);

// <LegoRR.exe @0044a7b0>
#define LegoObject_FindNearestSlugHole ((bool32 (__cdecl* )(LegoObject* liveObj, OPTIONAL OUT Point2I* blockPos))0x0044a7b0)
//bool32 __cdecl LegoObject_FindNearestSlugHole(LegoObject* liveObj, OPTIONAL OUT Point2I* blockPos);

// <LegoRR.exe @0044a890>
#define LegoObject_FindNearestWall ((bool32 (__cdecl* )(LegoObject* liveObj, OUT sint32* bx, OUT sint32* by, bool32 min1BlockDist, bool32 allowCorner, bool32 allowReinforced))0x0044a890)
//bool32 __cdecl LegoObject_FindNearestWall(LegoObject* liveObj, OUT sint32* bx, OUT sint32* by, bool32 min1BlockDist, bool32 allowCorner, bool32 allowReinforced);

// DATA: Vector3F*
// <LegoRR.exe @0044aa60>
#define LegoObject_QsortCompareWallDistances ((bool32 (__cdecl* )(const void* a, const void* b))0x0044aa60)
//bool32 __cdecl LegoObject_QsortCompareWallDistances(const void* a, const void* b);

// <LegoRR.exe @0044aa90>
#define LegoObject_LoadMeshLOD ((MeshLOD* (__cdecl* )(const Gods98::Config* act, const char* gameName, const char* dirname, LOD_PolyLevel polyLOD, uint32 numCameraFrames))0x0044aa90)
//MeshLOD* __cdecl LegoObject_LoadMeshLOD(const Gods98::Config* act, const char* gameName, const char* dirname, LOD_PolyLevel polyLOD, uint32 numCameraFrames);

// mouseWorldPos is only optional if execute is false.
// <LegoRR.exe @0044abd0>
//#define LegoObject_UpdateBuildingPlacement ((bool32 (__cdecl* )(LegoObject_Type objType, LegoObject_ID objID, bool32 leftReleased, bool32 rightReleased, OPTIONAL const Point2F* mouseWorldPos, uint32 mouseBlockX, uint32 mouseBlockY, bool32 execute, SelectPlace* selectPlace))0x0044abd0)
bool32 __cdecl LegoObject_UpdateBuildingPlacement(LegoObject_Type objType, LegoObject_ID objID, bool32 leftReleased, bool32 rightReleased,
													OPTIONAL const Point2F* mouseWorldPos, uint32 mouseBlockX, uint32 mouseBlockY,
													bool32 execute, SelectPlace* selectPlace);

// <LegoRR.exe @0044af80>
#define LegoObject_LoadObjTtsSFX ((void (__cdecl* )(const Gods98::Config* config, const char* gameName))0x0044af80)
//void __cdecl LegoObject_LoadObjTtsSFX(const Gods98::Config* config, const char* gameName);

// <LegoRR.exe @0044b040>
#define LegoObject_GetObjTtSFX ((SFX_ID (__cdecl* )(LegoObject* liveObj))0x0044b040)
//SFX_ID __cdecl LegoObject_GetObjTtSFX(LegoObject* liveObj);

// <LegoRR.exe @0044b080>
//#define LegoObject_SetLevelEnding ((void (__cdecl* )(bool32 ending))0x0044b080)
void __cdecl LegoObject_SetLevelEnding(bool32 ending);

// <LegoRR.exe @0044b0a0>
//#define LegoObject_FUN_0044b0a0 ((void (__cdecl* )(LegoObject* liveObj))0x0044b0a0)
void __cdecl LegoObject_FUN_0044b0a0(LegoObject* liveObj);

// <LegoRR.exe @0044b110>
#define LegoObject_SpawnDropCrystals_FUN_0044b110 ((void (__cdecl* )(LegoObject* liveObj, sint32 crystalCount, bool32 param_3))0x0044b110)
//void __cdecl LegoObject_SpawnDropCrystals_FUN_0044b110(LegoObject* liveObj, sint32 crystalCount, bool32 param_3);

// <LegoRR.exe @0044b250>
#define LegoObject_CallsSpawnDropCrystals_FUN_0044b250 ((void (__cdecl* )(LegoObject* liveObj))0x0044b250)
//void __cdecl LegoObject_CallsSpawnDropCrystals_FUN_0044b250(LegoObject* liveObj);

// <LegoRR.exe @0044b270>
#define LegoObject_GenerateTinyRMs_FUN_0044b270 ((void (__cdecl* )(LegoObject* liveObj))0x0044b270)
//void __cdecl LegoObject_GenerateTinyRMs_FUN_0044b270(LegoObject* liveObj);

// <LegoRR.exe @0044b400>
#define LegoObject_GenerateSmallSpiders ((void (__cdecl* )(uint32 bx, uint32 by, uint32 randSeed))0x0044b400)
//void __cdecl LegoObject_GenerateSmallSpiders(uint32 bx, uint32 by, uint32 randSeed);

// <LegoRR.exe @0044b510>
#define LegoObject_DoThrowLegoman ((void (__cdecl* )(LegoObject* liveObj, LegoObject* thrownObj))0x0044b510)
//void __cdecl LegoObject_DoThrowLegoman(LegoObject* liveObj, LegoObject* thrownObj);

// <LegoRR.exe @0044b610>
#define LegoObject_Tool_IsBeamWeapon ((bool32 (__cdecl* )(LegoObject_ToolType toolType))0x0044b610)
//bool32 __cdecl LegoObject_Tool_IsBeamWeapon(LegoObject_ToolType toolType);

// <LegoRR.exe @0044b630>
#define LegoObject_MiniFigure_EquipTool ((void (__cdecl* )(LegoObject* liveObj, LegoObject_ToolType toolType))0x0044b630)
//void __cdecl LegoObject_MiniFigure_EquipTool(LegoObject* liveObj, LegoObject_ToolType toolType);

// <LegoRR.exe @0044b740>
#define LegoObject_HasToolEquipped ((bool32 (__cdecl* )(LegoObject* liveObj, LegoObject_ToolType toolType))0x0044b740)
//bool32 __cdecl LegoObject_HasToolEquipped(LegoObject* liveObj, LegoObject_ToolType toolType);

// <LegoRR.exe @0044b780>
#define LegoObject_TaskHasTool_FUN_0044b780 ((bool32 (__cdecl* )(LegoObject* liveObj, AITask_Type taskType))0x0044b780)
//bool32 __cdecl LegoObject_TaskHasTool_FUN_0044b780(LegoObject* liveObj, AITask_Type taskType);

// <LegoRR.exe @0044b7c0>
#define LegoObject_DoGetTool ((sint32 (__cdecl* )(LegoObject* liveObj, LegoObject_ToolType toolType))0x0044b7c0)
//sint32 __cdecl LegoObject_DoGetTool(LegoObject* liveObj, LegoObject_ToolType toolType);

// <LegoRR.exe @0044b940>
#define LegoObject_Flocks_Initialise ((void (__cdecl* )(LegoObject* liveObj))0x0044b940)
//void __cdecl LegoObject_Flocks_Initialise(LegoObject* liveObj);

// <LegoRR.exe @0044ba60>
#define LegoObject_FlocksMatrix_FUN_0044ba60 ((void (__cdecl* )(Gods98::Container* resData, IN OUT Matrix4F* matrix, real32 scalar))0x0044ba60)
//void __cdecl LegoObject_FlocksMatrix_FUN_0044ba60(Gods98::Container* resData, IN OUT Matrix4F* matrix, real32 scalar);

// <LegoRR.exe @0044bb10>
#define LegoObject_Flocks_Callback_SubdataOrientationAnim ((void (__cdecl* )(Flocks* flocksData, FlocksItem* subdata, real32* pElapsed))0x0044bb10)
//void __cdecl LegoObject_Flocks_Callback_SubdataOrientationAnim(Flocks* flocksData, FlocksItem* subdata, real32* pElapsed);

// <LegoRR.exe @0044bd70>
#define LegoObject_Flocks_Container_ReleaseCallback ((void (__cdecl* )(Flocks* flockData, FlocksItem* subdata, void* data))0x0044bd70)
//void __cdecl LegoObject_Flocks_Container_ReleaseCallback(Flocks* flockData, FlocksItem* subdata, void* data);

// <LegoRR.exe @0044bda0>
#define LegoObject_Flocks_Free ((void (__cdecl* )(Flocks* flockData))0x0044bda0)
//void __cdecl LegoObject_Flocks_Free(Flocks* flockData);

// <LegoRR.exe @0044bdf0>
#define LegoObject_Flocks_SetParameters ((void (__cdecl* )(LegoObject* liveObj, bool32 additive))0x0044bdf0)
//void __cdecl LegoObject_Flocks_SetParameters(LegoObject* liveObj, bool32 additive);

// <LegoRR.exe @0044bef0>
#define LegoObject_Flocks_FUN_0044bef0 ((void (__cdecl* )(LegoObject* liveObj, real32 elapsed))0x0044bef0)
//void __cdecl LegoObject_Flocks_FUN_0044bef0(LegoObject* liveObj, real32 elapsed);

// <LegoRR.exe @0044c0f0>
#define LegoObject_FlocksDestroy ((void (__cdecl* )(LegoObject* liveObj))0x0044c0f0)
//void __cdecl LegoObject_FlocksDestroy(LegoObject* liveObj);

// <LegoRR.exe @0044c1c0>
#define LegoObject_Flocks_Update_FUN_0044c1c0 ((void (__cdecl* )(real32* pElapsed))0x0044c1c0)
//void __cdecl LegoObject_Flocks_Update_FUN_0044c1c0(real32* pElapsed);

// <LegoRR.exe @0044c290>
#define LegoObject_DestroyRockMonster_FUN_0044c290 ((bool32 (__cdecl* )(LegoObject* liveObj))0x0044c290)
//bool32 __cdecl LegoObject_DestroyRockMonster_FUN_0044c290(LegoObject* liveObj);

// <LegoRR.exe @0044c2f0>
//#define LegoObject_Freeze ((bool32 (__cdecl* )(LegoObject* liveObj, real32 freezerTime))0x0044c2f0)
bool32 __cdecl LegoObject_Freeze(LegoObject* liveObj, real32 freezerTime);

// <LegoRR.exe @0044c3d0>
#define LegoObject_FUN_0044c3d0 ((void (__cdecl* )(LegoObject* liveObj))0x0044c3d0)
//void __cdecl LegoObject_FUN_0044c3d0(LegoObject* liveObj);

// <LegoRR.exe @0044c410>
#define LegoObject_Push ((bool32 (__cdecl* )(LegoObject* liveObj, const Point2F* pushVec2D, real32 pushDist))0x0044c410)
//bool32 __cdecl LegoObject_Push(LegoObject* liveObj, const Point2F* pushVec2D, real32 pushDist);

// <LegoRR.exe @0044c470>
#define LegoObject_UpdatePushing ((void (__cdecl* )(LegoObject* liveObj, real32 elapsed))0x0044c470)
//void __cdecl LegoObject_UpdatePushing(LegoObject* liveObj, real32 elapsed);

// <LegoRR.exe @0044c760>
#define LegoObject_TryEnterLaserTrackerMode ((void (__cdecl* )(LegoObject* liveObj))0x0044c760)
//void __cdecl LegoObject_TryEnterLaserTrackerMode(LegoObject* liveObj);

// <LegoRR.exe @0044c7c0>
#define LegoObject_Callback_ExitLaserTrackerMode ((bool32 (__cdecl* )(LegoObject* liveObj, void* unused))0x0044c7c0)
//bool32 __cdecl LegoObject_Callback_ExitLaserTrackerMode(LegoObject* liveObj, void* unused);

// <LegoRR.exe @0044c7f0>
#define LegoObject_MiniFigureHasBeamEquipped2 ((bool32 (__cdecl* )(LegoObject* liveObj))0x0044c7f0)
//bool32 __cdecl LegoObject_MiniFigureHasBeamEquipped2(LegoObject* liveObj);

// Called by Panel_CheckCollision for type Panel_CameraControl.
// <LegoRR.exe @0044c810>
//#define LegoObject_CameraCycleUnits ((void (__cdecl* )(void))0x0044c810)
void __cdecl LegoObject_CameraCycleUnits(void);

/// CUSTOM: Wrapper around LegoObject_Callback_CameraCycleFindUnitByFlags with extra limit handling.
bool LegoObject_CameraCycleFindNextUnitByFlags(LegoObject_TypeFlags objTypeFlags, OPTIONAL OUT bool* unitsExist);

/// CUSTOM: Extension of LegoObject_Callback_CameraCycleFindUnit so that we can customize what objects are included in the cycle.
bool LegoObject_Callback_CameraCycleFindUnitByFlags(LegoObject* liveObj, LegoObject_TypeFlags objTypeFlags, OPTIONAL IN OUT bool* unitsExist);

// REPLACED BY: LegoObject_Callback_CameraCycleFindUnitByFlags
// DATA: OPTIONAL bool32* pNoBuildings (defaults to false when NULL).
// <LegoRR.exe @0044c8b0>
//#define LegoObject_Callback_CameraCycleFindUnit ((bool32 (__cdecl* )(LegoObject* liveObj, OPTIONAL void* pNoBuildings))0x0044c8b0)
bool32 __cdecl LegoObject_Callback_CameraCycleFindUnit(LegoObject* liveObj, OPTIONAL void* pNoBuildings);



/// CUSTOM: Starts the tickdown for dynamite or sonic blaster.
void LegoObject_StartTickDown(LegoObject* liveObj, bool showInfoMessage);

#pragma endregion

}
