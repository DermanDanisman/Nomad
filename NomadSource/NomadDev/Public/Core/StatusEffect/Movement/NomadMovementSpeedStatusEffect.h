// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/StatusEffect/NomadInfiniteStatusEffect.h"
#include "GameplayTagContainer.h"
#include "NomadMovementSpeedStatusEffect.generated.h"

/**
 * UNomadMovementSpeedStatusEffect
 * -------------------------------
 * Base class for movement speed modification status effects.
 * 
 * Key Features:
 * - Inherits from UNomadInfiniteStatusEffect for persistent effects
 * - Automatically applies movement speed modifiers from config (PersistentAttributeModifier)
 * - Supports both temporary and permanent movement speed changes
 * - Can optionally block input actions (sprint, jump, etc.) via config
 * - Integrates with the attribute system for proper stat modification
 * 
 * Design Philosophy:
 * - All movement speed values are data-driven via UNomadInfiniteEffectConfig
 * - No hard-coded multipliers or thresholds
 * - Supports stacking if configured (for multiple speed effects)
 * - Automatic cleanup when effect ends
 * 
 * Usage Examples:
 * - Speed boosts (haste effects, equipment bonuses)
 * - Speed penalties (survival effects, debuffs, curses)
 * - Conditional movement changes (environmental effects)
 */
UCLASS(BlueprintType, Abstract)
class NOMADDEV_API UNomadMovementSpeedStatusEffect : public UNomadInfiniteStatusEffect
{
    GENERATED_BODY()

public:
    /** Constructor - sets up default values for movement effects */
    UNomadMovementSpeedStatusEffect();

    /**
     * Sets the movement speed multiplier for this effect instance.
     * This is used for runtime adjustments when the same effect class needs different multipliers.
     * Note: The primary movement speed modification should come from PersistentAttributeModifier in config.
     * 
     * @param InMultiplier Movement speed multiplier (1.0 = no change, 0.5 = 50% speed, 2.0 = 200% speed)
     */
    UFUNCTION(BlueprintCallable, Category="Movement Effect")
    void SetMovementSpeedMultiplier(float InMultiplier);

    /**
     * Gets the current movement speed multiplier of this effect.
     * Used by UI and other systems to display effect strength.
     */
    UFUNCTION(BlueprintPure, Category="Movement Effect")
    float GetMovementSpeedMultiplier() const { return MovementSpeedMultiplier; }

    /**
     * Gets the recommended display text for this movement effect.
     * Returns user-friendly text like "Movement Speed: +25%" or "Movement Speed: -50%"
     */
    UFUNCTION(BlueprintPure, Category="Movement Effect")
    FText GetMovementEffectDisplayText() const;

protected:
    /** Current movement speed multiplier for this effect instance */
    UPROPERTY(BlueprintReadOnly, Category="Movement Effect")
    float MovementSpeedMultiplier = 1.0f;

    /** 
     * Called when the movement effect starts.
     * Applies movement speed modifiers and input blocking based on config.
     */
    virtual void OnStatusEffectStarts_Implementation(ACharacter* Character) override;

    /**
     * Called when the movement effect ends.
     * Removes all movement speed modifiers and input blocking.
     */
    virtual void OnStatusEffectEnds_Implementation() override;

    /**
     * Called on each periodic tick if enabled in config.
     * Can be used for dynamic movement speed adjustments.
     */
    virtual void HandleInfiniteTick() override;

    /**
     * Applies movement-specific visual/audio effects.
     * Override in subclasses for effect-specific feedback (speed lines, audio cues, etc.).
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Movement Effect|Visual")
    void ApplyMovementVisualEffects();

    /**
     * Removes all movement-specific visual/audio effects.
     * Override in subclasses to clean up effect-specific feedback.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Movement Effect|Visual")
    void RemoveMovementVisualEffects();

private:
    /** Tracks if we've applied movement speed modifiers to avoid double-application */
    bool bHasAppliedMovementModifiers = false;
    
    /** Tracks if we've applied input blocking to ensure proper cleanup */
    bool bHasAppliedInputBlocking = false;
};

/**
 * UNomadSpeedBoostStatusEffect
 * ----------------------------
 * Status effect for temporary or permanent movement speed increases.
 * 
 * Applied for haste effects, equipment bonuses, or environmental speed boosts.
 * Uses positive multipliers in PersistentAttributeModifier config.
 * 
 * Requirements Implementation:
 * - Movement Speed Increase (via PersistentAttributeModifier in config)
 * - Optional Visual Effects (speed lines, glowing feet, etc.)
 * - Optional Audio Effects (wind sounds, energy hums, etc.)
 * - Duration: Configurable (instant, timed, or infinite)
 */
UCLASS(BlueprintType)
class NOMADDEV_API UNomadSpeedBoostStatusEffect : public UNomadMovementSpeedStatusEffect
{
    GENERATED_BODY()

public:
    UNomadSpeedBoostStatusEffect()
    {
        // Set gameplay tag for this specific movement effect
        SetStatusEffectTag(FGameplayTag::RequestGameplayTag("StatusEffect.Movement.SpeedBoost"));
    }

protected:
    /**
     * Applies speed boost-specific visual effects.
     * Should implement speed lines, particle effects, character glow, etc.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Speed Boost Effect")
    void ApplySpeedBoostVisuals();

    /**
     * Removes speed boost-specific visual effects.
     * Should stop all speed boost visual and audio effects.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Speed Boost Effect")  
    void RemoveSpeedBoostVisuals();
};

/**
 * UNomadSpeedPenaltyStatusEffect
 * ------------------------------
 * Status effect for temporary or permanent movement speed decreases.
 * 
 * Applied for slowing effects, equipment penalties, debuffs, or survival conditions.
 * Uses negative multipliers or sub-1.0 multipliers in PersistentAttributeModifier config.
 * Can optionally block sprint/jump actions via BlockingTags in config.
 * 
 * Requirements Implementation:
 * - Movement Speed Decrease (via PersistentAttributeModifier in config)
 * - Optional Input Blocking (via BlockingTags in config)
 * - Optional Visual Effects (slow motion effect, heavy footsteps, etc.)
 * - Duration: Configurable (instant, timed, or infinite)
 */
UCLASS(BlueprintType)
class NOMADDEV_API UNomadSpeedPenaltyStatusEffect : public UNomadMovementSpeedStatusEffect
{
    GENERATED_BODY()

public:
    UNomadSpeedPenaltyStatusEffect()
    {
        // Set gameplay tag for this specific movement effect
        SetStatusEffectTag(FGameplayTag::RequestGameplayTag("StatusEffect.Movement.SpeedPenalty"));
    }

protected:
    /**
     * Applies speed penalty-specific visual effects.
     * Should implement slow motion visuals, heavy particle effects, fatigue indicators, etc.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Speed Penalty Effect")
    void ApplySpeedPenaltyVisuals();

    /**
     * Removes speed penalty-specific visual effects.
     * Should stop all speed penalty visual and audio effects.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Speed Penalty Effect")
    void RemoveSpeedPenaltyVisuals();
};

/**
 * UNomadMovementDisabledStatusEffect
 * ----------------------------------
 * Status effect for completely disabling movement.
 * 
 * Applied for paralysis, stun, root effects, or other complete movement restrictions.
 * Sets movement speed to near-zero and blocks all movement-related actions.
 * 
 * Requirements Implementation:
 * - Movement Speed set to 0 or very low value (via PersistentAttributeModifier in config)
 * - Block Sprint, Jump, and potentially other actions (via BlockingTags in config)
 * - Visual Effects (chains, ice, roots, etc.)
 * - Duration: Usually timed effects
 */
UCLASS(BlueprintType)
class NOMADDEV_API UNomadMovementDisabledStatusEffect : public UNomadMovementSpeedStatusEffect
{
    GENERATED_BODY()

public:
    UNomadMovementDisabledStatusEffect()
    {
        // Set gameplay tag for this specific movement effect
        SetStatusEffectTag(FGameplayTag::RequestGameplayTag("StatusEffect.Movement.Disabled"));
    }

protected:
    /**
     * Applies movement disabled-specific visual effects.
     * Should implement paralysis visuals, binding effects, status indicators, etc.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Movement Disabled Effect")
    void ApplyMovementDisabledVisuals();

    /**
     * Removes movement disabled-specific visual effects.
     * Should stop all movement disabled visual and audio effects.
     */
    UFUNCTION(BlueprintImplementableEvent, Category="Movement Disabled Effect")
    void RemoveMovementDisabledVisuals();
};