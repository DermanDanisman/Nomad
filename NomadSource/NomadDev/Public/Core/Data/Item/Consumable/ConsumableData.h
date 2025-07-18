// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ACMTypes.h"
#include "ARSTypes.h"
#include "Core/Data/Item/BaseItemData.h"
#include "Engine/DataAsset.h"
#include "StatusEffects/ACFBaseStatusEffect.h"
#include "ConsumableData.generated.h"

USTRUCT(BlueprintType)
struct FConsumableItemInfo : public FBaseItemInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Static Mesh")
    TObjectPtr<UStaticMesh> StaticMesh = nullptr;

    // ================================
    // Consumable-Specific Properties
    // ================================

    /**
     * Effect triggered when the consumable is used.
     * This could be a custom effect such as healing, a buff, or any other type of effect that is applied
     * to the character or the game world when the consumable is consumed.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
    FActionEffect OnUsedEffect;

    /**
     * The desired action required to trigger the consumable's use.
     * This could be something like "Use" or "Consume" which defines what action should be associated with the consumable.
     * Typically linked to user input or a button press in the game.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
    FGameplayTag DesiredUseAction;

    /**
     * Whether this consumable is consumed upon use.
     * If set to true, the consumable is used up when activated (like a healing potion).
     * If false, it might be reusable, such as a "key item" or non-consumable (e.g., a reusable buff item).
     */
    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Consumable")
    bool bConsumeOnUse = true;

    /**
     * Stat modifiers applied to the player or character when the consumable is used.
     * For example, it could increase or decrease stats like health, stamina, damage output, etc.
     * These modifications are applied immediately upon consumption.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
    TArray<FStatisticValue> StatModifier;

    /**
     * Timed attribute set modifiers that apply over a period of time when the consumable is used.
     * For example, a consumable might apply a health regeneration effect over a few seconds after being consumed.
     * These modifiers are temporary and last for a set duration.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable | Attributes")
    TArray<FTimedAttributeSetModifier> TimedAttributeSetModifier;

    /**
     * A gameplay effect that is applied when the consumable is used.
     * This could be a healing effect, a damage buff, a status effect like poison or fire, etc.
     * A gameplay effect could modify attributes, apply debuffs, or trigger other game systems.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable | GAS")
    TSubclassOf<UGameplayEffect> ConsumableGameplayEffect;

};

/**
 *
 */
UCLASS(BlueprintType)
class NOMADDEV_API UConsumableData : public UDataAsset
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item Information")
    FConsumableItemInfo ConsumableItemInfo;
};
