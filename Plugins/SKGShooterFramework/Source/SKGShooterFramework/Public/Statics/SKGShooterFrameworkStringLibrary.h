// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "DataTypes/SKGShooterFrameworkDataTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SKGShooterFrameworkStringLibrary.generated.h"

/**
 * 
 */
UCLASS()
class SKGSHOOTERFRAMEWORK_API USKGShooterFrameworkStringLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "SKGStringLibrary", meta = (DisplayName = "To String (Firearm Stats)", CompactNodeTitle = "->", BlueprintAutocast))
	static FString Conv_FirearmStatsToString(const FSKGFirearmStats& InStats) { return InStats.ToString(); }
};
