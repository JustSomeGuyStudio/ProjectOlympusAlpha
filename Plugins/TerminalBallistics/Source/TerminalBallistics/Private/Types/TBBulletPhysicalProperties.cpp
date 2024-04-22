// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "Types/TBBulletPhysicalProperties.h"
#include "MathUtils.h"
#include "PhysicsUtils.h"
#include "TerminalBallisticsMacrosAndFunctions.h"



static constexpr double CoefficientOfDrag_Sphere = 0.47;

static constexpr double KgToGrains(const double ValueInKg)
{
	return 15432.3583529 * ValueInKg;
}

FTBBulletPhysicalProperties::FTBBulletPhysicalProperties(double InMass, double InDensity, double InRadius, double InExpansionCoefficient, double InDeformationResistance, double InPenetrationMultiplier)
	: FTBProjectilePhysicalProperties(2.0 * InRadius, InRadius, InMass, InDensity, UE_DOUBLE_PI * TB::MathUtils::sqr(InRadius / 100.0), FRotator::ZeroRotator, CoefficientOfDrag_Sphere)
	, DragModel(ETBGModel::NA)
	, bIsSpherical(true)
	, PenetrationMultiplier(InPenetrationMultiplier)
	, ExpansionCoefficient(InExpansionCoefficient)
	, DeformationResistance(InDeformationResistance)
{
	Ogive = FTBOgive(); // Not applicable
	CoefficientOfForm = 1.0; // Not applicable
	BallisticCoefficient = 1.0; // Not applicable

	CoefficientOfDrag = CoefficientOfDrag_Sphere;
	CoefficientOfDrag3D = FVector(CoefficientOfDrag_Sphere);
	CSA = FVector(CrossSectionalArea);

	using namespace TB::MathUtils;

	SectionalDensity = KgToGrains(Mass) / (UE_DOUBLE_PI * 7000.0 * sqr(2.0 * Radius));

	ApparentLength = 1.0 / 3.0 * InRadius;
	ApparentRadius = sqrt(sqr(InRadius) - (1.0 / 3.0 * InRadius - 1.0));
	ApparentImpactArea = UE_DOUBLE_PI * sqr(ApparentRadius);
	ApparentRigidIndenterAngle = 45.0;
}

double FTBBulletPhysicalProperties::GetDragCoefficient(bool bForceRecalculate) const
{
	if (CoefficientOfDrag == -1 || bForceRecalculate)
	{
		if (bIsSpherical)
		{
			CoefficientOfDrag = CoefficientOfDrag_Sphere;
		}
		else
		{
			const double GModelDragCoefficient = TB::Drag::GetDragCoefficient(DragModel, 0.0);
			CoefficientOfDrag = GModelDragCoefficient * GetCoefficientOfForm();
		}
	}
	return CoefficientOfDrag;
}

double FTBBulletPhysicalProperties::GetDragCoefficient(const double Speed) const
{
	if (bIsSpherical)
	{
		const double Re = TB::GetReynoldsNumber(TB::Constants::FluidDensity_Air, TB::Constants::DynamicViscosity_Air, 2.0 * GetRadiusMeters(), Speed);
		return TB::Drag::GetDragCoefficient(ETBGModel::NA, 0.0, Re);
	}
	else
	{
		const double GModelDragCoefficient = TB::Drag::GetDragCoefficient(DragModel, Speed);
		const double cd = GModelDragCoefficient * GetCoefficientOfForm();
		return GModelDragCoefficient * GetCoefficientOfForm();
	}
}

FVector FTBBulletPhysicalProperties::GetDragCoefficient3D(const FVector& Velocity) const
{
	if (bIsSpherical)
	{
		const FVector Re = TB::GetReynoldsNumber(TB::Constants::FluidDensity_Air, TB::Constants::DynamicViscosity_Air, FVector(2.0 * GetRadiusMeters()), Velocity);
		return FVector(
			TB::Drag::GetDragCoefficient(ETBGModel::NA, 0.0, Re.X),
			TB::Drag::GetDragCoefficient(ETBGModel::NA, 0.0, Re.Y),
			TB::Drag::GetDragCoefficient(ETBGModel::NA, 0.0, Re.Z)
		);
	}
	else
	{
		const double OgiveCd = Ogive.GetCD();
		return RotateByOrientation(FVector(GetDragCoefficient(abs(Velocity.X)), OgiveCd, OgiveCd));
	}
}

FVector FTBBulletPhysicalProperties::GetDragCoefficient3D(const bool bForceRecalculate) const
{
	if (CoefficientOfDrag3D.GetMin() <= 0.0 || bForceRecalculate)
	{
		if (bIsSpherical)
		{
			CoefficientOfDrag3D = FVector(CoefficientOfDrag_Sphere);
		}
		else
		{
			const double OgiveCD = Ogive.GetCD();
			CoefficientOfDrag3D = RotateByOrientation(FVector(GetDragCoefficient(), OgiveCD, OgiveCD));
		}
	}
	return CoefficientOfDrag3D;
}

FArchive& FTBBulletPhysicalProperties::Serialize_Impl(FArchive& Ar, FTBBulletPhysicalProperties& BulletPhysicalProperties)
{
	Ar = BulletPhysicalProperties.SerializeProjectile(Ar);
	Ar << BulletPhysicalProperties.bIsSpherical;
	Ar << BulletPhysicalProperties.DragModel;
	Ar << BulletPhysicalProperties.PenetrationMultiplier;
	Ar << BulletPhysicalProperties.ApparentRadius;
	Ar << BulletPhysicalProperties.ApparentLength;
	Ar << BulletPhysicalProperties.ApparentImpactArea;
	Ar << BulletPhysicalProperties.ApparentRigidIndenterAngle;
	Ar << BulletPhysicalProperties.BallisticCoefficient;
	Ar << BulletPhysicalProperties.SectionalDensity;
	TB::SerializeValueConditional(Ar.IsSaving(), Ar, BulletPhysicalProperties.Ogive, FTBOgive(), !BulletPhysicalProperties.bIsSpherical);
	Ar << BulletPhysicalProperties.ExpansionCoefficient;
	Ar << BulletPhysicalProperties.DeformationResistance;

	if (Ar.IsLoading())
	{
		BulletPhysicalProperties.GetCoefficientOfForm(true);
	}

	return Ar;
}

bool FTBBulletPhysicalProperties::CheckValid() const noexcept
{
	if (!CheckNonZero() || !FTBProjectilePhysicalProperties::CheckValid())
	{
		return false;
	}

	return Length > ApparentLength
		&& ApparentRigidIndenterAngle <= 90.0;
}
