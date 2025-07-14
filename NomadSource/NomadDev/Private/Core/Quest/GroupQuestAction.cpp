// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.


#include "Core/Quest/GroupQuestAction.h"
#include "AQSQuestFunctionLibrary.h"
#include "AQSQuestManagerComponent.h"
#include "GameFramework/PlayerController.h"

void UGroupQuestAction::ExecuteAction_Implementation(APlayerController* PlayerController, UAGSGraphNode* NodeOwner)
{
    // Ensure the player controller is valid before proceeding
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("GroupQuestAction: Invalid PlayerController."));
        return;
    }

    // Get the global quest manager associated with this player
    UAQSQuestManagerComponent* GlobalManager = UAQSQuestFunctionLibrary::GetGlobalQuestManager(PlayerController);
    if (!IsValid(GlobalManager))
    {
        UE_LOG(LogTemp, Warning, TEXT("GroupQuestAction: Invalid GlobalManager."));
        return;
    }

    // Retrieve the quest using the specified QuestTag
    UAQSQuest* GlobalQuest = GlobalManager->GetQuest(QuestTag);
    if (!IsValid(GlobalQuest))
    {
        UE_LOG(LogTemp, Warning, TEXT("GroupQuestAction: Quest not found for tag: %s"), *QuestTag.ToString());
        return;
    }

    // Check if the quest is a group quest and the objective is not already completed
    if (GlobalQuest->GetGroupQuest() && !GlobalManager->IsObjectiveCompletedByTag(QuestTag, ObjectiveTag))
    {
        UAQSQuestObjective* Objective = GlobalQuest->GetObjectiveByTag(ObjectiveTag);
        if (IsValid(Objective))
        {
            int32 Repetitions = Objective->GetRepetitions();

            // Complete the objective for the required number of repetitions
            for (int32 i = 0; i < Repetitions; ++i)
            {
                GlobalManager->ServerCompleteObjective(ObjectiveTag);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("GroupQuestAction: Objective not found for tag: %s"), *ObjectiveTag.ToString());
        }
    }
}