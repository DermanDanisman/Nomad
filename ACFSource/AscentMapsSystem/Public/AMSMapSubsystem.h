// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "AMSTypes.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "AMSMapSubsystem.generated.h"

class AAMSMapArea;
class UAMSMapMarkerComponent;
class AAMSActorMarker;
class AAMSMapLocation;
class UAMSMarkersConfigDataAsset;
class UAMSCompassConfigDataAsset;
class UAMSMarkerWidget;

/**
 *
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapMarkerAdded, UAMSMapMarkerComponent*, marker);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapMarkerRemoved, UAMSMapMarkerComponent*, marker);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrackedMarkerChanged, UAMSMapMarkerComponent*, marker);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapAreaChanged, AAMSMapArea*, mapArea);

UCLASS()
class ASCENTMAPSSYSTEM_API UAMSMapSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:

    UAMSMapSubsystem();
    /*MAP AREAS*/
    void RegisterMapArea(const FName& tag, TObjectPtr<class AAMSMapArea> map);

    void RegisterPlayerInArea(const FName& tag, int32 priority);

    void UnregisterPlayerInArea(const FName& tag);

    /*Gets the MapArea with the provided name*/
    UFUNCTION(BlueprintCallable, Category = AMS)
    AAMSMapArea* GetRegisteredMapArea(const FName& tag) const;

    /*Returns the current Map Area set with SetCurrentMapArea*/
    UFUNCTION(BlueprintPure, Category = AMS)
    AAMSMapArea* GetCurrentMapArea() const;

    /*Set a map area as the current one to be used */
    UFUNCTION(BlueprintCallable, Category = AMS)
    void SetCurrentMapArea(AAMSMapArea* mapActor);

    UFUNCTION(BlueprintCallable, Category = AMS)
    TMap<FName, int32> GetAllAreasWithLocalPlayer() const
    {
        return Priorities;
    }

    /*LOCATIONS*/
    /*Returns all the locations in current map*/
    UFUNCTION(BlueprintPure, Category = AMS)
    TArray<AAMSMapLocation*> GetAllLocations() const;

    UFUNCTION(BlueprintPure, Category = AMS)
    AAMSMapLocation* GetLocationByName(const FName& locationName) const;

    /*Returns all the discovered locations in current map*/
    UFUNCTION(BlueprintPure, Category = AMS)
    TArray<AAMSMapLocation*> GetAllDiscoveredLocation() const;

    /*Returns all the discovered teleport locations in current map*/
    UFUNCTION(BlueprintPure, Category = AMS)
    TArray<AAMSMapLocation*> GetAllDiscoveredFastTravelLocation() const;

    /*MARKERS*/
    UFUNCTION(BlueprintPure, Category = AMS)
    TArray<UAMSMapMarkerComponent*> GetAllMarkers() const;

    /*Cheat to immediately discover all locations*/
    UFUNCTION(BlueprintCallable, Category = AMS)
    void DiscoverAllLocation();

    UFUNCTION(BlueprintPure, Category = AMS)
    bool IsMarkerActive(const UAMSMapMarkerComponent* markerComp) const;

    UFUNCTION(BlueprintCallable, Category = AMS)
    void RegisterMarker(class UAMSMapMarkerComponent* markerComp);

    UFUNCTION(BlueprintCallable, Category = AMS)
    void RemoveMarker(class UAMSMapMarkerComponent* markerComp);

    UFUNCTION(BlueprintCallable, Category = AMS)
    AAMSActorMarker* SpawnMarkerActor(const TSubclassOf<AAMSActorMarker>& markerClass, const FVector& worldPos, bool bProjectToNavmesh = true);

    UFUNCTION(BlueprintCallable, Category = AMS)
    void RemoveAllMarkerActors();

    UFUNCTION(BlueprintPure, Category = AMS)
    bool HasAtLeastOneMarkerActor() const;

    UFUNCTION(BlueprintPure, Category = AMS)
    TArray<class UAMSMapMarkerComponent*> GetAllMarkersByCategory(const FGameplayTag& markerComp) const;

    UFUNCTION(BlueprintCallable, Category = AMS)
    void RemoveAllMarkersByCategory(const FGameplayTag& markerComp);

    //     UFUNCTION(BlueprintCallable, Category = AMS)
    //     UAMSCompassConfigDataAsset* GetCompassConfig() const;

    UFUNCTION(BlueprintCallable, Category = AMS)
    UAMSMarkersConfigDataAsset* GetIconConfig() const;

    UPROPERTY(BlueprintAssignable, Category = AMS)
    FOnMapMarkerAdded OnMapMarkerAdded;

    UPROPERTY(BlueprintAssignable, Category = AMS)
    FOnMapMarkerRemoved OnMapMarkerRemoved;

    UPROPERTY(BlueprintAssignable, Category = AMS)
    FOnMapAreaChanged OnMapAreaChanged;

    UFUNCTION(BlueprintPure, Category = AMS)
    bool HasAnyTrackedMarker() const;

    UFUNCTION(BlueprintCallable, Category = AMS)
    void TrackMarker(UAMSMapMarkerComponent* marker);

    UFUNCTION(BlueprintCallable, Category = AMS)
    void UntrackMarker();

    UFUNCTION(BlueprintPure, Category = AMS)
    UAMSMapMarkerComponent* GetCurrentlytTrackedMarker() const; 

    UPROPERTY(BlueprintAssignable, Category = AMS)
    FOnTrackedMarkerChanged OnTrackedMarkerChanged;

private:
    TMap<FName, TObjectPtr<AAMSMapArea>> Maps;

    TObjectPtr<AAMSMapArea> currentMap;

    TMap<FName, int32> Priorities;

    TArray<TObjectPtr<UAMSMapMarkerComponent>> Markers;

    TObjectPtr<UAMSMapMarkerComponent> TrackedMarker;
    bool bHasMarkerTracked;

    TArray<TObjectPtr<AAMSActorMarker>> MarkerActors;

    void UpdateCurrentMap();
};
