// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SKGMuzzleHelpers.generated.h"

struct FSKGMuzzleTransform;
/**
 * 
 */
UCLASS()
class SKGMUZZLE_API USKGMuzzleHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "SKGMuzzle|Conversion", meta = (DisplayName = "SKGMuzzleTransform To Transform", CompactNodeTitle = "->", BlueprintAutocast))
	static FTransform Conv_MuzzleTransformToTransform(const FSKGMuzzleTransform& MuzzleTransform);
	UFUNCTION(BlueprintPure, Category = "SKGMuzzle|Conversion", meta = (DisplayName = "Array Of SKGMuzzleTransform To Transform", CompactNodeTitle = "->", BlueprintAutocast))
	static TArray<FTransform> Conv_MuzzleTransformsToTransforms(const TArray<FSKGMuzzleTransform>& MuzzleTransforms);
};
