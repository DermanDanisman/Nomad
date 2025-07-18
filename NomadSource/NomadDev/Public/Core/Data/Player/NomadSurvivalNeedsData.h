// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "NomadSurvivalNeedsData.generated.h"

// Forward declarations
class UNomadBaseStatusEffect;
class UACFBaseStatusEffect;
class UNomadSurvivalStatusEffect;

/**
 * Struct: FAdvancedSurvivalTempParams
 * -----------------------------------
 * Advanced parameters for body temperature simulation.
 * Only change these for fine-tuning body temp realism.
 */
USTRUCT(BlueprintType)
struct FAdvancedSurvivalTempParams
{
    GENERATED_BODY()

    /** 
     * The minimum rate (°C/min) at which body temperature can change.
     * Prevents body temperature from "stalling" if the difference from ambient is very small.
     * Raise for more responsive simulation, lower for more gradual.
     */
    UPROPERTY(EditAnywhere, Category="Advanced", meta=(AdvancedDisplay))
    float MinBodyTempChangeRate = 0.01f;

    /**
     * Main proportional rate (per minute) for body temp adjustment toward ambient temp.
     * Example: 0.0125 means if ambient is 10°C away from body, body temp changes by 0.125°C per minute.
     * Lower for slower/softer changes, higher for more dramatic/arcadey effect.
     */
    UPROPERTY(EditAnywhere, Category="Advanced", meta=(AdvancedDisplay))
    float BodyTempAdjustRate = 0.0125f;

    /**
     * The maximum rate (°C/min) at which body temperature is allowed to change.
     * Prevents unrealistic "jumps" in body temp in extreme environments.
     * Raise for more lethality, lower for more realism.
     */
    UPROPERTY(EditAnywhere, Category="Advanced", meta=(AdvancedDisplay))
    float MaxBodyTempChangeRate = 0.05f;

    /**
     * Curve for body temperature drift as a function of normalized distance from safe zone.
     * X = [0..1] normalized distance, Y = multiplier for drift rate.
     */
    UPROPERTY(EditDefaultsOnly, Category="Advanced|BodyTemp")
    TObjectPtr<UCurveFloat> BodyTempDriftCurve = nullptr;
};

/**
 * Struct: FCurvesForAdvancedModifierTuning
 * ----------------------------------------
 * Centralized collection of all designer-tunable curves for advanced, non-linear survival effect tuning.
 * Use these to control how various gameplay modifiers (hunger/thirst decay, activity impact, temperature drift, etc.)
 * respond to normalized values. Each curve can be set in the editor for fine-grained balancing and unique gameplay feel.
 * 
 * - All curves are optional; fallback logic will be used if a curve is unset.
 * - See tooltips on each property for usage guidelines.
 */

USTRUCT(BlueprintType)
struct FCurvesForAdvancedModifierTuning
{
    GENERATED_BODY()

    // =========================
    // [Curves for Advanced Modifier Tuning]
    // =========================

    /** 
     * Curve controlling how hunger decay scales with normalized temperature (0=coldest, 1=warmest).
     * X: Normalized temperature, Y: Multiplier for hunger decay.
     * Allows designers to tune non-linear effects of temperature on hunger.
     */
    UPROPERTY(EditDefaultsOnly, Category="Survival|Curves")
    TObjectPtr<UCurveFloat> HungerDecayByTemperatureCurve;

    /** 
     * Curve controlling how thirst decay scales with normalized temperature (0=coldest, 1=warmest).
     * X: Normalized temperature, Y: Multiplier for thirst decay.
     * Allows designers to tune non-linear effects of temperature on thirst.
     */
    UPROPERTY(EditDefaultsOnly, Category="Survival|Curves")
    TObjectPtr<UCurveFloat> ThirstDecayByTemperatureCurve;

    /** 
     * Curve controlling how hunger decay scales with normalized activity (0=idle, 1=sprinting).
     * X: Normalized activity, Y: Multiplier for hunger decay.
     * Allows designers to tune non-linear effects of activity on hunger.
     */
    UPROPERTY(EditDefaultsOnly, Category="Survival|Curves")
    TObjectPtr<UCurveFloat> HungerDecayByActivityCurve;

    /** 
     * Curve controlling how thirst decay scales with normalized activity (0=idle, 1=sprinting).
     * X: Normalized activity, Y: Multiplier for thirst decay.
     * Allows designers to tune non-linear effects of activity on thirst.
     */
    UPROPERTY(EditDefaultsOnly, Category="Survival|Curves")
    TObjectPtr<UCurveFloat> ThirstDecayByActivityCurve;
};

/**
 * UNomadSurvivalNeedsData
 * -----------------------
 * Data Asset for all survival gameplay tuning parameters.
 * Designers edit these in editor; assign to character/component.
 * All variables are categorized and documented for clarity.
 * 
 * UPDATED (2025-07-17): Added new survival status effect class references for data-driven system
 */
UCLASS(BlueprintType)
class NOMADDEV_API UNomadSurvivalNeedsData : public UDataAsset {
    GENERATED_BODY()

public:
    // =========================
    // [Decay Rates]
    // =========================

    /** 
     * How much hunger (stat units) the character loses in 24 in-game hours if idle in normal weather.
     * Divided by 1440 (minutes/day) for per-minute decay.
     * Raise for more survival challenge, lower for longer play sessions.
     */
    UPROPERTY(EditAnywhere, Category="Decay|Base", meta=(ClampMin="0"))
    float DailyHungerLoss = 50.f;

    /** 
     * How much thirst (stat units) the character loses in 24 in-game hours if idle in normal weather.
     * Divided by 1440 for per-minute decay.
     * Raise for more challenge, lower for longer play sessions.
     */
    UPROPERTY(EditAnywhere, Category="Decay|Base", meta=(ClampMin="0"))
    float DailyThirstLoss = 80.f;

    /** 
     * Multiplies all decay rates for accelerated testing.
     * Set to 60 for "one day per hour", 1 for normal.
     * Only use for debug/testing, not in shipped builds.
     */
    UPROPERTY(EditAnywhere, Category="Decay|Debug", meta=(ClampMin="1"))
    float DebugDecayMultiplier = 1.f;

    // =========================
    // [Activity Modifiers]
    // =========================

    /** 
     * Speed (cm/s) below which movement counts as walking.
     * Adjust to match your character's walk speed.
     */
    UPROPERTY(EditAnywhere, Category="Activity|Tiers", meta=(ClampMin="0"))
    float WalkingSpeedThreshold = 300.f;

    /** 
     * Speed (cm/s) below which movement counts as running.
     * Adjust to match your character's run speed.
     */
    UPROPERTY(EditAnywhere, Category="Activity|Tiers", meta=(ClampMin="0"))
    float RunningSpeedThreshold = 600.f;

    /** 
     * Speed (cm/s) above which movement counts as sprinting.
     * Adjust to match your character's sprint speed.
     */
    UPROPERTY(EditAnywhere, Category="Activity|Tiers", meta=(ClampMin="0"))
    float SprintingSpeedThreshold = 900.f;

    // =========================
    // [Attribute Modifiers]
    // =========================

    /** 
     * Percentage reduction in decay per Endurance attribute point.
     * 0.01 = -1% per point. Raise for more impact of Endurance.
     */
    UPROPERTY(EditAnywhere, Category="Attributes", meta=(ClampMin="0"))
    float EnduranceDecayPerPoint = 0.01f;

    // =========================
    // [Environmental Safe Zone (ambient temp where body temp stays stable)]
    // =========================

    /**
     * Minimum ambient (external) temperature in Celsius at which the player's body temperature is considered stable.
     * If the ambient temperature is equal to or above this value (and below SafeAmbientTempMaxC), body temperature will drift toward the normal value.
     * Outside this range, body temperature will start to drift more rapidly toward the ambient temperature.
     * Example: 15.0 = The "lower bound" of a comfortable climate for humans.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Survival|Temperature")
    float SafeAmbientTempMinC = 15.f;

    /**
     * Maximum ambient (external) temperature in Celsius at which the player's body temperature is considered stable.
     * If the ambient temperature is equal to or below this value (and above SafeAmbientTempMinC), body temperature will drift toward the normal value.
     * Outside this range, body temperature will start to drift more rapidly toward the ambient temperature.
     * Example: 25.0 = The "upper bound" of a comfortable climate for humans.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Survival|Temperature")
    float SafeAmbientTempMaxC = 25.f;

    // --- Body Temperature Safe Zone (no hazards/effects) ---

    /**
     * Minimum core body temperature in Celsius at which no hazard effects (like hypothermia) occur.
     * If player's body temperature drops below this value, hypothermia risk increases and hazard effects may be applied.
     * Example: 36.0 = Lower bound of healthy human body temperature.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Survival|Temperature")
    float SafeBodyTempMinC = 35.5f;

    /**
     * Maximum core body temperature in Celsius at which no hazard effects (like heatstroke) occur.
     * If player's body temperature rises above this value, heatstroke risk increases and hazard effects may be applied.
     * Example: 37.5 = Upper bound of healthy human body temperature.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Survival|Temperature")
    float SafeBodyTempMaxC = 37.5f;

    // =========================
    // [Temperature Ranges & Normalization]
    // =========================

    /**
     * Minimum/maximum expected external temperature for your game world (Celsius).
     * Controls weather normalization. Set for your climate.
     */
    UPROPERTY(EditAnywhere, Category="Temperature|Normalization")
    float MinExternalTempCelsius = -20.f;

    /**
     * Minimum/maximum expected external temperature for your game world (Celsius).
     * Controls weather normalization. Set for your climate.
     */
    UPROPERTY(EditAnywhere, Category="Temperature|Normalization")
    float MaxExternalTempCelsius = 40.f;

    /**
     * Minimum/maximum external temperature (Fahrenheit).
     * Use if your weather system is in °F.
     */
    UPROPERTY(EditAnywhere, Category="Temperature|Normalization")
    float MinExternalTempFahrenheit = -4.f;

    /**
     * Minimum/maximum external temperature (Fahrenheit).
     * Use if your weather system is in °F.
     */
    UPROPERTY(EditAnywhere, Category="Temperature|Normalization")
    float MaxExternalTempFahrenheit = 104.f;

    /**
     * Multiplies the effect of temperature on needs decay after normalization.
     * 1 = default, >1 = more dramatic temperature effects.
     */
    UPROPERTY(EditAnywhere, Category="Temperature|Normalization")
    float ExternalTemperatureScale = 1.f;

    // =========================
    // [Body Temperature Simulation]
    // =========================

    /**
     * Normal/healthy human body temperature in °C.
     * Used as the baseline for body temperature simulation.
     */
    UPROPERTY(EditAnywhere, Category="BodyTemp")
    float NormalBodyTemperature = 36.f;

    /**
     * Advanced parameters for body temperature simulation.
     * Only adjust for advanced tuning!
     */
    UPROPERTY(EditAnywhere, Category="BodyTemp", meta=(ShowOnlyInnerProperties))
    FAdvancedSurvivalTempParams AdvancedBodyTempParams;

    // =========================
    // [Hazard Thresholds & Damage]
    // =========================

    /**
     * Body temperature above which heatstroke can occur (°C).
     * Lower for more risk, higher for more forgiving gameplay.
     */
    UPROPERTY(EditAnywhere, Category="Hazards|Thresholds")
    float HeatstrokeThreshold = 40.f;

    /**
     * Body temperature below which hypothermia can occur (°C).
     * Higher for more risk, lower for more forgiving gameplay.
     */
    UPROPERTY(EditAnywhere, Category="Hazards|Thresholds")
    float HypothermiaThreshold = 32.f;

    /**
     * Minutes of exposure above/below threshold before hazard triggers.
     * Raise for more forgiving gameplay, lower for more threat.
     */
    UPROPERTY(EditAnywhere, Category="Hazards|Thresholds", meta=(ClampMin="1"))
    int HeatstrokeDurationMinutes = 10;

    /**
     * Minutes of exposure above/below threshold before hazard triggers.
     * Raise for more forgiving gameplay, lower for more threat.
     */
    UPROPERTY(EditAnywhere, Category="Hazards|Thresholds", meta=(ClampMin="1"))
    int HypothermiaDurationMinutes = 10;

    // =========================
    // [Temperature Hazard Movement Slow - LEGACY (keeping for existing system)]
    // =========================

    /**
     * Heatstroke Mild threshold (°C): Body temp at/above this value triggers mild heatstroke slow.
     * Set just above normal human body temperature.
     */
    UPROPERTY(EditAnywhere, Category="Hazards|Heatstroke")
    float HeatstrokeMildThreshold = 38.0f;

    /**
     * Heatstroke Heavy threshold (°C): Body temp at/above this value triggers heavy heatstroke slow.
     */
    UPROPERTY(EditAnywhere, Category="Hazards|Heatstroke")
    float HeatstrokeHeavyThreshold = 39.0f;

    /**
     * Heatstroke Extreme threshold (°C): Body temp at/above this value triggers extreme heatstroke slow.
     */
    UPROPERTY(EditAnywhere, Category="Hazards|Heatstroke")
    float HeatstrokeExtremeThreshold = 40.0f;

    /**
     * Hypothermia Mild threshold (°C): Body temp at/below this value triggers mild hypothermia slow.
     */
    UPROPERTY(EditAnywhere, Category="Hazards|Hypothermia")
    float HypothermiaMildThreshold = 35.0f;

    /**
     * Hypothermia Heavy threshold (°C): Body temp at/below this value triggers heavy hypothermia slow.
     */
    UPROPERTY(EditAnywhere, Category="Hazards|Hypothermia")
    float HypothermiaHeavyThreshold = 34.0f;

    /**
     * Hypothermia Extreme threshold (°C): Body temp at/below this value triggers extreme hypothermia slow.
     */
    UPROPERTY(EditAnywhere, Category="Hazards|Hypothermia")
    float HypothermiaExtremeThreshold = 33.0f;

    // =========================
    // [Hunger/Thirst Side-Effects - LEGACY (keeping for existing system)]
    // =========================

    /**
     * Hunger at/below which movement slows and health may be lost.
     * Raise for more warning, lower for more risk.
     */
    UPROPERTY(EditAnywhere, Category="SideEffects|Thresholds", meta=(ClampMin="0"))
    float HungerSlowThreshold = 50.f;

    /**
     * Thirst at/below which movement slows and health may be lost.
     * Raise for more warning, lower for more risk.
     */
    UPROPERTY(EditAnywhere, Category="SideEffects|Thresholds", meta=(ClampMin="0"))
    float ThirstSlowThreshold = 50.f;

    // =========================
    // [Warning Thresholds]
    // =========================

    /**
     * Player is warned if hunger drops to/below this, but above zero.
     * Raise to give player more warning before starvation.
     */
    UPROPERTY(EditAnywhere, Category="Warnings")
    float StarvationWarningThreshold = 5.f;

    /**
     * Player is warned if thirst drops to/below this, but above zero.
     * Raise to give player more warning before dehydration.
     */
    UPROPERTY(EditAnywhere, Category="Warnings")
    float DehydrationWarningThreshold = 5.f;

    /**
     * Player is warned if body temp is within this many °C of heatstroke threshold.
     * Raise for earlier warnings.
     */
    UPROPERTY(EditAnywhere, Category="Warnings")
    float HeatstrokeWarningDelta = 0.5f;

    /**
     * Player is warned if body temp is within this many °C of hypothermia threshold.
     * Raise for earlier warnings.
     */
    UPROPERTY(EditAnywhere, Category="Warnings")
    float HypothermiaWarningDelta = 0.5f;

    // =========================
    // [Warning Event Cooldowns]
    // =========================

    /**
     * Minimum in-game minutes between consecutive starvation warnings.
     * Raise to reduce warning spam, lower for more granular (frequent) alerts.
     */
    UPROPERTY(EditAnywhere, Category="Warnings|Cooldowns", meta=(ClampMin="0"))
    float StarvationWarningCooldown = 5.f;

    /**
     * Minimum in-game minutes between consecutive dehydration warnings.
     * Raise to reduce warning spam, lower for more granular (frequent) alerts.
     */
    UPROPERTY(EditAnywhere, Category="Warnings|Cooldowns", meta=(ClampMin="0"))
    float DehydrationWarningCooldown = 5.f;

    /**
     * Minimum in-game minutes between consecutive heatstroke warnings.
     * Raise to reduce warning spam, lower for more granular (frequent) alerts.
     */
    UPROPERTY(EditAnywhere, Category="Warnings|Cooldowns", meta=(ClampMin="0"))
    float HeatstrokeWarningCooldown = 10.f;

    /**
     * Minimum in-game minutes between consecutive hypothermia warnings.
     * Raise to reduce warning spam, lower for more granular (frequent) alerts.
     */
    UPROPERTY(EditAnywhere, Category="Warnings|Cooldowns", meta=(ClampMin="0"))
    float HypothermiaWarningCooldown = 10.f;

    // =========================
    // [Gameplay Tags]
    // =========================

    /**
     * GameplayTag for hunger stat (required for ARS/attribute system).
     */
    UPROPERTY(EditAnywhere, Category="Tags")
    FGameplayTag HungerStatTag;

    /**
     * GameplayTag for thirst stat (required for ARS/attribute system).
     */
    UPROPERTY(EditAnywhere, Category="Tags")
    FGameplayTag ThirstStatTag;

    /**
     * GameplayTag for health stat (required for ARS/attribute system).
     */
    UPROPERTY(EditAnywhere, Category="Tags")
    FGameplayTag HealthStatTag;

    /**
     * GameplayTag for body temperature stat (required for ARS/attribute system).
     */
    UPROPERTY(EditAnywhere, Category="Tags")
    FGameplayTag BodyTempStatTag;

    /**
     * GameplayTag for endurance attribute (required for ARS/attribute system).
     */
    UPROPERTY(EditAnywhere, Category="Tags")
    FGameplayTag EnduranceStatTag;
    
    // =========================
    // [Legacy Status Effects - KEEPING for compatibility]
    // =========================

    /**
     * Percent of max health lost per second when starving (0.005 = 0.5% per sec).
     * Designers set this in editor.
     */
    UPROPERTY(EditAnywhere, Category="Legacy|Status Effects", meta=(ClampMin="0.0", ClampMax="1.0"))
    float StarvationHealthDoTPercent = 0.005f;

    /**
     * Percent of max health lost per second when dehydrated (0.01 = 1% per sec).
     * Designers set this in editor.
     */
    UPROPERTY(EditAnywhere, Category="Legacy|Status Effects", meta=(ClampMin="0.0", ClampMax="1.0"))
    float DehydrationHealthDoTPercent = 0.01f;

    /** Status Effect applied when the player is starving (hunger depleted).
     *  This effect should be a debuff such as stamina drain, vision blur, etc.
     */
    UPROPERTY(EditDefaultsOnly, Category="Legacy|Status Effects")
    TSubclassOf<UNomadBaseStatusEffect> StarvationDebuffEffect;

    /** Status Effect applied when the player is dehydrated (thirst depleted).
     *  This effect should be a debuff such as health drain, stamina drain, etc.
     */
    UPROPERTY(EditDefaultsOnly, Category="Legacy|Status Effects")
    TSubclassOf<UNomadBaseStatusEffect> DehydrationDebuffEffect;

    /** Status Effect applied when the player is suffering from heatstroke (body temperature hazard).
     *  This effect could be stamina drain, vision blur, overheating, etc.
     */
    UPROPERTY(EditDefaultsOnly, Category="Legacy|Status Effects")
    TSubclassOf<UNomadBaseStatusEffect> HeatstrokeDebuffEffect;

    /** Status Effect applied when the player is hypothermic (body temperature hazard).
     *  This effect could be stamina drain, movement slow, frost effects, etc.
     */
    UPROPERTY(EditDefaultsOnly, Category="Legacy|Status Effects")
    TSubclassOf<UNomadBaseStatusEffect> HypothermiaDebuffEffect;

    UPROPERTY(EditDefaultsOnly, Category="Legacy|Status Effects")
    FGameplayTag StarvationDebuffTag;

    UPROPERTY(EditDefaultsOnly, Category="Legacy|Status Effects")
    FGameplayTag DehydrationDebuffTag;

    UPROPERTY(EditDefaultsOnly, Category="Legacy|Status Effects")
    FGameplayTag HeatstrokeDebuffTag;

    UPROPERTY(EditDefaultsOnly, Category="Legacy|Status Effects")
    FGameplayTag HypothermiaDebuffTag;

    // =========================
    // [NEW: Survival Status Effect Classes - Data-Driven System]
    // =========================

    /** NEW: Mild starvation effect (hunger below warning threshold) */
    UPROPERTY(EditAnywhere, Category="Survival Status Effects|Starvation")
    TSubclassOf<UNomadSurvivalStatusEffect> StarvationMildEffectClass;

    /** NEW: Severe starvation effect (hunger at/below 0) */
    UPROPERTY(EditAnywhere, Category="Survival Status Effects|Starvation")
    TSubclassOf<UNomadSurvivalStatusEffect> StarvationSevereEffectClass;

    /** NEW: Mild dehydration effect (thirst below warning threshold) */
    UPROPERTY(EditAnywhere, Category="Survival Status Effects|Dehydration")
    TSubclassOf<UNomadSurvivalStatusEffect> DehydrationMildEffectClass;

    /** NEW: Severe dehydration effect (thirst at/below 0) */
    UPROPERTY(EditAnywhere, Category="Survival Status Effects|Dehydration")
    TSubclassOf<UNomadSurvivalStatusEffect> DehydrationSevereEffectClass;

    /** NEW: Mild heatstroke effect */
    UPROPERTY(EditAnywhere, Category="Survival Status Effects|Temperature")
    TSubclassOf<UNomadSurvivalStatusEffect> HeatstrokeMildEffectClass;

    /** NEW: Heavy heatstroke effect */
    UPROPERTY(EditAnywhere, Category="Survival Status Effects|Temperature")
    TSubclassOf<UNomadSurvivalStatusEffect> HeatstrokeHeavyEffectClass;

    /** NEW: Extreme heatstroke effect */
    UPROPERTY(EditAnywhere, Category="Survival Status Effects|Temperature")
    TSubclassOf<UNomadSurvivalStatusEffect> HeatstrokeExtremeEffectClass;

    /** NEW: Mild hypothermia effect */
    UPROPERTY(EditAnywhere, Category="Survival Status Effects|Temperature")
    TSubclassOf<UNomadSurvivalStatusEffect> HypothermiaMildEffectClass;

    /** NEW: Heavy hypothermia effect */
    UPROPERTY(EditAnywhere, Category="Survival Status Effects|Temperature")
    TSubclassOf<UNomadSurvivalStatusEffect> HypothermiaHeavyEffectClass;

    /** NEW: Extreme hypothermia effect */
    UPROPERTY(EditAnywhere, Category="Survival Status Effects|Temperature")
    TSubclassOf<UNomadSurvivalStatusEffect> HypothermiaExtremeEffectClass;

    /**
     * Advanced, non-linear tuning for all survival modifiers.
     * All curves are grouped in this struct for clarity and designer convenience.
     */
    UPROPERTY(EditAnywhere, Category="Survival|Curves", meta=(ShowOnlyInnerProperties))
    FCurvesForAdvancedModifierTuning AdvancedModifierCurves;

    // =========================
    // [Blueprint Getters - EXISTING (keeping all)]
    // =========================

    // All getters are fully documented for BP/UMG access.
    // Each getter returns a single config value, so you can use them in UI or tuning BPs.

    UFUNCTION(BlueprintPure, Category="Decay|Base") float GetDailyHungerLoss() const { return DailyHungerLoss; }
    UFUNCTION(BlueprintPure, Category="Decay|Base") float GetDailyThirstLoss() const { return DailyThirstLoss; }
    UFUNCTION(BlueprintPure, Category="Decay|Debug") float GetDebugDecayMultiplier() const { return DebugDecayMultiplier; }
    
    UFUNCTION(BlueprintPure, Category="Activity|Tiers") float GetWalkingSpeedThreshold() const { return WalkingSpeedThreshold; }
    UFUNCTION(BlueprintPure, Category="Activity|Tiers") float GetRunningSpeedThreshold() const { return RunningSpeedThreshold; }
    UFUNCTION(BlueprintPure, Category="Activity|Tiers") float GetSprintingSpeedThreshold() const { return SprintingSpeedThreshold; }

    UFUNCTION(BlueprintPure, Category="Attributes") float GetEnduranceDecayPerPoint() const { return EnduranceDecayPerPoint; }

    // --- Environmental Safe Zone (ambient temp where body temp stays stable) ---
    UFUNCTION(BlueprintPure, Category="Survival|Temperature")
    float GetSafeAmbientTempMinC() const { return SafeAmbientTempMinC; }
    UFUNCTION(BlueprintPure, Category="Survival|Temperature")
    float GetSafeAmbientTempMaxC() const { return SafeAmbientTempMaxC; }

    // --- Body Temperature Safe Zone (no hazards/effects) ---
    UFUNCTION(BlueprintPure, Category="Survival|Temperature")
    float GetSafeBodyTempMinC() const { return SafeBodyTempMinC; };
    UFUNCTION(BlueprintPure, Category="Survival|Temperature")
    float GetSafeBodyTempMaxC() const { return SafeBodyTempMaxC; };

    UFUNCTION(BlueprintPure, Category="BodyTemp") float GetNormalBodyTemperature() const { return NormalBodyTemperature; }
    UFUNCTION(BlueprintPure, Category="BodyTemp") float GetBodyTempAdjustRate() const { return AdvancedBodyTempParams.BodyTempAdjustRate; }
    UFUNCTION(BlueprintPure, Category="BodyTemp") float GetMinBodyTempChangeRate() const { return AdvancedBodyTempParams.MinBodyTempChangeRate; }
    UFUNCTION(BlueprintPure, Category="BodyTemp") float GetMaxBodyTempChangeRate() const { return AdvancedBodyTempParams.MaxBodyTempChangeRate; }
    UFUNCTION(BlueprintPure, Category="BodyTemp") UCurveFloat* GetBodyTempDriftCurve() const { return AdvancedBodyTempParams.BodyTempDriftCurve; }

    UFUNCTION(BlueprintPure, Category="Hazards|Thresholds") float GetHeatstrokeThreshold() const { return HeatstrokeThreshold; }
    UFUNCTION(BlueprintPure, Category="Hazards|Thresholds") float GetHypothermiaThreshold() const { return HypothermiaThreshold; }
    UFUNCTION(BlueprintPure, Category="Hazards|Thresholds") int32 GetHeatstrokeDurationMinutes() const { return HeatstrokeDurationMinutes; }
    UFUNCTION(BlueprintPure, Category="Hazards|Thresholds") int32 GetHypothermiaDurationMinutes() const { return HypothermiaDurationMinutes; }

    // --- Temperature Hazard Movement Slow Getters ---
    UFUNCTION(BlueprintPure, Category="Hazards|Heatstroke") float GetHeatstrokeMildThreshold() const { return HeatstrokeMildThreshold; }
    UFUNCTION(BlueprintPure, Category="Hazards|Heatstroke") float GetHeatstrokeHeavyThreshold() const { return HeatstrokeHeavyThreshold; }
    UFUNCTION(BlueprintPure, Category="Hazards|Heatstroke") float GetHeatstrokeExtremeThreshold() const { return HeatstrokeExtremeThreshold; }

    UFUNCTION(BlueprintPure, Category="Hazards|Hypothermia") float GetHypothermiaMildThreshold() const { return HypothermiaMildThreshold; }
    UFUNCTION(BlueprintPure, Category="Hazards|Hypothermia") float GetHypothermiaHeavyThreshold() const { return HypothermiaHeavyThreshold; }
    UFUNCTION(BlueprintPure, Category="Hazards|Hypothermia") float GetHypothermiaExtremeThreshold() const { return HypothermiaExtremeThreshold; }

    UFUNCTION(BlueprintPure, Category="SideEffects|Thresholds") float GetHungerSlowThreshold() const { return HungerSlowThreshold; }
    UFUNCTION(BlueprintPure, Category="SideEffects|Thresholds") float GetThirstSlowThreshold() const { return ThirstSlowThreshold; }
    
    UFUNCTION(BlueprintPure, Category="Warnings") float GetStarvationWarningThreshold() const { return StarvationWarningThreshold; }
    UFUNCTION(BlueprintPure, Category="Warnings") float GetDehydrationWarningThreshold() const { return DehydrationWarningThreshold; }
    UFUNCTION(BlueprintPure, Category="Warnings") float GetHeatstrokeWarningDelta() const { return HeatstrokeWarningDelta; }
    UFUNCTION(BlueprintPure, Category="Warnings") float GetHypothermiaWarningDelta() const { return HypothermiaWarningDelta; }
    
    UFUNCTION(BlueprintPure, Category="Warnings|Cooldowns") float GetStarvationWarningCooldown() const { return StarvationWarningCooldown; }
    UFUNCTION(BlueprintPure, Category="Warnings|Cooldowns") float GetDehydrationWarningCooldown() const { return DehydrationWarningCooldown; }
    UFUNCTION(BlueprintPure, Category="Warnings|Cooldowns") float GetHeatstrokeWarningCooldown() const { return HeatstrokeWarningCooldown; }
    UFUNCTION(BlueprintPure, Category="Warnings|Cooldowns") float GetHypothermiaWarningCooldown() const { return HypothermiaWarningCooldown; }

    UFUNCTION(BlueprintPure, Category="Temperature|Normalization") float GetMinExternalTempCelsius() const { return MinExternalTempCelsius; }
    UFUNCTION(BlueprintPure, Category="Temperature|Normalization") float GetMaxExternalTempCelsius() const { return MaxExternalTempCelsius; }
    UFUNCTION(BlueprintPure, Category="Temperature|Normalization") float GetMinExternalTempFahrenheit() const { return MinExternalTempFahrenheit; }
    UFUNCTION(BlueprintPure, Category="Temperature|Normalization") float GetMaxExternalTempFahrenheit() const { return MaxExternalTempFahrenheit; }
    UFUNCTION(BlueprintPure, Category="Temperature|Normalization") float GetExternalTemperatureScale() const { return ExternalTemperatureScale; }

    UFUNCTION(BlueprintPure, Category="Tags") FGameplayTag GetHungerStatTag() const { return HungerStatTag; }
    UFUNCTION(BlueprintPure, Category="Tags") FGameplayTag GetThirstStatTag() const { return ThirstStatTag; }
    UFUNCTION(BlueprintPure, Category="Tags") FGameplayTag GetHealthStatTag() const { return HealthStatTag; }
    UFUNCTION(BlueprintPure, Category="Tags") FGameplayTag GetBodyTempStatTag() const { return BodyTempStatTag; }
    UFUNCTION(BlueprintPure, Category="Tags") FGameplayTag GetEnduranceStatTag() const { return EnduranceStatTag; }
    
    // Legacy getters
    UFUNCTION(BlueprintPure, Category="Legacy|Status Effects") TSubclassOf<UNomadBaseStatusEffect> GetStarvationDebuffEffect() const { return StarvationDebuffEffect; }
    UFUNCTION(BlueprintPure, Category="Legacy|Status Effects") TSubclassOf<UNomadBaseStatusEffect> GetDehydrationDebuffEffect() const { return DehydrationDebuffEffect; }
    UFUNCTION(BlueprintPure, Category="Legacy|Status Effects") TSubclassOf<UNomadBaseStatusEffect> GetHeatstrokeDebuffEffect() const { return HeatstrokeDebuffEffect; }
    UFUNCTION(BlueprintPure, Category="Legacy|Status Effects") TSubclassOf<UNomadBaseStatusEffect> GetHypothermiaDebuffEffect() const { return HypothermiaDebuffEffect; }

    // =========================
    // [NEW: Status Effect Class Getters]
    // =========================
    
    UFUNCTION(BlueprintPure, Category="Survival Status Effects") TSubclassOf<UNomadSurvivalStatusEffect> GetStarvationMildEffectClass() const { return StarvationMildEffectClass; }
    UFUNCTION(BlueprintPure, Category="Survival Status Effects") TSubclassOf<UNomadSurvivalStatusEffect> GetStarvationSevereEffectClass() const { return StarvationSevereEffectClass; }
    UFUNCTION(BlueprintPure, Category="Survival Status Effects") TSubclassOf<UNomadSurvivalStatusEffect> GetDehydrationMildEffectClass() const { return DehydrationMildEffectClass; }
    UFUNCTION(BlueprintPure, Category="Survival Status Effects") TSubclassOf<UNomadSurvivalStatusEffect> GetDehydrationSevereEffectClass() const { return DehydrationSevereEffectClass; }
    
    UFUNCTION(BlueprintPure, Category="Survival Status Effects") TSubclassOf<UNomadSurvivalStatusEffect> GetHeatstrokeMildEffectClass() const { return HeatstrokeMildEffectClass; }
    UFUNCTION(BlueprintPure, Category="Survival Status Effects") TSubclassOf<UNomadSurvivalStatusEffect> GetHeatstrokeHeavyEffectClass() const { return HeatstrokeHeavyEffectClass; }
    UFUNCTION(BlueprintPure, Category="Survival Status Effects") TSubclassOf<UNomadSurvivalStatusEffect> GetHeatstrokeExtremeEffectClass() const { return HeatstrokeExtremeEffectClass; }
    
    UFUNCTION(BlueprintPure, Category="Survival Status Effects") TSubclassOf<UNomadSurvivalStatusEffect> GetHypothermiaMildEffectClass() const { return HypothermiaMildEffectClass; }
    UFUNCTION(BlueprintPure, Category="Survival Status Effects") TSubclassOf<UNomadSurvivalStatusEffect> GetHypothermiaHeavyEffectClass() const { return HypothermiaHeavyEffectClass; }
    UFUNCTION(BlueprintPure, Category="Survival Status Effects") TSubclassOf<UNomadSurvivalStatusEffect> GetHypothermiaExtremeEffectClass() const { return HypothermiaExtremeEffectClass; }
};