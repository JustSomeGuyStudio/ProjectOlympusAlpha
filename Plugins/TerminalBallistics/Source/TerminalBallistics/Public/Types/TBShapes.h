// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TerminalBallisticsTraits.h"
#include <variant>
#include "TBShapes.generated.h"


USTRUCT(BlueprintType)
struct TERMINALBALLISTICS_API FTBCylinder
{
	GENERATED_BODY()
	TB_WITH_OPTIMIZED_SERIALIZER();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Cylinder, meta = (Units = "cm"))
	/* Radius in cm */
	double Radius = 0.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Cylinder, meta = (Units = "cm"))
	/* Length in cm */
	double Length = 2.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Cylinder)
	double Density = 0.0;

protected:
	mutable double CSA = -1;
	mutable double Volume = -1;

public:
	FTBCylinder()
	{
		RecalculateValues();
	}

	FTBCylinder(double InRadius, double InLength)
		: Radius(InRadius)
		, Length(InLength)
	{
		RecalculateValues();
	}

	/**
	* Returns the cross sectional area of this cylinder perpendicular to it's axis of revolution (m^2).
	* @param bForceRecalculate	If true, this value will be recalculated.
	*/
	double GetCSA(const bool bForceRecalculate = false) const;

	/**
	* Returns the volume of this cylinder in cubic meters.
	* @param bForceRecalculate	If true, this value will be recalculated.
	*/
	double GetVolume(const bool bForceRecalculate = false) const;

	/**
	* Returns the drag coefficent of this cylinder perpendicular to it's axis of revolution.
	*/
	double GetCD() const;

	double GetFrontalCd() const;

	double GetCenterOfMass() const
	{
		return Length / 200.0;
	}

	FVector GetInertia(const double Mass) const;

	void RecalculateValues();

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSucces)
	{
		Ar << *this;

		bOutSucces = true;
		return true;
	}

	friend FArchive& operator<<(FArchive& Ar, FTBCylinder& Cylinder)
	{
		Ar << Cylinder.Radius << Cylinder.Length << Cylinder.Density;
		Cylinder.RecalculateValues();
		return Ar;
	}

	bool operator==(const FTBCylinder& other) const
	{
		return Radius == other.Radius
			&& Length == other.Length;
	}

	bool operator!=(const FTBCylinder& other) const
	{
		return !(*this == other);
	}
};
template<> struct TStructOpsTypeTraits<FTBCylinder> : public TStructOpsTypeTraitsBase2<FTBCylinder>
{
	enum
	{
		WithIdenticalViaEquality = true,
		WithNetSerializer = true,
		WithNetSharedSerialization = true
	};
};


struct FTBEllipticalNose
{
	double MinorRadius = 0.0;
	double MajorRadius = 0.0;

	double CSA = 0.0;

protected:
	mutable double Volume = -1;
	mutable double SurfaceArea = -1;

public:
	FTBEllipticalNose() = default;

	FTBEllipticalNose(const double BaseRadius, const double Length, const double CSA)
		: MinorRadius(BaseRadius)
		, MajorRadius(Length)
		, CSA(CSA)
	{
		RecalculateValues();
	}

	double GetShapeLength() const;

	double GetShapeRadius(const double x) const;

	double GetShapeRadius() const
	{
		return MinorRadius;
	}

	double GetShapeVolume(const bool bForceRecalculate = false) const;

	double GetRadiusMeters() const
	{
		return MinorRadius / 100.0;
	}

	double GetShapeSurfaceArea(const bool bForceRecalculate = false) const;

	double GetShapeCd() const
	{
		return 0.59;
	}

	double GetShapeCSA() const
	{
		return CSA;
	}

	double GetShapeFrontalCSA() const;

	void RecalculateValues()
	{
		GetShapeVolume(true);
		GetShapeSurfaceArea(true);
	}
};


//TODO: Refine this system so it can be used to calculate ballistic parameters
USTRUCT(BlueprintType)
struct TERMINALBALLISTICS_API FTBOgive
{
	GENERATED_BODY()
	TB_WITH_OPTIMIZED_SERIALIZER();
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Ogive)
	// Radius (cm)
	mutable double Radius = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Ogive)
	// Ogive radius (cm)
	double OgiveRadius = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Ogive)
	// Cross sectional area (m^2)
	double CSA = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Ogive)
	// Ogive Length (cm)
	double Length = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Ogive)
	FTBCylinder Cylinder = FTBCylinder();

protected:
	// Cache these to avoid extra computation

	mutable double FrontalCSA = -1;
	mutable double Cd = -1;
	mutable double Volume = -1;

	mutable double alpha = -1;
	mutable double cosAlpha = -1;
	mutable double sinAlpha = -1;
	mutable double SurfaceArea = -1;

public:
	FTBOgive()
	{
		RecalculateValues();
	}

	FTBOgive(double Radius, double InOgiveRadius, double Length, double CSA, FTBCylinder Cylinder)
		: Radius(Radius)
		, OgiveRadius(InOgiveRadius)
		, CSA(CSA)
		, Length(Length)
		, Cylinder(Cylinder)
	{
		if (OgiveRadius < 2.0 * Radius) // Not a valid ogive
		{
			OgiveRadius = CalculateOgiveRadius();
		}
		RecalculateValues();
	}

	FTBOgive(double Radius, double Length, double CSA, FTBCylinder Cylinder)
		: Radius(Radius)
		, CSA(CSA)
		, Length(Length)
		, Cylinder(Cylinder)
	{
		OgiveRadius = CalculateOgiveRadius();
		RecalculateValues();
	}

	~FTBOgive() = default;

	double GetSharpness() const
	{
		return OgiveRadius / (2 * Cylinder.Radius);
	}

	double GetArcCenterHeight() const
	{
		return OgiveRadius - (2 * Cylinder.Radius);
	}

	bool IsTangentOgive() const
	{
		return GetArcCenterHeight() == 0;
	}

	double GetCSA() const
	{
		return CSA + Cylinder.GetCSA();
	}

	double GetShapeCSA() const
	{
		return CSA;
	}

	// Returns the surface area (minus the base) in m^2
	double GetSurfaceArea(const bool bForceRecalculate = false) const;

	double GetShapeSurfaceArea(const bool bForceRecalculate = false) const;

protected:
	double GetAlpha(const bool bForceRecalculate = false) const;

	double GetSinAlpha(const bool bForceRecalculate = false) const;

	double GetCosAlpha(const bool bForceRecalculate = false) const;

	double CalculateOgiveRadius();

public:
	double GetRadius() const
	{
		return fmax(Radius, Cylinder.Radius);
	}

	double GetRadiusMeters() const
	{
		return GetRadius() / 100.0;
	}

	/**
	* Returns the radius of the ogive. (cm)
	* @param x	The distance from the tip of the ogive that the radius will be evaluated at.
	*/
	double GetOgiveRadius(const double x = 0) const;

	double GetSlope(const double x, const bool IsTop = true) const;

	double GetTangentAngle(const double x, const bool IsTop = true) const;

	/**
	* Returns the cross sectional area of this shape as viewed along it's axis of revolution. (m^2)
	* @param bForceRecalculate	If true, this value will be recalculated.
	*/
	double GetFrontalCSA(const bool bForceRecalculate = false) const;

	double GetShapeFrontalCSA() const
	{
		return GetFrontalCSA();
	}

	double GetOgiveCd() const
	{
		return 0.59;
	}
	double GetShapeCd() const
	{
		return GetOgiveCd();
	}

	/**
	* Returns the coefficient of drag perpendicular to the axis of revolution.
	* @param bForceRecalculate	If true, this value will be recalculated.
	*/
	double GetCD(const bool bForceRecalculate = false) const;

	/**
	* Returns the volume of the ogive in cubic meters.
	* @param bForceRecalculate	If true, this value will be recalculated.
	*/
	double GetShapeVolume(const bool bForceRecalculate = false) const;

	double GetCenterOfMass() const;

	double GetCenterOfMassFull(const double Mass) const;

	FVector GetOgiveInertia(const double Mass) const;

	FVector GetFullInertia(const double Mass) const;

	double GetFullVolume() const
	{
		return GetShapeVolume() + Cylinder.GetVolume();
	}

	void RecalculateValues()
	{
		Cylinder.RecalculateValues();
		GetFrontalCSA(true);
		GetShapeVolume(true);

		// GetAlpha(true) is implicitly called by both GetSinAlpha(true) and GetCosAlpha(true)
		GetSinAlpha(true);
		GetCosAlpha(true);
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSucces)
	{
		Ar << *this;
		bOutSucces = true;
		return true;
	}

	friend FArchive& operator<<(FArchive& Ar, FTBOgive& Ogive)
	{
		Ar << Ogive.Cylinder;
		Ar << Ogive.Radius;
		Ar << Ogive.OgiveRadius;
		Ar << Ogive.CSA;
		Ar << Ogive.Length;
		Ar << Ogive.SurfaceArea;
		Ar << Ogive.FrontalCSA;
		Ar << Ogive.Volume;
		if(Ar.IsLoading())
		{
			Ogive.GetSinAlpha(true);
			Ogive.GetCosAlpha(true);
		}
		return Ar;
	}

	bool operator==(const FTBOgive& other) const
	{
		return Radius == other.Radius
			&& OgiveRadius == other.OgiveRadius
			&& CSA == other.CSA
			&& Length == other.Length
			&& Cylinder == other.Cylinder;
	}

	bool operator!=(const FTBOgive& other) const
	{
		return !(*this == other);
	}

	// Inherited via FTBShape
	double GetShapeLength() const
	{
		return Length;
	}
	double GetShapeRadius(const double x) const
	{
		return GetOgiveRadius(x);
	}
	double GetShapeRadius() const
	{
		return OgiveRadius;
	}
};
template<> struct TStructOpsTypeTraits<FTBOgive> : public TStructOpsTypeTraitsBase2<FTBOgive>
{
	enum
	{
		WithIdenticalViaEquality = true,
		WithNetSerializer = true,
		WithNetSharedSerialization = true
	};
};



using TNoseVariant = std::variant<FTBOgive, FTBEllipticalNose>;
struct FTBNose
{
	TNoseVariant ShapeVariant;

	double GetLength() const;
	double GetRadius() const;
	double GetRadiusAt(const double x) const;
	double GetSlopeAt(const double x) const;
	double GetAngleAt(const double x) const;
		
	double GetCSA() const;
	double GetFrontalCSA() const;
	// Returns the coefficient of drag perpendicular to its axis of rotation
	double GetPerpendicularCd() const;
	double GetSurfaceArea(const bool bForceRecalculate = false) const;
	double GetVolume(const bool bForceRecalculate = false) const;

	void RecalculateValues();

	double GetLengthMeters() const
	{
		return GetLength() / 100.0;
	}
	double GetRadiusMeters() const
	{
		return GetRadius() / 100.0;
	}
};


USTRUCT(BlueprintType)
struct FTBBulletShape
{
	GENERATED_BODY()

	FTBNose Nose;
	FTBCylinder Cylinder;

protected:
	mutable uint8 bIsOgive : 1;

public:
	bool IsOgive() const;

	double GetLength() const;

	double GetRadius() const;

	double GetRadiusAt(const double x) const;

	double GetVolume(const bool bForceRecalculate = false) const;

	double GetSurfaceArea() const;

	double GetRadiusMeters() const;

	double GetSlope(const double x, const bool IsTop = true) const;

	double GetTangentAngle(const double x, const bool IsTop = true) const;

	double GetCSA() const;

	/**
	* Returns the coefficient of drag perpendicular to the axis of revolution.
	* @param bForceRecalculate	If true, this value will be recalculated.
	*/
	double GetCD(const bool bForceRecalculate = false) const;

	/**
	* Returns the cross sectional area of this shape as viewed along it's axis of revolution. (m^2)
	*/
	double GetFrontalCSA() const;

	void RecalculateValues();
};
