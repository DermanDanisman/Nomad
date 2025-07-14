// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Graph/AQSObjectiveNode.h"

#include "AQSQuestObjective.generated.h"

class UAQSQuestManagerComponent;

UENUM(BlueprintType)
enum class ETargetReferenceType : uint8 {
    ESoftRef UMETA(DisplayName = "Soft Reference"),
    ETag UMETA(DisplayName = "Gameplay Tag"),
};
/**
 *
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew)
class ASCENTQUESTSYSTEM_API UAQSQuestObjective : public UObject {
    GENERATED_BODY()

public:
    friend class UAQSObjectiveNode;

    UAQSQuestObjective();

    /*Returns the list of the actors referenced by this objective as set in
the graph node*/
    UFUNCTION(BlueprintPure, Category = AQS)
    TArray<class AActor*> GetObjectiveTargetsActors() const;

    /*Returns the list of the actors referenced by this objective as set in
    the graph node*/
    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE TArray<class UAQSQuestTargetComponent*> GetObjectiveTargets() const
    {
        return questTargets;
    }

      UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE ETargetReferenceType GetTargetRefType() const
    {
        return TargetRefType;
    }

    /*Returns the tag of this objective as set in
    the graph node*/
    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE FGameplayTag GetObjectiveTag() const
    {
        return ObjectiveTag;
    }

    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE FText GetObjectiveName() const
    {
        return ObjectiveName;
    }

    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE FText GetDescription() const
    {
        return ObjectiveDescription;
    }

    UFUNCTION(BlueprintPure, Category = AQS)
    class APlayerController* GetLocalPlayerController() const
    {
        if (nodeOwner) {
            return nodeOwner->GetPlayerController();
        }
        return nullptr;
    }

    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE class UWorld* GetWorldContextObject() const
    {
        return GetWorld();
    }

    UFUNCTION(BlueprintPure, Category = AQS)
    int32 GetCurrentRepetitions() const;

    UFUNCTION(BlueprintPure, Category = AQS)
    int32 GetRepetitions() const;

    UFUNCTION(BlueprintCallable, Category = AQS)
    void SetCurrentRepetitions(int32 currentRep);

    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE bool IsObjectiveCompleted() const
    {
        return nodeOwner->IsObjectiveCompleted();
    }

    UFUNCTION(BlueprintPure, Category = AQS)
    FORCEINLINE bool IsTracked() const
    {
        return nodeOwner->IsTracked();
    }

    TArray<TSoftObjectPtr<AActor>> GetObjectiveTargetsSoftRef() const;

    UFUNCTION(BlueprintCallable, Category = AQS)
    void CompleteObjective();

private:
    void Internal_OnObjectiveStarted(class UAQSObjectiveNode* inNodeOwner);

    void Internal_OnObjectiveEnded(bool bInterrupted);

    void FillTargetsRef();
    UPROPERTY()
    UAQSObjectiveNode* nodeOwner;

    UPROPERTY()
    TArray<class UAQSQuestTargetComponent*> questTargets;

protected:
    /*Unique Tag to identify this objective*/
    UPROPERTY(EditDefaultsOnly, Category = AQS)
    FGameplayTag ObjectiveTag;

    /*A description for this objective, can be used for UI*/
    UPROPERTY(EditDefaultsOnly, Category = AQS)
    FText ObjectiveName;

    /*A description for this objective, can be used for UI*/
    UPROPERTY(EditDefaultsOnly, Category = AQS)
    FText ObjectiveDescription;

    /*Define how you want to identify the targets of this objectives
    if you want tor efer directly hem thorught soft references or thorught
    their GameplayTags*/
    UPROPERTY(EditDefaultsOnly, Category = AQS)
    ETargetReferenceType TargetRefType;

    /*A list of targets that, if they have a QuestTargetComponent, will be
    updated with this objective progresses. All targets with that tag in the component
    will be alerted*/
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "TargetRefType == ETargetReferenceType::ETag"), Category = AQS)
    TArray<FGameplayTag> ReferencedTargets;

    /*A list of targets that, if they have a QuestTargetComponent, will be
    updated with this objective progresses. The provided actors need to have
    a QuestTargetComponent to work*/
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "TargetRefType == ETargetReferenceType::ESoftRef"), Category = AQS)
    TArray<TSoftObjectPtr<AActor>> ReferencedActors;

    /*Forces the load of the assets if they are not loaded (es for world partition)*/
     UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "TargetRefType == ETargetReferenceType::ESoftRef"), Category = AQS)
     bool bForceLoadReferencedActors = true;

    /*Called when this objective is started*/
    UFUNCTION(BlueprintNativeEvent, Category = AQS)
    void OnObjectiveStarted();
    virtual void OnObjectiveStarted_Implementation();

    UFUNCTION(BlueprintNativeEvent, Category = AQS)
    void ClientOnObjectiveStarted();
    virtual void ClientOnObjectiveStarted_Implementation();

    /*Called when this objective is completed*/
    UFUNCTION(BlueprintNativeEvent, Category = AQS)
    void OnObjectiveCompleted();
    virtual void OnObjectiveCompleted_Implementation();

    /*Called when this objective is completed*/
    UFUNCTION(BlueprintNativeEvent, Category = AQS)
    void ClientOnObjectiveCompleted();
    virtual void ClientOnObjectiveCompleted_Implementation();

    UFUNCTION(BlueprintNativeEvent, Category = AQS)
    void OnObjectiveUpdated();
    virtual void OnObjectiveUpdated_Implementation();

    UFUNCTION(BlueprintNativeEvent, Category = AQS)
    void OnObjectiveInterrupted();
    virtual void OnObjectiveInterrupted_Implementation();

    UFUNCTION(BlueprintNativeEvent, Category = AQS)
    void ClientOnObjectiveInterrupted();
    virtual void ClientOnObjectiveInterrupted_Implementation();



    UFUNCTION(BlueprintPure, Category = AQS)
    UAQSQuestManagerComponent* GetLocalQuestManager() const;


    // UObject interface
    UWorld* GetWorld() const override { return GetLocalPlayerController() ? GetLocalPlayerController()->GetWorld() : nullptr; }
    // End of UObject interface
};
