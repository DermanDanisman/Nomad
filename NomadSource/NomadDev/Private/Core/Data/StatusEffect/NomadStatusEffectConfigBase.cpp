// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/Data/StatusEffect/NomadStatusEffectConfigBase.h"
#include "Engine/Texture2D.h"
#include "Core/Debug/NomadLogCategories.h"

#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#include "Misc/DataValidation.h"
#endif

// =====================================================
//         CONSTRUCTOR & INITIALIZATION
// =====================================================

UNomadStatusEffectConfigBase::UNomadStatusEffectConfigBase()
{
    // Initialize basic properties
    EffectName = FText::FromString(TEXT("Unknown Effect"));
    Description = FText::FromString(TEXT("No description provided"));
    
    // Set safe defaults for all properties
    EffectTag = FGameplayTag::EmptyTag;
    Category = ENomadStatusCategory::Neutral;
    ApplicationMode = EStatusEffectApplicationMode::StatModification;
    DamageTypeClass = nullptr;
    
    // Behavior defaults
    bShowNotifications = true;
    bCanStack = false;
    MaxStackSize = 1;
    bCustomDamageCalculation = false;
    
    // Notification defaults
    NotificationColor = FLinearColor::Transparent;
    NotificationDuration = 4.0f;
    AppliedMessage = FText::GetEmpty();
    RemovedMessage = FText::GetEmpty();
    
    // Clear arrays
    DamageStatisticMods.Empty();
    BlockingTags.Empty();
}

// =====================================================
//         NOTIFICATION SYSTEM
// =====================================================

UTexture2D* UNomadStatusEffectConfigBase::GetNotificationIcon() const
{
    // Load and return the icon, or nullptr if not set
    if (Icon.IsNull())
    {
        return nullptr;
    }
    
    return Icon.LoadSynchronous();
}

FLinearColor UNomadStatusEffectConfigBase::GetNotificationColor() const
{
    // Return custom color if set (alpha > 0), otherwise use category color
    if (NotificationColor.A > 0.0f)
    {
        return NotificationColor;
    }
    
    // Fallback to category-based colors
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
    // Return custom message if set, otherwise generate default
    if (bWasAdded)
    {
        if (!AppliedMessage.IsEmpty())
        {
            return AppliedMessage;
        }
        
        // Generate default applied message
        return FText::Format(
            FText::FromString(TEXT("{0} applied")),
            EffectName
        );
    }
    else
    {
        if (!RemovedMessage.IsEmpty())
        {
            return RemovedMessage;
        }
        
        // Generate default removed message
        return FText::Format(
            FText::FromString(TEXT("{0} removed")),
            EffectName
        );
    }
}

// =====================================================
//         VALIDATION SYSTEM
// =====================================================

bool UNomadStatusEffectConfigBase::IsConfigValid() const
{
    // Check all validation errors
    TArray<FString> Errors = GetValidationErrors();
    return Errors.Num() == 0;
}

TArray<FString> UNomadStatusEffectConfigBase::GetValidationErrors() const
{
    TArray<FString> Errors;
    
    // Check required fields
    if (EffectName.IsEmpty())
    {
        Errors.Add(TEXT("Effect Name is required"));
    }
    
    if (!EffectTag.IsValid())
    {
        Errors.Add(TEXT("Effect Tag is required and must be valid"));
    }
    
    // Check stacking configuration
    if (bCanStack && MaxStackSize < 1)
    {
        Errors.Add(TEXT("Max Stack Size must be at least 1 when stacking is enabled"));
    }
    
    // Check damage event mode requirements
    if (ApplicationMode == EStatusEffectApplicationMode::DamageEvent || 
        ApplicationMode == EStatusEffectApplicationMode::Both)
    {
        if (!DamageTypeClass)
        {
            Errors.Add(TEXT("Damage Type Class is required when using Damage Event mode"));
        }
        
        if (DamageStatisticMods.Num() == 0)
        {
            Errors.Add(TEXT("Damage Statistic Modifications are required when using Damage Event mode"));
        }
    }
    
    // Check notification duration
    if (NotificationDuration <= 0.0f)
    {
        Errors.Add(TEXT("Notification Duration must be greater than 0"));
    }
    
    return Errors;
}

#if WITH_EDITOR
// =====================================================
//         EDITOR VALIDATION
// =====================================================

void UNomadStatusEffectConfigBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    const FName PropertyName = PropertyChangedEvent.GetPropertyName();
    
    // Auto-correct common issues
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadStatusEffectConfigBase, MaxStackSize))
    {
        // Ensure stack size is at least 1
        MaxStackSize = FMath::Max(1, MaxStackSize);
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadStatusEffectConfigBase, NotificationDuration))
    {
        // Ensure duration is positive
        NotificationDuration = FMath::Max(0.1f, NotificationDuration);
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UNomadStatusEffectConfigBase, bCanStack))
    {
        // Reset max stack size when disabling stacking
        if (!bCanStack)
        {
            MaxStackSize = 1;
        }
    }
}

EDataValidationResult UNomadStatusEffectConfigBase::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);
    
    // Get all validation errors
    TArray<FString> Errors = GetValidationErrors();
    
    // Report each error to the validation context
    for (const FString& Error : Errors)
    {
        Context.AddError(FText::FromString(Error));
        Result = EDataValidationResult::Invalid;
    }
    
    // Log validation result
    if (Result == EDataValidationResult::Invalid)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[CONFIG] Validation failed for %s: %d errors"), 
                          *GetName(), Errors.Num());
    }
    
    return Result;
}

#endif // WITH_EDITOR