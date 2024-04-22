// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "Subsystems/TerminalBallisticsSubsystem.h"
#include "BallisticFunctions.h"
#include "Bullets.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "NiagaraFunctionLibrary.h"
#include "PhysicsUtils.h"
#include "SceneManagement.h"
#include "Subsystems/EnvironmentSubsystem.h"
#include "TerminalBallisticsGameModeBaseInterface.h"
#include "TerminalBallisticsLogChannels.h"
#include "TerminalBallisticsMacrosAndFunctions.h"
#include "TerminalBallisticsStatics.h"
#include "Threading/TerminalBallisticsProjectileThread.h"
#include "Types/TBProjectile.h"
#include <condition_variable>
#include <mutex>



constexpr double SimFrequency = 1.0 / 64.0;


UTerminalBallisticsSubsystem* FTBGetSubsystemHelper::TBSubsystem = nullptr;

UTerminalBallisticsSubsystem* FTBGetSubsystemHelper::GetTBSubsystem(const UObject* WorldContextObject)
{
	UWorld* World = nullptr;
	if (IsValid(WorldContextObject))
	{
		World = WorldContextObject->GetWorld();
	}
	else if (IsValid(TBSubsystem) && TBSubsystem->GetWorld())
	{
		World = TBSubsystem->GetWorld();
	}
	if (IsValid(World))
	{
		const ENetMode WorldNetMode = World->GetNetMode();
		if (UTerminalBallisticsSubsystem::SupportsNetMode(WorldNetMode))
		{
			if (IsValid(TBSubsystem) && TBSubsystem->SupportsNetMode(TBSubsystem->World->GetNetMode()))
			{
				return TBSubsystem;
			}
			else if (IsValid(WorldContextObject))
			{
				return World->GetSubsystem<UTerminalBallisticsSubsystem>();
			}
		}
	}

	return nullptr;
}


UTBProjectileThreadQueue::~UTBProjectileThreadQueue()
{
	Empty();
}

#pragma region TerminalBallisticsSubsystem
int32 UTerminalBallisticsSubsystem::ThreadCount = 0;

UTerminalBallisticsSubsystem::UTerminalBallisticsSubsystem()
	: UTickableWorldSubsystem()
{}

bool UTerminalBallisticsSubsystem::IsTickable() const
{
	return HasAnyBullets()
		|| HasAnyProjectiles()
		|| HasAnyBulletsOrProjectiles()
		|| !BulletsToMakeActive.IsEmpty()
		|| !ProjectilesToMakeActive.IsEmpty()
		|| !BulletRemovalQueue.IsEmpty()
		|| !ProjectileRemovalQueue.IsEmpty()
		|| !BulletsToAdd.IsEmpty()
		|| !ProjectilesToAdd.IsEmpty()
		|| !BulletLaunchQueue.IsEmpty()
		|| !ProjectileLaunchQueue.IsEmpty()
		|| (ProjectileThreadQueue && ProjectileThreadQueue->HasOutputData());
}

void UTerminalBallisticsSubsystem::Tick(float DeltaTime)
{
	GetOrUpdateWorld();
	if (World && ProjectileThread)
	{
		if (GetOrUpdateWorld()->IsPaused())
		{
			if (!bWasPaused)
			{
				bWasPaused = true;
				ProjectileThread->Pause();
			}
			return;
		}
		else
		{
			if (bWasPaused || (ProjectileThread->ThreadPaused() && ProjectileThread->HasActive()))
			{
				ProjectileThread->Unpause();
			}
			bWasPaused = false;
		}
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(UTerminalBallisticsSubsystem::Tick);
	ProjectilesLaunchedThisTick = 0;

	UTickableWorldSubsystem::Tick(DeltaTime);

	TB_RET_COND(bShuttingDown); // If the world is shutting down, abort

	if (ProjectileThread)
	{
		const float CurrentTimeDilation = GetOrUpdateWorld()->GetWorldSettings()->GetEffectiveTimeDilation();
		ProjectileThread->SetTimeDilation(CurrentTimeDilation);
	}

	if (!EnvironmentSubsystem)
	{
		TryGetEnvironmentSubsystem();
	}

	FTBBulletSimData bulletData;
	while (BulletsToAdd.Dequeue(bulletData))
	{
		AddBullet(bulletData, bulletData.Owner.Get());
	}
	FTBProjectileSimData projectileData;
	while (ProjectilesToAdd.Dequeue(projectileData))
	{
		AddProjectile(projectileData, projectileData.Owner.Get());
	}

	ProjectileOrId ProjectileToRemove;
	while (ProjectileRemovalQueue.Dequeue(ProjectileToRemove))
	{
		if (ProjectileToRemove.IsType<FTBProjectileId>())
		{
			RemoveProjectileById(ProjectileToRemove.Get<FTBProjectileId>());
		}
		else
		{
			RemoveProjectile(ProjectileToRemove.Get<FTBProjectileSimData>());
		}
	}

	BulletOrId BulletToRemove;
	while (BulletRemovalQueue.Dequeue(BulletToRemove))
	{
		if (BulletToRemove.IsType<FTBProjectileId>())
		{
			RemoveBulletById(BulletToRemove.Get<FTBProjectileId>());
		}
		else
		{
			RemoveBullet(BulletToRemove.Get<FTBBulletSimData>());
		}
	}

	while (BulletsToMakeActive.Dequeue(bulletData))
	{
		if (InactiveBullets.RemoveSingleSwap(bulletData))
		{
			ActiveBullets.Add(bulletData);
		}
	}
	while (ProjectilesToMakeActive.Dequeue(projectileData))
	{
		if (InactiveProjectiles.RemoveSingleSwap(projectileData))
		{
			ActiveProjectiles.Add(projectileData);
		}
	}


	FTBLaunchData launchData;
	while (CanFire() && BulletLaunchQueue.Dequeue(launchData))
	{
		FireBullet(launchData.Id, launchData.LaunchParams, launchData.DebugType);
	}
	while (CanFire() && ProjectileLaunchQueue.Dequeue(launchData))
	{
		FireProjectile(launchData.Id, launchData.LaunchParams, launchData.DebugType);
	}
	GetResultsFromProjectileThread();
}

void UTerminalBallisticsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	onBulletHit.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UTerminalBallisticsSubsystem, CallGameModeHit));
	onBulletExitHit.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UTerminalBallisticsSubsystem, CallGameModeExitHit));
	onBulletInjure.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UTerminalBallisticsSubsystem, CallGameModeInjure));

	onProjectileHit.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UTerminalBallisticsSubsystem, CallGameModeHitBasic));
	onProjectileExitHit.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UTerminalBallisticsSubsystem, CallGameModeExitHitBasic));
	onProjectileInjure.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UTerminalBallisticsSubsystem, CallGameModeInjureBasic));

	OnWorldBeginTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUFunction(this, GET_FUNCTION_NAME_CHECKED(UTerminalBallisticsSubsystem, StartShutdown));

	OnWorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUFunction(this, GET_FUNCTION_NAME_CHECKED(UTerminalBallisticsSubsystem, StartShutdown));
}

void UTerminalBallisticsSubsystem::Deinitialize()
{
	ShutdownProjectileThread();

	FWorldDelegates::OnWorldBeginTearDown.Remove(OnWorldBeginTearDownHandle);
	OnWorldBeginTearDownHandle.Reset();

	FWorldDelegates::OnWorldCleanup.Remove(OnWorldCleanupHandle);
	OnWorldCleanupHandle.Reset();

	FTBGetSubsystemHelper::TBSubsystem = nullptr;

	Super::Deinitialize();
}

bool UTerminalBallisticsSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}
	UWorld* OuterWorld = Cast<UWorld>(Outer);
	if (OuterWorld)
	{
		const ENetMode WorldNetMode = OuterWorld->GetNetMode();
		return SupportsNetMode(WorldNetMode);
	}
	return false;
}

void UTerminalBallisticsSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	GetOrUpdateWorld();
	EnvironmentSubsystem = World->GetSubsystem<UEnvironmentSubsystem>();
	if (AGameModeBase* WorldGameMode = World->GetAuthGameMode())
	{
		if (WorldGameMode->Implements<UTerminalBallisticsGameModeBaseInterface>())
		{
			bHasValidGameMode = true;
		}
	}

	InitProjectileThread();

	FTBGetSubsystemHelper::TBSubsystem = this;
}

bool UTerminalBallisticsSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return Super::DoesSupportWorldType(WorldType)
		&& WorldType != EWorldType::EditorPreview && WorldType != EWorldType::GamePreview;
}

bool UTerminalBallisticsSubsystem::HasAnyBullets() const
{
	return HasActiveBullets()
		|| HasInactiveBullets();
}

bool UTerminalBallisticsSubsystem::HasAnyProjectiles() const
{
	return HasActiveProjectiles()
		|| HasInactiveProjectiles();
}

bool UTerminalBallisticsSubsystem::HasAnyBulletsOrProjectiles() const
{
	return ProjectileThread && ProjectileThread->HasActive();
}

bool UTerminalBallisticsSubsystem::TryGetEnvironmentSubsystem()
{
	if (!EnvironmentSubsystem)
	{
		if (GetOrUpdateWorld())
		{
			EnvironmentSubsystem = World->GetSubsystem<UEnvironmentSubsystem>();
		}
	}
	return TB_VALID(EnvironmentSubsystem);
}

void UTerminalBallisticsSubsystem::StartShutdown(UWorld*, bool, bool)
{
	bShuttingDown = true;
	ShutdownProjectileThread();
}

#pragma region Drag
double UTerminalBallisticsSubsystem::UpdateProjectileDrag(const double V, const double GravityZ, const FVector& Location, const FTBProjectilePhysicalProperties& ProjectileProperties, ETBDragComplexity DragCalculationType, double p, const bool bAllowAtmosphericDensityOverride)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTerminalBallisticsSubsystem::UpdateProjectileDrag);
	if (ProjectileProperties.CheckValid())
	{
		if (!EnvironmentSubsystem)
		{
			if (!TryGetEnvironmentSubsystem())
			{
				return 0.0;
			}
		}

		if (DragCalculationType == ETBDragComplexity::DEFAULT)
		{
			DragCalculationType = GetDefault<UTBConfiguration>()->DefaultDragComplexity;
		}
		if (bAllowAtmosphericDensityOverride && DragComplexityRequiresPressure(DragCalculationType))
		{
			p = UTerminalBallisticsStatics::CalculateAirDensityAtAltitude(GravityZ / 100.0, UTerminalBallisticsStatics::GetAltitude(EnvironmentSubsystem, Location), EnvironmentSubsystem->GetTemperatureAtLocation(Location), TB::Constants::ReferencePressure_SeaLevel, EnvironmentSubsystem->GetRelativeHumidityAtLocation(Location));
		}

		using namespace TB::Drag;
		switch (DragCalculationType)
		{
		case ETBDragComplexity::BASIC:
			[[fallthrough]];
		case ETBDragComplexity::ATMOSPHERIC:
			return CalculateDragForce(ProjectileProperties, V, 0.0, p);
		case ETBDragComplexity::WIND:
			return CalculateDragForce(ProjectileProperties, V, EnvironmentSubsystem->GetWindDataAtLocation(Location).Speed / 100, p);
		case ETBDragComplexity::WIND3D:
			[[fallthrough]];
		case ETBDragComplexity::COMPLEX:
			return CalculateDragForce(ProjectileProperties, V, EnvironmentSubsystem->GetWindDataAtLocation(Location).Speed / 100, p); // This shouldn't be hit, since ETBDragComplexity > WIND requires vector inputs, but just in case...
		default: // Really shouldn't ever happen, but just in case...
			UE_ASSUME(0);
			return 0.0;
			break;
		}
	}
	else
	{
		UE_LOG(LogTerminalBallistics, Error, TEXT("Invalid ProjectileProperties"));
		return 0.0;
	}
}

FVector UTerminalBallisticsSubsystem::CalculateProjectileDrag(const FVector& V, const double GravityZ, const FVector& Location, const FRotator& ProjectileRotation, const FTBProjectilePhysicalProperties& ProjectileProperties, ETBDragComplexity DragCalculationType, double p, const bool bAllowAtmosphericDensityOverride)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTerminalBallisticsSubsystem::CalculateProjectileDrag);

	if (ProjectileProperties.CheckValid())
	{
		if (!EnvironmentSubsystem)
		{
			if (!TryGetEnvironmentSubsystem())
			{
				return FVector::ZeroVector;
			}
		}

		if (DragCalculationType == ETBDragComplexity::DEFAULT)
		{
			DragCalculationType = GetDefault<UTBConfiguration>()->DefaultDragComplexity;
		}
		if (bAllowAtmosphericDensityOverride && DragCalculationType >= ETBDragComplexity::ATMOSPHERIC)
		{
			p = UTerminalBallisticsStatics::CalculateAirDensityAtAltitude(GravityZ / 100.0, UTerminalBallisticsStatics::GetAltitude(EnvironmentSubsystem, Location), EnvironmentSubsystem->GetTemperatureAtLocation(Location), TB::Constants::ReferencePressure_SeaLevel, EnvironmentSubsystem->GetRelativeHumidityAtLocation(Location));
		}

		FWindData WindData;

		FVector ReturnValue = FVector::ZeroVector;

		const FVector v = V + (WindData.Speed / 100) * WindData.Direction;

		using namespace TB::Drag;
		switch (DragCalculationType)
		{
		case ETBDragComplexity::BASIC:
			ReturnValue.X = CalculateDragForce(ProjectileProperties, V.Size());
			break;
		case ETBDragComplexity::ATMOSPHERIC:
			ReturnValue.X = CalculateDragForce(ProjectileProperties, V.Size(), 0.0, p);
			break;
		case ETBDragComplexity::WIND:
			ReturnValue.X = CalculateDragForce(ProjectileProperties, V.Size(), EnvironmentSubsystem->GetWindDataAtLocation(Location).Speed / 100);
			break;
		case ETBDragComplexity::WIND3D:
			[[fallthrough]];
		case ETBDragComplexity::COMPLEX:
			WindData = EnvironmentSubsystem->GetWindDataAtLocation(Location);
			ReturnValue = CalculateDragForce(ProjectileProperties, V, (WindData.Speed / 100) * WindData.Direction, ProjectileRotation.Quaternion(), p);
			break;
		default: // Should never happen, but just in case...
			UE_ASSUME(0);
			break;
		}

		return ReturnValue;
	}
	else
	{
		UE_LOG(LogTerminalBallistics, Error, TEXT("Invalid ProjectileProperties"));
		return FVector::ZeroVector;
	}
}
#pragma endregion

FTBBulletSimData UTerminalBallisticsSubsystem::CreateBulletSimDataFromDataAsset(const UBulletDataAsset* BulletDataAsset, const FTBProjectileId& Id, int32 DebugType)
{
	BulletPointer bullet = FTBBullet::CreatePtrFromBulletDataAsset(BulletDataAsset);
	FTBBulletSimData SimData = FTBBulletSimData(bullet, DebugType, 0, false, Id);
	return SimData;
}

TArray<FTBBulletSimData> UTerminalBallisticsSubsystem::CreateBulletSimDataFromDataAssetMultiple(const UBulletDataAsset* BulletDataAsset, const FTBProjectileId Id, const int32 DebugType)
{
	BulletPointer bullet = FTBBullet::CreatePtrFromBulletDataAsset(BulletDataAsset);
	if (bullet->ProjectileCount > 1)
	{
		TArray<FTBBulletSimData> Bullets;
		Bullets.Reserve(bullet->ProjectileCount);
		for (int i = 0; i < bullet->ProjectileCount; i++)
		{
			Bullets.Emplace(bullet, DebugType, 0, false, Id);
		}
		return Bullets;
	}
	return { FTBBulletSimData(bullet, DebugType, 0, false, Id) };
}

void UTerminalBallisticsSubsystem::BindFunctionsToBulletSimData(FTBBulletSimData& BulletSimData, FBPOnProjectileComplete BulletComplete, FBPOnBulletHit BulletHit, FBPOnBulletExitHit BulletExitHit, FBPOnBulletInjure BulletInjure)
{
	BulletSimData.onComplete.Add(BulletComplete);
	BulletSimData.onBulletHit.Add(BulletHit);
	BulletSimData.onBulletExitHit.Add(BulletExitHit);
	BulletSimData.onBulletInjure.Add(BulletInjure);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddBulletFromDataAsset(const UBulletDataAsset* BulletDataAsset, AActor* Owner, const FTBProjectileId& Id, int32 DebugType)
{
	BulletPointer bullet = FTBBullet::CreatePtrFromBulletDataAsset(BulletDataAsset);
	FTBBulletSimData simData = FTBBulletSimData(bullet, DebugType, 0, false, Id);
	simData.Owner = Owner;
	return AddBullet(simData, Owner);
}

#pragma region AddBullet
FTBProjectileId UTerminalBallisticsSubsystem::AddBullet(FTBBulletSimData& ToAdd, AActor* Owner, AController* Instigator)
{
	if (!ToAdd.Owner.IsValid())
	{
		ToAdd.Owner = Owner;
	}
	ToAdd.Instigator = Instigator;
	if (!ToAdd.Instigator.IsValid() && ToAdd.Owner.IsValid())
	{
		ToAdd.Instigator = Owner->GetInstigatorController();
	}
	BindDefaultDelegates(ToAdd);
	ToAdd.World = World;
	ToAdd.TerminalBallisticsSubsystem = this;

	if (!ToAdd.HasBeenAssignedId) // Don't reassign the ID
	{
		FScopeLock Lock(&AddBulletMutex);
		ToAdd.GenerateId();
	}

	FWriteScopeLock Lock(InactiveBulletsLock);
	InactiveBullets.Add(ToAdd);

	return ToAdd.GetId();
}

FTBProjectileId UTerminalBallisticsSubsystem::AddBullet(BulletPointer Bullet, AActor* Owner, AController* Instigator, const FTBProjectileId& Id)
{
	FTBBulletSimData SimData(Bullet, 0);
	SimData.SetId(Id);
	return AddBullet(SimData, Owner, Instigator);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddBulletWithCallbacks(
	FTBBulletSimData& ToAdd,
	FBPOnProjectileComplete OnBulletComplete,
	FBPOnBulletHit OnBulletHit,
	FBPOnBulletExitHit OnBulletExitHit,
	FBPOnBulletInjure OnBulletInjure,
	AActor* Owner
)
{
	BindFunctionsToBulletSimData(ToAdd, OnBulletComplete, OnBulletHit, OnBulletExitHit, OnBulletInjure);
	return AddBullet(ToAdd, Owner);
}
FTBProjectileId UTerminalBallisticsSubsystem::AddBulletWithCallbacks(
	BulletPointer Bullet,
	FBPOnProjectileComplete OnBulletComplete,
	FBPOnBulletHit OnBulletHit,
	FBPOnBulletExitHit OnBulletExitHit,
	FBPOnBulletInjure OnBulletInjure,
	AActor* Owner,
	const FTBProjectileId& Id
)
{
	FTBBulletSimData SimData(Bullet, 0);
	SimData.SetId(Id);
	BindFunctionsToBulletSimData(SimData, OnBulletComplete, OnBulletHit, OnBulletExitHit, OnBulletInjure);
	return AddBullet(SimData, Owner);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddBulletWithCallbacksAndUpdate(
	FTBBulletSimData& ToAdd,
	FBPOnProjectileComplete OnBulletComplete,
	FBPOnBulletHit OnBulletHit,
	FBPOnBulletExitHit OnBulletExitHit,
	FBPOnBulletInjure OnBulletInjure,
	FBPOnProjectileUpdate OnUpdate,
	AActor* Owner
)
{
	BindFunctionsToBulletSimData(ToAdd, OnBulletComplete, OnBulletHit, OnBulletExitHit, OnBulletInjure);
	ToAdd.OnUpdateDelegate.Add(OnUpdate);
	return AddBullet(ToAdd, Owner);
}
FTBProjectileId UTerminalBallisticsSubsystem::AddBulletWithCallbacksAndUpdate(
	FTBBulletSimData& ToAdd,
	FBPOnProjectileComplete OnBulletComplete,
	FBPOnBulletHit OnBulletHit,
	FBPOnBulletExitHit OnBulletExitHit,
	FBPOnBulletInjure OnBulletInjure,
	TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
	AActor* Owner
)
{
	BindFunctionsToBulletSimData(ToAdd, OnBulletComplete, OnBulletHit, OnBulletExitHit, OnBulletInjure);
	ToAdd.OnUpdate = OnUpdateFunction;
	return AddBullet(ToAdd, Owner);
}
FTBProjectileId UTerminalBallisticsSubsystem::AddBulletWithCallbacksAndUpdate(
	FTBBulletSimData& ToAdd,
	FBPOnProjectileComplete OnBulletComplete,
	FBPOnBulletHit OnBulletHit,
	FBPOnBulletExitHit OnBulletExitHit,
	FBPOnBulletInjure OnBulletInjure,
	FBPOnProjectileUpdate OnUpdate,
	TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
	AActor* Owner
)
{
	BindFunctionsToBulletSimData(ToAdd, OnBulletComplete, OnBulletHit, OnBulletExitHit, OnBulletInjure);
	ToAdd.OnUpdateDelegate.Add(OnUpdate);
	ToAdd.OnUpdate = OnUpdateFunction;
	return AddBullet(ToAdd, Owner);
}
FTBProjectileId UTerminalBallisticsSubsystem::AddBulletWithCallbacksAndUpdate(
	BulletPointer Bullet,
	FBPOnProjectileComplete OnBulletComplete,
	FBPOnBulletHit OnBulletHit,
	FBPOnBulletExitHit OnBulletExitHit,
	FBPOnBulletInjure OnBulletInjure,
	FBPOnProjectileUpdate OnUpdate,
	AActor* Owner,
	const FTBProjectileId& Id
)
{
	FTBBulletSimData SimData(Bullet, 0);
	SimData.SetId(Id);
	BindFunctionsToBulletSimData(SimData, OnBulletComplete, OnBulletHit, OnBulletExitHit, OnBulletInjure);
	SimData.OnUpdateDelegate.Add(OnUpdate);
	return AddBullet(SimData, Owner);
}
FTBProjectileId UTerminalBallisticsSubsystem::AddBulletWithCallbacksAndUpdate(
	BulletPointer Bullet,
	FBPOnProjectileComplete OnBulletComplete,
	FBPOnBulletHit OnBulletHit,
	FBPOnBulletExitHit OnBulletExitHit,
	FBPOnBulletInjure OnBulletInjure,
	FBPOnProjectileUpdate OnUpdate,
	TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
	AActor* Owner,
	const FTBProjectileId& Id
)
{
	FTBBulletSimData SimData(Bullet, 0);
	SimData.SetId(Id);
	BindFunctionsToBulletSimData(SimData, OnBulletComplete, OnBulletHit, OnBulletExitHit, OnBulletInjure);
	SimData.OnUpdateDelegate.Add(OnUpdate);
	SimData.OnUpdate = OnUpdateFunction;
	return AddBullet(SimData, Owner);
}
FTBProjectileId UTerminalBallisticsSubsystem::AddBulletWithCallbacksAndUpdate(
	BulletPointer Bullet,
	FBPOnProjectileComplete OnBulletComplete,
	FBPOnBulletHit OnBulletHit,
	FBPOnBulletExitHit OnBulletExitHit,
	FBPOnBulletInjure OnBulletInjure,
	TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
	AActor* Owner,
	const FTBProjectileId& Id
)
{
	FTBBulletSimData SimData(Bullet, 0);
	SimData.SetId(Id);
	BindFunctionsToBulletSimData(SimData, OnBulletComplete, OnBulletHit, OnBulletExitHit, OnBulletInjure);
	SimData.OnUpdate = OnUpdateFunction;
	return AddBullet(SimData, Owner);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddBulletWithUpdate(
	FTBBulletSimData& ToAdd,
	TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
	AActor* Owner
)
{
	ToAdd.OnUpdate = OnUpdateFunction;
	return AddBullet(ToAdd, Owner);
}
FTBProjectileId UTerminalBallisticsSubsystem::AddBulletWithUpdate(
	FTBBulletSimData& ToAdd,
	FBPOnProjectileUpdate OnUpdate,
	AActor* Owner
)
{
	ToAdd.OnUpdateDelegate.Add(OnUpdate);
	return AddBullet(ToAdd, Owner);
}
FTBProjectileId UTerminalBallisticsSubsystem::AddBulletWithUpdate(
	FTBBulletSimData& ToAdd,
	FBPOnProjectileUpdate OnUpdate,
	TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
	AActor* Owner
)
{
	ToAdd.OnUpdateDelegate.Add(OnUpdate);
	ToAdd.OnUpdate = OnUpdateFunction;
	return AddBullet(ToAdd, Owner);
}
#pragma endregion

void UTerminalBallisticsSubsystem::RemoveBullet(FTBBulletSimData& ToRemove)
{
	if (HasBullet(ToRemove))
	{
		int32 Removed = 0;
		{
			FWriteScopeLock Lock(ActiveBulletsLock);
			Removed += ActiveBullets.RemoveSingleSwap(ToRemove) << 2;
		}
		{
			FWriteScopeLock Lock(InactiveBulletsLock);
			Removed += InactiveBullets.RemoveSingleSwap(ToRemove) << 1;
		}
		{
			FWriteScopeLock AddLock(BulletsToAddLock);
			Removed += BulletsToAdd.RemoveSingleSwap(ToRemove) << 0;
		}
		if (Removed == 0)
		{
			BulletOrId bulletToRemove;
			bulletToRemove.Set<FTBBulletSimData>(ToRemove);
			BulletRemovalQueue.Enqueue(bulletToRemove);
		}
		else if (ProjectileThread)
		{
			const bool bWasRemovedFromActive = Removed >> 2 == 1; // Since we set the third bit if ActiveBullets.RemoveSingleSwap removed the bullet, this will check if that happened.
			ProjectileThread->RequestProjectileTermination(ToRemove.GetId(), TB::SimTasks::ETBThreadTaskAffinity::BULLET_TASK);
		}
	}
}

void UTerminalBallisticsSubsystem::RemoveBulletById(const FTBProjectileId& Id)
{
	if (HasBullet(Id))
	{
		auto Pred = [Id](FTBBulletSimData a) { return Id == a.GetId(); };

		FWriteScopeLock ActiveLock(ActiveBulletsLock);
		FTBBulletSimData* ToRemove = ActiveBullets.FindByPredicate(Pred);
		if (ToRemove)
		{
			if (ActiveBullets.RemoveSingleSwap(*ToRemove) > 0)
			{
				if (ProjectileThread)
				{
					ProjectileThread->RequestProjectileTermination(Id, TB::SimTasks::ETBThreadTaskAffinity::BULLET_TASK);
				}
			}
		}
		FWriteScopeLock InactiveLock(InactiveBulletsLock);
		ToRemove = InactiveBullets.FindByPredicate(Pred);
		if (ToRemove)
		{
			InactiveBullets.RemoveSingleSwap(*ToRemove);
		}
		FWriteScopeLock AddLock(BulletsToAddLock);
		ToRemove = BulletsToAdd.FindByPredicate(Pred);
		if (ToRemove)
		{
			BulletsToAdd.RemoveSingleSwap(*ToRemove);
		}
	}
}

bool UTerminalBallisticsSubsystem::GetBullet(const FTBProjectileId& Id, FTBBulletSimData& Bullet) const
{
	auto Pred = [Id](FTBBulletSimData a) { return Id == a.GetId(); };
	int i = INDEX_NONE;
	{
		FReadScopeLock WriteLock(BulletsToAddLock);
		i = BulletsToAdd.IndexOfByPredicate(Pred);
		if (i != INDEX_NONE)
		{
			Bullet = *BulletsToAdd.FindByPredicate(Pred);
			return true;
		}
	}
	{
		FReadScopeLock WriteLock(ActiveBulletsLock);
		i = ActiveBullets.IndexOfByPredicate(Pred);
		if (i != INDEX_NONE)
		{
			Bullet = ActiveBullets[i];
			return true;
		}
	}
	{
		FReadScopeLock WriteLock(InactiveBulletsLock);
		i = InactiveBullets.IndexOfByPredicate(Pred);
		if (i != INDEX_NONE)
		{
			Bullet = InactiveBullets[i];
			return true;
		}
	}
	return false;
}

void UTerminalBallisticsSubsystem::FireBullet(const FTBProjectileId& BulletId, const FTBLaunchParams& LaunchParams, int32 DebugType)
{
	_FireBulletInternal(BulletId, LaunchParams, DebugType);
}

void UTerminalBallisticsSubsystem::FireBulletExplicit(FTBBulletSimData* BulletToFire, const FTBLaunchParams& LaunchParams, int32 DebugType)
{
	if (CanFire())
	{
		_FireProjectileImpl(BulletToFire, LaunchParams, DebugType);
	}
	else
	{
		ProjectileLaunchQueue.Enqueue(FTBLaunchData(LaunchParams, BulletToFire->GetId(), DebugType));
	}
}

#pragma region AddAndFireBullet
FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireBullet(FTBBulletSimData& SimData, const FTBLaunchParams& LaunchParams, AActor* Owner, int32 DebugType)
{
	const FTBProjectileId BulletId = AddBullet(SimData, Owner);
	FireBulletExplicit(&SimData, LaunchParams, DebugType);
	return BulletId;
}


TArray<FTBProjectileId> UTerminalBallisticsSubsystem::AddAndFireBullets(TArray<FTBBulletSimData>& SimData, const FTBLaunchParams& LaunchParams, int32 DebugType, TOptional<FTBBulletTaskCallbacks> Delegates)
{
	TArray<FTBProjectileId> Ids = AddBullets(SimData, { LaunchParams }, Delegates);
	TArray<FTBLaunchData> LaunchData;
	LaunchData.Reserve(Ids.Num());
	for (int i = 0; i < Ids.Num(); i++)
	{
		LaunchData.Emplace(LaunchParams, Ids[i], DebugType);
	}
	FireMultiple(TArray<FTBBulletSimData>(SimData), LaunchData); // Creating a new array since we don't know the lifetime of the reference to "SimData"
	return Ids;
}

TArray<FTBProjectileId> UTerminalBallisticsSubsystem::AddAndFireBullets(TArray<FTBBulletSimData>& SimData, const TArray<FTBLaunchParams>& LaunchParams, int32 DebugType, TOptional<FTBBulletTaskCallbacks> Delegates)
{
	TArray<FTBProjectileId> Ids = AddBullets(SimData, LaunchParams , Delegates);
	TArray<FTBLaunchData> LaunchData;
	LaunchData.Reserve(Ids.Num());
	for (int i = 0; i < Ids.Num(); i++)
	{
		LaunchData.Emplace(LaunchParams[i], Ids[i], DebugType);
	}
	FireMultiple(TArray<FTBBulletSimData>(SimData), LaunchData); // Creating a new array since we don't know the lifetime of the reference to "SimData"
	return Ids;
}

FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireBulletWithCallbacks(
	const UBulletDataAsset* BulletDataAsset,
	const FTBLaunchParams& LaunchParams,
	FBPOnProjectileComplete OnBulletComplete,
	FBPOnBulletHit OnBulletHit,
	FBPOnBulletExitHit OnBulletExitHit,
	FBPOnBulletInjure OnBulletInjure,
	AActor* Owner,
	const FTBProjectileId& Id,
	int32 DebugType
)
{
	FTBBulletSimData SimData = CreateBulletSimDataFromDataAsset(BulletDataAsset, Id, DebugType);
	BindFunctionsToBulletSimData(SimData, OnBulletComplete, OnBulletHit, OnBulletExitHit, OnBulletInjure);
	return AddAndFireBullet(SimData, LaunchParams, Owner, DebugType);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireBulletWithCallbacksAndUpdate(
	const UBulletDataAsset* BulletDataAsset,
	const FTBLaunchParams& LaunchParams,
	FBPOnProjectileComplete OnBulletComplete,
	FBPOnBulletHit OnBulletHit,
	FBPOnBulletExitHit OnBulletExitHit,
	FBPOnBulletInjure OnBulletInjure,
	FBPOnProjectileUpdate OnUpdate,
	AActor* Owner,
	const FTBProjectileId& Id,
	int32 DebugType
)
{
	FTBBulletSimData SimData = CreateBulletSimDataFromDataAsset(BulletDataAsset, Id, DebugType);
	BindFunctionsToBulletSimData(SimData, OnBulletComplete, OnBulletHit, OnBulletExitHit, OnBulletInjure);
	SimData.OnUpdateDelegate.Add(OnUpdate);
	return AddAndFireBullet(SimData, LaunchParams, Owner, DebugType);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireBulletWithCallbacks(
	BulletPointer Bullet,
	const FTBLaunchParams& LaunchParams,
	FBPOnProjectileComplete OnBulletComplete,
	FBPOnBulletHit OnBulletHit,
	FBPOnBulletExitHit OnBulletExitHit,
	FBPOnBulletInjure OnBulletInjure,
	AActor* Owner,
	const FTBProjectileId& Id,
	int32 DebugType
)
{
	FTBBulletSimData SimData(Bullet, DebugType);
	SimData.SetId(Id);
	BindFunctionsToBulletSimData(SimData, OnBulletComplete, OnBulletHit, OnBulletExitHit, OnBulletInjure);
	return AddAndFireBullet(SimData, LaunchParams, Owner, DebugType);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireBulletWithCallbacks(
	BulletPointer Bullet,
	const FTBLaunchParams& LaunchParams,
	FBPOnProjectileComplete OnBulletComplete,
	FBPOnBulletHit OnBulletHit,
	FBPOnBulletExitHit OnBulletExitHit,
	FBPOnBulletInjure OnBulletInjure,
	FBPOnProjectileUpdate OnUpdate,
	AActor* Owner,
	const FTBProjectileId& Id,
	int32 DebugType
)
{
	FTBBulletSimData SimData(Bullet, DebugType);
	SimData.SetId(Id);
	BindFunctionsToBulletSimData(SimData, OnBulletComplete, OnBulletHit, OnBulletExitHit, OnBulletInjure);
	SimData.OnUpdateDelegate.Add(OnUpdate);
	return AddAndFireBullet(SimData, LaunchParams, Owner, DebugType);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireBulletWithCallbacks(
	BulletPointer Bullet,
	const FTBLaunchParams& LaunchParams,
	TB::SimTasks::FBulletTaskDelegates Delegates,
	AActor* Owner,
	const FTBProjectileId& Id,
	int32 DebugType
)
{
	FTBBulletSimData SimData(Bullet, DebugType);
	SimData.SetId(Id);
	BindDelegates(SimData, FTBBulletTaskCallbacks(Delegates));
	return AddAndFireBullet(SimData, LaunchParams, Owner, DebugType);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireBulletWithCallbacks(
	BulletPointer Bullet,
	const FTBLaunchParams& LaunchParams,
	TB::SimTasks::FBulletTaskDelegates Delegates,
	TFunction<void(const FTBProjectileFlightData&)> OnUpdate,
	AActor* Owner,
	const FTBProjectileId& Id,
	int32 DebugType
)
{
	FTBBulletSimData SimData(Bullet, DebugType);
	SimData.SetId(Id);
	auto Callbacks = FTBBulletTaskCallbacks(Delegates);
	Callbacks.OnUpdateFunction = OnUpdate;
	BindDelegates(SimData, Callbacks);
	return AddAndFireBullet(SimData, LaunchParams, Owner, DebugType);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireBulletWithUpdate(
	BulletPointer Bullet,
	const FTBLaunchParams& LaunchParams,
	TFunction<void(const FTBProjectileFlightData&)> OnUpdate,
	AActor* Owner,
	const FTBProjectileId& Id,
	int32 DebugType
)
{
	FTBBulletSimData SimData(Bullet, DebugType);
	SimData.SetId(Id);
	SimData.OnUpdate = OnUpdate;
	return AddAndFireBullet(SimData, LaunchParams, Owner, DebugType);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireBulletWithCallbacks(
	const UBulletDataAsset* BulletDataAsset,
	const FTBLaunchParams& LaunchParams,
	TB::SimTasks::FBulletTaskDelegates Delegates,
	AActor* Owner,
	const FTBProjectileId& Id,
	int32 DebugType
)
{
	FTBBulletSimData SimData = CreateBulletSimDataFromDataAsset(BulletDataAsset, Id, DebugType);
	BindDelegates(SimData, FTBBulletTaskCallbacks(Delegates));
	return AddAndFireBullet(SimData, LaunchParams, Owner, DebugType);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireBulletWithCallbacks(
	const UBulletDataAsset* BulletDataAsset,
	const FTBLaunchParams& LaunchParams,
	TB::SimTasks::FBulletTaskDelegates Delegates,
	TFunction<void(const FTBProjectileFlightData&)> OnUpdate,
	AActor* Owner,
	const FTBProjectileId& Id,
	int32 DebugType
)
{
	FTBBulletSimData SimData = CreateBulletSimDataFromDataAsset(BulletDataAsset, Id, DebugType);
	auto Callbacks = FTBBulletTaskCallbacks(Delegates);
	Callbacks.OnUpdateFunction = OnUpdate;
	BindDelegates(SimData, Callbacks);
	return AddAndFireBullet(SimData, LaunchParams, Owner, DebugType);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireBulletWithUpdate(
	const UBulletDataAsset* BulletDataAsset,
	const FTBLaunchParams& LaunchParams,
	TFunction<void(const FTBProjectileFlightData&)> OnUpdate,
	AActor* Owner,
	const FTBProjectileId& Id,
	int32 DebugType
)
{
	FTBBulletSimData SimData = CreateBulletSimDataFromDataAsset(BulletDataAsset, Id, DebugType);
	SimData.OnUpdate = OnUpdate;
	return AddAndFireBullet(SimData, LaunchParams, Owner, DebugType);
}
#pragma endregion AddAndFireBullet

void UTerminalBallisticsSubsystem::CallOnBulletComplete(const FTBProjectileId& id, const TArray<FPredictProjectilePathPointData>& pathResults)
{
	RemoveBulletByIdIgnoreThread(id);
}

void UTerminalBallisticsSubsystem::CallOnProjectileComplete(const FTBProjectileId& id, const TArray<FPredictProjectilePathPointData>& pathResults)
{
	RemoveProjectileByIdIgnoreThread(id);
}

void UTerminalBallisticsSubsystem::BindFunctionsToProjectileSimData(FTBProjectileSimData& ProjectileSimData, FBPOnProjectileComplete OnProjectileComplete, FBPOnProjectileHit OnProjectileHit, FBPOnProjectileExitHit OnProjectileExitHit, FBPOnProjectileInjure OnProjectileInjure)
{
	ProjectileSimData.onComplete.Add(OnProjectileComplete);
	ProjectileSimData.onProjectileHit.Add(OnProjectileHit);
	ProjectileSimData.onProjectileExitHit.Add(OnProjectileExitHit);
	ProjectileSimData.onProjectileInjure.Add(OnProjectileInjure);
}

#pragma region AddProjectile
FTBProjectileId UTerminalBallisticsSubsystem::AddProjectile(FTBProjectileSimData& ToAdd, AActor* Owner, AController* Instigator)
{
	BindDefaultDelegates(ToAdd);
	if (!ToAdd.Owner.IsValid())
	{
		ToAdd.Owner = Owner;
	}
	ToAdd.Instigator = Instigator;
	if (!ToAdd.Instigator.IsValid() && ToAdd.Owner.IsValid())
	{
		ToAdd.Instigator = Owner->GetInstigatorController();
	}
	ToAdd.World = World;
	ToAdd.TerminalBallisticsSubsystem = this;

	if (!ToAdd.HasBeenAssignedId)
	{
		FScopeLock Lock(&AddProjectileMutex);
		ToAdd.GenerateId();
	}

	FWriteScopeLock WriteLock(InactiveProjectilesLock);
	InactiveProjectiles.Add(ToAdd);

	return ToAdd.GetId();
}

FTBProjectileId UTerminalBallisticsSubsystem::AddProjectileWithCallbacks(
	FTBProjectileSimData& ToAdd,
	FBPOnProjectileComplete OnProjectileComplete,
	FBPOnProjectileHit OnProjectileHit,
	FBPOnProjectileExitHit OnProjectileExitHit,
	FBPOnProjectileInjure OnProjectileInjure,
	AActor* Owner
)
{
	BindFunctionsToProjectileSimData(ToAdd, OnProjectileComplete, OnProjectileHit, OnProjectileExitHit, OnProjectileInjure);
	return AddProjectile(ToAdd, Owner);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddProjectileWithCallbacksAndUpdate(
	FTBProjectileSimData& ToAdd,
	FBPOnProjectileComplete OnProjectileComplete,
	FBPOnProjectileHit OnProjectileHit,
	FBPOnProjectileExitHit OnProjectileExitHit,
	FBPOnProjectileInjure OnProjectileInjure,
	FBPOnProjectileUpdate OnUpdate,
	AActor* Owner
)
{
	BindFunctionsToProjectileSimData(ToAdd, OnProjectileComplete, OnProjectileHit, OnProjectileExitHit, OnProjectileInjure);
	ToAdd.OnUpdateDelegate.Add(OnUpdate);
	return AddProjectile(ToAdd, Owner);
}
FTBProjectileId UTerminalBallisticsSubsystem::AddProjectileWithCallbacksAndUpdate(
	FTBProjectileSimData& ToAdd,
	FBPOnProjectileComplete OnProjectileComplete,
	FBPOnProjectileHit OnProjectileHit,
	FBPOnProjectileExitHit OnProjectileExitHit,
	FBPOnProjectileInjure OnProjectileInjure,
	FBPOnProjectileUpdate OnUpdate,
	TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
	AActor* Owner
)
{
	BindFunctionsToProjectileSimData(ToAdd, OnProjectileComplete, OnProjectileHit, OnProjectileExitHit, OnProjectileInjure);
	ToAdd.OnUpdateDelegate.Add(OnUpdate);
	ToAdd.OnUpdate = OnUpdateFunction;
	return AddProjectile(ToAdd, Owner);
}
FTBProjectileId UTerminalBallisticsSubsystem::AddProjectileWithCallbacksAndUpdate(
	FTBProjectileSimData& ToAdd,
	FBPOnProjectileComplete OnProjectileComplete,
	FBPOnProjectileHit OnProjectileHit,
	FBPOnProjectileExitHit OnProjectileExitHit,
	FBPOnProjectileInjure OnProjectileInjure,
	TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
	AActor* Owner
)
{
	BindFunctionsToProjectileSimData(ToAdd, OnProjectileComplete, OnProjectileHit, OnProjectileExitHit, OnProjectileInjure);
	ToAdd.OnUpdate = OnUpdateFunction;
	return AddProjectile(ToAdd, Owner);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddProjectileWithUpdate(
	FTBProjectileSimData& ToAdd,
	FBPOnProjectileUpdate OnUpdate,
	AActor* Owner
)
{
	ToAdd.OnUpdateDelegate.Add(OnUpdate);
	return AddProjectile(ToAdd, Owner);
}
FTBProjectileId UTerminalBallisticsSubsystem::AddProjectileWithUpdate(
	FTBProjectileSimData& ToAdd,
	FBPOnProjectileUpdate OnUpdate,
	TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
	AActor* Owner
)
{
	ToAdd.OnUpdateDelegate.Add(OnUpdate);
	ToAdd.OnUpdate = OnUpdateFunction;
	return AddProjectile(ToAdd, Owner);
}
FTBProjectileId UTerminalBallisticsSubsystem::AddProjectileWithUpdate(
	FTBProjectileSimData& ToAdd,
	TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
	AActor* Owner
)
{
	ToAdd.OnUpdate = OnUpdateFunction;
	return AddProjectile(ToAdd, Owner);
}
#pragma endregion

TArray<FTBProjectileId> UTerminalBallisticsSubsystem::AddProjectiles(TArray<FTBProjectileSimData>& ToAdd, AActor* Owner)
{
	TArray<FTBProjectileId> idArr;
	for (FTBProjectileSimData& projectile : ToAdd)
	{
		idArr.Add(AddProjectile(projectile, Owner));
	}
	return idArr;
}

void UTerminalBallisticsSubsystem::FireProjectileExplicit(FTBProjectileSimData* ProjectileToFire, const FTBLaunchParams& LaunchParams, int32 DebugType)
{
	if (CanFire())
	{
		ProjectilesToMakeActive.Enqueue(*ProjectileToFire);
		_FireProjectileImpl(ProjectileToFire, LaunchParams, DebugType);
	}
	else
	{
		ProjectileLaunchQueue.Enqueue(FTBLaunchData(LaunchParams, ProjectileToFire->GetId(), DebugType));
	}
}

void UTerminalBallisticsSubsystem::RemoveProjectile(const FTBProjectileSimData& ToRemove)
{
	if (HasProjectile(ToRemove))
	{
		int32 Removed = 0;
		{
			FWriteScopeLock ActiveWriteLock(ActiveProjectilesLock);
			Removed += ActiveProjectiles.RemoveSingleSwap(ToRemove) << 2;
		}
		{
			FWriteScopeLock InactiveWriteLock(InactiveProjectilesLock);
			Removed += InactiveProjectiles.RemoveSingleSwap(ToRemove) << 1;
		}
		{
			FWriteScopeLock ToAddWriteLock(ProjectilesToAddLock);
			Removed += ProjectilesToAdd.RemoveSingleSwap(ToRemove) << 0;
		}
		if (Removed == 0)
		{
			ProjectileOrId projectileToRemove;
			projectileToRemove.Set<FTBProjectileSimData>(ToRemove);
			ProjectileRemovalQueue.Enqueue(projectileToRemove);
		}
		else if (ProjectileThread)
		{
			const bool bWasRemovedFromActive = Removed >> 2 == 1; // Since we set the third bit if ActiveProjectiles.RemoveSingleSwap removed the bullet, this will check if that happened.
			ProjectileThread->RequestProjectileTermination(ToRemove.GetId(), TB::SimTasks::ETBThreadTaskAffinity::PROJECTILE_TASK);
		}
	}
}

void UTerminalBallisticsSubsystem::RemoveProjectileById(const FTBProjectileId& Id)
{
	auto Pred = [Id](FTBProjectileSimData a) { return Id == a.GetId(); };
	FTBProjectileSimData* ToRemove = nullptr;
	{
		FWriteScopeLock WriteLock(ActiveProjectilesLock);
		ToRemove = ActiveProjectiles.FindByPredicate(Pred);
		if (ToRemove)
		{
			if (ActiveProjectiles.RemoveSingleSwap(*ToRemove) > 0)
			{
				if (ProjectileThread)
				{
					ProjectileThread->RequestProjectileTermination(Id, TB::SimTasks::ETBThreadTaskAffinity::PROJECTILE_TASK);
				}
			}
		}
	}
	{
		FWriteScopeLock WriteLock(InactiveProjectilesLock);
		ToRemove = InactiveProjectiles.FindByPredicate(Pred);
		if (ToRemove)
		{
			InactiveProjectiles.RemoveSingleSwap(*ToRemove);
		}
	}
	{
		FWriteScopeLock WriteLock(ProjectilesToAddLock);
		ToRemove = ProjectilesToAdd.FindByPredicate(Pred);
		if (ToRemove)
		{
			ProjectilesToAdd.RemoveSingleSwap(*ToRemove);
		}
	}
}

void UTerminalBallisticsSubsystem::RemoveProjectiles(const TArray<FTBProjectileSimData>& ToRemove)
{
	for (const FTBProjectileSimData& Projectile : ToRemove)
	{
		RemoveProjectile(Projectile);
	}
}

void UTerminalBallisticsSubsystem::RemoveProjectilesById(const TArray<FTBProjectileId>& ToRemove)
{
	for (const FTBProjectileId& Id : ToRemove)
	{
		RemoveProjectileById(Id);
	}
}

bool UTerminalBallisticsSubsystem::GetProjectile(const FTBProjectileId& Id, FTBProjectileSimData& Projectile) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTerminalBallisticsSubsystem::GetProjectile);
	auto Pred = [Id](FTBProjectileSimData a) { return Id == a.GetId(); };
	int i = INDEX_NONE;
	{
		FReadScopeLock ReadLock(InactiveProjectilesLock);
		i = InactiveProjectiles.IndexOfByPredicate(Pred);
		if (i != INDEX_NONE)
		{
			Projectile = *InactiveProjectiles.FindByPredicate(Pred);
			return true;
		}
	}
	{
		FReadScopeLock ReadLock(ActiveProjectilesLock);
		i = ActiveProjectiles.IndexOfByPredicate(Pred);
		if (i != INDEX_NONE)
		{
			Projectile = ActiveProjectiles[i];
			return true;
		}
	}
	return false;
}

void UTerminalBallisticsSubsystem::FireProjectiles(const TArray<FTBLaunchData> ProjectilesToLaunch)
{
	TArray<FTBProjectileSimData> Projectiles;
	Projectiles.Reserve(ProjectilesToLaunch.Num());
	TArray<FTBLaunchData> ActualLaunchData;
	ActualLaunchData.Reserve(ProjectilesToLaunch.Num());

	FTBProjectileSimData SimData;
	for (const FTBLaunchData& LaunchData : ProjectilesToLaunch)
	{
		if (GetProjectile(LaunchData.Id, SimData))
		{
			Projectiles.Emplace(SimData);
			ActualLaunchData.Emplace(LaunchData);
		}
	}
	FireMultiple(Projectiles, ActualLaunchData);
}

void UTerminalBallisticsSubsystem::FireProjectile(const FTBProjectileId& Id, const FTBLaunchParams& LaunchParams, int32 DebugType)
{
	_FireProjectileInternal(Id, LaunchParams, DebugType);
}

#pragma region AddAndFireProjectile
FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireProjectile(FTBProjectileSimData& ProjectileSimData, const FTBLaunchParams& LaunchParams, AActor* Owner, int32 DebugType)
{
	const FTBProjectileId ProjectileId = AddProjectile(ProjectileSimData, Owner);
	FireProjectileExplicit(&ProjectileSimData, LaunchParams, DebugType);
	return ProjectileId;
}

FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireProjectileWithCallbacks(
	FTBProjectileSimData& ProjectileSimData,
	const FTBLaunchParams& LaunchParams,
	FBPOnProjectileComplete OnProjectileComplete,
	FBPOnProjectileHit OnProjectileHit,
	FBPOnProjectileExitHit OnProjectileExitHit,
	FBPOnProjectileInjure OnProjectileInjure,
	AActor* Owner,
	int32 DebugType
)
{
	BindFunctionsToProjectileSimData(ProjectileSimData, OnProjectileComplete, OnProjectileHit, OnProjectileExitHit, OnProjectileInjure);
	const FTBProjectileId ProjectileId = AddProjectile(ProjectileSimData, Owner);
	FireProjectileExplicit(&ProjectileSimData, LaunchParams, DebugType);
	return ProjectileId;
}

FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireProjectileWithCallbacksAndUpdate(
	FTBProjectileSimData& ProjectileSimData,
	const FTBLaunchParams& LaunchParams,
	FBPOnProjectileComplete OnProjectileComplete,
	FBPOnProjectileHit OnProjectileHit,
	FBPOnProjectileExitHit OnProjectileExitHit,
	FBPOnProjectileInjure OnProjectileInjure,
	FBPOnProjectileUpdate OnUpdate,
	AActor* Owner,
	int32 DebugType
)
{
	BindFunctionsToProjectileSimData(ProjectileSimData, OnProjectileComplete, OnProjectileHit, OnProjectileExitHit, OnProjectileInjure);
	ProjectileSimData.OnUpdateDelegate.Add(OnUpdate);
	const FTBProjectileId ProjectileId = AddProjectile(ProjectileSimData, Owner);
	FireProjectileExplicit(&ProjectileSimData, LaunchParams, DebugType);
	return ProjectileId;
}

FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireProjectileWithCallbacks(
	FTBProjectileSimData& ProjectileSimData,
	const FTBLaunchParams& LaunchParams,
	TB::SimTasks::FProjectileTaskDelegates Delegates,
	AActor* Owner,
	int32 DebugType
)
{
	BindDelegates(ProjectileSimData, FTBProjectileTaskCallbacks(Delegates));
	return AddAndFireProjectile(ProjectileSimData, LaunchParams, Owner, DebugType);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireProjectileWithCallbacks(
	FTBProjectileSimData& ProjectileSimData,
	const FTBLaunchParams& LaunchParams,
	TB::SimTasks::FProjectileTaskDelegates Delegates,
	TFunction<void(const FTBProjectileFlightData&)> OnUpdate,
	AActor* Owner,
	int32 DebugType
)
{
	auto Callbacks = FTBProjectileTaskCallbacks(Delegates);
	Callbacks.OnUpdateFunction = OnUpdate;
	BindDelegates(ProjectileSimData, Callbacks);
	return AddAndFireProjectile(ProjectileSimData, LaunchParams, Owner, DebugType);
}

FTBProjectileId UTerminalBallisticsSubsystem::AddAndFireProjectileWithCallbacks(
	FTBProjectileSimData& ProjectileSimData,
	const FTBLaunchParams& LaunchParams,
	TFunction<void(const FTBProjectileFlightData&)> OnUpdate,
	AActor* Owner,
	int32 DebugType
)
{
	ProjectileSimData.OnUpdate = OnUpdate;
	return AddAndFireProjectile(ProjectileSimData, LaunchParams, Owner, DebugType);
}
#pragma endregion

void UTerminalBallisticsSubsystem::BindDelegates(FTBBulletSimData& SimData, FTBBulletTaskCallbacks Delegates)
{
	auto AddIfSet = [](auto& Delegate, auto& DelegateToAdd)
	{
		if (DelegateToAdd.IsSet())
		{
			Delegate.Add(DelegateToAdd.Get());
		}
	};

	auto SetIfPossible = [](auto& Delegate, auto& NewValue)
	{
		if (NewValue.IsSet())
		{
			Delegate = NewValue.Get();
		}
	};

	SetIfPossible(SimData.onComplete, Delegates.NativeDelegates.OnComplete);
	SetIfPossible(SimData.onBulletHit, Delegates.NativeDelegates.OnHit);
	SetIfPossible(SimData.onBulletExitHit, Delegates.NativeDelegates.OnExitHit);
	SetIfPossible(SimData.onBulletInjure, Delegates.NativeDelegates.OnInjure);
	SetIfPossible(SimData.OnUpdateDelegate, Delegates.NativeDelegates.OnUpdate);

	AddIfSet(SimData.onComplete, Delegates.DelegatesBP.OnComplete);
	AddIfSet(SimData.onBulletHit, Delegates.DelegatesBP.OnHit);
	AddIfSet(SimData.onBulletExitHit, Delegates.DelegatesBP.OnExitHit);
	AddIfSet(SimData.onBulletInjure, Delegates.DelegatesBP.OnInjure);
	AddIfSet(SimData.OnUpdateDelegate, Delegates.DelegatesBP.OnUpdate);

	if(Delegates.OnUpdateFunction.IsSet())
	{
		SimData.OnUpdate = Delegates.OnUpdateFunction.GetValue();
	}
}

void UTerminalBallisticsSubsystem::BindDelegates(FTBProjectileSimData& SimData, FTBProjectileTaskCallbacks Delegates)
{
	auto AddIfSet = [](auto& Delegate, auto& DelegateToAdd)
		{
			if (DelegateToAdd.IsSet())
			{
				Delegate.Add(DelegateToAdd.Get());
			}
		};

	auto SetIfPossible = [](auto& Delegate, auto& NewValue)
		{
			if (NewValue.IsSet())
			{
				Delegate = NewValue.Get();
			}
		};

	SetIfPossible(SimData.onComplete, Delegates.NativeDelegates.OnComplete);
	SetIfPossible(SimData.onProjectileHit, Delegates.NativeDelegates.OnHit);
	SetIfPossible(SimData.onProjectileExitHit, Delegates.NativeDelegates.OnExitHit);
	SetIfPossible(SimData.onProjectileInjure, Delegates.NativeDelegates.OnInjure);
	SetIfPossible(SimData.OnUpdateDelegate, Delegates.NativeDelegates.OnUpdate);

	AddIfSet(SimData.onComplete, Delegates.DelegatesBP.OnComplete);
	AddIfSet(SimData.onProjectileHit, Delegates.DelegatesBP.OnHit);
	AddIfSet(SimData.onProjectileExitHit, Delegates.DelegatesBP.OnExitHit);
	AddIfSet(SimData.onProjectileInjure, Delegates.DelegatesBP.OnInjure);
	AddIfSet(SimData.OnUpdateDelegate, Delegates.DelegatesBP.OnUpdate);

	if (Delegates.OnUpdateFunction.IsSet())
	{
		SimData.OnUpdate = Delegates.OnUpdateFunction.GetValue();
	}
}

TArray<FTBProjectileId> UTerminalBallisticsSubsystem::AddBullets(TArray<FTBBulletSimData>& SimData, const TArray<FTBLaunchParams>& LaunchParams, TOptional<FTBBulletTaskCallbacks> Delegates)
{
	if (SimData.Num() != LaunchParams.Num())
	{
		TB_LOG(Error, TEXT("SimData and LaunchParams must have the same number of elements."));
		return {};
	}
	TArray<FTBProjectileId> Ids;
	Ids.Reserve(SimData.Num());
	for (int i = 0; i < SimData.Num(); i++)
	{
		if(Delegates.IsSet())
		{
			BindDelegates(SimData[i], Delegates.GetValue());
		}
		Ids.Emplace(AddBullet(SimData[i], LaunchParams[i].Owner, LaunchParams[i].Instigator));
	}
	return Ids;
}

TArray<FTBProjectileId> UTerminalBallisticsSubsystem::AddProjectiles(TArray<FTBProjectileSimData>& SimData, const TArray<FTBLaunchParams>& LaunchParams, TOptional<FTBProjectileTaskCallbacks> Delegates)
{
	if (SimData.Num() != LaunchParams.Num())
	{
		TB_LOG(Error, TEXT("SimData and LaunchParams must have the same number of elements."));
		return {};
	}
	TArray<FTBProjectileId> Ids;
	Ids.Reserve(SimData.Num());
	for (int i = 0; i < SimData.Num(); i++)
	{
		if (Delegates.IsSet())
		{
			BindDelegates(SimData[i], Delegates.GetValue());
		}
		Ids.Emplace(AddProjectile(SimData[i], LaunchParams[i].Owner, LaunchParams[i].Instigator));
	}
	return Ids;
}


bool UTerminalBallisticsSubsystem::HasBullet(const FTBBulletSimData Bullet) const
{
	FReadScopeLock ActiveReadLock(ActiveBulletsLock);
	FReadScopeLock InactiveReadLock(InactiveBulletsLock);
	FReadScopeLock ToAddReadLock(BulletsToAddLock);
	const bool Found = ActiveBullets.Contains(Bullet) || BulletsToAdd.Contains(Bullet) || InactiveBullets.Contains(Bullet);
	return Found;
}

bool UTerminalBallisticsSubsystem::HasBullet(const FTBProjectileId& Id) const
{
	bool Found = false;
	auto Pred = [Id](const FTBBulletSimData& SimData) { return SimData.GetId() == Id; };
	FReadScopeLock ActiveReadLock(ActiveBulletsLock);
	FReadScopeLock InactiveReadLock(InactiveBulletsLock);
	FReadScopeLock ToAddReadLock(BulletsToAddLock);
	Found = ActiveBullets.ContainsByPredicate(Pred) || BulletsToAdd.ContainsByPredicate(Pred) || InactiveBullets.ContainsByPredicate(Pred);
	return Found;
}

bool UTerminalBallisticsSubsystem::HasProjectile(const FTBProjectileSimData& Projectile) const
{
	bool Found = false;
	FReadScopeLock ActiveReadLock(ActiveProjectilesLock);
	FReadScopeLock InactiveReadLock(InactiveProjectilesLock);
	FReadScopeLock ToAddReadLock(ProjectilesToAddLock);
	Found = ActiveProjectiles.Contains(Projectile) || InactiveProjectiles.Contains(Projectile) || ProjectilesToAdd.Contains(Projectile);
	return Found;
}

bool UTerminalBallisticsSubsystem::HasProjectile(const FTBProjectileId& Id) const
{
	bool Found = false;
	auto Pred = [Id](const FTBProjectileSimData& SimData) { return SimData.GetId() == Id; };
	FReadScopeLock ActiveReadLock(ActiveProjectilesLock);
	FReadScopeLock InactiveReadLock(InactiveProjectilesLock);
	FReadScopeLock ToAddReadLock(ProjectilesToAddLock);
	Found = ActiveProjectiles.ContainsByPredicate(Pred) || InactiveProjectiles.ContainsByPredicate(Pred) || ProjectilesToAdd.ContainsByPredicate(Pred);
	return Found;
}

template<typename TypeOfSimData>
typename TEnableIf<TB::Traits::TIsSimData<TypeOfSimData>::Value, void>::Type UTerminalBallisticsSubsystem::_SetupProjectileLaunchData(TypeOfSimData* ProjectileToFire, const FTBLaunchParams& LaunchParams, int32 DebugType)
{
	ProjectileToFire->bDrawDebugTrace = DebugType & (uint8)ETBBallisticsDebugType::DrawDebugTrace;
	ProjectileToFire->bPrintDebugInfo = DebugType & (uint8)ETBBallisticsDebugType::PrintDebugInfo;
	if (!ProjectileToFire->Instigator.IsValid())
	{
		if (LaunchParams.Instigator)
		{
			ProjectileToFire->Instigator = LaunchParams.Instigator;
		}
		else
		{
			const AActor* Owner = LaunchParams.Owner;
			if (!Owner)
			{
				Owner = ProjectileToFire->Owner.Get();
			}
			if (Owner)
			{
				ProjectileToFire->Instigator = Owner->GetInstigatorController();
			}
		}
	}
	FVector startVelocity = LaunchParams.ProjectileSpeed * LaunchParams.FireTransform.Direction * 100.0; // m/s to cm/s
	if (LaunchParams.AddToOwnerVelocity && LaunchParams.Owner)
	{
		startVelocity += LaunchParams.Owner->GetVelocity();
	}
	const double maxSimTime = (3.0 * LaunchParams.EffectiveRange) / LaunchParams.ProjectileSpeed; // Trace for thrice the effective range of the Projectile just to be sure
	ProjectileToFire->Timescale = LaunchParams.Timescale;
	ProjectileToFire->MaxSimTime = maxSimTime;
	ProjectileToFire->OwnerIgnoreDistance = LaunchParams.OwnerIgnoreDistance;
	ProjectileToFire->StartLocation = LaunchParams.FireTransform.Location;
	ProjectileToFire->StartVelocity = startVelocity;
	ProjectileToFire->PredictParams = FPredictProjectilePathParams(ProjectileToFire->GetRadius(), LaunchParams.FireTransform.Location, startVelocity, maxSimTime);
	ProjectileToFire->PredictParams.ObjectTypes.Append(LaunchParams.ObjectTypes);
	ProjectileToFire->PredictParams.bTraceComplex = true;
	ProjectileToFire->PredictParams.bTraceWithChannel = true;
	ProjectileToFire->PredictParams.bTraceWithCollision = true;
	ProjectileToFire->PredictParams.SimFrequency = SimFrequency > maxSimTime ? (1.0 / maxSimTime * 2.0) : SimFrequency;
	ProjectileToFire->PredictParams.OverrideGravityZ = 0.f;
	ProjectileToFire->PredictParams.TraceChannel = LaunchParams.TraceChannel;
	ProjectileToFire->PredictParams.ActorsToIgnore.Append(LaunchParams.ToIgnore);
	if (LaunchParams.IgnoreOwner)
	{
		ProjectileToFire->PredictParams.ActorsToIgnore.Add(LaunchParams.Owner);
	}
	else if (ProjectileToFire->PredictParams.ActorsToIgnore.Contains(LaunchParams.Owner))
	{
		ProjectileToFire->PredictParams.ActorsToIgnore.Remove(LaunchParams.Owner);
	}
	ProjectileToFire->bDrawDebugTrace |= CVarBallisticsDebugDraw.GetValueOnAnyThread();
	ProjectileToFire->bPrintDebugInfo |= CVarPrintBallisticsDebugInfo.GetValueOnAnyThread();
	const UTBConfiguration* TBConfig = GetDefault<UTBConfiguration>();
	if (ProjectileToFire->DragCalculationType == ETBDragComplexity::DEFAULT)
	{
		ProjectileToFire->DragCalculationType = TBConfig->DefaultDragComplexity;
	}
	ProjectileToFire->SimType = LaunchParams.SimType;
	if (ProjectileToFire->SimType == ETBProjectileSimulationType::Default)
	{
		ProjectileToFire->SimType = TBConfig->DefaultSimType;
	}
	if (!ProjectileToFire->AreDefaultDelegatesBound())
	{
		BindDefaultDelegates(ProjectileToFire);
	}
}

void UTerminalBallisticsSubsystem::_FireBulletInternal(const FTBProjectileId& BulletId, const FTBLaunchParams& LaunchParams, int32 DebugType)
{
	TB_RET_COND(bShuttingDown);
	TRACE_CPUPROFILER_EVENT_SCOPE(UTerminalBallisticsSubsystem::_FireBulletInternal);

	if (CanFire())
	{
		if (FTBBulletSimData* SimData = GetBulletFromInactive(BulletId))
		{
			FReadScopeLock Lock(InactiveBulletsLock);
			_FireProjectileImpl(SimData, LaunchParams, DebugType);
		}
	}
	else
	{
		BulletLaunchQueue.Enqueue(FTBLaunchData(LaunchParams, BulletId, DebugType));
	}
}

void UTerminalBallisticsSubsystem::_FireProjectileInternal(const FTBProjectileId& Id, const FTBLaunchParams& LaunchParams, int32 DebugType)
{
	TB_RET_COND(bShuttingDown);
	TRACE_CPUPROFILER_EVENT_SCOPE(UTerminalBallisticsSubsystem::_FireProjectileInternal);

	if (CanFire())
	{
		if (FTBProjectileSimData* SimData = GetProjectileFromInactive(Id))
		{
			FReadScopeLock Lock(InactiveProjectilesLock);
			_FireProjectileImpl(SimData, LaunchParams, DebugType);
		}
	}
	else
	{
		ProjectileLaunchQueue.Enqueue(FTBLaunchData(LaunchParams, Id, DebugType));
	}
}

template<typename TypeOfSimData>
typename TEnableIf<TB::Traits::TIsSimData<TypeOfSimData>::Value, void>::Type UTerminalBallisticsSubsystem::_FireProjectileImpl(TypeOfSimData* ProjectileToFire, const FTBLaunchParams& LaunchParams, int32 DebugType)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTerminalBallisticsSubsystem::_FireProjectileImpl);
	ProjectilesLaunchedThisTick++;
	_SetupProjectile(ProjectileToFire, LaunchParams, DebugType);

	if (!ProjectileToFire->GetId().IsValid())
	{
		UE_LOG(LogTerminalBallistics, Warning, TEXT("Attempted to fire projectile without an Id. Regenerating Id."));
		ProjectileToFire->GenerateId();
	}

	ProjectileToFire->bDrawDebugTrace |= (DebugType & (uint8)ETBBallisticsDebugType::DrawDebugTrace) != 0U;
	ProjectileToFire->bPrintDebugInfo |= (DebugType & (uint8)ETBBallisticsDebugType::PrintDebugInfo) != 0U;

	TB_RET_COND(bShuttingDown);
	if (ProjectileThread)
	{
		if constexpr (std::is_same_v<TypeOfSimData, FTBBulletSimData>)
		{
			FTBBulletSimData SimData = *ProjectileToFire;
			ProjectileThread->AddNewBulletTask(std::forward<FTBBulletSimData>(SimData));
		}
		else if constexpr (std::is_same_v<TypeOfSimData, FTBProjectileSimData>)
		{
			//std::unique_lock lock(ProjectileThread->TaskMutex);
			FTBProjectileSimData SimData = *ProjectileToFire;
			ProjectileThread->DataQueue->Enqueue(std::forward<TypeOfSimData>(SimData));
			//ProjectileThread->TaskCondition.notify_one();
			//ProjectileThread->AddNewProjectileTask(std::forward<FTBProjectileSimData>(SimData));
		}
		//ProjectileThread->DataQueue->Enqueue(std::forward<TypeOfSimData>(*ProjectileToFire));
		ProjectileThread->Unpause();
	}
	else
	{
		UE_LOG(LogTerminalBallistics, Error, TEXT("Projectile thread was null"));
	}
}

template<typename TypeOfSimData>
typename TEnableIf<TB::Traits::TIsSimData<TypeOfSimData>::Value, void>::Type UTerminalBallisticsSubsystem::_SetupProjectile(TypeOfSimData* ProjectileToFire, const FTBLaunchParams& LaunchParams, int32 DebugType)
{
	if (ProjectileToFire->Owner == nullptr || !ProjectileToFire->Owner.IsValid())
	{
		if (LaunchParams.Owner)
		{
			ProjectileToFire->Owner = LaunchParams.Owner;
		}
		else
		{
			UE_LOG(LogTerminalBallistics, Error, TEXT("Attempted to fire projectile with invalid owner."));
			return;
		}
	}

	_SetupProjectileLaunchData(ProjectileToFire, LaunchParams, DebugType);
	if (!IsRunningDedicatedServer() && !LaunchParams.bForceNoTracer && ProjectileToFire->TracerSystem)
	{
		FGCScopeGuard GCGuard;
		ProjectileToFire->TracerComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ProjectileToFire->TracerSystem, LaunchParams.FireTransform.Location, FRotator::ZeroRotator, FVector::OneVector, true, true, ENCPoolMethod::ManualRelease);
		static const FName DirectionVariableName = "BeamDirection";
		static const FName DurationVariableName = "Duration";
		ProjectileToFire->TracerComponent->SetVariableFloat(DurationVariableName, ProjectileToFire->MaxSimTime);
		ProjectileToFire->TracerComponent->SetVariablePosition(DirectionVariableName, LaunchParams.FireTransform.Direction);
	}
}

template<typename TypeOfSimData>
typename TEnableIf<TB::Traits::TIsSimData<std::decay_t<TypeOfSimData>>::Value, void>::Type UTerminalBallisticsSubsystem::FireMultiple(TArray<TypeOfSimData> ProjectilesToFire, const TArray<FTBLaunchData>& LaunchData)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTerminalBallisticsSubsystem::FireMultiple);
	for (int i = 0; i < ProjectilesToFire.Num(); i++)
	{
		TypeOfSimData* ProjectileToFire = &ProjectilesToFire[i];
		const FTBLaunchData CurrentLaunchData = LaunchData[i];

		ProjectilesLaunchedThisTick++;
		_SetupProjectile(ProjectileToFire, CurrentLaunchData.LaunchParams, CurrentLaunchData.DebugType);

		if (!ProjectileToFire->GetId().IsValid())
		{
			if (CurrentLaunchData.Id.IsValid())
			{
				ProjectileToFire->SetId(CurrentLaunchData.Id);
			}
			else
			{
				UE_LOG(LogTerminalBallistics, Warning, TEXT("Attempted to fire projectile without an Id. Regenerating Id."));
				ProjectileToFire->GenerateId();
			}
		}

		TB_RET_COND(bShuttingDown);
		if (ProjectileThread)
		{
			ProjectileThread->DataQueue->Enqueue(std::forward<TypeOfSimData>(*ProjectileToFire));
			ProjectileThread->Unpause();
		}
		else
		{
			UE_LOG(LogTerminalBallistics, Error, TEXT("Projectile thread was null"));
		}
	}
}

UWorld* UTerminalBallisticsSubsystem::GetOrUpdateWorld() const
{
	if (!IsValid(World))
	{
		World = GetWorld();
	}
	return World;
}

bool UTerminalBallisticsSubsystem::SupportsNetMode(const ENetMode NetMode)
{
	const ETBSimTaskNetMode AllowedNetMode = GetDefault<UTBConfiguration>()->SimulationNetMode;
	const bool IsClientAllowed = (uint8)(AllowedNetMode & ETBSimTaskNetMode::Client) != 0U;
	const bool IsServerAllowed = (uint8)(AllowedNetMode & ETBSimTaskNetMode::Server) != 0U;
	const bool IsTypeOfServer = NetMode < NM_Client;
	if ((IsTypeOfServer && IsServerAllowed) || (!IsTypeOfServer && IsClientAllowed))
	{
		return true;
	}
	if (NetMode == NM_Standalone) // Special case, since the server also acts as the client
	{
		return true;
	}
	return false;
}

void UTerminalBallisticsSubsystem::RemoveBulletByIdIgnoreThread(const FTBProjectileId& Id)
{
	auto Pred = [Id](const FTBBulletSimData& SimData) { return Id == SimData.GetId(); };
	FTBBulletSimData* ToRemove = nullptr;
	{
		FWriteScopeLock WriteLock(ActiveBulletsLock);
		ToRemove = ActiveBullets.FindByPredicate(Pred);
		if (ToRemove)
		{
			ActiveBullets.RemoveSingleSwap(*ToRemove);
		}
	}
}

void UTerminalBallisticsSubsystem::RemoveProjectileByIdIgnoreThread(const FTBProjectileId& Id)
{
	auto Pred = [Id](const FTBProjectileSimData& SimData) { return Id == SimData.GetId(); };
	FTBProjectileSimData* ToRemove = nullptr;
	{
		FWriteScopeLock WriteLock(ActiveProjectilesLock);
		ToRemove = ActiveProjectiles.FindByPredicate(Pred);
		if (ToRemove)
		{
			ActiveProjectiles.RemoveSingleSwap(*ToRemove);
		}
	}
}

#pragma region Game Mode
void UTerminalBallisticsSubsystem::CallGameModeHit(const FTBImpactParams& ImpactParams)
{
	TB_RET_COND(bShuttingDown || !bHasValidGameMode);
	TObjectPtr<AGameModeBase> GameMode = GetOrUpdateWorld()->GetAuthGameMode();
	TB_RET_COND(!TB_VALID(GameMode));
	TB_RET_COND(!ImpactParams.IsValid());
	ITerminalBallisticsGameModeBaseInterface::Execute_BulletImpactEvent(GameMode, ImpactParams);
}

void UTerminalBallisticsSubsystem::CallGameModeHitBasic(const FTBImpactParamsBasic& ImpactParams)
{
	TB_RET_COND(bShuttingDown || !bHasValidGameMode);
	TObjectPtr<AGameModeBase> GameMode = GetOrUpdateWorld()->GetAuthGameMode();
	TB_RET_COND(!TB_VALID(GameMode));
	TB_RET_COND(!ImpactParams.IsValid());
	ITerminalBallisticsGameModeBaseInterface::Execute_ProjectileImpactEvent(GameMode, ImpactParams);
}

void UTerminalBallisticsSubsystem::CallGameModeExitHit(const FTBImpactParams& ImpactParams)
{
	TB_RET_COND(bShuttingDown || !bHasValidGameMode);
	TObjectPtr<AGameModeBase> GameMode = GetOrUpdateWorld()->GetAuthGameMode();
	TB_RET_COND(!TB_VALID(GameMode));
	TB_RET_COND(!ImpactParams.IsValid());
	ITerminalBallisticsGameModeBaseInterface::Execute_BulletExitEvent(GameMode, ImpactParams);
}

void UTerminalBallisticsSubsystem::CallGameModeExitHitBasic(const FTBImpactParamsBasic& ImpactParams)
{
	TB_RET_COND(bShuttingDown || !bHasValidGameMode);
	TObjectPtr<AGameModeBase> GameMode = GetOrUpdateWorld()->GetAuthGameMode();
	TB_RET_COND(!TB_VALID(GameMode));
	TB_RET_COND(!ImpactParams.IsValid());
	ITerminalBallisticsGameModeBaseInterface::Execute_ProjectileExitEvent(GameMode, ImpactParams);
}

void UTerminalBallisticsSubsystem::CallGameModeInjure(const FTBImpactParams& ImpactParams, const FTBProjectileInjuryParams& Injury)
{
	TB_RET_COND(bShuttingDown || !bHasValidGameMode);
	TObjectPtr<AGameModeBase> GameMode = GetOrUpdateWorld()->GetAuthGameMode();
	TB_RET_COND(!TB_VALID(GameMode));
	TB_RET_COND(!Injury.IsValid());
	TB_RET_COND(!ImpactParams.IsValid());
	ITerminalBallisticsGameModeBaseInterface::Execute_BulletInjureEvent(GameMode, ImpactParams, Injury);
}

void UTerminalBallisticsSubsystem::CallGameModeInjureBasic(const FTBImpactParamsBasic& ImpactParams, const FTBProjectileInjuryParams& Injury)
{
	TB_RET_COND(bShuttingDown || !bHasValidGameMode);
	TObjectPtr<AGameModeBase> GameMode = GetOrUpdateWorld()->GetAuthGameMode();
	TB_RET_COND(!TB_VALID(GameMode));
	TB_RET_COND(!Injury.IsValid());
	TB_RET_COND(!ImpactParams.IsValid());
	ITerminalBallisticsGameModeBaseInterface::Execute_ProjectileInjureEvent(GameMode, ImpactParams, Injury);
}
#pragma endregion Game Mode

#pragma region Theading
void UTerminalBallisticsSubsystem::InitProjectileThread()
{
	ProjectileThreadQueue = NewObject<UTBProjectileThreadQueue>(this, TEXT("Thread Queue"));

	ShutdownProjectileThread(); // Make sure only one is running at a time

	FTimespan TickRate = FTimespan::FromSeconds(1.0 / TB::Configuration::ThreadTickFrequency);

	FString ThreadName = "TBProjectileThread #";
	ThreadName += FString::FromInt(++ThreadCount);
	ProjectileThread = new FTBProjectileThread(TickRate, *ThreadName, this, ProjectileThreadQueue);
	ProjectileThread->Init();
}

void UTerminalBallisticsSubsystem::ShutdownProjectileThread()
{
	if (ProjectileThread)
	{
		ProjectileThread->Shutdown();
		ProjectileThread->Stop();

		const int timeout = 50; // 5 seconds
		int timeSlept = 0;
		while (!ProjectileThread->ThreadStopped() && timeSlept < timeout)
		{
			FPlatformProcess::Sleep(0.1);
			++timeSlept;
		}

		delete ProjectileThread;
	}

	ProjectileThread = nullptr;
}

void UTerminalBallisticsSubsystem::GetResultsFromProjectileThread()
{
	TB_RET_COND(!ProjectileThread || !ProjectileThread->DataQueue->HasOutputData());

	const int32 ThreadConsumtionRate = TB::Configuration::ThreadResponseConsumtionRate;

	int32 ResultsTaken = 0;
	FBulletTaskResult BulletTaskResult;
	while (!bShuttingDown && ResultsTaken < ThreadConsumtionRate && ProjectileThread->DataQueue->Dequeue(BulletTaskResult))
	{
		RemoveBulletByIdIgnoreThread(BulletTaskResult.ProjectileId);
		ProcessTaskResult(BulletTaskResult);
		BulletTaskResult.~TTBBaseProjectileTaskResult();
		++ResultsTaken;
	}
	FProjectileTaskResult ProjectileTaskResult;
	while (!bShuttingDown && ResultsTaken < ThreadConsumtionRate && ProjectileThread->DataQueue->Dequeue(ProjectileTaskResult))
	{
		RemoveProjectileByIdIgnoreThread(ProjectileTaskResult.ProjectileId);
		ProcessTaskResult(ProjectileTaskResult);
		ProjectileTaskResult.~TTBBaseProjectileTaskResult();
		++ResultsTaken;
	}
}

template<typename ImpactStruct>
void UTerminalBallisticsSubsystem::ProcessTaskResult(TTBBaseProjectileTaskResult<ImpactStruct> TaskResult)
{
	TB_RET_COND(bShuttingDown);
#if ENABLE_DRAW_DEBUG
	if (IsInGameThread() && !CVarBallisticsDebugDrawRealtime.GetValueOnAnyThread() && (TaskResult.bDrawDebugTrace || CVarBallisticsDebugDraw.GetValueOnGameThread()) && TaskResult.PathData.Num() > 0)
	{
		GetOrUpdateWorld();
		FVector PreviousLocation = TaskResult.StartLocation;
		FLinearColor Color;
		for (const FPredictProjectilePathPointData& PathPt : TaskResult.PathData)
		{
			if (PathPt.Location == PreviousLocation || PathPt.Location == FVector::ZeroVector || PathPt.Location == TaskResult.StartLocation)
			{
				continue;
			}
			Color = UBallisticFunctions::GetTraceColor(PathPt.Velocity.Size(), TaskResult.InitialVelocity.Size());
			DrawDebugLine(World, PreviousLocation, PathPt.Location, Color.ToFColor(true), false, 30.f);
			PreviousLocation = PathPt.Location;
		}
	}
#endif
}
#pragma endregion Threading
#pragma endregion
