// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "AGSGraph.h"
#include "CoreMinimal.h"
#include <Engine/DataTable.h>
#include <GameFramework/Character.h>

#include "ADSDialogue.generated.h"

class UADSDialoguePartecipantComponent;
class UADSStartDialogueNode;
class UADSGraphNode;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueNodeActivated, class UADSGraphNode*, node);

UCLASS()
class ASCENTDIALOGUESYSTEM_API UADSDialogue : public UAGSGraph {
    GENERATED_BODY()

public:
    UADSDialogue();

    bool ActivateNode(class UAGSGraphNode* node) override;

    UFUNCTION(BlueprintCallable, Category = ADS)
    class UAGSGraphNode* StartDialogue(class APlayerController* inController, const TArray<UADSDialoguePartecipantComponent*>& participants);

    UFUNCTION(BlueprintCallable, Category = ADS)
    TArray<class UADSDialogueResponseNode*> GetAllButtonAnswersForCurrentNode() const;

    UFUNCTION(BlueprintCallable, Category = ADS)
    class UADSDialogueNode* MoveToNextNode();

    UFUNCTION(BlueprintPure, Category = ADS)
    bool HasPartecipant(const FGameplayTag& partecipantTag) const { return partecipantsRef.Contains(partecipantTag); }

    UFUNCTION(BlueprintPure, Category = ADS)
    bool IsDialogueStarted() const { return bIsStarted; }

    UFUNCTION(BlueprintCallable, Category = ADS)
    class UADSDialoguePartecipantComponent* FindPartecipant(FGameplayTag partecipantTag) const;

    UPROPERTY(BlueprintAssignable, Category = ADS)
    FOnDialogueStarted OnDialogueStarted;

    UPROPERTY(BlueprintAssignable, Category = ADS)
    FOnDialogueEnded OnDialogueEnded;

    UPROPERTY(BlueprintAssignable, Category = ADS)
    FOnDialogueNodeActivated OnDialogueNodeActivated;

    UFUNCTION(BlueprintPure, Category = ADS)
    FORCEINLINE class UADSGraphNode* GetCurrentNode() const { return currentNode; }

    UFUNCTION(BlueprintPure, Category = ADS)
    FORCEINLINE FGameplayTag GetDialogueTag() const { return DialogueTag; }

    UFUNCTION(BlueprintPure, Category = ADS)
    FORCEINLINE FGameplayTag GetDefaultParticipantTag() const { return DefaultParticipantTag; }

    FORCEINLINE bool operator==(const FGameplayTag& OtherTag) const
    {
        return this->DialogueTag == OtherTag;
    }

    FORCEINLINE bool operator!=(const FGameplayTag& OtherTag) const
    {
        return this->DialogueTag != OtherTag;
    }
    FORCEINLINE bool operator==(const UADSDialogue*& Other) const
    {
        return DialogueTag == Other->DialogueTag;
    }

    FORCEINLINE bool operator!=(const UADSDialogue*& Other) const
    {
        return DialogueTag != Other->DialogueTag;
    }

protected:
    /*Unique Tag for this Dialogue*/
    UPROPERTY(EditDefaultsOnly, Category = AQS)
    FGameplayTag DialogueTag;

    UPROPERTY(EditDefaultsOnly, Category = AQS)
    FGameplayTag DefaultParticipantTag;

private:
 
    TMap < FGameplayTag, TObjectPtr<UADSDialoguePartecipantComponent>> partecipantsRef;

    TObjectPtr <UADSGraphNode> currentNode;

    TObjectPtr<UADSStartDialogueNode> currentDialogueStart;

    bool bIsStarted;

    void EndDialogue();
};
