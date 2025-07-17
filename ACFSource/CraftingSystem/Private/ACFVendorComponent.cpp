// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "ACFVendorComponent.h"
#include "Components/ACFEquipmentComponent.h"
#include "Components/ACFCurrencyComponent.h"
#include <Kismet/GameplayStatics.h>
#include "ACFItemsManagerComponent.h"
#include "ACFItemSystemFunctionLibrary.h"
#include <GameFramework/Pawn.h>

// Constructor: Initialize the component's properties
UACFVendorComponent::UACFVendorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// BeginPlay: Initialize the currency component if not already set
void UACFVendorComponent::BeginPlay()
{
    Super::BeginPlay();

    // Try to find the currency component attached to the owner of this vendor.
    sellerCurrency = GetOwner()->FindComponentByClass<UACFCurrencyComponent>();
    if (!sellerCurrency && bUseVendorCurrencyComponent) {
        const APawn* pawn = Cast<APawn>(GetOwner());
        if (pawn) {
            sellerCurrency = pawn->GetController()->FindComponentByClass<UACFCurrencyComponent>();
        }
    }

    // Log an error if the vendor has no currency component and one is required.
    if (!sellerCurrency && bUseVendorCurrencyComponent) {
        UE_LOG(LogTemp, Error, TEXT("Seller with No Currencies! - UACFVendorComponent::BeginPlay"))
    }
}

/*------------------- CHECKS -----------------------------------*/

// Calculate how many items the buyer can purchase based on available currency
int32 UACFVendorComponent::HowManyItemsCanBuy(const FBaseItem& itemsToBuy, const APawn* buyer) const
{
    FItemDescriptor outItem;
    if (TryGetItemDescriptor(itemsToBuy, outItem)) {
        const float pawnCurrency = GetPawnCurrency(buyer);
        const float unitCost = GetItemCostPerUnit(itemsToBuy.ItemClass);
        if (unitCost <= 0.f) {
            return itemsToBuy.Count; // If the item costs nothing, the player can buy all available
        }
        const int32 maxAmount = trunc(pawnCurrency / unitCost); // Calculate the maximum number the player can afford
        return FMath::Min(maxAmount, itemsToBuy.Count); // Return the lesser of the two (what they can afford vs. how many are available)
    }
    return 0;
}

// Check how many items the vendor can buy from the player
int32 UACFVendorComponent::HowManyItemsCanSell(const FBaseItem& itemsToSell, const APawn* seller) const
{
    FItemDescriptor outItem;
    if (!bUseVendorCurrencyComponent) {
        return itemsToSell.Count; // If vendor does not have a currency component, they can buy unlimited items
    }
    if (TryGetItemDescriptor(itemsToSell, outItem) && sellerCurrency) {
        const float pawnCurrency = sellerCurrency->GetCurrentCurrencyAmount(); // Get the vendor's available currency
        const int32 maxAmount = trunc(pawnCurrency / GetItemCostPerUnit(itemsToSell.ItemClass)); // Calculate how many the vendor can afford
        return FMath::Min(maxAmount, itemsToSell.Count); // Return the lesser of what they can afford vs. what's available
    }
    return 0;
}

// Check if the player can buy the item based on their available currency and the item's price
bool UACFVendorComponent::CanPawnBuyItems(const FBaseItem& itemsToBuy, const APawn* buyer) const
{
    if (!Items.Contains(itemsToBuy.ItemClass)) {
        return false; // The vendor doesn't have the item to sell
    }
    const FBaseItem* tobeSold = Items.FindByKey(itemsToBuy.ItemClass);
    if (tobeSold && tobeSold->Count >= itemsToBuy.Count) {
        if (PriceMultiplierOnSell == 0.f) {
            return true; // If the price multiplier is 0, the item is free
        }
        FItemDescriptor itemToBuyDesc;
        if (TryGetItemDescriptor(itemsToBuy, itemToBuyDesc)) {
            // Check if the player has enough currency to buy the items
            return (itemToBuyDesc.CurrencyValue * itemsToBuy.Count * PriceMultiplierOnSell) <= GetPawnCurrency(buyer);
        }
    }
    return false;
}

// Check if the player can sell an item from their inventory to the vendor
bool UACFVendorComponent::CanPawnSellItemFromHisInventory(const FInventoryItem& itemTobeSold, const APawn* seller, int32 count /*= 1*/) const
{
    UACFEquipmentComponent* equipComp = GetPawnEquipment(seller);
    if (!equipComp) {
        return false; // No equipment component, can't sell
    }

    if (bUseVendorCurrencyComponent && !sellerCurrency) {
        return false; // Vendor requires currency but doesn't have a currency component
    }

    if (bUseVendorCurrencyComponent) {
        // Check if the seller has enough currency to buy the item and if they have enough stock of the item
        return itemTobeSold.Count >= count && itemTobeSold.ItemInfo.CurrencyValue * count * PriceMultiplierOnBuy <= sellerCurrency->GetCurrentCurrencyAmount();
    }

    return itemTobeSold.Count >= count && itemTobeSold.ItemInfo.CurrencyValue;
}

/* ----------- TO SERVER---------------*/

// Function to handle buying items by the player
void UACFVendorComponent::BuyItems(const FBaseItem& item, APawn* instigator)
{
    if (GetItemsManager()) {
        GetItemsManager()->BuyItems(item, instigator, this); // Call the items manager to handle the actual buying logic
    }
}

// Function to handle selling items to the vendor
void UACFVendorComponent::SellItemsToVendor(const FInventoryItem& itemTobeSold, APawn* instigator, int32 count /*= 1*/)
{
    if (GetItemsManager()) {
        GetItemsManager()->SellItemsToVendor(itemTobeSold, instigator, count, this); // Call the items manager to handle the selling logic
    }
}

/*-------------------PLAYER STUFF-----------------------------------*/

// Get the items manager component
UACFItemsManagerComponent* UACFVendorComponent::GetItemsManager() const
{
    const APlayerController* gameState = UGameplayStatics::GetPlayerController(this, 0); // Get the player controller
    if (gameState) {
        return gameState->FindComponentByClass<UACFItemsManagerComponent>(); // Get the items manager component from the player
    }
    return nullptr;
}

// Get the vendor's current currency amount
float UACFVendorComponent::GetVendorCurrency() const
{
    if (sellerCurrency) {
        return sellerCurrency->GetCurrentCurrencyAmount(); // Return the vendor's currency if available
    }
    return -1.f; // Return -1 if the vendor doesn't have currency
}

// Try to get the item descriptor (details like price, weight) for an item
bool UACFVendorComponent::TryGetItemDescriptor(const FBaseItem& item, FItemDescriptor& outItemDescriptor) const
{
    if (item.ItemClass) {
        return TryGetItemDescriptorByClass(item.ItemClass, outItemDescriptor); // Retrieve item descriptor by class
    }
    return false;
}

// Try to get the item descriptor by item class
bool UACFVendorComponent::TryGetItemDescriptorByClass(const TSubclassOf<AACFItem>& ItemClass, FItemDescriptor& outItemDescriptor) const
{
    return UACFItemSystemFunctionLibrary::GetItemData(ItemClass, outItemDescriptor); // Use the library function to get item descriptor
}

// Get the cost per unit of an item class
float UACFVendorComponent::GetItemCostPerUnit(const TSubclassOf<AACFItem>& itemClass) const
{
    FItemDescriptor outItem;
    if (TryGetItemDescriptorByClass(itemClass, outItem)) {
        return outItem.CurrencyValue * PriceMultiplierOnSell; // Calculate the cost based on the item's value and the vendor's price multiplier
    }
    return -1.f; // Return -1 if item descriptor not found
}
