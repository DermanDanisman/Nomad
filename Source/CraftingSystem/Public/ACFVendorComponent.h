// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once


#include "Components/ACFStorageComponent.h" // Include base class for storage functionality.
#include "CoreMinimal.h" // Basic Unreal Engine core types.

#include "ACFVendorComponent.generated.h"

// Forward declaration for currency-related component class
class UACFCurrencyComponent;
class UACFItemsManagerComponent;

/**
 * This class represents a vendor component that can manage buying and selling items with players.
 */
UCLASS(Blueprintable, ClassGroup = (ACF), meta = (BlueprintSpawnableComponent))
class CRAFTINGSYSTEM_API UACFVendorComponent : public UACFStorageComponent {
    GENERATED_BODY()

public:
    // Default constructor to set up component properties
    UACFVendorComponent();

    /*------------------- CHECKS -----------------------------------*/

    // Returns the maximum number of items the buyer can purchase based on available currency
    UFUNCTION(BlueprintCallable, Category = "ACF | Checks")
    int32 HowManyItemsCanBuy(const FBaseItem& itemsToBuy, const APawn* buyer) const;

    // Returns the maximum number of items the seller can sell based on available vendor currency
    UFUNCTION(BlueprintCallable, Category = "ACF | Checks")
    int32 HowManyItemsCanSell(const FBaseItem& itemsToSell, const APawn* seller) const;

    // Checks if the pawn can buy the specified items based on their currency
    UFUNCTION(BlueprintCallable, Category = "ACF | Checks")
    bool CanPawnBuyItems(const FBaseItem& itemsToBuy, const APawn* buyer) const;

    // Checks if the pawn can sell an item from their inventory to the vendor
    UFUNCTION(BlueprintCallable, Category = "ACF | Checks")
    bool CanPawnSellItemFromHisInventory(const FInventoryItem& itemTobeSold, const APawn* seller, int32 count = 1) const;

    /*------------------- SERVER SIDE -----------------------------------*/

    // Buy items from the vendor and update the inventory and currency
    UFUNCTION(BlueprintCallable, Category = "ACF | Vendor")
    void BuyItems(const FBaseItem& item, APawn* instigator);

    // Sell items to the vendor and update the inventory and vendor currency
    UFUNCTION(BlueprintCallable, Category = "ACF | Vendor")
    void SellItemsToVendor(const FInventoryItem& itemTobeSold, APawn* instigator, int32 count = 1);

    /*-------------------PLAYER STUFF-----------------------------------*/

    // Get the items manager component from the player controller
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    UACFItemsManagerComponent* GetItemsManager() const;

    // Get the vendor's current currency
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    float GetVendorCurrency() const;

    // Get the vendor's currency component
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    class UACFCurrencyComponent* GetVendorCurrencyComp() const {
        return sellerCurrency;
    }

    // Get the vendor's price multiplier on buying items
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    float GetVendorPriceMultiplierOnBuy() const {
        return PriceMultiplierOnBuy;
    }

    // Get the vendor's price multiplier on selling items
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    float GetVendorPriceMultiplierOnSell() const {
        return PriceMultiplierOnSell;
    }

    // Check if the vendor uses a currency component
    UFUNCTION(BlueprintPure, Category = "ACF | Getters")
    bool VendorUsesCurrency() const {
        return bUseVendorCurrencyComponent;
    }

    // Get the cost per unit for a specific item class
    UFUNCTION(BlueprintCallable, Category = "ACF | Getters")
    float GetItemCostPerUnit(const TSubclassOf<AACFItem>& itemClass) const;

    /*------------------- FUNCTION LIBRARY WRAPPERS-----------------------------------*/

    // Try to get item descriptor data for an item
    UFUNCTION(BlueprintCallable, Category = "ACF | Library")
    bool TryGetItemDescriptor(const FBaseItem& item, FItemDescriptor& outItemDescriptor) const;

    // Try to get item descriptor data by item class
    UFUNCTION(BlueprintCallable, Category = "ACF | Library")
    bool TryGetItemDescriptorByClass(const TSubclassOf<AACFItem>& ItemClass, FItemDescriptor& outItemDescriptor) const;

protected:
    // Multiplier applied when selling items to the player (defines item value)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    float PriceMultiplierOnSell = 1.f;

    // Whether or not this vendor uses a currency component
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    bool bUseVendorCurrencyComponent = true;

    // Multiplier applied when buying items from the player (defines item value)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    float PriceMultiplierOnBuy = 0.2f;

    // Called when the game starts or when this component is initialized
    virtual void BeginPlay() override;

private:
    // The currency component for this vendor (if any)
    TObjectPtr<UACFCurrencyComponent> sellerCurrency;
};
