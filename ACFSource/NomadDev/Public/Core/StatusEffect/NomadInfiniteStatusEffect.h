// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/StatusEffect/NomadBaseStatusEffect.h"
#include "Core/Data/StatusEffect/NomadInfiniteEffectConfig.h"
#include "ARSTypes.h"
#include "Engine/World.h"
#include "NomadInfiniteStatusEffect.generated.h"

/**
 * =========================
 *    Infinite Status Effects
 * =========================
 *
 * Infinite status effects persist indefinitely on a character until manually or forcibly removed.
 * They are used for persistent buffs, curses, traits, equipment bonuses, and any effect that should not expire on its own.
 * 
 * Design Features:
 *  - Data-driven: All properties are set by a config asset (UNomadInfiniteEffectConfig).
 *  - Stackable: Can support stacking, as configured.
 *  - Permissioned removal: Can restrict removal to special bypass tags or admin commands.
 *  - Attribute/stat mods: Can apply persistent or ticking modifiers.
 *  - Periodic ticking: Optionally ticks at intervals for ongoing effects (e.g., regen, drain).
 *  - Full integration: Works with ACF, affliction UI, save/load, and robust memory/network handling.
 *  - Robust against edge cases: Handles double removal, save corruption, permission checks, timer cleanup, stat re-application, etc.
 * 
 * Example Use Cases:
 *  - Equipment bonuses (e.g., +10 STR on sword equip)
 *  - Permanent curses or blessings (e.g., vampirism, lycanthropy)
 *  - Racial/class traits
 *  - Persistent zone effects
 *  - Permanent injuries/debuffs
 *
 * ===============================================
 *     NomadInfiniteStatusEffect (Hybrid)
 * ===============================================
 *
 * Applies persistent/periodic effects as defined in the config.
 *
 * === Hybrid System Usage ===
 * - ApplicationMode (from config) determines whether to apply stat mods, damage, or both on activation/tick/deactivation.
 * - For DamageEvent/Both modes:
 *     - Config.DamageStatisticMods MUST have at least one FStatisticValue (Health, negative value for damage).
 *     - Config.DamageTypeClass MUST be set.
 * - For StatModification:
 *     - Only stat mod arrays are used.
 * - Always call ApplyHybridEffect and OnStatModificationsApplied for analytics/UI.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta=(DisplayName="Infinite Status Effect"))
class NOMADDEV_API UNomadInfiniteStatusEffect : public UNomadBaseStatusEffect
{
    GENERATED_BODY()

public:
    /** Default constructor. Initializes all runtime state. */
    UNomadInfiniteStatusEffect();

protected:
    // ======== Configuration (Asset-Driven) ========
    /** Soft pointer to the infinite effect configuration asset. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Configuration")
    TSoftObjectPtr<UNomadInfiniteEffectConfig> EffectConfig;

    // ======== Runtime State ========
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

    // ======== Hybrid system: Track last damage dealt for UI/analytics ========
    UPROPERTY()
    float LastTickDamage = 0.0f;

public:
    
    // ======== Configuration Access ========
    /** Loads and returns the config asset, or nullptr if not set/invalid. */
    UFUNCTION(BlueprintPure, Category="Configuration")
    UNomadInfiniteEffectConfig* GetEffectConfig() const;

    /** Applies all configuration data to this effect instance. */
    UFUNCTION(BlueprintCallable, Category="Configuration")
    void ApplyConfiguration();

    /** Returns true if configuration is loaded and valid. */
    UFUNCTION(BlueprintPure, Category="Configuration")
    bool HasValidConfiguration() const;

    // ======== Config Integration (ACF-Compatible) ========
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

    // ======== Infinite Effect Properties ========
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

    // ======== Manual/Forced Control ========
    /** Attempt manual removal (checks permissions, calls removal events). */
    UFUNCTION(BlueprintCallable, Category="Nomad Infinite Effect")
    bool TryManualRemoval(AActor* Remover = nullptr);

    /** Force remove this effect (ignores permissions, always succeeds). */
    UFUNCTION(BlueprintCallable, Category="Nomad Infinite Effect", CallInEditor)
    void ForceRemoval();

    /** Call to trigger standard activation logic (for scripting/BP/manual triggers). */
    void Nomad_OnStatusEffectStarts(ACharacter* Character);

protected:
    // ======== ACF Base Class Overrides ========
    /** Called when effect starts. Applies config, stat mods, ticking, and triggers activation events. */
    virtual void OnStatusEffectStarts_Implementation(ACharacter* Character) override;

    /** Called when effect ends. Cleans up all stat mods, timers, and triggers deactivation events. */
    virtual void OnStatusEffectEnds_Implementation() override;

    // ======== Hybrid System: Stat/Damage/Both Application ========
    /**
     * Applies stat mods and/or damage according to the config's ApplicationMode.
     * Used for OnActivationStatModifications, OnTickStatModifications, OnDeactivationStatModifications.
     */
    // ======== Hybrid System: Stat/Damage/Both Application ========
    /**
     * Applies stat mods and/or damage according to the config's ApplicationMode.
     * Used for OnActivationStatModifications, OnTickStatModifications, OnDeactivationStatModifications.
     */
    virtual void ApplyHybridEffect(const TArray<FStatisticValue>& StatMods, AActor* Target, UObject* EffectConfig) override;

    // --- Helper for safe causer, NEVER pass nullptr to ApplyDamage ---
    FORCEINLINE AActor* GetSafeDamageCauser(AActor* Target) const
    {
        return (DamageCauser.IsValid() && !DamageCauser->IsPendingKillPending()) ? DamageCauser.Get() : Target;
    }


    // ======== Infinite Effect Events (Blueprint Hooks) ========
    /** Called once on effect activation; use for custom setup, listeners, etc. */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect")
    void OnInfiniteEffectActivated(ACharacter* Character);

    /** Called every periodic tick, if ticking is enabled. */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect")
    void OnInfiniteTick(float Uptime, int32 CurrentTickCount);

    /** Called when a manual removal is attempted. Return true to allow, false to block. */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect")
    bool OnManualRemovalAttempt(AActor* Remover);

    /** Called when effect is deactivated (manual or forced removal). */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect")
    void OnInfiniteEffectDeactivated();

    /** Called when stat modifications are applied. */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Infinite Effect")
    void OnStatModificationsApplied(const TArray<FStatisticValue>& Modifications);

    // ======== Save/Load Events (Blueprint Hooks) ========
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Save/Load")
    void OnSaveEffectData();

    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Save/Load")
    void OnLoadEffectData();

    // ======== C++ Virtual Event Implementations ========
    virtual void OnInfiniteEffectActivated_Implementation(ACharacter* Character) {}
    virtual void OnInfiniteTick_Implementation(float Uptime, int32 CurrentTickCount) {}
    virtual bool OnManualRemovalAttempt_Implementation(AActor* Remover) { return CanBeManuallyRemoved(); }
    virtual void OnInfiniteEffectDeactivated_Implementation() {}
    virtual void OnStatModificationsApplied_Implementation(const TArray<FStatisticValue>& Modifications) {}
    virtual void OnSaveEffectData_Implementation() {}
    virtual void OnLoadEffectData_Implementation() {}

private:
    // ======== Timer Management ========
    /** Handle for periodic tick timer. Used to start/clear ticking safely. */
    FTimerHandle TickTimerHandle;

    // ======== Internal Helpers ========
    /** Internal function called on each periodic tick. */
    void HandleInfiniteTick();

    /** Sets up periodic ticking if enabled by config. */
    void SetupInfiniteTicking();

    /** Clears/cancels periodic ticking timer. */
    void ClearInfiniteTicking();

    /** Applies stat modifications from config to owner. */
    void ApplyStatModifications(const TArray<FStatisticValue>& Modifications);

    /** Applies persistent attribute set modifier (for attributes, primaries, stats). */
    void ApplyAttributeSetModifier();

    /** Removes previously-applied persistent attribute set modifier. */
    void RemoveAttributeSetModifier();

    /** Caches config values on activation for performance/safety. */
    void CacheConfigurationValues();
};