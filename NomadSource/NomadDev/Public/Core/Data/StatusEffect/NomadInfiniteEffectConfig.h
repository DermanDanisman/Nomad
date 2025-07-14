// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/StatusEffect/NomadStatusEffectConfigBase.h"
#include "ARSTypes.h"
#include "GameplayTagContainer.h"
#include "NomadInfiniteEffectConfig.generated.h"

class UNomadBaseStatusEffect;

/**
 * UNomadInfiniteEffectConfig
 * --------------------------
 * Configuration data asset for infinite duration status effects.
 * These effects persist indefinitely until manually removed.
 * 
 * Features:
 * - Persistent attribute modifiers (applied until removal)
 * - Stat modifications on activation/deactivation/tick
 * - Optional periodic ticking with configurable intervals
 * - Manual removal permission system
 * - Save/load persistence control
 * - Chain effect support for activation/deactivation
 * - HYBRID SYSTEM: Supports stat modification, damage event, or both (set in ApplicationMode).
 * 
 * Perfect for: Equipment bonuses, permanent curses/blessings, racial traits,
 *              class features, persistent world effects
 */
UCLASS(BlueprintType, meta=(DisplayName="Infinite Effect Config"))
class NOMADDEV_API UNomadInfiniteEffectConfig : public UNomadStatusEffectConfigBase
{
    GENERATED_BODY()

public:
    UNomadInfiniteEffectConfig();

    // ======== Infinite Effect Settings ========

    /** 
     * Should this effect tick periodically? 
     * Use for ongoing effects like regeneration, damage over time, etc.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Infinite Duration Settings")
    bool bHasPeriodicTick = false;

    /** 
     * How often should periodic ticks occur (in seconds)?
     * Only relevant if bHasPeriodicTick is true.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Infinite Duration Settings", 
              meta=(ClampMin="0.1", EditCondition="bHasPeriodicTick"))
    float TickInterval = 5.0f;

    // ======== Removal Control ========

    /** 
     * Can this effect be manually removed by scripts/events?
     * Set to false for permanent effects like racial traits.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Removal Control")
    bool bCanBeManuallyRemoved = true;

    /** 
     * Should this effect persist through save/load cycles?
     * Set to false for temporary effects like equipment bonuses.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Removal Control")
    bool bPersistThroughSaveLoad = true;

    /** 
     * Gameplay tags that can bypass removal restrictions.
     * Effects/abilities with these tags can always remove this effect.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Removal Control")
    FGameplayTagContainer BypassRemovalTags;

    // ======== Persistent Modifiers ========

    /** 
     * Attribute set modifier applied for the entire duration of the effect.
     * This provides persistent stat bonuses/penalties.
     * ✅ CORRECTED: Using FAttributesSetModifier (with 's')
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Persistent Modifiers", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent", EditConditionHides))
    FAttributesSetModifier PersistentAttributeModifier;

    // ======== Stat Modifications & Damage Hybrid ========

    /** 
     * Stat modifications applied when the effect is first activated.
     * Use for one-time effects when the infinite effect starts.
     * If ApplicationMode is StatModification, applies as stat mods.
     * If ApplicationMode is DamageEvent, applies as damage (using DamageTypeClass from base).
     * If ApplicationMode is Both, applies both.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stat Modifications", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent", EditConditionHides))
    TArray<FStatisticValue> OnActivationStatModifications;

    /** 
     * Stat modifications applied on each periodic tick.
     * Only used if bHasPeriodicTick is true.
     * If ApplicationMode is StatModification, applies as stat mods.
     * If ApplicationMode is DamageEvent, applies as damage (using DamageTypeClass from base).
     * If ApplicationMode is Both, applies both.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stat Modifications", meta=(
        EditCondition="bHasPeriodicTick && ApplicationMode != EStatusEffectApplicationMode::DamageEvent", EditConditionHides))
    TArray<FStatisticValue> OnTickStatModifications;

    /** 
     * Stat modifications applied when the effect is deactivated/removed.
     * Use for cleanup or final effects when the infinite effect ends.
     * If ApplicationMode is StatModification, applies as stat mods.
     * If ApplicationMode is DamageEvent, applies as damage (using DamageTypeClass from base).
     * If ApplicationMode is Both, applies both.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stat Modifications", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent", EditConditionHides))
    TArray<FStatisticValue> OnDeactivationStatModifications;
    
    // ======== Chain Effects ========

    /** 
     * Should activation trigger additional chain effects?
     * Useful for complex effects that need multiple components.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects")
    bool bTriggerActivationChainEffects = false;

    /** 
     * Effects to trigger when this effect is activated.
     * These run alongside this effect, not instead of it.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects",
              meta=(EditCondition="bTriggerActivationChainEffects"))
    TArray<TSoftClassPtr<UNomadBaseStatusEffect>> ActivationChainEffects;

    /** 
     * Should deactivation trigger additional chain effects?
     * Useful for effects that need to trigger other effects when removed.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects")
    bool bTriggerDeactivationChainEffects = false;

    /** 
     * Effects to trigger when this effect is deactivated/removed.
     * These run after this effect ends.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects",
              meta=(EditCondition="bTriggerDeactivationChainEffects"))
    TArray<TSoftClassPtr<UNomadBaseStatusEffect>> DeactivationChainEffects;

    // ======== UI/Display Settings ========

    /** 
     * Should this effect show an infinity symbol or timer in UI?
     * True = infinity symbol, False = shows uptime
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI/Display")
    bool bShowInfinitySymbolInUI = true;

    /** 
     * Should periodic ticks be displayed to the player?
     * Set to false for subtle background effects.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI/Display",
              meta=(EditCondition="bHasPeriodicTick"))
    bool bShowTickNotifications = false;

    /** 
     * Priority for UI display when multiple infinite effects are active.
     * Higher values are displayed more prominently.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI/Display", meta=(ClampMin="0", ClampMax="100"))
    int32 DisplayPriority = 50;

    // ======== Validation & Info ========

    /** 
     * Developer notes about this effect's intended use and behavior.
     * Visible in editor for documentation purposes.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Documentation", meta=(MultiLine=true))
    FString DeveloperNotes;

public:
    // ======== Validation ========

    /** Validate this configuration for correctness */
    virtual bool IsConfigValid() const override;

    /** Get validation error messages */
    virtual TArray<FString> GetValidationErrors() const;

    /** Get a description of this effect for designers */
    UFUNCTION(BlueprintPure, Category="Configuration")
    FString GetEffectDescription() const;

    /** Check if this effect can be removed by a specific tag */
    UFUNCTION(BlueprintPure, Category="Configuration")
    bool CanBeRemovedByTag(const FGameplayTag& RemovalTag) const;

    /** Get the total number of stat modifications across all phases */
    UFUNCTION(BlueprintPure, Category="Configuration")
    int32 GetTotalStatModificationCount() const;

    /** Returns a type description for asset browsers, tooltips, etc. */
    virtual FText GetEffectTypeDescription() const override
    {
        return FText::FromString(TEXT("Infinite Effect"));
    }

protected:
    // ======== Editor Validation ========

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    
    // ✅ NEW API: Use FDataValidationContext instead of TArray<FText>
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};