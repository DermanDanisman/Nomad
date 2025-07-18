// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/StatusEffect/NomadInfiniteStatusEffect.h"
#include "GameplayTagContainer.h"
#include "Core/Component/NomadSurvivalNeedsComponent.h"
#include "NomadSurvivalStatusEffect.generated.h"

/**
 * UNomadSurvivalStatusEffect
 * --------------------------
 * Base class for all survival-related status effects (starvation, dehydration, temperature hazards).
 *
 * Key Features:
 * - Inherits from UNomadInfiniteStatusEffect for persistent effects until conditions improve
 * - Automatically applies attribute modifiers from config (movement speed, stamina cap reductions)
 * - Supports damage over time based on survival requirements
 * - Handles visual effects (screen tints, particles) for different severity levels
 * - Integrates with the survival component for condition-based application/removal
 *
 * Design Philosophy:
 * - All gameplay values are data-driven via UNomadInfiniteEffectConfig
 * - No hard-coded multipliers or thresholds
 * - Severity levels determine which config asset to use
 * - Automatic cleanup when survival conditions improve
 */
UCLASS(BlueprintType, Abstract)
class NOMADDEV_API UNomadSurvivalStatusEffect : public UNomadInfiniteStatusEffect
{
    GENERATED_BODY()

public:
    /** Constructor - sets up default values for survival effects */
    UNomadSurvivalStatusEffect();

    /**
     * Sets the severity level for this survival effect instance.
     * This determines which attribute modifiers and visual effects are applied.
     * Called by the survival component when applying effects based on stat thresholds.
     */
    UFUNCTION(BlueprintCallable, Category="Survival Effect")
    void SetSeverityLevel(ESurvivalSeverity InSeverity);

    /**
     * Gets the current severity level of this effect.
     * Used by UI and other systems to determine appropriate feedback.
     */
    UFUNCTION(BlueprintPure, Category="Survival Effect")
    ESurvivalSeverity GetSeverityLevel() const { return CurrentSeverity; }

    /**
     * Sets the damage over time percentage for this effect.
     * Called by survival component based on config values.
     * @param InDoTPercent Percentage of max health to damage per tick (e.g., 0.005 = 0.5% per tick)
     */
    UFUNCTION(BlueprintCallable, Category="Survival Effect")
    void SetDoTPercent(float InDoTPercent);

protected:
    /** Current severity level of this survival effect */
    UPROPERTY(BlueprintReadOnly, Category="Survival Effect")
    ESurvivalSeverity CurrentSeverity = ESurvivalSeverity::None;

    /** Damage over time percentage (of max health) applied per tick */
    UPROPERTY(BlueprintReadOnly, Category="Survival Effect")
    float DoTPercent = 0.0f;

    /**
     * Called when the survival effect starts.
     * Applies attribute modifiers and visual effects based on severity.
     */
    virtual void OnStatusEffectStarts_Implementation(ACharacter* Character) override;

    /**
     * Called when the survival effect ends.
     * Removes all modifiers and visual effects.
     */
    virtual void OnStatusEffectEnds_Implementation() override;

    /**
     * Called on each periodic tick (every 5 seconds by default).
     * Applies damage over time if DoTPercent is set.
     */
    virtual void HandleInfiniteTick() override;

    /**
     * Applies visual effects appropriate for the current severity level.
     * Override in subclasses for effect-specific visuals (screen tints, particles, etc.).
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Survival Effect|Visual")
    void ApplyVisualEffects();

    /**
     * Removes all visual effects when the condition improves or ends.
     * Override in subclasses to clean up effect-specific visuals.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Survival Effect|Visual")
    void RemoveVisualEffects();

private:
    /** Tracks last damage dealt for analytics and UI feedback */
    float LastDamageDealt = 0.0f;
};

/**
 * UNomadStarvationStatusEffect
 * ----------------------------
 * Status effect for hunger-related penalties.
 *
 * Applied when hunger drops below threshold levels.
 * Causes stamina cap reduction, movement speed penalties, and health damage over time.
 *
 * Requirements Implementation:
 * - Stamina Cap Decrease (via attribute modifiers)
 * - %0 Damage Over Time % 0.5 per second (via DoT system)
 * - Movement speed reduction (via attribute modifiers)
 * - Visual: Pulsating Desaturation / Stomach Growl (via Blueprint events)
 * - Duration: Permanent until hunger improves
 */
UCLASS(BlueprintType)
class NOMADDEV_API UNomadStarvationStatusEffect : public UNomadSurvivalStatusEffect
{
    GENERATED_BODY()

public:
    UNomadStarvationStatusEffect()
    {
        // Set gameplay tag for this specific survival effect
        // This tag is used by the status effect manager for identification and removal
        SetStatusEffectTag(FGameplayTag::RequestGameplayTag("StatusEffect.Survival.Starvation"));
    }

protected:
    /**
     * Applies starvation-specific visual effects.
     * Should implement pulsating desaturation and stomach growl audio.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Starvation Effect")
    void ApplyStarvationVisuals();

    /**
     * Removes starvation-specific visual effects.
     * Should stop desaturation and audio effects.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Starvation Effect")
    void RemoveStarvationVisuals();
};

/**
 * UNomadDehydrationStatusEffect
 * -----------------------------
 * Status effect for thirst-related penalties.
 *
 * Applied when thirst drops below threshold levels.
 * Causes stamina cap reduction, movement speed penalties, and health damage over time.
 *
 * Requirements Implementation:
 * - Stamina Cap Decrease (via attribute modifiers)
 * - %0 Damage Over Time % 1 per second (via DoT system)
 * - Movement speed reduction (via attribute modifiers)
 * - Visual: Faint B&W Filter, Subtle Crack Pattern overlay, heavy breathing (via Blueprint events)
 * - Duration: Permanent until thirst improves
 */
UCLASS(BlueprintType)
class NOMADDEV_API UNomadDehydrationStatusEffect : public UNomadSurvivalStatusEffect
{
    GENERATED_BODY()

public:
    UNomadDehydrationStatusEffect()
    {
        // Set gameplay tag for this specific survival effect
        SetStatusEffectTag(FGameplayTag::RequestGameplayTag("StatusEffect.Survival.Dehydration"));
    }

protected:
    /**
     * Applies dehydration-specific visual effects.
     * Should implement B&W filter, crack pattern overlay, and breathing audio.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Dehydration Effect")
    void ApplyDehydrationVisuals();

    /**
     * Removes dehydration-specific visual effects.
     * Should stop all dehydration visual and audio effects.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Dehydration Effect")
    void RemoveDehydrationVisuals();
};

/**
 * UNomadHeatstrokeStatusEffect
 * ----------------------------
 * Status effect for heat-related penalties.
 *
 * Applied when body temperature exceeds safe thresholds.
 * Has multiple severity levels (Mild/Heavy/Extreme) with escalating penalties.
 *
 * Requirements Implementation:
 * - Thirst consumption multiplier (X2, X3, X4 based on severity)
 * - Movement speed reduction (%10, %20, %30 based on severity)
 * - Visual: Slight Mirage effect on screen corners, Subtle orange red character tint
 * - Weather dependency: 7-step state from extreme cold to optimal to extreme heat
 */
UCLASS(BlueprintType)
class NOMADDEV_API UNomadHeatstrokeStatusEffect : public UNomadSurvivalStatusEffect
{
    GENERATED_BODY()

public:
    UNomadHeatstrokeStatusEffect()
    {
        // Set gameplay tag for this specific survival effect
        SetStatusEffectTag(FGameplayTag::RequestGameplayTag("StatusEffect.Survival.Heatstroke"));
    }

protected:
    /**
     * Applies heatstroke-specific visual effects.
     * Should implement mirage effect and orange/red character tint based on severity.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Heatstroke Effect")
    void ApplyHeatstrokeVisuals();

    /**
     * Removes heatstroke-specific visual effects.
     * Should stop mirage and tinting effects.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Heatstroke Effect")
    void RemoveHeatstrokeVisuals();
};

/**
 * UNomadHypothermiaStatusEffect
 * -----------------------------
 * Status effect for cold-related penalties.
 *
 * Applied when body temperature drops below safe thresholds.
 * Has multiple severity levels (Mild/Heavy/Extreme) with escalating penalties.
 *
 * Requirements Implementation:
 * - Hunger consumption multiplier (X2, X3, X4 based on severity)
 * - Movement speed reduction (%10, %20, %30 based on severity)
 * - Visual: Slight Frost Screen corners, faint blue tone character
 * - Weather dependency: 7-step state from extreme cold to optimal to extreme heat
 */
UCLASS(BlueprintType)
class NOMADDEV_API UNomadHypothermiaStatusEffect : public UNomadSurvivalStatusEffect
{
    GENERATED_BODY()

public:
    UNomadHypothermiaStatusEffect()
    {
        // Set gameplay tag for this specific survival effect
        SetStatusEffectTag(FGameplayTag::RequestGameplayTag("StatusEffect.Survival.Hypothermia"));
    }

protected:
    /**
     * Applies hypothermia-specific visual effects.
     * Should implement frost screen corners and blue character tint based on severity.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Hypothermia Effect")
    void ApplyHypothermiaVisuals();

    /**
     * Removes hypothermia-specific visual effects.
     * Should stop frost and blue tinting effects.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Hypothermia Effect")
    void RemoveHypothermiaVisuals();
};