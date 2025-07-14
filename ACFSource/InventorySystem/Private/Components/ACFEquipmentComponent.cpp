// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Components/ACFEquipmentComponent.h"

// Include various dependencies used by this component.
#include <GameFramework/CharacterMovementComponent.h>
#include <GameplayTagContainer.h>
#include <Kismet/KismetSystemLibrary.h>
#include <NavigationSystem.h>

#include "ACFItemSystemFunctionLibrary.h"
#include "ARSStatisticsComponent.h"
#include "Components/ACFArmorSlotComponent.h"
#include "Components/ACFStorageComponent.h"
#include "GameFramework/Character.h"
#include "Items/ACFAccessory.h"
#include "Items/ACFArmor.h"
#include "Items/ACFConsumable.h"
#include "Items/ACFEquippableItem.h"
#include "Items/ACFItem.h"
#include "Items/ACFMeleeWeapon.h"
#include "Items/ACFProjectile.h"
#include "Items/ACFRangedWeapon.h"
#include "Items/ACFWeapon.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include <GameFramework/Actor.h>

//---------------------------------------------------------------------
// GetLifetimeReplicatedProps
//---------------------------------------------------------------------
void UACFEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    // Call the base class function to include base properties for replication.
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    // Replicate the Equipment struct (which stores equipped items).
    DOREPLIFETIME(UACFEquipmentComponent, Equipment);
    // Replicate the entire Inventory array.
    DOREPLIFETIME(UACFEquipmentComponent, Inventory);
    // Replicate the current total weight of all inventory items.
    DOREPLIFETIME(UACFEquipmentComponent, currentInventoryWeight);
    // Replicate the tag of the currently equipped slot.
    DOREPLIFETIME(UACFEquipmentComponent, CurrentlyEquippedSlotType);

    DOREPLIFETIME_CONDITION_NOTIFY(UACFEquipmentComponent, ActiveQuickbarEnum, COND_None, REPNOTIFY_Always);
}

//---------------------------------------------------------------------
// Constructor
//---------------------------------------------------------------------
UACFEquipmentComponent::UACFEquipmentComponent()
{
    // Disable per-frame ticking for better performance if it's not needed.
    PrimaryComponentTick.bCanEverTick = false;
    // Enable replication on this component.
    SetIsReplicatedByDefault(true);
    // Clear the Inventory array initially.
    Inventory.Empty();
}

//---------------------------------------------------------------------
// BeginPlay
//---------------------------------------------------------------------
void UACFEquipmentComponent::BeginPlay()
{
    // Called when the game starts or the actor is spawned.
    Super::BeginPlay();

    // Cache the owning character by calling a helper function.
    GatherCharacterOwner();
}

//---------------------------------------------------------------------
// GatherCharacterOwner
//---------------------------------------------------------------------
void UACFEquipmentComponent::GatherCharacterOwner()
{
    // If CharacterOwner is not valid, attempt to cast the owning actor to ACharacter.
    if (!CharacterOwner)
    {
        CharacterOwner = Cast<ACharacter>(GetOwner());
    }
}

//---------------------------------------------------------------------
// EndPlay
//---------------------------------------------------------------------
void UACFEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // If the component is removed from the world, sheath (store away) the current weapon.
    if (EndPlayReason == EEndPlayReason::RemovedFromWorld)
    {
        SheathCurrentWeapon();
    }
    // Call the base class EndPlay to finish cleanup.
    Super::EndPlay(EndPlayReason);
}

//---------------------------------------------------------------------
// OnComponentLoaded_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::OnComponentLoaded_Implementation()
{
    // Destroy any currently equipped items.
    DestroyEquipment();

    // Clear the equipped items from the Equipment struct.
    Equipment.EquippedItems.Empty();
    // Loop through each item in the inventory.
    for (auto& slot : Inventory)
    {
        // Refresh the item descriptor in case the underlying data has been updated.
        slot.RefreshDescriptor();
        // Update item info by reading from the data asset (through a helper function).
        UACFItemSystemFunctionLibrary::GetItemData(slot.ItemClass, slot.ItemInfo);
        // If the item is marked as equipped, re-equip it.
        if (slot.bIsEquipped)
        {
            EquipInventoryItem(slot);
        }
    }

    UpdateEquippedItemsVisibility();
    // Update the total weight value for the inventory.
    RefreshTotalWeight();
}

//---------------------------------------------------------------------
// AddItemToInventory_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::AddItemToInventory_Implementation(const FBaseItem& ItemToAdd, bool bAutoEquip)
{
    // Internally add the item to the inventory.
    Internal_AddItem(ItemToAdd, bAutoEquip);
}

//---------------------------------------------------------------------
// AddItemToInventoryByClass_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::AddItemToInventoryByClass_Implementation(TSubclassOf<AACFItem> inItem, int32 count /*= 1*/, bool bAutoEquip)
{
    // Create a FBaseItem from the class and count, then add it to the inventory.
    AddItemToInventory(FBaseItem(inItem, count), bAutoEquip);
}

//---------------------------------------------------------------------
// RemoveItemByIndex_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::RemoveItemByIndex_Implementation(const int32 index, int32 count /*= 1*/)
{
    // If the index exists in the inventory array, remove that item by count.
    if (Inventory.IsValidIndex(index))
    {
        RemoveItem(Inventory[index], count);
    }
}

//---------------------------------------------------------------------
// DropItem_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::DropItem_Implementation(const FInventoryItem& item, int32 count /*= 1*/)
{
    // Get a pointer to the inventory item based on its GUID.
    FInventoryItem* itemptr = Internal_GetInventoryItem(item);
    if (!itemptr)
    {
        return; // Cannot drop if item pointer is invalid.
    }

    // Check if the item is droppable according to its item info.
    if (item.ItemInfo.bDroppable)
    {
        // Create an array to hold the item(s) to drop.
        TArray<FBaseItem> toDrop;
        toDrop.Add(FBaseItem(item.ItemClass, count));
        // Spawn a world item near the owner based on the drop list.
        SpawnWorldItem(toDrop);

        // Remove the dropped items from the inventory.
        RemoveItem(item, count);
    }
}

//---------------------------------------------------------------------
// RemoveItem_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::RemoveItem_Implementation(const FInventoryItem& item, int32 count /*= 1*/)
{
    // Get a pointer to the inventory item using its GUID.
    FInventoryItem* itemptr = Internal_GetInventoryItem(item);
    if (itemptr)
    {
        // Determine the actual number of items to remove (cannot exceed available count).
        const int32 finalCount = FMath::Min(count, itemptr->Count);
        // Calculate the total weight that will be removed.
        const float weightRemoved = finalCount * itemptr->ItemInfo.ItemWeight;
        // Decrement the count.
        itemptr->Count -= finalCount;

        // If the item count drops to 0 or below...
        if (itemptr->Count <= 0)
        {
            // If the item is currently equipped, remove it from equipment.
            if (itemptr->bIsEquipped)
            {
                FEquippedItem outItem;
                GetEquippedItemSlot(itemptr->EquipmentSlot, outItem);
                RemoveItemFromEquipment(outItem);
            }
            // Then remove the item from the inventory array.
            FInventoryItem toBeRemoved;
            if (GetItemByGuid(itemptr->GetItemGuid(), toBeRemoved))
            {
                Inventory.Remove(toBeRemoved);
            }
        } else
        {
            // If item is still equipped, update the equipped count in the equipment structure.
            if (itemptr->bIsEquipped && Equipment.EquippedItems.Contains(item.EquipmentSlot))
            {
                const int32 index = Equipment.EquippedItems.IndexOfByKey(item.EquipmentSlot);
                Equipment.EquippedItems[index].InventoryItem.Count = itemptr->Count;
                RefreshEquipment();
                OnEquipmentChanged.Broadcast(Equipment);
            }
        }
        // Subtract the removed weight from the current inventory weight.
        currentInventoryWeight -= weightRemoved;
        // Broadcast that items have been removed.
        OnItemRemoved.Broadcast(FBaseItem(item.ItemClass, finalCount));
        // Broadcast the updated inventory.
        OnInventoryChanged.Broadcast(Inventory);
    }
}

//---------------------------------------------------------------------
// ToggleEquipInventoryItem_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::ToggleEquipInventoryItem_Implementation(const FInventoryItem& item, bool bIsSuccessful)
{
    if (bIsSuccessful)
    {
        FInventoryItem invItem;
        // Retrieve the inventory item by its GUID.
        if (GetItemByGuid(item.GetItemGuid(), invItem))
        {
            // If the item is not equipped, equip it.
            if (!invItem.bIsEquipped)
            {
                EquipInventoryItem(invItem);
            }
            // Otherwise, unequip it.
            else
            {
                UnequipItemBySlot(invItem.EquipmentSlot);
            }
        }
    }
}

//---------------------------------------------------------------------
// ToggleEquipItemByIndex_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::ToggleEquipItemByIndex_Implementation(int32 index)
{
    // If the index is valid, use the corresponding inventory item.
    if (Inventory.IsValidIndex(index))
    {
        FInventoryItem item = Inventory[index];
        ToggleEquipInventoryItem(item, true);
    }
}

//---------------------------------------------------------------------
// HasEnoughItemsOfType
//---------------------------------------------------------------------
bool UACFEquipmentComponent::HasEnoughItemsOfType(const TArray<FBaseItem>& ItemsToCheck)
{
    // Loop through each item type that we need to check.
    for (const auto& item : ItemsToCheck)
    {
        int32 numberToCheck = item.Count;
        // Find all inventory items of this specific class.
        TArray<FInventoryItem*> invItems = FindItemsByClass(item.ItemClass);
        int32 TotItems = 0;
        // Sum up all counts of this item type.
        for (const auto& invItem : invItems)
        {
            if (invItem)
            {
                TotItems += invItem->Count;
            }
        }
        // If the total count is less than required, return false.
        if (TotItems < numberToCheck)
        {
            return false;
        }
    }
    return true;
}

//---------------------------------------------------------------------
// GetCurrentDesiredMovesetTag
//---------------------------------------------------------------------
FGameplayTag UACFEquipmentComponent::GetCurrentDesiredMovesetTag() const
{
    // If a secondary weapon exists and overrides the main-hand moveset, use its moveset tag.
    if (Equipment.SecondaryWeapon && Equipment.SecondaryWeapon->OverridesMainHandMoveset())
    {
        return Equipment.SecondaryWeapon->GetAssociatedMovesetTag();
    }
    // Otherwise, if a main weapon exists, use its moveset tag.
    if (Equipment.MainWeapon)
    {
        return Equipment.MainWeapon->GetAssociatedMovesetTag();
    }
    // Return an empty tag if none is set.
    return FGameplayTag();
}

//---------------------------------------------------------------------
// GetCurrentDesiredMovesetActionTag
//---------------------------------------------------------------------
FGameplayTag UACFEquipmentComponent::GetCurrentDesiredMovesetActionTag() const
{
    // Similar logic: prefer the secondary weapon's moveset action tag if it overrides the main-hand actions.
    if (Equipment.SecondaryWeapon && Equipment.SecondaryWeapon->OverridesMainHandMovesetActions())
    {
        return Equipment.SecondaryWeapon->GetAssociatedMovesetActionsTag();
    }
    if (Equipment.MainWeapon)
    {
        return Equipment.MainWeapon->GetAssociatedMovesetActionsTag();
    }
    return FGameplayTag();
}

//---------------------------------------------------------------------
// GetCurrentDesiredOverlayTag
//---------------------------------------------------------------------
FGameplayTag UACFEquipmentComponent::GetCurrentDesiredOverlayTag() const
{
    // Check if the secondary weapon provides a moveset overlay override.
    if (Equipment.SecondaryWeapon && Equipment.SecondaryWeapon->OverridesMainHandOverlay())
    {
        return Equipment.SecondaryWeapon->GetAssociatedMovesetOverlayTag();
    }
    if (Equipment.MainWeapon)
    {
        return Equipment.MainWeapon->GetAssociatedMovesetOverlayTag();
    }
    return FGameplayTag();
}

//---------------------------------------------------------------------
// ConsumeItems_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::ConsumeItems_Implementation(const TArray<FBaseItem>& ItemsToCheck)
{
    // Loop through each item type that should be consumed.
    for (const auto& item : ItemsToCheck)
    {
        // Find all inventory items of that item type.
        const TArray<FInventoryItem*> invItems = FindItemsByClass(item.ItemClass);
        // If at least one exists, remove the specified count.
        if (invItems.IsValidIndex(0))
        {
            RemoveItem(*(invItems[0]), item.Count);
        }
    }
}

//---------------------------------------------------------------------
// MoveItemsToInventory_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::MoveItemsToInventory_Implementation(const TArray<FBaseItem>& inItems, UACFStorageComponent* storage)
{
    // Check if the provided storage component is valid.
    if (!storage)
    {
        UE_LOG(LogTemp, Error,
            TEXT("Invalid Storage, verify that the owner of this component is repliacted! - ACFEquipmentComp"));
        return;
    }

    TArray<FBaseItem> pendingRemove;
    // Loop through each item to be moved.
    for (const auto& item : inItems)
    {
        // Determine how many items can be added based on weight and stack limits.
        const int32 numItems = UKismetMathLibrary::Min(NumberOfItemCanTake(item.ItemClass), item.Count);
        // Add the item(s) to the inventory.
        AddItemToInventoryByClass(item.ItemClass, numItems, true);
        // Record the number removed for later removal from storage.
        pendingRemove.Add(FBaseItem(item.ItemClass, numItems));
    }
    // Remove the items from the storage component.
    storage->RemoveItems(pendingRemove);
}

//---------------------------------------------------------------------
// OnRep_Equipment
//---------------------------------------------------------------------
void UACFEquipmentComponent::OnRep_Equipment()
{
    // When equipment changes are replicated, refresh the equipment display and notify listeners.
    RefreshEquipment();
    OnEquipmentChanged.Broadcast(Equipment);
}

//---------------------------------------------------------------------
// RefreshEquipment
//---------------------------------------------------------------------
void UACFEquipmentComponent::RefreshEquipment()
{
    // Ensure that CharacterOwner is valid; if not, attempt to cast GetOwner().
    if (!CharacterOwner)
    {
        CharacterOwner = Cast<ACharacter>(GetOwner());
    }
    // Update the modular meshes based on the equipped armor slots.
    FillModularMeshes();
    // Loop over each equipped item in the Equipment struct.
    for (const auto& item : Equipment.EquippedItems)
    {
        // Attempt to cast the equipped item to an equippable item.
        AACFEquippableItem* equippable = Cast<AACFEquippableItem>(item.Item);
        if (equippable)
        {
            // Try casting to a weapon.
            AACFWeapon* WeaponToEquip = Cast<AACFWeapon>(equippable);
            if (WeaponToEquip)
            {
                // If the weapon is already assigned as main or secondary, skip further processing.
                if (WeaponToEquip == Equipment.MainWeapon || WeaponToEquip == Equipment.SecondaryWeapon)
                {
                    continue;
                }
                // Otherwise, attach the weapon to the body (e.g., sheathed position).
                AttachWeaponOnBody(WeaponToEquip);
            }

            // If the item is armor, hide the actor and add its skeletal mesh component.
            AACFArmor* ArmorToEquip = Cast<AACFArmor>(equippable);
            if (ArmorToEquip)
            {
                ArmorToEquip->SetActorHiddenInGame(true);
                AddSkeletalMeshComponent(ArmorToEquip->GetClass(), item.ItemSlot);
            }
            // If the item is a projectile, hide it.
            AACFProjectile* proj = Cast<AACFProjectile>(equippable);
            if (proj)
            {
                proj->SetActorHiddenInGame(true);
            }

            // If the item is an accessory, attach it to the main character mesh at its designated socket.
            AACFAccessory* itemToEquip = Cast<AACFAccessory>(equippable);
            if (itemToEquip)
            {
                itemToEquip->AttachToComponent(MainCharacterMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, itemToEquip->GetAttachmentSocket());
            }
        }
    }
}

//---------------------------------------------------------------------
// RefreshTotalWeight
//---------------------------------------------------------------------
void UACFEquipmentComponent::RefreshTotalWeight()
{
    // Reset current inventory weight.
    currentInventoryWeight = 0.f;
    // Sum up the weight of all items in inventory.
    for (const auto& item : Inventory)
    {
        currentInventoryWeight += item.ItemInfo.ItemWeight * item.Count;
    }
}

//---------------------------------------------------------------------
// ShouldUseLeftHandIK
//---------------------------------------------------------------------
bool UACFEquipmentComponent::ShouldUseLeftHandIK() const
{
    // If a main weapon exists, return whether it uses left-hand IK positioning.
    if (Equipment.MainWeapon)
    {
        return Equipment.MainWeapon->IsUsingLeftHandIK();
    }
    return false;
}

//---------------------------------------------------------------------
// GetLeftHandIkPos
//---------------------------------------------------------------------
FVector UACFEquipmentComponent::GetLeftHandIkPos() const
{
    // If a main weapon exists, get its left-hand IK position.
    if (Equipment.MainWeapon)
    {
        return Equipment.MainWeapon->GetLeftHandleIKPosition();
    }
    return FVector(); // Return zero vector if not applicable.
}

//---------------------------------------------------------------------
// IsSlotAvailable
//---------------------------------------------------------------------
bool UACFEquipmentComponent::IsSlotAvailable(const FGameplayTag& itemSlot) const
{
    // Return false if the tag is empty.
    if (itemSlot == FGameplayTag())
    {
        return false;
    }

    // Check if the tag is a valid item slot.
    if (!UACFItemSystemFunctionLibrary::IsValidItemSlotTag(itemSlot))
    {
        UE_LOG(LogTemp, Log,
            TEXT("Invalid item Slot Tag!!! -  UACFEquipmentComponent::IsSlotAvailable"));
        return false;
    }
    // Return true if the slot is not already occupied and exists in the available equipment slots.
    return !Equipment.EquippedItems.Contains(itemSlot) && GetAvailableEquipmentSlot().Contains(itemSlot);
}

//---------------------------------------------------------------------
// TryFindAvailableItemSlot
//---------------------------------------------------------------------
bool UACFEquipmentComponent::TryFindAvailableItemSlot(const TArray<FGameplayTag>& itemSlots, FGameplayTag& outAvailableSlot)
{
    // Loop through the provided item slots.
    for (const auto& slot : itemSlots)
    {
        // If the slot is available, assign it to outAvailableSlot and return true.
        if (IsSlotAvailable(slot))
        {
            outAvailableSlot = slot;
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------
// HaveAtLeastAValidSlot
//---------------------------------------------------------------------
bool UACFEquipmentComponent::HaveAtLeastAValidSlot(const TArray<FGameplayTag>& itemSlots)
{
    // Check if any of the provided slots exist in the available equipment slots.
    for (const auto& slot : itemSlots)
    {
        if (GetAvailableEquipmentSlot().Contains(slot))
        {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------
// OnRep_Inventory
//---------------------------------------------------------------------

/* A function edited by Nomad Dev Team
 * OnRep_Inventory - Detect inventory changes and broadcast add/remove events
 */
void UACFEquipmentComponent::OnRep_Inventory()
{
    // Compare old cached inventory with new replicated inventory to detect differences
    HandleInventoryChanges(CachedInventory, Inventory);

    // Update the cached inventory
    CachedInventory = Inventory;

    // Broadcast the generic inventory changed event
    OnInventoryChanged.Broadcast(Inventory);
}

//---------------------------------------------------------------------
// FillModularMeshes
//---------------------------------------------------------------------
void UACFEquipmentComponent::FillModularMeshes()
{
    // Get all armor slot components from the owning actor.
    TArray<UACFArmorSlotComponent*> slots;
    GetOwner()->GetComponents<UACFArmorSlotComponent>(slots, false);
    // Clear the current modular meshes array.
    ModularMeshes.Empty();
    // For each armor slot component, create a FModularPart and set its leader pose to MainCharacterMesh.
    for (const auto slot : slots)
    {
        ModularMeshes.Add(FModularPart(slot));
        slot->SetLeaderPoseComponent(MainCharacterMesh);
    }
}

//---------------------------------------------------------------------
// OnEntityOwnerDeath_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::OnEntityOwnerDeath_Implementation()
{
    // When the owning character dies, check if items should be dropped.
    if (CharacterOwner && bDropItemsOnDeath)
    {
        TArray<AActor*> attachedActors;
        // Get all actors attached to the owning character.
        CharacterOwner->GetAttachedActors(attachedActors, true);
        TArray<FBaseItem> projCount;
        // Iterate over each attached actor.
        for (const auto& actor : attachedActors)
        {
            if (IsValid(actor) && !actor->IsPendingKillPending())
            {
                // Try to cast the actor to a projectile.
                AACFProjectile* proj = Cast<AACFProjectile>(actor);
                if (IsValid(proj))
                {
                    // Generate a random percentage.
                    const float percentage = FMath::RandRange(0.f, 100.f);
                    // If the projectile is droppable on death and its drop chance meets the random criteria...
                    if (proj->ShouldBeDroppedOnDeath() && proj->GetDropOnDeathPercentage() >= percentage)
                    {
                        // Check if this projectile type is already recorded.
                        if (projCount.Contains(proj->GetClass()))
                        {
                            FBaseItem* actualItem = projCount.FindByKey(proj->GetClass());
                            // Increment the count.
                            actualItem->Count += 1;
                        } else
                        {
                            // Add a new base item for this projectile.
                            projCount.Add(FBaseItem(proj->GetClass(), 1));
                        }
                    }
                    // Set the projectile to a short lifespan so it is destroyed soon.
                    proj->SetLifeSpan(0.2f);
                }
            }
        }
        // Spawn world items near the owner's feet using the collected drop data.
        UACFItemSystemFunctionLibrary::SpawnWorldItemNearLocation(this, projCount, CharacterOwner->GetCharacterMovement()->GetActorFeetLocation(), 100.f);
    }
    // If items should be destroyed on death, call the function to destroy equipped items.
    if (bDestroyItemsOnDeath)
    {
        DestroyEquippedItems();
    }
}

//---------------------------------------------------------------------
// Internal_OnArmorUnequipped_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::Internal_OnArmorUnequipped_Implementation(const FGameplayTag& slot)
{
    FModularPart outMesh;
    FEquippedItem outEquip;

    // Check if a modular mesh exists for the given equipment slot.
    if (GetModularMesh(slot, outMesh) && outMesh.meshComp)
    {
        // Reset the mesh component in this slot to its default (empty) state.
        outMesh.meshComp->ResetSlotToEmpty();
        // Broadcast an event to notify that armor has been unequipped.
        OnEquippedArmorChanged.Broadcast(slot);
    }
}

//---------------------------------------------------------------------
// CanBeEquipped
//---------------------------------------------------------------------
bool UACFEquipmentComponent::CanBeEquipped(const TSubclassOf<AACFItem>& equippable)
{
    FItemDescriptor ItemData;
    TArray<FAttribute> attributes;
    // Retrieve item data from the data asset for the given equippable item.
    UACFItemSystemFunctionLibrary::GetItemData(equippable, ItemData);

    // Ensure the owning character is valid.
    GatherCharacterOwner();
    // Check if the character has at least one valid slot for this item.
    if (!HaveAtLeastAValidSlot(ItemData.GetPossibleItemSlots()))
    {
        UE_LOG(LogTemp, Log, TEXT("No VALID item slots! Impossible to equip! - ACFEquipmentComp"));
        return false;
    }
    // If the item has attribute requirements, verify that the character meets them.
    if (UACFItemSystemFunctionLibrary::GetEquippableAttributeRequirements(equippable, attributes))
    {
        const UARSStatisticsComponent* statcomp = CharacterOwner->FindComponentByClass<UARSStatisticsComponent>();
        if (statcomp)
        {
            return statcomp->CheckPrimaryAttributesRequirements(attributes);
        }
        UE_LOG(LogTemp, Log,
            TEXT("Add UARSStatisticsComponent to your character!! - ACFEquipmentComp"));
    } else
    {
        // No attribute requirements, so it can be equipped.
        return true;
    }
    return false;
}

//---------------------------------------------------------------------
// Internal_AddItem
//---------------------------------------------------------------------
int32 UACFEquipmentComponent::Internal_AddItem(const FBaseItem& itemToAdd, bool bTryToEquip /*= true*/, float dropChancePercentage /*= 0.f*/)
{
    int32 addeditemstotal = 0;
    int32 addeditemstmp = 0;
    bool bSuccessful = false;
    FItemDescriptor itemData;

    // Retrieve the item data (e.g., weight, max stack) for the item to add.
    UACFItemSystemFunctionLibrary::GetItemData(itemToAdd.ItemClass, itemData);

    // Ensure MaxInventoryStack is not zero to avoid division errors.
    if (itemData.MaxInventoryStack == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Max Inventory Stack cannot be 0!!!! - UACFEquipmentComponent::Internal_AddItem"));
        return -1;
    }

    // Check if the current inventory weight already exceeds the limit.
    if (currentInventoryWeight >= MaxInventoryWeight)
    {
        return -1;
    }
    const int32 itemweight = itemData.ItemWeight;
    int32 maxAddableByWeightTotal = itemToAdd.Count;
    // Calculate maximum items that can be added based on weight remaining.
    if (itemweight > 0)
    {
        maxAddableByWeightTotal = FMath::TruncToInt((MaxInventoryWeight - currentInventoryWeight) / itemweight);
    }
    int32 count = itemToAdd.Count;
    // Limit the count to the maximum allowed by weight.
    if (maxAddableByWeightTotal < itemToAdd.Count)
    {
        count = maxAddableByWeightTotal;
    }
    if (count <= 0)
    {
        return -1;
    }
    int32 MaxInventoryStack = itemData.MaxInventoryStack;
    // Find existing inventory items of the same class.
    TArray<FInventoryItem*> outItems = FindItemsByClass(itemToAdd.ItemClass);

    // If there are existing items of the same type, try to add to existing stacks.
    bool bGate = true;
    if (outItems.Num() > 0)
    {
        for (const auto& outItem : outItems)
        {
            if (outItem->Count < itemData.MaxInventoryStack)
            {
                if (outItem->Count + count <= itemData.MaxInventoryStack && count * itemData.ItemWeight + currentInventoryWeight <= MaxInventoryWeight)
                {
                    // If adding the full count does not exceed stack or weight limits.
                    addeditemstmp = count;
                } else
                {
                    // Otherwise, add as much as possible.
                    int32 maxAddableByStack = itemData.MaxInventoryStack - outItem->Count;
                    addeditemstmp = maxAddableByStack;
                }

                // Increase the count in the existing stack.
                outItem->Count += addeditemstmp;
                addeditemstotal += addeditemstmp;
                // Decrease the remaining count to add.
                count -= addeditemstmp;
                // Update the drop chance.
                outItem->DropChancePercentage = dropChancePercentage;
                // If the item is equipped, update its count in the equipment array.
                if (outItem->bIsEquipped && Equipment.EquippedItems.Contains(outItem->EquipmentSlot))
                {
                    int32 index = Equipment.EquippedItems.IndexOfByKey(outItem->EquipmentSlot);
                    Equipment.EquippedItems[index].InventoryItem.Count = outItem->Count;
                    OnEquipmentChanged.Broadcast(Equipment);
                }
                // Otherwise, if auto-equip is enabled and not currently equipped in that slot, equip it.
                else if (bTryToEquip && !Equipment.EquippedItems.Contains(outItem->EquipmentSlot))
                {
                    EquipInventoryItem(*outItem);
                }
                bSuccessful = true;
                // Optionally break out if we've fulfilled the count.
            }
        }
    }

    // If additional items still remain after adding to existing stacks, create new stacks.
    const int32 NumberOfItemNeed = FMath::CeilToInt(static_cast<float>(count) / static_cast<float>(itemData.MaxInventoryStack));
    const int32 FreeSpaceInInventory = MaxInventorySlots - Inventory.Num();
    const int32 NumberOfStackToCreate = FGenericPlatformMath::Min(NumberOfItemNeed, FreeSpaceInInventory);
    for (int i = 0; i < NumberOfStackToCreate; i++)
    {
        if (Inventory.Num() < MaxInventorySlots)
        {
            FInventoryItem newItem(itemToAdd);
            // Limit new stack to the maximum stack size.
            if (count > itemData.MaxInventoryStack)
            {
                newItem.Count = itemData.MaxInventoryStack;
            } else
            {
                newItem.Count = count;
            }
            newItem.DropChancePercentage = dropChancePercentage;
            // Set the inventory index for the new stack.
            newItem.InventoryIndex = GetFirstEmptyInventoryIndex();
            addeditemstotal += newItem.Count;
            count -= newItem.Count;
            // Add the new item stack to the inventory.
            Inventory.Add(newItem);
            FGameplayTag outTag;
            // If auto-equip is enabled and an available slot is found, equip the item.
            if (bTryToEquip && TryFindAvailableItemSlot(newItem.ItemInfo.GetPossibleItemSlots(), outTag))
            {
                EquipInventoryItem(newItem);
            }
            bSuccessful = true;
        }
    }
    // If any items were added successfully...
    if (bSuccessful)
    {
        // Increase the current inventory weight by the weight of the items added.
        currentInventoryWeight += itemData.ItemWeight * addeditemstotal;
        // Broadcast that the inventory has changed.
        OnInventoryChanged.Broadcast(Inventory);
        if (addeditemstotal > 0)
        {
            // Broadcast that an item was added.
            OnItemAdded.Broadcast(FBaseItem(itemToAdd.ItemClass, addeditemstotal));
        }
        return addeditemstotal;
    }

    // Return the total number of items added.
    return addeditemstotal;
}

/* A function added by Nomad Dev Team
 * Helper function to compare inventories and broadcast add/remove events
 */
void UACFEquipmentComponent::HandleInventoryChanges(const TArray<FInventoryItem>& OldInventory, const TArray<FInventoryItem>& NewInventory)
{
    // Detect added items
    for (const FInventoryItem& NewItem : NewInventory)
    {
        const int32* OldCountPtr = nullptr;
        for (const FInventoryItem& OldItem : OldInventory)
        {
            if (OldItem.GetItemGuid() == NewItem.GetItemGuid())
            {
                OldCountPtr = &OldItem.Count;
                break;
            }
        }

        if (!OldCountPtr)
        {
            // Item is completely new - broadcast add event with full count
            OnItemAdded.Broadcast(FBaseItem(NewItem.ItemClass, NewItem.Count));
        }
        else if (*OldCountPtr < NewItem.Count)
        {
            // Count increased - broadcast add event for difference
            OnItemAdded.Broadcast(FBaseItem(NewItem.ItemClass, NewItem.Count - *OldCountPtr));
        }
    }

    // Detect removed items
    for (const FInventoryItem& OldItem : OldInventory)
    {
        const int32* NewCountPtr = nullptr;
        for (const FInventoryItem& NewItem : NewInventory)
        {
            if (OldItem.GetItemGuid() == NewItem.GetItemGuid())
            {
                NewCountPtr = &NewItem.Count;
                break;
            }
        }

        if (!NewCountPtr)
        {
            // Item was completely removed - broadcast remove event with old count
            OnItemRemoved.Broadcast(FBaseItem(OldItem.ItemClass, OldItem.Count));
        }
        else if (*NewCountPtr < OldItem.Count)
        {
            // Count decreased - broadcast remove event for difference
            OnItemRemoved.Broadcast(FBaseItem(OldItem.ItemClass, OldItem.Count - *NewCountPtr));
        }
    }
}

//---------------------------------------------------------------------
// SetInventoryItemSlotIndex_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::SetInventoryItemSlotIndex_Implementation(const FInventoryItem& item, int newIndex)
{
    // Check if the new index is within the allowed inventory slot range.
    if (newIndex < MaxInventorySlots)
    {
        if (Inventory.Contains(item))
        {
            // Retrieve the inventory item pointer.
            FInventoryItem* invItem = Internal_GetInventoryItem(item);
            if (invItem->InventoryIndex != newIndex)
            {
                // If the new slot is empty, assign the index directly.
                if (IsSlotEmpty(newIndex))
                {
                    invItem->InventoryIndex = newIndex;
                }
                else
                {
                    // Otherwise, swap indices between items.
                    FInventoryItem itemTemp;
                    if (GetItemByInventoryIndex(newIndex, itemTemp))
                    {
                        /**
                          * Edited by Nomad Dev team:
                          * When swapping inventory slots, instead of manually setting indices,
                          * call SwapInventoryItems to ensure both UI and data stay consistent.
                          */
                        SwapInventoryItems(itemTemp.InventoryIndex, invItem->InventoryIndex);
                        //FInventoryItem* itemDestination = Internal_GetInventoryItem(itemTemp);
                        //itemDestination->InventoryIndex = invItem->InventoryIndex;
                        //invItem->InventoryIndex = newIndex;
                    }
                }
            }
        }
    }
}

//---------------------------------------------------------------------
// SwapInventoryItems
//---------------------------------------------------------------------

void UACFEquipmentComponent::SwapInventoryItems_Implementation(int32 indexA, int32 indexB)
{
    if (!Inventory.IsValidIndex(indexA) || !Inventory.IsValidIndex(indexB) || indexA == indexB)
    {
        return;
    }

    // Swap positions in the array
    Inventory.Swap(indexA, indexB);

    // Update their InventoryIndex values to match their new array positions
    Inventory[indexA].InventoryIndex = indexA;
    Inventory[indexB].InventoryIndex = indexB;

    // Notify UI and listeners
    OnInventoryChanged.Broadcast(Inventory);
}

//---------------------------------------------------------------------
// FindItemsByClass
//---------------------------------------------------------------------
TArray<FInventoryItem*> UACFEquipmentComponent::FindItemsByClass(const TSubclassOf<AACFItem>& itemToFind)
{
    TArray<FInventoryItem*> foundItems;

    if (!IsValid(this))
    {
        UE_LOG(LogTemp, Error, TEXT("UACFEquipmentComponent is invalid!"));
        return foundItems;
    }

    if (!Inventory.IsValidIndex(0) && Inventory.Num() > 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Inventory invalid but has Num > 0! Possible corruption."));
        return foundItems;
    }

    int32 InvNum = Inventory.Num(); // safer to cache

    for (int32 i = 0; i < InvNum; i++)
    {
        UClass* invClass = Inventory[i].ItemClass;
        if (!invClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("Null ItemClass in Inventory[%d]"), i);
            continue;
        }
        if (invClass == itemToFind)
        {
            foundItems.Add(&Inventory[i]);
        }
    }
    return foundItems;
}

//---------------------------------------------------------------------
// BeginDestroy
//---------------------------------------------------------------------
void UACFEquipmentComponent::BeginDestroy()
{
    // Optionally, you can call Internal_DestroyEquipment() to destroy equipment items.
    // Internal_DestroyEquipment();
    Super::BeginDestroy();
}

//---------------------------------------------------------------------
// AttachWeaponOnBody
//---------------------------------------------------------------------
void UACFEquipmentComponent::AttachWeaponOnBody(AACFWeapon* WeaponToEquip)
{
    if (MainCharacterMesh)
    {
        // Get the socket name where the weapon should be attached on the character's body.
        const FName socket = WeaponToEquip->GetOnBodySocketName();

        if (socket != NAME_None)
        {
            // Attach the weapon to the socket using SnapToTargetIncludingScale rules.
            WeaponToEquip->AttachToComponent(MainCharacterMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, socket);
            // Notify the weapon that it has been sheathed.
            WeaponToEquip->Internal_OnWeaponSheathed();
        } else
        {
            UE_LOG(LogTemp, Log, TEXT("Remember to setup sockets in your weapon! - ACFEquipmentComp"));
        }
    }
}

//---------------------------------------------------------------------
// AttachWeaponOnHand
//---------------------------------------------------------------------
void UACFEquipmentComponent::AttachWeaponOnHand(AACFWeapon* localWeapon)
{
    // Get the socket name where the weapon should be attached when in hand.
    const FName socket = localWeapon->GetEquippedSocketName();
    if (socket != NAME_None)
    {
        // Attach the weapon to the main character mesh using snap rules.
        localWeapon->AttachToComponent(MainCharacterMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, socket);
        // Notify the weapon that it has been unsheathed.
        localWeapon->Internal_OnWeaponUnsheathed();
    } else
    {
        UE_LOG(LogTemp, Log, TEXT("Remember to setup sockets in your weapon! - ACFEquipmentComp"));
    }
}

//---------------------------------------------------------------------
// AddSkeletalMeshComponent_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::AddSkeletalMeshComponent_Implementation(TSubclassOf<class AACFArmor> ArmorClass, FGameplayTag itemSlot)
{
    // Ensure that the character owner is valid.
    if (!CharacterOwner)
    {
        return;
    }
    FModularPart outMesh;
    // Get the default object of the Armor class.
    const AACFArmor* ArmorToAdd = Cast<AACFArmor>(ArmorClass->GetDefaultObject());
    if (!ArmorToAdd || !ArmorToAdd->GetArmorMesh())
    {
        UE_LOG(LogTemp, Error, TEXT("Trying to wear an armor without armor mesh!!! - ACFEquipmentComp"));
        return;
    }

    // If a modular mesh exists for the given slot...
    if (GetModularMesh(itemSlot, outMesh) && outMesh.meshComp)
    {
        // Remove any override materials.
        outMesh.meshComp->EmptyOverrideMaterials();
        // Update the mesh with the new armor mesh.
        outMesh.meshComp->SetSkinnedAssetAndUpdate(ArmorToAdd->GetArmorMesh());
        // Make sure the mesh is visible.
        outMesh.meshComp->SetVisibility(true);
        // Use leader pose bounds for proper animation blending.
        outMesh.meshComp->bUseBoundsFromLeaderPoseComponent = true;
        // Attach the armor mesh component to the main character mesh with snap rules.
        outMesh.meshComp->AttachToComponent(MainCharacterMesh, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true));
        // Set the leader pose component for synchronized animations.
        outMesh.meshComp->SetLeaderPoseComponent(MainCharacterMesh);
    } else
    {
        // If no modular mesh exists for this slot, create a new armor slot component.
        UACFArmorSlotComponent* NewComp = NewObject<UACFArmorSlotComponent>(CharacterOwner, itemSlot.GetTagName());
        // Register the new component with the engine.
        NewComp->RegisterComponent();
        // Set its world location and rotation to defaults.
        NewComp->SetWorldLocation(FVector::ZeroVector);
        NewComp->SetWorldRotation(FRotator::ZeroRotator);
        // Associate the slot tag with the new component.
        NewComp->SetSlotTag(itemSlot);
        // Attach the new component to the main character mesh.
        NewComp->AttachToComponent(MainCharacterMesh, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true));
        // Set the armor mesh on the new component.
        NewComp->SetSkinnedAssetAndUpdate(ArmorToAdd->GetArmorMesh());
        // Set the leader pose component.
        NewComp->SetLeaderPoseComponent(MainCharacterMesh);
        // Enable the use of bounds from the leader pose for proper rendering.
        NewComp->bUseBoundsFromLeaderPoseComponent = true;
        // Add this new modular part to the modular meshes array.
        ModularMeshes.Add(FModularPart(NewComp));
    }
    // Broadcast an event that armor was equipped in the specified slot.
    OnEquippedArmorChanged.Broadcast(itemSlot);
}

//---------------------------------------------------------------------
// UseEquippedItemBySlot_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::UseEquippedItemBySlot_Implementation(FGameplayTag ItemSlot)
{
    if (!UACFItemSystemFunctionLibrary::IsValidItemSlotTag(ItemSlot))
    {
        UE_LOG(LogTemp, Log, TEXT("Invalid item Slot Tag!!! - ACFEquipmentComp"));
        return;
    }

    FEquippedItem EquipSlot;
    if (!GetEquippedItemSlot(ItemSlot, EquipSlot))
    {
        return;
    }

    AACFWeapon* localWeapon = Cast<AACFWeapon>(EquipSlot.Item);
    if (!localWeapon)
    {
        // Handle consumables or accessories
        if (EquipSlot.Item && EquipSlot.Item->IsA(AACFConsumable::StaticClass()))
        {
            UseEquippedConsumable(EquipSlot, CharacterOwner);
        }
        return;
    }

    const EHandleType HandleType = localWeapon->GetHandleType();

    if (HandleType == EHandleType::OffHand)
    {
        // Toggle off if already equipped
        if (Equipment.SecondaryWeapon == localWeapon)
        {
            SheathWeapon(localWeapon);
            Equipment.SecondaryWeapon = nullptr;
        }
        else
        {
            // Block if main is two-handed
            if (Equipment.MainWeapon && Equipment.MainWeapon->GetHandleType() == EHandleType::TwoHanded)
            {
                UE_LOG(LogTemp, Log, TEXT("Main weapon is two-handed, cannot equip offhand!"));
                return;
            }
            Equipment.SecondaryWeapon = localWeapon;
            AttachWeaponOnHand(localWeapon);
        }
    }
    else // Main-hand weapon
    {
        if (Equipment.MainWeapon == localWeapon)
        {
            // Toggle off if already equipped
            SheathWeapon(localWeapon);
            Equipment.MainWeapon = nullptr;
            CurrentlyEquippedSlotType = UACFItemSystemFunctionLibrary::GetItemSlotTagRoot();
        }
        else
        {
            // If two-handed, remove off-hand
            if (localWeapon->GetHandleType() == EHandleType::TwoHanded && Equipment.SecondaryWeapon)
            {
                SheathWeapon(Equipment.SecondaryWeapon);
                Equipment.SecondaryWeapon = nullptr;
            }

            if (Equipment.MainWeapon)
            {
                SheathWeapon(Equipment.MainWeapon);
            }

            Equipment.MainWeapon = localWeapon;
            AttachWeaponOnHand(localWeapon);
            CurrentlyEquippedSlotType = ItemSlot;
        }
    }

    OnEquipmentChanged.Broadcast(Equipment);
}

//---------------------------------------------------------------------
// UnequipItemBySlot_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::UnequipItemBySlot_Implementation(FGameplayTag itemSlot)
{
    // Verify that the provided slot tag is valid.
    if (!UACFItemSystemFunctionLibrary::IsValidItemSlotTag(itemSlot))
    {
        UE_LOG(LogTemp, Log, TEXT("Invalid item Slot Tag!!! - ACFEquipmentComp"));
        return;
    }
    
    FEquippedItem EquipSlot;
    if (GetEquippedItemSlot(itemSlot, EquipSlot))
    {
        AACFWeapon* weapon = Cast<AACFWeapon>(EquipSlot.Item);
        if (weapon)
        {
            if (Equipment.MainWeapon == weapon)
            {
                SheathWeapon(weapon);
                Equipment.MainWeapon = nullptr;
                CurrentlyEquippedSlotType = UACFItemSystemFunctionLibrary::GetItemSlotTagRoot();
            }
            else if (Equipment.SecondaryWeapon == weapon)
            {
                SheathWeapon(weapon);
                Equipment.SecondaryWeapon = nullptr;
            }
        }
        RemoveItemFromEquipment(EquipSlot);
        UpdateEquippedItemsVisibility();
    }
}

//---------------------------------------------------------------------
// UnequipItemByGuid_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::UnequipItemByGuid_Implementation(const FGuid& itemGuid)
{
    FEquippedItem EquipSlot;
    // If an equipped item matching the GUID is found, remove it.
    if (GetEquippedItem(itemGuid, EquipSlot))
    {
        RemoveItemFromEquipment(EquipSlot);
    }
}

void UACFEquipmentComponent::SheathWeapon(AACFWeapon* Weapon)
{
    if (!Weapon || !MainCharacterMesh) return;

    const FName socket = Weapon->GetOnBodySocketName();
    if (socket != NAME_None)
    {
        Weapon->AttachToComponent(MainCharacterMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, socket);
        Weapon->Internal_OnWeaponSheathed();
    }

    // Clear from main or secondary weapon if it matches
    if (Equipment.MainWeapon == Weapon)
    {
        Equipment.MainWeapon = nullptr;
    }
    if (Equipment.SecondaryWeapon == Weapon)
    {
        Equipment.SecondaryWeapon = nullptr;
    }

    // Broadcast updated state
    OnEquipmentChanged.Broadcast(Equipment);

    // Immediately enforce quickbar‐based show/hide
    UpdateEquippedItemsVisibility();
}

//---------------------------------------------------------------------
// SheathCurrentWeapon_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::SheathCurrentWeapon_Implementation()
{
    if (Equipment.MainWeapon)
    {
        SheathWeapon(Equipment.MainWeapon);
    }
    if (Equipment.SecondaryWeapon)
    {
        SheathWeapon(Equipment.SecondaryWeapon);
    }

    CurrentlyEquippedSlotType = UACFItemSystemFunctionLibrary::GetItemSlotTagRoot();

    // After sheathing both, refresh visibility so only weapons assigned to ActiveQuickbar pop up
    UpdateEquippedItemsVisibility();
}

//---------------------------------------------------------------------
// EquipInventoryItem_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::EquipInventoryItem_Implementation(const FInventoryItem& inItem)
{
    // Default to equipping without specifying a slot; call the more detailed version.
    EquipInventoryItemInSlot(inItem, FGameplayTag());
}

//---------------------------------------------------------------------
// EquipInventoryItemInSlot_Implementation
//---------------------------------------------------------------------

/* Checks, validations, and unequipping if necessary
 * Spawns item actor, assigns to a specific slot
 * Handles special cases for equippable and non-equippable items
 * Refreshes UI, equipment display, broadcasts event
 */ 
void UACFEquipmentComponent::EquipInventoryItemInSlot_Implementation(const FInventoryItem& inItem, FGameplayTag slot)
{
    FInventoryItem item;
    // Check if the inventory contains the item (by GUID).
    if (Inventory.Contains(inItem.GetItemGuid()))
    {
        item = *(Internal_GetInventoryItemByGuid(inItem.GetItemGuid()));
    } else
    {
        return; // If not found, exit the function.
    }

    // Verify the item can be equipped; if not, log a warning.
    if (!CanBeEquipped(item.ItemClass))
    {
        UE_LOG(LogTemp, Warning, TEXT("Item is not equippable  - ACFEquipmentComp"));
        return;
    }

    // If the item is already equipped...
    if (item.bIsEquipped)
    {
        // Ensure that its EquipmentSlot is valid (non-empty).
        ensure(item.EquipmentSlot != FGameplayTag());
        FEquippedItem currentItem;
        // If no slot is provided, default to the slot stored in the item.
        if (slot == FGameplayTag())
        {
            slot = item.EquipmentSlot;
        }
        // If the item is already equipped in the requested slot, do nothing.
        if (GetEquippedItemSlot(slot, currentItem) && currentItem.InventoryItem.GetItemGuid() == inItem.GetItemGuid())
        {
            return; // Already equipped in the same slot.
        }
        // Otherwise, unequip the item first to swap the slot.
        UnequipItemByGuid(inItem.GetItemGuid());
    }

    // Setup spawn parameters for creating the item actor.
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    SpawnParams.Instigator = CharacterOwner;
    SpawnParams.OverrideLevel = CharacterOwner->GetLevel();
    UWorld* world = GetWorld();

    // If the world pointer is invalid, exit.
    if (!world)
    {
        return;
    }

    // Spawn the item actor from its class at the character's location.
    AACFItem* itemInstance = world->SpawnActor<AACFItem>(item.ItemClass, CharacterOwner->GetActorLocation(), FRotator(0), SpawnParams);
    if (!itemInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("Impossible to spawn item!!! - ACFEquipmentComp"));
        return;
    }
    // Set the spawned item's owner to the character.
    itemInstance->SetItemOwner(CharacterOwner);

    // Attempt to cast the spawned item to an equippable item.
    AACFEquippableItem* equippable = Cast<AACFEquippableItem>(itemInstance);
    // If the item is equippable but its conditions for being equipped are not met, destroy it and exit.
    if (equippable && !equippable->CanBeEquipped(this))
    {
        equippable->Destroy();
        return;
    }

    FGameplayTag selectedSlot;
    // Determine which equipment slot the item should occupy.
    if (slot == FGameplayTag())
    {
        // If no slot was provided, try to find an available one from the item's possible slots.
        if (!TryFindAvailableItemSlot(item.ItemInfo.ItemSlots, selectedSlot) && item.ItemInfo.ItemSlots.Num() > 0)
        {
            // Fallback to the first possible slot.
            selectedSlot = item.ItemInfo.ItemSlots[0];
        }
    } else if (itemInstance->GetPossibleItemSlots().Contains(slot))
    {
        selectedSlot = slot;
    } else
    {
        // Log an error if the specified slot is invalid for this item.
        UE_LOG(LogTemp, Error, TEXT("Trying to equip an item in to an invalid Slot!!! - ACFEquipmentComp"));
        return;
    }

    // Unequip any item already occupying the selected slot.
    UnequipItemBySlot(selectedSlot);

    if (equippable)
    {
        // Notify the equippable item that it has been equipped.
        equippable->Internal_OnEquipped(CharacterOwner);
    } else
    {
        // For non-equippable items, attach to the actor to avoid garbage collection.
        FAttachmentTransformRules defaultRules = FAttachmentTransformRules::SnapToTargetNotIncludingScale;
        itemInstance->AttachToActor(CharacterOwner, defaultRules);
    }
    // Add the item to the Equipment structure.
    Equipment.EquippedItems.Add(FEquippedItem(item, selectedSlot, itemInstance));
    // Mark the item in the inventory as equipped.
    MarkItemOnInventoryAsEquipped(item, true, selectedSlot);

    // Update the equipment display.
    RefreshEquipment();
    UpdateEquippedItemsVisibility();
    // Broadcast equipment changed event.
    OnEquipmentChanged.Broadcast(Equipment);
}

//---------------------------------------------------------------------
// DropItemByInventoryIndex_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::DropItemByInventoryIndex_Implementation(int32 itemIndex, int32 count)
{
    // If the index is valid in the inventory, call DropItem on that item.
    if (Inventory.IsValidIndex(itemIndex))
    {
        DropItem(Inventory[itemIndex], count);
    }
}

//---------------------------------------------------------------------
// RemoveItemFromEquipment
//---------------------------------------------------------------------
void UACFEquipmentComponent::RemoveItemFromEquipment(const FEquippedItem& equippedItem)
{
    // Get the index of the equipped item in the Equipment.EquippedItems array using its slot key.
    const int32 index = Equipment.EquippedItems.IndexOfByKey(equippedItem.GetItemSlot());
    // Mark the item as unequipped in the inventory.
    MarkItemOnInventoryAsEquipped(equippedItem.InventoryItem, false, FGameplayTag());
    // If the item pointer is valid, proceed to notify and destroy it.
    if (equippedItem.Item->IsValidLowLevelFast())
    {
        AACFEquippableItem* equippable = Cast<AACFEquippableItem>(equippedItem.Item);
        if (equippable)
        {
            // Notify the item that it has been unequipped.
            equippable->Internal_OnUnEquipped();
            // If the item is armor, call the internal armor unequip handler.
            if (equippable->IsA(AACFArmor::StaticClass()))
            {
                Internal_OnArmorUnequipped(equippedItem.GetItemSlot());
            }
        }
        // Destroy the item actor.
        equippedItem.Item->Destroy();
    }
    // Remove the item from the Equipment array.
    Equipment.EquippedItems.RemoveAt(index);
    // Refresh equipment display.
    RefreshEquipment();
    // Broadcast that equipment has changed.
    OnEquipmentChanged.Broadcast(Equipment);
}

//---------------------------------------------------------------------
// MarkItemOnInventoryAsEquipped
//---------------------------------------------------------------------
void UACFEquipmentComponent::MarkItemOnInventoryAsEquipped(const FInventoryItem& item, bool bIsEquipped, const FGameplayTag& itemSlot)
{
    // Find the inventory item by its GUID.
    FInventoryItem* itemstruct = Internal_GetInventoryItem(item);
    if (itemstruct)
    {
        // Set the equipped flag and update its equipment slot.
        itemstruct->bIsEquipped = bIsEquipped;
        itemstruct->EquipmentSlot = itemSlot;
    }
}

//---------------------------------------------------------------------
// Internal_GetInventoryItem
//---------------------------------------------------------------------
FInventoryItem* UACFEquipmentComponent::Internal_GetInventoryItem(const FInventoryItem& item)
{
    // Retrieve an inventory item pointer by the item's GUID.
    return Internal_GetInventoryItemByGuid(item.GetItemGuid());
}

//---------------------------------------------------------------------
// Internal_GetInventoryItemByGuid
//---------------------------------------------------------------------
FInventoryItem* UACFEquipmentComponent::Internal_GetInventoryItemByGuid(const FGuid& itemToSearch)
{
    // Use the FindByKey function to get a pointer to the inventory item matching the GUID.
    return Inventory.FindByKey(itemToSearch);
}

//---------------------------------------------------------------------
// GetMainWeaponSocketLocation
//---------------------------------------------------------------------
FVector UACFEquipmentComponent::GetMainWeaponSocketLocation() const
{
    // Attempt to cast the current main weapon to a ranged weapon to get its shooting socket location.
    AACFRangedWeapon* rangedWeap = Cast<AACFRangedWeapon>(GetCurrentMainWeapon());
    if (rangedWeap)
    {
        return rangedWeap->GetShootingSocket();
    }
    // If no ranged weapon is equipped, return a zero vector.
    return FVector();
}

//---------------------------------------------------------------------
// GetItemByGuid
//---------------------------------------------------------------------
bool UACFEquipmentComponent::GetItemByGuid(const FGuid& itemGuid, FInventoryItem& outItem) const
{
    // Check if an item with the specified GUID exists in the inventory.
    if (Inventory.Contains(itemGuid))
    {
        outItem = *Inventory.FindByKey(itemGuid);
        return true;
    }
    return false;
}

//---------------------------------------------------------------------
// GetTotalCountOfItemsByClass
//---------------------------------------------------------------------
int32 UACFEquipmentComponent::GetTotalCountOfItemsByClass(const TSubclassOf<AACFItem>& ItemClass) const
{
    int32 totalItems = 0;
    TArray<FInventoryItem> outItems;
    // Fill outItems with all items of the specified class.
    GetAllItemsOfClassInInventory(ItemClass, outItems);
    // Sum the count of each item.
    for (const auto& item : outItems)
    {
        totalItems += item.Count;
    }
    return totalItems;
}

//---------------------------------------------------------------------
// GetAllItemsOfClassInInventory
//---------------------------------------------------------------------
void UACFEquipmentComponent::GetAllItemsOfClassInInventory(const TSubclassOf<AACFItem>& ItemClass, TArray<FInventoryItem>& outItems) const
{
    outItems.Empty();
    // Loop through the inventory and add items matching the specified class.
    for (int32 i = 0; i < Inventory.Num(); i++)
    {
        if (Inventory[i].ItemClass == ItemClass)
        {
            outItems.Add(Inventory[i]);
        }
    }
}

//---------------------------------------------------------------------
// GetAllSellableItemsInInventory
//---------------------------------------------------------------------
void UACFEquipmentComponent::GetAllSellableItemsInInventory(TArray<FInventoryItem>& outItems) const
{
    outItems.Empty();
    // Loop through inventory and add items flagged as sellable.
    for (const auto& item : Inventory)
    {
        if (item.ItemInfo.bSellable)
        {
            outItems.Add(item);
        }
    }
}

//---------------------------------------------------------------------
// FindFirstItemOfClassInInventory
//---------------------------------------------------------------------
bool UACFEquipmentComponent::FindFirstItemOfClassInInventory(const TSubclassOf<AACFItem>& ItemClass, FInventoryItem& outItem) const
{
    // Loop through inventory and return the first matching item.
    for (int32 i = 0; i < Inventory.Num(); i++)
    {
        if (Inventory[i].ItemClass == ItemClass)
        {
            outItem = Inventory[i];
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------
// GetEquippedItemSlot
//---------------------------------------------------------------------
bool UACFEquipmentComponent::GetEquippedItemSlot(const FGameplayTag& itemSlot, FEquippedItem& outSlot) const
{
    // Check if Equipment.EquippedItems contains an entry with the given slot tag.
    if (Equipment.EquippedItems.Contains(itemSlot))
    {
        const int32 index = Equipment.EquippedItems.IndexOfByKey(itemSlot);
        outSlot = Equipment.EquippedItems[index];
        return true;
    }
    return false;
}

//---------------------------------------------------------------------
// GetEquippedItem
//---------------------------------------------------------------------
bool UACFEquipmentComponent::GetEquippedItem(const FGuid& itemGuid, FEquippedItem& outSlot) const
{
    // Check if Equipment.EquippedItems contains an item matching the given GUID.
    if (Equipment.EquippedItems.Contains(itemGuid))
    {
        const int32 index = Equipment.EquippedItems.IndexOfByKey(itemGuid);
        outSlot = Equipment.EquippedItems[index];
        return true;
    }
    return false;
}

//---------------------------------------------------------------------
// GetModularMesh
//---------------------------------------------------------------------
bool UACFEquipmentComponent::GetModularMesh(FGameplayTag itemSlot, FModularPart& outMesh) const
{
    // Find the modular mesh for the specified slot.
    const FModularPart* slot = ModularMeshes.FindByKey(itemSlot);
    if (slot)
    {
        outMesh = *slot;
        return true;
    }
    return false;
}

//---------------------------------------------------------------------
// HasAnyItemInEquipmentSlot
//---------------------------------------------------------------------
bool UACFEquipmentComponent::HasAnyItemInEquipmentSlot(FGameplayTag itemSlot) const
{
    // Returns true if Equipment.EquippedItems contains the specified slot tag.
    return Equipment.EquippedItems.Contains(itemSlot);
}

//---------------------------------------------------------------------
// UseConsumableOnActorBySlot_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::UseConsumableOnActorBySlot_Implementation(FGameplayTag itemSlot, ACharacter* target)
{
    // Validate the provided item slot tag.
    if (!UACFItemSystemFunctionLibrary::IsValidItemSlotTag(itemSlot))
    {
        UE_LOG(LogTemp, Log, TEXT("Invalid item Slot Tag!!! - ACFEquipmentComp"));
        return;
    }

    FEquippedItem EquipSlot;
    // If the equipped item for this slot is found, use it as a consumable on the target.
    if (GetEquippedItemSlot(itemSlot, EquipSlot))
    {
        UseEquippedConsumable(EquipSlot, target);
    }
}

//---------------------------------------------------------------------
// SetDamageActivation
//---------------------------------------------------------------------
void UACFEquipmentComponent::SetDamageActivation(bool isActive, const TArray<FName>& traceChannels, bool isSecondaryWeapon /*= false*/)
{
    AACFMeleeWeapon* weapon;
    // Decide whether to use the secondary or main weapon based on the flag.
    if (isSecondaryWeapon && Equipment.SecondaryWeapon)
    {
        weapon = Cast<AACFMeleeWeapon>(Equipment.SecondaryWeapon);
    } else if (!isSecondaryWeapon && Equipment.MainWeapon)
    {
        weapon = Cast<AACFMeleeWeapon>(Equipment.MainWeapon);
    } else
    {
        return;
    }

    // If a weapon is found, start or stop its swing based on isActive.
    if (weapon)
    {
        if (isActive)
        {
            weapon->StartWeaponSwing(traceChannels);
        } else
        {
            weapon->StopWeaponSwing();
        }
    }
}

//---------------------------------------------------------------------
// SetMainMesh
//---------------------------------------------------------------------
void UACFEquipmentComponent::SetMainMesh(USkeletalMeshComponent* newMesh, bool bRefreshEquipment)
{
    // Update the main character mesh pointer.
    MainCharacterMesh = newMesh;
    if (bRefreshEquipment)
    {
        // If requested, refresh the equipment display.
        RefreshEquipment();
    }
}

//---------------------------------------------------------------------
// CanSwitchToRanged
//---------------------------------------------------------------------
bool UACFEquipmentComponent::CanSwitchToRanged()
{
    // Check if any equipped item is a ranged weapon.
    for (const auto& weap : Equipment.EquippedItems)
    {
        if (weap.Item->IsA(AACFRangedWeapon::StaticClass()))
        {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------
// CanSwitchToMelee
//---------------------------------------------------------------------
bool UACFEquipmentComponent::CanSwitchToMelee()
{
    // Check if any equipped item is a melee weapon.
    for (const auto& weap : Equipment.EquippedItems)
    {
        if (weap.Item->IsA(AACFMeleeWeapon::StaticClass()))
        {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------
// HasOnBodyAnyWeaponOfType
//---------------------------------------------------------------------
bool UACFEquipmentComponent::HasOnBodyAnyWeaponOfType(TSubclassOf<AACFWeapon> weaponClass) const
{
    // Loop through equipped items to see if any match the specified weapon class.
    for (const auto& weapon : Equipment.EquippedItems)
    {
        if (weapon.Item->IsA(weaponClass))
        {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------
// InitializeInventoryAndEquipment
//---------------------------------------------------------------------
void UACFEquipmentComponent::InitializeInventoryAndEquipment(USkeletalMeshComponent* inMainMesh)
{
    // Cast the owning actor to ACharacter and cache it.
    CharacterOwner = Cast<ACharacter>(GetOwner());
    // Set the main mesh (pass false to avoid immediate refresh).
    SetMainMesh(inMainMesh, false);
    // Only the server should initialize the inventory.
    if (GetOwner()->HasAuthority())
    {
        Inventory.Empty(); // Clear the current inventory.
        currentInventoryWeight = 0.f; // Reset inventory weight.
        // Loop through all starting items specified in the component.
        for (const FStartingItem& item : StartingItems)
        {
            // Add the item to the inventory; auto-equip if specified.
            Internal_AddItem(item, item.bAutoEquip, item.DropChancePercentage);
            // Log if the inventory exceeds the maximum slots.
            if (Inventory.Num() > MaxInventorySlots)
            {
                UE_LOG(LogTemp, Log, TEXT("Invalid Inventory setup, too many slots on character!!! - ACFEquipmentComp"));
            }
        }
    }
}

//---------------------------------------------------------------------
// SpawnWorldItem
//---------------------------------------------------------------------
void UACFEquipmentComponent::SpawnWorldItem(const TArray<FBaseItem>& items)
{
    // If the character owner is valid...
    if (CharacterOwner)
    {
        // Get the starting location from the character's navigation agent.
        const FVector startLoc = CharacterOwner->GetNavAgentLocation();
        // Spawn a world item near the character using a helper function.
        UACFItemSystemFunctionLibrary::SpawnWorldItemNearLocation(this, items, startLoc);
    }
}

//---------------------------------------------------------------------
// UseEquippedConsumable
//---------------------------------------------------------------------
void UACFEquipmentComponent::UseEquippedConsumable(FEquippedItem& EquipSlot, ACharacter* target)
{
    // If the equipped item is a consumable...
    if (EquipSlot.Item->IsA(AACFConsumable::StaticClass()))
    {
        AACFConsumable* consumable = Cast<AACFConsumable>(EquipSlot.Item);
        // Use the consumable via an internal function that applies its effects.
        Internal_UseItem(consumable, target, EquipSlot.InventoryItem);
    }
}

//---------------------------------------------------------------------
// UseConsumableOnTarget
//---------------------------------------------------------------------
void UACFEquipmentComponent::UseConsumableOnTarget(const FInventoryItem& Inventoryitem, ACharacter* target)
{
    // Spawn a consumable actor from the Inventoryitem's class at a default location.
    AACFConsumable* consumable = GetWorld()->SpawnActor<AACFConsumable>(Inventoryitem.ItemClass, FVector(0.f), FRotator(0));
    // Check if the consumable can be used by the character.
    if (consumable->CanBeUsed(CharacterOwner))
    {
        // Set the consumable's owner.
        consumable->SetItemOwner(CharacterOwner);
        // Use the consumable on the target.
        Internal_UseItem(consumable, target, Inventoryitem);
    }
    // Set the consumable to be destroyed shortly after (to avoid lingering).
    consumable->SetLifeSpan(.2f);
}

//---------------------------------------------------------------------
// CanUseConsumable
//---------------------------------------------------------------------
bool UACFEquipmentComponent::CanUseConsumable(const FInventoryItem& Inventoryitem)
{
    // Spawn a temporary consumable actor to check if it can be used.
    AACFConsumable* consumable = GetWorld()->SpawnActor<AACFConsumable>(Inventoryitem.ItemClass, FVector(0.f), FRotator(0));
    consumable->SetLifeSpan(.2f); // Short lifespan to auto-destroy.
    // Return whether the consumable is valid and can be used.
    return consumable && consumable->CanBeUsed(CharacterOwner);
}

//---------------------------------------------------------------------
// Internal_UseItem
//---------------------------------------------------------------------
void UACFEquipmentComponent::Internal_UseItem(AACFConsumable* consumable, ACharacter* target, const FInventoryItem& Inventoryitem)
{
    // Check if the consumable is valid and can be used.
    if (consumable && consumable->CanBeUsed(CharacterOwner))
    {
        // Call the consumable's internal use function to apply its effect to the target.
        consumable->Internal_UseItem(target);
        // If the consumable is consumed on use, remove one unit from the inventory.
        if (consumable->bConsumeOnUse)
        {
            RemoveItem(Inventoryitem, 1);
        }
    } else
    {
        // Log an error if the consumable is invalid.
        UE_LOG(LogTemp, Error, TEXT("Invalid Consumable!!! - UACFEquipmentComponent::UseConsumableOnTarget"));
    }
}

//---------------------------------------------------------------------
// DestroyEquipment
//---------------------------------------------------------------------
void UACFEquipmentComponent::DestroyEquipment()
{
    // Loop through all equipped items.
    for (auto& equip : Equipment.EquippedItems)
    {
        // Attempt to cast the item to an equippable item.
        AACFEquippableItem* equippable = Cast<AACFEquippableItem>(equip.Item);
        if (equippable)
        {
            // Notify the item that it is being unequipped.
            equippable->Internal_OnUnEquipped();
        }
        // Destroy the item actor.
        equip.Item->Destroy();
    }
}

//---------------------------------------------------------------------
// DestroyEquippedItems_Implementation
//---------------------------------------------------------------------
void UACFEquipmentComponent::DestroyEquippedItems_Implementation()
{
    // Call the internal function to destroy equipment.
    Internal_DestroyEquipment();

    TArray<FBaseItem> toDrop;
    // If items should be dropped upon death and there are items in inventory...
    if (bDropItemsOnDeath && Inventory.Num() > 0)
    {
        // Loop backwards through the Inventory array.
        for (int32 Index = Inventory.Num() - 1; Index >= 0; --Index)
        {
            // Ensure the index is valid and the item is droppable.
            if (Inventory.IsValidIndex(Index) && Inventory[Index].ItemInfo.bDroppable)
            {
                FBaseItem newItem(Inventory[Index]);
                newItem.Count = 0;

                // Loop to determine how many of the item should be dropped based on its drop chance.
                for (uint8 i = 0; i < Inventory[Index].Count; i++)
                {
                    if (Inventory[Index].DropChancePercentage > FMath::RandRange(0.f, 100.f))
                    {
                        newItem.Count++;
                    }
                }

                // If at least one item should be dropped...
                if (newItem.Count > 0)
                {
                    toDrop.Add(newItem);
                    // If items are not to be collapsed into a single world item...
                    if (!bCollapseDropInASingleWorldItem)
                    {
                        TArray<FBaseItem> newDrop;
                        newDrop.Add(newItem);
                        SpawnWorldItem(newDrop);
                        RemoveItem(Inventory[Index], Inventory[Index].Count);
                    } else
                    {
                        // Remove the entire stack.
                        RemoveItem(Inventory[Index], Inventory[Index].Count);
                    }
                }
            }
        }

        // If items are to be collapsed into one world item, spawn it.
        if (bCollapseDropInASingleWorldItem)
        {
            SpawnWorldItem(toDrop);
        }
    }
}

//---------------------------------------------------------------------
// Internal_DestroyEquipment
//---------------------------------------------------------------------
void UACFEquipmentComponent::Internal_DestroyEquipment()
{
    // Loop through each equipped item.
    for (auto& weap : Equipment.EquippedItems)
    {
        if (weap.Item)
        {
            // If the item is equippable, notify that it is being unequipped.
            AACFEquippableItem* equippable = Cast<AACFEquippableItem>(weap.Item);
            if (equippable)
            {
                equippable->Internal_OnUnEquipped();
            }
            // Set a short lifespan for the item to ensure it is cleaned up.
            weap.Item->SetLifeSpan(.1f);
        }
    }
}

//---------------------------------------------------------------------
// NumberOfItemCanTake
//---------------------------------------------------------------------
int32 UACFEquipmentComponent::NumberOfItemCanTake(const TSubclassOf<AACFItem>& itemToCheck)
{
    int32 addeditemstotal = 0;
    // Get existing inventory items of the specified class.
    TArray<FInventoryItem*> outItems = FindItemsByClass(itemToCheck);
    FItemDescriptor itemInfo;
    // Retrieve the item data (including weight and max stack) for the item.
    UACFItemSystemFunctionLibrary::GetItemData(itemToCheck, itemInfo);
    float MaxByWeight = 999.f;
    // Determine how many items can be added based on weight.
    if (itemInfo.ItemWeight > 0)
    {
        MaxByWeight = (MaxInventoryWeight - currentInventoryWeight) / itemInfo.ItemWeight;
    }
    const int32 maxAddableByWeight = FMath::TruncToInt(MaxByWeight);
    // Calculate free inventory space (in terms of stacks).
    const int32 FreeSpaceInInventory = MaxInventorySlots - Inventory.Num();
    int32 maxAddableByStack = FreeSpaceInInventory * itemInfo.MaxInventoryStack;
    // If there are already items of this type, calculate additional stack space available.
    if (outItems.Num() > 0)
    {
        for (const auto& outItem : outItems)
        {
            maxAddableByStack += itemInfo.MaxInventoryStack - outItem->Count;
        }
    }
    // The maximum number of items that can be added is the minimum of the weight and stack limits.
    addeditemstotal = FGenericPlatformMath::Min(maxAddableByStack, maxAddableByWeight);
    return addeditemstotal;
}

//---------------------------------------------------------------------
// FInventoryItem Constructors
//---------------------------------------------------------------------
FInventoryItem::FInventoryItem(const FBaseItem& inItem)
{
    // Set initial count from the base item.
    Count = inItem.Count;
    // Retrieve item information from the data asset.
    UACFItemSystemFunctionLibrary::GetItemData(inItem.ItemClass, ItemInfo);
    // Generate a unique identifier for this inventory item.
    ItemGuid = FGuid::NewGuid();
    // Store the pointer to the item class.
    ItemClass = inItem.ItemClass.Get();
}

FInventoryItem::FInventoryItem(const FStartingItem& inItem)
{
    Count = inItem.Count;
    // Retrieve item data for the starting item.
    UACFItemSystemFunctionLibrary::GetItemData(inItem.ItemClass, ItemInfo);
    ItemGuid = FGuid::NewGuid();
    ItemClass = inItem.ItemClass.Get();
    // Store drop chance defined in the starting item.
    DropChancePercentage = inItem.DropChancePercentage;
}

//---------------------------------------------------------------------
// RefreshDescriptor (for FInventoryItem)
//---------------------------------------------------------------------
void FInventoryItem::RefreshDescriptor()
{
    // Update the item information by re-reading data from the data asset.
    UACFItemSystemFunctionLibrary::GetItemData(ItemClass, ItemInfo);
}

// UpdateEquippedItemsVisibility method (added by Nomad Dev team)
void UACFEquipmentComponent::UpdateEquippedItemsVisibility()
{
    for (const auto& Equipped : Equipment.EquippedItems)
    {
        // only care about weapons
        AACFWeapon* Weapon = Cast<AACFWeapon>(Equipped.Item);
        if (!Weapon) continue;

        // 1) If it’s the weapon I’m holding (main or off-hand), always show it in-hand
        if (Weapon == Equipment.MainWeapon || Weapon == Equipment.SecondaryWeapon)
        {
            Weapon->SetActorHiddenInGame(false);
            AttachWeaponOnHand(Weapon);
            continue;
        }

        // 2) Otherwise it’s sheathed—show it on the back only if its quickbar matches
        bool bShouldShowOnBack =
            (Equipped.InventoryItem.AssignedQuickbarEnum == ActiveQuickbarEnum);

        Weapon->SetActorHiddenInGame(!bShouldShowOnBack);
        if (bShouldShowOnBack)
            AttachWeaponOnBody(Weapon);
        else
            Weapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    }
}

// SetActiveQuickbar (added by Nomad Dev team)
void UACFEquipmentComponent::SetActiveQuickbarEnum(EActiveQuickbar NewQuickbarEnum)
{
    // Debug logging with on-screen messages
    FString RoleString = GetOwnerRole() == ROLE_Authority ? TEXT("Authority") : 
                        GetOwnerRole() == ROLE_AutonomousProxy ? TEXT("AutonomousProxy") : TEXT("SimulatedProxy");
    
    UE_LOG(LogTemp, Warning, TEXT("SetActiveQuickbarEnum called: Current=%d, New=%d, Role=%s"), 
           ActiveQuickbarEnum, 
           (int32)NewQuickbarEnum,
           *RoleString);

    // Always update locally immediately (so your client sees it right away)
    if (ActiveQuickbarEnum != NewQuickbarEnum)
    {
        ActiveQuickbarEnum = NewQuickbarEnum;
        OnRep_ActiveQuickbarEnum();
    }

    // If we're a client, also tell the server to replicate to everyone else
    if (GetOwnerRole() < ROLE_Authority)
    {
        Server_SetActiveQuickbarEnum(NewQuickbarEnum);
    }
    // otherwise (we're the server), the property change above will replicate
}

void UACFEquipmentComponent::Server_SetActiveQuickbarEnum_Implementation(const EActiveQuickbar NewActiveQuickbar)
{
    SetActiveQuickbarEnum(NewActiveQuickbar);
}

// AssignItemToQuickbar (added by Nomad Dev team)
void UACFEquipmentComponent::AssignItemToQuickbar_Implementation(const FGuid& ItemGuid, const EActiveQuickbar ActiveQuickbar)
{
    // Find the inventory item
    if (FInventoryItem* item = Internal_GetInventoryItemByGuid(ItemGuid))
    {
        // store enum as its underlying int
        item->AssignedQuickbarEnum = ActiveQuickbar;

        // immediately re-evaluate who belongs on the back vs in-hand
        UpdateEquippedItemsVisibility();

        // if you want Blueprint-side listeners to fire:
        OnInventoryChanged.Broadcast(Inventory);
    }
}

void UACFEquipmentComponent::OnRep_ActiveQuickbarEnum()
{
    UpdateEquippedItemsVisibility();
}


// void
// UACFEquipmentComponent::MoveItemToAnotherInventory_Implementation(UACFEquipmentComponent*
// OtherEquipmentComponent, class AACFItem* itemToMove, int32 count /*= 1*/)
// {
//     if (!itemToMove) {
//         return;
//     }
//     if (itemToMove->GetItemInfo().MaxInventoryStack > 1) {
//         int32 howmanycantake =
//         OtherEquipmentComponent->NumberOfItemCanTake(itemToMove); if
//         (howmanycantake < count) {
//             count = howmanycantake;
//         }
//         int NumberToRemove =
//         OtherEquipmentComponent->Internal_AddItem(itemToMove, count);
//         //int
//         NumberToRemove=OtherEquipmentComponent->AddItemToInventoryByClass(itemToMove->GetClass(),
//         count, false); RemoveItem(itemToMove, NumberToRemove);
//     } else {
//         int NumberToRemove =
//         OtherEquipmentComponent->Internal_AddItem(itemToMove, 1); if
//         (NumberToRemove == 1) {
//             RemoveItem(itemToMove);
//         }
//     }
// }

// bool
// UACFEquipmentComponent::MoveItemToAnotherInventory_Validate(UACFEquipmentComponent*
// OtherEquipmentComponent, class AACFItem* itemToMove, int32 count /*= 1*/)
// {
//     return true;
// }