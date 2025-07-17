// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NomadStatusEffectGameplayHelpers.generated.h"

/**
 * Utility library for syncing ARS/ACF MovementSpeed attribute with Character movement.
 * - Call after any stat/attribute mod (buff, debuff, slow, haste, etc.).
 * - Works with both percentage-based and additive mods: make sure your MovementSpeed attribute is updated properly!
 * - Call from C++ or Blueprint (e.g., on stat change, on status effect applied/removed, on item equip/unequip).
 */
UCLASS(Blueprintable)
class NOMADDEV_API UNomadStatusEffectGameplayHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

    /**
     * Syncs Character movement speed to the current value of the "RPG.Attributes.MovementSpeed" attribute.
     *
     * - Should be called any time MovementSpeed changes due to a status effect, stat mod, or attribute change.
     * - Requires both UARSStatisticsComponent and UACFCharacterMovementComponent on the Character.
     *
     * Usage Examples:
     *   C++:      UNomadStatusEffectGameplayHelpers::SyncMovementSpeedFromStat(MyCharacter);
     *   Blueprint: Call 'Sync Movement Speed From Stat' after stat/status changes.
     *
     * How the attribute affects speed:
     *   - Suppose the base MovementSpeed is 600.
     *   - Status effects can modify it via percentage:
     *         Value    | Effect Type     | Resulting Speed
     *         -------------------------------------------------
     *         25       | +25% boost      | 600 + 25% = 750
     *         -50      | -50% slow       | 600 - 50% = 300
     *         0        | No change       | 600 (unchanged)
     *         100      | +100% (double)  | 600 + 100% = 1200
     *         -100     | -100% (stop)    | 0 (clamped to min if needed)
     *
     *   - Value of "75" is a 75% haste; "-30" is a 30% slow. (These are *not* fractions; use integers or floats.)
     *
     * Design Notes:
     *   - Call this helper after **any** movement speed-affecting effect (buff, debuff, item, status, etc.).
     *   - This ensures that multiplayer clients and the server are always in sync.
     *
     * @param Character   The character to update. Must have ARSStatisticsComponent and ACFCharacterMovementComponent.
     */
    UFUNCTION(BlueprintCallable, Category="Nomad | Status Effect Gameplay Helpers | Movement")
    static void SyncMovementSpeedFromStat(ACharacter* Character);

    /**
     * Utility to check if the character is currently blocked from sprinting by any active status effect.
     *
     * - Looks for the "Status.Block.Sprint" gameplay tag in the character's active blocking tags (on the status effect manager).
     * - Returns true if any effect is blocking sprint, false otherwise.
     * - Safe to call on both server and client (blocking tags are replicated).
     *
     * @param Character The character to check.
     * @return true if sprint is blocked by any effect; false otherwise.
     */
    UFUNCTION(BlueprintPure, Category="Nomad|Movement")
    static bool IsSprintBlocked(ACharacter* Character);

    /**
     * Apply a movement speed multiplier to a specific locomotion state (walk, jog, sprint, etc).
     * 
     * Example: To slow walk/jog by 20%, call with Multiplier = 0.8.
     * 
     * @param MoveComp The movement component to modify.
     * @param State The locomotion state to affect (EWalk, EJog, ESprint, etc).
     * @param Multiplier The multiplier to apply (0.8 = 20% slow, 1.0 = no change).
     * @param Guid A unique identifier for this modifier (so you can remove it later).
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Movement")
    static void ApplyMovementSpeedModifierToState(
        UACFCharacterMovementComponent* MoveComp,
        ELocomotionState State,
        float Multiplier,
        const FGuid& Guid);

    /**
     * Remove a movement speed modifier from a specific locomotion state (by Guid).
     * 
     * @param MoveComp The movement component to modify.
     * @param State The locomotion state to affect.
     * @param Guid The unique identifier used when you added the modifier.
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Movement")
    static void RemoveMovementSpeedModifierFromState(
        UACFCharacterMovementComponent* MoveComp,
        ELocomotionState State,
        const FGuid& Guid);


};