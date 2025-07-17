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
    CurrentSeverity = InSeverity;
}

void UNomadSurvivalStatusEffect::SetDoTPercent(float InDoTPercent)
{
    DoTPercent = FMath::Max(0.0f, InDoTPercent); // Ensure non-negative
}

void UNomadSurvivalStatusEffect::OnStatusEffectStarts_Implementation(ACharacter* Character)
{
    // Call parent implementation first to handle base setup
    Super::OnStatusEffectStarts_Implementation(Character);
    
    // Apply visual effects appropriate for this survival condition
    // This will be implemented in Blueprint for each specific effect type
    ApplyVisualEffects();
}

void UNomadSurvivalStatusEffect::OnStatusEffectEnds_Implementation()
{
    // Remove visual effects before parent cleanup
    RemoveVisualEffects();
    
    // Call parent implementation to handle base cleanup
    Super::OnStatusEffectEnds_Implementation();
}

void UNomadSurvivalStatusEffect::HandleInfiniteTick()
{
    // Call parent implementation first (handles config-based stat modifications)
    Super::HandleInfiniteTick();
    
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