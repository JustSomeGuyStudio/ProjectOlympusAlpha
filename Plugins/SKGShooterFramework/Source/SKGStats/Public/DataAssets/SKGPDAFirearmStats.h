// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SKGPDAFirearmStats.generated.h"

/**
 * 
 */
UCLASS()
class SKGSTATS_API USKGPDAFirearmStats : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SKGFirearmStats")
	float Weight {0.0f};
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SKGFirearmStats")
	float Durability {100.0f};
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SKGFirearmStats")
	float Ergonomics {50.0f};
	// Accuracy is in MOA
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SKGFirearmStats")
	float Accuracy {2.0f};
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SKGFirearmStats")
	float VerticalRecoil {100.0f};
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SKGFirearmStats")
	float HorizontalRecoil {100.0f};
	// Rounds per minute
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SKGFirearmStats")
	float FireRate {600.0f};
};
