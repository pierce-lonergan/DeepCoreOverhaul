// SelectPlace.h : 
//

#pragma once

#include "../GameCommon.h"


namespace LegoRR
{; // !<---

/**********************************************************************************
 ******** Constants
 **********************************************************************************/

#pragma region Constants

#define SELECTPLACE_MAXSHAPETILES		10
#define SELECTPLACE_MAXSHAPEPOINTS		20

#pragma endregion

/**********************************************************************************
 ******** Enumerations
 **********************************************************************************/

#pragma region Enums

enum class SelectPlace_ArrowVisibility
{
	Never,		// Never show the arrow.
	SolidOnly,	// Show the arrow for buildings that only have solid tiles.
	Always,		// Always show the arrow.
};

#pragma endregion

/**********************************************************************************
 ******** Structures
 **********************************************************************************/

#pragma region Structs

struct SelectPlace // [LegoRR/SelectPlace.c|struct:0x8] Building selection highlight shown on map when placing
{
	/*0,4*/	Gods98::Container* contMesh;
	/*4,4*/	real32 tileDepth; // (init: 5.0) Z height of each coloured square when drawing the building placement grid.
	/*8*/
	/// EXPANSION: An arrow mesh used to show the rotation over the origin tile.
	/*8,4*/	Gods98::Container* contMeshArrow;
	/*c,4*/	SelectPlace_ArrowVisibility arrowVisibility;
	/*10*/
};
//assert_sizeof(SelectPlace, 0x8);

#pragma endregion

/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @005023c8>
extern Point2I (& s_TransformShapePoints)[SELECTPLACE_MAXSHAPEPOINTS];

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

/// CUSTOM: Gets when the origin tile arrow is displayed.
SelectPlace_ArrowVisibility SelectPlace_GetArrowVisibility(const SelectPlace* selectPlace);

/// CUSTOM: Sets when the origin tile arrow is displayed.
void SelectPlace_SetArrowVisibility(SelectPlace* selectPlace, SelectPlace_ArrowVisibility visibility);

/// CUSTOM: Determines if the origin tile arrow is visible based on the visibility setting.
bool _SelectPlace_ShouldShowArrow(const SelectPlace* selectPlace, bool shapeHasPath);

/// CUSTOM: Creates the origin tile arrow mesh.
bool _SelectPlace_CreateArrow(SelectPlace* selectPlace, Gods98::Container* contRoot);

/// CUSTOM: Assigns the origin tile arrow mesh vertices relative to the SelectPlace vertex positions. Also unhides the arrow mesh.
void _SelectPlace_UpdateArrow(SelectPlace* selectPlace, Direction direction, bool shapeHasPath, const Vector3F* selectVertPoses);

/// CUSTOM: Hides the origin tile arrow mesh.
void _SelectPlace_HideArrow(SelectPlace* selectPlace);


// Prefer defining this in the cpp file. So that in the future, this include file can be made independent of `Containers.h`.
// <inlined, unused>
bool32 SelectPlace_IsHidden(const SelectPlace* selectPlace);



// `tileDepth` is the Z height of each coloured square when drawing the building placement grid.
// <LegoRR.exe @004641c0>
//#define SelectPlace_Create ((SelectPlace* (__cdecl* )(Gods98::Container* contRoot, real32 tileDepth))0x004641c0)
SelectPlace* __cdecl SelectPlace_Create(Gods98::Container* contRoot, real32 tileDepth);

// Returns a temporary list of transformed points. This value is valid until the next function call.
// <LegoRR.exe @004643d0>
//#define SelectPlace_TransformShapePoints ((const Point2I* (__cdecl* )(const Point2I* translation, const Point2I* shapePoints, uint32 shapeCount, Direction rotation))0x004643d0)
const Point2I* __cdecl SelectPlace_TransformShapePoints(const Point2I* translation, const Point2I* shapePoints, uint32 shapeCount, Direction rotation);

// Checks the validity of a selection placement and updates the render state of the SelectPlace tiles.
// Returns the temporary result of SelectPlace_TransformShapePoints, or null.
// `waterEntrances` is a signed int, because a value of `-1` (or lower) is required to define the origin tile as a Yellow Path tile.
// <LegoRR.exe @00464480>
//#define SelectPlace_CheckAndUpdate ((const Point2I* (__cdecl* )(SelectPlace* selectPlace, const Point2I* blockPos, const Point2I* shapePoints, uint32 shapeCount, Direction direction, Map3D* map, sint32 waterEntrances))0x00464480)
const Point2I* __cdecl SelectPlace_CheckAndUpdate(SelectPlace* selectPlace, const Point2I* blockPos, const Point2I* shapePoints, uint32 shapeCount, Direction direction, Map3D* map, sint32 waterEntrances);

// <LegoRR.exe @004649e0>
//#define SelectPlace_Hide ((void (__cdecl* )(SelectPlace* selectPlace, bool32 hide))0x004649e0)
void __cdecl SelectPlace_Hide(SelectPlace* selectPlace, bool32 hide);

#pragma endregion

}
