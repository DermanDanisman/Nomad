// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFDamageType.h"
#include "CoreMinimal.h"

#include "ACFDamageCalculation.generated.h"

struct FACFDamageEvent;

/**
 * Base class for damage calculation logic in the Ascent Combat Framework (ACF).
 *
 * UACFDamageCalculation is designed to be extended for different games or damage systems,
 * allowing you to override damage logic, critical hit checks, and hit response evaluation.
 *
 * - Marked as Blueprintable and Abstract so designers can create Blueprint subclasses.
 * - Supports instancing and inline editing for advanced customization.
 * - All major calculation functions are BlueprintNativeEvent for Blueprint/C++ flexibility.
 *
 * Usage:
 *   - Override CalculateFinalDamage to implement custom damage formulas.
 *   - Override EvaluateHitResponseAction to select which gameplay tag/action to trigger on hit.
 *   - Override IsCriticalDamage to determine if a hit is critical (e.g., for multipliers, VFX).
 *   - Use GetDamageType to retrieve the damage type asset from the damage event.
 */
UCLASS(Blueprintable, Abstract, BlueprintType, DefaultToInstanced, EditInlineNew)
class ASCENTCOMBATFRAMEWORK_API UACFDamageCalculation : public UObject {
    GENERATED_BODY()

public:
    /**
     * Calculates the final amount of damage to apply for a given damage event.
     *
     * @param inDamageEvent - The damage event data, including source, target, tags, and base damage.
     * @return The computed final damage value (after all modifiers, resistances, etc.).
     *
     * Override this method in Blueprints/C++ to implement your own damage calculation logic.
     */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    float CalculateFinalDamage(const FACFDamageEvent& inDamageEvent);

    /**
     * C++ implementation of CalculateFinalDamage (default: returns inDamageEvent.FinalDamage).
     * Override in subclasses for custom logic.
     */
    virtual float CalculateFinalDamage_Implementation(const FACFDamageEvent& inDamageEvent);

    /**
     * Evaluates which hit response action should occur for this damage event.
     *
     * @param inDamageEvent - The damage event data.
     * @param hitResponseActions - An array of possible actions, each with associated probabilities or conditions.
     * @return The gameplay tag representing the chosen hit response action (e.g., "Hit.Stagger", "Hit.Knockdown").
     *
     * Override this method to select hit animations, effects, or responses based on tags or chance.
     */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    FGameplayTag EvaluateHitResponseAction(const FACFDamageEvent& inDamageEvent, const TArray<FOnHitActionChances>& hitResponseActions);

    /**
     * C++ implementation of EvaluateHitResponseAction (default: returns empty tag).
     * Override in subclasses to implement your own logic.
     */
    virtual FGameplayTag EvaluateHitResponseAction_Implementation(const FACFDamageEvent& inDamageEvent, const TArray<FOnHitActionChances>& hitResponseActions);

    /**
     * Determines if the damage event is considered a critical hit.
     *
     * @param inDamageEvent - The damage event data.
     * @return true if the damage is critical (e.g., should trigger a critical hit effect), false otherwise.
     *
     * Override this method for custom critical hit logic based on stats, tags, locations, etc.
     */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    bool IsCriticalDamage(const FACFDamageEvent& inDamageEvent);

    /**
     * C++ implementation of IsCriticalDamage (default: returns false).
     * Override in subclasses for your own critical hit detection.
     */
    virtual bool IsCriticalDamage_Implementation(const FACFDamageEvent& inDamageEvent);

    /**
     * Utility function to get the UACFDamageType asset/class associated with this damage event.
     *
     * @param inDamageEvent - The damage event data.
     * @return The default object for the UACFDamageType class, or nullptr if not set.
     *
     * Use this to access tags, scaling, and properties from the damage type asset.
     */
    UFUNCTION(BlueprintPure, Category = ACF)
    UACFDamageType* GetDamageType(const FACFDamageEvent& inDamageEvent);
};