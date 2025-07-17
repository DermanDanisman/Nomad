// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/ACFItem.h"
#include <Engine/DataAsset.h>
#include "ACFCraftRecipeDataAsset.generated.h"

/**
 * FACFCraftingRecipe
 * A data structure representing one crafting recipe: what you need, what you get,
 * how much it costs, and how long it takes.
 */
USTRUCT(BlueprintType)
struct FACFCraftingRecipe : public FTableRowBase {
    GENERATED_BODY()

public:
    FACFCraftingRecipe() {};

    /** List of input items required to perform this craft. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | Recipe")
    TArray<FBaseItem> RequiredItems;

    /** The item produced by this recipe. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | Recipe")
    FBaseItem OutputItem;

    /** Monetary cost to craft this recipe. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | Recipe")
    float CraftingCost = 0.f;

    /** Time in seconds required to complete this craft. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF | Recipe")
    float CraftingTime = 0.f;

    //
    // Comparison operators - identify recipes by their OutputItem's class
    //

    /** Compare to another recipe by checking output item class equality. */
    FORCEINLINE bool operator!=(const FBaseItem& Other) const
    {
        return this->OutputItem.ItemClass != Other.ItemClass;
    }

    /** Inverse of ==. */
    FORCEINLINE bool operator==(const FBaseItem& Other) const
    {
        return this->OutputItem.ItemClass == Other.ItemClass;
    }

    /** Compare to a raw FBaseItem (treating it as the output). */
    FORCEINLINE bool operator!=(const TSubclassOf<class AACFItem>& Other) const
    {
        return this->OutputItem.ItemClass != Other;
    }

    FORCEINLINE bool operator==(const TSubclassOf<class AACFItem>& Other) const
    {
        return this->OutputItem.ItemClass == Other;
    }

    /** Compare to an item class directly. */
    FORCEINLINE bool operator!=(const FACFCraftingRecipe& Other) const
    {
        return this->OutputItem.ItemClass != Other.OutputItem.ItemClass;
    }

    FORCEINLINE bool operator==(const FACFCraftingRecipe& Other) const
    {
        return this->OutputItem.ItemClass == Other.OutputItem.ItemClass;
    }
};

/**
 * UACFCraftRecipeDataAsset
 * A Data Asset wrapper around a single FACFCraftingRecipe, so you can
 * author recipes directly in the Editor and reference them at runtime.
 */

UCLASS()
class CRAFTINGSYSTEM_API UACFCraftRecipeDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public: 

    /** Assigns a new recipe configuration to this asset. */
    UFUNCTION(BlueprintCallable, Category = "ACF | Recipe")
    void SetCraftingRecipe(const FACFCraftingRecipe& InRecipe)
    {
        RecipeConfig = InRecipe;
    }

    /** Retrieves the recipe configuration stored in this asset. */
    UFUNCTION(BlueprintPure, Category = "ACF | Recipe")
    FACFCraftingRecipe GetCraftingRecipe() const
    {
        return RecipeConfig;
    }

protected:
    /** The actual recipe data serialized by the Data Asset. */
    UPROPERTY(EditAnywhere, Category = "ACF | Recipe")
    FACFCraftingRecipe RecipeConfig;
	
};
