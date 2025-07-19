// Copyright (C) Developed by Gamegine, Published by Gamegine 2025. All Rights Reserved.

#include "Core/FunctionLibrary/NomadStatusEffectGameplayHelpers.h"

#include "ARSStatisticsComponent.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Components/ACFCharacterMovementComponent.h"
#include "Core/StatusEffect/Component/NomadStatusEffectManagerComponent.h"
#include "Core/Component/NomadSurvivalNeedsComponent.h"
// Movement speed modifications are now handled through existing status effect types with configs
#include "Core/StatusEffect/NomadBaseStatusEffect.h"
#include "GameFramework/CharacterMovementComponent.h"

