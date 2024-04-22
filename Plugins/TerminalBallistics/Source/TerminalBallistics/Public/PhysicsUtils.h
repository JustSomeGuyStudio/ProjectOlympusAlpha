// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MathUtils.h"
#include "Types/TBEnums.h"
#include "ThirdParty/gcem/gcem.hpp"


struct FTBProjectilePhysicalProperties;
struct FPhysMatProperties;
class UEnvironmentSubsystem;
namespace TB
{
	namespace Constants
	{
		constexpr double FluidDensity_Air = 1.225;
		constexpr double DynamicViscosity_Air = 1.789e-5;

		constexpr double FluidDensity_Water = 1000.0;

		constexpr double PSI_TO_PA = 0.00014503773800722;
		constexpr double PA_TO_PSI = 145.03773800722;
		constexpr double ATM_TO_PA = 101325;
		constexpr double ATM_TO_PSI = 14.696;
		constexpr double PSI_TO_ATM = 1.0 / 14.696;

		constexpr double MolarMass_Air = 0.0289644;

		constexpr double UniversalGasConstant = 8.31432;
		constexpr double SpecificGasConstant_Air = 287.0500676;
		constexpr double SpecificGasConstant_WaterVapor = 461.495;

		constexpr double ReferencePressure_SeaLevel = 101325;

		constexpr double Mach1 = 343; // Speed of sound in dry air at 20c

		constexpr double AngularSpeedOfEarth = 7.2921159e-5; // radians/s (sidreal day)
	};

	namespace UnitConversions
	{
#pragma region Temperature
		/**
		* Converts pressure from Celsius to Kelvin
		*/
		constexpr double CelsiusToKelvin(double T)
		{
			return T + 273.15;
		}
		/**
		* Converts pressure from Celsius to Fahrenheit
		*/
		constexpr double CelsiusToFahrenheit(double T)
		{
			return (5 * (T - 32)) / 9.0;
		}
		/**
		* Converts pressure from Fahrenheit to Celsius
		*/
		constexpr double FahrenheitToCelsius(double T)
		{
			return 1.8 * T + 32;
		}
		/**
		* Converts pressure from Fahrenheit to Kelvin
		*/
		constexpr double FahrenheitToKelvin(double T)
		{
			return CelsiusToKelvin(FahrenheitToCelsius(T));
		}
		/**
		* Converts pressure from Kelvin to Celsius
		*/
		constexpr double KelvinToCelsius(double T)
		{
			return T - 273.15;
		}
		/**
		* Converts pressure from Kelvin to Fahrenheit
		*/
		constexpr double KelvinToFahrenheit(double T)
		{
			return (T - 273.15) * (9.0 / 5.0) + 32;
		}
#pragma endregion

#pragma region Pressure
		constexpr bool IsPressureUnitMetric(const ETBPressureUnit Unit)
		{
			return Unit == ETBPressureUnit::UNIT_PA
				|| Unit == ETBPressureUnit::UNIT_KPA
				|| Unit == ETBPressureUnit::UNIT_MPA
				|| Unit == ETBPressureUnit::UNIT_GPA;
		}

		/**
		* Converts pressure from GPa to Pa
		*/
		constexpr double GPaToPa(double P)
		{
			return P * 1e9;
		}
		/**
		* Converts pressure from GPa to KPa
		*/
		constexpr double GPaToKPa(double P)
		{
			return P * 1e6;
		}
		/**
		* Converts pressure from GPa to MPa
		*/
		constexpr double GPaToMPa(double P)
		{
			return P * 1e3;
		}

		/**
		* Converts pressure from MPa to Pa
		*/
		constexpr double MPaToPa(double P)
		{
			return P * 1e6;
		}
		/**
		* Converts pressure from MPa to Kpa
		*/
		constexpr double MPaToKpa(double P)
		{
			return P * 1e3;
		}
		/**
		* Converts pressure from MPa to GPa
		*/
		constexpr double MPaToGPa(double P)
		{
			return P * 1e-3;
		}

		/**
		* Converts pressure from KPa to Pa
		*/
		constexpr double KPaToPa(double P)
		{
			return P * 1e3;
		}
		/**
		* Converts pressure from KPa to MPa
		*/
		constexpr double KPaToMPa(double P)
		{
			return P * 1e-3;
		}
		/**
		* Converts pressure from KPa to GPa
		*/
		constexpr double KPaToGPa(double P)
		{
			return P * 1e-6;
		}

		/**
		* Converts pressure from Pa to KPa
		*/
		constexpr double PaToKPa(double P)
		{
			return P * 1e-3;
		}
		/**
		* Converts pressure from Pa to MPa
		*/
		constexpr double PaToMPa(double P)
		{
			return P * 1e-6;
		}
		/**
		* Converts pressure from Pa to GPa
		*/
		constexpr double PaToGPa(double P)
		{
			return P * 1e-9;
		}

		/**
		* Converts pressure from Atm to Pa
		*/
		constexpr double AtmToPa(const double P)
		{
			return Constants::ATM_TO_PA * P;
		}
		/**
		* Converts pressure from Atm to Psi
		*/
		constexpr double AtmToPsi(const double P)
		{
			return Constants::ATM_TO_PSI * P;
		}
		/**
		* Converts pressure from Pa to Atm
		*/
		constexpr double PaToAtm(const double P)
		{
			return P / Constants::ATM_TO_PA;
		}
		/**
		* Converts pressure from Pa to Psi
		*/
		constexpr double PaToPsi(const double P)
		{
			return Constants::PA_TO_PSI * P;
		}
		/**
		* Converts pressure from Psi to Atm
		*/
		constexpr double PsiToAtm(double P)
		{
			return Constants::PSI_TO_ATM * P;
		}
		/**
		* Converts pressure from Psi to Pa
		*/
		constexpr double PsiToPa(const double P)
		{
			return Constants::PSI_TO_PA * P;
		}

		/**
		* Converts a pressure value to Pascals (Pa)
		* @param P				Pressure
		* @param PreviousUnit	Unit of pressure
		*/
		constexpr double ConvertToPa(double P, const ETBPressureUnit PreviousUnit)
		{
			if (IsPressureUnitMetric(PreviousUnit))
			{
				return P * pow(10.0, 3.0 * (uint8)PreviousUnit); // See comments in pressure enum for explanation
			}
			else if (PreviousUnit == ETBPressureUnit::UNIT_PSI)
			{
				return PsiToPa(P);
			}
			else // PreviousUnit must be UNIT_ATM then...
			{
				return AtmToPa(P);
			}
		}

		constexpr double ConvertToPsi(double P, const ETBPressureUnit PreviousUnit)
		{
			if (IsPressureUnitMetric(PreviousUnit))
			{
				P *= pow(10.0, 3.0 * (uint8)PreviousUnit); // See comments in pressure enum for explanation
			}
			else if (PreviousUnit == ETBPressureUnit::UNIT_PSI)
			{
				return P;
			}
			else
			{
				return AtmToPsi(P);
			}
			return PaToPsi(P);
		}

		constexpr double ConvertToAtm(double P, const ETBPressureUnit PreviousUnit)
		{
			if (IsPressureUnitMetric(PreviousUnit))
			{
				P *= pow(10.0, 3.0 * (uint8)PreviousUnit); // See comments in pressure enum for explanation
			}
			else if (PreviousUnit == ETBPressureUnit::UNIT_PSI)
			{
				return PsiToAtm(P);
			}
			else
			{
				return P;
			}
			return PaToPsi(P);
		}

		constexpr double ConvertPressureUnits(double P, const ETBPressureUnit PreviousUnit, const ETBPressureUnit NewUnit)
		{
			if (PreviousUnit == NewUnit)
			{
				return P;
			}
			if (NewUnit == ETBPressureUnit::UNIT_PSI)
			{
				return ConvertToPsi(P, PreviousUnit);
			}
			else if (NewUnit == ETBPressureUnit::UNIT_ATM)
			{
				return ConvertToAtm(P, PreviousUnit);
			}
			else
			{
				const int UnitDifference = (uint8)PreviousUnit - (uint8)NewUnit; // Difference in orders of magnitude between the units
				if (UnitDifference > 0)
				{
					return P * pow(10.0, 3.0 * UnitDifference);
				}
				else
				{
					return P * pow(10.0, 3.0 * abs(UnitDifference));
				}
			}
		}
#pragma endregion

#pragma region Rotation
		template<typename T>
		constexpr T RPMToRadiansPerSecond(const T RPM)
		{
			return (T)((RPM * UE_DOUBLE_PI) / 30.0);
		}

		template<typename T>
		constexpr T RPMToDegreesPerSecond(const T RPM)
		{
			return (T)(RPM / 6.0);
		}

		template<typename T>
		constexpr T RadiansPerSecondToRPM(const T RadiansPerSecond)
		{
			return (T)((30.0 * RadiansPerSecond) / UE_DOUBLE_PI);
		}

		template<typename T>
		constexpr T DegreesPerSecondToRadiansPerSecond(const T DegreesPerSecond)
		{
			return FMath::DegreesToRadians(DegreesPerSecond);
		}

		template<typename T>
		constexpr T RadiansPerSecondToDegreesPerSecond(const T RadiansPerSecond)
		{
			return FMath::RadiansToDegrees(RadiansPerSecond);
		}
#pragma endregion
	};

	static FQuat IntegrateAngularVelocity(const FQuat& R0, const FVector& AngularVelocity, const double dt)
	{
		const FQuat AngularVelocityAsQuat = FQuat(AngularVelocity.X, AngularVelocity.Y, AngularVelocity.Z, 0.0);
		return R0 + (AngularVelocityAsQuat * R0) * dt * 0.5;
	}

	/**
	* Returns the Saturation Vapor Pressure (kPa) of water at a given temperature.
	* @param T				Temperature (celsius)
	*/
	constexpr double GetSaturationVaporPressureOfWater(const double T)
	{
		constexpr double A = -7.90298;
		constexpr double B = 5.028;
		constexpr double C1 = 1.3816;
		constexpr double C2 = 11.344;
		constexpr double D1 = -0.00011343;
		constexpr double D2 = 23.832;
		constexpr double D3 = 3656.7;

		const double Tc = UnitConversions::CelsiusToKelvin(T);
		const double logT = gcem::log10(Tc / 273.15);
		const double C = gcem::pow(10, C2 - D3 / Tc);
		const double D = D1 * gcem::pow(10, D2 - D3 / Tc);
		const double E = gcem::exp(B + A * (1.0 - C + D * (1.0 - C) * (1.0 - C)));

		return 101.325 * E / 760.0; // Convert from mmHg to kPa
	}

	/**
	* Calculates Reynold's Number for an object moving through a fluid.
	* @param FluidDensity		Density of the fluid (kg/m^3)
	* @param DynamicViscosity	Dynamic viscosity of the fluid (Pa*s)
	* @param Diameter			Diameter of the object (m)
	* @param Velocity			Speed relative to the fluid. (m/s)
	*/
	template<typename T>
	T GetReynoldsNumber(const double FluidDensity, const double DynamicViscosity, const T Diameter, const T Velocity)
	{
		return (FluidDensity * Velocity * Diameter) / DynamicViscosity;
	}

	template<typename T>
	T GetReynoldsCd(const T ReynoldsNumber, const T InCd)
	{
		const T Denominator = (InCd / (UE_DOUBLE_PI * ReynoldsNumber)) + (T)1.0;
		return InCd / Denominator;
	}

	/**
	* Determines the critical angle in degrees for projectile impacts on fluids
	* An impact angle below this value indicates the possibility of a ricochet.
	* Any units may be provided for density, so long as both inputs share the same units.
	* @param FluidDensity		Density of the fluid.
	* @param ObjectDensity		Density of the object.
	* @return					Crtitical Angle (degrees)
	*/
	constexpr double GetCriticalRicochetAngleForFluid(const double FluidDensity, const double ObjectDensity)
	{
		return 18.0 / gcem::sqrt(FluidDensity / ObjectDensity);
	}

	constexpr double GetProjectileVelocityInFluid(const double InitialVelocity, const double DragForce, const double Depth)
	{
		return InitialVelocity * gcem::exp(-DragForce * Depth);
	}

	constexpr double GetFroudeNumber(const double Radius, const double Velocity, const double Gravity)
	{
		return Velocity / gcem::sqrt(Radius * Gravity);
	}

	/**
	* Calcuates the dynamic pressure of an object.
	* @param Density		Density (kg/m^3)
	* @param Speed			Speed (m/s)
	* @return				Dynamic Pressure (Pa)
	*/
	constexpr double CalculateDynamicPressure(double Density, double Speed)
	{
		UE_ASSUME(Density > 0 && Speed > 0);
		return (Density * TB_SQR(Speed)) / 2.0; // 1/2pv^2
	}

	/**
	* Calcuates the kinetic energy of an object.
	* @param Mass			Mass (kg)
	* @param Speed			Speed (m/s)
	* @return				Kinetic Energy (J)
	*/
	constexpr double CalculateKineticEnergy(const double Mass, const double Speed)
	{
		UE_ASSUME(Mass > 0 && Speed > 0);
		return (Mass * TB_SQR(Speed)) / 2.0; // KE = 1/2mv^2
	}

	/**
	* Calculates the velocity of an object given it's kinetic energy.
	* @param KineticEnergy	Kinetic Energy (J)
	* @param Mass			Mass (kg)
	* @return				Speed (m/s)
	*/
	constexpr double CalculateVelocityFromKineticEnergy(const double KineticEnergy, const double Mass)
	{
		UE_ASSUME(Mass > 0);
		const double NewV = gcem::sqrt((2.0 * gcem::abs(KineticEnergy)) / Mass);
		return gcem::copysign(NewV, KineticEnergy); // Deal with the case where KineticEnergy is negative
	}

	double CalculateDeformationEnergy(const double KineticEnergy, const double ContactArea, const double DeformationLength, const double OriginalLength);
	double CalculateDeformationEnergy(const double Stress, const double DeformationLength, const double OriginalLength);

	constexpr double CalculateElasticDeformationEnergy(const double DeformedLength, const double OriginalLength, const double YoungsModulus)
	{
		const double DeltaLength = OriginalLength - DeformedLength;
		const double Strain = DeltaLength / OriginalLength;
		return 0.5 * YoungsModulus * gcem::pow(Strain, 2);
	}
	
	/**
	* Returns the terminal velocity (m/s) of an object with the given parameters
	* @param Mass			Mass of the object (kg)
	* @param g				Acceleration due to Gravity (m/s)
	* @param c				Coefficent of drag for the object on each axis.
	* @param a				Cross sectional area of the object for each axis. (m^2)
	* @param r				Rotation of the object
	* @param p				Fluid density of the medium the object is passing through. Defaults to air.
	*/
	double GetTerminalVelocity(const double m, const double g, const FVector& c, const FVector& a, const FRotator& r, double p);

	/**
	* Calculates the air pressure at a given altitude.
	* @param g				Acceleration due to Gravity (m/s)
	* @param h				Altitude (m)
	* @param T				Temperature (Celsius)
	* @param P0				Reference Pressure/Pressure at sea level (Pa)
	* @return				Pressure (Pa)
	*/
	constexpr double CalculateAirPressureAtAltitude(const double g, const double h, const double T, const double P0 = TB::Constants::ReferencePressure_SeaLevel)
	{
		const double input = (-g * TB::Constants::MolarMass_Air * h) / (TB::Constants::UniversalGasConstant * TB::UnitConversions::CelsiusToKelvin(T));
		return P0 * gcem::exp(input);
	}

	/**
	* Calcutes the density of air with the given parameters
	* @param p				Absolute Pressure (Pa)
	* @param T				Absolute Temperature (Kelvin)
	* @param Rh				Relative Humitidy
	* @return				Air density (kg/m^3)
	*/
	constexpr double CalculateAirDensity(const double p, const double T, const double Rh)
	{
		using namespace TB::Constants;
		using namespace TB::UnitConversions;
		UE_ASSUME(p > 0 && T > 0);
		const double T_Celsius = KelvinToCelsius(T);
		if (Rh > 0.0 && Rh <= 1)
		{
			const double pv = KPaToPa(TB::GetSaturationVaporPressureOfWater(T_Celsius) * Rh);
			const double pd = p - pv;
			return (pd / (SpecificGasConstant_Air * T)) + (pv / (SpecificGasConstant_WaterVapor * T));
		}
		else
		{
			return p / (SpecificGasConstant_Air * T);
		}
	}

	/**
	* Calculates the air density at a given altitude.
	* @param g				Acceleration due to Gravity (m/s)
	* @param h				Altitude (m)
	* @param T				Temperature (Celsius)
	* @param P0				Reference Pressure/Pressure at sea level (Pa)
	* @param Rh				Relative Humitidy
	* @return				Air density (kg/m^3)
	*/
	double CalculateAirDensityAtAltitude(const double g, const double h, const double T, const double P0, double Rh);

	/**
	* Calculates both the density and pressure of air at a given altitude.
	* @param AirDensity		The calculated air density (kg/m^3)
	* @param AirPressure	The calculated air pressure (Pa)
	* @param g				Acceleration due to Gravity (m/s)
	* @param h				Altitude (m)
	* @param T				Temperature (Celsius)
	* @param P0				Reference Pressure/Pressure at sea level (Pa)
	* @param Rh				Relative Humitidy
	*/
	void CalculateAirDensityAndPressureAtAltitude(double& AirDensity, double& AirPressure, const double g, const double h, const double T, const double P0, double Rh = -1);

	/**
	* Calculates the humidity at a given altitude
	* @param g					Acceleration due to Gravity (m/s)
	* @param h					Altitude (m)
	* @param T					Temperature (Celsius)
	* @param ReferenceHumidity	Humidity at sea level
	* @param ReferencePressure	Pressure at sea level (Pa)
	*/
	double CalculateHumidityAtAltitude(const double g, const double h, const double T, double ReferenceHumidity = -1, double ReferencePressure = -1);

	/**
	* Calculates the humidity of air given temperature, pressure, air denstity, and the humidity at sea level.
	* @param T					Temperature (Celsius)
	* @param P					Pressure (Pa)
	* @param Density			Air density (kg/m^3)
	* @param ReferenceHumidity	Humidity at sea level
	*/
	constexpr double CalculateHumidity(const double Temperature, double Pressure, double Density, double ReferenceHumidity)
	{
		if (ReferenceHumidity <= 0)
		{
			return 0;
		}

		UE_ASSUME(Pressure > 0.0 && Temperature > 0.0 && Density > 0.0 && ReferenceHumidity >= 0.0 && ReferenceHumidity <= 1.0);
		Pressure = TB::UnitConversions::PaToKPa(Pressure);
		const double SaturationVaporPressure = GetSaturationVaporPressureOfWater(Temperature);
		const double ActualVaporPressure = (Density * TB::Constants::SpecificGasConstant_Air * Temperature) / 0.622 * Pressure;
		return (ActualVaporPressure / SaturationVaporPressure) / ReferenceHumidity;
	}

	/**
	* Calculates the viscosity of air given temperature and pressure.
	* @param T			Temerature (Celsius)
	* @param P			Absolute Pressure (Pa)
	*/
	constexpr double CalculateAirViscosity(const double T, const double P)
	{
		constexpr double T0 = 273.15;
		constexpr double C = 120;

		const double T1 = T + T0;

		using namespace TB::Constants;
		double mu = DynamicViscosity_Air * (T0 + C) / (T1 + C) * gcem::pow(T1 / T0, 1.5);
		return mu * (ReferencePressure_SeaLevel / P);
	}

	/**
	* Returns the heat capcity ratio of air at a given temperature.
	* @param T			Temperature (Celsius)
	*/
	double GetHeatCapacityOfAir(const double T);

	/**
	* Calculates the speed of sound in air at a given temperature.
	* @param T			Temperature (Celsius)
	*/
	double CalculateSpeedOfSoundInAir(const double T);

	/**
	* Calculates the stress (Pa) caused by a projectile impact.
	* Passing a value of 0 for either the ImpactArea or ObjectThickness will result in a division by zero.
	* @param ProjectileMass		Mass of the projectile (kg)
	* @param ImpactSpeed		Speed at the time of impact (m/s)
	* @param ImpactArea			Contact area at the point of impact (m^2)
	* @param ImpactAngle		Angle between the velocity vector of the projectile and the surface normal of the impacted object (radians)
	* @param ObjectThickness	Thickness of the object in the direction perpendicular to the impact (m)
	* @return					Stress (Pa)
	*/
	double CalculateImpactStress(const double ProjectileMass, const double ImpactSpeed, const double ImpactArea, const double ImpactAngle, const double ObjectThickness);

	/**
	* Calculates the stress (Pa) caused by a projectile impact.
	* Passing a value of 0 for either the ImpactArea or ObjectThickness will result in a division by zero.
	* @param KineticEnergy		Kinetic energy of the projectile (J)
	* @param ImpactArea			Contact area at the point of impact (m^2)
	* @param ImpactAngle		Angle between the velocity vector of the projectile and the surface normal of the impacted object (radians)
	* @param ObjectThickness	Thickness of the object in the direction perpendicular to the impact (m)
	* @return					Stress (Pa)
	*/
	double CalculateImpactStress(const double KineticEnergy, const double ImpactArea, const double ImpactAngle, const double ObjectThickness);

	/**
	* Calculates the stress (Pa) caused by a projectile impact.
	* Passing a value of 0 for either the ImpactArea or ObjectThickness will result in a division by zero.
	* @param KineticEnergy		Kinetic energy of the projectile (J)
	* @param ImpactArea			Contact area at the point of impact (m^2)
	* @param ImpactAngle		Angle between the velocity vector of the projectile and the surface normal of the impacted object (radians)
	* @param ObjectThickness	Thickness of the object in the direction perpendicular to the impact (m)
	* @param ObjectE			Young's modulus of the object (Pa)
	* @param ProjectileE		Young's modulus of the projectile (Pa)
	* @param ObjectKic			Fracture toughness of the object (Pa/m^0.5)
	* @return					Stress (Pa)
	*/
	constexpr double CalculateImpactStress(const double KineticEnergy, const double ImpactArea, const double ImpactAngle, const double ObjectThickness, const double ObjectE, const double ProjectileE, const double ObjectKic)
	{
		double CosAngle = gcem::cos(ImpactAngle);
		if (CosAngle == 0.0) // This would result in a division by zero, but we can just adjust the angle slightly to avoid that.
		{
			CosAngle = 1e-6;
		}

		const double Denominator = ((ObjectE + ProjectileE) * ObjectThickness * gcem::sqrt(DOUBLE_PI * ImpactArea * CosAngle) / 2);

		return (KineticEnergy * ObjectKic) / Denominator;
	}

	double CalculateImpactStress(const double KineticEnergy, const double ImpactArea, const double ImpactAngle, const double ObjectThickness, const FPhysMatProperties& ObjectProperties, const FPhysMatProperties& ProjectileProperties);

	FVector CalculateCoriolisAcceleration(const FVector& Velocity, const double Latitude, const double Azimuth);

	namespace Drag
	{
		namespace DragModels
		{
			constexpr ETBGModel G1 = ETBGModel::G1;
			constexpr ETBGModel G7 = ETBGModel::G7;
			constexpr ETBGModel NA = ETBGModel::NA;

			namespace Private
			{
				template<ETBGModel Model>
				struct GModelCurveParams
				{
				};

				template<> struct GModelCurveParams<G1>
				{
				protected:
					inline static constexpr const double TransitionSpeed = 1.198;

					// Subsonic

					inline static constexpr double a1 = -0.637957;
					inline static constexpr double b1 = 8.02114;
					inline static constexpr double c1 = 6.62363;
					inline static constexpr double d1 = 0.887957;
					inline static constexpr double n1 = -0.75;

					static constexpr long double GetSubsonic(const double Speed)
					{
						const long double exp_1 = a1 * gcem::pow(Speed, b1);
						const long double exp_2 = gcem::pow(Speed, c1);
						const long double exp_3 = gcem::pow(d1, Speed);
						return n1 + exp_1 + exp_2 + exp_3;
					}


					// Supersonic

					inline static constexpr double a2 = 151.103604911;
					inline static constexpr double b2 = 1313.50068422;
					inline static constexpr double c2 = 0.497675503145;
					inline static constexpr double d2 = 8.46571089406;
					inline static constexpr double n2 = -0.199438891387;

					static constexpr long double GetSupersonic(const double Speed)
					{
						const long double denominator = b2 + gcem::pow(Speed, d2);
						return c2 + (a2 / denominator) - n2 * gcem::pow(Speed, -4.0L);
					}

				public:
					static constexpr long double GetDragCoefficient(const double Speed)
					{
						const double asMach = gcem::abs(Speed) / Constants::Mach1;
						if (asMach < TransitionSpeed)
						{
							return GetSubsonic(asMach);
						}
						else
						{
							return GetSupersonic(asMach);
						}
					}
				};
				template<> struct GModelCurveParams<G7>
				{
				protected:
					inline static constexpr const double TransitionSpeed = 1.018;

					// Supersonic

					inline static constexpr long double a1 = 0.217877;
					inline static constexpr long double b1 = 0.975364;
					inline static constexpr long double c1 = -0.747357;
					inline static constexpr long double n1 = 1.18208;

					static constexpr long double GetSupersonic(const double Speed)
					{
						const long double exp_1 = gcem::pow(Speed, n1);
						const long double exp_2 = gcem::pow(b1, Speed);
						return (a1 / exp_1) + exp_2 + c1;
					}

					// Subsonic

					inline static constexpr long double a2 = 2.22098;
					inline static constexpr long double b2 = -1.0134e-11;
					inline static constexpr long double c2 = 1.908e11;
					inline static constexpr long double d2 = 0.125;
					inline static constexpr long double n2 = 23.8977;

					static constexpr long double GetSubsonic(const double Speed)
					{
						const long double exp_1 = a2 * gcem::pow(Speed, n2);
						const long double exp_2 = b2 * gcem::pow(c2, Speed);
						return exp_1 + exp_2 + d2;
					}

				public:
					static constexpr long double GetDragCoefficient(const double Speed)
					{
						const double asMach = gcem::abs(Speed) / Constants::Mach1;
						if (asMach < TransitionSpeed)
						{
							return GetSubsonic(asMach);
						}
						else
						{
							return GetSupersonic(asMach);
						}
					}
				};

				template<> struct GModelCurveParams<NA>
				{
				protected:
					inline static constexpr double ScalingFactor = 0.0600859;

					// Region 1

					inline static constexpr long double a1 = -2.50006166693;
					inline static constexpr long double b1 = 1.00721623437L;
					inline static constexpr long double c1 = 0.79600185584L;
					inline static constexpr long double d1 = 3.47403874616L;
					inline static constexpr long double f1 = -0.146607907256L;
					inline static constexpr long double g1 = 2.67441459061L;

					static constexpr long double GetRegion1(const long double Re)
					{
						const auto exp_1 = a1 * gcem::pow(Re, b1);
						const auto exp_2 = gcem::pow(d1, Re);
						const auto exp_3 = f1 * gcem::pow(g1, 2.0L * Re);
						return exp_1 + exp_2 + exp_3 + c1;
					}

					// Region 2

					inline static constexpr long double Region2Start = 1.562;

					inline static constexpr long double a2 = -3.5548304213e-8L;
					inline static constexpr long double b2 = 21.4299726796L;
					inline static constexpr long double c2 = 0.705502803231L;
					inline static constexpr long double d2 = -3.4696050421e-54L;
					inline static constexpr long double f2 = 5.9270619937e25L;

					static constexpr long double GetRegion2(const long double Re)
					{
						const auto exp_1 = a2 * gcem::pow(Re, b2);
						const auto exp_2 = d2 * gcem::pow(f2, Re);
						return exp_1 + exp_2 + c2;
					}

					// Region 3

					inline static constexpr long double Region3Start = 2.028;

					inline static constexpr long double a3 = 4.81506451168e7L;
					inline static constexpr long double b3 = -10.628048444L;
					inline static constexpr long double c3 = 0.632906908083L;
					inline static constexpr long double d3 = 1.2308623e67L;
					inline static constexpr long double f3 = 3.4397202946e-34L;
					inline static constexpr long double g3 = -4.81415953863e7L;
					inline static constexpr long double h3 = -10.6277666902L;

					static constexpr long double GetRegion3(const long double Re)
					{
						const auto exp_1 = a3 * gcem::pow(Re, b3);
						const auto exp_2 = d3 * gcem::pow(f3, Re);
						const auto exp_3 = g3 * gcem::pow(Re, h3);
						return exp_1 + exp_2 + exp_3 + c3;
					}

				public:
					static constexpr long double GetDragCoefficient(const long double ReynoldsNumber)
					{
						const long double CorrectionFactor = 1.1 * gcem::pow(ReynoldsNumber, -ScalingFactor / 2.0L);
						const long double Re = gcem::abs(gcem::pow(ReynoldsNumber, ScalingFactor));
						if (Re < Region2Start)
						{
							return CorrectionFactor * GetRegion1(Re);
						}
						else if (Re < Region3Start)
						{
							return CorrectionFactor * GetRegion2(Re);
						}
						else
						{
							return CorrectionFactor * GetRegion3(Re);
						}
					}
				};
			};
		};
		
		/**
		* Calculates the force required to overcome drag
		* Wind speed is not relative. For the wind to oppose the motion of the object, wind speed would have to be negative.
		* @param cd			Coefficient of drag for the object.
		* @param csa		Cross sectional area of the object. (m^2)
		* @param speed		Speed of the object. (m/s)
		* @param windSpeed	Wind speed. (m/s)
		* @param p			Fluid density of the medium the object is passing through. Defaults to air.
		*/
		constexpr double CalculateDragForce(const double cd, const double csa, const double speed, const double windSpeed = 0, double p = -1)
		{
			UE_ASSUME(cd > 0.0 && csa > 0.0 && p != 0.0);
			if (p == -1)
			{
				p = TB::Constants::FluidDensity_Air;
			}
			/*
			*	Pd	= Fd * v
			*		= 1/2 * p * a * c * v(vw + v)^2
			*
			*	Fd	= 1/2 * p * a * c * (vw + v)^2
			*/
			return 0.5 * TB_SQR(speed + windSpeed) * p * csa * cd;
		}
		/**
		* Calculates the force required to overcome drag in 3D
		* @param cd				Coefficent of drag for the object on each axis.
		* @param csa			Cross sectional area of the object for each axis. (m^2)
		* @param velocity		Velocity of the object. (m/s)
		* @param orientation	Rotation of the object.
		* @param p				Fluid density of the medium the object is passing through. Defaults to air.
		*/
		FVector CalculateDragForce(const FVector& cd, const FVector& csa, const FVector& velocity, const FQuat& orientation = FQuat::Identity, double p = -1);
		/**
		* Calculates the force required to overcome drag in 3D with wind
		* Wind velocity is not relative, so a wind velocity of {1, 0, 0} would act as a tailwind for a projectile moving in the +X direction.
		* @param cd				Coefficent of drag for the object on each axis.
		* @param csa			Cross sectional area of the object for each axis. (m^2)
		* @param velocity		Velocity of the object. (m/s)
		* @param windVelocity	Wind velocity. (m/s)
		* @param orientation	Rotation of the object.
		* @param p				Fluid density of the medium the object is passing through. Defaults to air.
		*/
		FVector CalculateDragForce(const FVector& cd, const FVector& csa, const FVector& velocity, const FVector& windVelocity, const FQuat& orientation = FQuat::Identity, double p = -1);
		
		FVector CalculateDragForce(const FTBProjectilePhysicalProperties& projectile, const FVector& velocity, const FVector& windVelocity, const FQuat& orientation, double p = -1);

		double CalculateDragForce(const FTBProjectilePhysicalProperties& projectile, const double speed, const double windSpeed = 0, double p = -1);

		FVector CalculateDragMoment(const FTBProjectilePhysicalProperties& projectile, const FVector& dragPosition, const FVector& velocity, const FQuat& orientation, const double WettedArea, const double ConicalHalfAngle, double p = -1);

		/**
		* Calculates the drag coefficient of an ideal G Model projectile based on its velocity.
		* @param GModel         Which G Model to use.
		* @param Speed          Speed of the projectile. (m/s)
		*/ 
		constexpr double GetDragCoefficient(const ETBGModel GModel, const double Speed, const double ReynoldsNumber = 0.0)
		{
			using namespace TB::Drag::DragModels;
			if (GModel == NA && ReynoldsNumber <= 0.0)
			{
				return 0.47;
			}

			switch (GModel)
			{
			case ETBGModel::G7:
				return Private::GModelCurveParams<G7>::GetDragCoefficient(Speed);
			case ETBGModel::NA:
				return Private::GModelCurveParams<NA>::GetDragCoefficient(ReynoldsNumber);
			case ETBGModel::None:
				[[fallthrough]];
			case ETBGModel::G1:
				[[fallthrough]];
			default:
				return Private::GModelCurveParams<G1>::GetDragCoefficient(Speed);
			}
		}
	};
};
