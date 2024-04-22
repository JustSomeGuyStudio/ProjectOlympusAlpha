// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "Types/TBSpreadParams.h"
#include "MathUtils.h"



double FTBSpreadParams::GetDistance() const
{
    return Distance;
}

FVector2D FTBSpreadParams::GetSpreadAngle() const
{
    if (SpreadAngle.ComponentwiseAllLessOrEqual(FVector2D(-1)))
    {
        if (Distance <= 0.0)
        {
            SpreadAngle = FVector2D::ZeroVector;
        }
        else
        {
            const double DistanceInMeters = (1.0 / 0.0254) * (36 * Distance);
            const FVector2D SpreadInMeters = (1.0 / 0.0254) * GetSpread();
            const FVector2D Slope = SpreadInMeters / DistanceInMeters;
            SpreadAngle = TB::MathUtils::atan(Slope);
        }
    }

    return SpreadAngle;
}

FVector2D FTBSpreadParams::GetSpread() const
{
    return FVector2D(HorizontalSpread, VerticalSpread);
}

FVector2D FTBSpreadParams::GetSpread(const double DistanceMeters) const
{
    if (Distance <= 0.0)
    {
        return GetSpread();
    }
    else
    {
        const double DistanceInMeters = (1.0 / 0.0254) * (36 * Distance);
        const FVector2D SpreadInMeters = (1.0 / 0.0254) * GetSpread();
        const FVector2D Slope = SpreadInMeters / DistanceInMeters;
        return Slope * DistanceMeters;
    }
}

FVector2D FTBSpreadParams::GenerateSpreadAtDistance(const double DistanceMeters) const
{
    const FVector2D UnalteredSpread = GetSpread(DistanceMeters);
    return UnalteredSpread + (Variance * DistanceMeters);
}

FVector FTBSpreadParams::GenerateSpreadVector(const FVector& Direction) const
{
    const FVector2D RandVariance = FVector2D(FMath::RandRange(-Variance.X, Variance.X), FMath::RandRange(-Variance.Y, Variance.Y));
    const FVector2D HalfAngle = GetSpreadAngle() * RandVariance;
    return TB::MathUtils::VectorUtils::VRandConeGuassian(Direction, HalfAngle.X, HalfAngle.Y);
}

bool FTBSpreadParams::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
    bOutSuccess = true;
    Ar << *this;
    return true;
}

FArchive& operator<<(FArchive& Ar, FTBSpreadParams& SpreadParams)
{
    Ar << SpreadParams.HorizontalSpread;
    Ar << SpreadParams.VerticalSpread;
    Ar << SpreadParams.Distance;
    Ar << SpreadParams.Variance;
    if (Ar.IsLoading())
    {
        SpreadParams.SpreadAngle = FVector2D(-1.0);
        SpreadParams.GetSpreadAngle();
    }
    return Ar;
}



FVector2D UTBSpreadParamsStatics::GetSpreadAngle(const FTBSpreadParams& SpreadParams)
{
    return SpreadParams.GetSpreadAngle();
}

FVector2D UTBSpreadParamsStatics::GetBaseSpread(const FTBSpreadParams& SpreadParams)
{
    return SpreadParams.GetSpread();
}

FVector2D UTBSpreadParamsStatics::GetSpread(const FTBSpreadParams& SpreadParams, const double DistanceMeters, const bool bIncludeVariance)
{
    if (bIncludeVariance)
    {
        return SpreadParams.GenerateSpreadAtDistance(DistanceMeters);
    }
    else
    {
        return SpreadParams.GetSpread(DistanceMeters);
    }
}

FVector UTBSpreadParamsStatics::GenerateSpreadVector(const FTBSpreadParams& SpreadParams, const FVector& Direction)
{
    return SpreadParams.GenerateSpreadVector(Direction);
}
