// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Actors/ACFCharacter.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Game/ACFDamageCalculation.h"
#include "Game/ACFDamageType.h"
#include "Game/ACFTypes.h"
#include "GameFramework/DamageType.h"
#include "ACFCoreTypes.h"

#include "ACFDamageHandlerComponent.generated.h"

// Forward declaration of damage event struct
struct FACFDamageEvent;
class UACFDamageCalculation;

// Delegate for broadcasting owner death event
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterDeath);

/**
 * UACFDamageHandlerComponent
 * 
 * Component responsible for handling all aspects of damage application, resistance, hit responses, and character death for ACF characters.
 * 
 * - Handles incoming damage, calculates final damage using a customizable damage calculator, and applies stat modifications.
 * - Supports team-based collision channel assignment for damage meshes.
 * - Broadcasts events for damage received, team changes, and character death.
 * - Can be extended with custom hit response logic and damage calculators.
 */
UCLASS(Blueprintable, ClassGroup = (ACF), meta = (BlueprintSpawnableComponent))
class ASCENTCOMBATFRAMEWORK_API UACFDamageHandlerComponent : public UActorComponent {
    GENERATED_BODY()

public:
    // Constructor: Sets default values for this component's properties
    UACFDamageHandlerComponent();

    /** Returns the last damage event information received by this component. */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE FACFDamageEvent GetLastDamageInfo() const
    {
        return LastDamageReceived;
    }

    /**
     * Assigns the correct collision channel to the character's damage meshes based on the specified team.
     * 
     * @param combatTeam - The team to assign collision channels for (e.g. Player, Enemy, Neutral).
     */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void InitializeDamageCollisions(ETeam combatTeam);

    /** Event called whenever this character receives damage. */
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnDamageReceived OnDamageReceived;

    /** Event called when this character's team changes. */
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnTeamChanged OnTeamChanged;

    /** Event broadcast when the character dies (health reaches zero). */
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnCharacterDeath OnOwnerDeath;

    /**
     * Handles the full flow of taking damage for this character.
     * 
     * @param damageReceiver - The actor receiving damage (usually the character owning this component).
     * @param Damage - The base amount of damage to apply.
     * @param DamageEvent - The Unreal Engine FDamageEvent containing hit info, damage type, etc.
     * @param EventInstigator - Controller responsible for the damage.
     * @param DamageCauser - The actor causing the damage (e.g., weapon, ability).
     * @return The final damage value actually applied after all calculations.
     */
    UFUNCTION(BlueprintCallable, Category = ACF)
    float TakeDamage(
        class AActor* damageReceiver, float Damage, const FDamageEvent& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser);

    /** Returns whether the character is currently alive (health > 0). */
    UFUNCTION(BlueprintPure, Category = ACF)
    bool GetIsAlive() const
    {
        return bIsAlive;
    }

    /**
     * Revives the character (sets alive state and starts stat regeneration).
     * Must be called on the server.
     */
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = ACF)
    void Revive();

protected:
    // Called when the game starts.
    virtual void BeginPlay() override;

    /** If true, will use a blocking collision channel when assigning collision profiles. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    bool bUseBlockingCollisionChannel = false;

    /** The class to use for damage calculation (must derive from UACFDamageCalculation). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    TSubclassOf<UACFDamageCalculation> DamageCalculatorClass;

    /** Array of hit response actions and their conditions, used to define automatic reactions to being hit (e.g., dodge, parry, counterattack, play animation). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ACF)
    TArray<FOnHitActionChances> HitResponseActions;

private:
    /**
     * Constructs the FACFDamageEvent struct with all relevant data for a damage event.
     * Populates hit info, direction, damage tags, and calculates the final damage using the DamageCalculator.
     */
    void ConstructDamageReceived(AActor* DamagedActor, float Damage, class AController* InstigatedBy, FVector HitLocation,
        class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, TSubclassOf<UDamageType> DamageType, AActor* DamageCauser);

    /** Called on all clients when this character receives damage. */
    UFUNCTION(NetMulticast, Reliable, Category = ACF)
    void ClientsReceiveDamage(const FACFDamageEvent& damageEvent);

    /** Instance of the damage calculator used to evaluate hit responses and final damage. */
    UPROPERTY()
    class UACFDamageCalculation* DamageCalculator;

    /** The last damage event data received and processed by this component. */
    UPROPERTY()
    FACFDamageEvent LastDamageReceived;

    /** Handles stat reaching zero (e.g., health depletion causing death). */
    UFUNCTION()
    void HandleStatReachedZero(FGameplayTag stat);

    /** Assigns the provided collision channel to all mesh components on the owning actor. */
    void AssignCollisionProfile(const TEnumAsByte<ECollisionChannel> channel);

    /** True if the character is alive (health > 0), replicated for multiplayer consistency. */
    UPROPERTY(Savegame, Replicated)
    bool bIsAlive = true;

    /** Internal flag to track component initialization. */
    bool bInit = false;

    /** The team this character belongs to, replicated for multiplayer. */
    UPROPERTY(Replicated)
    ETeam combatTeam;
};