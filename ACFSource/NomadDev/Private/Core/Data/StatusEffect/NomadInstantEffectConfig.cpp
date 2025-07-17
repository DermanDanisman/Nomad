// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Data/StatusEffect/NomadInstantEffectConfig.h"
#include "Core/Debug/NomadLogCategories.h"
#include "Misc/DataValidation.h"

UNomadInstantEffectConfig::UNomadInstantEffectConfig()
{
    NotificationDuration = 2.0f;
    bTriggerScreenEffects = false;
    bTriggerChainEffects = false;
    
    // Initialize arrays
    InstantStatModifications.Empty();
    ChainEffects.Empty();

    // Hybrid: defaults are set in base
}

#if WITH_EDITOR
EDataValidationResult UNomadInstantEffectConfig::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    // Get our validation errors
    TArray<FString> ErrorStrings = GetValidationErrors();
    
    // Add errors to the context
    for (const FString& Error : ErrorStrings)
    {
        Context.AddError(FText::FromString(Error));
        Result = EDataValidationResult::Invalid;
    }

    // Hybrid validation: warn if nothing to apply
    if (ApplicationMode == EStatusEffectApplicationMode::StatModification && InstantStatModifications.Num() == 0)
    {
        Context.AddWarning(FText::FromString(TEXT("No stat modifications specified for instant effect in StatModification mode.")));
    }
    if ((ApplicationMode == EStatusEffectApplicationMode::DamageEvent || ApplicationMode == EStatusEffectApplicationMode::Both) && (!DamageTypeClass))
    {
        Context.AddWarning(FText::FromString(TEXT("DamageTypeClass should be set for DamageEvent or Both modes.")));
    }

    // Add warnings for instant-specific issues
    if (NotificationDuration > 10.0f)
    {
        Context.AddWarning(FText::FromString(TEXT("Very long notification duration (>10s) may clutter UI")));
    }

    if (bTriggerChainEffects && ChainEffects.Num() > 10)
    {
        Context.AddWarning(FText::FromString(TEXT("Many chain effects (>10) may impact performance")));
    }

    if (Result == EDataValidationResult::Valid)
    {
        UE_LOG_AFFLICTION(Verbose, TEXT("[CONFIG] Instant effect config validation passed: %s"), 
                          *EffectName.ToString());
    }

    return Result;
}
#endif