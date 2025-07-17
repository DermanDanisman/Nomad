// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include <GameplayTagContainer.h>

#include "ACFCurrencyComponent.generated.h"

// Delegate broadcast whenever currency changes (new total, and delta)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCurrencyValueChanged,float,newValue,float,variation);

//------------------------------------------------------------------------------
// UACFCurrencyComponent.h
//------------------------------------------------------------------------------
UCLASS(ClassGroup=(ACF), Blueprintable, meta=(BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UACFCurrencyComponent : public UActorComponent {
    GENERATED_BODY()

public:
    //------------------------------------------------------------------------------
    /**
     * Constructor: disable tick, enable replication, initialize currency.
     */
    UACFCurrencyComponent();

    //------------------------------------------------------------------------------
    /** Register CurrencyAmount for replication. */
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    //------------------------------------------------------------------------------
    /**
     * Add `Amount` to the currency total on the server.
     * @param Amount  Positive delta to apply.
     */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category=ACF)
    void AddCurrency(float Amount);

    //------------------------------------------------------------------------------
    /**
     * Remove `Amount` from the currency total on the server.
     * Ensures non-negative result via clamping.
     * @param Amount  Positive delta to subtract.
     */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category=ACF)
    void RemoveCurrency(float Amount);

    //------------------------------------------------------------------------------
    /**
     * Directly set the currency total on the server.
     * @param Amount  New total value.
     */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category=ACF)
    void SetCurrency(float Amount);

    //------------------------------------------------------------------------------
    /** Returns true if `CurrencyAmount` ≥ `Amount`. */
    UFUNCTION(BlueprintPure, Category=ACF)
    FORCEINLINE bool HasEnoughCurrency(float Amount) const
    {
        return CurrencyAmount >= Amount;
    }

    //------------------------------------------------------------------------------
    /** Get the current currency total. */
    UFUNCTION(BlueprintPure, Category=ACF)
    FORCEINLINE float GetCurrentCurrencyAmount() const
    {
        return CurrencyAmount;
    }

    //------------------------------------------------------------------------------
    /** Broadcast when currency changes (new total and delta). */
    UPROPERTY(BlueprintAssignable, Category=ACF)
    FOnCurrencyValueChanged OnCurrencyChanged;

protected:
    //------------------------------------------------------------------------------
    /** BeginPlay: bind to health-zero event if drop-on-death is enabled. */
    virtual void BeginPlay() override;

    //------------------------------------------------------------------------------
    /** Replicated currency total; calls OnRep_Currency when updated on clients. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame,
              ReplicatedUsing=OnRep_Currency, Category=ACF)
    float CurrencyAmount = 0.f;

    //------------------------------------------------------------------------------
    /** If true, drop currency into world on owner death (health ≤ 0). */
    UPROPERTY(EditDefaultsOnly, Category=ACF)
    bool bDropCurrencyOnOwnerDeath = true;

    //------------------------------------------------------------------------------
    /** Variation in dropped Amount (±) when owner dies. */
    UPROPERTY(EditAnywhere, Category=ACF,
              meta=(EditCondition="bDropCurrencyOnOwnerDeath"))
    float CurrencyDropVariation = 5.f;

    //------------------------------------------------------------------------------
    /** Hook for custom reactions to any currency change (client & server). */
    virtual void HandleCurrencyChanged();

private:
    //------------------------------------------------------------------------------
    /** Handler for stat reaching zero (e.g. health); spawns currency pickup. */
    UFUNCTION()
    void HandleStatReachedZero(FGameplayTag stat);

    //------------------------------------------------------------------------------
    /** Client-side notifier for CurrencyAmount replication. */
    UFUNCTION()
    void OnRep_Currency();

    //------------------------------------------------------------------------------
    /** Internal: broadcast OnCurrencyChanged and call HandleCurrencyChanged. */
    void DispatchCurrencyChanged(float deltaAmount);
};
