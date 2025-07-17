// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/StatusEffect/NomadBaseStatusEffect.h"
#include "Core/Component/NomadSurvivalNeedsComponent.h"
#include "NomadStatusEffectIntegrationValidator.generated.h"

class ACharacter;
class UNomadStatusEffectManagerComponent;

/**
 * UNomadStatusEffectIntegrationValidator
 * -------------------------------------
 * Debug utility class for validating the integration of unused status effect methods.
 * 
 * This class provides validation functions to test:
 * - Jump blocking functionality for severe survival conditions
 * - Movement speed synchronization across all status effect types
 * - Proper cleanup when effects are removed
 * - Multiplayer synchronization (basic validation)
 * 
 * Usage:
 * - Call validation functions during development/testing
 * - Use console commands to trigger validation
 * - Integrate into automated testing if available
 */
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class NOMADDEV_API UNomadStatusEffectIntegrationValidator : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Validates that jump blocking works correctly for severe survival conditions.
     * Tests both application and removal of jump blocking tags.
     * 
     * @param TestCharacter Character to test on
     * @return true if validation passes, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category="Debug|Status Effect Integration", CallInEditor=true)
    static bool ValidateJumpBlockingIntegration(ACharacter* TestCharacter);

    /**
     * Validates that movement speed synchronization works correctly.
     * Tests sync when status effects with movement modifiers are applied/removed.
     * 
     * @param TestCharacter Character to test on
     * @return true if validation passes, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category="Debug|Status Effect Integration", CallInEditor=true)
    static bool ValidateMovementSpeedSyncIntegration(ACharacter* TestCharacter);

    /**
     * Validates that survival effect integration works end-to-end.
     * Tests the full lifecycle of survival effects with integrated methods.
     * 
     * @param TestCharacter Character to test on
     * @return true if validation passes, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category="Debug|Status Effect Integration", CallInEditor=true)
    static bool ValidateSurvivalEffectIntegration(ACharacter* TestCharacter);

    /**
     * Comprehensive validation of all integration points.
     * Runs all validation tests and reports results.
     * 
     * @param TestCharacter Character to test on
     * @return true if all validations pass, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category="Debug|Status Effect Integration", CallInEditor=true)
    static bool ValidateAllIntegration(ACharacter* TestCharacter);

    /**
     * Validates that blocking tags are properly managed.
     * Tests tag application, removal, and state consistency.
     * 
     * @param TestCharacter Character to test on
     * @param TestTag Tag to test with
     * @return true if validation passes, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category="Debug|Status Effect Integration", CallInEditor=true)
    static bool ValidateBlockingTagManagement(ACharacter* TestCharacter, FGameplayTag TestTag);

private:
    /** Helper function to get status effect manager from character */
    static UNomadStatusEffectManagerComponent* GetStatusEffectManager(ACharacter* Character);
    
    /** Helper function to get survival needs component from character */
    static UNomadSurvivalNeedsComponent* GetSurvivalNeedsComponent(ACharacter* Character);
    
    /** Helper function to log validation results */
    static void LogValidationResult(const FString& TestName, bool bPassed, const FString& Details = "");
    
    /** Helper function to simulate severe survival condition */
    static bool SimulateSevereStarvation(ACharacter* Character);
    
    /** Helper function to clean up test effects */
    static void CleanupTestEffects(ACharacter* Character);
};