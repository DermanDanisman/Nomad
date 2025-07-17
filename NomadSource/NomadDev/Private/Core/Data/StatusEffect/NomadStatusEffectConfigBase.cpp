// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Data/StatusEffect/NomadStatusEffectConfigBase.h"
#include "Core/StatusEffect/NomadStatusTypes.h"
#include "Core/Debug/NomadLogCategories.h"
#include "Misc/DataValidation.h"

// =====================================================
//         CONSTRUCTOR & INITIALIZATION
// =====================================================

UNomadStatusEffectConfigBase::UNomadStatusEffectConfigBase()
{
    // Initialize basic info
    EffectName = FText::FromString(TEXT("Unnamed Effect"));
    Description = FText::FromString(TEXT("No description provided"));
    Icon = nullptr;
    EffectTag = FGameplayTag();
    Category = ENomadStatusCategory::Neutral;

    // Initialize application mode
    ApplicationMode = EStatusEffectApplicationMode::StatModification;
    DamageTypeClass = nullptr;
    bCustomDamageCalculation = false;
    DamageStatisticMods.Empty();

    // Initialize behavior
    bShowNotifications = true;
    bCanStack = false;
    MaxStackSize = 1;

    // Initialize blocking
    BlockingTags = FGameplayTagContainer();

    // Initialize audio/visual
    StartSound = nullptr;
    EndSound = nullptr;
    AttachedEffect = nullptr;
    AttachedNiagaraEffect = nullptr;

    // Initialize notifications
    NotificationColor = FLinearColor::Transparent;
    NotificationDuration = 4.0f;
    AppliedMessage = FText();
    RemovedMessage = FText();

    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[CONFIG] Base status effect config constructed"));
}

// =====================================================
//         UTILITY FUNCTIONS
// =====================================================

UTexture2D* UNomadStatusEffectConfigBase::GetNotificationIcon() const
{
    return Icon.IsValid() ? Icon.Get() : nullptr;
}

FLinearColor UNomadStatusEffectConfigBase::GetNotificationColor() const
{
    // If a custom color is set (alpha > 0), use it
    if (NotificationColor.A > 0.0f)
    {
        return NotificationColor;
    }

    // Otherwise, use category-based color
    switch (Category)
    {
        case ENomadStatusCategory::Positive:
            return FLinearColor::Green;
        case ENomadStatusCategory::Negative:
            return FLinearColor::Red;
        case ENomadStatusCategory::Neutral:
        default:
            return FLinearColor::White;
    }
}

FText UNomadStatusEffectConfigBase::GetNotificationMessage(bool bWasAdded) const
{
    // Use custom messages if provided
    if (bWasAdded && !AppliedMessage.IsEmpty())
    {
        return AppliedMessage;
    }
    
    if (!bWasAdded && !RemovedMessage.IsEmpty())
    {
        return RemovedMessage;
    }

    // Generate fallback messages
    if (bWasAdded)
    {
        return FText::Format(
            NSLOCTEXT("StatusEffect", "Applied", "You are now affected by {0}"),
            EffectName
        );
    }
    else
    {
        return FText::Format(
            NSLOCTEXT("StatusEffect", "Removed", "You recovered from {0}"),
            EffectName
        );
    }
}

// =====================================================
//         VALIDATION SYSTEM
// =====================================================

bool UNomadStatusEffectConfigBase::IsConfigValid() const
{
    // Check basic requirements
    if (EffectName.IsEmpty())
    {
        UE_LOG_AFFLICTION(Error, TEXT("[CONFIG] Effect name cannot be empty"));
        return false;
    }

    if (!EffectTag.IsValid())
    {
        UE_LOG_AFFLICTION(Error, TEXT("[CONFIG] Effect tag must be valid"));
        return false;
    }

    // Validate stacking configuration
    if (bCanStack && MaxStackSize <= 1)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[CONFIG] Max stacks must be > 1 when stacking is enabled"));
        return false;
    }

    // Validate hybrid system configuration
    switch (ApplicationMode)
    {
        case EStatusEffectApplicationMode::DamageEvent:
        case EStatusEffectApplicationMode::Both:
            {
                if (!DamageTypeClass)
                {
                    UE_LOG_AFFLICTION(Error, TEXT("[CONFIG] DamageTypeClass required for DamageEvent/Both modes"));
                    return false;
                }
                
                if (DamageStatisticMods.Num() == 0)
                {
                    UE_LOG_AFFLICTION(Error, TEXT("[CONFIG] DamageStatisticMods required for DamageEvent/Both modes"));
                    return false;
                }
                break;
            }
        case EStatusEffectApplicationMode::StatModification:
        default:
            // No additional validation needed for stat modification mode
            break;
    }

    // Validate notification settings
    if (NotificationDuration <= 0.0f)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[CONFIG] Notification duration must be > 0"));
        return false;
    }

    return true;
}

TArray<FString> UNomadStatusEffectConfigBase::GetValidationErrors() const
{
    TArray<FString> Errors;

    // Basic validation
    if (EffectName.IsEmpty())
    {
        Errors.Add(TEXT("Effect name cannot be empty"));
    }

    if (!EffectTag.IsValid())
    {
        Errors.Add(TEXT("Effect tag must be valid"));
    }

    // Stacking validation
    if (bCanStack && MaxStackSize <= 1)
    {
        Errors.Add(TEXT("Max stacks must be > 1 when stacking is enabled"));
    }

    if (MaxStackSize < 1)
    {
        Errors.Add(TEXT("Max stacks cannot be less than 1"));
    }

    // Hybrid system validation
    switch (ApplicationMode)
    {
        case EStatusEffectApplicationMode::DamageEvent:
        case EStatusEffectApplicationMode::Both:
            {
                if (!DamageTypeClass)
                {
                    Errors.Add(TEXT("DamageTypeClass must be set for DamageEvent or Both modes"));
                }
                
                if (DamageStatisticMods.Num() == 0)
                {
                    Errors.Add(TEXT("DamageStatisticMods must have at least one entry for DamageEvent or Both modes"));
                }
                
                // Validate damage stat mods
                for (int32 i = 0; i < DamageStatisticMods.Num(); i++)
                {
                    const FStatisticValue& StatMod = DamageStatisticMods[i];
                    if (!StatMod.Statistic.IsValid())
                    {
                        Errors.Add(FString::Printf(TEXT("DamageStatisticMods[%d] has invalid statistic tag"), i));
                    }
                }
                break;
            }
        case EStatusEffectApplicationMode::StatModification:
        default:
            // No additional validation for stat modification mode
            break;
    }

    // Notification validation
    if (NotificationDuration <= 0.0f)
    {
        Errors.Add(TEXT("Notification duration must be greater than 0"));
    }

    return Errors;
}

#if WITH_EDITOR
void UNomadStatusEffectConfigBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();

        // Auto-correct max stack size
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadStatusEffectConfigBase, MaxStackSize))
        {
            MaxStackSize = FMath::Max(1, MaxStackSize);
            
            // Auto-enable stacking if max stacks > 1
            if (MaxStackSize > 1)
            {
                bCanStack = true;
            }
            else if (MaxStackSize == 1)
            {
                bCanStack = false;
            }
        }

        // Auto-update stacking settings
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadStatusEffectConfigBase, bCanStack))
        {
            if (!bCanStack)
            {
                MaxStackSize = 1;
            }
            else if (MaxStackSize <= 1)
            {
                MaxStackSize = 5; // Reasonable default
            }
        }

        // Validate notification duration
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadStatusEffectConfigBase, NotificationDuration))
        {
            NotificationDuration = FMath::Max(0.1f, NotificationDuration);
        }

        // Clear damage stats if switching to StatModification mode
        if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadStatusEffectConfigBase, ApplicationMode))
        {
            if (ApplicationMode == EStatusEffectApplicationMode::StatModification)
            {
                DamageStatisticMods.Empty();
            }
        }
    }
}

EDataValidationResult UNomadStatusEffectConfigBase::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    // Get validation errors and add them to context
    const TArray<FString> ErrorStrings = GetValidationErrors();
    for (const FString& Error : ErrorStrings)
    {
        Context.AddError(FText::FromString(Error));
        Result = EDataValidationResult::Invalid;
    }

    // Add warnings for missing optional data
    if (Description.IsEmpty())
    {
        Context.AddWarning(FText::FromString(TEXT("Description is empty - consider adding for designers")));
    }

    if (Icon.IsNull())
    {
        Context.AddWarning(FText::FromString(TEXT("No icon set - effect will use default in UI")));
    }

    if (StartSound.IsNull() && EndSound.IsNull())
    {
        Context.AddWarning(FText::FromString(TEXT("No audio feedback configured")));
    }

    // Hybrid system warnings
    if (ApplicationMode == EStatusEffectApplicationMode::Both)
    {
        Context.AddWarning(FText::FromString(TEXT("Both mode applies stat mods AND damage - ensure this is intended")));
    }

    if (bCustomDamageCalculation && ApplicationMode == EStatusEffectApplicationMode::StatModification)
    {
        Context.AddWarning(FText::FromString(TEXT("Custom damage calculation enabled but using StatModification mode")));
    }

    if (Result == EDataValidationResult::Valid)
    {
        UE_LOG_AFFLICTION(Verbose, TEXT("[CONFIG] Base config validation passed: %s"), 
                          *EffectName.ToString());
    }

    return Result;
}
#endif