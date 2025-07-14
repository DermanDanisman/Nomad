// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ACFItemSystemFunctionLibrary.h"
#include "Core/Data/Item/Resource/PickupItemActorData.h"
#include "NomadItemSystemFunctionLibrary.generated.h"

class UCameraComponent;
class AACFWorldItem;
/**
 * 
 */
UCLASS()
class NOMADDEV_API UNomadItemSystemFunctionLibrary : public UACFItemSystemFunctionLibrary {

    GENERATED_BODY()

public:
    /**
     * Nomad Dev Team added this function.
     * This function is for resource spawning after gathering completes.
     */
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Nomad Function Library")
    static AACFWorldItem* SpawnResourceWorldItemNearLocation(UObject* WorldContextObject,
        const TArray<FBaseItem>& ContainedItem, const FVector& Location, float AcceptanceRadius /*= 100.f*/, bool bUsePhysics, UPickupItemActorData* ItemActorData);

    UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "Nomad Function Library")
    static FHitResult PerformLineTraceFromCameraManager(UObject* WorldContextObject, APlayerController* PlayerController, float TraceLength, bool bShowDebug = false);
};