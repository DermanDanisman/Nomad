// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#include "Actions/ACFActionsSet.h"
#include "ACFActionTypes.h"
#include "GameplayTagContainer.h"

/**
 * Retrieves an action by tag from the actions set.
 * @param Action - The gameplay tag to search for.
 * @param outAction - Receives the found action state.
 * @return true if found, false otherwise.
 */
bool UACFActionsSet::GetActionByTag(const FGameplayTag& Action, FActionState& outAction) const
{
    const FActionState* actionState = Actions.FindByKey(Action);
    if (actionState)
    {
        outAction = *actionState;
        return true;
    }
    return false;
}

/**
 * Adds a new action or updates an existing one with the same tag.
 * Ensures there are no duplicate actions for a given tag.
 * @param action - The action state to add or modify.
 */
void UACFActionsSet::AddOrModifyAction(const FActionState& action)
{
    if (Actions.Contains(action.TagName))
    {
        Actions.Remove(action);
    }
    Actions.AddUnique(action);
}