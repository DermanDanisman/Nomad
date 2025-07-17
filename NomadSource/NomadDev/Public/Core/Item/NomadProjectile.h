// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/ACFProjectile.h"
#include "NomadProjectile.generated.h"

// Forward declaration of the projectile data asset class.
class UProjectileData;

// Define a log category for logging messages related to the projectile.
DEFINE_LOG_CATEGORY_STATIC(LogNomadProjectile, Log, All);

/**
 * ANomadProjectile
 *
 * This class represents a projectile in the game.
 * It inherits from AACFProjectile to gain base projectile functionality.
 * The projectile's properties (e.g., speed, gravity, collision settings) are configured
 * using a data asset (UProjectileData), which holds all the necessary settings.
 *
 * It also implements interaction interface methods to allow pawn interaction.
 */
UCLASS()
class NOMADDEV_API ANomadProjectile : public AACFProjectile
{
	GENERATED_BODY()

public:
    // Constructor: Called when an instance of ANomadProjectile is created.
    ANomadProjectile();

    // INTERACTION INTERFACE METHODS
    // Called when a pawn interacts with this projectile.
    virtual void OnInteractedByPawn_Implementation(class APawn* Pawn, const FString& interactionType = "") override;
    
    // Determines whether this projectile can be interacted with by a pawn.
    virtual bool CanBeInteracted_Implementation(class APawn* Pawn) override;
    
    // Returns the name of the interactable (for UI purposes).
    virtual FText GetInteractableName_Implementation() override;

    // Called when the projectile is registered as interactable by a pawn.
    virtual void OnInteractableRegisteredByPawn_Implementation(APawn* Pawn) override;
    
    // Called when the projectile is unregistered as interactable by a pawn.
    virtual void OnInteractableUnregisteredByPawn_Implementation(APawn* Pawn) override;
    
    // Called when local interaction occurs.
    virtual void OnLocalInteractedByPawn_Implementation(APawn* Pawn, const FString& String) override;
    // END INTERACTION INTERFACE METHODS

    // Returns the thumbnail image for this projectile (used in UI).
    virtual UTexture2D* GetThumbnailImage() const override;

    // Returns the display name for this projectile.
    virtual FText GetItemName() const override;

    // Returns the description for this projectile.
    virtual FText GetItemDescription() const override;

    // Returns the type of item this projectile is (used by the item system).
    virtual EItemType GetItemType() const override;

    // Returns a complete descriptor structure with all item details.
    virtual FItemDescriptor GetItemInfo() const override;

    // Returns an array of gameplay tags indicating the valid item slots for this projectile.
    virtual TArray<FGameplayTag> GetPossibleItemSlots() const override;

protected:
    // BeginPlay: Called when the game starts or when the actor is spawned.
    virtual void BeginPlay() override;

    // InitializeItem: Reads values from the ProjectileData asset and applies them to configure this projectile.
    // Configures properties such as movement, collision, lifetime, and visual effects.
    UFUNCTION(BlueprintCallable, Category = Item)
    void InitializeItem();
    
    // Pointer to the data asset containing all the projectile settings.
    UPROPERTY(EditAnywhere, Category = "Item Data Asset")
    TObjectPtr<UProjectileData> ProjectileData;

    // Optional sound cue for gathering (if applicable).
    UPROPERTY(BlueprintReadWrite, Category = "Item Shared Properties")
    TObjectPtr<USoundCue> GatherSound;
};
