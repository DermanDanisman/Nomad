// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "ACMDamageActor.h"
#include <Components/InterpToMovementComponent.h>
#include <Components/MeshComponent.h>

// Default constructor: Initializes components and sets up replication.
AACMDamageActor::AACMDamageActor()
{
    // This actor doesn't need to tick every frame for performance reasons.
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);

    // Create the core components for damage logic, movement, and visuals.
    CollisionComp = CreateDefaultSubobject<UACMCollisionManagerComponent>(TEXT("Collisions Manager"));
    MovementComp = CreateDefaultSubobject<UInterpToMovementComponent>(TEXT("Movement Comp"));
    MeshComp = CreateDefaultSubobject<UMeshComponent>(TEXT("Mesh Comp"));
}

/**
 * Sets up the collision manager with a new owner and attaches it to the mesh.
 * Ignores the owner for collision (avoiding self-hit), and triggers the OnSetup event for further user customization.
 */
void AACMDamageActor::SetupCollisions(AActor* inOwner)
{
    if (CollisionComp)
    {
        CollisionComp->SetActorOwner(inOwner);
        CollisionComp->SetupCollisionManager(MeshComp);
        CollisionComp->AddActorToIgnore(inOwner);
        OnSetup(inOwner);
    }
}

/** Starts all collision traces (enables hit detection, e.g. for a sword swing or projectile). */
void AACMDamageActor::StartDamageTraces()
{
    if (CollisionComp)
    {
        CollisionComp->StartAllTraces();
    }
}

/** Stops all collision traces (disables hit detection). */
void AACMDamageActor::StopDamageTraces()
{
    if (CollisionComp)
    {
        CollisionComp->StopAllTraces();
    }
}

/**
 * Starts area-of-effect damage at the actor's current location.
 * @param radius - Radius of the damage area.
 * @param damageInterval - How often damage is applied (seconds).
 */
void AACMDamageActor::StartAreaDamage(float radius /*= 100.f*/, float damageInterval)
{
    if (CollisionComp)
    {
        CollisionComp->StartAreaDamage(GetActorLocation(), radius, damageInterval);
    }
}

/** Stops any ongoing area damage effect. */
void AACMDamageActor::StopAreaDamage()
{
    if (CollisionComp)
    {
        CollisionComp->StopCurrentAreaDamage();
    }
}

/** BlueprintNativeEvent: For user customization after collision setup. */
void AACMDamageActor::OnSetup_Implementation(AActor* new0wner) {}

/**
 * Called when the game starts or when spawned.
 * Binds the OnActorDamaged event from the collision manager and sets up initial collisions.
 */
void AACMDamageActor::BeginPlay()
{
    Super::BeginPlay();

    if (CollisionComp)
    {
        CollisionComp->OnActorDamaged.AddDynamic(this, &AACMDamageActor::HandleDamagedActor);
        SetupCollisions(ActorOwner);
    }
}

/**
 * Called when the actor ends play (is destroyed or removed from the world).
 * Unbinds the damage event to prevent dangling references.
 */
void AACMDamageActor::EndPlay(EEndPlayReason::Type end)
{
    Super::EndPlay(end);
    if (CollisionComp)
    {
        CollisionComp->OnActorDamaged.RemoveDynamic(this, &AACMDamageActor::HandleDamagedActor);
    }
}

/**
 * Internal handler: Called when the collision manager reports an actor was damaged.
 * Broadcasts the event to Blueprint and C++ listeners.
 */
void AACMDamageActor::HandleDamagedActor(AActor* damagedActor)
{
    OnActorDamaged.Broadcast(damagedActor);
}