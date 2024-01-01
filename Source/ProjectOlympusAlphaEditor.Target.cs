// Copyright 2023, Dakota Dawe, All rights reserved

using UnrealBuildTool;
using System.Collections.Generic;

public class ProjectOlympusAlphaEditorTarget : TargetRules
{
	public ProjectOlympusAlphaEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V4;

		ExtraModuleNames.AddRange( new string[] { "ProjectOlympusAlpha" } );
	}
}
