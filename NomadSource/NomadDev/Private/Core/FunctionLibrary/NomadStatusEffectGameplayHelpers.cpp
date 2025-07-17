// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/FunctionLibrary/NomadStatusEffectGameplayHelpers.h"

#include "ARSStatisticsComponent.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Components/ACFCharacterMovementComponent.h"
#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromStat(ACharacter* Character)
{
    // Syncs ARS "RPG.Attributes.MovementSpeed" to ACF movement component MaxWalkSpeed.
    // Should be called whenever the MovementSpeed stat changes (from effects, items, etc).
    if (!Character) return;
    const UARSStatisticsComponent* StatsComp = Character->FindComponentByClass<UARSStatisticsComponent>();
    UACFCharacterMovementComponent* MoveComp = Character->FindComponentByClass<UACFCharacterMovementComponent>();
    if (!StatsComp || !MoveComp) return;

    // Get the current value of the MovementSpeed attribute from the stat system.
    const float NewSpeed = StatsComp->GetCurrentAttributeValue(FGameplayTag::RequestGameplayTag(TEXT("RPG.Attributes.MovementSpeed")));
    if (NewSpeed > 0.f)
    {
        MoveComp->MaxWalkSpeed = NewSpeed;
    }
}

bool UNomadStatusEffectGameplayHelpers::IsSprintBlocked(ACharacter* Character)
{
    // Checks if the character is currently blocked from sprinting by an active status effect.
    // This is true if the "Status.Block.Sprint" tag is active in the status effect manager.
    static const FGameplayTag SprintBlockTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Block.Sprint"));
    if (!Character) return false;
    auto* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>();
    return (SEManager && SEManager->HasBlockingTag(SprintBlockTag));
}

void UNomadStatusEffectGameplayHelpers::ApplyMovementSpeedModifierToState(
    UACFCharacterMovementComponent* MoveComp,
    ELocomotionState State,
    float Multiplier,
    const FGuid& Guid)
{
    /**
     * Applies a movement speed modifier to the given locomotion state.
     * - If a modifier with the same Guid exists, it is replaced.
     * - Only applies if Multiplier != 1.0 (no change).
     */
    if (!MoveComp) return;
    FACFLocomotionState* LocState = MoveComp->GetLocomotionStateStruct(State);
    if (!LocState) return;

    // Remove any previous mod with this Guid first
    LocState->StateModifier.Guid = Guid;
    LocState->StateModifier.AttributesMod.Empty();

    if (FMath::Abs(Multiplier - 1.0f) > KINDA_SMALL_NUMBER)
    {
        LocState->StateModifier.AttributesMod.Add(
            FAttributeModifier(
                FGameplayTag::RequestGameplayTag(TEXT("RPG.Attributes.MovementSpeed")),
                EModifierType::EMultiplicative,
                Multiplier
            )
        );
    }
}

void UNomadStatusEffectGameplayHelpers::RemoveMovementSpeedModifierFromState(
    UACFCharacterMovementComponent* MoveComp,
    ELocomotionState State,
    const FGuid& Guid)
{
    /**
     * Removes a movement speed modifier from the given locomotion state if the Guid matches.
     */
    if (!MoveComp) return;
    FACFLocomotionState* LocState = MoveComp->GetLocomotionStateStruct(State);
    if (!LocState) return;

    if (LocState->StateModifier.Guid == Guid)
    {
        LocState->StateModifier.AttributesMod.Empty();
    }
}