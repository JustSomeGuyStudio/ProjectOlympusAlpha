// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SKGOpticLibrary.generated.h"

/**
 * 
 */
UCLASS()
class SKGOPTIC_API USKGOpticLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "SKGOpticLibrary")
	static float MagnificationToFOVAngle(const float Magnification);
};
