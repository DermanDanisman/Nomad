// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NomadStatusEffectIntegrationTestLibrary.generated.h"

class ACharacter;

/**
 * UNomadStatusEffectIntegrationTestLibrary
 * ---------------------------------------
 * Blueprint function library for easy access to status effect integration validation.
 *
 * This provides Blueprint-callable functions to test the integration of unused status effect methods.
 * Can be used in Blueprint graphs, called from console commands, or used in automated testing.
 */
UCLASS()
class NOMADDEV_API UNomadStatusEffectIntegrationTestLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Runs all status effect integration validation tests on the specified character.
     * This is the main entry point for comprehensive testing.
     *
     * @param Character Character to test on (usually the player character)
     * @return true if all tests pass, false if any test fails
     */
    UFUNCTION(BlueprintCallable, Category="Debug|Status Effect Tests", CallInEditor=true)
    static bool RunAllStatusEffectIntegrationTests(ACharacter* Character);

    /**
     * Tests specifically the jump blocking integration for severe survival conditions.
     *
     * @param Character Character to test on
     * @return true if jump blocking tests pass, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category="Debug|Status Effect Tests", CallInEditor=true)
    static bool TestJumpBlockingIntegration(ACharacter* Character);

    /**
     * Tests specifically the movement speed synchronization integration.
     *
     * @param Character Character to test on
     * @return true if movement speed sync tests pass, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category="Debug|Status Effect Tests", CallInEditor=true)
    static bool TestMovementSpeedSyncIntegration(ACharacter* Character);

    /**
     * Tests the full survival effect integration (hunger, thirst, temperature effects).
     *
     * @param Character Character to test on
     * @return true if survival effect tests pass, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category="Debug|Status Effect Tests", CallInEditor=true)
    static bool TestSurvivalEffectIntegration(ACharacter* Character);

    /**
     * Gets the player character for testing purposes.
     * Convenience function to get a test target in Blueprint.
     *
     * @param WorldContext World context for finding the player
     * @return Player character if found, nullptr otherwise
     */
    UFUNCTION(BlueprintPure, Category="Debug|Status Effect Tests", CallInEditor=true, meta=(WorldContext="WorldContextObject"))
    static ACharacter* GetPlayerCharacterForTesting(const UObject* WorldContextObject);

    /**
     * Logs test results to the console and on-screen debug messages.
     *
     * @param TestName Name of the test that was run
     * @param bPassed Whether the test passed or failed
     * @param Details Additional details about the test result
     */
    UFUNCTION(BlueprintCallable, Category="Debug|Status Effect Tests")
    static void LogTestResult(const FString& TestName, bool bPassed, const FString& Details = "");

    /**
     * Simulates severe survival conditions for testing purposes.
     * This is a debug function that forces survival stats to critical levels.
     *
     * @param Character Character to modify
     * @param bStarvation Whether to simulate starvation (hunger = 0)
     * @param bDehydration Whether to simulate dehydration (thirst = 0)
     * @param bTemperatureExtreme Whether to simulate extreme body temperature
     */
    UFUNCTION(BlueprintCallable, Category="Debug|Status Effect Tests", CallInEditor=true)
    static void SimulateSevereConditionsForTesting(ACharacter* Character, bool bStarvation = true, bool bDehydration = false, bool bTemperatureExtreme = false);

    /**
     * Restores character to normal survival conditions after testing.
     *
     * @param Character Character to restore
     */
    UFUNCTION(BlueprintCallable, Category="Debug|Status Effect Tests", CallInEditor=true)
    static void RestoreNormalConditionsAfterTesting(ACharacter* Character);
};