// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actions/ACFBaseAction.h"
#include "Game/ACFDamageType.h"
#include "ACFAdvancedHitAction.generated.h"

/**
 * UACFAdvancedHitAction
 * ------------------------------------------------------------------------------
 * Extends UACFHitAction to allow for more granular and immersive hit reactions.
 * FEATURES:
 * - Maps not just direction but also specific hit bones (e.g., Head, Arm, Leg)
 *   to custom montage sections for highly detailed reactions.
 * - Supports root motion warping, allowing the character to be moved/rotated
 *   precisely upon being hit (e.g., knockback, cinematic stumbles).
 * - Customizable hit warp distance and montage selection logic.
 *
 * TYPICAL USAGE:
 * - Use this when you want advanced reactions such as different animations when
 *   hit in the head, torso, or limbs, and/or want to apply directional movement
 *   or force during the animation.
 */
UCLASS()
class ASCENTCOMBATFRAMEWORK_API UACFAdvancedHitAction : public UACFBaseAction {
    GENERATED_BODY()

    /** Constructor: Initializes default direction-to-section map and configures motion warping. */
    UACFAdvancedHitAction();

protected:
    /**
     * Called when the advanced hit action starts.
     * - Stores the current action state (for recovery).
     * - (Optionally) applies additional setup for advanced animation/logic.
     */
    virtual void OnActionStarted_Implementation(const FString& contextString = "", AActor* InteractedActor = nullptr, FGameplayTag ItemSlotTag = FGameplayTag()) override;

    /** Called when the advanced hit action ends. Used for cleanup. */
    virtual void OnActionEnded_Implementation() override;

    /**
     * If using motion warping, computes the transform the character should warp to
     * during the hit animation (e.g., for knockback, cinematic movement).
     * - Considers the damage direction, hit momentum, and configurable distance.
     */
    virtual FTransform GetWarpTransform_Implementation() override;

    /**
     * Selects the montage section to play for this hit reaction.
     * - Considers both direction and specific hit bone (using FrontDetailsSectionByBoneNames).
     * - If a bone-specific section is found, it takes precedence.
     */
    virtual FName GetMontageSectionName_Implementation() override;

    /**
     * Maps each hit direction (Front, Back, Left, Right) to a montage section name.
     * Used as a fallback if no bone-specific section is found.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = ACF)
    TMap<EHitDirection, FName> HitDirectionToMontageSectionMap;

    /**
     * Array mapping specific bone names (e.g., "Head", "Chest", "Leg") to
     * montage sections for highly detailed front-facing hit reactions.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = ACF)
    TArray<FBoneSections> FrontDetailsSectionByBoneNames;

    /**
     * If snap-to-target warping is enabled, this is the distance used for the
     * root motion warp (e.g., how far the character should be knocked back).
     */
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bSnapToTarget), Category = "ACF| Warp")
    float hitWarpDistance = 200.f;

private:
    /**
     * Helper: Gets montage section name from direction (safe, fallback to default if missing).
     */
    FName GetMontageSectionFromHitDirectionSafe(const EHitDirection hitDir) const;

    /**
     * Helper: Gets montage section for the front direction, considering bone-specific overrides.
     */
    FName GetMontageSectionFromFront(const FACFDamageEvent& damageEvent) const;
};