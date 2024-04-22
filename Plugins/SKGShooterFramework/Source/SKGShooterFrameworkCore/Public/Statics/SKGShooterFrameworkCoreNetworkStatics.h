// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SKGShooterFrameworkCoreNetworkStatics.generated.h"


UCLASS()
class SKGSHOOTERFRAMEWORKCORE_API USKGShooterFrameworkCoreNetworkStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "SKGShooterFrameworkCoreNetworkStatics")
	static FORCEINLINE uint8 CompressFloatToByte(const float Value) { return FRotator::CompressAxisToByte(Value); }
	UFUNCTION(BlueprintPure, Category = "SKGShooterFrameworkCoreNetworkStatics")
	static FORCEINLINE float DecompressByteToFloat(const float Value) { return FRotator::DecompressAxisFromByte(Value); }
};