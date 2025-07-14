// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ACFStatusTypes.h"
#include "GameplayTagContainer.h"
#include "NomadStatusTypes.generated.h"

class UNomadBaseStatusEffect;

/**
 * ENomadStatusCategory
 * --------------------
 * Simple status categories for organizing effects in UI, logic, and filtering.
 */
UENUM(BlueprintType)
enum class ENomadStatusCategory : uint8
{
    /** Good effects like healing, buffs */
    Positive        UMETA(DisplayName="Positive"),
    
    /** Bad effects like poison, debuffs */
    Negative        UMETA(DisplayName="Negative"),
    
    /** Neutral or special effects */
    Neutral         UMETA(DisplayName="Neutral")
};

/**
 * FNomadStatusEffect
 * ------------------
 * Extended version of ACF's FStatusEffect with additional Nomad-specific data.
 * Holds category info, exposes convenience functions, and enables enhanced filtering.
 */
USTRUCT(BlueprintType)
struct NOMADDEV_API FNomadStatusEffect
{
    GENERATED_BODY()

public:
    FNomadStatusEffect()
    {
        ACFStatusEffect = FStatusEffect();
        Category = ENomadStatusCategory::Neutral;
    }

    FNomadStatusEffect(const FStatusEffect& InACFStatusEffect)
    {
        ACFStatusEffect = InACFStatusEffect;
        Category = ENomadStatusCategory::Neutral;
    }

    /** The original ACF status effect data (core info, tag, instance, icon, etc) */
    UPROPERTY(BlueprintReadOnly, Category="Status Effect")
    FStatusEffect ACFStatusEffect;

    /** Our simple category for this effect (positive, negative, neutral) */
    UPROPERTY(BlueprintReadOnly, Category="Status Effect")
    ENomadStatusCategory Category;

    // ======== Convenience Functions ========

    /** Get the status tag from ACF data */
    FGameplayTag GetStatusTag() const { return ACFStatusEffect.StatusTag; }

    /** Get the effect instance from ACF data */
    UACFBaseStatusEffect* GetEffectInstance() const { return ACFStatusEffect.effectInstance; }

    /** Get the status icon from ACF data */
    UTexture2D* GetStatusIcon() const { return ACFStatusEffect.StatusIcon; }

    /** Check if this status effect is valid (has both a tag and an instance) */
    bool IsValid() const { return ACFStatusEffect.StatusTag.IsValid() && ACFStatusEffect.effectInstance != nullptr; }

    // ======== Operators ========

    FORCEINLINE bool operator==(const FNomadStatusEffect& Other) const 
    { 
        return ACFStatusEffect == Other.ACFStatusEffect; 
    }

    FORCEINLINE bool operator!=(const FNomadStatusEffect& Other) const 
    { 
        return ACFStatusEffect != Other.ACFStatusEffect; 
    }

    FORCEINLINE bool operator==(const FGameplayTag& OtherTag) const 
    { 
        return ACFStatusEffect == OtherTag; 
    }

    FORCEINLINE bool operator==(const FStatusEffect& OtherACFEffect) const 
    { 
        return ACFStatusEffect == OtherACFEffect; 
    }
};

/**
 * UNomadStatusTypes
 * -----------------
 * Extension of ACF's UACFStatusTypes with Nomad helper functions for enhanced status effect handling.
 * Provides category-aware filtering, color mapping, and conversion utilities.
 */
UCLASS()
class NOMADDEV_API UNomadStatusTypes : public UACFStatusTypes
{
    GENERATED_BODY()

public:
    /** Convert ACF status effect to Nomad version (with optional category override) */
    UFUNCTION(BlueprintPure, Category="Nomad Status")
    static FNomadStatusEffect CreateNomadStatusEffect(const FStatusEffect& ACFStatusEffect, ENomadStatusCategory Category = ENomadStatusCategory::Neutral);

    /** Convert an array of ACF status effects to Nomad equivalents (all default Neutral category) */
    UFUNCTION(BlueprintPure, Category="Nomad Status")
    static TArray<FNomadStatusEffect> ConvertACFStatusEffects(const TArray<FStatusEffect>& ACFStatusEffects);

    /** Get a UI color for a status category (green=positive, red=negative, white=neutral) */
    UFUNCTION(BlueprintPure, Category="Nomad Status")
    static FLinearColor GetCategoryColor(ENomadStatusCategory Category);

    /** Filter a list of Nomad status effects by category */
    UFUNCTION(BlueprintPure, Category="Nomad Status")
    static TArray<FNomadStatusEffect> FilterByCategory(const TArray<FNomadStatusEffect>& StatusEffects, ENomadStatusCategory Category);
};