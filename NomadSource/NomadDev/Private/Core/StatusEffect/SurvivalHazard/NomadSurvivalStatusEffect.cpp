// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/StatusEffect/SurvivalHazard/NomadSurvivalStatusEffect.h"
#include "ARSStatisticsComponent.h"
#include "GameFramework/Character.h"
#include "Core/Debug/NomadLogCategories.h"

// =====================================================
//         BASE SURVIVAL STATUS EFFECT
// =====================================================

UNomadSurvivalStatusEffect::UNomadSurvivalStatusEffect()
{
    // Survival effects are infinite duration - they persist until conditions improve
    // They should be non-stacking since multiple severity levels don't make sense
    CurrentSeverity = ESurvivalSeverity::None;
    DoTPercent = 0.0f;
    LastDamageDealt = 0.0f;
}

void UNomadSurvivalStatusEffect::SetSeverityLevel(ESurvivalSeverity InSeverity)
{
    ESurvivalSeverity PreviousSeverity = CurrentSeverity;
    CurrentSeverity = InSeverity;
    
    // Handle jump blocking based on severity changes
    if (CharacterOwner)
    {
        // If transitioning to severe/extreme from mild/none, apply jump blocking
        bool bWasSevere = (PreviousSeverity == ESurvivalSeverity::Severe || PreviousSeverity == ESurvivalSeverity::Extreme);
        bool bIsSevere = (CurrentSeverity == ESurvivalSeverity::Severe || CurrentSeverity == ESurvivalSeverity::Extreme);
        
        if (!bWasSevere && bIsSevere)
        {
            // Apply jump blocking when transitioning to severe condition
            ApplyJumpBlockTag(CharacterOwner);
            UE_LOG_AFFLICTION(Log, TEXT("[SURVIVAL] Applied jump blocking due to severity increase to %s"), 
                              *StaticEnum<ESurvivalSeverity>()->GetNameStringByValue((int64)CurrentSeverity));
        }
        else if (bWasSevere && !bIsSevere)
        {
            // Remove jump blocking when transitioning away from severe condition
            RemoveJumpBlockTag(CharacterOwner);
            UE_LOG_AFFLICTION(Log, TEXT("[SURVIVAL] Removed jump blocking due to severity decrease to %s"), 
                              *StaticEnum<ESurvivalSeverity>()->GetNameStringByValue((int64)CurrentSeverity));
        }
        
        // Always sync movement speed when severity changes as modifiers may change
        SyncMovementSpeedModifier(CharacterOwner, 1.0f);
    }
}

void UNomadSurvivalStatusEffect::SetDoTPercent(float InDoTPercent)
{
    DoTPercent = FMath::Max(0.0f, InDoTPercent); // Ensure non-negative
}

void UNomadSurvivalStatusEffect::OnStatusEffectStarts_Implementation(ACharacter* Character)
{
    // Call parent implementation first to handle base setup
    Super::OnStatusEffectStarts_Implementation(Character);
    
    // Apply blocking tags for severe survival conditions
    if (CurrentSeverity == ESurvivalSeverity::Severe || CurrentSeverity == ESurvivalSeverity::Extreme)
    {
        // Block jumping for severe survival conditions to prevent unrealistic mobility
        ApplyJumpBlockTag(Character);
        
        UE_LOG_AFFLICTION(Log, TEXT("[SURVIVAL] Applied jump blocking for severe %s effect"), 
                          *GetStatusEffectTag().ToString());
    }
    
    // Sync movement speed modifiers to ensure proper application of movement penalties
    // The actual speed modification is handled by the config-based attribute modifiers,
    // but we need to sync the movement component to reflect the changes
    SyncMovementSpeedModifier(Character, 1.0f); // Use 1.0f as multiplier since actual modifier comes from config
    
    // Apply visual effects appropriate for this survival condition
    // This will be implemented in Blueprint for each specific effect type
    ApplyVisualEffects();
}

void UNomadSurvivalStatusEffect::OnStatusEffectEnds_Implementation()
{
    // Remove blocking tags that were applied during effect start
    if (CurrentSeverity == ESurvivalSeverity::Severe || CurrentSeverity == ESurvivalSeverity::Extreme)
    {
        // Remove jump blocking when severe survival conditions end
        RemoveJumpBlockTag(CharacterOwner);
        
        UE_LOG_AFFLICTION(Log, TEXT("[SURVIVAL] Removed jump blocking for ended %s effect"), 
                          *GetStatusEffectTag().ToString());
    }
    
    // Remove movement speed modifiers and sync to ensure proper cleanup
    RemoveMovementSpeedModifier(CharacterOwner);
    
    // Remove visual effects before parent cleanup
    RemoveVisualEffects();
    
    // Call parent implementation to handle base cleanup
    Super::OnStatusEffectEnds_Implementation();
}

void UNomadSurvivalStatusEffect::HandleInfiniteTick()
{
    // Call parent implementation first (handles config-based stat modifications)
    Super::HandleInfiniteTick();
    
    // Ensure movement speed stays properly synced during long-running effects
    // This helps maintain consistency in multiplayer and after other system changes
    if (CharacterOwner)
    {
        SyncMovementSpeedModifier(CharacterOwner, 1.0f);
    }
    
    // Apply damage over time if configured
    if (DoTPercent > 0.0f && CharacterOwner)
    {
        UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
        if (StatsComp)
        {
            // Get current max health to calculate damage amount
            const FGameplayTag HealthTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Statistics.Health"));
            const float MaxHealth = StatsComp->GetMaxValueForStatitstic(HealthTag);
            
            if (MaxHealth > 0.0f)
            {
                // Calculate damage: percentage of max health per tick
                // DoTPercent is expected to be a small decimal (e.g., 0.005 for 0.5% per tick)
                const float DamageAmount = MaxHealth * DoTPercent;
                
                // Apply damage (negative value reduces health)
                StatsComp->ModifyStatistic(HealthTag, -DamageAmount);
                LastDamageDealt = DamageAmount;
            }
        }
    }
}