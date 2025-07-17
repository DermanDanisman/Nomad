// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ARSTypes.h"
#include "Core/Data/Item/BaseItemData.h"
#include "Engine/DataAsset.h"
#include "EquipableItemData.generated.h"

USTRUCT(BlueprintType)
struct FEquipableItemInfo : public FBaseItemInfo
{
    GENERATED_BODY()

    /** Default constructor to initialize all members with safe defaults. */
    FEquipableItemInfo()
        : SkeletalMesh(nullptr)
        , AnimInstanceClass(nullptr)
        , EquipSound(nullptr)
        , UnequipSound(nullptr)
        , PrimaryAttributesRequirement()
        , AttributeModifier()
        , GameplayModifier(nullptr)
    {}

    // ================================
    // Skeletal Mesh and Weapon Type Properties
    // ================================

    /** 
     * The skeletal mesh used by the armor.
     * This mesh represents the 3D model of the armor that will be displayed in the game.
     * This property is typically set to the mesh of the armor.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skeletal Mesh")
    TObjectPtr<USkeletalMesh> SkeletalMesh;

    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Weapon Animations & Effects")
    TSubclassOf<UAnimInstance> AnimInstanceClass;

    // ================================
    // Equippable Item Properties
    // ================================

    /** 
     * Sound effect played when the item is equipped (e.g., weapon, armor, clothing, or accessories).
     * This is typically used for UI feedback or character animation when an item is equipped.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Equippable | Sounds")
    TObjectPtr<USoundCue> EquipSound;

    /** 
     * Sound effect played when the item is unequipped (e.g., weapon, armor, clothing, or accessories).
     * Similar to EquipSound, this is used for UI feedback or character animation when an item is removed.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Equippable | Sounds")
    TObjectPtr<USoundCue> UnequipSound;

    /** 
     * List of primary attribute requirements to equip this item (e.g., strength, agility).
     * This ensures that a character meets specific criteria before they can equip the item.
     * For example, an armor might require a minimum strength attribute to wear.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equippable | Attributes")
    TArray<FAttribute> PrimaryAttributesRequirement;

    /** 
     * Modifier applied to a character's attributes once the item is equipped.
     * For example, an item might increase or decrease certain stats like health, stamina, or defense.
     * This modifier could affect things like strength, agility, or other player attributes.
     */
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "Equippable | Attributes")
    FAttributesSetModifier AttributeModifier;

    /** 
     * Gameplay effect that is applied once the item is equipped (e.g., healing, buffs, debuffs).
     * This could be an effect like increasing a character's damage output or granting them immunity to certain debuffs.
     * For example, an armor piece might grant a defensive buff or an accessory could provide a healing effect over time.
     */
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "Equippable | GAS")
    TSubclassOf<UGameplayEffect> GameplayModifier;
};

/**
 * 
 */
UCLASS(BlueprintType)
class NOMADDEV_API UEquipableItemData : public UDataAsset
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item Information")
    FEquipableItemInfo EquipableItemInfo;
	
};
