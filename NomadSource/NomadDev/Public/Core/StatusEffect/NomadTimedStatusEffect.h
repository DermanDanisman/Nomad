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

// =====================================================
//             CLASS DECLARATION: TIMED EFFECT
// =====================================================

/**
 * UNomadTimedStatusEffect
 * ----------------------------------------------------------
 * Data-driven, highly extensible timed status effect.
 * Supports both duration-based (finite time) and tick-based (finite ticks) effects.
 * - Useful for poison, burning, regeneration, shield, temporary buffs, etc.
 * - Handles stat/attribute modifications at start, on tick, and at end.
 * - Periodic ticking (DoT, HoT, etc.) is fully supported.
 * - Stackable if configured (manager handles stacking logic).
 * - Notifies and cleans up via manager on expiration.
 * - Robust timer and memory management to avoid leaks or double-removal.
 * - No UI logic here: notifications/affliction bar are handled by the manager.
 * - Supports damage-causer for robust analytics/gameplay.
 * 
 * Timing is determined by the associated config asset.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta=(DisplayName="Timed Status Effect"))
class NOMADDEV_API UNomadTimedStatusEffect : public UNomadBaseStatusEffect
{
    GENERATED_BODY()

public:
    // =====================================================
    //         CONSTRUCTOR & INITIALIZATION
    // =====================================================

    UNomadTimedStatusEffect();

    // =====================================================
    //         MANAGER INTEGRATION
    // =====================================================

    /** The manager that owns this effect (set on creation) */
    UPROPERTY()
    UNomadStatusEffectManagerComponent* OwningManager = nullptr;

    // =====================================================
    //         RUNTIME STATE
    // =====================================================

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
    
    /** Internal, current stack count (updated by manager). */
    int32 StackCount = 1;

    // =====================================================
    //         CONFIGURATION
    // =====================================================

    /** Loads configuration asset (synchronously, safe for runtime use). */
    virtual UNomadTimedEffectConfig* GetEffectConfig() const override;
    
    // =====================================================
    //         MANAGER INTERFACE
    // =====================================================

    /** Called by manager to start this effect and bind manager for stack/tick queries. */
    void NomadStartEffectWithManager(ACharacter* Character, UNomadStatusEffectManagerComponent* Manager);

    /** Ends the effect and unbinds manager. */
    void NomadEndEffectWithManager();

    // =====================================================
    //         STACKING/REFRESH LOGIC
    // =====================================================

    /** Restarts duration/tick timers (used for stacking/refreshing). */
    UFUNCTION(BlueprintCallable, Category="Nomad Timed Status Effect | Stacking")
    virtual void RestartTimerIfStacking();

    /** Called to notify when chain effects are triggered. Cosmetic only. */
    UFUNCTION(BlueprintCallable, Category="Nomad Timed Status Effect | Chain Effects")
    void TriggerChainEffects(const TArray<TSoftClassPtr<UNomadBaseStatusEffect>>& ChainEffects);

    UFUNCTION(BlueprintPure, Category="Nomad Timed Status Effect | Damage")
    float GetLastTickDamage() const { return LastTickDamage; }

    /** Utility: Get the latest stack count for this effect from the manager (by tag). */
    UFUNCTION(BlueprintPure, Category="Nomad Timed Status Effect | Stacking")
    int32 GetCurrentStackCount() const;

    /** Called by manager when a stack is removed. */
    virtual void OnUnstacked(int32 NewStackCount);

    // --- Stacking/Refresh logic for C++ or Blueprint ---
    UFUNCTION(BlueprintNativeEvent, Category="Nomad Timed Status Effect | Stacking")
    void OnStacked(int32 NewStackCount);
    virtual void OnStacked_Implementation(int32 NewStackCount);

    UFUNCTION(BlueprintNativeEvent, Category="Nomad Timed Status Effect | Stacking")
    void OnRefreshed();
    virtual void OnRefreshed_Implementation();

protected:
    // =====================================================
    //         CORE API: START/END
    // =====================================================

    /** Implementation of start logic (calls base, applies config, etc.). */
    virtual void OnStatusEffectStarts_Implementation(ACharacter* Character) override;

    /** Implementation of end logic (calls base, applies config cleanup, etc.). */
    virtual void OnStatusEffectEnds_Implementation() override;
    
    // =====================================================
    //         TIMED EFFECT EVENTS (Blueprint Hooks)
    // =====================================================

    /** Main entry: called immediately when timed effect starts. Put your VFX/SFX/UI logic here. */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect | Events")
    void OnTimedEffectStarted(ACharacter* Character);

    /** Called every time the effect ticks (on periodic tick, if enabled). Good for tick VFX/SFX/UI. */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect | Events")
    void OnTimedEffectTicked(int32 TickCount);

    /** Called right before the effect ends (on timer/tick completion). Use for end VFX/SFX/UI. */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect | Events")
    void OnTimedEffectEnded();

    /** Called when stat modifications are applied (start/tick/end). */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect | Events")
    void OnTimedEffectStatModificationsApplied(const TArray<FStatisticValue>& Modifications);

    /** Called when attribute set modifier is applied (usually at start). */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect | Events")
    void OnTimedEffectAttributeModifierApplied(const FAttributesSetModifier& Modifier);

    /** Called when chain effects are triggered (if any). */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect | Events")
    void OnTimedEffectChainEffectsTriggered(const TArray<TSoftClassPtr<UNomadBaseStatusEffect>>& ChainEffects);
    
    // =====================================================
    //         TIMER MANAGEMENT
    // =====================================================

    /** Timer handle for effect end. */
    FTimerHandle TimerHandle_End;

    /** Timer handle for periodic tick. */
    FTimerHandle TimerHandle_Tick;

    /** Sets up timers for duration and periodic ticks based on config. */
    void SetupTimers();

    /** Clears timers on end, stacking, or removal. */
    void ClearTimers();

    /** Internal tick handler, called each interval (if periodic tick enabled). */
    void HandleTick();

    /** Internal end handler, called on duration/tick completion. */
    void HandleEnd();

    // =====================================================
    //         STAT/ATTRIBUTE MODIFIERS
    // =====================================================

    /** Apply persistent attribute set modifier from config, if any. */
    void ApplyAttributeSetModifier();

    /** Remove persistent attribute set modifier on cleanup. */
    void RemoveAttributeSetModifier();

    /** Applies stat mods and/or damage according to the config's ApplicationMode. */
    virtual void ApplyHybridEffect(const TArray<FStatisticValue>& InStatMods, AActor* InTarget, UObject* InEffectConfig) override;
};