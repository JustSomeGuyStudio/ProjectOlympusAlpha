// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/DataAsset.h"
#include "SKGPDAProjectile.generated.h"

class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FSKGProjectileDebugData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGProjectileDebugData")
	bool bDebugPath {false};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGProjectileDebugData")
	float LineThickness {1.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGProjectileDebugData")
	bool bDebugImpact {false};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGProjectileDebugData")
	float ImpactRadius {5.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGProjectileDebugData")
	float DebugLifetime {10.0f};
};

USTRUCT(BlueprintType)
struct FSKGProjectileParticleData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGShooterFramework")
	TObjectPtr<UNiagaraSystem> Particle;
	// The delayed range before the particle spawns, such as a tracer with a 100 meter delay would have a value of 10000
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGShooterFramework")
	float ParticleSpawnDelayDistance {0.0f};

	operator bool () const
	{
		return Particle != nullptr;
	}
};

UCLASS()
class SKGPROJECTILE_API USKGPDAProjectile : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	// Velocity of the projectile in cm/s
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGProjectile")
	double Velocity {75000.0};
	// Weight of the projectile in grains
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGProjectile")
	double Weight {55.0};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGProjectile")
	double DragCoefficient {0.283};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGProjectile")
	float Lifetime {6.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGProjectile")
	TEnumAsByte<ECollisionChannel> CollisionChannel {ECC_Visibility};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGProjectile")
	FSKGProjectileParticleData ParticleData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGProjectile")
	FSKGProjectileDebugData DebugData;
};
