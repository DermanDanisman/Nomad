// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/Item/BaseItemData.h"
#include "Engine/DataAsset.h"
#include "CraftingMaterialData.generated.h"

USTRUCT(BlueprintType)
struct FCraftingMaterialInfo : public FBaseItemInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Static Mesh")
    TObjectPtr<UStaticMesh> StaticMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Crafting")
    FGameplayTag MaterialType; // e.g., "Material.Metal.Iron"
};

/**
 * 
 */
UCLASS(BlueprintType)
class NOMADDEV_API UCraftingMaterialData : public UDataAsset
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item Information")
    FCraftingMaterialInfo CraftingMaterialInfo;
	
};
