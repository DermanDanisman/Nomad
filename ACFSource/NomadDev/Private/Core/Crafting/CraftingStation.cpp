// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.


#include "Core/Crafting/CraftingStation.h"

#include "Core/Crafting/NomadCraftingComponent.h"
#include "NomadDev/NomadDev.h"

// Sets default values
ACraftingStation::ACraftingStation()
{
    // Disable ticking for better performance since we don't need Tick()
    PrimaryActorTick.bCanEverTick = false;

    // Create and set root component
    DefaultRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRootComponent"));
    SetRootComponent(DefaultRootComponent);

    // Create skeletal mesh component and attach to root
    CraftingStationSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CraftingStationSkeletalMesh"));
    CraftingStationSkeletalMesh->SetupAttachment(GetRootComponent());
    CraftingStationSkeletalMesh->SetCollisionProfileName(FName("Interactable"));
    CraftingStationSkeletalMesh->SetCollisionObjectType(ECC_Interactable);
    CraftingStationSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    // Create static mesh component and attach to root
    CraftingStationStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CraftingStationStaticMesh"));
    CraftingStationStaticMesh->SetupAttachment(GetRootComponent());
    CraftingStationStaticMesh->SetCollisionProfileName(FName("Interactable"));
    CraftingStationStaticMesh->SetCollisionObjectType(ECC_Interactable);
    CraftingStationStaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    // Create map marker component and attach to root
    CraftingStationMapMarkerComponent = CreateDefaultSubobject<UAMSMapMarkerComponent>(TEXT("CraftingStationMapMarkerComponent"));
    CraftingStationMapMarkerComponent->SetupAttachment(GetRootComponent());

    // Create crafting component (child class)
    NomadCraftingComponent = CreateDefaultSubobject<UNomadCraftingComponent>(TEXT("NomadCraftingComponent"));
}

void ACraftingStation::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // Setup meshes and map marker
    UpdateMeshesAndMarker();

    // Initialize crafting component with data asset
    if (IsValid(NomadCraftingComponent))
    {
        NomadCraftingComponent->InitializeFromDataAsset(CraftingStationData);
    }
}

// Called when the game starts or when spawned
void ACraftingStation::BeginPlay()
{
    Super::BeginPlay();

    // Setup meshes and map marker again at runtime
    UpdateMeshesAndMarker();

    // Initialize crafting component with data asset at runtime
    if (IsValid(NomadCraftingComponent))
    {
        NomadCraftingComponent->InitializeFromDataAsset(CraftingStationData);
    }
}

// Helper function to setup meshes and marker from data asset
void ACraftingStation::UpdateMeshesAndMarker() const
{
    if (!CraftingStationData)
    {
        UE_LOG(LogTemp, Warning, TEXT("CraftingStationData not assigned on %s!"), *GetName());
        return;
    }

    // Setup Skeletal Mesh if valid
    if (IsValid(CraftingStationSkeletalMesh) && IsValid(CraftingStationData->GetSkeletalMesh()))
    {
        if (CraftingStationSkeletalMesh->GetSkinnedAsset() != CraftingStationData->GetSkeletalMesh())
        {
            CraftingStationSkeletalMesh->SetSkeletalMesh(CraftingStationData->GetSkeletalMesh());
        }
        CraftingStationSkeletalMesh->SetVisibility(true);

        // Hide static mesh when skeletal mesh is used
        if (IsValid(CraftingStationStaticMesh))
        {
            CraftingStationStaticMesh->SetVisibility(false);
        }
    }
    // Setup Static Mesh if Skeletal Mesh is not set
    else if (IsValid(CraftingStationStaticMesh) && IsValid(CraftingStationData->GetStaticMesh()))
    {
        if (CraftingStationStaticMesh->GetStaticMesh() != CraftingStationData->GetStaticMesh())
        {
            CraftingStationStaticMesh->SetStaticMesh(CraftingStationData->GetStaticMesh());
        }
        CraftingStationStaticMesh->SetVisibility(true);

        // Hide skeletal mesh when static mesh is used
        if (IsValid(CraftingStationSkeletalMesh))
        {
            CraftingStationSkeletalMesh->SetVisibility(false);
        }
    }
    else
    {
        // Hide both meshes if none assigned
        if (IsValid(CraftingStationStaticMesh))
        {
            CraftingStationStaticMesh->SetVisibility(false);
        }
        if (IsValid(CraftingStationSkeletalMesh))
        {
            CraftingStationSkeletalMesh->SetVisibility(false);
        }
    }

    // Setup map marker component properties from data asset
    if (IsValid(CraftingStationMapMarkerComponent))
    {
        CraftingStationMapMarkerComponent->SetMarkerTexture(CraftingStationData->GetMarkerTexture());
        CraftingStationMapMarkerComponent->SetMarkerCategory(CraftingStationData->GetMarkerCategory());
        CraftingStationMapMarkerComponent->SetMarkerName(CraftingStationData->GetMarkerName().ToString());
        CraftingStationMapMarkerComponent->SetShouldRotate(CraftingStationData->ShouldRotate());
        CraftingStationMapMarkerComponent->SetActivateWorldWidget(CraftingStationData->ShouldActivateWorldWidget());
    }
}

// Called when a pawn registers this as interactable
void ACraftingStation::OnInteractableRegisteredByPawn_Implementation(class APawn* Pawn)
{
    // Implementation here (optional)
}

// Called when a pawn unregisters this as interactable
void ACraftingStation::OnInteractableUnregisteredByPawn_Implementation(class APawn* Pawn)
{
    // Implementation here (optional)
}

// Called when a pawn interacts with this station
void ACraftingStation::OnInteractedByPawn_Implementation(class APawn* Pawn, const FString& interactionType)
{
    // Implementation here (optional)
}

// Returns the display name of this interactable
FText ACraftingStation::GetInteractableName_Implementation()
{
    if (CraftingStationData)
    {
        return CraftingStationData->GetCraftingStationName();
    }
    return FText::FromString(TEXT("Unknown Crafting Station"));
}

// Whether the pawn can interact with this station
bool ACraftingStation::CanBeInteracted_Implementation(class APawn* Pawn)
{
    return true; // Adjust as needed
}
