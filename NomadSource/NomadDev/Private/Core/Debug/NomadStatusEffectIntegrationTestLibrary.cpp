// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Debug/NomadStatusEffectIntegrationTestLibrary.h"
#include "Core/Component/NomadSurvivalNeedsComponent.h"
#include "Core/Debug/NomadStatusEffectIntegrationValidator.h"
#include "Core/Debug/NomadLogCategories.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "ARSStatisticsComponent.h"
#include "Engine/Engine.h"

bool UNomadStatusEffectIntegrationTestLibrary::RunAllStatusEffectIntegrationTests(ACharacter* Character)
{
    LogTestResult("Status Effect Integration Test Suite", false, "Starting comprehensive testing...");
    
    if (!Character)
    {
        LogTestResult("Status Effect Integration Test Suite", false, "No character provided for testing");
        return false;
    }
    
    bool bAllTestsPassed = UNomadStatusEffectIntegrationValidator::ValidateAllIntegration(Character);
    
    FString ResultMessage = bAllTestsPassed ? "All integration tests PASSED" : "Some integration tests FAILED";
    LogTestResult("Status Effect Integration Test Suite", bAllTestsPassed, ResultMessage);
    
    return bAllTestsPassed;
}

bool UNomadStatusEffectIntegrationTestLibrary::TestJumpBlockingIntegration(ACharacter* Character)
{
    if (!Character)
    {
        LogTestResult("Jump Blocking Test", false, "No character provided");
        return false;
    }
    
    bool bPassed = UNomadStatusEffectIntegrationValidator::ValidateJumpBlockingIntegration(Character);
    LogTestResult("Jump Blocking Test", bPassed, bPassed ? "PASSED" : "FAILED");
    
    return bPassed;
}

bool UNomadStatusEffectIntegrationTestLibrary::TestMovementSpeedSyncIntegration(ACharacter* Character)
{
    if (!Character)
    {
        LogTestResult("Movement Speed Sync Test", false, "No character provided");
        return false;
    }
    
    bool bPassed = UNomadStatusEffectIntegrationValidator::ValidateMovementSpeedSyncIntegration(Character);
    LogTestResult("Movement Speed Sync Test", bPassed, bPassed ? "PASSED" : "FAILED");
    
    return bPassed;
}

bool UNomadStatusEffectIntegrationTestLibrary::TestSurvivalEffectIntegration(ACharacter* Character)
{
    if (!Character)
    {
        LogTestResult("Survival Effect Integration Test", false, "No character provided");
        return false;
    }
    
    bool bPassed = UNomadStatusEffectIntegrationValidator::ValidateSurvivalEffectIntegration(Character);
    LogTestResult("Survival Effect Integration Test", bPassed, bPassed ? "PASSED" : "FAILED");
    
    return bPassed;
}

ACharacter* UNomadStatusEffectIntegrationTestLibrary::GetPlayerCharacterForTesting(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }
    
    // Get the first player character for testing
    ACharacter* PlayerCharacter = Cast<ACharacter>(UGameplayStatics::GetPlayerCharacter(WorldContextObject, 0));
    
    if (!PlayerCharacter)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("No player character found for testing"));
    }
    
    return PlayerCharacter;
}

void UNomadStatusEffectIntegrationTestLibrary::LogTestResult(const FString& TestName, bool bPassed, const FString& Details)
{
    // Log to console
    if (bPassed)
    {
        UE_LOG_AFFLICTION(Log, TEXT("[TEST] ✓ %s: %s"), *TestName, *Details);
    }
    else
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[TEST] ✗ %s: %s"), *TestName, *Details);
    }
    
    // Also show on-screen debug message for easy visibility
    if (GEngine)
    {
        FColor MessageColor = bPassed ? FColor::Green : FColor::Red;
        FString Message = FString::Printf(TEXT("[TEST] %s %s: %s"), 
                                        bPassed ? TEXT("✓") : TEXT("✗"), 
                                        *TestName, *Details);
        
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, MessageColor, Message);
    }
}

void UNomadStatusEffectIntegrationTestLibrary::SimulateSevereConditionsForTesting(ACharacter* Character, bool bStarvation, bool bDehydration, bool bTemperatureExtreme)
{
    if (!Character)
    {
        LogTestResult("Simulate Severe Conditions", false, "No character provided");
        return;
    }
    
    UARSStatisticsComponent* StatsComp = Character->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp)
    {
        LogTestResult("Simulate Severe Conditions", false, "No statistics component found");
        return;
    }
    
    FString ConditionsApplied;
    
    if (bStarvation)
    {
        // Set hunger to 0 to trigger severe starvation
        const FGameplayTag HungerTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Statistics.Hunger"));
        const float CurrentHunger = StatsComp->GetCurrentValueForStatitstic(HungerTag);
        StatsComp->ModifyStatistic(HungerTag, -CurrentHunger);
        ConditionsApplied += "Starvation ";
    }
    
    if (bDehydration)
    {
        // Set thirst to 0 to trigger severe dehydration
        const FGameplayTag ThirstTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Statistics.Thirst"));
        const float CurrentThirst = StatsComp->GetCurrentValueForStatitstic(ThirstTag);
        StatsComp->ModifyStatistic(ThirstTag, -CurrentThirst);
        ConditionsApplied += "Dehydration ";
    }
    
    if (bTemperatureExtreme)
    {
        // Set body temperature to extreme level
        const FGameplayTag BodyTempTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Statistics.BodyTemperature"));
        const float CurrentBodyTemp = StatsComp->GetCurrentValueForStatitstic(BodyTempTag);
        
        // Set to extreme heat (assuming normal body temp is around 37°C, set to 45°C for heatstroke)
        const float ExtremeTemp = 45.0f;
        const float TempChange = ExtremeTemp - CurrentBodyTemp;
        StatsComp->ModifyStatistic(BodyTempTag, TempChange);
        ConditionsApplied += "Extreme Temperature ";
    }
    
    LogTestResult("Simulate Severe Conditions", true, 
                  FString::Printf(TEXT("Applied conditions: %s"), *ConditionsApplied));
}

void UNomadStatusEffectIntegrationTestLibrary::RestoreNormalConditionsAfterTesting(ACharacter* Character)
{
    if (!Character)
    {
        LogTestResult("Restore Normal Conditions", false, "No character provided");
        return;
    }
    
    UARSStatisticsComponent* StatsComp = Character->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp)
    {
        LogTestResult("Restore Normal Conditions", false, "No statistics component found");
        return;
    }
    
    // Restore hunger to 75% of max
    const FGameplayTag HungerTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Statistics.Hunger"));
    const float MaxHunger = StatsComp->GetMaxValueForStatitstic(HungerTag);
    const float CurrentHunger = StatsComp->GetCurrentValueForStatitstic(HungerTag);
    const float TargetHunger = MaxHunger * 0.75f;
    StatsComp->ModifyStatistic(HungerTag, TargetHunger - CurrentHunger);
    
    // Restore thirst to 75% of max
    const FGameplayTag ThirstTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Statistics.Thirst"));
    const float MaxThirst = StatsComp->GetMaxValueForStatitstic(ThirstTag);
    const float CurrentThirst = StatsComp->GetCurrentValueForStatitstic(ThirstTag);
    const float TargetThirst = MaxThirst * 0.75f;
    StatsComp->ModifyStatistic(ThirstTag, TargetThirst - CurrentThirst);
    
    // Restore body temperature to normal (37°C)
    const FGameplayTag BodyTempTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Statistics.BodyTemperature"));
    const float CurrentBodyTemp = StatsComp->GetCurrentValueForStatitstic(BodyTempTag);
    const float NormalBodyTemp = 37.0f;
    StatsComp->ModifyStatistic(BodyTempTag, NormalBodyTemp - CurrentBodyTemp);
    
    // Clean up any test effects via the survival component
    if (UNomadSurvivalNeedsComponent* SurvivalComp = Character->FindComponentByClass<UNomadSurvivalNeedsComponent>())
    {
        SurvivalComp->RemoveAllSurvivalEffects();
    }
    
    LogTestResult("Restore Normal Conditions", true, "All survival stats restored to normal levels");
}