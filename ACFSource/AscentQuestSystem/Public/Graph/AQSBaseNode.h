// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "AGSGraphNode.h"
#include "AQSBaseNode.generated.h"

/**
 * 
 */
UCLASS(abstract)
class ASCENTQUESTSYSTEM_API UAQSBaseNode : public UAGSGraphNode
{
	GENERATED_BODY()

public:

	virtual void ActivateNode() override;

	virtual void DeactivateNode() override;

	virtual bool CanBeActivated() const;

	UAQSBaseNode();

	UFUNCTION(BlueprintPure, Category = AQS)
	FORCEINLINE bool IsCompleted() const {
		return bIsCompleted;
	}

protected:

	UPROPERTY(EditDefaultsOnly, Instanced, Category = AQS)
	TArray<class UAGSAction*> ActivationActions;
	
	bool bIsCompleted;
};
