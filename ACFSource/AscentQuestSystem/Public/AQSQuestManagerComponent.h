// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "AQSQuestTargetComponent.h"
#include "AQSTypes.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Graph/AQSQuest.h"
#include <Containers/Map.h>
#include <GameplayTagContainer.h>

#include "AQSQuestManagerComponent.generated.h"

class UAQSQuestObjective;
class UAQSQuestTargetComponent;
class UAQSQuest;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInProgressQuestsUpdate);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCompletedQuestsUpdate);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFailedQuestsUpdate);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTrackedQuestUpdate);

UCLASS(Blueprintable, ClassGroup = (ATS), meta = (BlueprintSpawnableComponent))
class ASCENTQUESTSYSTEM_API UAQSQuestManagerComponent : public UActorComponent {
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UAQSQuestManagerComponent();

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    /*When a quest starts, if no quests are tracked, automatically
    sets the new quest as the tracked one*/
    UPROPERTY(EditDefaultsOnly, Category = AQS)
    bool bAutoTrackQuest = true;

    /*BETA: if this bool is set to true, quests can be completed by anyone
    of the players in the game. Please notice that in this case this component
    should be placed in GAMESTATE*/
    UPROPERTY(EditDefaultsOnly, Category = AQS)
    bool bTeamQuests = false;

    /*The Database of quests*/
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = AQS)
    class UDataTable* QuestsDB;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = AQS)
    void OnComponentSaved();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = AQS)
    void OnComponentLoaded();

public:
    //** REPLICATION **//

    /*Replicated Version of Complete Objective*/
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = AQS)
    void ServerCompleteObjective(const FGameplayTag& objectiveToComplete);

    UFUNCTION(BlueprintCallable, Server, Reliable, Category = AQS)
    void ServerCompleteBranchedObjective(const FGameplayTag& objectiveToComplete, const TArray<FName>& optionalTransitionFilters);

    UFUNCTION(BlueprintCallable, Server, Reliable, Category = AQS)
    void ServerStartQuest(const FGameplayTag& questTag);

    UFUNCTION(BlueprintCallable, Server, Reliable, Category = AQS)
    void ServerRemoveInProgressQuest(const FGameplayTag& questTag);

    UFUNCTION(Server, Reliable, Category = AQS)
    void ServerDispatchObjectiveUpdate(const FGameplayTag& objectiveTag, const FGameplayTag& questTag, EQuestUpdateType updateType);

    //** END REPLICATION **//

    // SINGLE PLAYER VERSION

    /*Starts the provided quest*/
    UFUNCTION(BlueprintCallable, Category = AQS)
    bool StartQuest(const FGameplayTag& questTag);

    /*Interrupts the provided quest from the in progress quests and reset his state */
    UFUNCTION(BlueprintCallable, Category = AQS)
    bool StopQuest(const FGameplayTag& questTag);

    /*Checks all the InProgress Quests and tries to complete the objective with the
    provided tag*/
    UFUNCTION(BlueprintCallable, Category = AQS)
    bool CompleteObjective(FGameplayTag objectiveToComplete);

    /*Checks all the InProgress Quests and tries to complete the objective with the
    provided tag unlocking only the transitions with the name provided*/
    UFUNCTION(BlueprintCallable, Category = AQS)
    bool CompleteBranchedObjective(FGameplayTag objectiveToComplete, TArray<FName> optionalTransitionFilters);

    /*If the quest provided is in progress, sets the provided quest as the tracked one*/
    UFUNCTION(BlueprintCallable, Category = AQS)
    void TrackInProgressQuestByTag(const FGameplayTag& questTag);

    /*If the quest provided is in progress, sets the provided quest as the tracked one*/
    UFUNCTION(BlueprintCallable, Category = AQS)
    bool TrackInProgressQuest(class UAQSQuest* questToTrack);

    UFUNCTION(BlueprintCallable, Category = AQS)
    bool RemoveInProgressQuest(UAQSQuest* quest);
    bool IsLocallyControlled() const;

    UFUNCTION(BlueprintCallable, Category = AQS)
    void UntrackCurrentQuest();
    //** END  **//

    /*SERVER ONLY GETTERS*/
    UFUNCTION(BlueprintPure, Category = "AQS| Server")
    FORCEINLINE bool IsQuestInProgress(class UAQSQuest* quest) const
    {
        return InProgressQuests.Contains(quest);
    }

    UFUNCTION(BlueprintPure, Category = "AQS| Server")
    FORCEINLINE TArray<class UAQSQuest*> GetInProgressQuests() const
    {
        return InProgressQuests;
    }

    UFUNCTION(BlueprintPure, Category = "AQS| Server")
    FORCEINLINE TArray<class UAQSQuest*> GetCompletedQuests() const
    {
        return CompletedQuests;
    }

    UFUNCTION(BlueprintPure, Category = "AQS| Server")
    FORCEINLINE TArray<class UAQSQuest*> GetFailedQuests() const
    {
        return FailedQuests;
    }

    UFUNCTION(BlueprintPure, Category = "AQS| Server")
    FORCEINLINE class UAQSQuest* GetCurrentlyTrackedQuest() const
    {
        return TrackedQuest;
    }

    UFUNCTION(BlueprintPure, Category = "AQS| Server")
    TArray<UAQSQuestObjective*> GetCurrentlyTrackedQuestObjectives() const;

    UFUNCTION(BlueprintPure, Category = "AQS| Server")
    TArray<UAQSQuestTargetComponent*> GetCurrentlyTrackedQuestObjectivesTargets() const;

    UFUNCTION(BlueprintPure, Category = "AQS| Server")
    TArray<AActor*> GetCurrentlyTrackedQuestObjectivesTargetActors() const;

    //** END  **//

    /*SERVER & CLIENT GETTERS*/
    UFUNCTION(BlueprintPure, Category = AQS)
    bool IsObjectiveInProgress(const FGameplayTag& objectiveTag) const;

    UFUNCTION(BlueprintCallable, Category = AQS)
    class UAQSQuest* GetQuestFromDB(const FGameplayTag& questTag);

    UFUNCTION(BlueprintCallable, Category = AQS)
    class UAQSQuest* GetQuest(const FGameplayTag& questTag) const;

    UFUNCTION(BlueprintCallable, Category = AQS)
    class UAQSQuestObjective* GetQuestObjectiveFromDB(const FGameplayTag& objectiveTag, const FGameplayTag& questTag) const;

    /*Tries to get the objective info for the provided objective, works on both client and server*/
    UFUNCTION(BlueprintCallable, Category = AQS)
    bool TryGetInProgressObjectiveInfo(const FGameplayTag& questTag, const FGameplayTag& objectiveTag, FAQSObjectiveInfo& outObjectiveInfo) const;

    /*Tries to get the objective info for the provided quest, works on both client and server*/
    UFUNCTION(BlueprintCallable, Category = AQS)
    bool TryGetInProgressQuestInfo(const FGameplayTag& questTag, FAQSQuestInfo& outQuestInfo) const;

    /*Tries to get the target actors for the in progress  objective. */
    UFUNCTION(BlueprintCallable, Category = AQS)
    bool TryGetTargetActorsForInProegressObjective(const FGameplayTag& objectiveTag, const FGameplayTag& questTag, TArray<class AActor*>& outTargets);

    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE bool DoesQuestHaveInProgressObjective(const FAQSQuestRecord& quest, const FGameplayTag& questTag) const
    {
        return quest.Objectives.Contains(questTag);
    }

    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE bool IsQuestInProgressByTag(const FGameplayTag& questTag) const
    {
        return InProgressQuestsRecords.Contains(questTag);
    }

    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE bool IsQuestCompletedByTag(const FGameplayTag& questTag) const
    {
        return CompletedQuestsTags.Contains(questTag);
    }

    UFUNCTION(BlueprintPure, Category = AQS)
    bool IsObjectiveCompletedByTag(const FGameplayTag& questTag, const FGameplayTag& objectiveTag) const;

    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE bool IsQuestFailedByTag(const FGameplayTag& questTag) const
    {
        return FailedQuestsTags.Contains(questTag);
    }

    /*Returns all the info of the in progress quests*/
    UFUNCTION(BlueprintPure, Category = AQS)
    void GetInProgressQuestsInfo(TArray<FAQSQuestInfo>& outInfo) const;

    /*Returns the base info of the in progress quests*/
    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE TArray<FAQSQuestRecord> GetInProgressQuestsRecords() const
    {
        return InProgressQuestsRecords;
    }

    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE TArray<FGameplayTag> GetCompletedQuestsTag() const
    {
        return CompletedQuestsTags;
    }

    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE TArray<FGameplayTag> GetFailedQuestsTag() const
    {
        return FailedQuestsTags;
    }

    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE FGameplayTag GetCurrentlyTrackedQuestTag() const
    {
        return TrackedQuestTag;
    }

    //** END  **//

    /*GET TARGETS*/
    /*Retrieves all targets with the provided tag*/
    UFUNCTION(BlueprintCallable, Category = AQS)
    TArray<class UAQSQuestTargetComponent*> GetAllTargetsWithTag(const FGameplayTag& targetTag) const;

    /*Retrieves all targets with the provided tags*/
    UFUNCTION(BlueprintCallable, Category = AQS)
    TArray<class UAQSQuestTargetComponent*> GetAllTargetsWithTags(const TArray<FGameplayTag>& targetTags) const;

    UPROPERTY(BlueprintAssignable, Category = AQS)
    FOnQuestStarted OnQuestStarted;

    UPROPERTY(BlueprintAssignable, Category = AQS)
    FOnQuestEnded OnQuestEnded;

    UPROPERTY(BlueprintAssignable, Category = AQS)
    FOnObjectiveStarted OnObjectiveStarted;

    UPROPERTY(BlueprintAssignable, Category = AQS)
    FOnObjectiveCompleted OnObjectiveCompleted;

    UPROPERTY(BlueprintAssignable, Category = AQS)
    FOnObjectiveUpdated OnObjectiveUpdated;

    UPROPERTY(BlueprintAssignable, Category = AQS)
    FOnObjectiveInterrupted OnObjectiveInterrupted;

    UPROPERTY(BlueprintAssignable, Category = AQS)
    FOnInProgressQuestsUpdate OnTrackedQuestUpdated;

    UPROPERTY(BlueprintAssignable, Category = AQS)
    FOnInProgressQuestsUpdate OnInProgressQuestsUpdate;

    UPROPERTY(BlueprintAssignable, Category = AQS)
    FOnCompletedQuestsUpdate OnCompletedQuestsUpdate;

    UPROPERTY(BlueprintAssignable, Category = AQS)
    FOnFailedQuestsUpdate OnFailedQuestsUpdate;

    UPROPERTY(BlueprintAssignable, Category = AQS)
    FOnTrackedQuestUpdate OnTrackedQuestChanged;

    void RegisterTarget(class UAQSQuestTargetComponent* targetComp);

    void UnregisterTarget(class UAQSQuestTargetComponent* targetComp);

    void DispatchObjectiveUpdate(const FGameplayTag& objectiveTag, const FGameplayTag& questTag, EQuestUpdateType updateType);

    UFUNCTION(Client, Reliable, Category = AQS)
    void ClientDispatchObjectiveUpdate(const FGameplayTag& objectiveTag, const FGameplayTag& questTag, EQuestUpdateType updateType);

    UFUNCTION(Client, Reliable, Category = AQS)
    void ClientDispatchQuestStarted(const FGameplayTag& questTag);

    UFUNCTION(Client, Reliable, Category = AQS)
    void ClientDispatchQuestCompleted(const FGameplayTag& questTag, bool bSucceded);

    UFUNCTION(NetMulticast, Reliable, Category = AQS)
    void ClientsDispatchObjectiveUpdate(const FGameplayTag& objectiveTag, const FGameplayTag& questTag, EQuestUpdateType updateType);

    // SAVE & LOAD NODES//
    // 
    // 
    //Get all the records to be saved for persistence
    UFUNCTION(BlueprintCallable, Category = AQS)
    void GetAllRecords(TArray<FAQSQuestRecord>& outInProgressQuests, TArray<FGameplayTag>& outCompletedQuests,
        TArray<FGameplayTag>& outFailedQuests, FGameplayTag& outTrackedQuest);

    /*Reloads all the graphs from the provided Data*/
    UFUNCTION(BlueprintCallable, Category = AQS)
    void ReloadFromRecords(const TArray<FAQSQuestRecord>& inInProgressQuests, const TArray<FGameplayTag>& inCompletedQuests,
        const TArray<FGameplayTag>& inFailedQuests, const FGameplayTag& inTrackedQuest);


    // DEBUG//
    UFUNCTION(BlueprintCallable, Category = AQS)
    void DEBUG_ProceedQuest(const FGameplayTag& inProgressQuest);

    UFUNCTION(BlueprintCallable, Category = AQS)
    void DEBUG_ProceedTrackedQuest();

private:
    bool Internal_StartQuest(UAQSQuest* questToStart, const bool bStartChildNodes, bool autoTrack = true);

    UPROPERTY()
    TArray<class UAQSQuest*> CompletedQuests;

    UPROPERTY()
    TArray<class UAQSQuest*> FailedQuests;

    UPROPERTY()
    TArray<class UAQSQuest*> InProgressQuests;

    UPROPERTY()
    class UAQSQuest* TrackedQuest;

    UPROPERTY(SaveGame, ReplicatedUsing = OnRep_CompletedQuestsTags)
    TArray<FGameplayTag> CompletedQuestsTags;

    UPROPERTY(SaveGame, ReplicatedUsing = OnRep_FailedQuestsTags)
    TArray<FGameplayTag> FailedQuestsTags;

    UPROPERTY(SaveGame, ReplicatedUsing = OnRep_InProgressQuestsRecords)
    TArray<FAQSQuestRecord> InProgressQuestsRecords;

    UPROPERTY(SaveGame, ReplicatedUsing = OnRep_TrackedQuest)
    FGameplayTag TrackedQuestTag;

    UFUNCTION()
    void OnRep_TrackedQuest();

    UFUNCTION()
    void OnRep_InProgressQuestsRecords();

    UFUNCTION()
    void OnRep_FailedQuestsTags();

    UFUNCTION()
    void OnRep_CompletedQuestsTags();

    TMultiMap<FGameplayTag, UAQSQuestTargetComponent*> QuestTargets;

    UFUNCTION()
    void HandleQuestCompleted(const FGameplayTag& questToComplete, bool bSuccesful);

    UFUNCTION()
    void HandleObjectiveStarted(const FGameplayTag& objective, const FGameplayTag& quest);

    UFUNCTION()
    void HandleObjectiveCompleted(const FGameplayTag& objective, const FGameplayTag& quest);

    UFUNCTION()
    void HandleObjectiveUpdated(const FGameplayTag& objective, const FGameplayTag& quest);

    void SyncGraphs();

    UPROPERTY()
    TMap<FGameplayTag, UAQSQuest*> loadedQuests;
};