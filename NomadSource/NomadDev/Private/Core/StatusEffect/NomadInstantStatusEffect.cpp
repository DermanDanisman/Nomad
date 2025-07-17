// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/StatusEffect/NomadInstantStatusEffect.h"
#include "Core/Data/StatusEffect/NomadInstantEffectConfig.h"
#include "ARSStatisticsComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "Core/StatusEffect/Utility/NomadStatusEffectUtils.h"
#include "Core/Debug/NomadLogCategories.h"
#include "GameFramework/Character.h"

// =====================================================
//         CONSTRUCTOR & INITIALIZATION
// =====================================================

UNomadInstantStatusEffect::UNomadInstantStatusEffect()
{
    // Initialize analytics state for last applied value.
    LastAppliedValue = 0.0f;
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

    // Load the configuration asset.
    UNomadInstantEffectConfig* Config = GetEffectConfig();
    if (!Config || !Character)
    {
        // Abort if config or character is missing, log error for debug.
        UE_LOG_AFFLICTION(Error, TEXT("[INSTANT] Invalid config or character on start"));
        SetEffectLifecycleState(EEffectLifecycleState::Removed);
        return;
    }

    // Prepare stat modifications array from config.
    // (Stacking is not supported for instant effects, so use as-is.)
    TArray<FStatisticValue> ScaledMods = Config->OnApplyStatModifications;

    // Hybrid system: apply stat/damage/both in a single call.
    ApplyHybridEffect(ScaledMods, Character, Config);

    // Trigger Blueprint and optional C++ event for VFX/SFX/UI notifications.
    OnInstantEffectApplied_Implementation(ScaledMods);
    OnInstantEffectApplied(ScaledMods);

    // End the effect immediately—instant effects do not persist.
    Nomad_OnStatusEffectEnds();
}

void UNomadInstantStatusEffect::OnStatusEffectEnds_Implementation()
{
    // Called after the instant effect finishes, for cleanup and event triggers.
    UE_LOG_AFFLICTION(Log, TEXT("[INSTANT] Instant effect ended"));

    // Transition to removed state to mark effect as finished.
    EffectState = EEffectLifecycleState::Removed;

    // Trigger Blueprint and optional C++ event for VFX/SFX/UI.
    OnInstantEffectEnded_Implementation();
    OnInstantEffectEnded();
}

// =====================================================
//         HYBRID SYSTEM: STAT/DAMAGE/BOTH APPLICATION
// =====================================================

void UNomadInstantStatusEffect::ApplyHybridEffect(const TArray<FStatisticValue>& StatMods, AActor* Target, UObject* EffectConfigObj)
{
    // Applies stat modifications, damage, or both, based on the effect's configuration.
    // This is the core gameplay logic for instant effects.

    // 1. Safety checks: must have valid target and config.
    if (!IsValid(Target) || Target->IsPendingKillPending() || !EffectConfigObj)
        return;

    UNomadInstantEffectConfig* Config = Cast<UNomadInstantEffectConfig>(EffectConfigObj);
    if (!Config) return;

    // 2. Prepare analytics for the total value of health changed (damage or healing).
    float EffectValue = 0.0f;

    // 3. Determine who caused the effect (for analytics, UI, kill credit, etc.).
    AActor* Causer = GetSafeDamageCauser(Target);

    // 4. Switch between the three hybrid modes: stat mod, damage, or both.
    switch (Config->ApplicationMode)
    {
    case EStatusEffectApplicationMode::StatModification:
        {
            // --- StatModification: Only stat/attribute changes, no UE damage event.
            UARSStatisticsComponent* StatsComp = Target->FindComponentByClass<UARSStatisticsComponent>();
            if (StatsComp)
            {
                UNomadStatusEffectUtils::ApplyStatModifications(StatsComp, StatMods);
                for (const FStatisticValue& Mod : StatMods)
                {
                    // Track any health modification for analytics.
                    if (Mod.Statistic.MatchesTag(Health))
                        EffectValue += Mod.Value;
                }
            }
        }
        break;
    case EStatusEffectApplicationMode::DamageEvent:
        {
            // --- DamageEvent: Only damage, no stat mods.
            if (Config->DamageTypeClass)
            {
                if (Config->DamageStatisticMods.Num() > 0)
                {
                    for (const FStatisticValue& Mod : Config->DamageStatisticMods)
                    {
                        // Only health is supported for direct damage.
                        if (Mod.Statistic.MatchesTag(Health) && !FMath::IsNearlyZero(Mod.Value))
                        {
                            // Apply UE damage event for correct gameplay logic (death, aggro, etc.).
                            UGameplayStatics::ApplyDamage(
                                Target,
                                FMath::Abs(Mod.Value), // Always positive.
                                nullptr,               // No specific instigator controller.
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
            // --- Both: Apply stat/attribute mods AND damage.
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

    // 5. Store for analytics, popups, and UI.
    LastAppliedValue = EffectValue;

    // 6. Only record analytics if effect did real damage (not just stat mods).
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
}

// =====================================================
//         DEFAULT BP EVENT IMPLEMENTATIONS (EMPTY)
// =====================================================

void UNomadInstantStatusEffect::OnInstantEffectApplied_Implementation(const TArray<FStatisticValue>& Modifications)
{
    // Default: Do nothing. Override in child C++ classes if desired.
}

void UNomadInstantStatusEffect::OnInstantEffectEnded_Implementation()
{
    // Default: Do nothing. Override in child C++ classes if desired.
}