// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once  // Ensure this header is included only once per compilation unit.

#include <Components/SkeletalMeshComponent.h>  // For using skeletal mesh components.
#include <Engine/DataTable.h>                   // For data table support.
#include <GameplayTagContainer.h>               // For working with gameplay tags.

#include "ACFItemTypes.h"                       // Include common item types for ACF.
#include "Components/ActorComponent.h"          // Base class for components.
#include "CoreMinimal.h"                        // Basic core types and macros.
#include "Items/ACFItem.h"                      // Base class for items.
#include "ACFEquipmentComponent.generated.h"    // Auto-generated header code (UHT).

// Forward declarations.
class USkeletalMeshComponent;
class AACFConsumable;

UENUM(BlueprintType)
enum class EActiveQuickbar : uint8
{
    Combat UMETA(DisplayName = "Combat Bar"),
    Tools UMETA(DisplayName = "Tools Bar")
};

// USTRUCT: FStartingItem – represents an item that a character may start with in the inventory.
USTRUCT(BlueprintType)
struct FStartingItem : public FBaseItem {
    GENERATED_BODY()

public:
    FStartingItem() {};  // Default constructor.

    // If true, this starting item will be automatically equipped on spawn.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ACF)
    bool bAutoEquip = true;

    // Drop chance percentage when the character dies (from 0 to 100).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "100.0"), Category = ACF)
    float DropChancePercentage = 0.f;

    // Operator overload to compare two starting items by their item class.
    FORCEINLINE bool operator==(const FStartingItem& Other) const
    {
        return this->ItemClass == Other.ItemClass;
    }

    // Operator overload for inequality.
    FORCEINLINE bool operator!=(const FStartingItem& Other) const
    {
        return this->ItemClass != Other.ItemClass;
    }
};

// USTRUCT: FInventoryItem – represents an individual item instance in the inventory.
USTRUCT(BlueprintType)
struct FInventoryItem : public FBaseItem {
    GENERATED_BODY()

public:
    FInventoryItem() {};  // Default constructor.

    // Constructs an inventory item from a base item.
    FInventoryItem(const FBaseItem& inItem);

    // Constructs an inventory item from a starting item.
    FInventoryItem(const FStartingItem& inItem);

    /* Information about the item in this inventory slot, such as name, weight, etc. */
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    FItemDescriptor ItemInfo;

    /* Inventory index used for grid-based inventories (not set by default) */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = ACF)
    int32 InventoryIndex = 0;

    /* Boolean flag indicating whether this item is currently equipped. */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = ACF)
    bool bIsEquipped = false;

    /* If the item is equipped, this tag denotes the equipment slot used.
       It remains unset if the item is not equipped. */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = ACF)
    FGameplayTag EquipmentSlot;

    // Nomad Dev Team: which quickbar (0 or 1) this stack is bound to
    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "ACF | Quickbar")
    EActiveQuickbar AssignedQuickbarEnum = EActiveQuickbar::Combat;

    /* Chance (in percentage) that this item will drop when the character dies. */
    UPROPERTY(SaveGame, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "100.0"), Category = ACF)
    float DropChancePercentage = 0.f;

    // Returns the unique identifier (GUID) of this inventory item.
    FGuid GetItemGuid() const
    {
        return ItemGuid;
    }

    // Forces the item to use a specific GUID.
    void ForceGuid(const FGuid& newGuid)
    {
        ItemGuid = newGuid;
    }

    // Update the item descriptor from the current item class data.
    void RefreshDescriptor();

    // Operator overloads for comparisons using GUID.
    FORCEINLINE bool operator==(const FInventoryItem& Other) const
    {
        return this->GetItemGuid() == Other.GetItemGuid();
    }
    FORCEINLINE bool operator!=(const FInventoryItem& Other) const
    {
        return this->GetItemGuid() != Other.GetItemGuid();
    }
    FORCEINLINE bool operator==(const FGuid& Other) const
    {
        return this->GetItemGuid() == Other;
    }
    FORCEINLINE bool operator!=(const FGuid& Other) const
    {
        return this->GetItemGuid() != Other;
    }
};

// USTRUCT: FEquippedItem – represents an item that is equipped (with additional runtime info).
USTRUCT(BlueprintType)
struct FEquippedItem {
    GENERATED_BODY()

public:
    // Default constructor: Initialize pointer to null.
    FEquippedItem() {
        Item = nullptr;
    };

    // Constructor that creates an equipped item with the inventory data, equipment slot, and the spawned item pointer.
    FEquippedItem(const FInventoryItem& item, const FGameplayTag& itemSlot, AACFItem* itemPtr)
    {
        ItemSlot = itemSlot;
        InventoryItem = item;
        InventoryItem.ForceGuid(item.GetItemGuid()); // Make sure the GUID remains consistent.
        InventoryItem.bIsEquipped = true;             // Mark this inventory item as equipped.
        InventoryItem.EquipmentSlot = ItemSlot;         // Assign the equipment slot.
        Item = itemPtr;                                 // Save the pointer to the actual item actor.
    }

    // The equipment slot tag where the item is equipped.
    UPROPERTY(BlueprintReadWrite, Category = ACF)
    FGameplayTag ItemSlot;

    // The inventory item data corresponding to this equipped item.
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    FInventoryItem InventoryItem;

    // Pointer to the actual item actor that is equipped.
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    class AACFItem* Item;

    // Returns the equipped slot tag.
    FGameplayTag GetItemSlot() const
    {
        return ItemSlot;
    }

    // Operator overloads for comparing equipped items.
    FORCEINLINE bool operator==(const FEquippedItem& Other) const
    {
        return this->ItemSlot == Other.ItemSlot;
    }
    FORCEINLINE bool operator!=(const FEquippedItem& Other) const
    {
        return this->ItemSlot != Other.ItemSlot;
    }
    FORCEINLINE bool operator==(const FGameplayTag& Other) const
    {
        return this->ItemSlot == Other;
    }
    FORCEINLINE bool operator!=(const FGameplayTag& Other) const
    {
        return this->ItemSlot != Other;
    }
    FORCEINLINE bool operator==(const FGuid& Other) const
    {
        return this->InventoryItem.GetItemGuid() == Other;
    }
    FORCEINLINE bool operator!=(const FGuid& Other) const
    {
        return this->InventoryItem.GetItemGuid() != Other;
    }
};

// USTRUCT: FEquipment – stores the currently equipped items on a character.
USTRUCT(BlueprintType)
struct FEquipment {
    GENERATED_BODY()

public:
    // Pointer to the main (primary) weapon equipped.
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    class AACFWeapon* MainWeapon;

    // Pointer to the secondary weapon equipped.
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    class AACFWeapon* SecondaryWeapon;

    // Array of all equipped items.
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    TArray<FEquippedItem> EquippedItems;

    // Default constructor: Initializes weapon pointers to nullptr.
    FEquipment()
    {
        MainWeapon = nullptr;
        SecondaryWeapon = nullptr;
    }
};

// Delegate declarations for broadcasting equipment and inventory changes.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentChanged, const FEquipment&, Equipment);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquippedArmorChanged, const FGameplayTag&, ArmorSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryChanged, const TArray<FInventoryItem>&, Inventory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAdded, const FBaseItem&, item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, const FBaseItem&, item);

// UCLASS: UACFEquipmentComponent – main component for managing a character's equipment and inventory.
UCLASS(Blueprintable, ClassGroup = (ACF), meta = (BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UACFEquipmentComponent : public UActorComponent {
    GENERATED_BODY()

public:
    // Constructor: Sets default property values for this component.
    UACFEquipmentComponent();

    /* Use this only on Server!!!
     *
     * Initialize a character's inventory and equipment.
     * Optionally receive a skeletal mesh to use for equipment attachments.
     */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void InitializeInventoryAndEquipment(USkeletalMeshComponent* inMainMesh = nullptr);

    // Called when the owner (entity) dies.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    void OnEntityOwnerDeath();

    /*------------------------ INVENTORY FUNCTIONS ------------------------*/

    // Adds an item to inventory by class.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void AddItemToInventoryByClass(TSubclassOf<AACFItem> inItem, int32 count = 1, bool bAutoEquip = true);

    // Adds a specific base item to the inventory.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void AddItemToInventory(const FBaseItem& ItemToAdd, bool bAutoEquip = true);

    // Equips or unequips an item from the inventory based on its index.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void ToggleEquipItemByIndex(int32 index);

    // Equips or unequips an item from the inventory given the FInventoryItem.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void ToggleEquipInventoryItem(const FInventoryItem& item, bool bIsSuccessful);

    // Removes a specified amount of the given item from the inventory.
    // If the item is equipped, it also unequips it.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void RemoveItem(const FInventoryItem& item, int32 count = 1);

    // Removes an item from the inventory by index.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void RemoveItemByIndex(const int32 index, int32 count = 1);

    // Drops a specified amount of the item from the inventory.
    // Also spawns a world item near the owner with the dropped items.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void DropItem(const FInventoryItem& item, int32 count = 1);

    // Drops an item from the inventory using its index.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void DropItemByInventoryIndex(int32 itemIndex, int32 count);

    // Consumes items from the inventory based on an array of base items.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void ConsumeItems(const TArray<FBaseItem>& ItemsToCheck);

    // Sets the inventory slot index for a given item.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void SetInventoryItemSlotIndex(const FInventoryItem& item, int newIndex);

    // Uses a consumable item on a target actor.
    UFUNCTION(BlueprintCallable, Category = "ACF | Inventory")
    void UseConsumableOnTarget(const FInventoryItem& Inventoryitem, ACharacter* target);

    // Checks if a consumable can be used.
    UFUNCTION(BlueprintCallable, Category = "ACF | Inventory")
    bool CanUseConsumable(const FInventoryItem& Inventoryitem);

    /*
     * A function that added by Nomad Dev Team
     * Swaps array indices of two inventory items when drag and drop operation applied.
     */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void SwapInventoryItems(int32 indexA, int32 indexB);

    /*------------------------ STORAGE FUNCTIONS ------------------------*/
    // Moves items from this inventory to a storage component.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Inventory")
    void MoveItemsToInventory(const TArray<FBaseItem>& inItems, UACFStorageComponent* equipComp);

    /*------------------------ EQUIPMENT FUNCTIONS ------------------------*/
    // Equips an item from the inventory.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Equipment")
    void EquipInventoryItem(const FInventoryItem& inItem);

    // Equips an item from the inventory into a specified equipment slot.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Equipment")
    void EquipInventoryItemInSlot(const FInventoryItem& inItem, FGameplayTag slot);

    // Uses an equipped item based on its equipment slot.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Equipment")
    void UseEquippedItemBySlot(FGameplayTag itemSlot);

    // Uses a consumable item equipped in a specific equipment slot on a target actor.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Equipment")
    void UseConsumableOnActorBySlot(FGameplayTag itemSlot, ACharacter* target);

    // Unequips an item from the given equipment slot.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Equipment")
    void UnequipItemBySlot(FGameplayTag itemSlot);

    // Unequips an item using its unique identifier (GUID).
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Equipment")
    void UnequipItemByGuid(const FGuid& itemGuid);

    // Sheaths the currently held weapon (puts it back into storage).
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Equipment")
    void SheathCurrentWeapon();

    // Sets whether damage traces are active on a weapon.
    UFUNCTION(BlueprintCallable, Category = "ACF | Equipment")
    void SetDamageActivation(bool isActive, const TArray<FName>& traceChannels, bool isLeftWeapon = false);

    // Overrides the main character's mesh with a new one, optionally refreshing equipment attachments.
    UFUNCTION(BlueprintCallable, Category = "ACF | Equipment")
    void SetMainMesh(USkeletalMeshComponent* newMesh, bool bRefreshEquipment = true);

    // Returns the main character's skeletal mesh.
    UFUNCTION(BlueprintPure, Category = "ACF | Equipment")
    USkeletalMeshComponent* GetMainMesh() const { return MainCharacterMesh; }

    // Destroys all currently equipped items.
    UFUNCTION(Server, Reliable, Category = "ACF | Equipment")
    void DestroyEquippedItems();

    // Refreshes the appearance of equipment on the owner (useful for late joiners).
    UFUNCTION(BlueprintCallable, Category = "ACF | Equipment")
    void RefreshEquipment();

    // Recalculates the total weight of items in the inventory.
    UFUNCTION(BlueprintCallable, Category = "ACF | Equipment")
    void RefreshTotalWeight();

    // Determines if left-hand IK should be used for weapon animations.
    UFUNCTION(BlueprintPure, Category = "ACF | Equipment")
    bool ShouldUseLeftHandIK() const;

    // Returns the left-hand IK position from the equipped weapon.
    UFUNCTION(BlueprintPure, Category = "ACF | Equipment")
    FVector GetLeftHandIkPos() const;

    // Checks if a specific equipment slot (tag) is available.
    UFUNCTION(BlueprintPure, Category = "ACF | Equipment")
    bool IsSlotAvailable(const FGameplayTag& itemSlot) const;

    // Attempts to find an available equipment slot among a list of tags; returns true if found.
    UFUNCTION(BlueprintCallable, Category = "ACF | Equipment")
    bool TryFindAvailableItemSlot(const TArray<FGameplayTag>& itemSlots, FGameplayTag& outAvailableSlot);

    // Checks if there is at least one valid slot among the given equipment slot tags.
    UFUNCTION(BlueprintCallable, Category = "ACF | Equipment")
    bool HaveAtLeastAValidSlot(const TArray<FGameplayTag>& itemSlots);

    /* A function that added by Nomad Dev Team
     * Assigns item to a quickbar
     */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF | Quickbar")
    void AssignItemToQuickbar(const FGuid& ItemGuid, EActiveQuickbar ActiveQuickbar);

    /*------------------------ SETTERS ------------------------*/

    // Sets the maximum inventory weight.
    UFUNCTION(BlueprintCallable, Category = "ACF | Setters")
    void SetMaxInventoryWeight(int32 newMax)
    {
        MaxInventoryWeight = newMax;
    }

    // Sets the maximum number of inventory slots.
    UFUNCTION(BlueprintCallable, Category = "ACF | Setters")
    void SetMaxInventorySlots(int32 newMax)
    {
        MaxInventorySlots = newMax;
    }

    /*------------------------ GETTERS ------------------------*/

    // Returns the maximum number of inventory slots.
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE int32 GetMaxInventorySlots() const
    {
        return MaxInventorySlots;
    }

    // Returns the maximum inventory weight.
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE int32 GetMaxInventoryWeight() const
    {
        return MaxInventoryWeight;
    }

    // Returns the currently equipped main weapon.
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE AACFWeapon* GetCurrentMainWeapon() const
    {
        return Equipment.MainWeapon;
    }

    // Returns the currently equipped offhand weapon.
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE AACFWeapon* GetCurrentOffhandWeapon() const
    {
        return Equipment.SecondaryWeapon;
    }

    // Returns the full equipment structure.
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE FEquipment GetCurrentEquipment() const
    {
        return Equipment;
    }

    // Returns the inventory array.
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE TArray<FInventoryItem> GetInventory() const
    {
        return Inventory;
    }

    // Returns the world location of the main weapon's socket (e.g., for spawning projectiles).
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FVector GetMainWeaponSocketLocation() const;

    // Checks if a particular inventory item is present.
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE bool IsInInventory(const FInventoryItem& item) const
    {
        return Inventory.Contains(item);
    }

    // Retrieves an inventory item by its GUID.
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    bool GetItemByGuid(const FGuid& itemGuid, FInventoryItem& outItem) const;

    // Retrieves an inventory item by its index.
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE bool GetItemByIndex(const int32 index, FInventoryItem& outItem) const
    {
        if (Inventory.IsValidIndex(index)) {
            outItem = Inventory[index];
            return true;
        }
        return false;
    }

    // Returns the first empty inventory slot index.
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE int32 GetFirstEmptyInventoryIndex() const
    {
        for (int32 i = 0; i < Inventory.Num(); i++) {
            if (IsSlotEmpty(i))
                return i;
        }
        return -1;
    }

    // Retrieves an inventory item by its designated inventory index.
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE bool GetItemByInventoryIndex(const int32 index, FInventoryItem& outItem) const
    {
        for (const FInventoryItem& item : Inventory) {
            if (item.InventoryIndex == index) {
                outItem = item;
                return true;
            }
        }
        return false;
    }

    // Checks if a given slot (by index) in the inventory is empty.
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE bool IsSlotEmpty(int32 index) const
    {
        for (const FInventoryItem& item : Inventory) {
            if (item.InventoryIndex == index)
                return false;
        }
        return true;
    }

    // Returns the total count of items in the inventory for a specific item class.
    UFUNCTION(BlueprintCallable, Category = "ACF | Getters")
    int32 GetTotalCountOfItemsByClass(const TSubclassOf<AACFItem>& itemClass) const;

    // Retrieves all inventory items of a specified class.
    UFUNCTION(BlueprintCallable, Category = "ACF | Getters")
    void GetAllItemsOfClassInInventory(const TSubclassOf<AACFItem>& itemClass, TArray<FInventoryItem>& outItems) const;

    // Retrieves all sellable items from the inventory.
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    void GetAllSellableItemsInInventory(TArray<FInventoryItem>& outItems) const;

    // Finds the first inventory item of a given class.
    UFUNCTION(BlueprintCallable, Category = "ACF | Getters")
    bool FindFirstItemOfClassInInventory(const TSubclassOf<AACFItem>& itemClass, FInventoryItem& outItem) const;

    // Returns the total weight of items currently in the inventory.
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE float GetCurrentInventoryTotalWeight() const
    {
        return currentInventoryWeight;
    }

    // Returns the modular meshes (e.g., for armor) attached to the character.
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    FORCEINLINE TArray<FModularPart> GetModularMeshes() const
    {
        return ModularMeshes;
    }

    // Retrieves an equipped item by its equipment slot tag.
    UFUNCTION(BlueprintCallable, Category = "ACF | Getters")
    bool GetEquippedItemSlot(const FGameplayTag& itemSlot, FEquippedItem& outSlot) const;

    // Retrieves an equipped item by its unique GUID.
    UFUNCTION(BlueprintCallable, Category = "ACF | Getters")
    bool GetEquippedItem(const FGuid& itemGuid, FEquippedItem& outSlot) const;

    // Retrieves a modular mesh for a given equipment slot tag.
    UFUNCTION(BlueprintCallable, Category = "ACF | Getters")
    bool GetModularMesh(FGameplayTag itemSlot, FModularPart& outMesh) const;

    // Returns the starting items for the character.
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    TArray<FStartingItem> GetStartingItems() const
    {
        return StartingItems;
    }

    /*------------------------ CHECKS ------------------------*/

    // Checks if there is any item equipped in a specific equipment slot.
    UFUNCTION(BlueprintCallable, Category = "ACF | Checks")
    bool HasAnyItemInEquipmentSlot(FGameplayTag itemSlot) const;

    // Returns the maximum number of items of a specific class that can be added.
    UFUNCTION(BlueprintCallable, Category = "ACF | Checks")
    int32 NumberOfItemCanTake(const TSubclassOf<AACFItem>& itemToCheck);

    // Checks if the character can switch to a ranged weapon.
    UFUNCTION(BlueprintPure, Category = "ACF | Checks")
    bool CanSwitchToRanged();

    // Checks if the character can switch to a melee weapon.
    UFUNCTION(BlueprintPure, Category = "ACF | Checks")
    bool CanSwitchToMelee();

    // Checks if a given item class can be equipped by evaluating attribute requirements.
    UFUNCTION(BlueprintPure, Category = "ACF | Checks")
    bool CanBeEquipped(const TSubclassOf<AACFItem>& equippable);

    // Checks if there is any equipped weapon on body of a given weapon type.
    UFUNCTION(BlueprintCallable, Category = "ACF | Checks")
    bool HasOnBodyAnyWeaponOfType(TSubclassOf<AACFWeapon> weaponClass) const;

    // Checks if the inventory contains enough items of the specified types.
    UFUNCTION(BlueprintCallable, Category = "ACF | Checks")
    bool HasEnoughItemsOfType(const TArray<FBaseItem>& ItemsToCheck);

    /*------------------------ DELEGATES ------------------------*/

    // Delegate broadcast when equipment changes.
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnEquipmentChanged OnEquipmentChanged;

    // Delegate broadcast when inventory changes.
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnInventoryChanged OnInventoryChanged;

    // Delegate broadcast when an item is added.
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnItemAdded OnItemAdded;

    // Delegate broadcast when an item is removed.
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnItemRemoved OnItemRemoved;

    // Delegate broadcast when equipped armor changes.
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnEquippedArmorChanged OnEquippedArmorChanged;

    /*------------------------ MOVESETS ------------------------*/
    // Returns the desired moveset tag based on equipped weapons.
    UFUNCTION(BlueprintCallable, Category = "ACF | Movesets")
    FGameplayTag GetCurrentDesiredMovesetTag() const;

    // Returns the desired moveset action tag.
    UFUNCTION(BlueprintCallable, Category = "ACF | Movesets")
    FGameplayTag GetCurrentDesiredMovesetActionTag() const;

    // Returns the desired moveset overlay tag.
    UFUNCTION(BlueprintCallable, Category = "ACF | Movesets")
    FGameplayTag GetCurrentDesiredOverlayTag() const;

    // ------------------------ Additional Code ------------------------
    virtual void BeginPlay() override; // Overridden to perform initialization routines.
    void GatherCharacterOwner();       // Caches a reference to the owning character.
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override; // Handles cleanup when play ends.
    void DestroyEquipment();           // Destroys current equipment items.

    // Returns the currently available equipment slots.
    UFUNCTION(BlueprintPure, Category = "ACF | Equipment")
    TArray<FGameplayTag> GetAvailableEquipmentSlot() const { return AvailableEquipmentSlot; }

    // Sets the currently available equipment slots.
    UFUNCTION(BlueprintCallable, Category = "ACF | Equipment")
    void SetAvailableEquipmentSlot(const TArray<FGameplayTag>& val) { AvailableEquipmentSlot = val; }

    // Returns allowed weapon types.
    UFUNCTION(BlueprintPure, Category = "ACF | Equipment")
    TArray<FGameplayTag> GetAllowedWeaponTypes() const { return AllowedWeaponTypes; }

    // Sets allowed weapon types.
    UFUNCTION(BlueprintCallable, Category = "ACF | Equipment")
    void SetAllowedWeaponTypes(const TArray<FGameplayTag>& val) { AllowedWeaponTypes = val; }

    /* A function that added by Nomad Dev Team
     * --- Switches which quickbar is active (and therefore which items are shown)
     * @param NewQuickbar: the quickbar index to activate (0 or 1)
     */
    UFUNCTION(BlueprintCallable, Category = "ACF | Quickbar")
    void SetActiveQuickbarEnum(EActiveQuickbar NewQuickbarEnum);

    UFUNCTION(Server, Reliable)
    void Server_SetActiveQuickbarEnum(EActiveQuickbar NewQuickbarEnum);

    /* A function that added by Nomad Dev Team
     * --- Returns the currently active quickbar index (0 or 1)
     */
    UFUNCTION(BlueprintCallable, Category = "ACF | Quickbar")
    EActiveQuickbar GetActiveQuickbarEnum() const { return ActiveQuickbarEnum; }

    /** 
     * A function added by Nomad Dev Team 
     * → Called when the player wants to switch which quickbar is “live.”
     *    NewQuickbarIndex should be 0 or 1.
     */
    UFUNCTION(BlueprintCallable, Category = "ACF | Quickbar")
    void UpdateEquippedItemsVisibility();

protected:
    // Array of equipment slot tags available to the character.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ACF)
    TArray<FGameplayTag> AvailableEquipmentSlot;

    // Array of gameplay tags for weapon types that the character can use.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ACF)
    TArray<FGameplayTag> AllowedWeaponTypes;

    // If true, all equipped items are destroyed when the character dies.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF|Drop")
    bool bDestroyItemsOnDeath = true;

    // If true, droppable inventory items will be dropped upon death.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF|Drop")
    bool bDropItemsOnDeath = true;

    // If true, drops will be collapsed into a single world item instead of separate items.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ACF|Drop")
    bool bCollapseDropInASingleWorldItem = true;

    // If true, certain equipped armors can hide or unhide the owner's main mesh.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    bool bUpdateMainMeshVisibility = true;

    // Pointer to the main skeletal mesh of the character.
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    USkeletalMeshComponent* MainCharacterMesh;

    // Maximum number of inventory slots.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Savegame, Category = ACF)
    int32 MaxInventorySlots = 40;

    // Whether an item picked up from the world is automatically equipped.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    bool bAutoEquipItem = true;

    // Maximum cumulative weight allowed in the inventory.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Savegame, Category = ACF)
    float MaxInventoryWeight = 180.f;

    // Array of starting items that the character has.
    UPROPERTY(EditAnywhere, meta = (TitleProperty = "ItemClass"), BlueprintReadWrite, Category = ACF)
    TArray<FStartingItem> StartingItems;

    // Blueprint native event called when the component is loaded.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    void OnComponentLoaded();
    virtual void BeginDestroy() override;

    // Local cache of inventory on clients to compare with replicated Inventory
    TArray<FInventoryItem> CachedInventory;

    /* A function added by Nomad Dev Team
     * Helper function to compare inventories and broadcast add/remove events
     */
    void HandleInventoryChanges(const TArray<FInventoryItem>& OldInventory, const TArray<FInventoryItem>& NewInventory);

private:
    // Inventory array holding all inventory items.
    UPROPERTY(SaveGame, Replicated, ReplicatedUsing = OnRep_Inventory)
    TArray<FInventoryItem> Inventory;

    // Equipment structure containing currently equipped items.
    UPROPERTY(Replicated, ReplicatedUsing = OnRep_Equipment)
    FEquipment Equipment;

    // Array holding modular parts (e.g., for equippable armor meshes).
    TArray<FModularPart> ModularMeshes;

    /* A member variable that added by Nomad Dev Team
     * --- Tracks which quickbar is in use for this character (replicated to clients)
     *   0 = combat quickbar, 1 = tools quickbar
     */
    UPROPERTY(ReplicatedUsing = OnRep_ActiveQuickbarEnum)
    EActiveQuickbar ActiveQuickbarEnum = EActiveQuickbar::Combat;

    UFUNCTION()
    void OnRep_ActiveQuickbarEnum();

    // Replication function called when Equipment is updated.
    UFUNCTION()
    void OnRep_Equipment();

    // Replication function called when Inventory is updated.
    UFUNCTION()
    void OnRep_Inventory();

    // Fills the ModularMeshes array by collecting all armor slot components from the owner.
    void FillModularMeshes();

    // Internal helper function: Retrieves a pointer to an inventory item based on the FInventoryItem structure.
    FInventoryItem* Internal_GetInventoryItem(const FInventoryItem& item);

    // Internal helper function: Retrieves a pointer to an inventory item by its unique GUID.
    FInventoryItem* Internal_GetInventoryItemByGuid(const FGuid& itemToSearch);

    // Internal function to destroy currently equipped items.
    void Internal_DestroyEquipment();
    
    // Finds all inventory items matching a given item class.
    TArray<FInventoryItem*> FindItemsByClass(const TSubclassOf<AACFItem>& itemToFind);

    // Pointer to the owning character.
    UPROPERTY()
    class ACharacter* CharacterOwner;

    /** The original skeletal mesh used by the Owner (if overridden). */
    UPROPERTY()
    class USkeletalMesh* originalMesh;

    // Replicated tag for the currently equipped equipment slot.
    UPROPERTY(Replicated)
    FGameplayTag CurrentlyEquippedSlotType;

    // Replicated float representing the current total weight of inventory items.
    UPROPERTY(Replicated)
    float currentInventoryWeight = 0.f;

    // Internal function to unequip an item (weapon, armor, or consumable) that is currently equipped.
    void RemoveItemFromEquipment(const FEquippedItem& item);

    // Marks an item in the inventory as equipped or unequipped and sets its slot tag.
    void MarkItemOnInventoryAsEquipped(const FInventoryItem& item, bool bIsEquipped, const FGameplayTag& itemSlot);

    // Internal function to add an item to the inventory, handling stacking and weight limits.
    int32 Internal_AddItem(const FBaseItem& item, bool bTryToEquip = false, float dropChancePercentage = 0.f);

    // Helper function to attach a weapon to the body (e.g., when sheathing).
    void AttachWeaponOnBody(AACFWeapon* WeaponToEquip);

    // Helper function to attach a weapon to the character's hand (when equipping).
    void AttachWeaponOnHand(AACFWeapon* _localWeapon);
    void SheathWeapon(AACFWeapon* Weapon);

    // Multicast function to add a skeletal mesh component for armor.
    UFUNCTION(NetMulticast, Reliable, Category = ACF)
    void AddSkeletalMeshComponent(TSubclassOf<class AACFArmor> ArmorClass, FGameplayTag itemSlot);

    // Multicast function called when armor is unequipped to reset its modular mesh.
    UFUNCTION(NetMulticast, Reliable, Category = ACF)
    void Internal_OnArmorUnequipped(const FGameplayTag& slot);

    // Spawns a world item near the owner (used when dropping items).
    void SpawnWorldItem(const TArray<FBaseItem>& items);

    // Uses an equipped consumable item on a target.
    void UseEquippedConsumable(FEquippedItem& EquipSlot, ACharacter* target);

    // Internal function to use a consumable item, then possibly remove it from the inventory.
    void Internal_UseItem(AACFConsumable* consumable, ACharacter* target, const FInventoryItem& Inventoryitem);

    
    // my addition code
    /*Move a item from one EquipmentComponent to another,usually used for a
     * storage*/
    //   UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable, Category =
    //   ACF)
    //  void MoveItemToAnotherInventory(UACFEquipmentComponent*
    //  OtherEquipmentComponent, class AACFItem* itemToMove, int32 count = 1);
};

