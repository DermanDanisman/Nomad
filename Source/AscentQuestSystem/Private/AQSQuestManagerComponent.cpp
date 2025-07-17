// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "AQSQuestManagerComponent.h"
#include "AQSQuestTargetComponent.h"
#include "AQSTypes.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Graph/AQSQuest.h"
#include "Net/UnrealNetwork.h"
#include <Containers/Map.h>
#include <Kismet/GameplayStatics.h>

// Sets default values for this component's properties
UAQSQuestManagerComponent::UAQSQuestManagerComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);

    // ...
}

void UAQSQuestManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UAQSQuestManagerComponent, CompletedQuestsTags);
    DOREPLIFETIME(UAQSQuestManagerComponent, FailedQuestsTags);
    DOREPLIFETIME(UAQSQuestManagerComponent, InProgressQuestsRecords);
    DOREPLIFETIME(UAQSQuestManagerComponent, TrackedQuestTag);
}
// Called when the game starts
void UAQSQuestManagerComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UAQSQuestManagerComponent::OnComponentSaved_Implementation()
{
    //     CompletedQuestsTags.Empty();
    //     FailedQuestsTags.Empty();
    //     InProgressQuestsRecords.Empty();
    //
    //     if (TrackedQuest) {
    //         TrackedQuestTag = TrackedQuest->GetQuestTag();
    //     }
    //
    //     for (const auto quest : CompletedQuests) {
    //         CompletedQuestsTags.Add(quest->GetQuestTag());
    //     }
    //
    //     for (const auto quest : FailedQuests) {
    //         FailedQuestsTags.Add(quest->GetQuestTag());
    //     }
    //
    //     for (const auto quest : InProgressQuests) {
    //         InProgressQuestsRecords.Add(FAQSQuestRecord(quest));
    //     }
}

void UAQSQuestManagerComponent::OnComponentLoaded_Implementation()
{
    SyncGraphs();
}

void UAQSQuestManagerComponent::ServerCompleteObjective_Implementation(const FGameplayTag& objectiveToComplete)
{
    CompleteObjective(objectiveToComplete);
}

void UAQSQuestManagerComponent::ServerCompleteBranchedObjective_Implementation(const FGameplayTag& objectiveToComplete, const TArray<FName>& optionalTransitionFilters)
{
    CompleteBranchedObjective(objectiveToComplete, optionalTransitionFilters);
}

void UAQSQuestManagerComponent::ServerStartQuest_Implementation(const FGameplayTag& questTag)
{
    StartQuest(questTag);
}

void UAQSQuestManagerComponent::ServerRemoveInProgressQuest_Implementation(const FGameplayTag& questTag)
{
    UAQSQuest* quest = GetQuestFromDB(questTag);
    RemoveInProgressQuest(quest);
}

bool UAQSQuestManagerComponent::StartQuest(const FGameplayTag& questTag)
{
    UAQSQuest* questToStart = GetQuestFromDB(questTag);
    const bool bStartChildNodes = true;
    const bool bStarted = Internal_StartQuest(questToStart, bStartChildNodes, bAutoTrackQuest);
    if (bStarted) {
        OnQuestStarted.Broadcast(questTag);

        // *** EDITED by Nomad Dev Team: Added client notification for listen server and standalone ***
        // Reason: In listen server setups (non-dedicated), the original condition misses notifying clients.
        // This ensures clients receive quest start notifications when running on listen servers.
        if (GetOwnerRole() == ROLE_Authority && !IsLocallyControlled())
        {
            ClientDispatchQuestStarted(questTag);
        }
    }
    return bStarted;
}

bool UAQSQuestManagerComponent::StopQuest(const FGameplayTag& questTag)
{
    if (!IsQuestInProgressByTag(questTag)) {
        return false;
    }

    const FAQSQuestRecord* questRec = InProgressQuestsRecords.FindByKey(questTag);

    if (!questRec) {
        return false;
    }
    UAQSQuest* questToStop = GetQuestFromDB(questTag);
    if (InProgressQuests.Contains(questToStop)) {

        questToStop->ResetQuest();

        InProgressQuests.Remove(questToStop);

        if (TrackedQuestTag == questTag) {
            UntrackCurrentQuest();
        }
        InProgressQuestsRecords.Remove(FAQSQuestRecord(questToStop));

        questToStop->OnQuestEnded.RemoveDynamic(this, &UAQSQuestManagerComponent::HandleQuestCompleted);

        questToStop->OnObjectiveStarted.RemoveDynamic(this, &UAQSQuestManagerComponent::HandleObjectiveStarted);
        questToStop->OnObjectiveCompleted.RemoveDynamic(this, &UAQSQuestManagerComponent::HandleObjectiveCompleted);
        questToStop->OnObjectiveUpdated.RemoveDynamic(this, &UAQSQuestManagerComponent::HandleObjectiveUpdated);

        OnQuestEnded.Broadcast(questTag, false);
        if (UKismetSystemLibrary::IsDedicatedServer(this) && !UKismetSystemLibrary::IsStandalone(this)) {
            ClientDispatchQuestCompleted(questTag, false);
        }
        return true;
    }

    return false;
}

bool UAQSQuestManagerComponent::Internal_StartQuest(UAQSQuest* questToStart, const bool bStartChildNodes, bool autoTrack)
{
    if (!questToStart) {
        return false;
    }
    if (IsQuestInProgress(questToStart) || CompletedQuestsTags.Contains(questToStart->GetQuestTag()) || FailedQuestsTags.Contains(questToStart->GetQuestTag())) {
        UE_LOG(LogTemp, Warning, TEXT("Quest is already Started!- UAQSQuestManagerComponent::StartQuest"));
        return false;
    }
    APlayerController* playerController = Cast<APlayerController>(GetOwner());

    ensure(playerController);
    if (questToStart && questToStart->StartQuest(playerController, this, bStartChildNodes)) {
        InProgressQuests.Add(questToStart);
        InProgressQuestsRecords.AddUnique(FAQSQuestRecord(questToStart));

        if (!questToStart->OnQuestEnded.IsAlreadyBound(this, &UAQSQuestManagerComponent::HandleQuestCompleted)) {
            questToStart->OnQuestEnded.AddDynamic(this, &UAQSQuestManagerComponent::HandleQuestCompleted);
        }
        if (!questToStart->OnObjectiveStarted.IsAlreadyBound(this, &UAQSQuestManagerComponent::HandleObjectiveStarted)) {
            questToStart->OnObjectiveStarted.AddDynamic(this, &UAQSQuestManagerComponent::HandleObjectiveStarted);
        }
        if (!questToStart->OnObjectiveCompleted.IsAlreadyBound(this, &UAQSQuestManagerComponent::HandleObjectiveCompleted)) {
            questToStart->OnObjectiveCompleted.AddDynamic(this, &UAQSQuestManagerComponent::HandleObjectiveCompleted);
        }
        if (!questToStart->OnObjectiveUpdated.IsAlreadyBound(this, &UAQSQuestManagerComponent::HandleObjectiveUpdated)) {
            questToStart->OnObjectiveUpdated.AddDynamic(this, &UAQSQuestManagerComponent::HandleObjectiveUpdated);
        }

        if (!TrackedQuest && autoTrack) {
            TrackInProgressQuest(questToStart);
        }

        return true;
    } else {
        UE_LOG(LogTemp, Warning, TEXT("Invalid quest - UAQSQuestManagerComponent::StartQuest"));
    }
    return false;
}

bool UAQSQuestManagerComponent::CompleteObjective(FGameplayTag objectiveToComplete)
{
    for (UAQSQuest* quest : InProgressQuests) {
        if (quest->HasActiveObjective(objectiveToComplete)) {
            const bool bCompleted = quest->CompleteObjective(objectiveToComplete);
            return bCompleted;
        }
    }
    return false;
}

TArray<UAQSQuestObjective*> UAQSQuestManagerComponent::GetCurrentlyTrackedQuestObjectives() const
{
    if (TrackedQuest) {
        return TrackedQuest->GetAllActiveObjectives();
    }
    return TArray<UAQSQuestObjective*>();
}

TArray<UAQSQuestTargetComponent*> UAQSQuestManagerComponent::GetCurrentlyTrackedQuestObjectivesTargets() const
{
    const TArray<UAQSQuestObjective*> objectives = GetCurrentlyTrackedQuestObjectives();
    TArray<UAQSQuestTargetComponent*> targets;

    for (const UAQSQuestObjective* obj : objectives) {
        targets.Append(obj->GetObjectiveTargets());
    }
    return targets;
}

TArray<AActor*> UAQSQuestManagerComponent::GetCurrentlyTrackedQuestObjectivesTargetActors() const
{
    const TArray<UAQSQuestObjective*> objectives = GetCurrentlyTrackedQuestObjectives();
    TArray<AActor*> targets;

    for (const UAQSQuestObjective* obj : objectives) {
        targets.Append(obj->GetObjectiveTargetsActors());
    }
    return targets;
}

bool UAQSQuestManagerComponent::IsObjectiveInProgress(const FGameplayTag& objectiveTag) const
{
    for (const auto& quest : InProgressQuestsRecords) {
        if (DoesQuestHaveInProgressObjective(quest, objectiveTag)) {
            return true;
        }
    }
    return false;
}

class UAQSQuest* UAQSQuestManagerComponent::GetQuestFromDB(const FGameplayTag& questTag)
{
    UAQSQuest* newQuest = GetQuest(questTag);
    if (newQuest) {
        return newQuest;
    }
    if (!QuestsDB) {
        UE_LOG(LogTemp, Error, TEXT("Missing Quests Database from Quest Manager! - UAQSQuestManagerComponent::GetQuestFromDB"));
        return nullptr;
    }

    for (const auto it : QuestsDB->GetRowMap()) {
        FAQSQuestData* questStruct = (FAQSQuestData*)(it.Value);
        if (!questStruct) {
            break;
        }
        UAQSQuest* quest = questStruct->Quest;
        if (!quest) {
            continue;
        }
        if (quest->GetQuestTag() == questTag) {
            newQuest = DuplicateObject(quest, GetOuter());
            loadedQuests.Add(questTag, newQuest);
            return newQuest;
        }
    }
    return nullptr;
}

class UAQSQuest* UAQSQuestManagerComponent::GetQuest(const FGameplayTag& questTag) const
{
    if (loadedQuests.Contains(questTag)) {
        return loadedQuests.FindChecked(questTag);
    }
    return nullptr;
}

class UAQSQuestObjective* UAQSQuestManagerComponent::GetQuestObjectiveFromDB(const FGameplayTag& objectiveTag, const FGameplayTag& questTag) const
{
    const UAQSQuest* quest = GetQuest(questTag);
    if (quest) {
        return quest->GetObjectiveByTag(objectiveTag);
    }
    return nullptr;
}

bool UAQSQuestManagerComponent::TryGetInProgressObjectiveInfo(const FGameplayTag& questTag, const FGameplayTag& objectiveTag, FAQSObjectiveInfo& outObjectiveInfo) const
{
    if (!IsQuestInProgressByTag(questTag) || !IsObjectiveInProgress(objectiveTag)) {
        return false;
    }

    const FAQSQuestRecord* questRec = InProgressQuestsRecords.FindByKey(questTag);

    if (!questRec) {
        return false;
    }

    const FAQSObjectiveRecord* objectiveRec = questRec->Objectives.FindByKey(objectiveTag);
    if (!objectiveRec) {
        return false;
    }

    const UAQSQuest* quest = GetQuest(questTag);
    if (quest) {
        const UAQSObjectiveNode* node = quest->GetActiveObjectiveNode(objectiveTag);
        if (node) {
            const UAQSQuestObjective* objective = node->GetQuestObjective();
            if (objective) {
                outObjectiveInfo = FAQSObjectiveInfo(objective, *objectiveRec);
                return true;
            }
        }
    }
    return false;
}

bool UAQSQuestManagerComponent::TryGetInProgressQuestInfo(const FGameplayTag& questTag, FAQSQuestInfo& outQuestInfo) const
{
    if (!IsQuestInProgressByTag(questTag)) {
        return false;
    }

    const FAQSQuestRecord* questRec = InProgressQuestsRecords.FindByKey(questTag);

    if (!questRec) {
        return false;
    }

    const UAQSQuest* quest = GetQuest(questTag);
    if (quest) {
        outQuestInfo = FAQSQuestInfo(quest, *questRec);
        return true;
    }

    return false;
}

bool UAQSQuestManagerComponent::TryGetTargetActorsForInProegressObjective(const FGameplayTag& objectiveTag, const FGameplayTag& questTag, TArray<class AActor*>& outTargets)
{
    UAQSQuestObjective* objective = GetQuestObjectiveFromDB(objectiveTag, questTag);

    if (!objective) {
        return false;
    }

    outTargets = objective->GetObjectiveTargetsActors();
    return IsObjectiveInProgress(objectiveTag);
}

bool UAQSQuestManagerComponent::IsObjectiveCompletedByTag(const FGameplayTag& questTag, const FGameplayTag& objectiveTag) const
{
    if (IsQuestCompletedByTag(questTag)) {
        return true;
    } else if (IsQuestInProgressByTag(questTag)) {
        UAQSQuest* quest = GetQuest(questTag);
        if (quest) {
            return quest->IsObjectiveCompleted(objectiveTag);
        }
    }

    return false;
}

void UAQSQuestManagerComponent::GetInProgressQuestsInfo(TArray<FAQSQuestInfo>& outInfo) const
{
    for (const auto& quest : InProgressQuestsRecords) {
        FAQSQuestInfo questInfo;
        if (TryGetInProgressQuestInfo(quest.Quest, questInfo)) {
            outInfo.Add(questInfo);
        }
    }
}

bool UAQSQuestManagerComponent::CompleteBranchedObjective(FGameplayTag objectiveToComplete, TArray<FName> optionalTransitionFilters)
{
    for (UAQSQuest* quest : InProgressQuests) {
        if (quest->HasActiveObjective(objectiveToComplete)) {
            const bool bCompleted = quest->CompleteBranchedObjective(objectiveToComplete, optionalTransitionFilters);
            return bCompleted;
        }
    }
    return false;
}

void UAQSQuestManagerComponent::TrackInProgressQuestByTag(const FGameplayTag& questTag)
{
    UAQSQuest* quest = GetQuestFromDB(questTag);
    if (quest) {
        TrackInProgressQuest(quest);
    }
}

bool UAQSQuestManagerComponent::TrackInProgressQuest(class UAQSQuest* questToTrack)
{
    if (InProgressQuests.Contains(questToTrack)) {
        UntrackCurrentQuest();
        TrackedQuest = questToTrack;
        TrackedQuestTag = questToTrack->GetQuestTag();
        TrackedQuest->SetQuestTracked(true);
        OnTrackedQuestChanged.Broadcast();
        return true;
    }

    return false;
}

void UAQSQuestManagerComponent::UntrackCurrentQuest()
{
    if (TrackedQuest) {
        TrackedQuestTag = FGameplayTag();
        TrackedQuest->SetQuestTracked(false);
        TrackedQuest = nullptr;
        OnTrackedQuestChanged.Broadcast();
    }
}

TArray<class UAQSQuestTargetComponent*> UAQSQuestManagerComponent::GetAllTargetsWithTag(const FGameplayTag& targetTag) const
{
    TArray<UAQSQuestTargetComponent*> outArray;
    QuestTargets.MultiFind(targetTag, outArray);
    return outArray;
}

TArray<class UAQSQuestTargetComponent*> UAQSQuestManagerComponent::GetAllTargetsWithTags(const TArray<FGameplayTag>& targetTags) const
{
    TArray<UAQSQuestTargetComponent*> outArray;
    for (const auto& tag : targetTags) {
        outArray.Append(GetAllTargetsWithTag(tag));
    }

    return outArray;
}

void UAQSQuestManagerComponent::RegisterTarget(UAQSQuestTargetComponent* targetComp)
{
    if (targetComp && targetComp->GetTargetTag() != FGameplayTag()) {
        QuestTargets.AddUnique(targetComp->GetTargetTag(), targetComp);
    }
}

void UAQSQuestManagerComponent::UnregisterTarget(class UAQSQuestTargetComponent* targetComp)
{
    if (targetComp) {
        QuestTargets.RemoveSingle(targetComp->GetTargetTag(), targetComp);
    }
}

void UAQSQuestManagerComponent::DispatchObjectiveUpdate(const FGameplayTag& objectiveTag, const FGameplayTag& questTag, EQuestUpdateType updateType)
{
    const UAQSQuest* quest = GetQuestFromDB(questTag);
    if (!quest) {
        return;
    }
    const UAQSQuestObjective* questObjective = quest->GetActiveObjective(objectiveTag);

    if (!questObjective) {
        return;
    }

    OnInProgressQuestsUpdate.Broadcast();

    bool bIsTracked = false;
    if (GetCurrentlyTrackedQuestTag() == questTag) {
        bIsTracked = true;
        OnTrackedQuestUpdated.Broadcast();
    }

    switch (updateType) {
    case EQuestUpdateType::EStarted:

        OnObjectiveStarted.Broadcast(questObjective->GetObjectiveTag(), questTag);
        break;
    case EQuestUpdateType::ECompleted:

        OnObjectiveCompleted.Broadcast(questObjective->GetObjectiveTag(), questTag);
        break;
    case EQuestUpdateType::EUpdated:

        OnObjectiveUpdated.Broadcast(questObjective->GetObjectiveTag(), questTag);
        break;
    case EQuestUpdateType::EInterrupted:
        OnObjectiveInterrupted.Broadcast(questObjective->GetObjectiveTag(), questTag);
        break;
    }
    TArray<UAQSQuestTargetComponent*> targetComps = questObjective->GetObjectiveTargets();
    for (UAQSQuestTargetComponent* targetComp : targetComps) {

        if (targetComp) {
            targetComp->DispatchObjectiveUpdated(objectiveTag, questTag, updateType, bIsTracked);
        } else {
            UE_LOG(LogTemp, Error, TEXT("Quest References without AQSTargetActorComponent! - UAQSObjectiveNode::UpdateReferences"));
        }
    }
}

void UAQSQuestManagerComponent::ServerDispatchObjectiveUpdate_Implementation(const FGameplayTag& objectiveTag, const FGameplayTag& questTag, EQuestUpdateType updateType)
{
    const UAQSQuest* quest = GetQuestFromDB(questTag);
    if (!quest) {
        return;
    }
    const UAQSQuestObjective* questObjective = quest->GetActiveObjective(objectiveTag);

    if (!questObjective) {
        return;
    }

    if (InProgressQuestsRecords.Contains(questTag)) {
        InProgressQuestsRecords.Remove(FAQSQuestRecord(quest));
    }
    InProgressQuestsRecords.AddUnique(FAQSQuestRecord(quest));
    OnInProgressQuestsUpdate.Broadcast();

    switch (updateType) {
    case EQuestUpdateType::EStarted:
        quest->OnObjectiveStarted.Broadcast(questObjective->GetObjectiveTag(), questTag);
        OnObjectiveStarted.Broadcast(questObjective->GetObjectiveTag(), questTag);
        break;
    case EQuestUpdateType::ECompleted:
        quest->OnObjectiveCompleted.Broadcast(questObjective->GetObjectiveTag(), questTag);
        OnObjectiveCompleted.Broadcast(questObjective->GetObjectiveTag(), questTag);
        break;
    case EQuestUpdateType::EUpdated:
        quest->OnObjectiveUpdated.Broadcast(questObjective->GetObjectiveTag(), questTag);
        OnObjectiveUpdated.Broadcast(questObjective->GetObjectiveTag(), questTag);
        break;
    case EQuestUpdateType::EInterrupted:
        quest->OnObjectiveInterrupted.Broadcast(questObjective->GetObjectiveTag(), questTag);
        OnObjectiveInterrupted.Broadcast(questObjective->GetObjectiveTag(), questTag);
        break;
    }

    if (bTeamQuests) {
        ClientsDispatchObjectiveUpdate(objectiveTag, questTag, updateType);
    } else {
        // DispatchObjectiveUpdate(objectiveTag, questTag, updateType);
        ClientDispatchObjectiveUpdate(objectiveTag, questTag, updateType);
    }
}

void UAQSQuestManagerComponent::ClientDispatchObjectiveUpdate_Implementation(const FGameplayTag& objectiveTag, const FGameplayTag& questTag, EQuestUpdateType updateType)
{
    DispatchObjectiveUpdate(objectiveTag, questTag, updateType);
}

void UAQSQuestManagerComponent::ClientDispatchQuestStarted_Implementation(const FGameplayTag& questTag)
{
    OnQuestStarted.Broadcast(questTag);
}

void UAQSQuestManagerComponent::ClientDispatchQuestCompleted_Implementation(const FGameplayTag& questTag, bool bSucceded)
{
    OnQuestEnded.Broadcast(questTag, bSucceded);
}

void UAQSQuestManagerComponent::ClientsDispatchObjectiveUpdate_Implementation(const FGameplayTag& objectiveTag, const FGameplayTag& questTag, EQuestUpdateType updateType)
{
    DispatchObjectiveUpdate(objectiveTag, questTag, updateType);
}

void UAQSQuestManagerComponent::GetAllRecords(TArray<FAQSQuestRecord>& outInProgressQuests, TArray<FGameplayTag>& outCompletedQuests,
    TArray<FGameplayTag>& outFailedQuests, FGameplayTag& outTrackedQuest)
{
    outInProgressQuests = InProgressQuestsRecords;
    outCompletedQuests = CompletedQuestsTags;
    outFailedQuests = FailedQuestsTags;
    outTrackedQuest = TrackedQuestTag;
}

void UAQSQuestManagerComponent::ReloadFromRecords(const TArray<FAQSQuestRecord>& inInProgressQuests, const TArray<FGameplayTag>& inCompletedQuests,
    const TArray<FGameplayTag>& inFailedQuests, const FGameplayTag& inTrackedQuest)
{
    InProgressQuestsRecords = inInProgressQuests;
    CompletedQuestsTags = inCompletedQuests;
    FailedQuestsTags = inFailedQuests;
    TrackedQuestTag = inTrackedQuest;
    OnComponentLoaded();
}

void UAQSQuestManagerComponent::DEBUG_ProceedQuest(const FGameplayTag& inProgressQuest)
{
    UAQSQuest* quest = GetQuest(inProgressQuest);
    if (quest) {
        const TArray<UAQSQuestObjective*> objectives = quest->GetAllActiveObjectives();
        for (const auto& obj : objectives) {
            ServerCompleteObjective(obj->GetObjectiveTag());
        }
    }
}

void UAQSQuestManagerComponent::DEBUG_ProceedTrackedQuest()
{
    DEBUG_ProceedQuest(GetCurrentlyTrackedQuestTag());
}

void UAQSQuestManagerComponent::OnRep_TrackedQuest()
{
    OnTrackedQuestChanged.Broadcast();
}

void UAQSQuestManagerComponent::OnRep_InProgressQuestsRecords()
{
    SyncGraphs();
    OnInProgressQuestsUpdate.Broadcast();
}

void UAQSQuestManagerComponent::OnRep_FailedQuestsTags()
{
    SyncGraphs();
    OnFailedQuestsUpdate.Broadcast();
}

void UAQSQuestManagerComponent::OnRep_CompletedQuestsTags()
{
    SyncGraphs();
    OnCompletedQuestsUpdate.Broadcast();
}

void UAQSQuestManagerComponent::HandleQuestCompleted(const FGameplayTag& questToComplete, bool bSuccesful)
{
    if (InProgressQuestsRecords.Contains(questToComplete)) {
        UAQSQuest* quest = GetQuestFromDB(questToComplete);

        if (quest) {
            if (TrackedQuestTag == questToComplete) {
                UntrackCurrentQuest();
            }
            InProgressQuests.Remove(quest);
            InProgressQuestsRecords.Remove(FAQSQuestRecord(quest));

            if (bSuccesful) {
                CompletedQuests.Add(quest);
                CompletedQuestsTags.Add(questToComplete);
                OnCompletedQuestsUpdate.Broadcast();
            } else {
                FailedQuests.Add(quest);
                FailedQuestsTags.Add(questToComplete);
                OnFailedQuestsUpdate.Broadcast();
            }
            quest->OnQuestEnded.RemoveDynamic(this, &UAQSQuestManagerComponent::HandleQuestCompleted);

            quest->OnObjectiveStarted.RemoveDynamic(this, &UAQSQuestManagerComponent::HandleObjectiveStarted);
            quest->OnObjectiveCompleted.RemoveDynamic(this, &UAQSQuestManagerComponent::HandleObjectiveCompleted);
            quest->OnObjectiveUpdated.RemoveDynamic(this, &UAQSQuestManagerComponent::HandleObjectiveUpdated);
        }

        OnQuestEnded.Broadcast(questToComplete, bSuccesful);

        // *** EDITED by Nomad Dev Team: Added client notification for listen server and standalone ***
        // Reason: Listen server setups do not meet the dedicated server condition,
        // so clients miss quest completion notifications.
        // This ensures clients receive quest complete notifications in listen server and standalone setups.
        if (GetOwnerRole() == ROLE_Authority && !IsLocallyControlled())
        {
            ClientDispatchQuestCompleted(questToComplete, bSuccesful);
        }
        
    } else {
        UE_LOG(LogTemp, Error, TEXT("Invalid quest!! - UAQSQuestManagerComponent::HandleQuestCompleted"));
    }
}

void UAQSQuestManagerComponent::HandleObjectiveStarted(const FGameplayTag& objective, const FGameplayTag& quest)
{
    OnObjectiveStarted.Broadcast(objective, quest);
}

void UAQSQuestManagerComponent::HandleObjectiveCompleted(const FGameplayTag& objective, const FGameplayTag& quest)
{
    OnObjectiveCompleted.Broadcast(objective, quest);
}

void UAQSQuestManagerComponent::HandleObjectiveUpdated(const FGameplayTag& objective, const FGameplayTag& quest)
{
    OnObjectiveUpdated.Broadcast(objective, quest);
}

void UAQSQuestManagerComponent::SyncGraphs()
{
    CompletedQuests.Empty();
    FailedQuests.Empty();
    InProgressQuests.Empty();
    for (const auto& questData : CompletedQuestsTags) {
        UAQSQuest* quest = GetQuestFromDB(questData);
        if (quest) {
            CompletedQuests.Add(quest);
        }
    }

    for (const auto& questData : FailedQuestsTags) {
        UAQSQuest* quest = GetQuestFromDB(questData);
        if (quest) {
            FailedQuests.Add(quest);
        }
    }

    // this NEEDS to be a copy! Otherwise the quest update will override it
    TArray<FAQSQuestRecord> tempRecords = InProgressQuestsRecords;
    for (const auto& questData : tempRecords) {
        UAQSQuest* quest = GetQuestFromDB(questData.Quest);

        if (quest) {
            if (Internal_StartQuest(quest, false, false)) {
                quest->SetCompletedObjectives(questData.CompletedObjectives);
                for (const auto& obj : questData.Objectives) {
                    UAQSObjectiveNode* objNode = quest->GetObjectiveNode(obj.Objective);
                    if (objNode) {
                        quest->ActivateNode(objNode);
                        objNode->SetCurrentRepetitions(obj.CurrentRepetitions);
                    }
                }
            } else {
                UE_LOG(LogTemp, Error, TEXT("Impossible To Start Quest - UAQSQuestManagerComponent::OnComponentLoaded"));
            }
        }
    }

    UAQSQuest* quest = GetQuestFromDB(TrackedQuestTag);
    if (quest) {
        TrackInProgressQuest(quest);
    }
}

bool UAQSQuestManagerComponent::RemoveInProgressQuest(UAQSQuest* quest)
{
    if (IsQuestInProgress(quest)) {
        InProgressQuests.Remove(quest);
        InProgressQuestsRecords.Remove(FAQSQuestRecord(quest));
        return true;
    }
    return false;
}

bool UAQSQuestManagerComponent::IsLocallyControlled() const
{
    if (const APawn* pawn = Cast<APawn>(GetOwner()))
    {
        return pawn->IsLocallyControlled();
    }
    if (const APlayerController* pc = Cast<APlayerController>(GetOwner()))
    {
        return pc->IsLocalController();
    }
    return false;
}