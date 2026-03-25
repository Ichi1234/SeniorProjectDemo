// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SeniorProjectDemo : ModuleRules
{
	public SeniorProjectDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Json",
            "JsonUtilities"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "HTTP"
        });
    }
}
