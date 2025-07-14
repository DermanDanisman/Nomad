// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFDeveloperSettings.h"
#include "ACFTypes.h"
#include "CoreMinimal.h"
#include "Game/ACFDamageCalculation.h"
#include "GameFramework/DamageType.h"
#include "GameplayTagContainer.h"
#include "ACFDamageType.h"

#include "ACFDamageTypeCalculator.generated.h"

class UDamageType;
struct FDamageInfluence;
struct FOnHitActionChances;

/**
 * UACFDamageTypeCalculator
 * =========================================================================
 * This class implements the ACF system's main logic for damage calculation,
 * hit reaction, critical hits, and context-based damage modification.
 *
 * USAGE SCENARIO:
 * - Called by UACFDamageHandlerComponent when an entity takes damage.
 * - Receives a detailed FACFDamageEvent (who, what, where, how).
 * - Computes the final amount of damage (after all modifiers).
 * - Determines whether the hit is critical, stagger, or triggers special reactions.
 * - Returns a gameplay tag for possible hit response actions (stagger, block, heavy, etc).
 *
 * DESIGN:
 * - Highly data-driven: most behavior is configured via TMaps and gameplay tags.
 * - Supports custom scaling/multipliers for different damage types, zones, hit reactions.
 * - Integrates with RPG statistics: attack parameters, defense parameters, critical chance, etc.
 */
UCLASS()
class ASCENTCOMBATFRAMEWORK_API UACFDamageTypeCalculator : public UACFDamageCalculation {
    GENERATED_BODY()

public:
    
    UACFDamageTypeCalculator();

protected:
    /** For every hit response (stagger, knockdown, etc.), defines a multiplier to apply to final damage. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF|Hit Responses")
    TMap<FGameplayTag, float> HitResponseActionMultiplier;

    /** For each damage type, defines which attribute/statistic is used for critical chance calculation. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF| Critical Damage Config")
    TMap<TSubclassOf<UDamageType>, FDamageInfluence> CritChancePercentageByParameter;

    /** Multiplier to apply to the damage when a hit is critical. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF| Critical Damage Config")
    float CritMultiplier = 1.5f;

    /**
     * Statistic used for "stagger resistance."
     * - When set, the value is reduced by the actual damage received on each hit.
     * - The owner is only staggered when this statistic reaches zero.
     * - Allows for enemies that don't get staggered by every hit (e.g., minibosses).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF|Hit Responses")
    FGameplayTag StaggerResistanceStastistic;

    /** Tag for the action to trigger when stagger resistance is heavily reduced (i.e., "heavy hit" or knockdown). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF|Hit Responses|Heavy Hit")
    FGameplayTag HeavyHitReaction;

    /**
     * When stagger resistance falls below this negative scaling factor (percentage of max),
     * a heavy hit reaction is triggered (e.g., knockdown instead of simple stagger).
     * - Higher values make it harder to trigger heavy hits.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF|Hit Responses|Heavy Hit")
    float StaggerResistanceForHeavyHitMultiplier = 2.f;

    /** (DEPRECATED) Old way to define parameter influences per damage type. Use FDamageInfluences in UACFDamageType instead. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF")
    TMap<TSubclassOf<UDamageType>, FDamageInfluences> DamageInfluencesByParameter;

    /** Random deviation (in percent) applied to final damage for variety (default is ±5%). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF")
    float DefaultRandomDamageDeviationPercentage = 5.0f;

    /** Multiplier applied to damage depending on the zone that was hit (e.g., headshots, limbs). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF")
    TMap<EDamageZone, float> DamageZoneToDamageMultiplier;

    /** Statistic used to reduce incoming damage when blocking (defense stance). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ACF")
    FGameplayTag DefenseStanceParameterWhenBlocked;

    /** Main calculation function: computes the final damage to apply, after all influences and multipliers. */
    virtual float CalculateFinalDamage_Implementation(const FACFDamageEvent& inDamageEvent) override;

    /** Determines which hit response action should be triggered (stagger, block, heavy, etc.). */
    virtual FGameplayTag EvaluateHitResponseAction_Implementation(const FACFDamageEvent& inDamageEvent,
        const TArray<FOnHitActionChances>& hitResponseActions) override;

    /** Determines whether a hit is critical (for increased damage, VFX, etc.). */
    virtual bool IsCriticalDamage_Implementation(const FACFDamageEvent& inDamageEvent) override;

private:
    /** Helper: checks if a hit response action is valid for a given damage event. */
    bool EvaluetHitResponseAction(const FOnHitActionChances& action, const FACFDamageEvent& damageEvent);
};