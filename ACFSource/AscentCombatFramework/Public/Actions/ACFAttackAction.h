// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actions/ACFComboAction.h"
#include "RootMotionModifier.h"
#include "Game/ACFTypes.h"
#include "ACFAttackAction.generated.h"

/**
 * UACFAttackAction
 * ------------------------------------------------------------------------------
 * Represents an offensive action (attack) such as a melee swing or a special
 * ability. Used for both basic and combo attacks in the ACF system.
 *
 * FEATURES:
 * - Controls the activation and deactivation of weapon or body collision traces
 *   during attack animations, using animation notifies or code.
 * - Supports root motion warping, allowing the character to auto-align or move
 *   towards their target during the attack (for cinematic or tactical combat).
 * - Provides fine-grained control over which damage traces/channels are active,
 *   for what duration, and under which attack phase.
 * - Can be extended for charged attacks, ranged attacks, or combo logic.
 *
 * TYPICAL USAGE:
 * - When an attack animation starts, it calls OnActionStarted, which determines
 *   whether to use root motion warping and sets up target tracking.
 * - During the animation, anim notifies or code call ActivateDamage/DeactivateDamage
 *   to enable/disable hit detection traces.
 * - The attack can warp the character towards the target, using configurable
 *   constraints (distance, angle, magnetism strength) for smooth and "sticky"
 *   attacks (e.g., lock-on or lunge).
 * - When the attack ends, all traces and effects are cleaned up.
 */
UCLASS()
class ASCENTCOMBATFRAMEWORK_API UACFAttackAction : public UACFComboAction {
    GENERATED_BODY()

public:
    /** Constructor: Sets default damage activation type and enables motion warping. */
    UACFAttackAction();

protected:
    /**
     * Called when the attack action starts.
     * - Determines if motion warping is to be used, and if so, calculates and stores the warp transform.
     * - Sets up which montage reproduction type to use (root motion or warped).
     * - Locates the current target and sets up for attack movement if warping is enabled.
     */
    virtual void OnActionStarted_Implementation(const FString& contextString = "", AActor* InteractedActor = nullptr, FGameplayTag ItemSlotTag = FGameplayTag()) override;

    /**
     * Called when the attack action ends.
     * - Disables any damage traces (so weapon no longer hits).
     * - Restores the montage reproduction type to its previous value.
     */
    virtual void OnActionEnded_Implementation() override;

    /**
     * Called when an attack substate (such as a combo window or special move phase) starts.
     * - Activates the corresponding damage traces (e.g., right hand, left hand, both).
     */
    virtual void OnSubActionStateEntered_Implementation() override;

    /**
     * Called when an attack substate ends.
     * - Deactivates the corresponding damage traces.
     */
    virtual void OnSubActionStateExited_Implementation() override;

    /**
     * If using motion warping, returns the transform the character should warp to
     * during the attack animation (e.g., to reach the enemy or align the swing).
     */
    virtual FTransform GetWarpTransform_Implementation() override;

    /**
     * For motion warping, returns the target component (e.g., a point on the enemy)
     * to warp to, if applicable.
     */
    virtual USceneComponent* GetWarpTargetComponent_Implementation() override;

    /**
     * Called every frame during this action if continuous update is enabled.
     * - Continuously updates the warp transform towards the target for "sticky"
     *   attacks or to track moving targets.
     */
    virtual void OnTick_Implementation(float DeltaTime) override;

    /**
     * Specifies which damage traces to activate (e.g., physical collision, left/right hand, both).
     */
    UPROPERTY(EditDefaultsOnly, Category = ACF)
    EDamageActivationType DamageToActivate;

    /**
     * List of named trace channels (e.g., "RightSword", "LeftClaw") that should be
     * activated during this attack.
     */
    UPROPERTY(EditDefaultsOnly, Category = ACF)
    TArray<FName> TraceChannels;

    /**
     * Enables or disables warp condition checking (e.g., distance and angle to target).
     * When true, the attack will only use motion warping if the target is within the
     * specified distance and angle constraints.
     */
    UPROPERTY(EditDefaultsOnly, Category = "ACF| Warp")
    bool bCheckWarpConditions = true;

    /**
     * Maximum distance at which warp will be performed (used for range-limited attacks).
     */
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bCheckWarpConditions), Category = "ACF| Warp")
    float maxWarpDistance = 500.f;

    /**
     * Minimum distance at which warp will be performed (prevents warping at point blank).
     */
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bCheckWarpConditions), Category = "ACF| Warp")
    float minWarpDistance = 10.f;

    /**
     * Maximum angle (in degrees) between the character and the target at which
     * warping is allowed (prevents warping to targets far off to the side).
     */
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bCheckWarpConditions), Category = "ACF| Warp")
    float maxWarpAngle = 270.f;

    /**
     * Enables continuous update of the warp target (e.g., for tracking moving enemies).
     * When false, the warp target is only computed once at the start.
     */
    UPROPERTY(EditDefaultsOnly, Category = "ACF| Warp")
    bool bContinuousUpdate = true;

    /**
     * Controls how quickly the character is pulled towards the warp target (higher
     * values = faster/more magnetic movement).
     */
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = bContinuousUpdate), Category = "ACF| Warp")
    float WarpMagnetismStrength = 1.0f;

    /**
     * Attempts to compute the warp transform for this attack.
     * - Returns true and outputs the transform if a valid target is found and within range.
     * - Returns false otherwise.
     */
    bool TryGetTransform(FTransform& outTranform) const;

private:
    /** Stores the current warp transform (used for motion warping). */
    FTransform warpTrans;

    /** Stores the current target component for warping (e.g., a socket on the enemy). */
    TObjectPtr<USceneComponent> currentTargetComp;

    /** Stores the montage reproduction type before this action started (so it can be restored later). */
    EMontageReproductionType storedReproType;
};