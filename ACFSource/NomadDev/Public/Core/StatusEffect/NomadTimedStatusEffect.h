// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/StatusEffect/NomadBaseStatusEffect.h"
#include "Core/Data/StatusEffect/NomadTimedEffectConfig.h"
#include "ARSTypes.h"
#include "Engine/World.h"
#include "NomadTimedStatusEffect.generated.h"

// Forward declare manager
class UNomadStatusEffectManagerComponent;

/**
 * UNomadTimedStatusEffect
 * ----------------------------------------------------------
 * Data-driven, highly extensible timed status effect.
 * Supports both duration-based (finite time) and tick-based (finite ticks) effects.
 * - Useful for poison, burning, regeneration, shield, temporary buffs, etc.
 * - Handles stat/attribute modifications at start, on tick, and at end.
 * - Periodic ticking (DoT, HoT, etc) is fully supported.
 * - Stackable if configured (manager handles stacking logic).
 * - Notifies and cleans up via manager on expiration.
 * - Robust timer and memory management to avoid leaks or double-removal.
 * - No UI logic here: notifications/affliction bar are handled by the manager.
 * - Supports damage-causer for robust analytics/gameplay.
 * 
 * Timing is determined by the associated config asset:
 *   - bIsPeriodic: true for periodic (ticking) effects, false for single-shot duration.
 *   - DurationMode: set to Duration (uses EffectDuration) or Ticks (uses NumTicks).
 *   - TickInterval: time between ticks if periodic.
 *   - EffectDuration: total time if using duration mode.
 *   - NumTicks: number of ticks if using tick mode.
 * 
 * ==========================================
 *     NomadTimedStatusEffect (Hybrid)
 * ==========================================
 *
 * Applies periodic or single-shot effects as configured.
 *
 * === Hybrid System Usage ===
 * - ApplicationMode (from config) determines whether to apply stat mods, damage, or both on start/tick/end.
 * - For DamageEvent/Both modes:
 *     - Config.DamageStatisticMods MUST have at least one FStatisticValue (Health, negative value for damage).
 *     - Config.DamageTypeClass MUST be set.
 * - For StatModification:
 *     - Only stat mod arrays are used.
 * - Always call ApplyHybridEffect and OnTimedEffectStatModificationsApplied for analytics/UI.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta=(DisplayName="Timed Status Effect"))
class NOMADDEV_API UNomadTimedStatusEffect : public UNomadBaseStatusEffect
{
    GENERATED_BODY()

public:
    UNomadTimedStatusEffect();

    // The manager that owns this effect (set on creation)
    UPROPERTY()
    UNomadStatusEffectManagerComponent* OwningManager = nullptr;

    // ======== Runtime State ========
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    float StartTime = 0.0f;

    /** Number of ticks elapsed since effect started. */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    int32 CurrentTickCount = 0;

    /** GUID of attribute set modifier applied by this effect (if any). */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    FGuid AppliedModifierGuid;
    
    /** The last amount of damage (or healing) applied on tick. */
    UPROPERTY()
    float LastTickDamage = 0.0f;

    // ======== Configuration ========
    /** Configuration data asset for this timed effect */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Configuration")
    TSoftObjectPtr<UNomadTimedEffectConfig> EffectConfig;

    // ======== Core API ========
    /** Loads configuration asset (synchronously, safe for runtime use). */
    UNomadTimedEffectConfig* GetConfig() const;

    /** Main entry point for starting a timed effect. Sets up timers, state, etc. */
    void OnStatusEffectStarts(ACharacter* Character, UNomadStatusEffectManagerComponent* Manager);

    /** Main entry point for ending a timed effect. Triggers cleanup, end logic, etc. */
    virtual void OnStatusEffectEnds();

    /** Restarts duration/tick timers (used for stacking/refreshing). */
    UFUNCTION(BlueprintCallable, Category="Status Effect|Stacking")
    virtual void RestartTimerIfStacking();

    /** Called to notify when chain effects are triggered. Cosmetic only. */
    UFUNCTION(BlueprintCallable, Category="Nomad Timed Effect")
    void TriggerChainEffects(const TArray<TSoftClassPtr<UNomadBaseStatusEffect>>& ChainEffects);

    UFUNCTION(BlueprintPure, Category="Nomad|Status Effect|Damage")
    float GetLastTickDamage() const { return LastTickDamage; }

protected:
    
    // ======== Timed Effect Events (Blueprint Hooks) ========

    /** Main entry: called immediately when timed effect starts. Put your VFX/SFX/UI logic here. */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect")
    void OnTimedEffectStarted(ACharacter* Character);

    /** Called every time the effect ticks (on periodic tick, if enabled). Good for tick VFX/SFX/UI. */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect")
    void OnTimedEffectTicked(int32 TickCount);

    /** Called right before the effect ends (on timer/tick completion). Use for end VFX/SFX/UI. */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect")
    void OnTimedEffectEnded();

    /** Called when stat modifications are applied (start/tick/end). */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect")
    void OnTimedEffectStatModificationsApplied(const TArray<FStatisticValue>& Modifications);

    /** Called when attribute set modifier is applied (usually at start). */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect")
    void OnTimedEffectAttributeModifierApplied(const FAttributesSetModifier& Modifier);

    /** Called when chain effects are triggered (if any). */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect")
    void OnTimedEffectChainEffectsTriggered(const TArray<TSoftClassPtr<UNomadBaseStatusEffect>>& ChainEffects);
    
    // ======== Timer Management ========
    FTimerHandle TimerHandle_End;
    FTimerHandle TimerHandle_Tick;

    /** Sets up timers for duration and periodic ticks based on config. */
    void SetupTimers();

    /** Clears timers on end, stacking, or removal. */
    void ClearTimers();

    /** Internal tick handler, called each interval (if periodic tick enabled). */
    void HandleTick();

    /** Internal end handler, called on duration/tick completion. */
    void HandleEnd();

    // ======== Stat/Attribute Modifiers ========
    /** Apply stat changes as defined in config. */
    void ApplyStatModifications(const TArray<FStatisticValue>& Modifications);

    /** Apply persistent attribute set modifier from config, if any. */
    void ApplyAttributeSetModifier();

    /** Remove persistent attribute set modifier on cleanup. */
    void RemoveAttributeSetModifier();

    // ======== ACF Status Effect Overrides ========
    /** Implementation of start logic (calls base, applies config, etc.). */
    virtual void OnStatusEffectStarts_Implementation(ACharacter* Character) override;

    /** Implementation of end logic (calls base, applies config cleanup, etc.). */
    virtual void OnStatusEffectEnds_Implementation() override;

    /** Applies stat mods and/or damage according to the config's ApplicationMode. */
    virtual void ApplyHybridEffect(const TArray<FStatisticValue>& StatMods, AActor* Target, UObject* EffectConfig) override;

    FORCEINLINE AActor* GetSafeDamageCauser(AActor* Target) const
    {
        return (DamageCauser.IsValid() && !DamageCauser->IsPendingKillPending()) ? DamageCauser.Get() : Target;
    }
};