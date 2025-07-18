// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "Core/StatusEffect/NomadTimedStatusEffect.h"
#include "Core/Data/StatusEffect/NomadTimedEffectConfig.h"
#include "Core/Component/NomadAfflictionComponent.h"
#include "Core/StatusEffect/NomadInfiniteStatusEffect.h"
#include "Core/StatusEffect/NomadInstantStatusEffect.h"
#include "Core/StatusEffect/SurvivalHazard/NomadSurvivalStatusEffect.h"
#include "Core/Data/StatusEffect/NomadInfiniteEffectConfig.h"
#include "Core/Debug/NomadLogCategories.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "StatusEffects/ACFBaseStatusEffect.h"

// =====================================================
//         CONSTRUCTOR & REPLICATION SETUP
// =====================================================

UNomadStatusEffectManagerComponent::UNomadStatusEffectManagerComponent()
{
    // Initialize analytics tracking
    TotalStatusEffectDamage = 0.0f;
    StatusEffectDamageTotals.Empty();

    // Initialize active effects array
    ActiveEffects.Empty();

    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[MANAGER] Status effect manager constructed"));
}

void UNomadStatusEffectManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Replicate active effects for client UI
    DOREPLIFETIME(UNomadStatusEffectManagerComponent, ActiveEffects);

    // Replicate blocking tags for client-side action validation
    DOREPLIFETIME(UNomadStatusEffectManagerComponent, ActiveBlockingTags);
}

// =====================================================
//         REPLICATION CALLBACK
// =====================================================

void UNomadStatusEffectManagerComponent::OnRep_ActiveEffects()
{
    // Called automatically on clients when ActiveEffects array is updated
    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[MANAGER] Active effects replicated to client"));

    // Notify any listening UI components
    if (AActor* Owner = GetOwner())
    {
        if (UNomadAfflictionComponent* AfflictionComp = Owner->FindComponentByClass<UNomadAfflictionComponent>())
        {
            // UI component will handle the visual updates
            AfflictionComp->OnActiveEffectsChanged();
        }
    }
}

// =====================================================
//         BLOCKING TAG SYSTEM
// =====================================================

void UNomadStatusEffectManagerComponent::AddBlockingTag(const FGameplayTag& Tag)
{
    if (!Tag.IsValid())
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[MANAGER] Attempted to add invalid blocking tag"));
        return;
    }

    ActiveBlockingTags.AddTag(Tag); // Adds or increments stack count
    UE_LOG_AFFLICTION(Verbose, TEXT("[MANAGER] Added blocking tag: %s"), *Tag.ToString());
}

void UNomadStatusEffectManagerComponent::RemoveBlockingTag(const FGameplayTag& Tag)
{
    if (!Tag.IsValid())
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[MANAGER] Attempted to remove invalid blocking tag"));
        return;
    }

    ActiveBlockingTags.RemoveTag(Tag); // Removes or decrements stack count
    UE_LOG_AFFLICTION(Verbose, TEXT("[MANAGER] Removed blocking tag: %s"), *Tag.ToString());
}

bool UNomadStatusEffectManagerComponent::HasBlockingTag(const FGameplayTag& Tag) const
{
    return ActiveBlockingTags.HasTag(Tag);
}

// =====================================================
//         SPECIALIZED APPLICATION METHODS
// =====================================================

UNomadSurvivalStatusEffect* UNomadStatusEffectManagerComponent::ApplyHazardDoTEffectWithPercent(
    const TSubclassOf<UNomadBaseStatusEffect>& EffectClass, float DotPercent)
{
    if (!EffectClass)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[MANAGER] Cannot apply hazard effect - effect class is null"));
        return nullptr;
    }

    // Create the survival effect instance
    UNomadSurvivalStatusEffect* Effect = NewObject<UNomadSurvivalStatusEffect>(GetOwner(), EffectClass);
    if (Effect)
    {
        // Configure the DoT percentage
        Effect->SetDoTPercent(DotPercent);

        // Register with active effects and activate
        AddStatusEffect(Effect, GetOwner());
        Effect->Nomad_OnStatusEffectStarts(Cast<ACharacter>(GetOwner()));

        UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Applied hazard DoT effect with %.3f%% DoT"), DotPercent * 100.0f);
    }
    else
    {
        UE_LOG_AFFLICTION(Error, TEXT("[MANAGER] Failed to create hazard effect instance"));
    }

    return Effect;
}

void UNomadStatusEffectManagerComponent::ApplyTimedStatusEffect(TSubclassOf<UNomadBaseStatusEffect> StatusEffectClass, float Duration)
{
    if (!StatusEffectClass)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[MANAGER] Cannot apply timed status effect - effect class is null"));
        return;
    }

    if (Duration <= 0.0f)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[MANAGER] Cannot apply timed status effect - duration must be > 0 (got %f)"), Duration);
        return;
    }

    // Verify this is a timed status effect class
    UObject* EffectCDO = StatusEffectClass->GetDefaultObject();
    if (!EffectCDO || !EffectCDO->IsA(UNomadTimedStatusEffect::StaticClass()))
    {
        UE_LOG_AFFLICTION(Error, TEXT("[MANAGER] ApplyTimedStatusEffect requires UNomadTimedStatusEffect class, got %s"),
                          StatusEffectClass ? *StatusEffectClass->GetName() : TEXT("null"));
        return;
    }

    // Create the timed effect instance
    UNomadTimedStatusEffect* TimedEffect = NewObject<UNomadTimedStatusEffect>(GetOwner(), StatusEffectClass);
    if (TimedEffect)
    {
        // Set the duration
        TimedEffect->SetDuration(Duration);

        // Apply the effect through the standard system
        AddStatusEffect(TimedEffect, GetOwner());
        TimedEffect->Nomad_OnStatusEffectStarts(Cast<ACharacter>(GetOwner()));

        UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Applied timed status effect %s with duration %f seconds"),
                          *StatusEffectClass->GetName(), Duration);
    }
    else
    {
        UE_LOG_AFFLICTION(Error, TEXT("[MANAGER] Failed to create timed status effect instance"));
    }
}

void UNomadStatusEffectManagerComponent::ApplyInfiniteStatusEffect(TSubclassOf<UNomadBaseStatusEffect> StatusEffectClass)
{
    if (!StatusEffectClass)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[MANAGER] Cannot apply infinite status effect - effect class is null"));
        return;
    }

    // Verify this is an infinite status effect class
    UObject* EffectCDO = StatusEffectClass->GetDefaultObject();
    if (!EffectCDO || !EffectCDO->IsA(UNomadInfiniteStatusEffect::StaticClass()))
    {
        UE_LOG_AFFLICTION(Error, TEXT("[MANAGER] ApplyInfiniteStatusEffect requires UNomadInfiniteStatusEffect class, got %s"),
                          StatusEffectClass ? *StatusEffectClass->GetName() : TEXT("null"));
        return;
    }

    // Create the infinite effect instance
    UNomadInfiniteStatusEffect* InfiniteEffect = NewObject<UNomadInfiniteStatusEffect>(GetOwner(), StatusEffectClass);
    if (InfiniteEffect)
    {
        // Apply the effect through the standard system
        AddStatusEffect(InfiniteEffect, GetOwner());
        InfiniteEffect->Nomad_OnStatusEffectStarts(Cast<ACharacter>(GetOwner()));

        UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Applied infinite status effect %s"),
                          *StatusEffectClass->GetName());
    }
    else
    {
        UE_LOG_AFFLICTION(Error, TEXT("[MANAGER] Failed to create infinite status effect instance"));
    }
}

// =====================================================
//         LIFECYCLE MANAGEMENT
// =====================================================

void UNomadStatusEffectManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Ending play, cleaning up %d active effects"), ActiveEffects.Num());

    // Clean up all active effects
    for (const FActiveEffect& Effect : ActiveEffects)
    {
        if (Effect.EffectInstance)
        {
            Effect.EffectInstance->Nomad_OnStatusEffectEnds();
        }
    }

    // Clear the array
    ActiveEffects.Empty();

    // Reset analytics
    ResetStatusEffectDamageTracking();

    Super::EndPlay(EndPlayReason);
}

// =====================================================
//         DAMAGE ANALYTICS SYSTEM
// =====================================================

void UNomadStatusEffectManagerComponent::AddStatusEffectDamage(const FGameplayTag EffectTag, const float Delta)
{
    if (!EffectTag.IsValid())
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[MANAGER] Cannot add damage for invalid effect tag"));
        return;
    }

    // Update totals
    TotalStatusEffectDamage += Delta;
    StatusEffectDamageTotals.FindOrAdd(EffectTag) += Delta;

    UE_LOG_AFFLICTION(VeryVerbose, TEXT("[MANAGER] Added %.2f damage for effect %s (total: %.2f)"),
                      Delta, *EffectTag.ToString(), StatusEffectDamageTotals[EffectTag]);
}

float UNomadStatusEffectManagerComponent::GetTotalStatusEffectDamage() const
{
    return TotalStatusEffectDamage;
}

float UNomadStatusEffectManagerComponent::GetStatusEffectDamageByTag(const FGameplayTag EffectTag) const
{
    if (const float* Found = StatusEffectDamageTotals.Find(EffectTag))
    {
        return *Found;
    }
    return 0.0f;
}

TMap<FGameplayTag, float> UNomadStatusEffectManagerComponent::GetAllStatusEffectDamages() const
{
    return StatusEffectDamageTotals;
}

void UNomadStatusEffectManagerComponent::ResetStatusEffectDamageTracking()
{
    UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Resetting damage tracking (was: %.2f total)"), TotalStatusEffectDamage);

    TotalStatusEffectDamage = 0.0f;
    StatusEffectDamageTotals.Empty();
}

// =====================================================
//         QUERY SYSTEM IMPLEMENTATION
// =====================================================

int32 UNomadStatusEffectManagerComponent::FindActiveEffectIndexByTag(const FGameplayTag Tag) const
{
    return ActiveEffects.IndexOfByPredicate([&](const FActiveEffect& Effect) {
        return Effect.Tag == Tag;
    });
}

int32 UNomadStatusEffectManagerComponent::GetStatusEffectStackCount(FGameplayTag StatusEffectTag) const
{
    const int32 Index = FindActiveEffectIndexByTag(StatusEffectTag);
    return (Index != INDEX_NONE) ? ActiveEffects[Index].StackCount : 0;
}

bool UNomadStatusEffectManagerComponent::HasStatusEffect(FGameplayTag StatusEffectTag) const
{
    return FindActiveEffectIndexByTag(StatusEffectTag) != INDEX_NONE;
}

int32 UNomadStatusEffectManagerComponent::GetStatusEffectMaxStacks(FGameplayTag StatusEffectTag) const
{
    const int32 Index = FindActiveEffectIndexByTag(StatusEffectTag);
    if (Index == INDEX_NONE) return 0;

    const FActiveEffect& Effect = ActiveEffects[Index];
    if (!Effect.EffectInstance) return 0;

    // Check effect type and get max stacks from config
    if (UNomadTimedStatusEffect* TimedEffect = Cast<UNomadTimedStatusEffect>(Effect.EffectInstance))
    {
        if (UNomadTimedEffectConfig* Config = TimedEffect->GetEffectConfig())
        {
            return Config->MaxStackSize;
        }
    }
    else if (UNomadInfiniteStatusEffect* InfiniteEffect = Cast<UNomadInfiniteStatusEffect>(Effect.EffectInstance))
    {
        if (UNomadInfiniteEffectConfig* Config = InfiniteEffect->GetEffectConfig())
        {
            return Config->MaxStackSize;
        }
    }

    return 1; // Default for non-stackable effects
}

bool UNomadStatusEffectManagerComponent::IsStatusEffectStackable(FGameplayTag StatusEffectTag) const
{
    const int32 Index = FindActiveEffectIndexByTag(StatusEffectTag);
    if (Index == INDEX_NONE) return false;

    const FActiveEffect& Effect = ActiveEffects[Index];
    if (!Effect.EffectInstance) return false;

    // Check if effect supports stacking based on config
    if (UNomadTimedStatusEffect* TimedEffect = Cast<UNomadTimedStatusEffect>(Effect.EffectInstance))
    {
        if (UNomadTimedEffectConfig* Config = TimedEffect->GetEffectConfig())
        {
            return Config->bCanStack && Config->MaxStackSize > 1;
        }
    }
    else if (UNomadInfiniteStatusEffect* InfiniteEffect = Cast<UNomadInfiniteStatusEffect>(Effect.EffectInstance))
    {
        if (UNomadInfiniteEffectConfig* Config = InfiniteEffect->GetEffectConfig())
        {
            return Config->bCanStack && Config->MaxStackSize > 1;
        }
    }

    return false; // Default: not stackable
}

EStatusEffectType UNomadStatusEffectManagerComponent::GetStatusEffectType(FGameplayTag StatusEffectTag) const
{
    const int32 Index = FindActiveEffectIndexByTag(StatusEffectTag);
    if (Index == INDEX_NONE) return EStatusEffectType::Unknown;

    const FActiveEffect& Effect = ActiveEffects[Index];
    if (!Effect.EffectInstance) return EStatusEffectType::Unknown;

    // Determine type based on class hierarchy
    if (Effect.EffectInstance->IsA<UNomadInstantStatusEffect>())
    {
        return EStatusEffectType::Instant;
    }
    else if (Effect.EffectInstance->IsA<UNomadTimedStatusEffect>())
    {
        return EStatusEffectType::Timed;
    }
    else if (Effect.EffectInstance->IsA<UNomadSurvivalStatusEffect>())
    {
        return EStatusEffectType::Survival;
    }
    else if (Effect.EffectInstance->IsA<UNomadInfiniteStatusEffect>())
    {
        return EStatusEffectType::Infinite;
    }

    return EStatusEffectType::Unknown;
}

// =====================================================
//         MAIN EXTERNAL CONTROL
// =====================================================

void UNomadStatusEffectManagerComponent::Nomad_AddStatusEffect(
    const TSubclassOf<UACFBaseStatusEffect> StatusEffectClass, AActor* Instigator)
{
    if (!StatusEffectClass)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[MANAGER] Cannot add null status effect class"));
        return;
    }

    UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Adding status effect: %s"), *StatusEffectClass->GetName());
    CreateAndApplyStatusEffect(StatusEffectClass, Instigator);
}

void UNomadStatusEffectManagerComponent::Nomad_RemoveStatusEffect(const FGameplayTag StatusEffectTag)
{
    if (!StatusEffectTag.IsValid())
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[MANAGER] Cannot remove effect with invalid tag"));
        return;
    }

    UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Removing status effect: %s"), *StatusEffectTag.ToString());
    RemoveStatusEffect(StatusEffectTag);
}

// =====================================================
//         SMART REMOVAL SYSTEM IMPLEMENTATION
// =====================================================

bool UNomadStatusEffectManagerComponent::Nomad_RemoveStatusEffectSmart(FGameplayTag StatusEffectTag)
{
    const int32 Index = FindActiveEffectIndexByTag(StatusEffectTag);
    if (Index == INDEX_NONE)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[MANAGER] Cannot smart remove non-existent effect: %s"),
                          *StatusEffectTag.ToString());
        return false;
    }

    const FActiveEffect& Effect = ActiveEffects[Index];
    if (!Effect.EffectInstance)
    {
        UE_LOG_AFFLICTION(Error, TEXT("[MANAGER] Effect instance is null for %s"),
                          *StatusEffectTag.ToString());
        return false;
    }

    // Determine effect type and removal strategy
    EStatusEffectType EffectType = GetStatusEffectType(StatusEffectTag);

    UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Smart removing effect %s (type: %s, stacks: %d)"),
                      *StatusEffectTag.ToString(),
                      *StaticEnum<EStatusEffectType>()->GetNameStringByValue((int64)EffectType),
                      Effect.StackCount);

    switch (EffectType)
    {
        case EStatusEffectType::Timed:
            {
                // Timed effects: Remove ALL stacks (like bandage removing all bleeding)
                UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Smart removal: Removing all %d stacks of timed effect %s"),
                                  Effect.StackCount, *StatusEffectTag.ToString());
                return Internal_RemoveStatusEffectAdvanced(StatusEffectTag, 0, true, false);
            }

        case EStatusEffectType::Infinite:
        case EStatusEffectType::Survival:
            {
                // Infinite/Survival effects: Complete removal (like water removing dehydration)
                UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Smart removal: Completely removing infinite/survival effect %s"),
                                  *StatusEffectTag.ToString());
                return Internal_RemoveStatusEffectAdvanced(StatusEffectTag, 0, true, false);
            }

        case EStatusEffectType::Instant:
            {
                // Instant effects shouldn't be active, but handle gracefully
                UE_LOG_AFFLICTION(Warning, TEXT("[MANAGER] Trying to remove instant effect %s (should have auto-removed)"),
                                  *StatusEffectTag.ToString());
                return Internal_RemoveStatusEffectAdvanced(StatusEffectTag, 0, true, false);
            }

        default:
            {
                // Unknown type: Default to complete removal
                UE_LOG_AFFLICTION(Warning, TEXT("[MANAGER] Unknown effect type for %s, defaulting to complete removal"),
                                  *StatusEffectTag.ToString());
                return Internal_RemoveStatusEffectAdvanced(StatusEffectTag, 0, true, false);
            }
    }
}

bool UNomadStatusEffectManagerComponent::Nomad_RemoveStatusEffectStack(FGameplayTag StatusEffectTag)
{
    // Only remove a single stack - used for natural decay or weak items
    const int32 Index = FindActiveEffectIndexByTag(StatusEffectTag);
    if (Index == INDEX_NONE)
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[MANAGER] Cannot remove stack from non-existent effect: %s"),
                          *StatusEffectTag.ToString());
        return false;
    }

    // Check if effect is stackable first
    if (!IsStatusEffectStackable(StatusEffectTag))
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[MANAGER] Cannot remove stack from non-stackable effect %s"),
                          *StatusEffectTag.ToString());
        return false;
    }

    UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Removing single stack from effect %s"),
                      *StatusEffectTag.ToString());
    return Internal_RemoveStatusEffectAdvanced(StatusEffectTag, 1, false, true);
}

bool UNomadStatusEffectManagerComponent::Nomad_RemoveStatusEffectCompletely(FGameplayTag StatusEffectTag)
{
    // Force complete removal regardless of type
    UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Force removing all stacks of effect %s"),
                      *StatusEffectTag.ToString());
    return Internal_RemoveStatusEffectAdvanced(StatusEffectTag, 0, true, false);
}

int32 UNomadStatusEffectManagerComponent::Nomad_RemoveStatusEffectsByParentTag(FGameplayTag ParentTag)
{
    if (!ParentTag.IsValid())
    {
        UE_LOG_AFFLICTION(Warning, TEXT("[MANAGER] Cannot remove effects with invalid parent tag"));
        return 0;
    }

    TArray<FGameplayTag> TagsToRemove;

    // Find all active effects that match the parent tag
    for (const FActiveEffect& ActiveEffect : ActiveEffects)
    {
        if (ActiveEffect.Tag.MatchesTag(ParentTag))
        {
            TagsToRemove.Add(ActiveEffect.Tag);
        }
    }

    // Remove all matching effects using smart removal
    int32 RemovedCount = 0;
    for (const FGameplayTag& Tag : TagsToRemove)
    {
        if (Nomad_RemoveStatusEffectSmart(Tag))
        {
            RemovedCount++;
        }
    }

    UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Smart removal: Removed %d effects matching parent tag %s"),
                      RemovedCount, *ParentTag.ToString());
    return RemovedCount;
}

int32 UNomadStatusEffectManagerComponent::Nomad_RemoveStatusEffectsByCategory(ENomadStatusCategory Category)
{
    TArray<FGameplayTag> TagsToRemove;

    // Find all active effects that match the category
    for (const FActiveEffect& ActiveEffect : ActiveEffects)
    {
        if (ActiveEffect.EffectInstance)
        {
            // Get the effect's category
            ENomadStatusCategory EffectCategory = ENomadStatusCategory::Neutral; // Default fallback

            if (UNomadTimedStatusEffect* TimedEffect = Cast<UNomadTimedStatusEffect>(ActiveEffect.EffectInstance))
            {
                if (UNomadTimedEffectConfig* Config = TimedEffect->GetEffectConfig())
                {
                    EffectCategory = Config->Category;
                }
            }
            else if (UNomadInfiniteStatusEffect* InfiniteEffect = Cast<UNomadInfiniteStatusEffect>(ActiveEffect.EffectInstance))
            {
                if (UNomadInfiniteEffectConfig* Config = InfiniteEffect->GetEffectConfig())
                {
                    EffectCategory = Config->Category;
                }
            }

            if (EffectCategory == Category)
            {
                TagsToRemove.Add(ActiveEffect.Tag);
            }
        }
    }

    // Remove all matching effects
    int32 RemovedCount = 0;
    for (const FGameplayTag& Tag : TagsToRemove)
    {
        if (Nomad_RemoveStatusEffectSmart(Tag))
        {
            RemovedCount++;
        }
    }

    UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Removed %d effects from category %s"),
                      RemovedCount, *StaticEnum<ENomadStatusCategory>()->GetNameStringByValue((int64)Category));
    return RemovedCount;
}

int32 UNomadStatusEffectManagerComponent::Nomad_RemoveStatusEffectsMultiple(const TArray<FGameplayTag>& StatusEffectTags)
{
    int32 RemovedCount = 0;

    for (const FGameplayTag& Tag : StatusEffectTags)
    {
        if (Nomad_RemoveStatusEffectSmart(Tag))
        {
            RemovedCount++;
        }
    }

    UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Smart removal: Removed %d effects from batch removal"), RemovedCount);
    return RemovedCount;
}

// =====================================================
//         INTERNAL REMOVAL SYSTEM
// =====================================================

bool UNomadStatusEffectManagerComponent::Internal_RemoveStatusEffectAdvanced(
    FGameplayTag StatusEffectTag,
    int32 StacksToRemove,
    bool bForceComplete,
    bool bRespectStackability)
{
    const int32 Index = FindActiveEffectIndexByTag(StatusEffectTag);
    if (Index == INDEX_NONE) return false;

    FActiveEffect& Effect = ActiveEffects[Index];
    const int32 PrevStacks = Effect.StackCount;

    // Determine actual removal behavior
    int32 ActualStacksToRemove;

    if (bForceComplete)
    {
        // Force complete removal
        ActualStacksToRemove = PrevStacks;
    }
    else if (bRespectStackability && !IsStatusEffectStackable(StatusEffectTag))
    {
        // Trying to remove stacks from non-stackable effect
        UE_LOG_AFFLICTION(Warning, TEXT("[MANAGER] Cannot remove stacks from non-stackable effect %s"),
                          *StatusEffectTag.ToString());
        return false;
    }
    else
    {
        // Normal stack removal
        ActualStacksToRemove = FMath::Min(StacksToRemove, PrevStacks);
    }

    const int32 NewStacks = PrevStacks - ActualStacksToRemove;

    UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Advanced removal: %s - Removing %d stacks (was %d, will be %d)"),
                      *StatusEffectTag.ToString(), ActualStacksToRemove, PrevStacks, NewStacks);

    if (NewStacks > 0)
    {
        // Update stack count and notify effect
        Effect.StackCount = NewStacks;

        if (Effect.EffectInstance)
        {
            if (UNomadTimedStatusEffect* TimedEffect = Cast<UNomadTimedStatusEffect>(Effect.EffectInstance))
            {
                TimedEffect->OnUnstacked(NewStacks);
            }
            else if (UNomadInfiniteStatusEffect* InfiniteEffect = Cast<UNomadInfiniteStatusEffect>(Effect.EffectInstance))
            {
                InfiniteEffect->OnUnstacked(NewStacks);
            }
        }

        NotifyAffliction(StatusEffectTag, ENomadAfflictionNotificationType::Unstacked, PrevStacks, NewStacks);
    }
    else
    {
        // Complete removal
        if (Effect.EffectInstance)
        {
            Effect.EffectInstance->Nomad_OnStatusEffectEnds();
            Effect.EffectInstance->ConditionalBeginDestroy();
        }

        ActiveEffects.RemoveAt(Index);
        NotifyAffliction(StatusEffectTag, ENomadAfflictionNotificationType::Removed, PrevStacks, 0);
    }

    return true;
}

// =====================================================
//         EFFECT LIFECYCLE (INTERNAL LOGIC)
// =====================================================

void UNomadStatusEffectManagerComponent::CreateAndApplyStatusEffect_Implementation(
    const TSubclassOf<UACFBaseStatusEffect> StatusEffectToConstruct, AActor* Instigator)
{
    if (!StatusEffectToConstruct) {
        UE_LOG_AFFLICTION(Warning, TEXT("[MANAGER] StatusEffectToConstruct not set or invalid!"));
        return;
    }

    UObject* EffectCDO = StatusEffectToConstruct->GetDefaultObject();
    if (!EffectCDO) return;

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor) return;

    // --- INSTANT EFFECT ---
    if (EffectCDO->IsA(UNomadInstantStatusEffect::StaticClass()))
    {
        UNomadInstantStatusEffect* NewEffect = NewObject<UNomadInstantStatusEffect>(OwnerActor, StatusEffectToConstruct);
        if (NewEffect)
        {
            ACharacter* OwnerChar = Cast<ACharacter>(OwnerActor);
            NewEffect->SetDamageCauser(Instigator ? Instigator : OwnerChar);
            NewEffect->Nomad_OnStatusEffectStarts(OwnerChar);
            UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Applied instant effect"));
        }
        return;
    }

    // --- TIMED EFFECT ---
    if (EffectCDO->IsA(UNomadTimedStatusEffect::StaticClass()))
    {
        const UNomadTimedStatusEffect* EffectCDO_Timed = Cast<UNomadTimedStatusEffect>(EffectCDO);
        const UNomadTimedEffectConfig* Config = EffectCDO_Timed ? EffectCDO_Timed->GetEffectConfig() : nullptr;
        if (!Config) return;
        const FGameplayTag EffectTag = Config->EffectTag;
        if (!EffectTag.IsValid()) return;

        const int32 Index = FindActiveEffectIndexByTag(EffectTag);
        if (Index != INDEX_NONE)
        {
            FActiveEffect& Eff = ActiveEffects[Index];
            const UNomadTimedEffectConfig* ActiveConfig = nullptr;
            if (Eff.EffectInstance)
            {
                ActiveConfig = Cast<UNomadTimedEffectConfig>(Cast<UNomadTimedStatusEffect>(Eff.EffectInstance)->GetEffectConfig());
            }
            if (ActiveConfig && ActiveConfig->bCanStack)
            {
                const int32 PrevStacks = Eff.StackCount;
                if (Eff.StackCount < ActiveConfig->MaxStackSize)
                {
                    Eff.StackCount++;
                    if (Eff.EffectInstance)
                    {
                        Cast<UNomadTimedStatusEffect>(Eff.EffectInstance)->OnStacked(Eff.StackCount);
                    }
                    NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Stacked, PrevStacks, Eff.StackCount);
                    UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Stacked timed effect %s to %d stacks"),
                                      *EffectTag.ToString(), Eff.StackCount);
                }
                else
                {
                    if (Eff.EffectInstance)
                    {
                        Cast<UNomadTimedStatusEffect>(Eff.EffectInstance)->OnRefreshed();
                    }
                    NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Refreshed, Eff.StackCount, Eff.StackCount);
                    UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Refreshed timed effect %s (at max stacks)"),
                                      *EffectTag.ToString());
                }
            }
            else
            {
                if (Eff.EffectInstance)
                {
                    Cast<UNomadTimedStatusEffect>(Eff.EffectInstance)->OnRefreshed();
                }
                NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Refreshed, Eff.StackCount, Eff.StackCount);
                UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Refreshed non-stackable timed effect %s"),
                                  *EffectTag.ToString());
            }
            return;
        }

        UNomadTimedStatusEffect* NewEffect = NewObject<UNomadTimedStatusEffect>(OwnerActor, StatusEffectToConstruct);
        if (NewEffect)
        {
            ACharacter* OwnerChar = Cast<ACharacter>(OwnerActor);
            NewEffect->SetDamageCauser(Instigator ? Instigator : OwnerChar);
            NewEffect->NomadStartEffectWithManager(OwnerChar, this);
            ActiveEffects.Add(FActiveEffect(EffectTag, 1, NewEffect));
            NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Applied, 0, 1);
            UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Applied new timed effect %s"), *EffectTag.ToString());
        }
        return;
    }

    // --- INFINITE EFFECT ---
    if (EffectCDO->IsA(UNomadInfiniteStatusEffect::StaticClass()))
    {
        const UNomadInfiniteStatusEffect* EffectCDO_Inf = Cast<UNomadInfiniteStatusEffect>(EffectCDO);
        const UNomadInfiniteEffectConfig* Config = EffectCDO_Inf ? EffectCDO_Inf->GetEffectConfig() : nullptr;
        if (!Config) return;
        const FGameplayTag EffectTag = Config->EffectTag;
        if (!EffectTag.IsValid()) return;

        const int32 Index = FindActiveEffectIndexByTag(EffectTag);
        if (Index != INDEX_NONE)
        {
            FActiveEffect& Eff = ActiveEffects[Index];
            const UNomadInfiniteEffectConfig* ActiveConfig = nullptr;
            if (Eff.EffectInstance)
            {
                ActiveConfig = Cast<UNomadInfiniteEffectConfig>(Cast<UNomadInfiniteStatusEffect>(Eff.EffectInstance)->GetEffectConfig());
            }
            if (ActiveConfig && ActiveConfig->bCanStack)
            {
                const int32 PrevStacks = Eff.StackCount;
                if (Eff.StackCount < ActiveConfig->MaxStackSize)
                {
                    Eff.StackCount++;
                    if (Eff.EffectInstance)
                    {
                        Cast<UNomadInfiniteStatusEffect>(Eff.EffectInstance)->OnStacked(Eff.StackCount);
                    }
                    NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Stacked, PrevStacks, Eff.StackCount);
                    UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Stacked infinite effect %s to %d stacks"),
                                      *EffectTag.ToString(), Eff.StackCount);
                }
                else
                {
                    if (Eff.EffectInstance)
                    {
                        Cast<UNomadInfiniteStatusEffect>(Eff.EffectInstance)->OnRefreshed();
                    }
                    NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Refreshed, Eff.StackCount, Eff.StackCount);
                    UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Refreshed infinite effect %s (at max stacks)"),
                                      *EffectTag.ToString());
                }
            }
            else
            {
                if (Eff.EffectInstance)
                {
                    Cast<UNomadInfiniteStatusEffect>(Eff.EffectInstance)->OnRefreshed();
                }
                NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Refreshed, Eff.StackCount, Eff.StackCount);
                UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Refreshed non-stackable infinite effect %s"),
                                  *EffectTag.ToString());
            }
            return;
        }

        UNomadInfiniteStatusEffect* NewEffect = NewObject<UNomadInfiniteStatusEffect>(OwnerActor, StatusEffectToConstruct);
        if (NewEffect)
        {
            ACharacter* OwnerChar = Cast<ACharacter>(OwnerActor);
            NewEffect->SetDamageCauser(Instigator ? Instigator : OwnerChar);
            NewEffect->Nomad_OnStatusEffectStarts(OwnerChar);
            ActiveEffects.Add(FActiveEffect(EffectTag, 1, NewEffect));
            NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Applied, 0, 1);
            UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Applied new infinite effect %s"), *EffectTag.ToString());
        }
        return;
    }

    UE_LOG_AFFLICTION(Warning, TEXT("[MANAGER] Unknown effect type for class %s"), *StatusEffectToConstruct->GetName());
}

void UNomadStatusEffectManagerComponent::AddStatusEffect(UACFBaseStatusEffect* StatusEffect, AActor* Instigator)
{
    Super::AddStatusEffect(StatusEffect, GetOwner());
}

void UNomadStatusEffectManagerComponent::RemoveStatusEffect_Implementation(const FGameplayTag EffectTag)
{
    const int32 Index = FindActiveEffectIndexByTag(EffectTag);
    if (Index == INDEX_NONE) return;

    FActiveEffect& Eff = ActiveEffects[Index];
    const int32 PrevStacks = Eff.StackCount;
    const int32 NewStacks = PrevStacks - 1;

    if (Eff.StackCount > 1)
    {
        Eff.StackCount--;
        NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Unstacked, PrevStacks, NewStacks);

        // Inform the effect instance a stack was removed
        if (Eff.EffectInstance)
        {
            if (UNomadTimedStatusEffect* TimedStatusEffect = Cast<UNomadTimedStatusEffect>(Eff.EffectInstance))
            {
                TimedStatusEffect->OnUnstacked(Eff.StackCount);
            }
            else if (UNomadInfiniteStatusEffect* InfiniteStatusEffect = Cast<UNomadInfiniteStatusEffect>(Eff.EffectInstance))
            {
                InfiniteStatusEffect->OnUnstacked(Eff.StackCount);
            }
        }
        UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Removed 1 stack from %s (now %d stacks)"),
                          *EffectTag.ToString(), Eff.StackCount);
    }
    else
    {
        // Last stack: call end logic, destroy instance, remove from array, notify UI
        if (Eff.EffectInstance)
        {
            Eff.EffectInstance->Nomad_OnStatusEffectEnds();
            Eff.EffectInstance->ConditionalBeginDestroy();
        }
        ActiveEffects.RemoveAt(Index);
        NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Removed, PrevStacks, 0);
        UE_LOG_AFFLICTION(Log, TEXT("[MANAGER] Completely removed effect %s"), *EffectTag.ToString());
    }
}

// =====================================================
//         UI NOTIFICATION
// =====================================================

void UNomadStatusEffectManagerComponent::NotifyAffliction(const FGameplayTag Tag, const ENomadAfflictionNotificationType Type, const int32 PrevStacks, const int32 NewStacks) const
{
    if (const AActor* OwnerActor = GetOwner())
    {
        if (UNomadAfflictionComponent* AfflictionComp = OwnerActor->FindComponentByClass<UNomadAfflictionComponent>())
        {
            AfflictionComp->UpdateAfflictionArray(Tag, Type, PrevStacks, NewStacks);
        }
    }
}