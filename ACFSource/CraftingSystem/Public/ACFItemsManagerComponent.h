// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFCraftRecipeDataAsset.h"
#include "ACFItemTypes.h"
#include "Components/ACFEquipmentComponent.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Items/ACFItem.h"

#include "ACFItemsManagerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemCrafted, const FACFCraftingRecipe&, recipe);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemSold, const FInventoryItem&, item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemPurchased, const FBaseItem&, item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUpgraded, const FInventoryItem&, item);

/** Forward Decleration */
class UACFStorageComponent;
class UACFCraftingComponent;

UCLASS(ClassGroup = (ACF), Blueprintable, meta = (BlueprintSpawnableComponent))
class CRAFTINGSYSTEM_API UACFItemsManagerComponent : public UActorComponent {
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UACFItemsManagerComponent();

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    // Reference to item database DataTable
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = ACF)
    UDataTable* ItemsDB;

public:
    /* Generates an array of FBaseItem matching the provided rules by selecting them from the provided ItemsDB
    returns true only if we are able to find matching items for ALL the provided rules*/
    UFUNCTION(BlueprintCallable, Category = ACF)
    bool GenerateItemsFromRules(const TArray<FACFItemGenerationRule>& generationRules, TArray<FBaseItem>& outItems);

    /* Generates an  FBaseItem matching the provide rule by selecting it from the provided ItemsDB
    returns true if at least one item is found*/
    UFUNCTION(BlueprintCallable, Category = ACF)
    bool GenerateItemFromRule(const FACFItemGenerationRule& generationRules, FBaseItem& outItems);

    /* Returns true if the provided itemSlot matches the provided slot rules*/
    UFUNCTION(BlueprintCallable, Category = ACF)
    bool DoesSlotMatchesRule(const FACFItemGenerationRule& generationRules, const FItemGenerationSlot& item);

    /*------------------- SERVER SIDE -----------------------------------*/
    UFUNCTION(Server, Reliable)
    void BuyItems(const FBaseItem& item, APawn* instigator, class UACFVendorComponent* vendorComp);

    UFUNCTION(Server, Reliable)
    void SellItemsToVendor(const FInventoryItem& itemTobeSold, APawn* instigator, int32 count, class UACFVendorComponent* vendorComp);

    /**
     * Nomad Dev Team
     * Server‐side RPC handler for crafting an item, consuming resources, and
     * routing the newly crafted output either into a storage component or
     * directly into the player's inventory.
     */
    UFUNCTION(Server, Reliable)
    void CraftItem(const FACFCraftingRecipe& ItemToCraft, APawn* instigator, UACFCraftingComponent* craftingComp, UACFStorageComponent* TargetStorage);

    UFUNCTION(Server, Reliable)
    void UpgradeItem(const FInventoryItem& itemToUpgrade, APawn* instigator, class UACFCraftingComponent* craftingComp);

    UFUNCTION(Server, Reliable)
    void ConstructBuildable(APawn* instigator, class UACFBuildableComponent* buildComp);

    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnItemCrafted OnItemCrafted;

    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnItemSold OnItemSold;

    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnItemPurchased OnItemPurchased;

    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnItemUpgraded OnItemUpgraded;
};
