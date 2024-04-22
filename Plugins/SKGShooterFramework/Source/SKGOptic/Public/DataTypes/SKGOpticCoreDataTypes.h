// Copyright 2023, Dakota Dawe, All rights reserved
#pragma once

#include "SKGOpticCoreDataTypes.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;

USTRUCT(BlueprintType)
struct FSKGOpticReticle
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic")
	TObjectPtr<UMaterialInterface> ReticleMaterial;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicReticleMaterial;
	float StartingReticleSizeParameterValue {1.0f};
	float StartingEyeboxRangeParameterValue {-2000.0f};

	operator bool () const
	{
		return DynamicReticleMaterial != nullptr;
	}
};