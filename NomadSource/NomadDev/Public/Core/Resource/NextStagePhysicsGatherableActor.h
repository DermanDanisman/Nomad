// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Resource/BaseGatherableActor.h"
#include "NextStagePhysicsGatherableActor.generated.h"

/**
 * APhysicsGatherableActor
 *
 * Spawns in physics-simulated mode, immediately fires an impulse
 * (tipping or radial), and after PhysicsSimulateDuration seconds
 * stops simulating physics.
 */
UCLASS()
class NOMADDEV_API APhysicsGatherableActor : public ABaseGatherableActor
{
    GENERATED_BODY()

public:
    APhysicsGatherableActor();

protected:
    virtual void BeginPlay() override;

    /** How long to let physics run before freezing. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics")
    float PhysicsSimulateDuration = 5.0f;

    /** Whether to apply an off-center tipping impulse. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics|Impulse")
    bool bApplyTippingImpulse = true;

    /** Factor to multiply mass by for the tipping impulse (Impulse = Mass * Factor). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics|Impulse", meta=(EditCondition="bApplyTippingImpulse"))
    float TippingImpulseFactor = 300.0f;

    /** Local-space direction for tipping (e.g. (1,0,-0.3) for forward + slight down). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics|Impulse", meta=(EditCondition="bApplyTippingImpulse"))
    FVector TippingDirection = FVector(1,0,-0.3f);

    /** Whether to apply a radial “explosion” impulse. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics|Impulse")
    bool bApplyRadialImpulse = false;

    /** Strength of radial impulse (raw magnitude). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics|Impulse", meta=(EditCondition="bApplyRadialImpulse"))
    float RadialImpulseStrength = 1500.0f;

    /** Radius over which radial impulse is applied. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics|Impulse", meta=(EditCondition="bApplyRadialImpulse"))
    float RadialImpulseRadius = 300.0f;

private:
    /** Handle to clear the physics timer if needed. */
    FTimerHandle StopPhysicsTimerHandle;

    /** Called when PhysicsSimulateDuration elapses. */
    UFUNCTION()
    void StopPhysics();
};
