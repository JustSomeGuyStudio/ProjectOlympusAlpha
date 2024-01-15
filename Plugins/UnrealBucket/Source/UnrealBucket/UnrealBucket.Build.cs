// Copyright Unreal Bucket 2022. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;

public class UnrealBucket : ModuleRules
{
    public UnrealBucket(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bUsePrecompiled = false;

        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
            }
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
            }
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "ZipUtility",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "AssetRegistry",
                "Blutility",
                "CoreUObject",
                "Engine",
				"InputCore",
				"LevelEditor",
				"ContentBrowser",
                "Projects",
                "Slate",
				"SlateCore",
                "UMG",
				"UMGEditor",
                "UnrealEd",
                "HTTP",
                "Json",
                "JsonUtilities",
                "ApplicationCore",
				"EditorStyle",
				"PropertyEditor",
                
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

#if UE_4_26_OR_LATER
        PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "DeveloperSettings",
			}
			);
#endif
#if UE_5_3_OR_LATER
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "ScriptableEditorWidgets",
            }
            );
#endif
    }
}
