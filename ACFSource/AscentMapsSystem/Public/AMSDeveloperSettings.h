// // Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include <GameplayTagContainer.h>

#include "AMSDeveloperSettings.generated.h"


class UAMSMarkersConfigDataAsset;
class UAMSCompassConfigDataAsset;

/**
 *
 */
UCLASS(config = Plugins, Defaultconfig, meta = (DisplayName = "Ascent Navigation Settings"))
class ASCENTMAPSSYSTEM_API UAMSDeveloperSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    UAMSMarkersConfigDataAsset* GetMarkerIconConfig() const;

    FGameplayTag GetTrackedMarkerTag() const {
        return TrackedMarkerCategory;
    }

protected:
    UPROPERTY(config, EditAnywhere, Category = Style, meta = (AllowedClasses = "AMSMarkersConfigDataAsset"))
    FSoftObjectPath MarkerIconConfig;

   
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "AMS|Actor Marker")
    FGameplayTag TrackedMarkerCategory;

};
