// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SKGShooterFramework : ModuleRules
{
	public SKGShooterFramework(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		IWYUSupport = IWYUSupport.Full;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"NetCore",
				"GameplayTags",
				"DeveloperSettings",
				"SKGShooterFrameworkCore",
				"SKGAttachment",
				"SKGLightLaser",
				"SKGProceduralAnim",
				"SKGMuzzle",
				"SKGOptic",
				"SKGProjectile",
				"SKGStats",
				"SKGStock"
			}
			);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine"
			}
			);
	}
}
