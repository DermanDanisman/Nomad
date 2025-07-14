// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"

#include "ADSDialogueDeveloperSettings.generated.h"


/**
 *
 */
UCLASS(config = Plugins, Defaultconfig, meta = (DisplayName = "Ascent Dialogue Settings"))
class ASCENTDIALOGUESYSTEM_API UADSDialogueDeveloperSettings : public UDeveloperSettings {
    
    GENERATED_BODY()

public: 

    FGameplayTag GetDefaultPlayerResponseTag() const {
        return DefaultPlayerResponseTag;
    }


protected:
    
    UPROPERTY(EditAnywhere, config, Category = "Default Tags")
    FGameplayTag DefaultPlayerResponseTag;
};
