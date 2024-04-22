// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "PhysicsUtils.h"
#include "PhysMatManager/PhysMat.h"
#include "Types/TBProjectile.h"
#include "TerminalBallisticsConfiguration.h"
#include "TerminalBallisticsMacrosAndFunctions.h"



namespace TB
{
	double CalculateDeformationEnergy(const double KineticEnergy, const double ContactArea, const double DeformationLength, const double OriginalLength)
	{
		const double Force = KineticEnergy * DeformationLength;
		const double Stress = Force / ContactArea;
		return CalculateDeformationEnergy(Stress, DeformationLength, OriginalLength);
	}

	double CalculateDeformationEnergy(const double Stress, const double DeformationLength, const double OriginalLength)
	{
		const double Strain = DeformationLength / OriginalLength;
		return (Stress * Strain) / 2.0;
	}

	double GetTerminalVelocity(const double m, const double g, const FVector& c, const FVector& a, const FRotator& r, double p)
	{
		UE_ASSUME(m > 0.0 && g > 0.0);
		if (p == -1)
		{
			p = TB::Constants::FluidDensity_Air;
		}

		const FVector C = r.RotateVector(c);
		const FVector A = r.RotateVector(a);

		return sqrt((2 * m * g) / (p * A.Z * C.Z));
	}

	double CalculateAirDensityAtAltitude(const double g, const double h, const double T, const double P0, double Rh)
	{
		const double P = CalculateAirPressureAtAltitude(g, h, T, P0);
		if (Rh > 1 || Rh < 0)
		{
			Rh = CalculateHumidity(T, P, P / (TB::Constants::SpecificGasConstant_Air * T), GetDefault<UTBConfiguration>()->EnvironmentData.WorldHumidity); // Fallback if "Rh" isn't provided or is invalid.
		}
		const double p = CalculateAirDensity(P, TB::UnitConversions::CelsiusToKelvin(T), Rh);
		if (isinf(p) || isnan(p))
		{
			return TB::Constants::FluidDensity_Air;
		}
		else
		{
			return p;
		}
	}

	void CalculateAirDensityAndPressureAtAltitude(double& AirDensity, double& AirPressure, const double g, const double h, const double T, const double P0, double Rh)
	{
		AirPressure = CalculateAirPressureAtAltitude(g, h, T, P0);
		if (Rh > 1 || Rh < 0)
		{
			Rh = CalculateHumidity(T, AirPressure, AirPressure / (TB::Constants::SpecificGasConstant_Air * T), GetDefault<UTBConfiguration>()->EnvironmentData.WorldHumidity); // Fallback if "Rh" isn't provided or is invalid.
		}
		AirDensity = CalculateAirDensity(AirPressure, TB::UnitConversions::CelsiusToKelvin(T), Rh);
	}

	double CalculateHumidityAtAltitude(const double g, const double h, const double T, double ReferenceHumidity, double ReferencePressure)
	{
		using namespace TB::Constants;
		if (ReferencePressure <= 0)
		{
			ReferencePressure = ReferencePressure_SeaLevel;
		}
		if (ReferenceHumidity > 1 || ReferenceHumidity < 0)
		{
			ReferenceHumidity = GetDefault<UTBConfiguration>()->EnvironmentData.WorldHumidity; // Fallback to default
		}

		double Pressure;
		double Density;
		CalculateAirDensityAndPressureAtAltitude(Density, Pressure, g, h, T, ReferencePressure, ReferenceHumidity);

		return CalculateHumidity(T, Pressure, Density, ReferenceHumidity);
	}

	double GetHeatCapacityOfAir(const double T)
	{
		using namespace MathUtils;
		if (T < 0)
		{
			return MapRangeClamped(0, -15, 1.403, 1.404, T);
		}
		else
		{
			return MapRangeClamped(0, 1000, 1.403, 1.365, T);
		}
	}

	double CalculateSpeedOfSoundInAir(const double T)
	{
		return sqrt(TB::UnitConversions::CelsiusToKelvin(T) * GetHeatCapacityOfAir(T) * TB::Constants::SpecificGasConstant_Air);
	}

	double CalculateImpactStress(const double ProjectileMass, const double ImpactSpeed, const double ImpactArea, const double ImpactAngle, const double ObjectThickness)
	{
		using namespace MathUtils;
		return CalculateImpactStress(0.5 * ProjectileMass * sqr(ImpactSpeed), ImpactArea, ImpactAngle, ObjectThickness);
	}

	double CalculateImpactStress(const double KineticEnergy, const double ImpactArea, const double ImpactAngle, const double ObjectThickness)
	{
		double CosAngle = cos(ImpactAngle);
		if (CosAngle == 0.0) // This would result in a division by zero, but we can just adjust the angle slightly to avoid that.
		{
			CosAngle = 1e6;
		}
		return KineticEnergy / (ImpactArea * ObjectThickness * CosAngle);
	}

	double CalculateImpactStress(const double KineticEnergy, const double ImpactArea, const double ImpactAngle, const double ObjectThickness, const FPhysMatProperties& ObjectProperties, const FPhysMatProperties& ProjectileProperties)
	{
		const FPhysMatProperties& ObjectPropertiesInBaseUnits = ObjectProperties.InBaseUnits();
		const FPhysMatProperties& ProjectilePropertiesInBaseUnits = ProjectileProperties.InBaseUnits();

		const float ObjectE = ObjectPropertiesInBaseUnits.YoungsModulus;
		const float ProjectileE = ProjectilePropertiesInBaseUnits.YoungsModulus;

		const float StrainRateFactorObject = ObjectPropertiesInBaseUnits.StrainRateFactor;
		const float StrainRateFactorProjectile = ProjectilePropertiesInBaseUnits.StrainRateFactor;

		double CosAngle = cos(ImpactAngle);
		if (CosAngle == 0.0)
		{
			CosAngle = 1e-6;
		}

		const float ObjectEAdjusted = ObjectE * StrainRateFactorObject;
		const float ProjectileEAdjusted = ProjectileE * StrainRateFactorProjectile;

		const float ObjectYieldStrengthAdjusted = ObjectPropertiesInBaseUnits.GetYieldStrength() * StrainRateFactorObject;
		const float ProjectileYieldStrengthAdjusted = ProjectilePropertiesInBaseUnits.GetYieldStrength() * StrainRateFactorProjectile;

		const float ObjectV = ObjectPropertiesInBaseUnits.PoissonsRatio;
		const float ProjectileV = ProjectilePropertiesInBaseUnits.PoissonsRatio;

		const float ObjectPoissonFactor = (1 + ObjectV) / (1 - ObjectV);
		const float ProjectilePoissonFactor = (1 + ProjectileV) / (1 - ProjectileV);

		const float ObjectEnergyDissipationFactor = 0.8; // Account for various mechanisms that dissipate energy during impact.
		const float ObjectFailureFactor = 0.9; // Account for the increased "britleness" of materials under high load rates.

		const float ObjectKicAdjusted = ObjectPropertiesInBaseUnits.FractureToughness * ObjectFailureFactor;

		const float PlasticDeformationDenominator = ((ObjectYieldStrengthAdjusted + ProjectileYieldStrengthAdjusted) * ObjectThickness * sqrt(UE_DOUBLE_PI * ImpactArea * CosAngle * ObjectPoissonFactor) / 2);
		const float ElasticDenominator = ((ObjectEAdjusted + ProjectileEAdjusted) * ObjectThickness * sqrt(UE_DOUBLE_PI * ImpactArea * CosAngle * ProjectilePoissonFactor) / 2);

		const float ImpactStress = (KineticEnergy * ObjectKicAdjusted * ObjectEnergyDissipationFactor) / (PlasticDeformationDenominator + ElasticDenominator);

		return ImpactStress;
	}

	FVector CalculateCoriolisAcceleration(const FVector& Velocity, const double Latitude, const double Azimuth)
	{
		const double TwoOmega = TB::Constants::AngularSpeedOfEarth * 2.0;
		double SinLat, CosLat;
		double SinAz, CosAz;
		FMath::SinCos(&SinLat, &CosLat, Latitude);
		FMath::SinCos(&SinAz, &CosAz, Azimuth);
		const double CosLat_CosAz = CosLat * CosAz;
		const double CosLat_SinAz = CosLat * SinAz;
		const double Ax = -TwoOmega * (Velocity.Y * CosLat_SinAz + Velocity.Z * SinLat);
		const double Ay = TwoOmega * (Velocity.X * CosLat_SinAz + Velocity.Z * CosLat_CosAz);
		const double Az = TwoOmega * (Velocity.X * SinLat - Velocity.Y * CosLat_CosAz);

		return FVector(Ax, Ay, Az);
	}

	namespace Drag
	{
		using namespace TB::Constants;

		FVector CalculateDragForce(const FVector& cd, const FVector& csa, const FVector& velocity, const FQuat& orientation, double p)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(TB::Drag::CalculateDragForce);
			UE_ASSUME(p != 0.0);
			if (p == -1)
			{
				p = TB::Constants::FluidDensity_Air;
			}

			if (orientation == FQuat::Identity)
			{
				return 0.5 * TB_SQR(velocity) * p * csa * cd;
			}
			else
			{
				const FVector LocalVelocity = orientation.Inverse() * velocity; // Velocity in the object coordinate system

				const FVector LocalDragForce = 0.5 * TB_SQR(LocalVelocity) * p * csa * cd;


				return orientation.RotateVector(LocalDragForce);
			}
		}

		FVector CalculateDragForce(const FVector& cd, const FVector& csa, const FVector& velocity, const FVector& windVelocity, const FQuat& orientation, double p)
		{
			return CalculateDragForce(cd, csa, velocity + windVelocity, orientation, p);
		}

		FVector CalculateDragForce(const FTBProjectilePhysicalProperties& projectile, const FVector& velocity, const FVector& windVelocity, const FQuat& orientation, double p)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(TB::Drag::CalculateDragForceProjectile);
			UE_ASSUME(p != 0.0);
			if (p <= -1)
			{
				p = TB::Constants::FluidDensity_Air;
			}

			const FVector TotalVelocity = velocity + windVelocity;
			const FVector LocalVelocity = orientation.UnrotateVector(TotalVelocity);

			const FVector csa = projectile.CSA3D();
			FVector cd = projectile.GetDragCoefficient3D();
			cd.X = projectile.GetDragCoefficient(LocalVelocity.X);

			const FVector LocalDragForce = 0.5 * p * (TB_SQR(LocalVelocity) * csa * cd);

			return orientation.RotateVector(LocalDragForce);
		}

		FVector CalculateDragMoment(const FTBProjectilePhysicalProperties& projectile, const FVector& dragPosition, const FVector& velocity, const FQuat& orientation, const double WettedArea, const double ConicalHalfAngle, double p)
		{
			if (p <= -1)
			{
				p = TB::Constants::FluidDensity_Air;
			}

			using namespace MathUtils;

			const FVector NosePressure = p * sqr(velocity) * sqr(cos(ConicalHalfAngle));
			const FVector NoseForce = NosePressure * WettedArea;

			const double Angle = VectorUtils::GetImpactAngle(orientation.GetForwardVector(), velocity);

			const FVector NoseForceLocal = orientation.Inverse().RotateVector(Angle * NoseForce);

			const FVector Moment = dragPosition.Length() * NoseForceLocal;
			return orientation.RotateVector(Moment);
		}

		FVector CalculateDragForce(const FTBProjectilePhysicalProperties& projectile, const FVector& velocity, const FVector& windVelocity, const FQuat& orientation, const double ConicalHalfAngle, const double WettedArea, double p)
		{
			if (p <= -1)
			{
				p = TB::Constants::FluidDensity_Air;
			}

			using namespace MathUtils;

			const FVector NosePressure = p * sqr(velocity) * sqr(cos(ConicalHalfAngle));
			const FVector NoseForce = NosePressure * WettedArea;
			const FVector NoseVelocity = NoseForce / velocity;

			const FVector TotalVelocity = velocity + windVelocity + NoseVelocity;
			const FVector LocalVelocity = orientation.Inverse().RotateVector(TotalVelocity);

			const FVector csa = projectile.CSA3D();
			FVector cd = projectile.GetDragCoefficient3D();
			cd.X = projectile.GetDragCoefficient(LocalVelocity.X);

			const FVector LocalDragForce = 0.5 * p * (TB_SQR(LocalVelocity) * csa * cd);

			return orientation.RotateVector(LocalDragForce);
		}

		double CalculateDragForce(const FTBProjectilePhysicalProperties& projectile, const double speed, const double windSpeed, double p)
		{
			return CalculateDragForce(projectile.GetDragCoefficient(speed + windSpeed), projectile.GetFrontalCSA(), speed, windSpeed, p);
		}
	};
};



