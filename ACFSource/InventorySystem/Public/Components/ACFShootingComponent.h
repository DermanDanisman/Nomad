// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFEquipmentComponent.h"
#include "ACFItemTypes.h"
#include "ACMTypes.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Items/ACFWeapon.h"
#include <Components/MeshComponent.h>

#include "ACFShootingComponent.generated.h"

struct FImpactFX;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCurrentAmmoChanged, int32, currentAmmoInMagazine, int32, totalAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShoot);


UCLASS(Blueprintable, ClassGroup = (ACF), meta = (BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UACFShootingComponent : public UActorComponent {
    GENERATED_BODY()

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Category = "ACF")
    FName ProjectileStartSocket = "ProjectileStart";

    UPROPERTY(EditAnywhere, Category = "ACF")
    struct FImpactFX ShootingEffect;

    UPROPERTY(EditDefaultsOnly, Category = "ACF | Ammo")
    bool bConsumeAmmo = true;

    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bConsumeAmmo"), Category = "ACF | Ammo")
    FGameplayTag AmmoSlot;

    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bConsumeAmmo"), Category = "ACF | Ammo")
    TArray<TSubclassOf<class AACFProjectile>> AllowedProjectiles;

    /*If this is set to true means that this weapon will need to be Reload every time his ammo in
    magazine are 0 to continue shooting. */
    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bConsumeAmmo"), Category = "ACF | Ammo")
    bool bUseMagazine = false;

    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bUseMagazine"), Category = "ACF | Ammo")
    int32 AmmoMagazine = 10;

    UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bConsumeAmmo == false"), Category = "ACF | Ammo")
    TSubclassOf<class AACFProjectile> ProjectileClassBP;

    /*Speed at wich the projectile is shot*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ACF | Projectile Shoot Config")
    float ProjectileShotSpeed;

    /*Radius of the shooting trace. 0 means linetrace*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ACF | SwipeTrace Shoot Config")
    float ShootRadius = 1.f;

    /*Distance at which the trace is done*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ACF | SwipeTrace Shoot Config")
    float ShootRange = 3500.f;

public:
    // Sets default values for this component's properties
    UACFShootingComponent();

    UFUNCTION(BlueprintCallable, Category = ACF)
    void RemoveAmmo();

    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = ACF)
    void SetupShootingComponent(class APawn* inOwner, class UMeshComponent* inMesh);

    UFUNCTION(BlueprintCallable, Category = ACF)
    void ReinitializeShootingComponent(class APawn* inOwner, class UMeshComponent* inMesh, FName inStartSocket, const FImpactFX& inShootingFX)
    {
        shootingMesh = inMesh;
        characterOwner = inOwner;
        ProjectileStartSocket = inStartSocket;
        ShootingEffect = inShootingFX;
    }

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE float GetProjectileSpeed() const { return ProjectileShotSpeed; }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void ShootAtActor(const AActor* target, float randomDeviation = 5.f, float charge = 1.f, TSubclassOf<class AACFProjectile> projectileOverride = nullptr, const FName socketOverride = NAME_None);

    UFUNCTION(BlueprintCallable, Category = ACF)
    void ShootAtDirection(const FRotator& direction, float charge = 1.f, TSubclassOf<class AACFProjectile> projectileOverride = nullptr, const FName socketOverride = NAME_None);

    UFUNCTION(BlueprintCallable, Category = ACF)
    void Shoot(APawn* SourcePawn, EShootingType type, EShootTargetType targetType, float charge = 1.f, TSubclassOf<class AACFProjectile> projectileOverride = nullptr);

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SwipeTraceShootAtDirection(const FVector& start, const FVector& direction, float shootDelay = 0.1f);

    UFUNCTION(BlueprintCallable, Server, Reliable, Category = ACF)
    void ReduceAmmoMagazine(int32 amount = 1);

    /*Fills the magazine with ammos. If TryToEquipAmmo is set to true
    it will try to reload also using ammos not in the equipment, taking them from
    the inventory*/
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = ACF)
    void Reload(bool bTryToEquipAmmo = true);

    UFUNCTION(BlueprintCallable, Category = ACF)
    bool TryEquipAmmoFromInventory();

    UFUNCTION(BlueprintPure, Category = ACF)
    bool CanShoot() const;

    UFUNCTION(BlueprintPure, Category = ACF)
    bool CanUseProjectile(const TSubclassOf<AACFProjectile>& projectileClass) const;

    UFUNCTION(BlueprintPure, Category = ACF)
    bool NeedsReload() const;

    UFUNCTION(BlueprintPure, Category = ACF)
    bool IsFullMagazine() const
    {
        return currentMagazine == AmmoMagazine;
    }

    UFUNCTION(BlueprintPure, Category = ACF)
    bool TryGetAllowedAmmoFromInventory(FInventoryItem& outAmmoSlot) const;

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE FVector GetShootingSocketPosition() const
    {
        return shootingMesh->GetSocketLocation(ProjectileStartSocket);
    }

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE FName GetProjectileStartSocketName() const
    {
        return ProjectileStartSocket;
    }
    
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE UMeshComponent* GetShootingMesh() const
    {
        return shootingMesh;
    }

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE TArray<TSubclassOf<AACFProjectile>> GetAllowedProjectiles() const
    {
        return AllowedProjectiles;
    }
    
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE int32 GetAmmoMagazine() const
    {
        return AmmoMagazine;
    }

    UFUNCTION(BlueprintPure, Category = ACF)
    int32 GetTotalEquippedAmmoCount() const;

    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE int32 GetCurrentAmmoInMagazine() const
    {
        return currentMagazine;
    }

    UFUNCTION(BlueprintPure, Category = ACF)
    int32 GetTotalAmmoCount() const;

    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnCurrentAmmoChanged OnCurrentAmmoChanged;

    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnShoot OnProjectileShoot;

    UFUNCTION(NetMulticast, Reliable, Category = ACF)
    void PlayMuzzleEffect();

    UFUNCTION(BlueprintPure, Category = ACF)
    bool GetUseMagazine() const { return bUseMagazine; }

    UFUNCTION(BlueprintPure, Category = ACF)
    bool TryGetAmmoSlot(FEquippedItem& outSlot) const;


    /*
     * Setters
     */
    
    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetProjectileStartSocketName(FName newSocket)
    {
        ProjectileStartSocket = newSocket;
    }
    
    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetShootingEffect(const FImpactFX& InShootingFX)
    {
        ShootingEffect = InShootingFX;
    }
    
    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetAllowedProjectiles(const TArray<TSubclassOf<class AACFProjectile>> InAllowedProjectiles)
    {
        AllowedProjectiles = InAllowedProjectiles;
    }
    
    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetAmmoSlot(const FGameplayTag& InAmmoSlot)
    {
        AmmoSlot = InAmmoSlot;
    }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetUseMagazine(bool val)
    {
        bUseMagazine = val;
    }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetProjectileClass(const TSubclassOf<class AACFProjectile> InProjectileClassBP)
    {
        ProjectileClassBP = InProjectileClassBP;
    }
    
    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetProjectileShotSpeed(const float InProjectileSpeed)
    {
        ProjectileShotSpeed = InProjectileSpeed;
    }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetShootRadius(const float InShootRadius)
    {
        ShootRadius = InShootRadius;
    }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetShootRange(const float InShootRange)
    {
        ShootRange = InShootRange;
    }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetShouldConsumeAmmo(const bool InConsumeAmmo)
    {
        bConsumeAmmo = InConsumeAmmo;
    }

private:
    UPROPERTY(Replicated)
    TObjectPtr<UMeshComponent> shootingMesh;

    UPROPERTY(Replicated)
    TObjectPtr<APawn> characterOwner;

    UPROPERTY(ReplicatedUsing = OnRep_currentMagazine)
    int32 currentMagazine;

    UFUNCTION()
    void OnRep_currentMagazine();

    UFUNCTION()
    void FinishSwipe(const FVector& start, const FVector& direction);

    void Internal_Shoot(const FTransform& spawnTransform, const FVector& ShotDirection, float charge, TSubclassOf<class AACFProjectile> inProjClass);

    UFUNCTION(NetMulticast, Reliable)
    void Internal_SetupComponent(class APawn* inOwner, class UMeshComponent* inMesh);

    class UACFEquipmentComponent* TryGetEquipment() const;

    TSubclassOf<AACFItem> GetBestProjectileToShoot() const;
    FTimerHandle newTimer;
    bool bSwipeShooting;
};
