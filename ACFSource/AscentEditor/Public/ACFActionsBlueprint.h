// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "ACFActionsBlueprint.generated.h"

/**
 * 
 */
UCLASS()
class ASCENTEDITOR_API UACFActionsBlueprint : public UBlueprint
{
	GENERATED_BODY()

public: 

	UACFActionsBlueprint() {
       //  BlueprintDisplayName = "Action";	
		 BlueprintDescription = "The base class to create gameplay actions and abilities";
	}

#if WITH_EDITOR
	virtual bool SupportedByDefaultBlueprintFactory() const override {
		return false;
	}
#endif	
};


