// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TerminalBallisticsStatics.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Types/TBProjectile.h"
#include "Bullets.h"
#include "TerminalBallisticsTypes.h"
#include "BallisticFunctions.generated.h"


namespace TB::BallisticFunctions
{
	enum EMaterialFailureMode : uint8
	{
		NoFailure = 0,
		Yield = 1 << 0,			// Material yield strength exceeded
		UTS = 1 << 1,			// Complete failure, ultimate tensile strength was exceeded
		Compressive = 1 << 2,	// Compressive strength was exceeded
		Shear = 1 << 3,			// Shear stress exceeded shear strength
		Impact = 1 << 4,		// Impact stress exceeded critical stress
		Fracture = 1 << 5		// Material fracture
	};
	ENUM_CLASS_FLAGS(EMaterialFailureMode);

	constexpr uint8 ModeIFailures = Compressive | Impact | Fracture | UTS;
	constexpr uint8 ModeIIFailure = Shear;

	const FString GetMaterialFailureModeString(const EMaterialFailureMode FailureMode);

	struct FMaterialFailure
	{
		EMaterialFailureMode FailureMode = EMaterialFailureMode::NoFailure;
		double FailureEnergy = 0.0;

		FMaterialFailure() = default;
		FMaterialFailure(const EMaterialFailureMode FailureMode, const double FailureEnergy)
			: FailureMode(FailureMode)
			, FailureEnergy(FailureEnergy)
		{}

		static double CalculatePenetrationEnergy(const double KineticEnergy, const double ImpactAngle, const double ContactArea, const double ProjectileNoseLength, const double ObjectThickness, const double ProjectileCSA, const FPhysMatProperties& PhysMatInBaseUnits, const FPhysMatProperties& ProjectilePhysMatInBaseUnits, double& MaxStress, double& AverageStress);

		static FMaterialFailure CheckForModeIFailures(const FVector& ImpactVelocity, const FVector& SurfaceNormal, const double ImpactAngle, const double ContactArea, const double ProjectileMass, const double ProjectileRadius, const double ProjectileNoseLength, const double ProjectileCSA, const double ObjectThickness, const FPhysMatProperties& PhysMatInBaseUnits, const FPhysMatProperties& ProjectilePhysMatInBaseUnits);
		
		static FMaterialFailure CheckForModeIIFailures(const double ImpactSpeed, const double ImpactAngle, const double ContactArea, const double ProjectileMass, const double ShearStrength, const double ObjectThickness);

		operator bool() const
		{
			return FailureMode != EMaterialFailureMode::NoFailure;
		}

		bool operator==(const FMaterialFailure& lhs) const
		{
			return FailureMode == lhs.FailureMode
				&& FailureEnergy == lhs.FailureEnergy;
		}

		FMaterialFailure& operator |=(const FMaterialFailure& rhs)
		{
			if (rhs)
			{
				if(!this || FailureEnergy > rhs.FailureEnergy)
				{
					*this = rhs;
				}
			}
			return *this;
		}

		static inline FMaterialFailure Min(const FMaterialFailure& lhs, const FMaterialFailure& rhs)
		{
			if (!rhs)
			{
				return lhs;
			}
			else if (!lhs)
			{
				return rhs;
			}
			else if (lhs.FailureEnergy < rhs.FailureEnergy)
			{
				return lhs;
			}
			else
			{
				return rhs;
			}
		}
		static inline FMaterialFailure Min(const FMaterialFailure& Failure1, const FMaterialFailure& Failure2, const FMaterialFailure& Failure3)
		{
			FMaterialFailure Min1 = Min(Failure1, Failure2);
			return Min(Min1, Failure3);
		}

		static FMaterialFailure NoMaterialFailure;
	};

	[[nodiscard]] bool ShouldRicochet(const FHitResult& HitResult, const FTBBullet& Bullet, const FVector& ImpactVelocity, const FPhysMatProperties& ObjectProperties, const FPhysMatProperties& ProjectileProperties, const double ObjectThickness, const double ImpactArea, double& AngleOfImpact, double& ImpartedEnergy, FVector& NewVelocity, const bool CheckForFailure = true, bool bDrawDebugTrace = false, bool bPrintDebugInfo = false);
	[[nodiscard]] bool ShouldRicochet(const FHitResult& HitResult, const FTBProjectile& Projectile, const FVector& ImpactVelocity, const FPhysMatProperties& ObjectProperties, const FPhysMatProperties& ProjectileProperties, const double ObjectThickness, const double ImpactArea, double& AngleOfImpact, double& ImpartedEnergy, FVector& NewVelocity, const bool CheckForFailure = true, bool bDrawDebugTrace = false, bool bPrintDebugInfo = false);

	FVector CalculateProjectileVelocityInCavityFormingPhase(const FHitResult& HitResult, FTBBullet& Bullet, const FVector& ImpactVelocity, const double FluidDensity, const FVector& InitialLocation, FVector& FinalLocation);
	FVector CalculateProjectileVelocityInCavityFormingPhase(const FHitResult& HitResult, FTBProjectile& Projectile, const FVector& ImpactVelocity, const double FluidDensity, const FVector& InitialLocation, FVector& FinalLocation);

	void GetRicochetVectorAndAngle(FVector& RicochetVector, double& Angle, const FHitResult& HitResult, const double& aria, const bool bRandomAdjustment = true);

	void DeformBullet(BulletPointer& Bullet, const double ImpartedEnergy, const double BulletYieldStrengthPascals, const double TargetYieldStrengthPascals);

	double MushroomBullet(BulletPointer& bullet, const double ImpactForce);

	double CalculateDepthOfPenetrationIntoFluid(const double InitialVelocity, const double DragForce);

	/**
	* Determine the velocity of a bullet after it has penetrated a target.
	* @param HitResult					HitResult structure
	* @param ActualBullet				The bullet
	* @param ImpactVelocity				Bullet velocity at time of impact (cm/s)
	* @param PenetrationThickness		Distance from the entry to exit points (cm)
	* @param ObjectProperties			Material properties of the hit object
	* @param BulletProperties			Material properties of the bullet
	* @param bIsZero					Is the final velocity of the bullet zero?
	* @param PenetrationMultiplier		A multiplier used to offset the penetration cabability of the bullet
	* @param DebugPrint					Whether or not to print debug info
	* @param DepthOfPenetration			Depth that the bullet was able to penetrate (cm)
	* @return							Velocity of the bullet after penetration (cm/s)
	*/
	[[nodiscard]] FVector CalculateExitVelocity(const FHitResult& HitResult, BulletPointer& ActualBullet, const FVector& ImpactVelocity, const double PenetrationThickness, const FPhysMatProperties& ObjectProperties, bool& bIsZero, double& dE, double& DepthOfPenetration, const float PenetrationMultiplier = 1.f, bool bDebugPrint = false);

	// STUB
	[[nodiscard]] FVector CalculateExitVelocitySimple();

	/**
	* Approximates the wound cavity created by projectile penetration.
	* @param ProjectileSpeed				Projectile Speed (m/s)
	* @param ImpartedEnergy					Energy imparted to the impacted object (J)
	* @param PenetrationDepth				How far the projectile penetrated into the object (m)
	* @param ObjectThickness				Thickness of the object where it was struck by the projectile
	* @param ProjectileProperties			Projectile Properties
	* @param ProjectilePhysicalProperties	Physical Properties of the projectile (FPhysMatProperties)
	* @param ObjectPhysicalProperties		Physical Properties of the penetrated object (FPhysMatProperties)
	* @return								The wound cavity created by the projectile
	*/
	FTBWoundCavity CalculateCavitationRadii(const double ProjectileSpeed, const double ImpartedEnergy, const double PenetrationDepth, const double ObjectThickness, const FTBProjectilePhysicalProperties& ProjectileProperties, const FPhysMatProperties& ProjectilePhysicalProperties, const FPhysMatProperties& ObjectPhysicalProperties);
};


UCLASS()
class TERMINALBALLISTICS_API UBallisticFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	/**
	* Newton's approximation for the impact depth of projectiles at high velocities, based only on momentum considerations.
	*/
	UFUNCTION(BlueprintPure, Category = "Ballistics", meta = (DisplayName = "Newtonian Depth Approximation"))
	static double NewtonianDepthApproximation(double ProjectileLength, double ProjectileDensity, double TargetDensity);

	/**
	* Approximation of impact depth and contact area derived by treating the Projectile as a conical indentor impacting an elastic half space as represented by the impacted object.
	* @param Projectile						Projectile
	* @param Velocity						Velocity of the projectile (cm/s)
	* @param ProjectileProperties			Properties of the Projectile
	* @param ObjectProperties				Properties of the impacted object
	* @param ContactRadius					Contact radius
	* @param P0								Maximum Pressure (Note, this is likely to be an extremely high number as it is close to the logarithmic asymptote of the pressure distribution. This should not be taken as the actual pressure exerted by the Projectile.)
	* @return								Total deformation depth
	*/
	static double ImpactApproximation(const FTBProjectilePhysicalProperties& Projectile, const FVector& Velocity, const FPhysMatProperties& ProjectileProperties, const FPhysMatProperties& ObjectProperties, double& ContactRadius, double& p0);
	/**
	* Approximation of impact depth and contact area derived by treating the Projectile as a conical indentor impacting an elastic half space as represented by the impacted object.
	* @param Projectile						Projectile
	* @param Velocity						Velocity of the projectile (cm/s)
	* @param ProjectileProperties			Properties of the Projectile
	* @param ObjectProperties				Properties of the impacted object
	* @param ContactRadius					Contact radius
	* @param P0								Maximum Pressure (Note, this is likely to be an extremely high number as it is close to the logarithmic asymptote of the pressure distribution. This should not be taken as the actual pressure exerted by the Projectile.)
	* @return								Total deformation depth
	*/
	UFUNCTION(BlueprintPure, Category = "Ballistics")
	static double ImpactApproximationForProjectile(const FTBProjectile& Projectile, const FVector& Velocity, const FPhysMatProperties& ProjectileProperties, const FPhysMatProperties& ObjectProperties, double& ContactRadius, double& P0)
	{
		return ImpactApproximation(Projectile, Velocity, ProjectileProperties, ObjectProperties, ContactRadius, P0);
	}
	/**
	* Approximation of impact depth and contact area derived by treating the Projectile as a conical indentor impacting an elastic half space as represented by the impacted object.
	* @param Projectile						Projectile
	* @param Velocity						Velocity of the projectile (cm/s)
	* @param ProjectileProperties			Properties of the Projectile
	* @param ObjectProperties				Properties of the impacted object
	* @param ContactRadius					Contact radius
	* @param P0								Maximum Pressure (Note, this is likely to be an extremely high number as it is close to the logarithmic asymptote of the pressure distribution. This should not be taken as the actual pressure exerted by the Projectile.)
	* @return								Total deformation depth
	*/
	UFUNCTION(BlueprintPure, Category = "Ballistics")
	static double ImpactApproximationBullet(const FTBBulletPhysicalProperties& Projectile, const FVector& Velocity, const FPhysMatProperties& ProjectileProperties, const FPhysMatProperties& ObjectProperties, double& ContactRadius, double& P0)
	{
		return ImpactApproximation(Projectile, Velocity, ProjectileProperties, ObjectProperties, ContactRadius, P0);
	}

	static FVector CalculateExitVelocity(const UObject* WorldContextObject, const FHitResult& HitResult, const FTBProjectilePhysicalProperties& Projectile, const FVector& ImpactVelocity, const double& PenetrationThickness, const TEnumAsByte<EPhysicalSurface>& SurfaceType, const FPhysMatProperties& ProjectilePhysicalMaterial, bool& bIsZero, double& dE, double& DepthOfPenetration, double PenetrationMultiplier = 1.0, bool DebugPrint = false);
	UFUNCTION(BlueprintCallable, Category = "Ballistics", DisplayName = "Calculate Exit Velocity", meta = (WorldContext = "WorldContextObject"))
	static FVector CalculateExitVelocityForBullet(const UObject* WorldContextObject, const FHitResult& HitResult, const FTBProjectilePhysicalProperties& Projectile, const FVector& ImpactVelocity, const double& PenetrationThickness, const TEnumAsByte<EPhysicalSurface>& SurfaceType, const FPhysMatProperties& ProjectilePhysicalMaterial, bool& bIsZero, double& dE, double& DepthOfPenetration, double PenetrationMultiplier = 1.0, bool DebugPrint = false)
	{
		return CalculateExitVelocity(WorldContextObject, HitResult, Projectile, ImpactVelocity, PenetrationThickness, SurfaceType, ProjectilePhysicalMaterial, bIsZero, dE, DepthOfPenetration, PenetrationMultiplier, DebugPrint);
	}
	UFUNCTION(BlueprintCallable, Category = "Ballistics", DisplayName = "Calculate Exit Velocity (Projectile)", meta = (WorldContext = "WorldContextObject"))
	static FVector CalculateExitVelocityForProjectile(const UObject* WorldContextObject, const FHitResult& HitResult, const FTBProjectilePhysicalProperties& Projectile, const FVector& ImpactVelocity, const double& PenetrationThickness, const TEnumAsByte<EPhysicalSurface>& SurfaceType, const FPhysMatProperties& ProjectilePhysicalMaterial, bool& bIsZero, double& dE, double& DepthOfPenetration, double PenetrationMultiplier = 1.0, bool DebugPrint = false)
	{
		return CalculateExitVelocity(WorldContextObject, HitResult, Projectile, ImpactVelocity, PenetrationThickness, SurfaceType, ProjectilePhysicalMaterial, bIsZero, dE, DepthOfPenetration, PenetrationMultiplier, DebugPrint);
	}

	UFUNCTION(BlueprintPure, Category = "Ballistics|Utils", DisplayName = "Get Trace Color")
	static FLinearColor GetTraceColor(const double CurrentSpeed, const double InitialSpeed);
};
