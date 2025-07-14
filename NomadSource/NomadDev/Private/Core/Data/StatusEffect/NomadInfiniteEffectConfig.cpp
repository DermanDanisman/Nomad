// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Data/StatusEffect/NomadInfiniteEffectConfig.h"
#include "Core/StatusEffect/NomadBaseStatusEffect.h"
#include "Core/Debug/NomadLogCategories.h"
#include "Engine/Engine.h"
#include "Misc/DataValidation.h"

UNomadInfiniteEffectConfig::UNomadInfiniteEffectConfig()
{
    // Set default values for infinite effects
    bHasPeriodicTick = false;
    TickInterval = 5.0f;
    bCanBeManuallyRemoved = true;
    bPersistThroughSaveLoad = true;
    bTriggerActivationChainEffects = false;
    bTriggerDeactivationChainEffects = false;
    bShowInfinitySymbolInUI = true;
    bShowTickNotifications = false;
    DisplayPriority = 50;
    MaxStackSize = 1;
    bShowNotifications = true;
    Category = ENomadStatusCategory::Neutral;
    PersistentAttributeModifier = FAttributesSetModifier();
    PersistentAttributeModifier.Guid = FGuid::NewGuid();
    DeveloperNotes = TEXT("Infinite duration status effect - persists until manually removed.");

    // Hybrid
    ApplicationMode = EStatusEffectApplicationMode::StatModification;
    DamageTypeClass = nullptr;
}

bool UNomadInfiniteEffectConfig::IsConfigValid() const
{
    if (!Super::IsConfigValid())
    {
        return false;
    }

    if (bHasPeriodicTick && TickInterval <= 0.0f)
    {
        return false;
    }

    if (MaxStackSize < 0)
    {
        return false;
    }

    if (DisplayPriority < 0 || DisplayPriority > 100)
    {
        return false;
    }

    if (bTriggerActivationChainEffects)
    {
        for (const TSoftClassPtr<UNomadBaseStatusEffect>& ChainEffect : ActivationChainEffects)
        {
            if (ChainEffect.IsNull())
            {
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
                return false;
            }
        }
    }

    // Hybrid validation for infinite effects can be added here if needed

    return true;
}

TArray<FString> UNomadInfiniteEffectConfig::GetValidationErrors() const
{
    TArray<FString> Errors;

    if (bHasPeriodicTick && TickInterval <= 0.0f)
    {
        Errors.Add(TEXT("Tick interval must be greater than 0 when periodic ticking is enabled"));
    }

    if (MaxStackSize < 0)
    {
        Errors.Add(TEXT("Max stacks cannot be negative"));
    }

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

    if (!PersistentAttributeModifier.Guid.IsValid())
    {
        Errors.Add(TEXT("Persistent attribute modifier must have a valid GUID"));
    }

    // Hybrid: warn if nothing to apply
    if (ApplicationMode == EStatusEffectApplicationMode::StatModification &&
        OnActivationStatModifications.Num() == 0 &&
        OnTickStatModifications.Num() == 0 &&
        OnDeactivationStatModifications.Num() == 0)
    {
        Errors.Add(TEXT("No stat modifications specified for infinite effect in StatModification mode."));
    }
    if ((ApplicationMode == EStatusEffectApplicationMode::DamageEvent || ApplicationMode == EStatusEffectApplicationMode::Both) && (!DamageTypeClass))
    {
        Errors.Add(TEXT("DamageTypeClass must be set when ApplicationMode is DamageEvent or Both."));
    }

    return Errors;
}

FString UNomadInfiniteEffectConfig::GetEffectDescription() const
{
    FString LocalDescription = FString::Printf(TEXT("Infinite Effect: %s\n"), *EffectName.ToString());
    
    LocalDescription += FString::Printf(TEXT("Category: %s\n"), 
                                  *StaticEnum<ENomadStatusCategory>()->GetNameStringByValue((int64)Category));

    if (bHasPeriodicTick)
    {
        LocalDescription += FString::Printf(TEXT("Ticks every %.1f seconds\n"), TickInterval);
    }
    else
    {
        LocalDescription += TEXT("No periodic ticking\n");
    }

    LocalDescription += FString::Printf(TEXT("Manual removal: %s\n"), 
                                  bCanBeManuallyRemoved ? TEXT("Allowed") : TEXT("Restricted"));

    LocalDescription += FString::Printf(TEXT("Persists through save/load: %s\n"), 
                                  bPersistThroughSaveLoad ? TEXT("Yes") : TEXT("No"));

    if (MaxStackSize == 0)
    {
        LocalDescription += TEXT("Unlimited stacking\n");
    }
    else if (MaxStackSize == 1)
    {
        LocalDescription += TEXT("No stacking (single instance)\n");
    }
    else
    {
        LocalDescription += FString::Printf(TEXT("Max %d stacks\n"), MaxStackSize);
    }

    int32 TotalMods = GetTotalStatModificationCount();
    if (TotalMods > 0)
    {
        LocalDescription += FString::Printf(TEXT("%d total stat modifications\n"), TotalMods);
    }

    if (bTriggerActivationChainEffects && ActivationChainEffects.Num() > 0)
    {
        LocalDescription += FString::Printf(TEXT("%d activation chain effects\n"), ActivationChainEffects.Num());
    }

    if (bTriggerDeactivationChainEffects && DeactivationChainEffects.Num() > 0)
    {
        LocalDescription += FString::Printf(TEXT("%d deactivation chain effects\n"), DeactivationChainEffects.Num());
    }

    if (!DeveloperNotes.IsEmpty())
    {
        LocalDescription += FString::Printf(TEXT("\nNotes: %s"), *DeveloperNotes);
    }

    return LocalDescription;
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
           OnDeactivationStatModifications.Num();
}

#if WITH_EDITOR
void UNomadInfiniteEffectConfig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property)
    {
        FName PropertyName = PropertyChangedEvent.Property->GetFName();

        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadInfiniteEffectConfig, PersistentAttributeModifier))
        {
            if (!PersistentAttributeModifier.Guid.IsValid())
            {
                PersistentAttributeModifier.Guid = FGuid::NewGuid();
                UE_LOG_AFFLICTION(Log, TEXT("[CONFIG] Generated new GUID for persistent attribute modifier"));
            }
        }

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

        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadInfiniteEffectConfig, bHasPeriodicTick))
        {
            if (!bHasPeriodicTick)
            {
                OnTickStatModifications.Empty();
            }
        }
    }
}

EDataValidationResult UNomadInfiniteEffectConfig::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    TArray<FString> ErrorStrings = GetValidationErrors();
    for (const FString& Error : ErrorStrings)
    {
        Context.AddError(FText::FromString(Error));
        Result = EDataValidationResult::Invalid;
    }

    if (!bCanBeManuallyRemoved && BypassRemovalTags.IsEmpty())
    {
        Context.AddWarning(FText::FromString(TEXT("Effect cannot be removed and has no bypass tags - may be impossible to remove")));
    }

    if (bHasPeriodicTick && TickInterval < 1.0f)
    {
        Context.AddWarning(FText::FromString(TEXT("Fast infinite ticking (<1s) on permanent effect may impact performance")));
    }

    if (MaxStackSize == 0)
    {
        Context.AddWarning(FText::FromString(TEXT("Unlimited stacking may cause balance issues")));
    }

    if (Result == EDataValidationResult::Valid)
    {
        UE_LOG_AFFLICTION(Verbose, TEXT("[CONFIG] Infinite effect config validation passed: %s"), 
                          *EffectName.ToString());
    }

    return Result;
}
#endif