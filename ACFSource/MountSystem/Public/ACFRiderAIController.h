// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "CoreMinimal.h"

#include "ACFRiderAIController.generated.h"

/**
 *
 */
UCLASS()
class MOUNTSYSTEM_API AACFRiderAIController : public AAIController {
    GENERATED_BODY()

public:
    AACFRiderAIController();

protected:
    virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;

    virtual void OnPossess(APawn* InPawn) override;

    virtual void OnUnPossess() override;

    UPROPERTY(EditDefaultsOnly, Category = ACF)
    bool bOverrideControlWithMountRotation = true;

    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bOverrideControlWithMountRotation == true"), Category = ACF)
    FRotator OffsetRotCorrection;

    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bOverrideControlWithMountRotation == true"), Category = ACF)
    FRotator ClampMin;

    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bOverrideControlWithMountRotation == true"), Category = ACF)
    FRotator ClampMax;

    virtual void GetPlayerViewPoint(FVector& out_Location, FRotator& out_Rotation) const override;

private:
    void SetRiderCompReference();

    TObjectPtr<class UACFRiderComponent> riderComp;
};
