// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NiagaraSystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundCue.h"
#include <GameFramework/DamageType.h>

#include "ACMTypes.generated.h"

class UParticleSystemComponent;
class UAudioComponent;

/**
 *
 */

/** Looping area-damage info */
USTRUCT(BlueprintType)
struct FAreaDamageInfo
{
    GENERATED_BODY()
    
    FAreaDamageInfo()
        : Radius(0.f)
          , Location(FVector::ZeroVector)
          , bIsActive(false)
          , AreaLoopTimer(FTimerHandle()) {}

    UPROPERTY(BlueprintReadOnly, Category = ACM)
    float Radius;

    UPROPERTY(BlueprintReadOnly, Category = ACM)
    FVector Location;

    UPROPERTY(BlueprintReadOnly, Category = ACM)
    bool bIsActive;

    /** Handle for repeating damage timers */
    UPROPERTY()
    FTimerHandle AreaLoopTimer;
};

USTRUCT(BlueprintType)
struct FHitActors
{
    GENERATED_BODY()

    FHitActors()
    : AlreadyHitActors()
    {}

    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = ACM)
    TArray<class AActor*> AlreadyHitActors;
};

UENUM(BlueprintType)
enum class EDebugType : uint8 {
    EDontShowDebugInfos = 0 UMETA(DisplayName = "Don't Show Debug Info"),
    EShowInfoDuringSwing = 1 UMETA(DisplayName = "Show Info During Swing"),
    EAlwaysShowDebug = 2 UMETA(DisplayName = "Always Show Debug Info"),
};

UENUM(BlueprintType)
enum class EDamageType : uint8 {
    EPoint UMETA(DisplayName = "Point Damage"),
    EArea UMETA(DisplayName = "Area Damage"),
};

UENUM(BlueprintType)
enum class ESpawnFXLocation : uint8 {
    ESpawnOnActorLocation UMETA(DisplayName = "Attached to Actor"),
    ESpawnAttachedToSocketOrBone UMETA(DisplayName = "Attached to Socket / Bone"),
    ESpawnAtLocation UMETA(DisplayName = "Spawn On Provided Tranform")
};

USTRUCT(BlueprintType)
struct FBaseFX : public FTableRowBase {
    GENERATED_BODY()

    FBaseFX()
        : ActionSound(nullptr)
        , NiagaraParticle(nullptr)
        , ActionParticle(nullptr)
    {}

    FBaseFX(USoundBase* InSound, UNiagaraSystem* InNiagara, UParticleSystem* InCascade)
        : ActionSound(InSound)
        , NiagaraParticle(InNiagara)
        , ActionParticle(InCascade)
    {}

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    class USoundBase* ActionSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    class UNiagaraSystem* NiagaraParticle;

    UPROPERTY(EditAnywhere, meta = (DeprecatedFunction, DeprecationMessage = "USE NIAGARA PARTICLE!!"), BlueprintReadWrite, Category = ACF)
    class UParticleSystem* ActionParticle;
};

USTRUCT(BlueprintType)
struct FAttachedComponents {
    GENERATED_BODY()

    FAttachedComponents()
        : CascadeComp(nullptr)
        , NiagaraComp(nullptr)
        , AudioComp(nullptr)
    {}

    TObjectPtr<UParticleSystemComponent> CascadeComp;
    TObjectPtr<UNiagaraComponent> NiagaraComp;
    TObjectPtr<UAudioComponent> AudioComp;
};

USTRUCT(BlueprintType, meta=(HasNativeStructInitializer))
struct FActionEffect : public FBaseFX {
    GENERATED_BODY()

    FActionEffect()
        : FBaseFX()
        , SocketOrBoneName(NAME_None)
        , SpawnLocation(ESpawnFXLocation::ESpawnOnActorLocation)
        , NoiseEmitted(0.f)
        , RelativeOffset(FTransform::Identity)
        , Guid(FGuid::NewGuid())
    {}

    FActionEffect(const FBaseFX& BaseFX, ESpawnFXLocation InLoc, const FName& InName)
        : FBaseFX(BaseFX)
        , SocketOrBoneName(InName)
        , SpawnLocation(InLoc)
        , NoiseEmitted(0.f)
        , RelativeOffset(FTransform::Identity)
        , Guid(FGuid::NewGuid())
    {}

    FGuid GetGuid() const
    {
        return Guid;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    FName SocketOrBoneName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    ESpawnFXLocation SpawnLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    float NoiseEmitted;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    FTransform RelativeOffset;

private:
    UPROPERTY(meta = (IgnoreForMemberInitializationTest))
    FGuid Guid;
};

USTRUCT(BlueprintType)
struct FImpactFX : public FBaseFX {
    GENERATED_BODY()

    FImpactFX()
    {
        ActionSound = nullptr;
        NiagaraParticle = nullptr;
        ActionParticle = nullptr;
        SpawnLocation = FTransform();
    }

    FImpactFX(const FBaseFX& baseFX, const FVector& location)
    {
        ActionSound = baseFX.ActionSound;
        NiagaraParticle = baseFX.NiagaraParticle;
        ActionParticle = baseFX.ActionParticle;
        SpawnLocation = FTransform(location);
    }

    FImpactFX(const FActionEffect& baseFX, const FTransform& location)
    {
        ActionSound = baseFX.ActionSound;
        NiagaraParticle = baseFX.NiagaraParticle;
        ActionParticle = baseFX.ActionParticle;
        SpawnLocation = location;
    }

    FImpactFX(const FImpactFX& baseFX)
    {
        ActionSound = baseFX.ActionSound;
        NiagaraParticle = baseFX.NiagaraParticle;
        ActionParticle = baseFX.ActionParticle;
        SpawnLocation = baseFX.SpawnLocation;
    }

    UPROPERTY(BlueprintReadWrite, Category = ACF)
    FTransform SpawnLocation;
};

USTRUCT(BlueprintType)
struct FMaterialImpactFX : public FBaseFX {
    GENERATED_BODY()

    FMaterialImpactFX()
        : FBaseFX()
        , ImpactMaterial(nullptr)
    {}

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    UPhysicalMaterial* ImpactMaterial;

    FORCEINLINE bool operator!=(const FMaterialImpactFX& Other) const
    {
        return ImpactMaterial != Other.ImpactMaterial;
    }

    FORCEINLINE bool operator==(const FMaterialImpactFX& Other) const
    {
        return ImpactMaterial == Other.ImpactMaterial;
    }

    FORCEINLINE bool operator!=(const UPhysicalMaterial* Other) const
    {
        return ImpactMaterial != Other;
    }

    FORCEINLINE bool operator==(const UPhysicalMaterial* Other) const
    {
        return ImpactMaterial == Other;
    }
};

USTRUCT(BlueprintType)
struct FImpactsArray {
    GENERATED_BODY()

    FImpactsArray()
        : ImpactsFX()
    {}

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    TArray<FMaterialImpactFX> ImpactsFX;
};

USTRUCT(BlueprintType)
struct FBaseTraceInfo {

    GENERATED_BODY()

    FBaseTraceInfo()
        : DamageTypeClass(UDamageType::StaticClass())
        , BaseDamage(0.f)
        , DamageType(EDamageType::EPoint)
    {}

    /** The type of damage applied*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACM)
    TSubclassOf<class UDamageType> DamageTypeClass;

    /** The base damage to apply to the actor (Can be modified in you TakeDamage Implementation)*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACM)
    float BaseDamage;

    /** Select if it's Area or Point Damage*/
    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ACM)
    EDamageType DamageType;
};

USTRUCT(BlueprintType)
struct FTraceInfo : public FBaseTraceInfo {
    GENERATED_BODY()

    FTraceInfo()
        : FBaseTraceInfo()
        , Radius(10.f)
        , TrailLength(1.f)
        , AttackSound(nullptr)
        , AttackParticle(nullptr)
        , NiagaraTrail(nullptr)
        , StartSocket(NAME_None)
        , EndSocket(NAME_None)
        , bCrossframeAccuracy(true)
        , bIsFirstFrame(true)
        , oldEndSocketPos(FVector::ZeroVector)
    {}

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACM)
    float Radius;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACM)
    float TrailLength;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACM)
    class USoundCue* AttackSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACM)
    class UParticleSystem* AttackParticle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACM)
    class UNiagaraSystem* NiagaraTrail;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACM)
    FName StartSocket;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACM)
    FName EndSocket;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ACM)
    bool bCrossframeAccuracy;

    bool bIsFirstFrame;
    FVector oldEndSocketPos;
};

UCLASS()
class COLLISIONSMANAGER_API UACMTypes : public UObject {
    GENERATED_BODY()
};