// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TerminalBallisticsTraits.h"

#include "TBSpreadParams.generated.h"


USTRUCT(BlueprintType)
struct TERMINALBALLISTICS_API FTBSpreadParams
{
	GENERATED_BODY()
protected:
	/* How much the projectiles are spread apart horizontally */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Spread, meta = (Units = "Inches", ClampMin = "0", UIMin = "0"))
	double HorizontalSpread = 2.5f;
	/* How much the projectiles are spread apart vertically */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Spread, meta = (Units = "Inches", ClampMin = "0", UIMin = "0"))
	double VerticalSpread = 2.5f;
	/* Distance at which the spread was measured */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Spread, meta = (Units = "Yards", ClampMin = "0", UIMin = "0"))
	double Distance = 10.f;

	UPROPERTY()
	mutable FVector2D SpreadAngle = FVector2D(-1.0);

	/* Random variation applied to spread. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Spread, meta = (Units = "Degrees", ClampMin = "0", UIMin = "0"))
	FVector2D Variance = FVector2D(0.2);

public:
	FTBSpreadParams() = default;
	FTBSpreadParams(const double HorizontalSpread, const double VerticalSpread, const double Distance, const FVector2D& Variance)
		: HorizontalSpread(HorizontalSpread)
		, VerticalSpread(VerticalSpread)
		, Distance(Distance)
		, Variance(Variance)
	{}

	double GetDistance() const;
	FVector2D GetSpreadAngle() const;
	/* Returns the Horizonal and Vertical components of the spread */
	FVector2D GetSpread() const;
	/**
	* Returns the Horizonal and Vertical components of the spread at a given distance, ignoring variance
	* @param DistanceMeters		Distance (m)
	*/
	FVector2D GetSpread(const double DistanceMeters) const;

	/**
	* Determines the Horizontal and Vertical components of the spread at a given distance, including random variance if applicable
	* @param DistanceMeters		Distance (m)
	*/
	FVector2D GenerateSpreadAtDistance(const double DistanceMeters) const;

	FVector GenerateSpreadVector(const FVector& Direction) const;

	TB_WITH_OPTIMIZED_SERIALIZER();
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

	friend FArchive& operator<<(FArchive& Ar, FTBSpreadParams& SpreadParams);

	bool operator==(const FTBSpreadParams& other) const
	{
		return HorizontalSpread == other.HorizontalSpread
			&& VerticalSpread == other.VerticalSpread
			&& Distance == other.Distance
			&& Variance.Equals(other.Variance);
	}

	bool operator!=(const FTBSpreadParams& other) const
	{
		return !(*this == other);
	}
};


UCLASS()
class TERMINALBALLISTICS_API UTBSpreadParamsStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics", meta = (BlueprintThreadSafe))
	static FVector2D GetSpreadAngle(const FTBSpreadParams& SpreadParams);
	/**
	* Returns the Horizonal and Vertical components of the spread at a given distance, ignoring variance
	* @param DistanceMeters		Distance (m)
	*/
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics", meta = (BlueprintThreadSafe))
	static FVector2D GetBaseSpread(const FTBSpreadParams& SpreadParams);
	/**
	* Returns the Horizonal and Vertical components of the spread at a given distance, ignoring variance
	* @param DistanceMeters		Distance (m)
	* @param bIncludeVariance	Whether or not to include random variance in spread
	*/
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics", meta = (BlueprintThreadSafe))
	static FVector2D GetSpread(const FTBSpreadParams& SpreadParams, const double DistanceMeters, const bool bIncludeVariance = true);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics", meta = (BlueprintThreadSafe))
	static FVector GenerateSpreadVector(const FTBSpreadParams& SpreadParams, const FVector& Direction);
};

