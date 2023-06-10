// Map3D.cpp : 
//

#include "Map3D.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004ebed0>
Point2I (& LegoRR::s_SurfaceMap_Points10)[10] = *(Point2I(*)[10])0x004ebed0;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @0044e380>
//LegoRR::Map3D* __cdecl LegoRR::Map3D_Create(Gods98::Container* root, const char* filename, real32 blockSize, real32 roughLevel);

// <LegoRR.exe @0044e790>
//void __cdecl LegoRR::Map3D_InitRoughness(Map3D* map);

// <LegoRR.exe @0044e930>
//void __cdecl LegoRR::Map3D_Remove(Map3D* map);

// <LegoRR.exe @0044e970>
//void __cdecl LegoRR::Map3D_SetTextureNoFade(Map3D* map, SurfaceTexture texture);

// <LegoRR.exe @0044e990>
//void __cdecl LegoRR::Map3D_SetBlockFadeInTexture(Map3D* map, sint32 bx, sint32 by, SurfaceTexture newTexture, uint8 direction);

// <LegoRR.exe @0044eb20>
//bool32 __cdecl LegoRR::Map3D_IsBlockMeshHidden(Map3D* map, sint32 bx, sint32 by);

// <LegoRR.exe @0044eb40>
//void __cdecl LegoRR::Map3D_UpdateAllBlockNormals(Map3D* map);

// <LegoRR.exe @0044eb80>
//bool32 __cdecl LegoRR::Map3D_CheckBuildingTolerance(Map3D* map, const Point2I* shapePoints, uint32 shapeCount, real32 buildTolerance, real32 buildMaxVariation);

// <LegoRR.exe @0044ed90>
//void __cdecl LegoRR::Map3D_FlattenShapeVertices(Map3D* map, const Point2I* shapePoints, uint32 shapeCount, real32 mult_4_0);

// <LegoRR.exe @0044f0b0>
//void __cdecl LegoRR::Map3D_SetBlockRotated(Map3D* map, uint32 bx, uint32 by, bool32 on);

// <LegoRR.exe @0044f270>
//void __cdecl LegoRR::Map3D_SetBlockVertexModified(Map3D* map, uint32 vx, uint32 vy);

// <LegoRR.exe @0044f2b0>
//void __cdecl LegoRR::Map3D_Update(Map3D* map, real32 elapsedGame);

// <LegoRR.exe @0044f350>
//void __cdecl LegoRR::Map3D_UpdateFadeInTransitions(Map3D* map, real32 elapsedGame);

// <LegoRR.exe @0044f460>
//void __cdecl LegoRR::Map3D_AddTextureMapping(Map3D* map, SurfaceTexture texA, SurfaceTexture texB);

// <LegoRR.exe @0044f4e0>
//void __cdecl LegoRR::Map3D_SetTextureSet(Map3D* map, Detail_TextureSet* tset);

// <LegoRR.exe @0044f4f0>
//void __cdecl LegoRR::Map3D_SetBlockTexture(Map3D* map, uint32 bx, uint32 by, SurfaceTexture newTexture, uint8 direction);

// <LegoRR.exe @0044f640>
//void __cdecl LegoRR::Map3D_MoveBlockVertices(Map3D* map, uint32 bx, uint32 by, real32 zDist);

// <LegoRR.exe @0044f750>
//void __cdecl LegoRR::Map3D_SetPerspectiveCorrectionAll(Map3D* map, bool32 on);

// <LegoRR.exe @0044f7a0>
//LegoRR::WallHighlightType __cdecl LegoRR::Map3D_SetBlockHighlight(Map3D* map, sint32 bx, sint32 by, WallHighlightType highlightType);

// <LegoRR.exe @0044f800>
//LegoRR::WallHighlightType __cdecl LegoRR::Map3D_GetBlockHighlight(Map3D* map, sint32 bx, sint32 by);

// <LegoRR.exe @0044f830>
//void __cdecl LegoRR::Map3D_ClearBlockHighlight(Map3D* map, sint32 bx, sint32 by);

// <LegoRR.exe @0044f880>
//void __cdecl LegoRR::Map3D_Block_SetColour(Map3D* map, sint32 bx, sint32 by, bool32 setColour, real32 r, real32 g, real32 b);

// <LegoRR.exe @0044f900>
//bool32 __cdecl LegoRR::Map3D_BlockToWorldPos(Map3D* map, uint32 bx, uint32 by, OUT real32* xPos, OUT real32* yPos);

// <LegoRR.exe @0044f990>
//bool32 __cdecl LegoRR::Map3D_WorldToBlockPos_NoZ(Map3D* map, real32 xPos, real32 yPos, OUT sint32* bx, OUT sint32* by);

// <LegoRR.exe @0044f9c0>
//bool32 __cdecl LegoRR::Map3D_WorldToBlockPos(Map3D* map, real32 xPos, real32 yPos, OUT sint32* bx, OUT sint32* by, OUT real32* unk_zPos);

// <LegoRR.exe @0044fad0>
//void __cdecl LegoRR::Map3D_FUN_0044fad0(Map3D* map, real32 xPos, real32 yPos, OUT sint32* bx, OUT sint32* by);

// <LegoRR.exe @0044fb30>
//bool32 __cdecl LegoRR::Map3D_FUN_0044fb30(Map3D* map, const Point2F* wPos2D, OPTIONAL OUT Point2F* blockPosf, OPTIONAL OUT Point2F* centerDir);

// <LegoRR.exe @0044fc00>
//real32 __cdecl LegoRR::Map3D_GetWorldZ(LegoRR::Map3D* map, real32 xPos, real32 yPos);

// <LegoRR.exe @0044fd70>
//void __cdecl LegoRR::Map3D_FUN_0044fd70(Map3D* map, real32 xPos, real32 yPos, OUT Vector3F* vector);

// <LegoRR.exe @0044fe50>
//bool32 __cdecl LegoRR::Map3D_FUN_0044fe50(Map3D* map, real32 xPos, real32 yPos, bool32 diagonal, real32 unkMultiplier, OUT real32* xOut, OUT real32* yOut);

// <LegoRR.exe @00450130>
//real32 __cdecl LegoRR::Map3D_UnkCameraXYFunc_RetZunk(Map3D* map, real32 xPos, real32 yPos);

// <LegoRR.exe @00450320>
//void __cdecl LegoRR::Map3D_GetBlockFirstVertexPosition(Map3D* map, sint32 vx, sint32 vy, OUT Vector3F* vector);

// <LegoRR.exe @00450390>
//bool32 __cdecl LegoRR::Map3D_GetBlockVertexPositions(Map3D* map, uint32 bx, uint32 by, OUT Vector3F* vertPoses);

// <LegoRR.exe @004504e0>
//void __cdecl LegoRR::Map3D_GetBlockVertexPositions_NoRot(Map3D* map, uint32 bx, uint32 by, OUT Vector3F* vertPoses);

// <LegoRR.exe @00450580>
//bool32 __cdecl LegoRR::Map3D_IsInsideDimensions(Map3D* map, uint32 bx, uint32 by);

// <LegoRR.exe @004505a0>
//bool32 __cdecl LegoRR::Map3D_GetIntersections(Map3D* map, Gods98::Viewport* view, uint32 mouseX, uint32 mouseY, OUT uint32* bx, OUT uint32* by, OUT Vector3F* vector);

// <LegoRR.exe @00450820>
//bool32 __cdecl LegoRR::Map3D_Intersections_Sub1_FUN_00450820(Map3D* map, const Vector3F* rayOrigin, const Vector3F* ray, OUT Vector3F* endPoint, OUT Point2I* blockPos, sint32 unkCount);

// <LegoRR.exe @004508c0>
//void __cdecl LegoRR::Map3D_AddVisibleBlocksInRadius_AndDoCallbacks(Map3D* map, sint32 bx, sint32 by, sint32 radius, OPTIONAL XYCallback callback);

// <LegoRR.exe @004509c0>
//void __cdecl LegoRR::Map3D_HideBlock(Map3D* map, sint32 bx, sint32 by, bool32 hide);

// <LegoRR.exe @004509f0>
//void __cdecl LegoRR::Map3D_AddVisibleBlock(Map3D* map, sint32 bx, sint32 by);

// <LegoRR.exe @00450a40>
//void __cdecl LegoRR::Map3D_HideVisibleBlocksList(Map3D* map);

// <LegoRR.exe @00450a90>
//void __cdecl LegoRR::Map3D_BlockVertexToWorldPos(Map3D* map, uint32 bx, uint32 by, OUT real32* xPos, OUT real32* yPos, OUT real32* zPos);

// <LegoRR.exe @00450b50>
//real32 __cdecl LegoRR::Map3D_BlockSize(Map3D* map);

// <LegoRR.exe @00450b60>
//sint32 __cdecl LegoRR::Map3D_CheckRoutingComparison_FUN_00450b60(sint32 param_1, sint32 param_2, sint32 param_3, sint32 param_4);

// <LegoRR.exe @00450c20>
//void __cdecl LegoRR::Map3D_SetBlockUVWobbles(LegoRR::Map3D* map, uint32 bx, uint32 by, bool32 on);

// <LegoRR.exe @00450d40>
//void __cdecl LegoRR::Map3D_SetEmissive(Map3D* map, bool32 on);

// <LegoRR.exe @00450e20>
//void __cdecl LegoRR::Map3D_UpdateTextureUVs(Map3D* map, real32 elapsedGame);

// <LegoRR.exe @004511f0>
//void __cdecl LegoRR::Map3D_UpdateBlockNormals(Map3D* map, uint32 bx, uint32 by);

// <LegoRR.exe @00451440>
//bool32 __cdecl LegoRR::Map3D_BlockPairHasTextureMatch(Map3D* map, uint32 bx1, uint32 by1, uint32 bx2, uint32 by2);

// <LegoRR.exe @004514f0>
//void __cdecl LegoRR::Map3D_SetBlockDirectionNormal(Map3D* map, uint32 bx, uint32 by, Direction direction, const Vector3F* normal);

// <LegoRR.exe @00451590>
//bool32 __cdecl LegoRR::Map3D_GetBlockDirectionNormal(Map3D* map, uint32 bx, uint32 by, Direction direction, OUT Vector3F* normal);

// <LegoRR.exe @00451710>
//void __cdecl LegoRR::Map3D_MoveBlockDirectionVertex(Map3D* map, uint32 bx, uint32 by, Direction direction, const Vector3F* vertDist);

// <LegoRR.exe @004517b0>
//void __cdecl LegoRR::Map3D_GenerateBlockPlaneNormals(Map3D* map, uint32 bx, uint32 by);

// <LegoRR.exe @00451860>
//void __cdecl LegoRR::Map3D_MapFileGetSpecs(const MapFileInfo* mapFileInfo, OUT uint32* width, OUT uint32* height);

// <LegoRR.exe @00451880>
//uint16 __cdecl LegoRR::Map3D_MapFileBlockValue(const MapFileInfo* mapFile, uint32 bx, uint32 by, uint32 gridWidth);

// <LegoRR.exe @004518a0>
//bool32 __cdecl LegoRR::Map3D_Intersections_Sub2_FUN_004518a0(Map3D* map, uint32 bx, uint32 by, const Vector3F* rayOrigin, const Vector3F* ray, OUT Vector3F* vector);

#pragma endregion
