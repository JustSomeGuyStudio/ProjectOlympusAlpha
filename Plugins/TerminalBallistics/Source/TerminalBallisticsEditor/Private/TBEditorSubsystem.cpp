// Copyright 2023, Erik Hedberg. All Rights Reserved.


#include "TBEditorSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Settings/LevelEditorPlaySettings.h"

void UTBEditorSubsystem::OnWorldBeginPlay(UWorld& World)
{
	if (World.IsValidLowLevel() && World.WorldType == EWorldType::PIE && World.HasBegunPlay())
	{
		const ULevelEditorPlaySettings* PIESettings = GetDefault<ULevelEditorPlaySettings>();
		EPlayNetMode PlayNetMode;
		PIESettings->GetPlayNetMode(PlayNetMode);
		if (PlayNetMode == EPlayNetMode::PIE_Standalone || World.GetNetMode() == NM_Standalone)
		{
			return; // Early out. We don't care about "Standalone" PIE sessions.
		}

		bool bIsRunUnderOneProcessActive = false;
		PIESettings->GetRunUnderOneProcess(bIsRunUnderOneProcessActive);
		static const FString ErrorMsg = "TerminalBallisticsSubsystem initialized with PIE setting \"bRunUnderOneProccess = true\"\nThis can result in undefined behavior across clients.";
		if (bIsRunUnderOneProcessActive)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(0xFFFF, 120.0f, FColor::Red, ErrorMsg, false);
			}
		}
	}
}
