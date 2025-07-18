// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Debug/NomadStatusEffectIntegrationValidator.h"
#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "Core/Component/NomadSurvivalNeedsComponent.h"
#include "Core/StatusEffect/SurvivalHazard/NomadSurvivalStatusEffect.h"
#include "Core/Debug/NomadLogCategories.h"
#include "GameFramework/Character.h"
#include "ARSStatisticsComponent.h"
#include "Components/ACFCharacterMovementComponent.h"

// =====================================================
//         VALIDATION FUNCTIONS
// =====================================================

bool UNomadStatusEffectIntegrationValidator::ValidateJumpBlockingIntegration(ACharacter* TestCharacter)
{
    LogValidationResult("Jump Blocking Integration", false, "Starting validation...");

    if (!TestCharacter)
    {
        LogValidationResult("Jump Blocking Integration", false, "Invalid test character");
        return false;
    }

    UNomadStatusEffectManagerComponent* SEManager = GetStatusEffectManager(TestCharacter);
    if (!SEManager)
    {
        LogValidationResult("Jump Blocking Integration", false, "No status effect manager found");
        return false;
    }

    // Test 1: Verify jump blocking tag can be applied
    const FGameplayTag JumpBlockTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Block.Jump"));

    // Apply jump block tag directly through base status effect method
    UNomadBaseStatusEffect* TestEffect = NewObject<UNomadBaseStatusEffect>();
    if (TestEffect)
    {
        TestEffect->ApplyJumpBlockTag(TestCharacter);

        // Check if blocking tag was applied
        bool bHasJumpBlock = SEManager->HasBlockingTag(JumpBlockTag);
        if (!bHasJumpBlock)
        {
            LogValidationResult("Jump Blocking Integration", false, "Jump blocking tag was not applied");
            return false;
        }

        // Test 2: Verify jump blocking tag can be removed
        TestEffect->RemoveJumpBlockTag(TestCharacter);

        bHasJumpBlock = SEManager->HasBlockingTag(JumpBlockTag);
        if (bHasJumpBlock)
        {
            LogValidationResult("Jump Blocking Integration", false, "Jump blocking tag was not removed");
            return false;
        }
    }

    LogValidationResult("Jump Blocking Integration", true, "All jump blocking tests passed");
    return true;
}

bool UNomadStatusEffectIntegrationValidator::ValidateMovementSpeedSyncIntegration(ACharacter* TestCharacter)
{
    LogValidationResult("Movement Speed Sync Integration", false, "Starting validation...");

    if (!TestCharacter)
    {
        LogValidationResult("Movement Speed Sync Integration", false, "Invalid test character");
        return false;
    }

    UACFCharacterMovementComponent* MoveComp = TestCharacter->FindComponentByClass<UACFCharacterMovementComponent>();
    if (!MoveComp)
    {
        LogValidationResult("Movement Speed Sync Integration", false, "No ACF movement component found");
        return false;
    }

    UARSStatisticsComponent* StatsComp = TestCharacter->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp)
    {
        LogValidationResult("Movement Speed Sync Integration", false, "No ARS statistics component found");
        return false;
    }

    // Store original movement speed
    const float OriginalSpeed = MoveComp->MaxWalkSpeed;

    // Test 1: Verify movement speed sync works
    UNomadBaseStatusEffect* TestEffect = NewObject<UNomadBaseStatusEffect>();
    if (TestEffect)
    {
        // Call sync method - this should update movement component with current stat values
        TestEffect->SyncMovementSpeedModifier(TestCharacter, 1.0f);

        // Verify the movement component speed is synced with stats
        const FGameplayTag MovementSpeedTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Attributes.MovementSpeed"));
        const float StatSpeed = StatsComp->GetCurrentAttributeValue(MovementSpeedTag);
        const float CurrentMoveSpeed = MoveComp->MaxWalkSpeed;

        // Allow small floating point differences
        if (FMath::Abs(StatSpeed - CurrentMoveSpeed) > 0.1f)
        {
            LogValidationResult("Movement Speed Sync Integration", false,
                FString::Printf(TEXT("Movement speed not synced: Stat=%.2f, Movement=%.2f"), StatSpeed, CurrentMoveSpeed));
            return false;
        }

        // Test 2: Verify remove movement speed modifier works
        TestEffect->RemoveMovementSpeedModifier(TestCharacter);

        // Should still be synced after removal call
        const float FinalMoveSpeed = MoveComp->MaxWalkSpeed;
        const float FinalStatSpeed = StatsComp->GetCurrentAttributeValue(MovementSpeedTag);

        if (FMath::Abs(FinalStatSpeed - FinalMoveSpeed) > 0.1f)
        {
            LogValidationResult("Movement Speed Sync Integration", false,
                FString::Printf(TEXT("Movement speed not synced after removal: Stat=%.2f, Movement=%.2f"), FinalStatSpeed, FinalMoveSpeed));
            return false;
        }
    }

    LogValidationResult("Movement Speed Sync Integration", true, "All movement speed sync tests passed");
    return true;
}

bool UNomadStatusEffectIntegrationValidator::ValidateSurvivalEffectIntegration(ACharacter* TestCharacter)
{
    LogValidationResult("Survival Effect Integration", false, "Starting validation...");

    if (!TestCharacter)
    {
        LogValidationResult("Survival Effect Integration", false, "Invalid test character");
        return false;
    }

    UNomadSurvivalNeedsComponent* SurvivalComp = GetSurvivalNeedsComponent(TestCharacter);
    if (!SurvivalComp)
    {
        LogValidationResult("Survival Effect Integration", false, "No survival needs component found");
        return false;
    }

    UNomadStatusEffectManagerComponent* SEManager = GetStatusEffectManager(TestCharacter);
    if (!SEManager)
    {
        LogValidationResult("Survival Effect Integration", false, "No status effect manager found");
        return false;
    }

    // Test 1: Simulate severe starvation and verify integration
    bool bStarvationSimulated = SimulateSevereStarvation(TestCharacter);
    if (!bStarvationSimulated)
    {
        LogValidationResult("Survival Effect Integration", false, "Failed to simulate severe starvation");
        return false;
    }

    // Wait a brief moment for effect application
    // Note: In a real test environment, you might want to use a timer or wait mechanism

    // Check if starvation effect was applied
    const FGameplayTag StarvationTag = FGameplayTag::RequestGameplayTag(TEXT("StatusEffect.Survival.Starvation"));
    bool bHasStarvationEffect = SEManager->HasActiveStatusEffect(StarvationTag);

    if (!bHasStarvationEffect)
    {
        LogValidationResult("Survival Effect Integration", false, "Starvation effect was not applied");
        CleanupTestEffects(TestCharacter);
        return false;
    }

    // Test 2: Verify jump blocking is applied for severe starvation
    const FGameplayTag JumpBlockTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Block.Jump"));
    bool bHasJumpBlock = SEManager->HasBlockingTag(JumpBlockTag);

    if (!bHasJumpBlock)
    {
        LogValidationResult("Survival Effect Integration", false, "Jump blocking not applied for severe starvation");
        CleanupTestEffects(TestCharacter);
        return false;
    }

    // Test 3: Clean up and verify removal
    CleanupTestEffects(TestCharacter);

    bHasStarvationEffect = SEManager->HasActiveStatusEffect(StarvationTag);
    bHasJumpBlock = SEManager->HasBlockingTag(JumpBlockTag);

    if (bHasStarvationEffect || bHasJumpBlock)
    {
        LogValidationResult("Survival Effect Integration", false, "Effects not properly cleaned up");
        return false;
    }

    LogValidationResult("Survival Effect Integration", true, "All survival effect integration tests passed");
    return true;
}

bool UNomadStatusEffectIntegrationValidator::ValidateBlockingTagManagement(ACharacter* TestCharacter, FGameplayTag TestTag)
{
    FString TestName = FString::Printf(TEXT("Blocking Tag Management (%s)"), *TestTag.ToString());
    LogValidationResult(TestName, false, "Starting validation...");

    if (!TestCharacter)
    {
        LogValidationResult(TestName, false, "Invalid test character");
        return false;
    }

    if (!TestTag.IsValid())
    {
        LogValidationResult(TestName, false, "Invalid test tag");
        return false;
    }

    UNomadStatusEffectManagerComponent* SEManager = GetStatusEffectManager(TestCharacter);
    if (!SEManager)
    {
        LogValidationResult(TestName, false, "No status effect manager found");
        return false;
    }

    // Test 1: Initial state should not have the tag
    bool bInitiallyHasTag = SEManager->HasBlockingTag(TestTag);
    if (bInitiallyHasTag)
    {
        LogValidationResult(TestName, false, "Tag already present at start of test");
        return false;
    }

    // Test 2: Add the tag
    SEManager->AddBlockingTag(TestTag);
    bool bHasTagAfterAdd = SEManager->HasBlockingTag(TestTag);
    if (!bHasTagAfterAdd)
    {
        LogValidationResult(TestName, false, "Tag not added successfully");
        return false;
    }

    // Test 3: Remove the tag
    SEManager->RemoveBlockingTag(TestTag);
    bool bHasTagAfterRemove = SEManager->HasBlockingTag(TestTag);
    if (bHasTagAfterRemove)
    {
        LogValidationResult(TestName, false, "Tag not removed successfully");
        return false;
    }

    LogValidationResult(TestName, true, "All blocking tag management tests passed");
    return true;
}

bool UNomadStatusEffectIntegrationValidator::ValidateAllIntegration(ACharacter* TestCharacter)
{
    LogValidationResult("Comprehensive Integration Validation", false, "Starting all validations...");

    bool bAllPassed = true;
    int32 PassedTests = 0;
    int32 TotalTests = 0;

    // Test 1: Jump Blocking Integration
    TotalTests++;
    if (ValidateJumpBlockingIntegration(TestCharacter))
    {
        PassedTests++;
    }
    else
    {
        bAllPassed = false;
    }

    // Test 2: Movement Speed Sync Integration
    TotalTests++;
    if (ValidateMovementSpeedSyncIntegration(TestCharacter))
    {
        PassedTests++;
    }
    else
    {
        bAllPassed = false;
    }

    // Test 3: Survival Effect Integration
    TotalTests++;
    if (ValidateSurvivalEffectIntegration(TestCharacter))
    {
        PassedTests++;
    }
    else
    {
        bAllPassed = false;
    }

    // Test 4: Blocking Tag Management
    TotalTests++;
    const FGameplayTag TestTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Block.Jump"));
    if (ValidateBlockingTagManagement(TestCharacter, TestTag))
    {
        PassedTests++;
    }
    else
    {
        bAllPassed = false;
    }

    FString ResultDetails = FString::Printf(TEXT("Passed %d out of %d tests"), PassedTests, TotalTests);
    LogValidationResult("Comprehensive Integration Validation", bAllPassed, ResultDetails);

    return bAllPassed;
}

// =====================================================
//         HELPER FUNCTIONS
// =====================================================

UNomadStatusEffectManagerComponent* UNomadStatusEffectIntegrationValidator::GetStatusEffectManager(ACharacter* Character)
{
    if (!Character)
    {
        return nullptr;
    }

    return Character->FindComponentByClass<UNomadStatusEffectManagerComponent>();
}

UNomadSurvivalNeedsComponent* UNomadStatusEffectIntegrationValidator::GetSurvivalNeedsComponent(ACharacter* Character)
{
    if (!Character)
    {
        return nullptr;
    }

    return Character->FindComponentByClass<UNomadSurvivalNeedsComponent>();
}

void UNomadStatusEffectIntegrationValidator::LogValidationResult(const FString& TestName, bool bPassed, const FString& Details)
{
    if (bPassed)
    {
        UE_LOG_AFFLICTION(Log, TEXT("[VALIDATION] ✓ %s - PASSED %s"), *TestName, *Details);
    }
    else
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[VALIDATION] ✗ %s - FAILED %s"), *TestName, *Details);
    }
}

bool UNomadStatusEffectIntegrationValidator::SimulateSevereStarvation(ACharacter* Character)
{
    if (!Character)
    {
        return false;
    }

    UARSStatisticsComponent* StatsComp = Character->FindComponentByClass<UARSStatisticsComponent>();
    if (!StatsComp)
    {
        return false;
    }

    UNomadSurvivalNeedsComponent* SurvivalComp = GetSurvivalNeedsComponent(Character);
    if (!SurvivalComp)
    {
        return false;
    }

    // Simulate severe starvation by setting hunger to 0
    const FGameplayTag HungerTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Statistics.Hunger"));
    const float CurrentHunger = StatsComp->GetCurrentValueForStatitstic(HungerTag);

    // Set hunger to 0 to trigger severe starvation
    StatsComp->ModifyStatistic(HungerTag, -CurrentHunger);

    // Trigger survival evaluation to apply effects
    // Note: In practice, this would happen during the normal survival tick
    // For testing, we might need to manually trigger the evaluation

    return true;
}

void UNomadStatusEffectIntegrationValidator::CleanupTestEffects(ACharacter* Character)
{
    if (!Character)
    {
        return;
    }

    UNomadSurvivalNeedsComponent* SurvivalComp = GetSurvivalNeedsComponent(Character);
    if (SurvivalComp)
    {
        // Remove all survival effects
        SurvivalComp->RemoveAllSurvivalEffects();
    }

    UNomadStatusEffectManagerComponent* SEManager = GetStatusEffectManager(Character);
    if (SEManager)
    {
        // Remove any test blocking tags
        const FGameplayTag JumpBlockTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Block.Jump"));
        SEManager->RemoveBlockingTag(JumpBlockTag);

        const FGameplayTag SprintBlockTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Block.Sprint"));
        SEManager->RemoveBlockingTag(SprintBlockTag);
    }

    // Restore hunger to a safe level
    UARSStatisticsComponent* StatsComp = Character->FindComponentByClass<UARSStatisticsComponent>();
    if (StatsComp)
    {
        const FGameplayTag HungerTag = FGameplayTag::RequestGameplayTag(TEXT("RPG.Statistics.Hunger"));
        const float MaxHunger = StatsComp->GetMaxValueForStatitstic(HungerTag);
        const float CurrentHunger = StatsComp->GetCurrentValueForStatitstic(HungerTag);

        // Restore to 75% of max hunger
        const float TargetHunger = MaxHunger * 0.75f;
        const float HungerToAdd = TargetHunger - CurrentHunger;

        if (HungerToAdd > 0.0f)
        {
            StatsComp->ModifyStatistic(HungerTag, HungerToAdd);
        }
    }
}