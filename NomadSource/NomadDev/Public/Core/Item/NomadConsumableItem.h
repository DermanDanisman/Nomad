// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/ACFConsumable.h"
#include "NomadConsumableItem.generated.h"

// Forward declaration of the consumable data asset class.
class UConsumableData;

// Define a log category for messages related to Nomad Consumable items.
DEFINE_LOG_CATEGORY_STATIC(LogNomadConsumable, Log, All);

/**
 * ANomadConsumableItem
 *
 * This class represents a consumable item in the game.
 * It inherits from AACFConsumable, which is the base class for consumable items.
 * This class handles initialization and setup of consumable item properties
 * (such as mesh, effects, and stat modifiers) using a consumable data asset.
 */
UCLASS()
class NOMADDEV_API ANomadConsumableItem : public AACFConsumable
{
    GENERATED_BODY()

public:
    // Constructor: Initializes components and sets up the item.
    ANomadConsumableItem();

    // Returns the thumbnail image to display in the UI.
    virtual UTexture2D* GetThumbnailImage() const override;

    // Returns the display name of the consumable item.
    virtual FText GetItemName() const override;

    // Returns the description of the consumable item.
    virtual FText GetItemDescription() const override;

    // Returns the type of the item (e.g., consumable).
    virtual EItemType GetItemType() const override;

    // Returns the complete item descriptor with all relevant details.
    virtual FItemDescriptor GetItemInfo() const override;

    // Returns an array of gameplay tags representing the valid item slots for this consumable.
    virtual TArray<FGameplayTag> GetPossibleItemSlots() const override;

protected:
    // BeginPlay: Called when the game starts or the actor is spawned.
    virtual void BeginPlay() override;

    // InitializeItem: Sets up the consumable item's properties using data from the data asset.
    // This method reads values such as mesh, effects, and stat modifiers from the asset.
    void InitializeItem();

    // Mesh component: Visual representation of the consumable item (e.g., a potion bottle).
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Mesh;

    // Data asset holding the properties and settings for this consumable item.
    UPROPERTY(EditAnywhere, Category = "Item Data Asset")
    TObjectPtr<UConsumableData> ConsumableItemData;

    // Optional sound cue that may play when the item is gathered.
    UPROPERTY(BlueprintReadWrite, Category = "Item Shared Properties")
    TObjectPtr<USoundCue> GatherSound;
};