// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.


#include "Core/Player/NomadPlayerState.h"
#include "Interface/CharacterCustomizationInterface.h"
#include "Net/UnrealNetwork.h"

ANomadPlayerState::ANomadPlayerState()
{
    // Enable replication for this PlayerState
    bReplicates = true;
}

void ANomadPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Register our CustomizationState for replication
    DOREPLIFETIME_CONDITION_NOTIFY(ANomadPlayerState, CustomizationState, COND_None, REPNOTIFY_Always);
}

void ANomadPlayerState::OnRep_CustomizationState_PS()
{
    TryApplyCustomizationToPawn();
}

void ANomadPlayerState::CopyProperties(APlayerState* PlayerState)
{
    Super::CopyProperties(PlayerState);

    // If the destination is also ANomadPlayerState, copy our customization data
    if (ANomadPlayerState* Other = Cast<ANomadPlayerState>(PlayerState))
    {
        Other->CustomizationState = CustomizationState;
    }
}

void ANomadPlayerState::Reset()
{
    Super::Reset();
    // By default, we don't clear CustomizationState across seamless travel
    // If you want to reset to defaults, uncomment:
    // CustomizationState = FMultiplayerPlayerCustomizationState();
}

void ANomadPlayerState::SetCustomizationState(const FMultiplayerPlayerCustomizationState& NewState)
{
    // 1) Server-authoritative assignment of the new struct
    CustomizationState = NewState;
    OnRep_CustomizationState_PS();
    //ServerSetCustomizationState(NewState);
}
void ANomadPlayerState::TryApplyCustomizationToPawn()
{
    APawn* P = GetPawn();
    if (P && P->Implements<UCharacterCustomizationInterface>())
    {
        // Apply customization immediately
        ICharacterCustomizationInterface::Execute_ApplyCustomization(P, CustomizationState);

        // Stop the timer if it's running
        if (CustomizationApplyTimerHandle.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(CustomizationApplyTimerHandle);
        }
        bPendingCustomizationApply = false;
    }
    else
    {
        // Pawn not ready yet—start/restart timer
        if (!bPendingCustomizationApply)
        {
            bPendingCustomizationApply = true;
            GetWorld()->GetTimerManager().SetTimer(
                CustomizationApplyTimerHandle,
                this,
                &ANomadPlayerState::TryApplyCustomizationToPawn,
                0.2f,     // Retry every 0.2 seconds
                true      // Looping
            );
        }
    }
}
