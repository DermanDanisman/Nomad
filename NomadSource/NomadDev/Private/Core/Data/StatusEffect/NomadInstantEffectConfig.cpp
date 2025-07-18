// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Data/StatusEffect/NomadInstantEffectConfig.h"
#include "Core/Debug/NomadLogCategories.h"
#include "Misc/DataValidation.h"

// =====================================================
//         CONSTRUCTOR & INITIALIZATION
// =====================================================

UNomadInstantEffectConfig::UNomadInstantEffectConfig()
{
    // Instant effects have shorter notification duration by default
    NotificationDuration = 2.0f;

    // Initialize instant effect settings
    bTriggerScreenEffects = false;
    bBypassCooldowns = false;

    // Initialize stat modifications
    OnApplyStatModifications.Empty();
    TemporaryAttributeModifier = FAttributesSetModifier();

    // Initialize chain effects
    bTriggerChainEffects = false;
    ChainEffects.Empty();
    ChainEffectDelay = 0.0f;

    // Initialize feedback settings
    bShowFloatingText = true;
    bInterruptsOtherEffects = false;
    InterruptTags = FGameplayTagContainer();

    // Instant effects typically don't stack
    bCanStack = false;
    MaxStackSize = 1;

    // Hybrid system defaults
    ApplicationMode = EStatusEffectApplicationMode::StatModification;
    DamageTypeClass = nullptr;

    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[CONFIG] Instant effect config constructed"));
}

// =====================================================
//         VALIDATION SYSTEM
// =====================================================

bool UNomadInstantEffectConfig::IsConfigValid() const
{
    // Start with base validation
    if (!Super::IsConfigValid())
    {
        return false;
    }

    // Validate chain effect delay
    if (bTriggerChainEffects && ChainEffectDelay < 0.0f)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[CONFIG] Chain effect delay cannot be negative"));
        return false;
    }

    // Validate chain effects
    if (bTriggerChainEffects)
    {
        for (const TSoftClassPtr<UNomadBaseStatusEffect>& ChainEffect : ChainEffects)
        {
            if (ChainEffect.IsNull())
            {
                UE_LOG_AFFLICTION(Error, TEXT("[CONFIG] Null chain effect found"));
                return false;
            }
        }
    }

    // Validate stat modifications for instant effects
    if (ApplicationMode == EStatusEffectApplicationMode::StatModification && OnApplyStatModifications.Num() == 0)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[CONFIG] No stat modifications specified for instant effect"));
    }

    return true;
}

TArray<FString> UNomadInstantEffectConfig::GetValidationErrors() const
{
    TArray<FString> Errors;

    // Get base validation errors
    Errors.Append(Super::GetValidationErrors());

    // Validate chain effect settings
    if (bTriggerChainEffects && ChainEffectDelay < 0.0f)
    {
        Errors.Add(TEXT("Chain effect delay cannot be negative"));
    }

    if (bTriggerChainEffects && ChainEffects.Num() == 0)
    {
        Errors.Add(TEXT("Chain effects enabled but no effects specified"));
    }

    // Validate chain effects
    if (bTriggerChainEffects)
    {
        for (int32 i = 0; i < ChainEffects.Num(); i++)
        {
            if (ChainEffects[i].IsNull())
            {
                Errors.Add(FString::Printf(TEXT("Chain effect at index %d is null"), i));
            }
        }
    }

    // Validate interrupt settings
    if (bInterruptsOtherEffects && InterruptTags.IsEmpty())
    {
        Errors.Add(TEXT("Interrupt other effects enabled but no interrupt tags specified"));
    }

    // Validate stat modifications
    if (ApplicationMode == EStatusEffectApplicationMode::StatModification && OnApplyStatModifications.Num() == 0)
    {
        Errors.Add(TEXT("No stat modifications specified for instant effect in StatModification mode"));
    }

    return Errors;
}

float UNomadInstantEffectConfig::GetEffectMagnitude() const
{
    // Calculate total magnitude for UI display
    float TotalMagnitude = 0.0f;

    for (const FStatisticValue& StatMod : OnApplyStatModifications)
    {
        TotalMagnitude += FMath::Abs(StatMod.Value);
    }

    return TotalMagnitude;
}

#if WITH_EDITOR
void UNomadInstantEffectConfig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();

        // Clear chain effects when disabled
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadInstantEffectConfig, bTriggerChainEffects))
        {
            if (!bTriggerChainEffects)
            {
                ChainEffects.Empty();
                ChainEffectDelay = 0.0f;
            }
        }

        // Clear interrupt tags when disabled
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadInstantEffectConfig, bInterruptsOtherEffects))
        {
            if (!bInterruptsOtherEffects)
            {
                InterruptTags = FGameplayTagContainer();
            }
        }

        // Validate chain effect delay
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadInstantEffectConfig, ChainEffectDelay))
        {
            ChainEffectDelay = FMath::Max(0.0f, ChainEffectDelay);
        }

        // Instant effects shouldn't stack by default
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadInstantEffectConfig, bCanStack))
        {
            if (bCanStack)
            {
                UE_LOG_AFFLICTION(Warning, TEXT("[CONFIG] Stacking enabled for instant effect - consider if this is intended"));
            }
        }
    }
}

EDataValidationResult UNomadInstantEffectConfig::IsDataValid(FDataValidationContext& Context) const
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

    // Add warnings for instant-specific issues
    if (NotificationDuration > 10.0f)
    {
        Context.AddWarning(FText::FromString(TEXT("Very long notification duration (>10s) may clutter UI")));
    }

    if (bTriggerChainEffects && ChainEffects.Num() > 5)
    {
        Context.AddWarning(FText::FromString(TEXT("Many chain effects (>5) may impact performance")));
    }

    if (bCanStack)
    {
        Context.AddWarning(FText::FromString(TEXT("Stacking enabled for instant effect - verify this is intended")));
    }

    if (ChainEffectDelay > 5.0f)
    {
        Context.AddWarning(FText::FromString(TEXT("Long chain effect delay (>5s) may feel unresponsive")));
    }

    if (Result == EDataValidationResult::Valid)
    {
        UE_LOG_AFFLICTION(Verbose, TEXT("[CONFIG] Instant effect config validation passed: %s"),
                          *EffectName.ToString());
    }

    return Result;
}
#endif