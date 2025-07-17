// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/StatusEffect/NomadBaseStatusEffect.h"
#include "Core/Data/StatusEffect/NomadTimedEffectConfig.h"
#include "ARSTypes.h"
#include "Engine/World.h"
#include "NomadTimedStatusEffect.generated.h"

// Forward declarations
class UNomadStatusEffectManagerComponent;
class UNomadBaseStatusEffect;

// =====================================================
//             CLASS DECLARATION: TIMED EFFECT
// =====================================================

/**
 * UNomadTimedStatusEffect
 * -----------------------
 * Data-driven, highly extensible timed status effect with enhanced smart removal support.
 * 
 * Key Features:
 * - Duration-based OR tick-based expiration
 * - Periodic ticking with configurable intervals
 * - Stack-aware stat modifications (all mods scale with stack count)
 * - Persistent attribute modifiers during effect lifetime
 * - Smart removal system integration
 * - Chain effect support
 * - Pause/resume functionality
 * - Full hybrid system support (stat mods, damage events, or both)
 * - Robust timer and memory management
 * - Manager integration for stacking and cleanup
 * 
 * Perfect for:
 * - Damage/healing over time (poison, regeneration)
 * - Temporary buffs/debuffs (haste, slow)
 * - Environmental effects (burning, freezing)
 * - Status conditions with duration
 * 
 * Design Philosophy:
 * - All behavior driven by config assets
 * - Manager handles UI notifications and stacking logic
 * - Stack-aware: all modifications scale with current stack count
 * - Smart removal: items remove ALL stacks (bandage removes all bleeding)
 * - Robust cleanup prevents memory leaks and double-removal
 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="Timed Status Effect"))
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
    TObjectPtr<UNomadStatusEffectManagerComponent> OwningManager = nullptr;

    /** Called by manager to start this effect and bind manager for stack/tick queries */
    void NomadStartEffectWithManager(ACharacter* Character, UNomadStatusEffectManagerComponent* Manager);

    /** Called by manager to end this effect and unbind manager */
    void NomadEndEffectWithManager();

    // =====================================================
    //         RUNTIME STATE
    // =====================================================

    /** Timestamp when effect started (seconds since world start) */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    float StartTime = 0.0f;

    /** Number of ticks elapsed since effect started */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    int32 CurrentTickCount = 0;

    /** GUID of attribute set modifier applied by this effect */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    FGuid AppliedModifierGuid;
    
    /** The last amount of damage (or healing) applied on tick */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    float LastTickDamage = 0.0f;

protected:
    /** Internal current stack count (updated by manager) */
    int32 StackCount = 1;

public:
    // =====================================================
    //         CONFIGURATION ACCESS
    // =====================================================

    /** Loads configuration asset (synchronously, safe for runtime use) */
    virtual UNomadTimedEffectConfig* GetEffectConfig() const override;
    
    // =====================================================
    //         STACKING/REFRESH LOGIC
    // =====================================================

    /** Restarts duration/tick timers (used for stacking/refreshing) */
    UFUNCTION(BlueprintCallable, Category="Nomad Timed Effect | Stacking")
    virtual void RestartTimerIfStacking();

    /** Called by manager when a stack is removed */
    virtual void OnUnstacked(int32 NewStackCount);

    /** Called when effect is stacked (gains additional stacks) */
    UFUNCTION(BlueprintNativeEvent, Category="Nomad Timed Effect | Stacking")
    void OnStacked(int32 NewStackCount);
    virtual void OnStacked_Implementation(int32 NewStackCount);

    /** Called when effect is refreshed (reapplied at max stacks) */
    UFUNCTION(BlueprintNativeEvent, Category="Nomad Timed Effect | Stacking")
    void OnRefreshed();
    virtual void OnRefreshed_Implementation();

    // =====================================================
    //         QUERY FUNCTIONS
    // =====================================================

    /** Gets current stack count from manager */
    UFUNCTION(BlueprintPure, Category="Nomad Timed Effect | Query")
    int32 GetCurrentStackCount() const;

    /** Gets last tick damage for analytics/UI */
    UFUNCTION(BlueprintPure, Category="Nomad Timed Effect | Query")
    float GetLastTickDamage() const { return LastTickDamage; }

    /** Gets effect uptime in seconds */
    UFUNCTION(BlueprintPure, Category="Nomad Timed Effect | Query")
    float GetUptime() const;

    /** Gets remaining duration in seconds */
    UFUNCTION(BlueprintPure, Category="Nomad Timed Effect | Query")
    float GetRemainingDuration() const;

    /** Gets progress as percentage (0.0 to 1.0) */
    UFUNCTION(BlueprintPure, Category="Nomad Timed Effect | Query")
    float GetProgressPercentage() const;

    // =====================================================
    //         CHAIN EFFECTS
    // =====================================================

    /** Triggers chain effects through the manager */
    UFUNCTION(BlueprintCallable, Category="Nomad Timed Effect | Chain Effects")
    void TriggerChainEffects(const TArray<TSoftClassPtr<UNomadBaseStatusEffect>>& ChainEffects);

protected:
    // =====================================================
    //         CORE API: START/END
    // =====================================================

    /** Implementation of start logic (calls base, applies config, etc.) */
    virtual void OnStatusEffectStarts_Implementation(ACharacter* Character) override;

    /** Implementation of end logic (calls base, applies config cleanup, etc.) */
    virtual void OnStatusEffectEnds_Implementation() override;
    
    // =====================================================
    //         TIMED EFFECT EVENTS (Blueprint Hooks)
    // =====================================================

    /** Called immediately when timed effect starts */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect | Events")
    void OnTimedEffectStarted(ACharacter* Character);

    /** Called every time the effect ticks (if periodic) */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect | Events")
    void OnTimedEffectTicked(int32 TickCount);

    /** Called right before the effect ends */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect | Events")
    void OnTimedEffectEnded();

    /** Called when stat modifications are applied */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect | Events")
    void OnTimedEffectStatModificationsApplied(const TArray<FStatisticValue>& Modifications);

    /** Called when attribute set modifier is applied */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect | Events")
    void OnTimedEffectAttributeModifierApplied(const FAttributesSetModifier& Modifier);

    /** Called when chain effects are triggered */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Timed Effect | Events")
    void OnTimedEffectChainEffectsTriggered(const TArray<TSoftClassPtr<UNomadBaseStatusEffect>>& ChainEffects);
    
    // =====================================================
    //         TIMER MANAGEMENT
    // =====================================================

    /** Timer handle for effect end */
    FTimerHandle TimerHandle_End;

    /** Timer handle for periodic tick */
    FTimerHandle TimerHandle_Tick;

    /** Sets up timers for duration and periodic ticks based on config */
    void SetupTimers();

    /** Clears timers on end, stacking, or removal */
    void ClearTimers();

    /** Internal tick handler, called each interval (if periodic tick enabled) */
    virtual void HandleTick();

    /** Internal end handler, called on duration/tick completion */
    virtual void HandleEnd();

    // =====================================================
    //         STAT/ATTRIBUTE MODIFIERS
    // =====================================================

    /** Apply persistent attribute set modifier from config, if any */
    void ApplyAttributeSetModifier();

    /** Remove persistent attribute set modifier on cleanup */
    void RemoveAttributeSetModifier();

    /** Applies stat mods and/or damage according to the config's ApplicationMode */
    virtual void ApplyHybridEffect(const TArray<FStatisticValue>& InStatMods, AActor* InTarget, UObject* InEffectConfig) override;
};