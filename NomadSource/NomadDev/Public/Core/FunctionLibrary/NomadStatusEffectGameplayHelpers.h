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
     * DEPRECATED: Use SyncMovementSpeedFromAttribute with configurable attribute tag instead.
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
    UFUNCTION(BlueprintCallable, Category="Nomad | Status Effect Gameplay Helpers | Movement", 
        meta=(DeprecatedFunction, DeprecationMessage="Use SyncMovementSpeedFromAttribute instead"))
    static void SyncMovementSpeedFromStat(ACharacter* Character);

    /**
     * Syncs Character movement speed to the current value of the specified attribute.
     *
     * - Should be called any time the movement speed attribute changes due to a status effect, stat mod, or attribute change.
     * - Requires both UARSStatisticsComponent and UACFCharacterMovementComponent on the Character.
     * - Uses configurable attribute tag instead of hardcoded "RPG.Attributes.MovementSpeed".
     *
     * @param Character      The character to update. Must have ARSStatisticsComponent and ACFCharacterMovementComponent.
     * @param AttributeTag   The gameplay tag for the movement speed attribute.
     */
    UFUNCTION(BlueprintCallable, Category="Nomad | Status Effect Gameplay Helpers | Movement")
    static void SyncMovementSpeedFromAttribute(ACharacter* Character, const FGameplayTag& AttributeTag);

    /**
     * Syncs Character movement speed using the default RPG.Attributes.MovementSpeed attribute.
     * This is the recommended method for most use cases.
     *
     * @param Character   The character to update. Must have ARSStatisticsComponent and ACFCharacterMovementComponent.
     */
    UFUNCTION(BlueprintCallable, Category="Nomad | Status Effect Gameplay Helpers | Movement")
    static void SyncMovementSpeedFromDefaultAttribute(ACharacter* Character);

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
     * Utility to check if the character is currently blocked from jumping by any active status effect.
     *
     * - Looks for the "Status.Block.Jump" gameplay tag in the character's active blocking tags.
     * - Returns true if any effect is blocking jump, false otherwise.
     * - Safe to call on both server and client (blocking tags are replicated).
     *
     * @param Character The character to check.
     * @return true if jump is blocked by any effect; false otherwise.
     */
    UFUNCTION(BlueprintPure, Category="Nomad|Movement")
    static bool IsJumpBlocked(ACharacter* Character);

    /**
     * Utility to check if a specific action is blocked by any active status effect.
     *
     * - Checks for the specified blocking tag in the character's active blocking tags.
     * - Returns true if any effect is blocking the action, false otherwise.
     * - Safe to call on both server and client (blocking tags are replicated).
     *
     * @param Character The character to check.
     * @param BlockingTag The specific blocking tag to check for (e.g., "Status.Block.Interact").
     * @return true if the action is blocked by any effect; false otherwise.
     */
    UFUNCTION(BlueprintPure, Category="Nomad|Movement")
    static bool IsActionBlocked(ACharacter* Character, const FGameplayTag& BlockingTag);

    /**
     * Apply a movement speed multiplier through the status effect system instead of direct manipulation.
     * 
     * DEPRECATED: Use status effects with PersistentAttributeModifier configuration instead.
     * 
     * Example: To slow walk/jog by 20%, call with Multiplier = 0.8.
     * 
     * @param MoveComp The movement component to modify.
     * @param State The locomotion state to affect (EWalk, EJog, ESprint, etc).
     * @param Multiplier The multiplier to apply (0.8 = 20% slow, 1.0 = no change).
     * @param Guid A unique identifier for this modifier (so you can remove it later).
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Movement", 
        meta=(DeprecatedFunction, DeprecationMessage="Use status effects with PersistentAttributeModifier configuration instead"))
    static void ApplyMovementSpeedModifierToState(
        UACFCharacterMovementComponent* MoveComp,
        ELocomotionState State,
        float Multiplier,
        const FGuid& Guid);

    /**
     * Remove a movement speed modifier from a specific locomotion state (by Guid).
     * 
     * DEPRECATED: Use status effects with PersistentAttributeModifier configuration instead.
     * 
     * @param MoveComp The movement component to modify.
     * @param State The locomotion state to affect.
     * @param Guid The unique identifier used when you added the modifier.
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Movement", 
        meta=(DeprecatedFunction, DeprecationMessage="Use status effects with PersistentAttributeModifier configuration instead"))
    static void RemoveMovementSpeedModifierFromState(
        UACFCharacterMovementComponent* MoveComp,
        ELocomotionState State,
        const FGuid& Guid);

    /**
     * Apply a movement speed effect through the status effect system.
     * This is the recommended approach for temporary movement speed modifications.
     * 
     * @param Character The character to apply the effect to.
     * @param StatusEffectClass The status effect class to apply (should have movement speed modifiers configured).
     * @param Duration Duration of the effect (0 for infinite).
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Movement")
    static void ApplyMovementSpeedStatusEffect(
        ACharacter* Character,
        TSubclassOf<class UNomadBaseStatusEffect> StatusEffectClass,
        float Duration = 0.0f);

    /**
     * Remove a movement speed effect by its gameplay tag.
     * 
     * @param Character The character to remove the effect from.
     * @param EffectTag The gameplay tag of the effect to remove.
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Movement")
    static void RemoveMovementSpeedStatusEffect(
        ACharacter* Character,
        const FGameplayTag& EffectTag);


};