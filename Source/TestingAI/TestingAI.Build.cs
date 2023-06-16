// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TestingAI : ModuleRules
{
	public TestingAI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "EnhancedInput", "Json", "TraceAnalysis","AIModule", "NavigationSystem" });
        PublicDefinitions.Add("WITH_ENGINE_STATS=1");
    }
}
