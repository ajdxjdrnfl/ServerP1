// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ServerP1 : ModuleRules
{
	public ServerP1(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Sockets", "Networking" });
    }
}
