// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "AMSMapLocation.generated.h"

class UAMSMapMarkerComponent;
class USphereComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDiscovered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFastTravel);

UCLASS()
class ASCENTMAPSSYSTEM_API AAMSMapLocation : public AActor {
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AAMSMapLocation();

    UFUNCTION(BlueprintPure, Category = AMS)
    FString GetLocationName() const;

    UFUNCTION(BlueprintCallable, Category = AMS)
    void SetLocationName(const FString& newName);

    UFUNCTION(BlueprintCallable, Category = AMS)
    void DiscoverLocation();

    UFUNCTION(BlueprintCallable, Category = AMS)
    void SetDiscoveredState(bool newState);

    UFUNCTION(BlueprintPure, Category = AMS)
    bool IsDiscovered() const { return bDiscovered; }

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = AMS)
    bool CanFastTravel() const;
    virtual bool CanFastTravel_Implementation() const;

    /*Teleports current player to the */
    UFUNCTION(BlueprintCallable, Category = AMS)
    void FastTraverlToLocation();

    UFUNCTION(BlueprintPure, Category = AMS)
    FVector GetFastTravelPoint() const;

    UFUNCTION(BlueprintPure, Category = AMS)
    UAMSMapMarkerComponent* GetMarkerComponent() const
    {
        return MarkerComp;
    }

    UFUNCTION(BlueprintPure, Category = AMS)
    USphereComponent* GetDiscoverAreaComponent() const
    {
        return SphereComp;
    }

    UPROPERTY(BlueprintAssignable, Category = AMS)
    FOnFastTravel OnFastTravelEvent;

    UPROPERTY(BlueprintAssignable, Category = AMS)
    FOnDiscovered OnDiscoveredEvent;

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere, DisplayName = "Discover Area Component", Category = "AMS")
    TObjectPtr<USphereComponent> SphereComp;

    UPROPERTY(VisibleAnywhere, DisplayName = "AMS Marker Component", Category = "AMS")
    TObjectPtr<UAMSMapMarkerComponent> MarkerComp;

    /*Indicates if this location will be discovered when local player overlaps with this actor*/
    UPROPERTY(EditAnywhere, Category = "AMS")
    bool bDiscoverOnPlayerOverlap = true;

    //     /*Indicates the name of this location*/
    //     UPROPERTY(EditAnywhere, Category = "AMS")
    //     FName LocationName = "DefaultLocationName";

    /*Indicates if this location can be used as teleport*/
    UPROPERTY(EditAnywhere, Category = "AMS")
    bool bCanFastTravel = false;

    /*Indicates where you'll player will be teleported when teleports to this location */
    UPROPERTY(EditAnywhere, meta = (MakeEditWidget), meta = (EditCondition = "bCanTeleport"), Category = "AMS")
    FVector FastTravelLocation;

    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void OnDiscovered();
    virtual void OnDiscovered_Implementation();

    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void OnTeleported();
    virtual void OnTeleported_Implementation();

private:
    UPROPERTY(Savegame)
    bool bDiscovered = false;

    UFUNCTION()
    void HandleLocalPlayerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
