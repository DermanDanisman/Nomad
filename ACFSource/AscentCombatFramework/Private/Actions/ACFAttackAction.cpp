// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Actions/ACFAttackAction.h"
#include "ATSBaseTargetComponent.h"
#include "ATSTargetPointComponent.h"
#include "Actions/ACFBaseAction.h"
#include "Actors/ACFCharacter.h"
#include "Animation/ACFAnimInstance.h"
#include "Components/ACFActionsManagerComponent.h"
#include "Components/ACFCharacterMovementComponent.h"
#include "Components/ActorComponent.h"
#include "Game/ACFFunctionLibrary.h"
#include "Interfaces/ACFEntityInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MotionWarpingComponent.h"
#include "RootMotionModifier.h"
#include <ATSBaseTargetComponent.h>
#include <GameFramework/CharacterMovementComponent.h>
#include <GameFramework/Controller.h>
#include <Kismet/KismetMathLibrary.h>
#include <GameFramework/Actor.h>

/*
 * Implementation for UACFAttackAction (Warping, Damage Trace Activation, Targeting)
 * ------------------------------------------------------------------------------
 * This file contains the implementation of UACFAttackAction's runtime logic.
 * Warping handles the character's auto-movement/alignment to the target for attacks
 * (e.g., lunges, cinematic moves). Damage is only activated during specific animation windows.
 */

UACFAttackAction::UACFAttackAction()
{
    // By default, activate both left and right attack traces.
    DamageToActivate = EDamageActivationType::EBoth;
    // Use motion warping as the default montage reproduction type for attacks.
    ActionConfig.MontageReproductionType = EMontageReproductionType::EMotionWarped;
}

bool UACFAttackAction::TryGetTransform(FTransform& outTranform) const
{
    // Try to find a valid target and compute the transform we'll warp to.
    if (!IsValid(CharacterOwner) || !IsValid(CharacterOwner->GetController()))
    {
        return false;
    }
    // Look for a targeting component on the controller (e.g., lock-on).
    const UATSBaseTargetComponent* targetComp = CharacterOwner->GetController()->FindComponentByClass<UATSBaseTargetComponent>();
    if (targetComp)
    {
        AActor* target = targetComp->GetCurrentTarget();
        if (target)
        {
            // Get the "extent" (radius) of the target actor for correct spacing.
            const float entityExtent = IACFEntityInterface::Execute_GetEntityExtentRadius(target);
            FVector ownerLoc = CharacterOwner->GetActorLocation();
            // Ensure warp is level with the target (ignore height difference).
            ownerLoc.Z = target->GetActorLocation().Z;
            const FVector diffVector = target->GetActorLocation() - ownerLoc;
            const float distance = CharacterOwner->GetDistanceTo(target);
            // Compute the final warp distance (subtract target's radius).
            const float warpDistance = distance - entityExtent;
            // Compute the point in the direction of the target, at the correct distance.
            const FVector finalPos = UACFFunctionLibrary::GetPointAtDirectionAndDistanceFromActor(CharacterOwner, diffVector, warpDistance, false);
            // Look at the warp point (keep pitch and roll zero for stability).
            FRotator finalRot = UKismetMathLibrary::FindLookAtRotation(CharacterOwner->GetActorLocation(), finalPos);
            finalRot.Roll = 0.f;
            finalRot.Pitch = 0.f;
            outTranform = FTransform(finalRot, finalPos);
            return true;
        }
    }
    return false;
}

void UACFAttackAction::OnTick_Implementation(float DeltaTime)
{
    // If continuous warping is enabled, this continuously updates the warp target
    // so the character tracks moving enemies (mostly on standalone).
    if (bContinuousUpdate && CharacterOwner && CharacterOwner->GetNetMode() == NM_Standalone &&
        ActionConfig.MontageReproductionType == EMontageReproductionType::EMotionWarped)
    {
        UMotionWarpingComponent* motionComp = CharacterOwner->FindComponentByClass<UMotionWarpingComponent>();
        if (motionComp)
        {
            FTransform targetPoint;
            if (TryGetTransform(targetPoint))
            {
                // Interpolate towards the new warp target smoothly (magnetism).
                warpTrans.SetRotation(FMath::QInterpTo(warpTrans.GetRotation(), targetPoint.GetRotation(), DeltaTime, WarpMagnetismStrength));
                warpTrans.SetLocation(FMath::VInterpTo(warpTrans.GetLocation(), targetPoint.GetLocation(), DeltaTime, WarpMagnetismStrength));
                if (ActionConfig.WarpInfo.bShowWarpDebug)
                {
                    UKismetSystemLibrary::DrawDebugSphere(CharacterOwner, warpTrans.GetLocation(), 100.f, 12, FLinearColor::Yellow, 1.f, 1.f);
                }
                // Update the warp target in the animation system.
                const FMotionWarpingTarget newTarget = FMotionWarpingTarget(ActionConfig.WarpInfo.SyncPoint, warpTrans);
                motionComp->AddOrUpdateWarpTarget(newTarget);
            }
        }
    }
    Super::OnTick_Implementation(DeltaTime);
}

USceneComponent* UACFAttackAction::GetWarpTargetComponent_Implementation()
{
    // Returns the current target component for warping (if any).
    return currentTargetComp;
}

FTransform UACFAttackAction::GetWarpTransform_Implementation()
{
    // Returns the current warp transform for this attack (used by the animation system).
    return warpTrans;
}

void UACFAttackAction::OnActionStarted_Implementation(const FString& contextString, AActor* InteractedActor, FGameplayTag ItemSlotTag)
{
    Super::OnActionStarted_Implementation();
    // Store the montage reproduction type so it can be restored later.
    storedReproType = ActionConfig.MontageReproductionType;
    // If using warping and have a target, calculate whether warp is valid and set up motion warping.
    if (CharacterOwner && bCheckWarpConditions && CharacterOwner->GetController() &&
        ActionConfig.MontageReproductionType == EMontageReproductionType::EMotionWarped)
    {
        const UMotionWarpingComponent* motionComp = CharacterOwner->FindComponentByClass<UMotionWarpingComponent>();
        const UATSBaseTargetComponent* targetComp = CharacterOwner->GetController()->FindComponentByClass<UATSBaseTargetComponent>();
        if (motionComp && targetComp && animMontage)
        {
            AActor* target = targetComp->GetCurrentTarget();
            IACFEntityInterface* entity = Cast<IACFEntityInterface>(target);
            IACFEntityInterface* ownerEntity = Cast<IACFEntityInterface>(CharacterOwner);
            if (target && entity && CharacterOwner)
            {
                const float entityExtent = IACFEntityInterface::Execute_GetEntityExtentRadius(target);
                FVector ownerLoc = CharacterOwner->GetActorLocation();
                ownerLoc.Z = target->GetActorLocation().Z;
                const FVector diffVector = target->GetActorLocation() - ownerLoc;
                const float distance = CharacterOwner->GetDistanceTo(target);
                const float warpDistance = distance - entityExtent;
                const FVector finalPos = UACFFunctionLibrary::GetPointAtDirectionAndDistanceFromActor(CharacterOwner, diffVector, warpDistance, ActionConfig.WarpInfo.bShowWarpDebug);
                FRotator finalRot = UKismetMathLibrary::FindLookAtRotation(CharacterOwner->GetActorLocation(), target->GetActorLocation());
                finalRot.Pitch = 0.f;
                finalRot.Roll = 0.f;
                // Compute the rotation difference (for warp angle limiting).
                const FRotator deltaRot = finalRot - CharacterOwner->GetActorForwardVector().Rotation();
                // Only allow warping if within allowed distance and angle.
                if (maxWarpDistance > warpDistance && warpDistance > minWarpDistance && maxWarpAngle > FMath::Abs(deltaRot.Yaw))
                {
                    warpTrans = FTransform(finalRot, finalPos);
                    // Set the target component (e.g., a specific socket if needed).
                    currentTargetComp = targetComp->GetCurrentTargetPoint();
                    SetMontageReproductionType(EMontageReproductionType::EMotionWarped);
                } else
                {
                    // Fallback: use root motion if warping is not suitable.
                    SetMontageReproductionType(EMontageReproductionType::ERootMotion);
                }
            } else
            {
                SetMontageReproductionType(EMontageReproductionType::ERootMotion);
            }
        }
    }
}

void UACFAttackAction::OnActionEnded_Implementation()
{
    // At the end of the attack, disable all active damage traces.
    AACFCharacter* acfCharacter = Cast<AACFCharacter>(CharacterOwner);
    if (acfCharacter && ActionsManager)
    {
        acfCharacter->DeactivateDamage(DamageToActivate, TraceChannels);
    }
    // Restore the previous montage reproduction type.
    ActionConfig.MontageReproductionType = storedReproType;
    Super::OnActionEnded_Implementation();
}

void UACFAttackAction::OnSubActionStateEntered_Implementation()
{
    Super::OnSubActionStateEntered_Implementation();
    // When entering a sub-action state (e.g., attack window), activate damage traces.
    AACFCharacter* acfCharacter = Cast<AACFCharacter>(CharacterOwner);
    if (acfCharacter && ActionsManager)
    {
        acfCharacter->ActivateDamage(DamageToActivate, TraceChannels);
    }
}

void UACFAttackAction::OnSubActionStateExited_Implementation()
{
    Super::OnSubActionStateExited_Implementation();
    // When exiting a sub-action state, deactivate damage traces.
    AACFCharacter* acfCharacter = Cast<AACFCharacter>(CharacterOwner);
    if (acfCharacter && ActionsManager)
    {
        acfCharacter->DeactivateDamage(DamageToActivate, TraceChannels);
    }
}