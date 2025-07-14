// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Actions/ACFBaseAction.h"
#include "Items/ACFConsumable.h"
#include "ACFUseItemAction.generated.h"

// Forward declaration (if needed later) for any data asset used by this action.
class UConsumableData;

// Define a log category for messages related to UACFUseItemAction (for debugging purposes).
DEFINE_LOG_CATEGORY_STATIC(LogNomadConsumable, Log, All);

/**
 * UACFUseItemAction
 *
 * This action allows a character to use an item (typically a consumable).
 * It inherits from UACFBaseAction, which is the base class for actions in the combat framework.
 * The action supports features like:
 *   - Disabling hand IK during the action.
 *   - Trying to equip an off-hand item if available.
 *   - Optionally attempting to equip ammunition for ranged weapons.
 *   - Handling interruptions gracefully.
 */
UCLASS()
class ASCENTCOMBATFRAMEWORK_API UACFUseItemAction : public UACFBaseAction
{
	GENERATED_BODY()

public: 

    // Constructor: Initializes the action configuration.
	UACFUseItemAction();

protected:

    // Gameplay tag representing the item slot that the action will use.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = ACF)
	FGameplayTag ItemSlot;

    // If true, the action should still attempt to use the item if interrupted.
	UPROPERTY(EditDefaultsOnly, Category = ACF)
	bool bShouldUseIfInterrupted = false;

    // If true, the action will try to equip an off-hand item in addition to the main item.
	UPROPERTY(EditDefaultsOnly, Category = ACF)
	bool bTryToEquipOffhand = false;

    // If true, the action will attempt to equip ammo for the current weapon.
	UPROPERTY(EditDefaultsOnly, Category = ACF)
    bool bTryToEquipAmmo = true;

    // If true, the action will check and modify hand IK (inverse kinematics) settings during execution.
	UPROPERTY(EditDefaultsOnly, Category = ACF)
	bool bCheckHandsIK = true;

    // When bTryToEquipOffhand is true, this tag specifies the off-hand slot to try to use.
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bTryToEquipOffhand"), Category = ACF)
	FGameplayTag OffHandSlot;

    // A local flag used to track whether the action was successfully completed.
	bool bSuccess = false;

    // This function is called when the action starts.
    // It optionally processes input context and disables hand IK if required.
	virtual void OnActionStarted_Implementation(const FString& contextString = "", AActor* InteractedActor = nullptr, FGameplayTag ItemSlotTag = FGameplayTag()) override;

    // Called when a notable point in the action is reached (for example, at a critical animation event).
	virtual void OnNotablePointReached_Implementation() override;

    // Called when the action ends. It checks if the action was successful or interrupted and may finish using the item.
	virtual void OnActionEnded_Implementation() override;

    // Determines whether the action can be executed by the given character.
	virtual bool CanExecuteAction_Implementation(class ACharacter* owner, FGameplayTag ItemSlotTag = FGameplayTag()) override;

private: 
	// Helper function that performs the actual "use item" logic.
	void UseItem();
};