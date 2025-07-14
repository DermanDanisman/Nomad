// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Item/NomadAccessory.h"
#include "Core/Data/Item/Equipable/EquipableItemData.h"

// Constructor: Initializes the accessory object. No dynamic initialization is done here.
ANomadAccessory::ANomadAccessory()
{
    // Constructor left empty as initialization is performed later in InitializeItem().
}

void ANomadAccessory::BeginPlay()
{
    // Called when the game starts or the actor is spawned.
    Super::BeginPlay();
    // Initialize the accessory properties from the data asset at runtime.
    InitializeItem();
}

void ANomadAccessory::InitializeItem()
{
    // Ensure that the AccessoryData asset is assigned; log an error if not.
    ensureMsgf(AccessoryData, TEXT("Accessory Data IS NOT SET!"));

    if (AccessoryData)
    {
        // Retrieve the equipable item information from the data asset.
        const FEquipableItemInfo& Info = AccessoryData->EquipableItemInfo;

        // ---------------------------
        // Equip Sounds
        // ---------------------------
        // If an equip sound is defined in the data asset, assign it.
        if (Info.EquipSound)
        {
            EquipSound = Info.EquipSound;
        }
        else
        {
            // Log a warning if no equip sound is provided.
            UE_LOG(LogNomadAccessory, Warning, TEXT("No EquipSound assigned for Accessory: %s"), *AccessoryData->GetName());
        }

        // If an unequip sound is defined in the data asset, assign it.
        if (Info.UnequipSound)
        {
            UnequipSound = Info.UnequipSound;
        }
        else
        {
            // Log a warning if no unequip sound is provided.
            UE_LOG(LogNomadAccessory, Warning, TEXT("No UnequipSound assigned for Accessory: %s"), *AccessoryData->GetName());
        }

        // ---------------------------
        // Gather Sound
        // ---------------------------
        // If a gather sound is specified in the data asset, assign it.
        if (Info.GatherSound)
        {
            GatherSound = Info.GatherSound;
        }
        else
        {
            // Log a warning if no gather sound is provided.
            UE_LOG(LogNomadAccessory, Warning, TEXT("No GatherSound assigned for Accessory: %s"), *AccessoryData->GetName());
        }
        
        // ---------------------------
        // Attribute & Gameplay Logic
        // ---------------------------
        // If attribute requirements are defined, copy them to the accessory.
        if (Info.PrimaryAttributesRequirement.Num() > 0)
        {
            PrimaryAttributesRequirement = Info.PrimaryAttributesRequirement;
        }
        else
        {
            // Log a warning if the primary attribute requirements array is empty.
            UE_LOG(LogNomadAccessory, Warning, TEXT("PrimaryAttributesRequirement is empty for Accessory: %s"), *AccessoryData->GetName());
        }
        
        // Copy the attribute modifier from the data asset.
        AttributeModifier = Info.AttributeModifier;
        
        // If a gameplay modifier is defined, assign it.
        if (Info.GameplayModifier)
        {
            GameplayModifier = Info.GameplayModifier;
        }
        else
        {
            // Log a warning if no gameplay modifier is provided.
            UE_LOG(LogNomadAccessory, Warning, TEXT("No GameplayModifier assigned for Accessory: %s"), *AccessoryData->GetName());
        }

        // ---------------------------
        // Item Information Setup
        // ---------------------------
        // Copy general item information (e.g., name, description, thumbnail) from the data asset.
        ItemInfo = Info.ItemInfo;
    }
    else
    {
        // Log an error if the AccessoryData asset is missing or invalid.
        UE_LOG(LogNomadAccessory, Error, TEXT("AccessoryData asset is missing or invalid! -> %s"), *GetName());
    }
}

UTexture2D* ANomadAccessory::GetThumbnailImage() const
{
    // Return the thumbnail image from the item info.
    return AccessoryData->EquipableItemInfo.ItemInfo.ThumbNail;
}

FText ANomadAccessory::GetItemName() const
{
    // Return the name of the accessory from the item info.
    return AccessoryData->EquipableItemInfo.ItemInfo.Name;
}

FText ANomadAccessory::GetItemDescription() const
{
    // Return the item description from the data asset.
    return AccessoryData->EquipableItemInfo.ItemInfo.Description;
}

EItemType ANomadAccessory::GetItemType() const
{
    // Return the type of the item (e.g., accessory) as defined in the item info.
    return AccessoryData->EquipableItemInfo.ItemInfo.ItemType;
}

FItemDescriptor ANomadAccessory::GetItemInfo() const
{
    // Return the complete item descriptor from the data asset.
    return AccessoryData->EquipableItemInfo.ItemInfo;
}

TArray<FGameplayTag> ANomadAccessory::GetPossibleItemSlots() const
{
    // Return an array of gameplay tags indicating the valid slots for this accessory.
    return AccessoryData->EquipableItemInfo.ItemInfo.GetPossibleItemSlots();
}