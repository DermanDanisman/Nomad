// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "ACFItemsManagerComponent.h"
#include "ACFCraftingComponent.h"
#include "Components/ACFCurrencyComponent.h"
#include "ACFVendorComponent.h"
#include "Components/ACFEquipmentComponent.h"
#include "Engine/DataTable.h"
#include "GameplayTagsManager.h"
#include "ACFBuildableComponent.h"

// Sets default values for this component's properties
UACFItemsManagerComponent::UACFItemsManagerComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = false;

    // ...
}

// Called when the game starts
void UACFItemsManagerComponent::BeginPlay()
{
    Super::BeginPlay();
}

bool UACFItemsManagerComponent::GenerateItemsFromRules(const TArray<FACFItemGenerationRule>& generationRules, TArray<FBaseItem>& outItems)
{
    outItems.Empty();
    bool bIsSuccessful = true;

    if (generationRules.Num() == 0) {
        UE_LOG(LogTemp, Warning, TEXT("Missing generation rules! - UACFItemsManagerComponent"));
        return false;
    }

    for (auto genRule : generationRules) {
        FBaseItem outItem;
        if (GenerateItemFromRule(genRule, outItem)) {
            outItems.Add(outItem);
        } else {
            bIsSuccessful = false;
        }
    }
    return bIsSuccessful;
}

bool UACFItemsManagerComponent::GenerateItemFromRule(const FACFItemGenerationRule& generationRules, FBaseItem& outItem)
{
    TArray<FItemGenerationSlot> matchingItems;
    if (ItemsDB) {
        const auto items = ItemsDB->GetRowMap();
        for (const auto itemItr : items) {
            FItemGenerationSlot* itemSlot = (FItemGenerationSlot*)itemItr.Value;
            if (itemSlot) {
                if (DoesSlotMatchesRule(generationRules, *itemSlot)) {
                    matchingItems.Add(*itemSlot);
                }
            } else {
                return false;
            }
        }

        if (matchingItems.Num() == 0) {
            UE_LOG(LogTemp, Warning, TEXT("No Matching Items in DB! - UACFItemsManagerComponent"));
            return false;
        }

        const int32 selectedSlot = FMath::RandRange(0, matchingItems.Num() - 1);
        const int32 selectedCount = FMath::RandRange(generationRules.MinItemCount, generationRules.MaxItemCount);
        if (matchingItems.IsValidIndex(selectedSlot) && selectedCount > 0) {
            const TSubclassOf<AACFItem> ItemClass = matchingItems[selectedSlot].ItemClass.LoadSynchronous();
            if (ItemClass) {
                outItem = FBaseItem(ItemClass, selectedCount);
                return true;
            }
        }
    } else {
        UE_LOG(LogTemp, Error, TEXT("No  ItemsDB! in ItemsManager!!!!- UACFItemsManagerComponent"));
    }
    return false;
}

bool UACFItemsManagerComponent::DoesSlotMatchesRule(const FACFItemGenerationRule& generationRules, const FItemGenerationSlot& item)
{
    return (
        (item.Category == generationRules.Category || UGameplayTagsManager::Get().RequestGameplayTagChildren(generationRules.Category).HasTag(item.Category))
        && (item.Rarity == generationRules.Rarity || UGameplayTagsManager::Get().RequestGameplayTagChildren(generationRules.Rarity).HasTag(item.Rarity)));
}

/*------------------- SERVER SIDE -----------------------------------*/
// void UACFItemsManagerComponent::AddRecipe_Implementation(const FName& receiptKey, class UACFCraftingComponent* craftingComp)
// {
// 	FACFCraftingRecipe outRecipe;
// 	if (TryGetRecipe(receiptKey, outRecipe) && craftingComp) {
// 		craftingComp->AddNewRecipe(outRecipe);
// 	}
// }
//
// bool UACFItemsManagerComponent::AddRecipe_Validate(const FName& receiptKey, class UACFCraftingComponent* craftingComp)
// {
// 	return true;
// }

void UACFItemsManagerComponent::SellItemsToVendor_Implementation(const FInventoryItem& itemTobeSold, APawn* instigator, int32 count /*= 1*/, UACFVendorComponent* vendorComp)
{
    if (!vendorComp) {
        return;
    }

    if (!vendorComp->CanPawnSellItemFromHisInventory(itemTobeSold, instigator, count)) {
        return;
    }

    UACFEquipmentComponent* equipComp = vendorComp->GetPawnEquipment(instigator);
    UACFCurrencyComponent* currencyComp = vendorComp->GetPawnCurrencyComponent(instigator);
    const float totalCost = itemTobeSold.ItemInfo.CurrencyValue * count * vendorComp->GetVendorPriceMultiplierOnBuy();
    if (equipComp && currencyComp) {
        equipComp->RemoveItem(itemTobeSold, count);
        currencyComp->AddCurrency(totalCost);
        if (vendorComp->VendorUsesCurrency() && vendorComp->GetVendorCurrencyComp()) {
            vendorComp->GetVendorCurrencyComp()->RemoveCurrency(totalCost);
        }
        vendorComp->AddItem(FBaseItem(itemTobeSold.ItemClass, count));
        OnItemSold.Broadcast(itemTobeSold);
    }
}

void UACFItemsManagerComponent::BuyItems_Implementation(const FBaseItem& item, APawn* instigator, UACFVendorComponent* vendorComp)
{
    if (!vendorComp) {
        return;
    }

    if (!vendorComp->CanPawnBuyItems(item, instigator)) {
        return;
    }

    FItemDescriptor itemToBuyDesc;
    if (vendorComp->TryGetItemDescriptor(item, itemToBuyDesc)) {
        const float totalCost = (itemToBuyDesc.CurrencyValue * item.Count * vendorComp->GetVendorPriceMultiplierOnSell());
        UACFCurrencyComponent* currencyComp = vendorComp->GetPawnCurrencyComponent(instigator);
        UACFEquipmentComponent* equipComp = vendorComp->GetPawnEquipment(instigator);
        if (currencyComp && equipComp) {
            currencyComp->RemoveCurrency(totalCost);
            equipComp->AddItemToInventory(item);
            vendorComp->RemoveItem(item);
            if (vendorComp->GetVendorCurrencyComp()) {
                vendorComp->GetVendorCurrencyComp()->AddCurrency(totalCost);
            }
            OnItemPurchased.Broadcast(item);
            return;
        }
    }
    return;
}

/**
 * Nomad Dev Team
 * Server‐side RPC handler for crafting an item, consuming resources, and
 * routing the newly crafted output either into a storage component or
 * directly into the player's inventory.
 */
void UACFItemsManagerComponent::CraftItem_Implementation(
    const FACFCraftingRecipe& ItemToCraft,
    APawn* instigator,
    UACFCraftingComponent* craftingComp,
    UACFStorageComponent* TargetStorage)
{
    // 1) Early‐out safety checks: ensure we have a valid crafting component and instigating pawn.
    //    Without these, we cannot proceed with crafting logic.
    if (!craftingComp || !instigator)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[UACFItemsManagerComponent::CraftItem] Invalid craftingComp (%s) or instigator (%s)"),
            *GetNameSafe(craftingComp), *GetNameSafe(instigator));
        return;
    }

    // 2) Resource validation: check that the pawn has enough materials and currency to craft.
    //    Uses the same logic exposed to Blueprints via CanPawnCraftItem().
    if (!craftingComp->CanPawnCraftItem(ItemToCraft, instigator))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[UACFItemsManagerComponent::CraftItem] Pawn '%s' cannot craft recipe '%s'"),
            *instigator->GetName(), *ItemToCraft.OutputItem.ItemClass->GetName());
        return;
    }

    // 3) Retrieve the pawn's equipment component to manipulate inventory items.
    UACFEquipmentComponent* equipComp = craftingComp->GetPawnEquipment(instigator);
    if (!equipComp)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[UACFItemsManagerComponent::CraftItem] Failed to get EquipmentComponent for '%s'"),
            *instigator->GetName());
        return;
    }

    // 4) Deduct crafting cost in currency: remove the appropriate amount from the pawn.
    //    PriceMultiplierOnSell comes from the vendor logic in the crafting component.
    const float Cost = craftingComp->GetVendorPriceMultiplierOnSell() * ItemToCraft.CraftingCost;
    craftingComp->GetPawnCurrencyComponent(instigator)->RemoveCurrency(Cost);

    // 5) Consume the required input items from the pawn's inventory.
    //    This loop is handled by the EquipmentComponent's ConsumeItems method.
    equipComp->ConsumeItems(ItemToCraft.RequiredItems);

    // 6) Output routing:
    //    - If a valid TargetStorage was provided (e.g., a chest or crafting station),
    //      add the crafted output there.
    //    - Otherwise, fall back to giving the item directly to the pawn's inventory.
    // Nomad Dev Team: extended to support sending crafted items into storage.
    if (TargetStorage && TargetStorage->IsValidLowLevel())
    {
        // Creates or stacks the new item(s) inside the storage component.
        TargetStorage->AddItemToStorageByClass(
            ItemToCraft.OutputItem.ItemClass,
            ItemToCraft.OutputItem.Count);
    }
    else
    {
        // Fallback: deposit crafted item(s) into the player's own inventory.
        equipComp->AddItemToInventoryByClass(
            ItemToCraft.OutputItem.ItemClass,
            ItemToCraft.OutputItem.Count);
    }

    // 7) Notify any listeners (UI, Blueprint, etc.) that crafting has completed.
    //    Broadcasts the recipe so subscribers can react (e.g., update UI, play VFX).
    OnItemCrafted.Broadcast(ItemToCraft);

    UE_LOG(LogTemp, Log,
        TEXT("[UACFItemsManagerComponent::CraftItem] Pawn '%s' crafted %d x '%s'"),
        *instigator->GetName(),
        ItemToCraft.OutputItem.Count,
        *ItemToCraft.OutputItem.ItemClass->GetName());
}

void UACFItemsManagerComponent::UpgradeItem_Implementation(const FInventoryItem& itemToUpgrade, APawn* instigator, class UACFCraftingComponent* craftingComp)
{
    if (!craftingComp) {
        return;
    }

    if (!craftingComp->CanPawnUpgradeItem(itemToUpgrade, instigator)) {
        return;
    }

    UACFEquipmentComponent* equipComp = craftingComp->GetPawnEquipment(instigator);
    UACFCurrencyComponent* currencyComp = craftingComp->GetPawnCurrencyComponent(instigator);
    if (equipComp && currencyComp) {
        FItemDescriptor itemInfo = itemToUpgrade.ItemInfo;

        if (itemInfo.NextLevelClass) {
            equipComp->ConsumeItems(itemInfo.RequiredItemsToUpgrade);
            currencyComp->RemoveCurrency(craftingComp->GetVendorPriceMultiplierOnSell() * itemInfo.UpgradeCurrencyCost);
            equipComp->RemoveItem(itemToUpgrade, 1);
            equipComp->AddItemToInventoryByClass(itemInfo.NextLevelClass, 1);
            OnItemUpgraded.Broadcast(itemToUpgrade);
            return;
        }
    }
    return;
}

void UACFItemsManagerComponent::ConstructBuildable_Implementation(APawn* instigator, class UACFBuildableComponent* buildComp)
{
    if (buildComp && instigator && buildComp->CanBeBuildByPawn(instigator)) {
        buildComp->GetPawnCurrencyComponent(instigator)->RemoveCurrency(buildComp->GetBuildingCost());
        TArray<FBaseItem> outItems;
        buildComp->GetBuildingRequirements(outItems);
        buildComp->GetPawnEquipment(instigator)->ConsumeItems(outItems);
        buildComp->SetBuildingState(EBuildableState::EBuilded);
    }
}
