// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "ACMCollisionManagerComponent.h"
#include "ACMCollisionsFunctionLibrary.h"
#include "ACMCollisionsMasterComponent.h"
#include "ACMTypes.h"
#include "Components/ActorComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include <Components/ActorComponent.h>
#include <Components/MeshComponent.h>
#include <Components/SceneComponent.h>
#include <Engine/EngineTypes.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>
#include <GameFramework/GameModeBase.h>
#include <Kismet/GameplayStatics.h>
#include <Kismet/KismetSystemLibrary.h>
#include <Particles/ParticleSystemComponent.h>
#include <Sound/SoundBase.h>
#include <Sound/SoundCue.h>
#include <Templates/Function.h>
#include <TimerManager.h>
#include <WorldCollision.h>

// Sets default values for this component's properties
UACMCollisionManagerComponent::UACMCollisionManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetComponentTickEnabled(false);
}

void UACMCollisionManagerComponent::BeginPlay()
{
    Super::BeginPlay();
    SetComponentTickEnabled(false);
    SetStarted(false);
}

void UACMCollisionManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
    {
        StopCurrentAreaDamage();
        StopAllTraces();
    }
    Super::EndPlay(EndPlayReason);
}

// Updates all active traces, processes collisions, and applies damage. Handles debug drawing.
void UACMCollisionManagerComponent::UpdateCollisions()
{
    if (damageMesh)
    {
        DisplayDebugTraces();

        if (pendingDelete.IsValidIndex(0))
        {
            for (const auto toDelete : pendingDelete)
            {
                if (activatedTraces.Contains(toDelete))
                {
                    activatedTraces.Remove(toDelete);
                }
                alreadyHitActors.Remove(toDelete);
            }
            pendingDelete.Empty();
        }
        if (activatedTraces.Num() == 0)
        {
            SetStarted(false);
            return;
        }
        if (CollisionChannels.IsValidIndex(0))
        {
            for (TPair<FName, FTraceInfo>& currentTrace : activatedTraces)
            {
                if (damageMesh->DoesSocketExist(currentTrace.Value.StartSocket) && damageMesh->DoesSocketExist(currentTrace.Value.EndSocket))
                {
                    FHitResult hitRes;

                    const FVector StartPos = damageMesh->GetSocketLocation(currentTrace.Value.StartSocket);
                    const FVector EndPos = damageMesh->GetSocketLocation(currentTrace.Value.EndSocket);
                    const FRotator orientation = GetLineRotation(StartPos, EndPos);
                    FCollisionQueryParams Params;

                    if (IgnoredActors.Num() > 0)
                    {
                        Params.AddIgnoredActors(IgnoredActors);
                    }

                    if (bIgnoreOwner)
                    {
                        Params.AddIgnoredActor(GetActorOwner());
                        Params.AddIgnoredActor(GetOwner());
                    }

                    Params.bReturnPhysicalMaterial = true;
                    Params.bTraceComplex = true;

                    FCollisionObjectQueryParams ObjectParams;
                    for (const TEnumAsByte<ECollisionChannel>& channel : CollisionChannels)
                    {
                        if (ObjectParams.IsValidObjectQuery(channel))
                        {
                            ObjectParams.AddObjectTypesToQuery(channel);
                        }
                    }

                    if (!bAllowMultipleHitsPerSwing)
                    {
                        FHitActors* hitResact = alreadyHitActors.Find(currentTrace.Key);
                        if (hitResact && hitResact->AlreadyHitActors.Num() > 0)
                        {
                            Params.AddIgnoredActors(hitResact->AlreadyHitActors);
                        }
                    }

                    if (ObjectParams.IsValid() == false)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Invalid Collision Channel - UACMCollisionManagerComponent::UpdateCollisions()"));
                        return;
                    }

                    UWorld* world = GetWorld();
                    if (world)
                    {
                        bool bHit = world->SweepSingleByObjectType(
                            hitRes, StartPos, EndPos, orientation.Quaternion(), ObjectParams, FCollisionShape::MakeSphere(currentTrace.Value.Radius), Params);

                        if (!bHit && currentTrace.Value.bCrossframeAccuracy && !currentTrace.Value.bIsFirstFrame)
                        {
                            const FRotator oldOrient = GetLineRotation(StartPos, EndPos);
                            bHit = world->SweepSingleByObjectType(
                                hitRes, StartPos, currentTrace.Value.oldEndSocketPos, oldOrient.Quaternion(), ObjectParams, FCollisionShape::MakeSphere(currentTrace.Value.Radius), Params);
                        }
                        if (bHit)
                        {
                            OnCollisionDetected.Broadcast(hitRes);
                            if (!bAllowMultipleHitsPerSwing)
                            {
                                FHitActors* hitResact = alreadyHitActors.Find(currentTrace.Key);
                                if (hitResact)
                                {
                                    hitResact->AlreadyHitActors.Add(hitRes.GetActor());
                                } else
                                {
                                    FHitActors newHit;
                                    newHit.AlreadyHitActors.Add(hitRes.GetActor());
                                    alreadyHitActors.Add(currentTrace.Key, newHit);
                                }
                            }
                            ApplyDamage(hitRes, currentTrace.Value);
                        }
                        currentTrace.Value.bIsFirstFrame = false;
                        currentTrace.Value.oldEndSocketPos = EndPos;
                    }
                } else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Invalid Socket Names!! - UACMCollisionManagerComponent::UpdateCollisions()"));
                }
            }
        } else
        {
            SetStarted(false);
        }
    }
}

// Returns the first trace in the DamageTraces map (if any).
FTraceInfo UACMCollisionManagerComponent::GetFirstTrace() const
{
    for (const auto& trace : DamageTraces)
    {
        return trace.Value;
    }
    return FTraceInfo();
}

// Sets the "started" state for this component (activates debug, registers with collisions master, etc).
void UACMCollisionManagerComponent::SetStarted(bool inStarted)
{
    bIsStarted = inStarted;
    AGameModeBase* gameMode = UGameplayStatics::GetGameMode(this);
    if (gameMode)
    {
        UACMCollisionsMasterComponent* collisionMaster = gameMode->FindComponentByClass<UACMCollisionsMasterComponent>();
        if (collisionMaster)
        {
            if (ShowDebugInfo == EDebugType::EAlwaysShowDebug || bIsStarted)
            {
                collisionMaster->AddComponent(this);
            } else
            {
                collisionMaster->RemoveComponent(this);
            }
        } else
        {
            UE_LOG(LogTemp, Error, TEXT("Add Collisions Master o your Game Mode!"));
        }
    }
}

// Utility: Get the rotation from start to end vectors.
FRotator UACMCollisionManagerComponent::GetLineRotation(FVector start, FVector end)
{
    const FVector diff = end - start;
    return diff.Rotation();
}

// Binds a mesh for sockets, initializes particle systems for each trace.
void UACMCollisionManagerComponent::SetupCollisionManager(class UMeshComponent* inDamageMesh)
{
    damageMesh = inDamageMesh;

    if (!damageMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid Damage mesh!!"));
        return;
    }

    for (const auto& trace : DamageTraces)
    {
        UParticleSystemComponent* ParticleSystemComp = NewObject<UParticleSystemComponent>(this, UParticleSystemComponent::StaticClass());
        ParticleSystemComp->SetupAttachment(damageMesh);
        ParticleSystemComp->SetRelativeLocation(FVector::ZeroVector);
        ParticleSystemComponents.Add(trace.Key, ParticleSystemComp);
        ParticleSystemComp->RegisterComponent();
    }
}

// Starts area damage, repeating at given interval.
void UACMCollisionManagerComponent::StartAreaDamage_Implementation(const FVector& damageCenter, float damageRadius, float damageInterval /*= 1.f*/)
{
    UWorld* world = GetWorld();
    if (world)
    {
        PerformAreaDamage_Single(damageCenter, damageRadius);
        world->GetTimerManager().SetTimer(
            AreaDamageTimer, this, &UACMCollisionManagerComponent::HandleAreaDamageLooping, damageInterval, true);
    }
}

// Stops current area damage.
void UACMCollisionManagerComponent::StopCurrentAreaDamage_Implementation()
{
    if (currentAreaDamage.bIsActive)
    {
        currentAreaDamage.bIsActive = false;
        UWorld* world = GetWorld();
        world->GetTimerManager().ClearTimer(currentAreaDamage.AreaLoopTimer);
    }
}

// Performs a one-shot area damage event (server).
void UACMCollisionManagerComponent::PerformAreaDamage_Single_Implementation(const FVector& damageCenter, float damageRadius)
{
    TArray<FHitResult> outHits;
    PerformAreaDamage_Single_Local(damageCenter, damageRadius, outHits);
}

// Performs a local-only area damage check, writes hit results to outHits.
void UACMCollisionManagerComponent::PerformAreaDamage_Single_Local(const FVector& damageCenter, float damageRadius, TArray<FHitResult>& outHits)
{
    FCollisionQueryParams Params;
    if (IgnoredActors.Num() > 0)
    {
        Params.AddIgnoredActors(IgnoredActors);
    }

    if (bIgnoreOwner)
    {
        Params.AddIgnoredActor(GetActorOwner());
    }

    UWorld* world = GetWorld();
    alreadyHitActorsBySphere.Empty();
    outHits.Empty();
    if (world)
    {
        for (const TEnumAsByte<ECollisionChannel>& channel : CollisionChannels)
        {
            TArray<FHitResult> outResults;
            const bool bHit = world->SweepMultiByChannel(outResults, damageCenter, damageCenter + FVector(1.f), FQuat::Identity, channel, FCollisionShape::MakeSphere(damageRadius), Params);

            if (bHit)
            {
                outHits.Append(outResults);
            }
        }
        for (const auto& hit : outHits)
        {
            if (!alreadyHitActorsBySphere.Contains(hit.GetActor()))
            {
                alreadyHitActorsBySphere.Add(hit.GetActor());
                ApplyDamage(hit, AreaDamageTraceInfo);
            }
        }
        if (static_cast<uint8>(ShowDebugInfo) > 0)
        {
            ShowDebugTrace(damageCenter, damageCenter + FVector(1.f), damageRadius, EDrawDebugTrace::ForDuration, 3.f, FColor::Red);
        }
    }
}

// Start area damage for a given duration and interval (server).
void UACMCollisionManagerComponent::PerformAreaDamageForDuration_Implementation(const FVector& damageCenter, float damageRadius, float duration, float damageInterval /*= 1.f*/)
{
    UWorld* world = GetWorld();
    if (world)
    {
        StartAreaDamage(damageCenter, damageRadius, damageInterval);

        world->GetTimerManager().SetTimer(
            AreaDamageTimer, this, &UACMCollisionManagerComponent::HandleAreaDamageFinished,
            duration, false);
    }
}

// Add an actor to the ignore list.
void UACMCollisionManagerComponent::AddActorToIgnore(class AActor* ignoredActor)
{
    IgnoredActors.AddUnique(ignoredActor);
}

// Add a single collision channel to the list.
void UACMCollisionManagerComponent::AddCollisionChannel(TEnumAsByte<ECollisionChannel> inTraceChannel)
{
    CollisionChannels.AddUnique(inTraceChannel);
}

// Add multiple collision channels.
void UACMCollisionManagerComponent::AddCollisionChannels(TArray<TEnumAsByte<ECollisionChannel>> inTraceChannels)
{
    for (const TEnumAsByte<ECollisionChannel>& chan : inTraceChannels)
    {
        AddCollisionChannel(chan);
    }
}

// Clear all collision channels.
void UACMCollisionManagerComponent::ClearCollisionChannels()
{
    CollisionChannels.Empty();
}

// Performs a swipe trace between two points (server) and applies damage if hit.
void UACMCollisionManagerComponent::PerformSwipeTraceShot_Implementation(const FVector& start, const FVector& end, float radius)
{
    FHitResult outHit;
    PerformSwipeTraceShot_Local(start, end, radius, outHit);
}

// Local-only swipe trace for single shot, outputs hit result.
void UACMCollisionManagerComponent::PerformSwipeTraceShot_Local(const FVector& start, const FVector& end, float radius /*= 0.f*/, FHitResult& outHit)
{
    if (actorOwner)
    {
        EDrawDebugTrace::Type debugType;
        switch (ShowDebugInfo)
        {
        case EDebugType::EAlwaysShowDebug:
        case EDebugType::EShowInfoDuringSwing:
            break;
        case EDebugType::EDontShowDebugInfos:
            debugType = EDrawDebugTrace::Type::None;
            break;
        default:
            debugType = EDrawDebugTrace::Type::None;
            break;
        }

        FCollisionQueryParams Params;
        if (IgnoredActors.Num() > 0)
        {
            Params.AddIgnoredActors(IgnoredActors);
        }

        if (bIgnoreOwner)
        {
            Params.AddIgnoredActor(GetActorOwner());
            Params.AddIgnoredActor(GetOwner());
        }

        Params.bReturnPhysicalMaterial = true;
        Params.bTraceComplex = true;
        alreadyHitActorsBySweep.Empty();
        UWorld* world = GetWorld();
        if (world)
        {
            FHitResult outResult;
            const FRotator orientation = GetLineRotation(start, end);

            FCollisionObjectQueryParams ObjectParams;
            for (const TEnumAsByte<ECollisionChannel>& channel : CollisionChannels)
            {
                if (ObjectParams.IsValidObjectQuery(channel))
                {
                    ObjectParams.AddObjectTypesToQuery(channel);
                }
            }

            if (ObjectParams.IsValid() == false)
            {
                UE_LOG(LogTemp, Warning, TEXT("Invalid Collision Channel - UACMCollisionManagerComponent::UpdateCollisions()"));
                return;
            }

            bool bHit = world->SweepSingleByObjectType(
                outResult, start, end, orientation.Quaternion(), ObjectParams, FCollisionShape::MakeSphere(radius), Params);

            if (bHit && !alreadyHitActorsBySweep.Contains(outResult.GetActor()))
            {
                alreadyHitActorsBySweep.Add(outResult.GetActor());
                ApplyDamage(outResult, SwipeTraceInfo);
                outHit = outResult;
                OnCollisionDetected.Broadcast(outResult);
            }

            switch (ShowDebugInfo)
            {
            case EDebugType::EAlwaysShowDebug:
            case EDebugType::EShowInfoDuringSwing:
                ShowDebugTrace(start, end, radius, EDrawDebugTrace::ForDuration, 3.f, FColor::Red);
                break;
            case EDebugType::EDontShowDebugInfos:
                break;
            default:
                break;
            }
        }
    }
}

// Starts all traces as active.
void UACMCollisionManagerComponent::StartAllTraces_Implementation()
{
    activatedTraces.Empty();
    pendingDelete.Empty();

    for (const auto& damage : DamageTraces)
    {
        StartSingleTrace(damage.Key);
    }
}

// Stops all traces.
void UACMCollisionManagerComponent::StopAllTraces_Implementation()
{
    pendingDelete.Empty();
    for (const auto& trace : activatedTraces)
    {
        StopSingleTrace(trace.Key);
    }
}

// Starts a single trace by name.
void UACMCollisionManagerComponent::StartSingleTrace_Implementation(const FName& Name)
{
    FTraceInfo* outTrace = DamageTraces.Find(Name);
    if (outTrace)
    {
        if (pendingDelete.Contains(Name))
        {
            pendingDelete.Remove(Name);
        }
        outTrace->bIsFirstFrame = true;
        activatedTraces.Add(Name, *outTrace);
        PlayTrails(Name);
        SetStarted(true);
    } else
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid Trace Name!!"));
    }
}

// Stops a single trace by name.
void UACMCollisionManagerComponent::StopSingleTrace_Implementation(const FName& Name)
{
    if (activatedTraces.Contains(Name))
    {
        StopTrails(Name);
        pendingDelete.AddUnique(Name);

        FHitActors* alreadyHit = alreadyHitActors.Find(Name);
        if (alreadyHit)
        {
            alreadyHit->AlreadyHitActors.Empty();
        }
    }
}

// Debug: draws traces based on debug settings.
void UACMCollisionManagerComponent::DisplayDebugTraces()
{
    TMap<FName, FTraceInfo> _sphere;
    FLinearColor DebugColor;
    switch (ShowDebugInfo)
    {
    case EDebugType::EAlwaysShowDebug:
        _sphere = DamageTraces;
        if (bIsStarted)
        {
            DebugColor = DebugActiveColor;
        } else
        {
            DebugColor = DebugInactiveColor;
        }
        break;
    case EDebugType::EShowInfoDuringSwing:
        if (bIsStarted)
        {
            _sphere = activatedTraces;
            DebugColor = DebugActiveColor;
        } else
        {
            return;
        }
        break;
    case EDebugType::EDontShowDebugInfos:
        return;
    default:
        return;
    }

    for (const TPair<FName, FTraceInfo>& box : _sphere)
    {
        if (damageMesh->DoesSocketExist(box.Value.StartSocket) && damageMesh->DoesSocketExist(box.Value.EndSocket))
        {
            const FVector StartPos = damageMesh->GetSocketLocation(box.Value.StartSocket);
            const FVector EndPos = damageMesh->GetSocketLocation(box.Value.EndSocket);
            const float radius = box.Value.Radius;

            ShowDebugTrace(StartPos, EndPos, radius, EDrawDebugTrace::ForDuration, 2.0f);
        }
    }
}

// Draws a debug cylinder to visualize the trace.
void UACMCollisionManagerComponent::ShowDebugTrace(const FVector& StartPos, const FVector& EndPos, const float radius, EDrawDebugTrace::Type DrawDebugType, float duration, FLinearColor DebugColor)
{
    FHitResult hitRes;
    UWorld* world = GetWorld();
    if (world)
    {
        UKismetSystemLibrary::DrawDebugCylinder(this, StartPos, EndPos, radius, 12, DebugColor, duration);
    }
}

// Starts a timed single trace.
void UACMCollisionManagerComponent::StartTimedSingleTrace_Implementation(const FName& TraceName, float Duration)
{
    UWorld* world = GetWorld();
    if (world)
    {
        StartSingleTrace(TraceName);
        FTimerHandle timerHandle;
        FTimerDelegate TimerDelegate;

        TFunction<void(void)> lambdaDelegate = [this, TraceName]()
        {
            HandleTimedSingleTraceFinished(TraceName);
        };
        TimerDelegate.BindLambda(lambdaDelegate);
        TraceTimers.Add(TraceName, timerHandle);
        world->GetTimerManager().SetTimer(timerHandle, TimerDelegate, Duration, false);
    }
}

// Starts all traces for a fixed duration.
void UACMCollisionManagerComponent::StartAllTimedTraces_Implementation(float Duration)
{
    UWorld* world = GetWorld();
    if (world && !bAllTimedTraceStarted)
    {
        StartAllTraces();
        world->GetTimerManager().SetTimer(AllTraceTimer, this,
            &UACMCollisionManagerComponent::HandleAllTimedTraceFinished, Duration, false);
        bAllTimedTraceStarted = true;
    }
}

// Gets the actor considered as "owner" for collision/damage.
AActor* UACMCollisionManagerComponent::GetActorOwner() const
{
    if (actorOwner)
    {
        return actorOwner;
    }
    return GetOwner();
}

// Sets trace config for a given trace name.
void UACMCollisionManagerComponent::SetTraceConfig(const FName& traceName, const FTraceInfo& traceInfo)
{
    DamageTraces.Add(traceName, traceInfo);
}

// Handles when a timed single trace finishes.
void UACMCollisionManagerComponent::HandleTimedSingleTraceFinished(const FName& traceEnded)
{
    if (IsValid(GetOwner()))
    {
        UWorld* world = GetWorld();
        if (world && TraceTimers.Contains(traceEnded))
        {
            StopSingleTrace(traceEnded);
            FTimerHandle* handle = TraceTimers.Find(traceEnded);
            world->GetTimerManager().ClearTimer(*handle);
        }
    }
}

// Handles when all timed traces finish.
void UACMCollisionManagerComponent::HandleAllTimedTraceFinished()
{
    StopAllTraces();
    if (GetOwner())
    {
        UWorld* world = GetWorld();
        if (world && bAllTimedTraceStarted)
        {
            world->GetTimerManager().ClearTimer(AllTraceTimer);
            bAllTimedTraceStarted = false;
        }
    }
}

// Returns if a trace by given name is currently active.
bool UACMCollisionManagerComponent::IsTraceActive(const FName& traceName)
{
    return activatedTraces.Contains(traceName);
}

// Sets the actor considered as "owner" for collision/damage.
void UACMCollisionManagerComponent::SetActorOwner(AActor* newOwner)
{
    actorOwner = newOwner;
}

// Applies damage to a hit result using the current trace config.
void UACMCollisionManagerComponent::ApplyDamage(const FHitResult& HitResult, const FBaseTraceInfo& currentTrace)
{
    if (IgnoredActors.Contains(HitResult.GetActor()))
    {
        return;
    }

    UACMCollisionsFunctionLibrary::PlayImpactEffect(currentTrace.DamageTypeClass, HitResult.PhysMaterial.Get(), HitResult.Location, this);
    switch (currentTrace.DamageType)
    {
    case EDamageType::EPoint:
        ApplyPointDamage(HitResult, currentTrace);
        break;
    case EDamageType::EArea:
        ApplyAreaDamage(HitResult, currentTrace);
        break;
    default:
        ApplyPointDamage(HitResult, currentTrace);
        break;
    }
}

// Applies point damage (e.g., sword poke).
void UACMCollisionManagerComponent::ApplyPointDamage(const FHitResult& HitResult, const FBaseTraceInfo& currentTrace)
{
    if (IsValid(HitResult.GetActor()))
    {
        const FVector damagerRelativePos = GetOwner()->GetActorLocation() - HitResult.GetActor()->GetActorLocation();
        const float damage = currentTrace.BaseDamage;
        FPointDamageEvent damageInfo;

        damageInfo.DamageTypeClass = currentTrace.DamageTypeClass;
        damageInfo.Damage = currentTrace.BaseDamage;
        damageInfo.HitInfo = HitResult;
        damageInfo.ShotDirection = damagerRelativePos;
        HitResult.GetActor()->TakeDamage(damage, damageInfo, GetActorOwner()->GetInstigatorController(), GetActorOwner());

        OnActorDamaged.Broadcast(HitResult.GetActor());
    }
}

// Applies area damage (e.g., explosion pulse).
void UACMCollisionManagerComponent::ApplyAreaDamage(const FHitResult& HitResult, const FBaseTraceInfo& currentTrace)
{
    if (IsValid(HitResult.GetActor()))
    {
        float damage = currentTrace.BaseDamage;
        FRadialDamageEvent damageInfo;

        damageInfo.DamageTypeClass = currentTrace.DamageTypeClass;
        damageInfo.Params.BaseDamage = currentTrace.BaseDamage;
        damageInfo.ComponentHits.Add(HitResult);
        damageInfo.Origin = HitResult.ImpactPoint;

        HitResult.GetActor()->TakeDamage(damage, damageInfo, GetActorOwner()->GetInstigatorController(), GetActorOwner());
        OnActorDamaged.Broadcast(HitResult.GetActor());
    }
}

// Handler for when area damage duration finishes.
void UACMCollisionManagerComponent::HandleAreaDamageFinished()
{
    StopCurrentAreaDamage();
}

// Handler for each pulse of looping area damage.
void UACMCollisionManagerComponent::HandleAreaDamageLooping()
{
    PerformAreaDamage_Single(currentAreaDamage.Location, currentAreaDamage.Radius);
}

// Multicast: Play trails (particles, Niagara) for a trace on all clients.
void UACMCollisionManagerComponent::PlayTrails_Implementation(const FName& trail)
{
    if (!DamageTraces.Contains(trail) || !damageMesh)
    {
        return;
    }
    FTraceInfo traceInfo = *DamageTraces.Find(trail);

    if (traceInfo.AttackParticle && ParticleSystemComponents.Contains(trail) && damageMesh->DoesSocketExist(traceInfo.StartSocket)
        && damageMesh->DoesSocketExist(traceInfo.EndSocket))
    {
        UParticleSystemComponent* partComp = *ParticleSystemComponents.Find(trail);
        partComp->SetTemplate(traceInfo.AttackParticle);
        partComp->BeginTrails(traceInfo.StartSocket, traceInfo.EndSocket, ETrailWidthMode_FromCentre, traceInfo.TrailLength);
    }

    if (traceInfo.AttackSound)
    {
        UGameplayStatics::SpawnSoundAttached(traceInfo.AttackSound, damageMesh, traceInfo.StartSocket);
    }

    if (traceInfo.NiagaraTrail)
    {
        UNiagaraComponent* niagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(traceInfo.NiagaraTrail,
            damageMesh, traceInfo.StartSocket, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, false, true);
        NiagaraSystemComponents.Add(trail, niagaraComp);
    }
}

// Multicast: Stop trails (particles, Niagara) for a trace on all clients.
void UACMCollisionManagerComponent::StopTrails_Implementation(const FName& trail)
{
    if (ParticleSystemComponents.Contains(trail))
    {
        UParticleSystemComponent* partComp = *ParticleSystemComponents.Find(trail);
        if (partComp)
        {
            partComp->EndTrails();
        }
    }
    if (NiagaraSystemComponents.Contains(trail))
    {
        UNiagaraComponent* partComp = *NiagaraSystemComponents.Find(trail);

        if (partComp)
        {
            partComp->DestroyComponent();
            partComp->DeactivateImmediate();
            partComp->DestroyInstance();
        }
        NiagaraSystemComponents.Remove(trail);
    }
}