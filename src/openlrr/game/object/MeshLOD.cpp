// MeshLOD.cpp : 
//

#include "../../engine/core/Utils.h"

#include "MeshLOD.h"


/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @00451c70>
LegoRR::MeshLOD* __cdecl LegoRR::MeshLOD_Create(OPTIONAL MeshLOD* prevMeshLOD, const char* partName, const char* dirname, const char* meshName, uint32 setID)
{
	char fname[256];
	std::sprintf(fname, "%s\\%s", dirname, meshName);

	Gods98::Container* cont = Gods98::Container_Load(nullptr, fname, CONTAINER_LWOSTRING, true);
	if (cont == nullptr) {
		cont = Gods98::Container_Load(nullptr, fname, CONTAINER_MESHSTRING, true);
		if (cont == nullptr) {
			// Failed to load, return a null MeshLOD. Null is allowed by all MeshLOD functions,
			//  but this should be considered an internal implementation detail.
			// Functions that may not use a MeshLOD should still null check before calling functions with it.
			return nullptr;
		}
	}

	MeshLOD* newMeshLOD = static_cast<MeshLOD*>(Gods98::Mem_Alloc(sizeof(MeshLOD)));
	if (prevMeshLOD != nullptr) {
		prevMeshLOD->next = newMeshLOD;
	}
	newMeshLOD->contMeshOrigin = cont;
	newMeshLOD->contMeshTarget = nullptr;
	newMeshLOD->partName = Gods98::Util_StrCpy(partName);
	newMeshLOD->setID = setID;
	newMeshLOD->flags = MESHLOD_FLAG_MEMBLOCK;
	newMeshLOD->next = nullptr;
	return newMeshLOD;
}

// <LegoRR.exe @00451d70>
LegoRR::MeshLOD* __cdecl LegoRR::MeshLOD_CreateEmpty(OPTIONAL MeshLOD* prevMeshLOD, const char* partName, uint32 setID)
{
	MeshLOD* newMeshLOD = static_cast<MeshLOD*>(Gods98::Mem_Alloc(sizeof(MeshLOD)));
	if (prevMeshLOD != nullptr) {
		prevMeshLOD->next = newMeshLOD;
	}
	newMeshLOD->contMeshOrigin = nullptr;
	newMeshLOD->contMeshTarget = nullptr;
	newMeshLOD->partName = Gods98::Util_StrCpy(partName);
	newMeshLOD->setID = setID;
	newMeshLOD->flags = MESHLOD_FLAG_MEMBLOCK;
	newMeshLOD->next = nullptr;
	return newMeshLOD;
}

// <LegoRR.exe @00451df0>
LegoRR::MeshLOD* __cdecl LegoRR::MeshLOD_Clone(IN MeshLOD* srcMeshLOD)
{
	// Count the number of items in the source linked list.
	// With this, we'll allocate a single block of memory holding all these items.
	uint32 count = 0;
	for (MeshLOD* srcItem = srcMeshLOD; srcItem != nullptr; srcItem = srcItem->next) {
		count++;
	}
	if (count == 0) {
		// Nothing to clone, see MeshLOD_Create for info on how a null MeshLOD is handled.
		return nullptr;
	}

	// Unlike cloned MeshLODs, we can't guarantee src is a contiguous memory block, so always use next item loop.
	MeshLOD* clonedMeshLOD = static_cast<MeshLOD*>(Gods98::Mem_Alloc(count * sizeof(MeshLOD)));
	uint32 i = 0;
	for (MeshLOD* srcItem = srcMeshLOD; srcItem != nullptr; srcItem = srcItem->next) {
		clonedMeshLOD[i] = *srcItem;
		//std::memcpy(&clonedMeshLOD[i], srcItem, sizeof(MeshLOD));

		clonedMeshLOD[i].contMeshTarget = nullptr;
		clonedMeshLOD[i].flags &= ~MESHLOD_FLAG_MEMBLOCK;
		clonedMeshLOD[i].flags |= MESHLOD_FLAG_CLONED;
		clonedMeshLOD[i].next = &clonedMeshLOD[i + 1];
		i++;
	}

	// The first item is the start of the memory allocation.
	clonedMeshLOD[0].flags |= MESHLOD_FLAG_MEMBLOCK;
	// The last item should not point to any next item.
	clonedMeshLOD[count - 1].next = nullptr;

	return clonedMeshLOD;
}

// <LegoRR.exe @00451e80>
void __cdecl LegoRR::MeshLOD_SwapTarget(MeshLOD* meshLOD, Gods98::Container* contActTarget, bool32 restore, uint32 setID)
{
	for (MeshLOD* item = meshLOD; item != nullptr; item = item->next) {
		if (setID == item->setID) {
			// Lazy load (lookup) the target part container.
			if (item->contMeshTarget == nullptr) {
				const char* partName = Gods98::Container_FormatPartName(contActTarget, item->partName, nullptr);
				item->contMeshTarget = Gods98::Container_SearchTree(contActTarget, partName, Gods98::Container_SearchMode::FirstMatch, nullptr);
			}

			// Swap the part container from or back to.
			if (item->contMeshTarget != nullptr) {
				// Interestingly, it's valid for the origin parameter to be null.
				Gods98::Container_Mesh_Swap(item->contMeshTarget, item->contMeshOrigin, restore);
			}
		}
	}
}

// <LegoRR.exe @00451ef0>
void __cdecl LegoRR::MeshLOD_RemoveTargets(MeshLOD* meshLOD)
{
	for (MeshLOD* item = meshLOD; item != nullptr; item = item->next) {
		/// FIXME: Properly remove Container references that were being leaked.
		///        This isn't ready yet because it's unclear how to best handle container references.
		//if (item->contMeshTarget != nullptr) {
		//	Gods98::Container_RemoveReference(item->contMeshTarget);
		//	item->contMeshTarget = nullptr;
		//}
		item->contMeshTarget = nullptr;
	}
}

// <LegoRR.exe @00451f10>
void __cdecl LegoRR::MeshLOD_Free(MeshLOD* meshLOD)
{
	/// FIX APPLY: Properly remove Container references that were being leaked.
	MeshLOD_RemoveTargets(meshLOD);

	// The last item that was a contiguous memory block. Used to splice out non-allocations.
	MeshLOD* lastMemItem = nullptr;
	// The first item to be a contiguous memory block. Note: This should always be the passed parameter or null.
	MeshLOD* firstMemItem = nullptr;

	for (MeshLOD* item = meshLOD; item != nullptr; item = item->next) {
		// Clean up resources attached to and owned by this item.
		if (!(item->flags & MESHLOD_FLAG_CLONED)) {
			// Clones do not hold ownership over this data.
			if (item->partName != nullptr) {
				Gods98::Mem_Free(item->partName);
				item->partName = nullptr;
			}
			if (item->contMeshOrigin != nullptr) {
				Gods98::Container_Remove(item->contMeshOrigin);
				item->contMeshOrigin = nullptr;
			}
			/// FIXME: Properly remove Container references when removing source.
			/// NOTE: This won't really solve the issue since the source MeshLOD is never active,
			///        meaning it never holds links to the created references.
			/*if (item->contMeshTarget != nullptr) {
				Gods98::Container_RemoveReference(item->contMeshTarget);
				item->contMeshTarget = nullptr;
			}*/
		}

		if (!(item->flags & MESHLOD_FLAG_MEMBLOCK)) {
			// Splice out items that are not the start of a contiguous memory block.
			// We need to do this so that freeing an entire block won't remove our links to the following memory blocks.
			if (lastMemItem != nullptr) {
				lastMemItem->next = item->next;
			}
		}
		else {
			lastMemItem = item;
			if (firstMemItem == nullptr) {
				firstMemItem = item;
			}
		}
	}

	// Free all memory blocks using the spliced linked list of only memory blocks.
	// Note: This expects (meshLOD->flags & MESHLOD_FLAG_MEMBLOCK) to always be true.
	/// SANITY: Handle meshLOD parameter not being a memory allocation, but throw a fit first.
	//if (meshLOD != nullptr && (meshLOD->flags & MESHLOD_FLAG_MEMBLOCK)) {
	if (firstMemItem != nullptr) {
		Error_Fatal(meshLOD != nullptr && !(meshLOD->flags & MESHLOD_FLAG_MEMBLOCK),
					"MeshLOD_Free: meshLOD first item is not a memory block");

		MeshLOD* nextItem = nullptr;
		for (MeshLOD* item = meshLOD; item != nullptr; item = nextItem) {
			nextItem = item->next; // Store the next item before freeing the memory that links to it.
			Gods98::Mem_Free(item);
		}
	}
}

#pragma endregion
