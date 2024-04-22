// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MathUtils.h"
#include "TerminalBallisticsTraits.h"
#include "Types/TBEnums.h"
#include "Types/TBProjectile.h"
#include "Types/TBShapes.h"
#include "TBBulletPhysicalProperties.generated.h"



//TODO: Overhaul the "apparent length" charactaristic system to instead use the ogive
/**
* Describes the various physical characteristics of a bullet.
*/
USTRUCT(BlueprintType)
struct TERMINALBALLISTICS_API FTBBulletPhysicalProperties : public FTBProjectilePhysicalProperties
{
	GENERATED_BODY()
	TB_WITH_OPTIMIZED_SERIALIZER();
public:
	using ImpactStruct = typename FTBProjectilePhysicalProperties::ImpactStruct<struct FTBImpactParams>;
	using ProjectileStruct = typename FTBProjectilePhysicalProperties::ImpactStruct<struct FTBBullet>;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Ballistics, meta = (DisplayAfter = "BallisticCoefficient"))
	ETBGModel DragModel = ETBGModel::G7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ballistics, meta = (ToolTip = "Should this be treated as a simple sphere? Useful for buckshot."))
	bool bIsSpherical = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ballistics)
	double PenetrationMultiplier = 1.0;

	// Radius (cm) of the cross-sectional area as displayed 1/3 of the distance from the Projectile tip to the point at which the Projectile is at it's full radius
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Bullet, meta = (Units = "cm", ShortToolTip = "Apparent Radius (cm)"))
	double ApparentRadius;
	// 1/3 of the distance from the tip of Projectile to the point at which the Projectile is at it's full radius
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Bullet, meta = (Units = "cm", ShortToolTip = "Apparent Length (cm)"))
	double ApparentLength;
	// Area (cm^2) of the circle described by the apparent radius
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Bullet, meta = (ShortToolTip = "Apparent Impact Area (cm^2)"))
	double ApparentImpactArea;
	// Angle of the line tangent to the Projectile 1/2 of the distance from the tip to the apparent radius
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = Bullet, meta = (ShortToolTip = "Apparent Rigid Indentor Angle"))
	double ApparentRigidIndenterAngle;
	// Ballistic Coefficient
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ballistics, AdvancedDisplay, meta = (ShortToolTip = "Ballistic Coefficient"))
	double BallisticCoefficient;
	// Sectional Density
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ballistics, AdvancedDisplay, meta = (ShortToolTip = "Sectional Density"))
	double SectionalDensity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ballistics, AdvancedDisplay)
	FTBOgive Ogive;
	// The ratio of the radius of the bullet when fully expanded to the radius of the bullet.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ballistics, AdvancedDisplay)
	double ExpansionCoefficient = 1.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ballistics)
	double DeformationResistance = 2.0;

protected:
	mutable double CoefficientOfForm = -1;
	
public:
	virtual ~FTBBulletPhysicalProperties() = default;

	FTBBulletPhysicalProperties()
		: FTBProjectilePhysicalProperties()
		, ApparentRadius(0.0)
		, ApparentLength(0.0)
		, ApparentImpactArea(0.0)
		, ApparentRigidIndenterAngle(0.0)
		, BallisticCoefficient(0.0)
		, SectionalDensity(0.0)
	{
		GetDragCoefficient(true);
		GetDragCoefficient3D(true);
	}

	// Constructor for a spherical projectile
	FTBBulletPhysicalProperties(double InMass, double InDensity, double InRadius, double InExpansionCoefficient = 1.0, double InDeformationResistance = 2.0, double InPenetrationMultiplier = 1.0);

	FTBBulletPhysicalProperties(double InMass, double InLength, double InDensity, double InRadius, double InCrossSectionalArea, double InApparentRadius, double InApparentLength, double InApparentImpactArea, double InApparentRigidIndentorAngle, double InBallisticCoefficient, double InSectionalDensity, ETBGModel DragModel = ETBGModel::G7, FTBOgive InOgive = FTBOgive(), double InExpansionCoefficient = 1.0, double InDeformationResistance = 2.0, double InPenetrationMultiplier = 1.0)
		: FTBProjectilePhysicalProperties(InLength, InRadius, InMass, InDensity, InCrossSectionalArea)
		, DragModel(DragModel)
		, PenetrationMultiplier(InPenetrationMultiplier)
		, ApparentRadius(InApparentRadius)
		, ApparentLength(InApparentLength)
		, ApparentImpactArea(InApparentImpactArea)
		, ApparentRigidIndenterAngle(InApparentRigidIndentorAngle)
		, BallisticCoefficient(InBallisticCoefficient)
		, SectionalDensity(InSectionalDensity)
		, Ogive(InOgive)
		, ExpansionCoefficient(InExpansionCoefficient)
		, DeformationResistance(InDeformationResistance)
	{
		GetDragCoefficient(true);
		GetDragCoefficient3D(true);
		Ogive.GetSurfaceArea(true);
	}

	// Returns the surface area of the projectile, excluding the base. (m^2)
	virtual double GetSurfaceArea() const
	{
		if (bIsSpherical)
		{
			return 4.0 * UE_DOUBLE_PI * pow(Radius / 100.0, 2.0);
		}
		else
		{
			return Ogive.GetSurfaceArea();
		}
	}

	virtual double GetRadiusMeters() const
	{
		if (bIsSpherical)
		{
			return Radius / 100.0;
		}
		else
		{
			return fmax(Ogive.GetRadiusMeters(), ApparentRadius / 100.0);
		}
	}

	double GetVolume() const
	{
		if (bIsSpherical)
		{
			return (4.0 / 3.0) * UE_DOUBLE_PI * pow(GetRadiusMeters(), 3.0);
		}
		else
		{
			const double OgiveVolume = Ogive.GetShapeVolume();
			const double BodyVolume = GetFrontalCSA() * (Length / 100.0 - Ogive.Length / 100.0);
			return OgiveVolume + BodyVolume;
		}
	}

	inline bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override
	{
		Ar << *this;
		bOutSuccess = true;
		return true;
	}

	inline friend FArchive& operator<<(FArchive& Ar, FTBBulletPhysicalProperties& BulletPhysicalProperties)
	{
		return FTBBulletPhysicalProperties::Serialize_Impl(Ar, BulletPhysicalProperties);
	}

	virtual bool CheckValid() const noexcept override;

	inline bool CheckNonZero() const noexcept
	{
		return Mass > 0.0
			&& Length > 0.0
			&& Density > 0.0
			&& Radius > 0.0
			&& CrossSectionalArea > 0.0
			&& ApparentRadius > 0.0
			&& ApparentLength > 0.0
			&& ApparentImpactArea > 0.0
			&& (bIsSpherical ||  (BallisticCoefficient > 0.0 && SectionalDensity > 0.0));
	}

	virtual FVector GetDragCoefficient3D(const bool bForceRecalculate = false) const override;

	inline virtual double GetFrontalCSA() const override
	{
		if (FrontalCSA <= 0.0)
		{
			const double radius = fmax(Radius, ApparentRadius) / 100.0;
			FrontalCSA = DOUBLE_PI * TB_SQR(radius);
		}
		return FrontalCSA;
	}

	inline double GetCoefficientOfForm(const bool bForceRecalculate = false) const
	{
		if (CoefficientOfForm == -1 || bForceRecalculate)
		{
			return CoefficientOfForm = SectionalDensity / BallisticCoefficient;
		}
		else
		{
			return CoefficientOfForm;
		}
	}

	virtual double GetDragCoefficient(bool bForceRecalculate = false) const override;

	virtual double GetDragCoefficient(const double Speed) const override;

	virtual FVector GetDragCoefficient3D(const FVector& Velocity) const override;

	bool operator==(const FTBBulletPhysicalProperties& other) const
	{
		return DragModel == other.DragModel
			&& PenetrationMultiplier == other.PenetrationMultiplier
			&& ApparentRadius == other.ApparentRadius
			&& ApparentLength == other.ApparentLength
			&& ApparentImpactArea == other.ApparentImpactArea
			&& ApparentRigidIndenterAngle == other.ApparentRigidIndenterAngle
			&& BallisticCoefficient == other.BallisticCoefficient
			&& SectionalDensity == other.SectionalDensity
			&& Ogive == other.Ogive
			&& ExpansionCoefficient == other.ExpansionCoefficient
			&& DeformationResistance == other.DeformationResistance
			&& CoefficientOfForm == other.CoefficientOfForm;
	}

	bool operator!=(const FTBBulletPhysicalProperties& other) const
	{
		return !(*this == other);
	}

protected:
	static FArchive& Serialize_Impl(FArchive& Ar, FTBBulletPhysicalProperties& BulletPhysicalProperties);
};
template<> struct TStructOpsTypeTraits<FTBBulletPhysicalProperties> : public TStructOpsTypeTraitsBase2<FTBBulletPhysicalProperties>
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true
	};
};
