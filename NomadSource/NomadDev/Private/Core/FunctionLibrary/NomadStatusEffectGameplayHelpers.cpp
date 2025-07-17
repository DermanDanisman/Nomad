// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/FunctionLibrary/NomadStatusEffectGameplayHelpers.h"

#include "ARSStatisticsComponent.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Components/ACFCharacterMovementComponent.h"
#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
// Movement speed modifications are now handled through existing status effect types with configs
#include "GameFramework/CharacterMovementComponent.h"

void UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromStat(ACharacter* Character)
{
    // DEPRECATED: Wrapper for backward compatibility
    // Syncs ARS "RPG.Attributes.MovementSpeed" to ACF movement component MaxWalkSpeed.
    SyncMovementSpeedFromDefaultAttribute(Character);
}

void UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromAttribute(ACharacter* Character, const FGameplayTag& AttributeTag)
{
    // Syncs movement speed from a configurable attribute tag to ACF movement component.
    // This replaces hardcoded attribute tags with a configurable approach.
    if (!Character) return;
    
    const UARSStatisticsComponent* StatsComp = Character->FindComponentByClass<UARSStatisticsComponent>();
    UACFCharacterMovementComponent* MoveComp = Character->FindComponentByClass<UACFCharacterMovementComponent>();
    if (!StatsComp || !MoveComp) return;

    // Get the current value of the specified movement speed attribute
    const float NewSpeed = StatsComp->GetCurrentAttributeValue(AttributeTag);
    if (NewSpeed > 0.f)
    {
        MoveComp->MaxWalkSpeed = NewSpeed;
    }
}

void UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromDefaultAttribute(ACharacter* Character)
{
    // Syncs movement speed using the default RPG.Attributes.MovementSpeed attribute.
    // This is the recommended method that reduces hardcoded tag usage.
    static const FGameplayTag DefaultMovementSpeedTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Attributes.MovementSpeed"));
    SyncMovementSpeedFromAttribute(Character, DefaultMovementSpeedTag);
}

bool UNomadStatusEffectGameplayHelpers::IsSprintBlocked(ACharacter* Character)
{
    // Checks if the character is currently blocked from sprinting by an active status effect.
    // This is true if the "Status.Block.Sprint" tag is active in the status effect manager.
    static const FGameplayTag SprintBlockTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Block.Sprint"));
    return IsActionBlocked(Character, SprintBlockTag);
}

bool UNomadStatusEffectGameplayHelpers::IsJumpBlocked(ACharacter* Character)
{
    // Checks if the character is currently blocked from jumping by an active status effect.
    // This is true if the "Status.Block.Jump" tag is active in the status effect manager.
    static const FGameplayTag JumpBlockTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Block.Jump"));
    return IsActionBlocked(Character, JumpBlockTag);
}

bool UNomadStatusEffectGameplayHelpers::IsActionBlocked(ACharacter* Character, const FGameplayTag& BlockingTag)
{
    // Generic method to check if any action is blocked by active status effects.
    // This reduces code duplication and provides a flexible blocking system.
    if (!Character || !BlockingTag.IsValid()) return false;
    
    auto* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>();
    return (SEManager && SEManager->HasBlockingTag(BlockingTag));
}

void UNomadStatusEffectGameplayHelpers::ApplyMovementSpeedModifierToState(
    UACFCharacterMovementComponent* MoveComp,
    ELocomotionState State,
    float Multiplier,
    const FGuid& Guid)
{
    /**
     * DEPRECATED: Applies a movement speed modifier to the given locomotion state.
     * This method is kept for backward compatibility but should be replaced with status effects.
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
     * DEPRECATED: Removes a movement speed modifier from the given locomotion state if the Guid matches.
     * This method is kept for backward compatibility but should be replaced with status effects.
     */
    if (!MoveComp) return;
    FACFLocomotionState* LocState = MoveComp->GetLocomotionStateStruct(State);
    if (!LocState) return;

    if (LocState->StateModifier.Guid == Guid)
    {
        LocState->StateModifier.AttributesMod.Empty();
    }
}

void UNomadStatusEffectGameplayHelpers::ApplyMovementSpeedStatusEffect(
    ACharacter* Character,
    TSubclassOf<UNomadBaseStatusEffect> StatusEffectClass,
    float Duration)
{
    /**
     * NEW: Applies a movement speed effect through the status effect system.
     * This is the recommended approach for temporary movement speed modifications.
     */
    if (!Character || !StatusEffectClass) return;
    
    auto* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>();
    if (!SEManager) return;
    
    // Apply the status effect - it will handle movement speed modifiers via its config
    if (Duration > 0.0f)
    {
        // Apply timed effect
        SEManager->ApplyTimedStatusEffect(StatusEffectClass, Duration);
    }
    else
    {
        // Apply infinite effect
        SEManager->ApplyInfiniteStatusEffect(StatusEffectClass);
    }
    
    // Sync movement speed after applying the effect
    SyncMovementSpeedFromDefaultAttribute(Character);
}

void UNomadStatusEffectGameplayHelpers::RemoveMovementSpeedStatusEffect(
    ACharacter* Character,
    const FGameplayTag& EffectTag)
{
    /**
     * NEW: Removes a movement speed effect by its gameplay tag.
     */
    if (!Character || !EffectTag.IsValid()) return;
    
    auto* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>();
    if (!SEManager) return;
    
    // Remove the status effect by tag
    SEManager->Nomad_RemoveStatusEffect(EffectTag);
    
    // Sync movement speed after removing the effect
    SyncMovementSpeedFromDefaultAttribute(Character);
}

bool UNomadStatusEffectGameplayHelpers::HasActiveMovementSpeedEffects(ACharacter* Character)
{
    /**
     * NEW: Checks if any movement speed effects are currently active.
     */
    if (!Character) return false;
    
    auto* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>();
    if (!SEManager) return false;
    
    // Check for common movement speed effect tags
    static const TArray<FGameplayTag> MovementEffectTags = {
        FGameplayTag::RequestGameplayTag("StatusEffect.Movement.SpeedBoost"),
        FGameplayTag::RequestGameplayTag("StatusEffect.Movement.SpeedPenalty"),
        FGameplayTag::RequestGameplayTag("StatusEffect.Movement.Disabled"),
        FGameplayTag::RequestGameplayTag("StatusEffect.Survival.Starvation"),
        FGameplayTag::RequestGameplayTag("StatusEffect.Survival.Dehydration"),
        FGameplayTag::RequestGameplayTag("StatusEffect.Survival.Heatstroke"),
        FGameplayTag::RequestGameplayTag("StatusEffect.Survival.Hypothermia")
    };
    
    for (const FGameplayTag& Tag : MovementEffectTags)
    {
        if (SEManager->HasStatusEffect(Tag))
        {
            return true;
        }
    }
    
    return false;
}

TArray<FGameplayTag> UNomadStatusEffectGameplayHelpers::GetActiveMovementSpeedEffectTags(ACharacter* Character)
{
    /**
     * NEW: Gets all active movement speed effect tags on the character.
     */
    TArray<FGameplayTag> ActiveTags;
    
    if (!Character) return ActiveTags;
    
    auto* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>();
    if (!SEManager) return ActiveTags;
    
    // Check for common movement speed effect tags
    static const TArray<FGameplayTag> MovementEffectTags = {
        FGameplayTag::RequestGameplayTag("StatusEffect.Movement.SpeedBoost"),
        FGameplayTag::RequestGameplayTag("StatusEffect.Movement.SpeedPenalty"),
        FGameplayTag::RequestGameplayTag("StatusEffect.Movement.Disabled"),
        FGameplayTag::RequestGameplayTag("StatusEffect.Survival.Starvation"),
        FGameplayTag::RequestGameplayTag("StatusEffect.Survival.Dehydration"),
        FGameplayTag::RequestGameplayTag("StatusEffect.Survival.Heatstroke"),
        FGameplayTag::RequestGameplayTag("StatusEffect.Survival.Hypothermia")
    };
    
    for (const FGameplayTag& Tag : MovementEffectTags)
    {
        if (SEManager->HasStatusEffect(Tag))
        {
            ActiveTags.Add(Tag);
        }
    }
    
    return ActiveTags;
}

void UNomadStatusEffectGameplayHelpers::ApplySurvivalMovementPenalty(
    ACharacter* Character,
    ESurvivalSeverity PenaltyLevel)
{
    /**
     * Helper method to apply standard survival movement penalty.
     * 
     * NOTE: This method provides a simplified interface, but the recommended approach
     * is to use UNomadSurvivalStatusEffect directly with appropriate config assets
     * that define PersistentAttributeModifier for movement speed changes and 
     * BlockingTags for input restrictions.
     * 
     * Example config setup:
     * - PersistentAttributeModifier: Set movement speed multiplier (0.8 for 20% penalty)
     * - BlockingTags: Include "Status.Block.Sprint" for severe penalties
     * - EffectTag: Use "StatusEffect.Survival.MovementPenalty.Mild/Heavy/Severe"
     */
    if (!Character) return;
    
    auto* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>();
    if (!SEManager) return;
    
    // Remove any existing survival movement penalty first
    RemoveSurvivalMovementPenalty(Character);
    
    // Get appropriate survival status effect tag based on penalty level
    FGameplayTag EffectTag;
    switch (PenaltyLevel)
    {
        case ESurvivalSeverity::Mild:
            EffectTag = FGameplayTag::RequestGameplayTag("StatusEffect.Survival.MovementPenalty.Mild");
            break;
        case ESurvivalSeverity::Heavy:
            EffectTag = FGameplayTag::RequestGameplayTag("StatusEffect.Survival.MovementPenalty.Heavy");
            break;
        case ESurvivalSeverity::Severe:
        case ESurvivalSeverity::Extreme:
            EffectTag = FGameplayTag::RequestGameplayTag("StatusEffect.Survival.MovementPenalty.Severe");
            break;
        default:
            return; // No penalty for None severity
    }
    
    // TODO: Apply survival status effect with the appropriate tag
    // This requires creating config assets for each severity level with proper
    // PersistentAttributeModifier values for movement speed reduction
    // 
    // Example: SEManager->ApplyInfiniteStatusEffect(UNomadSurvivalStatusEffect::StaticClass(), EffectTag);
    
    UE_LOG_AFFLICTION(Warning, TEXT("[SURVIVAL] ApplySurvivalMovementPenalty requires config assets for tag: %s"), 
        *EffectTag.ToString());
}

void UNomadStatusEffectGameplayHelpers::RemoveSurvivalMovementPenalty(ACharacter* Character)
{
    /**
     * Helper method to remove survival movement penalties.
     * Removes all survival-related movement penalty effects.
     */
    if (!Character) return;
    
    auto* SEManager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>();
    if (!SEManager) return;
    
    // Remove all types of survival-related movement penalties using their gameplay tags
    FGameplayTag MildTag = FGameplayTag::RequestGameplayTag("StatusEffect.Survival.MovementPenalty.Mild");
    FGameplayTag HeavyTag = FGameplayTag::RequestGameplayTag("StatusEffect.Survival.MovementPenalty.Heavy");
    FGameplayTag SevereTag = FGameplayTag::RequestGameplayTag("StatusEffect.Survival.MovementPenalty.Severe");
    
    if (MildTag.IsValid()) SEManager->Nomad_RemoveStatusEffect(MildTag);
    if (HeavyTag.IsValid()) SEManager->Nomad_RemoveStatusEffect(HeavyTag);
    if (SevereTag.IsValid()) SEManager->Nomad_RemoveStatusEffect(SevereTag);
    
    // Sync movement speed after removing effects
    SyncMovementSpeedFromDefaultAttribute(Character);
}