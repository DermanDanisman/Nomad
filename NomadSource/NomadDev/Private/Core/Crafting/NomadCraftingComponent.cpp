// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.


#include "Core/Crafting/NomadCraftingComponent.h"

#include "ACFCraftRecipeDataAsset.h"

void UNomadCraftingComponent::InitializeFromDataAsset(UCraftingStationData* CraftingStationData)
{
    if (!CraftingStationData)
    {
        UE_LOG(LogTemp, Warning, TEXT("InitializeFromDataAsset called with null CraftingStationData"));
        return;
    }

    // Clear any existing recipes before adding new ones
    CraftableItems.Empty();
    ItemsRecipes.Empty();

    // Iterate over recipe assets from CraftingStationData
    for (UDataAsset* RecipeAsset : CraftingStationData->GetItemRecipes())
    {
        // Cast to your specific recipe data asset type
        if (UACFCraftRecipeDataAsset* CraftRecipe = Cast<UACFCraftRecipeDataAsset>(RecipeAsset))
        {
            // Store the recipe asset reference
            ItemsRecipes.Add(CraftRecipe);

            // Extract the actual recipe struct from the asset
            FACFCraftingRecipe Recipe = CraftRecipe->GetCraftingRecipe();

            // Add the recipe to the CraftableItems array in the base class
            AddNewRecipe(Recipe);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("NomadCraftingComponent initialized with %d recipes"), CraftableItems.Num());
}