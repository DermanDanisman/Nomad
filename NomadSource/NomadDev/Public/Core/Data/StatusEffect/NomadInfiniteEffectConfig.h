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
 * Configuration for infinite duration status effects.
 *
 * Key Features:
 * - Persistent until manually removed
 * - Optional periodic ticking
 * - Persistent attribute modifiers
 * - Stat modifications on activation/tick/deactivation
 * - Manual removal permission system
 * - Save/load persistence control
 * - Chain effect support
 * - Full hybrid system integration
 *
 * Use Cases:
 * - Equipment bonuses
 * - Permanent curses/blessings
 * - Racial traits
 * - Class features
 * - Environmental effects
 * - Character states
 */
UCLASS(BlueprintType, meta=(DisplayName="Infinite Effect Config"))
class NOMADDEV_API UNomadInfiniteEffectConfig : public UNomadStatusEffectConfigBase
{
    GENERATED_BODY()

public:
    UNomadInfiniteEffectConfig();

    // =====================================================
    //         INFINITE EFFECT SETTINGS
    // =====================================================

    /** Should this effect tick periodically? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Infinite Duration", meta=(
        ToolTip="Enable for ongoing effects like regeneration or damage over time"))
    bool bHasPeriodicTick = false;

    /** How often should periodic ticks occur (seconds)? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Infinite Duration", meta=(
        ClampMin="0.1", EditCondition="bHasPeriodicTick",
        ToolTip="Interval between ticks in seconds"))
    float TickInterval = 5.0f;

    // =====================================================
    //         REMOVAL CONTROL
    // =====================================================

    /** Can this effect be manually removed? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Removal Control", meta=(
        ToolTip="Allow removal by scripts, items, or abilities"))
    bool bCanBeManuallyRemoved = true;

    /** Should this effect persist through save/load? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Removal Control", meta=(
        ToolTip="Whether effect survives game save/load cycles"))
    bool bPersistThroughSaveLoad = true;

    /** Tags that can bypass removal restrictions */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Removal Control", meta=(
        ToolTip="Effects/abilities with these tags can always remove this effect"))
    FGameplayTagContainer BypassRemovalTags;

    // =====================================================
    //         PERSISTENT MODIFIERS
    // =====================================================

    /** Persistent attribute modifiers applied for the effect's lifetime */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Persistent Modifiers", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent",
        EditConditionHides, ToolTip="Attribute bonuses/penalties that last until effect removal"))
    FAttributesSetModifier PersistentAttributeModifier;

    // =====================================================
    //         STAT MODIFICATIONS (HYBRID SYSTEM)
    // =====================================================

    /** Stat modifications applied when effect is first activated */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stat Modifications", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent",
        EditConditionHides, ToolTip="One-time stat changes when effect starts"))
    TArray<FStatisticValue> OnActivationStatModifications;

    /** Stat modifications applied on each periodic tick */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stat Modifications", meta=(
        EditCondition="bHasPeriodicTick && ApplicationMode != EStatusEffectApplicationMode::DamageEvent",
        EditConditionHides, ToolTip="Recurring stat changes on each tick"))
    TArray<FStatisticValue> OnTickStatModifications;

    /** Stat modifications applied when effect is deactivated */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stat Modifications", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent",
        EditConditionHides, ToolTip="Final stat changes when effect ends"))
    TArray<FStatisticValue> OnDeactivationStatModifications;

    // =====================================================
    //         CHAIN EFFECTS
    // =====================================================

    /** Should activation trigger additional effects? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects", meta=(
        ToolTip="Trigger other effects when this one activates"))
    bool bTriggerActivationChainEffects = false;

    /** Effects to trigger on activation */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects", meta=(
        EditCondition="bTriggerActivationChainEffects",
        ToolTip="Additional effects applied alongside this one"))
    TArray<TSoftClassPtr<UNomadBaseStatusEffect>> ActivationChainEffects;

    /** Should deactivation trigger additional effects? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects", meta=(
        ToolTip="Trigger other effects when this one ends"))
    bool bTriggerDeactivationChainEffects = false;

    /** Effects to trigger on deactivation */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects", meta=(
        EditCondition="bTriggerDeactivationChainEffects",
        ToolTip="Effects applied when this effect ends"))
    TArray<TSoftClassPtr<UNomadBaseStatusEffect>> DeactivationChainEffects;

    // =====================================================
    //         UI/DISPLAY SETTINGS
    // =====================================================

    /** Show infinity symbol instead of timer in UI? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI/Display", meta=(
        ToolTip="Display ∞ symbol instead of uptime counter"))
    bool bShowInfinitySymbolInUI = true;

    /** Should periodic ticks show notifications? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI/Display", meta=(
        EditCondition="bHasPeriodicTick",
        ToolTip="Show UI notifications for each tick"))
    bool bShowTickNotifications = false;

    /** Priority for UI display ordering */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI/Display", meta=(
        ClampMin="0", ClampMax="100",
        ToolTip="Higher values appear more prominently in UI"))
    int32 DisplayPriority = 50;

    // =====================================================
    //         DOCUMENTATION
    // =====================================================

    /** Developer notes for documentation */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Documentation", meta=(
        MultiLine=true, ToolTip="Design notes and usage information"))
    FString DeveloperNotes;

public:
    // =====================================================
    //         VALIDATION & INFO
    // =====================================================

    /** Validate configuration */
    virtual bool IsConfigValid() const override;

    /** Get validation errors */
    virtual TArray<FString> GetValidationErrors() const override;

    /** Get designer-friendly description */
    UFUNCTION(BlueprintPure, Category="Configuration")
    FString GetEffectDescription() const;

    /** Check if removable by specific tag */
    UFUNCTION(BlueprintPure, Category="Configuration")
    bool CanBeRemovedByTag(const FGameplayTag& RemovalTag) const;

    /** Get total stat modification count */
    UFUNCTION(BlueprintPure, Category="Configuration")
    int32 GetTotalStatModificationCount() const;

    /** Returns effect type description */
    virtual FText GetEffectTypeDescription() const override
    {
        return FText::FromString(TEXT("Infinite Effect"));
    }

protected:
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};