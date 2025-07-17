// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actions/ACFBaseAction.h"
#include "Game/ACFDamageType.h"
#include "ACFHitAction.generated.h"

/**
 * UACFHitAction
 * ------------------------------------------------------------------------------
 * Represents a "hit" or "stagger" reaction state for a character in ACF.
 * This action is typically triggered when the character receives damage and
 * a corresponding "hit response" is required (e.g., being staggered or flinched).
 *
 * FEATURES:
 * - Selects the animation montage section based on the direction of the hit
 *   (front, back, left, right), so that character reactions are context-aware.
 * - Stores the damage event that triggered this action, making it available for
 *   animation, effects, or custom logic.
 * - Can be extended for more nuanced reactions (see UACFAdvancedHitAction).
 * - Integrates with the ActionsManager and the overall state system, allowing
 *   for modular, interruptible, and prioritized reactions.
 *
 * TYPICAL USAGE:
 * - When a character's UACFDamageHandlerComponent processes damage and
 *   determines a "hit response" is needed, it triggers this action via the
 *   ActionsManager.
 * - The action will play the correct hit animation, apply any temporary effects,
 *   and then automatically end, returning control to default or queued actions.
 */
UCLASS()
class ASCENTCOMBATFRAMEWORK_API UACFHitAction : public UACFBaseAction {
    GENERATED_BODY()

    /**
     * Constructor:
     * - Initializes the direction-to-montage-section map with standard directions.
     * - Ensures that if a hit comes from front, back, left, or right, the correct
     *   animation section will be selected for a more realistic reaction.
     */
    UACFHitAction();

protected:
    /**
     * Called when this hit action starts (e.g., when the character is hit).
     * - Retrieves the last FACFDamageEvent from the owning character, which contains
     *   info about the hit (who hit us, from which direction, to which bone, etc).
     * - Stores this info locally for use in animation and effects.
     * - Stores the current action state in the ActionsManager for later recovery.
     * - If the action uses a special motion mode (e.g., ECurveOverrideSpeedAndDirection),
     *   applies velocity/force overrides based on hit direction or momentum.
     */
    virtual void OnActionStarted_Implementation(const FString& contextString = "", AActor* InteractedActor = nullptr, FGameplayTag ItemSlotTag = FGameplayTag()) override;

    /**
     * Called when this hit action ends.
     * - Cleans up any temporary velocity or movement overrides (if used).
     * - Resets any state or flags as necessary.
     */
    virtual void OnActionEnded_Implementation() override;

    /**
     * Selects the name of the montage section to play for this hit reaction.
     * - Looks up the hit direction in the HitDirectionToMontageSectionMap.
     * - If a matching section is found (e.g., "Front" for a frontal hit), that section is played.
     * - If no match is found, falls back to the base implementation (typically plays the default section).
     */
    virtual FName GetMontageSectionName_Implementation() override;

    /**
     * Maps each possible hit direction (Front, Back, Left, Right) to an animation montage section name.
     * - Allows designers to use direction-specific hit animations for realism.
     * - Can be customized per action or subclassed for advanced behavior.
     */
    UPROPERTY(EditDefaultsOnly, Category = ACF)
    TMap<EACFDirection, FName> HitDirectionToMontageSectionMap;

    /**
     * Stores the full damage event data for the hit that triggered this action.
     * - Includes the dealer, receiver, direction, zone, bone, damage type, etc.
     * - Used for animation selection, effects, and advanced logic.
     */
    FACFDamageEvent damageReceived;
};