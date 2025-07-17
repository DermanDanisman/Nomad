// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ACMTypes.h"
#include "Core/Data/Item/BaseItemData.h"
#include "Core/Data/Item/Equipable/EquipableItemData.h"
#include "Engine/DataAsset.h"
#include "ProjectileData.generated.h"

USTRUCT(BlueprintType)
struct FProjectileInfo : public FEquipableItemInfo
{
    GENERATED_BODY()

    /** Default constructor initializing all members to safe defaults. */
    FProjectileInfo()
        : FEquipableItemInfo()  // base initializer
        , bAllowMultipleHitsPerSwing(false)
        , CollisionChannels()
        , IgnoredActors()
        , bIgnoreOwner(true)
        , DamageTraces()
        , SwipeTraceInfo()
        , AreaDamageTraceInfo()
        , ProjectileInitialSpeed(4000.f)
        , ProjectileMaxSpeed(5000.f)
        , bRotationFollowsVelocity(true)
        , bRotationRemainsVertical(false)
        , bInitialVelocityInLocalSpace(true)
        , ProjectileGravityScale(1.f)
        , ProjectileLifespan(5.f)
        , HitPolicy(EProjectileHitPolicy::AttachOnHit)
        , AttachedLifespan(10.f)
        , bDroppableWhenAttached(true)
        , DropRatePercentage(100.f)
        , ImpactEffect()
    {}
    
    // ================================
    // Projectile and Collision Properties
    // ================================

    /** 
     * Determines whether the weapon or projectile allows multiple hits per swing.
     * If true, the projectile or weapon can hit the same target multiple times in a single swing or shot.
     * This is typically used for weapons like swords or projectiles that have a wide hit area.
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Collision Properties")
    bool bAllowMultipleHitsPerSwing;

    /** 
     * List of collision channels that the weapon or projectile interacts with.
     * This defines which types of objects or surfaces the weapon or projectile can collide with. 
     * For example, this could include things like "WorldStatic", "Pawn", "Weapon", etc.
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Collision Properties")
    TArray<TEnumAsByte<ECollisionChannel>> CollisionChannels;

    /** 
     * List of actors that should be ignored by the weapon or projectile.
     * This is useful for cases where you don't want the projectile or weapon to hit certain actors, like allies or the character's owner.
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Collision Properties")
    TArray<class AActor*> IgnoredActors;

    /** 
     * Determines if the weapon or projectile should ignore the component's owner.
     * When true, the weapon or projectile will not affect the actor that owns it (e.g., a weapon that won't hit the player using it).
     * This is useful for preventing friendly fire or self-hitting.
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Collision Properties")
    bool bIgnoreOwner;

    /** 
     * A map of damage traces associated with the weapon or projectile. 
     * Each trace is identified by a name and contains the information about the trace (e.g., start point, end point, damage).
     * This is typically used for complex weapons that require multiple traces for detecting damage (e.g., sword swings).
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Collision Properties | Traces")
    TMap<FName, FTraceInfo> DamageTraces;

    /** 
     * Trace information used for swipe attacks (e.g., area damage from a sword swing).
     * This trace defines the area in which damage can be applied to targets, allowing for things like sweeping or slashing attacks.
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Collision Properties | Traces")
    FBaseTraceInfo SwipeTraceInfo;

    /** 
     * Trace information used for area damage (e.g., explosions or blast effects).
     * Defines the area of effect for applying damage to multiple targets within the blast radius.
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Collision Properties | Traces")
    FBaseTraceInfo AreaDamageTraceInfo;

    // ================================
    // Projectile-Specific Properties
    // ================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
    float ProjectileInitialSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
    float ProjectileMaxSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
    bool bRotationFollowsVelocity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
    bool bRotationRemainsVertical;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
    bool bInitialVelocityInLocalSpace;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
    float ProjectileGravityScale;
    
    /** 
     * The lifespan of the projectile in seconds.
     * This determines how long the projectile exists in the game world before it is destroyed automatically.
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Projectile")
    float ProjectileLifespan;

    /** 
     * Defines the policy for what happens when the projectile hits an actor or surface.
     * - AttachOnHit: The projectile attaches to the actor it hits.
     * - DestroyOnHit: The projectile is destroyed when it hits something.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Projectile")
    EProjectileHitPolicy HitPolicy;

    /** 
     * If the projectile is set to attach to an actor upon impact, this defines the lifespan of the attached projectile.
     * This is used when the projectile sticks to an actor (e.g., a dart that sticks to a character).
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (EditCondition = "HitPolicy == EProjectileHitPolicy::AttachOnHit"), Category = "Projectile")
    float AttachedLifespan;

    /** 
     * Whether the projectile can be dropped as a world item if it is attached to a character and that character dies.
     * When true, the projectile becomes a dropable item in the world once the attached character dies.
     */
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "HitPolicy == EProjectileHitPolicy::AttachOnHit"), Category = "Projectile")
    bool bDroppableWhenAttached;

    /** 
     * When the projectile is attached to an adversary, this defines the chance that the projectile will be dropped when the adversary dies.
     * This drop rate is represented as a percentage, where 100% means the projectile is guaranteed to drop.
     */
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "HitPolicy == EProjectileHitPolicy::AttachOnHit"), Category = "Projectile")
    float DropRatePercentage;

    /** 
     * Defines the impact effect (e.g., particle effects, sounds) that occurs when the projectile hits an actor or surface.
     * This can be used for visual effects like explosions, sparks, or smoke when the projectile impacts its target.
     */
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "HitPolicy == EProjectileHitPolicy::DestroyOnHit"), Category = "Projectile")
    FImpactFX ImpactEffect;
};

/**
 * 
 */
UCLASS(BlueprintType)
class NOMADDEV_API UProjectileData : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item Information")
    FProjectileInfo ProjectileInfo;
	
};
