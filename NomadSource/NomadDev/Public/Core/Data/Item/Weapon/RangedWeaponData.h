// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ACFItemTypes.h"
#include "ACMTypes.h"
#include "BaseWeaponData.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "RangedWeaponData.generated.h"

USTRUCT(BlueprintType)
struct FRangedWeaponInfo : public FBaseWeaponInfo
{
    GENERATED_BODY()

    FRangedWeaponInfo()
        : ProjectileStartSocket(TEXT("ProjectileStart"))
        , ShootingEffect()
        , ShootingType(EShootingType::EProjectile)
        , TryEquipAmmos(true)
        , bConsumeAmmo(true)
        , AmmoSlot()
        , AllowedProjectiles()
        , ProjectileClassBP(nullptr)
        , ProjectileShotSpeed(1000.f)
        , ShootRadius(1.f)
        , ShootRange(3500.f)
    {}

    // ================================
    // Ranged Weapon Specific Properties
    // ================================

    /**
     * Socket name where the projectile originates when the weapon is fired.
     * This is typically used to determine the starting point for the projectile's trajectory.
     */
    UPROPERTY(EditAnywhere, Category = "Weapon | Ranged | General")
    FName ProjectileStartSocket;

    /**
     * The visual and/or audio effect played when the weapon is fired.
     * This effect may include particles, sounds, or other feedback to indicate that the weapon is shooting.
     */
    UPROPERTY(EditAnywhere, Category = "Weapon | Ranged | General")
    FImpactFX ShootingEffect;

    /**
     * Type of shooting mechanism for the ranged weapon (e.g., projectile, hitscan, etc.).
     * Default is EProjectile, indicating a projectile-based weapon.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weapon | Ranged | General")
    EShootingType ShootingType;

    /**
     * Whether the weapon will attempt to equip ammo (typically for weapons that require ammunition to function).
     * Set to true by default, indicating that the weapon will manage its ammo automatically.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weapon | Ranged | Ammo")
    bool TryEquipAmmos;

    /**
     * Whether this weapon consumes ammo when used.
     * Set to true by default, indicating that ammo is consumed upon firing.
     */
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bConsumeAmmo"), Category = "Weapon | Ranged | Ammo")
    bool bConsumeAmmo;

    /**
     * The slot where the ammo will be equipped. This is only relevant if ammo consumption is enabled (bConsumeAmmo == true).
     * It identifies the category of ammo required by this weapon.
     */
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bConsumeAmmo"), Category = "Weapon | Ranged | Ammo")
    FGameplayTag AmmoSlot;

    /**
     * The types of projectiles this ranged weapon is allowed to fire.
     * This array holds classes of projectiles that the weapon can fire, ensuring compatibility with specific ammo types.
     */
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bConsumeAmmo"), Category = "Weapon | Ranged | Ammo")
    TArray<TSubclassOf<class AACFProjectile>> AllowedProjectiles;

    /**
     * If the weapon doesn't consume ammo but instead fires a specific projectile, this class reference is used.
     * This is only relevant if bConsumeAmmo is set to false.
     */
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bConsumeAmmo == false"), Category = "Weapon | Ranged | Ammo")
    TSubclassOf<AACFProjectile> ProjectileClassBP;

    /**
     * The speed at which the projectile is shot from the weapon.
     * This value determines how fast the projectile moves through the air after being fired.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weapon | Ranged | Properties")
    float ProjectileShotSpeed;

    /**
     * The radius of the shooting trace (if using a trace or area effect for the shot).
     * A value of 0 would indicate a line trace, while a non-zero value suggests an area-based hit detection.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weapon | Ranged | Properties")
    float ShootRadius;

    /**
     * The maximum distance at which the weapon can shoot.
     * This defines how far the projectile or trace effect will travel before being considered ineffective.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weapon | Ranged | Properties")
    float ShootRange;
};

/**
 *
 */
UCLASS(BlueprintType)
class NOMADDEV_API URangedWeaponData : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item Information")
    FRangedWeaponInfo RangedWeaponInfo;
};
