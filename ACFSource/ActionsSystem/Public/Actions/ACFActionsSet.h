// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ACFActionTypes.h"
#include "Engine/DataAsset.h"
#include "ACFActionsSet.generated.h"

/**
 * UACFActionsSet
 *
 * Data asset that serves as a repository for a group of actions (FActionState) that can be referenced by a character, weapon, or moveset.
 * 
 * Usage:
 * - Assign this to a character, weapon, or moveset to define its available actions.
 * - Use AddOrModifyAction to dynamically update actions at runtime (e.g., buffs, unlocks).
 * - Use GetActionByTag to retrieve an action definition by its gameplay tag.
 * - Use GetActions to retrieve the full list of defined actions.
 *
 * Blueprintable and Editor-friendly: Actions can be edited in data assets.
 */
UCLASS(BlueprintType, Blueprintable, Category = ACF)
class ACTIONSSYSTEM_API UACFActionsSet : public UPrimaryDataAsset {
    GENERATED_BODY()

protected:
    /** The array of all possible actions defined in this set (editable in editor) */
    UPROPERTY(EditAnywhere, meta = (TitleProperty = "TagName"), BlueprintReadWrite, Category = "ACF | Actions")
    TArray<FActionState> Actions;

public:
    /**
     * Adds a new action or replaces an existing action with the same tag.
     * If an action with the same TagName exists, it is removed and replaced.
     * @param action - The action state to add or modify.
     */
    void AddOrModifyAction(const FActionState& action);

    /**
     * Looks for an action by its tag.
     * @param action - The gameplay tag to search for.
     * @param outAction - The found action state (if any).
     * @return true if found, false otherwise.
     */
    bool GetActionByTag(const FGameplayTag& action, FActionState& outAction) const;

    /**
     * Retrieves all actions in this set.
     * @param outActions - Array populated with all actions.
     */
    void GetActions(TArray<FActionState>& outActions) const
    {
        outActions = Actions;
    }
};