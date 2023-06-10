// AITask.cpp : 
//

#include "../../engine/Main.h"

#include "Object.h"
#include "AITask.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @004b41c8>
LegoRR::AITask_Globs & LegoRR::aiGlobs = *(LegoRR::AITask_Globs*)0x004b41c8;

LegoRR::AITask_ListSet LegoRR::aiListSet = LegoRR::AITask_ListSet(LegoRR::aiGlobs);

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

/// BETA: RockFall
// <LegoStripped.exe @00402723>
const char* __cdecl LegoRR::AITask_GetTaskName(const AITask* aiTask)
{
	return aiGlobs.taskName[aiTask->taskType];
}

// When turned on, AITasks will not update during `AITask_UpdateAll`. Used for Editor Mode.
/// BETA: RockFall
// <LegoStripped.exe @00402791>
void __cdecl LegoRR::AITask_DisableTaskUpdates(BOOL disabled)
{
	if (disabled) aiGlobs.flags |= AITASK_GLOB_FLAG_DISABLEUPDATES;
	else          aiGlobs.flags &= ~AITASK_GLOB_FLAG_DISABLEUPDATES;
}


// <LegoRR.exe @00401bf0>
bool32 __cdecl LegoRR::AIPriority_GetType(const char* priorityName, OUT AI_Priority* priorityType)
{
	for (uint32 i = 0; i < AI_Priority_Count; i++) {
		if (::_stricmp(aiGlobs.priorityName[i], priorityName) == 0) {
			*priorityType = (AI_Priority)i;
			return true;
		}
	}
	return false;
}

// <LegoRR.exe @00401c30>
void __cdecl LegoRR::AITask_Initialise(void)
{
	AITask_RegisterName(AITask_Type_Goto);
	AITask_RegisterName(AITask_Type_Follow);
	AITask_RegisterName(AITask_Type_FollowAttack);
	AITask_RegisterName(AITask_Type_Collect);
	AITask_RegisterName(AITask_Type_Gather);
	AITask_RegisterName(AITask_Type_Deposit);
	AITask_RegisterName(AITask_Type_Dump);
	AITask_RegisterName(AITask_Type_Request);
	AITask_RegisterName(AITask_Type_Dig);
	AITask_RegisterName(AITask_Type_Dynamite);
	AITask_RegisterName(AITask_Type_Repair);
	AITask_RegisterName(AITask_Type_Reinforce);
	AITask_RegisterName(AITask_Type_Clear);
	AITask_RegisterName(AITask_Type_Wait);
	AITask_RegisterName(AITask_Type_AnimationWait);
	AITask_RegisterName(AITask_Type_ElecFence);
	AITask_RegisterName(AITask_Type_Eat);
	AITask_RegisterName(AITask_Type_GotoEat);
	AITask_RegisterName(AITask_Type_FindDriver);
	AITask_RegisterName(AITask_Type_GetTool);
	AITask_RegisterName(AITask_Type_BirdScarer);
	AITask_RegisterName(AITask_Type_Upgrade);
	AITask_RegisterName(AITask_Type_BuildPath);
	AITask_RegisterName(AITask_Type_Train);
	AITask_RegisterName(AITask_Type_Depart);
	AITask_RegisterName(AITask_Type_AttackPath);
	AITask_RegisterName(AITask_Type_AttackRockMonster);
	AITask_RegisterName(AITask_Type_Recharge);
	AITask_RegisterName(AITask_Type_Dock);
	AITask_RegisterName(AITask_Type_AttackObject);
	AITask_RegisterName(AITask_Type_FindLoad);

	AIPriority_RegisterName(AI_Priority_Crystal);
	AIPriority_RegisterName(AI_Priority_Ore);
	AIPriority_RegisterName(AI_Priority_DefaultCollect);
	AIPriority_RegisterName(AI_Priority_Destruction);
	AIPriority_RegisterName(AI_Priority_Construction);
	AIPriority_RegisterName(AI_Priority_Request);
	AIPriority_RegisterName(AI_Priority_Reinforce);
	AIPriority_RegisterName(AI_Priority_Repair);
	AIPriority_RegisterName(AI_Priority_Clearing);
	AIPriority_RegisterName(AI_Priority_Storage);
	AIPriority_RegisterName(AI_Priority_Refining);
	AIPriority_RegisterName(AI_Priority_HealthLow);
	AIPriority_RegisterName(AI_Priority_GetIn);
	AIPriority_RegisterName(AI_Priority_Upgrade);
	AIPriority_RegisterName(AI_Priority_BuildPath);
	AIPriority_RegisterName(AI_Priority_AttackRockMonster);
	AIPriority_RegisterName(AI_Priority_Barrier);
	AIPriority_RegisterName(AI_Priority_FindLoad);
	AIPriority_RegisterName(AI_Priority_Recharge);
	AIPriority_RegisterName(AI_Priority_UpgradeBuilding);
	AIPriority_RegisterName(AI_Priority_Gather);
	AIPriority_RegisterName(AI_Priority_Steal);
	AIPriority_RegisterName(AI_Priority_Punch);
	AIPriority_RegisterName(AI_Priority_Depart);
	AIPriority_RegisterName(AI_Priority_AttackPath);
	AIPriority_RegisterName(AI_Priority_AttackObject);
	AIPriority_RegisterName(AI_Priority_Congregate);


	// Assign default priority value (of `49`) to all tasks before assigning individual values.
	for (uint32 i = 0; i < AI_Priority_Count; i++) {
		aiGlobs.priorityValues[i] = 49;
	}

	aiGlobs.priorityValues[AI_Priority_Crystal]           = 55;

	aiGlobs.priorityValues[AI_Priority_Destruction]       = 40;
	aiGlobs.priorityValues[AI_Priority_Construction]      = 60;
	aiGlobs.priorityValues[AI_Priority_Request]           = 20;
	aiGlobs.priorityValues[AI_Priority_Reinforce]         = 70;

	aiGlobs.priorityValues[AI_Priority_Clearing]          = 20;
	aiGlobs.priorityValues[AI_Priority_Storage]           =  5;
	aiGlobs.priorityValues[AI_Priority_Refining]          = 15;

	aiGlobs.priorityValues[AI_Priority_AttackRockMonster] = 56;

	aiGlobs.priorityValues[AI_Priority_UpgradeBuilding]   = 99;
	aiGlobs.priorityValues[AI_Priority_Gather]            = 49;
	aiGlobs.priorityValues[AI_Priority_Steal]             = 90;
	aiGlobs.priorityValues[AI_Priority_Punch]             = 49;
	aiGlobs.priorityValues[AI_Priority_Depart]            =  0;
	aiGlobs.priorityValues[AI_Priority_AttackPath]        = 20;
	aiGlobs.priorityValues[AI_Priority_AttackObject]      = 30;
	aiGlobs.priorityValues[AI_Priority_Congregate]        = 99;


	aiListSet.Initialise();
	aiGlobs.flags = AITASK_GLOB_FLAG_INITIALISED;
}

// <LegoRR.exe @00401f40>
//void __cdecl LegoRR::AITask_FUN_00401f40(AITask_Type taskType, LegoObject* liveObj, OPTIONAL const Point2I* blockPos);

// <LegoRR.exe @00401fa0>
//void __cdecl LegoRR::AITask_SetPriorityType(AITask* aiTask, AI_Priority priorityType);

// <LegoRR.exe @00401fd0>
//void __cdecl LegoRR::AITask_Game_SetPriorityValue(AI_Priority priorityType, uint32 aiTaskValue);

// <LegoRR.exe @00401fe0>
//void __cdecl LegoRR::AITask_Game_SetPriorityOff(AI_Priority priorityType, bool32 off);

// <LegoRR.exe @00401ff0>
//bool32 __cdecl LegoRR::AITask_Game_IsPriorityOff(AI_Priority priorityType);

// <LegoRR.exe @00402000>
void __cdecl LegoRR::AITask_Shutdown(void)
{
	aiListSet.Shutdown();
	aiGlobs.flags = AITASK_GLOB_FLAG_NONE;
}

// <LegoRR.exe @00402040>
//void __cdecl LegoRR::AITask_CleanupLevel(bool32 end);

// <LegoRR.exe @004020b0>
//void __cdecl LegoRR::AITask_Game_SetNoGather(bool32 noGather);

// <LegoRR.exe @00402120>
//bool32 __cdecl LegoRR::AITask_Callback_Remove(AITask* aiTask);

// <LegoRR.exe @00402150>
//void __cdecl LegoRR::AITask_UpdateAll(real32 elapsedGame);

// <LegoRR.exe @00402240>
//void __cdecl LegoRR::AITask_FUN_00402240(IN OUT AITask** aiTask);

// <LegoRR.exe @00402440>
//void __cdecl LegoRR::AITask_DoDig_AtBlockPos(const Point2I* blockPos, bool32 param_2, bool32 param_3);

// <LegoRR.exe @00402530>
//void __cdecl LegoRR::AITask_UnkInitRouting_FUN_00402530(AITask* aiTask, bool32 dropCarried);

// <LegoRR.exe @004025a0>
//void __cdecl LegoRR::AITask_DoAttackRockMonster_Target(LegoObject* targetObj);

// <LegoRR.exe @004025f0>
//void __cdecl LegoRR::AITask_LiveObject_FUN_004025f0(LegoObject* liveObj);

// <LegoRR.exe @00402630>
//bool32 __cdecl LegoRR::AITask_IsCollectAndSameTarget(AITask* aiTask, LegoObject* liveObj);

// <LegoRR.exe @00402650>
//void __cdecl LegoRR::AITask_DoCollect(LegoObject* liveObj, real32 param_2);

// <LegoRR.exe @004026d0>
//void __cdecl LegoRR::AITask_DoBuildPath_AtPosition(const Point2I* blockPos);

// <LegoRR.exe @00402730>
//void __cdecl LegoRR::AITask_DoGather_Count(sint32 count);

// <LegoRR.exe @00402780>
//void __cdecl LegoRR::AITask_DoCollect_Target(LegoObject* targetObj);

// <LegoRR.exe @004027c0>
//void __cdecl LegoRR::AITask_DoTrain_Target(LegoObject* targetObj, LegoObject_AbilityFlags trainFlags, bool32 param_3);

// <LegoRR.exe @00402890>
//void __cdecl LegoRR::AITask_DoFindDriver_Target(LegoObject* targetObj);

// <LegoRR.exe @00402970>
//bool32 __cdecl LegoRR::AITask_RemoveAttackPathReferences(const Point2I* blockPos);

// <LegoRR.exe @004029a0>
//bool32 __cdecl LegoRR::AITask_Callback_RemoveAttackPathReference(AITask* aiTask, void* pBlockPos);

// <LegoRR.exe @004029d0>
//void __cdecl LegoRR::AITask_RemoveAttackRockMonsterReferences(LegoObject* targetObj);

// <LegoRR.exe @004029f0>
//bool32 __cdecl LegoRR::AITask_Callback_RemoveAttackRockMonsterReference(AITask* aiTask, void* targetObj);

// <LegoRR.exe @00402a10>
//void __cdecl LegoRR::AITask_RemoveWallReferences(const Point2I* blockPos, bool32 digConnection);

// <LegoRR.exe @00402a60>
//bool32 __cdecl LegoRR::AITask_RemoveReinforceReferences(const Point2I* blockPos);

// <LegoRR.exe @00402a90>
//bool32 __cdecl LegoRR::AITask_DelayOrRemoveClearReferences(const Point2I* blockPos, bool32 delay);

// <LegoRR.exe @00402ae0>
//bool32 __cdecl LegoRR::AITask_Callback_RemoveWallReference(AITask* aiTask, void* search);

// <LegoRR.exe @00402b50>
//bool32 __cdecl LegoRR::AITask_Callback_RemoveReinforceReference(AITask* aiTask, void* search);

// <LegoRR.exe @00402ba0>
//bool32 __cdecl LegoRR::AITask_Callback_RemoveClearReference(AITask* aiTask, void* pBlockPos);

// <LegoRR.exe @00402bd0>
//bool32 __cdecl LegoRR::AITask_Callback_DelayClearReference(AITask* aiTask, void* pBlockPos);

// <LegoRR.exe @00402c00>
void __cdecl LegoRR::AITask_RemoveGetToolReferences(AITask* aiTask)
{
	if (aiTask->taskType == AITask_Type_GetTool) {
		if (aiTask->getToolTask != nullptr) {
			/// FIXME: This is USELESS USELESS USELESS USELESS! Passing the address of this function's
			///        parameter on the stack when it's trying to compare against field getToolTask!!!

			// Now for the real treat. THIS FUNCTION DOES NOTHING ANYWAY.
			// The callback returns true if the fields match, but outside of that.
			// ABSOLUTELY NOTHING is done with the result.
			AITask_RunThroughLists(AITask_Callback_FindGetToolReference, &aiTask);
		}
	}
	else {
		if (aiTask->getToolTask != nullptr) {

			// I'm going to assume this is a list of child tasks, that aren't managed by themselves or something(?)
			AITask* aiTaskUnk = aiGlobs.pendingTaskList; // Linked list for something.
			while (aiTaskUnk != nullptr) {

				if ((aiTaskUnk->taskType == AITask_Type_GetTool) && (aiTask->getToolTask == aiTaskUnk)) {
					aiTaskUnk->flags |= AITASK_FLAG_REMOVING; // Mark for removal.
					// No break after finding a match.... Why??
					// Having a duplicate task in the linked list can *potentially* cause infinite loops.
				}
				aiTaskUnk = aiTaskUnk->next;
			}
		}
	}
}

// <LegoRR.exe @00402c60>
//bool32 __cdecl LegoRR::AITask_Callback_FindGetToolReference(AITask* aiTask, void* otherTask);

// <LegoRR.exe @00402c80>
//bool32 __cdecl LegoRR::AITask_DoDynamite_AtBlockPos(const Point2I* blockPos, bool32 noRoutingUnk);

// <LegoRR.exe @00402d70>
//void __cdecl LegoRR::AITask_DoBirdScarer_AtPosition(const Point2I* blockPos);

// <LegoRR.exe @00402df0>
//void __cdecl LegoRR::AITask_DoFindLoad(LegoObject* targetObj);

// <LegoRR.exe @00402e60>
//void __cdecl LegoRR::AITask_DoRepair_Target(LegoObject* targetObj, bool32 condition);

// <LegoRR.exe @00402ea0>
//void __cdecl LegoRR::AITask_QueueGotoEat_Target(LegoObject* liveObj, LegoObject* targetObj);

// <LegoRR.exe @00402ee0>
//void __cdecl LegoRR::AITask_DoElecFence(const Point2I* blockPos);

// <LegoRR.exe @00402f60>
//void __cdecl LegoRR::AITask_DoReinforce_AtBlockPos(const Point2I* blockPos);

// <LegoRR.exe @00402fe0>
//void __cdecl LegoRR::AITask_DoClear_AtPosition(const Point2I* blockPos, Message_Type completeAction);

// <LegoRR.exe @00403010>
//void __cdecl LegoRR::AITask_DoGetTool_FromTask(AITask* aiTask);

// <LegoRR.exe @004030c0>
//bool32 __cdecl LegoRR::AITask_DoGetTool(LegoObject_ToolType toolType);

// <LegoRR.exe @00403110>
//bool32 __cdecl LegoRR::AITask_Game_SelectedUnits_UnkEquippedTool_FUN_00403110(LegoObject_ToolType toolType);

// <LegoRR.exe @004031a0>
//void __cdecl LegoRR::AITask_QueueGotoBlock_Group(LegoObject** unitList, uint32 unitCount, const Point2I* blockPos, bool32 reevalTasks);

// <LegoRR.exe @00403250>
//void __cdecl LegoRR::AITask_DoUpgrade(LegoObject* liveObj, sint32 newObjLevel);

// <LegoRR.exe @004032f0>
//void __cdecl LegoRR::AITask_DoDock(LegoObject* liveObj);

// <LegoRR.exe @00403320>
//void __cdecl LegoRR::AITask_DoGoto_Congregate(const Point2I* blockPos);

// <LegoRR.exe @00403360>
//void __cdecl LegoRR::AITask_StopGoto_Congregate(const Point2I* blockPos);

// <LegoRR.exe @004033d0>
//void __cdecl LegoRR::AITask_DoAttackObject(LegoObject* liveObj);

// <LegoRR.exe @00403410>
//void __cdecl LegoRR::AITask_DoAttackPath(Point2I* blockPos);

// <LegoRR.exe @00403450>
//void __cdecl LegoRR::AITask_DoRepair(LegoObject* liveObj);

// <LegoRR.exe @00403490>
//void __cdecl LegoRR::AITask_StopRepairForObject(LegoObject* liveObj);

// <LegoRR.exe @004034f0>
//void __cdecl LegoRR::AITask_Game_PTL_GotoOrRMGoto(LegoObject* liveObj, const Point2I* blockPos, OPTIONAL AITask* referrerTask);

// <LegoRR.exe @00403540>
//void __cdecl LegoRR::AITask_QueueDepositInObject(LegoObject* liveObj, LegoObject* targetObj);

// <LegoRR.exe @00403580>
//void __cdecl LegoRR::AITask_PushFollowObject_Group(LegoObject** unitList, uint32 unitCount, LegoObject* targetObj);

// <LegoRR.exe @004035f0>
//void __cdecl LegoRR::AITask_Game_PTL_FollowAttack(LegoObject* liveObj, LegoObject* targetObj);

// <LegoRR.exe @00403630>
//void __cdecl LegoRR::AITask_Construction_FUN_00403630(uint32 constructHandle, bool32 checkObjType, LegoObject_Type objType, LegoObject_ID objID);

// <LegoRR.exe @00403680>
//void __cdecl LegoRR::AITask_DoRequest_ObjectType(LegoObject_Type objType, LegoObject_ID objID, sint32 objLevel, const Point2I* blockPos, undefined4 param_5, Point2F* pointFloat, bool32 param_7);

// <LegoRR.exe @00403740>
//void __cdecl LegoRR::AITask_PlaceRequestObject(const Point2I* blockPos, LegoObject_Type objType, LegoObject_ID objID, sint32 objLevel);

// <LegoRR.exe @004037f0>
//LegoRR::AITask* __cdecl LegoRR::AITask_DoDeposit_ObjectType(LegoObject* liveObj, LegoObject_Type objType, LegoObject_ID objID, sint32 objLevel);

// <LegoRR.exe @00403840>
//void __cdecl LegoRR::AITask_DoRequestObjectCallbacks(LegoObject* liveObj);

// <LegoRR.exe @004038d0>
//void __cdecl LegoRR::AITask_LiveObject_FUN_004038d0(LegoObject* liveObj);

// <LegoRR.exe @00403900>
//void __cdecl LegoRR::AITask_DoGotoEat(LegoObject* liveObj);

// <LegoRR.exe @00403940>
//bool32 __cdecl LegoRR::AITask_LiveObject_FUN_00403940(LegoObject* liveObj);

// <LegoRR.exe @00403960>
//bool32 __cdecl LegoRR::AITask_Callback_FUN_00403960(AITask* aiTask, LegoObject* liveObj);

// <LegoRR.exe @00403980>
//void __cdecl LegoRR::AITask_RemoveTargetObjectReferences(LegoObject* targetObj);

// <LegoRR.exe @00403a00>
//bool32 __cdecl LegoRR::AITask_Callback_RemoveTargetObjectReference(AITask* aiTask, void* targetObj);

// <LegoRR.exe @00403a20>
//void __cdecl LegoRR::AITask_Route_End(LegoObject* liveObj, bool32 completed);

// <LegoRR.exe @00403a70>
//void __cdecl LegoRR::AITask_LiveObject_FUN_00403a70(LegoObject* liveObj);

// <LegoRR.exe @00403a90>
//void __cdecl LegoRR::AITask_VariousGatherTasks_FUN_00403a90(LegoObject* liveObj);

// <LegoRR.exe @00403b30>
//void __cdecl LegoRR::AITask_LiveObject_FUN_00403b30(LegoObject* holderObj, AITask_Type taskType, LegoObject* carriedObj);

// <LegoRR.exe @00403c40>
//void __cdecl LegoRR::AITask_LiveObject_SetAITaskUnk(LegoObject* liveObj1, AITask_Type taskType, LegoObject* liveObj2, bool32 param_4);

// <LegoRR.exe @00403e20>
//bool32 __cdecl LegoRR::AITask_RemoveCollectReferences(LegoObject* targetObj);

// <LegoRR.exe @00403e40>
//bool32 __cdecl LegoRR::AITask_Callback_RemoveCollectReference(AITask* aiTask, void* targetObj);

// <LegoRR.exe @00403e60>
//void __cdecl LegoRR::AITask_DoAnimationWait(LegoObject* liveObj);

// <LegoRR.exe @00403e90>
//void __cdecl LegoRR::AITask_LiveObject_Unk_UpdateAITask_AnimationWait(LegoObject* liveObj);

// <LegoRR.exe @00403eb0>
//sint32 __cdecl LegoRR::AITask_QSortCompare(const void* task_a, const void* task_b);

// <LegoRR.exe @00403f60>
//LegoRR::AITask* __cdecl LegoRR::AITask_InitTask_1(AITask* aiTask, AI_Priority priorityType);

// <LegoRR.exe @00403fa0>
//void __cdecl LegoRR::AITask_Init_2_NoPriority(AITask* aiTask);

// <LegoRR.exe @00403fd0>
//void __cdecl LegoRR::AITask_Game_UnkLiveObjectHandleDynamite(LegoObject* liveObj);

// <LegoRR.exe @00404030>
//void __cdecl LegoRR::AITask_ReevaluateObjectTasks(LegoObject* liveObj);

// <LegoRR.exe @004040c0>
//LegoRR::AITask* __cdecl LegoRR::AITask_GetObjectTaskListTail(LegoObject* liveObj);

// <LegoRR.exe @004040e0>
//bool32 __cdecl LegoRR::AITask_LiveObject_IsCurrentTaskType(LegoObject* liveObj, AITask_Type taskType);

// <LegoRR.exe @00404110>
//void __cdecl LegoRR::AITask_LiveObject_FUN_00404110(LegoObject* liveObj);

// <LegoRR.exe @00404180>
//bool32 __cdecl LegoRR::AITask_Callback_UpdateTask(AITask* aiTask, void* pElapsedGame);

// <LegoRR.exe @004041b0>
//bool32 __cdecl LegoRR::AITask_Callback_UpdateObject(LegoObject* liveObj, void* pElapsedGame);

// <LegoRR.exe @00404d30>
//bool32 __cdecl LegoRR::AITask_LiveObject_FUN_00404d30(LegoObject* liveObj, const Point2I* blockPos2, real32* param_3);

// <LegoRR.exe @00404e00>
//bool32 __cdecl LegoRR::AITask_LiveObject_FUN_00404e00(LegoObject* liveObj);

// <LegoRR.exe @00404e40>
//bool32 __cdecl LegoRR::AITask_FUN_00404e40(LegoObject_Type objType, LegoObject_ID objID, sint32 objLevel);

// <LegoRR.exe @00404e80>
//bool32 __cdecl LegoRR::AITask_Callback_FUN_00404e80(AITask* aiTask, sint32* param_2);

// <LegoRR.exe @00404ef0>
//bool32 __cdecl LegoRR::AITask_FUN_00404ef0(AITask* aiTask, LegoObject* liveObj_2, real32* param_3, real32* param_4, undefined4* param_5, sint32 param_6, sint32 param_7);

// <LegoRR.exe @00405880>
//void __cdecl LegoRR::AITask_FUN_00405880(void);

// <LegoRR.exe @00405b40>
//void __cdecl LegoRR::AITask_FUN_00405b40(void);

// <LegoRR.exe @00406290>
//void __cdecl LegoRR::AITask_FUN_00406290(AITask* aiTask1, AITask* aiTask2, LegoObject* liveObj);

// <LegoRR.exe @00406310>
//void __cdecl LegoRR::AITask_RemoveAssignedToObjectReferences(LegoObject* assignedToObj);

// <LegoRR.exe @00406330>
LegoRR::AITask* __cdecl LegoRR::AITask_Clone(AITask* aiTask)
{
	AITask* newAITask = AITask_Create(aiTask->taskType);

	ListSet::MemCopy(newAITask, aiTask);

	newAITask->flags |= AITASK_FLAG_CLONED;
	newAITask->next = nullptr;

	return newAITask;
}

// <LegoRR.exe @00406370>
LegoRR::AITask* __cdecl LegoRR::AITask_Create(AITask_Type taskType)
{
	AITask* newAITask = aiListSet.Add();
	ListSet::MemZero(newAITask);

	newAITask->creationTime = Gods98::Main_GetTime();
	newAITask->taskType = taskType;

	return newAITask;
}

// <LegoRR.exe @004063b0>
void __cdecl LegoRR::AITask_Remove(AITask* deadTask, bool32 levelCleanup)
{
	if (!levelCleanup) {
		// Extra removal behaviour that should NOT be called during level cleanup.
		AITask_RemoveGetToolReferences(deadTask);
	}

	// Free all dynamically allocated memory housed in this structure (as long as we're not a clone).
	if (deadTask->unitList != nullptr && !(deadTask->flags & AITASK_FLAG_CLONED)) {
		Gods98::Mem_Free(deadTask->unitList);
	}

	aiListSet.Remove(deadTask);
}

// <LegoRR.exe @004063f0>
void __cdecl LegoRR::AITask_AddList(void)
{
	// NOTE: This function is no longer called, aiListSet.Add already does so.
	aiListSet.AddList();
}

// <LegoRR.exe @00406470>
bool32 __cdecl LegoRR::AITask_RunThroughLists(AITask_RunThroughListsCallback callback, void* data)
{
	for (AITask* task : aiListSet.EnumerateAlive()) {
		if (callback(task, data))
			return true;
	}
	return false;
}

#pragma endregion
