// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/ACFAccessory.h"
#include "NomadAccessory.generated.h"

// Forward declaration of the equipable item data asset class.
class UEquipableItemData;

// Define a log category for messages related to NomadAccessory.
DEFINE_LOG_CATEGORY_STATIC(LogNomadAccessory, Log, All);

/**
 * ANomadAccessory
 *
 * Represents an equippable accessory item in the game (such as rings, amulets, or trinkets).
 * Inherits from AACFAccessory and utilizes a data asset (UEquipableItemData) to configure its properties,
 * including sounds, attribute modifiers, and general item information.
 */
UCLASS()
class NOMADDEV_API ANomadAccessory : public AACFAccessory
{
    GENERATED_BODY()

public:
    // Constructor: Initializes the accessory object.
    ANomadAccessory();

    // Returns the thumbnail image for this accessory, used in the UI.
    virtual UTexture2D* GetThumbnailImage() const override;

    // Returns the display name of the accessory.
    virtual FText GetItemName() const override;

    // Returns the description of the accessory.
    virtual FText GetItemDescription() const override;

    // Returns the item type (e.g., accessory) as defined in the item information.
    virtual EItemType GetItemType() const override;

    // Returns a complete item descriptor containing all relevant details.
    virtual FItemDescriptor GetItemInfo() const override;

    // Returns an array of gameplay tags that indicate the valid equipment slots for this accessory.
    virtual TArray<FGameplayTag> GetPossibleItemSlots() const override;

protected:
    // BeginPlay: Called when the game starts or when the actor is spawned into the world.
    virtual void BeginPlay() override;

    // InitializeItem: Reads data from the AccessoryData asset and applies those properties to this accessory.
    void InitializeItem();

    // Data asset: Contains the settings and properties for this accessory.
    UPROPERTY(EditAnywhere, Category = "Item Data Asset")
    TObjectPtr<UEquipableItemData> AccessoryData;

    // Optional sound cue: Plays when the accessory is gathered or interacted with.
    UPROPERTY(BlueprintReadWrite, Category = "Item Shared Properties")
    TObjectPtr<USoundCue> GatherSound;
};