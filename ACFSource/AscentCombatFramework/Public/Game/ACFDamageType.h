// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFCCTypes.h"
#include "ACFTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include <Engine/EngineTypes.h>
#include "GameplayTagContainer.h"

#include "ACFDamageType.generated.h"

/**
 * Base Damage Type for Ascent Combat Framework (ACF).
 * 
 * UACFDamageType and its subclasses define the properties and behavior for different types of damage in ACF.
 * 
 * - Each damage type can specify gameplay tags (DamageTags) to describe its nature (e.g. "Damage.Fire", "Attack.Backstab").
 * - DamageTypes can also define scaling influences for advanced calculations.
 * - Subclasses (UMeleeDamageType, URangedDamageType, etc.) allow for further specialization and Blueprint inheritance.
 */
UCLASS()
class ASCENTCOMBATFRAMEWORK_API UACFDamageType : public UDamageType {
    GENERATED_BODY()

public:
    UACFDamageType()
    {
        StaggerMultiplier = 1.f;
    }

    /** 
     * How much this damage type contributes to the stagger of the receiver.
     * 
     * Use this to scale the impact of attacks on stagger mechanics.
     * Default is 1.0 (neutral).
     */
    UPROPERTY(EditAnywhere, Category = ACF)
    float StaggerMultiplier;

    /**
     * Gameplay tags describing this damage type.
     * 
     * Tags can be used for filtering, resistance checks, conditional modifiers, etc.
     * Examples: "Damage.Fire", "Attack.Heavy", "Element.Ice"
     * 
     * Designers can set these in the editor for each damage type asset.
     */
    UPROPERTY(EditAnywhere, Category = ACF)
    FGameplayTagContainer DamageTags;

    /**
     * Defines which offensive and defensive stats this damage type uses in calculation.
     * 
     * - Offensive influences are summed and multiplied by the scaling factor to form the base attack.
     * - Defensive influences are summed and used as a percentage reduction (capped at 100%).
     * 
     * Example:
     *   - A "Fire" damage type may be influenced by "MagicAttack" on the dealer and reduced by "FireResistance" on the target.
     *   - Designers can configure these influences in the editor.
     */
    UPROPERTY(EditAnywhere, Category = ACF)
    FDamageInfluences DamageScaling;

    UPROPERTY(EditAnywhere, Category = ACF)
    bool bSuppressHitResponse;
};

/**
 * Melee Damage Type
 * Use this class to define melee-specific damage types.
 */
UCLASS()
class ASCENTCOMBATFRAMEWORK_API UMeleeDamageType : public UACFDamageType {
    GENERATED_BODY()
};

/**
 * Ranged Damage Type
 * Use this class to define ranged-specific damage types (e.g. arrows, bullets).
 */
UCLASS()
class ASCENTCOMBATFRAMEWORK_API URangedDamageType : public UACFDamageType {
    GENERATED_BODY()
};

/**
 * Area (AoE) Damage Type
 * Use this class to define area-of-effect damage types (e.g. explosions, clouds).
 */
UCLASS()
class ASCENTCOMBATFRAMEWORK_API UAreaDamageType : public UACFDamageType {
    GENERATED_BODY()
};

/**
 * Spell Damage Type
 * Use this class to define spell-based damage types (e.g. fireball, frostbolt).
 */
UCLASS()
class ASCENTCOMBATFRAMEWORK_API USpellDamageType : public UACFDamageType {
    GENERATED_BODY()
};

/**
 * Fall Damage Type
 * Use this class to define fall/impact/environmental damage types.
 */
UCLASS()
class ASCENTCOMBATFRAMEWORK_API UFallDamageType : public UACFDamageType {
    GENERATED_BODY()
};

/**
 * FACFDamageEvent
 * 
 * Struct representing all contextual data for a single damage event in ACF.
 * 
 * This is the main data structure passed around when applying, calculating, and reacting to damage.
 * 
 * Key features:
 *  - Contains references to dealer, receiver, damage type, physical material, etc.
 *  - Holds gameplay tags (DamageTags) for both static (from DamageType) and per-hit (contextual) tags.
 *  - Used by the damage calculator, resistance checks, and other subsystems.
 */
USTRUCT(BlueprintType)
struct FACFDamageEvent {
    GENERATED_BODY()

    FACFDamageEvent()
    {
        DamageDealer = nullptr;
        DamageReceiver = nullptr;
        PhysMaterial = nullptr;
        DamageZone = EDamageZone::ENormal;
        FinalDamage = 0.f;
        HitDirection = FVector(0.f);
        DamageDirection = EACFDirection::Front;
    }

    /**
     * Optional tag describing a custom hit response action for this event.
     * 
     * Example: "Hit.Stagger", "Hit.Knockdown"
     * Used by animation or effect systems to trigger appropriate responses.
     */
    UPROPERTY(BlueprintReadWrite, Category = ACF)
    FGameplayTag HitResponseAction;

    /**
     * Optional context string for extra information (e.g. skill name, effect source).
     * 
     * Can be used for debugging, analytics, or custom logic.
     */
    UPROPERTY(BlueprintReadWrite, Category = ACF)
    FName ContextString;

    /**
     * The actor (usually character or weapon) that dealt the damage.
     */
    UPROPERTY(BlueprintReadWrite, Category = ACF)
    class AActor* DamageDealer;

    /**
     * The actor (usually character) that received the damage.
     */
    UPROPERTY(BlueprintReadWrite, Category = ACF)
    class AActor* DamageReceiver;

    /**
     * Physical material at the hit location, if any (for surface effects or armor logic).
     */
    UPROPERTY(BlueprintReadWrite, Category = ACF)
    class UPhysicalMaterial* PhysMaterial;

    /**
     * The damage zone (e.g. head, torso, limbs) hit by this event.
     * 
     * Used for location-based multipliers or effects.
     */
    UPROPERTY(BlueprintReadWrite, Category = ACF)
    EDamageZone DamageZone;

    /**
     * The final, calculated amount of damage dealt after all modifiers and reductions.
     * 
     * This is the value that will be subtracted from the receiver's health.
     */
    UPROPERTY(BlueprintReadWrite, Category = ACF)
    float FinalDamage;

    /**
     * The full hit result for this damage event.
     * 
     * Contains detailed information about the impact (location, normal, bone, etc.).
     */
    UPROPERTY(BlueprintReadWrite, Category = ACF)
    FHitResult HitResult;

    /**
     * The direction from which the hit came, as a vector.
     * 
     * Useful for knockback, stagger, and directional effects.
     */
    UPROPERTY(BlueprintReadWrite, Category = ACF)
    FVector HitDirection;

    /**
     * The damage type asset/class associated with this event.
     * 
     * This determines the base tags, influences, and calculation logic.
     */
    UPROPERTY(BlueprintReadWrite, Category = ACF)
    TSubclassOf<class UACFDamageType> DamageClass;

    /**
     * The cardinal direction (from the receiver's perspective) from which the damage came.
     * 
     * Used for front/back/side hit logic, backstab detection, etc.
     */
    UPROPERTY(BlueprintReadWrite, Category = ACF)
    EACFDirection DamageDirection;

    /**
     * True if this hit was a critical strike (e.g. extra damage, special effects).
     */
    UPROPERTY(BlueprintReadWrite, Category = ACF)
    bool bIsCritical = false;

    /**
     * The complete set of gameplay tags for this damage event.
     * 
     * - Includes tags from the DamageType asset (static)
     * - Plus any per-hit/contextual tags (added at runtime)
     * 
     * Used for bonuses, resistances, conditional effects, etc.
     * 
     * Example: "Damage.Fire", "Attack.Backstab", "Buff.Berserk"
     */
    UPROPERTY(BlueprintReadWrite, Category = ACF)
    FGameplayTagContainer DamageTags;
};