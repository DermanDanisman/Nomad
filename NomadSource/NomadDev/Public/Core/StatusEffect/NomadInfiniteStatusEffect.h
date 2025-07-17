// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/StatusEffect/NomadBaseStatusEffect.h"
#include "Core/Data/StatusEffect/NomadInfiniteEffectConfig.h"
#include "ARSTypes.h"
#include "Engine/World.h"
#include "NomadInfiniteStatusEffect.generated.h"

// =====================================================
//     CLASS DECLARATION: INFINITE STATUS EFFECT
// =====================================================

/**
 * UNomadInfiniteStatusEffect
 * ----------------------------------------------------------
 * Persistent, data-driven status effect that lasts indefinitely until manually or forcibly removed.
 * - Used for permanent or semi-permanent buffs, curses, traits, equipment bonuses, etc.
 * - Supports stacking, periodic ticking, and persistent attribute/stat/primary modifications.
 * - All logic is driven by config asset (UNomadInfiniteEffectConfig).
 * - Handles robust cleanup and edge cases (double removal, admin removal, manual/forced, etc.).
 * - Provides full integration with ACF, analytics, and affliction UI.
 * - Stacking only changes the magnitude of the effect; no timer/duration reset is performed.
 * - When a stack is gained or lost, persistent modifiers and tick scaling update to match new stack count.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta=(DisplayName="Infinite Status Effect"))
class NOMADDEV_API UNomadInfiniteStatusEffect : public UNomadBaseStatusEffect
{
    GENERATED_BODY()

public:
    // =====================================================
    //         CONSTRUCTOR & INITIALIZATION
    // =====================================================
    UNomadInfiniteStatusEffect();

    // =====================================================
    //         STACKING / REFRESH LOGIC (BP & C++)
    // =====================================================
    UFUNCTION(BlueprintNativeEvent, Category="Status Effect|Stacking")
    void OnStacked(int32 NewStackCount);
    virtual void OnStacked_Implementation(int32 NewStackCount);

    UFUNCTION(BlueprintNativeEvent, Category="Status Effect|Stacking")
    void OnRefreshed();
    virtual void OnRefreshed_Implementation();

    /** Called by manager when a stack is removed. */
    virtual void OnUnstacked(int32 NewStackCount);

protected:
    // =====================================================
    //         RUNTIME STATE
    // =====================================================

    /** Cached tick interval (seconds), loaded from config on activation. */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    float CachedTickInterval = 5.0f;

    /** If true, this effect should tick periodically (from config). */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    bool bCachedHasPeriodicTick = false;

    /** Persistent attribute set modifier GUID, for removal. */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    FGuid AppliedModifierGuid;

    /** Timestamp of activation (seconds since world start). */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    float StartTime = 0.0f;

    /** Number of ticks elapsed since activation. */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    int32 TickCount = 0;

    /** Tracks last tick's damage for analytics/UI. */
    UPROPERTY()
    float LastTickDamage = 0.0f;

    /** Internal, current stack count (updated by manager). */
    int32 StackCount = 1;

public:

    // =====================================================
    //         OVERRIDES & CONFIGURATION ACCESS
    // =====================================================
    
    /** Loads and returns the config asset, or nullptr if not set/invalid. */
    virtual UNomadInfiniteEffectConfig* GetEffectConfig() const override;

    /** Applies all configuration data to this effect instance. */
    UFUNCTION(BlueprintCallable, Category="Configuration")
    void ApplyConfiguration();

    /** Returns true if configuration is loaded and valid. */
    UFUNCTION(BlueprintPure, Category="Configuration")
    bool HasValidConfiguration() const;

    /** Applies tag from configuration to this effect instance. */
    UFUNCTION(BlueprintCallable, Category="Configuration")
    void ApplyConfigurationTag();

    /** Applies icon from configuration to this effect instance. */
    UFUNCTION(BlueprintCallable, Category="Configuration")
    void ApplyConfigurationIcon();

    /** Gets the current effect tag (from config or ACF base). */
    UFUNCTION(BlueprintPure, Category="Status Effect")
    FGameplayTag GetEffectiveTag() const;

    /** Returns effect category from config, or from parent if missing. */
    virtual ENomadStatusCategory GetStatusCategory_Implementation() const override;

    UFUNCTION(BlueprintPure, Category="Nomad|Status Effect|Damage")
    float GetLastTickDamage() const { return LastTickDamage; }

    // =====================================================
    //         INFINITE EFFECT PROPERTIES (QUERIES)
    // =====================================================

    /** Returns the tick interval for periodic ticking (in seconds). */
    UFUNCTION(BlueprintPure, Category="Nomad Infinite Effect")
    float GetEffectiveTickInterval() const { return CachedTickInterval; }

    /** Returns true if this effect is configured to tick periodically. */
    UFUNCTION(BlueprintPure, Category="Nomad Infinite Effect")
    bool HasPeriodicTick() const { return bCachedHasPeriodicTick; }

    /** Returns the uptime (in seconds) since this effect was activated. */
    UFUNCTION(BlueprintPure, Category="Nomad Infinite Effect")
    float GetUptime() const;

    /** Returns the total number of ticks that have occurred since activation. */
    UFUNCTION(BlueprintPure, Category="Nomad Infinite Effect")
    int32 GetTickCount() const { return TickCount; }

    /** Returns true if this effect can be removed manually (per config). */
    UFUNCTION(BlueprintPure, Category="Nomad Infinite Effect")
    bool CanBeManuallyRemoved() const;

    /** Returns true if this effect should persist through save/load operations. */
    UFUNCTION(BlueprintPure, Category="Nomad Infinite Effect")
    bool ShouldPersistThroughSaveLoad() const;

    // =====================================================
    //         MANUAL/FORCED CONTROL (REMOVAL)
    // =====================================================

    /** Attempt manual removal (checks permissions, calls removal events). */
    UFUNCTION(BlueprintCallable, Category="Nomad Infinite Effect")
    bool TryManualRemoval(AActor* Remover = nullptr);

    /** Force remove this effect (ignores permissions, always succeeds). */
    UFUNCTION(BlueprintCallable, Category="Nomad Infinite Effect", CallInEditor)
    void ForceRemoval();

    /** Call to trigger standard activation logic (for scripting/BP/manual triggers). */
    virtual void Nomad_OnStatusEffectStarts(ACharacter* Character) override;

    /** Utility: Get current stack count for this effect from the manager (by tag) */
    UFUNCTION(BlueprintPure, Category="Status Effect|Stacking")
    int32 GetCurrentStackCount() const;

protected:
    // =====================================================
    //         HYBRID SYSTEM: STAT/DAMAGE/BOTH APPLICATION
    // =====================================================

    /**
     * Applies stat mods and/or damage according to the config's ApplicationMode.
     * Used for OnActivationStatModifications, OnTickStatModifications, OnDeactivationStatModifications.
     */
    virtual void ApplyHybridEffect(const TArray<FStatisticValue>& InStatMods, AActor* InTarget, UObject* InEffectConfig) override;

    // =====================================================
    //         EFFECT LIFECYCLE: START / END
    // =====================================================

    /** Called when the effect is applied to a character. */
    virtual void OnStatusEffectStarts_Implementation(ACharacter* Character) override;

    /** Called when the effect is removed from the character (by manager or forcibly). */
    virtual void OnStatusEffectEnds_Implementation() override;

    // =====================================================
    //         INFINITE EFFECT EVENTS (BLUEPRINT HOOKS)
    // =====================================================

    /**
     * Called once when this infinite status effect is activated on a character.
     * - Use for custom setup, initializing listeners, UI, or other per-effect logic.
     * - Called after configuration and stat mods have been applied.
     * @param Character The character this effect was applied to.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect")
    void OnInfiniteEffectActivated(ACharacter* Character);
    virtual void OnInfiniteEffectActivated_Implementation(ACharacter* Character);

    /**
     * Called on every periodic tick if ticking is enabled for this effect.
     * - Implement in Blueprint for time-based effects (e.g., heal/damage over time, UI updates).
     * @param Uptime            Total time (in seconds) since effect activation.
     * @param CurrentTickCount  Number of ticks since effect activation.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect")
    void OnInfiniteTick(float Uptime, int32 CurrentTickCount);
    virtual void OnInfiniteTick_Implementation(float Uptime, int32 CurrentTickCount);

    /**
     * Called when a manual removal of this effect is attempted (e.g., by a player or admin).
     * - Return true to allow removal, false to block (e.g., if effect is "uncurable").
     * @param Remover   The actor attempting to remove this effect.
     * @return          True if removal is allowed; false otherwise.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect")
    bool OnManualRemovalAttempt(AActor* Remover);
    virtual bool OnManualRemovalAttempt_Implementation(AActor* Remover);

    /**
     * Called when this infinite effect is deactivated and removed from the character
     * (either manually, forcibly, or via expiration).
     * - Use for cleanup, VFX/SFX, and removing listeners/UI.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect")
    void OnInfiniteEffectDeactivated();
    virtual void OnInfiniteEffectDeactivated_Implementation();

    /**
     * Called when stat modifications are applied by this effect (on activation, tick, or removal).
     * - Use for gameplay reactions, UI, or helper calls tied to stat changes.
     * @param StatisticModifications  Array of FStatisticValue detailing each applied stat modification.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect")
    void OnStatModificationsApplied(const TArray<FStatisticValue>& StatisticModifications);
    virtual void OnStatModificationsApplied_Implementation(const TArray<FStatisticValue>& StatisticModifications);

    /**
     * Event called when a persistent FAttributeModifier is applied to the character by this status effect.
     * - Implement in Blueprint to react to persistent buffs/debuffs (e.g., movement slow, max health bonus, etc.).
     * - Called once per FAttributeModifier in the config when the effect is activated or a stack is gained.
     * @param PersistentAttributeModifier    The persistent attribute modifier that was applied (tag, value, type).
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Nomad Infinite Effect | Persistent Attributes")
    void OnPersistentAttributeApplied(const FAttributeModifier& PersistentAttributeModifier);

    /**
     * Event called when a persistent FAttributeModifier is removed from the character by this status effect.
     * - Implement in Blueprint to undo gameplay/visual logic or cleanup (e.g., restore movement speed).
     * - Called once per FAttributeModifier in the config when the effect ends or a stack is lost.
     * @param PersistentAttributeModifier    The persistent attribute modifier that was removed (tag, value, type).
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Nomad Infinite Effect | Persistent Attributes")
    void OnPersistentAttributeRemoved(const FAttributeModifier& PersistentAttributeModifier);

private:
    // =====================================================
    //         TIMER MANAGEMENT (PERIODIC TICK)
    // =====================================================

    /** Handle for periodic tick timer. Used to start/clear ticking safely. */
    FTimerHandle TickTimerHandle;

    /** Internal function called on each periodic tick. */
    virtual void HandleInfiniteTick();

    /** Sets up periodic ticking if enabled by config. */
    void SetupInfiniteTicking();

    /** Clears/cancels periodic ticking timer. */
    void ClearInfiniteTicking();

    // =====================================================
    //         MODIFIER HELPERS
    // =====================================================

    /**
     * Applies persistent attribute set modifiers from the effect config to the character.
     * - Handles all persistent PrimaryAttributes, Attributes, and Statistics modifiers.
     * - Persists the modifier GUID for later removal.
     * - Notifies Blueprint via OnPersistentAttributeApplied for each FAttributeModifier applied.
     * - No-ops if already applied, or if config has no persistent mods.
     */
    void ApplyAttributeSetModifier();

    /**
     * Removes previously-applied persistent attribute set modifiers from the character.
     * - Removes all persistent mods by GUID for this effect instance.
     * - Notifies Blueprint via OnPersistentAttributeRemoved for each FAttributeModifier removed.
     * - Safe to call multiple times (no double removal or crash).
     */
    void RemoveAttributeSetModifier();

    /** Caches config values on activation for performance/safety. */
    void CacheConfigurationValues();
};