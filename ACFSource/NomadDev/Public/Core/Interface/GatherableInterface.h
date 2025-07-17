// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Core/Data/Item/Resource/PickupItemActorData.h"
#include "UObject/Interface.h"
#include "GatherableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGatherableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for gatherable items in the game
 */
class NOMADDEV_API IGatherableInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Gatherable Interface")
    FGameplayTag GetCollectionTag() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Gatherable Interface")
    FGameplayTag GetRequiredToolTag() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Gatherable Interface")
    void PerformGatherAction();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Gatherable Interface")
    void GetCharacterControlRotation(FRotator ControlRotation, FVector ForwardVector);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Gatherable Interface")
    bool GetGatherableActorDepleted() const;
};
