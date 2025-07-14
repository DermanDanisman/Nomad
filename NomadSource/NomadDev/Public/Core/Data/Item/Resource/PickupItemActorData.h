// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ARSTypes.h"
#include "GameplayTagContainer.h"
#include "Core/Data/Item/BaseItemData.h"
#include "Engine/DataAsset.h"
#include "PickupItemActorData.generated.h"

/**
 * All settings for a pickup actor, inlined into owning Data Asset.
 */
USTRUCT(BlueprintType)
struct FPickupActorInfo
{
    GENERATED_BODY()

    FPickupActorInfo()
        : bPickOnOverlap(true)
        , bAutoEquipOnPick(true)
        , OnPickupEffect()
        , OnPickupBuff()
        , ItemName(FText())
        , bDestroyAfterGathering(true)
        , Items()
    {}

    /* === Pickup Behavior === */
    /** If true, the item is picked up automatically when the player overlaps it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Behavior")
    bool bPickOnOverlap;

    /** If true, the item is automatically equipped on pickup. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Behavior")
    bool bAutoEquipOnPick;

    /* === Stat Effects === */
    /** Instant-stat modifications (e.g., health dmg/heal) applied when picked up. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    TArray<FStatisticValue> OnPickupEffect;

    /** Timed attribute set modifiers (buffs/debuffs) applied on pickup. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    TArray<FTimedAttributeSetModifier> OnPickupBuff;
    
    /* === Item & Cleanup === */

    /** Should the pickup actor destroy itself after granting items/effects. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items")
    FText ItemName;
    
    /** Should the pickup actor destroy itself after granting items/effects. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items")
    bool bDestroyAfterGathering;

    /** List of inventory items granted by this pickup actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items")
    TArray<FBaseItem> Items;

    /** Tag applied to the player when collecting this resource (e.g. Action.Collect.Tree). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
    FGameplayTag CollectResourceTag;

    // Getter functions
    bool GetPickOnOverlap() const { return bPickOnOverlap; }
    bool GetAutoEquipOnPick() const { return bAutoEquipOnPick; }
    const TArray<FStatisticValue>& GetOnPickupEffect() const { return OnPickupEffect; }
    const TArray<FTimedAttributeSetModifier>& GetOnPickupBuff() const { return OnPickupBuff; }
    const FText& GetItemName() const { return ItemName; }
    bool GetDestroyAfterGathering() const { return bDestroyAfterGathering; }
    const TArray<FBaseItem>& GetItems() const { return Items; }
    const FGameplayTag& GetCollectResourceTag() const { return CollectResourceTag; }
};

/**
 * Data Asset for configuring pickup actor behavior and loot.
 */
UCLASS()
class NOMADDEV_API UPickupItemActorData : public UDataAsset
{
    GENERATED_BODY()

public:
    /** Inlined pickup settings (shows all sub-properties in editor). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Information", meta = (ShowOnlyInnerProperties))
    FPickupActorInfo PickupActorInfo;

    // Getter function
    const FPickupActorInfo& GetPickupActorInfo() const { return PickupActorInfo; }
};
