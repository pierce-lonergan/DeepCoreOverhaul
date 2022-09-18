// RadarMap.h : 
//

#pragma once

#include "../../engine/drawing/Draw.h"

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

#define RADARMAP_MAXTABLECOLOURS	30
#define RADARMAP_MAXARROWPOINTS		20

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

struct SearchRadarObjectInArea // [LegoRR/search.c|struct:0x10]
{
	/*00,4*/	LegoObject* object;
	/*04,8*/	Point2F worldPos;
	/*0c,4*/	real32 radius; // BlockSize -or- (BlockSize / 6.0)
	/*10*/
};
assert_sizeof(SearchRadarObjectInArea, 0x10);


struct RadarMap // [LegoRR/RadarMap.c|struct:0x3c]
{
	/*00,4*/	Map3D* map;
	/*04,10*/	Area2F screenRect; // Passed value of (16.0f, 13.0f, 151.0f, 151.0f).
	/*14,4*/	real32 zoom; // Always assigned a value in the range [10,20], with 15 being the default. Higher values are more zoomed-in.
	/*18,4*/	real32 blockSize;
	/*1c,8*/	Point2F centerPos;
	/*24,10*/	Rect2F worldRect;
	/*34,4*/	Gods98::Draw_Rect* drawRectList; // Pointer to a list not owned by this struct.
	/*38,4*/	uint32 drawRectCount;
	/*3c*/
};
assert_sizeof(RadarMap, 0x3c);


struct RadarMap_Globs // [LegoRR/RadarMap.c|struct:0x3f4|tags:GLOBS]
{
	/*000,168*/	ColourRGBF colourTable[RADARMAP_MAXTABLECOLOURS]; // (constant, RGBf [0,255] -> [0,1] on RadarMap_Initialise)
	/*168,8*/	Point2I highlightBlockPos; // (init: (-1, -1)) Unused, but can flash-highlight a radar map block in rgb(255,  0,  0).
	/*170,140*/	Vector4F arrowPointsFrom[RADARMAP_MAXARROWPOINTS];
	/*2b0,140*/	Vector4F arrowPointsTo[RADARMAP_MAXARROWPOINTS];
	/*3f0,4*/	uint32 arrowPointCount;
	/*3f4*/
};
assert_sizeof(RadarMap_Globs, 0x3f4);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004a9f28>
extern RadarMap_Globs & radarmapGlobs;

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

 /// CUSTOM:
void RadarMap_ClearHighlightBlock();

/// CUSTOM:
void RadarMap_SetHighlightBlock(OPTIONAL const Point2I* blockPos);

/// CUSTOM:
real32 RadarMap_GetZoom(const RadarMap* radarMap);


// <LegoRR.exe @0045db60>
//#define RadarMap_SetZoom ((void (__cdecl* )(RadarMap* radarMap, real32 zoom))0x0045db60)
void __cdecl RadarMap_SetZoom(RadarMap* radarMap, real32 zoom);

// <LegoRR.exe @0045db70>
//#define RadarMap_Initialise ((void (__cdecl* )(void))0x0045db70)
void __cdecl RadarMap_Initialise(void);

// <LegoRR.exe @0045dd50>
//#define RadarMap_Create ((RadarMap* (__cdecl* )(Map3D* map, const Area2F* screenRect, real32 zoom))0x0045dd50)
RadarMap* __cdecl RadarMap_Create(Map3D* map, const Area2F* screenRect, real32 zoom);

// <LegoRR.exe @0045ddb0>
//#define RadarMap_Free ((void (__cdecl* )(RadarMap* radarMap))0x0045ddb0)
void __cdecl RadarMap_Free(RadarMap* radarMap);

// Radius is in units relative to blockSize.
// <LegoRR.exe @0045ddc0>
//#define RadarMap_DrawSurveyDotCircle ((void (__cdecl* )(RadarMap* radarMap, const Point2F* center, real32 radius, real32 brightness))0x0045ddc0)
void __cdecl RadarMap_DrawSurveyDotCircle(RadarMap* radarMap, const Point2F* center, real32 radius, real32 brightness);

// DRAW MODE: Only Draw API drawing calls can be used within this function.
// <LegoRR.exe @0045de80>
//#define RadarMap_Draw ((void (__cdecl* )(RadarMap* radarMap, const Point2F* centerPos))0x0045de80)
void __cdecl RadarMap_Draw(RadarMap* radarMap, const Point2F* centerPos);

/// CUSTOM: Isolate Draw API calls from RadarMap_Draw.
void __cdecl RadarMap_ClearScreen(RadarMap* radarMap);

// <LegoRR.exe @0045e6c0>
//#define RadarMap_CanShowObject ((bool32 (__cdecl* )(LegoObject* liveObj))0x0045e6c0)
bool32 __cdecl RadarMap_CanShowObject(LegoObject* liveObj);

// DATA: RadarMap* radarMap
// <LegoRR.exe @0045e720>
//#define RadarMap_Callback_AddObjectDrawRect ((bool32 (__cdecl* )(LegoObject* liveObj, void* pRadarMap))0x0045e720)
bool32 __cdecl RadarMap_Callback_AddObjectDrawRect(LegoObject* liveObj, void* pRadarMap);

/// CUSTOM:
bool RadarMap_InsideRadarWorld(const RadarMap* radarMap, const Point2F* wPos);

// <LegoRR.exe @0045e920>
//#define RadarMap_InsideRadarScreen ((bool32 (__cdecl* )(RadarMap* radarMap, uint32 mouseX, uint32 mouseY))0x0045e920)
bool32 __cdecl RadarMap_InsideRadarScreen(const RadarMap* radarMap, uint32 mouseX, uint32 mouseY);

// <LegoRR.exe @0045e990>
//#define RadarMap_ScreenToWorldBlockPos ((bool32 (__cdecl* )(RadarMap* radarMap, uint32 mouseX, uint32 mouseY, OPTIONAL OUT Point2F* worldPos, OUT sint32* bx, OUT sint32* by))0x0045e990)
bool32 __cdecl RadarMap_ScreenToWorldBlockPos(RadarMap* radarMap, uint32 mouseX, uint32 mouseY, OPTIONAL OUT Point2F* worldPos, OUT sint32* bx, OUT sint32* by);

// <LegoRR.exe @0045eae0>
//#define RadarMap_TrySelectObject ((bool32 (__cdecl* )(RadarMap* radarMap, uint32 mouseX, uint32 mouseY, OUT LegoObject** liveObj, OPTIONAL OUT Point2F* objPosition))0x0045eae0)
bool32 __cdecl RadarMap_TrySelectObject(RadarMap* radarMap, uint32 mouseX, uint32 mouseY, OUT LegoObject** liveObj, OPTIONAL OUT Point2F* objPos);

// DATA: const SearchRadarObjectInArea* search
// <LegoRR.exe @0045eba0>
//#define RadarMap_Callback_FindObjectInClickArea ((bool32 (__cdecl* )(LegoObject* liveObj, void* pSearch))0x0045eba0)
bool32 __cdecl RadarMap_Callback_FindObjectInClickArea(LegoObject* liveObj, void* pSearch);

// Transforms units relative to blockSize into zoomed size.
// <LegoRR.exe @0045ec00>
//#define RadarMap_TransformRect ((void (__cdecl* )(RadarMap* radarMap, IN OUT Area2F* rect))0x0045ec00)
void __cdecl RadarMap_TransformRect(const RadarMap* radarMap, IN OUT Area2F* rect);

// <LegoRR.exe @0045eca0>
//#define RadarMap_GetBlockColour ((bool32 (__cdecl* )(IN OUT Gods98::Draw_Rect* drawRect, uint32 bx, uint32 by))0x0045eca0)
bool32 __cdecl RadarMap_GetBlockColour(IN OUT Gods98::Draw_Rect* drawRect, uint32 bx, uint32 by);

#pragma endregion

}
