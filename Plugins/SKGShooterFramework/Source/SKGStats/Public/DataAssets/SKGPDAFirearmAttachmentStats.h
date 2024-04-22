// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SKGPDAFirearmAttachmentStats.generated.h"

/**
 * 
 */
UCLASS()
class SKGSTATS_API USKGPDAFirearmAttachmentStats : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SKGFirearmAttachmentStats")
	float Weight {0.0f};
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SKGFirearmAttachmentStats")
	float Ergonomics {0.0f};
	// Accuracy is in MOA
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SKGFirearmAttachmentStats")
	float Accuracy {0.0f};
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SKGFirearmAttachmentStats")
	float VerticalRecoil {0.0f};
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SKGFirearmAttachmentStats")
	float HorizontalRecoil {0.0f};
	// Higher = faster muzzle velocity (such as going from 11.5in barrel to 20in barrel)
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SKGFirearmAttachmentStats")
	float MuzzleVelocityPercentage {0.0f};
	// Rounds per minute
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SKGFirearmAttachmentStats")
	float FireRateChangePercent {0.0f};
};
