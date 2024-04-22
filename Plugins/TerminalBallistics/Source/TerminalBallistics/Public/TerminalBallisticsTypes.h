// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CollisionPresets.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Serialization/StructuredArchive.h"
#include "TerminalBallisticsMacrosAndFunctions.h"
#include "TerminalBallisticsTraits.h"
#include "Types/FindExitHelperTypes.h"
#include "Types/TBProjectileId.h"
#include "TerminalBallisticsTypes.generated.h"



USTRUCT(BlueprintType)
/**
* Wrapper struct for TArray<FOverlapResult>
*/
struct TERMINALBALLISTICS_API FTBOverlapArray
{
	GENERATED_BODY()

public:
	TArray<FOverlapResult> Overlaps;

	FTBOverlapArray() = default;

	FTBOverlapArray(TArray<FOverlapResult> arr)
		: Overlaps(arr)
	{}

	TB_ARRAY_WRAPPED_STRUCT_ITERATOR_FUNCS(Overlaps)
};

USTRUCT(BlueprintType)
struct TERMINALBALLISTICS_API FTBCollisionParams
{
	GENERATED_BODY()

public:
	FCollisionQueryParams QueryParams;
	FCollisionObjectQueryParams ObjectQueryParams;

	FTBCollisionParams()
	{
		QueryParams = FCollisionQueryParams::DefaultQueryParam;
		ObjectQueryParams = FCollisionObjectQueryParams::DefaultObjectQueryParam;
	}

	FTBCollisionParams(FCollisionQueryParams params, FCollisionObjectQueryParams objParams)
	{
		QueryParams = params;
		ObjectQueryParams = objParams;
	}
};

USTRUCT(BlueprintType, DisplayName = "LocationAndDirection", meta = (DisableSplitPin, HasNativeMake = "/Script/TerminalBallistics.TerminalBallisticsStatics.MakeLocationAndDirection", HasNativeBreak = "/Script/TerminalBallistics.TerminalBallisticsStatics.BreakLocationAndDirection"))
struct TERMINALBALLISTICS_API FTBLocationAndDirection
{
	GENERATED_BODY()
	TB_WITH_OPTIMIZED_SERIALIZER();
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Location and Direction")
	FVector Location = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Location and Direction")
	FVector_NetQuantizeNormal Direction = FVector_NetQuantizeNormal::ZeroVector;
	
	FTBLocationAndDirection() = default;

	template<typename Vec1, typename Vec2, TEMPLATE_REQUIRES(TAnd<TB::Traits::TIsFVector<Vec1>, TB::Traits::TIsFVector<Vec2>>::Value)>
	FTBLocationAndDirection(const Vec1& InLocation, const Vec2& InDirection)
		: Location(InLocation)
		, Direction(InDirection)
	{}

	template<typename Vec, TEMPLATE_REQUIRES(TB::Traits::TIsFVector<Vec>::Value)>
	FTBLocationAndDirection(const Vec& InLocation, const FRotator& Rotation)
		: Location(InLocation)
		, Direction(Rotation.Quaternion().GetForwardVector())
	{}

	template<typename Vec, TEMPLATE_REQUIRES(TB::Traits::TIsFVector<Vec>::Value)>
	FTBLocationAndDirection(const Vec& InLocation, const FQuat& Rotation)
		: Location(InLocation)
		, Direction(Rotation.GetForwardVector())
	{}

	FTBLocationAndDirection(const FTransform& Transform)
		: Location(Transform.GetLocation())
		, Direction(Transform.GetRotation().GetForwardVector())
	{}

	inline FRotator GetDirectionAsRotator() const
	{
		return Direction.ToOrientationRotator();
	}

	inline FQuat GetDirectionAsQuat() const
	{
		return Direction.ToOrientationQuat();
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	friend FArchive& operator<<(FArchive& Ar, FTBLocationAndDirection& LocationAndDirection);

	bool operator==(const FTBLocationAndDirection& other) const
	{
		return Location.Equals(other.Location, 1e-3)
			&& Direction.Equals(other.Direction, 1e-3);
	}

	bool operator!=(const FTBLocationAndDirection& other) const
	{
		return !(*this == other);
	}

	template<typename FReal>
	operator UE::Math::TTransform<FReal>()
	{
		return UE::Math::TTransform<FReal>(GetDirectionAsQuat(), Location);
	}
};
template<> struct TIsPODType<FTBLocationAndDirection> { enum { Value = true }; };
template<> struct TStructOpsTypeTraits<FTBLocationAndDirection> : public TStructOpsTypeTraitsBase2<FTBLocationAndDirection>
{
	enum
	{
		WithIdenticalViaEquality = true,
		WithNetSerializer = true,
		WithNetSharedSerialization = true
	};
};

USTRUCT(BlueprintType, DisplayName = "LaunchParams", meta = (DisableSplitPin, HasNativeMake = "/Script/TerminalBallistics.TerminalBallisticsStatics.MakeLaunchParams", HasNativeBreak = "/Script/TerminalBallistics.TerminalBallisticsStatics.BreakLaunchParams"))
struct TERMINALBALLISTICS_API FTBLaunchParams
{
	GENERATED_BODY()
	TB_WITH_OPTIMIZED_SERIALIZER();
public:
	UPROPERTY(BlueprintReadWrite, AdvancedDisplay, Category = "Launch Params")
	ETBProjectileSimulationType SimType = ETBProjectileSimulationType::Default;

	UPROPERTY(BlueprintReadWrite, Category = "Launch Params", meta = (Units = "m/s"))
	/* Speed of the Projectile(m / s) */
	double ProjectileSpeed = 360.0;

	UPROPERTY(BlueprintReadWrite, Category = "Launch Params", meta = (Units = "meters"))
	/** 
	* Effective range (m)
	* This controls roughly how far the Projectile can travel.
	*/
	double EffectiveRange = 150.0;

	UPROPERTY(BlueprintReadWrite, Category = "Launch Params", meta = (Min = "0", UIMin = "0", ToolTip = "Controls how quickly the simulation will run. Can be used for a \"Slow Motion\" effect. A value of zero or less is ignored."))
	double Timescale = 1.0;

	UPROPERTY(BlueprintReadWrite, Category = "Launch Params", AdvancedDisplay, meta = (Units = "meters"))
	/* OwnerIgnoreDistance	Owning actor will be ignored until the projectile has travelled this distance(m).Helps to prevent erroneous hits on the owner due to latency. */
	double OwnerIgnoreDistance = 10.0;

	UPROPERTY(BlueprintReadWrite, Category = "Launch Params")
	/* Where the Projectile is being fired from, and which direction it is pointing. */
	FTBLocationAndDirection FireTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadWrite, Category = "Launch Params")
	TArray<TObjectPtr<AActor>> ToIgnore = {};

	UPROPERTY(BlueprintReadWrite, Category = "Launch Params")
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = CollisionPresets::DefaultCollisionQueryTypes;

	UPROPERTY(BlueprintReadWrite, Category = "Launch Params")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel10;

	UPROPERTY(BlueprintReadWrite, Category = "Launch Params")
	uint8 IgnoreOwner : 1;

	UPROPERTY(BlueprintReadWrite, Category = "Launch Params", meta = (ToolTip = "When true, the velocity of the projectile will be added to the velocity of the owning actor."))
	uint8 AddToOwnerVelocity : 1;

	UPROPERTY(BlueprintReadWrite, Category = "Launch Params", meta = (ToolTip = "If true, no tracer will be used, even if one exists."))
	uint8 bForceNoTracer : 1;

	UPROPERTY(BlueprintReadWrite, Category = "Launch Params")
	TObjectPtr<AActor> Owner = nullptr;
	UPROPERTY(BlueprintReadWrite, Category = "Launch Params")
	TObjectPtr<AController> Instigator = nullptr;

public:
	FTBLaunchParams()
		: IgnoreOwner(true)
		, AddToOwnerVelocity(true)
		, bForceNoTracer(false)
	{}

	FTBLaunchParams(
		TObjectPtr<AActor> InOwner,
		TObjectPtr<AController> InInstigator,
		const double InProjectileSpeed,
		const double InEffectiveRange,
		const FTBLocationAndDirection& InFireTransform,
		const TArray<AActor*> InToIgnore,
		const TArray<TEnumAsByte<EObjectTypeQuery>> InObjectTypes = CollisionPresets::DefaultCollisionQueryTypes,
		const TEnumAsByte<ECollisionChannel> InTraceChannel = ECC_GameTraceChannel10,
		const bool InIgnoreOwner = true,
		const bool InAddToOwnerVelocity = true,
		const bool InForceNoTracer = false,
		const ETBProjectileSimulationType SimType = ETBProjectileSimulationType::Default,
		const double Timescale = 1.0,
		const double OwnerIgnoreDistance = 10.0
	)
		: SimType(SimType)
		, ProjectileSpeed(InProjectileSpeed)
		, EffectiveRange(InEffectiveRange)
		, Timescale(Timescale)
		, OwnerIgnoreDistance(OwnerIgnoreDistance)
		, FireTransform(InFireTransform)
		, ToIgnore(InToIgnore)
		, ObjectTypes(InObjectTypes)
		, TraceChannel(InTraceChannel)
		, IgnoreOwner(InIgnoreOwner)
		, AddToOwnerVelocity(InAddToOwnerVelocity)
		, bForceNoTracer(InForceNoTracer)
		, Owner(InOwner)
		, Instigator(InInstigator)
	{
		if (IgnoreOwner && !ToIgnore.Contains(Owner))
		{
			ToIgnore.Add(Owner);
		}
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

	bool operator==(const FTBLaunchParams& other) const
	{
		return SimType == other.SimType
			&& FMath::IsNearlyEqual(ProjectileSpeed, other.ProjectileSpeed, 1e-4)
			&& FMath::IsNearlyEqual(EffectiveRange, other.EffectiveRange, 1e-4)
			&& FMath::IsNearlyEqual(Timescale, other.Timescale, 1e-4)
			&& FMath::IsNearlyEqual(OwnerIgnoreDistance, other.OwnerIgnoreDistance, 1e-4)
			&& FireTransform == other.FireTransform
			&& ToIgnore == other.ToIgnore
			&& ObjectTypes == other.ObjectTypes
			&& TraceChannel == other.TraceChannel
			&& IgnoreOwner == other.IgnoreOwner
			&& AddToOwnerVelocity == other.AddToOwnerVelocity
			&& bForceNoTracer == other.bForceNoTracer
			&& Owner == other.Owner
			&& Instigator == other.Instigator;
	}

	bool operator!=(const FTBLaunchParams& other) const
	{
		return !(*this == other);
	}
};
template<> struct TStructOpsTypeTraits<FTBLaunchParams> : public TStructOpsTypeTraitsBase2<FTBLaunchParams>
{
	enum
	{
		WithIdenticalViaEquality = true,
		WithNetSerializer = true
	};
};


USTRUCT(BlueprintType, DisplayName = "LaunchData")
struct TERMINALBALLISTICS_API FTBLaunchData
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Launch Data")
	FTBLaunchParams LaunchParams;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Launch Data")
	FTBProjectileId Id = FTBProjectileId::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Launch Data", meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType"))
	int32 DebugType = 0;

	FTBLaunchData() = default;

	FTBLaunchData(const FTBLaunchParams& InLaunchParams, FTBProjectileId InId, int32 InDebugType)
		: LaunchParams(InLaunchParams)
		, Id(InId)
		, DebugType(InDebugType)
	{}
};

USTRUCT(BlueprintType)
struct TERMINALBALLISTICS_API FTBFlyBy
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FlyBy")
	FTBProjectileId ProjectileId;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FlyBy")
	FVector ProjectilePosition = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FlyBy")
	FVector ProjectileVelocity = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FlyBy")
	double Distance = 0.0;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FlyBy", meta = (ToolTip = "Maximum distance from the projectile that the actor could be while still triggering this event."))
	double MaximumDistance = 200.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FlyBy")
	AActor* Actor = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FlyBy")
	ETBProjectileSize ProjectileSize = ETBProjectileSize::None;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FlyBy")
	ETBBulletCaliber BulletCaliber = ETBBulletCaliber::None;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FlyBy")
	uint8 bIsBullet : 1;

	FTBFlyBy()
		: bIsBullet(false)
	{
		MaximumDistance = TB::Configuration::FlyByTraceRadius;
	}

	FTBFlyBy(const FTBProjectileId& ProjectileId, const FVector& ProjectilePosition, const FVector& ProjectileVelocity, const double Distance, AActor* Actor, const ETBBulletCaliber BulletCaliber)
		: ProjectileId(ProjectileId)
		, ProjectilePosition(ProjectilePosition)
		, ProjectileVelocity(ProjectileVelocity)
		, Distance(Distance)
		, Actor(Actor)
		, BulletCaliber(BulletCaliber)
		, bIsBullet(true)
	{
		MaximumDistance = TB::Configuration::FlyByTraceRadius;
	}

	FTBFlyBy(const FTBProjectileId& ProjectileId, const FVector& ProjectilePosition, const FVector& ProjectileVelocity, const double Distance, AActor* Actor, const ETBProjectileSize ProjectileSize)
		: ProjectileId(ProjectileId)
		, ProjectilePosition(ProjectilePosition)
		, ProjectileVelocity(ProjectileVelocity)
		, Distance(Distance)
		, Actor(Actor)
		, ProjectileSize(ProjectileSize)
		, bIsBullet(false)
	{
		MaximumDistance = TB::Configuration::FlyByTraceRadius;
	}

	void NotifyActorOfFlyBy();
	
	FORCEINLINE bool IsValid() const
	{
		return ::IsValid(Actor)
			&& ProjectileId.IsValid();
	}

	FORCEINLINE bool operator==(const FTBFlyBy& rhs) const
	{
		return ProjectileId == rhs.ProjectileId
			&& ProjectileSize == rhs.ProjectileSize
			&& BulletCaliber == rhs.BulletCaliber
			&& bIsBullet == rhs.bIsBullet;
	}

	FORCEINLINE bool operator!=(const FTBFlyBy& other) const
	{
		return !(*this == other);
	}
};
