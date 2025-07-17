
#pragma once

#include "StatusEffectSystem/Public/Components/ACFStatusEffectManagerComponent.h"
#include "CoreMinimal.h"
#include <GameplayTagContainer.h>

#include "ACFBaseStatusEffect.generated.h"


UCLASS(Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew, abstract)
class STATUSEFFECTSYSTEM_API UACFBaseStatusEffect : public UObject {
    GENERATED_BODY()

    friend UACFStatusEffectManagerComponent;

public:
    UFUNCTION(BlueprintCallable, Category = "ACF")
    void EndEffect();

    UPROPERTY(BlueprintAssignable, BlueprintCallable)
    FOnStatusEffectChanged OnStatusEffectStarted;

    UPROPERTY(BlueprintAssignable, BlueprintCallable)
    FOnStatusEffectChanged OnStatusEffectEnded;

    UFUNCTION(BlueprintPure, Category = ACF)
    class ACharacter* GetCharacterOwner() const { return CharacterOwner; }

    UFUNCTION(BlueprintPure, Category = ACF)
    class AActor* GetInstigator() const { return Instigator; }

    UFUNCTION(BlueprintPure, Category = ACF)
    UTexture2D* GetStatusIcon() const { return Icon; }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetStatusIcon(UTexture2D* val) { Icon = val; }

    UFUNCTION(BlueprintPure, Category = ACF)
    virtual FGameplayTag GetStatusEffectTag() const { return StatusEffectTag; }

    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetStatusEffectTag(FGameplayTag val) { StatusEffectTag = val; }

protected:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ACF")
    void OnStatusEffectStarts(ACharacter* Character);
    virtual void OnStatusEffectStarts_Implementation(ACharacter* Character);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ACF")
    void OnTriggerStatusEffect();
    virtual void OnTriggerStatusEffect_Implementation();
        
    //Called when this status is retriggered again while is 
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ACF")
    void OnStatusRetriggered();
    virtual void OnStatusRetriggered_Implementation();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ACF")
    void OnStatusEffectEnds();
    virtual void OnStatusEffectEnds_Implementation();

    /*If this effect can applied more than once simultaneously*/
    UPROPERTY(BlueprintReadWrite, Category = "ACF")
    FGameplayTag StatusEffectTag;

    /*Icon for UI*/
    UPROPERTY(BlueprintReadWrite, Category = "ACF")
    UTexture2D* Icon;

    UPROPERTY(BlueprintReadWrite, Category = "ACF")
    bool bCanBeRetriggered = false;

    UPROPERTY(BlueprintReadOnly, Category = "ACF")
    class ACharacter* CharacterOwner;

    UPROPERTY(BlueprintReadOnly, Category = "ACF")
    class AActor* Instigator;

    UPROPERTY(BlueprintReadOnly, Category = "ACF")
    class UACFStatusEffectManagerComponent* StatusComp;

private:
    void Internal_OnEffectStarted(ACharacter* Character, AActor* inInstigator);

    UWorld* GetWorld() const;
};
