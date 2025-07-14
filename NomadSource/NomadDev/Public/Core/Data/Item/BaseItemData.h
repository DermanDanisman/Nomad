// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/ACFItem.h"
#include "BaseItemData.generated.h"

class USoundCue;

USTRUCT(BlueprintType)
struct FBaseItemInfo
{
    GENERATED_BODY()

    /** Sound effect played when the item is gathered, picked or collected. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shared Properties")
    TObjectPtr<USoundCue> GatherSound = nullptr;

    /** General item descriptor containing additional information about the item */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item Information")
    FItemDescriptor ItemInfo = FItemDescriptor();
};

/**
 * 
 */
UCLASS(BlueprintType)
class NOMADDEV_API UBaseItemData : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item Information")
    FBaseItemInfo BaseItemInfo;
};
