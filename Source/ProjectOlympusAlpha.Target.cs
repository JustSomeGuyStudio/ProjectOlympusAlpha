// Copyright 2023, Dakota Dawe, All rights reserved

using UnrealBuildTool;
using System.Collections.Generic;

public class ProjectOlympusAlphaTarget : TargetRules
{
	public ProjectOlympusAlphaTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V4;

		ExtraModuleNames.AddRange( new string[] { "ProjectOlympusAlpha" } );
	}
}
