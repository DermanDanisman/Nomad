// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/StatusEffect/SurvivalHazard/NomadSurvivalStatusEffect.h"
#include "ARSStatisticsComponent.h"
#include "Components/ACFCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Core/Debug/NomadLogCategories.h"

UNomadSurvivalStatusEffect::UNomadSurvivalStatusEffect()
{
    CurrentSeverity = ESurvivalSeverity::None;
    DoTPercent = 0.0f;
    LastDamageDealt = 0.0f;
    bModifierApplied = false;
    bBoundToARSDelegate = false;
}

// === LIFECYCLE MANAGEMENT ===

void UNomadSurvivalStatusEffect::OnStatusEffectStarts_Implementation(ACharacter* Character)
{
    Super::OnStatusEffectStarts_Implementation(Character);
    
    if (!Character) return;
    
    UE_LOG_AFFLICTION(Log, TEXT("[SURVIVAL] Starting survival effect on %s"), *Character->GetName());
    
    // Bind to ARS delegate system for automatic synchronization
    BindToARSDelegate();
    
    // Apply configuration modifiers (movement speed + stamina penalties)
    ApplyConfigurationModifiers(Character);
    
    // Apply visual effects
    ApplyVisualEffects();
    
    UE_LOG_AFFLICTION(Log, TEXT("[SURVIVAL] Survival effect successfully started"));
}

void UNomadSurvivalStatusEffect::OnStatusEffectEnds_Implementation()
{
    UE_LOG_AFFLICTION(Log, TEXT("[SURVIVAL] Beginning survival effect removal and recovery"));
    
    // === STEP 1: REMOVE ATTRIBUTE MODIFIERS (RESTORES PENALTIES) ===
    if (bModifierApplied && CharacterOwner)
    {
        UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
        if (StatsComp)
        {
            // Get values BEFORE removal for logging
            const FGameplayTag MovementTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Attributes.MovementSpeed"));
            const FGameplayTag EnduranceTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Attributes.Endurance"));
            const FGameplayTag StaminaTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Statistics.Stamina"));
            
            float PenalizedMovement = StatsComp->GetCurrentAttributeValue(MovementTag);
            float PenalizedEndurance = StatsComp->GetCurrentPrimaryAttributeValue(EnduranceTag);
            float PenalizedStaminaMax = StatsComp->GetMaxValueForStatitstic(StaminaTag);
            
            // CRITICAL: Remove the modifier to restore normal values
            StatsComp->RemoveAttributeSetModifier(AppliedModifier);
            bModifierApplied = false;
            
            // Get values AFTER removal for verification
            float RestoredMovement = StatsComp->GetCurrentAttributeValue(MovementTag);
            float RestoredEndurance = StatsComp->GetCurrentPrimaryAttributeValue(EnduranceTag);
            float RestoredStaminaMax = StatsComp->GetMaxValueForStatitstic(StaminaTag);
            
            UE_LOG_AFFLICTION(Log, TEXT("[SURVIVAL] RECOVERY - Movement: %f -> %f"), 
                              PenalizedMovement, RestoredMovement);
            UE_LOG_AFFLICTION(Log, TEXT("[SURVIVAL] RECOVERY - Endurance: %f -> %f"), 
                              PenalizedEndurance, RestoredEndurance);
            UE_LOG_AFFLICTION(Log, TEXT("[SURVIVAL] RECOVERY - Stamina Max: %f -> %f"), 
                              PenalizedStaminaMax, RestoredStaminaMax);
            
            UE_LOG_AFFLICTION(Log, TEXT("[SURVIVAL] Attribute modifiers removed - penalties recovered"));
        }
        else
        {
            UE_LOG_AFFLICTION(Error, TEXT("[SURVIVAL] Cannot recover - no ARS component found"));
        }
    }
    else
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[SURVIVAL] No modifiers to remove (already recovered or never applied)"));
    }
    
    // === STEP 4: UNBIND FROM DELEGATES ===
    UnbindFromARSDelegate();
    
    // === STEP 5: REMOVE VISUAL EFFECTS ===
    RemoveVisualEffects();
    
    // === STEP 6: RESET INTERNAL STATE ===
    CurrentSeverity = ESurvivalSeverity::None;
    DoTPercent = 0.0f;
    LastDamageDealt = 0.0f;
    
    // Call parent cleanup
    Super::OnStatusEffectEnds_Implementation();
    
    UE_LOG_AFFLICTION(Log, TEXT("[SURVIVAL] Complete recovery finished - all penalties removed"));
}

void UNomadSurvivalStatusEffect::HandleInfiniteTick()
{
    Super::HandleInfiniteTick();
    
    // Apply damage over time if configured
    if (DoTPercent > 0.0f && CharacterOwner)
    {
        UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
        if (StatsComp)
        {
            const FGameplayTag HealthTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Statistics.Health"));
            const float MaxHealth = StatsComp->GetMaxValueForStatitstic(HealthTag);
            
            if (MaxHealth > 0.0f)
            {
                const float DamageAmount = MaxHealth * DoTPercent;
                StatsComp->ModifyStatistic(HealthTag, -DamageAmount);
                LastDamageDealt = DamageAmount;
                
                UE_LOG_AFFLICTION(Verbose, TEXT("[SURVIVAL] Applied DoT damage: %f"), DamageAmount);
            }
        }
    }
}

// === CONFIGURATION APPLICATION ===

void UNomadSurvivalStatusEffect::ApplyConfigurationModifiers(ACharacter* Character)
{
    if (!Character) return;
    
    const UNomadInfiniteEffectConfig* Config = GetEffectConfig();
    if (!Config) 
    {
        UE_LOG_AFFLICTION(Error, TEXT("[SURVIVAL] No config found - cannot apply penalties"));
        return;
    }
    
    UARSStatisticsComponent* StatsComp = Character->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[SURVIVAL] No ARS Statistics Component found"));
        return;
    }
    
    const FAttributesSetModifier& Modifier = Config->PersistentAttributeModifier;
    
    // Apply modifier to ARS (automatically replicates)
    StatsComp->AddAttributeSetModifier(Modifier);
    
    // Store for removal later
    AppliedModifier = Modifier;
    bModifierApplied = true;
    
    UE_LOG_AFFLICTION(Log, TEXT("[SURVIVAL] Applied modifiers: %d attributes, %d primary, %d statistics"), 
                      Modifier.AttributesMod.Num(), Modifier.PrimaryAttributesMod.Num(), Modifier.StatisticsMod.Num());
}

// === MULTIPLAYER-SAFE SYNCHRONIZATION ===

void UNomadSurvivalStatusEffect::BindToARSDelegate()
{
    if (!CharacterOwner || bBoundToARSDelegate) return;
    
    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (StatsComp)
    {
        // Always unbind first to prevent double binding
        StatsComp->OnAttributeSetModified.RemoveDynamic(this, &UNomadSurvivalStatusEffect::OnAttributeSetChanged);
        StatsComp->OnAttributeSetModified.AddDynamic(this, &UNomadSurvivalStatusEffect::OnAttributeSetChanged);
        bBoundToARSDelegate = true;
        
        UE_LOG_AFFLICTION(Verbose, TEXT("[SURVIVAL] Bound to ARS delegate (with cleanup)"));
    }
}

void UNomadSurvivalStatusEffect::UnbindFromARSDelegate()
{
    if (!CharacterOwner || !bBoundToARSDelegate) return;
    
    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (StatsComp)
    {
        if (StatsComp->OnAttributeSetModified.IsAlreadyBound(this, &UNomadSurvivalStatusEffect::OnAttributeSetChanged))
        {
            StatsComp->OnAttributeSetModified.RemoveDynamic(this, &UNomadSurvivalStatusEffect::OnAttributeSetChanged);
            bBoundToARSDelegate = false;
            
            UE_LOG_AFFLICTION(Verbose, TEXT("[SURVIVAL] Unbound from ARS delegate"));
        }
    }
}

void UNomadSurvivalStatusEffect::OnAttributeSetChanged()
{
    // This is called automatically whenever ARS attributes change
    // It handles both server and client synchronization
    if (!CharacterOwner) return;
}

// === UTILITY FUNCTIONS ===

void UNomadSurvivalStatusEffect::SetSeverityLevel(ESurvivalSeverity InSeverity)
{
    ESurvivalSeverity PreviousSeverity = CurrentSeverity;
    CurrentSeverity = InSeverity;
    
    if (CharacterOwner)
    {
        UE_LOG_AFFLICTION(Log, TEXT("[SURVIVAL] Severity changed: %d -> %d"), 
                          (int32)PreviousSeverity, (int32)CurrentSeverity);
    }
}

void UNomadSurvivalStatusEffect::SetDoTPercent(float InDoTPercent)
{
    DoTPercent = FMath::Max(0.0f, InDoTPercent);
}