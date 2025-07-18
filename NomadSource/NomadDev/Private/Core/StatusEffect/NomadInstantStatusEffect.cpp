// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/StatusEffect/NomadInstantStatusEffect.h"
#include "Core/Data/StatusEffect/NomadInstantEffectConfig.h"
#include "ARSStatisticsComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "Core/StatusEffect/Utility/NomadStatusEffectUtils.h"
#include "Core/Debug/NomadLogCategories.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "TimerManager.h"

// =====================================================
//         CONSTRUCTOR & INITIALIZATION
// =====================================================

UNomadInstantStatusEffect::UNomadInstantStatusEffect()
{
    // Initialize analytics state
    LastAppliedValue = 0.0f;

    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[INSTANT] Instant status effect constructed"));
}

// =====================================================
//         CONFIGURATION ACCESS
// =====================================================

UNomadInstantEffectConfig* UNomadInstantStatusEffect::GetEffectConfig() const
{
    // Loads and returns the config asset for this instant effect, or nullptr if not set.
    // Asset is loaded synchronously; safe for runtime use as this effect is instant and short-lived.
    return Cast<UNomadInstantEffectConfig>(EffectConfig.IsNull() ? nullptr : EffectConfig.LoadSynchronous());
}

bool UNomadInstantStatusEffect::HasValidConfiguration() const
{
    // Checks if the configuration asset is set and valid.
    UNomadInstantEffectConfig* Config = GetEffectConfig();
    return Config && Config->IsConfigValid();
}

// =====================================================
//         MANAGER ACTIVATION ENTRYPOINT
// =====================================================

void UNomadInstantStatusEffect::Nomad_OnStatusEffectStarts(ACharacter* Character)
{
    Super::Nomad_OnStatusEffectStarts(Character);
    // Called by the manager or scripts to activate this instant effect.
    // Enables safe polymorphic activation for all effect subclasses.
    OnStatusEffectStarts_Implementation(Character);
}

// =====================================================
//         CORE LIFECYCLE: START / END
// =====================================================

void UNomadInstantStatusEffect::OnStatusEffectStarts_Implementation(ACharacter* Character)
{
    Super::OnStatusEffectStarts_Implementation(Character);

    // Called the moment the instant effect is created and applied to the target.
    // Applies the effect's logic immediately, triggers analytics/UI, and then ends itself.

    UE_LOG_AFFLICTION(Log, TEXT("[INSTANT] Starting instant effect on %s"),
                      Character ? *Character->GetName() : TEXT("Unknown"));

    // Load the configuration asset
    UNomadInstantEffectConfig* Config = GetEffectConfig();
    if (!Config || !Character)
    {
        // Abort if config or character is missing, log error for debug
        UE_LOG_AFFLICTION(Error, TEXT("[INSTANT] Invalid config or character on start"));
        SetEffectLifecycleState(EEffectLifecycleState::Removed);
        return;
    }

    if (!Config->IsConfigValid())
    {
        UE_LOG_AFFLICTION(Error, TEXT("[INSTANT] Config validation failed"));
        SetEffectLifecycleState(EEffectLifecycleState::Removed);
        return;
    }

    // Handle interrupt logic if configured
    if (Config->bInterruptsOtherEffects && !Config->InterruptTags.IsEmpty())
    {
        if (UNomadStatusEffectManagerComponent* Manager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>())
        {
            // Remove effects with matching interrupt tags
            int32 InterruptedCount = 0;
            for (const FGameplayTag& InterruptTag : Config->InterruptTags.GetGameplayTagArray())
            {
                if (Manager->Nomad_RemoveStatusEffectSmart(InterruptTag))
                {
                    InterruptedCount++;
                }
            }

            if (InterruptedCount > 0)
            {
                UE_LOG_AFFLICTION(Log, TEXT("[INSTANT] Interrupted %d effects"), InterruptedCount);
            }
        }
    }

    // Prepare stat modifications array from config
    TArray<FStatisticValue> StatMods = Config->OnApplyStatModifications;

    // Apply temporary attribute modifier if configured
    if (Config->TemporaryAttributeModifier.PrimaryAttributesMod.Num() > 0 ||
        Config->TemporaryAttributeModifier.AttributesMod.Num() > 0 ||
        Config->TemporaryAttributeModifier.StatisticsMod.Num() > 0)
    {
        if (UARSStatisticsComponent* StatsComp = Character->FindComponentByClass<UARSStatisticsComponent>())
        {
            // Apply temporary modifier briefly for calculation purposes
            StatsComp->AddAttributeSetModifier(Config->TemporaryAttributeModifier);

            // Remove it after a very short delay (next frame)
            if (UWorld* World = Character->GetWorld())
            {
                FTimerHandle RemovalTimer;
                World->GetTimerManager().SetTimer(RemovalTimer, [StatsComp, Config]()
                {
                    if (IsValid(StatsComp))
                    {
                        StatsComp->RemoveAttributeSetModifier(Config->TemporaryAttributeModifier);
                    }
                }, 0.001f, false);
            }
        }
    }

    // Hybrid system: apply stat/damage/both in a single call
    ApplyHybridEffect(StatMods, Character, Config);

    // Sync movement speed to reflect any instant changes to movement attributes
    SyncMovementSpeedModifier(Character, 1.0f);

    // Trigger Blueprint and optional C++ event for VFX/SFX/UI notifications
    OnInstantEffectApplied_Implementation(StatMods);
    OnInstantEffectApplied(StatMods);

    // Handle chain effects if configured and with optional delay
    if (Config->bTriggerChainEffects && Config->ChainEffects.Num() > 0)
    {
        if (Config->ChainEffectDelay > 0.0f)
        {
            // Apply chain effects with delay
            if (UWorld* World = Character->GetWorld())
            {
                FTimerHandle ChainTimer;
                World->GetTimerManager().SetTimer(ChainTimer, [this, Character, Config]()
                {
                    ApplyChainEffects(Character, Config);
                }, Config->ChainEffectDelay, false);
            }
        }
        else
        {
            // Apply chain effects immediately
            ApplyChainEffects(Character, Config);
        }
    }

    UE_LOG_AFFLICTION(Log, TEXT("[INSTANT] Effect applied successfully, value: %.2f"), LastAppliedValue);

    // End the effect immediately—instant effects do not persist
    Nomad_OnStatusEffectEnds();
}

void UNomadInstantStatusEffect::OnStatusEffectEnds_Implementation()
{
    // Called after the instant effect finishes, for cleanup and event triggers.
    UE_LOG_AFFLICTION(Log, TEXT("[INSTANT] Instant effect ended"));

    // Transition to removed state to mark effect as finished
    SetEffectLifecycleState(EEffectLifecycleState::Removed);

    // Trigger Blueprint and optional C++ event for VFX/SFX/UI
    OnInstantEffectEnded_Implementation();
    OnInstantEffectEnded();

    // Call parent implementation
    Super::OnStatusEffectEnds_Implementation();
}

// =====================================================
//         HYBRID SYSTEM: STAT/DAMAGE/BOTH APPLICATION
// =====================================================

void UNomadInstantStatusEffect::ApplyHybridEffect(const TArray<FStatisticValue>& StatMods, AActor* Target, UObject* EffectConfigObj)
{
    // Applies stat modifications, damage, or both, based on the effect's configuration.
    // This is the core gameplay logic for instant effects.

    // 1. Safety checks: must have valid target and config
    if (!IsValid(Target) || Target->IsPendingKillPending() || !EffectConfigObj)
        return;

    UNomadInstantEffectConfig* Config = Cast<UNomadInstantEffectConfig>(EffectConfigObj);
    if (!Config) return;

    // 2. Prepare analytics for the total value of health changed (damage or healing)
    float EffectValue = 0.0f;

    // 3. Determine who caused the effect (for analytics, UI, kill credit, etc.)
    AActor* Causer = GetSafeDamageCauser(Target);

    // 4. Switch between the three hybrid modes: stat mod, damage, or both
    switch (Config->ApplicationMode)
    {
    case EStatusEffectApplicationMode::StatModification:
        {
            // --- StatModification: Only stat/attribute changes, no UE damage event
            UARSStatisticsComponent* StatsComp = Target->FindComponentByClass<UARSStatisticsComponent>();
            if (StatsComp)
            {
                UNomadStatusEffectUtils::ApplyStatModifications(StatsComp, StatMods);

                // Track any health modification for analytics
                for (const FStatisticValue& Mod : StatMods)
                {
                    if (Mod.Statistic.MatchesTag(Health))
                        EffectValue += Mod.Value;
                }
            }
        }
        break;

    case EStatusEffectApplicationMode::DamageEvent:
        {
            // --- DamageEvent: Only damage, no stat mods
            if (Config->DamageTypeClass)
            {
                if (Config->DamageStatisticMods.Num() > 0)
                {
                    for (const FStatisticValue& Mod : Config->DamageStatisticMods)
                    {
                        // Only health is supported for direct damage
                        if (Mod.Statistic.MatchesTag(Health) && !FMath::IsNearlyZero(Mod.Value))
                        {
                            // Apply UE damage event for correct gameplay logic (death, aggro, etc.)
                            UGameplayStatics::ApplyDamage(
                                Target,
                                FMath::Abs(Mod.Value), // Always positive
                                nullptr,               // No specific instigator controller
                                Causer,
                                Config->DamageTypeClass
                            );
                            EffectValue += Mod.Value;
                        }
                    }
                }
            }
        }
        break;

    case EStatusEffectApplicationMode::Both:
        {
            // --- Both: Apply stat/attribute mods AND damage
            UARSStatisticsComponent* StatsComp = Target->FindComponentByClass<UARSStatisticsComponent>();
            if (StatsComp)
            {
                UNomadStatusEffectUtils::ApplyStatModifications(StatsComp, StatMods);
            }

            if (Config->DamageTypeClass)
            {
                for (const FStatisticValue& Mod : StatMods)
                {
                    if (Mod.Statistic.MatchesTag(Health) && !FMath::IsNearlyZero(Mod.Value))
                    {
                        UGameplayStatics::ApplyDamage(
                            Target,
                            FMath::Abs(Mod.Value),
                            nullptr,
                            Causer,
                            Config->DamageTypeClass
                        );
                        EffectValue += Mod.Value;
                    }
                }
            }
        }
        break;
    default:
        break;
    }

    // 5. Store for analytics, popups, and UI
    LastAppliedValue = EffectValue;

    // 6. Only record analytics if effect did real damage (not just stat mods)
    if (Config->ApplicationMode != EStatusEffectApplicationMode::StatModification && !FMath::IsNearlyZero(EffectValue))
    {
        if (UActorComponent* Comp = Target->GetComponentByClass(UNomadStatusEffectManagerComponent::StaticClass()))
        {
            if (auto* SEManager = Cast<UNomadStatusEffectManagerComponent>(Comp))
            {
                SEManager->AddStatusEffectDamage(Config->EffectTag, EffectValue);
            }
        }
    }

    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[INSTANT] Applied hybrid effect: %.2f value"), EffectValue);
}

// =====================================================
//         CHAIN EFFECTS
// =====================================================

void UNomadInstantStatusEffect::ApplyChainEffects(ACharacter* Character, UNomadInstantEffectConfig* Config)
{
    // Apply chain effects through the status effect manager
    if (!Character || !Config) return;

    UNomadStatusEffectManagerComponent* Manager = Character->FindComponentByClass<UNomadStatusEffectManagerComponent>();
    if (!Manager)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[INSTANT] No status effect manager found for chain effects"));
        return;
    }

    int32 AppliedCount = 0;

    // Apply each chain effect
    for (const TSoftClassPtr<UNomadBaseStatusEffect>& ChainEffectClass : Config->ChainEffects)
    {
        if (!ChainEffectClass.IsNull())
        {
            // Load the chain effect class
            TSubclassOf<UACFBaseStatusEffect> LoadedClass = ChainEffectClass.LoadSynchronous();
            if (LoadedClass)
            {
                // Apply through the manager for proper handling
                Manager->Nomad_AddStatusEffect(LoadedClass, Character);
                AppliedCount++;

                UE_LOG_AFFLICTION(VeryVerbose, TEXT("[INSTANT] Applied chain effect: %s"),
                                  *LoadedClass->GetName());
            }
            else
            {
                UE_LOG_AFFLICTION(Warning, TEXT("[INSTANT] Failed to load chain effect class"));
            }
        }
    }

    if (AppliedCount > 0)
    {
        // Trigger Blueprint event for VFX/SFX
        OnChainEffectsTriggered(Config->ChainEffects);

        UE_LOG_AFFLICTION(Log, TEXT("[INSTANT] Successfully applied %d chain effects"), AppliedCount);
    }
}

float UNomadInstantStatusEffect::CalculateEffectMagnitude() const
{
    // Calculate total magnitude for UI display
    const UNomadInstantEffectConfig* Config = GetEffectConfig();
    if (!Config)
    {
        return FMath::Abs(LastAppliedValue);
    }

    float TotalMagnitude = 0.0f;

    // Sum up all stat modification values
    for (const FStatisticValue& StatMod : Config->OnApplyStatModifications)
    {
        TotalMagnitude += FMath::Abs(StatMod.Value);
    }

    // Add damage stat mods if using damage events
    if (Config->ApplicationMode == EStatusEffectApplicationMode::DamageEvent ||
        Config->ApplicationMode == EStatusEffectApplicationMode::Both)
    {
        for (const FStatisticValue& DamageMod : Config->DamageStatisticMods)
        {
            TotalMagnitude += FMath::Abs(DamageMod.Value);
        }
    }

    return TotalMagnitude;
}

float UNomadInstantStatusEffect::GetEffectMagnitude() const
{
    return CalculateEffectMagnitude();
}

// =====================================================
//         DEFAULT BP EVENT IMPLEMENTATIONS (EMPTY)
// =====================================================

void UNomadInstantStatusEffect::OnInstantEffectApplied_Implementation(const TArray<FStatisticValue>& Modifications)
{
    // Default: Do nothing. Override in child C++ classes or Blueprint if desired.
}

void UNomadInstantStatusEffect::OnInstantEffectEnded_Implementation()
{
    // Default: Do nothing. Override in child C++ classes or Blueprint if desired.
}