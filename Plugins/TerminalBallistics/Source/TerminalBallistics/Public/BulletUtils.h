// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Bullets.h"
#include "BulletUtils.generated.h"

UCLASS()
class TERMINALBALLISTICS_API UBulletUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics")
	static FTBBulletPhysicalProperties GetBulletFromType(ETBBulletCaliber Type);

	static FTBBullet GetFullBulletFromType(ETBBulletCaliber Type, TArray<ETBBulletVariation> Variation = {});

	static FTBBullet GetFullBulletFromName(ETBBulletNames Name);

	static ETBBulletNames GetNameFromBullet(const FTBBullet& Bullet);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics", meta = (ReturnDisplayName = "Muzzle Velocity (m/s)"))
	static double GetTypicalMuzzleVelocityForBullet(const ETBBulletCaliber Caliber);
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics", meta = (ReturnDisplayName = "Effective Range (meters)"))
	static double GetTypicalEffectiveRangeForBullet(const ETBBulletCaliber Caliber);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics")
	static void GetTypicalMuzzleVelocityAndEffectiveRangeForBullet(const ETBBulletCaliber Caliber, double& MuzzleVelocity, double& EffectiveRange);
};