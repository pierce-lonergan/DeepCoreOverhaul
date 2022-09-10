// SelectPlace.cpp : 
//

#include "../Game.h"

#include "Map3D.h"
#include "SelectPlace.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @005023c8>
Point2I (& LegoRR::s_TransformShapePoints)[SELECTPLACE_MAXSHAPEPOINTS] = *(Point2I(*)[SELECTPLACE_MAXSHAPEPOINTS])0x005023c8;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// Prefer defining this in the cpp file. So that in the future, this include file can be made independent of `Containers.h`.
// <inlined, unused>
bool32 LegoRR::SelectPlace_IsHidden(const SelectPlace* selectPlace)
{
	return Gods98::Container_IsHidden(selectPlace->contMesh);
}



// `tileDepth` is the Z height of each coloured square when drawing the building placement grid.
// <LegoRR.exe @004641c0>
LegoRR::SelectPlace* __cdecl LegoRR::SelectPlace_Create(Gods98::Container* contRoot, real32 tileDepth)
{
	SelectPlace* selectPlace = (SelectPlace*)Gods98::Mem_Alloc(sizeof(SelectPlace));
	if (selectPlace == nullptr)
		return nullptr;

	selectPlace->tileDepth = tileDepth;
	selectPlace->contMesh = Gods98::Container_MakeMesh2(contRoot, Gods98::Container_MeshType::Transparent);

	if (selectPlace->contMesh != nullptr) {
		const Point2F TUV_COORDS[4] = {
			{ 0.0f, 0.0f },
			{ 1.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 0.0f, 1.0f },
		};
		const Vector3F VERT_NORMALS[5] = {
			{  0.0f,  0.0f, -1.0f },
			{  0.0f,  1.0f,  0.0f },
			{  1.0f,  0.0f,  0.0f },
			{  0.0f, -1.0f,  0.0f },
			{ -1.0f,  0.0f,  0.0f },
		};

		uint32 faceData[30] = { 0 }; // dummy init

		const uint32 FACE_INITIAL[6] = {
			0, 1, 3, 1, 2, 3,
		};
		for (uint32 i = 0; i < 5; i++) {
			for (uint32 j = 0; j < 6; j++) {
				faceData[(i*6) + j] = FACE_INITIAL[j] + (i*4); // Note: unrelated to (i*6) for index.
			}
		}

		for (uint32 groupID = 0; groupID < SELECTPLACE_MAXSHAPETILES; groupID++) {
			Gods98::Container_Mesh_AddGroup(selectPlace->contMesh, 20, 10, 3, faceData);
			for (uint32 i = 0; i < 5; i++) {

				Vertex vertices[4] = { { 0.0f } }; // dummy init
				for (uint32 j = 0; j < 4; j++) {
					// Ummm... not setting the vertex positions???
					vertices[j].normal = VERT_NORMALS[i];
					vertices[j].tuv = TUV_COORDS[j];
					vertices[j].colour = 0xffffffff; // white
				}
				Gods98::Container_Mesh_SetVertices(selectPlace->contMesh, groupID, (i*4), 4, vertices);
			}
			Gods98::Container_Mesh_SetQuality(selectPlace->contMesh, groupID, Gods98::Container_Quality::Gouraud);
		}

		Gods98::Container_Hide(selectPlace->contMesh, true);
		return selectPlace;
	}
	Gods98::Mem_Free(selectPlace);
	return nullptr;
}

// Returns a temporary list of transformed points. This value is valid until the next function call.
// <LegoRR.exe @004643d0>
const Point2I* __cdecl LegoRR::SelectPlace_TransformShapePoints(const Point2I* translation, const Point2I* shapePoints, uint32 shapeCount, Direction rotation)
{
	/// SANITY: Bounds checks.
	Error_Fatal(shapeCount > SELECTPLACE_MAXSHAPEPOINTS, "Too many points in shape passed to SelectPlace_TransformShapePoints");

	for (uint32 i = 0; i < shapeCount; i++) {
		// Apply rotation to shapePoints (and ensure rotation is wrapped to [0,3]).
		switch (DirectionWrap(rotation)) {
		case DIRECTION_UP: // (x, y)
		default:
			s_TransformShapePoints[i].x = shapePoints[i].x;
			s_TransformShapePoints[i].y = shapePoints[i].y;
			break;
		case DIRECTION_RIGHT: // (-y, x)
			s_TransformShapePoints[i].x = -shapePoints[i].y;
			s_TransformShapePoints[i].y = shapePoints[i].x;
			break;
		case DIRECTION_DOWN: // (-x, -y)
			s_TransformShapePoints[i].x = -shapePoints[i].x;
			s_TransformShapePoints[i].y = -shapePoints[i].y;
			break;
		case DIRECTION_LEFT: // (y, -x)
			s_TransformShapePoints[i].x = shapePoints[i].y;
			s_TransformShapePoints[i].y = -shapePoints[i].x;
			break;
		}

		// Apply translation to shapePoints.
		s_TransformShapePoints[i].x += translation->x;
		s_TransformShapePoints[i].y += translation->y;
	}
	return s_TransformShapePoints;
}

// Checks the validity of a selection placement and updates the render state of the SelectPlace tiles.
// Returns the temporary result of SelectPlace_TransformShapePoints, or null.
// `waterEntrances` is a signed int, because a value of `-1` (or lower) is required to define the origin tile as a Yellow Path tile.
// <LegoRR.exe @00464480>
const Point2I* __cdecl LegoRR::SelectPlace_CheckAndUpdate(SelectPlace* selectPlace, const Point2I* blockPos, const Point2I* shapePoints, uint32 shapeCount, Direction direction, Map3D* map, sint32 waterEntrances)
{
	// Vertices to use for each of the 5 faces of a select place tile (the bottom face is not rendered).
	const uint32 VERT_INDEX[20] = {
		0, 1, 2, 3, 0, 4, 5, 1, 1, 5, 6, 2, 2, 6, 7, 3, 3, 7, 4, 0,
	};

	const Point2I DIRECTIONS[DIRECTION__COUNT] = {
		{  0, -1 },
		{  1,  0 },
		{  0,  1 },
		{ -1,  0 },
	};

	bool canPlace = true;

	const Point2I* pShape = SelectPlace_TransformShapePoints(blockPos, shapePoints, shapeCount, direction);

	bool okRoughness;
	if (Cheat_IsBuildOnAnyRoughness()) {
		okRoughness = true;
	}
	else {
		okRoughness = Map3D_CheckBuildingTolerance(Lego_GetMap(), pShape, shapeCount,
												   Lego_GetLevel()->BuildingTolerance, Lego_GetLevel()->BuildingMaxVariation);
	}

	// Find path connections.
	bool nonSolidConnection = false;
	bool anyConnection = false;
	bool shapeHasPath = false;

	for (uint32 i = 0, shapeIdx = 0; i < shapeCount; i++, shapeIdx++) {
		// Check for adjacent power paths (but not building power paths).
		bool adjacentPath = false;
		for (uint32 dir = 0; dir < DIRECTION__COUNT; dir++) {
			const Point2I blockPos = {
				pShape[i].x + DIRECTIONS[dir].x,
				pShape[i].y + DIRECTIONS[dir].y,
			};

			if (Level_Block_IsPath(&blockPos) && !Level_Block_IsPathBuilding(&blockPos)) {
				anyConnection = true;
				adjacentPath = true;
			}
		}

		// Repeated points are non-solid Paths/Water Entrances.
		if (i+1 < shapeCount && pShape[i].x == pShape[i+1].x && pShape[i].y == pShape[i+1].y) {
			shapeHasPath = true;
			if (adjacentPath) {
				nonSolidConnection = true;
			}
			i++; // Skip repeated point.
		}
	}

	// If a shape has a non-solid path, then only non-solid paths can meet the connected power path requirement.
	const bool hasPathConnection = (shapeHasPath ? nonSolidConnection : anyConnection);

	// Confirm that we can place the shape, and setup the mesh positions/colours.
	uint32 tileIndex = 0;
	for (uint32 i = 0; i < shapeCount; i++, tileIndex++) {
		/// SANITY: Bounds checks.
		Error_Fatal(i >= SELECTPLACE_MAXSHAPETILES, "Too many tiles in shape passed to SelectPlace_CheckAndUpdate");

		bool isRepeatedPath = false;
		bool isWaterEntrance = false;
		// Repeated tiles in shape definitions are denoted as paths (or water entrances).
		if (i+1 < shapeCount && pShape[i].x == pShape[i+1].x && pShape[i].y == pShape[i+1].y) {
			isRepeatedPath = true;
			// Treat as signed int because -1 is required to define the origin tile as path.
			isWaterEntrance = (static_cast<sint32>(i) < waterEntrances + 1);
		}

		/// FIX APPLY: Honor the OnlyBuildOnPaths setting.
		// Colour individual tiles based on their requirements, and determine if we can place.
		ColourRGBF rgb = { 0.0f }; // dummy init
		if ((!hasPathConnection && Lego_IsOnlyBuildOnPaths()) ||
			!Level_CanBuildOnBlock(pShape[i].x, pShape[i].y, isRepeatedPath, isWaterEntrance) ||
			Level_BlockPointerCheck(blockPos)) // Can't or must build over tutorial-determined blocks(?)
		{
			canPlace = false;
			rgb = ColourRGBF { 0.6f, 0.0f, 0.0f }; // Red: Can't place, no path connections, bad terrain, or block pointer.
		}
		else if (isWaterEntrance) {
			rgb = ColourRGBF { 0.0f, 0.7f, 0.9f }; // Cyan: Can place water entrance.
		}
		else if (isRepeatedPath) {
			rgb = ColourRGBF { 0.7f, 0.7f, 0.0f }; // Yellow: Can place path.
		}
		else if (!okRoughness) {
			// Roughness only applies to solid tiles.
			canPlace = false;
			rgb = ColourRGBF { 0.7f, 0.0f, 0.7f }; // Magenta: Can't place solid, too rough.
		}
		else {
			rgb = ColourRGBF { 0.0f, 0.7f, 0.1f }; // Green: Can place solid.
		}
		Gods98::Container_Mesh_SetColourAlpha(selectPlace->contMesh, tileIndex,
											  rgb.red, rgb.green, rgb.blue, 0.2f);
		Gods98::Container_Mesh_SetEmissive(selectPlace->contMesh, tileIndex,
										   rgb.red * 0.5f, rgb.green * 0.5f, rgb.blue * 0.5f);


		if (!Map3D_IsInsideDimensions(map, pShape[i].x, pShape[i].y)) {
			Gods98::Container_Mesh_HideGroup(selectPlace->contMesh, tileIndex, true);
			// CHANGE: Set canPlace to false because why would we still be able to place here!?
			canPlace = false;
		}
		else {
			// First 4 vertPoses are raised tile depths, next 4 are at the original positions.
			Vector3F selectVertPoses[8];
			Map3D_GetBlockVertexPositions_NoRot(map, pShape[i].x, pShape[i].y, selectVertPoses);
			for (uint32 j = 0; j < 4; j++) {
				selectVertPoses[j + 4] = selectVertPoses[j];
				// Assign raised tile depth to first 4 vertex positions.
				selectVertPoses[j].z -= selectPlace->tileDepth; // Note: -Z is up
			}

			// Set vertex positions for individual tiles, and unhide them.
			Vertex vertices[20];
			Gods98::Container_Mesh_GetVertices(selectPlace->contMesh, tileIndex, 0, 20, vertices);
			
			// 5 faces with 4 corners per face.
			for (uint32 j = 0; j < 5; j++) {
				for (uint32 k = 0; k < 4; k++) {
					vertices[(j*4) + k].position = selectVertPoses[VERT_INDEX[(j*4) + k]];
				}
			}
			Gods98::Container_Mesh_SetVertices(selectPlace->contMesh, tileIndex, 0, 20, vertices);
			Gods98::Container_Mesh_HideGroup(selectPlace->contMesh, tileIndex, false);
		}

		// Skip the next point, if we determined it to be a path/water entrance.
		if (isRepeatedPath) {
			i++; // Skip repeated point.
		}
	}

	// Hide remaining tile meshes that aren't used.
	for (; tileIndex < SELECTPLACE_MAXSHAPETILES; tileIndex++) {
		Gods98::Container_Mesh_HideGroup(selectPlace->contMesh, tileIndex, true);
	}

	return (canPlace ? pShape : nullptr);
}

// <LegoRR.exe @004649e0>
void __cdecl LegoRR::SelectPlace_Hide(SelectPlace* selectPlace, bool32 hide)
{
	Gods98::Container_Hide(selectPlace->contMesh, hide);
}

#pragma endregion
