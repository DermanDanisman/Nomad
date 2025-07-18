// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/ACFMaterial.h"
#include "NomadResourceItem.generated.h"

// Forward declaration of the crafting material data asset class.
class UCraftingMaterialData;
class USoundCue;

// Define a log category for messages related to Nomad Crafting Material.
DEFINE_LOG_CATEGORY_STATIC(LogNomadCraftingMaterial, Log, All);

/**
 * ANomadResourceItem
 *
 * This class represents a crafting material item in the game.
 * It inherits from AACFMaterial, which is the base class for material items.
 * The item's properties (mesh, material type, and general item information) are defined via a
 * data asset (UCraftingMaterialData).
 */
UCLASS()
class NOMADDEV_API ANomadResourceItem : public AACFMaterial
{
    GENERATED_BODY()

public:
    // Constructor: Initializes components and sets up the item.
    ANomadResourceItem();

    // Returns the thumbnail image to display for the item in the UI.
    virtual UTexture2D* GetThumbnailImage() const override;

    // Returns the display name of the item.
    virtual FText GetItemName() const override;

    // Returns a descriptive text for the item.
    virtual FText GetItemDescription() const override;

    // Returns the type of the item (e.g., material).
    virtual EItemType GetItemType() const override;

    // Returns the complete item descriptor containing all relevant item info.
    virtual FItemDescriptor GetItemInfo() const override;

    // Returns an array of gameplay tags representing the possible equipment slots for this item.
    virtual TArray<FGameplayTag> GetPossibleItemSlots() const override;

protected:
    // BeginPlay: Called when the game starts or the actor is spawned.
    virtual void BeginPlay() override;

    // InitializeItem: Reads data from the CraftingMaterialData asset and applies it to this item.
    // Sets up the mesh, material type, sounds, and general item information.
    void InitializeItem();

    // Mesh component representing the visual model of the crafting material.
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Mesh = nullptr;

    // Gameplay tag representing the material type (e.g., "Item.Material.Metal.Iron").
    UPROPERTY(EditDefaultsOnly, Category = "Crafting")
    FGameplayTag MaterialType;

    // Data asset containing the settings and properties for this crafting material.
    UPROPERTY(EditAnywhere, Category = "Item Data Asset")
    TObjectPtr<UCraftingMaterialData> CraftingMaterialData = nullptr;

    // Optional sound cue to play when gathering or interacting with the material.
    UPROPERTY(BlueprintReadWrite, Category = "Item Shared Properties")
    TObjectPtr<USoundCue> GatherSound = nullptr;
};