// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "ACFActionTypes.h"
#include "ARSStatisticsComponent.h"
#include "ARSTypes.h"
#include "Actions/ACFActionsSet.h"
#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"
#include <Engine/DataTable.h>
#include <GameplayTagContainer.h>

#include "ACFActionsManagerComponent.generated.h"

// Forward declaration for base action class
class UACFBaseAction;

// Delegate for broadcasting when an action starts (passes the tag of the started action)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionStarted, FGameplayTag, ActionState);

// Delegate for broadcasting when an action ends (passes the tag of the ended action)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionEnded, FGameplayTag, ActionState);

// Delegate for broadcasting when an action is triggered (passes the tag and priority)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActionTriggered, FGameplayTag, ActionState, EActionPriority, Priority);

/**
 * UACFActionsManagerComponent
 *
 * Main system for managing, executing, and replicating character actions (attacks, abilities, etc.) in Ascent Combat Framework.
 * 
 * Key Features:
 * - Handles action sets, moveset-dependent actions, action triggering, priorities, cooldowns, and animation montage playback.
 * - Supports both common actions and weapon/moveset-specific actions.
 * - Manages action queueing, locking, and substate transitions.
 * - Replicated for multiplayer; all major events are exposed to Blueprint.
 *
 * Usage:
 * - Add to your character (via Blueprint or C++).
 * - Configure ActionsSet and MovesetActions in editor or code.
 * - Use TriggerAction, TriggerActionByName to request actions from input, AI, or gameplay systems.
 * - Listen to OnActionStarted, OnActionFinished, OnActionTriggered for gameplay hooks.
 */
UCLASS(ClassGroup = (ACF), Blueprintable, meta = (BlueprintSpawnableComponent))
class ACTIONSSYSTEM_API UACFActionsManagerComponent : public UActorComponent {
    GENERATED_BODY()

public:
    /** Constructor: Initializes default values for this component (ticking, replication, etc.). */
    UACFActionsManagerComponent();

protected:
    friend class UACFBaseAction;
    /** Called when the game starts; sets up action sets, references, and statistics. */
    virtual void BeginPlay() override;

    /** The character that owns this component (set automatically at BeginPlay). */
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    class ACharacter* CharacterOwner;

    /** Whether this component should tick every frame (can be toggled at runtime, replicated). */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = ACF)
    bool bCanTick = true;

    /** Whether to print debug information when entering/exiting actions (for development/debugging). */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = ACF)
    bool bPrintDebugInfo = false;

    /** Base set of actions (e.g., attacks, blocks, rolls) for this character. */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = ACF)
    TSubclassOf<UACFActionsSet> ActionsSet;

    /** Array of moveset-specific action sets (e.g., for different weapons). */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (TitleProperty = "TagName"), Category = ACF)
    TArray<FActionsSet> MovesetActions;

    /** Instantiated base action set (created at runtime from ActionsSet). */
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    TObjectPtr<UACFActionsSet> ActionsSetInst = nullptr;

    /** Instantiated action sets for each moveset, mapped by tag. */
    UPROPERTY(BlueprintReadOnly, Category = ACF)
    TMap<FGameplayTag, TObjectPtr<UACFActionsSet>> MovesetsActionsInst;

public:
    /** Called every frame, if ticking is enabled. Handles ticking of current action (for combos, charge, etc.). */
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Enables/disables ticking for this component (server authoritative, replicated). */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF")
    void SetCanTick(bool bNewCanTick);

    /** Enables/disables debug print info (server authoritative, replicated). */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF")
    void SetPrintDebugInfo(bool bNewPrintDebugInfo);

    /** Sets the current ActionsSet class for this character (server authoritative, replicated). */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ACF")
    void SetActionsSet(TSubclassOf<UACFActionsSet> NewActionsSet);

    /**
     * Triggers an action by name (FName, will be resolved to GameplayTag).
     * @param ActionTagName      The FName representing the action (should match a registered tag).
     * @param Priority           The action's priority (interrupts lower-priority actions).
     * @param bCanBeStored       If true, queues the action if it can't start immediately.
     * @param contextString      Optional string for context (passed to events).
     */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void TriggerActionByName(FName ActionTagName, EActionPriority Priority = EActionPriority::ELow, bool bCanBeStored = false, const FString& contextString = "");

    /** Locks all actions, preventing new actions from being triggered until unlocked. (Immediate termination of current action.) */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void LockActionsTrigger()
    {
        bIsLocked = true;
        TerminateCurrentAction();
    }

    /** Unlocks actions, allowing them to be triggered again. */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void UnlockActionsTrigger()
    {
        bIsLocked = false;
    }

    /**
     * Main entry point for triggering an action by tag.
     * Handles priorities, queueing, and context.
     * @param ActionState        The GameplayTag describing this action (e.g., "Action.Attack.Heavy").
     * @param Priority           The action's priority.
     * @param bCanBeStored       If true, queues the action if it can't start immediately.
     * @param contextString      Optional context string.
     * @param InteractedActor    Optional actor reference (e.g., for interactions).
     * @param ItemSlotTag        Optional tag for item slot involved.
     */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void TriggerAction(FGameplayTag ActionState, EActionPriority Priority = EActionPriority::ELow, bool bCanBeStored = false, const FString& contextString = "", AActor* InteractedActor = nullptr, FGameplayTag ItemSlotTag = FGameplayTag());

    /** Sets the moveset action set in use, by tag (server authoritative, replicated). */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void SetMovesetActions(const FGameplayTag& movesetActionsTag);

    /** Sets the full moveset action array (server authoritative, replicated). */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void SetMovesetActionArray(const TArray<FActionsSet>& NewMovesetActions);

    /** Gets the currently active moveset actions tag. */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE FGameplayTag GetCurrentMovesetActionsTag() const
    {
        return currentMovesetActionsTag;
    }

    /** Plays a replicated montage (server authoritative, validated). */
    UFUNCTION(Server, WithValidation, Reliable, BlueprintCallable, Category = ACF)
    void PlayReplicatedMontage(const FACFMontageInfo& montageInfo);

    /** Plays a montage on all clients. */
    UFUNCTION(NetMulticast, Reliable)
    void ClientPlayMontage(const FACFMontageInfo& montageInfo);

    /** Stops storing actions (actions will not be queued). */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void StopStoringActions()
    {
        bCanStoreAction = false;
    }

    /** Starts storing actions (actions can be queued if they can't play immediately). */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void StartStoringActions()
    {
        bCanStoreAction = true;
    }

    /** Returns true if the given action is currently on cooldown. */
    UFUNCTION(BlueprintCallable, Category = ACF)
    bool IsActionOnCooldown(FGameplayTag action) const;

    /** Stores an action for later execution (usually after the current action ends). */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void StoreAction(FGameplayTag Action, const FString& contextString = "");

    /** Gets the currently stored (queued) action, if any. */
    UFUNCTION(Blueprintpure, Category = ACF)
    FORCEINLINE FGameplayTag GetStoredAction() const
    {
        return StoredAction;
    }

    /** Returns true if the given action can currently be executed (requirements, cooldown, locks, etc). */
    UFUNCTION(BlueprintCallable, Category = ACF)
    bool CanExecuteAction(FGameplayTag Action, FGameplayTag ItemSlotTag = FGameplayTag()) const;

    /** Exits the current action and, if a queued action exists, launches it. */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void ExitAction();

    /** If current action is 'actionTag', plays its final montage section and terminates the action. */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void ReleaseSustainedAction(FGameplayTag actionTag);

    /** If current action is 'actionTag', plays the specified montage section without terminating it. */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = ACF)
    void PlayMontageSectionFromAction(FGameplayTag actionTag, FName montageSection);

    /** Event called when an action starts (BlueprintAssignable). */
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnActionStarted OnActionStarted;

    /** Event called when an action finishes (BlueprintAssignable). */
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnActionEnded OnActionFinished;

    /** Event called when an action is triggered (BlueprintAssignable). */
    UPROPERTY(BlueprintAssignable, Category = ACF)
    FOnActionTriggered OnActionTriggered;

    /** Gets the currently executing action's tag, or empty if none. */
    UFUNCTION(BlueprintPure, Category = ACF)
    FGameplayTag GetCurrentActionTag() const;

    /** Returns the currently executing action instance (can be nullptr). */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE class UACFBaseAction* GetCurrentAction() const
    {
        return PerformingAction;
    }

    /** Immediately interrupts and stops the current action and its animation. (Server only.) */
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = ACF)
    void StopActionImmeditaley();

    /** Gets the action state by tag (searches moveset and common actions). */
    UFUNCTION(BlueprintCallable, Category = ACF)
    bool GetActionByTag(const FGameplayTag& Action, FActionState& outAction) const;

    /** Plays any VFX/SFX associated with the current action. */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void PlayCurrentActionFX();

    /** Returns true if the character is currently in the specified action state. */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE bool IsInActionState(FGameplayTag state) const
    {
        return CurrentActionTag == state;
    }

    /** Returns true if an action is currently being performed. */
    UFUNCTION(BlueprintPure, Category = ACF)
    FORCEINLINE bool IsPerformingAction() const
    {
        return bIsPerformingAction;
    }

    /** Returns true if currently in a substate of the current action (e.g., parry, charging, etc.). */
    UFUNCTION(BlueprintPure, Category = ACF)
    bool IsInActionSubstate() const;

    /** Called when a notable point in the action's animation or logic is reached (for advanced combos, etc.). */
    void AnimationsReachedNotablePoint();
    
    /** Starts a cooldown for the specified action (prevents immediate retrigger). */
    void StartCooldown(const FGameplayTag& action, UACFBaseAction* actionRef);

    /** Enters a substate (e.g., windup, parry) for the current action. */
    void StartSubState();

    /** Exits the current substate of the action. */
    void EndSubState();

    /** Ends the current action and, if a queued action exists, launches it. */
    void FreeAction();

    /** Gets a moveset-specific action by tag. */
    UFUNCTION(BlueprintCallable, Category = ACF)
    bool GetMovesetActionByTag(const FGameplayTag& action, const FGameplayTag& Moveset, FActionState& outAction) const;

    /** Gets a common (non-moveset) action by tag. */
    UFUNCTION(BlueprintCallable, Category = ACF)
    bool GetCommonActionByTag(const FGameplayTag& action, FActionState& outAction) const;

    /** Adds or modifies an action in the current action set (for dynamic gameplay). */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void AddOrModifyAction(const FActionState& action);

    /** Sets the current priority for action execution (higher = more important). */
    UFUNCTION(BlueprintCallable, Category = ACF)
    void SetCurrentPriority(EActionPriority newPriority);

private:
    /** Internal handler for exiting actions and launching queued actions. */
    void InternalExitAction();

    /** Core logic for launching an action (transitions, VFX, montage, etc.). */
    void LaunchAction(const FGameplayTag& ActionState, const EActionPriority priority, const FString& contextString = "", AActor* InteractedActor = nullptr, const FGameplayTag& ItemSlotTag = FGameplayTag());

    /** Sets the current action tag (internal). */
    void SetCurrentAction(const FGameplayTag& state);

    /** Terminates the current action and cleans up state (internal). */
    void TerminateCurrentAction();

    /** Animation instance for the owning character (used for montage playback). */
    UPROPERTY()
    class UAnimInstance* animInst;

    /** Multicast: Notifies clients when an action starts (with context). */
    UFUNCTION(NetMulticast, Reliable)
    void ClientsReceiveActionStarted(const FGameplayTag& ActionState, const FString& contextString);

    /** Multicast: Notifies clients when an action ends. */
    UFUNCTION(NetMulticast, Reliable)
    void ClientsReceiveActionEnded(const FGameplayTag& ActionState);

    /** Multicast: Notifies clients to immediately stop any current action. */
    UFUNCTION(NetMulticast, Reliable)
    void ClientsStopActionImmeditaley();

    /** Whether an action is currently being performed (replicated across network). */
    UPROPERTY(Replicated)
    bool bIsPerformingAction;

    /** Pointer to the currently performing action (may be nullptr). */
    UPROPERTY()
    TObjectPtr<UACFBaseAction> PerformingAction;

    /** State struct for the currently active action. */
    UPROPERTY()
    FActionState CurrentActionState;

    /** Tag for the currently active action (replicated). */
    UPROPERTY(Replicated)
    FGameplayTag CurrentActionTag;

    /** Stores a queued action for later execution (after current action ends). */
    FGameplayTag StoredAction;

    /** Stores a context string for the queued (stored) action. */
    FString StoredString;

    /** Current action execution priority (replicated). */
    UPROPERTY(Replicated)
    int32 CurrentPriority;

    /** Current moveset actions tag (replicated). */
    UPROPERTY(Replicated)
    FGameplayTag currentMovesetActionsTag;

    /** Whether actions can be stored for later execution (default: true). */
    UPROPERTY()
    bool bCanStoreAction = true;

    /** Reference to the statistics component for the owning character (requirements, costs, etc.). */
    UPROPERTY()
    class UARSStatisticsComponent* StatisticComp;

    /** Prints debug info for action transitions (if enabled). */
    void PrintStateDebugInfo(bool bIsEntring);

    /** Plays the current montage (handles root motion, warping, etc.). */
    void PlayCurrentMontage();

    /** Prepares root motion warping for current action's montage (if any). */
    void PrepareWarp();

    /** List of actions currently on cooldown (prevents immediate retrigger). */
    UPROPERTY()
    TArray<FGameplayTag> onCooldownActions;

    /** Replicated montage info for synchronizing animation playback. */
    UPROPERTY(ReplicatedUsing = OnRep_MontageInfo)
    FACFMontageInfo MontageInfo;

    /** Handler for montage info replication notification. */
    UFUNCTION()
    void OnRep_MontageInfo();

    /** Handler called when a cooldown finishes for an action. */
    UFUNCTION()
    void OnCooldownFinished(const FGameplayTag& action);

    /** Helper to stop the current animation immediately. */
    void Internal_StopCurrentAnimation();

    /** Whether action triggering is currently locked (prevents input/AI from launching actions). */
    bool bIsLocked = false;
};