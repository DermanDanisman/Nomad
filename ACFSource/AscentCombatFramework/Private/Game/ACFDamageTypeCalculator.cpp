// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "Game/ACFDamageTypeCalculator.h"
#include "ARSStatisticsComponent.h"
#include "Actors/ACFCharacter.h"
#include "Components/ACFDefenseStanceComponent.h"
#include "Game/ACFDamageType.h"
#include "Game/ACFFunctionLibrary.h"
#include "Game/ACFTypes.h"

/**
 * Implementation of UACFDamageTypeCalculator methods.
 * See the header documentation for a full description of all features and 
 * intended usage.
 */

UACFDamageTypeCalculator::UACFDamageTypeCalculator() {}

bool UACFDamageTypeCalculator::IsCriticalDamage_Implementation(const FACFDamageEvent& inDamageEvent)
{
    // Check if the dealer exists and if there is a crit config for this damage type.
    if (inDamageEvent.DamageDealer)
    {
        const FDamageInfluence* critChance = CritChancePercentageByParameter.Find(inDamageEvent.DamageClass);
        const UARSStatisticsComponent* dealerComp = inDamageEvent.DamageDealer->FindComponentByClass<UARSStatisticsComponent>();

        // If config found, get the dealer's attribute value and calculate crit %
        if (critChance && dealerComp)
        {
            const float percentage = dealerComp->GetCurrentAttributeValue(critChance->Parameter) * critChance->ScalingFactor;
            if (FMath::RandRange(0.f, 100.f) < percentage)
            {
                return true;
            }
        }
    }
    return false;
}

float UACFDamageTypeCalculator::CalculateFinalDamage_Implementation(const FACFDamageEvent& inDamageEvent)
{
    // Defensive: Ensure all required actors and types are present.
    if (!inDamageEvent.DamageReceiver)
    {
        UE_LOG(LogTemp, Error, TEXT("Missing Damage receiver!!! - UACFDamageCalculation::CalculateFinalDamage"));
        return inDamageEvent.FinalDamage;
    }

    if (!inDamageEvent.DamageDealer)
    {
        UE_LOG(LogTemp, Error, TEXT("Missing Damage Dealer!!! - UACFDamageCalculation::CalculateFinalDamage"));
        return inDamageEvent.FinalDamage;
    }

    if (!inDamageEvent.DamageClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Missing Damage Class!!!! - UACFDamageCalculation::CalculateFinalDamage"));
        return inDamageEvent.FinalDamage;
    }
    UACFDamageType* damageClass = GetDamageType(inDamageEvent);

    if (!damageClass)
    {
        UE_LOG(LogTemp, Error, TEXT("DamageClass Influence NOT Set!!!! - UACFDamageCalculation::CalculateFinalDamage"));
        return inDamageEvent.FinalDamage;
    }
    FDamageInfluences damagesInf = damageClass->DamageScaling;

    // Start with the base (raw) damage provided in the event.
    float totalDamage = inDamageEvent.FinalDamage;

    const UARSStatisticsComponent* dealerComp = inDamageEvent.DamageDealer->FindComponentByClass<UARSStatisticsComponent>();
    UARSStatisticsComponent* receiverComp = inDamageEvent.DamageReceiver->FindComponentByClass<UARSStatisticsComponent>();

    // STEP 1: Apply all attacker parameter influences (e.g., Strength, WeaponPower).
    for (const auto& damInf : damagesInf.AttackParametersInfluence)
    {
        totalDamage += dealerComp->GetCurrentAttributeValue(damInf.Parameter) * damInf.ScalingFactor;
    }

    // STEP 2: Apply all defender parameter reductions (e.g., Armor, Resistance).
    for (const auto& damInf : damagesInf.DefenseParametersPercentages)
    {
        totalDamage = UACFFunctionLibrary::ReduceDamageByPercentage(totalDamage,
            receiverComp->GetCurrentAttributeValue(damInf.Parameter) * damInf.ScalingFactor);
    }

    // STEP 3: Apply critical hit multiplier if this hit is critical.
    if (inDamageEvent.bIsCritical)
    {
        totalDamage *= CritMultiplier;
    }

    // STEP 4: Apply random deviation for "natural" damage variation.
    if (totalDamage != 0.f)
    {
        const float deviation = totalDamage * DefaultRandomDamageDeviationPercentage / 100;
        totalDamage = FMath::FRandRange(totalDamage - deviation, totalDamage + deviation);
    }

    // STEP 5: Check for defense stance (blocking). If blocking, reduce further.
    UACFDefenseStanceComponent* defComp = inDamageEvent.DamageReceiver->FindComponentByClass<UACFDefenseStanceComponent>();
    FGameplayTag outResponse;

    if (defComp && defComp->IsInDefensePosition() && defComp->TryBlockIncomingDamage(inDamageEvent, totalDamage, outResponse))
    {
        const float reducedPercentage = receiverComp->GetCurrentAttributeValue(DefenseStanceParameterWhenBlocked);
        totalDamage = UACFFunctionLibrary::ReduceDamageByPercentage(totalDamage, reducedPercentage);
    } else
    {
        // STEP 6: Apply zone-based multipliers (e.g., headshot, limb shot).
        const float* zoneMult = DamageZoneToDamageMultiplier.Find(inDamageEvent.DamageZone);
        if (zoneMult)
        {
            totalDamage *= *zoneMult;
        }

        // STEP 7: Apply hit response multipliers (e.g., stagger, knockdown).
        const float* hitMult = HitResponseActionMultiplier.Find(inDamageEvent.HitResponseAction);
        if (hitMult)
        {
            totalDamage *= *hitMult;
        }
    }

    return totalDamage;
}

FGameplayTag UACFDamageTypeCalculator::EvaluateHitResponseAction_Implementation(const FACFDamageEvent& damageEvent, const TArray<FOnHitActionChances>& hitResponseActions)
{
    UACFDefenseStanceComponent* defComp = damageEvent.DamageReceiver->FindComponentByClass<UACFDefenseStanceComponent>();
    FGameplayTag outResponse;

    if (!damageEvent.DamageDealer)
    {
        return FGameplayTag();
    }

    // STEP 1: If defender is blocking and blocks this damage, return block action.
    if (defComp && defComp->IsInDefensePosition() && defComp->CanBlockDamage(damageEvent))
    {
        return defComp->GetBlockAction();
    }

    // STEP 2: If defender counterattacks, return counter action.
    if (defComp && defComp->TryCounterAttack(damageEvent, outResponse))
    {
        return outResponse;
    }

    // STEP 3: Evaluate all hit response actions in order.
    const AACFCharacter* charOwn = Cast<AACFCharacter>(damageEvent.DamageReceiver);
    if (charOwn)
    {
        for (const auto& elem : hitResponseActions)
        {
            if (UACFFunctionLibrary::ShouldExecuteAction(elem, charOwn) && EvaluetHitResponseAction(elem, damageEvent))
            {
                outResponse = elem.ActionTag;
                break;
            }
        }
    }

    // STEP 4: Handle stagger resistance and heavy hit logic.
    UARSStatisticsComponent* receiverComp = damageEvent.DamageReceiver->FindComponentByClass<UARSStatisticsComponent>();
    UACFDamageType* DamageType = GetDamageType(damageEvent);
    if (receiverComp && DamageType && StaggerResistanceStastistic != FGameplayTag() && outResponse == UACFFunctionLibrary::GetDefaultHitState())
    {
        const float FinalDamgeTemp = CalculateFinalDamage(damageEvent) * DamageType->StaggerMultiplier;
        receiverComp->ModifyStatistic(StaggerResistanceStastistic, -FinalDamgeTemp);
        if (receiverComp->GetCurrentValueForStatitstic(StaggerResistanceStastistic) > 1.f)
        {
            return FGameplayTag();
        }
    }

    // STEP 5: If stagger resistance is depleted, trigger heavy hit reaction.
    if (receiverComp && StaggerResistanceStastistic != FGameplayTag() && HeavyHitReaction != FGameplayTag())
    {
        const float CurrentResistance = receiverComp->GetCurrentValueForStatitstic(StaggerResistanceStastistic);
        const float HeavyHitThreshold = -StaggerResistanceForHeavyHitMultiplier * receiverComp->GetMaxValueForStatitstic(StaggerResistanceStastistic);
        if (CurrentResistance < HeavyHitThreshold)
        {
            outResponse = HeavyHitReaction;
        }
    }

    return outResponse;
}

bool UACFDamageTypeCalculator::EvaluetHitResponseAction(const FOnHitActionChances& action, const FACFDamageEvent& damageEvent)
{
    // Direction check: only allow actions from allowed directions (or any).
    if (static_cast<uint8>(damageEvent.DamageDirection) != static_cast<uint8>(action.AllowedFromDirection) && action.AllowedFromDirection != EActionDirection::EveryDirection)
    {
        return false;
    }

    // Damage type check: only allow actions for the correct damage types.
    for (const TSubclassOf<UDamageType>& damageType : action.AllowedDamageTypes)
    {
        if (damageEvent.DamageClass->IsChildOf(damageType))
        {
            return true;
        }
    }
    return false;
}