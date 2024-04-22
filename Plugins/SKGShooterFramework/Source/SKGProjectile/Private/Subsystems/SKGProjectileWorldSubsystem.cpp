// Copyright 2023, Dakota Dawe, All rights reserved


#include "Subsystems/SKGProjectileWorldSubsystem.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/WindDirectionalSourceComponent.h"
#include "Engine/WindDirectionalSource.h"
#include "Engine/World.h"
#include "SceneManagement.h"
#include "Kismet/KismetMathLibrary.h"

DECLARE_CYCLE_STAT(TEXT("Tick"), STAT_SKGTick, STATGROUP_SKGShooterFrameworkProjectile);

constexpr double Gravity = -982.0;

void FSKGProjectileData::Initalize(UWorld* World)
{
	Location = LaunchTransform.GetLocation();
	ForwardVelocity = LaunchTransform.GetRotation().GetForwardVector() * Velocity;
	WorldPtr = World;

	if (bHandleVisualComponent && !VisualComponent && ParticleData && ParticleData.ParticleSpawnDelayDistance == 0.0f)
	{
		VisualComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldPtr, ParticleData.Particle, Location, ForwardVelocity.Rotation());
	}
}

void FSKGProjectileData::PerformStep(const FVector& Wind, float DeltaSeconds)
{
	const double DragForce = 0.5f * FMath::Square(ForwardVelocity.Size() * 0.01) * 1.225 * (UE_DOUBLE_PI * FMath::Square(0.0057)) * DragCoefficient;

	const FVector GravityVelocity = FVector(0.0, 0.0, Gravity);
	const FVector DragVelocity = (FVector(DragForce) * (ForwardVelocity * 0.01)) * 10.0;
	const FVector AccumulativeVelocity = -DragVelocity + GravityVelocity + Wind;
	ForwardVelocity += AccumulativeVelocity * DeltaSeconds;
	End = Location + ForwardVelocity * DeltaSeconds;
	
#if WITH_EDITOR
	if (DebugData.bDebugPath)
	{
		DrawDebugLine(WorldPtr, Location, End, FColor::Red, false, DebugData.DebugLifetime, 0, DebugData.LineThickness);
	}
#endif
	PreviousLocation = Location;
	Location = End;
}

FHitResult FSKGProjectileData::PerformTrace() const
{
	FHitResult NewHitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActors(ActorsToIgnore);
	Params.bReturnPhysicalMaterial = true;
	WorldPtr->LineTraceSingleByChannel(NewHitResult, PreviousLocation, End, CollisionChannel, Params);
	return NewHitResult;
}

void USKGProjectileWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	World = GetWorld();
}

void USKGProjectileWorldSubsystem::Deinitialize()
{
	Super::Deinitialize();

	for (int32 i = 0; i < Projectiles.Num(); ++i)
	{
		RemoveProjectile(i);
	}
}

void USKGProjectileWorldSubsystem::PerformProjectileStep(FSKGProjectileData& Projectile, const FVector& Wind)
{
	Projectile.PerformStep(Wind, Projectile.WorldPtr->DeltaTimeSeconds);
	Projectile.OnPositionUpdate.ExecuteIfBound(Projectile.Location, Projectile.ForwardVelocity, Projectile.ProjectileID);

	if (Projectile.bHandleVisualComponent)
	{
		if (Projectile.VisualComponent)
		{
			//Projectile.VisualComponent->SetWorldLocation(Projectile.Location);
			Projectile.VisualComponent->SetWorldLocationAndRotation(Projectile.Location, Projectile.ForwardVelocity.Rotation());
		}
		else if (Projectile.ParticleData && FVector::Dist(Projectile.LaunchTransform.GetLocation(), Projectile.Location) > Projectile.ParticleData.ParticleSpawnDelayDistance)
		{
			Projectile.VisualComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(Projectile.WorldPtr, Projectile.ParticleData.Particle, Projectile.Location);
		}
	}
		
	const FHitResult NewHitResult = Projectile.PerformTrace();
	if (NewHitResult.GetActor() != Projectile.HitResult.GetActor())
	{
		Projectile.HitResult = NewHitResult;
		Projectile.bHadImpact = true;
		Projectile.OnImpact.ExecuteIfBound(Projectile.HitResult, Projectile.ForwardVelocity, Projectile.ProjectileID);
#if WITH_EDITOR
		if (Projectile.DebugData.bDebugPath)
		{
			DrawDebugSphere(World, Projectile.HitResult.Location, Projectile.DebugData.ImpactRadius, 6.0f, FColor::Green, false, Projectile.DebugData.DebugLifetime, 0, 1.0f);
		}
#endif
	}
}

FVector USKGProjectileWorldSubsystem::GetWindSourceVelocity(const FSKGProjectileData& Projectile)
{
	FVector WindVelocity = FVector::ZeroVector;
	float CurrentClosestWind = 100000000.0f;
	UWindDirectionalSourceComponent* ClosestWindSource = nullptr;
	for (UWindDirectionalSourceComponent* WindSource : WindSources)
	{
		if (WindSource)
		{
			const FVector WindLocation = WindSource->GetComponentLocation();
			const float WindDistance = FVector::Dist(WindLocation, Projectile.Location);
			if (WindDistance < CurrentClosestWind)
			{
				CurrentClosestWind = WindDistance;
				ClosestWindSource = WindSource;
			}
		}
	}
	if (ClosestWindSource)
	{
		FWindData WindData;
		float Weight;
		ClosestWindSource->GetWindParameters(Projectile.Location, WindData, Weight);
		WindVelocity = WindData.Direction * WindData.Speed;
	}
	return WindVelocity;
}

void USKGProjectileWorldSubsystem::RemoveProjectile(const int32 Index)
{
	FSKGProjectileData& Projectile = Projectiles[Index];
	if (Projectile.VisualComponent)
	{
		Projectile.VisualComponent->DestroyComponent();
	}
	Projectiles.RemoveAt(Index, 1, false);
}

void USKGProjectileWorldSubsystem::SetWindSources(TArray<AWindDirectionalSource*> WindDirectionalSources)
{
	WindSources.Empty(WindDirectionalSources.Num());
	for (AWindDirectionalSource* WindDirectionalSource : WindDirectionalSources)
	{
		WindSources.Add(WindDirectionalSource->GetComponent());
	}
}

void USKGProjectileWorldSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!World)
	{
		World = GetWorld();
	}
	
	SCOPE_CYCLE_COUNTER(STAT_SKGTick);
	const float CurrentTimeSeconds = GetWorld()->GetTimeSeconds();
	for (int32 i = 0; i < Projectiles.Num(); ++i)
	{
		FSKGProjectileData& Projectile = Projectiles[i];
		PerformProjectileStep(Projectile, GetWindSourceVelocity(Projectile));
		if (Projectile.bHadImpact || CurrentTimeSeconds - Projectile.LaunchStartTime > Projectile.Lifetime)
		{
			RemoveProjectile(i);
		}
	}
}

void USKGProjectileWorldSubsystem::FireProjectile(const int32 ProjectileID, USKGPDAProjectile* DataAsset, const TArray<AActor*>& ActorsToIgnore, const FTransform& LaunchTransform, UPrimitiveComponent* VisualComponentOverride, FSKGOnProjectileImpact OnImpact, FSKGOnProjetilePositionUpdate OnPositionUpdate)
{
	if (ensureMsgf(DataAsset, TEXT("Data Asset INVALID for FireProjectile")))
	{
		FSKGProjectileData Projectile;
		Projectile.ProjectileID = ProjectileID;
		Projectile.Velocity = DataAsset->Velocity;
		Projectile.Weight = DataAsset->Weight;
		Projectile.DragCoefficient = DataAsset->DragCoefficient;
		Projectile.Lifetime = DataAsset->Lifetime;
		Projectile.CollisionChannel = DataAsset->CollisionChannel;
		Projectile.ActorsToIgnore = ActorsToIgnore;
		Projectile.ParticleData = DataAsset->ParticleData;
		Projectile.VisualComponent = VisualComponentOverride;
		Projectile.DebugData = DataAsset->DebugData;

		Projectile.OnImpact = OnImpact;
		Projectile.OnPositionUpdate = OnPositionUpdate;
		Projectile.LaunchTransform = LaunchTransform;
		Projectile.LaunchStartTime = GetWorld()->GetTimeSeconds();

		Projectile.bHandleVisualComponent = Projectile.VisualComponent || DataAsset->ParticleData;
		
		Projectile.Initalize(GetWorld());
		Projectiles.Add(Projectile);
	}
}

void USKGProjectileWorldSubsystem::FireProjectiles(const TArray<int32>& ProjectileIDs, USKGPDAProjectile* DataAsset,
	const TArray<AActor*>& ActorsToIgnore, const TArray<FTransform>& LaunchTransforms, FSKGOnProjectileImpact OnImpact,
	FSKGOnProjetilePositionUpdate OnPositionUpdate)
{
	if (ensureMsgf(DataAsset, TEXT("Data Asset INVALID for FireProjectiles")))
	{
		for (int32 i = 0; i < LaunchTransforms.Num(); ++i)
		{
			const FTransform LaunchTransform = LaunchTransforms[i];
			const int32 ProjectileID = ProjectileIDs.Num() > i ? ProjectileIDs[i] : -1;
			FireProjectile(ProjectileID, DataAsset, ActorsToIgnore, LaunchTransform, nullptr, OnImpact, OnPositionUpdate);
		}
	}
}

bool USKGProjectileWorldSubsystem::GetProjectileZero(USKGPDAProjectile* DataAsset, double Distance, FTransform LaunchTransform, FTransform OpticAimSocket, FRotator& OUTLookAtRotation)
{
	if (ensureMsgf(DataAsset, TEXT("Data Asset INVALID for GetProjectileZero")))
	{
		if (!LaunchTransform.GetLocation().Equals(FVector::ZeroVector) && !OpticAimSocket.GetLocation().Equals(FVector::ZeroVector))
		{
			const double ZOffset = UKismetMathLibrary::MakeRelativeTransform(OpticAimSocket, LaunchTransform).GetLocation().Z;
			LaunchTransform.SetRotation(FQuat());
			FVector LookAtStart = LaunchTransform.GetLocation();
			LookAtStart.Z += ZOffset;

			FVector ProjectileEndLocation;
			GetProjectileLocationAtDistance(DataAsset, Distance, LaunchTransform, ProjectileEndLocation);
			OUTLookAtRotation = UKismetMathLibrary::FindLookAtRotation(LookAtStart, ProjectileEndLocation);
			return true;
		}
	}
	return false;
}

bool USKGProjectileWorldSubsystem::GetProjectileZeroAtLocation(FRotator& OUTLookAtRotation, const FVector& Location, FTransform LaunchTransform, FTransform OpticAimSocket)
{
	if (!LaunchTransform.GetLocation().Equals(FVector::ZeroVector) && !Location.Equals(FVector::ZeroVector))
	{
		const double ZOffset = UKismetMathLibrary::MakeRelativeTransform(OpticAimSocket, LaunchTransform).GetLocation().Z;
		LaunchTransform.SetRotation(FQuat());
		FVector LookAtStart = LaunchTransform.GetLocation();
		LookAtStart.Z += ZOffset;

		OUTLookAtRotation = UKismetMathLibrary::FindLookAtRotation(LookAtStart, Location);
		return true;
	}
	return false;
}

bool USKGProjectileWorldSubsystem::GetProjectileLocationAtDistance(USKGPDAProjectile* DataAsset, double Distance, FTransform LaunchTransform, FVector& OUTLocation)
{
	if (ensureMsgf(DataAsset, TEXT("Data Asset INVALID for GetProjectileLocationAtDistance")))
	{
		if (World->DeltaTimeSeconds > 0.0f)
		{
			LaunchTransform.SetRotation(FQuat(0.0));
			Distance *= 100.0f;
			FSKGProjectileData Projectile;
			Projectile.DragCoefficient = DataAsset->DragCoefficient;
			Projectile.Velocity = DataAsset->Velocity;
			Projectile.LaunchTransform = LaunchTransform;
			Projectile.Initalize(GetWorld());
		
			const FVector StartLocation = LaunchTransform.GetLocation();
			int32 Counter = 0;
			while (FVector::Distance(StartLocation, Projectile.Location) < Distance && Counter < 5000)
			{
				++Counter;
				Projectile.PerformStep(FVector::ZeroVector, World->DeltaTimeSeconds);
			}

			//DrawDebugSphere(World, Projectile.Location, 20.0f, 3.0f, FColor::Yellow, true, -1, 0);
			OUTLocation = Projectile.Location;
			return true;
		}
	}
	OUTLocation = FVector::ZeroVector;
	return false;
}

bool USKGProjectileWorldSubsystem::GetProjectileByID(const int32 ID, FSKGProjectileData& ProjectileData) const
{
	for (const FSKGProjectileData& Projectile : Projectiles)
	{
		if (Projectile.ProjectileID == ID)
		{
			ProjectileData = Projectile;
			return true;
		}
	}
	return false;
}
