// Copyright Unreal Bucket 2022. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;
using Microsoft.Win32;

public class ZipUtility : ModuleRules
{
    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../ThirdParty/")); }
    }

    private string SevenZppPath
    {
        get { return Path.GetFullPath(Path.Combine(ThirdPartyPath, "7zpp")); }
    }

    private string VsDirectory
    {
        get
        {
			//string vs2019DefaultBasePath = @"C:\Program Files (x86)\Microsoft Visual Studio\2019";
			string vs2022DefaultBasePath = @"C:\Program Files\Microsoft Visual Studio\2022";
			string vsDefaultBasePath = "";

			if(Directory.Exists(vs2022DefaultBasePath))
			{
				vsDefaultBasePath = vs2022DefaultBasePath;
			}
			//else if(Directory.Exists(vs2019DefaultBasePath))
			//{
			//	vsDefaultBasePath = vs2019DefaultBasePath;
			//}
			else
			{
				vsDefaultBasePath = @"E:\Microsoft Visual Studio\2022";
                return vsDefaultBasePath;
			}

			//Console.WriteLine("ZipUtility Warning: Using default VS path: " + vsPath); //@Bucket
			string vsVersion = Directory.GetDirectories(vsDefaultBasePath)[0];
			string vsPath = Path.Combine(vsDefaultBasePath, vsVersion);
            //Console.WriteLine("ZipUtility Warning: Using default VS path: " + vsPath); //@Bucket
            //Console.WriteLine("ZipUtility Warning: Version: " + vsVersion); //@Bucket
			return vsPath;
        }
    }

    private string ATLPath
    {
        get
        {
            // Trying to find ATL path similar to:
            // C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.14.26428/atlmfc
            string atlPath = "";
            try
            {
                var vsDir = VsDirectory;
                if (!string.IsNullOrEmpty(vsDir))
                {

                    string msvcPath = Path.Combine(vsDir, @"VC\Tools\MSVC\");
                    //Console.WriteLine("ZipUtility Warning: MSVC path: " + msvcPath); //@Bucket
                    string[] msvcDirectories = Directory.GetDirectories(msvcPath);
                    string msvcVersion = msvcDirectories[msvcDirectories.Length - 1];
                    atlPath = Path.Combine(msvcPath, msvcVersion, "atlmfc");
                    //Console.WriteLine("ZipUtility Warning: ATL path: " + atlPath); //@Bucket
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("ZipUtility Error: can't find VS path: " + ex.ToString());
            }

            if (!Directory.Exists(atlPath))
            {
                Console.WriteLine("ZipUtility Error: Couldn't find an ATLPath, fix it in ZipUtility.Build.cs");
            }
            return atlPath;
        }
    }

    public ZipUtility(ReadOnlyTargetRules Target) : base(Target)
    {
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivatePCHHeaderFile = "Private/ZipUtilityPrivatePCH.h";
        bUsePrecompiled = false;

        PublicIncludePaths.AddRange(
            new string[] {
				Path.Combine(ModuleDirectory, "Public"),
				// ... add public include paths required here ...
			}
            );

        PrivateIncludePaths.AddRange(
            new string[] {
				Path.Combine(ModuleDirectory, "Private"),
                Path.Combine(SevenZppPath, "Include"),
				Path.Combine(ATLPath, "include"),
				// ... add other private include paths required here ...
			}
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "WindowsFileUtility"
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "Projects"
				// ... add private dependencies that you statically link with here ...	
			}
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );

        LoadLib(Target);
    }

    public bool LoadLib(ReadOnlyTargetRules Target)
    {
        bool isLibrarySupported = false;

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            isLibrarySupported = true;

            string PlatformSubPath = (Target.Platform == UnrealTargetPlatform.Win64) ? "Win64" : "Win32";
            string LibrariesPath = Path.Combine(SevenZppPath, "Lib");
            string DLLPath = Path.Combine(SevenZppPath, "dll");

            PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, PlatformSubPath, "atls.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, PlatformSubPath, "7zpp_u.lib"));
            //PublicLibraryPaths.Add(Path.Combine(LibrariesPath, PlatformSubPath)); //@Bucket

            PublicDelayLoadDLLs.Add("7z.dll");
            RuntimeDependencies.Add(Path.Combine(DLLPath, PlatformSubPath, "7z.dll"));
        }

        if (isLibrarySupported)
        {
            // Include path
            //PublicIncludePaths.Add(Path.Combine(SevenZppPath, "Include"));
        }

        return isLibrarySupported;
    }
}
