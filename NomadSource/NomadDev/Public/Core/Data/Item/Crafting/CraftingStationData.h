// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "CraftingStationData.generated.h"

USTRUCT(BlueprintType)
struct FCraftingStationInfo
{
    GENERATED_BODY()

    FCraftingStationInfo()
        : CraftingStationName(FText())
        , MarkerTexture(nullptr)
        , MarkerCategory(FGameplayTag())
        , MarkerName(NAME_None)
        , bShouldRotate(false)
        , bActivateWorldWidget(false)
    {}

    // Optional meshes to represent the crafting station visually
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Station | Visuals")
    TObjectPtr<USkeletalMesh> CraftingStationSkeletalMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Station | Visuals")
    TObjectPtr<UStaticMesh> CraftingStationStaticMesh;

    // Display name for the crafting station
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Station | Data")
    FText CraftingStationName;

    // Recipes available at this station
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Station | Recipes")
    TArray<UDataAsset*> ItemRecipes;

    // Map marker related properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Station | Map Marker")
    TObjectPtr<UTexture2D> MarkerTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Station | Map Marker")
    FGameplayTag MarkerCategory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Station | Map Marker")
    FName MarkerName;

    // Should the station rotate in the world?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Station | Map Marker")
    bool bShouldRotate;

    // Should the world widget be active at this station?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Station | Map Marker")
    bool bActivateWorldWidget;
};

/**
 *
 */
UCLASS()
class NOMADDEV_API UCraftingStationData : public UDataAsset
{
    GENERATED_BODY()

public:

    UCraftingStationData()
        : CraftingStationInfo()
    {}

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Station Information")
    FCraftingStationInfo CraftingStationInfo;

    // Getters (const, to avoid modification)
    UFUNCTION(BlueprintCallable, Category = "Crafting Station")
    FText GetCraftingStationName() const { return CraftingStationInfo.CraftingStationName; }

    UFUNCTION(BlueprintCallable, Category = "Crafting Station")
    const TArray<UDataAsset*>& GetItemRecipes() const { return CraftingStationInfo.ItemRecipes; }

    UFUNCTION(BlueprintCallable, Category = "Crafting Station")
    UTexture2D* GetMarkerTexture() const { return CraftingStationInfo.MarkerTexture.Get(); }

    UFUNCTION(BlueprintCallable, Category = "Crafting Station")
    FGameplayTag GetMarkerCategory() const { return CraftingStationInfo.MarkerCategory; }

    UFUNCTION(BlueprintCallable, Category = "Crafting Station")
    FName GetMarkerName() const { return CraftingStationInfo.MarkerName; }

    UFUNCTION(BlueprintCallable, Category = "Crafting Station")
    bool ShouldRotate() const { return CraftingStationInfo.bShouldRotate; }

    UFUNCTION(BlueprintCallable, Category = "Crafting Station")
    bool ShouldActivateWorldWidget() const { return CraftingStationInfo.bActivateWorldWidget; }

    UFUNCTION(BlueprintCallable, Category = "Crafting Station")
    USkeletalMesh* GetSkeletalMesh() const { return CraftingStationInfo.CraftingStationSkeletalMesh.Get(); }

    UFUNCTION(BlueprintCallable, Category = "Crafting Station")
    UStaticMesh* GetStaticMesh() const { return CraftingStationInfo.CraftingStationStaticMesh.Get(); }
};
