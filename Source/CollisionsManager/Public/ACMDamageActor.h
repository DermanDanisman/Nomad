// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACMCollisionManagerComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "ACMDamageActor.generated.h"

/**
 * AACMDamageActor
 *
 * This actor represents a damage-causing entity in the world, such as a projectile, trap, or melee hitbox.
 * It is designed for modularity and reusability: it owns a collision manager component, mesh, and movement component,
 * and provides a unified interface to start/stop traces and area damage.
 *
 * Key Features:
 * - Exposes collision, mesh, and movement components to Blueprint and C++.
 * - Can be set up with an owning actor for damage attribution and collision ignore logic.
 * - Handles both direct traces (e.g., sword swings) and area-of-effect damage (e.g., explosions).
 * - Broadcasts events when actors are damaged, allowing for customizable gameplay reactions.
 * - Designed for use in both singleplayer and multiplayer (replicated).
 *
 * Typical Usage:
 * - Spawn this actor for a projectile, explosion, or temporary melee hitbox.
 * - Call SetupCollisions() with the damaging actor as owner.
 * - Use StartDamageTraces/StopDamageTraces for melee/projectile traces.
 * - Use StartAreaDamage/StopAreaDamage for area-of-effect attacks.
 * - Bind to OnActorDamaged to implement hit reactions, effects, etc.
 */
UCLASS(BlueprintType, Blueprintable, Category = ACM)
class COLLISIONSMANAGER_API AACMDamageActor : public AActor {
    GENERATED_BODY()

public:
    /** Default constructor: sets up components and replication. */
    AACMDamageActor();

    /** Event: Called when this Damage Actor damages another actor. */
    UPROPERTY(BlueprintAssignable, Category = ACM)
    FOnActorDamaged OnActorDamaged;

    /**
     * Initializes the collision manager with an owner and sets up mesh collision.
     * @param inOwner The actor who "owns" this damage actor (for ignore logic, attribution, etc).
     * Typically called right after spawning.
     */
    UFUNCTION(BlueprintCallable, Category = ACM)
    void SetupCollisions(AActor* inOwner);

    /** Returns the actor that owns this damage actor (for damage attribution, collision ignore, etc). */
    UFUNCTION(BlueprintPure, Category = ACM)
    AActor* GetActorOwner() const
    {
        return ActorOwner;
    }

    /** Starts all collision traces (useful for melee swings, projectiles, etc). */
    UFUNCTION(BlueprintCallable, Category = ACM)
    void StartDamageTraces();

    /** Stops all collision traces. */
    UFUNCTION(BlueprintCallable, Category = ACM)
    void StopDamageTraces();

    /**
     * Begins area-of-effect damage at this actor's location.
     * @param radius The radius of the area in world units.
     * @param damageInterval How often damage is applied in seconds.
     */
    UFUNCTION(BlueprintCallable, Category = ACM)
    void StartAreaDamage(float radius = 100.f, float damageInterval = 1.5f);

    /** Stops any ongoing area-of-effect damage. */
    UFUNCTION(BlueprintCallable, Category = ACM)
    void StopAreaDamage();

    /** Returns the collision manager component. */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE class UACMCollisionManagerComponent* GetCollisionsComponent() const
    {
        return CollisionComp;
    }

    /** Returns the movement component (for projectiles, moving traps, etc). */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE class UInterpToMovementComponent* GetMovementComponent() const
    {
        return MovementComp;
    }

    /** Returns the mesh component (for visual representation). */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE class UMeshComponent* GetMesh() const
    {
        return MeshComp;
    }

protected:
    /**
     * Called after collisions are set up with a new owner.
     * Override in Blueprint or C++ to customize post-setup behavior.
     */
    UFUNCTION(BlueprintNativeEvent, Category = ACM)
    void OnSetup(AActor* new0wner);
    virtual void OnSetup_Implementation(AActor* new0wner);

    /** Called when the game starts or this actor is spawned. */
    virtual void BeginPlay() override;

    /** Called when the actor is destroyed or removed from the world. */
    virtual void EndPlay(EEndPlayReason::Type end) override;

    /** The mesh component (can be a static mesh, skeletal mesh, etc). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, DisplayName = "Mesh Component", Category = ACF)
    TObjectPtr<class UMeshComponent> MeshComp;

    /** The collision manager component (handles traces, area checks, etc). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, DisplayName = "ACF Collisions ManagerComp", Category = ACF)
    TObjectPtr<class UACMCollisionManagerComponent> CollisionComp;

    /** The movement component (handles interpolation/movement for projectiles, etc). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, DisplayName = "Movement Comp", Category = ACF)
    TObjectPtr<class UInterpToMovementComponent> MovementComp;

    /**
     * The actor who owns this damage actor (e.g., the player or enemy that "fired" it).
     * Used for damage attribution, collision ignore, and gameplay events.
     * Exposed on spawn for easy setting at creation.
     */
    UPROPERTY(BlueprintReadWrite, Category = ACM, meta = (ExposeOnSpawn = true))
    AActor* ActorOwner;

private:
    /** Internal handler for when an actor is damaged by this damage actor. Broadcasts OnActorDamaged. */
    UFUNCTION()
    void HandleDamagedActor(AActor* damagedActor);
};