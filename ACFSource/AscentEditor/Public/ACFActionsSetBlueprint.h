// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "ACFActionsSetBlueprint.generated.h"

/**
 * 
 */
UCLASS()
class ASCENTEDITOR_API UACFActionsSetBlueprint : public UBlueprint
{
	GENERATED_BODY()

public:

	UACFActionsSetBlueprint() {
         BlueprintDisplayName = "Actions Set";	
		 BlueprintDescription = "A database of gameplay actions and abilities that can be assigned to a pawn";
	}

#if WITH_EDITOR
	virtual bool SupportedByDefaultBlueprintFactory() const override {
		return false;
	}
#endif	
	
};
