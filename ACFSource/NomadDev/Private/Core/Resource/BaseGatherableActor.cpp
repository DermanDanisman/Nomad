// Core/Resource/BaseGatherableActor.cpp
#include "Core/Resource/BaseGatherableActor.h"

#include "Components/ACFEquipmentComponent.h"
#include "Components/ACFStorageComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/Data/Item/Resource/GatherableActorData.h"
#include "Core/FunctionLibrary/NomadItemSystemFunctionLibrary.h"
#include "Game/ACFFunctionLibrary.h"
#include "Net/UnrealNetwork.h"


ABaseGatherableActor::ABaseGatherableActor()
{
    // Disable tick; no per-frame logic needed for this actor
    PrimaryActorTick.bCanEverTick = false;

    // Setup replication for this actor to ensure it updates correctly across the network
    bReplicates = true;
    AActor::SetReplicateMovement(true);
    FRepMovement RepMovement;
    RepMovement.bRepPhysics = true;   // Replicating physics for smoother movement
    RepMovement.ServerPhysicsHandle = true;  // Ensure the server handles the physics for this actor
    SetReplicatedMovement(RepMovement);
    NetUpdateFrequency    = 66.f;   // Network update frequency for smoother movement updates
    MinNetUpdateFrequency = 10.f;   // Minimum update frequency for network updates

    // Create the default scene root, which serves as the base for attaching other components
    DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
    RootComponent    = DefaultSceneRoot;   // Set DefaultSceneRoot as the root of the actor

    // Create and attach the storage component, which manages the inventory for this actor
    StorageComponent = CreateDefaultSubobject<UACFStorageComponent>(TEXT("StorageComponent"));

    // Create the mesh component for the actor to visually represent the gatherable resource
    ActorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ActorMesh"));
    ActorMesh->SetupAttachment(DefaultSceneRoot);  // Attach mesh to the root component
    ActorMesh->SetCollisionProfileName(TEXT("BlockAll")); // Block collisions with everything
    ActorMesh->bReceivesDecals = false; // Turn off decals to avoid unnecessary visual effects
    ActorMesh->SetSimulatePhysics(false); // Disable physics simulation, since this actor doesn't move by itself
    ActorMesh->SetIsReplicated(true); // Ensure the mesh updates are replicated across the network
}

void ABaseGatherableActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    // Replicate the current mesh and other properties so they can sync across clients
    DOREPLIFETIME(ABaseGatherableActor, CurrentMesh);
    DOREPLIFETIME_CONDITION_NOTIFY(ABaseGatherableActor, ControlRotationForwardVector, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(ABaseGatherableActor, bGatherableActorDepleted, COND_None, REPNOTIFY_Always);
}

void ABaseGatherableActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    
    // Check if GatherableItemData is valid
    if (!GatherableItemData)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: No GatherableItemData assigned!"), *GetName());
        return;
    }

    const FGatherableActorInfo& Info = GatherableItemData->GatherableActorInfo;

    // Set the initial mesh based on the data asset (e.g., tree, bush, etc.)
    if (Info.GetGatherableMesh())
    {
        ActorMesh->SetStaticMesh(Info.GetGatherableMesh());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: No initial mesh set in data asset"), *GetName());
    }
}

void ABaseGatherableActor::BeginPlay()
{
    Super::BeginPlay();
    
    // Ensure GatherableItemData is set before proceeding
    if (!GatherableItemData)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: No GatherableItemData assigned!"), *GetName());
        return;
    }

    const FGatherableActorInfo& Info = GatherableItemData->GatherableActorInfo;

    // Set the initial mesh for this gatherable actor (e.g., a tree, bush, etc.)
    if (Info.GetGatherableMesh())
    {
        ActorMesh->SetStaticMesh(Info.GetGatherableMesh());
    }

    // Initialize the health of the gatherable actor
    CurrentHealth = Info.GetMaxHealth();
}

void ABaseGatherableActor::StartGather()
{
    if (bGatherableActorDepleted) return;
    
    // If not the server, forward the call to the server to update health
    if (!HasAuthority())
    {
        ServerStartGather();
        return;
    }
    if (!GatherableItemData) return;

    const FGatherableActorInfo& Info = GatherableItemData->GatherableActorInfo;
    
    // Decrease health by the damage per hit (e.g., the player hits the resource)
    CurrentHealth = FMath::Max(CurrentHealth - Info.GetDamagePerHit(), 0);

    // Log the current health for debugging purposes
    UE_LOG(LogTemp, Log, TEXT("%s: Hit! Health=%d"), *GetName(), CurrentHealth);

    // Update the mesh based on the current health of the resource
    ChangeMeshesWhileGathering(); 
}

void ABaseGatherableActor::ServerStartGather_Implementation()
{
    // Server-side logic for gathering, to ensure server authority
    if (!GatherableItemData) return;

    const FGatherableActorInfo& Info = GatherableItemData->GatherableActorInfo;
    
    // Decrease health on the server
    CurrentHealth = FMath::Max(CurrentHealth - Info.GetDamagePerHit(), 0);

    // Log health for debugging
    UE_LOG(LogTemp, Log, TEXT("%s: Hit! Health=%d"), *GetName(), CurrentHealth);

    // Update mesh based on health
    ChangeMeshesWhileGathering();
}

void ABaseGatherableActor::PerformGatherAction_Implementation()
{
    StartGather();
}

void ABaseGatherableActor::ChangeMeshesWhileGathering()
{
    // If no data is available, exit early
    if (!GatherableItemData) return;
    const FGatherableActorInfo& Info = GatherableItemData->GatherableActorInfo;

    // Calculate health percentage based on the current health
    const int32 HealthPercentage = (CurrentHealth * 100) / Info.GetMaxHealth();
    
    // If the health reaches 0, change the mesh to the gathered (depleted) state
    if (HealthPercentage <= 0.f)
    {
        CurrentMesh = Info.GetGatheredMesh(); // Set the mesh to the depleted state (e.g., an empty bush)
        bGatherableActorDepleted = true; // Mark as depleted (no more gathering possible)
        OnGatherComplete(); // Complete the gathering process
    } 
    else
    {
        // Otherwise, update the mesh based on the health percentage
        if (HealthPercentage <= 25)
        {
            if (Info.GetGatherStageMeshes().Num() > 0)
            {
                CurrentMesh = Info.GetGatherStageMeshes()[2]; // Set mesh to stage 1
                HandlePostGather(); // Apply changes to the mesh
            }
        }
        else if (HealthPercentage <= 50)
        {
            if (Info.GetGatherStageMeshes().Num() > 1)
            {
                CurrentMesh = Info.GetGatherStageMeshes()[1]; // Set mesh to stage 2
                HandlePostGather();
            }
        }
        else if (HealthPercentage <= 75)
        {
            if (Info.GetGatherStageMeshes().Num() > 2)
            {
                CurrentMesh = Info.GetGatherStageMeshes()[0]; // Set mesh to stage 3
                HandlePostGather();
            }
        }
        else
        {
            // Default mesh for the resource if not gathered
            if (Info.GetGatherableMesh())
            {
                CurrentMesh = Info.GetGatherableMesh(); // Set to the original (full) mesh
                HandlePostGather(); // Apply changes
            }
        }
    }
}

void ABaseGatherableActor::OnInteractedByPawn_Implementation(class APawn* Pawn, const FString& interactionType)
{
    // If no data or the actor is already depleted, exit
    if (!GatherableItemData) return;
    const FGatherableActorInfo& Info = GatherableItemData->GatherableActorInfo;
    
    if (Pawn && !bGatherableActorDepleted && Info.IsPickupItem())
    {
        UACFEquipmentComponent* EquipComp = Pawn->FindComponentByClass<UACFEquipmentComponent>();
        if (EquipComp && StorageComponent)
        {
            // Spawn loot items for the player
            for (const FGatheredItem& Entry : Info.GetLootItems())
            {
                if (Entry.ResourceItem.ItemClass && Entry.ResourceItem.Count > 0)
                {
                    // Move the items to the player's inventory
                    StorageComponent->MoveItemsToInventory({ FBaseItem(Entry.ResourceItem.ItemClass, Entry.ResourceItem.Count) }, EquipComp);
                }
            }
            
            // Gather currency (if applicable) for the interaction
            StorageComponent->GatherCurrency(StorageComponent->GetCurrentCurrencyAmount(), StorageComponent->GetPawnCurrencyComponent(Pawn));
            bGatherableActorDepleted = true; // Mark the actor as depleted after gathering
        }

        // Start the timer to reset the depletion state after a delay (e.g., 5 seconds)
        StartGatherableActorDepletionTimer();
    }

    // If the resource should be destroyed after gathering, destroy it
    if (Info.ShouldDestroyAfterGather())
    {
        Destroy();
    }
}

FText ABaseGatherableActor::GetInteractableName_Implementation()
{
    if (!GatherableItemData) return FText();
    const FGatherableActorInfo& Info = GatherableItemData->GatherableActorInfo;

    if (Info.IsPickupItem())
    {
        if (Info.GetLootItems()[0].ResourceItem.ItemClass)
        {
            // Get the gathered item name for debugging or UI purposes
            return Info.GetLootItems()[0].GetGatheredItemName(); // Fetch the item name
        }
    }
    return FText();
}

void ABaseGatherableActor::OnGatherComplete()
{
    if (!GatherableItemData) return;
    const FGatherableActorInfo& Info = GatherableItemData->GatherableActorInfo;

    // If the actor has a next stage (e.g., tree to log), spawn the next stage actor
    if (Info.UsesNextStage() && Info.GetNextStageClass())
    {
        FVector SpawnLoc = GetActorLocation() + FVector(0, 0, 20); // Spawn offset
        FTransform SpawnXform(GetActorRotation(), SpawnLoc); // Spawn transform
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        ABaseGatherableActor* GatherableActor = GetWorld()->SpawnActor<ABaseGatherableActor>(Info.GetNextStageClass(), SpawnXform, SpawnParams);
    }
    else
    {
        // If no next stage, spawn loot
        SpawnGatheredLoot();
    }
    
    // Mark the health as 0 after gathering is complete
    CurrentHealth = 0;
    HandlePostGather(); // Finalize the mesh update
}

void ABaseGatherableActor::HandlePostGather()
{
    const FGatherableActorInfo& Info = GatherableItemData->GatherableActorInfo;

    // If the mesh should change, update the actor's mesh
    if (CurrentMesh != nullptr)
    {
        ActorMesh->SetStaticMesh(CurrentMesh);  // Set the new mesh
        OnRep_GatherableActorDepleted();  // Replicate the change to clients
    }

    // If the actor is depleted and should be destroyed, destroy it
    if (bGatherableActorDepleted && Info.ShouldDestroyAfterGather())
    {
        Destroy();  // Destroy the actor
    }
}

void ABaseGatherableActor::SpawnGatheredLoot()
{
    if (!GatherableItemData) return;
    const FGatherableActorInfo& Info = GatherableItemData->GatherableActorInfo;

    // Spawn loot items for the player
    for (const FGatheredItem& Entry : Info.GetLootItems())
    {
        if (Entry.ResourceItem.ItemClass && Entry.ResourceItem.Count > 0)
        {
            for (int32 i = 0; i < Entry.ResourceItem.Count; ++i)
            {
                FVector Offset(
                    FMath::FRandRange(-200.f,200.f),  // Random offset for item placement
                    FMath::FRandRange(-200.f,200.f),
                    10.f
                );

                // Spawn the loot items at a random offset from the actor's location
                UNomadItemSystemFunctionLibrary::SpawnResourceWorldItemNearLocation(
                    this,
                    { FBaseItem(Entry.ResourceItem.ItemClass, 1) },  // Spawn one item at a time
                    GetActorLocation() + Offset,  // Position the loot
                    100.f,  // Drop radius
                    Info.UsesPhysicsDrop(), // Whether the loot should use physics
                    Entry.GetPickupItemActorData()
                );
            }
        }
    }
}

bool ABaseGatherableActor::GetGatherableActorDepleted_Implementation() const
{
    return bGatherableActorDepleted;
}

void ABaseGatherableActor::OnRep_GatherableActorDepleted()
{
    if (ActorMesh && CurrentMesh)
    {
        ActorMesh->SetStaticMesh(CurrentMesh); // Update mesh on the client
    }
}

void ABaseGatherableActor::OnRep_ControlRotationForwardVector()
{
    // Replication callback to handle rotation changes
}

void ABaseGatherableActor::StartGatherableActorDepletionTimer()
{
    // Start the timer to reset the depletion state after a delay (e.g., 5 seconds)
    GetWorld()->GetTimerManager().SetTimer(ResetDepletionTimer, this, &ABaseGatherableActor::ResetGatherableState, 5.f, false);  // Reset after 5 seconds
}

void ABaseGatherableActor::ResetGatherableState()
{
    bGatherableActorDepleted = false; // Reset the depletion state to allow gathering again
    UE_LOG(LogTemp, Log, TEXT("Gatherable actor state reset"));
}

FGameplayTag ABaseGatherableActor::GetCollectionTag_Implementation() const
{
    if (!GatherableItemData) return FGameplayTag();
    const FGatherableActorInfo& Info = GatherableItemData->GatherableActorInfo;

    return Info.GetCollectTag();
}

FGameplayTag ABaseGatherableActor::GetRequiredToolTag_Implementation() const
{
    if (!GatherableItemData) return FGameplayTag();
    const FGatherableActorInfo& Info = GatherableItemData->GatherableActorInfo;

    return Info.GetRequiredToolTag();
}

void ABaseGatherableActor::GetCharacterControlRotation_Implementation(FRotator ControlRotation, FVector ForwardVector)
{
    ControlRotationForwardVector = ForwardVector;
}


