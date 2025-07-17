// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "NomadSurvivalHazardEffectConfig.generated.h"
class UNomadSurvivalStatusEffect;

/**
 * UNomadSurvivalHazardConfig
 * --------------------------
 * Data asset for survival hazards (starvation, dehydration, heatstroke, hypothermia)
 * Holds config for DoT percent, effect class, gameplay tags, and visual/audio cues.
 */
USTRUCT(BlueprintType)
struct FNomadHazardConfigRow
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category="Hazard")
    FName Name;

    UPROPERTY(EditDefaultsOnly, Category="Hazard")
    FGameplayTag HazardTag;

    UPROPERTY(EditDefaultsOnly, Category="Hazard")
    TSubclassOf<UNomadSurvivalStatusEffect> EffectClass;

    UPROPERTY(EditDefaultsOnly, Category="Hazard")
    float DoTPercent = 0.f; // e.g. 0.005 for 0.5% per second

    UPROPERTY(EditDefaultsOnly, Category="Hazard")
    FString StatType; // "HUNGER", "THIRST", "HOT", "COLD"

    UPROPERTY(EditDefaultsOnly, Category="Hazard")
    FString UIType;   // e.g. "BAR", "STATES", etc.

    UPROPERTY(EditDefaultsOnly, Category="Hazard")
    FString Gameplay; // Description of gameplay impact

    UPROPERTY(EditDefaultsOnly, Category="Hazard")
    FString VisualCue; // Description or reference to VFX/SFX

    UPROPERTY(EditDefaultsOnly, Category="Hazard")
    FString DesignerNotes;
};

UCLASS(BlueprintType)
class NOMADDEV_API UNomadSurvivalHazardConfig : public UDataAsset
{
    GENERATED_BODY()
public:
    // List of all survival hazards (starvation, dehydration, heatstroke, hypothermia)
    UPROPERTY(EditDefaultsOnly, Category="Hazards")
    TArray<FNomadHazardConfigRow> HazardConfigs;

    // Map from tag to index for fast lookup
    TMap<FGameplayTag, int32> BuildTagIndexMap() const
    {
        TMap<FGameplayTag, int32> Out;
        for (int32 i = 0; i < HazardConfigs.Num(); ++i)
        {
            Out.Add(HazardConfigs[i].HazardTag, i);
        }
        return Out;
    }

    // Get config row by tag
    const FNomadHazardConfigRow* GetHazardConfig(FGameplayTag Tag) const
    {
        for (const FNomadHazardConfigRow& Row : HazardConfigs)
        {
            if (Row.HazardTag == Tag) return &Row;
        }
        return nullptr;
    }
};