// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Item/NomadResourceItem.h"
#include "Core/Data/Item/Crafting/CraftingMaterialData.h"

// Constructor: Creates and attaches the necessary components.
ANomadResourceItem::ANomadResourceItem()
{
    // Create a default root component for the item.
    RootComponent = CreateDefaultSubobject<USceneComponent>("DefaultSceneRoot");

    // Create a static mesh component that represents the item's 3D model.
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");

    // Attach the mesh to the root component so it becomes part of the actor's hierarchy.
    Mesh->SetupAttachment(GetRootComponent());
}

void ANomadResourceItem::BeginPlay()
{
    Super::BeginPlay();
    // Initialize the item at runtime to apply all settings from the data asset.
    InitializeItem();
}

void ANomadResourceItem::InitializeItem()
{
    // Verify that the CraftingMaterialData asset is assigned; if not, log an error.
    ensureMsgf(CraftingMaterialData, TEXT("CraftingMaterialData asset is missing or invalid!"));

    // Proceed only if the data asset is valid.
    if (CraftingMaterialData)
    {
        // Retrieve the crafting material information from the data asset.
        const FCraftingMaterialInfo& Info = CraftingMaterialData->CraftingMaterialInfo;

        // ---------------------------
        // Mesh Setup
        // ---------------------------
        // If a static mesh is defined in the data asset, assign it to the Mesh component.
        if (Info.StaticMesh)
        {
            Mesh->SetStaticMesh(Info.StaticMesh);
        }
        else
        {
            // Log a warning if no static mesh is assigned.
            UE_LOG(LogNomadCraftingMaterial, Warning, TEXT("No Static Mesh assigned for Crafting Material: %s"), *CraftingMaterialData->GetName());
        }

        // ---------------------------
        // Material Type Setup
        // ---------------------------
        // Set the material type based on the data asset.
        MaterialType = Info.MaterialType;

        // ---------------------------
        // Sound Setup
        // ---------------------------
        // If a gather sound is specified, assign it; otherwise, log a warning.
        if (Info.GatherSound)
        {
            GatherSound = Info.GatherSound;
        }
        else
        {
            UE_LOG(LogNomadCraftingMaterial, Warning, TEXT("No GatherSound assigned for Crafting Material: %s"), *CraftingMaterialData->GetName());
        }

        // ---------------------------
        // Item Information Setup
        // ---------------------------
        // Assign general item information (name, description, etc.) from the data asset.
        ItemInfo = Info.ItemInfo;
    }
    else
    {
        // Log an error if the CraftingMaterialData asset is missing or invalid.
        UE_LOG(LogNomadCraftingMaterial, Error, TEXT("CraftingMaterialData asset is missing or invalid! -> %s"), *GetName());
    }
}

UTexture2D* ANomadResourceItem::GetThumbnailImage() const
{
    // Return the thumbnail image from the item information.
    return CraftingMaterialData ? CraftingMaterialData->CraftingMaterialInfo.ItemInfo.ThumbNail : nullptr;
}

FText ANomadResourceItem::GetItemName() const
{
    // Return the item name as defined in the data asset.
    return CraftingMaterialData ? CraftingMaterialData->CraftingMaterialInfo.ItemInfo.Name : FText::GetEmpty();
}

FText ANomadResourceItem::GetItemDescription() const
{
    // Return the item description as defined in the data asset.
    return CraftingMaterialData ? CraftingMaterialData->CraftingMaterialInfo.ItemInfo.Description : FText::GetEmpty();
}

EItemType ANomadResourceItem::GetItemType() const
{
    // Return the item type (e.g., crafting material) as defined in the data asset.
    return CraftingMaterialData ? CraftingMaterialData->CraftingMaterialInfo.ItemInfo.ItemType : EItemType::Default;
}

FItemDescriptor ANomadResourceItem::GetItemInfo() const
{
    // Return the complete item descriptor from the data asset.
    return CraftingMaterialData ? CraftingMaterialData->CraftingMaterialInfo.ItemInfo : FItemDescriptor();
}

TArray<FGameplayTag> ANomadResourceItem::GetPossibleItemSlots() const
{
    // Return the list of valid item slots for this item, as defined in the data asset.
    return CraftingMaterialData ? CraftingMaterialData->CraftingMaterialInfo.ItemInfo.GetPossibleItemSlots() : TArray<FGameplayTag>();
}