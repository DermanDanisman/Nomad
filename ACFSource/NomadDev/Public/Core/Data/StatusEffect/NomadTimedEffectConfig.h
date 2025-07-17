// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/StatusEffect/NomadStatusEffectConfigBase.h"
#include "ARSTypes.h"
#include "NomadTimedEffectConfig.generated.h"

/**
 * Enum for how a timed effect's duration is defined.
 */
UENUM(BlueprintType)
enum class EEffectDurationMode : uint8
{
    Duration UMETA(DisplayName="By Duration"),
    Ticks UMETA(DisplayName="By Tick Count")
};

/**
 * UNomadTimedEffectConfig
 * -----------------------
 * Configuration DataAsset for all timer-based (finite duration or periodic) status effects.
 *
 * Key Features:
 * - Data-driven: All timing, ticking, and stat modification logic is defined per asset.
 * - Flexible duration control: Effects can expire by time or by tick count.
 * - Supports both one-off and periodic effects (DoT, HoT, buffs, debuffs, etc.).
 * - Attribute set modifiers and stat modifications supported at start, each tick, and end.
 * - Editor-friendly: All options are available in Blueprints and the Unreal Editor.
 * - HYBRID SYSTEM: Supports stat modification, damage event, or both (set in ApplicationMode).
 *
 * Use for: Bleeds, poisons, burns, temporary shields, timed buffs, periodic heals, and any timed effect.
 */
UCLASS(BlueprintType, meta=(DisplayName="Timed Effect Config"))
class NOMADDEV_API UNomadTimedEffectConfig : public UNomadStatusEffectConfigBase
{
    GENERATED_BODY()

public:

    UNomadTimedEffectConfig();
    
    // --- Timing Options ---

    /** If true, this effect ticks periodically (e.g., DoT/HoT). If false, only OnStart/OnEnd logic runs. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing")
    bool bIsPeriodic = false;

    /** The interval between ticks in seconds (only used if bIsPeriodic). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(EditCondition="bIsPeriodic", ClampMin="0.01"))
    float TickInterval = 1.0f;

    /** Defines whether the effect duration is based on time or number of ticks (only used if bIsPeriodic). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(EditCondition="bIsPeriodic"))
    EEffectDurationMode DurationMode = EEffectDurationMode::Duration;

    /** Effect duration in seconds (only used if bIsPeriodic && DurationMode == Duration). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(EditCondition="bIsPeriodic && DurationMode==EEffectDurationMode::Duration", ClampMin="0.01"))
    float EffectDuration = 10.0f;

    /** Number of ticks (only used if bIsPeriodic && DurationMode == Ticks). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(EditCondition="bIsPeriodic && DurationMode==EEffectDurationMode::Ticks", ClampMin="1"))
    int32 NumTicks = 5;

    // --- Stat Modifications & Damage Hybrid ---

    /** Stat modifications to apply when the effect starts. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent", EditConditionHides))
    TArray<FStatisticValue> OnStartStatModifications;
    
    /** Stat modifications to apply on each tick (if periodic). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(
        EditCondition="bIsPeriodic && ApplicationMode != EStatusEffectApplicationMode::DamageEvent", EditConditionHides))
    TArray<FStatisticValue> OnTickStatModifications;


    /** Stat modifications to apply when the effect ends. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent", EditConditionHides))
    TArray<FStatisticValue> OnEndStatModifications;

    /** Persistent attribute/primary/stat modifiers applied for the effect's lifetime. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats", meta=(
        EditCondition="ApplicationMode != EStatusEffectApplicationMode::DamageEvent", EditConditionHides))
    FAttributesSetModifier AttributeModifier;

    // === Chain Effects (optional) ===

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects")
    bool bTriggerActivationChainEffects = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects", meta=(EditCondition="bTriggerActivationChainEffects"))
    TArray<TSoftClassPtr<UNomadBaseStatusEffect>> ActivationChainEffects;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects")
    bool bTriggerDeactivationChainEffects = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chain Effects", meta=(EditCondition="bTriggerDeactivationChainEffects"))
    TArray<TSoftClassPtr<UNomadBaseStatusEffect>> DeactivationChainEffects;

    // --- HYBRID SYSTEM: Application Mode and DamageTypeClass are inherited from UNomadStatusEffectConfigBase ---

    /** Returns a type description for asset browsers, tooltips, etc. */
    virtual FText GetEffectTypeDescription() const override
    {
        return FText::FromString(TEXT("Timed Effect"));
    }

#if WITH_EDITOR
    /** Editor-side validation of config properties. */
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};