// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "TerminalBallisticsStatics.h"
#include "Async/ParallelFor.h"
#include "BallisticFunctions.h"
#include "BulletDataAsset.h"
#include "CollisionQueryParams.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameplayTagAssetInterface.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Materials/MaterialInterface.h"
#include "MathUtils.h"
#include "PhysicsUtils.h"
#include "PhysMatManager/PhysMatManager.h"
#include "Subsystems/EnvironmentSubsystem.h"
#include "Subsystems/TerminalBallisticsSubsystem.h"
#include "TerminalBallisticsTags.h"
#include "Threading/TerminalBallisticsThreadingTypes.h"
#include "Types/ImpactParams.h"
#include "Types/TBProjectile.h"



namespace TB
{
	bool CheckIfIsHitZone(const FHitResult& hitResult, TEnumAsByte<EPhysicalSurface>& surfaceType, bool& isBone, bool& isDead, bool& isFlesh)
	{
		using namespace TB::Tags;

		const auto SurfaceTypeFlesh = UTBPhysMatManager::Get().GetSurfaceTypeFlesh();
		const auto SurfaceTypeBone = UTBPhysMatManager::Get().GetSurfaceTypeBone();

		bool isHitZone = false;
		if (hitResult.Component.IsValid() && Cast<IGameplayTagAssetInterface>(hitResult.GetComponent()))
		{
			IGameplayTagAssetInterface* asTagInterface = Cast<IGameplayTagAssetInterface>(hitResult.GetComponent());
			isHitZone = asTagInterface->HasMatchingGameplayTag(FTerminalBallisticsTags::Get().HitZone);
			if (isHitZone)
			{
				FGameplayTagContainer hitTags;
				asTagInterface->GetOwnedGameplayTags(hitTags);
				if (hitTags.HasTag(FTerminalBallisticsTags::Get().HitZoneFlesh))
				{
					surfaceType = SurfaceTypeFlesh;
					isFlesh = true;
				}
				else if (hitTags.HasTag(FTerminalBallisticsTags::Get().HitZoneBone))
				{
					surfaceType = SurfaceTypeBone;
					isBone = true;
				}
				isDead = hitTags.HasTag(FTerminalBallisticsTags::Get().HitZoneDead);
			}
		}
		else if (const AActor* HitActor = hitResult.GetActor())
		{
			if (HitActor->ActorHasTag(PlainTag_Flesh))
			{
				surfaceType = SurfaceTypeFlesh;
				isFlesh = true;
				isHitZone = true;
			}
			else if (HitActor->ActorHasTag(PlainTag_Bone))
			{
				surfaceType = SurfaceTypeBone;
				isBone = true;
				isHitZone = true;
			}
			else if (HitActor->ActorHasTag(PlainTag_HitZone))
			{
				isHitZone = true;
			}
			/* Now see if the component has one of the tags... */
			else if (hitResult.GetComponent()->ComponentHasTag(PlainTag_Flesh))
			{
				surfaceType = SurfaceTypeFlesh;
				isFlesh = true;
				isHitZone = true;
			}
			else if (hitResult.GetComponent()->ComponentHasTag(PlainTag_Bone))
			{
				surfaceType = SurfaceTypeBone;
				isBone = true;
				isHitZone = true;
			}
			else if (hitResult.GetActor()->IsA<APawn>() || hitResult.GetComponent()->GetCollisionObjectType() == ECC_Pawn) // no tags, but it's a pawn
			{
				isHitZone = true;
				if (hitResult.PhysMaterial.IsValid())
				{
					const UPhysicalMaterial* PhysMaterial = hitResult.PhysMaterial.Get();
					if (PhysMaterial == GEngine->DefaultPhysMaterial)
					{
						surfaceType = SurfaceTypeFlesh;
					}
					else
					{
						surfaceType = PhysMaterial->SurfaceType;
					}
					isFlesh = surfaceType == SurfaceTypeFlesh;
					isBone = surfaceType == SurfaceTypeBone;
				}
			}
		}

		return isHitZone;
	}

	EPhysicalSurface GetSurfaceType(FHitResult& HitResult)
	{
		return GetSurfaceProperties(HitResult).SurfaceType;
	}

	static FCriticalSection GetPhysMatMutex;
	TWeakObjectPtr<UPhysicalMaterial> GetPhysicalMaterialFromHitResult(const FHitResult& HitResult, bool& Success)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TB::GetPhysicalMaterialFromHitResult);
		Success = false;
		if (!HitResult.GetActor() || !HitResult.GetComponent())
		{
			return nullptr;
		}
		int32 zero = 0;
		TWeakObjectPtr<UPhysicalMaterial> physMat = nullptr;
		TWeakObjectPtr<UMaterialInterface> mat = nullptr;

		{
			FGCScopeGuard GCGuard;
			if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(HitResult.GetComponent()))
			{
				UMaterialInterface* Material = TB::PhysMatHelpers::GetMaterialFromStaticMesh(StaticMeshComp, 0);
				if (Material)
				{
					mat = MakeWeakObjectPtr(Material);
				}
			}
			else if (HitResult.FaceIndex == -1)
			{
				mat = TWeakObjectPtr<UMaterialInterface>(HitResult.GetComponent()->GetMaterial(0));
			}
			else
			{
				mat = TWeakObjectPtr<UMaterialInterface>(HitResult.GetComponent()->GetMaterialFromCollisionFaceIndex(HitResult.FaceIndex, zero));
			}
		}

		if (mat.IsValid())
		{
			FScopeLock Lock(&GetPhysMatMutex);
			physMat = TWeakObjectPtr<UPhysicalMaterial>(mat.Get()->GetPhysicalMaterial());
			Lock.Unlock();
			Success = physMat.IsValid();
		}

		return physMat;
	}

	FPhysMatProperties GetSurfaceProperties(FHitResult& HitResult, bool* IsHitZone, bool* IsBone, bool* IsDead, bool* IsFlesh)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TB::GetSurfaceProperties);
		bool bIsHitZone = false, bIsBone = false, bIsDead = false, bIsFlesh = false;
		auto SetOutputPtrs = [&]()
		{
			auto SetIfNotNullptr = [](bool* Var, bool Value)
			{
				if(Var)
				{
					*Var = Value;
				}
			};
			SetIfNotNullptr(IsHitZone, bIsHitZone);
			SetIfNotNullptr(IsBone, bIsBone);
			SetIfNotNullptr(IsDead, bIsDead);
			SetIfNotNullptr(IsFlesh, bIsFlesh);
		};
		if (!HitResult.GetActor() || !HitResult.GetComponent())
		{
			FTBPhysMatMapping DefaultMapping = FTBPhysMatMapping::GetDefault();
			HitResult.PhysMaterial = MakeWeakObjectPtr(DefaultMapping.HasPhysicalMaterialAsset() ? TObjectPtr<UPhysicalMaterial>(DefaultMapping.PhysicalMaterial.Get()) : GEngine->DefaultPhysMaterial);
			bIsBone = TB::PhysMatHelpers::IsBone(DefaultMapping.PhysMatProperties);
			bIsFlesh = TB::PhysMatHelpers::IsFlesh(DefaultMapping.PhysMatProperties);
			bIsHitZone = bIsBone || bIsFlesh;
			SetOutputPtrs();
			return DefaultMapping.PhysMatProperties;
		}
		const FTBPhysMatMapping& DefaultMapping = FTBPhysMatMapping::GetDefault();
		TEnumAsByte<EPhysicalSurface> SurfaceType = DefaultMapping.PhysMatProperties.SurfaceType;

		bIsHitZone = CheckIfIsHitZone(HitResult, SurfaceType, bIsBone, bIsDead, bIsFlesh);

		bool Success = false;
		using namespace TB::Tags;
		if (!bIsHitZone)
		{
			bool found = false;
			if (IGameplayTagAssetInterface* asTagInterface = Cast<IGameplayTagAssetInterface>(HitResult.GetComponent()))
			{
				FGameplayTagContainer Tags;
				asTagInterface->GetOwnedGameplayTags(Tags);
				Tags = Tags.Filter(FTerminalBallisticsTags::Get().PhysMatTag.GetSingleTagContainer());
				const FGameplayTag& Tag = Tags.First();
				if (!Tags.IsEmpty())
				{
					const auto& Mappings = GetDefault<UTBConfiguration>()->PhysMats;
					for (const auto& Mapping : Mappings)
					{
						if (Mapping.PhysMatProperties.GameplayTag == Tag)
						{
							if (Mapping.HasPhysicalMaterialAsset())
							{
								HitResult.PhysMaterial = Mapping.PhysicalMaterial.Get();
							}
							bIsBone |= TB::PhysMatHelpers::IsBone(Mapping.PhysMatProperties);
							bIsBone |= TB::PhysMatHelpers::IsFlesh(Mapping.PhysMatProperties);
							bIsHitZone |= bIsBone || bIsFlesh;
							SetOutputPtrs();
							return Mapping.PhysMatProperties;
						}
					}
				}
			}

			FPhysMatProperties Properties;

			bool SetDefault = false;
			bool SetFromMaterial = false;
			bool HasDefaultPhysMaterial = false;
			if (HitResult.PhysMaterial.IsValid())
			{
				HasDefaultPhysMaterial = HitResult.PhysMaterial.Get() == GEngine->DefaultPhysMaterial;
				if (HasDefaultPhysMaterial) // The physical material was the default, let's double check using it's material (texture)
				{
					SetFromMaterial = true;
				}
				else
				{
					Success = FPhysMatProperties::GetFromPhysicalMaterial(HitResult.PhysMaterial, Properties);
					if (Success)
					{
						bIsBone |= TB::PhysMatHelpers::IsBone(Properties);
						bIsFlesh |= TB::PhysMatHelpers::IsFlesh(Properties);
						bIsHitZone |= bIsBone || bIsFlesh;
						SetOutputPtrs();
						return Properties;
					}
					else
					{
						SetFromMaterial = true;
					}
				}
			}

			const bool NoPhysicalMaterial = !HitResult.PhysMaterial.IsValid() || (HitResult.GetComponent() && !HitResult.GetComponent()->bReturnMaterialOnMove);

			if (SetFromMaterial || NoPhysicalMaterial)
			{
				int32 zero = 0;
				bool FoundPhysMat = false;
				TWeakObjectPtr<UPhysicalMaterial> physMat = GetPhysicalMaterialFromHitResult(HitResult, FoundPhysMat);
				if (FoundPhysMat)
				{
					HitResult.PhysMaterial = physMat.Get();
					Success = FPhysMatProperties::GetFromPhysicalMaterial(HitResult.PhysMaterial, Properties);
					if (Success)
					{
						bIsBone |= TB::PhysMatHelpers::IsBone(Properties);
						bIsFlesh |= TB::PhysMatHelpers::IsFlesh(Properties);
						bIsHitZone |= bIsBone || bIsFlesh;
						SetOutputPtrs();
						return Properties;
					}
					else
					{
						SetDefault = !NoPhysicalMaterial; // Only set to default if there wasn't a physical material
					}
				}
				else
				{
					SetDefault = true;
				}
			}
			else
			{
				SetDefault = true;
			}

			if (SetDefault)
			{
				HitResult.PhysMaterial = MakeWeakObjectPtr(DefaultMapping.HasPhysicalMaterialAsset() ? TObjectPtr<UPhysicalMaterial>(DefaultMapping.PhysicalMaterial.Get()) : GEngine->DefaultPhysMaterial);
				Success = FPhysMatProperties::GetFromPhysicalMaterial(HitResult.PhysMaterial, Properties);
				if (Success)
				{
					bIsBone |= TB::PhysMatHelpers::IsBone(Properties);
					bIsFlesh |= TB::PhysMatHelpers::IsFlesh(Properties);
					bIsHitZone |= bIsBone || bIsFlesh;
					SetOutputPtrs();
					return Properties;
				}
			}
		}

		bIsHitZone |= TB::PhysMatHelpers::IsFleshOrBone(SurfaceType);
		if (bIsHitZone)
		{
			const auto TBConfig = GetDefault<UTBConfiguration>();
			const auto& Flesh = UTBPhysMatManager::GetPropertiesForPresets().Flesh.GetValue();
			FGameplayTag Tag = Flesh.GameplayTag;
			if (bIsBone)
			{
				const auto& Bone = UTBPhysMatManager::GetPropertiesForPresets().Bone.GetValue();
				Tag = Bone.GameplayTag;
				const FTBPhysMatMapping* FoundMapping = TBConfig->PhysMats.FindByPredicate([Bone](const FTBPhysMatMapping& Mapping) { return Mapping.PhysMatProperties == Bone; });
				HitResult.PhysMaterial = FoundMapping ? FoundMapping->PhysicalMaterial.Get() : DefaultMapping.PhysicalMaterial.Get();
				SetOutputPtrs();
				return FoundMapping ? FoundMapping->PhysMatProperties : DefaultMapping.PhysMatProperties;
			}
			else if (bIsFlesh)
			{
				const FTBPhysMatMapping* FoundMapping = TBConfig->PhysMats.FindByPredicate([Flesh](const FTBPhysMatMapping& Mapping) { return Mapping.PhysMatProperties == Flesh; });
				HitResult.PhysMaterial = FoundMapping ? FoundMapping->PhysicalMaterial.Get() : DefaultMapping.PhysicalMaterial.Get();
				SetOutputPtrs();
				return FoundMapping ? FoundMapping->PhysMatProperties : DefaultMapping.PhysMatProperties;
			}
		}
		SetOutputPtrs();
		return FPhysMatProperties::GetDefault();
	}
	
	LLM_DEFINE_TAG(TBSweepSingle);
	bool SweepSingle(const UWorld* World, const FCollisionShape& CollisionShape, const FQuat& Rot, FHitResult& OutHit, FVector Start, FVector End, ECollisionChannel TraceChannel, const FCollisionQueryParams& Params, const FCollisionResponseParams& ResponseParams, const FCollisionObjectQueryParams& ObjectParams)
	{
		LLM_SCOPE_BYTAG(TBSweepSingle);
		if (!World)
		{
			ensure(false);
			return false;
		}

		auto IsValidPhysMat = [&](auto PhysMat)
		{
			return PhysMat != nullptr && PhysMat != GEngine->DefaultPhysMaterial;
		};

		bool bHit = false;
		if (CollisionShape.IsNearlyZero())
		{
			 bHit = FPhysicsInterface::RaycastSingle(World, OutHit, Start, End, TraceChannel, Params, ResponseParams, ObjectParams);
			 const auto PhysMat = OutHit.PhysMaterial.Get();
			 if (Params.bTraceComplex || !IsValidPhysMat(PhysMat))
			 {
				 auto NewParams = Params;
				 NewParams.bTraceComplex = false;
				 NewParams.bReturnPhysicalMaterial = true;
			 }
		}
		
		return FPhysicsInterface::GeomSweepSingle(World, CollisionShape, Rot, OutHit, Start, End, TraceChannel, Params, ResponseParams, ObjectParams);
	}

	bool DoesShapeOverlap(const FHitResult& HitResult, const FVector& Location, const FCollisionShape& CollisionShape, const FQuat& Rot)
	{
		if (IsEngineExitRequested())
		{
			return false;
		}
		const UPrimitiveComponent* HitComponent = HitResult.GetComponent();
		TB_RET_COND_VALUE(!HitComponent || HitComponent->IsBeingDestroyed() || !IsValid(HitComponent), false);
		if(const USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(HitComponent))
		{
			FName BoneName = HitResult.BoneName;
			if (BoneName == NAME_None)
			{
				BoneName = SkeletalMeshComponent->FindClosestBone(Location, nullptr, 0, true);
			}

			if (FBodyInstance* BodyInstance = SkeletalMeshComponent->GetBodyInstance(BoneName))
			{
				return BodyInstance->OverlapTest(Location, Rot, CollisionShape);
			}
		}

		if (HitComponent->GetBodyInstance() && HitComponent->GetBodyInstance()->IsValidBodyInstance())
		{
			return HitComponent->OverlapComponent(Location, Rot, CollisionShape);
		}
		else
		{
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DoesShapeOverlap), true);
			FCollisionResponseParams ResponseParams = CollisionPresets::SoloResponseChannel(HitComponent->GetCollisionObjectType(), ECR_Overlap);
			TArray<FHitResult> Hits;
			Hits.Reserve(10);
			const UWorld* World = HitComponent->GetWorld();
			const FVector Direction = (HitResult.TraceEnd - HitResult.TraceStart).GetSafeNormal();
			World->SweepMultiByChannel(Hits, Location - (CollisionShape.GetExtent().GetMax() * Direction), Location - (CollisionShape.GetExtent().GetMax() * Direction), Rot, HitComponent->GetCollisionObjectType(), CollisionShape, QueryParams, ResponseParams);
			Hits.Shrink();
			if (Hits.IsEmpty())
			{
				return false;
			}

			for (const FHitResult& Hit : Hits)
			{
				if (Hit.GetComponent() == HitComponent)
				{
					return true;
				}
			}
		}
		return false;
	}

	bool FindExitFromComponentWithoutBodyInstance(FPath Path, UPrimitiveComponent* ComponentToFindExitFrom, const double Radius, FExitResult& ExitResult, bool bTraceComplex, ECollisionChannel TraceChannel)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TB::FindExitFromComponentWithoutBodyInstance);

		ExitResult.Reset(true);
		bool ExitFound = false;
		TB_RET_COND_VALUE(!ComponentToFindExitFrom, ExitFound);
		const UWorld* World = ComponentToFindExitFrom->GetWorld();
		TB_RET_COND_VALUE(!World, ExitFound);

		FVector End = Path.GetPointAlongPath(2.0 * ComponentToFindExitFrom->Bounds.SphereRadius + 10);

		const FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FindExitFromComponentWithoutBodyInstance), bTraceComplex);
		QueryParams.bReturnPhysicalMaterial = true;
		FCollisionResponseParams ResponseParams = CollisionPresets::SoloResponseChannel(TraceChannel, ECR_Overlap);

		TArray<FHitResult> Hits;
		if (!ExitResult.Component)
		{
			ExitResult.Component = ComponentToFindExitFrom;
			Hits.Reserve(10);
			World->SweepMultiByChannel(Hits, Path.GetPointAlongPath(-2.0 * Radius), Path.PathStart, FQuat::Identity, TraceChannel, Shape, QueryParams);
			Hits.Shrink();
			TB_RET_COND_VALUE(Hits.IsEmpty(), ExitFound);
			AActor* OwningActor = ComponentToFindExitFrom->GetOwner();
			TArray<AActor*> HitActors;
			TArray<UPrimitiveComponent*> HitComponents;
			bool found = false;
			for (const FHitResult& Hit : Hits)
			{
				if (Hit.GetComponent() == ComponentToFindExitFrom)
				{
					found = true;
					ExitResult.HitResult = Hit;
					ExitResult.ImpactPoint = Hit.Location;
				}
				else
				{
					AActor* HitActor = Hit.GetActor();
					if (HitActor && HitActor != OwningActor)
					{
						HitActors.AddUnique(HitActor);
						if (Hit.GetComponent())
						{
							HitComponents.AddUnique(Hit.GetComponent());
						}
					}
				}
			}
			Hits.Empty();
			TB_RET_COND_VALUE(!found, false);
			QueryParams.AddIgnoredActors(HitActors);
			QueryParams.AddIgnoredComponents(HitComponents);
		}

		if (USkeletalMeshComponent* AsSkeletalMeshComponent = Cast<USkeletalMeshComponent>(ComponentToFindExitFrom))
		{
			FClosestPointOnPhysicsAsset CPPA;
			bool Found = AsSkeletalMeshComponent->GetClosestPointOnPhysicsAsset(End, CPPA, false);
			if (Found)
			{
				End = Path.GetClosestPointOnPath(CPPA.ClosestWorldPosition);
				Path.NudgePointAlongPath(End, 2.0 * Radius);
			}
		}

		Hits.Reserve(10);
		World->SweepMultiByChannel(Hits, End, Path.PathStart, FQuat::Identity, TraceChannel, Shape, QueryParams, ResponseParams);
		Hits.Shrink();
		if (Hits.Num() > 0)
		{
			bool found = false;
			FHitResult HitResult(NoInit);
			double Best = DOUBLE_BIG_NUMBER;
			double Dist = 0;
			for (const FHitResult& Hit : Hits)
			{
				Dist = FVector::DistSquared(Path.PathStart, Hit.ImpactPoint);
				if (!FMath::IsNearlyZero(Dist) && Hit.GetComponent() == ComponentToFindExitFrom && Dist < Best)
				{
					Best = Dist;
					HitResult = Hit;
					found = true;
				}
			}
			Hits.Empty();
			if (found)
			{
				ExitResult.ExitHitResult = HitResult;
				ExitResult.CalcExitVarsFromExitResult();
			}
			return found;
		}
		else
		{
			Hits.Empty();
			return false;
		}
	}

	bool FindExitFromSkeletalMeshComponent(FPath Path, USkeletalMeshComponent* SkeletalMeshComponent, const double Radius, FExitResult& ExitResult, bool bTraceComplex, ECollisionChannel TraceChannel)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TB::FindExitFromSkeletalMeshComponent);

		ExitResult.Reset(true);
		bool ExitFound = false;
		TB_RET_COND_VALUE(!SkeletalMeshComponent, false);
		const UWorld* World = SkeletalMeshComponent->GetWorld();
		TB_RET_COND_VALUE(!World, false);

		FVector End = Path.GetPointAlongPath(2.0 * SkeletalMeshComponent->Bounds.SphereRadius + 10);

		const FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FindExitFromSkeletalMeshComponent), bTraceComplex);
		QueryParams.bReturnPhysicalMaterial = true;
		FCollisionResponseParams ResponseParams = CollisionPresets::SoloResponseChannel(TraceChannel, ECR_Overlap);

		if (!ExitResult.Component)
		{
			const bool hit = SweepSingle(World, Shape, FQuat::Identity, ExitResult.HitResult, Path.GetPointAlongPath(-Radius), Path.GetPointAlongPath(Radius), TraceChannel, QueryParams);
			if (!hit)
			{
				return FindExitFromComponentWithoutBodyInstance(Path, SkeletalMeshComponent, Radius, ExitResult, bTraceComplex, TraceChannel);
			}
			ExitResult.Component = SkeletalMeshComponent;
			ExitResult.ImpactPoint = Path.PathStart;
		}

		FName BoneName = ExitResult.HitResult.BoneName;
		if (BoneName == NAME_None)
		{
			BoneName = SkeletalMeshComponent->FindClosestBone(ExitResult.ImpactPoint, nullptr, 0, true);
		}
		if (BoneName == NAME_None)
		{
			return FindExitFromComponentWithoutBodyInstance(Path, SkeletalMeshComponent, Radius, ExitResult, bTraceComplex, TraceChannel);
		}

		FBodyInstance* BodyInstance = SkeletalMeshComponent->GetBodyInstance(BoneName);
		if (!BodyInstance)
		{
			return FindExitFromComponentWithoutBodyInstance(Path, SkeletalMeshComponent, Radius, ExitResult, bTraceComplex, TraceChannel);
		}

		FBox Box = BodyInstance->GetBodyBounds();
		End = Box.GetClosestPointTo(End);

		FClosestPointOnPhysicsAsset CPPA;
		bool Found = SkeletalMeshComponent->GetClosestPointOnPhysicsAsset(End, CPPA, false);
		if (Found)
		{
			End = Path.GetClosestPointOnPath(CPPA.ClosestWorldPosition);
			Path.NudgePointAlongPath(End, 2.0 * Radius + 0.1);
		}
		else
		{
			TB_LOG(Error, TEXT("CPPA wasn't found."));
		}


		ExitFound = BodyInstance->Sweep(ExitResult.ExitHitResult, End, Path.PathStart, FQuat::Identity, Shape, true);
		if (ExitFound)
		{
			ExitResult.CalcExitVarsFromExitResult();
			//ExitResult.Nudge(Path.GetDirectionVector(), Radius * 2.0);
			return true;
		}
		else
		{
			return FindExitFromComponentWithoutBodyInstance(Path, SkeletalMeshComponent, Radius, ExitResult, bTraceComplex, TraceChannel);
		}
	}

	FVector FindExitFromComponent(const FVector& PathStart, const FVector& PathDirection, UPrimitiveComponent* ComponentToFindExitFrom, const double Radius, bool& ExitFound, double Resolution, const bool bTraceComplex, bool bUseSecondaryCollision, ECollisionChannel SecondaryTraceChannel)
	{
		FExitResult ExitResult = FExitResult(PathStart, ComponentToFindExitFrom);
		ExitFound = FindExitFromComponent(PathStart, PathDirection, ComponentToFindExitFrom, Radius, ExitResult, Resolution, bTraceComplex, bUseSecondaryCollision, SecondaryTraceChannel);
		return ExitResult.ExitLocation;
	}

	FVector FindExitFromComponent(const FVector& PathStart, const FVector& PathDirection, UPrimitiveComponent* ComponentToFindExitFrom, const double Radius, bool& ExitFound, FHitResult& HitResult, double Resolution, bool bTraceComplex, bool bUseSecondaryCollision, ECollisionChannel SecondaryTraceChannel)
	{
		FExitResult ExitResult = FExitResult(PathStart, ComponentToFindExitFrom);
		ExitFound = FindExitFromComponent(PathStart, PathDirection, ComponentToFindExitFrom, Radius, ExitResult, Resolution, bTraceComplex, bUseSecondaryCollision, SecondaryTraceChannel);
		HitResult = ExitResult.ExitHitResult;
		return ExitResult.ExitLocation;
	}

	/* TODO: Optimize */
	bool FindExitFromComponent(const FVector& PathStart, const FVector& PathDirection, UPrimitiveComponent* ComponentToFindExitFrom, const double Radius, FExitResult& ExitResult, double Resolution, bool bTraceComplex, bool bUseSecondaryCollision, ECollisionChannel SecondaryTraceChannel)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TB::FindExitFromComponent);

		ExitResult.Reset(true);
		bool ExitFound = false;
		TB_RET_COND_VALUE(!ComponentToFindExitFrom, ExitFound);
		const UWorld* World = ComponentToFindExitFrom->GetWorld();
		TB_RET_COND_VALUE(!World, ExitFound);

		if (Resolution == 1.0)
		{
			const double BoundsMin = abs(ComponentToFindExitFrom->Bounds.GetBox().GetExtent().GetMin());
			if (BoundsMin < Resolution * 2)
			{
				Resolution = BoundsMin / 2;
			}
		}

		FPath Path = FPath(PathDirection, PathStart);

		if (USkeletalMeshComponent* AsSkeletalMesh = Cast<USkeletalMeshComponent>(ComponentToFindExitFrom))
		{
			return FindExitFromSkeletalMeshComponent(Path, AsSkeletalMesh, Radius, ExitResult, bTraceComplex, SecondaryTraceChannel);
		}

		FVector End = Path.GetPointAlongPath(2.0 * ComponentToFindExitFrom->Bounds.SphereRadius + 10);
		const double PathDist = Path.GetDistanceAlongPath(End);
		if (PathDist < 0.0)
		{
			End = Path.GetPointAlongPath(abs(PathDist));
		}

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FindExitFromComponent), bTraceComplex);

		const bool HasValidBodyInstance = ComponentToFindExitFrom->GetBodyInstance() && ComponentToFindExitFrom->GetBodyInstance()->IsValidBodyInstance();
		if (!HasValidBodyInstance)
		{
			return FindExitFromComponentWithoutBodyInstance(Path, ComponentToFindExitFrom, Radius, ExitResult, bTraceComplex, SecondaryTraceChannel);
		}

		if (ComponentToFindExitFrom->BodyInstance.GetBodyBounds().IsInsideOrOn(End)) // Make sure we're not going to be tracing from inside the component.
		{
			const double Max = abs(ComponentToFindExitFrom->BodyInstance.GetBodyBounds().GetSize().GetMax());
			End = Path.GetPointAlongPath(Max + 5);
		}

		if (!Path.IsAfterPoint(End, PathStart))
		{
			const double distOnPath = Path.GetDistanceAlongPath(End);
			End = Path.GetPointAlongPath(abs(distOnPath));
		}

		const FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
		FHitResult TempHitResult;
		if (!ExitResult.Component)
		{
			ExitResult.Component = ComponentToFindExitFrom;
			ExitResult.ImpactPoint = PathStart;
			bool found = ComponentToFindExitFrom->BodyInstance.Sweep(ExitResult.HitResult, PathStart - (PathDirection * 3.0), PathStart + (PathDirection * 3.0), FQuat::Identity, Shape, bTraceComplex);
			if (!found)
			{
				found = ComponentToFindExitFrom->BodyInstance.Sweep(ExitResult.HitResult, PathStart - (PathDirection * 3.0), PathStart + (PathDirection * 3.0), FQuat::Identity, Shape, false); // Try again, without complex collision
			}
			if (found)
			{
				bool tempHit = ComponentToFindExitFrom->BodyInstance.LineTrace(TempHitResult, ExitResult.HitResult.ImpactPoint - PathDirection, PathStart + (PathDirection * 3.0), bTraceComplex, true); // One more trace, just to extract the physical material.
				auto TryAssignPhysMat = [&]()
				{
					if (tempHit && TempHitResult.PhysMaterial.IsValid())
					{
						ExitResult.HitResult.PhysMaterial = TempHitResult.PhysMaterial;
						return true;
					}
					return false;
				};
				if(!TryAssignPhysMat())
				{
					tempHit = ComponentToFindExitFrom->BodyInstance.LineTrace(TempHitResult, ExitResult.HitResult.ImpactPoint - PathDirection, PathStart + (PathDirection * 3.0), false, true);
					TryAssignPhysMat();
				}
			}
		}
		if (ExitResult.HitResult.PhysMaterial.IsExplicitlyNull() || (ExitResult.HitResult.PhysMaterial.IsValid() && ExitResult.HitResult.PhysMaterial.Get() == GEngine->DefaultPhysMaterial.Get()))
		{
			bool success = false;
			ExitResult.HitResult.PhysMaterial = GetPhysicalMaterialFromHitResult(ExitResult.HitResult, success);
		}

		ExitFound = ComponentToFindExitFrom->BodyInstance.Sweep(ExitResult.ExitHitResult, End, PathStart - (PathDirection * 3.0), FQuat::Identity, Shape, bTraceComplex);

		if (ExitFound)
		{
			ExitResult.CalcExitVarsFromExitResult();
		}
		else
		{
			TRACE_BOOKMARK(TEXT("Terminal Ballistics: No Exit Found."));
			return false;
		}

		if (Resolution <= 0)
		{
			return true;
		}

		const double First = FVector::Dist(PathStart, ExitResult.ExitHitResult.ImpactPoint);
		double Attempt = Resolution;
		FVector CurrentLocation = PathStart;
		Path.NudgePointAlongPath(CurrentLocation, Radius);
		FHitResult HitResult;
		const ECollisionChannel CollisionChannel = ComponentToFindExitFrom->GetCollisionObjectType();
		int NumAttempts = 0;
		while (Attempt < First && NumAttempts < 50) // Keep testing for empty space, going further away from PathStart each time.
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(TB::FindExitFromComponent::Loop);
			NumAttempts++;
			Path.NudgePointAlongPath(CurrentLocation, Resolution);
			if (ComponentToFindExitFrom->BodyInstance.LineTrace(TempHitResult, CurrentLocation, CurrentLocation - PathDirection, bTraceComplex))
			{
				continue;
			}
			else
			{
				if (ComponentToFindExitFrom->BodyInstance.Sweep(HitResult, CurrentLocation - (Radius * PathDirection), PathStart, FQuat::Identity, Shape, bTraceComplex) && HitResult.GetComponent() && !HitResult.bStartPenetrating)
				{
					const double dist = FVector::Dist(PathStart, HitResult.ImpactPoint);
					if (HitResult.GetComponent() == ComponentToFindExitFrom)
					{
						if (dist < First)
						{
							if (!HitResult.PhysMaterial.Get() && HasValidBodyInstance)
							{
								ComponentToFindExitFrom->BodyInstance.LineTrace(TempHitResult, HitResult.ImpactPoint, PathStart, bTraceComplex, true);
								HitResult.PhysMaterial = TempHitResult.PhysMaterial;
							}
							ExitResult.ExitHitResult = HitResult;
							ExitResult.CalcExitVarsFromExitResult();
							return true;
						}
					}
					else
					{
						if (dist < First)
						{
							if (!bUseSecondaryCollision || (bUseSecondaryCollision && HitResult.GetComponent()->GetCollisionResponseToChannel(SecondaryTraceChannel) == ECR_Block))
							{
								ComponentToFindExitFrom->BodyInstance.LineTrace(TempHitResult, HitResult.ImpactPoint, PathStart, bTraceComplex, true);
								HitResult.PhysMaterial = TempHitResult.PhysMaterial;
								ExitResult.ExitHitResult = HitResult;
								ExitResult.CalcExitVarsFromExitResult();
								return true;
							}
						}
						QueryParams.AddIgnoredComponent(HitResult.GetComponent());
					}
				}
			}
			Attempt += Resolution;
		}
		if (!ExitResult.HasValidPhysicalMaterial())
		{
			bool temp;
			if (UPrimitiveComponent* Component = ExitResult.HitResult.GetComponent())
			{
				ExitResult.HitResult.PhysMaterial = GetPhysicalMaterialFromHitResult(ExitResult.HitResult, temp);
			}
			if (UPrimitiveComponent* Component = ExitResult.ExitHitResult.GetComponent())
			{
				ExitResult.ExitHitResult.PhysMaterial = GetPhysicalMaterialFromHitResult(ExitResult.ExitHitResult, temp);
			}
		}
		/* Just give it a tiny nudge to ensure depenetration */
		ExitResult.NudgeExit(Path.GetDirectionVector(), 1e-3);
		return ExitFound;
	}

	LLM_DEFINE_TAG(TBGetExitLocation);
	FExitResults GetExitLocation(const UObject* WorldContextObject, const FHitResult& HitResult, const double Radius, bool& bExitFound, bool& bMultipleHits, ECollisionChannel TraceChannel)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TB::GetExitLocation);
		LLM_SCOPE_BYTAG(TBGetExitLocation);

		bExitFound = false;
		bMultipleHits = false;
		const UWorld* World = WorldContextObject->GetWorld();
		UPrimitiveComponent* Component = HitResult.GetComponent();
		if (!World || !Component)
		{
			TB_LOG(Error, TEXT("Invalid World or Component. World: %s   Component: %s"), *GetNameSafe(World), *GetNameSafe(Component));
			return FExitResults();
		};

		const FVector StartLocation = HitResult.ImpactPoint;
		FPath Path = FPath::FromTwoPoints(HitResult.TraceStart, HitResult.TraceEnd);
		Path.PathStart = StartLocation;
		const FVector Direction = Path.GetDirectionVector();

		FExitResults Results = FExitResults(StartLocation, Direction);
		Results.Reserve(10);

		const FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);

		FExitResult PotentialResult;
		bExitFound = FindExitFromComponent(StartLocation, Direction, Component, Radius, PotentialResult, 1.0, true, true, TraceChannel);
		if (CVarBallisticsDebugDraw.GetValueOnAnyThread())
		{
			GameThreadTask([World, PotentialResult]()
			{
				if (World)
				{
					DrawDebugPoint(World, PotentialResult.ImpactPoint, 5.f, FColor::Blue, false, 30.f);
					DrawDebugPoint(World, PotentialResult.ExitLocation, 5.f, FColor::Purple, false, 30.f);
				}
			});
		}
		TB_RET_COND_VALUE(!bExitFound, Results);

		Results.AddResult(PotentialResult);
		FVector Exit = PotentialResult.ExitLocation;

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GetExitLocation), true);
		QueryParams.bReturnPhysicalMaterial = true;
		QueryParams.bReturnFaceIndex = true;

		FCollisionObjectQueryParams ObjQueryParams = CollisionPresets::DefaultCollsionObjectQueryParams;
		ObjQueryParams.AddObjectTypesToQuery(Component->GetCollisionObjectType());

		bool bHit;
		int iterations = 0;
		const int maxIterations = TB::Configuration::MaxPenetrations - 1;
		FExitResult PreviousResult = PotentialResult;
		FExitResult NewExitResult;
		FHitResult hitResult, tempResult;
		double PreviousDistFromStart = Path.GetDistanceAlongPath(PotentialResult.ExitHitResult.Location);
		do
		{
			bHit = SweepSingle(World, Sphere, FQuat::Identity, hitResult, PreviousResult.ExitLocation + (Direction * Radius + 0.1), PreviousResult.ExitLocation + (Direction * 2.0 * Radius), TraceChannel, QueryParams); // Figure out if the previous exit location was empty or not.
			if (!bHit || !hitResult.GetComponent())
			{
				break;
			}

			// If bHit is true, then the resulting FHitResult can be taken as the impact hit result for the next FExitResult.
			NewExitResult = FExitResult(hitResult.Location, hitResult.GetComponent(), hitResult);

			const double NewDistFromStart = Path.GetDistanceAlongPath(hitResult.Location);
			if (NewDistFromStart < PreviousDistFromStart)
			{
				break;
			}
			if (FMath::IsNearlyZero(abs(NewDistFromStart - PreviousDistFromStart), 1e-3) || HitResultsAreEqualStrict(hitResult, PreviousResult.HitResult, false, true, true, true, true, 1e-3)) // Hitting the same spot as last time...
			{
				break;
			}
			PreviousDistFromStart = NewDistFromStart;

			FVector endLoc = NewExitResult.ImpactPoint + (Direction * (NewExitResult.Component->Bounds.SphereRadius * 2.0 + 10.0));
			bHit = hitResult.GetComponent()->BodyInstance.Sweep(hitResult, endLoc, NewExitResult.ImpactPoint, FQuat::Identity, Sphere, true); // Find the exit location for the next component.
			if (!bHit)
			{
				break;
			}
			if (!hitResult.PhysMaterial.Get() && NewExitResult.Component->BodyInstance.IsValidBodyInstance())
			{
				NewExitResult.Component->BodyInstance.LineTrace(tempResult, endLoc, NewExitResult.ImpactPoint - Direction, true, true);
				if (tempResult.PhysMaterial.IsExplicitlyNull() || tempResult.PhysMaterial == GEngine->DefaultPhysMaterial)
				{
					NewExitResult.Component->BodyInstance.LineTrace(tempResult, endLoc, NewExitResult.ImpactPoint - Direction, false, true); // Try again, but without complex collision. (PhysMatOverride sometimes will kick in)
				}
				hitResult.PhysMaterial = tempResult.PhysMaterial;
			}
			hitResult.ImpactPoint += Direction * 1e-3;
			hitResult.Location += Direction * 1e-3;

			NewExitResult.ExitHitResult = hitResult;
			Path.MoveToPath(NewExitResult);

			if (NewExitResult.PenetrationThickness > TB::Configuration::MinPenetrationDepth) // Ignore the hit if the penetration depth is too small.
			{
				Results.AddResult(NewExitResult);
			}
			if (CVarBallisticsDebugDraw.GetValueOnAnyThread())
			{
				GameThreadTask([World, hitResult]()
				{
					if (World)
					{
						DrawDebugPoint(World, hitResult.ImpactPoint, 5.f, FColor::Green, false, 30.f);
					}
				});
			}
			PreviousResult = NewExitResult;
			++iterations;
		}
		while (bHit && iterations < maxIterations && PreviousResult.ExitHitResult.GetComponent());

		Results.Shrink();

		Results.TryShrink();

		bMultipleHits = Results.Num() > 1;
		Results.ExitResults.Last().NudgeExit(Direction, Radius); // Depenetrate
		if(Results.Num() == 0)
		{
			TB_LOG(Error, TEXT("WHY"));
		}
		return Results;
	}
};


#pragma region SimTask Latent Info
template<typename TaskResultType = FBulletTaskResult>
class TSimTaskLatentAction : public FPendingLatentAction
{
public:
	using AtomicBool = std::atomic_bool;

	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;

	AtomicBool bIsComplete = false;
	TArray<FPredictProjectilePathPointData>& Results;

	float ElapsedTime = 0.f;

	TSimTaskLatentAction(const FLatentActionInfo& LatentInfo, TArray<FPredictProjectilePathPointData>& InResults)
		: ExecutionFunction(LatentInfo.ExecutionFunction)
		, OutputLink(LatentInfo.Linkage)
		, CallbackTarget(LatentInfo.CallbackTarget)
		, Results(InResults)
	{
		Results.Empty();
	}

	virtual void UpdateOperation(FLatentResponse& Response) override final
	{
		ElapsedTime += Response.ElapsedTime();
		if (bIsComplete)
		{
			Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
			return;
		}
		else if (ElapsedTime > TB::Configuration::MaximumProjectileSimulationDuration + 1.0)
		{
			Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
		}
	}

	void OnComplete(const TaskResultType& Result)
	{
		Results = Result.PathData;
		bIsComplete = true;
	}
};

template<typename TaskResultType, typename ImpactParamType>
class TSimTaskLatentActionWithCallbacks : public FPendingLatentAction
{
public:
	using AtomicBool = std::atomic_bool;

	FName ExecutionFunction;
	int32 CompleteLink;
	int32 HitLink;
	int32 ExitHitLink;
	int32 InjureLink;
	FWeakObjectPtr CallbackTarget;

	TArray<FPredictProjectilePathPointData> Results;
	ImpactParamType& HitImpactParams;
	ImpactParamType& ExitHitImpactParams;
	ImpactParamType& InjureImpactParams;
	FTBProjectileInjuryParams& InjureParams;

	AtomicBool bIsComplete = false;
	AtomicBool bHitTrigger = false;
	AtomicBool bExitHitTrigger = false;
	AtomicBool bInjureTrigger = false;

	float ElapsedTime = 0.f;

	TSimTaskLatentActionWithCallbacks(const FLatentActionInfo& LatentInfo, TArray<FPredictProjectilePathPointData>& InResults, ImpactParamType& HitImpactParams, ImpactParamType& ExitHitImpactParams, ImpactParamType& InjureImpactParams, FTBProjectileInjuryParams& InjureParams)
		: ExecutionFunction(LatentInfo.ExecutionFunction)
		, CompleteLink(LatentInfo.Linkage)
		, HitLink(LatentInfo.Linkage + 1)
		, ExitHitLink(LatentInfo.Linkage + 2)
		, InjureLink(LatentInfo.Linkage + 3)
		, CallbackTarget(LatentInfo.CallbackTarget)
		, HitImpactParams(HitImpactParams)
		, ExitHitImpactParams(ExitHitImpactParams)
		, InjureImpactParams(InjureImpactParams)
		, InjureParams(InjureParams)
	{
		Results.Empty();
	}

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		ElapsedTime += Response.ElapsedTime();

		if (bHitTrigger)
		{
			bHitTrigger = false;
			Response.TriggerLink(ExecutionFunction, HitLink, CallbackTarget);
		}
		if (bExitHitTrigger)
		{
			bExitHitTrigger = false;
			Response.TriggerLink(ExecutionFunction, ExitHitLink, CallbackTarget);
		}
		if (bInjureTrigger)
		{
			bInjureTrigger = false;
			Response.TriggerLink(ExecutionFunction, InjureLink, CallbackTarget);
		}

		if(bIsComplete)
		{
			Response.FinishAndTriggerIf(true, ExecutionFunction, CompleteLink, CallbackTarget);
		}
		else
		{
			Response.DoneIf(ElapsedTime > TB::Configuration::MaximumProjectileSimulationDuration + 1.0);
		}
	}

	void OnComplete(const TaskResultType& Result)
	{
		Results = Result.PathData;
		bIsComplete = true;
	}

	void OnHit(const ImpactParamType& ImpactParams)
	{
		HitImpactParams = ImpactParams;
		bHitTrigger = true;
	}

	void OnExitHit(const ImpactParamType& ImpactParams)
	{
		ExitHitImpactParams = ImpactParams;
		bExitHitTrigger = true;
	}
	
	void OnInjure(const ImpactParamType& ImpactParams, const FTBProjectileInjuryParams& InjuryParams)
	{
		InjureImpactParams = ImpactParams;
		InjureParams = InjuryParams;
		bInjureTrigger = true;
	}

};
#pragma endregion


TArray<FVector> UTerminalBallisticsStatics::DistributePointsOnSphere(const double SphereRadius, const int NumPoints)
{
	if (NumPoints <= 0)
	{
		return {};
	}
	if (NumPoints == 250)
	{
		TArray<FVector> ToReturn = TB::DistributePointsOnSphere::Unit_OffsetFibLattice_250;
		if (SphereRadius == 1.0)
		{
			return ToReturn;
		}
		else
		{
			for (FVector& Point : ToReturn)
			{
				Point *= SphereRadius;
			}
		}
	}
	return TB::DistributePointsOnSphere::OffsetFibLattice(SphereRadius, NumPoints);
}

#pragma region PhysicsUtils
double UTerminalBallisticsStatics::GetAltitude(const UObject* WorldContextObject, const FVector& Location)
{
	if (WorldContextObject)
	{
		if (UEnvironmentSubsystem* EnvironmentSubsystem = WorldContextObject->GetWorld()->GetSubsystem<UEnvironmentSubsystem>())
		{
			return EnvironmentSubsystem->GetAltitude(Location);
		}
	}
	return Location.Z == 0.0 ? 0.0 : Location.Z / 100.0;
}

double UTerminalBallisticsStatics::GetCurrentTemperature(const UObject* WorldContextObject, const FVector& Location)
{
	if (WorldContextObject)
	{
		if (UEnvironmentSubsystem* EnvironmentSubsystem = WorldContextObject->GetWorld()->GetSubsystem<UEnvironmentSubsystem>())
		{
			return EnvironmentSubsystem->GetTemperatureAtLocation(Location);
		}
	}
	return 10.0;
}

double UTerminalBallisticsStatics::GetRelativeHumidity(const UObject* WorldContextObject, const FVector& Location)
{
	if (WorldContextObject)
	{
		if (UEnvironmentSubsystem* EnvironmentSubsystem = WorldContextObject->GetWorld()->GetSubsystem<UEnvironmentSubsystem>())
		{
			return EnvironmentSubsystem->GetRelativeHumidityAtLocation(Location);
		}
	}
	return 0.5;
}

double UTerminalBallisticsStatics::ConvertToNewPressureUnit(ETBPressureUnit OldUnit, ETBPressureUnit NewUnit, double Pressure)
{
	return TB::UnitConversions::ConvertPressureUnits(Pressure, OldUnit, NewUnit);
}

double UTerminalBallisticsStatics::CalculateDynamicPressure(const double Density, const double Speed) // 1/2pv^2
{
	return TB::CalculateDynamicPressure(Density, Speed);
}

double UTerminalBallisticsStatics::CalculateKineticEnergy(const double Mass, const double Speed) // KE = 1/2mv^2
{
	return TB::CalculateKineticEnergy(Mass, Speed);
}

double UTerminalBallisticsStatics::CalculateDragForce(double cd, double csa, double speed, double windSpeed, double p)
{
	return TB::Drag::CalculateDragForce(cd, csa, speed, windSpeed, p);
}

FVector UTerminalBallisticsStatics::CalculateDragForce3D(const FVector& cd, const FVector& csa, const FVector& velocity, const FVector& windVelocity, const FRotator& orientation, double p)
{
	return TB::Drag::CalculateDragForce(cd, csa, velocity, windVelocity, orientation.Quaternion(), p);
}

double UTerminalBallisticsStatics::ConvertPowerToWork(double Power, double dt)
{
	UE_ASSUME(dt > 0.0);
	if (dt > 0.0)
	{
		return Power / dt;
	}
	else
	{
		return 0.0;
	}
}

FVector UTerminalBallisticsStatics::ConvertPowerToWork(FVector Power, double dt)
{
	return FVector(ConvertPowerToWork(Power.X, dt), ConvertPowerToWork(Power.Y, dt), ConvertPowerToWork(Power.Z, dt));
}

double UTerminalBallisticsStatics::CalculateVelocityChangeFromWork(double Work, double InitialSpeed, double Mass)
{
	UE_ASSUME(Mass > 0.0);
	const long double sqrtM = sqrt(Mass);
	const long double mvSqr = Mass * TB_SQR(InitialSpeed);
	UE_ASSUME(sqrtM > 0.0 && mvSqr > 0.0);
	const long double numerator = sqrt(2 * Work + mvSqr);
	return numerator / sqrtM;
}

FVector UTerminalBallisticsStatics::CalculateVelocityChangeFromWork(FVector Work, FVector InitialVelocity, double Mass)
{
	const double dVx = CalculateVelocityChangeFromWork(Work.X, InitialVelocity.X, Mass);
	const double dVy = CalculateVelocityChangeFromWork(Work.Y, InitialVelocity.Y, Mass);
	const double dVz = CalculateVelocityChangeFromWork(Work.Z, InitialVelocity.Z, Mass);
	return FVector(dVx, dVy, dVz);
}

FVector UTerminalBallisticsStatics::CalculateVelocityChangeFromWork(double Work, FVector WorkDirection, FVector InitialVelocity, double Mass)
{
	UE_ASSUME(Work > 0.0 && Mass > 0.0);
	const FVector WorkVector = Work * WorkDirection;
	return CalculateVelocityChangeFromWork(WorkVector, InitialVelocity, Mass);
}

double UTerminalBallisticsStatics::CalculateVelocityChangeFromPower(double Power, double InitialSpeed, double Mass, double dt)
{
	UE_ASSUME(Power > 0.0 && dt > 0.0);
	const double Work = ConvertPowerToWork(Power, dt);
	return CalculateVelocityChangeFromWork(Work, InitialSpeed, Mass);
}

double UTerminalBallisticsStatics::CalculateVelocityChangeFromPower(double Power, FVector PowerDirection, double InitialSpeed, double Mass, double dt)
{
	UE_ASSUME(Power > 0.0 && dt > 0.0);
	const double Work = ConvertPowerToWork(Power, dt);
	return CalculateVelocityChangeFromWork(Work * PowerDirection.X, InitialSpeed, Mass);
}

FVector UTerminalBallisticsStatics::CalculateVelocityChangeFromPower(FVector Power, FVector InitialVelocity, double Mass, double dt)
{
	const FVector Work = ConvertPowerToWork(Power, dt);
	return CalculateVelocityChangeFromWork(Power, InitialVelocity, Mass);
}

double UTerminalBallisticsStatics::GetTerminalVelocity(double m, double g, FVector c, FVector a, FRotator r, double p)
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

double UTerminalBallisticsStatics::CalculateAirPressureAtAltitude(const double g, const double h, const double T, const double P0)
{
	return TB::CalculateAirPressureAtAltitude(g, h, T, P0);
}

double UTerminalBallisticsStatics::CalculateAirDensity(double p, double T, double Rh)
{
	using namespace TB::Constants;
	using namespace TB::UnitConversions;
	const double T_Celsius = KelvinToCelsius(T);
	if (Rh > 0.0)
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

double UTerminalBallisticsStatics::CalculateAirDensityAtAltitude(const double g, const double h, const double T, double P0, double Rh)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTerminalBallisticsStatics::CalculateAirDensityAtAltitude);
	if (P0 < 0)
	{
		P0 = TB::Constants::ReferencePressure_SeaLevel;
	}
	if (Rh < 0 || Rh > 1)
	{ // If relative humidity is of bounds [0, 1], then fall back to environment defaults
		Rh = GetDefault<UTBConfiguration>()->EnvironmentData.WorldHumidity;
	}
	return TB::CalculateAirDensityAtAltitude(g, h, T, P0, Rh);
}

double UTerminalBallisticsStatics::GetWorkFromForce(double F, double s) // W = F*dS
{
	return F * s;
}
#pragma endregion

#pragma region Overlaps
bool UTerminalBallisticsStatics::OverlapMultiSortedByActor(UWorld* World, TMap<AActor*, FTBOverlapArray>& OverlapsSortedByActor, const FVector& Location, const FQuat& Rotation, const FTBCollisionParams& CollisionParams, const double Radius)
{
	return OverlapMultiSortedByActorWithShape(World, OverlapsSortedByActor, Location, Rotation, CollisionParams, FCollisionShape::MakeSphere(Radius));
}

bool UTerminalBallisticsStatics::OverlapMultiSortedByActorWithShape(UWorld* World, TMap<AActor*, FTBOverlapArray>& OverlapsSortedByActor, const FVector& Location, const FQuat& Rotation, const FTBCollisionParams& CollisionParams, const FCollisionShape& CollisionShape)
{
	TArray<FOverlapResult> overlaps;
	bool anyHits = World->OverlapMultiByObjectType(overlaps, Location, Rotation, CollisionParams.ObjectQueryParams, CollisionShape, CollisionParams.QueryParams);

	AActor* currentActor;
	FTBOverlapArray* currentOverlaps;

	for (const FOverlapResult& overlap : overlaps) // Group overlaps by actor
	{
		currentActor = overlap.GetActor();
		if (TB_VALID_OBJECT(currentActor))
		{
			currentOverlaps = OverlapsSortedByActor.Find(currentActor);
			if (currentOverlaps != nullptr)
			{
				currentOverlaps->Overlaps.Add(overlap);
			}
			else
			{
				OverlapsSortedByActor.Add(currentActor, FTBOverlapArray({ overlap }));
			}
		}
	}

	return anyHits;
}

bool UTerminalBallisticsStatics::OverlapsObjectWithTag(UWorld* World, const FVector& Location, const FTBCollisionParams& CollisionParams, const double Radius, const FName& tag)
{
	TArray<FOverlapResult> overlaps;
	bool anyHits = World->OverlapMultiByObjectType(overlaps, Location, FQuat(), CollisionParams.ObjectQueryParams, FCollisionShape::MakeSphere(Radius), CollisionParams.QueryParams);

	if (anyHits)
	{
		AActor* currentActor;
		UPrimitiveComponent* currentComponent;
		for (const FOverlapResult& overlap : overlaps)
		{
			currentActor = overlap.GetActor();
			currentComponent = overlap.GetComponent();
			if (TB_VALID_OBJECT(currentActor))
			{
				if (currentActor->ActorHasTag(tag))
				{
					return true;
				}
				else if (TB_VALID_OBJECT(currentComponent) && currentComponent->ComponentHasTag(tag))
				{
					return true;
				}
			}
		}
	}

	return false;
}

FOverlapResult UTerminalBallisticsStatics::GetClosestOverlap(const FTBOverlapArray& Overlaps, FVector Location)
{
	double dist = -1;
	double currentDist;
	FOverlapResult ClosestOverlap;
	for (const FOverlapResult& overlap : Overlaps.Overlaps)
	{
		if (!overlap.GetComponent())
		{
			continue;
		}
		currentDist = FVector::Dist(Location, overlap.GetComponent()->GetComponentLocation());
		if (dist == -1 || dist > currentDist)
		{
			dist = currentDist;
			ClosestOverlap = overlap;
		}
	}
	return dist == -1 ? FOverlapResult() : ClosestOverlap;
}
#pragma endregion

#pragma region FTBLocationAndDirection
FTBLocationAndDirection UTerminalBallisticsStatics::MakeLocationAndDirection(const FVector Location, const FRotator Direction)
{
	return FTBLocationAndDirection(Location, Direction);
}

FTBLocationAndDirection UTerminalBallisticsStatics::MakeLocationAndDirectionFromTransform(const FTransform Transform)
{
	return FTBLocationAndDirection(Transform);
}

FTBLocationAndDirection UTerminalBallisticsStatics::MakeLocationAndDirectionFromVectors(const FVector Location, const FVector Direction)
{
	return FTBLocationAndDirection(Location, Direction);
}

void UTerminalBallisticsStatics::BreakLocationAndDirection(const FTBLocationAndDirection& LocationAndDirection, FVector& Location, FVector& Direction, FRotator& Rotation)
{
	Location = LocationAndDirection.Location;
	Direction = LocationAndDirection.Direction;
}

FTransform UTerminalBallisticsStatics::LocationAndDirectionToTransform(const FTBLocationAndDirection LocationAndDirection)
{
	return FTransform(LocationAndDirection.GetDirectionAsQuat(), LocationAndDirection.Location);
}
#pragma endregion

#pragma region FTBLaunchParams
FTBLaunchParams UTerminalBallisticsStatics::MakeLaunchParams(
	double ProjectileSpeed,
	double EffectiveRange,
	double Timescale,
	FVector FireLocation,
	FRotator FireDirection,
	TArray<AActor*> ToIgnore,
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes,
	TEnumAsByte<ECollisionChannel> TraceChannel,
	bool bIgnoreOwner,
	bool bAddToOwnerVelocity,
	bool bForceNoTracer,
	AActor* Owner,
	AController* Instigator,
	ETBProjectileSimulationType SimType,
	const double OwnerIgnoreDistance
)
{
	if (ObjectTypes.IsEmpty())
	{
		ObjectTypes = CollisionPresets::DefaultCollisionQueryTypes;
	}
	return FTBLaunchParams(Owner, Instigator, ProjectileSpeed, EffectiveRange, FTBLocationAndDirection(FireLocation, FireDirection), ToIgnore, ObjectTypes, TraceChannel, bIgnoreOwner, bAddToOwnerVelocity, bForceNoTracer, SimType, Timescale, OwnerIgnoreDistance);
}

FTBLaunchParams UTerminalBallisticsStatics::MakeLaunchParamsWithDirectionVector(
	const double ProjectileSpeed,
	const double EffectiveRange,
	const double Timescale,
	const FVector FireLocation,
	const FVector FireDirection,
	const TArray<AActor*> ToIgnore,
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes,
	TEnumAsByte<ECollisionChannel> TraceChannel,
	bool bIgnoreOwner,
	bool bAddToOwnerVelocity,
	bool bForceNoTracer,
	AActor* Owner,
	AController* Instigator,
	ETBProjectileSimulationType SimType,
	double OwnerIgnoreDistance
)
{
	if (ObjectTypes.IsEmpty())
	{
		ObjectTypes = CollisionPresets::DefaultCollisionQueryTypes;
	}
	return FTBLaunchParams(Owner, Instigator, ProjectileSpeed, EffectiveRange, FTBLocationAndDirection(FireLocation, FireDirection), ToIgnore, ObjectTypes, TraceChannel, bIgnoreOwner, bAddToOwnerVelocity, bForceNoTracer, SimType, Timescale, OwnerIgnoreDistance);
}

void UTerminalBallisticsStatics::BreakLaunchParams(
	const FTBLaunchParams& LaunchParams,
	double& ProjectileSpeed,
	double& EffectiveRange,
	double& Timescale,
	double& OwnerIgnoreDistance,
	FVector& FireLocation,
	FRotator& FireDirection,
	TArray<AActor*>& ToIgnore,
	TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes,
	TEnumAsByte<ECollisionChannel>& TraceChannel,
	bool& bIgnoreOwner,
	bool& bAddToOwnerVelocity,
	bool& bForceNoTracer,
	AActor*& Owner,
	AController*& Instigator,
	ETBProjectileSimulationType& SimType
)
{
	ProjectileSpeed = LaunchParams.ProjectileSpeed;
	EffectiveRange = LaunchParams.EffectiveRange;
	Timescale = LaunchParams.Timescale;
	OwnerIgnoreDistance = LaunchParams.OwnerIgnoreDistance;
	FireLocation = LaunchParams.FireTransform.Location;
	FireDirection = LaunchParams.FireTransform.GetDirectionAsRotator();
	ToIgnore = LaunchParams.ToIgnore;
	ObjectTypes = LaunchParams.ObjectTypes;
	TraceChannel = LaunchParams.TraceChannel;
	bIgnoreOwner = LaunchParams.IgnoreOwner;
	bAddToOwnerVelocity = LaunchParams.AddToOwnerVelocity;
	bForceNoTracer = LaunchParams.bForceNoTracer;
	Owner = LaunchParams.Owner;
	Instigator = LaunchParams.Instigator;
	SimType = LaunchParams.SimType;
}

TArray<FTBLaunchParams> UTerminalBallisticsStatics::GenerateLaunchParamsDistributedOnSphere(const FTBLaunchParams & BaseLaunchParams, const int32 Num, const double SphereRadius)
{
	if (Num <= 0)
	{
		return {};
	}
	const TArray<FVector> PointsOnSphere = DistributePointsOnSphere(SphereRadius, Num);
	TArray<FTBLaunchParams> LaunchParamsArr;
	LaunchParamsArr.Init(FTBLaunchParams(), Num);

	EParallelForFlags Flags = Num > 50 ? EParallelForFlags::None : EParallelForFlags::ForceSingleThread;
	ParallelFor(Num, [&](int32 i) {
		TRACE_CPUPROFILER_EVENT_SCOPE(UTerminalBallisticsStatics::GenerateLaunchParamsDistributedOnSphere);
		FTBLaunchParams LaunchParams(BaseLaunchParams);
		LaunchParams.FireTransform.Direction = PointsOnSphere[i];
		LaunchParamsArr[i] = LaunchParams;
		}, Flags);

	LaunchParamsArr.Shrink();
	return LaunchParamsArr;
}
#pragma endregion

FString UTerminalBallisticsStatics::ProjectileIdToString(const FTBProjectileId& Id)
{
	return Id.Guid.ToString();
}

#pragma region Projectiles
FTBProjectileId UTerminalBallisticsStatics::AddAndFireBullet(AActor* Owner, TSoftObjectPtr<class UBulletDataAsset> Bullet, FTBLaunchParams& LaunchParams, const FTBProjectileId& Id, int32 DebugType)
{
	if (Owner)
	{
		if (UTerminalBallisticsSubsystem* TBSubsystem = FTBGetSubsystemHelper::GetTBSubsystem(Owner))
		{
			UBulletDataAsset* BulletDataAsset = Bullet.LoadSynchronous();
			if (TB_VALID(BulletDataAsset))
			{
				if (!LaunchParams.Owner)
				{
					LaunchParams.Owner = Owner;
				}
				return TBSubsystem->AddAndFireBullet(BulletDataAsset, LaunchParams, Id, Owner, DebugType);
			}
		}
	}
	return FTBProjectileId::None;
}

FTBProjectileId UTerminalBallisticsStatics::AddAndFireBulletWithCallbacks(
	AActor* Owner,
	TSoftObjectPtr<UBulletDataAsset> Bullet,
	FTBLaunchParams& LaunchParams,
	FBPOnProjectileComplete OnBulletComplete,
	FBPOnBulletHit OnBulletHit,
	FBPOnBulletExitHit OnBulletExitHit,
	FBPOnBulletInjure OnBulletInjure,
	const FTBProjectileId& Id,
	int32 DebugType
)
{
	if (Owner)
	{
		if (UTerminalBallisticsSubsystem* TBSubsystem = FTBGetSubsystemHelper::GetTBSubsystem(Owner))
		{
			UBulletDataAsset* BulletDataAsset = Bullet.LoadSynchronous();
			if (TB_VALID(BulletDataAsset))
			{
				if (!LaunchParams.Owner)
				{
					LaunchParams.Owner = Owner;
				}
				return TBSubsystem->AddAndFireBulletWithCallbacks(BulletDataAsset, LaunchParams, OnBulletComplete, OnBulletHit, OnBulletExitHit, OnBulletInjure, Owner, Id, DebugType);
			}
		}
	}
	return FTBProjectileId::None;
}

FTBProjectileId UTerminalBallisticsStatics::AddAndFireBulletWithCallbacksAndUpdate(
	AActor* Owner,
	TSoftObjectPtr<UBulletDataAsset> Bullet,
	FTBLaunchParams& LaunchParams,
	FBPOnProjectileComplete OnBulletComplete,
	FBPOnBulletHit OnBulletHit,
	FBPOnBulletExitHit OnBulletExitHit,
	FBPOnBulletInjure OnBulletInjure,
	FBPOnProjectileUpdate OnUpdate,
	const FTBProjectileId& Id,
	int32 DebugType
)
{
	if (Owner)
	{
		if (UTerminalBallisticsSubsystem* TBSubsystem = FTBGetSubsystemHelper::GetTBSubsystem(Owner))
		{
			UBulletDataAsset* BulletDataAsset = Bullet.LoadSynchronous();
			if (TB_VALID(BulletDataAsset))
			{
				if (!LaunchParams.Owner)
				{
					LaunchParams.Owner = Owner;
				}
				return TBSubsystem->AddAndFireBulletWithCallbacksAndUpdate(BulletDataAsset, LaunchParams, OnBulletComplete, OnBulletHit, OnBulletExitHit, OnBulletInjure, OnUpdate, Owner, Id, DebugType);
			}
		}
	}
	return FTBProjectileId::None;
}

FTBProjectileId UTerminalBallisticsStatics::AddAndFireProjectile(AActor* Owner, const FTBProjectile& Projectile, const FPhysMatProperties& ProjectilePhysicalProperties, FTBLaunchParams& LaunchParams, const FTBProjectileId& Id, int32 DebugType)
{
	if (Owner)
	{
		if (UTerminalBallisticsSubsystem* TBSubsystem = FTBGetSubsystemHelper::GetTBSubsystem(Owner))
		{
			FTBProjectileSimData simData = FTBProjectileSimData(Projectile, ProjectilePhysicalProperties, DebugType);
			simData.SetId(Id);
			simData.Owner = Owner;
			return TBSubsystem->AddAndFireProjectile(simData, LaunchParams, Owner);
		}
	}
	return FTBProjectileId::None;
}

FTBProjectileId UTerminalBallisticsStatics::AddAndFireProjectileWithCallbacks(
	AActor* Owner,
	const FTBProjectile& Projectile,
	const FPhysMatProperties& ProjectilePhysicalProperties,
	FTBLaunchParams& LaunchParams,
	FBPOnProjectileComplete OnProjectileComplete,
	FBPOnProjectileHit OnProjectileHit,
	FBPOnProjectileExitHit OnProjectileExitHit,
	FBPOnProjectileInjure OnProjectileInjure,
	const FTBProjectileId& Id,
	int32 DebugType
)
{
	if (Owner)
	{
		if (UTerminalBallisticsSubsystem* TBSubsystem = FTBGetSubsystemHelper::GetTBSubsystem(Owner))
		{
			FTBProjectileSimData simData = FTBProjectileSimData(Projectile, ProjectilePhysicalProperties, DebugType);
			simData.SetId(Id);
			simData.Owner = Owner;
			TBSubsystem->AddAndFireProjectileWithCallbacks(simData, LaunchParams, OnProjectileComplete, OnProjectileHit, OnProjectileExitHit, OnProjectileInjure, Owner, DebugType);
		}
	}
	return FTBProjectileId::None;
}

FTBProjectileId UTerminalBallisticsStatics::AddAndFireProjectileWithCallbacksAndUpdate(
	AActor* Owner,
	const FTBProjectile& Projectile,
	const FPhysMatProperties& ProjectilePhysicalProperties,
	FTBLaunchParams& LaunchParams,
	FBPOnProjectileComplete OnProjectileComplete,
	FBPOnProjectileHit OnProjectileHit,
	FBPOnProjectileExitHit OnProjectileExitHit,
	FBPOnProjectileInjure OnProjectileInjure,
	FBPOnProjectileUpdate OnUpdate,
	const FTBProjectileId& Id,
	int32 DebugType
)
{
	if (Owner)
	{
		if (UTerminalBallisticsSubsystem* TBSubsystem = FTBGetSubsystemHelper::GetTBSubsystem(Owner))
		{
			FTBProjectileSimData simData = FTBProjectileSimData(Projectile, ProjectilePhysicalProperties, DebugType);
			simData.SetId(Id);
			simData.Owner = Owner;
			TBSubsystem->AddAndFireProjectileWithCallbacksAndUpdate(simData, LaunchParams, OnProjectileComplete, OnProjectileHit, OnProjectileExitHit, OnProjectileInjure, OnUpdate, Owner, DebugType);
		}
	}
	return FTBProjectileId::None;
}

TArray<FTBProjectileId> UTerminalBallisticsStatics::AddAndFireProjectiles(AActor* Owner, TArray<FTBProjectile> Projectiles, const FPhysMatProperties& ProjectilePhysicalProperties, TArray<FTBLaunchParams> LaunchParams, TArray<FTBProjectileId>& Ids, int32 DebugType)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTerminalBallisticsStatics::AddAndFireProjectiles);
	if (Projectiles.Num() == LaunchParams.Num() && Projectiles.Num() > 0)
	{
		Ids.SetNum(Projectiles.Num()); // Handles the cases where no Ids were provided, or not enough were.
		if (Owner)
		{
			if (UTerminalBallisticsSubsystem* TBSubsystem = FTBGetSubsystemHelper::GetTBSubsystem(Owner))
			{
				TArray<FTBProjectileId> Out;
				Out.AddUninitialized(Projectiles.Num());
				ParallelFor(Projectiles.Num(), [&](int32 i) {
					TRACE_CPUPROFILER_EVENT_SCOPE(UTerminalBallisticsStatics::AddAndFireProjectiles_ParallelFor);
					const FTBProjectile Projectile = Projectiles[i];
					FTBProjectileSimData simData = FTBProjectileSimData(Projectile, ProjectilePhysicalProperties, DebugType);
					simData.SetId(Ids[i]);
					const FTBProjectileId Id = TBSubsystem->AddAndFireProjectile(simData, LaunchParams[i], Owner, DebugType);
					Out[i] = Id;
				}, EParallelForFlags::Unbalanced);
				Out.Shrink();
				return Out;
			}
		}
	}
	return { FTBProjectileId::None };
}

TArray<FTBProjectileId> UTerminalBallisticsStatics::AddAndFireProjectilesWithCallbacks(AActor* Owner, TArray<FTBProjectile> Projectiles, const FPhysMatProperties& ProjectilePhysicalProperties, TArray<FTBLaunchParams> LaunchParams, TArray<FTBProjectileId>& Ids, FBPOnProjectileComplete OnProjectileComplete, FBPOnProjectileHit OnProjectileHit, FBPOnProjectileExitHit OnProjectileExitHit, FBPOnProjectileInjure OnProjectileInjure, int32 DebugType)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTerminalBallisticsStatics::AddAndFireProjectiles);
	if (Projectiles.Num() == LaunchParams.Num() && Projectiles.Num() > 0)
	{
		Ids.SetNum(Projectiles.Num()); // Handles the cases where no Ids were provided, or not enough were.
		if (Owner)
		{
			if (UTerminalBallisticsSubsystem* TBSubsystem = FTBGetSubsystemHelper::GetTBSubsystem(Owner))
			{
				TArray<FTBProjectileId> Out;
				Out.SetNumUninitialized(Projectiles.Num());
				ParallelFor(Projectiles.Num(), [&](int32 i) {
					TRACE_CPUPROFILER_EVENT_SCOPE(UTerminalBallisticsStatics::AddAndFireProjectiles_ParallelFor);
					const FTBProjectile Projectile = Projectiles[i];
					FTBProjectileSimData simData = FTBProjectileSimData(Projectile, ProjectilePhysicalProperties, DebugType);
					simData.SetId(Ids[i]);
					const FTBProjectileId Id = TBSubsystem->AddAndFireProjectileWithCallbacks(simData, LaunchParams[i], OnProjectileComplete, OnProjectileHit, OnProjectileExitHit, OnProjectileInjure, Owner, DebugType);
					Out[i] = Id;
				}, EParallelForFlags::Unbalanced);
				Out.Shrink();
				return Out;
			}
		}
	}
	return { FTBProjectileId::None };
}

TArray<FTBProjectileId> UTerminalBallisticsStatics::AddAndFireProjectilesWithCallbacksAndUpdate(AActor* Owner, TArray<FTBProjectile> Projectiles, const FPhysMatProperties& ProjectilePhysicalProperties, TArray<FTBLaunchParams> LaunchParams, TArray<FTBProjectileId>& Ids, FBPOnProjectileComplete OnProjectileComplete, FBPOnProjectileHit OnProjectileHit, FBPOnProjectileExitHit OnProjectileExitHit, FBPOnProjectileInjure OnProjectileInjure, FBPOnProjectileUpdate OnUpdate, int32 DebugType)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTerminalBallisticsStatics::AddAndFireProjectiles);
	if (Projectiles.Num() == LaunchParams.Num() && Projectiles.Num() > 0)
	{
		Ids.SetNum(Projectiles.Num()); // Handles the cases where no Ids were provided, or not enough were.
		if (Owner)
		{
			if (UTerminalBallisticsSubsystem* TBSubsystem = FTBGetSubsystemHelper::GetTBSubsystem(Owner))
			{
				TArray<FTBProjectileId> Out;
				Out.SetNumUninitialized(Projectiles.Num());
				ParallelFor(Projectiles.Num(), [&](int32 i) {
					TRACE_CPUPROFILER_EVENT_SCOPE(UTerminalBallisticsStatics::AddAndFireProjectiles_ParallelFor);
					const FTBProjectile Projectile = Projectiles[i];
					FTBProjectileSimData simData = FTBProjectileSimData(Projectile, ProjectilePhysicalProperties, DebugType);
					simData.SetId(Ids[i]);
					const FTBProjectileId Id = TBSubsystem->AddAndFireProjectileWithCallbacksAndUpdate(simData, LaunchParams[i], OnProjectileComplete, OnProjectileHit, OnProjectileExitHit, OnProjectileInjure, OnUpdate, Owner, DebugType);
					Out[i] = Id;
				}, EParallelForFlags::Unbalanced);
				Out.Shrink();
				return Out;
			}
		}
	}
	return { FTBProjectileId::None };
}

FTBProjectileId UTerminalBallisticsStatics::AddAndFireTestBullet(AActor* Owner, TSoftObjectPtr<class UBulletDataAsset> Bullet, FTBLaunchParams& LaunchParams, TArray<FPredictProjectilePathPointData>& Results, FLatentActionInfo LatentInfo, const FTBProjectileId& Id)
{
	if(Owner)
	{
		if (UWorld* World = Owner->GetWorld())
		{
			if (UTerminalBallisticsSubsystem* TBSubsystem = FTBGetSubsystemHelper::GetTBSubsystem(Owner))
			{
				TSimTaskLatentAction<FBulletTaskResult>* LatentAction = new TSimTaskLatentAction<FBulletTaskResult>(LatentInfo, Results);
				FTBBulletSimData SimData = TBSubsystem->CreateBulletSimDataFromDataAsset(Bullet.LoadSynchronous(), Id);
				SimData.CompletionDelegate.AddRaw(LatentAction, &TSimTaskLatentAction<FBulletTaskResult>::OnComplete);
				SimData.IsTestTask = true;
				if (!SimData.Owner.IsValid())
				{
					SimData.Owner = Owner;
				}
				const FTBProjectileId AssignedId = TBSubsystem->AddAndFireBullet(SimData, LaunchParams, Owner);
				World->GetLatentActionManager().AddNewAction(Owner, LatentInfo.UUID, LatentAction);
				return AssignedId;
			}
		}
	}
	return FTBProjectileId::None;
}

template<typename TaskType>
class TSimTaskCompletionDummy
{
	TFunction<void(const TArray<FPredictProjectilePathPointData>&)>& CompletionFunction;
public:

	TSimTaskCompletionDummy() = delete;
	TSimTaskCompletionDummy(const TSimTaskCompletionDummy&) = delete;

	TSimTaskCompletionDummy(TFunction<void(const TArray<FPredictProjectilePathPointData>&)>& InCompletionFunction)
		: CompletionFunction(InCompletionFunction)
	{}

	void OnComplete(const TaskType& Result)
	{
		CompletionFunction(Result.PathData);
	}
};

FTBProjectileId UTerminalBallisticsStatics::AddAndFireTestBullet(AActor* Owner, TSoftObjectPtr<UBulletDataAsset> Bullet, FTBLaunchParams& LaunchParams, TFunction<void(const TArray<FPredictProjectilePathPointData>&)> CompletionFunction, const FTBProjectileId& Id)
{
	if (Owner)
	{
		if (UWorld* World = Owner->GetWorld())
		{
			if (UTerminalBallisticsSubsystem* TBSubsystem = FTBGetSubsystemHelper::GetTBSubsystem(Owner))
			{
				TSimTaskCompletionDummy<FBulletTaskResult> CompletionDummy = TSimTaskCompletionDummy<FBulletTaskResult>(CompletionFunction);
				FTBBulletSimData SimData = TBSubsystem->CreateBulletSimDataFromDataAsset(Bullet.LoadSynchronous(), Id);
				SimData.CompletionDelegate.AddRaw(&CompletionDummy, &TSimTaskCompletionDummy<FBulletTaskResult>::OnComplete);
				SimData.IsTestTask = true;
				if (!SimData.Owner.IsValid())
				{
					SimData.Owner = Owner;
				}
				return TBSubsystem->AddAndFireBullet(SimData, LaunchParams, Owner);
			}
		}
	}
	return FTBProjectileId::None;
}
#pragma endregion



FTBWoundCavity UTerminalBallisticsStatics::CalculateCavitationRadii(const double ProjectileSpeed, const double ImpartedEnergy, const double PenetrationDepth, const double ObjectThickness, const FTBProjectilePhysicalProperties& ProjectileProperties, const FPhysMatProperties& ProjectilePhysicalProperties, const FPhysMatProperties& ObjectPhysicalProperties)
{
	return TB::BallisticFunctions::CalculateCavitationRadii(ProjectileSpeed, ImpartedEnergy, PenetrationDepth, ObjectThickness, ProjectileProperties, ProjectilePhysicalProperties, ObjectPhysicalProperties);
}

FVector UTerminalBallisticsStatics::FindExitFromComponent(const UObject* WorldContextObject, const FVector& PathStart, const FVector& PathDirection, UPrimitiveComponent* ComponentToFindExitFrom, const double Radius, bool& ExitFound, double Resolution, bool bTraceComplex, bool bUseSecondaryCollision, ECollisionChannel SecondaryTraceChannel)
{
	return TB::FindExitFromComponent(PathStart, PathDirection, ComponentToFindExitFrom, Radius, ExitFound, Resolution, bTraceComplex, bUseSecondaryCollision, SecondaryTraceChannel);
}

namespace TB::DistributePointsOnSphere
{
	inline static double GoldenRatio = (1 + sqrt(5)) / 2.0;

	TArray<FVector> FibLattice(const double SphereRadius, const int N)
	{
		if (N <= 0)
		{
			return {};
		}
		TArray<FVector> Points;
		Points.AddUninitialized(N);

		double phi, theta = 0.0;
		double x, y, z = 0.0;
		double CosPhi, SinPhi = 0.0;
		double CosTheta, SinTheta = 0.0;
		for (int i = 0; i < N; i++)
		{
			phi = acos(1 - 2 * (i + 0.5) / N);
			theta = 2 * DOUBLE_PI * i / GoldenRatio;

			FMath::SinCos(&SinPhi, &CosPhi, phi);
			FMath::SinCos(&SinTheta, &CosTheta, theta);
			x = CosTheta * SinPhi;
			y = SinTheta * SinPhi;
			z = CosPhi;
			Points[i] = FVector(x, y, z) * SphereRadius;
		}
		Points.Shrink();
		return Points;
	}

	double Epsilon(const int N)
	{
		if (N >= 600000)		return 214;
		else if (N >= 400000)	return 75;
		else if (N >= 11000)	return 27;
		else if (N >= 890)		return 10;
		else if (N >= 177)		return 3.33;
		else if (N >= 24)		return 1.33;
		else					return 0.33;
	}

	TArray<FVector> OffsetFibLattice(const double SphereRadius, const int N)
	{
		if (N <= 0)
		{
			return {};
		}
		TArray<FVector> Points;
		Points.AddUninitialized(N);

		const double epsilon = Epsilon(N);

		double phi, theta = 0.0;
		double x, y, z = 0.0;
		double CosPhi, SinPhi = 0.0;
		double CosTheta, SinTheta = 0.0;
		for (int i = 0; i < N; i++)
		{
			phi = acos(1 - 2 * (i + epsilon) / (N - 1 + 2 * epsilon));
			theta = 2 * DOUBLE_PI * i / GoldenRatio;

			FMath::SinCos(&SinPhi, &CosPhi, phi);
			FMath::SinCos(&SinTheta, &CosTheta, theta);
			x = CosTheta * SinPhi;
			y = SinTheta * SinPhi;
			z = CosPhi;
			Points[i] = FVector(x, y, z) * SphereRadius;
		}
		Points.Shrink();
		return Points;
	}
};
