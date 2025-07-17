// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ACMTypes.h"
#include "ARSTypes.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "Core/Data/Item/Equipable/EquipableItemData.h"
#include "Engine/DataAsset.h"
#include "BaseWeaponData.generated.h"

// -----------------------------------------------------------------------------
// FBaseWeaponInfo
// -----------------------------------------------------------------------------

// This USTRUCT extends FEquipableItemInfo (shared properties among equippable items)
// and adds additional weapon-specific properties.
USTRUCT(BlueprintType)
struct FBaseWeaponInfo : public FEquipableItemInfo
{
    GENERATED_BODY()

    FBaseWeaponInfo()
        : bAllowMultipleHitsPerSwing(false)
        , CollisionChannels()
        , IgnoredActors()
        , bIgnoreOwner(true)
        , DamageTraces()
        , SwipeTraceInfo()
        , AreaDamageTraceInfo()
        , HandleType(EHandleType::OneHanded)
        , bOverrideMainHandMoveset(false)
        , bOverrideMainHandMovesetActions(false)
        , bOverrideMainHandOverlay(false)
        , bUseLeftHandIKPosition(false)
        , bResourceTool(false)
        , RequiredToolTag()
        , AttachmentOffset(FTransform::Identity)
        , WeaponType()
        , Moveset()
        , MovesetOverlay()
        , MovesetActions()
        , OnBodySocketName(NAME_None)
        , InHandsSocketName(NAME_None)
        , WeaponAnimations()
        , UnsheathedAttributeModifier()
        , UnsheatedGameplayEffect(nullptr)
    {}

    // ---------------------------
    // Collision Properties
    // ---------------------------

    /** 
     * Allows the weapon to register multiple hits in a single swing.
     * When true, the weapon can damage multiple targets or hit the same target more than once per swing.
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Collision Properties")
    bool bAllowMultipleHitsPerSwing;

    /** 
     * Specifies which collision channels the weapon interacts with.
     * Determines what types of objects the weapon can collide with during attacks.
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Collision Properties")
    TArray<TEnumAsByte<ECollisionChannel>> CollisionChannels;

    /** 
     * A list of actors that should be ignored by the weapon's collision detection.
     * Prevents the weapon from hitting certain actors (e.g., the wielder or friendly NPCs).
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Collision Properties")
    TArray<class AActor*> IgnoredActors;

    /** 
     * Indicates whether the weapon should ignore collisions with its owner's components.
     * Useful to prevent self-damage or unintended interactions.
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Collision Properties")
    bool bIgnoreOwner;

    // ---------------------------
    // Trace Properties for Damage Detection
    // ---------------------------

    /** 
     * A mapping of trace names to their corresponding trace configuration.
     * Each trace (e.g., for a sword swing) defines the area and parameters used to detect hits.
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Collision Properties | Traces")
    TMap<FName, FTraceInfo> DamageTraces;

    /** 
     * Trace information used for swipe attacks.
     * Defines how and where the weapon should check for collisions during a swing attack.
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Collision Properties | Traces")
    FBaseTraceInfo SwipeTraceInfo;

    /** 
     * Trace information used for area-of-effect damage.
     * Configures the detection area for attacks that affect multiple targets at once.
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Collision Properties | Traces")
    FBaseTraceInfo AreaDamageTraceInfo;

    // ================================
    // Weapon Handling (One-Handed, Two-Handed, Off-Hand)
    // ================================

    /** 
     * The type of weapon handle. Defines whether the weapon is one-handed, off-hand (for dual-wielding), or two-handed.
     * - OneHanded: The weapon is meant to be used with one hand.
     * - OffHand: The weapon is meant for use in the off-hand, typically in dual-wielding setups.
     * - TwoHanded: The weapon requires both hands to be used effectively (e.g., large weapons like a greatsword).
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weapon Handling")
    EHandleType HandleType;

    /** 
     * Determines whether the main-hand moveset should be overridden when the weapon is used as an off-hand weapon.
     * This is only relevant when HandleType is set to OffHand.
     * When true, it allows customization of the moveset for the off-hand weapon in a dual-wielding setup.
     */
    UPROPERTY(BlueprintReadOnly, meta = (EditCondition = "HandleType == EHandleType::OffHand"), EditDefaultsOnly, Category = "Weapon Handling")
    bool bOverrideMainHandMoveset;

    /** 
     * Determines whether the main-hand actions should be overridden when the weapon is used as an off-hand weapon.
     * This is only relevant when HandleType is set to OffHand.
     * When true, it allows customization of the actions (attacks, abilities) for the off-hand weapon in a dual-wielding setup.
     */
    UPROPERTY(BlueprintReadOnly, meta = (EditCondition = "HandleType == EHandleType::OffHand"), EditDefaultsOnly, Category = "Weapon Handling")
    bool bOverrideMainHandMovesetActions;

    /** 
     * Determines whether the main-hand overlay (visual effects, animations) should be overridden for off-hand weapons in a dual-wielding setup.
     * This is only relevant when HandleType is set to OffHand.
     * When true, this property allows the customization of the main-hand overlay when the weapon is used in the off-hand.
     */
    UPROPERTY(BlueprintReadOnly, meta = (EditCondition = "HandleType == EHandleType::OffHand"), EditDefaultsOnly, Category = "Weapon Handling")
    bool bOverrideMainHandOverlay;

    /** 
     * Determines whether the left-hand IK (Inverse Kinematics) position should be used for two-handed weapons.
     * This is only relevant when HandleType is set to TwoHanded.
     * When true, it allows for proper positioning and alignment of the left hand for two-handed weapon usage, ensuring realistic hand positioning.
     */
    UPROPERTY(BlueprintReadOnly, meta = (EditCondition = "HandleType == EHandleType::TwoHanded"), EditDefaultsOnly, Category = "Weapon Handling")
    bool bUseLeftHandIKPosition;

    // ---------------------------
    // Weapon Type and Attachment Information
    // ---------------------------

    /** 
     * Indicates whether the weapon is also considered a resource tool.
     * For example, a pickaxe or gathering tool may use similar properties as a weapon but is used for resource collection.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weapon Type & Attachment", meta = (ExposeOnSpawn = "true"))
    bool bResourceTool;

    /** 
     * Tool tag required to gather (e.g. Tool.Axe). Leave None to allow bare-handed gather.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditCondition = "bResourceTool"), Category = "Weapon Type & Attachment", meta = (ExposeOnSpawn = "true"))
    TArray<FGameplayTag> RequiredToolTag;

    /** 
     * Defines the relative transform (position, rotation, scale) used to attach the weapon to a character.
     * This ensures the weapon appears correctly on the character's body or hands.
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Weapon Type & Attachment")
    FTransform AttachmentOffset;

    /** 
     * A gameplay tag that categorizes the type of weapon (e.g., sword, axe, bow).
     * Useful for applying type-specific behaviors, stats, or animations.
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Weapon Type & Attachment")
    FGameplayTag WeaponType;

    /** 
     * Specifies the moveset to be used when the weapon is equipped.
     * Determines the base set of animations and actions available with this weapon.
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Weapon Type & Attachment")
    FGameplayTag Moveset;

    /** 
     * An additional overlay tag to modify or augment the base moveset.
     * Could be used to apply special visual effects or animations when the weapon is in use.
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Weapon Type & Attachment")
    FGameplayTag MovesetOverlay;

    /** 
     * Defines the set of actions (attacks, abilities) available when the weapon is equipped.
     * This property helps determine the interactive behavior of the weapon during combat.
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Weapon Type & Attachment")
    FGameplayTag MovesetActions;

    /** 
     * The socket name on the character's body where the weapon is attached when not in use (e.g., on the back).
     * This is typically set to a socket defined in the character's skeleton.
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Weapon Type & Attachment")
    FName OnBodySocketName;

    /** 
     * The socket name where the weapon is attached when it is actively in use (e.g., in hand).
     * Ensures the weapon is correctly positioned when drawn.
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Weapon Type & Attachment")
    FName InHandsSocketName;

    // ---------------------------
    // Weapon Animations and Effects
    // ---------------------------
    
    /** 
     * A mapping of gameplay tags to animation montages.
     * Allows different animations (attack, idle, etc.) to be triggered based on the gameplay context.
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Weapon Animations & Effects")
    TMap<FGameplayTag, UAnimMontage*> WeaponAnimations;

    /** 
     * Modifier that is applied to character attributes when the weapon is unsheathed.
     * Can alter stats such as attack power, defense, or stamina, influencing combat performance.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Animations & Effects | Attributes")
    FAttributesSetModifier UnsheathedAttributeModifier;

    /** 
     * Gameplay effect that is applied when the weapon is unsheathed.
     * Could include buffs or debuffs that modify gameplay (e.g., increased damage or temporary defense boosts).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Animations & Effects | GAS")
    TSubclassOf<UGameplayEffect> UnsheatedGameplayEffect;
};

// -----------------------------------------------------------------------------
// UBaseWeaponData
// -----------------------------------------------------------------------------

/**
 * UBaseWeaponData
 *
 * Data asset class that holds the base configuration for weapons.
 * It uses FBaseWeaponInfo to define shared properties such as collision, attachment,
 * animations, and gameplay effects that are common across various weapon types.
 */
UCLASS(BlueprintType)
class NOMADDEV_API UBaseWeaponData : public UDataAsset
{
    GENERATED_BODY()

public:
    // BaseWeaponInfo holds all the shared weapon properties.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item Information")
    FBaseWeaponInfo BaseWeaponInfo;
};