// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFActionTypes.h"
#include "Animation/AnimMontage.h"
#include "Components/ACFActionsManagerComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include <GameplayTagContainer.h>

#include "ACFBaseAction.generated.h"

/**
 * UACFBaseAction
 * ----------------------------------------------------------
 * The central base class for all modular actions in the ACF system.
 * Actions are used to represent character states such as Attacking,
 * Getting Hit, Blocking, Rolling, Casting Spells, etc.
 *
 * Actions are not monolithic enums or flat states, but full UObject
 * instances, allowing for rich per-action customization and runtime
 * configuration. They are activated/deactivated by the
 * UACFActionsManagerComponent, which ensures only one action is
 * performed at a time for a character.
 *
 * DESIGN HIGHLIGHTS:
 * - Each action can define its animation, effects, requirements,
 *   gameplay cost, montage section selection, etc.
 * - Supports animation-driven logic (montage notifies, state changes)
 * - Provides hooks for custom effects, cost, cooldown, and
 *   execution/exit conditions.
 * - Fully Blueprintable and instanced for each owning character.
 * - Communicates with the ActionsManager for state transitions,
 *   replication, and input management.
 */
UCLASS(Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew)
class ACTIONSSYSTEM_API UACFBaseAction : public UObject {
    GENERATED_BODY()

    // Friend allows ActionsManagerComponent to manage action internals.
    friend UACFActionsManagerComponent;

public:
    UACFBaseAction();

    /** Returns the time remaining on this action's cooldown (seconds). If not on cooldown, returns 0. */
    UFUNCTION(BlueprintCallable, Category = ACF)
    float GetCooldownTimeRemaining();

    /** Starts the cooldown for this action. Call on both server and client for UI and gameplay consistency. */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void StartCooldown();

    /** Returns the current configuration (cost, montage, effect, requirements, etc.) for this action. */
    UFUNCTION(BlueprintPure, Category = ACF)
    FActionConfig GetActionConfig() const { return ActionConfig; }

    /** Allows runtime change of this action's configuration. */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetActionConfig(const FActionConfig& newConfig);

    /** Assigns the animation montage which will be played when this action is executed. */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetAnimMontage(UAnimMontage* newMontage);

    /** (Blueprint) Interrupt this action (forcibly aborts the action, e.g., due to a higher priority event) */
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = ACF)
    void ActionInterrupt();

    /** Returns the animation montage assigned to this action, or nullptr. */
    UFUNCTION(BlueprintPure, Category = ACF)
    UAnimMontage* GetAnimMontage() const { return animMontage; }

    /** Returns the gameplay tag that identifies this action (used for switching, querying, and replication). */
    UFUNCTION(BlueprintPure, Category = ACF)
    FGameplayTag GetActionTag() const { return ActionTag; }

protected:
    //
    // ---- OVERRIDABLE LIFECYCLE HOOKS ----
    //

    /**
     * Called when the action is successfully triggered and started.
     * This is the main entry point for custom logic, initialization, or setup.
     * - contextString: Optional extra context (e.g., from input or gameplay).
     * - InteractedActor: Optional actor involved in this action (e.g., a chest, target).
     * - ItemSlotTag: Optional inventory slot or similar context.
     */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void OnActionStarted(const FString& contextString = "", AActor* InteractedActor = nullptr, FGameplayTag ItemSlotTag = FGameplayTag());
    virtual void OnActionStarted_Implementation(const FString& contextString = "", AActor* InteractedActor = nullptr, FGameplayTag ItemSlotTag = FGameplayTag());

    /** Called on all clients (for UI/FX) when the action starts. */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void ClientsOnActionStarted(const FString& contextString = "");
    virtual void ClientsOnActionStarted_Implementation(const FString& contextString = "");

    /** Called when the action finishes, is interrupted, or forcibly exited. Use for cleanup. */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void OnActionEnded();
    virtual void OnActionEnded_Implementation();

    /** Called on all clients (for UI/FX) when the action ends. */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void ClientsOnActionEnded();
    virtual void ClientsOnActionEnded_Implementation();

    /**
     * Called if the action is entered by transitioning from another action.
     * This is called before OnActionStarted. Use to handle state transitions or copy context from previous action.
     */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void OnActionTransition(class UACFBaseAction* previousState);
    virtual void OnActionTransition_Implementation(class UACFBaseAction* previousState);

    /** Play any VFX/SFX or gameplay effects associated with the action. Called by ActionsManager when appropriate. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ACF)
    void PlayEffects();
    virtual void PlayEffects_Implementation();

    /** Called every frame during this action if ActionsManagerComponent::bCanTick is enabled. */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void OnTick(float DeltaTime);
    virtual void OnTick_Implementation(float DeltaTime);

    /**
     * Determines whether this action can be executed, given the current context (stats, requirements, item slot, etc.).
     * Override this to restrict actions (e.g., can't attack if stamina too low).
     */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    bool CanExecuteAction(class ACharacter* owner = nullptr, FGameplayTag ItemSlotTag = FGameplayTag());
    virtual bool CanExecuteAction_Implementation(class ACharacter* owner = nullptr, FGameplayTag ItemSlotTag = FGameplayTag());

    /**
     * Selects the name of the animation montage section to play for this action.
     * Override to implement direction-based, context-based, or random section selection.
     */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    FName GetMontageSectionName();
    virtual FName GetMontageSectionName_Implementation();

    /**
     * Configures the root motion/motion warping info for this action, if required.
     * Only called if MontageReproductionType is set to a warping type.
     */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void GetWarpInfo(FACFWarpReproductionInfo& outWarpInfo);
    virtual void GetWarpInfo_Implementation(FACFWarpReproductionInfo& outWarpInfo);

    /**
     * Provides a transform for the actor to warp to during this action (e.g., lunge, attack, cinematic).
     * Only called if motion warping is enabled for this action.
     */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    FTransform GetWarpTransform();
    virtual FTransform GetWarpTransform_Implementation();

    /**
     * Provides a scene component to warp to (for more complex motion warping targeting).
     */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    class USceneComponent* GetWarpTargetComponent();
    virtual class USceneComponent* GetWarpTargetComponent_Implementation();

    /** For quickbar or hotbar actions, returns the active quickbar index for this action. */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    int32 GetActiveQuickbarIndex(const int32 CurrentActiveQuickbarIndex);
    virtual int32 GetActiveQuickbarIndex_Implementation(const int32 CurrentActiveQuickbarIndex);

    /** For quickbar or hotbar actions, returns the slot index for this action. */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    int32 GetQuickbarSlotIndex(const int32 CurrentQuickbarSlotIndex);
    virtual int32 GetQuickbarSlotIndex_Implementation(const int32 CurrentQuickbarSlotIndex);

    /** Allows external code to set up montage execution info for this action. */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetMontageInfo(const FACFMontageInfo& montageInfo);

    /** Immediately interrupts and stops the action (and any associated animation/montage). */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void StopActionImmediately();

    /**
     * Called when a notable point in the animation is reached (e.g., via an AnimNotify).
     * Use this to trigger gameplay logic at precise animation moments (like hit windows).
     */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void OnNotablePointReached();
    virtual void OnNotablePointReached_Implementation();

    /** Client version for notable point reached (e.g., UI, SFX sync). */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void ClientsOnNotablePointReached();
    virtual void ClientsOnNotablePointReached_Implementation();

    /**
     * Called when a sub-action state is entered (e.g., combo window, parry window, charge phase).
     * Use this to enable/disable special logic or input.
     */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void OnSubActionStateEntered();
    virtual void OnSubActionStateEntered_Implementation();

    /** Called when a sub-action state is exited. */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void OnSubActionStateExited();
    virtual void OnSubActionStateExited_Implementation();

    /** Client version for entering sub-action state. */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void ClientsOnSubActionStateEntered();
    virtual void ClientsOnSubActionStateEntered_Implementation();

    /** Client version for exiting sub-action state. */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    void ClientsOnSubActionStateExited();
    virtual void ClientsOnSubActionStateExited_Implementation();

    /** Returns the rate at which the montage should play for this action (default 1.0). */
    UFUNCTION(BlueprintNativeEvent, Category = ACF)
    float GetPlayRate();
    virtual float GetPlayRate_Implementation();

    /** Executes the action: prepares and plays the montage, applies cost/effects, begins state. */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void ExecuteAction();

    /** If true, the action's logic is bound to the animation (default: true). Set false for logic-only actions. */
    bool bBindActionToAnimation = true;

    /** Set the animation reproduction type (e.g., RootMotion, Warped) at runtime. Useful for dynamic action logic. */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetMontageReproductionType(EMontageReproductionType reproType);

    /** Exits this action, ending its state and allowing queued actions to trigger. */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void ExitAction();

    //
    // ---- KEY DATA MEMBERS ----
    //

    /** All configuration for this action (cost, montage, effect, cooldown, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ACF)
    FActionConfig ActionConfig;

    /** Manager that owns and controls this action (auto-set). */
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    TObjectPtr<class UACFActionsManagerComponent> ActionsManager;

    /** Owning character (auto-set). */
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    TObjectPtr<class ACharacter> CharacterOwner;

    /** Animation montage asset for this action (if any). */
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    TObjectPtr<class UAnimMontage> animMontage;

    /** Montage execution information (section, speed, type, etc.). */
    UPROPERTY(BlueprintReadWrite, Category = ACF)
    FACFMontageInfo MontageInfo;

    /** Gameplay tag uniquely identifying this action. */
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    FGameplayTag ActionTag;

    /** If true, this action is currently being executed. */
    bool bIsExecutingAction = false;

    /** If true, automatically commit (consume) action cost and requirements. */
    bool bAutoCommit = true;

    /** Statistics component of the owning character (auto-set). */
    TObjectPtr<class UARSStatisticsComponent> StatisticComp;

    /** Reference to the cooldown timer for this action (for UI and logic). */
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    FTimerHandle CooldownTimerReference;

    //
    // ---- INTERNAL IMPLEMENTATION ----
    //

    /** Called by ActionsManager when the action is activated. Handles setup, cost, and context. */
    virtual void Internal_OnActivated(class UACFActionsManagerComponent* actionmanger, class UAnimMontage* inAnimMontage, const FString& contextString, AActor* InteractedActor = nullptr, const FGameplayTag& ItemSlotTag = FGameplayTag());

    /** Called by ActionsManager when the action is deactivated. Handles cleanup and state exit. */
    virtual void Internal_OnDeactivated();

    /** Prepares montage information (section, speed, type) before playing montage. */
    void PrepareMontageInfo();

    /** Returns the world from the owning character (helper for timers, GEngine, etc). */
    virtual UWorld* GetWorld() const override { return CharacterOwner ? CharacterOwner->GetWorld() : nullptr; }

    /** Bind animation events for this action (start/stop, notifies). */
    void BindAnimationEvents();

    /** Unbinds animation events for safe cleanup. */
    void UnbinAnimationEvents();

    /** Called when the montage starts (used for event binding). */
    UFUNCTION()
    void HandleMontageStarted(UAnimMontage* inAnimMontage);

    /** Called when the montage finishes (used for event binding and state cleanup). */
    UFUNCTION()
    void HandleMontageFinished(UAnimMontage* _animMontage, bool _bInterruptted);

private:
    /** Tracks whether the action has been terminated (for safe state control). */
    bool bTerminated = false;

    /** Tracks if a sub-action state is currently active. */
    bool bIsInSubState = false;

    /** Returns whether this action has been terminated. (Internal use only!) */
    bool GetTerminated() const { return bTerminated; }

    /** Sets the terminated flag. (Internal use only!) */
    void SetTerminated(bool val) { bTerminated = val; }
};
