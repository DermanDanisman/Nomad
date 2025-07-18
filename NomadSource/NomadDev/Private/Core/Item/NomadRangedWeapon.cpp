// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.


#include "Core/Item/NomadRangedWeapon.h"

#include "Core/Data/Item/Weapon/RangedWeaponData.h"

// Constructor for the ranged weapon base.
// Currently, no specific initialization is done in the constructor.
ANomadRangedWeapon::ANomadRangedWeapon()
{
    
}

// BeginPlay is called when the game starts or the actor is spawned.
void ANomadRangedWeapon::BeginPlay()
{
    Super::BeginPlay();  // Call the base class implementation.
    InitializeItem();    // Initialize weapon properties at runtime.
}

// InitializeItem sets up the ranged weapon's properties using data from the RangedWeaponData asset.
void ANomadRangedWeapon::InitializeItem()
{
    // Ensure that RangedWeaponData is valid; if not, log an error.
     ensureMsgf(RangedWeaponData, TEXT("Ranged Weapon Data IS NOT SET!"));
    
    // Proceed only if RangedWeaponData is valid.
    if (RangedWeaponData)
    {
        // Retrieve the ranged weapon info from the data asset for easy access.
        const FRangedWeaponInfo& Info = RangedWeaponData->RangedWeaponInfo;

        // ---------------------------
        // Mesh Setup
        // ---------------------------
        // Set the skeletal mesh for the weapon if one is provided.
        if (Info.SkeletalMesh)
        {
            Mesh->SetSkeletalMesh(Info.SkeletalMesh);
            if (Info.AnimInstanceClass)
            {
                // Set the animation instance class if one is provided.
                Mesh->SetAnimInstanceClass(Info.AnimInstanceClass);
            }
            else
            {
                // Log a warning if no animation instance class is assigned.
                UE_LOG(LogNomadRangedWeapon, Warning, TEXT("No Anim Instance Class assigned for ranged weapon: %s"), *RangedWeaponData->GetName());
            }
        }
        else
        {
            // Log a warning if the skeletal mesh is not assigned.
            UE_LOG(LogNomadRangedWeapon, Warning, TEXT("No SkeletalMesh assigned for ranged weapon: %s"), *RangedWeaponData->GetName());
        }

        // ---------------------------
        // Ranged-Specific Properties
        // ---------------------------
        if (!ShootingComp)
        {
            // Log an error if the shooting component is null.
            UE_LOG(LogNomadRangedWeapon, Error, TEXT("ShootingComp is null! Check component setup."));
            return;
        }
        
        // Configure the shooting component's visual shooting effect.
        ShootingComp->SetShootingEffect(Info.ShootingEffect);
        // Assign shooting type (e.g., projectile-based, hitscan).
        ShootingType = Info.ShootingType;
        // Set whether the weapon should automatically equip ammo.
        TryEquipAmmos = Info.TryEquipAmmos;
        // Configure whether ammo should be consumed upon firing.
        ShootingComp->SetShouldConsumeAmmo(Info.bConsumeAmmo);
        // Set the ammo slot for the weapon.
        ShootingComp->SetAmmoSlot(Info.AmmoSlot);
        // Define the list of allowed projectile classes (if ammo is consumed).
        ShootingComp->SetAllowedProjectiles(Info.AllowedProjectiles);
        // If ammo is not consumed, assign the specific projectile class to fire.
        ShootingComp->SetProjectileClass(Info.ProjectileClassBP);
        // Set the projectile's shot speed.
        ShootingComp->SetProjectileShotSpeed(Info.ProjectileShotSpeed);
        // Set the shot trace radius (for area-of-effect or spread shots).
        ShootingComp->SetShootRadius(Info.ShootRadius);
        // Define the maximum range for the shot.
        ShootingComp->SetShootRange(Info.ShootRange);

        // ---------------------------
        // Weapon Handling Properties
        // ---------------------------
        // Set the weapon handle type.
        SetHandleType(Info.HandleType);
        // Configure whether to override the main-hand moveset for off-hand use.
        bOverrideMainHandMoveset = Info.bOverrideMainHandMoveset;
        // Configure whether to override the main-hand moveset actions.
        bOverrideMainHandMovesetActions = Info.bOverrideMainHandMovesetActions;
        // Configure whether to override the main-hand overlay.
        bOverrideMainHandOverlay = Info.bOverrideMainHandOverlay;
        // Configure whether to use left-hand IK (for two-handed weapons).
        bUseLeftHandIKPosition = Info.bUseLeftHandIKPosition;

        // ---------------------------
        // Resource Tool Flag
        // ---------------------------
        // Set whether this weapon is a resource tool (e.g., for gathering).
        bResourceTool = Info.bResourceTool;

        // ---------------------------
        // Attachment Information
        // ---------------------------
        // Assign the attachment offset to correctly position the weapon on the character.
        AttachmentOffset = Info.AttachmentOffset;

        // ---------------------------
        // Weapon Type & Tags
        // ---------------------------
        // Set the gameplay tag identifying the weapon type.
        WeaponType = Info.WeaponType;
        if (Info.Moveset.IsValid())
        {
            // Set the moveset tag for animations.
            Moveset = Info.Moveset;
        }
        else
        {
            UE_LOG(LogNomadRangedWeapon, Warning, TEXT("Invalid Moveset for ranged weapon: %s"), *RangedWeaponData->GetName());
        }

        if (Info.MovesetOverlay.IsValid())
        {
            // Set the moveset overlay tag.
            MovesetOverlay = Info.MovesetOverlay;
        }
        else
        {
            UE_LOG(LogNomadRangedWeapon, Warning, TEXT("Invalid Moveset Overlay for ranged weapon: %s"), *RangedWeaponData->GetName());
        }

        if (Info.MovesetActions.IsValid())
        {
            // Set the moveset actions tag.
            MovesetActions = Info.MovesetActions;
        }
        else
        {
            UE_LOG(LogNomadRangedWeapon, Warning, TEXT("Invalid Moveset Actions for ranged weapon: %s"), *RangedWeaponData->GetName());
        }

        // ---------------------------
        // Socket Information
        // ---------------------------
        // Validate and set the on-body socket name where the weapon is attached when not in use.
        if (!Info.OnBodySocketName.IsNone())
        {
            OnBodySocketName = Info.OnBodySocketName;
        }
        else
        {
            UE_LOG(LogNomadRangedWeapon, Warning, TEXT("OnBodySocketName is missing for ranged weapon: %s"), *RangedWeaponData->GetName());
        }

        // Validate and set the in-hands socket name where the weapon is attached when in use.
        if (!Info.InHandsSocketName.IsNone())
        {
            InHandsSocketName = Info.InHandsSocketName;
        }
        else
        {
            UE_LOG(LogNomadRangedWeapon, Warning, TEXT("InHandsSocketName is missing for ranged weapon: %s"), *RangedWeaponData->GetName());
        }

        // ---------------------------
        // Animation Settings
        // ---------------------------
        // Assign the weapon animation map if available.
        if (Info.WeaponAnimations.Num() > 0)
        {
            WeaponAnimations = Info.WeaponAnimations;
        }
        else
        {
            UE_LOG(LogNomadRangedWeapon, Warning, TEXT("WeaponAnimations are missing or empty for ranged weapon: %s"), *RangedWeaponData->GetName());
        }

        // ---------------------------
        // Attribute and Gameplay Effects
        // ---------------------------
        // Set the attribute modifier applied when the weapon is unsheathed.
        UnsheathedAttributeModifier = Info.UnsheathedAttributeModifier;
        // Set the gameplay effect applied when the weapon is unsheathed.
        UnsheatedGameplayEffect = Info.UnsheatedGameplayEffect;

        // Validate and assign the equip sound.
        if (Info.EquipSound)
        {
            EquipSound = Info.EquipSound;
        }
        else
        {
            UE_LOG(LogNomadRangedWeapon, Warning, TEXT("EquipSound is missing for ranged weapon: %s"), *RangedWeaponData->GetName());
        }

        // Validate and assign the unequip sound.
        if (Info.UnequipSound)
        {
            UnequipSound = Info.UnequipSound;
        }
        else
        {
            UE_LOG(LogNomadRangedWeapon, Warning, TEXT("UnequipSound is missing for ranged weapon: %s"), *RangedWeaponData->GetName());
        }
        
        // Assign the gather sound if provided.
        if (Info.GatherSound)
        {
            GatherSound = Info.GatherSound;
        }
        else
        {
            UE_LOG(LogNomadRangedWeapon, Warning, TEXT("No GatherSound assigned for ranged weapon: %s"), *RangedWeaponData->GetName());
        }
        
        // ---------------------------
        // Equipment Attribute Requirements
        // ---------------------------
        // Set the primary attribute requirements for equipping this weapon.
        if (Info.PrimaryAttributesRequirement.Num() > 0)
        {
            PrimaryAttributesRequirement = Info.PrimaryAttributesRequirement;
        }
        else
        {
            UE_LOG(LogNomadRangedWeapon, Warning, TEXT("PrimaryAttributesRequirement is empty for ranged weapon: %s"), *RangedWeaponData->GetName());
        }
        
        // Set the attribute modifier that is applied when the weapon is equipped.
        AttributeModifier = Info.AttributeModifier;
        
        // Check and assign the gameplay modifier applied when the weapon is equipped.
        if (Info.GameplayModifier)
        {
            GameplayModifier = Info.GameplayModifier;
        }
        else
        {
            UE_LOG(LogNomadRangedWeapon, Warning, TEXT("No GameplayModifier assigned for ranged weapon: %s"), *RangedWeaponData->GetName());
        }
        
        // ---------------------------
        // Item Information
        // ---------------------------
        // Set the overall item information (name, description, etc.) from the data asset.
        ItemInfo = Info.ItemInfo;
    }
    else
    {
        // Log an error if the ranged weapon data asset is missing or invalid.
        UE_LOG(LogNomadRangedWeapon, Error, TEXT("RangedWeaponData asset is missing or invalid! -> %s"), *GetName());
    }
}

// Returns the thumbnail image for the ranged weapon (used in UI).
UTexture2D* ANomadRangedWeapon::GetThumbnailImage() const
{
    return RangedWeaponData ? RangedWeaponData->RangedWeaponInfo.ItemInfo.ThumbNail : nullptr;
}

// Returns the item name as defined in the data asset.
FText ANomadRangedWeapon::GetItemName() const
{
    return RangedWeaponData ? RangedWeaponData->RangedWeaponInfo.ItemInfo.Name : FText::GetEmpty();
}

// Returns the item description from the data asset.
FText ANomadRangedWeapon::GetItemDescription() const
{
    return RangedWeaponData ? RangedWeaponData->RangedWeaponInfo.ItemInfo.Description : FText::GetEmpty();
}

// Returns the item type (e.g., ranged weapon).
EItemType ANomadRangedWeapon::GetItemType() const
{
    return RangedWeaponData ? RangedWeaponData->RangedWeaponInfo.ItemInfo.ItemType : EItemType::Default;
}

// Returns the full item descriptor containing all item details.
FItemDescriptor ANomadRangedWeapon::GetItemInfo() const
{
    return RangedWeaponData ? RangedWeaponData->RangedWeaponInfo.ItemInfo : FItemDescriptor();
}

// Returns a list of possible item slots that this weapon can occupy.
TArray<FGameplayTag> ANomadRangedWeapon::GetPossibleItemSlots() const
{
    return RangedWeaponData ? RangedWeaponData->RangedWeaponInfo.ItemInfo.GetPossibleItemSlots() : TArray<FGameplayTag>();
}