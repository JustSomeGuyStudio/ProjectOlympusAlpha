// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "BallisticFunctions.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Kismet/DataTableFunctionLibrary.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetNodeHelperLibrary.h"
#include "MathUtils.h"
#include "PhysicsUtils.h"
#include "PhysMatManager/PhysMat.h"
#include "PhysMatManager/PhysMatManager.h"
#include "Threading/GraphTasks/GraphTasks.h"

#include "TerminalBallisticsLogChannels.h"


double UBallisticFunctions::NewtonianDepthApproximation(double ProjectileLength, double ProjectileDensity, double TargetDensity)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UBallisticFunctions::NewtonianDepthApproximation);

	if (FMath::IsNearlyZero(ProjectileLength * ProjectileDensity * TargetDensity))
	{
		return 0;
	}
	else
	{
		UE_ASSUME(ProjectileDensity != 0.0 && TargetDensity != 0.0);
		return ProjectileLength * (ProjectileDensity / TargetDensity);
	}
}

double UBallisticFunctions::ImpactApproximation(const FTBProjectilePhysicalProperties& Projectile, const FVector& Velocity, const FPhysMatProperties& ProjectileProperties, const FPhysMatProperties& ObjectProperties, double& ContactRadius, double& P0)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UBallisticFunctions::ImpactApproximation);

	using namespace TB::MathUtils;

	double vel = Velocity.Size() / 100.0; // Convert velocity to m/s
	double dynP = UTerminalBallisticsStatics::CalculateDynamicPressure(Projectile.Density * 1000.0, vel);
	double R = Projectile.Radius / 100.0;
	double F = dynP * (DOUBLE_PI * sqr(R));
	double E0 = ProjectileProperties.YoungsModulus * 1e9;
	double v0 = ProjectileProperties.PoissonsRatio;
	double E1 = ObjectProperties.YoungsModulus * 1e9;
	double v1 = ObjectProperties.PoissonsRatio;
	UE_ASSUME(v0 > 0.0 && v1 > 0.0);

	double E = (E0 * E1) / (-E0 * sqr(v1) + E0 - E1 * sqr(v0) + E1);
	double d = pow((9.0 * sqr(F)) / (16 * sqr(E) * R), 1.0 / 3.0);
	double a = sqrt(R * d);
	P0 = 1.0 / DOUBLE_PI * sqrt((6 * F * sqr(E)) / sqr(R)); // 1/pi * (6FE^2/R^2)^0.5

	ContactRadius = a * 100.0;
	return d * 100.0; // Convert depth(d) to cm (unreal units)
}

namespace TB::BallisticFunctions
{
	const FString GetMaterialFailureModeString(const EMaterialFailureMode FailureMode)
	{
		switch (FailureMode)
		{
		case EMaterialFailureMode::NoFailure:
			return FString("None");
		case EMaterialFailureMode::Yield:
			return FString("Yield");
		case EMaterialFailureMode::UTS:
			return FString("UTS");
		case EMaterialFailureMode::Compressive:
			return FString("Compressive");
		case EMaterialFailureMode::Shear:
			return FString("Shear");
		case EMaterialFailureMode::Impact:
			return FString("Impact");
		case EMaterialFailureMode::Fracture:
			return FString("Fracture");
		default:
			return FString("None");
		}
	}

	FMaterialFailure FMaterialFailure::NoMaterialFailure = FMaterialFailure();

	double FMaterialFailure::CalculatePenetrationEnergy(const double KineticEnergy, const double ImpactAngle, const double ContactArea, const double ProjectileNoseLength, const double ObjectThickness, const double ProjectileCSA, const FPhysMatProperties& PhysMatInBaseUnits, const FPhysMatProperties& ProjectilePhysMatInBaseUnits, double& MaxStress, double& AverageStress)
	{
		const double StressConcentrationFactor = abs(1 + 2 * (1 - cos(ImpactAngle) / 2)); // Peterson's formula
		const double FullStress = CalculateImpactStress(KineticEnergy, ProjectileCSA, ImpactAngle, ObjectThickness, PhysMatInBaseUnits.YoungsModulus, ProjectilePhysMatInBaseUnits.YoungsModulus, PhysMatInBaseUnits.FractureToughness);
		if (ProjectileNoseLength < ObjectThickness)
		{
			// TODO: integrate tip stress (coarse numerical integration should be fine)
			double RemainingDistance = ObjectThickness - ProjectileNoseLength;
			RemainingDistance = RemainingDistance < 0 ? 0 : RemainingDistance;
			const double Stress = CalculateImpactStress(KineticEnergy, ContactArea, ImpactAngle, ObjectThickness, PhysMatInBaseUnits.YoungsModulus, ProjectilePhysMatInBaseUnits.YoungsModulus, PhysMatInBaseUnits.FractureToughness);
			const double NoseLength = fmin(ProjectileNoseLength, ObjectThickness);
			const double NoseWeight = NoseLength / ObjectThickness;
			const double RemainingWeight = RemainingDistance / ObjectThickness;

			double EnergyFull = Stress * StressConcentrationFactor * ProjectileNoseLength * NoseWeight; // Assume higher stress while the bullet nose is penetrating
			EnergyFull += FullStress * RemainingDistance * RemainingWeight;

			AverageStress = abs((Stress * NoseWeight) + (FullStress * RemainingWeight));
			MaxStress = abs(fmax(FullStress, Stress));
			return abs(EnergyFull);
		}
		else
		{
			AverageStress = abs(FullStress);
			MaxStress = abs(FullStress);
			const double EnergyFull = FullStress * StressConcentrationFactor * ObjectThickness;
			return abs(EnergyFull);
		}
	}

	FMaterialFailure TB::BallisticFunctions::FMaterialFailure::CheckForModeIFailures(const FVector& ImpactVelocity, const FVector& SurfaceNormal, const double ImpactAngle, const double ContactArea, const double ProjectileMass, const double ProjectileRadius, const double ProjectileNoseLength, const double ProjectileCSA, const double ObjectThickness, const FPhysMatProperties& PhysMatInBaseUnits, const FPhysMatProperties& ProjectilePhysMatInBaseUnits)
	{
		using namespace TB::MathUtils;

		const double ImpactSpeed = ImpactVelocity | SurfaceNormal; // Only considering the perpendicular component

		const double RemainingDistance = ObjectThickness - ProjectileNoseLength;

		const double StressConcentrationFactor = 1 + 2 * (1 - cos(ImpactAngle) / 2); // Peterson's formula

		const double KineticEnergy = CalculateKineticEnergy(ProjectileMass, ImpactSpeed);

		const double Stress = CalculateImpactStress(KineticEnergy, ContactArea, ImpactAngle, ObjectThickness, PhysMatInBaseUnits.YoungsModulus, ProjectilePhysMatInBaseUnits.YoungsModulus, PhysMatInBaseUnits.FractureToughness);
		const double Strain = Stress / PhysMatInBaseUnits.YoungsModulus;
		const double Volume = ContactArea * ProjectileRadius;
		const double Volume_Full = ContactArea * ObjectThickness;
		const double ElasticDeformationEnergy = 0.5 * Stress * Strain * Volume;
		const double ElasticDeformationEnergy_Full = 0.5 * Stress * Strain * Volume_Full;

		const double StressIntensityFactorAtCrackTip = Stress * sqrt(DOUBLE_PI * ObjectThickness);

		const double FractureEnergy = sqr(StressIntensityFactorAtCrackTip) / (PhysMatInBaseUnits.YoungsModulus * DOUBLE_PI * ContactArea);

		double EnergyFull = 0.0;
		const double FullStress = CalculateImpactStress(KineticEnergy, ProjectileCSA, ImpactAngle, ObjectThickness, PhysMatInBaseUnits.YoungsModulus, ProjectilePhysMatInBaseUnits.YoungsModulus, PhysMatInBaseUnits.FractureToughness);
		if (ProjectileNoseLength < ObjectThickness)
		{
			const double NoseLength = fmin(ProjectileNoseLength, ObjectThickness);
			const double NoseWeight = NoseLength / ObjectThickness;
			const double RemainingWeight = RemainingDistance / ObjectThickness;

			EnergyFull = (Stress * StressConcentrationFactor * ProjectileNoseLength) * NoseWeight; // Assume higher stress while the bullet nose is penetrating
			EnergyFull += (FullStress * RemainingDistance) * RemainingWeight;
		}
		else
		{
			EnergyFull = FullStress * StressConcentrationFactor * ObjectThickness;
		}

		const double dE_UTS = EnergyFull;
		const double dE_Yield = ElasticDeformationEnergy_Full;
		const double dE_Compression = (PhysMatInBaseUnits.CompressiveStrength * ContactArea) * ObjectThickness;
		const double dE_Fracture = FractureEnergy; //+ (sqr(StressIntensityFactorAtCrackTip) / (PhysMatInBaseUnits.YoungsModulus * ProjectileCSA));
		const bool Failure_UTS = PhysMatInBaseUnits.UltimateTensileStrength > 0 ? Stress * StressConcentrationFactor > PhysMatInBaseUnits.UltimateTensileStrength: 0; // Bit of an assumption, since this implies the force is causing tensile stress. (this may be the case for bending loads, however)
		const bool Failure_Yield = PhysMatInBaseUnits.GetYieldStrength() > 0 ? Stress * StressConcentrationFactor > PhysMatInBaseUnits.GetYieldStrength() : false;
		const bool Failure_Compression = PhysMatInBaseUnits.CompressiveStrength > 0 ? Stress * StressConcentrationFactor > PhysMatInBaseUnits.CompressiveStrength : false;

		const bool Failure_InitialFracture = (Stress * StressIntensityFactorAtCrackTip) / sqrt(ProjectileNoseLength) > PhysMatInBaseUnits.FractureToughness; // Can we initiate the fracture?
		const bool Failure_RemainingFracture = FullStress / sqrt(RemainingDistance) > PhysMatInBaseUnits.FractureToughness; // Can we propagate the fracture through the rest of the thickness

		FMaterialFailure UTSFailure(Failure_UTS ? UTS : NoFailure, dE_UTS);
		FMaterialFailure YieldFailure(Failure_Yield ? Yield : NoFailure, dE_Yield);
		FMaterialFailure CompressiveFailure(Failure_Compression ? Compressive : NoFailure, dE_Compression);
		FMaterialFailure FractureFailure(Failure_InitialFracture && Failure_RemainingFracture ? Fracture : NoFailure, dE_Fracture);

		if (Failure_UTS || Failure_Yield || Failure_Compression || FractureFailure)
		{
			FMaterialFailure Min1 = FMaterialFailure::Min(UTSFailure, YieldFailure);
			FMaterialFailure Min2 = FMaterialFailure::Min(CompressiveFailure, FractureFailure);
			return FMaterialFailure::Min(Min1, Min2);
		}
		else
		{
			return FMaterialFailure::NoMaterialFailure;
		}
	}

	FMaterialFailure FMaterialFailure::CheckForModeIIFailures(const double ImpactSpeed, const double ImpactAngle, const double ContactArea, const double ProjectileMass, const double ShearStrength, const double ObjectThickness)
	{
		using namespace TB::MathUtils;

		const double ShearStress = CalculateImpactStress(ProjectileMass, ImpactSpeed, ContactArea, ImpactAngle, ObjectThickness);

		const double StressConcentrationFactor = 1 + 2 * (1 - cos(ImpactAngle) / 2); // Peterson's formula

		const double CriticalShearStress = ShearStrength / ObjectThickness * StressConcentrationFactor;

		if (ShearStress >= CriticalShearStress)
		{
			const double FailureEnergy = abs((ShearStress - CriticalShearStress) * ContactArea);
			return FMaterialFailure(Shear, FailureEnergy);
		}

		return FMaterialFailure::NoMaterialFailure;
	}

	template<typename T, TEMPLATE_REQUIRES(TIsDerivedFrom<T, FTBBullet>::Value || TIsDerivedFrom<T, FTBProjectilePhysicalProperties>::Value)>
	bool TShouldRicochet(const FHitResult& HitResult, const T& Projectile, const FVector& ImpactVelocity, const FPhysMatProperties& ObjectProperties, const FPhysMatProperties& ProjectilePhysProperties, const double ObjectThickness, const double ImpactArea, double& AngleOfImpact, double& ImpartedEnergy, FVector& NewVelocity, const bool CheckForFailure, bool bDrawDebugTrace = false, bool bPrintDebugInfo = false)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TB::BallisticFunctions::ShouldRicochet);

		TB_CHECK_RICOCHET_BULLET_RET(false);

		using namespace TB::UnitConversions;
		using namespace TB::MathUtils;

		bDrawDebugTrace |= CVarBallisticsDebugDraw.GetValueOnAnyThread();
		bPrintDebugInfo |= CVarPrintBallisticsDebugInfo.GetValueOnAnyThread();

		UWorld* World = nullptr;
		if (TB_VALID_OBJECT(HitResult.GetActor()))
		{
			World = HitResult.GetActor()->GetWorld();
		}
		else if (TB_VALID_OBJECT(HitResult.GetComponent()))
		{
			World = HitResult.GetComponent()->GetWorld();
		}
		TB_RET_COND_VALUE(!World, false);
		const FVector SurfaceNormal = HitResult.Normal;

		double AngleOfImpactRadians = abs(NormalizeAngleToPlusMinus90DegreeRangeRadians(VectorUtils::GetImpactAngle(SurfaceNormal, ImpactVelocity)));
		AngleOfImpact = abs(FMath::RadiansToDegrees(AngleOfImpactRadians));


		bool bUseProbability = false;
		double Probability = 1.0;
		double OverrideAngle = 180.0;
		if (ObjectProperties.bUseCustomRicochetProperties)
		{
			OverrideAngle = ObjectProperties.RicochetProperties.RicochetAngleCutoff;
			if (!ObjectProperties.CanEverHaveRicochet())
			{
				return false;
			}
			else if (ObjectProperties.RicochetProperties.bProbabilityOverridesMaterialCalculations) // Bypass further calculation and just return true.
			{
				bUseProbability = true;
				if (ObjectProperties.RicochetProperties.RicochetProbability < FMath::FRand())
				{
					return false;
				}
			}
			else if (ObjectProperties.RicochetProperties.bAddRicochetRandomness)
			{
				bUseProbability = true;
				Probability *= ObjectProperties.RicochetProperties.RicochetProbability;
			}
		}
		if (ProjectilePhysProperties.bUseCustomRicochetProperties)
		{
			OverrideAngle = fmin(OverrideAngle, ProjectilePhysProperties.RicochetProperties.RicochetAngleCutoff);
			if (!ProjectilePhysProperties.CanEverHaveRicochet())
			{
				return false;
			}
			else if (ProjectilePhysProperties.RicochetProperties.bProbabilityOverridesMaterialCalculations)
			{
				if (ProjectilePhysProperties.RicochetProperties.RicochetProbability < FMath::FRand())
				{
					return false;
				}
			}
			else if (ProjectilePhysProperties.RicochetProperties.bAddRicochetRandomness)
			{
				bUseProbability = true;
				Probability *= ProjectilePhysProperties.RicochetProperties.RicochetProbability;
			}
		}

		const FTBProjectilePhysicalProperties& ProjectileProperties = Projectile;

		if constexpr (TIsDerivedFrom<T, FTBBullet>::Value)
		{
			if (Projectile.BulletVariation.Contains(ETBBulletVariation::FRANGIBLE))
			{
				return false;
			}
		}

		const double KineticEnergy = TB::CalculateKineticEnergy(ProjectileProperties.Mass, ImpactVelocity.Size());

		const double DeformationFactor = MapRangeClamped(0.0, 90.0, 0.4, 3.0, AngleOfImpact);
		double DeformationLength = 7e-6;
		DeformationLength *= MapRangeClamped(0.0, 1000.0, 0.1, 1.0, ImpactVelocity.Size());
		double DeformationEnergy = CalculateElasticDeformationEnergy(ProjectileProperties.Length / 100.0 - DeformationLength, ProjectileProperties.Length / 100.0, ProjectilePhysProperties.InBaseUnits().YoungsModulus);
		DeformationEnergy += CalculateElasticDeformationEnergy(ObjectThickness - DeformationLength, ObjectThickness, ObjectProperties.InBaseUnits().YoungsModulus);
		DeformationEnergy *= DeformationFactor;
		//const double DeformationEnergy = CalculateDeformationEnergy(KineticEnergy, ImpactArea, DeformationLength, ProjectileProperties.Length / 100.0) * DeformationFactor;
		ImpartedEnergy = DeformationEnergy;
		if(bPrintDebugInfo)
		{
			TB_LOG_SPACER();
			TB_LOG(Display, TEXT("Imparted Energy: %f"), ImpartedEnergy);
		}
		const double MinEnergy = MapRangeClamped(0.0, 90, 5, 200, AngleOfImpact);
		if (ImpartedEnergy <= MinEnergy) // Prevents ricochets where very little energy was imparted
		{
			return false;
		}
		if (ImpactVelocity.Size() < 25) // Prevents very low velocity ricochets
		{
			return false;
		}

		const double CriticalAngle = ObjectProperties.bIsFluid ? GetCriticalRicochetAngleForFluid(ObjectProperties.InBaseUnits().Density, ProjectilePhysProperties.InBaseUnits().Density) : ObjectProperties.GetCriticalRicochetAngle();

		const double FroudeNumber = GetFroudeNumber(ProjectileProperties.GetRadiusMeters(), ImpactVelocity.Length(), 9.81);
		double x = (20.0 / DOUBLE_PI) * gcem::sqrt(ObjectProperties.InBaseUnits().Density / ProjectilePhysProperties.InBaseUnits().Density);
		
		if (FroudeNumber <= (20.0 / DOUBLE_PI) * gcem::sqrt(ProjectilePhysProperties.InBaseUnits().Density))
		{
			return false;
		}

		double ImpactAxisOffset; // distance between the point of impact and the bullet's axis of revolution
		const double ImpactNoseOffset = ProjectileProperties.GetRadiusMeters() - FVector::Dist(VectorUtils::GetClosestPointOnLine(HitResult.ImpactPoint, HitResult.TraceStart, HitResult.TraceEnd, ImpactAxisOffset), HitResult.Location);

		double AngleBetweenSlopeAndNormal = 0.0;
		if constexpr (TIsDerivedFrom<T, FTBBullet>::Value)
		{
			const double BulletSlope = Projectile.BulletProperties.Ogive.GetSlope(ImpactNoseOffset, FMath::Sign(AngleOfImpactRadians) == 1);
			AngleBetweenSlopeAndNormal = NormalizeAngleToPlusMinus90DegreeRange(FMath::RadiansToDegrees(acos(BulletSlope)));

			// TODO: Use meplat diameter instead
			if (Projectile.BulletVariation.Contains(ETBBulletVariation::HOLLOW) && ImpactAxisOffset < (Projectile.BulletProperties.Ogive.Length / 3.0)) // if the bullet is hollow point, assume that it will likely just deform unless the impact point is more than a third of the way from the nose to the full diameter.
			{
				return false;
			}
		}
		else
		{
			AngleBetweenSlopeAndNormal = NormalizeAngleToPlusMinus90DegreeRange(AcosD(ImpactAxisOffset / ProjectileProperties.Radius));
		}
		/*AngleOfImpactRadians -= FMath::DegreesToRadians(AngleBetweenSlopeAndNormal);
		AngleOfImpact = FMath::RadiansToDegrees(AngleOfImpactRadians);*/

		if (bPrintDebugInfo)
		{
			TB_LOG(Display, TEXT("Ricochet Angle: %f\nAngleBetweenSlopeAndNormal: %f"), AngleOfImpact, AngleBetweenSlopeAndNormal);
		}


		const double MaximumEnergyTransfer = ObjectProperties.EstimateMaxImpartedRicochetEnergy(ImpactArea, ObjectThickness);

		if (bPrintDebugInfo)
		{
			TB_LOG(Display, TEXT("Critical Angle: %f\nImpact Angle: %f\nMaximum Energy Transfer: %f\nKinetic Energy: %f"), CriticalAngle, AngleOfImpact, MaximumEnergyTransfer, KineticEnergy);
		}

		if (AngleOfImpact > CriticalAngle || AngleOfImpact > TB::Configuration::RicochetAngleCutoff || AngleOfImpact > OverrideAngle) // Angle of impact is too steep
		{
			return false;
		}
		else if (ImpartedEnergy > KineticEnergy) // Ricochet would require too much energy
		{
			return false;
		}
		else if (KineticEnergy < TB::Configuration::RicochetEnergyRatioThreshold * ImpartedEnergy) // Don't bounce off unless we have some kinetic energy to spare
		{
			return false;
		}
		else if (KineticEnergy < MaximumEnergyTransfer) // We have enough kinetic energy to ricochet, but not so much that we are more likely to penetrate instead.
		{
			if (bUseProbability && Probability < FMath::FRand())
			{
				return false;
			}

			if(CheckForFailure)
			{
				const double ImpactSpeed = ImpactVelocity.Size();
				double ImpactedArea = 0.0;
				double Length = ProjectileProperties.GetRadiusMeters() / 3.0;
				if constexpr (TIsDerivedFrom<T, FTBBullet>::Value)
				{
					ImpactedArea = Projectile.BulletProperties.ApparentImpactArea / 10000.0;
					Length = Projectile.BulletProperties.ApparentLength * 300.0;
				}
				else
				{
					ImpactedArea = ProjectileProperties.GetFrontalCSA();
				}
				const double ImpactDuration = (ProjectileProperties.GetRadiusMeters() / 2.0) / ImpactSpeed;
				const double VelocityPerpendicular = abs(ImpactVelocity | SurfaceNormal); // Component of the ImpactVelocity perpendicular to the SurfaceNormal
				FMaterialFailure failure = FMaterialFailure::CheckForModeIFailures(ImpactVelocity, SurfaceNormal, AngleOfImpactRadians, ImpactedArea, ProjectileProperties.Mass, ProjectileProperties.GetRadiusMeters(), Length, ProjectileProperties.GetFrontalCSA(), ObjectThickness, ObjectProperties.InBaseUnits(),ProjectilePhysProperties.InBaseUnits());
				failure |= FMaterialFailure::CheckForModeIIFailures(VelocityPerpendicular, AngleOfImpactRadians, ImpactedArea, ProjectileProperties.Mass, MPaToPa(ObjectProperties.ShearStrength), ObjectThickness);
				if (failure && failure.FailureEnergy > KineticEnergy) // Bullet will cause material to fail, it's probably not going to be bouncing off...
				{
					if (bPrintDebugInfo)
					{
						TB_LOG_WRAPPED(Warning, TEXT("Failure\nFailure Type: %s\nImparted Energy: %f\n dV: %f"), *GetMaterialFailureModeString(failure.FailureMode), failure.FailureEnergy, CalculateVelocityFromKineticEnergy(KineticEnergy - abs(failure.FailureEnergy), ProjectileProperties.Mass));
					}
					return false;
				}
			}

			NewVelocity = NewVelocity.RotateAngleAxis(-2.0*AngleBetweenSlopeAndNormal - 5, SurfaceNormal);
			NewVelocity = FMath::GetReflectionVector(ImpactVelocity.GetSafeNormal(), SurfaceNormal);
			const double NewMagnitude = CalculateVelocityFromKineticEnergy(KineticEnergy - ImpartedEnergy, ProjectileProperties.Mass);
			const FVector VelNorm = NewVelocity.GetSafeNormal();
			NewVelocity = VelNorm * NewMagnitude;
			
			//const bool bIsMovingIntoWall = (VelNorm | SurfaceNormal) < 0.01; // New velocity is off, the bullet should be pointing away from the surface, not into it.
			//if (bIsMovingIntoWall)
			//{
			//	// Constrain ricochet vector to be coplanar to the object
			//	const FVector NewNorm = SurfaceNormal ^ VelNorm;
			//	NewVelocity = NewNorm * NewMagnitude;
			//}

			NewVelocity = PhysMatHelpers::GetAdjustedVelocity(NewVelocity, SurfaceNormal, ObjectProperties);
	
			const FVector ImpactPoint = HitResult.Location;
			if(bDrawDebugTrace)
			{
				GameThreadTask([World, ImpactPoint, NewVelocity]()
				{
					if (World)
					{
						DrawDebugDirectionalArrow(World, ImpactPoint, ImpactPoint + (NewVelocity.GetSafeNormal() * 5.0), 2.5, FColor::Blue, false, 30.f);
					}
				});
			}

			return true;
		}
		else
		{
			// Again, very unlikely
			return false;
		}
	}

	bool ShouldRicochet(const FHitResult& HitResult, const FTBBullet& Bullet, const FVector& ImpactVelocity, const FPhysMatProperties& ObjectProperties, const FPhysMatProperties& ProjectileProperties, const double ObjectThickness, const double ImpactArea, double& AngleOfImpact, double& ImpartedEnergy, FVector& NewVelocity, const bool CheckForFailure, bool bDrawDebugTrace, bool bPrintDebugInfo)
	{
		return TShouldRicochet<FTBBullet>(HitResult, Bullet, ImpactVelocity, ObjectProperties, ProjectileProperties, ObjectThickness, ImpactArea, AngleOfImpact, ImpartedEnergy, NewVelocity, CheckForFailure, bDrawDebugTrace, bPrintDebugInfo);
	}

	bool ShouldRicochet(const FHitResult& HitResult, const FTBProjectile& Projectile, const FVector& ImpactVelocity, const FPhysMatProperties& ObjectProperties, const FPhysMatProperties& ProjectileProperties, const double ObjectThickness, const double ImpactArea, double& AngleOfImpact, double& ImpartedEnergy, FVector& NewVelocity, const bool CheckForFailure, bool bDrawDebugTrace, bool bPrintDebugInfo)
	{
		return TShouldRicochet<FTBProjectile>(HitResult, Projectile, ImpactVelocity, ObjectProperties, ProjectileProperties, ObjectThickness, ImpactArea, AngleOfImpact, ImpartedEnergy, NewVelocity, CheckForFailure, bDrawDebugTrace, bPrintDebugInfo);
	}

	namespace CavityFormingPhaseUtils
	{
		double GetAddedMass(const double Depth, const double FluidDensity, const double ProjectileRadius)
		{
			if (Depth > ProjectileRadius)
			{
				return 0.0;
			}
			else
			{
				static constexpr double TwoPiOverThree = (2 * DOUBLE_PI) / 3.0;
				static constexpr double ThreeOverTwo = 3.0 / 2.0;

				using namespace MathUtils;
				return TwoPiOverThree * FluidDensity * gcem::pow((2 * ProjectileRadius * Depth) - sqr(Depth), ThreeOverTwo); // 2pi/3 * p * (2Rd - d^2)^3/2
			}
		}

		FVector GetVelocityInCavityFormingPhase(const double Mass, const FVector& InitialVelocity, const double AddedMass)
		{
			return (Mass * InitialVelocity) / (Mass + AddedMass);
		}
	}

	FVector TCalculateProjectileVelocityInCavityFormingPhase(const FHitResult& HitResult, const FVector& ImpactVelocity, const double FluidDensity, const double Mass, const double Radius, const double ApparentRigidIndenterAngle, const double ImpactArea, const FVector& InitialLocation, FVector& FinalLocation)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TB::BallisticFunctions::TCalculateProjectileVelocityInCavityFormingPhase);

		using namespace TB::MathUtils;
		using namespace CavityFormingPhaseUtils;

		UWorld* World = nullptr;
		if (TB_VALID_OBJECT(HitResult.GetActor()))
		{
			World = HitResult.GetActor()->GetWorld();
		}
		else if (TB_VALID_OBJECT(HitResult.GetComponent()))
		{
			World = HitResult.GetComponent()->GetWorld();
		}
		TB_RET_COND_VALUE(!World, FVector::ZeroVector);
		const FVector SurfaceNormal = HitResult.Normal;

		const double SquareCosineOfAria = sqr(cos(FMath::DegreesToRadians(ApparentRigidIndenterAngle)));


		FVector FinalVelocity = ImpactVelocity;
		FVector CurrentLocation = InitialLocation;
		/* Separate cavity forming phase into sections for better accuracy. */
		{
			auto GetVelocityAtSection = [&](const double depthToRadiusRatio, const FVector& PreviousSectionVelocity)
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(TB::BallisticFunctions::TCalculateProjectileVelocityInCavityFormingPhase::GetVelocityAtSection);

				const double Distance = depthToRadiusRatio * Radius;
				const double Angle = NormalizeAngleToPlusMinus90DegreeRangeRadians(VectorUtils::GetImpactAngle(SurfaceNormal, PreviousSectionVelocity));
				FVector SectionVelocity = GetVelocityInCavityFormingPhase(Mass, ImpactVelocity, GetAddedMass(Distance, FluidDensity, Radius));
				const double SectionTime = Distance * ((PreviousSectionVelocity.Size() + SectionVelocity.Size()) / 2.0); // t = s / v_avg
				FVector SectionRadialAcceleration = ApparentRigidIndenterAngle > 0 ? MapRangeClamped(-DOUBLE_PI, DOUBLE_PI, -1.0, 1.0, Angle) * ((FluidDensity * SectionVelocity * SquareCosineOfAria) / ImpactArea) : FVector::ZeroVector; // Add in the force caused by the angle between our velocity vector and the normal of the side of the projectile
				SectionRadialAcceleration /= Mass;

				FinalLocation = CurrentLocation + (SectionVelocity * SectionTime) + 0.5 * (SectionRadialAcceleration * sqr(SectionTime)); // x + v*dt + 0.5 * a*dt^2
				return SectionVelocity;
			};

			FinalVelocity = GetVelocityAtSection(0.1, FinalVelocity);
			FinalVelocity = GetVelocityAtSection(0.25, FinalVelocity);
			FinalVelocity = GetVelocityAtSection(0.5, FinalVelocity);
			FinalVelocity = GetVelocityAtSection(1, FinalVelocity);
		}
		return FinalLocation;
	}

	FVector CalculateProjectileVelocityInCavityFormingPhase(const FHitResult& HitResult, FTBBullet& Bullet, const FVector& ImpactVelocity, const double FluidDensity, const FVector& InitialLocation, FVector& FinalLocation)
	{
		return TCalculateProjectileVelocityInCavityFormingPhase(HitResult, ImpactVelocity, FluidDensity, Bullet.BulletProperties.Mass, Bullet.BulletProperties.GetRadiusMeters(), Bullet.BulletProperties.ApparentRigidIndenterAngle, Bullet.BulletProperties.GetSurfaceArea(), InitialLocation, FinalLocation);
	}

	FVector CalculateProjectileVelocityInCavityFormingPhase(const FHitResult& HitResult, FTBProjectile& Projectile, const FVector& ImpactVelocity, const double FluidDensity, const FVector& InitialLocation, FVector& FinalLocation)
	{
		return TCalculateProjectileVelocityInCavityFormingPhase(HitResult, ImpactVelocity, FluidDensity, Projectile.Mass, Projectile.GetRadiusMeters(), 0.0, Projectile.GetSurfaceArea(), InitialLocation, FinalLocation);
	}

	void GetRicochetVectorAndAngle(FVector& RicochetVector, double& Angle, const FHitResult& HitResult, const double& aria, const bool bRandomAdjustment)
	{
		using namespace TB::MathUtils;
		FVector Axis;
		const FVector SurfaceNormal = HitResult.Normal;
		FVector ImpactDirection = (HitResult.TraceEnd - HitResult.TraceStart).GetSafeNormal();
		double AngleOfImpact = NormalizeAngleToPlusMinus90DegreeRangeRadians(VectorUtils::GetImpactAngle(SurfaceNormal, ImpactDirection));
		if (bRandomAdjustment)
		{
			const double AngleOfImpactDeg = FMath::RadiansToDegrees(AngleOfImpact);
			double AngleAdjustment = FMath::RandRange(-5.0, 22.5);
			AngleAdjustment = AngleAdjustment >= AngleOfImpactDeg ? AngleOfImpactDeg - 2.0 : AngleAdjustment;
			ImpactDirection = ImpactDirection.RotateAngleAxis(AngleAdjustment, SurfaceNormal);
		}
		RicochetVector = FMath::GetReflectionVector(ImpactDirection, SurfaceNormal);
		AngleOfImpact = abs(FMath::RadiansToDegrees(AngleOfImpact));
		if (AngleOfImpact < 50)
		{
			AngleOfImpact = abs(AngleOfImpact - aria);
		}
		Angle = AngleOfImpact;
	}

	void DeformBullet(BulletPointer& Bullet, const double ImpartedEnergy, const double BulletYieldStrengthPascals, const double TargetYieldStrengthPascals)
	{
		using namespace TB::UnitConversions;
		using namespace TB::MathUtils;
		const double Stress = (BulletYieldStrengthPascals + TargetYieldStrengthPascals) * ImpartedEnergy / Bullet->BulletProperties.GetFrontalCSA();
		const double Strain = Stress / GPaToPa(Bullet->PhysicalProperties.YoungsModulus);
		double dL = 2.0 * Bullet->BulletProperties.GetRadiusMeters() * Strain;
		dL /= Bullet->BulletProperties.DeformationResistance;
		if (dL > Bullet->BulletProperties.Length / 100.0)
		{
			dL = Bullet->BulletProperties.Length + 1e-6;
		}
		const double MaxRadius = Bullet->BulletProperties.ExpansionCoefficient * Bullet->BulletProperties.GetRadiusMeters();
		const double dR = -Bullet->PhysicalProperties.PoissonsRatio * dL;
		double NewRadius = Bullet->BulletProperties.GetRadiusMeters() + dR;
		NewRadius = NewRadius > MaxRadius ? MaxRadius : NewRadius;
		const double Actual_dR = NewRadius - Bullet->BulletProperties.GetRadiusMeters();
		const double ExpansionRatio = NewRadius / MaxRadius;
		Bullet->BulletProperties.Length -= dL * 100.0;
		Bullet->BulletProperties.ApparentLength -= dL * 100.0 > Bullet->BulletProperties.ApparentLength ? Bullet->BulletProperties.ApparentLength + 1e-7 : dL * 100.0;
		Bullet->BulletProperties.Radius = NewRadius * 100.0;
		Bullet->BulletProperties.ApparentRadius += Actual_dR * 100.0;
		Bullet->BulletProperties.ApparentImpactArea = sqr(Bullet->BulletProperties.ApparentRadius / 100.0) * DOUBLE_PI;
		Bullet->BulletProperties.BallisticCoefficient = Bullet->BulletProperties.Mass / (Bullet->BulletProperties.GetFrontalCSA() * 1.8); // Change the ballistic coefficient to more accurately represent a flag-nosed projectile.
		Bullet->BulletProperties.DragModel = ETBGModel::G1; // Also change to G1
		Bullet->BulletProperties.ApparentRigidIndenterAngle = FMath::Lerp(Bullet->BulletProperties.ApparentRigidIndenterAngle, 90.0, ExpansionRatio);
	}

	// WIP
	double MushroomBullet(BulletPointer& bullet, const double ImpactEnergy)
	{
		// TODO_LOWPRI: Utilize contact theory, but treat the bullet as the plane and the impacted object as the indentor?
		const double ec = bullet->BulletProperties.ExpansionCoefficient;
		const double maxRadius = bullet->BulletProperties.Radius * ec;

		const double al = bullet->BulletProperties.ApparentLength;
		const double initialRadius = bullet->BulletProperties.ApparentRadius / 100;
		const double initialArea = TB_SQR(DOUBLE_PI * initialRadius);
		const double E = bullet->PhysicalProperties.YoungsModulus;

		double dl = ec * sqrt((ImpactEnergy * 1000 * (al / 100)) / (initialArea * TB_SQR(E))) / (initialArea * TB_SQR(E)) / bullet->BulletProperties.DeformationResistance;
		dl = dl > al ? al - 0.01 : dl;
		const double deformationRatio = al / dl;
		bullet->BulletProperties.ApparentLength -= dl;
		bullet->BulletProperties.Length -= dl;
		bullet->BulletProperties.BallisticCoefficient = bullet->BulletProperties.Mass / (bullet->BulletProperties.CrossSectionalArea * 1.8); // Change the ballistic coefficient to represent a flat-nosed Projectile.
		double newAr = bullet->BulletProperties.ApparentImpactArea / (al - dl);
		newAr = newAr <= maxRadius ? (newAr >= bullet->BulletProperties.ApparentRadius ? newAr : bullet->BulletProperties.ApparentRadius) : maxRadius;
		bullet->BulletProperties.ApparentRadius = newAr;
		bullet->BulletProperties.ApparentImpactArea = TB_SQR(DOUBLE_PI * newAr);
		bullet->BulletProperties.ApparentRigidIndenterAngle = FMath::Lerp(bullet->BulletProperties.ApparentRigidIndenterAngle, 90.0, deformationRatio);

		dl /= 100.0;
		const double rootA0EPI = sqrt(initialArea * TB_SQR(E) * DOUBLE_PI);
		double expendedEnergy = (dl * TB_SQR(E) * rootA0EPI) / sqrt(al / 100);
		//double expendedEnergy = (TB_SQR(dl) * powf(initialArea * TB_SQR(E), 3)) / (al / 100);
		double newEnergy = ImpactEnergy - expendedEnergy;
		return newEnergy > 0 && ImpactEnergy > newEnergy ? newEnergy : 0.0;
	}

	double CalculateDepthOfPenetrationIntoFluid(const double InitialVelocity, const double DragForce)
	{
		if (DragForce == 0)
		{
			return 0;
		}
		else
		{
			return gcem::log(InitialVelocity / 0.1) / DragForce;
		}
	}

	FVector CalculateExitVelocity(const FHitResult& HitResult, BulletPointer& ActualBullet, const FVector& ImpactVelocity, const double PenetrationThickness, const FPhysMatProperties& ObjectProperties, bool& bIsZero, double& dE, double& DepthOfPenetration, const float PenetrationMultiplier, bool bDebugPrint)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TB::BallisticFunctions::CalculateExitVelocity);

		bDebugPrint |= CVarPrintBallisticsDebugInfo.GetValueOnAnyThread();

		using namespace UnitConversions;
		using namespace MathUtils;

		const double ObjectThickness = PenetrationThickness / 100.0;
		const FVector& SurfaceNormal = HitResult.ImpactNormal;

		const FTBBulletPhysicalProperties& BulletProperties = ActualBullet->BulletProperties;

		FPhysMatProperties TempBulletPhysProperties = ActualBullet->PhysicalProperties;
		TempBulletPhysProperties.Density = TempBulletPhysProperties.IsInBaseUnits() ? BulletProperties.Density / 1000.0 : BulletProperties.Density;
		const FPhysMatProperties BulletPhysMatPropertiesInBaseUnits = TempBulletPhysProperties.InBaseUnits();
		const FPhysMatProperties ObjectPropertiesInBaseUnits = ObjectProperties.InBaseUnits();
		const double ObjectStiffness = 1.0 - sqr(ObjectPropertiesInBaseUnits.PoissonsRatio);
		const double StrengthValue = ObjectPropertiesInBaseUnits.CompressiveStrength > 0 ? gcem::min(ObjectPropertiesInBaseUnits.GetYieldStrength(), ObjectPropertiesInBaseUnits.CompressiveStrength) : ObjectPropertiesInBaseUnits.GetYieldStrength();

		const FVector& ImpactVelocityMS = ImpactVelocity / 100.0;
		const double ImpactSpeed = ImpactVelocityMS.Size();

		const double KineticEnergy = CalculateKineticEnergy(BulletProperties.Mass, ImpactSpeed);

		const double ImpactAngle = VectorUtils::GetImpactAngle(HitResult.Normal, ImpactVelocity.GetSafeNormal());
		const double ImpactAngleDeg = FMath::RadiansToDegrees(ImpactAngle);
		
		const double StressConcentrationFactorPeterson =	1 + (1 - gcem::cos(ImpactAngle));
		const double StressConcentrationFactorBullet =  1 +	2 * (1 - gcem::cos(FMath::DegreesToRadians(BulletProperties.ApparentRigidIndenterAngle)));
		const double CombinedStressConcentrationFactor = (StressConcentrationFactorPeterson * StressConcentrationFactorBullet) / 2.0;
		const double StressConcentrationFactor = CombinedStressConcentrationFactor > 0 ? CombinedStressConcentrationFactor : gcem::max(StressConcentrationFactorBullet, StressConcentrationFactorPeterson);

		const double TipLength = BulletProperties.bIsSpherical ? BulletProperties.GetRadiusMeters() : BulletProperties.Ogive.Length / 100.0;
		const double TipVolume = BulletProperties.bIsSpherical ? BulletProperties.GetVolume() / 2.0 : BulletProperties.Ogive.GetShapeVolume();

		/* Calculate energy required for tip to penetrate */
		double TipEnergy = ObjectPropertiesInBaseUnits.FractureToughness * TipLength * TipVolume * StressConcentrationFactor;
		TipEnergy += StrengthValue	* TipLength * TipVolume * StressConcentrationFactor;
		TipEnergy *= gcem::pow(TipEnergy / KineticEnergy, 1.1);

		const double BodyVolume = BulletProperties.GetVolume() - TipVolume;
		double BodyWork = (StrengthValue * BodyVolume * ObjectThickness * StressConcentrationFactor * gcem::exp(10 * ObjectThickness)) / ObjectStiffness;

		const double EnergyToPropagateFracture = ObjectPropertiesInBaseUnits.CalculateEnergyRequiredToPropagateFracture(ImpactSpeed, BulletProperties.GetFrontalCSA(), ObjectThickness);
		BodyWork += EnergyToPropagateFracture;

		const double DensityFactor = gcem::sqrt(ObjectPropertiesInBaseUnits.Density / BulletPhysMatPropertiesInBaseUnits.Density);
		const double SectionalDensityFactor = sqr(1 + BulletProperties.SectionalDensity);

		double TotalWork = (TipEnergy + BodyWork) * FMath::Clamp(DensityFactor, 0.0, 1.5);
		TotalWork /= SectionalDensityFactor;
		TotalWork /= PenetrationMultiplier;

		double NewSpeed = ImpactSpeed - CalculateVelocityFromKineticEnergy(TotalWork, BulletProperties.Mass);

		DepthOfPenetration = PenetrationThickness;

		if (NewSpeed <= 0.0)
		{
			bIsZero = true;
			DepthOfPenetration = PenetrationThickness * (ImpactSpeed / abs(ImpactSpeed - NewSpeed));
		}

		
		/* Determine new velocity */
		const FVector NormalizedVelocity = ImpactVelocityMS.GetSafeNormal();
		FVector NewVelocity = NormalizedVelocity * NewSpeed;
		const FVector VelocityPerpendicular = (NewVelocity | SurfaceNormal) * SurfaceNormal;
		const FVector VelocityParallel = NewVelocity - VelocityPerpendicular;
		const double ParallelModifier = MapRangeClamped(0, 90, 0.95, 0.85, abs(FMath::RadiansToDegrees(NormalizeAngleToPlusMinus90DegreeRangeRadians(ImpactAngle)))); // Reduce the parallel velocity by 5-15% depending on the impact angle to account for energy lost due to heat, sound, etc...
		NewVelocity = VelocityParallel * ParallelModifier + VelocityPerpendicular;

		
		bIsZero |= NewVelocity.Size() <= 0.0;

		if (NewVelocity.Size() > ImpactVelocityMS.Size())
		{
			bIsZero = true;
			DepthOfPenetration = 0.0;
		}
		if (bIsZero || NewVelocity.Size() > ImpactVelocityMS.Size())
		{
			NewVelocity = FVector::ZeroVector;
		}

		dE = KineticEnergy - CalculateKineticEnergy(BulletProperties.Mass, NewVelocity.Size());
		
		double dV = ImpactVelocityMS.Size() - NewVelocity.Size();

		if(bDebugPrint)
		{
			TB_LOG_WRAPPED(Warning, TEXT("Material: %s\nv0: %f | v1: %f | dV: %f | dE: %f | l: %f | pd: %f"), *ObjectProperties.ToString(), ImpactSpeed, NewVelocity.Size(), dV, dE, PenetrationThickness, DepthOfPenetration);
		}

		return NewVelocity * 100.0; // m/s to cm/s
	}

	FVector CalculateExitVelocitySimple()
	{
		return FVector();
	}

	FTBWoundCavity CalculateCavitationRadii(const double ProjectileSpeed, const double ImpartedEnergy, const double PenetrationDepth, const double ObjectThickness, const FTBProjectilePhysicalProperties& ProjectileProperties, const FPhysMatProperties& ProjectilePhysicalProperties, const FPhysMatProperties& ObjectPhysicalProperties)
	{
		if (ProjectileSpeed <= 0.0 || ImpartedEnergy <= 0.0 || PenetrationDepth <= 0.0 || ObjectThickness <= 0.0)
		{
			return FTBWoundCavity();
		}

		using namespace TB::MathUtils;

		FTBWoundCavity WoundCavity;

		const double ObjectDensity = ObjectPhysicalProperties.Density * 1000; // g/cm^3 to kg/m^3
		const double ProjectileDiameter = ProjectileProperties.GetRadiusMeters() * 2.0;

		const double Q = (ImpartedEnergy / ObjectDensity) * sqrt(ProjectileDiameter / ObjectThickness);
		WoundCavity.TemporaryCavityRadius = 1.18 * pow(Q, 1.0 / 3.0);
		WoundCavity.PermanentCavityDepth = PenetrationDepth;
		WoundCavity.PermanentCavityRadius = 0.29 * ProjectileDiameter * sqr(ProjectileSpeed / 1000.0) * pow(ObjectDensity / 1000.0, 1.0 / 3.0);

		return WoundCavity;
	}
};

FVector UBallisticFunctions::CalculateExitVelocity(const UObject* WorldContextObject, const FHitResult& HitResult, const FTBProjectilePhysicalProperties& Projectile, const FVector& ImpactVelocity, const double& PenetrationThickness, const TEnumAsByte<EPhysicalSurface>& SurfaceType, const FPhysMatProperties& ProjectilePhysicalMaterial, bool& bIsZero, double& dE, double& DepthOfPenetration, double PenetrationMultiplier, bool DebugPrint)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UBallisticFunctions::CalculateExitVelocity);

	using namespace TB::UnitConversions;
	using namespace TB::MathUtils;

	const FPhysMatProperties& physProperties = UTBPhysMatManager::Get().GetFromHitResult(HitResult);
	const double s0 = ImpactVelocity.Size() / 100.0; // Impact velocity in m/s
	const double pt1 = PenetrationThickness / 100.0;
	const double aia = sqr(UE_DOUBLE_PI * Projectile.Radius) / 10000.0;
	const double al = (Projectile.Length / 2.0 - 2.0 * Projectile.Radius) / 100.0;
	const double asa = (2 * sqr(UE_DOUBLE_PI * Projectile.Radius) + 2 * UE_DOUBLE_PI * Projectile.Radius * (Projectile.Length / 2.0 - 2.0 * Projectile.Radius)) / 10000.0;
	const double fsa = Projectile.GetSurfaceArea();
	double tl = Projectile.Radius;
	double L0 = Projectile.Length - tl; // Length of Projectile, minus the tip
	tl = pt1 < tl ? pt1 : tl; // If the penetration thickness is less than the tip length, clamp tip length to penetration thickness
	L0 = PenetrationThickness > L0 ? L0 : pt1; // Distance the rest of the Projectile has to travel to fully embed itself into the object after the tip has penetrated.

	// Determine contact pressure and kinetic energy of Projectile
	double dynP = UTerminalBallisticsStatics::CalculateDynamicPressure(Projectile.Density, s0);
	double ke = UTerminalBallisticsStatics::CalculateKineticEnergy(Projectile.Mass, s0);
	double p = PaToMPa((ke / al) / aia); //MPa
	bool failure = p > physProperties.UltimateTensileStrength;
	bool dynPFailure = dynP > physProperties.UltimateTensileStrength;

	double contactPressure;
	double radius;
	double approxDepth = 5.0 * ImpactApproximation(Projectile, ImpactVelocity, ProjectilePhysicalMaterial, physProperties, radius, contactPressure);
	bool bNoseOnly = approxDepth >= PenetrationThickness; // Petalling failure mode

	DepthOfPenetration = PenetrationThickness; // Assume it penetrates

	double s1 = 0.0;
	double s2 = 0.0;
	double s3 = 0.0;

	// Stage 1: Nose Penetration
	double y = physProperties.YoungsModulus * 0.1;
	double noseMPa = physProperties.FractureToughness * sqrt(tl);
	double m1 = physProperties.UltimateTensileStrength * tl * y * (dynPFailure ? 0.85f : 1.0);
	noseMPa += m1 + y;
	s1 = sqrt((noseMPa * aia) / (Projectile.Mass / 2.0));

	double requiredMPa;
	if (s1 >= s0)
	{
		requiredMPa = (Projectile.Mass * sqr(s1)) / (2.0 * tl);
		DepthOfPenetration = (noseMPa / requiredMPa) * tl;
	}
	else if (bNoseOnly)
	{
		DepthOfPenetration = PenetrationThickness;
	}

	if (!bNoseOnly)
	{
		// Stage 2: Full Penetration
		y *= 2.0;
		double fullMPa = physProperties.FractureToughness * sqrt(L0) * y;
		m1 = physProperties.UltimateTensileStrength * L0 * failure;
		fullMPa += m1 + physProperties.YoungsModulus;
		s2 = sqrt((fullMPa *asa) / (Projectile.Mass / 2.0));

		if (s2 + s1 >= s0)
		{
			requiredMPa = (Projectile.Mass * TB_SQR(s2)) / (2.0 * L0);
			DepthOfPenetration = (fullMPa / requiredMPa) * L0;
		}

		// Stage 3: Complete Penetration
		double penDepth = (PenetrationThickness > Projectile.Length ? PenetrationThickness - al : PenetrationThickness) / 100.0;
		double completeMPa = physProperties.FractureToughness * sqrt(penDepth) * y * PenetrationMultiplier;
		m1 = physProperties.UltimateTensileStrength * penDepth * failure;
		completeMPa += m1 + y;
		const double fa = fsa + (UE_DOUBLE_PI * Projectile.GetRadiusMeters() / 2.0) * penDepth;
		s3 = sqrt((completeMPa * fa) / (Projectile.Mass / 2.0));

		if (s3 + s2 + s1 >= s0)
		{
			requiredMPa = (Projectile.Mass * sqr(s3)) / (2.0 * penDepth);
			DepthOfPenetration = (completeMPa / requiredMPa) * penDepth;
		}
		else
		{
			DepthOfPenetration = PenetrationThickness;
		}
	}
	// Calculate velocity loss
	double dV = s1 + s2 + s3;
	double sNew = s0 - dV; // New speed(m/s)
	double forwardSpeed = ((ImpactVelocity - ImpactVelocity.GetSafeNormal() * (sNew * 100.0)).Rotation().Quaternion().GetForwardVector() * sNew).Size();
	bIsZero = FMath::IsNearlyZero(sNew) || sNew < 0.0 || s0 < sNew;

#if !NO_LOGGING
	if (DebugPrint)
	{
		TB_LOG_WRAPPED(Warning, TEXT("v0: %f | v1: %f | dV: %f | l: %f | Pd: %f"), s0, sNew, dV, PenetrationThickness, DepthOfPenetration);
	}
#endif

	dE = ke; // Default dE to be the inital kinetic energy. (Assumes Projectile has stopped during penetration)
	if (s0 - forwardSpeed > -0.5)
	{
		if (!bIsZero)
		{
			dE = ke - UTerminalBallisticsStatics::CalculateKineticEnergy(Projectile.Mass, sNew); // Since the Projectile has penetrated fully, set the change in kinetic energy appropriately
			return ImpactVelocity.GetSafeNormal() * (sNew * 100.0);
		}
	}
	else
	{
		bIsZero = true;
		return FVector::ZeroVector;
	}
	return FVector::ZeroVector;
}

FLinearColor UBallisticFunctions::GetTraceColor(const double CurrentSpeed, const double InitialSpeed)
{
	if (InitialSpeed > 0.0)
	{
		double p = CurrentSpeed > InitialSpeed ? 1.0 : CurrentSpeed / InitialSpeed;
		double hue = 0.0;
		double saturation = 1.0;
		if (p > 0.75)
		{
			hue = FMath::Lerp(80.0, 0.0, (p - 0.75) * 4.0);
		}
		else if (p > 0.5)
		{
			hue = FMath::Lerp(160.0, 80.0, (p - 0.5) * 4.0);
		}
		else if (p > 0.25)
		{
			hue = FMath::Lerp(240.0, 160.0, (p - 0.25) * 4.0);
		}
		else
		{
			hue = FMath::Lerp(360.0, 240.0, p * 4.0);
			saturation = FMath::Lerp(0.0, 1.0, p * 4.0);
		}
		return FLinearColor(hue, saturation, 1.0).HSVToLinearRGB();
	}
	else
	{
		return FLinearColor();
	}
}
