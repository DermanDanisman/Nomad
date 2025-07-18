// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Item/NomadConsumableItem.h"

#include "ARSStatisticsComponent.h"
#include "Core/Data/Item/Consumable/ConsumableData.h"
#include "GameFramework/Character.h"

// Constructor: Initializes the consumable item and sets up its component hierarchy.
ANomadConsumableItem::ANomadConsumableItem()
{
    // Create a default root component for the item.
    RootComponent = CreateDefaultSubobject<USceneComponent>("DefaultSceneRoot");

    // Create the static mesh component representing the consumable.
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");

    // Attach the Mesh to the root component so it becomes part of this actor's hierarchy.
    Mesh->SetupAttachment(GetRootComponent());
}

// BeginPlay: Called when the game starts or the actor is spawned into the world.
void ANomadConsumableItem::BeginPlay()
{
    // Call base BeginPlay implementation.
    Super::BeginPlay();

    // Initialize consumable item properties at runtime.
    InitializeItem();
}

// InitializeItem: Configures the consumable item's properties by reading from the data asset.
void ANomadConsumableItem::InitializeItem()
{
    // Ensure that the ConsumableItemData asset is assigned; if not, log an error.
    ensureMsgf(ConsumableItemData, TEXT("Consumable Item Data IS NOT SET!"));
    
    // Only proceed if the ConsumableItemData asset is valid.
    if (ConsumableItemData)
    {
        // Retrieve the consumable item info from the data asset for easier access.
        const FConsumableItemInfo& Info = ConsumableItemData->ConsumableItemInfo;
        
        // ---------------------------
        // Mesh Setup
        // ---------------------------
        // If a static mesh is defined in the data asset, set it on the Mesh component.
        if (Info.StaticMesh)
        {
            Mesh->SetStaticMesh(Info.StaticMesh);
        }
        else
        {
            // Log a warning if no static mesh is provided.
            UE_LOG(LogNomadConsumable, Warning, TEXT("No Static Mesh assigned for Consumable Item: %s"), *ConsumableItemData->GetName());
        }
        
        // ---------------------------
        // Effect Setup
        // ---------------------------
        // Assign the OnUsedEffect from the data asset, defining the effect when the item is used.
        OnUsedEffect = Info.OnUsedEffect;

        // ---------------------------
        // Sound Setup
        // ---------------------------
        // Set the gather sound if it's specified in the data asset.
        if (Info.GatherSound)
        {
            GatherSound = Info.GatherSound;
        }
        else
        {
            // Log a warning if no gather sound is assigned.
            UE_LOG(LogNomadConsumable, Warning, TEXT("No GatherSound assigned for Consumable Item: %s"), *ConsumableItemData->GetName());
        }

        // ---------------------------
        // Interaction Setup
        // ---------------------------
        // Check and assign the desired use action (e.g., "Use", "Consume") for the consumable.
        if (Info.DesiredUseAction.IsValid())
        {
            DesiredUseAction = Info.DesiredUseAction;
        }
        else
        {
            // Log a warning if no desired use action is provided.
            UE_LOG(LogNomadConsumable, Warning, TEXT("No Desired Use Action assigned for Consumable Item: %s"), *ConsumableItemData->GetName());
        }

        // ---------------------------
        // Stat Modifiers Setup
        // ---------------------------
        // If there are stat modifiers defined in the asset, assign them.
        if (Info.StatModifier.Num() > 0)
        {
            StatModifier = Info.StatModifier;
        }
        else
        {
            // Log a warning if no stat modifiers are provided.
            UE_LOG(LogNomadConsumable, Warning, TEXT("No Stat Modifier assigned for Consumable Item: %s"), *ConsumableItemData->GetName());
        }

        // ---------------------------
        // Timed Attribute Modifiers Setup
        // ---------------------------
        // If timed attribute set modifiers exist, assign them.
        if (Info.TimedAttributeSetModifier.Num() > 0)
        {
            TimedAttributeSetModifier = Info.TimedAttributeSetModifier;
        }
        else
        {
            // Log a warning if no timed attribute modifiers are provided.
            UE_LOG(LogNomadConsumable, Warning, TEXT("No Timed Attribute Set Modifier assigned for Consumable Item: %s"), *ConsumableItemData->GetName());
        }

        // ---------------------------
        // Gameplay Effect Setup
        // ---------------------------
        // If a consumable gameplay effect is provided, assign it.
        if (Info.ConsumableGameplayEffect)
        {
            ConsumableGameplayEffect = Info.ConsumableGameplayEffect;
        }
        else
        {
            // Log a warning if no gameplay effect is assigned.
            UE_LOG(LogNomadConsumable, Warning, TEXT("No Consumable Gameplay Effect assigned for Consumable Item: %s"), *ConsumableItemData->GetName());
        }

        // ---------------------------
        // Item Information Setup
        // ---------------------------
        // Assign general item information (name, description, etc.) from the data asset.
        ItemInfo = Info.ItemInfo;
    }
    else
    {
        // Log an error if the consumable data asset is missing or invalid.
        UE_LOG(LogNomadConsumable, Error, TEXT("ConsumableItemData asset is missing or invalid! -> %s"), *GetName());
    }
}

UTexture2D* ANomadConsumableItem::GetThumbnailImage() const
{
    // Return the thumbnail image defined in the item info, checking if ConsumableItemData is valid.
    return ConsumableItemData ? ConsumableItemData->ConsumableItemInfo.ItemInfo.ThumbNail : nullptr;
}

FText ANomadConsumableItem::GetItemName() const
{
    // Return the display name of the item from the data asset.
    return ConsumableItemData ? ConsumableItemData->ConsumableItemInfo.ItemInfo.Name : FText::GetEmpty();
}

FText ANomadConsumableItem::GetItemDescription() const
{
    // Return the description of the item from the data asset.
    return ConsumableItemData ? ConsumableItemData->ConsumableItemInfo.ItemInfo.Description : FText::GetEmpty();
}

EItemType ANomadConsumableItem::GetItemType() const
{
    // Return the item type (e.g., consumable) as defined in the data asset.
    return ConsumableItemData ? ConsumableItemData->ConsumableItemInfo.ItemInfo.ItemType : EItemType::Default;
}

FItemDescriptor ANomadConsumableItem::GetItemInfo() const
{
    // Return the complete item descriptor from the data asset.
    return ConsumableItemData ? ConsumableItemData->ConsumableItemInfo.ItemInfo : FItemDescriptor();
}

TArray<FGameplayTag> ANomadConsumableItem::GetPossibleItemSlots() const
{
    // Return an array of gameplay tags indicating the valid slots for this item, checking if ConsumableItemData is valid.
    return ConsumableItemData ? ConsumableItemData->ConsumableItemInfo.ItemInfo.GetPossibleItemSlots() : TArray<FGameplayTag>();
}
}