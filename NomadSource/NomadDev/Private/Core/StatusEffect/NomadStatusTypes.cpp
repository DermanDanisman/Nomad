// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/StatusEffect/NomadStatusTypes.h"
#include "Core/StatusEffect/NomadBaseStatusEffect.h"

FNomadStatusEffect UNomadStatusTypes::CreateNomadStatusEffect(const FStatusEffect& ACFStatusEffect, ENomadStatusCategory Category)
{
    FNomadStatusEffect NomadEffect(ACFStatusEffect);
    NomadEffect.Category = Category;

    // In future: Extract category from the effect instance itself if more dynamic logic is needed.
    if (UNomadBaseStatusEffect* NomadBaseEffect = Cast<UNomadBaseStatusEffect>(ACFStatusEffect.effectInstance))
    {
        // Optionally, assign category from the effect's config if needed.
        // For now, keeps provided Category parameter.
    }

    return NomadEffect;
}

TArray<FNomadStatusEffect> UNomadStatusTypes::ConvertACFStatusEffects(const TArray<FStatusEffect>& ACFStatusEffects)
{
    TArray<FNomadStatusEffect> NomadEffects;
    NomadEffects.Reserve(ACFStatusEffects.Num());

    for (const FStatusEffect& ACFEffect : ACFStatusEffects)
    {
        NomadEffects.Add(CreateNomadStatusEffect(ACFEffect));
    }

    return NomadEffects;
}

FLinearColor UNomadStatusTypes::GetCategoryColor(ENomadStatusCategory Category)
{
    switch (Category)
    {
    case ENomadStatusCategory::Positive:
        return FLinearColor(0.0f, 1.0f, 0.0f, 1.0f); // Green
    case ENomadStatusCategory::Negative:
        return FLinearColor(1.0f, 0.0f, 0.0f, 1.0f); // Red
    case ENomadStatusCategory::Neutral:
    default:
        return FLinearColor(1.0f, 1.0f, 1.0f, 1.0f); // White
    }
}

TArray<FNomadStatusEffect> UNomadStatusTypes::FilterByCategory(const TArray<FNomadStatusEffect>& StatusEffects, ENomadStatusCategory Category)
{
    TArray<FNomadStatusEffect> FilteredEffects;

    for (const FNomadStatusEffect& Effect : StatusEffects)
    {
        if (Effect.Category == Category)
        {
            FilteredEffects.Add(Effect);
        }
    }

    return FilteredEffects;
}