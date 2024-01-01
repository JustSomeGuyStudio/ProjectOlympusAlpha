// Copyright 2022-2023 Dominik Lips. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class GMCCore : ModuleRules
{
  public GMCCore(ReadOnlyTargetRules Target) : base(Target)
  {
    // The logging setup partially depends on the order of macro definitions (which can break with unity builds). Setting the override to a high value will
    // effectively disable unity builds for this module.
    MinSourceFilesForUnityBuildOverride = Int32.MaxValue;

    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

    PrivatePCHHeaderFile = "Private/GMCCore.pch";

    PublicDependencyModuleNames.AddRange(new[] {
      "Core",
      "CoreUObject",
      "Engine",
      "InputCore",
      "NetCore",
      "PhysicsCore",
      "SlateCore",
      "AnimGraphRuntime",
      "AIModule",
      "UMG",
      "EnhancedInput",
      "GameplayTags"
    });

    // Public include directories.
    PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
    PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/Actors"));
    PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/Components"));
    PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/Replication"));
    PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/Utility"));
    PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/Widgets"));

    // Private includes directories.
    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));
    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Actors"));
    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Components"));
    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Debug"));
    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Replication"));
    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Utility"));
    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Widgets"));

    // Uncomment to activate additional check macros throughout the plugin.
    //PublicDefinitions.Add("GMC_DO_CHECK");
  }
}
