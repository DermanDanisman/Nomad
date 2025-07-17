// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/StatusEffect/NomadBaseStatusEffect.h"
#include "Core/Data/StatusEffect/NomadInfiniteEffectConfig.h"
#include "ARSTypes.h"
#include "Engine/World.h"
#include "NomadInfiniteStatusEffect.generated.h"

// Forward declarations
class UNomadBaseStatusEffect;

// =====================================================
//     CLASS DECLARATION: INFINITE STATUS EFFECT
// =====================================================

/**
 * UNomadInfiniteStatusEffect
 * --------------------------
 * Persistent, data-driven status effect that lasts indefinitely until manually removed.
 * 
 * Key Features:
 * - Persistent until manually removed (water removes dehydration)
 * - Optional periodic ticking for ongoing effects
 * - Stack-aware persistent attribute modifiers
 * - Stat modifications on activation/tick/deactivation
 * - Manual removal permission system with bypass tags
 * - Save/load persistence control
 * - Chain effect support for activation/deactivation
 * - Full hybrid system integration
 * - Smart removal system support
 * 
 * Perfect for:
 * - Equipment bonuses that last until unequipped
 * - Permanent curses until cured
 * - Racial traits and class features
 * - Environmental states (well-fed, dehydrated)
 * - Character conditions (diseased, blessed)
 * 
 * Design Philosophy:
 * - Smart removal: items remove completely (water removes all dehydration)
 * - Stack-aware: all modifications scale with current stack count
 * - Permission-based removal with bypass system
 * - Configurable persistence through save/load
 * - Manager integration for proper cleanup
 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="Infinite Status Effect"))
class NOMADDEV_API UNomadInfiniteStatusEffect : public UNomadBaseStatusEffect
{
    GENERATED_BODY()

public:
    // =====================================================
    //         CONSTRUCTOR & INITIALIZATION
    // =====================================================
    UNomadInfiniteStatusEffect();

    // =====================================================
    //         STACKING / REFRESH LOGIC
    // =====================================================
    
    /** Called when effect is stacked (gains additional stacks) */
    UFUNCTION(BlueprintNativeEvent, Category="Nomad Infinite Effect | Stacking")
    void OnStacked(int32 NewStackCount);
    virtual void OnStacked_Implementation(int32 NewStackCount);

    /** Called when effect is refreshed (reapplied at max stacks) */
    UFUNCTION(BlueprintNativeEvent, Category="Nomad Infinite Effect | Stacking")
    void OnRefreshed();
    virtual void OnRefreshed_Implementation();

    /** Called by manager when a stack is removed */
    virtual void OnUnstacked(int32 NewStackCount);

protected:
    // =====================================================
    //         RUNTIME STATE
    // =====================================================

    /** Cached tick interval (seconds), loaded from config on activation */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    float CachedTickInterval = 5.0f;

    /** If true, this effect should tick periodically (from config) */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    bool bCachedHasPeriodicTick = false;

    /** Persistent attribute set modifier GUID, for removal */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    FGuid AppliedModifierGuid;

    /** Timestamp of activation (seconds since world start) */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    float StartTime = 0.0f;

    /** Number of ticks elapsed since activation */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    int32 TickCount = 0;

    /** Tracks last tick's damage for analytics/UI */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    float LastTickDamage = 0.0f;

    /** Internal current stack count (updated by manager) */
    int32 StackCount = 1;

public:
    // =====================================================
    //         CONFIGURATION ACCESS
    // =====================================================
    
    /** Loads and returns the config asset, or nullptr if not set/invalid */
    virtual UNomadInfiniteEffectConfig* GetEffectConfig() const override;

    /** Applies all configuration data to this effect instance */
    UFUNCTION(BlueprintCallable, Category="Nomad Infinite Effect | Configuration")
    void ApplyConfiguration();

    /** Returns true if configuration is loaded and valid */
    UFUNCTION(BlueprintPure, Category="Nomad Infinite Effect | Configuration")
    bool HasValidConfiguration() const;

    /** Applies tag from configuration to this effect instance */
    UFUNCTION(BlueprintCallable, Category="Nomad Infinite Effect | Configuration")
    void ApplyConfigurationTag();

    /** Applies icon from configuration to this effect instance */
    UFUNCTION(BlueprintCallable, Category="Nomad Infinite Effect | Configuration")
    void ApplyConfigurationIcon();

    /** Gets the current effect tag (from config or ACF base) */
    UFUNCTION(BlueprintPure, Category="Nomad Infinite Effect | Configuration")
    FGameplayTag GetEffectiveTag() const;

    /** Returns effect category from config, or from parent if missing */
    virtual ENomadStatusCategory GetStatusCategory_Implementation() const override;

    // =====================================================
    //         QUERY FUNCTIONS
    // =====================================================

    /** Returns the tick interval for periodic ticking (in seconds) */
    UFUNCTION(BlueprintPure, Category="Nomad Infinite Effect | Query")
    float GetEffectiveTickInterval() const { return CachedTickInterval; }

    /** Returns true if this effect is configured to tick periodically */
    UFUNCTION(BlueprintPure, Category="Nomad Infinite Effect | Query")
    bool HasPeriodicTick() const { return bCachedHasPeriodicTick; }

    /** Returns the uptime (in seconds) since this effect was activated */
    UFUNCTION(BlueprintPure, Category="Nomad Infinite Effect | Query")
    float GetUptime() const;

    /** Returns the total number of ticks that have occurred since activation */
    UFUNCTION(BlueprintPure, Category="Nomad Infinite Effect | Query")
    int32 GetTickCount() const { return TickCount; }

    /** Returns true if this effect can be removed manually (per config) */
    UFUNCTION(BlueprintPure, Category="Nomad Infinite Effect | Query")
    bool CanBeManuallyRemoved() const;

    /** Returns true if this effect should persist through save/load operations */
    UFUNCTION(BlueprintPure, Category="Nomad Infinite Effect | Query")
    bool ShouldPersistThroughSaveLoad() const;

    /** Gets last tick damage for analytics/UI */
    UFUNCTION(BlueprintPure, Category="Nomad Infinite Effect | Query")
    float GetLastTickDamage() const { return LastTickDamage; }

    /** Gets current stack count from manager */
    UFUNCTION(BlueprintPure, Category="Nomad Infinite Effect | Query")
    int32 GetCurrentStackCount() const;

    // =====================================================
    //         MANUAL/FORCED CONTROL (REMOVAL)
    // =====================================================

    /** Attempt manual removal (checks permissions, calls removal events) */
    UFUNCTION(BlueprintCallable, Category="Nomad Infinite Effect | Control")
    bool TryManualRemoval(AActor* Remover = nullptr);

    /** Force remove this effect (ignores permissions, always succeeds) */
    UFUNCTION(BlueprintCallable, Category="Nomad Infinite Effect | Control")
    void ForceRemoval();

    /** Call to trigger standard activation logic (for scripting/BP/manual triggers) */
    virtual void Nomad_OnStatusEffectStarts(ACharacter* Character) override;

protected:
    // =====================================================
    //         HYBRID SYSTEM: STAT/DAMAGE/BOTH APPLICATION
    // =====================================================

    /** Applies stat mods and/or damage according to the config's ApplicationMode */
    virtual void ApplyHybridEffect(const TArray<FStatisticValue>& InStatMods, AActor* InTarget, UObject* InEffectConfig) override;

    // =====================================================
    //         EFFECT LIFECYCLE: START / END
    // =====================================================

    /** Called when the effect is applied to a character */
    virtual void OnStatusEffectStarts_Implementation(ACharacter* Character) override;

    /** Called when the effect is removed from the character */
    virtual void OnStatusEffectEnds_Implementation() override;

    // =====================================================
    //         INFINITE EFFECT EVENTS (BLUEPRINT HOOKS)
    // =====================================================

    /** Called once when this infinite status effect is activated on a character */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect | Events")
    void OnInfiniteEffectActivated(ACharacter* Character);
    virtual void OnInfiniteEffectActivated_Implementation(ACharacter* Character);

    /** Called on every periodic tick if ticking is enabled for this effect */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect | Events")
    void OnInfiniteTick(float Uptime, int32 CurrentTickCount);
    virtual void OnInfiniteTick_Implementation(float Uptime, int32 CurrentTickCount);

    /** Called when a manual removal of this effect is attempted */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect | Events")
    bool OnManualRemovalAttempt(AActor* Remover);
    virtual bool OnManualRemovalAttempt_Implementation(AActor* Remover);

    /** Called when this infinite effect is deactivated and removed */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect | Events")
    void OnInfiniteEffectDeactivated();
    virtual void OnInfiniteEffectDeactivated_Implementation();

    /** Called when stat modifications are applied by this effect */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect | Events")
    void OnStatModificationsApplied(const TArray<FStatisticValue>& StatisticModifications);
    virtual void OnStatModificationsApplied_Implementation(const TArray<FStatisticValue>& StatisticModifications);

    /** Event called when a persistent attribute modifier is applied */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect | Events")
    void OnPersistentAttributeApplied(const FAttributeModifier& PersistentAttributeModifier);

    /** Event called when a persistent attribute modifier is removed */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect | Events")
    void OnPersistentAttributeRemoved(const FAttributeModifier& PersistentAttributeModifier);

    // =====================================================
    //         TIMER MANAGEMENT (PERIODIC TICK)
    // =====================================================

    /** Internal function called on each periodic tick */
    virtual void HandleInfiniteTick();

    /** Sets up periodic ticking if enabled by config */
    void SetupInfiniteTicking();

    /** Clears/cancels periodic ticking timer */
    void ClearInfiniteTicking();

private:
    // =====================================================
    //         TIMER HANDLE
    // =====================================================

    /** Handle for periodic tick timer */
    FTimerHandle TickTimerHandle;

    // =====================================================
    //         MODIFIER HELPERS
    // =====================================================

    /** Applies persistent attribute set modifiers from the effect config */
    void ApplyAttributeSetModifier();

    /** Removes previously-applied persistent attribute set modifiers */
    void RemoveAttributeSetModifier();

    /** Caches config values on activation for performance/safety */
    void CacheConfigurationValues();
};