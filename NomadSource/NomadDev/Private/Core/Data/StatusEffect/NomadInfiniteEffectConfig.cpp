// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Data/StatusEffect/NomadInfiniteEffectConfig.h"
#include "Core/StatusEffect/NomadBaseStatusEffect.h"
#include "Core/Debug/NomadLogCategories.h"
#include "Engine/Engine.h"
#include "Misc/DataValidation.h"

// =====================================================
//         CONSTRUCTOR & INITIALIZATION
// =====================================================

UNomadInfiniteEffectConfig::UNomadInfiniteEffectConfig()
{
    // Set default values for infinite effects
    bHasPeriodicTick = false;
    TickInterval = 5.0f;
    bCanBeManuallyRemoved = true;
    bPersistThroughSaveLoad = true;
    
    // Initialize chain effects
    bTriggerActivationChainEffects = false;
    bTriggerDeactivationChainEffects = false;
    ActivationChainEffects.Empty();
    DeactivationChainEffects.Empty();
    
    // Initialize UI settings
    bShowInfinitySymbolInUI = true;
    bShowTickNotifications = false;
    DisplayPriority = 50;
    
    // Initialize stat modifications
    OnActivationStatModifications.Empty();
    OnTickStatModifications.Empty();
    OnDeactivationStatModifications.Empty();
    
    // Initialize persistent modifier with valid GUID
    PersistentAttributeModifier = FAttributesSetModifier();
    PersistentAttributeModifier.Guid = FGuid::NewGuid();
    
    // Set appropriate defaults for infinite effects
    bCanStack = false;
    MaxStackSize = 1;
    bShowNotifications = true;
    Category = ENomadStatusCategory::Neutral;
    
    // Hybrid system defaults
    ApplicationMode = EStatusEffectApplicationMode::StatModification;
    DamageTypeClass = nullptr;
    
    // Documentation
    DeveloperNotes = TEXT("Infinite duration status effect - persists until manually removed.");
    
    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[CONFIG] Infinite effect config constructed"));
}

// =====================================================
//         VALIDATION SYSTEM
// =====================================================

bool UNomadInfiniteEffectConfig::IsConfigValid() const
{
    // Start with base validation
    if (!Super::IsConfigValid())
    {
        return false;
    }

    // Validate tick settings
    if (bHasPeriodicTick && TickInterval <= 0.0f)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[CONFIG] Tick interval must be > 0 when periodic ticking enabled"));
        return false;
    }

    // Validate display settings
    if (DisplayPriority < 0 || DisplayPriority > 100)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[CONFIG] Display priority must be between 0-100"));
        return false;
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

    // Validate persistent modifier GUID
    if (!PersistentAttributeModifier.Guid.IsValid())
    {
        UE_LOG_AFFLICTION(Error, TEXT("[CONFIG] Persistent attribute modifier must have valid GUID"));
        return false;
    }

    return true;
}

TArray<FString> UNomadInfiniteEffectConfig::GetValidationErrors() const
{
    TArray<FString> Errors;
    
    // Get base validation errors
    Errors.Append(Super::GetValidationErrors());

    // Validate tick settings
    if (bHasPeriodicTick && TickInterval <= 0.0f)
    {
        Errors.Add(TEXT("Tick interval must be greater than 0 when periodic ticking is enabled"));
    }

    // Validate display settings
    if (DisplayPriority < 0 || DisplayPriority > 100)
    {
        Errors.Add(TEXT("Display priority must be between 0 and 100"));
    }

    // Validate chain effects
    if (bTriggerActivationChainEffects)
    {
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
        for (int32 i = 0; i < DeactivationChainEffects.Num(); i++)
        {
            if (DeactivationChainEffects[i].IsNull())
            {
                Errors.Add(FString::Printf(TEXT("Deactivation chain effect at index %d is null"), i));
            }
        }
    }

    // Validate persistent modifier
    if (!PersistentAttributeModifier.Guid.IsValid())
    {
        Errors.Add(TEXT("Persistent attribute modifier must have a valid GUID"));
    }

    // Hybrid system validation
    if (ApplicationMode == EStatusEffectApplicationMode::StatModification &&
        OnActivationStatModifications.Num() == 0 &&
        OnTickStatModifications.Num() == 0 &&
        OnDeactivationStatModifications.Num() == 0 &&
        PersistentAttributeModifier.PrimaryAttributesMod.Num() == 0 &&
        PersistentAttributeModifier.AttributesMod.Num() == 0 &&
        PersistentAttributeModifier.StatisticsMod.Num() == 0)
    {
        Errors.Add(TEXT("No stat modifications or persistent modifiers specified for infinite effect in StatModification mode"));
    }

    return Errors;
}

// =====================================================
//         UTILITY FUNCTIONS
// =====================================================

FString UNomadInfiniteEffectConfig::GetEffectDescription() const
{
    FString Description = FString::Printf(TEXT("Infinite Effect: %s\n"), *EffectName.ToString());
    
    Description += FString::Printf(TEXT("Category: %s\n"), 
                                  *StaticEnum<ENomadStatusCategory>()->GetNameStringByValue((int64)Category));

    if (bHasPeriodicTick)
    {
        Description += FString::Printf(TEXT("Ticks every %.1f seconds\n"), TickInterval);
    }
    else
    {
        Description += TEXT("No periodic ticking\n");
    }

    Description += FString::Printf(TEXT("Manual removal: %s\n"), 
                                  bCanBeManuallyRemoved ? TEXT("Allowed") : TEXT("Restricted"));

    Description += FString::Printf(TEXT("Persists through save/load: %s\n"), 
                                  bPersistThroughSaveLoad ? TEXT("Yes") : TEXT("No"));

    if (bCanStack)
    {
        if (MaxStackSize == 0)
        {
            Description += TEXT("Unlimited stacking\n");
        }
        else
        {
            Description += FString::Printf(TEXT("Max %d stacks\n"), MaxStackSize);
        }
    }
    else
    {
        Description += TEXT("No stacking (single instance)\n");
    }

    const int32 TotalMods = GetTotalStatModificationCount();
    if (TotalMods > 0)
    {
        Description += FString::Printf(TEXT("%d total stat modifications\n"), TotalMods);
    }

    if (bTriggerActivationChainEffects && ActivationChainEffects.Num() > 0)
    {
        Description += FString::Printf(TEXT("%d activation chain effects\n"), ActivationChainEffects.Num());
    }

    if (bTriggerDeactivationChainEffects && DeactivationChainEffects.Num() > 0)
    {
        Description += FString::Printf(TEXT("%d deactivation chain effects\n"), DeactivationChainEffects.Num());
    }

    if (!DeveloperNotes.IsEmpty())
    {
        Description += FString::Printf(TEXT("\nNotes: %s"), *DeveloperNotes);
    }

    return Description;
}

bool UNomadInfiniteEffectConfig::CanBeRemovedByTag(const FGameplayTag& RemovalTag) const
{
    if (!bCanBeManuallyRemoved)
    {
        return BypassRemovalTags.HasTag(RemovalTag);
    }
    return true;
}

int32 UNomadInfiniteEffectConfig::GetTotalStatModificationCount() const
{
    return OnActivationStatModifications.Num() + 
           OnTickStatModifications.Num() + 
           OnDeactivationStatModifications.Num() +
           PersistentAttributeModifier.PrimaryAttributesMod.Num() +
           PersistentAttributeModifier.AttributesMod.Num() +
           PersistentAttributeModifier.StatisticsMod.Num();
}

#if WITH_EDITOR
void UNomadInfiniteEffectConfig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();

        // Ensure persistent modifier has valid GUID
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadInfiniteEffectConfig, PersistentAttributeModifier))
        {
            if (!PersistentAttributeModifier.Guid.IsValid())
            {
                PersistentAttributeModifier.Guid = FGuid::NewGuid();
                UE_LOG_AFFLICTION(Log, TEXT("[CONFIG] Generated new GUID for persistent attribute modifier"));
            }
        }

        // Clear chain effects when disabled
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadInfiniteEffectConfig, bTriggerActivationChainEffects))
        {
            if (!bTriggerActivationChainEffects)
            {
                ActivationChainEffects.Empty();
            }
        }

        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadInfiniteEffectConfig, bTriggerDeactivationChainEffects))
        {
            if (!bTriggerDeactivationChainEffects)
            {
                DeactivationChainEffects.Empty();
            }
        }

        // Clear tick modifications when ticking disabled
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadInfiniteEffectConfig, bHasPeriodicTick))
        {
            if (!bHasPeriodicTick)
            {
                OnTickStatModifications.Empty();
                bShowTickNotifications = false;
            }
        }

        // Validate tick interval
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadInfiniteEffectConfig, TickInterval))
        {
            TickInterval = FMath::Max(0.1f, TickInterval);
        }

        // Validate display priority
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadInfiniteEffectConfig, DisplayPriority))
        {
            DisplayPriority = FMath::Clamp(DisplayPriority, 0, 100);
        }
    }
}

EDataValidationResult UNomadInfiniteEffectConfig::IsDataValid(FDataValidationContext& Context) const
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

    // Add warnings for infinite-specific issues
    if (!bCanBeManuallyRemoved && BypassRemovalTags.IsEmpty())
    {
        Context.AddWarning(FText::FromString(TEXT("Effect cannot be removed and has no bypass tags - may be impossible to remove")));
    }

    if (bHasPeriodicTick && TickInterval < 1.0f)
    {
        Context.AddWarning(FText::FromString(TEXT("Fast infinite ticking (<1s) on permanent effect may impact performance")));
    }

    if (bCanStack && MaxStackSize == 0)
    {
        Context.AddWarning(FText::FromString(TEXT("Unlimited stacking may cause balance issues")));
    }

    if (bTriggerActivationChainEffects && ActivationChainEffects.Num() > 5)
    {
        Context.AddWarning(FText::FromString(TEXT("Many activation chain effects (>5) may impact performance")));
    }

    if (Result == EDataValidationResult::Valid)
    {
        UE_LOG_AFFLICTION(Verbose, TEXT("[CONFIG] Infinite effect config validation passed: %s"), 
                          *EffectName.ToString());
    }

    return Result;
}
#endif