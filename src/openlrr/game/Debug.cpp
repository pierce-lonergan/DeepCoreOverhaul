// Debug.cpp : 
//

#include "../engine/core/Maths.h"

#include "mission/Messages.h"
#include "object/Object.h"
#include "world/Map3D.h"

#include "Debug.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// (+1 for TVCamera)
static const char* _debugObjectTypeNames[] = {
	/*-1*/  "TVCamera",
	/* 0*/  "None",
	/* 1*/  "Vehicle",
	/* 2*/  "MiniFigure",
	/* 3*/  "RockMonster",
	/* 4*/  "Building",
	/* 5*/  "Boulder",
	/* 6*/  "PowerCrystal",
	/* 7*/  "Ore",
	/* 8*/  "Dynamite",
	/* 9*/  "Barrier",
	/*10*/  "UpgradePart",
	/*11*/  "ElectricFence",
	/*12*/  "SpiderWeb",
	/*13*/  "OohScary",
	/*14*/  "ElectricFenceStud",
	/*15*/  "Path",
	/*16*/  "Pusher",
	/*17*/  "Freezer",
	/*18*/  "IceCube",
	/*19*/  "LaserShot",
};

static const char* _debugRewardTypeNames[] = {
	/*0*/   "Crystals",
	/*1*/   "Ore",
	/*2*/   "Diggable",
	/*3*/   "Constructions",
	/*4*/   "Caverns",
	/*5*/   "Figures",
	/*6*/   "RockMonsters",
	/*7*/   "Oxygen",
	/*8*/   "Timer",
	/*9*/   "Score",
};

static const char* _debugContainerTypeNames[] {
	/*0*/ "Null",
	/*1*/ "Mesh",
	/*2*/ "Frame",
	/*3*/ "Anim",
	/*4*/ "FromActivity",
	/*5*/ "Light",
	/*6*/ "Reference",
	/*7*/ "LWS",
	/*8*/ "LWO",
};


static std::map<LegoRR::LegoObject*, LegoRR::Debug_RouteVisual*> _routeVisuals;
static bool _routeVisualEnabled = false;
static bool _routeVisualCompletedPathsEnabled = false;
static bool _routeVisualCurvePathsEnabled = false;
static LegoRR::Debug_RouteVisualAuto _routeVisualAutoMode = LegoRR::Debug_RouteVisualAuto::None;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

const char* LegoRR::Debug_GetObjectTypeName(LegoObject_Type objType)
{
	return _debugObjectTypeNames[(sint32)objType + 1]; // +1 for TVCamera
}

const char* LegoRR::Debug_GetObjectIDName(LegoObject_Type objType, LegoObject_ID objID)
{
	switch (objID) {
	case LegoObject_Type::LegoObject_TVCamera:
	case LegoObject_Type::LegoObject_None:
	case LegoObject_Type::LegoObject_Boulder:
	case LegoObject_Type::LegoObject_ElectricFenceStud:
	case LegoObject_Type::LegoObject_IceCube:
		return Debug_GetObjectTypeName(objType);

	case LegoObject_Type::LegoObject_Ore:
		switch (objID) {
		case LegoObject_ID::LegoObject_ID_Ore:
			return legoGlobs.langOre_name;
		case LegoObject_ID::LegoObject_ID_ProcessedOre:
			return legoGlobs.langProcessedOre_name;
		}
		break;

	case LegoObject_Type::LegoObject_Vehicle:
		return legoGlobs.langVehicle_name[objID];
	case LegoObject_Type::LegoObject_MiniFigure:
		return legoGlobs.langMiniFigure_name[objID];
	case LegoObject_Type::LegoObject_RockMonster:
		return legoGlobs.langRockMonster_name[objID];
	case LegoObject_Type::LegoObject_Building:
		return legoGlobs.langBuilding_name[objID];

	case LegoObject_Type::LegoObject_PowerCrystal:
		return legoGlobs.langPowerCrystal_name;
	case LegoObject_Type::LegoObject_Dynamite:
		return legoGlobs.langDynamite_name;
	case LegoObject_Type::LegoObject_Barrier:
		return legoGlobs.langBarrier_name;

	case LegoObject_Type::LegoObject_UpgradePart:
		// Generally this is never defined in Lego.cfg (but check anyway)
		if (legoGlobs.langUpgrade_name && legoGlobs.langUpgrade_name[objID])
			return legoGlobs.langUpgrade_name[objID];
		else
			return legoGlobs.upgradeName[objID];

	case LegoObject_Type::LegoObject_ElectricFence:
		return legoGlobs.langElectricFence_name;
	case LegoObject_Type::LegoObject_SpiderWeb:
		return legoGlobs.langSpiderWeb_name;
	case LegoObject_Type::LegoObject_OohScary:
		return legoGlobs.langOohScary_name;

	case LegoObject_Type::LegoObject_Path:
		return legoGlobs.langPath_name;
	case LegoObject_Type::LegoObject_Pusher:
		return legoGlobs.langTool_name[LegoObject_ToolType::LegoObject_ToolType_PusherGun];
	case LegoObject_Type::LegoObject_Freezer:
		return legoGlobs.langTool_name[LegoObject_ToolType::LegoObject_ToolType_FreezerGun];

	case LegoObject_Type::LegoObject_LaserShot:
		return legoGlobs.langTool_name[LegoObject_ToolType::LegoObject_ToolType_Laser];
	}
	return nullptr;
}


#pragma region RouteVisual

bool LegoRR::Debug_RouteVisual_IsEnabled()
{
	return _routeVisualEnabled;
}

void LegoRR::Debug_RouteVisual_SetEnabled(bool enabled)
{
	_routeVisualEnabled = enabled;
}

bool LegoRR::Debug_RouteVisual_IsCompletedPathsEnabled()
{
	return _routeVisualCompletedPathsEnabled;
}

void LegoRR::Debug_RouteVisual_SetCompletedPathsEnabled(bool enabled)
{
	_routeVisualCompletedPathsEnabled = enabled;
}

bool LegoRR::Debug_RouteVisual_IsCurvePathsEnabled()
{
	return _routeVisualCurvePathsEnabled;
}

void LegoRR::Debug_RouteVisual_SetCurvePathsEnabled(bool enabled)
{
	_routeVisualCurvePathsEnabled = enabled;
}

LegoRR::Debug_RouteVisualAuto LegoRR::Debug_RouteVisual_GetAutoMode()
{
	return _routeVisualAutoMode;
}

void LegoRR::Debug_RouteVisual_SetAutoMode(Debug_RouteVisualAuto autoMode)
{
	_routeVisualAutoMode = autoMode;
}


bool LegoRR::Debug_RouteVisual_CanAdd(LegoObject* liveObj, bool friendlyOnly)
{
	if (liveObj == nullptr)
		return false;

	switch (liveObj->type) {
	case LegoObject_Vehicle:
	case LegoObject_MiniFigure:
		return true;
	case LegoObject_RockMonster:
		return !friendlyOnly;
	default:
		return false;
	}
}


LegoRR::Debug_RouteVisual* LegoRR::Debug_RouteVisual_Get(LegoObject* liveObj)
{
	auto it = _routeVisuals.find(liveObj);
	if (it != _routeVisuals.end()) {
		return it->second;
	}
	return nullptr;
}

void LegoRR::_Debug_RouteVisual_Hide(Debug_RouteVisual* routeVisual, bool hide)
{
	Gods98::Container_Hide(routeVisual->contMeshLines, hide);
	Gods98::Container_Hide(routeVisual->contMeshCurve, hide);
}

bool LegoRR::_Debug_RouteVisual_IsHidden(Debug_RouteVisual* routeVisual)
{
	return Gods98::Container_IsHidden(routeVisual->contMeshLines);
}

LegoRR::Debug_RouteVisual* LegoRR::Debug_RouteVisual_Add(LegoObject* liveObj, bool autoAdd)
{
	Debug_RouteVisual* routeVisual = Debug_RouteVisual_Get(liveObj);
	if (routeVisual == nullptr) {
		routeVisual = (Debug_RouteVisual*)Gods98::Mem_Alloc(sizeof(Debug_RouteVisual));

		routeVisual->object = liveObj;
		routeVisual->autoAdded = autoAdd;

		const Gods98::Container_MeshType meshType = Gods98::Container_MeshType::Immediate; //Transparent;
		routeVisual->contMeshLines = Gods98::Container_MakeMesh2(Gods98::Container_GetRoot(), meshType);
		routeVisual->contMeshCurve = Gods98::Container_MakeMesh2(Gods98::Container_GetRoot(), meshType);

		Gods98::Container_Hide(routeVisual->contMeshLines, true);
		Gods98::Container_Hide(routeVisual->contMeshCurve, true);

		_routeVisuals[liveObj] = routeVisual;
	}
	else if (!autoAdd) {
		// If manually added, then this unit is not longer tracked by auto mode.
		routeVisual->autoAdded = autoAdd;
	}
	return routeVisual;
}

bool LegoRR::Debug_RouteVisual_Remove(LegoObject* liveObj)
{
	Debug_RouteVisual* routeVisual = Debug_RouteVisual_Get(liveObj);
	if (routeVisual != nullptr) {
		_Debug_RouteVisual_Remove2(routeVisual, true);
		return true;
	}
	return false;
}

void LegoRR::_Debug_RouteVisual_Remove2(Debug_RouteVisual* routeVisual, bool erase)
{
	LegoObject* liveObj = routeVisual->object;

	if (routeVisual->contMeshLines) {
		Gods98::Container_Remove(routeVisual->contMeshLines);
		routeVisual->contMeshLines = nullptr;
	}
	if (routeVisual->contMeshCurve) {
		Gods98::Container_Remove(routeVisual->contMeshCurve);
		routeVisual->contMeshCurve = nullptr;
	}
	Gods98::Mem_Free(routeVisual);

	if (erase) {
		_routeVisuals.erase(liveObj);
	}
}

void LegoRR::_Debug_RouteVisual_Update(Debug_RouteVisual* routeVisual, real32 elapsedWorld, real32 elapsedInterface)
{
	if (!Debug_RouteVisual_IsEnabled()) {
		_Debug_RouteVisual_Hide(routeVisual, true);
		return;
	}

	LegoObject* liveObj = routeVisual->object;

	const real32 depthLines = 6.0f;
	const real32 depthCurve = depthLines + 0.05f;
	const real32 thickness  = 0.4f;

	// Render the curve for the current block section the unit is routing.
	{
		real32 dist = 0.0f;
		uint32 groupID = 0;
		Vector3F fromPos = { 0.0f }; // dummy init
		if (Debug_RouteVisual_IsCurvePathsEnabled() && liveObj->routeBlocksTotal > 0) {
			for (uint32 i = 0; i < liveObj->routingCurve.count; i++) {
				const sint32 cmp = (dist > liveObj->routeCurveCurrDist ? 1 : -1);
				const bool visible = (cmp > 0 || Debug_RouteVisual_IsCompletedPathsEnabled());

				dist += liveObj->routingCurve.distances[i];
				const Point2F curvePoint = liveObj->routingCurve.points[i];

				Vector3F toPos = Vector3F { curvePoint.x, curvePoint.y, 0.0f };
				toPos.z = Map3D_GetWorldZ(Lego_GetMap(), toPos.x, toPos.y);
				toPos.z -= depthCurve;

				// Only render once we have a fromPos.
				if (i > 0) {
					ColourRGBF rgb;
					if (cmp < 0)
						rgb = ColourRGBF { 0.0f, 0.5f, 1.0f }; // Light Blue: Completed curve line.
					else
						rgb = ColourRGBF { 0.0f, 0.0f, 1.0f }; // Blue: Curve line in-progress.

					_Debug_RouteVisual_UpdateLine(routeVisual->contMeshCurve, groupID, thickness, &fromPos, &toPos,
												  rgb.r, rgb.g, rgb.b, 1.0f, visible);
					groupID++;
				}
				fromPos = toPos;
			}
		}
		// Hide remaining curve lines that have previously been used.
		for (; groupID < Gods98::Container_Mesh_GetGroupCount(routeVisual->contMeshCurve); groupID++) {
			Gods98::Container_Mesh_HideGroup(routeVisual->contMeshCurve, groupID, true);
		}
	}

	// Render the full route of the unit.
	{
		uint32 groupID = 0;
		Vector3F fromPos = { 0.0f }; // dummy init
		if (liveObj->routeBlocksTotal > 0) {
			for (uint32 i = 0; i < liveObj->routeBlocksTotal; i++) {
				const sint32 cmp = static_cast<sint32>(i - liveObj->routeBlocksCurrent);
				const bool visible = ((cmp >  0 || Debug_RouteVisual_IsCompletedPathsEnabled()) ||
									  (cmp == 0 && !Debug_RouteVisual_IsCurvePathsEnabled()));

				const RoutingBlock* toBlock = &liveObj->routeBlocks[i];

				Vector3F toPos = Vector3F { 0.0f };
				Map3D_BlockToWorldPos(Lego_GetMap(), toBlock->blockPos.x, toBlock->blockPos.y, &toPos.x, &toPos.y);
				toPos.x += (toBlock->blockOffset.x - 0.5f) * Lego_GetLevel()->BlockSize;
				toPos.y += (toBlock->blockOffset.y - 0.5f) * Lego_GetLevel()->BlockSize;
				toPos.z = Map3D_GetWorldZ(Lego_GetMap(), toPos.x, toPos.y);
				toPos.z -= depthLines;

				// Only render once we have a fromPos.
				if (i > 0) {
					ColourRGBF rgb;
					if (cmp < 0)
						rgb = ColourRGBF { 0.0f, 1.0f, 0.0f }; // Green: Completed.
					else if (cmp == 0)
						rgb = ColourRGBF { 1.0f, 1.0f, 0.0f }; // Yellow: In-progress.
					else
						rgb = ColourRGBF { 1.0f, 0.0f, 0.0f }; // Red: Not yet started.

					_Debug_RouteVisual_UpdateLine(routeVisual->contMeshLines, groupID, thickness, &fromPos, &toPos,
												  rgb.r, rgb.g, rgb.b, 1.0f, visible);
					groupID++;
				}
				fromPos = toPos;
			}
		}
		// Hide remaining route lines that have previously been used.
		for (; groupID < Gods98::Container_Mesh_GetGroupCount(routeVisual->contMeshLines); groupID++) {
			Gods98::Container_Mesh_HideGroup(routeVisual->contMeshLines, groupID, true);
		}
	}
	_Debug_RouteVisual_Hide(routeVisual, false);
}

void LegoRR::Debug_RouteVisual_HideAll(bool hide)
{
	for (auto& pair : _routeVisuals) {
		_Debug_RouteVisual_Hide(pair.second, hide);
	}
}

void LegoRR::Debug_RouteVisual_AddAll(bool friendlyOnly)
{
	if (Lego_IsInLevel()) {
		for (auto obj : objectListSet.EnumerateSkipUpgradeParts()) {
			if (Debug_RouteVisual_CanAdd(obj, false)) {
				Debug_RouteVisual_Add(obj, false);
			}
		}
	}
}

void LegoRR::Debug_RouteVisual_AddSelected()
{
	if (Lego_IsInLevel()) {
		uint32 count;
		LegoObject** selectedUnits = Message_GetSelectedUnits2(&count);
		for (uint32 i = 0; i < count; i++) {
			LegoObject* unit = selectedUnits[i];
			if (Debug_RouteVisual_CanAdd(unit, false)) {
				Debug_RouteVisual_Add(unit, false);
			}
		}
	}
}

void LegoRR::Debug_RouteVisual_RemoveAll()
{
	for (auto& pair : _routeVisuals) {
		_Debug_RouteVisual_Remove2(pair.second, false); // Don't erase because we're iterating.
	}
	_routeVisuals.clear();
}

void LegoRR::Debug_RouteVisual_RemoveSelected()
{
	if (Lego_IsInLevel()) {
		uint32 count;
		LegoObject** selectedUnits = Message_GetSelectedUnits2(&count);
		for (uint32 i = 0; i < count; i++) {
			LegoObject* unit = selectedUnits[i];
			Debug_RouteVisual_Remove(unit);
		}
	}
}

void LegoRR::Debug_RouteVisual_UpdateAll(real32 elapsedWorld, real32 elapsedInterface)
{
	if (!Debug_RouteVisual_IsEnabled()) {
		Debug_RouteVisual_HideAll(true);
		return;
	}

	const bool friendlyOnly = (Debug_RouteVisual_GetAutoMode() == Debug_RouteVisualAuto::AllFriendly);
	LegoObject* trackedObj = legoGlobs.cameraTrack->trackObj;

	// Auto-remove units that are no longer tracked by auto mode.
	for (auto it = _routeVisuals.begin(); it != _routeVisuals.end(); ) {
		bool remove = false;
		if (it->second->autoAdded) {
			switch (Debug_RouteVisual_GetAutoMode()) {
			case Debug_RouteVisualAuto::None:
				remove = true;
				break;
			case Debug_RouteVisualAuto::TrackedOnRadar:
				remove = (it->first != trackedObj);
				break;
			case Debug_RouteVisualAuto::AllFriendly:
				remove = !Debug_RouteVisual_CanAdd(it->first, friendlyOnly);
				break;
			}
		}
		if (remove) {
			_Debug_RouteVisual_Remove2(it->second, false); // Don't erase so we can get the new iterator.
			it = _routeVisuals.erase(it); // Get a new valid iterator.
		}
		else {
			it++;
		}
	}

	// Auto-add units that should be tracked by auto mode.
	switch (Debug_RouteVisual_GetAutoMode()) {
	case Debug_RouteVisualAuto::TrackedOnRadar:
		if (trackedObj != nullptr) {
			Debug_RouteVisual_Add(trackedObj, true);
		}
		break;
	case Debug_RouteVisualAuto::All:
	case Debug_RouteVisualAuto::AllFriendly:
		for (auto obj : objectListSet.EnumerateSkipUpgradeParts()) {
			if (Debug_RouteVisual_CanAdd(obj, friendlyOnly))
				Debug_RouteVisual_Add(obj, true);
		}
		break;
	}

	// Update tracked units.
	for (auto& pair : _routeVisuals) {
		_Debug_RouteVisual_Update(pair.second, elapsedWorld, elapsedInterface);
	}
}

// <LegoRR.exe @00470a20>
void __cdecl LegoRR::_Debug_RouteVisual_UpdateLine(Gods98::Container* contMesh, uint32 groupID, real32 thickness, const Vector3F* fromPos, const Vector3F* toPos, real32 red, real32 green, real32 blue, real32 alpha, bool visible)
{
	// Taken from: Weapon_Lazer_InitMesh <LegoRR.exe @00470a20>
	static constexpr const uint32 FACE_DATA[16*3] = {
		 0,  1,  9,
		 9,  8,  0,
		 1,  2, 10,
		10,  9,  1,
		 2,  3, 11,
		11, 10,  2,
		 3,  4, 12,
		12, 11,  3,
		 4,  5, 13,
		13, 12,  4,
		 5,  6, 14,
		14, 13,  5,
		 6,  7, 15,
		15, 14,  6,
		 7,  0,  8,
		 8, 15,  7,
	};

	static constexpr const Point2F TEXT_COORDS[16] = {
		{ 0.0f, 1.0f },
		{ 0.0f, 0.0f },
		{ 1.0f, 0.0f },
		{ 1.0f, 1.0f },
		{ 0.0f, 1.0f },
		{ 0.0f, 0.0f },
		{ 1.0f, 0.0f },
		{ 1.0f, 1.0f },
		{ 0.0f, 1.0f },
		{ 0.0f, 0.0f },
		{ 1.0f, 0.0f },
		{ 1.0f, 1.0f },
		{ 0.0f, 1.0f },
		{ 0.0f, 0.0f },
		{ 1.0f, 0.0f },
		{ 1.0f, 1.0f },
	};

	if (groupID >= Gods98::Container_Mesh_GetGroupCount(contMesh)) {
		// Add a new group if needed.
		uint32 newGroupID = Gods98::Container_Mesh_AddGroup(contMesh, 16, 16, 3, FACE_DATA);
		Error_Fatal(newGroupID != groupID, "Added mesh group ID not equal to stated group ID.");

		Gods98::Container_Mesh_SetQuality(contMesh, 0, Gods98::Container_Quality::Gouraud);
	}

	Vector3F distPos;
	Gods98::Maths_Vector3DSubtract(&distPos, toPos, fromPos);
	if (distPos.x == 0.0f && distPos.y == 0.0f && distPos.z == 0.0f) {
		// Zero distance, nothing to render.
		visible = false;
	}

	if (visible) {
		// I have absolutely no clue what this vertex position math is doing.
		// At least the variables are now labeled by axis...

		real32 local_400, local_404;
		if (distPos.y == 0.0f && distPos.z == 0.0f) {
			if (distPos.x != 0.0f) {
				Gods98::Maths_Vector3DNormalize(&distPos);
			}
			local_400 = 1.0f;
			local_404 = 0.0f;
		}
		else if (distPos.y == 0.0f) {
			Gods98::Maths_Vector3DNormalize(&distPos);
			local_400 = 0.0f;
			local_404 = 1.0f;
		}
		else {
			Gods98::Maths_Vector3DNormalize(&distPos);
			local_400 = std::sqrt(1.0f / ((distPos.z * distPos.z) / (distPos.y * distPos.y) + 1.0f));
			local_404 = -((distPos.z / distPos.y) * local_400);
		}

		const real32 x1_a = ((local_400 * distPos.y) - (local_404 * distPos.z)) * thickness;
		const real32 y1_a = -(local_400 * distPos.x) * thickness;
		const real32 z1_a = (local_404 * distPos.x) * thickness;
		const real32 y0_b = local_404 * thickness;
		const real32 z0_b = local_400 * thickness;
		const real32 x0_a = x1_a * 0.4142136f;
		const real32 y0_a = y1_a * 0.4142136f;
		const real32 z0_a = z1_a * 0.4142136f;
		const real32 y1_b = y0_b * 0.4142136f;
		const real32 z1_b = z0_b * 0.4142136f;

		const Vector3F BASE_VERTPOSES[8] = {
			{ (x0_a + 0.0f), (y0_a + y0_b), (z0_a + z0_b) },
			{ (x1_a + 0.0f), (y1_a + y1_b), (z1_a + z1_b) },
			{ (x1_a - 0.0f), (y1_a - y1_b), (z1_a - z1_b) },
			{ (x0_a - 0.0f), (y0_a - y0_b), (z0_a - z0_b) },
			{ (-x0_a - 0.0f), (-y0_a - y0_b), (-z0_a - z0_b) },
			{ (-x1_a - 0.0f), (-y1_a - y1_b), (-z1_a - z1_b) },
			{ (-x1_a + 0.0f), (-y1_a + y1_b), (-z1_a + z1_b) },
			{ (-x0_a + 0.0f), (-y0_a + y0_b), (-z0_a + z0_b) },
		};

		Vertex vertices[16] = { { 0.0f } }; // dummy init
		const Vector3F normal = { 0.0f, 0.0f, -1.0f };
		for (uint32 i = 0; i < 8; i++) {
			Gods98::Maths_Vector3DAdd(&vertices[i + 0].position, &BASE_VERTPOSES[i], fromPos);
			Gods98::Maths_Vector3DAdd(&vertices[i + 8].position, &BASE_VERTPOSES[i], toPos);
			vertices[i + 0].normal = normal;
			vertices[i + 8].normal = normal;
			vertices[i + 0].tuv = TEXT_COORDS[i + 0];
			vertices[i + 8].tuv = TEXT_COORDS[i + 8];
		}

		Gods98::Container_Mesh_SetColourAlpha(contMesh, groupID, red, green, blue, alpha);
		Gods98::Container_Mesh_SetEmissive(contMesh, groupID, red * 0.5f, green * 0.5f, blue * 0.5f);
		Gods98::Container_Mesh_SetVertices(contMesh, groupID, 0, 16, vertices);
	}
	Gods98::Container_Mesh_HideGroup(contMesh, groupID, !visible);
}

#pragma endregion

#pragma endregion
