// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MultiplayerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UMultiplayerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for multiplayer-specific functionality
 * Provides contract for objects that need multiplayer customization support
 */
class NOMADDEV_API IMultiplayerInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Customization|Application")
    void BP_ApplyCustomizationState();
};
