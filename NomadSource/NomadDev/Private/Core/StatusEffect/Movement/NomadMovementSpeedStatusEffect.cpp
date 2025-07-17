// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/StatusEffect/Movement/NomadMovementSpeedStatusEffect.h"
#include "Core/Debug/NomadLogCategories.h"
#include "Core/FunctionLibrary/NomadStatusEffectGameplayHelpers.h"
#include "GameFramework/Character.h"
#include "ARSStatisticsComponent.h"

// =====================================================
//         BASE MOVEMENT SPEED STATUS EFFECT
// =====================================================

UNomadMovementSpeedStatusEffect::UNomadMovementSpeedStatusEffect()
{
    // Movement effects are infinite duration by default - they persist until conditions change
    // They should be non-stacking for simplicity unless specifically configured otherwise
    MovementSpeedMultiplier = 1.0f;
    bHasAppliedMovementModifiers = false;
    bHasAppliedInputBlocking = false;
}

void UNomadMovementSpeedStatusEffect::SetMovementSpeedMultiplier(float InMultiplier)
{
    MovementSpeedMultiplier = FMath::Max(0.0f, InMultiplier); // Ensure non-negative multiplier
    
    // If effect is already active, update the movement speed immediately
    if (CharacterOwner && bHasAppliedMovementModifiers)
    {
        UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromDefaultAttribute(CharacterOwner);
    }
}

FText UNomadMovementSpeedStatusEffect::GetMovementEffectDisplayText() const
{
    // Generate user-friendly display text for UI
    if (FMath::IsNearlyEqual(MovementSpeedMultiplier, 1.0f, 0.01f))
    {
        return FText::FromString(TEXT("Movement Speed: No Change"));
    }
    else if (MovementSpeedMultiplier > 1.0f)
    {
        const int32 PercentIncrease = FMath::RoundToInt((MovementSpeedMultiplier - 1.0f) * 100.0f);
        return FText::FromString(FString::Printf(TEXT("Movement Speed: +%d%%"), PercentIncrease));
    }
    else
    {
        const int32 PercentDecrease = FMath::RoundToInt((1.0f - MovementSpeedMultiplier) * 100.0f);
        return FText::FromString(FString::Printf(TEXT("Movement Speed: -%d%%"), PercentDecrease));
    }
}

void UNomadMovementSpeedStatusEffect::OnStatusEffectStarts_Implementation(ACharacter* Character)
{
    // Call parent implementation first to handle base setup and config loading
    Super::OnStatusEffectStarts_Implementation(Character);
    
    if (!Character)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[MOVEMENT] Cannot start movement effect - no character"));
        return;
    }
    
    // Mark that we've applied movement modifiers for tracking
    bHasAppliedMovementModifiers = true;
    
    // The actual movement speed modification is handled by PersistentAttributeModifier in the config
    // We just need to sync the movement component to reflect the new attribute values
    UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromDefaultAttribute(Character);
    
    // Apply visual/audio effects for this movement effect
    ApplyMovementVisualEffects();
    
    UE_LOG_AFFLICTION(Log, TEXT("[MOVEMENT] Movement speed effect started (multiplier: %f)"), MovementSpeedMultiplier);
}

void UNomadMovementSpeedStatusEffect::OnStatusEffectEnds_Implementation()
{
    // Remove visual effects before parent cleanup
    if (bHasAppliedMovementModifiers)
    {
        RemoveMovementVisualEffects();
        bHasAppliedMovementModifiers = false;
    }
    
    // Call parent implementation to handle base cleanup (this also removes attribute modifiers)
    Super::OnStatusEffectEnds_Implementation();
    
    // Sync movement speed after removing the effect to reflect updated attributes
    if (CharacterOwner)
    {
        UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromDefaultAttribute(CharacterOwner);
    }
    
    UE_LOG_AFFLICTION(Log, TEXT("[MOVEMENT] Movement speed effect ended"));
}

void UNomadMovementSpeedStatusEffect::HandleInfiniteTick()
{
    // Call parent implementation first (handles config-based stat modifications)
    Super::HandleInfiniteTick();
    
    // Movement effects typically don't need periodic ticking,
    // but this can be overridden for dynamic movement adjustments
    // (e.g., movement speed that changes based on environment)
    
    // Ensure movement speed is properly synced on each tick if needed
    if (CharacterOwner && bHasAppliedMovementModifiers)
    {
        UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromDefaultAttribute(CharacterOwner);
    }
}