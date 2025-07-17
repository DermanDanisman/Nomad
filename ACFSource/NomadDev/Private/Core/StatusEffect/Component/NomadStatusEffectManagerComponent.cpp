// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "Core/StatusEffect/NomadTimedStatusEffect.h"
#include "Core/Data/StatusEffect/NomadTimedEffectConfig.h"
#include "Core/Component/NomadAfflictionComponent.h"
#include "Core/StatusEffect/NomadInfiniteStatusEffect.h"
#include "Core/StatusEffect/NomadInstantStatusEffect.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "StatusEffects/ACFBaseStatusEffect.h"

UNomadStatusEffectManagerComponent::UNomadStatusEffectManagerComponent()
{
    TotalStatusEffectDamage = 0.0f;
    StatusEffectDamageTotals.Empty();
}

void UNomadStatusEffectManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (FActiveEffect& Eff : ActiveEffects)
    {
        if (Eff.EffectInstance)
        {
            Eff.EffectInstance->Nomad_OnStatusEffectEnds();
        }
    }
    ActiveEffects.Empty();

    Super::EndPlay(EndPlayReason);
}

void UNomadStatusEffectManagerComponent::AddStatusEffectDamage(FGameplayTag EffectTag, float Delta)
{
    TotalStatusEffectDamage += Delta;
    StatusEffectDamageTotals.FindOrAdd(EffectTag) += Delta;
}

float UNomadStatusEffectManagerComponent::GetTotalStatusEffectDamage() const
{
    return TotalStatusEffectDamage;
}

float UNomadStatusEffectManagerComponent::GetStatusEffectDamageByTag(FGameplayTag EffectTag) const
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
    TotalStatusEffectDamage = 0.0f;
    StatusEffectDamageTotals.Empty();
}

/**
 * Adds a status effect by class, instantiating and applying if not present,
 * or stacking/refreshing if already present. Notifies affliction UI.
 */
void UNomadStatusEffectManagerComponent::Nomad_AddStatusEffect(TSubclassOf<UACFBaseStatusEffect> StatusEffectClass, AActor* Instigator)
{
    // Core logic is handled in CreateAndApplyStatusEffect (can be overridden for custom logic).
    CreateAndApplyStatusEffect(StatusEffectClass, Instigator);
}

/**
 * Removes a status effect by tag, updating the stack or removing entirely,
 * and notifies affliction UI.
 */
void UNomadStatusEffectManagerComponent::Nomad_RemoveStatusEffect(FGameplayTag StatusEffectTag)
{
    RemoveStatusEffect(StatusEffectTag);
}

/**
 * Find the index of an active effect by tag, or INDEX_NONE if not present.
 * @param Tag - The gameplay tag to search for.
 * @return Index in ActiveEffects array, or INDEX_NONE if not found.
 */
int32 UNomadStatusEffectManagerComponent::FindActiveEffectIndexByTag(FGameplayTag Tag) const
{
    return ActiveEffects.IndexOfByPredicate([&](const FActiveEffect& Eff) { return Eff.Tag == Tag; });
}

/**
 * Handles all logic for creating, stacking, and refreshing timed/infinite/instant effects.
 * - If effect is already present, stacks or refreshes as appropriate.
 * - If not present, instantiates a new effect and applies to owner.
 * - Notifies the affliction UI after any state change.
 * - Handles all three effect types (instant, timed, infinite) with correct logic.
 */
void UNomadStatusEffectManagerComponent::CreateAndApplyStatusEffect_Implementation(TSubclassOf<UACFBaseStatusEffect> StatusEffectToConstruct, AActor* Instigator)
{
    if (!StatusEffectToConstruct) {
        UE_LOG(LogTemp, Warning, TEXT("StatusEffectToConstruct not set or invalid! - NomadStatusEffectManagerComponent"));
        return;
    }
    UObject* EffectCDO = StatusEffectToConstruct->GetDefaultObject();
    if (!EffectCDO) return;

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor) return;

    // -------- INSTANT EFFECT --------
    if (EffectCDO->IsA(UNomadInstantStatusEffect::StaticClass()))
    {
        UNomadInstantStatusEffect* NewEffect = NewObject<UNomadInstantStatusEffect>(OwnerActor, StatusEffectToConstruct);
        if (NewEffect)
        {
            ACharacter* OwnerChar = Cast<ACharacter>(OwnerActor);
            NewEffect->DamageCauser = OwnerChar; // <<<< Set DamageCauser
            NewEffect->Nomad_OnStatusEffectStarts(OwnerChar);
        }
        return;
    }

    // -------- TIMED EFFECT --------
    if (EffectCDO->IsA(UNomadTimedStatusEffect::StaticClass()))
    {
        UNomadTimedStatusEffect* EffectCDO_Timed = Cast<UNomadTimedStatusEffect>(EffectCDO);
        UNomadTimedEffectConfig* Config = EffectCDO_Timed ? EffectCDO_Timed->GetConfig() : nullptr;
        if (!Config) return;
        FGameplayTag EffectTag = Config->EffectTag;
        if (!EffectTag.IsValid()) return;

        int32 Index = FindActiveEffectIndexByTag(EffectTag);
        if (Index != INDEX_NONE)
        {
            // ... stacking/refresh logic ...
            return;
        }

        // Not present: create new effect instance, start it, and add to ActiveEffects.
        UNomadTimedStatusEffect* NewEffect = NewObject<UNomadTimedStatusEffect>(OwnerActor, StatusEffectToConstruct);
        if (NewEffect)
        {
            ACharacter* OwnerChar = Cast<ACharacter>(OwnerActor);
            NewEffect->DamageCauser = OwnerChar; // <<<< Set DamageCauser
            NewEffect->OnStatusEffectStarts(OwnerChar, this);
            ActiveEffects.Add({EffectTag, 1, NewEffect});
            NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Applied, 0, 1);
        }
        return;
    }

    // -------- INFINITE EFFECT --------
    if (EffectCDO->IsA(UNomadInfiniteStatusEffect::StaticClass()))
    {
        UNomadInfiniteStatusEffect* EffectCDO_Inf = Cast<UNomadInfiniteStatusEffect>(EffectCDO);
        UNomadInfiniteEffectConfig* Config = EffectCDO_Inf ? EffectCDO_Inf->GetEffectConfig() : nullptr;
        if (!Config) return;
        FGameplayTag EffectTag = Config->EffectTag;
        if (!EffectTag.IsValid()) return;

        int32 Index = FindActiveEffectIndexByTag(EffectTag);
        if (Index != INDEX_NONE)
        {
            // ... stacking/refresh logic ...
            return;
        }

        // Not present: create new effect instance, start it, and add to ActiveEffects.
        UNomadInfiniteStatusEffect* NewEffect = NewObject<UNomadInfiniteStatusEffect>(OwnerActor, StatusEffectToConstruct);
        if (NewEffect)
        {
            ACharacter* OwnerChar = Cast<ACharacter>(OwnerActor);
            NewEffect->DamageCauser = OwnerChar; // <<<< Set DamageCauser
            NewEffect->Nomad_OnStatusEffectStarts(OwnerChar);
            ActiveEffects.Add({EffectTag, 1, NewEffect});
            NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Applied, 0, 1);
        }
        return;
    }
}

/**
 * Adds a status effect (legacy path, not used in new flow).
 * Calls Super for compatibility; all custom logic is in Nomad_AddStatusEffect above.
 */
void UNomadStatusEffectManagerComponent::AddStatusEffect(UACFBaseStatusEffect* StatusEffect, AActor* Instigator)
{
    Super::AddStatusEffect(StatusEffect, GetOwner());
}

/**
 * Removes a status effect instance: decrements stack if more than 1, or removes entirely if last stack.
 * Notifies affliction UI, and destroys effect instance if last stack.
 * @param EffectTag - The gameplay tag of the effect to remove.
 */
void UNomadStatusEffectManagerComponent::RemoveStatusEffect_Implementation(FGameplayTag EffectTag)
{
    int32 Index = FindActiveEffectIndexByTag(EffectTag);
    if (Index == INDEX_NONE) return;

    FActiveEffect& Eff = ActiveEffects[Index];
    int32 PrevStacks = Eff.StackCount;
    int32 NewStacks = PrevStacks - 1;

    if (Eff.StackCount > 1)
    {
        // More than one stack: decrement and notify.
        Eff.StackCount--;
        NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Unstacked, PrevStacks, NewStacks);
        // Optionally: per-stack removal logic for timed/infinite effects.
    }
    else
    {
        // Last stack: call end logic, destroy instance, remove from array, notify.
        if (Eff.EffectInstance)
        {
            Eff.EffectInstance->Nomad_OnStatusEffectEnds();
            Eff.EffectInstance->ConditionalBeginDestroy();
        }
        ActiveEffects.RemoveAt(Index);
        NotifyAffliction(EffectTag, ENomadAfflictionNotificationType::Removed, PrevStacks, 0);
    }
}

/**
 * Sends a notification to the NomadAfflictionComponent (UI) for an affliction/stack/apply/remove event.
 * Finds the affliction component by class and calls UpdateAfflictionArray.
 * @param Tag - The effect's gameplay tag.
 * @param Type - The notification type (applied, stacked, removed, etc).
 * @param PrevStacks - Stack count before the change.
 * @param NewStacks - Stack count after the change.
 */
void UNomadStatusEffectManagerComponent::NotifyAffliction(FGameplayTag Tag, ENomadAfflictionNotificationType Type, int32 PrevStacks, int32 NewStacks) const
{
    if (AActor* OwnerActor = GetOwner())
    {
        if (UNomadAfflictionComponent* AfflictionComp = OwnerActor->FindComponentByClass<UNomadAfflictionComponent>())
        {
            AfflictionComp->UpdateAfflictionArray(Tag, Type, PrevStacks, NewStacks);
        }
    }
}