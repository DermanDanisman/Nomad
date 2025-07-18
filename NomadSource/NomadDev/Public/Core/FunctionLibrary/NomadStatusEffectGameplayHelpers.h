// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NomadStatusEffectGameplayHelpers.generated.h"

// Forward declarations to reduce coupling
enum class ESurvivalSeverity : uint8;

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
     * CURRENT: Syncs Character movement speed to the current value of the specified attribute.
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
     * RECOMMENDED: Syncs Character movement speed using the default RPG.Attributes.MovementSpeed attribute.
     * This is the recommended method for most use cases when using the standard attribute.
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
     * Apply a movement speed effect through the status effect system.
     * 
     * RECOMMENDED APPROACH: Use existing status effect types with proper config assets:
     * - UNomadInfiniteStatusEffect: For permanent movement changes
     * - UNomadTimedStatusEffect: For temporary movement changes  
     * - UNomadSurvivalStatusEffect: For survival-related movement effects
     * 
     * Configure movement speed via config asset's PersistentAttributeModifier.
     * Configure input blocking via config asset's BlockingTags.
     * 
     * @param Character The character to apply the effect to.
     * @param StatusEffectClass The status effect class (should have PersistentAttributeModifier configured).
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

    /**
     * Utility method to check if any movement speed effects are currently active.
     * This can be used by UI systems to show movement status indicators.
     * 
     * @param Character The character to check.
     * @return true if any movement speed effects are active; false otherwise.
     */
    UFUNCTION(BlueprintPure, Category="Nomad|Movement")
    static bool HasActiveMovementSpeedEffects(ACharacter* Character);

    /**
     * Gets all active movement speed effect tags on the character.
     * Useful for debugging and UI display of current movement modifiers.
     * 
     * @param Character The character to check.
     * @return Array of gameplay tags for active movement speed effects.
     */
    UFUNCTION(BlueprintPure, Category="Nomad|Movement")
    static TArray<FGameplayTag> GetActiveMovementSpeedEffectTags(ACharacter* Character);

    /**
     * Helper method to apply standard survival movement penalty.
     * This replaces hardcoded movement slowing with a data-driven status effect approach.
     * 
     * @param Character The character to apply the penalty to.
     * @param PenaltyLevel Severity of the penalty (Mild, Heavy, Severe).
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Survival")
    static void ApplySurvivalMovementPenalty(
        ACharacter* Character,
        ESurvivalSeverity PenaltyLevel);

    /**
     * Helper method to remove survival movement penalties.
     * 
     * @param Character The character to remove penalties from.
     */
    UFUNCTION(BlueprintCallable, Category="Nomad|Survival")
    static void RemoveSurvivalMovementPenalty(ACharacter* Character);

private:
    /**
     * Returns configurable movement speed effect tags.
     * This replaces hardcoded tags with a data-driven approach.
     * 
     * In the future, this could be moved to:
     * - A data asset (UNomadMovementSpeedTagsConfig)
     * - Game settings (UNomadGameplaySettings) 
     * - Project settings (UNomadDeveloperSettings)
     */
    static TArray<FGameplayTag> GetConfigurableMovementSpeedEffectTags();


};