// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "PhysMatManager/PhysMat.h"
#include "Types/TBBulletPhysicalProperties.h"
#include "Types/TBEnums.h"
#include "Types/TBSpreadParams.h"
#include "Bullets.generated.h"



class UBulletDataAsset;
class UNiagaraSystem;
class UStaticMesh;
class UTexture2D;

/**
* Main struct used to describe a bullet.
*
* Includes physical characteristics as well as additional properties such as the name, mesh, and casing system.
* All Meshes and Particle Systems are optional and only there for convenience.
*
* @see FTBBulletPhysicalProperties
* @see UBulletDataAsset
*/
USTRUCT()
struct TERMINALBALLISTICS_API FTBBullet
{
	GENERATED_BODY();
public:
	UPROPERTY()
	FTBBulletPhysicalProperties BulletProperties;
	UPROPERTY()
	FPhysMatProperties PhysicalProperties;
	UPROPERTY()
	FName BulletName;
	UPROPERTY()
	ETBBulletCaliber BulletType = ETBBulletCaliber::Bullet_9mm;
	UPROPERTY()
	ETBProjectileSize ProjectileSize = ETBProjectileSize::Small;
	UPROPERTY()
	TArray<ETBBulletVariation> BulletVariation;
	UPROPERTY()
	float MuzzleVelocity = 350.f;
	UPROPERTY()
	int32 ProjectileCount = 1;

	UPROPERTY()
	FTBSpreadParams SpreadParams;


	UPROPERTY()
	TSoftObjectPtr<UTexture2D> Icon = nullptr;
	UPROPERTY()
	TSoftObjectPtr<UStaticMesh> Mesh = nullptr;
	UPROPERTY()
	TSoftObjectPtr<UStaticMesh> BulletMesh = nullptr;
	UPROPERTY()
	TSoftObjectPtr<UStaticMesh> CartridgeMesh = nullptr;

	UPROPERTY()
	TSoftObjectPtr<UNiagaraSystem> BulletCasing = nullptr;
	UPROPERTY()
	TSoftObjectPtr<UNiagaraSystem> BulletSystem = nullptr;
	UPROPERTY()
	TSoftObjectPtr<UNiagaraSystem> TracerSystem = nullptr;
	UPROPERTY()
	TSoftObjectPtr<UNiagaraSystem> DefaultTracerSystem = nullptr;
	
	FTBBullet() = default;

	FTBBullet(const FTBBulletPhysicalProperties& Properties, const FPhysMatProperties& PhysMatProperties, const FName& Name, const ETBBulletCaliber Type, const ETBProjectileSize Size, const float MuzzleVelocity, const TArray<ETBBulletVariation> Variation = {}, const int32 ProjectileCount = 1, const FTBSpreadParams& SpreadParams = FTBSpreadParams())
		: BulletProperties(Properties)
		, PhysicalProperties(PhysMatProperties)
		, BulletName(Name)
		, BulletType(Type)
		, ProjectileSize(Size)
		, BulletVariation(Variation)
		, MuzzleVelocity(MuzzleVelocity)
		, ProjectileCount(ProjectileCount)
		, SpreadParams(SpreadParams)
	{}

	FTBBullet(const FTBBulletPhysicalProperties& Properties, const FPhysMatProperties& PhysMatProperties, const FName& Name, const ETBBulletCaliber Type, const ETBProjectileSize Size, const float MuzzleVelocity, const ETBBulletVariation Variation, const int32 ProjectileCount = 1, const FTBSpreadParams& SpreadParams = FTBSpreadParams())
		: BulletProperties(Properties)
		, PhysicalProperties(PhysMatProperties)
		, BulletName(Name)
		, BulletType(Type)
		, ProjectileSize(Size)
		, BulletVariation({ Variation })
		, MuzzleVelocity(MuzzleVelocity)
		, ProjectileCount(ProjectileCount)
		, SpreadParams(SpreadParams)
	{}

	explicit FTBBullet(TSoftClassPtr<UBulletDataAsset> DataAsset);
	explicit FTBBullet(const UBulletDataAsset* DataAsset);

	FORCEINLINE bool IsValid() const
	{
		return BulletProperties.CheckValid()
			&& PhysicalProperties.CheckValid();
	}

	FORCEINLINE operator bool() const
	{
		return IsValid();
	}

	bool CanUseTracer() const;

	bool CanUseDefaultTracer() const;

	UNiagaraSystem* GetTracerSystem() const;

	[[nodiscard]] static FTBBullet* CreateFromBulletDataAsset(TSoftClassPtr<UBulletDataAsset> DataAsset);
	[[nodiscard]] static FTBBullet* CreateFromBulletDataAsset(const UBulletDataAsset* DataAsset);
	[[nodiscard]] static TSharedPtr<FTBBullet> CreatePtrFromBulletDataAsset(TSoftClassPtr<UBulletDataAsset> DataAsset);
	[[nodiscard]] static TSharedPtr<FTBBullet> CreatePtrFromBulletDataAsset(const UBulletDataAsset* DataAsset);

	[[nodiscard]] static FTBBullet* GetDefaultBullet();
	[[nodiscard]] static TSharedPtr<FTBBullet> GetDefaultBulletPtr();

	static FTBBullet DefaultBullet;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

	operator FTBBulletPhysicalProperties() const
	{
		return BulletProperties;
	}

	bool operator==(const FTBBullet& Other) const
	{
		return BulletProperties == Other.BulletProperties
			&& PhysicalProperties == Other.PhysicalProperties
			&& BulletName == Other.BulletName
			&& BulletType == Other.BulletType
			&& ProjectileSize == Other.ProjectileSize
			&& BulletVariation == Other.BulletVariation
			&& FMath::IsNearlyEqual(MuzzleVelocity, Other.MuzzleVelocity)
			&& ProjectileCount == Other.ProjectileCount
			&& Icon == Other.Icon
			&& Mesh == Other.Mesh
			&& BulletMesh == Other.BulletMesh
			&& CartridgeMesh == Other.CartridgeMesh
			&& BulletCasing == Other.BulletCasing
			&& BulletSystem == Other.BulletSystem
			&& TracerSystem == Other.TracerSystem
			&& DefaultTracerSystem == Other.DefaultTracerSystem;
	}
};
template<> struct TStructOpsTypeTraits<FTBBullet> : public TStructOpsTypeTraitsBase2<FTBBullet>
{
	enum
	{
		WithNetSerializer = true
	};
};
typedef TSharedPtr<FTBBullet> BulletPointer;
typedef TUniquePtr<FTBBullet> BulletPointerUnique;


namespace TB::Bullets
{
	namespace BulletNames
	{
		extern TERMINALBALLISTICS_API const FName BulletName9mm;
		extern TERMINALBALLISTICS_API const FName BulletName556x45;
		extern TERMINALBALLISTICS_API const FName BulletName556x45_M855;
		extern TERMINALBALLISTICS_API const FName BulletName556x45_M856;
		extern TERMINALBALLISTICS_API const FName BulletName762x51;
		extern TERMINALBALLISTICS_API const FName BulletName762x51LR;
		extern TERMINALBALLISTICS_API const FName BulletName762x54;
		extern TERMINALBALLISTICS_API const FName BulletName762x39;
		extern TERMINALBALLISTICS_API const FName BulletName545x39;
		extern TERMINALBALLISTICS_API const FName BulletName545x39AP;
		extern TERMINALBALLISTICS_API const FName BulletName50;
		extern TERMINALBALLISTICS_API const FName BulletName308;
		extern TERMINALBALLISTICS_API const FName BulletName308FMJ;
		extern TERMINALBALLISTICS_API const FName BulletName338;
		extern TERMINALBALLISTICS_API const FName BulletName46x30;
		extern TERMINALBALLISTICS_API const FName BulletName57x28;
		extern TERMINALBALLISTICS_API const FName BulletName22;
		extern TERMINALBALLISTICS_API const FName BulletName65;
		extern TERMINALBALLISTICS_API const FName BulletName9x18;
		extern TERMINALBALLISTICS_API const FName BulletName300Blackout;
		extern TERMINALBALLISTICS_API const FName BulletName300BlackoutSubsonic;
		extern TERMINALBALLISTICS_API const FName BulletName300Savage;
		extern TERMINALBALLISTICS_API const FName BulletName300Whisper;
		extern TERMINALBALLISTICS_API const FName BulletName300WinMag;
		extern TERMINALBALLISTICS_API const FName BulletName380ACP;
		extern TERMINALBALLISTICS_API const FName BulletName408CheyTac;
		extern TERMINALBALLISTICS_API const FName BulletName416Barrett;
		extern TERMINALBALLISTICS_API const FName BulletName45ACP;
		extern TERMINALBALLISTICS_API const FName BulletName45WinMag;
		extern TERMINALBALLISTICS_API const FName BulletName50Beowulf;
		extern TERMINALBALLISTICS_API const FName BulletName127x108;
		extern TERMINALBALLISTICS_API const FName BulletName145x114;

		extern TERMINALBALLISTICS_API const FName BulletName12Gauge00Buckshot;
	};

	namespace BulletOgives
	{
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_9x19PARA;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_556x45NATO;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_762x51NATO;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_762x51NATOLR;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_762x39;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_762x54;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_50BMG;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_545x39;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_545x39AP;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_57x28;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_300Blackout;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_308;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_308FMJ;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_338;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_46x30;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_9x18;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_45ACP;
		extern TERMINALBALLISTICS_API const FTBOgive Ogive_380ACP;
	};

	namespace BulletPhysicalProperties
	{
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_9x19PARA;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_556x45NATO;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_556x45_M855;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_556x45_M856;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_762x51NATO;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_762x51NATOLR;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_762x39;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_762x54;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_50BMG;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_545x39;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_545x39AP;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_57x28;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_300Blackout;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_300Subsonic;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_308;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_308FMJ;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_338;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_46x30;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_9x18;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_45ACP;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_380ACP;
		extern TERMINALBALLISTICS_API const FTBBulletPhysicalProperties BULLET_12Gauge00Buck;
	};

	struct TERMINALBALLISTICS_API BulletTypes
	{
	public:
		BulletTypes() = default;

		static void Init();

		static const BulletTypes& Get() { return StaticBulletTypes; }

		FTBBullet Bullet_9x19Para;
		FTBBullet Bullet_556x45NATO;
		FTBBullet Bullet_556x45_M855;
		FTBBullet Bullet_556x45_M856;
		FTBBullet Bullet_762x51NATO;
		FTBBullet Bullet_762x51NATOLR;
		FTBBullet Bullet_762x39;
		FTBBullet Bullet_762x54;
		FTBBullet Bullet_50BMG;
		FTBBullet Bullet_545x39;
		FTBBullet Bullet_545x39AP;
		FTBBullet Bullet_308;
		FTBBullet Bullet_308FMJ;
		FTBBullet Bullet_338;
		FTBBullet Bullet_46x30;
		FTBBullet Bullet_57x28;
		FTBBullet Bullet_22LR;
		FTBBullet Bullet_65;
		FTBBullet Bullet_9x18;
		FTBBullet Bullet_300AAC;
		FTBBullet Bullet_300AACSub;
		FTBBullet Bullet_300SVG;
		FTBBullet Bullet_300WHSP;
		FTBBullet Bullet_300WinMag;
		FTBBullet Bullet_380ACP;
		FTBBullet Bullet_408;
		FTBBullet Bullet_416;
		FTBBullet Bullet_45ACP;
		FTBBullet Bullet_45WinMag;
		FTBBullet Bullet_50Beowulf;
		FTBBullet Bullet_127x108;
		FTBBullet Bullet_145x114;

		FTBBullet Bullet_12Gauge00Buckshot;

		TMap<ETBBulletNames, FTBBullet> GetMappings() const
		{
			return BulletMappings;
		}
	private:
		TMap<ETBBulletNames, FTBBullet> BulletMappings;

		static BulletTypes StaticBulletTypes;
	};

};

static const FName GetDefaultBulletName()
{
	return FTBBullet::DefaultBullet.BulletName;
}


// Names of preset bullets
UENUM(BlueprintType)
enum class ETBBulletNames : uint8
{
	None				= 0		UMETA(Hidden),
	Name_9mm			= 1		UMETA(DisplayName = "9x19mm"),
	Name_556x45			= 2		UMETA(DisplayName = "5.56x45 NATO"),
	Name_762x51			= 3		UMETA(DisplayName = "7.62x51 NATO"),
	Name_762x51LR		= 4		UMETA(DisplayName = "7.62x51 NATO LR"),
	Name_762x54			= 5		UMETA(DisplayName = "7.62x54mmr"),
	Name_762x39			= 6		UMETA(DisplayName = "7.62x39 NATO"),
	Name_545x39			= 7		UMETA(DisplayName = "5.45x39"),
	Name_545x39AP		= 8		UMETA(DisplayName = "5.45x39 AP"),
	Name_50				= 9		UMETA(DisplayName = ".50 BMG"),
	Name_308			= 10	UMETA(DisplayName = ".308"),
	Name_308FMJ			= 11	UMETA(DisplayName = ".308 FMJ"),
	Name_338			= 12	UMETA(DisplayName = ".338"),
	Name_46x30			= 13	UMETA(DisplayName = "4.6x30mm"),
	Name_57x28			= 14	UMETA(DisplayName = "5.7x28"),
	Name_22LR			= 15	UMETA(DisplayName = ".22LR"),
	Name_65				= 16	UMETA(DisplayName = "6.5mm Creedmoor"),
	Name_9x18			= 17	UMETA(DisplayName = "9x18mm PM"),
	Name_300AAC			= 18	UMETA(DisplayName = ".300 AAC Blackout"),
	Name_300SVG			= 19	UMETA(DisplayName = ".300 Savage"),
	Name_300WHSP		= 20	UMETA(DisplayName = ".300 Whisper"),
	Name_300WINMAG		= 21	UMETA(DisplayName = ".300 Winchester Magnum"),
	Name_408			= 22	UMETA(DisplayName = ".408 Cheyenne Tactical"),
	Name_416			= 23	UMETA(DisplayName = ".416 Barrett"),
	Name_45ACP			= 24	UMETA(DisplayName = ".45 ACP"),
	Name_45WINMAG		= 25	UMETA(DisplayName = ".45 Winchester Magnum"),
	Name_50BEOWULF		= 26	UMETA(DisplayName = ".50 Beowulf"),
	Name_127x108		= 27	UMETA(DisplayName = "12.7x108mm"),
	Name_145x114		= 28	UMETA(DisplayName = "14.5x114mm"),

	Name_556x45_M855	= 29	UMETA(DisplayName = "5.56x45 M855 Ball"),
	Name_556x45_M856	= 30	UMETA(DisplayName = "5.56x45 NATO Tracer (Red)"),
	Name_300AAC_Subsonic= 31	UMETA(DisplayName = ".300 AAC Blackout (Subsonic)"),

	Name_380ACP			= 35	UMETA(DisplayName = ".380 ACP"),

	Name_12Gauge00Buck	= 37	UMETA(DisplayName = "12 Gauge 00 Buckshot"),


	Default = Name_9mm			UMETA(Hidden)
};
