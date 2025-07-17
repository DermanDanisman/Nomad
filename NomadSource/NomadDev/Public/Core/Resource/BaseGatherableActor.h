/// Core/Resource/BaseGatherableActor.h
// Copyright (C) Developed by Gamegine, Published by Gamegine 2025.
// All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/Data/Item/Resource/GatherableActorData.h"
#include "Core/Interface/GatherableInterface.h"
#include "Interfaces/ACFInteractableInterface.h"  // Include for interaction interface with Pawn
#include "BaseGatherableActor.generated.h"

class UACFStorageComponent; // Forward declaration of the storage component for inventory management


UCLASS()
class NOMADDEV_API ABaseGatherableActor : public AActor, public IACFInteractableInterface, public IGatherableInterface
{
    GENERATED_BODY()

public:
    // Constructor to initialize components and replication settings
    ABaseGatherableActor();

    // Define properties that need to replicate to clients
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Stores player forward direction for gathering animation alignment
    UPROPERTY(ReplicatedUsing=OnRep_ControlRotationForwardVector, BlueprintReadWrite, meta=(ExposeOnSpawn=true), Category = "Player")
    FVector ControlRotationForwardVector = FVector(1,0,0);

    // Called when ControlRotationForwardVector is replicated on clients
    UFUNCTION()
    void OnRep_ControlRotationForwardVector();

    // Interface function to return tag defining the type of gatherable
    virtual FGameplayTag GetCollectionTag_Implementation() const override;

    // Interface function to execute gather logic
    virtual void PerformGatherAction_Implementation() override;

    // Interface function to return what tool tag is required to gather this item
    virtual FGameplayTag GetRequiredToolTag_Implementation() const override;

    // Stores control rotation vector passed from the interacting player
    virtual void GetCharacterControlRotation_Implementation(FRotator ControlRotation, FVector ForwardVector) override;

protected:
    // Called when the actor is spawned or when the editor changes the actor's properties
    virtual void OnConstruction(const FTransform& Transform) override;

    // Called when the actor begins play in the game world
    virtual void BeginPlay() override;
    
    /** Root component for attaching mesh and effects to this actor */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gatherable")
    TObjectPtr<USceneComponent> DefaultSceneRoot;

    /** Mesh component used to visually represent the gatherable resource (e.g., bush, tree, etc.) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gatherable")
    TObjectPtr<UStaticMeshComponent> ActorMesh;

    /** Component for managing storage and item transfers for the actor */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Gatherable")
    TObjectPtr<UACFStorageComponent> StorageComponent;

    /** The mesh for the actor that persists after gathering */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Gatherable")
    TObjectPtr<UStaticMesh> CurrentMesh;

    /** Whether the actor has been depleted after gathering (e.g., no resources left) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_GatherableActorDepleted, Category = "Gatherable")
    bool bGatherableActorDepleted = false;

    /** Interface function to return bGatherableActorDepleted (e.g., no resources left) */
    virtual bool GetGatherableActorDepleted_Implementation() const override;

    /** Timer handle used to reset the depletion flag after a specified time delay */
    FTimerHandle ResetDepletionTimer;

    // Called when the actor's mesh changes after depletion, to synchronize across clients
    UFUNCTION()
    void OnRep_GatherableActorDepleted();

    /**
     * Holds configuration data for this gatherable actor such as meshes, health, loot items, etc.
     * Set in the editor per-instance of the actor.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gatherable")
    TObjectPtr<UGatherableActorData> GatherableItemData;

    /** The current health of the resource; it gets decremented each time it is gathered */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gatherable")
    int32 CurrentHealth;
    
    /**
     * Entry point for a gather action (e.g., player hits the resource with a tool or action).
     * This function is called locally and forwards the request to the server if needed.
     */
    UFUNCTION(BlueprintCallable, Category = "Gatherable")
    void StartGather();

    /**
     * Server-side function to handle the gathering logic, ensuring that only the server modifies health and spawns new actors.
     * This helps in preventing cheating and ensuring proper authority.
     */
    UFUNCTION(Server, Reliable, WithValidation, Category = "Gatherable")
    void ServerStartGather();
    bool ServerStartGather_Validate() { return true; }  // Always returns true, could be expanded for validation
    void ServerStartGather_Implementation();  // Implementation of the server-side logic

    /** Called when the current health of the resource falls to zero or below */
    UFUNCTION()
    void OnGatherComplete();

    /** Spawns loot items based on the current data and sends them to the player */
    void SpawnGatheredLoot();

    /** Handles resetting or updating the mesh and/or state of the actor after gathering */
    void HandlePostGather();

    /** Changes the mesh of the resource based on its current health and depletion state */
    void ChangeMeshesWhileGathering();

    /** Starts a timer to reset the depletion state after a delay */
    void StartGatherableActorDepletionTimer();

    /** Resets the depletion state of the actor, allowing it to be interacted with again */
    void ResetGatherableState();

    /**
     * This is the implementation for interaction with a pawn (e.g., the player). 
     * It’s part of the `IACFInteractableInterface` which allows the pawn to interact with this actor.
     */
    virtual void OnInteractedByPawn_Implementation(class APawn* Pawn, const FString& interactionType = "") override;

    // Returns display name used in HUD
    virtual FText GetInteractableName_Implementation() override;
};