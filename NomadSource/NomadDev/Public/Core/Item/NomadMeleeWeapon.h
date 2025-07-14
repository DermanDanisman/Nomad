// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/ACFMeleeWeapon.h"
#include "NomadMeleeWeapon.generated.h"

// Forward declaration of the melee weapon data asset class.
class UMeleeWeaponData;

// Define a log category to use for logging messages related to the melee weapon.
DEFINE_LOG_CATEGORY_STATIC(LogNomadMeleeWeapon, Log, All);

/**
 * ANomadMeleeWeapon
 *
 * This class represents a melee weapon in the game.
 * It inherits from AACFMeleeWeapon, which is the base class for melee weapons.
 * The class initializes weapon properties like mesh, collision settings, handle type, animations, sounds, and other
 * effects by reading from a UMeleeWeaponData data asset.
 */
UCLASS()
class NOMADDEV_API ANomadMeleeWeapon : public AACFMeleeWeapon
{
    GENERATED_BODY()

public:
    // Constructor: Called when the weapon object is created to initialize its properties.
    ANomadMeleeWeapon();

    // Returns the thumbnail image for this weapon (used in UIs).
    virtual UTexture2D* GetThumbnailImage() const override;

    // Returns the display name of the weapon.
    virtual FText GetItemName() const override;

    // Returns the description of the weapon.
    virtual FText GetItemDescription() const override;

    // Returns the item type (e.g., melee weapon).
    virtual EItemType GetItemType() const override;

    // Returns the complete item descriptor with all details.
    virtual FItemDescriptor GetItemInfo() const override;

    // Returns an array of gameplay tags indicating the possible equipment slots for this weapon.
    virtual TArray<FGameplayTag> GetPossibleItemSlots() const override;

    // Returns an array of gameplay tags indicating the required tool tag for this weapon.
    UFUNCTION(BlueprintCallable)
    virtual TArray<FGameplayTag> GetRequiredToolTag() const;

protected:
    // BeginPlay: Called when the game starts or the actor is spawned.
    virtual void BeginPlay() override;

    // InitializeItem: Reads the melee weapon properties from the data asset and applies them to configure the weapon.
    UFUNCTION(BlueprintCallable, Category = Item)
    void InitializeItem();

    // Data asset containing all the settings and properties for this melee weapon.
    UPROPERTY(EditAnywhere, Category = "Item Data Asset")
    TObjectPtr<UMeleeWeaponData> MeleeWeaponData;

    // Optional sound cue for gathering actions (if applicable).
    UPROPERTY(BlueprintReadWrite, Category = "Item Shared Properties")
    TObjectPtr<USoundCue> GatherSound;
};