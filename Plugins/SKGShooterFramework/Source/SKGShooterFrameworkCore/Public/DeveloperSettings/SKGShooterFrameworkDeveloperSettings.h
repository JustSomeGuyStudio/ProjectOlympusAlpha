// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/DeveloperSettings.h"
#include "SKGShooterFrameworkDeveloperSettings.generated.h"

class UMaterialParameterCollection;
USTRUCT(BlueprintType)
struct FSKGNightVisionMaterialParameterCollection
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NightVision")
	TObjectPtr<UMaterialParameterCollection> NightVisionMaterialParameterCollection;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "SKG Shooter Framework Developer Settings"))
class SKGSHOOTERFRAMEWORKCORE_API USKGShooterFrameworkDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	USKGShooterFrameworkDeveloperSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual FName GetCategoryName() const override { return FName("Plugins"); }
	
	// The collision channel to be used for firearm collision
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "FirearmCollision")
	TEnumAsByte<ECollisionChannel> FirearmCollisionChannel {ECC_Visibility};
	// The collision channel for the laser to use to detect blocking hits
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LightLaser")
	TEnumAsByte<ECollisionChannel> LaserCollisionChannel {ECC_Visibility};
	// The collision channel for the "laser" of the range finder to use to detect blocking hits
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RangeFinder")
	TEnumAsByte<ECollisionChannel> RangeFinderCollisionChannel {ECC_Visibility};
	// The material parameter collection to be used for handling infrared (when you turn night vision on/off)
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "NightVision")
	FSKGNightVisionMaterialParameterCollection NightVisionMaterialParameterCollection;
};
