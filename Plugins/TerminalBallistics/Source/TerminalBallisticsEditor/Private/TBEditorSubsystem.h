// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TBEditorSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class TERMINALBALLISTICSEDITOR_API UTBEditorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void OnWorldBeginPlay(UWorld& World) override;
};
