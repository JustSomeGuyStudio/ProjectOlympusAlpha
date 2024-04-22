// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "DataAssets/SKGPDAProjectile.h"
#include "Subsystems/WorldSubsystem.h"
#include "SKGProjectileWorldSubsystem.generated.h"

class UNiagaraComponent;
class AWindDirectionalSource;
class UWorld;
class UPrimitiveComponent;
class UWindDirectionalSourceComponent;

DECLARE_STATS_GROUP(TEXT("SKGShooterFrameworkProjectileStatGroup"), STATGROUP_SKGShooterFrameworkProjectile,
					STATCAT_Advanced);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FSKGOnProjectileImpact, const FHitResult&, HitResult, const FVector&, Direction, const int32, ProjectileID);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FSKGOnProjetilePositionUpdate, const FVector&, Location, const FVector&, Velocity, const int32, ProjectileID);

USTRUCT(BlueprintType)
struct FSKGProjectileData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, NotReplicated, BlueprintReadWrite, Category = "SKGProjectile")
	int32 ProjectileID {-1};
	UPROPERTY(EditAnywhere, NotReplicated, BlueprintReadWrite, Category = "SKGProjectile")
	double Velocity {10000.0};
	UPROPERTY(EditAnywhere, NotReplicated, BlueprintReadWrite, Category = "SKGProjectile")
	double Weight {55.0};
	UPROPERTY(EditAnywhere, NotReplicated, BlueprintReadWrite, Category = "SKGProjectile")
	double DragCoefficient {0.283};
	UPROPERTY(EditAnywhere, NotReplicated, BlueprintReadWrite, Category = "SKGProjectile")
	float Lifetime {6.0f};
	UPROPERTY(EditAnywhere, NotReplicated, BlueprintReadWrite, Category = "SKGProjectile")
	TEnumAsByte<ECollisionChannel> CollisionChannel {ECC_Visibility};
	UPROPERTY(EditAnywhere, NotReplicated, BlueprintReadWrite, Category = "SKGProjectile")
	TArray<AActor*> ActorsToIgnore;
	UPROPERTY(EditAnywhere, NotReplicated, BlueprintReadWrite, Category = "SKGProjectile")
	FSKGProjectileDebugData DebugData;
	UPROPERTY(EditAnywhere, NotReplicated, BlueprintReadWrite, Category = "SKGProjectile")
	FSKGProjectileParticleData ParticleData;
	
	FSKGOnProjectileImpact OnImpact;
	FSKGOnProjetilePositionUpdate OnPositionUpdate;

	UPROPERTY(NotReplicated)
	TObjectPtr<UWorld> WorldPtr;
	UPROPERTY(EditAnywhere, NotReplicated, BlueprintReadWrite, Category = "SKGProjectile")
	TObjectPtr<UPrimitiveComponent> VisualComponent;
	bool bHandleVisualComponent {false};

	UPROPERTY(EditAnywhere, NotReplicated, BlueprintReadWrite, Category = "SKGProjectile")
	FTransform LaunchTransform {FTransform()};
	float LaunchStartTime {0.0f};
	bool bHadImpact {false};
	
	UPROPERTY(EditAnywhere, NotReplicated, BlueprintReadWrite, Category = "SKGProjectile")
	FVector ForwardVelocity {FVector::ZeroVector};
	UPROPERTY(EditAnywhere, NotReplicated, BlueprintReadWrite, Category = "SKGProjectile")
	FVector Location {FVector::ZeroVector};
	
	FVector End {FVector::ZeroVector};
	FVector PreviousLocation {FVector::ZeroVector};

	FHitResult HitResult;
	
	void Initalize(UWorld* World);
	void PerformStep(const FVector& Wind, float DeltaSeconds);
	FHitResult PerformTrace() const;
};

UCLASS()
class SKGPROJECTILE_API USKGProjectileWorldSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool IsTickable() const override { return Projectiles.Num() > 0; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(USKGProjectileWorldSubsystem, STATGROUP_Tickables); }
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<UWorld> World;
	UPROPERTY()
	TArray<UWindDirectionalSourceComponent*> WindSources;
	
	// MAKE INLINE ON STACK LATER
	TArray<FSKGProjectileData, TInlineAllocator<40>> Projectiles;
	void PerformProjectileStep(FSKGProjectileData& Projectile, const FVector& Wind);
	FVector GetWindSourceVelocity(const FSKGProjectileData& Projectile);
	void RemoveProjectile(const int32 Index);

public:
	UFUNCTION(BlueprintCallable, Category = "SKGShooterWorldSubsystem|Projectile")
	void SetWindSources(TArray<AWindDirectionalSource*> WindDirectionalSources);
	// VisualComponentOverride will replace the ParticleData
	UFUNCTION(BlueprintCallable, Category = "SKGShooterWorldSubsystem|Projectile")
	void FireProjectile(const int32 ProjectileID, USKGPDAProjectile* DataAsset, const TArray<AActor*>& ActorsToIgnore, const FTransform& LaunchTransform, UPrimitiveComponent* VisualComponentOverride, FSKGOnProjectileImpact OnImpact, FSKGOnProjetilePositionUpdate OnPositionUpdate);
	// Number of LaunchTransforms dictates number of projectiles
	UFUNCTION(BlueprintCallable, Category = "SKGShooterWorldSubsystem|Projectile")
	void FireProjectiles(const TArray<int32>& ProjectileIDs, USKGPDAProjectile* DataAsset, const TArray<AActor*>& ActorsToIgnore, const TArray<FTransform>& LaunchTransforms, FSKGOnProjectileImpact OnImpact, FSKGOnProjetilePositionUpdate OnPositionUpdate);
	// Returns true if valid (LaunchTransform AND OpticAimSocket valid)
	UFUNCTION(BlueprintCallable, Category = "SKGShooterWorldSubsystem|Projectile")
	bool GetProjectileZero(USKGPDAProjectile* DataAsset, double Distance, FTransform LaunchTransform, FTransform OpticAimSocket, FRotator& OUTLookAtRotation);
	// Returns true if valid (LaunchTransform AND Location valid)
	UFUNCTION(BlueprintCallable, Category = "SKGShooterWorldSubsystem|Projectile")
	bool GetProjectileZeroAtLocation(FRotator& OUTLookAtRotation, const FVector& Location, FTransform LaunchTransform, FTransform OpticAimSocket);
	// Returns true if valid (LaunchTransform not invalid)
	UFUNCTION(BlueprintCallable, Category = "SKGShooterWorldSubsystem|Projectile")
	virtual bool GetProjectileLocationAtDistance(USKGPDAProjectile* DataAsset, double Distance, FTransform LaunchTransform, FVector& OUTLocation);

	UFUNCTION(BlueprintPure, Category = "SKGShooterWorldSubsystem|Projectile")
	bool GetProjectileByID(const int32 ID, FSKGProjectileData& ProjectileData) const;
};
