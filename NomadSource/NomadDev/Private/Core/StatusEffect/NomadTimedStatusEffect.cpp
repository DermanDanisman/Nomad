// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/StatusEffect/NomadTimedStatusEffect.h"
#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "ARSStatisticsComponent.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Core/StatusEffect/Utility/NomadStatusEffectUtils.h"
#include "Kismet/GameplayStatics.h"

// =====================================================
//         CONSTRUCTOR & INITIALIZATION
// =====================================================

UNomadTimedStatusEffect::UNomadTimedStatusEffect()
    : Super()
{
    StartTime = 0.0f;
    CurrentTickCount = 0;
    AppliedModifierGuid = FGuid();
    LastTickDamage = 0.0f;
}

UNomadTimedEffectConfig* UNomadTimedStatusEffect::GetEffectConfig() const
{
    // Loads the configuration asset for this effect (synchronously).
    return Cast<UNomadTimedEffectConfig>(EffectConfig.IsNull() ? nullptr : EffectConfig.LoadSynchronous());
}

void UNomadTimedStatusEffect::NomadStartEffectWithManager(ACharacter* Character, UNomadStatusEffectManagerComponent* Manager)
{
    // Called by the manager to start the effect and bind back to the manager for stack/tick queries.
    CharacterOwner = Character;
    OwningManager = Manager;
    OnStatusEffectStarts_Implementation(Character);
}

void UNomadTimedStatusEffect::NomadEndEffectWithManager()
{
    // Called by the manager to end the effect and unbind manager.
    OnStatusEffectEnds_Implementation();
}

void UNomadTimedStatusEffect::RestartTimerIfStacking()
{
    // When a stack is added or effect is refreshed, restart all timers to extend duration.
    ClearTimers();
    SetupTimers();
}

// =====================================================
//         STACKING/REFRESH (HOOKS)
// =====================================================

void UNomadTimedStatusEffect::OnStacked_Implementation(const int32 NewStackCount)
{
    // Called when the effect is stacked (e.g., poison applied multiple times).
    StackCount = NewStackCount;
    RestartTimerIfStacking();
    UE_LOG(LogTemp, Log, TEXT("[TIMED EFFECT] Stack increased to %d"), NewStackCount);
}

void UNomadTimedStatusEffect::OnRefreshed_Implementation()
{
    // Called when the effect is refreshed (reapplied at max stacks).
    RestartTimerIfStacking();
    UE_LOG(LogTemp, Log, TEXT("[TIMED EFFECT] Effect refreshed, duration/timer reset"));
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
    if (!Config || !CharacterOwner) return;

    StartTime = CharacterOwner->GetWorld()->GetTimeSeconds();
    CurrentTickCount = 0;

    // Apply stat/damage/both modifications defined for effect start, scaled by current stack count.
    int32 CurrentStacks = GetCurrentStackCount();
    StackCount = CurrentStacks;
    TArray<FStatisticValue> ScaledMods = Config->OnStartStatModifications;
    for (FStatisticValue& Mod : ScaledMods)
    {
        Mod.Value *= CurrentStacks;
    }
    ApplyHybridEffect(ScaledMods, CharacterOwner, Config);

    // Cosmetic Blueprint hook: effect started.
    OnTimedEffectStarted(Character);

    // Apply attribute set modifier if not DamageEvent-only.
    if (Config->ApplicationMode != EStatusEffectApplicationMode::DamageEvent)
        ApplyAttributeSetModifier();

    if (Config->ApplicationMode != EStatusEffectApplicationMode::DamageEvent &&
        (Config->AttributeModifier.PrimaryAttributesMod.Num() > 0 ||
         Config->AttributeModifier.AttributesMod.Num() > 0 ||
         Config->AttributeModifier.StatisticsMod.Num() > 0))
    {
        OnTimedEffectAttributeModifierApplied(Config->AttributeModifier);
    }

    SetupTimers();
}

void UNomadTimedStatusEffect::OnStatusEffectEnds_Implementation()
{
    // Called when the effect is removed from the character (ACF base).
    // Handles stat/damage cleanup, timer removal, and analytics.
    UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (Config)
    {
        // Only apply end logic if the effect is truly ending.
        if (GetEffectLifecycleState() != EEffectLifecycleState::Active && GetEffectLifecycleState() != EEffectLifecycleState::Ending)
            return;
        
        ApplyHybridEffect(Config->OnEndStatModifications, CharacterOwner, Config);

        // Only remove persistent attribute set modifier if it was applied.
        if (Config->ApplicationMode != EStatusEffectApplicationMode::DamageEvent)
            RemoveAttributeSetModifier();
    }

    OnTimedEffectEnded();

    SetEffectLifecycleState(EEffectLifecycleState::Removed);
    Super::OnStatusEffectEnds_Implementation();
}

// =====================================================
//         TIMER MANAGEMENT
// =====================================================

void UNomadTimedStatusEffect::SetupTimers()
{
    // Sets up timers for effect expiration (duration/ticks) and periodic ticking (if periodic).
    const UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (!Config || !CharacterOwner || !CharacterOwner->GetWorld()) return;

    const UWorld* World = CharacterOwner->GetWorld();
    FTimerManager& TimerManager = World->GetTimerManager();

    float EndTime = 0.f;

    // Decide end time depending on duration mode (duration or tick count).
    if (Config->bIsPeriodic)
    {
        if (Config->DurationMode == EEffectDurationMode::Duration)
        {
            EndTime = Config->EffectDuration;
        }
        else
        {
            EndTime = Config->TickInterval * Config->NumTicks;
        }
    }
    else
    {
        EndTime = Config->EffectDuration;
    }

    // Set end timer (if non-instant).
    if (EndTime > 0.0f)
        TimerManager.SetTimer(TimerHandle_End, this, &UNomadTimedStatusEffect::HandleEnd, EndTime, false);

    // Set periodic tick timer if periodic.
    if (Config->bIsPeriodic)
    {
        TimerManager.SetTimer(TimerHandle_Tick, this, &UNomadTimedStatusEffect::HandleTick, Config->TickInterval, true);
    }
}

void UNomadTimedStatusEffect::ClearTimers()
{
    // Clears any running timers (for stacking, end, or removal).
    if (!CharacterOwner || !CharacterOwner->GetWorld()) return;
    const UWorld* World = CharacterOwner->GetWorld();
    FTimerManager& TimerManager = World->GetTimerManager();
    TimerManager.ClearTimer(TimerHandle_End);
    TimerManager.ClearTimer(TimerHandle_Tick);
}

void UNomadTimedStatusEffect::HandleTick()
{
    // Called by timer each tick interval (for periodic effects).
    UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (!Config) return;

    // Scale all stat/damage mods by current stack count.
    ++CurrentTickCount;
    int32 CurrentStacks = GetCurrentStackCount();
    StackCount = CurrentStacks;
    
    TArray<FStatisticValue> ScaledMods = Config->OnTickStatModifications;
    for (FStatisticValue& Mod : ScaledMods)
    {
        Mod.Value *= StackCount;
    }
    ApplyHybridEffect(ScaledMods, CharacterOwner, Config);

    OnTimedEffectTicked(CurrentTickCount);

    // If tick-based duration, check if finished.
    if (Config->bIsPeriodic && Config->DurationMode == EEffectDurationMode::Ticks && CurrentTickCount >= Config->NumTicks)
    {
        HandleEnd();
    }
}

void UNomadTimedStatusEffect::HandleEnd()
{
    // Called when duration/tick cycle completes.
    ClearTimers();

    const UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (OwningManager && Config)
    {
        int32 Index = OwningManager->FindActiveEffectIndexByTag(Config->EffectTag);
        if (Index != INDEX_NONE)
        {
            const int32 CurrentStacks = OwningManager->GetActiveEffects()[Index].StackCount;
            if (CurrentStacks > 1)
            {
                // Remove one stack (manager will call OnUnstacked, which will handle restart)
                OwningManager->RemoveStatusEffect(Config->EffectTag);
                // Do NOT destroy this instance; OnUnstacked will reset and continue.
                return;
            }
            else
            {
                // Last stack: remove and cleanup
                OwningManager->RemoveStatusEffect(Config->EffectTag);
                // Manager will destroy this instance.
                return;
            }
        }
    }

    // If not managed, just end the effect
    OnStatusEffectEnds_Implementation();
}

// =====================================================
//         STAT/ATTRIBUTE MODIFIERS
// =====================================================

void UNomadTimedStatusEffect::ApplyAttributeSetModifier()
{
    // Purpose: 
    //   Apply a persistent attribute set modifier (such as a stat bonus, penalty, or transformation)
    //   from the effect's config to the owning character's stats component.
    //   This is usually used for buffs/debuffs that last the duration of the effect.

    // Retrieve the effect configuration asset.
    const UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (!Config || !CharacterOwner) return; // Sanity check: must have config and owning character.

    // Only proceed if there's at least one attribute or stat modification set in the config.
    if (Config->AttributeModifier.PrimaryAttributesMod.Num() == 0 &&
        Config->AttributeModifier.AttributesMod.Num() == 0 &&
        Config->AttributeModifier.StatisticsMod.Num() == 0)
        return;

    // Get the ARS statistics component from the character.
    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp) return; // Can't modify stats if the component is missing.

    // Store the modifier's GUID for later removal (prevents duplicates and ensures correct cleanup).
    AppliedModifierGuid = Config->AttributeModifier.Guid;

    // Actually apply the attribute set modifier as defined in the config asset.
    StatsComp->AddAttributeSetModifier(Config->AttributeModifier);

    // Trigger a Blueprint (and C++) event for any cosmetic/UI logic.
    OnTimedEffectAttributeModifierApplied(Config->AttributeModifier);
}

void UNomadTimedStatusEffect::RemoveAttributeSetModifier()
{
    // Purpose:
    //   Remove the previously-applied attribute set modifier from the owning character's stats component.
    //   Called when the effect ends or is removed.
    //   Ensures no lingering stat changes remain after effect expiration.

    // Only proceed if we have a valid character and a valid modifier GUID (was actually applied).
    if (!CharacterOwner || !AppliedModifierGuid.IsValid()) return;

    // Get the ARS statistics component from the character.
    UARSStatisticsComponent* StatsComp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp) return;

    // Retrieve the config to access the correct attribute modifier to remove.
    const UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (Config)
        StatsComp->RemoveAttributeSetModifier(Config->AttributeModifier);

    // Clear the GUID so we don't accidentally remove it again.
    AppliedModifierGuid = FGuid();
}

// =====================================================
//         HYBRID SYSTEM: STAT/DAMAGE/BOTH APPLICATION
// =====================================================

void UNomadTimedStatusEffect::ApplyHybridEffect(const TArray<FStatisticValue>& InStatMods, AActor* InTarget, UObject* InEffectConfig)
{
    // Purpose:
    //   This function is the core for applying the effect's gameplay logic: 
    //   - Stat modifications (buffs/debuffs, healing, poison, etc.)
    //   - Damage events (DoTs, direct damage over time, etc.)
    //   - Or both, depending on the config's ApplicationMode
    //   Called at effect start, each tick, and on effect end.

    // ---------------------------------------------------------------------
    // Step 1: Safety checks
    // ---------------------------------------------------------------------
    // Don't apply if the target is invalid, being destroyed, or config is null.
    if (!IsValid(InTarget) || InTarget->IsPendingKillPending() || !InEffectConfig)
        return;

    UNomadTimedEffectConfig* Config = Cast<UNomadTimedEffectConfig>(InEffectConfig);
    if (!Config) return;

    // Track the total amount of health modified by this tick (for analytics/UI).
    float EffectDamage = 0.0f;

    // Figure out who is responsible for the damage/stat change (for analytics or kill credit).
    AActor* Causer = GetSafeDamageCauser(InTarget);

    // ---------------------------------------------------------------------
    // Step 2: Mode switch -- apply stat, damage, or both
    // ---------------------------------------------------------------------
    switch (Config->ApplicationMode)
    {
    case EStatusEffectApplicationMode::StatModification:
        {
            // --- StatModification mode: Only modify stats, no UE damage event fired ---
            UARSStatisticsComponent* StatsComp = InTarget->FindComponentByClass<UARSStatisticsComponent>();
            if (StatsComp)
            {
                // Apply all stat modifications in the StatMods array.
                UNomadStatusEffectUtils::ApplyStatModifications(StatsComp, InStatMods);

                // Sum up the health change (for analytics or healing/damage popups).
                for (const FStatisticValue& Mod : InStatMods)
                {
                    if (Mod.Statistic.MatchesTag(Health))
                        EffectDamage += Mod.Value;
                }
                // Cosmetic/UI callback.
                OnTimedEffectStatModificationsApplied(InStatMods);
            }
        }
        break;

    case EStatusEffectApplicationMode::DamageEvent:
        {
            // --- DamageEvent mode: Only fire UE damage event, no stat mods ---
            if (Config->DamageTypeClass)
            {
                // Use DamageStatisticMods from config, not StatMods from parameter.
                if (Config->DamageStatisticMods.Num() > 0)
                {
                    for (const FStatisticValue& Mod : Config->DamageStatisticMods)
                    {
                        // Only apply to health and skip zero values for performance.
                        if (Mod.Statistic.MatchesTag(Health) && !FMath::IsNearlyZero(Mod.Value))
                        {
                            // Fire a UE damage event on the target, for proper death/aggro/AI response.
                            UGameplayStatics::ApplyDamage(
                                InTarget,
                                FMath::Abs(Mod.Value), // Damage must be positive.
                                nullptr,               // No specific instigator controller.
                                Causer,                // Who caused the damage (for analytics).
                                Config->DamageTypeClass
                            );
                            EffectDamage += Mod.Value;
                        }
                    }
                }
                // Cosmetic/UI callback (StatMods passed for analytics).
                OnTimedEffectStatModificationsApplied(InStatMods);
            }
        }
        break;

    case EStatusEffectApplicationMode::Both:
        {
            // --- Both mode: Apply stat mods and fire damage events ---
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
            // Cosmetic/UI callback.
            OnTimedEffectStatModificationsApplied(InStatMods);
        }
        break;
    default:
        break;
    }

    // ---------------------------------------------------------------------
    // Step 3: Store analytics and trigger manager for UI/analytics
    // ---------------------------------------------------------------------

    // Store the last tick's damage or healing for analytics/UI.
    LastTickDamage = EffectDamage;

    // Only record analytics if the effect did real damage (not for pure stat mods).
    if (Config->ApplicationMode != EStatusEffectApplicationMode::StatModification && !FMath::IsNearlyZero(EffectDamage))
    {
        // Find the owning status effect manager and record the damage.
        if (UActorComponent* Comp = InTarget->GetComponentByClass(UNomadStatusEffectManagerComponent::StaticClass()))
        {
            if (auto* SEManager = Cast<UNomadStatusEffectManagerComponent>(Comp))
            {
                SEManager->AddStatusEffectDamage(Config->EffectTag, EffectDamage);
            }
        }
    }
}

// =====================================================
//         COSMETIC/CHAIN EFFECTS
// =====================================================

void UNomadTimedStatusEffect::TriggerChainEffects(const TArray<TSoftClassPtr<UNomadBaseStatusEffect>>& ChainEffects)
{
    // Cosmetic only; triggers Blueprint event for chain effect VFX/SFX.
    OnTimedEffectChainEffectsTriggered(ChainEffects);
}

// =====================================================
//         STACK COUNT (UTILITY)
// =====================================================

int32 UNomadTimedStatusEffect::GetCurrentStackCount() const
{
    // Returns the current stack count for this effect from the manager, or 1 if unbound.
    if (!OwningManager) return StackCount;
    const UNomadTimedEffectConfig* Config = GetEffectConfig();
    if (!Config) return StackCount;
    const int32 Index = OwningManager->FindActiveEffectIndexByTag(Config->EffectTag);
    if (Index != INDEX_NONE)
        return OwningManager->GetActiveEffects()[Index].StackCount;
    return StackCount;
}

void UNomadTimedStatusEffect::OnUnstacked(const int32 NewStackCount)
{
    StackCount = NewStackCount;
    if (StackCount > 0)
    {
        // Restart for new duration/tick cycle for remaining stacks
        RestartTimerIfStacking();
    }
    // No extra tick/damage here unless your game specifically wants it!
}