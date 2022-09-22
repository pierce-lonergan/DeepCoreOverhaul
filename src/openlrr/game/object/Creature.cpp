// Creature.cpp : 
//

#include "MeshLOD.h"

#include "Creature.h"


/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

/// CUSTOM: Properly cleanup Container reference items.
void LegoRR::_Creature_RemoveNulls(CreatureModel* creature)
{
	/// FIXME: Properly remove Container references that were being leaked.
	if (creature->drillNull) {
		//Gods98::Container_RemoveReference(creature->drillNull);
		creature->drillNull = nullptr;
	}
	if (creature->footStepNull) {
		//Gods98::Container_RemoveReference(creature->footStepNull);
		creature->footStepNull = nullptr;
	}
	if (creature->carryNull) {
		//Gods98::Container_RemoveReference(creature->carryNull);
		creature->carryNull = nullptr;
	}
	if (creature->throwNull) {
		//Gods98::Container_RemoveReference(creature->throwNull);
		creature->throwNull = nullptr;
	}
	if (creature->depositNull) {
		//Gods98::Container_RemoveReference(creature->depositNull);
		creature->depositNull = nullptr;
	}

	for (uint32 i = 0; i < CREATURE_MAXCAMERAS; i++) {
		if (creature->cameraNulls[i]) {
			//Gods98::Container_RemoveReference(creature->cameraNulls[i]);
			creature->cameraNulls[i] = nullptr;
		}
	}
}



// Merged function: Object_Hide
// <LegoRR.exe @00406bc0>
void __cdecl LegoRR::Creature_Hide(CreatureModel* creature, bool32 hide)
{
	Gods98::Container_Hide(creature->contAct, hide);
}

// Merged function: Object_GetActivityContainer
// <LegoRR.exe @00406d60>
Gods98::Container* __cdecl LegoRR::Creature_GetActivityContainer(CreatureModel* creature)
{
	return creature->contAct;
}

// Merged function: Object_FindNull
// <LegoRR.exe @00406e80>
Gods98::Container* __cdecl LegoRR::Creature_FindNull(CreatureModel* creature, const char* name, uint32 frameNo)
{
	const char* partName = Gods98::Container_FormatPartName(creature->contAct, name, &frameNo);
	return Gods98::Container_SearchTree(creature->contAct, partName, Gods98::Container_SearchMode::FirstMatch, nullptr);
}

// Merged function: Object_SetOwnerObject
// <LegoRR.exe @004082b0>
void __cdecl LegoRR::Creature_SetOwnerObject(CreatureModel* creature, LegoObject* liveObj)
{
	Gods98::Container_SetUserData(creature->contAct, liveObj);
}

// Merged function: Object_IsHidden
// <LegoRR.exe @004085d0>
bool32 __cdecl LegoRR::Creature_IsHidden(CreatureModel* creature)
{
	return Gods98::Container_IsHidden(creature->contAct);
}

// Merged function: NERPFunc__True
// <LegoRR.exe @00484e50>
uint32 __cdecl LegoRR::Creature_GetCarryNullFrames(CreatureModel* creature)
{
	return 1;
}

// <missing>
uint32 __cdecl LegoRR::Creature_GetCameraNullFrames(CreatureModel* creature)
{
	return creature->cameraNullFrames;
}



// <LegoRR.exe @004068b0>
bool32 __cdecl LegoRR::Creature_IsCameraFlipDir(CreatureModel* creature)
{
	return creature->cameraFlipDir == BoolTri::BOOL3_TRUE;
}

// <LegoRR.exe @004068c0>
//bool32 __cdecl LegoRR::Creature_Load(OUT CreatureModel* creature, LegoObject_ID objID, Gods98::Container* root, const char* filename, const char* gameName);

// <LegoRR.exe @00406b30>
void __cdecl LegoRR::Creature_SwapPolyMedium(CreatureModel* creature, bool32 swap)
{
	MeshLOD_SwapTarget(creature->polyMedium, creature->contAct, !swap, 0);
}

// <LegoRR.exe @00406b60>
void __cdecl LegoRR::Creature_SwapPolyHigh(CreatureModel* creature, bool32 swap)
{
	MeshLOD_SwapTarget(creature->polyHigh, creature->contAct, !swap, 0);
}

// <LegoRR.exe @00406b90>
void __cdecl LegoRR::Creature_SwapPolyFP(CreatureModel* creature, bool32 swap, uint32 cameraNo)
{
	MeshLOD_SwapTarget(creature->polyFP, creature->contAct, !swap, cameraNo);
}



// <LegoRR.exe @00406be0>
void __cdecl LegoRR::Creature_Clone(IN CreatureModel* srcCreature, OUT CreatureModel* destCreature)
{
	// Directly copy data from source model.
	*destCreature = *srcCreature;

	// Dissociate nulls from source model.
	/// FIX APPLY: We don't want to be referencing the nulls of another container.
	destCreature->drillNull    = nullptr;
	destCreature->footStepNull = nullptr;
	destCreature->carryNull    = nullptr;
	destCreature->throwNull    = nullptr;
	destCreature->depositNull  = nullptr;

	for (uint32 i = 0; i < CREATURE_MAXCAMERAS; i++) {
		destCreature->cameraNulls[i] = nullptr;
	}

	// This is a cloned model, and doesn't own string memory allocations.
	destCreature->flags &= ~CREATURE_FLAG_SOURCE;

	// Clone MeshLODs.
	destCreature->polyMedium = MeshLOD_Clone(srcCreature->polyMedium);
	destCreature->polyHigh   = MeshLOD_Clone(srcCreature->polyHigh);
	destCreature->polyFP     = MeshLOD_Clone(srcCreature->polyFP);

	// Clone activity containers.
	destCreature->contAct = Gods98::Container_Clone(srcCreature->contAct);
}

// <LegoRR.exe @00406c40>
void __cdecl LegoRR::Creature_SetAnimationTime(CreatureModel* creature, real32 time)
{
	Gods98::Container_SetAnimationTime(creature->contAct, time);
}

// <LegoRR.exe @00406c60>
real32 __cdecl LegoRR::Creature_MoveAnimation(CreatureModel* creature, real32 elapsed, uint32 repeatCount)
{
	real32 overrun = Gods98::Container_MoveAnimation(creature->contAct, elapsed);
	if (repeatCount > 1 && overrun != 0.0f) {
		const uint32 animFrames = Gods98::Container_GetAnimationFrames(creature->contAct);
		overrun -= static_cast<real32>(animFrames * (repeatCount - 1));
	}
	return overrun;
}

// <LegoRR.exe @00406cd0>
real32 __cdecl LegoRR::Creature_GetAnimationTime(CreatureModel* creature)
{
	return Gods98::Container_GetAnimationTime(creature->contAct);
}

// <LegoRR.exe @00406cf0>
void __cdecl LegoRR::Creature_SetOrientation(CreatureModel* creature, real32 xDir, real32 yDir)
{
	Gods98::Container_SetOrientation(creature->contAct, nullptr, xDir, yDir, 0.0f, 0.0f, 0.0f, -1.0f);
}

// <LegoRR.exe @00406d20>
void __cdecl LegoRR::Creature_SetPosition(CreatureModel* creature, real32 xPos, real32 yPos, GetWorldZCallback zCallback, Map3D* map)
{
	const real32 zPos = zCallback(xPos, yPos, map);
	Gods98::Container_SetPosition(creature->contAct, nullptr, xPos, yPos, zPos);
}

// <LegoRR.exe @00406d70>
bool32 __cdecl LegoRR::Creature_SetActivity(CreatureModel* creature, const char* activityName, real32 elapsed)
{
	MeshLOD_RemoveTargets(creature->polyMedium);
	MeshLOD_RemoveTargets(creature->polyHigh);
	MeshLOD_RemoveTargets(creature->polyFP);

	/// FIX APPLY: Remove Container references that were being leaked after every SetActivity.
	_Creature_RemoveNulls(creature);
	
	bool success = Gods98::Container_SetActivity(creature->contAct, activityName);
	Gods98::Container_SetAnimationTime(creature->contAct, elapsed);
	return success;
}

// <LegoRR.exe @00406df0>
void __cdecl LegoRR::Creature_Remove(CreatureModel* creature)
{
	// Remove MeshLODs.
	MeshLOD_Free(creature->polyMedium);
	MeshLOD_Free(creature->polyHigh);
	MeshLOD_Free(creature->polyFP);
	creature->polyMedium = nullptr;
	creature->polyHigh   = nullptr;
	creature->polyFP     = nullptr;

	// Remove source string allocations.
	if (creature->flags & CREATURE_FLAG_SOURCE) {
		if (creature->drillNullName) {
			Gods98::Mem_Free(creature->drillNullName);
			creature->drillNullName = nullptr;
		}
		if (creature->footStepNullName) {
			Gods98::Mem_Free(creature->footStepNullName);
			creature->footStepNullName = nullptr;
		}
		if (creature->carryNullName) {
			Gods98::Mem_Free(creature->carryNullName);
			creature->carryNullName = nullptr;
		}
		if (creature->throwNullName) {
			Gods98::Mem_Free(creature->throwNullName);
			creature->throwNullName = nullptr;
		}
		if (creature->depositNullName) {
			Gods98::Mem_Free(creature->depositNullName);
			creature->depositNullName = nullptr;
		}

		if (creature->cameraNullName) {
			Gods98::Mem_Free(creature->cameraNullName);
			creature->cameraNullName = nullptr;
		}
	}

	// Remove null containers.
	/// FIX APPLY: Properly remove Container references that were being leaked.
	_Creature_RemoveNulls(creature);

	// Remove activity containers.
	Gods98::Container_Remove(creature->contAct);
	creature->contAct = nullptr;
}


// <LegoRR.exe @00406eb0>
Gods98::Container* __cdecl LegoRR::Creature_GetCameraNull(CreatureModel* creature, uint32 frameNo)
{
	if (creature->cameraNullName != nullptr) {
		if (creature->cameraNulls[frameNo] == nullptr) {
			creature->cameraNulls[frameNo] = Creature_FindNull(creature, creature->cameraNullName, frameNo);
		}
		return creature->cameraNulls[frameNo];
	}
	return nullptr;
}

// <LegoRR.exe @00406ee0>
Gods98::Container* __cdecl LegoRR::Creature_GetDrillNull(CreatureModel* creature)
{
	/// CHANGE: Uniform behaviour by null checking drillNullName before returning drillNull.
	///         This *shouldn't* have any side effects unless drillNull can be assigned elsewhere.
	if (creature->drillNullName != nullptr) {
		if (creature->drillNull == nullptr) {
			creature->drillNull = Creature_FindNull(creature, creature->drillNullName, 0);
		}
		return creature->drillNull;
	}
	return nullptr;
}

// <missing>
Gods98::Container* __cdecl LegoRR::Creature_GetFootStepNull(CreatureModel* creature)
{
	if (creature->footStepNullName != nullptr) {
		if (creature->footStepNull == nullptr) {
			creature->footStepNull = Creature_FindNull(creature, creature->footStepNullName, 0);
		}
		return creature->footStepNull;
	}
	return nullptr;
}

// <LegoRR.exe @00406f10>
Gods98::Container* __cdecl LegoRR::Creature_GetCarryNull(CreatureModel* creature)
{
	if (creature->carryNullName != nullptr) {
		if (creature->carryNull == nullptr) {
			creature->carryNull = Creature_FindNull(creature, creature->carryNullName, 0);
		}
		return creature->carryNull;
	}
	return nullptr;
}

// <inlined>
Gods98::Container* __cdecl LegoRR::Creature_GetThrowNull(CreatureModel* creature)
{
	if (creature->throwNullName != nullptr) {
		if (creature->throwNull == nullptr) {
			creature->throwNull = Creature_FindNull(creature, creature->throwNullName, 0);
		}
		return creature->throwNull;
	}
	return nullptr;
}

// <LegoRR.exe @00406f40>
Gods98::Container* __cdecl LegoRR::Creature_GetDepositNull(CreatureModel* creature)
{
	if (creature->depositNullName != nullptr) {
		if (creature->depositNull == nullptr) {
			creature->depositNull = Creature_FindNull(creature, creature->depositNullName, 0);
		}
		return creature->depositNull;
	}
	return nullptr;
}

// Checks if the model has a throw null, and performs initialisation for the null if not found.
// 
// A more correct name would be Creature_HasThrowNull, but the fact that this performs initialisation is still awkward to convey.
// <LegoRR.exe @00406f70>
bool32 __cdecl LegoRR::Creature_CheckThrowNull(CreatureModel* creature)
{
	Gods98::Container* throwNull = Creature_GetThrowNull(creature);
	if (throwNull != nullptr) {
		return (Gods98::Container_GetZXRatio(throwNull) != 1.0f);
	}
	return false;
}

// <LegoRR.exe @00406fc0>
real32 __cdecl LegoRR::Creature_GetTransCoef(CreatureModel* creature)
{
	return Gods98::Container_GetTransCoef(creature->contAct);
}

#pragma endregion
