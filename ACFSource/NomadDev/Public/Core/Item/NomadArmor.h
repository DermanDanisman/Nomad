// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/ACFArmor.h"
#include "NomadArmor.generated.h"

// Forward declaration of the data asset class used to configure the armor.
class UEquipableItemData;

// Define a log category for messages related to NomadArmor.
DEFINE_LOG_CATEGORY_STATIC(LogNomadArmor, Log, All);

/**
 * ANomadArmor
 *
 * Represents an equippable armor item in the game.
 * Inherits from AACFArmor and uses a data asset (UEquipableItemData) to drive its configuration.
 * This class sets up armor properties (mesh, sounds, attributes, etc.) using the provided data asset.
 */
UCLASS()
class NOMADDEV_API ANomadArmor : public AACFArmor
{
    GENERATED_BODY()

public:
    // Constructor: Initializes the armor object.
    ANomadArmor();

    // Returns the thumbnail image for the armor (used in UI).
    virtual UTexture2D* GetThumbnailImage() const override;

    // Returns the display name of the armor.
    virtual FText GetItemName() const override;

    // Returns the description of the armor.
    virtual FText GetItemDescription() const override;

    // Returns the item type (e.g., armor).
    virtual EItemType GetItemType() const override;

    // Returns the complete item descriptor with all relevant details.
    virtual FItemDescriptor GetItemInfo() const override;

    // Returns an array of gameplay tags indicating valid equipment slots for this armor.
    virtual TArray<FGameplayTag> GetPossibleItemSlots() const override;

protected:
    // BeginPlay: Called when the game starts or the actor is spawned into the world.
    virtual void BeginPlay() override;

    // InitializeItem: Reads data from the ArmorData asset and applies the properties to the armor.
    // This function sets up the mesh, sounds, attribute modifiers, and general item information.
    void InitializeItem();

    // ArmorData is the data asset containing all the settings for this armor item.
    UPROPERTY(EditAnywhere, Category = "Item Data Asset")
    TObjectPtr<UEquipableItemData> ArmorData;

    // Optional sound cue for gathering actions.
    UPROPERTY(BlueprintReadWrite, Category = "Item Shared Properties")
    TObjectPtr<USoundCue> GatherSound;
};