// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SKGShooterFrameworkCoreMiscStatics.generated.h"

/**
 * 
 */
UCLASS()
class SKGSHOOTERFRAMEWORKCORE_API USKGShooterFrameworkCoreMiscStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "USKGShooterFrameworkCoreMiscStatics", meta = (WorldContext = "WorldContextObject"))
	static bool IsWorldTearingDown(const UObject* WorldContextObject);
};
