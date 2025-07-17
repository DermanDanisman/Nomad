// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Item/NomadArmor.h"
#include "Core/Data/Item/Equipable/EquipableItemData.h"

// Constructor: Default constructor; no initialization of data is performed here.
// All configuration is done in InitializeItem().
ANomadArmor::ANomadArmor()
{
    // Empty constructor implementation.
}

void ANomadArmor::BeginPlay()
{
    // Called when the game starts or when the actor is spawned.
    Super::BeginPlay();
    
    // Initialize the armor's properties using the data asset.
    InitializeItem();
}

void ANomadArmor::InitializeItem()
{
    // Ensure that the ArmorData asset is valid; if not, log an error.
    ensureMsgf(ArmorData, TEXT("Armor Data IS NOT SET!"));

    if (ArmorData)
    {
        // Retrieve the equipable item information from the data asset for easier access.
        // This structure holds all shared properties for equippable items (e.g., sounds, attributes, and general info).
        const FEquipableItemInfo& Info = ArmorData->EquipableItemInfo;

        // ---------------------------
        // Mesh Setup
        // ---------------------------
        // If a skeletal mesh is specified in the data asset, apply it to the armor's mesh component.
        if (Info.SkeletalMesh)
        {
            MeshComp->SetSkeletalMesh(Info.SkeletalMesh);
        }
        // (Optional) You can add an else clause here if you wish to log a warning or provide a fallback.

        // ---------------------------
        // Equip Sounds
        // ---------------------------
        // Assign the sound to be played when the armor is equipped, if available.
        if (Info.EquipSound)
        {
            EquipSound = Info.EquipSound;
        }
        else
        {
            UE_LOG(LogNomadArmor, Warning, TEXT("No EquipSound assigned for armor: %s"), *ArmorData->GetName());
        }

        // Assign the sound to be played when the armor is unequipped, if available.
        if (Info.UnequipSound)
        {
            UnequipSound = Info.UnequipSound;
        }
        else
        {
            UE_LOG(LogNomadArmor, Warning, TEXT("No UnequipSound assigned for armor: %s"), *ArmorData->GetName());
        }

        // ---------------------------
        // Gather Sound
        // ---------------------------
        // Assign the gather sound if it's specified in the data asset.
        if (Info.GatherSound)
        {
            GatherSound = Info.GatherSound;
        }
        else
        {
            UE_LOG(LogNomadArmor, Warning, TEXT("No GatherSound assigned for armor: %s"), *ArmorData->GetName());
        }

        // ---------------------------
        // Attribute & Gameplay Logic
        // ---------------------------
        // If attribute requirements are defined, apply them to the armor.
        if (Info.PrimaryAttributesRequirement.Num() > 0)
        {
            PrimaryAttributesRequirement = Info.PrimaryAttributesRequirement;
        }
        else
        {
            UE_LOG(LogNomadArmor, Warning, TEXT("PrimaryAttributesRequirement is empty for armor: %s"), *ArmorData->GetName());
        }
        
        // Set any attribute modifiers that should be applied when the armor is equipped.
        AttributeModifier = Info.AttributeModifier;

        // ---------------------------
        // Gameplay Effect Modifier
        // ---------------------------
        // If a gameplay effect modifier is provided, assign it.
        if (Info.GameplayModifier)
        {
            GameplayModifier = Info.GameplayModifier;
        }
        else
        {
            UE_LOG(LogNomadArmor, Warning, TEXT("No GameplayModifier assigned for armor: %s"), *ArmorData->GetName());
        }

        // ---------------------------
        // Item Information Setup
        // ---------------------------
        // Assign the general item information (such as name, description, and thumbnail) from the data asset.
        ItemInfo = Info.ItemInfo;
    }
    else
    {
        // Log an error if the ArmorData asset is missing or invalid.
        UE_LOG(LogNomadArmor, Error, TEXT("ArmorData asset is missing or invalid! -> %s"), *GetName());
    }
}

UTexture2D* ANomadArmor::GetThumbnailImage() const
{
    // Return the thumbnail image from the armor's item information.
    return ArmorData->EquipableItemInfo.ItemInfo.ThumbNail;
}

FText ANomadArmor::GetItemName() const
{
    // Return the display name of the armor from the data asset.
    return ArmorData->EquipableItemInfo.ItemInfo.Name;
}

FText ANomadArmor::GetItemDescription() const
{
    // Return the item description from the data asset.
    return ArmorData->EquipableItemInfo.ItemInfo.Description;
}

EItemType ANomadArmor::GetItemType() const
{
    // Return the item type (e.g., armor) as defined in the data asset.
    return ArmorData->EquipableItemInfo.ItemInfo.ItemType;
}

FItemDescriptor ANomadArmor::GetItemInfo() const
{
    // Return the complete item descriptor from the data asset.
    return ArmorData->EquipableItemInfo.ItemInfo;
}

TArray<FGameplayTag> ANomadArmor::GetPossibleItemSlots() const
{
    // Return an array of gameplay tags representing the valid equipment slots for this armor.
    return ArmorData->EquipableItemInfo.ItemInfo.ItemSlots;
}