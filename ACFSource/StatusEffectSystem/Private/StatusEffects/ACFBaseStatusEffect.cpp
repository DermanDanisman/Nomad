
#include "StatusEffects/ACFBaseStatusEffect.h"
#include "Components/ACFStatusEffectManagerComponent.h"
#include <GameFramework/Character.h>

void UACFBaseStatusEffect::Internal_OnEffectStarted(ACharacter* Character, AActor* inInstigator)
{
    Instigator = inInstigator;
    if (Character) {
        CharacterOwner = Character ;
        StatusComp = Character->FindComponentByClass<UACFStatusEffectManagerComponent>();
        OnStatusEffectStarts(Character);
    } else {
    	UE_LOG(LogTemp, Error, TEXT("Invalid Character - UACFBaseStatusEffect::Internal_OnEffectStarted"));
    }
}

void UACFBaseStatusEffect::OnStatusEffectStarts_Implementation(ACharacter* Character)
{
    
}

void UACFBaseStatusEffect::OnTriggerStatusEffect_Implementation()
{
}

void UACFBaseStatusEffect::OnStatusRetriggered_Implementation()
{
}

void UACFBaseStatusEffect::OnStatusEffectEnds_Implementation()
{
}


void UACFBaseStatusEffect::EndEffect()
{
    OnStatusEffectEnds();
    OnStatusEffectEnded.Broadcast(GetStatusEffectTag());
}

UWorld* UACFBaseStatusEffect::GetWorld() const
{
    if (GetCharacterOwner()) {
        return GetCharacterOwner()->GetWorld();
    }
    return nullptr;
}
