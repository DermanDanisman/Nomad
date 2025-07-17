// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using UnrealBuildTool;

public class NomadDev : ModuleRules
{
	public NomadDev(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "AscentCombatFramework", "AscentQuestSystem", "OnlineSubsystem", "OnlineSubsystemSteam", "OnlineSubsystemUtils", "UMG", "CommonUI", "Niagara"});
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput", 
			"CharacterCustomization", 
			"GameplayTags", 
			"GameplayAbilities", 
			"GameplayTasks", 
			"AdvancedRPGSystem", 
			"InventorySystem",
			"CollisionsManager",
			"AscentCoreInterfaces",
			"ActionsSystem", 
			"AscentCombatFramework",
			"CharacterController",
			"AscentMapsSystem",
			"StatusEffectSystem",
			"CraftingSystem",
			"AscentSaveSystem",
			"MultiplayerSessions",

		});
	}
}
