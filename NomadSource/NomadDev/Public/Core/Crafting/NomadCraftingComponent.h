// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ACFCraftingComponent.h"
#include "Core/Data/Item/Crafting/CraftingStationData.h"
#include "NomadCraftingComponent.generated.h"

/**
 * 
 */
UCLASS()
class NOMADDEV_API UNomadCraftingComponent : public UACFCraftingComponent
{
    GENERATED_BODY()

public:

    // Initializes the crafting component with data from a crafting station data asset.
    UFUNCTION(BlueprintCallable, Category = "ACF | Initialization")
    void InitializeFromDataAsset(UCraftingStationData* CraftingStationData);
};