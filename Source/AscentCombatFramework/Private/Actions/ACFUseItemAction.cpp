// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Actions/ACFUseItemAction.h"
#include "ACFItemTypes.h"                          // Includes definitions for item types.
#include "Actors/ACFCharacter.h"                   // The character class used in the combat framework.
#include "Animation/ACFAnimInstance.h"             // Provides animation instance functionality.
#include "Components/ACFEquipmentComponent.h"      // For handling equipment and inventory.
#include "Items/ACFRangedWeapon.h"                  // For ranged weapon functionality.
#include "Items/ACFWeapon.h"                        // Base weapon functionality.
#include <Runtime/GameplayTags/Classes/GameplayTagsManager.h> // For handling gameplay tags.

//---------------------------------------------------------------------
// OnActionStarted_Implementation
//---------------------------------------------------------------------
void UACFUseItemAction::OnActionStarted_Implementation(const FString& contextString, AActor* InteractedActor, FGameplayTag ItemSlotTag)
{
    // Reset success flag when the action starts.
    bSuccess = false;

    // If the ItemSlot property is not valid, then use the provided ItemSlotTag.
    //if (!ItemSlot.IsValid())
    //{
        //ItemSlot = ItemSlotTag;
    //}
    
    // If checking for hand inverse kinematics is enabled...
    if (bCheckHandsIK)
    {
        // Attempt to cast the character owner to AACFCharacter.
        const AACFCharacter* acfCharacter = Cast<AACFCharacter>(CharacterOwner);
        // If a context string is provided and not empty, request a gameplay tag from it.
        if (!contextString.IsEmpty())
        {
            // Use the Gameplay Tags Manager to convert the context string to a tag.
            ItemSlot = UGameplayTagsManager::Get().RequestGameplayTag(*contextString, false);
        }

        // If the character is valid, then disable hand IK.
        if (acfCharacter)
        {
            // Get a pointer to the equipment component from the character.
            const UACFEquipmentComponent* equipComp = acfCharacter->GetEquipmentComponent();
            // Get the animation instance to modify IK settings.
            UACFAnimInstance* animInst = acfCharacter->GetACFAnimInstance();
            if (equipComp && animInst)
            {
                // Disable hand IK for the duration of this action.
                animInst->SetEnableHandIK(false);
            }
        }
    }
}

//---------------------------------------------------------------------
// OnNotablePointReached_Implementation
//---------------------------------------------------------------------
void UACFUseItemAction::OnNotablePointReached_Implementation()
{
    // When a significant point in the action is reached (e.g., animation event),
    // perform the use item logic.
    UseItem();
    // Mark the action as successful.
    bSuccess = true;
}

//---------------------------------------------------------------------
// OnActionEnded_Implementation
//---------------------------------------------------------------------
void UACFUseItemAction::OnActionEnded_Implementation()
{
    // If the action ended without reaching the notable point (bSuccess is false)
    // and it is configured to use the item even if interrupted, then use the item.
    if (!bSuccess && bShouldUseIfInterrupted)
    {
        UseItem();
    }
    // If hand IK checking is enabled, then re-enable IK based on equipment settings.
    if (bCheckHandsIK)
    {
        // Cast the character owner to AACFCharacter.
        const AACFCharacter* acfCharacter = Cast<AACFCharacter>(CharacterOwner);
        if (acfCharacter)
        {
            // Retrieve the equipment component.
            const UACFEquipmentComponent* equipComp = acfCharacter->GetEquipmentComponent();
            // Retrieve the character's animation instance.
            UACFAnimInstance* animInst = acfCharacter->GetACFAnimInstance();
            if (equipComp && animInst)
            {
                // Re-enable hand IK if indicated by the equipment component.
                animInst->SetEnableHandIK(equipComp->ShouldUseLeftHandIK());
            }
        }
    }
}

//---------------------------------------------------------------------
// Constructor: UACFUseItemAction
//---------------------------------------------------------------------
UACFUseItemAction::UACFUseItemAction()
{
    // Add a movement mode in which the action is allowed: for example, falling.
    ActionConfig.PerformableInMovementModes.Add(MOVE_Falling);
}

//---------------------------------------------------------------------
// UseItem (Private Helper)
//---------------------------------------------------------------------
void UACFUseItemAction::UseItem()
{
    // Cast the CharacterOwner to AACFCharacter.
    AACFCharacter* acfCharacter = Cast<AACFCharacter>(CharacterOwner);
    
    // If the cast was successful...
    if (acfCharacter)
    {
        // Retrieve the equipment component from the character.
        UACFEquipmentComponent* equipComp = acfCharacter->GetEquipmentComponent();
        if (equipComp)
        {
            // Execute the use item command on the equipment component for the slot specified in ItemSlot.
            equipComp->UseEquippedItemBySlot(ItemSlot);
            // If configured to try equipping an off-hand item after the main item...
            if (bTryToEquipOffhand)
            {
                // Retrieve the current main weapon.
                const AACFWeapon* mainWeap = equipComp->GetCurrentMainWeapon();
                // If the main weapon exists and its handle type is OneHanded (allowing dual wield)...
                if (mainWeap && mainWeap->GetHandleType() == EHandleType::OneHanded)
                {
                    // Use the item from the off-hand slot.
                    equipComp->UseEquippedItemBySlot(OffHandSlot);
                }
            }
            // If configured to try equipping ammo...
            if (bTryToEquipAmmo)
            {
                // Attempt to cast the current main weapon to a ranged weapon.
                AACFRangedWeapon* rangedWeap = Cast<AACFRangedWeapon>(equipComp->GetCurrentMainWeapon());
                if (rangedWeap)
                {
                    // Call the Reload function on the ranged weapon.
                    rangedWeap->Reload(bTryToEquipAmmo);
                }
            }
        }
    }
}

//---------------------------------------------------------------------
// CanExecuteAction_Implementation
//---------------------------------------------------------------------
bool UACFUseItemAction::CanExecuteAction_Implementation(ACharacter* owner, FGameplayTag ItemSlotTag)
{
    // Attempt to cast the provided owner to our game-specific character class.
    AACFCharacter* acfCharacter = Cast<AACFCharacter>(owner);
    if (!acfCharacter)
    {
        // If the cast fails, the action cannot execute.
        return false;
    }

    // Get the character's equipment component, which holds inventory and equipped items.
    const UACFEquipmentComponent* equipComp = acfCharacter->GetEquipmentComponent();
    if (equipComp)
    {
        // If a valid ItemSlotTag is provided, override the current ItemSlot.
        //if (ItemSlotTag.IsValid())
        //{
            //ItemSlot = ItemSlotTag;
        //}
        // Finally, check whether the equipment component contains any item in the (possibly updated) ItemSlot.
        return equipComp->HasAnyItemInEquipmentSlot(ItemSlot);
    }
    
    // If no equipment component is found, default to allowing the action.
    return true;
}