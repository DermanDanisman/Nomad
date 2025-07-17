// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "AGSGraphEdge.h"
#include "CoreMinimal.h"
#include <GameplayTagContainer.h>

#include "ACFTransition.generated.h"


/**
 *
 */
UCLASS()
class ASCENTCOMBOGRAPH_API UACFTransition : public UAGSGraphEdge {
    GENERATED_BODY()

protected:
    UPROPERTY(EditDefaultsOnly, Category = ASM)
    UInputAction* TransitionInput;

public:
    UFUNCTION(BlueprintPure, Category = ACF)
    UInputAction* GetTransitionInput() const
    {
        return TransitionInput;
    }
};
