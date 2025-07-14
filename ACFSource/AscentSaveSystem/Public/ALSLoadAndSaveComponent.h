// // Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once


#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "ALSLoadAndSaveComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActorSaved);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActorLoaded);




class UALSLoadAndSaveSubsystem;

UCLASS(ClassGroup = (ACF), meta = (BlueprintSpawnableComponent))
class ASCENTSAVESYSTEM_API UALSLoadAndSaveComponent : public UActorComponent {
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UALSLoadAndSaveComponent();

    
    UFUNCTION(BlueprintCallable, Category = ALS)
    void SaveActor();

    UFUNCTION(BlueprintCallable, Category = ALS)
    void LoadActor();

    void DispatchLoaded();

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Category = ALS)
    bool bAutoReload = true;


    UPROPERTY(BlueprintAssignable, Category = ALS)
    FOnActorSaved OnActorSaved;
  
    UPROPERTY(BlueprintAssignable, Category = ALS)
    FOnActorLoaded OnActorLoaded;

private:


    bool bAlreadyLoaded;

    UALSLoadAndSaveSubsystem* GetSaveSubsystem() const;
};
