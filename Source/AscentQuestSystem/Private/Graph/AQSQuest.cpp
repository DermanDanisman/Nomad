// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Graph/AQSQuest.h"
#include "AGSGraphNode.h"
#include "Graph/AQSBaseNode.h"
#include "Graph/AQSEdge.h"
#include "Graph/AQSObjectiveNode.h"
#include "Graph/AQSStartQuestNode.h"
#include <WorldPartition/DataLayer/DataLayerManager.h>

bool UAQSQuest::ActivateNode(class UAGSGraphNode* node)
{

    return Super::ActivateNode(node);
}

UAQSQuest::UAQSQuest()
{
    NodeType = UAQSBaseNode::StaticClass();
    EdgeType = UAQSEdge::StaticClass();
    bAllowCycles = true;
}

void UAQSQuest::CompleteQuest(bool bSucceded)
{
    OnQuestEnded.Broadcast(GetQuestTag(), bSucceded);
}

bool UAQSQuest::HasActiveObjective(const FGameplayTag& objectiveTag) const
{
    for (UAGSGraphNode* node : GetActiveNodes()) {
        UAQSObjectiveNode* objective = Cast<UAQSObjectiveNode>(node);

        if (objective && objective->GetObjectiveTag() == objectiveTag) {
            return true;
        }
    }
    return false;
}

UAQSObjectiveNode* UAQSQuest::GetActiveObjectiveNode(const FGameplayTag& objectiveTag) const
{
    for (UAGSGraphNode* node : GetActiveNodes()) {
        UAQSObjectiveNode* objective = Cast<UAQSObjectiveNode>(node);

        if (objective && objective->GetObjectiveTag() == objectiveTag) {
            return objective;
        }
    }
    return nullptr;
}

UAQSQuestObjective* UAQSQuest::GetActiveObjective(const FGameplayTag& objectiveTag) const
{
    for (const auto obj : GetActiveNodes()) {
        UAQSObjectiveNode* objNode = Cast<UAQSObjectiveNode>(obj);
        if (objNode && objNode->GetObjectiveTag() == objectiveTag) {
            return objNode->GetQuestObjective();
        }
    }
    return nullptr;
}

UAQSObjectiveNode* UAQSQuest::GetObjectiveNode(const FGameplayTag& objectiveTag) const
{
    for (const auto obj : GetAllNodes()) {
        UAQSObjectiveNode* objNode = Cast<UAQSObjectiveNode>(obj);
        if (objNode && objNode->GetObjectiveTag() == objectiveTag) {
            return objNode;
        }
    }
    return nullptr;
}

TArray<UAQSQuestObjective*> UAQSQuest::GetAllActiveObjectives() const
{
    TArray<UAQSQuestObjective*> objectives;
    for (const auto& obj : GetActiveNodes()) {
        const UAQSObjectiveNode* objNode = Cast<UAQSObjectiveNode>(obj);
        if (objNode) {
            objectives.Add(objNode->GetQuestObjective());
        } else {
            UE_LOG(LogTemp, Error, TEXT("Wrong Node types in graph! -  UAQSQuest::GetAllActiveObjectives"));
        }
    }
    return objectives;
}

UAQSQuestObjective* UAQSQuest::GetObjectiveByTag(const FGameplayTag& objectiveTag) const
{
    for (const auto obj : GetAllNodes()) {
        UAQSObjectiveNode* objNode = Cast<UAQSObjectiveNode>(obj);
        if (objNode && objNode->GetObjectiveTag() == objectiveTag) {
            return objNode->GetQuestObjective();
        }
    }
    return nullptr;
}

bool UAQSQuest::StartQuest(class APlayerController* inController, TObjectPtr<UAQSQuestManagerComponent> inQuestManager, bool bActivateChildNodes /*= true*/)
{
    controller = inController;
    questManager = inQuestManager;
    DeactivateAllNodes();
    CompletedObjectives.Empty();
    if (bActivateChildNodes) {
        for (const UAGSGraphNode* root : RootNodes) {
            const UAQSStartQuestNode* startNode = Cast<UAQSStartQuestNode>(root);
            if (startNode) {
                for (const auto& node : startNode->ChildrenNodes) {
                    UAQSObjectiveNode* objective = Cast<UAQSObjectiveNode>(node);
                    if (objective) {
                        ActivateNode(objective);
                    }
                }
                if (inController && LayerToLoad) {
                    UDataLayerManager::GetDataLayerManager(inController)->SetDataLayerRuntimeState(LayerToLoad, EDataLayerRuntimeState::Loaded);
                }

                OnQuestStarted.Broadcast(QuestTag);
                return true;
            } else {
                UE_LOG(LogTemp, Error, TEXT("No Root nodes on quest -  UAQSQuest::StartQuest"));
            }
        }
    } else {
        return true;
    }
    return false;
}

void UAQSQuest::SetQuestTracked(bool inTracked)
{
    bIsTracked = inTracked;
    for (UAGSGraphNode* node : GetActiveNodes()) {
        UAQSObjectiveNode* objective = Cast<UAQSObjectiveNode>(node);
        if (objective) {
            objective->SetIsTracked(bIsTracked);
        }
    }
}

bool UAQSQuest::CompleteBranchedObjective(const FGameplayTag& objectiveTag, const TArray<FName>& optionalTransitionFilters /*= TArray<FName>()*/)
{
    for (UAGSGraphNode* node : GetActiveNodes()) {
        UAQSObjectiveNode* objective = Cast<UAQSObjectiveNode>(node);

        if (objective && objective->GetObjectiveTag() == objectiveTag) {
            if (objective->TryToComplete()) {
                DeactivateNode(node);
                CompletedObjectives.Add(objectiveTag);
                for (auto childNode : node->ChildrenNodes) {
                    UAQSEdge* aqsEdge = Cast<UAQSEdge>(objective->GetEdge(childNode));

                    if (!aqsEdge) {
                        UE_LOG(LogTemp, Error, TEXT("Invalid Edge! -  UAQSQuest::CompleteObjective"));
                    }

                    if (optionalTransitionFilters.Num() == 0 || optionalTransitionFilters.Contains(aqsEdge->GetTransitionName())) {

                        UAQSBaseNode* aqsChild = Cast<UAQSBaseNode>(childNode);
                        if (aqsChild && aqsChild->CanBeActivated()) {
                            ActivateNode(aqsChild);
                        }
                    }
                }
                return true;
            }
        }
    }

    return false;
}

bool UAQSQuest::CompleteObjective(const FGameplayTag& objectiveTag)
{
    for (UAGSGraphNode* node : GetActiveNodes()) {
        UAQSObjectiveNode* objective = Cast<UAQSObjectiveNode>(node);

        if (objective && objective->GetObjectiveTag() == objectiveTag) {
            if (objective->TryToComplete()) {
                DeactivateNode(node);
                CompletedObjectives.Add(objectiveTag);
                for (auto childNode : node->ChildrenNodes) {
                    UAQSEdge* aqsEdge = Cast<UAQSEdge>(objective->GetEdge(childNode));
                    if (!aqsEdge) {
                        UE_LOG(LogTemp, Error, TEXT("Invalid Edge! -  UAQSQuest::CompleteObjective"));
                    }
                    UAQSBaseNode* aqsChild = Cast<UAQSBaseNode>(childNode);
                    if (aqsChild && aqsChild->CanBeActivated()) {
                        ActivateNode(aqsChild);
                    }
                }
                return true;
            }
        }
    }

    return false;
}

void UAQSQuest::ResetQuest()
{
    for (UAGSGraphNode* node : GetActiveNodes()) {
        UAQSObjectiveNode* objectiveNode = Cast<UAQSObjectiveNode>(node);

        if (objectiveNode) {
            objectiveNode->InterruptObjective();
            DeactivateNode(node);
        }
    }
}


