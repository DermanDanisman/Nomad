#include "Components/ACFStorageComponent.h"
#include "Components/ACFCurrencyComponent.h"
#include "Components/ACFEquipmentComponent.h"
#include "Items/ACFItem.h"
#include "Net/UnrealNetwork.h"
#include <GameFramework/Pawn.h>
#include "ACFItemSystemFunctionLibrary.h"

// Constructor disables tick, enables replication
UACFStorageComponent::UACFStorageComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UACFStorageComponent::BeginPlay()
{
    Super::BeginPlay();

    // On start, check if storage is empty and notify
    CheckEmpty();
}

void UACFStorageComponent::OnComponentLoaded_Implementation()
{
    // Empty default; override in blueprint for custom load logic
}

void UACFStorageComponent::OnComponentSaved_Implementation()
{
    // Empty default; override in blueprint for custom save logic
}

void UACFStorageComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Replicate Items array to clients
    DOREPLIFETIME(UACFStorageComponent, Items);
}

// Server-side removal of multiple items by decreasing counts or removing stacks
void UACFStorageComponent::RemoveItems_Implementation(const TArray<FBaseItem>& inItems)
{
    TArray<FBaseItem> pendingRemove;

    for (auto& item : inItems) {
        FBaseItem* currentItem = Items.FindByKey(item);

        if (currentItem) {
            currentItem->Count -= item.Count;

            if (currentItem->Count <= 0) {
                pendingRemove.Add(*currentItem);
            }
        }
    }

    for (auto& removed : pendingRemove) {
        Items.Remove(removed);
    }

    OnItemChanged.Broadcast(Items);
    CheckEmpty();
}

// Server-side removal of a single item stack partially or fully
void UACFStorageComponent::RemoveItem_Implementation(const FBaseItem& inItem)
{
    FBaseItem* currentItem = Items.FindByKey(inItem);
    if (currentItem) {
        currentItem->Count -= inItem.Count;

        if (currentItem->Count <= 0) {
            int32 index = Items.IndexOfByKey(inItem);
            if (Items.IsValidIndex(index)) {
                Items.RemoveAt(index);
            }
        }

        OnItemChanged.Broadcast(Items);
        CheckEmpty();
    }
}

// Add multiple items on server by calling AddItem for each
void UACFStorageComponent::AddItems_Implementation(const TArray<FBaseItem>& inItems)
{
    for (const auto& item : inItems) {
        AddItem(item);
    }
}

// Add a single item stack: stack with existing if found, else add new
void UACFStorageComponent::AddItem_Implementation(const FBaseItem& inItem)
{
    FBaseItem* currentItem = Items.FindByKey(inItem);

    if (currentItem) {
        currentItem->Count += inItem.Count;
    } else {
        Items.Add(inItem);
    }

    OnItemChanged.Broadcast(Items);
}

// Move items from storage to equipment component inventory
void UACFStorageComponent::MoveItemsToInventory(const TArray<FBaseItem>& inItems, UACFEquipmentComponent* equipComp)
{
    if (equipComp) {
        equipComp->MoveItemsToInventory(inItems, this);
    }
}

// Called on clients when Items replicate; broadcast update & check empty
void UACFStorageComponent::OnRep_Items()
{
    OnItemChanged.Broadcast(Items);
    CheckEmpty();
}

// Broadcast OnStorageEmpty if storage empty (items + currency)
void UACFStorageComponent::CheckEmpty()
{
    if (IsStorageEmpty()) {
        OnStorageEmpty.Broadcast();
    }
}

// Returns true if no items and currency <= 0
bool UACFStorageComponent::IsStorageEmpty()
{
    return GetItems().IsEmpty() && GetCurrentCurrencyAmount() <= 0;
}

// Transfer specified currency amount to another currency component
void UACFStorageComponent::GatherCurrency(float amount, UACFCurrencyComponent* currencyComp)
{
    currencyComp->AddCurrency(amount);
    RemoveCurrency(amount);
}

// Transfer all currency to another currency component
void UACFStorageComponent::GatherAllCurrency(UACFCurrencyComponent* currencyComp)
{
    GatherCurrency(GetCurrentCurrencyAmount(), currencyComp);
}

// Helper to get pawn currency component via external function library
UACFCurrencyComponent* UACFStorageComponent::GetPawnCurrencyComponent(const APawn* pawn) const
{
    return UACFItemSystemFunctionLibrary::GetPawnCurrencyComponent(pawn);
}

// Helper to get pawn currency amount via external function library
float UACFStorageComponent::GetPawnCurrency(const APawn* pawn) const
{
    return UACFItemSystemFunctionLibrary::GetPawnCurrency(pawn);
}

// Helper to get pawn equipment component via external function library
UACFEquipmentComponent* UACFStorageComponent::GetPawnEquipment(const APawn* pawn) const
{
    return UACFItemSystemFunctionLibrary::GetPawnEquipment(pawn);
}

// Helper to get pawn inventory via their equipment component
TArray<FInventoryItem> UACFStorageComponent::GetPawnInventory(const APawn* pawn) const
{
    const UACFEquipmentComponent* equipComp = GetPawnEquipment(pawn);
    if (equipComp) {
        return equipComp->GetInventory();
    }
    return TArray<FInventoryItem>();
}

// Override to handle currency changes; triggers empty check
void UACFStorageComponent::HandleCurrencyChanged()
{
    CheckEmpty();
}

/**
 * Nomad Dev Team
 * Adds a number of items of the specified class into this storage component.
 *
 * @param ItemClass   The subclass of AACFItem to add.
 * @param Count       How many of that item to add.
 */
void UACFStorageComponent::AddItemToStorageByClass(TSubclassOf<AACFItem> ItemClass, int32 Count)
{
    // 1) Validate inputs: ensure we have a valid item class and a positive count.
    //    If either check fails, we do nothing to avoid invalid state.
    if (!ItemClass || Count <= 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[UACFStorageComponent] AddItemToStorageByClass called with invalid parameters: Class=%s, Count=%d"),
            *GetNameSafe(ItemClass), Count);
        return;
    }

    // 2) Wrap the raw class/count into our FBaseItem struct,
    //    which carries both the class reference and the quantity.
    FBaseItem NewItem(ItemClass, Count);

    // 3) Delegate to the existing AddItem server‐RPC method to actually
    //    insert (or stack) the new items into the Items array.
    //    This call handles replication, merging stacks, and broadcasting OnItemChanged.
    AddItem(NewItem);
}