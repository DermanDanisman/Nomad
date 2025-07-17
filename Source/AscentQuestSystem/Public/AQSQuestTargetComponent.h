// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "AQSQuestFunctionLibrary.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Graph/AQSObjectiveNode.h"
#include <GameplayTagContainer.h>

#include "AQSQuestTargetComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrackedStatusChanged, bool, bTracked);

UCLASS(ClassGroup = (ATS), Blueprintable, meta = (BlueprintSpawnableComponent))
class ASCENTQUESTSYSTEM_API UAQSQuestTargetComponent : public UActorComponent {
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UAQSQuestTargetComponent();

    /*Called every one of the referencing objectives is updated*/
    UPROPERTY(BlueprintAssignable, Category = AQS)
    FOnObjectiveUpdated OnReferencingObjectiveUpdated;

    /*Called every one of the referencing objectives is updated*/
    UPROPERTY(BlueprintAssignable, Category = AQS)
    FOnObjectiveStarted OnReferencingObjectiveStarted;

    UPROPERTY(BlueprintAssignable, Category = AQS)
    FOnObjectiveCompleted OnReferencingObjectiveCompleted;

    UPROPERTY(BlueprintAssignable, Category = AQS)
    FOnObjectiveCompleted OnReferencingObjectiveInterrupted;

    UPROPERTY(BlueprintAssignable, Category = AQS)
    FOnTrackedStatusChanged OnTrackedStatusChanged;

    /*Checks all the InProgress Quests and tries to complete the objective with the
    provided tag*/
    UFUNCTION(BlueprintCallable, Category = AQS)
    void CompleteObjective(FGameplayTag objectiveToComplete);

    /*Checks all the InProgress Quests and Objectives and tries to complete the objective +
    that has the owner of this component as ReferencedTarget*/
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = AQS)
    void CompleteReferencingObjetive();

    /*Checks all the InProgress Quests and tries to complete the objective with the
    provided tag unlocking only the transitions with the name provided*/
    UFUNCTION(BlueprintCallable, Category = AQS)
    void CompleteBranchedObjective(FGameplayTag objectiveToComplete, const TArray<FName>& transitionFilters);

    /*UNUSED if you are referencing it throught soft references*/
    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE FGameplayTag GetTargetTag() const {
        return TargetTag;
    }

    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE bool IsTrackedTarget() const  {
        return bTracked;
    }

    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE bool IsCurrentlyTargetOfObjective() const   {
        return bIsCurrentlyTargetOfObjective;
    }

    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE FGameplayTag GetReferencingObjective() const    {
        return currentlyTargetingObjective;
    }

    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE FGameplayTag GetReferencingQuest() const   {
        return currentlyTargetingQuest;
    }

    UFUNCTION(BlueprintNativeEvent, Category = AQS)
    void OnObjectiveStarted(const FGameplayTag& objectiveTag, const FGameplayTag& questTag);

    UFUNCTION(BlueprintNativeEvent, Category = AQS)
    void OnObjectiveCompleted(const FGameplayTag& objectiveTag, const FGameplayTag& questTag);

    UFUNCTION(BlueprintNativeEvent, Category = AQS)
    void OnObjectiveUpdated(const FGameplayTag& objectiveTag, const FGameplayTag& questTag);

    UFUNCTION(BlueprintNativeEvent, Category = AQS)
    void OnObjectiveInterrupted(const FGameplayTag& objectiveTag, const FGameplayTag& questTag);

    UFUNCTION(BlueprintNativeEvent, Category = AQS)
    void OnTrackedChanged(bool bNewTracked);

    void DispatchObjectiveUpdated(const FGameplayTag& objectiveTag, const FGameplayTag& questTag, EQuestUpdateType objectiveUpdate, bool inTracked);
    void ClientDispatchObjectiveUpdated(const FGameplayTag& objectiveTag, const FGameplayTag& questTag, EQuestUpdateType objectiveUpdate, bool inTracked);

   
    void SetTracked(bool inTracked);

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    /*Tag of this target. Used to call the events once a referencing objective is updated*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AQS)
    FGameplayTag TargetTag;

private:
    UPROPERTY(SaveGame, Replicated)
    bool bIsCurrentlyTargetOfObjective = false;

    UPROPERTY(SaveGame, Replicated)
    FGameplayTag currentlyTargetingObjective;

    UPROPERTY(SaveGame, Replicated)
    FGameplayTag currentlyTargetingQuest;

    bool bTracked;
};
