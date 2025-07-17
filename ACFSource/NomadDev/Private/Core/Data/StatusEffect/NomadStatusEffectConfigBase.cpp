// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Data/StatusEffect/NomadStatusEffectConfigBase.h"
#include "Core/Debug/NomadLogCategories.h"
#include "Misc/DataValidation.h"

UNomadStatusEffectConfigBase::UNomadStatusEffectConfigBase()
{
    // Set default values for all properties
    EffectName = FText::FromString(TEXT("Unnamed Effect"));
    Description = FText::FromString(TEXT("No description provided"));
    Icon = nullptr;
    EffectTag = FGameplayTag();
    Category = ENomadStatusCategory::Neutral;

    bShowNotifications = true;
    bCanStack = false;
    MaxStacks = 1;

    StartSound = nullptr;
    EndSound = nullptr;
    AttachedEffect = nullptr;
    AttachedNiagaraEffect = nullptr;

    // Hybrid system
    ApplicationMode = EStatusEffectApplicationMode::StatModification;
    DamageTypeClass = nullptr;
}

bool UNomadStatusEffectConfigBase::IsConfigValid() const
{
    bool bValid = true;

    if (ApplicationMode == EStatusEffectApplicationMode::DamageEvent && DamageStatisticMods.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] DamageEvent mode but ApplyDamageTo is empty! This effect will not apply any damage."),
            *GetNameSafe(this));
        bValid = false;
    }

    // If you want to aggregate errors from GetValidationErrors():
    TArray<FString> Errors = GetValidationErrors();
    if (Errors.Num() > 0)
        bValid = false;

    return bValid;
}

TArray<FString> UNomadStatusEffectConfigBase::GetValidationErrors() const
{
    TArray<FString> Errors;

    // Basic validation for required fields
    if (EffectName.IsEmpty())
    {
        Errors.Add(TEXT("Effect name cannot be empty"));
    }

    if (!EffectTag.IsValid())
    {
        Errors.Add(TEXT("Effect tag must be valid"));
    }

    if (bCanStack && MaxStacks <= 0)
    {
        Errors.Add(TEXT("Max stacks must be > 0 when stacking is enabled"));
    }

    // Hybrid validation
    if ((ApplicationMode == EStatusEffectApplicationMode::DamageEvent || ApplicationMode == EStatusEffectApplicationMode::Both) && !DamageTypeClass)
    {
        Errors.Add(TEXT("DamageTypeClass must be set when ApplicationMode is DamageEvent or Both."));
    }

    return Errors;
}

#if WITH_EDITOR
void UNomadStatusEffectConfigBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property)
    {
        FName PropertyName = PropertyChangedEvent.Property->GetFName();

        // Clamp max stacks to minimum 1
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadStatusEffectConfigBase, MaxStacks))
        {
            MaxStacks = FMath::Max(1, MaxStacks);
        }

        // Synchronize bCanStack and MaxStacks for sanity
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadStatusEffectConfigBase, MaxStacks) && MaxStacks == 1)
        {
            bCanStack = false;
        }

        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadStatusEffectConfigBase, MaxStacks) && MaxStacks > 1)
        {
            bCanStack = true;
        }
    }
}

EDataValidationResult UNomadStatusEffectConfigBase::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (ApplicationMode == EStatusEffectApplicationMode::DamageEvent && DamageStatisticMods.Num() == 0)
    {
        Context.AddError(
            FText::Format(
                NSLOCTEXT("NomadStatusEffect", "ConfigNoDamageArray", "In DamageEvent mode, ApplyDamageTo must have at least one entry in '{0}'."),
                FText::FromString(GetName())
            )
        );
        Result = EDataValidationResult::Invalid;
    }

    TArray<FString> ErrorStrings = GetValidationErrors();

    // Add errors to the validation context (editor will display these)
    for (const FString& Error : ErrorStrings)
    {
        Context.AddError(FText::FromString(Error));
        Result = EDataValidationResult::Invalid;
    }

    // Add warnings for missing but non-critical data
    if (Description.IsEmpty())
    {
        Context.AddWarning(FText::FromString(TEXT("Description is empty - consider adding a description for designers")));
    }

    if (Icon.IsNull())
    {
        Context.AddWarning(FText::FromString(TEXT("No icon set - effect will use default icon in UI")));
    }

    if (Result == EDataValidationResult::Valid)
    {
        UE_LOG_AFFLICTION(Verbose, TEXT("[CONFIG] Base effect config validation passed: %s"),
                          *EffectName.ToString());
    }

    return Result;
}
#endif