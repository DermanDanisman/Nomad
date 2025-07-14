// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/ACFRangedWeapon.h"
#include "NomadRangedWeapon.generated.h"

// Forward declaration of the ranged weapon data asset class.
class URangedWeaponData;

// Define a log category for logging messages related to the ranged weapon.
DEFINE_LOG_CATEGORY_STATIC(LogNomadRangedWeapon, Log, All);

/**
 * ANomadRangedWeapon
 *
 * This class represents a ranged weapon in the game. It inherits from AACFRangedWeapon, which provides
 * base functionality for ranged weapons. The class utilizes a data asset (URangedWeaponData) to initialize
 * its properties such as visuals, firing behavior, and effects.
 */
UCLASS()
class NOMADDEV_API ANomadRangedWeapon : public AACFRangedWeapon
{
    GENERATED_BODY()

public:
    // Constructor: Called when the weapon object is created.
    ANomadRangedWeapon();

    // Returns a thumbnail image used to represent the item in the UI.
    virtual UTexture2D* GetThumbnailImage() const override;

    // Returns the display name of the item.
    virtual FText GetItemName() const override;

    // Returns the description of the item.
    virtual FText GetItemDescription() const override;

    // Returns the type of the item (e.g., ranged weapon).
    virtual EItemType GetItemType() const override;

    // Returns a struct containing detailed item information.
    virtual FItemDescriptor GetItemInfo() const override;

    // Returns an array of gameplay tags representing the valid item slots for this weapon.
    virtual TArray<FGameplayTag> GetPossibleItemSlots() const override;

protected:
    // BeginPlay is called when the game starts or when the actor is spawned into the world.
    virtual void BeginPlay() override;

    // InitializeItem sets up the weapon's properties based on the data asset.
    // This function reads values from the RangedWeaponData asset and applies them to the weapon.
    UFUNCTION(BlueprintCallable, Category = Item)
    void InitializeItem();
    
    // Data asset containing all the settings and properties for this ranged weapon.
    UPROPERTY(EditAnywhere, Category = "Item Data Asset")
    TObjectPtr<URangedWeaponData> RangedWeaponData;

    // Optional sound cue for gathering (if applicable for a ranged weapon).
    UPROPERTY(BlueprintReadWrite, Category = "Item Shared Properties")
    TObjectPtr<USoundCue> GatherSound;
};