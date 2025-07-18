// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Item/NomadMeleeWeapon.h"
#include "ACMCollisionManagerComponent.h"
#include "Core/Data/Item/Weapon/MeleeWeaponData.h"
#include "Sound/SoundCue.h"

// Constructor: Initializes the melee weapon; currently, no additional initialization is performed here.
ANomadMeleeWeapon::ANomadMeleeWeapon()
{
    // No explicit initialization; properties are set in InitializeItem.
}

// BeginPlay: Called when the game starts or the actor is spawned.
void ANomadMeleeWeapon::BeginPlay()
{
    Super::BeginPlay();   // Call the base class BeginPlay.
    InitializeItem();     // Initialize or reinitialize weapon properties at runtime.
}

// InitializeItem: Reads the melee weapon's properties from the MeleeWeaponData asset and applies them to this weapon.
void ANomadMeleeWeapon::InitializeItem()
{
    // Ensure that the MeleeWeaponData asset is assigned. If not, log an error.
    ensureMsgf(MeleeWeaponData, TEXT("Melee Weapon Data IS NOT SET!"));
    
    // Proceed only if the MeleeWeaponData is valid.
    if (MeleeWeaponData)
    {
        // Retrieve the melee weapon information structure for easier access.
        const FMeleeWeaponInfo& Info = MeleeWeaponData->MeleeWeaponInfo;
        
        // ---------------------------
        // Mesh Setup
        // ---------------------------
        // If a skeletal mesh is provided, assign it to the weapon's mesh component.
        if (Info.SkeletalMesh)
        {
            Mesh->SetSkeletalMesh(Info.SkeletalMesh);
            if (Info.AnimInstanceClass)
            {
                // Set the animation instance class if defined.
                Mesh->SetAnimInstanceClass(Info.AnimInstanceClass);
            }
            else
            {
                // Log a warning if no animation instance class is assigned.
                UE_LOG(LogNomadMeleeWeapon, Warning, TEXT("No Anim Instance Class assigned for weapon: %s"), *MeleeWeaponData->GetName());
            }
        }
        else
        {
            // Log a warning if no skeletal mesh is set.
            UE_LOG(LogNomadMeleeWeapon, Warning, TEXT("No SkeletalMesh assigned for weapon: %s"), *MeleeWeaponData->GetName());
        }

        // ---------------------------
        // Collision Setup
        // ---------------------------
        // Configure collision properties from the data asset.
        CollisionComp->SetAllowMultipleHitsPerSwing(Info.bAllowMultipleHitsPerSwing);
        CollisionComp->SetCollisionChannels(Info.CollisionChannels);
        CollisionComp->SetIgnoredActors(Info.IgnoredActors);
        CollisionComp->SetIgnoreOwner(Info.bIgnoreOwner);
        CollisionComp->SetDamageTraces(Info.DamageTraces);
        CollisionComp->SetSwipeTraceInfo(Info.SwipeTraceInfo);
        CollisionComp->SetAreaDamageTraceInfo(Info.AreaDamageTraceInfo);

        // ---------------------------
        // Weapon Handling Setup
        // ---------------------------
        // Set the handle type (e.g., one-handed, two-handed, off-hand).
        SetHandleType(Info.HandleType);
        // Set flags for overriding main-hand moveset, actions, and overlay.
        bOverrideMainHandMoveset = Info.bOverrideMainHandMoveset;
        bOverrideMainHandMovesetActions = Info.bOverrideMainHandMovesetActions;
        bOverrideMainHandOverlay = Info.bOverrideMainHandOverlay;
        // Determine if left-hand IK should be used (for two-handed weapons).
        bUseLeftHandIKPosition = Info.bUseLeftHandIKPosition;
        bResourceTool = Info.bResourceTool;

        // ---------------------------
        // Attachment Setup
        // ---------------------------
        // If the attachment offset is valid, assign it to position the weapon correctly.
        if (Info.AttachmentOffset.IsValid())
        {
            AttachmentOffset = Info.AttachmentOffset;
        }
        else
        {
            // Log a warning if the attachment offset is invalid.
            UE_LOG(LogNomadMeleeWeapon, Warning, TEXT("Invalid AttachmentOffset for weapon: %s"), *MeleeWeaponData->GetName());
        }

        // ---------------------------
        // Weapon Tags and Movesets
        // ---------------------------
        // Set the gameplay tag identifying the weapon type.
        WeaponType = Info.WeaponType;
        if (Info.Moveset.IsValid())
        {
            // Assign the moveset tag for the weapon.
            Moveset = Info.Moveset;
        }
        else
        {
            // Log a warning if the moveset tag is invalid.
            UE_LOG(LogNomadMeleeWeapon, Warning, TEXT("Invalid Moveset for weapon: %s"), *MeleeWeaponData->GetName());
        }

        if (Info.MovesetOverlay.IsValid())
        {
            // Assign the moveset overlay tag if available.
            MovesetOverlay = Info.MovesetOverlay;
        }
        else
        {
            // Log a warning if the moveset overlay tag is invalid.
            UE_LOG(LogNomadMeleeWeapon, Warning, TEXT("Invalid Moveset Overlay for weapon: %s"), *MeleeWeaponData->GetName());
        }

        if (Info.MovesetActions.IsValid())
        {
            // Assign the moveset actions tag for available weapon actions.
            MovesetActions = Info.MovesetActions;
        }
        else
        {
            // Log a warning if the moveset actions tag is invalid.
            UE_LOG(LogNomadMeleeWeapon, Warning, TEXT("Invalid Moveset Actions Overlay for weapon: %s"), *MeleeWeaponData->GetName());
        }
        
        // ---------------------------
        // Socket Names
        // ---------------------------
        // Validate and assign the on-body socket name (where the weapon is attached when not in use).
        if (!Info.OnBodySocketName.IsNone())
        {
            OnBodySocketName = Info.OnBodySocketName;
        }
        else
        {
            // Log a warning if the on-body socket name is missing.
            UE_LOG(LogNomadMeleeWeapon, Warning, TEXT("OnBodySocketName is missing for weapon: %s"), *MeleeWeaponData->GetName());
        }

        // Validate and assign the in-hands socket name (where the weapon is held when in use).
        if (!Info.OnBodySocketName.IsNone())
        {
            InHandsSocketName = Info.InHandsSocketName;
        }
        else
        {
            // Log a warning if the in-hands socket name is missing.
            UE_LOG(LogNomadMeleeWeapon, Warning, TEXT("InHandsSocketName is missing for weapon: %s"), *MeleeWeaponData->GetName());
        }

        // ---------------------------
        // Animation & Visual Effects
        // ---------------------------
        // Assign the map of weapon animations keyed by gameplay tags.
        if (Info.WeaponAnimations.Num() > 0)
        {
            WeaponAnimations = Info.WeaponAnimations;
        }
        else
        {
            // Log a warning if no weapon animations are provided.
            UE_LOG(LogNomadMeleeWeapon, Warning, TEXT("WeaponAnimations are missing or empty for weapon: %s"), *MeleeWeaponData->GetName());
        }

        // ---------------------------
        // Attribute Modifiers & Gameplay Effects
        // ---------------------------
        // Assign the attribute modifier that applies when the weapon is unsheathed.
        UnsheathedAttributeModifier = Info.UnsheathedAttributeModifier;
        // Assign the gameplay effect applied when the weapon is unsheathed.
        UnsheatedGameplayEffect = Info.UnsheatedGameplayEffect;

        // ---------------------------
        // Sound Cues
        // ---------------------------
        // Validate and assign the equip sound for the weapon.
        if (Info.EquipSound)
        {
            EquipSound = Info.EquipSound;
        }
        else
        {
            UE_LOG(LogNomadMeleeWeapon, Warning, TEXT("EquipSound is missing for weapon: %s"), *MeleeWeaponData->GetName());
        }

        // Validate and assign the unequip sound for the weapon.
        if (Info.UnequipSound)
        {
            UnequipSound = Info.UnequipSound;
        }
        else
        {
            UE_LOG(LogNomadMeleeWeapon, Warning, TEXT("UnequipSound is missing for weapon: %s"), *MeleeWeaponData->GetName());
        }

        // ---------------------------
        // Additional Sounds
        // ---------------------------
        // Assign the gather sound if specified in the data asset.
        if (Info.GatherSound)
        {
            GatherSound = Info.GatherSound;
        }
        else
        {
            UE_LOG(LogNomadMeleeWeapon, Warning, TEXT("No GatherSound assigned for weapon: %s"), *MeleeWeaponData->GetName());
        }

        // ---------------------------
        // Attribute Requirements
        // ---------------------------
        // Assign the list of attribute requirements needed to equip the weapon.
        if (Info.PrimaryAttributesRequirement.Num() > 0)
        {
            PrimaryAttributesRequirement = Info.PrimaryAttributesRequirement;
        }
        else
        {
            UE_LOG(LogNomadMeleeWeapon, Warning, TEXT("PrimaryAttributesRequirement is empty for weapon: %s"), *MeleeWeaponData->GetName());
        }

        // ---------------------------
        // Equipment Effects
        // ---------------------------
        // Set the attribute modifier that is applied when the weapon is equipped.
        AttributeModifier = Info.AttributeModifier;

        // Validate and assign the gameplay modifier applied when the weapon is equipped.
        if (Info.GameplayModifier)
        {
            GameplayModifier = Info.GameplayModifier;
        }
        else
        {
            UE_LOG(LogNomadMeleeWeapon, Warning, TEXT("No GameplayModifier assigned for weapon: %s"), *MeleeWeaponData->GetName());
        }

        // ---------------------------
        // Item Information
        // ---------------------------
        // Set the complete item descriptor containing name, description, and other shared properties.
        ItemInfo = Info.ItemInfo;
    }
    else
    {
        // Log an error if the MeleeWeaponData asset is missing or invalid.
        UE_LOG(LogNomadMeleeWeapon, Error, TEXT("MeleeWeaponData asset is missing or invalid! -> %s"), *GetName());
    }
}

UTexture2D* ANomadMeleeWeapon::GetThumbnailImage() const
{
    // Return the thumbnail image from the item info, checking if MeleeWeaponData is valid.
    return MeleeWeaponData ? MeleeWeaponData->MeleeWeaponInfo.ItemInfo.ThumbNail : nullptr;
}

FText ANomadMeleeWeapon::GetItemName() const
{
    // Return the item name from the item info.
    return MeleeWeaponData ? MeleeWeaponData->MeleeWeaponInfo.ItemInfo.Name : FText::GetEmpty();
}

FText ANomadMeleeWeapon::GetItemDescription() const
{
    // Return the item description from the item info.
    return MeleeWeaponData ? MeleeWeaponData->MeleeWeaponInfo.ItemInfo.Description : FText::GetEmpty();
}

EItemType ANomadMeleeWeapon::GetItemType() const
{
    // Return the item type (e.g., melee weapon) from the item info.
    return MeleeWeaponData ? MeleeWeaponData->MeleeWeaponInfo.ItemInfo.ItemType : EItemType::Default;
}

FItemDescriptor ANomadMeleeWeapon::GetItemInfo() const
{
    // Return the complete item descriptor from the item info.
    return MeleeWeaponData ? MeleeWeaponData->MeleeWeaponInfo.ItemInfo : FItemDescriptor();
}

TArray<FGameplayTag> ANomadMeleeWeapon::GetPossibleItemSlots() const
{
    // Return the list of possible equipment slot tags from the item info, checking if MeleeWeaponData is valid.
    return MeleeWeaponData ? MeleeWeaponData->MeleeWeaponInfo.ItemInfo.GetPossibleItemSlots() : TArray<FGameplayTag>();
}

TArray<FGameplayTag> ANomadMeleeWeapon::GetRequiredToolTag() const
{
    // Return the list of required tool tags from the item, checking if MeleeWeaponData is valid.
    return MeleeWeaponData ? MeleeWeaponData->MeleeWeaponInfo.RequiredToolTag : TArray<FGameplayTag>();
}