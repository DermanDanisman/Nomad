// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Graph/ADSDialogue.h"
#include <Components/SceneComponent.h>
#include <GameplayTagContainer.h>

#include "ADSDialoguePartecipantComponent.generated.h"


class USkeletalMeshComponent;
class UADSDialogue;

UCLASS(Blueprintable, ClassGroup = (ATS), meta = (BlueprintSpawnableComponent))
class ASCENTDIALOGUESYSTEM_API UADSDialoguePartecipantComponent : public USceneComponent {
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UADSDialoguePartecipantComponent();

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    virtual void EndPlay(EEndPlayReason::Type reason) override;

public:
    UFUNCTION(BlueprintCallable, Category = ADS)
    bool TryStartDialogue(const TArray<UADSDialoguePartecipantComponent*>& participants, UADSDialogue* dialogueToStart);

   UFUNCTION(BlueprintCallable, Category = ADS)
    bool TryStartDialogueFromActors(const TArray<AActor*>& participants, UADSDialogue* dialogueToStart);

    UPROPERTY(BlueprintAssignable, Category = ADS)
    FOnDialogueStarted OnDialogueStarted;

    UPROPERTY(BlueprintAssignable, Category = ADS)
    FOnDialogueEnded OnDialogueEnded;

    UFUNCTION(BlueprintPure, Category = ADS)
    FORCEINLINE FName GetParticipantName() const
    {
        return PartecipantName;
    }

    UFUNCTION(BlueprintPure, Category = ADS)
    FORCEINLINE FGameplayTag GetParticipantTag() const
    {
        return PartecipantTag;
    }

    UFUNCTION(BlueprintPure, Category = ADS)
    FORCEINLINE UTexture2D* GetParticipantIcon() const
    {
        return PartecipantIcon;
    }

    UFUNCTION(BlueprintPure, Category = ADS)
    FORCEINLINE FName GetVoiceSpawningSocketName() const
    {
        return VoiceSpawningSocketName;
    }

    UFUNCTION(BlueprintPure, Category = ADS)
    UADSDialogue* GetDialogue(FGameplayTag dialogueTag, bool& bFound) const;

    UFUNCTION(BlueprintPure, Category = ADS)
    USkeletalMeshComponent* GetOwnerMesh();

    UFUNCTION(BlueprintCallable, Category = ADS)
    void ChangeParticipantIcon(class UTexture2D* icon)
    {
        PartecipantIcon = icon;
    }

    UFUNCTION(BlueprintCallable, Category = ADS)
    void ChangeParticipantName(const FName& newName)
    {
        PartecipantName = newName;
    }

    UFUNCTION(BlueprintCallable, Category = ADS)
    void SetParticipantSkeletalMesh(class USkeletalMeshComponent* mesh)
    {
        skeletalMesh = mesh;
    }

    UFUNCTION(BlueprintCallable, Category = ADS)
    virtual void PlayAnimationOnCharacterOwner(UAnimMontage* animationTag);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ADS)
    TArray<class UADSDialogue*> Dialogues;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ADS)
    FName PartecipantName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ADS)
    FGameplayTag PartecipantTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ADS)
    class UTexture2D* PartecipantIcon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ADS)
    FName VoiceSpawningSocketName = "head";

    UFUNCTION(BlueprintNativeEvent, Category = ADS)
    void OnDialogueStartedEvent();

    UFUNCTION(BlueprintNativeEvent, Category = ADS)
    void OnDialogueEndedEvent();

    UPROPERTY()
    USkeletalMeshComponent* skeletalMesh;

private:
    UFUNCTION()
    void HandleDialogueStarted();

    UFUNCTION()
    void HandleDialogueEnded();

    UPROPERTY()
    TArray<class UADSDialogue*> instancedDialogues;
};
