// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ACFItemTypes.h"
#include "BaseWeaponData.h"
#include "Engine/DataAsset.h"
#include "MeleeWeaponData.generated.h"

USTRUCT(BlueprintType)
struct FMeleeWeaponInfo : public FBaseWeaponInfo
{
    GENERATED_BODY()
    FMeleeWeaponInfo()
    {
        // initialize any new melee-specific defaults here
    }
};

/**
 *
 */
UCLASS(BlueprintType)
class NOMADDEV_API UMeleeWeaponData : public UDataAsset
{
    GENERATED_BODY()

public:

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item Information")
    FMeleeWeaponInfo MeleeWeaponInfo;
};
