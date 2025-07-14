// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/Item/Resource/PickupItemActorData.h"
#include "Items/ACFPickup.h"
#include "NomadPickupItem.generated.h"

/**
 * 
 */
UCLASS()
class NOMADDEV_API ANomadPickupItem : public AACFPickup {
    GENERATED_BODY()

public:

    ANomadPickupItem();

    /*———— Data & Runtime State ————*/
    /** 
     * Asset containing all configuration: mesh, tags, tool requirements, 
     * loot table, health, gather time, destroy flag.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gatherable")
    TObjectPtr<UPickupItemActorData> PickupItemData;

    virtual void OnInteractedByPawn_Implementation(class APawn* Pawn, const FString& interactionType = "") override;

protected:

    virtual void BeginPlay() override;
};