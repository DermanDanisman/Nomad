// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#include "Game/ACFDamageCalculation.h"
#include "Game/ACFDamageType.h"

/**
 * Default implementation for calculating final damage.
 * 
 * This simply returns the FinalDamage value as set in the damage event.
 * - Override in your subclasses to perform additional calculations (resistance, critical, buffs, etc.).
 */
float UACFDamageCalculation::CalculateFinalDamage_Implementation(const FACFDamageEvent& inDamageEvent)
{
    return inDamageEvent.FinalDamage;
}

/**
 * Default implementation for evaluating the hit response action.
 *
 * Returns an empty tag, meaning no special action is triggered.
 * - Override in your subclasses to select from hitResponseActions array based on probability, tags, or custom logic.
 */
FGameplayTag UACFDamageCalculation::EvaluateHitResponseAction_Implementation(const FACFDamageEvent& inDamageEvent,
    const TArray<FOnHitActionChances>& hitResponseActions)
{
    return FGameplayTag();
}

/**
 * Default implementation for critical damage check.
 *
 * Always returns false (not a critical hit).
 * - Override in your subclasses to implement critical hit logic (chance, tag, location, etc.).
 */
bool UACFDamageCalculation::IsCriticalDamage_Implementation(const FACFDamageEvent& inDamageEvent)
{
    return false;
}

/**
 * Utility to get the default object for the UACFDamageType class from the damage event.
 *
 * - Returns the UACFDamageType asset associated with this damage event, if any.
 * - Can be used to access tags, scaling, and other damage type properties.
 */
UACFDamageType* UACFDamageCalculation::GetDamageType(const FACFDamageEvent& inDamageEvent)
{
    if (inDamageEvent.DamageClass)
    {
        // Get the default object for the damage class (asset), cast to UACFDamageType
        return Cast<UACFDamageType>(inDamageEvent.DamageClass->GetDefaultObject(true));
    }

    return nullptr;
}