// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ALSSavableInterface.h"
#include "AMSMapMarkerComponent.h"
#include "Core/Data/Item/Crafting/CraftingStationData.h"
#include "GameFramework/Actor.h"
#include "Interfaces/ACFInteractableInterface.h"
#include "CraftingStation.generated.h"

class UNomadCraftingComponent;

UCLASS()
class NOMADDEV_API ACraftingStation : public AActor, public IACFInteractableInterface, public IALSSavableInterface
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    ACraftingStation();

    virtual void OnInteractableRegisteredByPawn_Implementation(class APawn* Pawn) override;

    virtual void OnInteractableUnregisteredByPawn_Implementation(class APawn* Pawn) override;

    virtual void OnInteractedByPawn_Implementation(class APawn* Pawn, const FString& interactionType = "") override;

    virtual FText GetInteractableName_Implementation() override;

    virtual bool CanBeInteracted_Implementation(class APawn* Pawn) override;

protected:

    // Called when the actor is spawned or when the editor changes the actor's properties
    virtual void OnConstruction(const FTransform& Transform) override;

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;
    void UpdateMeshesAndMarker() const;

    UPROPERTY(VisibleAnywhere, Category = "Crafting Station")
    TObjectPtr<USceneComponent> DefaultRootComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting Station")
    TObjectPtr<USkeletalMeshComponent> CraftingStationSkeletalMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting Station")
    TObjectPtr<UStaticMeshComponent> CraftingStationStaticMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting Station")
    TObjectPtr<UAMSMapMarkerComponent> CraftingStationMapMarkerComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting Station")
    TObjectPtr<UNomadCraftingComponent> NomadCraftingComponent;


    // Data asset: Contains the settings and properties for this crafting station.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting Station Data Asset")
    TObjectPtr<UCraftingStationData> CraftingStationData;




};