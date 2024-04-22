// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TBEnums.generated.h"



UENUM(BlueprintType, DisplayName = "Projectile Simulation Type")
enum class ETBProjectileSimulationType : uint8
{
	Default =	0,
	Instant =	1 << 0,
	Realtime =	1 << 1
};
ENUM_CLASS_FLAGS(ETBProjectileSimulationType);

UENUM(BlueprintType, DisplayName = "Penetration Complexity")
enum class ETBPenetrationComplexity : uint8
{
	Simplified	UMETA(ToolTip = "Unimplemented"),
	Complex		UMETA(ToolTip = "Uses the physical properties of the projectile and the impacted object to calculate penetration.")
};

UENUM(BlueprintType, DisplayName = "Ballistics Debug Type", meta = (BitFlags, UseEnumValuesAsMaskValuesInEditor = true))
enum class ETBBallisticsDebugType : uint8
{
	None = 0 UMETA(Hidden),
	DrawDebugTrace = 1 << 1,
	PrintDebugInfo = 1 << 2
};
ENUM_CLASS_FLAGS(ETBBallisticsDebugType);

UENUM(BlueprintType, DisplayName = "Projectile Network Mode")
enum class ETBSimTaskNetMode : uint8
{
	None = 0 UMETA(Hidden),
	Client = 1 << 0,
	Server = 1 << 1,
	Both = Client | Server 
};
ENUM_CLASS_FLAGS(ETBSimTaskNetMode);

UENUM(BlueprintType, DisplayName = "Exit Code Log Level")
enum class EExitCodeLogLevel : uint8
{
	None = 0,
	Error = 1<<0,
	Verbose = 1<<1
};
FORCEINLINE bool operator==(const uint8& lhs, const EExitCodeLogLevel& rhs)
{
	return lhs == (uint8)rhs;
}
ENUM_CLASS_FLAGS(EExitCodeLogLevel);

UENUM(BlueprintType, DisplayName = "Exit Code Log Filter")
enum class EExitCodeLogFilter : uint8
{
	BulletTasks = 0,
	ProjectileTasks = 1<<0,
	Both = 1<<1
};
FORCEINLINE bool operator==(const uint8& lhs, const EExitCodeLogFilter& rhs)
{
	return lhs == static_cast<uint8>(rhs);
}
ENUM_CLASS_FLAGS(EExitCodeLogFilter);

UENUM(BlueprintType, DisplayName = "Drag Complexity")
enum class ETBDragComplexity : uint8
{
	DEFAULT,
	BASIC				UMETA(DisplayName = "Basic"),
	ATMOSPHERIC			UMETA(DisplayName = "Altitude based"),
	WIND				UMETA(DisplayName = "Wind"),
	WIND3D				UMETA(DisplayName = "3D Wind"),
	COMPLEX				UMETA(DisplayName = "Complex")
};
constexpr bool DragComplexityRequiresPressure(const ETBDragComplexity Complexity)
{
	return Complexity == ETBDragComplexity::ATMOSPHERIC || Complexity == ETBDragComplexity::COMPLEX;
}

UENUM(BlueprintType, DisplayName = "Drag Model")
enum class ETBGModel : uint8
{
	None				UMETA(Hidden),
	G1,
	G7,
	NA					UMETA(DisplayName = "N/A")
};

UENUM(BlueprintType, DisplayName = "Bullet Variation") //Bullet Variations
enum class ETBBulletVariation : uint8
{
	AP					UMETA(DisplayName = "Armor Piercing"),
	SUBSONIC			UMETA(DisplayName = "Subsonic"),
	FMJ					UMETA(DisplayName = "Full Metal Jacket"),
	HOLLOW				UMETA(DisplayName = "Hollow Point"),
	TRACER				UMETA(DisplayName = "Tracer"),
	INCEND				UMETA(DisplayName = "Incendiary"),
	FRANGIBLE			UMETA(DisplayName = "Frangible"),
	BUCKSHOT			UMETA(DisplayName = "Buckshot")
};
ENUM_CLASS_FLAGS(ETBBulletVariation);
constexpr ETBBulletVariation AP_BulletVariation = ETBBulletVariation::AP;
constexpr ETBBulletVariation Subsoncic_BulletVariation = ETBBulletVariation::SUBSONIC;
constexpr ETBBulletVariation FMJ_BulletVariation = ETBBulletVariation::FMJ;
constexpr ETBBulletVariation HollowPoint_BulletVariation = ETBBulletVariation::HOLLOW;
constexpr ETBBulletVariation Tracer_BulletVariation = ETBBulletVariation::TRACER;
constexpr ETBBulletVariation Incendiary_BulletVariation = ETBBulletVariation::INCEND;
constexpr ETBBulletVariation Frangible_BulletVariation = ETBBulletVariation::FRANGIBLE;
constexpr ETBBulletVariation Buckshot_BulletVariation = ETBBulletVariation::BUCKSHOT;

UENUM(BlueprintType, DisplayName = "Projectile Size")
enum class ETBProjectileSize : uint8
{
	None,
	Small			UMETA(DisplayName = "Small"),
	Medium			UMETA(DisplayName = "Medium"),
	Large			UMETA(DisplayName = "Large")
};
constexpr ETBProjectileSize PS_Small = ETBProjectileSize::Small;
constexpr ETBProjectileSize PS_Medium = ETBProjectileSize::Medium;
constexpr ETBProjectileSize PS_Large = ETBProjectileSize::Large;

UENUM(BlueprintType, DisplayName = "Bullet Caliber")
enum class ETBBulletCaliber : uint8
{

	Bullet_9mm			= 0		UMETA(DisplayName = "9x19mm"),
	Bullet_556x45		= 1		UMETA(DisplayName = "5.56x45 NATO"),
	Bullet_762x51		= 2		UMETA(DisplayName = "7.62x51 NATO"),
	Bullet_762x54		= 3		UMETA(DisplayName = "7.62x54mmr"),
	Bullet_762x39		= 4		UMETA(DisplayName = "7.62x39 NATO"),
	Bullet_545x39		= 5		UMETA(DisplayName = "5.45x39"),
	Bullet_50			= 6		UMETA(DisplayName = ".50 BMG"),
	Bullet_308			= 7		UMETA(DisplayName = ".308"),
	Bullet_338			= 8		UMETA(DisplayName = ".338"),
	Bullet_46x30		= 9		UMETA(DisplayName = "4.6x30mm"),
	Bullet_57x28		= 10	UMETA(DisplayName = "5.7x28"),
	Bullet_22LR			= 11	UMETA(DisplayName = ".22LR"),
	Bullet_65			= 12	UMETA(DisplayName = "6.5mm Creedmoor"),
	Bullet_9x18			= 13	UMETA(DisplayName = "9x18mm"),
	Bullet_300AAC		= 14	UMETA(DisplayName = ".300 AAC Blackout"),
	Bullet_300SVG		= 15	UMETA(DisplayName = ".300 Savage"),
	Bullet_300WHSP		= 16	UMETA(DisplayName = ".300 Whisper"),
	Bullet_300WINMAG	= 17	UMETA(DisplayName = ".300 Winchester Magnum"),
	Bullet_408			= 18	UMETA(DisplayName = ".408 Cheyenne Tactical"),
	Bullet_416			= 19	UMETA(DisplayName = ".416 Barrett"),
	Bullet_10mm			= 20	UMETA(DisplayName = "10mm Auto"),
	Bullet_40			= 21	UMETA(DisplayName = ".40 S&W"),
	Bullet_44Mag		= 22	UMETA(DisplayName = ".44 Magnum"),
	Bullet_44SW			= 23	UMETA(DisplayName = ".44 S&W"),
	Bullet_444			= 24	UMETA(DisplayName = ".444 Marlin"),
	Bullet_45ACP		= 25	UMETA(DisplayName = ".45 ACP"),
	Bullet_45WINMAG		= 26	UMETA(DisplayName = ".45 Winchester Magnum"),
	Bullet_50BEOWULF	= 27	UMETA(DisplayName = ".50 Beowulf"),
	Bullet_50AE			= 29	UMETA(DisplayName = ".50 Action Express"),
	Bullet_127x108		= 30	UMETA(DisplayName = "12.7x108mm"),
	Bullet_145x114		= 31	UMETA(DisplayName = "14.5x114mm"),
	Bullet_38SP			= 32	UMETA(DisplayName = ".38 Special"),
	Bullet_38ACP		= 33	UMETA(DisplayName = ".38 ACP"),
	Bullet_380			= 34	UMETA(DisplayName = ".380 ACP"),
	Bullet_357			= 35	UMETA(DisplayName = ".357 Magnum"),
	Bullet_357SIG		= 36	UMETA(DisplayName = ".357 SIG"),
	Bullet_12Gauge00	= 37	UMETA(DisplayName = "12 Gauge 00 Buckshot"),
	None,
	Max							UMETA(Hidden)

	// Preserve order/enumeration values to prevent blueprints from being messed up
	// When new bullets are actually defined, ensure they are accounted for in BulletUtils
};
ENUM_CLASS_FLAGS(ETBBulletCaliber);
constexpr ETBBulletCaliber BulletCaliber_Default = ETBBulletCaliber::Bullet_9mm;
constexpr ETBBulletCaliber BulletCaliber_Max = ETBBulletCaliber::Max;
constexpr bool IsBulletCaliberPreset(const ETBBulletCaliber BulletCaliber) // Update if preset calibers are changed
{
	return BulletCaliber <= ETBBulletCaliber::Bullet_357SIG;
}

UENUM(BlueprintType, DisplayName = "Pressure Unit")
enum class ETBPressureUnit : uint8
{
	UNIT_PA = 0			UMETA(DisplayName = "Pa"),
	UNIT_KPA = 1		UMETA(DisplayName = "KPa"),
	UNIT_MPA = 2		UMETA(DisplayName = "MPa"),
	UNIT_GPA = 3		UMETA(DisplayName = "GPa"),
	UNIT_PSI = 10		UMETA(DisplayName = "psi"),
	UNIT_ATM = 11		UMETA(DisplayName = "Atm")

	// For the metric units, if n is the underlying value of the enum value, the formula p * 10^3n will convert a pressure "p" into pascals.
	/*
		ex:
		double NewPressureInPascals = OldPressure * pow(10.0, 3.0 * (uint8)OldPressureUnit);
	*/
};
