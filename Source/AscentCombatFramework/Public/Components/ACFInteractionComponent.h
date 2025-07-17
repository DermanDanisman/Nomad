// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <Components/SphereComponent.h>

#include "ACFInteractionComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableRegistered, AActor*, interctableActor);

UCLASS(Blueprintable, ClassGroup = (ACF), meta = (BlueprintSpawnableComponent))
class ASCENTCOMBATFRAMEWORK_API UACFInteractionComponent : public USphereComponent {
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UACFInteractionComponent();

    /*Interacts with the best interactable nearby, calls both the server and client function*/
    UFUNCTION( BlueprintCallable, Category = ACF)
    void Interact(const FString& interactionType = "");

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetCurrentBestInteractable(class AActor* actor);

    UFUNCTION(BlueprintCallable, Category = ACF)
    FORCEINLINE class AActor* GetCurrentBestInteractableActor() { return currentBestInteractableActor; }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void EnableDetection(bool bIsEnabled);

    UFUNCTION(BlueprintCallable, Category = ACF)
    void AddCollisionChannel(TEnumAsByte<ECollisionChannel> inTraceChannel);

    UFUNCTION(BlueprintCallable, Category = ACF)
    void RemoveCollisionChannel(TEnumAsByte<ECollisionChannel> inTraceChannel);

    UFUNCTION(BlueprintCallable, Category = ACF)
    void RefreshInteractions();

    UFUNCTION(BlueprintCallable, Category = "Nomad Functions ")
    void NomadRefreshInteractions(AActor* InInteractableActor);

    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnInteractableRegistered OnInteractableRegistered;
    
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnInteractableRegistered OnInteractableUnregistered;

    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnInteractableRegistered OnInteractionSucceded;

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    /*Channels used to check for interactable objects*/
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = ACF)
    TArray<TEnumAsByte<ECollisionChannel>> CollisionChannels;

    UPROPERTY(EditDefaultsOnly, Category = ACF)
    float InteractableArea = 180.f;

    UPROPERTY(EditDefaultsOnly, Category = ACF)
    bool bAutoEnableOnBeginPlay = false;

public:
    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY()
    class APawn* PawnOwner;

    UFUNCTION(Server, Reliable)
    void ServerInteract(const FString& interactionType = "", AActor* bestInteractable = nullptr);

    void LocalInteract(const FString& interactionType = "");

    //UPROPERTY()
    class IACFInteractableInterface* currentBestInteractable;

    UPROPERTY(Transient)
    class AActor* currentBestInteractableActor;

    UPROPERTY()
    TArray<class AActor*> interactables;

    UFUNCTION()
    void UpdateInteractionArea();

    void InitChannels();



    UFUNCTION()
    void OnActorEnteredDetector(UPrimitiveComponent* _overlappedComponent,
        AActor* _otherActor, UPrimitiveComponent* _otherComp, int32 _otherBodyIndex,
        bool _bFromSweep, const FHitResult& _SweepResult);

    UFUNCTION()
    void OnActorLeavedDetector(UPrimitiveComponent* _overlappedComponent,
        AActor* _otherActor, UPrimitiveComponent* _otherComp, int32 _otherBodyIndex);



    void Internal_Interact(const FString& interactionType = "");
};
