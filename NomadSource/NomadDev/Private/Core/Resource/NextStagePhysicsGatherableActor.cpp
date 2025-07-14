// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.


#include "Core/Resource/NextStagePhysicsGatherableActor.h"

#include "Kismet/GameplayStatics.h"

APhysicsGatherableActor::APhysicsGatherableActor()
{
    // Replicate the actor and its movement so clients see the physics simulation
    bReplicates = true;
    AActor::SetReplicateMovement(true);
    FRepMovement RepMovement;
    RepMovement.bRepPhysics = true;
    RepMovement.ServerPhysicsHandle = true;
    SetReplicatedMovement(RepMovement);

    // Turn on physics and collision notifications on the mesh
    ActorMesh->SetSimulatePhysics(true);
    ActorMesh->SetEnableGravity(true);
    ActorMesh->SetNotifyRigidBodyCollision(true);
    ActorMesh->SetIsReplicated(true);

    // Increase network update rate for smoother motion
    NetUpdateFrequency    = 66.f;
    MinNetUpdateFrequency = 10.f;
}

void APhysicsGatherableActor::BeginPlay()
{
    Super::BeginPlay();

    // 1) Apply initial impulse(s) immediately
    if (ActorMesh->IsSimulatingPhysics())
    {
        const float Mass = ActorMesh->GetMass();

        // — Tipping Impulse —
        if (bApplyTippingImpulse)
        {
            // Transform local direction into world-space
            FVector WorldDir = ActorMesh->GetComponentTransform()
                                       .TransformVectorNoScale(TippingDirection)
                                       .GetSafeNormal();

            // Impulse magnitude = mass * factor
            FVector Impulse = WorldDir * Mass * TippingImpulseFactor;

            // Hit near top of mesh for maximum torque:
            FVector HitLocation = ActorMesh->Bounds.Origin +
                                  FVector(0, 0, ActorMesh->Bounds.BoxExtent.Z);

            ActorMesh->AddImpulseAtLocation(Impulse, HitLocation);
        }

        // — Radial Explosion Impulse —
        if (bApplyRadialImpulse)
        {
            ActorMesh->AddRadialImpulse(
                GetActorLocation(),       // world center of the blast
                RadialImpulseRadius,      // how far it affects
                RadialImpulseStrength,    // magnitude of the impulse
                ERadialImpulseFalloff::RIF_Linear,  // falloff
                /*bVelChange=*/ true      // ignore mass, perfect velocity change
            );
        }
    }

    // 2) Schedule stop-physics after delay
    if (PhysicsSimulateDuration > 0.0f && GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            StopPhysicsTimerHandle,
            this,
            &APhysicsGatherableActor::StopPhysics,
            PhysicsSimulateDuration,
            false
        );
    }
}

void APhysicsGatherableActor::StopPhysics()
{
    if (ActorMesh)
    {
        // Freeze the mesh in place
        ActorMesh->SetSimulatePhysics(false);

        // Stop further movement replication (optional)
        AActor::SetReplicateMovement(false);
        FRepMovement RepMovement;
        RepMovement.bRepPhysics = false;
        RepMovement.ServerPhysicsHandle = false;
        SetReplicatedMovement(RepMovement);
    }
}