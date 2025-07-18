// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "Components/ACFStatusEffectManagerComponent.h"
#include "GameplayTagContainer.h"
#include "Core/StatusEffect/NomadStatusTypes.h"
#include "NomadStatusEffectManagerComponent.generated.h"

class UNomadSurvivalStatusEffect;
class UNomadBaseStatusEffect;
class UNomadTimedStatusEffect;
class UNomadInfiniteStatusEffect;
class UNomadInstantStatusEffect;

// =====================================================
//                DATA STRUCTURES
// =====================================================

/**
 * FActiveEffect
 * -------------
 * Represents a currently active status effect with enhanced metadata.
 * Contains the effect's gameplay tag, stack count, pointer to effect instance, and timing information.
 * Used for replication, analytics, and UI display.
 */
USTRUCT(BlueprintType)
struct NOMADDEV_API FActiveEffect
{
    GENERATED_BODY()

    /** Default constructor */
    FActiveEffect()
    {
        Tag = FGameplayTag();
        StackCount = 1;
        EffectInstance = nullptr;
        StartTime = 0.0f;
        Duration = 0.0f;
    }

    /** Constructor with tag and instance */
    FActiveEffect(FGameplayTag InTag, int32 InStackCount, UNomadBaseStatusEffect* InInstance)
    {
        Tag = InTag;
        StackCount = InStackCount;
        EffectInstance = InInstance;
        StartTime = 0.0f;
        Duration = 0.0f;
    }

    /** Unique tag for this effect (used for stacking/removal and analytics) */
    UPROPERTY(BlueprintReadOnly, Category="Active Effect")
    FGameplayTag Tag;

    /** Number of stacks for this effect (1 if not stackable, >1 if stacking) */
    UPROPERTY(BlueprintReadOnly, Category="Active Effect")
    int32 StackCount = 1;

    /** The runtime effect instance. (Not replicated, only valid on authority.) */
    UPROPERTY()
    UNomadBaseStatusEffect* EffectInstance = nullptr;

    /** Time (in seconds) when the effect started. Used for replication/persistence. */
    UPROPERTY(BlueprintReadOnly, Category="Active Effect")
    float StartTime = 0.0f;

    /** Duration (in seconds) for this effect instance. Used for replication/persistence. */
    UPROPERTY(BlueprintReadOnly, Category="Active Effect")
    float Duration = 0.0f;

    // ======== Operators for container support ========

    bool operator==(const FActiveEffect& Other) const
    {
        return Tag == Other.Tag;
    }

    bool operator==(const FGameplayTag& OtherTag) const
    {
        return Tag == OtherTag;
    }
};

// =====================================================
//        CLASS DECLARATION: STATUS EFFECT MANAGER
// =====================================================

/**
 * UNomadStatusEffectManagerComponent
 * ----------------------------------
 * Enhanced status effect manager that extends ACF with Nomad-specific functionality.
 *
 * Key Features:
 * - Smart Removal System: Intelligently removes effects based on their type
 * - Damage Analytics: Tracks damage/healing done by status effects
 * - Blocking Tags: Prevents certain actions while effects are active
 * - Enhanced Stacking: Proper stack management with notifications
 * - Replication: Efficient replication for multiplayer support
 * - Query System: Rich querying capabilities for effects
 * - UI Integration: Seamless integration with affliction UI components
 *
 * Supported Effect Types:
 * - Instant: Apply once and done (healing potions, damage spells)
 * - Timed: Duration-based effects that can stack (poison, buffs)
 * - Infinite: Permanent effects until manually removed (curses, traits)
 * - Survival: Special survival-related effects (hunger, temperature)
 *
 * Design Philosophy:
 * - Manager handles all UI notifications and analytics
 * - Effects themselves contain no UI logic
 * - Data-driven configuration via config assets
 * - Supports both item-based and condition-based removal
 * - Thread-safe and network-optimized
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
    //         SMART REMOVAL SYSTEM
    // =====================================================

    /**
     * Intelligently removes a status effect based on its type and configuration.
     * - Timed/Stackable Effects: Removes ALL stacks (like bandage removing all bleeding)
     * - Infinite Effects: Removes completely (like water removing dehydration)
     * - Returns true if effect was found and removed
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect|Smart Removal")
    bool Nomad_RemoveStatusEffectSmart(FGameplayTag StatusEffectTag);

    /**
     * Removes a single stack from stackable effects only.
     * Used for natural decay or weak items that only remove one stack.
     * Non-stackable effects are unaffected.
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect|Stack Management")
    bool Nomad_RemoveStatusEffectStack(FGameplayTag StatusEffectTag);

    /**
     * Force removes all stacks of any effect type.
     * Use for powerful items or admin commands.
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect|Force Removal")
    bool Nomad_RemoveStatusEffectCompletely(FGameplayTag StatusEffectTag);

    /**
     * Removes all effects matching a parent tag.
     * Example: "StatusEffect.Survival" removes all survival effects
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect|Batch Removal")
    int32 Nomad_RemoveStatusEffectsByParentTag(FGameplayTag ParentTag);

    /**
     * Removes all effects of a specific category.
     * Example: Remove all debuffs, all buffs, etc.
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect|Batch Removal")
    int32 Nomad_RemoveStatusEffectsByCategory(ENomadStatusCategory Category);

    /**
     * Removes multiple specific effects by their exact tags.
     * Useful for items that cure multiple specific conditions.
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect|Batch Removal")
    int32 Nomad_RemoveStatusEffectsMultiple(const TArray<FGameplayTag>& StatusEffectTags);

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
    //         QUERY SYSTEM
    // =====================================================

    /** Returns a copy of all currently active effects. */
    TArray<FActiveEffect> GetActiveEffects() const { return ActiveEffects; }

    /** Finds the index of an active effect by tag. Returns INDEX_NONE if not found. */
    int32 FindActiveEffectIndexByTag(FGameplayTag Tag) const;

    /** Gets current stack count for an effect (0 if not active). */
    UFUNCTION(BlueprintPure, Category="Nomad|Status Effect|Query")
    int32 GetStatusEffectStackCount(FGameplayTag StatusEffectTag) const;

    /** Checks if effect is currently active. */
    UFUNCTION(BlueprintPure, Category="Nomad|Status Effect|Query")
    bool HasStatusEffect(FGameplayTag StatusEffectTag) const;

    /** Gets maximum possible stacks for this effect type. */
    UFUNCTION(BlueprintPure, Category="Nomad|Status Effect|Query")
    int32 GetStatusEffectMaxStacks(FGameplayTag StatusEffectTag) const;

    /** Checks if effect can be stacked. */
    UFUNCTION(BlueprintPure, Category="Nomad|Status Effect|Query")
    bool IsStatusEffectStackable(FGameplayTag StatusEffectTag) const;

    /** Gets effect type (Timed, Infinite, Instant, Survival). */
    UFUNCTION(BlueprintPure, Category="Nomad|Status Effect|Query")
    EStatusEffectType GetStatusEffectType(FGameplayTag StatusEffectTag) const;

    // =====================================================
    //         DAMAGE ANALYTICS SYSTEM
    // =====================================================

    /** Adds to the total and per-effect damage analytics. */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect|Damage")
    void AddStatusEffectDamage(FGameplayTag EffectTag, float Delta);

    /** Gets the total damage done by all effects. */
    UFUNCTION(BlueprintPure, Category="Nomad|Status Effect|Damage")
    float GetTotalStatusEffectDamage() const;

    /** Gets the total damage done by a specific effect (by tag). */
    UFUNCTION(BlueprintPure, Category="Nomad|Status Effect|Damage")
    float GetStatusEffectDamageByTag(FGameplayTag EffectTag) const;

    /** Gets a map of all effect tags to their damage totals. */
    UFUNCTION(BlueprintPure, Category="Nomad|Status Effect|Damage")
    TMap<FGameplayTag, float> GetAllStatusEffectDamages() const;

    /** Resets all tracked status effect damage values (call on respawn, phase change, etc.). */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect|Damage")
    void ResetStatusEffectDamageTracking();

    // =====================================================
    //         BLOCKING TAG SYSTEM
    // =====================================================

    /** Adds a blocking tag to prevent certain actions. */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect|Blocking")
    void AddBlockingTag(const FGameplayTag& Tag);

    /** Removes a blocking tag to restore certain actions. */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect|Blocking")
    void RemoveBlockingTag(const FGameplayTag& Tag);

    /** Checks if a specific action is currently blocked. */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect|Blocking")
    bool HasBlockingTag(const FGameplayTag& Tag) const;

    // =====================================================
    //         SPECIALIZED APPLICATION METHODS
    // =====================================================

    /** Applies a hazard DoT status effect and sets the DoT percent. Returns the effect instance. */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect|Specialized")
    UNomadSurvivalStatusEffect* ApplyHazardDoTEffectWithPercent(const TSubclassOf<UNomadBaseStatusEffect>& EffectClass, float DotPercent);

    /**
     * Applies a timed status effect with the specified duration.
     * The effect will automatically be removed when the duration expires.
     *
     * @param StatusEffectClass The status effect class to apply (must be UNomadTimedStatusEffect or derived)
     * @param Duration Duration in seconds for the effect (must be > 0)
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect|Application")
    void ApplyTimedStatusEffect(TSubclassOf<UNomadBaseStatusEffect> StatusEffectClass, float Duration);

    /**
     * Applies an infinite status effect that persists until manually removed.
     * The effect will remain active indefinitely until explicitly removed.
     *
     * @param StatusEffectClass The status effect class to apply (must be UNomadInfiniteStatusEffect or derived)
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Status Effect|Application")
    void ApplyInfiniteStatusEffect(TSubclassOf<UNomadBaseStatusEffect> StatusEffectClass);

protected:
    // =====================================================
    //         REPLICATION & NETWORKING
    // =====================================================

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Replicated array of all currently active effects. (EffectInstance is NOT replicated.) */
    UPROPERTY(ReplicatedUsing=OnRep_ActiveEffects)
    TArray<FActiveEffect> ActiveEffects;

    /** Called on clients when ActiveEffects changes (used to update UI, VFX, etc.) */
    UFUNCTION()
    void OnRep_ActiveEffects();

    /** Replicated container of active blocking tags. */
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Status Effects")
    FGameplayTagContainer ActiveBlockingTags;

    // =====================================================
    //         DAMAGE ANALYTICS DATA
    // =====================================================

    /** Total damage (or healing, if negative) done by all status effects. */
    UPROPERTY(BlueprintReadOnly, Category="Nomad|Status Effect|Damage")
    float TotalStatusEffectDamage = 0.0f;

    /** Map of effect tag to total damage/healing done. */
    UPROPERTY(BlueprintReadOnly, Category="Nomad|Status Effect|Damage")
    TMap<FGameplayTag, float> StatusEffectDamageTotals;

    // =====================================================
    //         EFFECT LIFECYCLE (INTERNAL)
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
    //         INTERNAL REMOVAL SYSTEM
    // =====================================================

    /** Internal removal with detailed control and logging. */
    bool Internal_RemoveStatusEffectAdvanced(FGameplayTag StatusEffectTag, int32 StacksToRemove, bool bForceComplete, bool bRespectStackability);
};