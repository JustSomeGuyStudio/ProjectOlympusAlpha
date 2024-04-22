// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveBase.h"
#include "SKGCurveVectorRotator.generated.h"

USTRUCT(BlueprintType)
struct FSKGVectorRotator
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "SKGCurveVectorRotator")
	FVector Vector {FVector::ZeroVector};
	UPROPERTY(BlueprintReadWrite, Category = "SKGCurveVectorRotator")
	FRotator Rotator {FRotator::ZeroRotator};
};

UCLASS()
class SKGSHOOTERFRAMEWORKCORE_API USKGCurveVectorRotator : public UCurveBase
{
	GENERATED_BODY()

	/** Keyframe data, one curve for X, Y, Z, Pitch, Yaw, Roll */
	UPROPERTY()
	FRichCurve FloatCurves[6];

	/** Evaluate this float curve at the specified time */
	UFUNCTION(BlueprintCallable, Category="Math|Curves")
	FSKGVectorRotator GetVectorRotatorValue(float InTime) const;
	/** Evaluate this float curve at the specified time */
	UFUNCTION(BlueprintCallable, Category="Math|Curves")
	FVector GetVectorValue(float InTime) const;
	/** Evaluate this float curve at the specified time */
	UFUNCTION(BlueprintCallable, Category="Math|Curves")
	FRotator GetRotatorValue(float InTime) const;

	// Begin FCurveOwnerInterface
	virtual TArray<FRichCurveEditInfoConst> GetCurves() const override;
	virtual TArray<FRichCurveEditInfo> GetCurves() override;

	/** Determine if Curve is the same */
	bool operator == (const USKGCurveVectorRotator& Curve) const;

	virtual bool IsValidCurve(FRichCurveEditInfo CurveInfo) override;
};
