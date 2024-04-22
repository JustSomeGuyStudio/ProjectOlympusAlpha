// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "TerminalBallisticsTraits.h"
#include "TBProjectileInjury.generated.h"



USTRUCT(BlueprintType)
struct TERMINALBALLISTICS_API FTBWoundCavity
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Wound Cavity")
	double TemporaryCavityRadius = 0.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Wound Cavity")
	double PermanentCavityRadius = 0.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Wound Cavity")
	double PermanentCavityDepth = 0.0;

	FTBWoundCavity() = default;
	FTBWoundCavity(const double TemporaryCavityRadius, const double PermanentCavityRadius, const double PermanentCavityDepth = 0.0)
		: TemporaryCavityRadius(TemporaryCavityRadius)
		, PermanentCavityRadius(PermanentCavityRadius)
		, PermanentCavityDepth(PermanentCavityDepth)
	{}

	FORCEINLINE friend FArchive& operator<<(FArchive& Ar, FTBWoundCavity& WoundCavity)
	{
		SerializeOptionalValue(Ar.IsSaving(), Ar, WoundCavity.TemporaryCavityRadius, 0.0);
		SerializeOptionalValue(Ar.IsSaving(), Ar, WoundCavity.PermanentCavityRadius, 0.0);
		SerializeOptionalValue(Ar.IsSaving(), Ar, WoundCavity.PermanentCavityDepth, 0.0);
		return Ar;
	}

	FORCEINLINE bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << *this;
		bOutSuccess = true;
		return true;
	}

	FORCEINLINE bool operator==(const FTBWoundCavity& rhs) const
	{
		return TemporaryCavityRadius == rhs.TemporaryCavityRadius
			&& PermanentCavityRadius == rhs.PermanentCavityRadius
			&& PermanentCavityDepth == rhs.PermanentCavityDepth;
	}
};
template<> struct TStructOpsTypeTraits<FTBWoundCavity> : public TStructOpsTypeTraitsBase2<FTBWoundCavity>
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true
	};
};


USTRUCT(BlueprintType)
struct TERMINALBALLISTICS_API FTBProjectileInjuryParams
{
	GENERATED_BODY()
public:

	// Depth of penetration (cm)
	UPROPERTY(BlueprintReadWrite, Category = Injury)
	double PenetrationDepth = 0.0;
	// Energy imparted during penetration (J)
	UPROPERTY(BlueprintReadWrite, Category = Injury)
	double ImpartedEnergy = 0.0;
	// Temporary and permanent wound cavities caused by penetration. Use with caution, they are only approximations.
	UPROPERTY(BlueprintReadWrite, Category = Injury)
	FTBWoundCavity WoundCavity;
	// Projectile velocity at time of impact
	UPROPERTY(BlueprintReadWrite, Category = Injury)
	FVector ImpactVelocity = FVector::ZeroVector;
	// Location at start of penetration
	UPROPERTY(BlueprintReadWrite, Category = Injury)
	FVector ImpactLocation = FVector::ZeroVector;
	// Exit location, if applicable
	UPROPERTY(BlueprintReadWrite, Category = Injury)
	FVector ExitLocation = FVector::ZeroVector;
	// Actor that was hit
	UPROPERTY(BlueprintReadWrite, Category = Injury)
	TObjectPtr<AActor> HitActor = nullptr;
	// Component that was hit
	UPROPERTY(BlueprintReadWrite, Category = Injury)
	TObjectPtr<UPrimitiveComponent> HitComponent = nullptr;
	// Controller that fired this projectile
	UPROPERTY(BlueprintReadWrite, Category = Injury)
	TObjectPtr<AController> Instigator = nullptr;
	UPROPERTY(BlueprintReadWrite, Category = Injury)
	// Actor responsible for this projectile
	TObjectPtr<AActor> DamageCauser = nullptr;
	UPROPERTY(BlueprintReadWrite, Category = Injury)
	mutable bool bIsValid = false;

public:
	FTBProjectileInjuryParams() = default;

	FTBProjectileInjuryParams(const FTBWoundCavity& InWoundCavity, const double InPenetrationDepth, const double InImpartedEnergy, const FVector& InImpactVelocity, const FVector& InImpactLocation, const FVector& InExitLocation, AActor* Actor, UPrimitiveComponent* Component, AController* Instigator = nullptr, AActor* DamageCauser = nullptr)
		: PenetrationDepth(InPenetrationDepth)
		, ImpartedEnergy(InImpartedEnergy)
		, WoundCavity(InWoundCavity)
		, ImpactVelocity(InImpactVelocity)
		, ImpactLocation(InImpactLocation)
		, ExitLocation(InExitLocation)
		, HitActor(Actor)
		, HitComponent(Component)
		, Instigator(Instigator)
		, DamageCauser(DamageCauser)
	{
		bIsValid = IsValid();
	}

	FTBProjectileInjuryParams(const FTBWoundCavity& InWoundCavity, const double InPenetrationDepth, const double InImpartedEnergy, const FVector& InImpactVelocity, const FVector& InImpactLocation, const FVector& InExitLocation, const FHitResult& HitResult, AController* Instigator = nullptr, AActor* DamageCauser = nullptr)
		: PenetrationDepth(InPenetrationDepth)
		, ImpartedEnergy(InImpartedEnergy)
		, WoundCavity(InWoundCavity)
		, ImpactVelocity(InImpactVelocity)
		, ImpactLocation(InImpactLocation)
		, ExitLocation(InExitLocation)
		, HitActor(HitResult.GetActor())
		, HitComponent(HitResult.GetComponent())
		, Instigator(Instigator)
		, DamageCauser(DamageCauser)
	{
		bIsValid = IsValid();
	}

	bool IsValid() const
	{
		return HitActor && HitComponent;
	}
};
template<> struct TB::Traits::TTypeTraitsIfInvalid<FTBProjectileInjuryParams>
{
	enum
	{
		UseDefaults = false,
		MarkIfInvalid = true
	};
};
