#pragma once

#include "Components/ACFCurrencyComponent.h" // Base currency handling component.
#include "Components/ActorComponent.h" // Basic ActorComponent features.
#include "CoreMinimal.h"  // Core engine types.
#include "Items/ACFItem.h"  // Base item class.

#include "ACFStorageComponent.generated.h"

// Delegate broadcasts when stored items change (used for UI, etc.)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemsChanged, const TArray<FBaseItem>&, currentItems);

// Delegate broadcasts when storage becomes empty
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStorageEmpty);

/**
 * UACFStorageComponent
 * Handles storage of items and currency on an actor (like chests).
 * Supports adding/removing items, moving items to inventory, replication.
 */
UCLASS(ClassGroup = (ACF), Blueprintable, meta = (BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UACFStorageComponent : public UACFCurrencyComponent {
    GENERATED_BODY()

public:
    UACFStorageComponent();

protected:
    virtual void BeginPlay() override;

    // Stored items replicated to clients
    UPROPERTY(SaveGame, EditAnywhere, ReplicatedUsing = OnRep_Items, Category = ACF)
    TArray<FBaseItem> Items;

    // Called on load (can override in Blueprint)
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    void OnComponentLoaded();

    // Called on save (can override in Blueprint)
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    void OnComponentSaved();

public:
    /*------------------- SERVER SIDE -----------------------------------*/

    // Remove multiple items on server
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void RemoveItems(const TArray<FBaseItem>& inItems);

    // Remove a single item stack on server
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void RemoveItem(const FBaseItem& inItems);

    // Add a single item stack on server
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void AddItem(const FBaseItem& inItems);

    // Add multiple items on server
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void AddItems(const TArray<FBaseItem>& inItems);

    // Moves items from this storage to the pawn's equipment/inventory
    UFUNCTION(BlueprintCallable, Category = ACF)
    void MoveItemsToInventory(const TArray<FBaseItem>& inItems, UACFEquipmentComponent* equipComp);

    // Gather specific amount of currency and give it to another currency component
    UFUNCTION(BlueprintCallable, Category = ACF)
    void GatherCurrency(float amount, UACFCurrencyComponent* currencyComp);

    // Gather all currency and transfer it to another currency component
    UFUNCTION(BlueprintCallable, Category = ACF)
    void GatherAllCurrency(UACFCurrencyComponent* currencyComp);
    
    /*------------------------ STORAGE -----------------------------------------*/

    // Get the pawn's currency component (helper function)
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    class UACFCurrencyComponent* GetPawnCurrencyComponent(const APawn* pawn) const;

    // Get pawn's current currency amount
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    float GetPawnCurrency(const APawn* pawn) const;

    // Get pawn's equipment component (inventory)
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    class UACFEquipmentComponent* GetPawnEquipment(const APawn* pawn) const;

    // Get pawn's inventory items array
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    TArray<FInventoryItem> GetPawnInventory(const APawn* pawn) const;

    // Get stored items array
    UFUNCTION(BlueprintPure, Category = ACF)
    TArray<FBaseItem> GetItems() const
    {
        return Items;
    }

    // Returns true if storage has no items and no currency
    UFUNCTION(BlueprintPure, Category = ACF)
    bool IsStorageEmpty();

    // Event triggered when items change in storage (for UI updates)
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnItemsChanged OnItemChanged;

    // Event triggered when storage becomes empty
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnStorageEmpty OnStorageEmpty;

    // Called when currency amount changes; we override to check empty state
    virtual void HandleCurrencyChanged() override;

    /**
     * Nomad Dev Team
     * Adds items to storage by item class and count.
     * @param ItemClass - The class of item to add.
     * @param Count - The amount of the item to add.
     */
    UFUNCTION(BlueprintCallable, Category="ACF | Storage")
    void AddItemToStorageByClass(TSubclassOf<AACFItem> ItemClass, int32 Count);

private:
    // RepNotify handler called when Items replicated on clients.
    UFUNCTION()
    void OnRep_Items();

    // Checks if storage is empty, broadcasts events as needed.
    void CheckEmpty();
};