// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "AGSGraphNode.h"
#include "CoreMinimal.h"
#include <GameplayTagContainer.h>

#include "ADSGraphNode.generated.h"

class UADSDialoguePartecipantComponent;
class APlayerController;
class UAudioComponent;
class USoundCue;

/**
 *
 */
UCLASS(abstract)
class ASCENTDIALOGUESYSTEM_API UADSGraphNode : public UAGSGraphNode {
    GENERATED_BODY()

    friend class UADSDialogue;

public:
    UFUNCTION(BlueprintPure, Category = ADS)
    UADSDialoguePartecipantComponent* GetDialogueParticipant() const;

    UFUNCTION(BlueprintPure, Category = ADS)
    USoundCue* GetSoundToPlay() const
    {
        return SoundToPlay;
    }

    UFUNCTION(BlueprintPure, Category = ADS)
    bool IsLocalPlayerPartecipant() const;

    UFUNCTION(BlueprintCallable, Category = ADS)
    virtual bool CanBeActivated(class APlayerController* inController);

    UFUNCTION(BlueprintPure, Category = ADS)
    FORCEINLINE FText GetDialogueText() const { return Text; }

#if WITH_EDITOR
    virtual FText GetNodeTitle() const override;
    virtual FText GetParagraphTitle() const override;

    virtual void InitializeNode() override;

#endif
protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ADS")
    FGameplayTag PartecipantTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "ADS")
    TArray<class UAGSAction*> ActivationActions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ADS|Dialogue")
    class USoundCue* SoundToPlay;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ADS|Dialogue")
    class UAnimMontage* Animation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ADS|Dialogue")
    class UAnimMontage* FacialAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = "true"), Category = "ADS|Dialogue")
    FText Text;

    virtual void ActivateNode() override;

    virtual void DeactivateNode() override;

    UADSDialoguePartecipantComponent* GatherPartecipant() const;

    FText ContextMenuName;

    // References
    TObjectPtr<UADSDialoguePartecipantComponent> partecipant;

    TObjectPtr<UAudioComponent> audioPlayer;
    TObjectPtr<APlayerController> controller;
};
