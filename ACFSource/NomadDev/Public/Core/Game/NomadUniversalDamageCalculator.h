// Copyright (C) 2025 Your Studio or Company. All Rights Reserved.

#pragma once

#include "Game/ACFDamageTypeCalculator.h"
#include "GameplayTagContainer.h"
#include "NomadUniversalDamageCalculator.generated.h"

/**
 * UNomadUniversalDamageCalculator
 * =================================================================
 * - A ready-to-use universal ACF damage calculation class for most games.
 * - Handles melee, ranged, spell, survival, affliction, environmental, etc.
 * - Highly configurable: tweak multipliers/tags in the editor.
 * - Add your own logic by extending this class or overriding functions.
 * - Remove or add any UPROPERTYs as needed for your project.
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class NOMADDEV_API UNomadUniversalDamageCalculator : public UACFDamageTypeCalculator
{
    GENERATED_BODY()

public:
    UNomadUniversalDamageCalculator();

    // --- Example custom variables (add your own as needed) ---

    /** Damage types that should ignore crits (e.g., starvation, poison, environmental). */
    UPROPERTY(EditDefaultsOnly, Category="Survival|Affliction")
    TArray<TSubclassOf<UACFDamageType>> DamageTypesIgnoreCritical;

    /** Damage types that ignore defense stats (e.g., true damage, starvation, etc.) */
    UPROPERTY(EditDefaultsOnly, Category="Survival|Affliction")
    TArray<TSubclassOf<UACFDamageType>> DamageTypesIgnoreDefense;

    /** Flat bonus damage for specific tags (e.g., bonus for magic, bleed, burn, etc.) */
    UPROPERTY(EditDefaultsOnly, Category="Custom")
    TMap<FGameplayTag, float> FlatBonusByDamageTag;

protected:
    // --- Override the main calculation to add project/game-specific logic ---
    virtual float CalculateFinalDamage_Implementation(const FACFDamageEvent& InDamageEvent) override;

    // --- Optionally override critical hit logic ---
    virtual bool IsCriticalDamage_Implementation(const FACFDamageEvent& InDamageEvent) override;

    virtual FGameplayTag EvaluateHitResponseAction_Implementation(const FACFDamageEvent& inDamageEvent, const TArray<FOnHitActionChances>& hitResponseActions) override;
};