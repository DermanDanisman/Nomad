// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/StatusEffect/NomadBaseStatusEffect.h"
#include "Core/Data/StatusEffect/NomadInstantEffectConfig.h"
#include "ARSTypes.h"
#include "Engine/World.h"
#include "NomadInstantStatusEffect.generated.h"

// Forward declare manager
class UNomadStatusEffectManagerComponent;

/**
 * UNomadInstantStatusEffect
 * ----------------------------------------------------------
 * Data-driven, highly extensible instant status effect.
 * Instantly applies its effect(s) and then self-terminates.
 * - Useful for instant damage, healing, stat buffs/debuffs, triggers, etc.
 * - Handles stat/attribute modifications and/or direct damage events.
 * - Can be triggered by environment, enemy, item, skill, etc.
 * - Fully supports the hybrid stat/damage system: stat, damage, or both.
 * - Supports DamageCauser property for robust analytics and gameplay.
 * - Notifies manager for analytics and safe removal.
 * - No UI logic here: notifications/affliction bar are handled by the manager.
 * 
 * Typical use cases:
 *   - Trap activation (instant damage)
 *   - Healing orb (instant healing)
 *   - Single-use powerup (stat boost for 0 duration)
 *   - On-hit triggers (e.g., instant slow, poison trigger)
 *   
 * ==========================================
 *     NomadInstantStatusEffect (Hybrid)
 * ==========================================
 *
 * Instantly applies a stat modification, damage event, or both, as driven by the config.
 *
 * === Hybrid System Usage ===
 * - ApplicationMode (from config) determines whether to apply stat mods, damage, or both.
 * - For DamageEvent/Both modes:
 *     - Config.DamageStatisticMods MUST have at least one FStatisticValue (Health, negative value for damage).
 *     - Config.DamageTypeClass MUST be set.
 * - For StatModification:
 *     - Only stat mod arrays are used.
 * - Always call ApplyHybridEffect and OnInstantEffectStatModificationsApplied for analytics/UI.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta=(DisplayName="Instant Status Effect"))
class NOMADDEV_API UNomadInstantStatusEffect : public UNomadBaseStatusEffect
{
    GENERATED_BODY()

public:
    UNomadInstantStatusEffect();

    // The manager that owns this effect (set on creation)
    UPROPERTY()
    UNomadStatusEffectManagerComponent* OwningManager = nullptr;

    // ======== Runtime State ========
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    float ActivationTime = 0.0f;

    /** GUID of attribute set modifier applied by this effect (if any). */
    UPROPERTY(BlueprintReadOnly, Category="Runtime")
    FGuid AppliedModifierGuid;

    /** The last amount of damage (or healing) applied. */
    UPROPERTY()
    float LastAppliedDamage = 0.0f;

    // ======== Configuration ========
    /** Configuration data asset for this instant effect */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Configuration")
    TSoftObjectPtr<UNomadInstantEffectConfig> EffectConfig;

    // ======== Core API ========
    /** Loads configuration asset (synchronously, safe for runtime use). */
    UNomadInstantEffectConfig* GetConfig() const;

    /** Main entry point for triggering an instant effect. Applies all effects and ends self. */
    void OnStatusEffectTriggered(ACharacter* Character, UNomadStatusEffectManagerComponent* Manager);

    /** Called to notify when chain effects are triggered. Cosmetic only. */
    UFUNCTION(BlueprintCallable, Category="Nomad Instant Effect")
    void TriggerChainEffects(const TArray<TSoftClassPtr<UNomadBaseStatusEffect>>& ChainEffects);

    UFUNCTION(BlueprintPure, Category="Nomad|Status Effect|Damage")
    float GetLastAppliedDamage() const { return LastAppliedDamage; }

    void Nomad_OnStatusEffectStarts(ACharacter* Character);

protected:
    // ======== Instant Effect Events (Blueprint Hooks) ========

    /** Called immediately after effect triggers. Put your VFX/SFX/UI logic here. */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Instant Effect")
    void OnInstantEffectTriggered(ACharacter* Character);

    /** Called when stat modifications are applied. */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Instant Effect")
    void OnInstantEffectStatModificationsApplied(const TArray<FStatisticValue>& Modifications);

    /** Called when attribute set modifier is applied (if any). */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Instant Effect")
    void OnInstantEffectAttributeModifierApplied(const FAttributesSetModifier& Modifier);

    /** Called when chain effects are triggered (if any). */
    UFUNCTION(BlueprintImplementableEvent, Category="Nomad Instant Effect")
    void OnInstantEffectChainEffectsTriggered(const TArray<TSoftClassPtr<UNomadBaseStatusEffect>>& ChainEffects);

    // ======== Stat/Attribute Modifiers ========
    /** Apply stat changes as defined in config. */
    void ApplyStatModifications(const TArray<FStatisticValue>& Modifications);

    /** Apply persistent attribute set modifier from config, if any. */
    void ApplyAttributeSetModifier();

    /** Remove persistent attribute set modifier on cleanup. */
    void RemoveAttributeSetModifier();

    // ======== ACF Status Effect Overrides ========
    /** Implementation of effect logic (calls base, applies config, etc.). */
    virtual void OnStatusEffectStarts_Implementation(ACharacter* Character) override;

    /** Implementation of end logic (calls base, applies config cleanup, etc.). */
    virtual void OnStatusEffectEnds_Implementation() override;

    /** Applies stat mods and/or damage according to the config's ApplicationMode. */
    virtual void ApplyHybridEffect(const TArray<FStatisticValue>& StatMods, AActor* Target, UObject* EffectConfig) override;

    // --- Helper for safe causer ---
    FORCEINLINE AActor* GetSafeDamageCauser(AActor* Target) const
    {
        return (DamageCauser.IsValid() && !DamageCauser->IsPendingKillPending()) ? DamageCauser.Get() : Target;
    }
};