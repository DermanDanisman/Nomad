// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ANSUITypes.h"
#include "Components/InputComponent.h"
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "UObject/NoExportTypes.h"

#include "ANSDeveloperSettings.generated.h"

/**
 *
 */

UCLASS(config = Plugins, Defaultconfig, meta = (DisplayName = "Ascent UI Input Settings"))
class ASCENTUINAVIGATIONSYSTEM_API UANSDeveloperSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    UANSDeveloperSettings()
    {
    }

    UDataTable* GetIconsByTagDT() const; 

    UDataTable* GetKeysConfigByPlatformDT(const FString& platformName) const;

private:

    /** List of data tables to load tags from */
    UPROPERTY(config, EditAnywhere, Category = "UI Keys", meta = (AllowedClasses = "/Script/Engine.DataTable"))
    FSoftObjectPath IconsByTag;

    UPROPERTY(config, EditAnywhere, Category = "UI Keys", meta = (AllowedClasses = "/Script/Engine.DataTable"))
    TMap<FString, FSoftObjectPath> KeysConfigByPlatform;

  
};
