// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/PlayerCharacterCustomizationData.h"
#include "GameFramework/PlayerState.h"
#include "NomadPlayerState.generated.h"

/**
 *
 */
UCLASS()
class NOMADDEV_API ANomadPlayerState : public APlayerState
{
	GENERATED_BODY()
public:

    ANomadPlayerState();

    /**
     * Override replication setup to register our custom properties.
     */
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // -------------------------------------------------------------
    // Customization State
    // -------------------------------------------------------------

    /**
     * Stores the player's chosen mesh/color indices for each slot.
     * Replicated to all clients; triggers OnRep when updated.
     */
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Replicated, ReplicatedUsing = OnRep_CustomizationState_PS, SaveGame)
    FMultiplayerPlayerCustomizationState CustomizationState;

    /**
     * Called on clients when CustomizationState changes.
     * Drives visual update via CharacterCustomizationInterface.
     */
    UFUNCTION()
    void OnRep_CustomizationState_PS();

    // -------------------------------------------------------------
    // Preserve across seamless travel and disconnects
    // -------------------------------------------------------------

    /**
     *  Called when copying PlayerState (seamless travel / disconnect)
     */
    virtual void CopyProperties(APlayerState* PlayerState) override;

    /**
     *  Reset is called on seamless travel; clear transient state here if needed.
     */
    virtual void Reset() override;

    UFUNCTION(BlueprintCallable)
    void SetCustomizationState(const FMultiplayerPlayerCustomizationState& NewState);

    UFUNCTION(BlueprintCallable)
    void TryApplyCustomizationToPawn();

private:

    FTimerHandle CustomizationApplyTimerHandle;
    bool bPendingCustomizationApply = false;

};
