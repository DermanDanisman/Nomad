// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "Components/ACFStatusEffectManagerComponent.h"
#include "GameplayTagContainer.h"
#include "NomadStatusEffectManagerComponent.generated.h"

class UNomadBaseStatusEffect;
class UNomadTimedStatusEffect;
class UNomadInfiniteStatusEffect;
enum class ENomadAfflictionNotificationType : uint8;

// =====================================================
//                DATA STRUCTURES
// =====================================================

/**
 * FActiveEffect
 * -------------
 * Represents a currently active status effect.
 * Contains the effect's gameplay tag, stack count, pointer to the effect instance, and timing information.
 */
USTRUCT(BlueprintType)
struct FActiveEffect
{
    GENERATED_BODY()

    /** Unique tag for this effect (used for stacking/removal and analytics) */
    FGameplayTag Tag;

    /** Number of stacks for this effect (1 if not stackable, >1 if stacking) */
    int32 StackCount = 1;

    /** The runtime effect instance. (Not replicated, only valid on authority.) */
    UPROPERTY()
    UNomadBaseStatusEffect* EffectInstance = nullptr;

    /** Time (in seconds) when the effect started. Used for replication/persistence. */
    UPROPERTY(BlueprintReadOnly)
    float StartTime = 0.0f;

    /** Duration (in seconds) for this effect instance. Used for replication/persistence. */
    UPROPERTY(BlueprintReadOnly)
    float Duration = 0.0f;
};

// =====================================================
//        CLASS DECLARATION: STATUS EFFECT MANAGER
// =====================================================

/**
 * UNomadStatusEffectManagerComponent
 * ----------------------------------
 * Manages all status effects for an owning actor.
 * Handles effect creation, stacking, refreshing, removal, analytics, and UI notifications.
 */
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NOMADDEV_API UNomadStatusEffectManagerComponent : public UACFStatusEffectManagerComponent
{
    GENERATED_BODY()

public:
    // =====================================================
    //         CONSTRUCTOR & INITIALIZATION
    // =====================================================

    UNomadStatusEffectManagerComponent();

    // =====================================================
    //         PUBLIC API: STATUS EFFECT CONTROL
    // =====================================================

    /** Adds a status effect by class. Handles stacking and UI notification. */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect")
    void Nomad_AddStatusEffect(TSubclassOf<UACFBaseStatusEffect> StatusEffectClass, AActor* Instigator);

    /** Removes a status effect (by tag). Handles stack updates and UI notification. */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect")
    void Nomad_RemoveStatusEffect(FGameplayTag StatusEffectTag);

    // =====================================================
    //         PUBLIC API: GETTERS / ACCESSORS
    // =====================================================

    /** Returns a copy of all currently active effects. */
    TArray<FActiveEffect> GetActiveEffects() const { return ActiveEffects; }

    /** Finds the index of an active effect by tag. Returns INDEX_NONE if not found. */
    int32 FindActiveEffectIndexByTag(FGameplayTag Tag) const;

    // =====================================================
    //         PUBLIC API: DAMAGE ANALYTICS
    // =====================================================

    /** Adds to the total and per-effect damage analytics. */
    UFUNCTION(BlueprintCallable, Category="Nomad | Status Effect | Damage")
    void AddStatusEffectDamage(FGameplayTag EffectTag, float Delta);

    /** Gets the total damage done by all effects. */
    UFUNCTION(BlueprintPure, Category="Nomad | Status Effect | Damage")
    float GetTotalStatusEffectDamage() const;

    /** Gets the total damage done by a specific effect (by tag). */
    UFUNCTION(BlueprintPure, Category="Nomad | Status Effect | Damage")
    float GetStatusEffectDamageByTag(FGameplayTag EffectTag) const;

    /** Gets a map of all effect tags to their damage totals. */
    UFUNCTION(BlueprintPure, Category="Nomad | Status Effect | Damage")
    TMap<FGameplayTag, float> GetAllStatusEffectDamages() const;

    /** Resets all tracked status effect damage values (call on respawn, phase change, etc.). */
    UFUNCTION(BlueprintCallable, Category="Nomad | Status Effect | Damage")
    void ResetStatusEffectDamageTracking();

    UFUNCTION(BlueprintCallable, Category="Nomad | Status Effect | Blocking Tag")
    void AddBlockingTag(const FGameplayTag& Tag);

    UFUNCTION(BlueprintCallable, Category="Nomad | Status Effect | Blocking Tag")
    void RemoveBlockingTag(const FGameplayTag& Tag);

    UFUNCTION(BlueprintCallable, Category="Nomad | Status Effect | Blocking Tag")
    bool HasBlockingTag(const FGameplayTag& Tag) const;

protected:
    // =====================================================
    //         PROTECTED: OVERRIDES
    // =====================================================

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // =====================================================
    //         PROTECTED: REPLICATION
    // =====================================================

    /** Replicated array of all currently active effects. (EffectInstance is NOT replicated.) */
    UPROPERTY(ReplicatedUsing=OnRep_ActiveEffects)
    TArray<FActiveEffect> ActiveEffects;

    /** Called on clients when ActiveEffects changes (used to update UI, VFX, etc.) */
    UFUNCTION()
    void OnRep_ActiveEffects();

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Status Effects")
    FGameplayTagContainer ActiveBlockingTags; // Replicates to clients!

    // =====================================================
    //         PROTECTED: EFFECT LIFECYCLE (INTERNAL)
    // =====================================================

    /**
     * Notifies the NomadAfflictionComponent (UI) of a change in affliction state.
     * @param Tag Effect gameplay tag.
     * @param Type Notification type (applied, stacked, removed, etc.).
     * @param PrevStacks Stack count before the change.
     * @param NewStacks Stack count after the change.
     */
    void NotifyAffliction(FGameplayTag Tag, ENomadAfflictionNotificationType Type, int32 PrevStacks, int32 NewStacks) const;

    /**
     * Core logic for effect instantiation, stacking, refreshing, and removal.
     * Called internally when applying/removing effects, and by the ACF base implementation.
     */
    virtual void CreateAndApplyStatusEffect_Implementation(TSubclassOf<UACFBaseStatusEffect> StatusEffectToConstruct, AActor* Instigator) override;
    virtual void AddStatusEffect(UACFBaseStatusEffect* StatusEffect, AActor* Instigator) override;
    virtual void RemoveStatusEffect_Implementation(FGameplayTag StatusEffectTag) override;

    // =====================================================
    //         PROTECTED: DAMAGE ANALYTICS (DATA)
    // =====================================================

    /** Total damage (or healing, if negative) done by all status effects. */
    UPROPERTY(BlueprintReadOnly, Category="Nomad|Status Effect|Damage")
    float TotalStatusEffectDamage = 0.0f;

    /** Map of effect tag to total damage/healing done. */
    UPROPERTY(BlueprintReadOnly, Category="Nomad|Status Effect|Damage")
    TMap<FGameplayTag, float> StatusEffectDamageTotals;
};