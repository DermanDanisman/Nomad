// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Graph/ADSDialogue.h"
#include "ADSDialogueWidgetInterface.generated.h"

/**
 * 
 */
 // This class does not need to be modified.
UINTERFACE(Blueprintable)
class UADSDialogueWidgetInterface : public UInterface {
    GENERATED_BODY()
};


class ASCENTDIALOGUESYSTEM_API IADSDialogueWidgetInterface 
{
	GENERATED_BODY()

public: 

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = ADS)
	void SetupWithDialogue(UADSDialogue* dialogueToPlay);
};
