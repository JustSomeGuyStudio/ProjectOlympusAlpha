// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SKGInfraredInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class SKGSHOOTERFRAMEWORKCORE_API USKGInfraredInterface : public UInterface
{
	GENERATED_BODY()
};


class SKGSHOOTERFRAMEWORKCORE_API ISKGInfraredInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual bool IsInfraredModeOnForDevice() const { return false; }
	// Called when something like night vision is enabled
	virtual void OnInfraredEnabledForPlayer() = 0;
	// Called when something like night vision is disabled
	virtual void OnInfraredDisabledForPlayer() = 0;
};
