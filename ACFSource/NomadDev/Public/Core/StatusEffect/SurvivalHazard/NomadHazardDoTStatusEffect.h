#pragma once

#include "CoreMinimal.h"
#include "Core/StatusEffect/NomadInfiniteStatusEffect.h"
#include "NomadHazardDoTStatusEffect.generated.h"

/**
 * UNomadHazardDoTStatusEffect
 * ----------------------------------------------------------
 * Infinite status effect for starvation and dehydration hazard damage-over-time (DoT).
 * 
 * - Applies periodic damage to the owning character based on a percentage of their max health.
 * - Damage per tick is configurable at runtime (via SetDoTPercent), allowing different hazards to share the same logic.
 * - All other stat modifications (movement slow, stamina cap, etc.) are driven by config assets and NOT hardcoded.
 * - This effect is designed to be non-stacking; each hazard instance works independently.
 * - The periodic tick is managed by the parent NomadInfiniteStatusEffect system.
 */
UCLASS(BlueprintType, Blueprintable)
class NOMADDEV_API UNomadHazardDoTStatusEffect : public UNomadInfiniteStatusEffect
{
    GENERATED_BODY()

public:
    /** Constructor. Initializes DoTPercent to zero. */
    UNomadHazardDoTStatusEffect();

    /**
     * Sets the percent of max health to use for DoT damage.
     * @param InPercent - Percent of max health to use (e.g. 0.005 for 0.5% per tick).
     * Usage: Call this when applying the effect from the survival system.
     */
    UFUNCTION(BlueprintCallable, Category="Hazard DoT")
    void SetDoTPercent(float InPercent);

protected:
    /**
     * The percent of max health to use for DoT damage each tick.
     * Typically set by the survival system when effect is applied (not by config).
     * Example: 0.005 = 0.5% per tick.
     */
    UPROPERTY(BlueprintReadOnly, Category="Hazard DoT")
    float DoTPercent = 0.0f;

    /**
     * Called automatically on every periodic tick (interval set by config).
     * Applies all stat modifications specified in the config asset, including DoT.
     * If DoT percent is set, damage is calculated as: MaxHealth * DoTPercent * TickInterval.
     */
    virtual void HandleInfiniteTick() override;
};