// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Types/TBEnums.h"
#include "TerminalBallisticsMacrosAndFunctions.h"
#include "TerminalBallisticsTraits.h"
#include "Types/TBBulletInfo.h"
#include "Types/TBBulletPhysicalProperties.h"
#include "Types/TBProjectile.h"
#include "Types/TBProjectileId.h"
#include "Types/TBProjectileInjury.h"
#include "ImpactParams.generated.h"



struct FTBBullet;
class UBulletDataAsset;

USTRUCT(BlueprintInternalUseOnly)
struct alignas(16) TERMINALBALLISTICS_API FTBImpact
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impact)
	FHitResult HitResult;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impact)
	FVector ImpactVelocity = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impact, meta = (DisplayAfter = "bRicochet"))
	FVector RicochetVector = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impact)
	FVector StartLocation = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impact)
	ETBProjectileSize ProjectileSize = PS_Small;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impact)
	TEnumAsByte<EPhysicalSurface> SurfaceType = EPhysicalSurface::SurfaceType_Default;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impact)
	bool bRicochet = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impact)
	bool bIsPenetrating = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impact)
	double PenetrationDepth = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impact)
	double dV = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impact)
	FTBProjectileId ProjectileId = FTBProjectileId::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impact)
	mutable bool bIsValid = false;

	FTBImpact() = default;
	FTBImpact(const FHitResult& HitResult, const FVector& Velocity, TEnumAsByte<EPhysicalSurface> SurfaceType, const FVector& StartLocation, const bool bIsRicochet = false, const bool bIsPenetrating = false, const double PenetrationDepth = 0.0, const double dV = 0.0, const FTBProjectileId ProjectileId = FTBProjectileId(), const FVector& RicochetVector = FVector::ZeroVector)
		: HitResult(HitResult)
		, ImpactVelocity(Velocity)
		, RicochetVector(RicochetVector)
		, StartLocation(StartLocation)
		, SurfaceType(SurfaceType)
		, bRicochet(bIsRicochet)
		, bIsPenetrating(bIsPenetrating)
		, PenetrationDepth(PenetrationDepth)
		, dV(dV)
		, ProjectileId(ProjectileId)
	{
		IsValid();
	}

	FTBImpact(const FHitResult& HitResult, FVector Velocity, TEnumAsByte<EPhysicalSurface> SurfaceType, const FVector& StartLocation, ETBProjectileSize ProjectileSize = PS_Small, bool bRicochet = false, const bool bIsPenetrating = false, const double PenetrationDepth = 0.0, const double dV = 0.0, const FTBProjectileId ProjectileId = FTBProjectileId(), const FVector& RicochetVector = FVector::ZeroVector)
		: HitResult(HitResult)
		, ImpactVelocity(Velocity)
		, RicochetVector(RicochetVector)
		, StartLocation(StartLocation)
		, ProjectileSize(ProjectileSize)
		, SurfaceType(SurfaceType)
		, bRicochet(bRicochet)
		, bIsPenetrating(bIsPenetrating)
		, PenetrationDepth(PenetrationDepth)
		, dV(dV)
		, ProjectileId(ProjectileId)
	{
		IsValid();
	}

	void GenerateId(const bool Regenerate = false)
	{
		if (Regenerate)
		{
			ProjectileId.Regenerate();
		}
		else
		{
			ProjectileId.Generate();
		}
	}

	inline bool IsValid() const
	{
		bIsValid = ProjectileId != FTBProjectileId::None
			&& HitResult.GetComponent()
			&& HitResult.GetHitObjectHandle().IsValid()
			&& HitResult.GetHitObjectHandle().FetchActor();
		return bIsValid;
	}

	inline operator bool()
	{
		return IsValid();
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		TB::BitPackHelpers::PackArchive<3>(Ar, &bRicochet, &bIsPenetrating, &bIsValid);

		bool bLocalSuccess = true;
		HitResult.NetSerialize(Ar, Map, bLocalSuccess);
		bOutSuccess &= bLocalSuccess;
		ImpactVelocity.NetSerialize(Ar, Map, bLocalSuccess);
		bOutSuccess &= bLocalSuccess;
		SerializeOptionalValue(Ar.IsSaving(), Ar, RicochetVector, FVector::ZeroVector);

		Ar << ProjectileSize;
		Ar << SurfaceType;
		Ar << ProjectileId;
		Ar << PenetrationDepth;
		Ar << dV;

		return bLocalSuccess;
	}

	FORCEINLINE bool operator==(const FTBImpact& other) const
	{
		return TB::HitResultsAreEqual(HitResult, other.HitResult)
			&& ImpactVelocity.Equals(other.ImpactVelocity)
			&& RicochetVector.Equals(other.RicochetVector)
			&& ProjectileSize == other.ProjectileSize
			&& SurfaceType == other.SurfaceType
			&& bRicochet == other.bRicochet
			&& bIsPenetrating == other.bIsPenetrating
			&& FMath::IsNearlyEqual(PenetrationDepth, other.PenetrationDepth)
			&& FMath::IsNearlyEqual(dV, other.dV)
			&& ProjectileId == other.ProjectileId
			&& bIsValid == other.bIsValid;
	}

	FORCEINLINE bool operator!=(const FTBImpact& other) const
	{
		return !(*this == other);
	}
};
template<> struct TB::Traits::TTypeTraitsIfInvalid<FTBImpact>
{
	enum
	{
		UseDefaults = false,
		MarkIfInvalid = true
	};
};


USTRUCT(BlueprintType)
struct TERMINALBALLISTICS_API FTBImpactParamsBasic : public FTBImpact
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impact)
	FTBProjectile Projectile;

	FTBImpactParamsBasic()
	{
		HitResult = FHitResult();
		ImpactVelocity = FVector();
		bRicochet = false;
		ProjectileSize = Projectile.ProjectileSize;
	}

	FTBImpactParamsBasic(const FHitResult& InHitResult, FTBProjectile InProjectile, const FVector& InVelocity, bool bIsPenetrating, TEnumAsByte<EPhysicalSurface> SurfaceType, const FVector& InStartLocation, bool bIsRicochet = false, const FTBProjectileId InProjectileId = FTBProjectileId(), const double dV = 0.0, const FVector& InRicochetVector = FVector::ZeroVector)
		: FTBImpact(InHitResult, InVelocity, SurfaceType, InStartLocation, InProjectile.ProjectileSize, bIsRicochet, bIsPenetrating, 0.0, dV, InProjectileId, InRicochetVector)
		, Projectile(InProjectile)
	{}

	FTBImpactParamsBasic(const FHitResult& InHitResult, FTBProjectile InProjectile, const FVector& InVelocity, bool bIsPenetrating, TEnumAsByte<EPhysicalSurface> SurfaceType, const FVector& InStartLocation, double InPenetrationDepth, double dV, bool bIsRicochet = false, const FTBProjectileId InProjectileId = FTBProjectileId(), const FVector& InRicochetVector = FVector::ZeroVector)
		: FTBImpact(InHitResult, InVelocity, SurfaceType, InStartLocation, InProjectile.ProjectileSize, bIsRicochet, bIsPenetrating, InPenetrationDepth, dV, InProjectileId, InRicochetVector)
		, Projectile(InProjectile)
	{}

	FORCEINLINE bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		bOutSuccess = true;
		FTBImpact::NetSerialize(Ar, Map, bOutSuccess);
		Ar << Projectile;
		return true;
	}

	FORCEINLINE bool operator==(const FTBImpactParamsBasic& other) const
	{
		return Super::operator==(other)
			&& Projectile == other.Projectile;
	}

	FORCEINLINE bool operator!=(const FTBImpactParamsBasic& other) const
	{
		return !(*this == other);
	}
};
template<> struct TB::Traits::TIsImpactStruct<FTBImpactParamsBasic> { enum { Value = true }; };
template<> struct TStructOpsTypeTraits<FTBImpactParamsBasic> : public TStructOpsTypeTraitsBase2<FTBImpactParamsBasic>
{
	enum
	{
		WithIdenticalViaEquality = true,
		WithNetSerializer = true
	};
};


USTRUCT(BlueprintType)
struct TERMINALBALLISTICS_API FTBImpactParams : public FTBImpact
{
	GENERATED_BODY()
	TB_WITH_OPTIMIZED_SERIALIZER();
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImpactParams)
	FTBBulletPhysicalProperties BulletProperties;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImpactParams)
	FTBBulletInfo BulletInfo;

	FTBImpactParams()
		: FTBImpact()
		, BulletProperties()
		, BulletInfo()
	{}

	FTBImpactParams(const FHitResult& InHitResult, UBulletDataAsset* BulletDataAsset, FVector InVelocity, bool bIsPenetrating, TEnumAsByte<EPhysicalSurface> SurfaceType, const FVector& InStartLocation, double PenetrationDepth, double dV, bool bIsRicochet = false, const FTBProjectileId InProjectileId = FTBProjectileId(), const FVector& InRicochetVector = FVector::ZeroVector);

	FTBImpactParams(const FHitResult& InHitResult, const FTBBullet& Bullet, const FVector& InVelocity, const bool bIsPenetrating, TEnumAsByte<EPhysicalSurface> SurfaceType, const FVector& InStartLocation, const bool bIsRicochet = false, const FTBProjectileId InProjectileId = FTBProjectileId(), const double dV = 0.0, const FVector& InRicochetVector = FVector::ZeroVector);

	FTBImpactParams(const FHitResult& InHitResult, FTBBullet* Bullet, FVector InVelocity, bool bIsPenetrating, TEnumAsByte<EPhysicalSurface> SurfaceType, const FVector& InStartLocation, bool bIsRicochet = false, const FTBProjectileId InProjectileId = FTBProjectileId(), const FVector& InRicochetVector = FVector::ZeroVector);

	FTBImpactParams(const FHitResult& InHitResult, FTBBullet* Bullet, FVector InVelocity, bool bIsPenetrating, TEnumAsByte<EPhysicalSurface> SurfaceType, const FVector& InStartLocation, double PenetrationDepth, double dV, bool bIsRicochet = false, const FTBProjectileId InProjectileId = FTBProjectileId(), const FVector& InRicochetVector = FVector::ZeroVector);

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

	FORCEINLINE bool operator==(const FTBImpactParams& other) const
	{
		return Super::operator==(other)
			&& BulletProperties == other.BulletProperties
			&& BulletInfo == other.BulletInfo;
	}

	FORCEINLINE bool operator!=(const FTBImpactParams& other) const
	{
		return !(*this == other);
	}
};
template<> struct TB::Traits::TIsImpactStruct<FTBImpactParams> { enum { Value = true }; };
template<> struct TStructOpsTypeTraits<FTBImpactParams> : public TStructOpsTypeTraitsBase2<FTBImpactParams>
{
	enum
	{
		WithIdenticalViaEquality = true,
		WithNetSerializer = true
	};
};



#pragma region Delegates
DECLARE_DYNAMIC_DELEGATE_TwoParams(FBPOnProjectileComplete, const FTBProjectileId, Id, const TArray<FPredictProjectilePathPointData>&, PathResults);

DECLARE_DYNAMIC_DELEGATE_OneParam(FBPOnBulletHit, const FTBImpactParams&, ImpactParams);
DECLARE_DYNAMIC_DELEGATE_OneParam(FBPOnBulletExitHit, const FTBImpactParams&, ImpactParams);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FBPOnBulletInjure, const FTBImpactParams&, impactParams, const FTBProjectileInjuryParams&, InjuryParams);

DECLARE_DYNAMIC_DELEGATE_OneParam(FBPOnProjectileHit, const FTBImpactParamsBasic&, ImpactParams);
DECLARE_DYNAMIC_DELEGATE_OneParam(FBPOnProjectileExitHit, const FTBImpactParamsBasic&, ImpactParams);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FBPOnProjectileInjure, const FTBImpactParamsBasic&, ImpactParams, const FTBProjectileInjuryParams&, InjuryParams);
#pragma endregion


UCLASS()
class TERMINALBALLISTICS_API UTBDamageType : public UDamageType
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TerminalBallistics)
	FTBProjectileInjuryParams InjuryParams;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TerminalBallistics)
	FTBImpactParams ImpactParams;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TerminalBallistics)
	FTBImpactParamsBasic ImpactParamsBasic;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TerminalBallistics)
	bool bCausedByBullet = true;
};
