// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Data/StatusEffect/NomadTimedEffectConfig.h"
#include "Core/Debug/NomadLogCategories.h"
#include "Misc/DataValidation.h"

UNomadTimedEffectConfig::UNomadTimedEffectConfig()
{
    bIsPeriodic = false;
    TickInterval = 1.0f;
    DurationMode = EEffectDurationMode::Duration;
    EffectDuration = 10.0f;
    NumTicks = 5;

    bTriggerActivationChainEffects = false;
    bTriggerDeactivationChainEffects = false;

    // Initialize stat modification arrays
    OnStartStatModifications.Empty();
    OnTickStatModifications.Empty();
    OnEndStatModifications.Empty();
    AttributeModifier = FAttributesSetModifier();

    ActivationChainEffects.Empty();
    DeactivationChainEffects.Empty();

    // Hybrid default
    ApplicationMode = EStatusEffectApplicationMode::StatModification;
    DamageTypeClass = nullptr;
}

#if WITH_EDITOR
EDataValidationResult UNomadTimedEffectConfig::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    // Gather validation errors from this class and base
    TArray<FString> ErrorStrings = GetValidationErrors();
    for (const FString& Error : ErrorStrings)
    {
        Context.AddError(FText::FromString(Error));
        Result = EDataValidationResult::Invalid;
    }

    // Hybrid validation: warn if nothing to apply
    if (ApplicationMode == EStatusEffectApplicationMode::StatModification &&
        OnStartStatModifications.Num() == 0 &&
        OnTickStatModifications.Num() == 0 &&
        OnEndStatModifications.Num() == 0)
    {
        Context.AddWarning(FText::FromString(TEXT("No stat modifications specified for timed effect in StatModification mode.")));
    }
    if ((ApplicationMode == EStatusEffectApplicationMode::DamageEvent || ApplicationMode == EStatusEffectApplicationMode::Both) && (!DamageTypeClass))
    {
        Context.AddWarning(FText::FromString(TEXT("DamageTypeClass should be set for DamageEvent or Both modes.")));
    }

    // Warnings for tick interval and duration
    if (bIsPeriodic && TickInterval < 0.05f)
    {
        Context.AddWarning(FText::FromString(TEXT("Very fast ticking (<0.05s) may impact performance.")));
    }
    if (bIsPeriodic && DurationMode == EEffectDurationMode::Duration && EffectDuration < 0.1f)
    {
        Context.AddWarning(FText::FromString(TEXT("Very short effect duration (<0.1s) may be unnoticeable.")));
    }
    if (bIsPeriodic && DurationMode == EEffectDurationMode::Ticks && NumTicks <= 0)
    {
        Context.AddWarning(FText::FromString(TEXT("Number of ticks must be > 0.")));
    }

    // Chain effect warnings
    if (bTriggerActivationChainEffects && ActivationChainEffects.Num() > 10)
    {
        Context.AddWarning(FText::FromString(TEXT("Many activation chain effects (>10) may impact performance.")));
    }
    if (bTriggerDeactivationChainEffects && DeactivationChainEffects.Num() > 10)
    {
        Context.AddWarning(FText::FromString(TEXT("Many deactivation chain effects (>10) may impact performance.")));
    }

    if (Result == EDataValidationResult::Valid)
    {
        UE_LOG_AFFLICTION(Verbose, TEXT("[CONFIG] Timed effect config validation passed: %s"), 
                          *EffectName.ToString());
    }

    return Result;
}
#endif