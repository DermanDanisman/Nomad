
/**
* UniversalDamageCalculator
 *
 * Purpose:
 * This class is responsible for calculating damage values based on various parameters such as tags,
 * resistances, scaling factors, and critical hit logic. It integrates seamlessly with damage type classes,
 * blueprints, and tags to provide extensible damage calculations for characters and objects.
 */

#include "Core/Game/NomadUniversalDamageCalculator.h"
#include "Game/ACFDamageType.h"
#include "ARSStatisticsComponent.h"

UNomadUniversalDamageCalculator::UNomadUniversalDamageCalculator()
{
    // Default settings can be tweaked in the editor
    CritMultiplier = 1.5f;
    DefaultRandomDamageDeviationPercentage = 5.0f;
}

float UNomadUniversalDamageCalculator::CalculateFinalDamage_Implementation(const FACFDamageEvent& InDamageEvent)
{
    // Use the standard ACF logic first (super call)
    float TotalDamage = Super::CalculateFinalDamage_Implementation(InDamageEvent);

    UACFDamageType* DamageTypeCDO = GetDamageType(InDamageEvent);

    // --- Ignore defense for specific types ---
    if (DamageTypeCDO && DamageTypesIgnoreDefense.Contains(DamageTypeCDO->GetClass()))
    {
        // Use only base damage, skip all defense modifiers (if any)
        TotalDamage = InDamageEvent.FinalDamage;
    }

    // --- Flat bonus by tag ---
    for (const FGameplayTag& Tag : InDamageEvent.DamageTags)
    {
        if (const float* Bonus = FlatBonusByDamageTag.Find(Tag))
        {
            TotalDamage += *Bonus;
        }
    }

    // Clamp to avoid negative damage
    TotalDamage = FMath::Max(0.f, TotalDamage);

    return TotalDamage;
}

bool UNomadUniversalDamageCalculator::IsCriticalDamage_Implementation(const FACFDamageEvent& InDamageEvent)
{
    UACFDamageType* DamageTypeCDO = GetDamageType(InDamageEvent);

    if (DamageTypeCDO && DamageTypesIgnoreCritical.Contains(DamageTypeCDO->GetClass()))
    {
        // Never crit for these types (e.g., starvation, poison)
        return false;
    }

    // Otherwise, use the default crit logic
    return Super::IsCriticalDamage_Implementation(InDamageEvent);
}

FGameplayTag UNomadUniversalDamageCalculator::EvaluateHitResponseAction_Implementation(const FACFDamageEvent& inDamageEvent, const TArray<FOnHitActionChances>& hitResponseActions)
{
    UACFDamageType* DamageTypeCDO = GetDamageType(inDamageEvent);

    // Option A: Using a bool property
    if (DamageTypeCDO && DamageTypeCDO->bSuppressHitResponse)
        return FGameplayTag();

    // Option B: Using tags
    if (DamageTypeCDO && DamageTypeCDO->DamageTags.HasTag(FGameplayTag::RequestGameplayTag("Damage.NoHitResponse")))
        return FGameplayTag();

    // Otherwise, use parent/default logic
    return Super::EvaluateHitResponseAction_Implementation(inDamageEvent, hitResponseActions);
}