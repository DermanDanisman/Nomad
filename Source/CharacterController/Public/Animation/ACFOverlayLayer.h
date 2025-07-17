// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFAnimLayer.h"
#include "ACFAnimTypes.h"
#include "ACFCCTypes.h"
#include "Animation/AimOffsetBlendSpace.h"
#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"

#include "ACFOverlayLayer.generated.h"


/**
 *
 */
class UAnimSequence;
UCLASS()
class CHARACTERCONTROLLER_API UACFOverlayLayer : public UACFAnimLayer {
    GENERATED_BODY()

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | AimOffset")
    UAimOffsetBlendSpace* AimOffset;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACF | AimOffset")
    float AimOffsetAlpha = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | Overlay")
    FOverlayConfig IdleOverlay;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | Overlay")
    FOverlayConfig AimOverlay;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | Overlay")
    FOverlayConfig BlockOverlay;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | Overlay")
    FOverlayConfig CustomOverlay;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACF | Overlay")
    float OverlayBlendAlfa = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | Overlay")
    float DefaultSwitchTime = 0.7f;

    UPROPERTY(BlueprintReadOnly, Category = ACF)
    class UACFCharacterMovementComponent* MovementComp;

    UPROPERTY(BlueprintReadOnly, Category = ACF)
    EMovementStance currentOverlay;

    void SetReferences();

    void SetMovStance(const EMovementStance inOverlay);

    /* begin play */
    virtual void NativeInitializeAnimation() override;

    virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

    virtual void OnActivated_Implementation() override;
};
