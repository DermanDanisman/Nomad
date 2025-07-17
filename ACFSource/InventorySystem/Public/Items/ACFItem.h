// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFItemTypes.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include <Engine/DataTable.h>
#include <GameplayTagContainer.h>
#include <AbilitySystemComponent.h>

#include "ACFItem.generated.h"

class UGameplayEffect;
struct FGameplayEffectSpecHandle;
class AACFItem;
class UTexture2D;
class UStaticMesh;
class UPrimaryDataAsset;

/**
 * Base item information with unique GUID, class and count.
 */
USTRUCT(BlueprintType, meta=(HasNativeStructInitializer))
struct FBaseItem : public FTableRowBase
{
    GENERATED_BODY()
    
    /** Default constructor initializes GUID, empty class, count=1 */
    FBaseItem()
        : ItemClass(nullptr)
        , Count(1)
        , ItemGuid(FGuid::NewGuid())
    {}

    /** Construct with forced GUID and count */
    FBaseItem(const TSubclassOf<AACFItem>& InItem, const FGuid& ForcedGuid, int32 InCount)
        : ItemClass(InItem)
        , Count(InCount)
        , ItemGuid(ForcedGuid)
    {}

    /** Construct with random GUID and specified count */
    FBaseItem(const TSubclassOf<AACFItem>& InItem, int32 InCount)
        : ItemClass(InItem)
        , Count(InCount)
        , ItemGuid(FGuid::NewGuid())
    {}

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = ACF)
    TSubclassOf<AACFItem> ItemClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = ACF)
    int32 Count;

    FORCEINLINE bool operator==(const FBaseItem& Other) const { return ItemClass == Other.ItemClass; }
    FORCEINLINE bool operator!=(const FBaseItem& Other) const { return ItemClass != Other.ItemClass; }
    FORCEINLINE bool operator==(const TSubclassOf<AACFItem>& Other) const { return ItemClass == Other; }
    FORCEINLINE bool operator!=(const TSubclassOf<AACFItem>& Other) const { return ItemClass != Other; }

protected:
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = ACF, meta = (IgnoreForMemberInitializationTest))
    FGuid ItemGuid;
};

/**
 * Descriptor for an item: icon, mesh, descriptive text, stack limits, etc.
 */
USTRUCT(BlueprintType)
struct FItemDescriptor : public FTableRowBase
{
    GENERATED_BODY()

public:
    /** Default constructor initializes all properties to safe defaults */
    FItemDescriptor()
        : ThumbNail(nullptr)
        , Scale(FVector2D(1.f, 1.f))
        , Name(FText::GetEmpty())
        , Description(FText::GetEmpty())
        , ItemType(EItemType::Other)
        , MaxInventoryStack(1)
        , ItemWeight(5.f)
        , WorldMesh(nullptr)
        , bDroppable(true)
        , bUpgradable(false)
        , UpgradeCurrencyCost(0.f)
        , RequiredItemsToUpgrade()
        , NextLevelClass(nullptr)
        , bSellable(true)
        , CurrencyValue(5.f)
        , Rarity()
        , ItemSlots()
        , GameSpecificData(nullptr)
    {}

    /** Icon to display in UI */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ACF|Icon")
    UTexture2D* ThumbNail;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ACF|Icon")
    FVector2D Scale;

    /** Name of the item */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    FText Name;

    /** Long description of the item */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    FText Description;

    /** Type of item */
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    EItemType ItemType;

    /** Max stack size in inventory */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    uint8 MaxInventoryStack;

    /** Weight of the item */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    float ItemWeight;

    /** Mesh to spawn when dropped */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    UStaticMesh* WorldMesh;

    /** Can be dropped in world */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    bool bDroppable;

    /** Can be upgraded */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    bool bUpgradable;

    /** Currency cost if upgradable */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "bUpgradable"), Category = ACF)
    float UpgradeCurrencyCost;

    /** Items required for upgrade */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "bUpgradable"), Category = ACF)
    TArray<FBaseItem> RequiredItemsToUpgrade;

    /** Class after upgrade */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "bUpgradable"), Category = ACF)
    TSubclassOf<AACFItem> NextLevelClass;

    /** Can be sold to vendors */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    bool bSellable;

    /** Base buy/sell price */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "bSellable"), Category = ACF)
    float CurrencyValue;

    /** Rarity tag */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    FGameplayTag Rarity;

    /** Equip/inventory slot tags */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    TArray<FGameplayTag> ItemSlots;
    
    TArray<FGameplayTag> GetPossibleItemSlots() const { return ItemSlots; }

    /** Game-specific data asset */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACF)
    UPrimaryDataAsset* GameSpecificData;
};

UCLASS()
class INVENTORYSYSTEM_API AACFItem : public AActor {
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AACFItem();

    UFUNCTION(BlueprintPure, Category = ACF)
    virtual FORCEINLINE class UTexture2D* GetThumbnailImage() const { return ItemInfo.ThumbNail; }

    UFUNCTION(BlueprintPure, Category = ACF)
    virtual FORCEINLINE FText GetItemName() const { return ItemInfo.Name; }

    UFUNCTION(BlueprintPure, Category = ACF)
    virtual FORCEINLINE FText GetItemDescription() const { return ItemInfo.Description; }

    UFUNCTION(BlueprintPure, Category = ACF)
    virtual FORCEINLINE EItemType GetItemType() const { return ItemInfo.ItemType; }

    UFUNCTION(BlueprintPure, Category = ACF)
    virtual FORCEINLINE class APawn* GetItemOwner() const { return ItemOwner; }

    UFUNCTION(BlueprintPure, Category = ACF)
    virtual FORCEINLINE FItemDescriptor GetItemInfo() const { return ItemInfo; }

    UFUNCTION(BlueprintPure, Category = ACF)
    virtual FORCEINLINE TArray<FGameplayTag> GetPossibleItemSlots() const { return ItemInfo.GetPossibleItemSlots(); }

    UFUNCTION(BlueprintCallable, Category = ACF)
    virtual void SetItemDescriptor(const FItemDescriptor& itemDesc)
    {
        ItemInfo = itemDesc;
    }

    virtual void SetItemOwner(APawn* inOwner)
    {
        ItemOwner = inOwner;
    }

protected:
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ItemOwner, Category = ACF)
    class APawn* ItemOwner;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ACF | Item")
    FItemDescriptor ItemInfo;

    UFUNCTION()
    virtual void OnRep_ItemOwner();


    FActiveGameplayEffectHandle AddGASModifierToOwner(const TSubclassOf<UGameplayEffect>& gameplayModifier );
    void RemoveGASModifierToOwner(const FActiveGameplayEffectHandle& modifierHandle);
    UAbilitySystemComponent* GetAbilityComponent() const;


    //     UFUNCTION(BlueprintCallable, CallInEditor)
    //     void AutoGenerateThumbnail();
};
