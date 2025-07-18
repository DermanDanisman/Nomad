// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/ACFWorldItem.h"
#include "ARSTypes.h"
#include "Core/Interface/GatherableInterface.h"
#include "NomadWorldItem.generated.h"

class UPickupItemActorData;

/**
 *
 */
UCLASS()
class NOMADDEV_API ANomadWorldItem : public AACFWorldItem, public IGatherableInterface
{
    GENERATED_BODY()

public:

    ANomadWorldItem();

    // Called only on the server:
    UFUNCTION()
    void StartPhysics();

    UFUNCTION()
    void StopPhysics();

    // Timer handles:
    FTimerHandle PhysicsStartTimerHandle;
    FTimerHandle PhysicsStopTimerHandle;

    // How long after spawn to kick off physics:
    static constexpr float StartDelay = 0.1f;
    // How long physics runs before we stop it:
    static constexpr float PhysicsWindow = 5.0f;

    /*———— Data & Runtime State ————*/
    /**
     * Asset containing all configuration: mesh, tags, tool requirements,
     * loot table, health, gather time, destroy flag.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gatherable")
    TObjectPtr<UPickupItemActorData> PickupItemData;

    UPROPERTY(EditAnywhere, Category = ACF)
    bool bPickOnOverlap = true;

    UPROPERTY(EditAnywhere, Category = ACF)
    bool bAutoEquipOnPick = true;

    UPROPERTY(EditAnywhere, Category = ACF)
    TArray<FStatisticValue> OnPickupEffect;

    UPROPERTY(EditAnywhere, Category = ACF)
    TArray<FTimedAttributeSetModifier> OnPickupBuff;

    void SetPickupItemData(UPickupItemActorData* InPickupItemActorData) { PickupItemData = InPickupItemActorData; }

    virtual FText GetInteractableName_Implementation() override;

    virtual FGameplayTag GetCollectionTag_Implementation() const override;

protected:
    virtual void PostInitializeComponents() override;
};
