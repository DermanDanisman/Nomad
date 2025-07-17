// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "Components/ACFStatusEffectManagerComponent.h"
#include "GameplayTagContainer.h"
#include "NomadStatusEffectManagerComponent.generated.h"

class UNomadBaseStatusEffect;
enum class ENomadAfflictionNotificationType : uint8;
class UNomadTimedStatusEffect;

/**
 * FActiveEffect
 * -------------
 * Struct representing a currently active status effect on an actor.
 * Each entry tracks:
 *  - The effect's unique gameplay tag
 *  - Its current stack count (for stackable effects)
 *  - The actual UObject instance (for runtime logic and callbacks)
 */
USTRUCT()
struct FActiveEffect
{
    GENERATED_BODY()

    /** Unique gameplay tag for this effect type, as defined in its config asset */
    FGameplayTag Tag;

    /** Stack count for this effect (1 if not stackable, 2+ if stacking) */
    int32 StackCount = 1;

    /** The runtime UObject instance for this effect (may be nullptr if not applied) */
    UPROPERTY()
    UNomadBaseStatusEffect* EffectInstance = nullptr;
};

/**
 * UNomadStatusEffectManagerComponent
 * ----------------------------------
 * Project-specific extension of the ACF status effect manager.
 * - Handles all status effect creation, stacking, refreshing, and removal for Nomad systems.
 * - Maintains a list of active effects (with stack count and instance pointer).
 * - Notifies the UI (affliction bar) via NomadAfflictionComponent for any change.
 * - Tracks and exposes total and per-effect status effect damage for analytics/UI.
 * - Use this class for all status effect scripting in Nomad Blueprints and code.
 */
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NOMADDEV_API UNomadStatusEffectManagerComponent : public UACFStatusEffectManagerComponent
{
    GENERATED_BODY()

public:

    UNomadStatusEffectManagerComponent();

    /**
     * Adds (creates and applies) a status effect by class, handles stacking or refresh if present,
     * notifies the UI after any change.
     * @param StatusEffectClass - The effect class (BP or C++) to add.
     * @param Instigator - The actor that caused the effect (may be nullptr).
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect")
    void Nomad_AddStatusEffect(TSubclassOf<UACFBaseStatusEffect> StatusEffectClass, AActor* Instigator);

    /**
     * Removes a status effect by gameplay tag, updating stack count or removing entirely.
     * Notifies the UI after any change.
     * @param StatusEffectTag - The gameplay tag of the effect to remove.
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect")
    void Nomad_RemoveStatusEffect(FGameplayTag StatusEffectTag);

    // ---- DAMAGE TRACKING ----

    /** Adds to the total and per-effect status effect damage (can be negative for healing). */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect|Damage")
    void AddStatusEffectDamage(FGameplayTag EffectTag, float Delta);

    /** Returns the total damage (or healing, if negative) done by all status effects. */
    UFUNCTION(BlueprintPure, Category="Nomad|Status Effect|Damage")
    float GetTotalStatusEffectDamage() const;

    /** Returns the total damage (or healing) done by a specific status effect (by tag). */
    UFUNCTION(BlueprintPure, Category="Nomad|Status Effect|Damage")
    float GetStatusEffectDamageByTag(FGameplayTag EffectTag) const;

    /** Returns a map of all status effect tags and their respective damage totals. */
    UFUNCTION(BlueprintPure, Category="Nomad|Status Effect|Damage")
    TMap<FGameplayTag, float> GetAllStatusEffectDamages() const;

    /** Resets all tracked status effect damage values (call on respawn, phase change, etc). */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect|Damage")
    void ResetStatusEffectDamageTracking();

protected:

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    
    /** Array of all currently active effects on the actor (each with tag, stack, and instance pointer) */
    UPROPERTY()
    TArray<FActiveEffect> ActiveEffects;

    /**
     * Returns array index for an effect by tag, or INDEX_NONE if not present.
     * @param Tag - The gameplay tag to search for.
     */
    int32 FindActiveEffectIndexByTag(FGameplayTag Tag) const;

    /**
     * Notifies the NomadAfflictionComponent (UI) of a change in affliction state (apply, stack, refresh, remove).
     * This is called after any change in effect state.
     * @param Tag - The effect's gameplay tag.
     * @param Type - The kind of notification (applied, stacked, removed, etc).
     * @param PrevStacks - Stack count before the change.
     * @param NewStacks - Stack count after the change.
     */
    void NotifyAffliction(FGameplayTag Tag, ENomadAfflictionNotificationType Type, int32 PrevStacks, int32 NewStacks) const;

    /**
     * Handles core logic for effect instantiation, stacking, refreshing, and removal.
     * Called internally when applying an effect, and by the ACF base implementation.
     */
    virtual void CreateAndApplyStatusEffect_Implementation(TSubclassOf<UACFBaseStatusEffect> StatusEffectToConstruct, AActor* Instigator) override;
    virtual void AddStatusEffect(UACFBaseStatusEffect* StatusEffect, AActor* Instigator) override;
    virtual void RemoveStatusEffect_Implementation(FGameplayTag StatusEffectTag) override;

    // ---- DAMAGE TRACKING DATA ----

    /** Total damage (or healing, if negative) done by status effects. */
    UPROPERTY(BlueprintReadOnly, Category="Nomad|Status Effect|Damage")
    float TotalStatusEffectDamage = 0.0f;

    /** Map of individual status effect tag to total damage/healing done. */
    UPROPERTY(BlueprintReadOnly, Category="Nomad|Status Effect|Damage")
    TMap<FGameplayTag, float> StatusEffectDamageTotals;
};