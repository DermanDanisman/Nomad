// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#pragma once

#include "ACMTypes.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include <Engine/EngineTypes.h>
#include <GameFramework/DamageType.h>
#include "Components/SphereComponent.h"
#include "GameplayTagContainer.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ACMCollisionManagerComponent.generated.h"

class AActor;

/**
 * Delegate broadcast when a collision is detected.
 * @param HitResult - Details of the hit, including impacted actor, location, etc.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCollisionDetected, const FHitResult&, HitResult);

/**
 * Delegate broadcast when an actor is damaged by this component.
 * @param damageReceiver - Actor that was damaged.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorDamaged, AActor*, damageReceiver);

/**
 * UACMCollisionManagerComponent
 *
 * Centralized component for handling all collision-based damage logic for weapons, projectiles, or area effects.
 * 
 * Major Features:
 * - Manages traces (swipes, areas, points) and maintains active/inactive states.
 * - Handles collision channels, ignores, debug drawing, and per-trace configuration.
 * - Applies both point and area damage, including all Unreal damage event integration.
 * - Supports both replicated (server-driven) and local traces for flexibility.
 * - Broadcasts events for collision and damage for game logic or VFX/SFX.
 * 
 * Usage:
 * - Attach to a weapon, damage actor, or character.
 * - Configure traces, collision channels, and ignored actors.
 * - Use Start/Stop methods to control traces or area damage.
 * - Listen to delegates for collision and damage events.
 */
UCLASS(Blueprintable, ClassGroup = (ACF), meta = (BlueprintSpawnableComponent))
class COLLISIONSMANAGER_API UACMCollisionManagerComponent : public UActorComponent {

    GENERATED_BODY()

public:
    /** Default constructor: initializes defaults. */
    UACMCollisionManagerComponent();

    /** Allow/disallow multiple hits per swing (if false, same actor not hit again until next trace start). */
    UFUNCTION(BlueprintCallable, Category = "ACM")
    void SetAllowMultipleHitsPerSwing(const bool bInAllowMultipleHitsPerSwing)
    {
        bAllowMultipleHitsPerSwing = bInAllowMultipleHitsPerSwing;
    }

    /** Set the collision channels used for traces (e.g., Pawn, WorldDynamic, etc). */
    UFUNCTION(BlueprintCallable, Category = "ACM")
    void SetCollisionChannels(const TArray<TEnumAsByte<ECollisionChannel>>& InCollisionChannels)
    {
        CollisionChannels = InCollisionChannels;
    }

    /** Set the list of actors to ignore in all trace checks. */
    UFUNCTION(BlueprintCallable, Category = "ACM")
    void SetIgnoredActors(const TArray<class AActor*>& InIgnoredActors)
    {
        IgnoredActors = InIgnoredActors;
    }

    /** Set whether to ignore this component's owner (useful for weapons, projectiles, etc). */
    UFUNCTION(BlueprintCallable, Category = "ACM")
    void SetIgnoreOwner(const bool bInIgnoreOwner)
    {
        bIgnoreOwner = bInIgnoreOwner;
    }

    /** Set the trace configurations for all named traces. */
    UFUNCTION(BlueprintCallable, Category = "ACM")
    void SetDamageTraces(const TMap<FName, FTraceInfo>& InDamageTraces)
    {
        DamageTraces = InDamageTraces;
    }

    /** Set the configuration for swipe traces (e.g., sword swings). */
    UFUNCTION(BlueprintCallable, Category = "ACM")
    void SetSwipeTraceInfo(const FBaseTraceInfo& InSwipeTraceInfo)
    {
        SwipeTraceInfo = InSwipeTraceInfo;
    }

    /** Set the configuration for area damage traces (e.g., explosions). */
    UFUNCTION(BlueprintCallable, Category = "ACM")
    void SetAreaDamageTraceInfo(const FBaseTraceInfo& InAreaDamageTraceInfo)
    {
        AreaDamageTraceInfo = InAreaDamageTraceInfo;
    }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Debug drawing type (always show, only during swing, or none). */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "ACM| Debug")
    EDebugType ShowDebugInfo;

    /** Color for debug traces when inactive. */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "ACM| Debug")
    FLinearColor DebugInactiveColor;

    /** Color for debug traces when active. */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "ACM| Debug")
    FLinearColor DebugActiveColor;

    /** If true, allows multiple hits per swing on the same actor. */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "ACM")
    bool bAllowMultipleHitsPerSwing;

    /** Collision channels used for traces. */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "ACM")
    TArray<TEnumAsByte<ECollisionChannel>> CollisionChannels;

    /** Actors to ignore in all traces. */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "ACM")
    TArray<class AActor*> IgnoredActors;

    /** If true, ignores the component's owner in trace checks. */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "ACM")
    bool bIgnoreOwner = true;

    /** All damage trace configurations (by name). */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "ACM|Traces")
    TMap<FName, FTraceInfo> DamageTraces;

    /** Swipe trace configuration (for broad melee attacks). */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "ACM|Traces")
    FBaseTraceInfo SwipeTraceInfo;

    /** Area damage trace configuration (for AOE effects). */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "ACM|Traces")
    FBaseTraceInfo AreaDamageTraceInfo;

public:
    /** Sets up the collision manager, binding to a mesh for sockets, etc. */
    UFUNCTION(BlueprintCallable, Category = ACM)
    void SetupCollisionManager(class UMeshComponent* inDamageMesh);

    /** Starts repeated area damage at a center with given radius and interval (server only). */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACM)
    void StartAreaDamage(const FVector& damageCenter, float damageRadius, float damageInterval = 1.f);

    /** Stops any ongoing area damage. */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACM)
    void StopCurrentAreaDamage();

    /** Applies area damage once at a location/radius (server only). */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACM)
    void PerformAreaDamage_Single(const FVector& damageCenter, float damageRadius);

    /** Local-only: applies area damage once and outputs hit results. */
    UFUNCTION(BlueprintCallable, Category = ACM)
    void PerformAreaDamage_Single_Local(const FVector& damageCenter, float damageRadius, TArray<FHitResult>& outHits);

    /** Starts repeated area damage for a given duration (server only). */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACM)
    void PerformAreaDamageForDuration(const FVector& damageCenter, float damageRadius, float duration, float damageInterval = 1.f);

    /** Adds an actor to the ignore list. */
    UFUNCTION(BlueprintCallable, Category = ACM)
    void AddActorToIgnore(class AActor* ignoredActor);

    /** Adds a collision channel for traces. */
    UFUNCTION(BlueprintCallable, Category = ACM)
    void AddCollisionChannel(TEnumAsByte<ECollisionChannel> inTraceChannel);

    /** Adds multiple collision channels for traces. */
    UFUNCTION(BlueprintCallable, Category = ACM)
    void AddCollisionChannels(TArray<TEnumAsByte<ECollisionChannel>> inTraceChannels);

    /** Clears all collision channels. */
    UFUNCTION(BlueprintCallable, Category = ACM)
    void ClearCollisionChannels();

    /** Performs a swipe trace between two points (server only). */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACM)
    void PerformSwipeTraceShot(const FVector& start, const FVector& end, float radius = 0.f);

    /** Local-only: performs a swipe trace between two points, outputs hit result. */
    UFUNCTION(BlueprintCallable, Category = ACM)
    void PerformSwipeTraceShot_Local(const FVector& start, const FVector& end, float radius, FHitResult& outHit);

    /** Starts all configured traces (server only). */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACM)
    void StartAllTraces();

    /** Stops all active traces (server only). */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACM)
    void StopAllTraces();

    /** Starts a single named trace (server only). */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACM)
    void StartSingleTrace(const FName& Name);

    /** Stops a single named trace (server only). */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACM)
    void StopSingleTrace(const FName& Name);

    /** Starts a single trace for a fixed duration (server only). */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACM)
    void StartTimedSingleTrace(const FName& TraceName, float Duration);

    /** Starts all traces for a fixed duration (server only). */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACM)
    void StartAllTimedTraces(float Duration);

    /** Multicast: start visual trails for a trace (clients). */
    UFUNCTION(NetMulticast, Reliable, Category = ACM)
    void PlayTrails(const FName& trail);

    /** Multicast: stop visual trails for a trace (clients). */
    UFUNCTION(NetMulticast, Reliable, Category = ACM)
    void StopTrails(const FName& trail);

    /** Returns the current damage trace configuration map. */
    UFUNCTION(BlueprintCallable, Category = ACM)
    TMap<FName, FTraceInfo> GetDamageTraces() const
    {
        return DamageTraces;
    };

    /** Returns true if a trace of the given name is currently active. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = ACM)
    bool IsTraceActive(const FName& traceName);

    /**
     * Sets the actor considered as the "owner" for collision (for damage attribution, ignore logic, etc).
     * Useful if the damage-dealer is not the component's owner (e.g., weapon vs. wielder).
     */
    UFUNCTION(BlueprintCallable, Category = ACM)
    void SetActorOwner(AActor* newOwner);

    /** Gets the actor considered as the "owner" for collision/damage. */
    UFUNCTION(BlueprintPure, Category = ACM)
    AActor* GetActorOwner() const;

    /** Sets the configuration for a trace by name. */
    UFUNCTION(BlueprintCallable, Category = ACM)
    void SetTraceConfig(const FName& traceName, const FTraceInfo& traceInfo);

    /** Tries to get the trace config for a given name. Returns true if found. */
    UFUNCTION(BlueprintCallable, Category = ACM)
    bool TryGetTraceConfig(const FName& traceName, FTraceInfo& outTraceInfo) const
    {
        if (DamageTraces.Contains(traceName))
        {
            outTraceInfo = *DamageTraces.Find(traceName);
            return true;
        }
        return false;
    };

    /** Delegate: collision detected (broadcasts every frame a collision hits). */
    UPROPERTY(BlueprintAssignable, Category = ACM)
    FOnCollisionDetected OnCollisionDetected;

    /** Delegate: actor was damaged by this component. */
    UPROPERTY(BlueprintAssignable, Category = ACM)
    FOnActorDamaged OnActorDamaged;

    /** Helper: gets the rotation from start to end point (for sweeps, traces, etc). */
    FRotator GetLineRotation(FVector start, FVector end);

    /** Updates all traces, applies hits/damage, and handles debug drawing. */
    void UpdateCollisions();

    /** Gets the first trace config in the map (useful for default logic). */
    FTraceInfo GetFirstTrace() const;

private:
    /** Actor considered as "owner" for damage/ignore logic. */
    TObjectPtr<AActor> actorOwner;

    /** The mesh used for sockets and trace references. */
    TObjectPtr<UMeshComponent> damageMesh;

    /** Map of currently activated traces (by name). */
    UPROPERTY()
    TMap<FName, FTraceInfo> activatedTraces;

    /** Traces that are pending removal after this frame. */
    UPROPERTY()
    TArray<FName> pendingDelete;

    /** For each trace, stores actors already hit (prevents multiple hits if disabled). */
    UPROPERTY()
    TMap<FName, FHitActors> alreadyHitActors;

    /** Actors already hit by current area damage (prevents repeated hits per area "pulse"). */
    TArray<TObjectPtr<AActor>> alreadyHitActorsBySphere;

    /** Actors already hit by current swipe trace (prevents repeated hits per swing). */
    TArray<TObjectPtr<AActor>> alreadyHitActorsBySweep;

    /** Internal: if the system is currently running traces. */
    bool bIsStarted = false;

    /** Handles debug drawing for all traces, based on ShowDebugInfo. */
    void DisplayDebugTraces();

    /** Draws a single debug trace as a cylinder. */
    void ShowDebugTrace(const FVector& StartPos, const FVector& EndPos, const float radius, EDrawDebugTrace::Type DrawDebugType, float duration, FLinearColor DebugColor = FLinearColor::Red);

    /** Handler: called when a timed single trace finishes. */
    UFUNCTION()
    void HandleTimedSingleTraceFinished(const FName& traceEnded);

    /** Handler: called when all timed traces finish. */
    UFUNCTION()
    void HandleAllTimedTraceFinished();

    /** Map of spawned trail particle system components, by trace name. */
    UPROPERTY()
    TMap<FName, class UParticleSystemComponent*> ParticleSystemComponents;

    /** Map of spawned Niagara system components, by trace name. */
    UPROPERTY()
    TMap<FName, class UNiagaraComponent*> NiagaraSystemComponents;

    /** Applies damage to a hit result using the current trace configuration. */
    void ApplyDamage(const FHitResult& HitResult, const FBaseTraceInfo& currentTrace);

    /** Applies point damage (single target, e.g., sword poke). */
    void ApplyPointDamage(const FHitResult& HitResult, const FBaseTraceInfo& currentTrace);

    /** Applies area damage (AOE, e.g., explosion pulse). */
    void ApplyAreaDamage(const FHitResult& HitResult, const FBaseTraceInfo& currentTrace);

    /** Timer for all traces running at once. */
    UPROPERTY()
    FTimerHandle AllTraceTimer;

    /** Timer for area damage pulse. */
    UPROPERTY()
    FTimerHandle AreaDamageTimer;

    /** Timer for area damage looping (if repeatedly pulsing AOE damage). */
    UPROPERTY()
    FTimerHandle AreaDamageLoopTimer;

    /** Handler: called when area damage finishes its duration. */
    UFUNCTION()
    void HandleAreaDamageFinished();

    /** Handler: called for each pulse of area damage. */
    UFUNCTION()
    void HandleAreaDamageLooping();

    /** Info for the currently active area damage (location, radius, etc). */
    UPROPERTY()
    FAreaDamageInfo currentAreaDamage;

    /** Map of timers for individual timed traces, by name. */
    UPROPERTY()
    TMap<FName, FTimerHandle> TraceTimers;

    /** Internal: tracks if a single timed trace is active. */
    UPROPERTY()
    bool bSingleTimedTraceStarted = false;

    /** Internal: tracks if all timed traces are active. */
    UPROPERTY()
    bool bAllTimedTraceStarted = false;

    /** Sets the started state for the system (activates/deactivates debug, etc). */
    void SetStarted(bool inStarted);
};