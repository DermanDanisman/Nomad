// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Item/NomadProjectile.h"
#include "ACMCollisionManagerComponent.h"       // For collision management, if needed.
#include "Core/Data/Item/Projectile/ProjectileData.h" // Include the projectile data asset header.
#include "GameFramework/ProjectileMovementComponent.h"

// Constructor: Called when an instance of ANomadProjectile is created.
ANomadProjectile::ANomadProjectile()
{
    // (No specific initialization here; properties will be set via InitializeItem.)
}

// BeginPlay: Called when the game starts or when the actor is spawned.
void ANomadProjectile::BeginPlay()
{
    // Call the base class BeginPlay to perform any inherited initialization.
    Super::BeginPlay();
    
    // Initialize the projectile properties from the data asset at runtime.
    InitializeItem();
}

// InitializeItem: Configures the projectile's properties by reading from the ProjectileData asset.
void ANomadProjectile::InitializeItem()
{
    // Ensure that the ProjectileData asset is set. If not, log an error.
    ensureMsgf(ProjectileData, TEXT("Projectile Data IS NOT SET!"));
    
    // Only proceed if the ProjectileData asset is valid.
    if (ProjectileData)
    {
        // Retrieve the projectile information structure from the data asset for easy access.
        const FProjectileInfo& Info = ProjectileData->ProjectileInfo;

        // ---------------------------
        // Mesh Setup
        // ---------------------------
        // If a world mesh is specified in the item info, assign it to the mesh component.
        if (Info.ItemInfo.WorldMesh)
        {
            MeshComp->SetStaticMesh(Info.ItemInfo.WorldMesh);
        }
        else
        {
            // Log a warning if no static mesh is assigned.
            UE_LOG(LogNomadProjectile, Warning, TEXT("No Static Mesh assigned for projectile: %s"), *ProjectileData->GetName());
        }
        
        // ---------------------------
        // Projectile Movement Settings
        // ---------------------------
        // Set the initial and maximum speed for the projectile.
        ProjectileMovementComp->InitialSpeed = Info.ProjectileInitialSpeed;
        ProjectileMovementComp->MaxSpeed = Info.ProjectileMaxSpeed;

        // Configure whether the projectile’s rotation should follow its velocity.
        ProjectileMovementComp->bRotationFollowsVelocity = Info.bRotationFollowsVelocity;

        // Configure whether the projectile’s rotation remains vertical.
        ProjectileMovementComp->bRotationRemainsVertical = Info.bRotationRemainsVertical;

        // Configure whether the initial velocity is defined in local space.
        ProjectileMovementComp->bInitialVelocityInLocalSpace = Info.bInitialVelocityInLocalSpace;

        // Set the gravity scale for the projectile.
        ProjectileMovementComp->ProjectileGravityScale = Info.ProjectileGravityScale;

        // ---------------------------
        // Lifetime and Hit Settings
        // ---------------------------
        // Set the lifespan for the projectile before auto-destruction.
        ProjectileLifespan = Info.ProjectileLifespan;

        // Set the hit policy (e.g., attach on hit or destroy on hit).
        HitPolicy = Info.HitPolicy;

        // For AttachOnHit policy, configure additional parameters.
        if (HitPolicy == EProjectileHitPolicy::AttachOnHit)
        {
            AttachedLifespan = Info.AttachedLifespan;
            bDroppableWhenAttached = Info.bDroppableWhenAttached;
            DropRatePercentage = Info.DropRatePercentage;
        }

        // For DestroyOnHit policy, assign the impact effect.
        if (HitPolicy == EProjectileHitPolicy::DestroyOnHit)
        {
            ImpactEffect = Info.ImpactEffect;
        }

        // ---------------------------
        // Sound & Collision Settings
        // ---------------------------
        // Assign the gather sound if specified.
        if (Info.GatherSound)
        {
            GatherSound = Info.GatherSound;
        }
        else
        {
            // Log a warning if no gather sound is defined.
            UE_LOG(LogNomadProjectile, Warning, TEXT("No GatherSound assigned for projectile: %s"), *ProjectileData->GetName());
        }

        // ---------------------------
        // Collision Configuration
        // ---------------------------
        // Configure collision properties for the projectile.
        CollisionComp->SetAllowMultipleHitsPerSwing(Info.bAllowMultipleHitsPerSwing);
        CollisionComp->SetCollisionChannels(Info.CollisionChannels);
        CollisionComp->SetIgnoredActors(Info.IgnoredActors);
        CollisionComp->SetIgnoreOwner(Info.bIgnoreOwner);
        CollisionComp->SetDamageTraces(Info.DamageTraces);
        CollisionComp->SetSwipeTraceInfo(Info.SwipeTraceInfo);
        CollisionComp->SetAreaDamageTraceInfo(Info.AreaDamageTraceInfo);

        // ---------------------------
        // Log Confirmation
        // ---------------------------
        // Log a confirmation message with key settings.
        UE_LOG(LogNomadProjectile, Log, TEXT("Projectile initialized with speed: %f, lifespan: %f"), Info.ProjectileInitialSpeed, ProjectileLifespan);
    }
    else
    {
        // Log an error if the data asset is missing.
        UE_LOG(LogNomadProjectile, Error, TEXT("ProjectileData asset is missing on projectile: %s"), *GetName());
    }
}

// Returns the thumbnail image for the projectile from the data asset.
UTexture2D* ANomadProjectile::GetThumbnailImage() const
{
    return ProjectileData->ProjectileInfo.ItemInfo.ThumbNail;
}

// Returns the item name from the projectile data.
FText ANomadProjectile::GetItemName() const
{
    return ProjectileData->ProjectileInfo.ItemInfo.Name;
}

// Returns the item description from the projectile data.
FText ANomadProjectile::GetItemDescription() const
{
    return ProjectileData->ProjectileInfo.ItemInfo.Description;
}

// Returns the item type (e.g., projectile) as defined in the projectile data.
EItemType ANomadProjectile::GetItemType() const
{
    return ProjectileData->ProjectileInfo.ItemInfo.ItemType;
}

// Returns the complete item descriptor, containing all the projectile's details.
FItemDescriptor ANomadProjectile::GetItemInfo() const
{
    return ProjectileData->ProjectileInfo.ItemInfo;
}

// Returns an array of gameplay tags representing possible item slots for this projectile.
TArray<FGameplayTag> ANomadProjectile::GetPossibleItemSlots() const
{
    return ProjectileData->ProjectileInfo.ItemInfo.GetPossibleItemSlots();
}

// ---------------------------
// INTERACTION INTERFACE IMPLEMENTATIONS
// ---------------------------

void ANomadProjectile::OnInteractableRegisteredByPawn_Implementation(APawn* Pawn)
{
    // Call the parent class implementation to handle any default behavior.
    Super::OnInteractableRegisteredByPawn_Implementation(Pawn);
}

void ANomadProjectile::OnInteractableUnregisteredByPawn_Implementation(APawn* Pawn)
{
    // Call the parent class implementation to handle any default behavior.
    Super::OnInteractableUnregisteredByPawn_Implementation(Pawn);
}

void ANomadProjectile::OnLocalInteractedByPawn_Implementation(APawn* Pawn, const FString& String)
{
    // Call the parent class implementation to handle any default local interaction behavior.
    Super::OnLocalInteractedByPawn_Implementation(Pawn, String);
}

void ANomadProjectile::OnInteractedByPawn_Implementation(APawn* Pawn, const FString& interactionType)
{
    // Call the parent class implementation to handle interaction.
    Super::OnInteractedByPawn_Implementation(Pawn, interactionType);
}

bool ANomadProjectile::CanBeInteracted_Implementation(APawn* Pawn)
{
    // Return whether the projectile can be interacted with, using the parent class's logic.
    return Super::CanBeInteracted_Implementation(Pawn);
}

FText ANomadProjectile::GetInteractableName_Implementation()
{
    // Return the name to display for interaction, using the parent class's logic.
    return Super::GetInteractableName_Implementation();
}