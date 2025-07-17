// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Data/StatusEffect/NomadTimedEffectConfig.h"
#include "Core/Debug/NomadLogCategories.h"
#include "Misc/DataValidation.h"

// =====================================================
//         CONSTRUCTOR & INITIALIZATION
// =====================================================

UNomadTimedEffectConfig::UNomadTimedEffectConfig()
{
    // Initialize timing settings
    bIsPeriodic = false;
    TickInterval = 1.0f;
    DurationMode = EEffectDurationMode::Duration;
    EffectDuration = 10.0f;
    NumTicks = 5;
    
    // Initialize chain effects
    bTriggerActivationChainEffects = false;
    bTriggerDeactivationChainEffects = false;
    ActivationChainEffects.Empty();
    DeactivationChainEffects.Empty();
    
    // Initialize stat modifications
    OnStartStatModifications.Empty();
    OnTickStatModifications.Empty();
    OnEndStatModifications.Empty();
    AttributeModifier = FAttributesSetModifier();
    
    // Initialize advanced settings
    bCanBePaused = false;
    PauseTags = FGameplayTagContainer();
    bStackingRefreshesDuration = true;
    
    // Timed effects often stack
    bCanStack = true;
    MaxStackSize = 5;
    
    // Hybrid system defaults
    ApplicationMode = EStatusEffectApplicationMode::StatModification;
    DamageTypeClass = nullptr;
    
    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[CONFIG] Timed effect config constructed"));
}

// =====================================================
//         VALIDATION SYSTEM
// =====================================================

bool UNomadTimedEffectConfig::IsConfigValid() const
{
    // Start with base validation
    if (!Super::IsConfigValid())
    {
        return false;
    }

    // Validate timing settings
    if (bIsPeriodic)
    {
        if (TickInterval <= 0.0f)
        {
            UE_LOG_AFFLICTION(Error, TEXT("[CONFIG] Tick interval must be > 0 for periodic effects"));
            return false;
        }

        if (DurationMode == EEffectDurationMode::Duration && EffectDuration <= 0.0f)
        {
            UE_LOG_AFFLICTION(Error, TEXT("[CONFIG] Effect duration must be > 0"));
            return false;
        }

        if (DurationMode == EEffectDurationMode::Ticks && NumTicks <= 0)
        {
            UE_LOG_AFFLICTION(Error, TEXT("[CONFIG] Number of ticks must be > 0"));
            return false;
        }
    }

    // Validate chain effects
    if (bTriggerActivationChainEffects)
    {
        for (const TSoftClassPtr<UNomadBaseStatusEffect>& ChainEffect : ActivationChainEffects)
        {
            if (ChainEffect.IsNull())
            {
                UE_LOG_AFFLICTION(Error, TEXT("[CONFIG] Null activation chain effect found"));
                return false;
            }
        }
    }

    if (bTriggerDeactivationChainEffects)
    {
        for (const TSoftClassPtr<UNomadBaseStatusEffect>& ChainEffect : DeactivationChainEffects)
        {
            if (ChainEffect.IsNull())
            {
                UE_LOG_AFFLICTION(Error, TEXT("[CONFIG] Null deactivation chain effect found"));
                return false;
            }
        }
    }

    return true;
}

TArray<FString> UNomadTimedEffectConfig::GetValidationErrors() const
{
    TArray<FString> Errors;
    
    // Get base validation errors
    Errors.Append(Super::GetValidationErrors());

    // Validate timing settings
    if (bIsPeriodic)
    {
        if (TickInterval <= 0.0f)
        {
            Errors.Add(TEXT("Tick interval must be greater than 0 for periodic effects"));
        }

        if (DurationMode == EEffectDurationMode::Duration && EffectDuration <= 0.0f)
        {
            Errors.Add(TEXT("Effect duration must be greater than 0"));
        }

        if (DurationMode == EEffectDurationMode::Ticks && NumTicks <= 0)
        {
            Errors.Add(TEXT("Number of ticks must be greater than 0"));
        }

        // Check for unreasonable combinations
        if (DurationMode == EEffectDurationMode::Duration && EffectDuration < TickInterval)
        {
            Errors.Add(TEXT("Effect duration is shorter than tick interval - effect may not tick"));
        }
    }

    // Validate chain effects
    if (bTriggerActivationChainEffects)
    {
        if (ActivationChainEffects.Num() == 0)
        {
            Errors.Add(TEXT("Activation chain effects enabled but no effects specified"));
        }
        
        for (int32 i = 0; i < ActivationChainEffects.Num(); i++)
        {
            if (ActivationChainEffects[i].IsNull())
            {
                Errors.Add(FString::Printf(TEXT("Activation chain effect at index %d is null"), i));
            }
        }
    }

    if (bTriggerDeactivationChainEffects)
    {
        if (DeactivationChainEffects.Num() == 0)
        {
            Errors.Add(TEXT("Deactivation chain effects enabled but no effects specified"));
        }
        
        for (int32 i = 0; i < DeactivationChainEffects.Num(); i++)
        {
            if (DeactivationChainEffects[i].IsNull())
            {
                Errors.Add(FString::Printf(TEXT("Deactivation chain effect at index %d is null"), i));
            }
        }
    }

    // Validate pause settings
    if (bCanBePaused && PauseTags.IsEmpty())
    {
        Errors.Add(TEXT("Effect can be paused but no pause tags specified"));
    }

    // Validate stat modifications
    if (ApplicationMode == EStatusEffectApplicationMode::StatModification &&
        OnStartStatModifications.Num() == 0 &&
        OnTickStatModifications.Num() == 0 &&
        OnEndStatModifications.Num() == 0 &&
        AttributeModifier.PrimaryAttributesMod.Num() == 0 &&
        AttributeModifier.AttributesMod.Num() == 0 &&
        AttributeModifier.StatisticsMod.Num() == 0)
    {
        Errors.Add(TEXT("No stat modifications specified for timed effect in StatModification mode"));
    }

    return Errors;
}

float UNomadTimedEffectConfig::GetTotalDuration() const
{
    if (!bIsPeriodic)
    {
        return 0.0f; // Non-periodic effects are instantaneous
    }

    if (DurationMode == EEffectDurationMode::Duration)
    {
        return EffectDuration;
    }
    else
    {
        return TickInterval * NumTicks;
    }
}

int32 UNomadTimedEffectConfig::GetTotalTickCount() const
{
    if (!bIsPeriodic)
    {
        return 0;
    }

    if (DurationMode == EEffectDurationMode::Ticks)
    {
        return NumTicks;
    }
    else
    {
        return FMath::FloorToInt(EffectDuration / TickInterval);
    }
}

#if WITH_EDITOR
void UNomadTimedEffectConfig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();

        // Clear tick modifications when periodic disabled
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadTimedEffectConfig, bIsPeriodic))
        {
            if (!bIsPeriodic)
            {
                OnTickStatModifications.Empty();
            }
        }

        // Clear chain effects when disabled
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadTimedEffectConfig, bTriggerActivationChainEffects))
        {
            if (!bTriggerActivationChainEffects)
            {
                ActivationChainEffects.Empty();
            }
        }

        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadTimedEffectConfig, bTriggerDeactivationChainEffects))
        {
            if (!bTriggerDeactivationChainEffects)
            {
                DeactivationChainEffects.Empty();
            }
        }

        // Clear pause tags when pausing disabled
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadTimedEffectConfig, bCanBePaused))
        {
            if (!bCanBePaused)
            {
                PauseTags = FGameplayTagContainer();
            }
        }

        // Validate and adjust timing values
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadTimedEffectConfig, TickInterval))
        {
            TickInterval = FMath::Max(0.01f, TickInterval);
        }

        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadTimedEffectConfig, EffectDuration))
        {
            EffectDuration = FMath::Max(0.01f, EffectDuration);
        }

        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadTimedEffectConfig, NumTicks))
        {
            NumTicks = FMath::Max(1, NumTicks);
        }

        // Auto-adjust duration mode based on values
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadTimedEffectConfig, DurationMode))
        {
            if (DurationMode == EEffectDurationMode::Duration && EffectDuration <= 0.0f)
            {
                EffectDuration = 10.0f; // Reasonable default
            }
            else if (DurationMode == EEffectDurationMode::Ticks && NumTicks <= 0)
            {
                NumTicks = 5; // Reasonable default
            }
        }
    }
}

EDataValidationResult UNomadTimedEffectConfig::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    // Add our specific validation errors
    const TArray<FString> ErrorStrings = GetValidationErrors();
    for (const FString& Error : ErrorStrings)
    {
        if (!Error.Contains(TEXT("Base")) && !Super::GetValidationErrors().Contains(Error))
        {
            Context.AddError(FText::FromString(Error));
            Result = EDataValidationResult::Invalid;
        }
    }

    // Add warnings for timed-specific issues
    if (bIsPeriodic && TickInterval < 0.1f)
    {
        Context.AddWarning(FText::FromString(TEXT("Very fast ticking (<0.1s) may impact performance")));
    }

    if (bIsPeriodic && DurationMode == EEffectDurationMode::Duration && EffectDuration < 1.0f)
    {
        Context.AddWarning(FText::FromString(TEXT("Very short effect duration (<1s) may be hard to notice")));
    }

    if (bTriggerActivationChainEffects && ActivationChainEffects.Num() > 5)
    {
        Context.AddWarning(FText::FromString(TEXT("Many activation chain effects (>5) may impact performance")));
    }

    if (bTriggerDeactivationChainEffects && DeactivationChainEffects.Num() > 5)
    {
        Context.AddWarning(FText::FromString(TEXT("Many deactivation chain effects (>5) may impact performance")));
    }

    if (bIsPeriodic && DurationMode == EEffectDurationMode::Duration && EffectDuration > 300.0f)
    {
        Context.AddWarning(FText::FromString(TEXT("Very long effect duration (>5min) may be excessive")));
    }

    if (Result == EDataValidationResult::Valid)
    {
        UE_LOG_AFFLICTION(Verbose, TEXT("[CONFIG] Timed effect config validation passed: %s"), 
                          *EffectName.ToString());
    }

    return Result;
}
#endif