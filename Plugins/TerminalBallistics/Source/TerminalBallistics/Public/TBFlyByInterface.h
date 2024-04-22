// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TerminalBallisticsTypes.h"
#include "UObject/Interface.h"

#include "TBFlyByInterface.generated.h"


UINTERFACE(MinimalAPI)
class UTBFlyByInterface : public UInterface
{
	GENERATED_BODY()
};

class TERMINALBALLISTICS_API ITBFlyByInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = FlyByInterface)
	void RecieveFlyByEvent(const FTBFlyBy& FlyBy);
	virtual void RecieveFlyByEvent_Implementation(const FTBFlyBy& FlyBy) {}
};
