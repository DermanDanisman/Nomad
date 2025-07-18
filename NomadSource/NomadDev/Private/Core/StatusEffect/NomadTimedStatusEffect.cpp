// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/StatusEffect/NomadTimedStatusEffect.h"
#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "Core/Data/StatusEffect/NomadTimedEffectConfig.h"
#include "ARSStatisticsComponent.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Core/StatusEffect/Utility/NomadStatusEffectUtils.h"
#include "Core/Debug/NomadLogCategories.h"
#include "Kismet/GameplayStatics.h"

// =====================================================
//         CONSTRUCTOR & INITIALIZATION
// =====================================================

UNomadTimedStatusEffect::UNomadTimedStatusEffect()
    : Super()
{
    // Initialize runtime state
    StartTime = 0.0f;
    CurrentTickCount = 0;
    AppliedModifierGuid = FGuid();
    LastTickDamage = 0.0f;
    StackCount = 1;
    OwningManager = nullptr;

    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[TIMED] Timed status effect constructed"));
}

// =====================================================
//         CONFIGURATION ACCESS
// =====================================================

UNomadTimedEffectConfig* UNomadTimedStatusEffect::GetEffectConfig() const
{
    // Loads the configuration asset for this effect (synchronously).
    return Cast<UNomadTimedEffectConfig>(EffectConfig.IsNull() ? nullptr : EffectConfig.LoadSynchronous());
}

void UNomadTimedStatusEffect::SetDuration(float InDuration)
{
    if (InDuration > 0.0f)
    {
        Duration = InDuration;
        SetupTimers(); // Recalculate timers based on the new duration
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid duration specified. Duration must be greater than 0."));
    }
}

// =====================================================
//         MANAGER INTEGRATION
// =====================================================

void UNomadTimedStatusEffect::NomadStartEffectWithManager(ACharacter* Character, UNomadStatusEffectManagerComponent* Manager)
{
    // Called by the manager to start the effect and bind back to the manager for stack/tick queries.
    CharacterOwner = Character;
    OwningManager = Manager;

    UE_LOG_AFFLICTION(Log, TEXT("[TIMED] Starting effect with manager binding"));
    OnStatusEffectStarts_Implementation(Character);
}

void UNomadTimedStatusEffect::NomadEndEffectWithManager()
{
    // Called by the manager to end the effect and unbind manager.
    UE_LOG_AFFLICTION(Log, TEXT("[TIMED] Ending effect with manager"));
    OnStatusEffectEnds_Implementation();
    OwningManager = nullptr;
}

// =====================================================
//         STACKING/REFRESH LOGIC
// =====================================================

void UNomadTimedStatusEffect::OnStacked_Implementation(const int32 NewStackCount)
{
    // Called when the effect is stacked (e.g., poison applied multiple times).
    StackCount = NewStackCount;
    RestartTimerIfStacking();

    UE_LOG_AFFLICTION(Log, TEXT("[TIMED] Effect stacked to %d, timers restarted"), NewStackCount);

    // Reapply persistent modifiers with new stack count
    const UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (Config && Config->ApplicationMode != EStatusEffectApplicationMode::DamageEvent)
    {
        RemoveAttributeSetModifier();
        ApplyAttributeSetModifier();
    }
}

void UNomadTimedStatusEffect::OnRefreshed_Implementation()
{
    // Called when the effect is refreshed (reapplied at max stacks).
    RestartTimerIfStacking();
    UE_LOG_AFFLICTION(Log, TEXT("[TIMED] Effect refreshed, duration reset"));

    // Optionally reapply modifiers
    const UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (Config && Config->ApplicationMode != EStatusEffectApplicationMode::DamageEvent)
    {
        RemoveAttributeSetModifier();
        ApplyAttributeSetModifier();
    }
}

void UNomadTimedStatusEffect::OnUnstacked(int32 NewStackCount)
{
    // Called by manager when a stack is removed.
    StackCount = NewStackCount;

    UE_LOG_AFFLICTION(Log, TEXT("[TIMED] Effect unstacked to %d"), NewStackCount);

    if (StackCount > 0)
    {
        // Update persistent modifiers for new stack count
        const UNomadTimedEffectConfig* Config = GetEffectConfig();
        if (Config && Config->ApplicationMode != EStatusEffectApplicationMode::DamageEvent)
        {
            RemoveAttributeSetModifier();
            ApplyAttributeSetModifier();
        }

        // Restart timers if stacking refreshes duration
        if (Config && Config->bStackingRefreshesDuration)
        {
            RestartTimerIfStacking();
        }
    }
    // If StackCount is 0, the manager will destroy this instance
}

void UNomadTimedStatusEffect::RestartTimerIfStacking()
{
    // When a stack is added or effect is refreshed, restart all timers to extend duration.
    ClearTimers();
    SetupTimers();
}

// =====================================================
//         EFFECT LIFECYCLE: START / END
// =====================================================

void UNomadTimedStatusEffect::OnStatusEffectStarts_Implementation(ACharacter* Character)
{
    // Called when the effect starts on a character (ACF base).
    // Handles config-driven initialization, stat/damage application, and timer setup.

    Super::OnStatusEffectStarts_Implementation(Character);
    SetEffectLifecycleState(EEffectLifecycleState::Active);

    UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (!Config || !CharacterOwner)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[TIMED] Cannot start - missing config or character"));
        return;
    }

    if (!Config->IsConfigValid())
    {
        UE_LOG_AFFLICTION(Error, TEXT("[TIMED] Config validation failed"));
        return;
    }

    // Initialize timing
    StartTime = CharacterOwner->GetWorld()->GetTimeSeconds();
    CurrentTickCount = 0;

    // Apply start stat modifications, scaled by current stack count
    int32 CurrentStacks = GetCurrentStackCount();
    StackCount = CurrentStacks;

    if (Config->OnStartStatModifications.Num() > 0)
    {
        TArray<FStatisticValue> ScaledMods = Config->OnStartStatModifications;
        for (FStatisticValue& Mod : ScaledMods)
        {
            Mod.Value *= CurrentStacks;
        }
        ApplyHybridEffect(ScaledMods, CharacterOwner, Config);
        OnTimedEffectStatModificationsApplied(ScaledMods);
    }

    // Apply persistent attribute modifiers
    if (Config->ApplicationMode != EStatusEffectApplicationMode::DamageEvent)
    {
        ApplyAttributeSetModifier();
    }

    // Sync movement speed modifiers to ensure proper application of movement changes
    SyncMovementSpeedModifier(Character, 1.0f);

    // Trigger Blueprint event
    OnTimedEffectStarted(Character);

    // Setup timers for duration and periodic ticking
    SetupTimers();

    UE_LOG_AFFLICTION(Log, TEXT("[TIMED] Effect started successfully with %d stacks"), CurrentStacks);
}

void UNomadTimedStatusEffect::OnStatusEffectEnds_Implementation()
{
    // Called when the effect is removed from the character (ACF base).
    // Handles stat/damage cleanup, timer removal, and analytics.

    UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (Config)
    {
        // Only apply end logic if the effect is truly ending
        if (GetEffectLifecycleState() != EEffectLifecycleState::Active &&
            GetEffectLifecycleState() != EEffectLifecycleState::Ending)
        {
            UE_LOG_AFFLICTION(Warning, TEXT("[TIMED] End called on non-active effect"));
            return;
        }

        // Apply end stat modifications
        if (Config->OnEndStatModifications.Num() > 0)
        {
            ApplyHybridEffect(Config->OnEndStatModifications, CharacterOwner, Config);
            OnTimedEffectStatModificationsApplied(Config->OnEndStatModifications);
        }

        // Remove persistent attribute modifiers
        if (Config->ApplicationMode != EStatusEffectApplicationMode::DamageEvent)
        {
            RemoveAttributeSetModifier();
        }

        // Remove movement speed modifiers and sync to ensure proper cleanup
        RemoveMovementSpeedModifier(CharacterOwner);

        // Trigger chain effects if configured
        if (Config->bTriggerDeactivationChainEffects && Config->DeactivationChainEffects.Num() > 0)
        {
            TriggerChainEffects(Config->DeactivationChainEffects);
            OnTimedEffectChainEffectsTriggered(Config->DeactivationChainEffects);
        }
    }

    // Clear timers
    ClearTimers();

    // Trigger Blueprint event
    OnTimedEffectEnded();

    // Call parent implementation
    Super::OnStatusEffectEnds_Implementation();

    UE_LOG_AFFLICTION(Log, TEXT("[TIMED] Effect ended successfully"));
}

// =====================================================
//         TIMER MANAGEMENT
// =====================================================

void UNomadTimedStatusEffect::SetupTimers()
{
    // Sets up timers for effect expiration (duration/ticks) and periodic ticking (if periodic).
    const UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (!Config || !CharacterOwner || !CharacterOwner->GetWorld())
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[TIMED] Cannot setup timers - missing config/character/world"));
        return;
    }

    UWorld* World = CharacterOwner->GetWorld();
    FTimerManager& TimerManager = World->GetTimerManager();

    // Calculate duration
    if (Config->bIsPeriodic)
    {
        Duration = CalculateDuration(Config);
    }
    else
    {
        Duration = Config->EffectDuration > 0.0f ? Config->EffectDuration : 0.01f; // Default to a short delay
    }

    // Set end timer
    if (Duration > 0.0f)
    {
        TimerManager.SetTimer(TimerHandle_End, this, &UNomadTimedStatusEffect::HandleEnd, Duration, false);
        UE_LOG_AFFLICTION(VeryVerbose, TEXT("[TIMED] End timer set for %.2f seconds"), Duration);
    }

    // Set periodic tick timer if enabled
    if (Config->bIsPeriodic && Config->TickInterval > KINDA_SMALL_NUMBER)
    {
        TimerManager.SetTimer(TimerHandle_Tick, this, &UNomadTimedStatusEffect::HandleTick, Config->TickInterval, true);
        UE_LOG_AFFLICTION(VeryVerbose, TEXT("[TIMED] Tick timer set for %.2f second intervals"), Config->TickInterval);
    }
}

float UNomadTimedStatusEffect::CalculateDuration(const UNomadTimedEffectConfig* Config) const
{
    if (Config->DurationMode == EEffectDurationMode::Duration)
    {
        return Config->EffectDuration;
    }
    else if (Config->DurationMode == EEffectDurationMode::Ticks)
    {
        return Config->TickInterval * Config->NumTicks;
    }
    return 0.0f; // Default fallback
}

void UNomadTimedStatusEffect::ClearTimers()
{
    // Clears any running timers (for stacking, end, or removal).
    if (!CharacterOwner || !CharacterOwner->GetWorld()) return;

    UWorld* World = CharacterOwner->GetWorld();
    FTimerManager& TimerManager = World->GetTimerManager();

    TimerManager.ClearTimer(TimerHandle_End);
    TimerManager.ClearTimer(TimerHandle_Tick);

    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[TIMED] Timers cleared"));
}

void UNomadTimedStatusEffect::HandleTick()
{
    // Called by timer each tick interval (for periodic effects).
    UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (!Config || !CharacterOwner)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[TIMED] HandleTick called with invalid state"));
        return;
    }

    ++CurrentTickCount;
    int32 CurrentStacks = GetCurrentStackCount();
    StackCount = CurrentStacks;

    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[TIMED] Tick %d with %d stacks"), CurrentTickCount, CurrentStacks);

    // Apply tick stat modifications, scaled by stack count
    if (Config->OnTickStatModifications.Num() > 0)
    {
        TArray<FStatisticValue> ScaledMods = Config->OnTickStatModifications;
        for (FStatisticValue& Mod : ScaledMods)
        {
            Mod.Value *= StackCount;
        }
        ApplyHybridEffect(ScaledMods, CharacterOwner, Config);
        OnTimedEffectStatModificationsApplied(ScaledMods);
    }

    // Trigger Blueprint tick event
    OnTimedEffectTicked(CurrentTickCount);

    // Check if we've reached the tick limit for tick-based duration
    if (Config->DurationMode == EEffectDurationMode::Ticks && CurrentTickCount >= Config->NumTicks)
    {
        UE_LOG_AFFLICTION(Log, TEXT("[TIMED] Reached tick limit (%d/%d), ending effect"),
                          CurrentTickCount, Config->NumTicks);
        HandleEnd();
    }
}

void UNomadTimedStatusEffect::HandleEnd()
{
    // Called when duration/tick cycle completes.
    UE_LOG_AFFLICTION(Log, TEXT("[TIMED] Effect duration expired"));

    ClearTimers();

    const UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (OwningManager && Config)
    {
        const int32 Index = OwningManager->FindActiveEffectIndexByTag(Config->EffectTag);
        if (Index != INDEX_NONE)
        {
            const int32 CurrentStacks = OwningManager->GetActiveEffects()[Index].StackCount;
            if (CurrentStacks > 1)
            {
                // Remove one stack (manager will call OnUnstacked, which will handle restart)
                UE_LOG_AFFLICTION(Log, TEXT("[TIMED] Removing 1 stack (%d remaining)"), CurrentStacks - 1);
                OwningManager->Nomad_RemoveStatusEffectStack(Config->EffectTag);
                // Do NOT destroy this instance; OnUnstacked will reset and continue.
                return;
            }
            else
            {
                // Last stack: remove completely
                UE_LOG_AFFLICTION(Log, TEXT("[TIMED] Last stack expired, removing effect"));
                OwningManager->Nomad_RemoveStatusEffect(Config->EffectTag);
                // Manager will destroy this instance.
                return;
            }
        }
    }

    // If not managed, just end the effect
    Nomad_OnStatusEffectEnds();
}

// =====================================================
//         STAT/ATTRIBUTE MODIFIERS
// =====================================================

void UNomadTimedStatusEffect::ApplyAttributeSetModifier()
{
    // Apply persistent attribute set modifier from config, if any.
    const UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (!Config || !CharacterOwner) return;

    // Check if there are any modifiers to apply
    if (Config->AttributeModifier.PrimaryAttributesMod.Num() == 0 &&
        Config->AttributeModifier.AttributesMod.Num() == 0 &&
        Config->AttributeModifier.StatisticsMod.Num() == 0)
    {
        return;
    }

    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[TIMED] No statistics component found for attribute modifier"));
        return;
    }

    // Store the modifier's GUID for later removal
    AppliedModifierGuid = Config->AttributeModifier.Guid;

    // Create a scaled version of the modifier based on stack count
    FAttributesSetModifier ScaledModifier = Config->AttributeModifier;

    // Scale all modifiers by stack count
    for (FAttributeModifier& AttrMod : ScaledModifier.PrimaryAttributesMod)
    {
        AttrMod.Value *= StackCount;
    }
    for (FAttributeModifier& AttrMod : ScaledModifier.AttributesMod)
    {
        AttrMod.Value *= StackCount;
    }
    // Scale statistics (both MaxValue and RegenValue)
    for (FStatisticsModifier& StatMod : ScaledModifier.StatisticsMod)
    {
        StatMod.MaxValue *= StackCount;
        StatMod.RegenValue *= StackCount;
    }

    // Apply the scaled modifier
    StatsComp->AddAttributeSetModifier(ScaledModifier);

    // Trigger Blueprint event
    OnTimedEffectAttributeModifierApplied(ScaledModifier);

    UE_LOG_AFFLICTION(Verbose, TEXT("[TIMED] Applied attribute set modifier with %d stacks"), StackCount);
}

void UNomadTimedStatusEffect::RemoveAttributeSetModifier()
{
    // Remove previously-applied attribute set modifier from the character.
    if (!CharacterOwner || !AppliedModifierGuid.IsValid()) return;

    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp) return;

    const UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (Config)
    {
        StatsComp->RemoveAttributeSetModifier(Config->AttributeModifier);
    }

    AppliedModifierGuid = FGuid();
    UE_LOG_AFFLICTION(Verbose, TEXT("[TIMED] Removed attribute set modifier"));
}

// =====================================================
//         HYBRID SYSTEM: STAT/DAMAGE/BOTH APPLICATION
// =====================================================

void UNomadTimedStatusEffect::ApplyHybridEffect(const TArray<FStatisticValue>& InStatMods, AActor* InTarget, UObject* InEffectConfig)
{
    // Core gameplay logic for applying stat modifications or damage events.
    if (!IsValid(InTarget) || InTarget->IsPendingKillPending() || !InEffectConfig)
        return;

    UNomadTimedEffectConfig* Config = Cast<UNomadTimedEffectConfig>(InEffectConfig);
    if (!Config) return;

    float EffectDamage = 0.0f;
    AActor* Causer = GetSafeDamageCauser(InTarget);

    switch (Config->ApplicationMode)
    {
    case EStatusEffectApplicationMode::StatModification:
        {
            // Apply stat modifications directly
            UARSStatisticsComponent* StatsComp = InTarget->FindComponentByClass<UARSStatisticsComponent>();
            if (StatsComp)
            {
                UNomadStatusEffectUtils::ApplyStatModifications(StatsComp, InStatMods);

                // Track health changes for analytics
                for (const FStatisticValue& Mod : InStatMods)
                {
                    if (Mod.Statistic.MatchesTag(Health))
                        EffectDamage += Mod.Value;
                }
            }
        }
        break;

    case EStatusEffectApplicationMode::DamageEvent:
        {
            // Apply damage through UE damage pipeline
            if (Config->DamageTypeClass && Config->DamageStatisticMods.Num() > 0)
            {
                for (const FStatisticValue& Mod : Config->DamageStatisticMods)
                {
                    if (Mod.Statistic.MatchesTag(Health) && !FMath::IsNearlyZero(Mod.Value))
                    {
                        UGameplayStatics::ApplyDamage(
                            InTarget,
                            FMath::Abs(Mod.Value),
                            nullptr,
                            Causer,
                            Config->DamageTypeClass
                        );
                        EffectDamage += Mod.Value;
                    }
                }
            }
        }
        break;

    case EStatusEffectApplicationMode::Both:
        {
            // Apply both stat modifications and damage events
            UARSStatisticsComponent* StatsComp = InTarget->FindComponentByClass<UARSStatisticsComponent>();
            if (StatsComp)
            {
                UNomadStatusEffectUtils::ApplyStatModifications(StatsComp, InStatMods);
            }

            if (Config->DamageTypeClass)
            {
                for (const FStatisticValue& Mod : InStatMods)
                {
                    if (Mod.Statistic.MatchesTag(Health) && !FMath::IsNearlyZero(Mod.Value))
                    {
                        UGameplayStatics::ApplyDamage(
                            InTarget,
                            FMath::Abs(Mod.Value),
                            nullptr,
                            Causer,
                            Config->DamageTypeClass
                        );
                        EffectDamage += Mod.Value;
                    }
                }
            }
        }
        break;
    default:
        break;
    }

    // Store analytics data
    LastTickDamage = EffectDamage;

    // Record damage in manager for analytics
    if (Config->ApplicationMode != EStatusEffectApplicationMode::StatModification && !FMath::IsNearlyZero(EffectDamage))
    {
        if (OwningManager)
        {
            OwningManager->AddStatusEffectDamage(Config->EffectTag, EffectDamage);
        }
    }
}

// =====================================================
//         UTILITY FUNCTIONS
// =====================================================

void UNomadTimedStatusEffect::TriggerChainEffects(const TArray<TSoftClassPtr<UNomadBaseStatusEffect>>& ChainEffects)
{
    // Trigger chain effects through the manager
    if (!OwningManager || !CharacterOwner) return;

    for (const TSoftClassPtr<UNomadBaseStatusEffect>& ChainEffectClass : ChainEffects)
    {
        if (!ChainEffectClass.IsNull())
        {
            // Load and apply the chain effect
            TSubclassOf<UACFBaseStatusEffect> LoadedClass = ChainEffectClass.LoadSynchronous();
            if (LoadedClass)
            {
                OwningManager->Nomad_AddStatusEffect(LoadedClass, CharacterOwner);
            }
        }
    }

    UE_LOG_AFFLICTION(Log, TEXT("[TIMED] Triggered %d chain effects"), ChainEffects.Num());
}

int32 UNomadTimedStatusEffect::GetCurrentStackCount() const
{
    // Returns the current stack count for this effect from the manager, or fallback value.
    if (!OwningManager) return StackCount;

    const UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (!Config) return StackCount;

    const int32 Index = OwningManager->FindActiveEffectIndexByTag(Config->EffectTag);
    if (Index != INDEX_NONE)
    {
        return OwningManager->GetActiveEffects()[Index].StackCount;
    }

    return StackCount;
}

// =====================================================
//         ADDITIONAL QUERY FUNCTIONS
// =====================================================

float UNomadTimedStatusEffect::GetUptime() const
{
    if (!CharacterOwner || !CharacterOwner->GetWorld() || StartTime <= 0.0f)
        return 0.0f;
    return CharacterOwner->GetWorld()->GetTimeSeconds() - StartTime;
}

float UNomadTimedStatusEffect::GetRemainingDuration() const
{
    const UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (!Config || !CharacterOwner || !CharacterOwner->GetWorld())
        return 0.0f;

    float TotalDuration = 0.0f;
    if (Config->bIsPeriodic)
    {
        if (Config->DurationMode == EEffectDurationMode::Duration)
        {
            TotalDuration = Config->EffectDuration;
        }
        else
        {
            TotalDuration = Config->TickInterval * Config->NumTicks;
        }
    }

    const float Elapsed = GetUptime();
    return FMath::Max(0.0f, TotalDuration - Elapsed);
}

float UNomadTimedStatusEffect::GetProgressPercentage() const
{
    const UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (!Config || !CharacterOwner || !CharacterOwner->GetWorld())
        return 0.0f;

    float TotalDuration = 0.0f;
    if (Config->bIsPeriodic)
    {
        if (Config->DurationMode == EEffectDurationMode::Duration)
        {
            TotalDuration = Config->EffectDuration;
        }
        else
        {
            TotalDuration = Config->TickInterval * Config->NumTicks;
        }
    }

    if (TotalDuration <= 0.0f)
        return 1.0f;

    const float Elapsed = GetUptime();
    return FMath::Clamp(Elapsed / TotalDuration, 0.0f, 1.0f);
}