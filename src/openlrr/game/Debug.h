// Debug.h : 
//

#pragma once

#include "GameCommon.h"
#include "Game.h"


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

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

enum class Debug_RouteVisualAuto
{
	None,
	TrackedOnRadar,
	AllFriendly,
	All,
};

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct Debug_RouteVisual
{
	LegoObject* object;
	Gods98::Container* contMeshLines; // Block-to-block lines for the full route.
	Gods98::Container* contMeshCurve; // Between-block curve lines for the current block section.
	bool autoAdded; // Auto-added units can be removed during UpdateAll (manually adding overwrites this).
};

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

const char* Debug_GetObjectTypeName(LegoObject_Type objType);
const char* Debug_GetObjectIDName(LegoObject_Type objType, LegoObject_ID objID);

#pragma region RouteVisual

bool Debug_RouteVisual_IsEnabled();
void Debug_RouteVisual_SetEnabled(bool enabled);
bool Debug_RouteVisual_IsCompletedPathsEnabled();
void Debug_RouteVisual_SetCompletedPathsEnabled(bool enabled);
bool Debug_RouteVisual_IsCurvePathsEnabled();
void Debug_RouteVisual_SetCurvePathsEnabled(bool enabled);
Debug_RouteVisualAuto Debug_RouteVisual_GetAutoMode();
void Debug_RouteVisual_SetAutoMode(Debug_RouteVisualAuto autoMode);

bool Debug_RouteVisual_CanAdd(LegoObject* liveObj, bool friendlyOnly);

Debug_RouteVisual* Debug_RouteVisual_Get(LegoObject* liveObj);
void _Debug_RouteVisual_Hide(Debug_RouteVisual* routeVisual, bool hide);
bool _Debug_RouteVisual_IsHidden(Debug_RouteVisual* routeVisual);
Debug_RouteVisual* Debug_RouteVisual_Add(LegoObject* liveObj, bool autoAdd = false);
bool Debug_RouteVisual_Remove(LegoObject* liveObj);
void _Debug_RouteVisual_Remove2(Debug_RouteVisual* routeVisual, bool erase);
void _Debug_RouteVisual_Update(Debug_RouteVisual* routeVisual, real32 elapsedWorld, real32 elapsedInterface);

void Debug_RouteVisual_UpdateAll(real32 elapsedWorld, real32 elapsedInterface);
void Debug_RouteVisual_HideAll(bool hide);

void Debug_RouteVisual_AddAll(bool friendlyOnly);
void Debug_RouteVisual_AddSelected();
void Debug_RouteVisual_RemoveAll();
void Debug_RouteVisual_RemoveSelected();

void _Debug_RouteVisual_UpdateLine(Gods98::Container* contMesh, uint32 groupID, real32 thickness, const Vector3F* fromPos, const Vector3F* toPos, real32 red, real32 green, real32 blue, real32 alpha, bool visible);

#pragma endregion

#pragma endregion

}
