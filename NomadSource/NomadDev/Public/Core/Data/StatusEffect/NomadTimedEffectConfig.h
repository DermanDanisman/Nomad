// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/StatusEffect/NomadStatusEffectConfigBase.h"
#include "ARSTypes.h"
#include "NomadTimedEffectConfig.generated.h"

class UNomadBaseStatusEffect;

/**
 * EEffectDurationMode
 * -------------------
 * Determines how timed effect duration is calculated.
 */
UENUM(BlueprintType)
enum class EEffectDurationMode : uint8
{
    Duration    UMETA(DisplayName="By Duration"),      // Effect lasts for X seconds
    Ticks       UMETA(DisplayName="By Tick Count")     // Effect lasts for X ticks
};

/**
 * UNomadTimedEffectConfig
 * -----------------------
 * Configuration for timer-based (finite duration) status effects.
 *
 * Key Features:
 * - Flexible duration control (time or tick count)
 * - Optional periodic ticking
 * - Stat modifications at start/tick/end
 * - Persistent attribute modifiers
 * - Chain effect support
 * - Full hybrid system integration
 * - Stacking support
 *
 * Use Cases:
 * - Damage over time (poison, burning)
 * - Healing over time (regeneration)
 * - Temporary buffs/debuffs
 * - Status conditions (slow, haste)
 * - Environmental effects
 */
UCLASS(BlueprintType, meta=(DisplayName="Timed Effect Config"))
class NOMADDEV_API UNomadTimedEffectConfig : public UNomadStatusEffectConfigBase
{
    GENERATED_BODY()

public:
    UNomadTimedEffectConfig();

    // =====================================================
    //         TIMING CONFIGURATION
    // =====================================================

    /** Should this effect tick periodically? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(
        ToolTip="Enable for DoT/HoT effects that apply changes over time"))
    bool bIsPeriodic = false;

    /** Interval between ticks (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(
        EditCondition="bIsPeriodic", ClampMin="0.01",
        ToolTip="Time between periodic applications"))
    float TickInterval = 1.0f;

    /** How is effect duration determined? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(
        EditCondition="bIsPeriodic",
        ToolTip="Whether effect ends after time or number of ticks"))
    EEffectDurationMode DurationMode = EEffectDurationMode::Duration;

    /** Effect duration in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(
        EditCondition="bIsPeriodic && DurationMode==EEffectDurationMode::Duration",
        ClampMin="0.01", ToolTip="How long the effect lasts"))
    float EffectDuration = 10.0f;

    /** Number of ticks before effect ends */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(
        EditCondition="bIsPeriodic && DurationMode==EEffectDurationMode::Ticks",
        ClampMin="1", ToolTip="How many ticks before effect ends"))
    int32 NumTicks = 5;

    // =====================================================
    //         STAT MODIFICATIONS (HYBRID SYSTEM)
    // =====================================================

    /** Stat modifications applied when effect starts */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stat Modifications", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent",
        EditConditionHides, ToolTip="One-time changes when effect begins"))
    TArray<FStatisticValue> OnStartStatModifications;

    /** Stat modifications applied on each tick */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stat Modifications", meta=(
        EditCondition="bIsPeriodic && ApplicationMode != EStatusEffectApplicationMode::DamageEvent",
        EditConditionHides, ToolTip="Recurring changes on each tick"))
    TArray<FStatisticValue> OnTickStatModifications;

    /** Stat modifications applied when effect ends */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stat Modifications", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent",
        EditConditionHides, ToolTip="Final changes when effect expires"))
    TArray<FStatisticValue> OnEndStatModifications;

    /** Persistent attribute modifiers for effect lifetime */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stat Modifications", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent",
        EditConditionHides, ToolTip="Continuous bonuses/penalties while active"))
    FAttributesSetModifier AttributeModifier;

    // =====================================================
    //         CHAIN EFFECTS
    // =====================================================

    /** Should activation trigger additional effects? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects", meta=(
        ToolTip="Trigger other effects when this one starts"))
    bool bTriggerActivationChainEffects = false;

    /** Effects to trigger on activation */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects", meta=(
        EditCondition="bTriggerActivationChainEffects",
        ToolTip="Additional effects applied when this starts"))
    TArray<TSoftClassPtr<UNomadBaseStatusEffect>> ActivationChainEffects;

    /** Should deactivation trigger additional effects? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects", meta=(
        ToolTip="Trigger other effects when this one ends"))
    bool bTriggerDeactivationChainEffects = false;

    /** Effects to trigger on deactivation */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects", meta=(
        EditCondition="bTriggerDeactivationChainEffects",
        ToolTip="Additional effects applied when this ends"))
    TArray<TSoftClassPtr<UNomadBaseStatusEffect>> DeactivationChainEffects;

    // =====================================================
    //         ADVANCED SETTINGS
    // =====================================================

    /** Should effect pause during certain conditions? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Advanced", meta=(
        ToolTip="Effect timer pauses when character has certain states"))
    bool bCanBePaused = false;

    /** Tags that cause this effect to pause */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Advanced", meta=(
        EditCondition="bCanBePaused",
        ToolTip="Effect pauses when character has any of these tags"))
    FGameplayTagContainer PauseTags;

    /** Should stacking refresh duration? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Advanced", meta=(
        EditCondition="bCanStack",
        ToolTip="Whether adding stacks resets the timer"))
    bool bStackingRefreshesDuration = true;

public:
    // =====================================================
    //         VALIDATION & INFO
    // =====================================================

    /** Validate configuration */
    virtual bool IsConfigValid() const override;

    /** Get validation errors */
    virtual TArray<FString> GetValidationErrors() const override;

    /** Get total effect duration in seconds */
    UFUNCTION(BlueprintPure, Category="Configuration")
    float GetTotalDuration() const;

    /** Get total number of ticks that will occur */
    UFUNCTION(BlueprintPure, Category="Configuration")
    int32 GetTotalTickCount() const;

    /** Returns effect type description */
    virtual FText GetEffectTypeDescription() const override
    {
        return FText::FromString(TEXT("Timed Effect"));
    }

protected:
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};