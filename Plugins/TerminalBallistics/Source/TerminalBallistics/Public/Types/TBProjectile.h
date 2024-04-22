// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MathUtils.h"
#include "Templates/AndOrNot.h"
#include "Templates/UnrealTypeTraits.h"
#include "TerminalBallisticsTraits.h"
#include "Types/TBEnums.h"
#include "TBProjectile.generated.h"



USTRUCT(BlueprintInternalUseOnly)
struct FTBProjectilePhysicalProperties
{
	GENERATED_BODY()
public:
	template<typename T>
	using ImpactStruct = T;
	template<typename T>
	using ProjectileStruct = T;

	// Mass of Projectile(kg)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (Units = "kg", ShortToolTip = "Mass(kg)"))
	double Mass = 1.0;
	// Length of Projectile(cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (Units = "cm", ShortToolTip = "Length(cm)"))
	double Length = 1.0;
	// Density of Projectile(g/cm^3)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (ShortToolTip = "Density(g/cm^3)"))
	double Density = 1.0;
	// Radius of Projectile(cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, meta = (Units = "cm", ShortToolTip = "Radius(cm)"))
	double Radius = 0.25;
	// Cross sectional area of the Projectile (m^2)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, AdvancedDisplay, meta = (ToolTip = "Cross sectional area of the projectile (m^2)\nWill be calculated automatically if 0 or less.", ShortToolTip = "Cross sectional area(m^2)"))
	double CrossSectionalArea = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile, AdvancedDisplay)
	mutable FQuat Orientation = FQuat::Identity;

protected:
	typedef FTBProjectilePhysicalProperties Super;
	// Cache calculated values. Also cache vector versions of values so they don't need to be serialized.

	mutable FVector CoefficientOfDrag3D = FVector::ZeroVector;
	mutable FVector CSA = FVector::ZeroVector;
	mutable double CoefficientOfDrag = -1;
	mutable double FrontalCSA = -1;
	
public:
	virtual ~FTBProjectilePhysicalProperties() = default;

	FTBProjectilePhysicalProperties()
	{
		if (CrossSectionalArea <= 0) {
			CrossSectionalArea = TB_SQR(DOUBLE_PI * Radius / 100.0) + ((Length / 100.0 - 2 * Radius / 100.0) * Radius / 100.0);
		}
		GetDragCoefficient3D(true);
		CSA3D(true);
	}

	FTBProjectilePhysicalProperties(const double InLength, const double InRadius, const double InMass, const double InDensity, const double InCrossSectionalArea = 0.0, const FRotator& Orientation = FRotator::ZeroRotator, const double InDragCoefficient = 0.38)
		: Mass(InMass)
		, Length(fmax(InLength, 2 * InRadius))
		, Density(InDensity)
		, Radius(InRadius)
		, Orientation(Orientation)
		, CoefficientOfDrag(InDragCoefficient)
	{
		if (CrossSectionalArea <= 0) {
			CrossSectionalArea = TB_SQR(DOUBLE_PI * Radius / 100.0) + ((Length / 100.0 - 2 * Radius / 100.0) * Radius / 100.0);
		}
		GetDragCoefficient3D(true);
		CSA3D(true);
	}

	virtual double GetSurfaceArea() const { return 0.0; }

	inline virtual FVector GetDragCoefficient3D(const bool bForceRecalculate = false) const
	{
		if (bForceRecalculate || CoefficientOfDrag3D == FVector::ZeroVector)
		{
			CoefficientOfDrag3D = RotateByOrientation(FVector(GetDragCoefficient(), 0.59, 0.59)).GetAbs();
		}
		return CoefficientOfDrag3D;
	}
	inline virtual FVector GetDragCoefficient3D(const FVector& Velocity) const
	{
		return GetDragCoefficient3D();
	}

	inline virtual double GetRadiusMeters() const
	{
		return Radius / 100.0;
	}

	// Returns the cross sectional area (m^2) of the projectile viewed from the front.
	inline virtual double GetFrontalCSA() const
	{
		if (FrontalCSA <= 0.0)
		{
			FrontalCSA = TB_SQR((Radius / 100.0) * DOUBLE_PI);
		}
		return FrontalCSA;
	}

	// Returns the cross sectional area (m^2) of the projectile along each axis.
	inline virtual FVector CSA3D(const bool bForceRecalculate = false) const
	{
		if (bForceRecalculate || CSA == FVector::ZeroVector)
		{
			CSA = RotateByOrientation(FVector(GetFrontalCSA(), CrossSectionalArea, CrossSectionalArea)).GetAbs();
		}
		return CSA;
	}

	inline virtual void SetDragCoefficient(const double NewDragCoefficient)
	{
		if (NewDragCoefficient > 0) {
			CoefficientOfDrag = NewDragCoefficient;
			GetDragCoefficient3D(true);
		}
	}

	inline virtual double GetDragCoefficient(bool bForceRecalculate = false) const
	{
		if (bForceRecalculate || CoefficientOfDrag <= 0.0) {
			return CoefficientOfDrag = 0.38;
		}
		else {
			return CoefficientOfDrag;
		}
	}
	inline virtual double GetDragCoefficient(const double Speed) const
	{
		return GetDragCoefficient();
	}

	inline virtual bool CheckValid() const noexcept
	{
		return Length > 0.0
			&& Radius > 0.0
			&& Length >= 2 * Radius
			&& Mass > 0.0
			&& Density > 0.0
			&& CoefficientOfDrag > 0.0
			&& CrossSectionalArea > 0.0
			&& FrontalCSA > 0.0;
	}

	inline operator bool()
	{
		return CheckValid();
	}

	FORCEINLINE virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << *this;
		bOutSuccess = true;
		return true;
	}

	FORCEINLINE friend FArchive& operator<<(FArchive& Ar, FTBProjectilePhysicalProperties& Projectile)
	{
		return Projectile.SerializeProjectile(Ar);
	}

protected:
	FORCEINLINE FArchive& SerializeProjectile(FArchive& Ar)
	{
		Ar << Length;
		Ar << Radius;
		Ar << Mass;
		Ar << Density;
		Ar << CrossSectionalArea;
		Ar << Orientation;
		Ar << FrontalCSA;
		Ar << CoefficientOfDrag;
		SerializeOptionalValue(Ar.IsSaving(), Ar, CSA, FVector::ZeroVector);
		if (Ar.IsLoading())
		{
			GetDragCoefficient3D(true);
		}
		return Ar;
	}

	inline FVector RotateByOrientation(const FVector& VectorToRotate) const
	{
		return Orientation.RotateVector(VectorToRotate);
	}

public:
	bool operator==(const FTBProjectilePhysicalProperties& other) const
	{
		return FMath::IsNearlyEqual(Mass, other.Mass)
			&& FMath::IsNearlyEqual(Length, other.Length)
			&& FMath::IsNearlyEqual(Density, other.Density)
			&& FMath::IsNearlyEqual(Radius, other.Radius)
			&& FMath::IsNearlyEqual(CrossSectionalArea, other.CrossSectionalArea)
			&& Orientation.Equals(other.Orientation)
			&& CoefficientOfDrag3D.Equals(other.CoefficientOfDrag3D)
			&& CSA.Equals(other.CSA)
			&& FMath::IsNearlyEqual(CoefficientOfDrag, other.CoefficientOfDrag)
			&& FMath::IsNearlyEqual(FrontalCSA, other.FrontalCSA);
	}

	bool operator!=(const FTBProjectilePhysicalProperties& other) const
	{
		return !(*this == other);
	}
};
template<> struct TStructOpsTypeTraits<FTBProjectilePhysicalProperties> : public TStructOpsTypeTraitsBase2<FTBProjectilePhysicalProperties>
{
	enum
	{
		WithIdenticalViaEquality = true,
		WithNetSerializer = true,
		WithNetSharedSerialization = true
	};
};


// Model of a basic Projectile in the shape of a capsule
USTRUCT(BlueprintType, meta = (DisplayName = "Projectile"))
struct FTBProjectile : public FTBProjectilePhysicalProperties
{
	GENERATED_BODY()
public:
	using ImpactStruct = typename FTBProjectilePhysicalProperties::ImpactStruct<struct FTBImpactParamsBasic>;
	using ProjectileStruct = typename FTBProjectilePhysicalProperties::ImpactStruct<FTBProjectile>;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Projectile)
	ETBProjectileSize ProjectileSize = ETBProjectileSize::Small;

	virtual ~FTBProjectile() = default;

	FTBProjectile()
		: FTBProjectilePhysicalProperties()
	{}

	FTBProjectile(double InLength, double InRadius, double InMass, double InDensity, ETBProjectileSize InProjectileSize)
	{
		Length = fmax(InLength, 2 * InRadius);
		Radius = InRadius;
		Mass = InMass;
		Density = InDensity;
		ProjectileSize = InProjectileSize;
		CrossSectionalArea = TB_SQR(DOUBLE_PI * Radius) + ((Length - 2 * Radius) * Radius);
	}

	virtual double GetSurfaceArea() const
	{
		const double SurfaceArea_Cylinder = 2.0 * DOUBLE_PI * Radius * Length;
		const double SurfaceArea_Caps = 4.0 * DOUBLE_PI * TB_SQR(Radius);
		return (SurfaceArea_Cylinder + SurfaceArea_Caps) / 10000.0;
	}

	FORCEINLINE virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << *this;
		bOutSuccess = true;
		return true;
	}

	FORCEINLINE friend FArchive& operator<<(FArchive& Ar, FTBProjectile& Projectile)
	{
		Ar = Projectile.SerializeProjectile(Ar);
		Ar << Projectile.ProjectileSize;
		return Ar;
	}

public:
	FORCEINLINE bool operator==(const FTBProjectile& other) const
	{
		return Super::operator==(other)
			&& ProjectileSize == other.ProjectileSize;
	}

	FORCEINLINE bool operator!=(const FTBProjectile& other) const
	{
		return !(*this == other);
	}
};
template<> struct TStructOpsTypeTraits<FTBProjectile> : public TStructOpsTypeTraitsBase2<FTBProjectile>
{
	enum
	{
		WithIdenticalViaEquality = true,
		WithNetSerializer = true,
		WithNetSharedSerialization = true
	};
};

namespace TB::Traits
{
	template<typename ImpactStruct, typename TypeOfSimData, typename ProjectilePropertyStruct>
	struct TIsValidSimTask
	{
		enum
		{
			Value =
			TAnd<
				TIsImpactStruct<ImpactStruct>,
				TIsSimData<TypeOfSimData>,
				TIsDerivedFrom<ProjectilePropertyStruct, FTBProjectilePhysicalProperties>
			>::Value
		};
	};
};
