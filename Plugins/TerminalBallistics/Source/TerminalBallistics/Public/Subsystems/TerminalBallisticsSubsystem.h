// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "TerminalBallisticsConfiguration.h"
#include "Types/TBEnums.h"
#include "TerminalBallisticsTraits.h"
#include "TerminalBallisticsTypes.h"
#include "Threading/TerminalBallisticsThreadingTypes.h"
#include "Types/ImpactParams.h"
#include "Types/TBProjectile.h"
#include "Types/TBProjectileId.h"
#include "Types/TBProjectileInjury.h"
#include "Types/TBSimData.h"

#include "TerminalBallisticsSubsystem.generated.h"



class UEnvironmentSubsystem;
class ITerminalBallisticsGameModeBaseInterface;

typedef TSharedPtr<struct FTBBullet> BulletPointer;


DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnSimTaskExit, const int, ExitCode, const FTBProjectileId&, Id, const TArray<FPredictProjectilePathPointData>&, Results);


UCLASS(MinimalApi)
class UTBProjectileThreadQueue : public UObject
{
	GENERATED_BODY()
public:
	UTBProjectileThreadQueue(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get())
		: Super(ObjectInitializer)
	{}

	UTBProjectileThreadQueue(FVTableHelper& Helper)
		: Super(Helper)
	{}

	~UTBProjectileThreadQueue();

	inline void Empty()
	{
		BulletInputQueue.Empty();
		BulletOutputQueue.Empty();
		ProjectileInputQueue.Empty();
		ProjectileOutputQueue.Empty();
	}

	inline bool HasInputData() const
	{
		return !BulletInputQueue.IsEmpty()
			|| !ProjectileInputQueue.IsEmpty();
	}

	inline bool HasOutputData() const
	{
		return !BulletOutputQueue.IsEmpty()
			|| !ProjectileOutputQueue.IsEmpty();
	}

	inline bool Enqueue(FTBBulletSimData&& BulletSimData)
	{
		//UnpauseThread();
		return BulletInputQueue.Enqueue(std::forward<FTBBulletSimData>(BulletSimData));
	}

	inline bool Enqueue(FTBProjectileSimData&& ProjectileSimData)
	{
		//UnpauseThread();
		return ProjectileInputQueue.Enqueue(std::forward<FTBProjectileSimData>(ProjectileSimData));
	}

	inline bool Enqueue(FBulletTaskResult&& TaskResult)
	{
		UnpauseThread();
		return BulletOutputQueue.Enqueue(std::forward<FBulletTaskResult>(TaskResult));
	}

	inline bool Enqueue(FProjectileTaskResult&& TaskResult)
	{
		UnpauseThread();
		return ProjectileOutputQueue.Enqueue(std::forward<FProjectileTaskResult>(TaskResult));
	}

	inline bool Dequeue(FBulletTaskResult& TaskResult)
	{
		return BulletOutputQueue.Dequeue(TaskResult);
	}

	inline bool Dequeue(FProjectileTaskResult& TaskResult)
	{
		return ProjectileOutputQueue.Dequeue(TaskResult);
	}

	inline bool Dequeue(FTBBulletSimData& BulletSimData)
	{
		return BulletInputQueue.Dequeue(BulletSimData);
	}

	inline bool Dequeue(FTBProjectileSimData& ProjectileSimData)
	{
		return ProjectileInputQueue.Dequeue(ProjectileSimData);
	}

	inline void SetUnpauseFunction(TUniqueFunction<void()> Function)
	{
		UnpauseThreadFunc = MoveTemp(Function);
	}

	inline void UnpauseThread()
	{
		TB_RET_COND(!IsThreadPaused);

		if (UnpauseThreadFunc) {
			UnpauseThreadFunc();
		}
	}

	std::atomic_bool IsThreadPaused;

protected:
	TUniqueFunction<void()> UnpauseThreadFunc;

	TQueue<FTBBulletSimData, EQueueMode::Mpsc> BulletInputQueue;
	TQueue<FTBProjectileSimData, EQueueMode::Mpsc> ProjectileInputQueue;

	TQueue<FBulletTaskResult, EQueueMode::Mpsc> BulletOutputQueue;
	TQueue<FProjectileTaskResult, EQueueMode::Mpsc> ProjectileOutputQueue;
};

namespace TB::SimTasks
{
	typedef TSimTaskDelegates<FOnComplete, FOnBulletHit, FOnBulletExitHit, FOnBulletInjure, FOnUpdate> FBulletTaskDelegates;
	typedef TSimTaskDelegates<FOnComplete, FOnProjectileHit, FOnProjectileExitHit, FOnProjectileInjure, FOnUpdate> FProjectileTaskDelegates;
};
namespace TB::Traits
{
	template<> struct TIsSimTaskDelegateStruct<TB::SimTasks::FBulletTaskDelegates>
	{
		enum
		{
			Value = true
		};
	};
	template<> struct TIsSimTaskDelegateStruct<TB::SimTasks::FProjectileTaskDelegates>
	{
		enum
		{
			Value = true
		};
	};
}

template<typename CompleteDelegateType, typename HitDelegateType, typename ExitHitDelegateType, typename InjureDelegateType, typename UpdateDelegateType, typename NativeDelegatesStruct>
struct TTBTaskCallbacks
{
	typedef TB::SimTasks::TSimTaskDelegates<CompleteDelegateType, HitDelegateType, ExitHitDelegateType, InjureDelegateType, UpdateDelegateType> FDelegatesBP;

	FDelegatesBP DelegatesBP;
	NativeDelegatesStruct NativeDelegates;

	TOptional<TFunction<void(const FTBProjectileFlightData&)>> OnUpdateFunction;

	TTBTaskCallbacks() = default;

	TTBTaskCallbacks(
		CompleteDelegateType CompleteDelegate,
		HitDelegateType HitDelegate,
		ExitHitDelegateType ExitHitDelegate,
		InjureDelegateType InjureDelegate,
		UpdateDelegateType UpdateDelegate,
		NativeDelegatesStruct NativeDelegates = NativeDelegatesStruct()
	)
		: DelegatesBP(CompleteDelegate, HitDelegate, ExitHitDelegate, InjureDelegate, UpdateDelegate)
		, NativeDelegates(NativeDelegates)
	{}

	TTBTaskCallbacks(NativeDelegatesStruct NativeDelegates)
		: NativeDelegates(NativeDelegates)
	{}

	~TTBTaskCallbacks()
	{
		DelegatesBP.Clear();
		NativeDelegates.Clear();
		if (OnUpdateFunction.IsSet())
		{
			OnUpdateFunction.GetValue().Reset();
		}
	}
};
typedef TTBTaskCallbacks<FBPOnProjectileComplete, FBPOnBulletHit, FBPOnBulletExitHit, FBPOnBulletInjure, FBPOnProjectileUpdate, TB::SimTasks::FBulletTaskDelegates> FTBBulletTaskCallbacks;
typedef TTBTaskCallbacks<FBPOnProjectileComplete, FBPOnProjectileHit, FBPOnProjectileExitHit, FBPOnProjectileInjure, FBPOnProjectileUpdate, TB::SimTasks::FProjectileTaskDelegates> FTBProjectileTaskCallbacks;

/**
* 
* Simplified Projectile Lifetime:
* 
*	Tick:
*		Pending removals are processed.		(RemovalQueue)
*		Pending additions are processed.	(ToAdd)
*		Any inactive items that are requesting to be made active are added to the Active array.
*		Pending launches are processed.		(ToLaunch)
*		Results are taken from the ProjectileThread and processed. (GetResultsFromProjectileThread)
*	
*	Add:
*		Data is converted to FTBSimData and assigned an Id. 
*		If projectiles can be added:
			SimData is added to the Inactive array, ready to be launched.
*		Otherwise:
			SimData is added to the ToAdd queue, and will be added next tick.
* 
*	Remove:
*		Appropriate arrays are locked. (Inactive and Active)
*		Data is removed	from active/inactive arrays and destructed.
*		Arrays are unlocked.
* 
*	Fire:
*		SimData is retrieved from the Inactive array.
*		If launching is not allowed:
*			launch data is added to the ToLaunch queue, and will be launched next tick.
*		Otherwise:
*			SimData is configured with the given launch data. (FTBLaunchParams, DebugType, etc...)
*			Projectile is launched.
*			SimData moved from Inactive to Active.
* 
*	OnProjectileComplete:
*		Projectile is added to the removal queue.
* 
* Simplified Projectile Lifetime on the ProjectileThread:
* 
*		Projectiles are added to the thread using the thread's DataQueue input. (DataQueue->Enqueue)
*		From there, the Projectile is picked up by the ProjectileThread and it begins simulation.
*		Delegates bound to the Projectile are executed on the game thread using the task graph system.
* 
*		Once complete, the Projectile is removed from the ProjectileThread and it's TaskResult is added to the DataQueue's output.
*		Next tick (or potentially the current tick), the DataQueue's output is checked by this subsystem, where TaskResults are retrieved from the DataQueue (DataQueue->Dequeue) and processed.
*		Then they are removed and the associated Projectile is added to the removal queue.
* 
*/
UCLASS(meta = (ShortToolTip="Subsystem that handles ballistics."))
class TERMINALBALLISTICS_API UTerminalBallisticsSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	UTerminalBallisticsSubsystem();

	// FTickableGameObject implementation Begin
	virtual ~UTerminalBallisticsSubsystem() override
	{
		ShutdownProjectileThread();
		ActiveBullets.Empty();
		InactiveBullets.Empty();
		ActiveProjectiles.Empty();
		InactiveProjectiles.Empty();
	}
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTerminalBallisticsSubsystem, STATGROUP_Tickables); }
	// FTickableGameObject implementation End

	// USubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	// USubsystem implementation End


	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

#pragma region Getters
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem")
	inline int GetNumActiveBullets() const { return ActiveBullets.Num(); }
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem")
	inline int GetNumActiveProjectiles() const { return ActiveProjectiles.Num(); }
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem")
	inline bool HasActiveBullets() const { return !ActiveBullets.IsEmpty(); }
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem")
	inline bool HasActiveProjectiles() const { return !ActiveProjectiles.IsEmpty(); }

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem")
	inline int GetNumInactiveBullets() const { return InactiveBullets.Num(); }
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem")
	inline int GetNumInactiveProjectiles() const { return InactiveProjectiles.Num(); }
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem")
	inline bool HasInactiveBullets() const { return !InactiveBullets.IsEmpty(); }
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem")
	inline bool HasInactiveProjectiles() const { return !InactiveProjectiles.IsEmpty(); }
	
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem")
	inline int GetTotalBullets() const { return InactiveBullets.Num() + ActiveBullets.Num(); }
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem")
	inline int GetTotalProjectiles() const { return InactiveProjectiles.Num() + ActiveProjectiles.Num(); }
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem")
	bool HasAnyBullets() const;
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem")
	bool HasAnyProjectiles() const;

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem")
	/**
	* Checks to see if there are any bullets/projectiles on the Projectile thread.
	*/
	bool HasAnyBulletsOrProjectiles() const;

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem", meta = (ReturnDisplayName = "Bullet Found"))
	/**
	* Checks to see if there is a bullet with the given Id, or if one is waiting to be added.
	* @param Id		The Id to check for.
	*/
	bool HasBullet(const FTBProjectileId& Id) const;
	bool HasBullet(const FTBBulletSimData Bullet) const;

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem", meta = (ReturnDisplayName = "Projectile Found"))
	/**
	* Checks to see if there is a projectile with the given Id, or if one is waiting to be added.
	* @param Id		The Id to check for.
	*/
	bool HasProjectile(const FTBProjectileId& Id) const;
	bool HasProjectile(const FTBProjectileSimData& Projectile) const;

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem", meta = (ReturnDisplayName = "Bullet Found"))
	/**
	* Checks to see if there is a bullet with the given Id currently inactive.
	* @param Id		The Id to check for.
	*/
	inline bool HasBulletInInactive(const FTBProjectileId& Id) const
	{
		auto Pred = [Id](const FTBBulletSimData& SimData) { return SimData.GetId() == Id; };
		return InactiveBullets.ContainsByPredicate(Pred);
	}
	inline bool HasBulletInInactive(const FTBBulletSimData Bullet) const
	{
		return InactiveBullets.Contains(Bullet);
	}

	inline FTBBulletSimData* GetBulletFromInactive(const FTBProjectileId& Id)
	{
		auto Pred = [Id](const FTBBulletSimData& SimData) { return SimData.GetId() == Id; };
		FReadScopeLock Lock(InactiveBulletsLock);
		return InactiveBullets.FindByPredicate(Pred);
	}


	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem", meta = (ReturnDisplayName = "Projectile Found"))
	/**
	* Checks to see if there is a bullet with the given Id currently inactive.
	* @param Id		The Id to check for.
	*/
	inline bool HasProjectileInInactive(const FTBProjectileId& Id) const
	{
		auto Pred = [Id](const FTBProjectileSimData& SimData) { return SimData.GetId() == Id; };
		return InactiveProjectiles.ContainsByPredicate(Pred);
	}
	inline bool HasProjectileInInactive(const FTBProjectileSimData& Projectile) const
	{
		return InactiveProjectiles.Contains(Projectile);
	}

	inline FTBProjectileSimData* GetProjectileFromInactive(const FTBProjectileId& Id)
	{
		auto Pred = [Id](const FTBProjectileSimData& SimData) { return SimData.GetId() == Id; };
		FReadScopeLock Lock(InactiveProjectilesLock);
		return InactiveProjectiles.FindByPredicate(Pred);
	}

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem", meta = (ReturnDisplayName = "Bullet Found"))
	bool GetBullet(const FTBProjectileId& Id, FTBBulletSimData& Bullet) const;
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics Subsystem", meta = (ReturnDisplayName = "Projectile Found"))
	bool GetProjectile(const FTBProjectileId& Id, FTBProjectileSimData& Projectile) const;
#pragma endregion

	FORCEINLINE bool CanFire() const
	{
		return ProjectilesLaunchedThisTick < TB::Configuration::MaxLaunchesPerTick
			&& !bShuttingDown;
	}

	double UpdateProjectileDrag(const double V, const double GravityZ, const FVector& Location, const FTBProjectilePhysicalProperties& ProjectileProperties, ETBDragComplexity DragCalculationType = ETBDragComplexity::DEFAULT, double p = -1, const bool bAllowAtmosphericDensityOverride = true);
	FVector CalculateProjectileDrag(const FVector& V, const double GravityZ, const FVector& Location, const FRotator& ProjectileRotation, const FTBProjectilePhysicalProperties& ProjectileProperties, ETBDragComplexity DragCalculationType = ETBDragComplexity::DEFAULT, double p = -1, const bool bAllowAtmosphericDensityOverride = true);

	/* Add/Remove/Get methods for projectiles and bullets are thread-safe */
#pragma region Bullet Functions
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (AdvancedDisplay = "DebugType"))
	FTBBulletSimData CreateBulletSimDataFromDataAsset(const UBulletDataAsset* BulletDataAsset, const FTBProjectileId& Id, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType")) int32 DebugType = 0);

	TArray<FTBBulletSimData> CreateBulletSimDataFromDataAssetMultiple(const UBulletDataAsset* BulletDataAsset, const FTBProjectileId Id, const int32 DebugType = 0);

	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem")
	void BindFunctionsToBulletSimData(UPARAM(Ref)FTBBulletSimData& BulletSimData, FBPOnProjectileComplete BulletComplete, FBPOnBulletHit BulletHit, FBPOnBulletExitHit BulletExitHit, FBPOnBulletInjure BulletInjure);

	/**
	 * Adds a bullet to the subsystem and returns a unique Id for that bullet.
	 *
	 * @param ToAdd				The bullet to add.
	 * @param Owner				The actor that owns the bullet (optional).
	 * @return FTBProjectileId	The Id assigned to the added bullet.
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (AdvancedDisplay = "Id", ReturnDisplayName = "Id", DefaultToSelf = "Owner"))
	FTBProjectileId AddBullet(UPARAM(Ref)FTBBulletSimData& ToAdd, AActor* Owner, AController* Instigator = nullptr);
	/**
	 * Adds a bullet to the subsystem and returns a unique Id for that bullet.
	 *
	 * @param Bullet			The bullet to add.
	 * @param Owner				The actor that owns the bullet (optional).
	 * @param Id				The Id of the bullet. If not provided, one will be generated.
	 * @return FTBProjectileId	The Id assigned to the added bullet, if one was not provided.
	 */
	FTBProjectileId AddBullet(BulletPointer Bullet, AActor* Owner, AController* Instigator = nullptr, const FTBProjectileId& Id = FTBProjectileId::None);

	/**
	 * Adds a bullet from a data asset to the subsystem for simulation and returns the Id of that bullet.
	 *
	 * @param BulletDataAsset	A data asset containing the bullet data.
	 * @param Owner				The actor that owns the bullet.
	 * @param Id				The Id of the bullet.
	 * @param DebugType			Debug type for the bullet.
	 * @return FTBProjectileId	The Id assigned to the added bullet, if one was not provided.
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (AdvancedDisplay = "DebugType, Id", ReturnDisplayName = "Id", DefaultToSelf = "Owner"))
	FTBProjectileId AddBulletFromDataAsset(const UBulletDataAsset* BulletDataAsset, AActor* Owner, const FTBProjectileId& Id, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType")) int32 DebugType = 0);

	/**
	 * Adds a bullet with various callback functions and returns a unique Id.
	 *
	 * @param ToAdd				The bullet to add.
	 * @param OnBulletComplete	Callback function for bullet completion.
	 * @param OnBulletHit		Callback function for bullet hitting an object.
	 * @param OnBulletExitHit	Callback function for bullet exiting after hitting an object.
	 * @param OnBulletInjure	Callback function for bullet causing injury to a player/pawn.
	 * @param Owner				The actor that owns the bullet (optional).
	 * @return FTBProjectileId	The Id assigned to the added bullet.
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (ReturnDisplayName = "Id"))
	FTBProjectileId AddBulletWithCallbacks(
		UPARAM(Ref)FTBBulletSimData& ToAdd,
		FBPOnProjectileComplete OnBulletComplete, 
		FBPOnBulletHit OnBulletHit, 
		FBPOnBulletExitHit OnBulletExitHit, 
		FBPOnBulletInjure OnBulletInjure,
		AActor* Owner
	);
	FTBProjectileId AddBulletWithCallbacks(
		BulletPointer Bullet,
		FBPOnProjectileComplete OnBulletComplete, 
		FBPOnBulletHit OnBulletHit, 
		FBPOnBulletExitHit OnBulletExitHit, 
		FBPOnBulletInjure OnBulletInjure,
		AActor* Owner,
		const FTBProjectileId& Id = FTBProjectileId::None
	);

	/**
	 * Adds a bullet with callbacks and a delegate to be called on update. Returns the unique Id assigned to that bullet.
	 *
	 * @param ToAdd				The bullet to add.
	 * @param OnBulletComplete	Callback function for bullet completion.
	 * @param OnBulletHit		Callback function for bullet hitting an object.
	 * @param OnBulletExitHit	Callback function for bullet exiting after hitting an object.
	 * @param OnBulletInjure	Callback function for bullet causing injury to a player/pawn.
	 * @param OnUpdate			Callback function for continuous bullet updates.
	 * @param Owner				The actor that owns the bullet (optional).
	 * @return FTBProjectileId	The Id assigned to the added bullet.
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (ReturnDisplayName = "Id"))
	FTBProjectileId AddBulletWithCallbacksAndUpdate(
		UPARAM(Ref)FTBBulletSimData& ToAdd,
		FBPOnProjectileComplete OnBulletComplete,
		FBPOnBulletHit OnBulletHit,
		FBPOnBulletExitHit OnBulletExitHit,
		FBPOnBulletInjure OnBulletInjure,
		FBPOnProjectileUpdate OnUpdate,
		AActor* Owner
	);
	FTBProjectileId AddBulletWithCallbacksAndUpdate(
		UPARAM(Ref)FTBBulletSimData& ToAdd,
		FBPOnProjectileComplete OnBulletComplete,
		FBPOnBulletHit OnBulletHit,
		FBPOnBulletExitHit OnBulletExitHit,
		FBPOnBulletInjure OnBulletInjure,
		FBPOnProjectileUpdate OnUpdate,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
		AActor* Owner
	);
	FTBProjectileId AddBulletWithCallbacksAndUpdate(
		UPARAM(Ref)FTBBulletSimData& ToAdd,
		FBPOnProjectileComplete OnBulletComplete,
		FBPOnBulletHit OnBulletHit,
		FBPOnBulletExitHit OnBulletExitHit,
		FBPOnBulletInjure OnBulletInjure,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
		AActor* Owner
	);
	FTBProjectileId AddBulletWithCallbacksAndUpdate(
		BulletPointer Bullet,
		FBPOnProjectileComplete OnBulletComplete,
		FBPOnBulletHit OnBulletHit,
		FBPOnBulletExitHit OnBulletExitHit,
		FBPOnBulletInjure OnBulletInjure,
		FBPOnProjectileUpdate OnUpdate,
		AActor* Owner,
		const FTBProjectileId& Id = FTBProjectileId::None
	);
	FTBProjectileId AddBulletWithCallbacksAndUpdate(
		BulletPointer Bullet,
		FBPOnProjectileComplete OnBulletComplete,
		FBPOnBulletHit OnBulletHit,
		FBPOnBulletExitHit OnBulletExitHit,
		FBPOnBulletInjure OnBulletInjure,
		FBPOnProjectileUpdate OnUpdate,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
		AActor* Owner,
		const FTBProjectileId& Id = FTBProjectileId::None
	);
	FTBProjectileId AddBulletWithCallbacksAndUpdate(
		BulletPointer Bullet,
		FBPOnProjectileComplete OnBulletComplete,
		FBPOnBulletHit OnBulletHit,
		FBPOnBulletExitHit OnBulletExitHit,
		FBPOnBulletInjure OnBulletInjure,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
		AActor* Owner,
		const FTBProjectileId& Id = FTBProjectileId::None
	);

	/**
	 * Adds a bullet with delegate to be called on update. Returns the unique Id assigned to that bullet.
	 *
	 * @param ToAdd				The bullet to add.
	 * @param OnUpdate			Callback function for continuous bullet updates.
	 * @param Owner				The actor that owns the bullet (optional).
	 * @return FTBProjectileId	The Id assigned to the added bullet.
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (ReturnDisplayName = "Id"))
	FTBProjectileId AddBulletWithUpdate(
		UPARAM(Ref)FTBBulletSimData& ToAdd,
		FBPOnProjectileUpdate OnUpdate,
		AActor* Owner
	);
	FTBProjectileId AddBulletWithUpdate(
		UPARAM(Ref)FTBBulletSimData& ToAdd,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
		AActor* Owner
	);
	FTBProjectileId AddBulletWithUpdate(
		UPARAM(Ref)FTBBulletSimData& ToAdd,
		FBPOnProjectileUpdate OnUpdate,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
		AActor* Owner
	);
	FTBProjectileId AddBulletWithUpdate(
		BulletPointer Bullet,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
		AActor* Owner
	)
	{
		FTBBulletSimData SimData(Bullet, 0);
		return AddBulletWithUpdate(SimData, OnUpdateFunction, Owner);
	}
	FTBProjectileId AddBulletWithUpdate(
		BulletPointer Bullet,
		FBPOnProjectileUpdate OnUpdate,
		AActor* Owner,
		const FTBProjectileId& Id = FTBProjectileId::None
	)
	{
		FTBBulletSimData SimData(Bullet, 0, 0, false, Id);
		return AddBulletWithUpdate(SimData, OnUpdate, Owner);
	}
	FTBProjectileId AddBulletWithUpdate(
		BulletPointer Bullet,
		FBPOnProjectileUpdate OnUpdate,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
		AActor* Owner,
		const FTBProjectileId& Id = FTBProjectileId::None
	)
	{
		FTBBulletSimData SimData(Bullet, 0, 0, false, Id);
		return AddBulletWithUpdate(SimData, OnUpdate, OnUpdateFunction, Owner);
	}

	/**
	 * Removes a bullet.
	 * This will stop any ongoing simulation of that bullet and also remove it from the pool of inactive bullets waiting to be fired.
	 *
	 * @param ToRemove The bullet to remove.
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem")
	void RemoveBullet(UPARAM(Ref)FTBBulletSimData& ToRemove);

	/**
	 * Removes a bullet based on its Id.
	 * This will stop any ongoing simulation of that bullet and also remove it from the pool of inactive bullets waiting to be fired.
	 *
	 * @param Id The unique Id of the bullet to remove.
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem")
	void RemoveBulletById(const FTBProjectileId& Id);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (AdvancedDisplay = "DebugType"))
	void FireBullet(const FTBProjectileId& BulletId, const FTBLaunchParams& LaunchParams, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType")) int32 DebugType = 0);

	/**
	* Explicitly fires a bullet.
	* Bullet is assumed to have already been added to the subsystem. If not, this can result in side effects.
	*/
	void FireBulletExplicit(FTBBulletSimData* BulletToFire, const FTBLaunchParams& LaunchParams, int32 DebugType = 0);

	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (AdvancedDisplay = "DebugType", ReturnDisplayName = "Id"))
	inline FTBProjectileId AddAndFireBullet(const UBulletDataAsset* BulletDataAsset, const FTBLaunchParams& LaunchParams, const FTBProjectileId& Id, AActor* Owner, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType")) int32 DebugType = 0)
	{
		FTBBulletSimData SimData = CreateBulletSimDataFromDataAsset(BulletDataAsset, Id, DebugType);
		return AddAndFireBullet(SimData, LaunchParams, Owner, DebugType);
	}

	inline FTBProjectileId AddAndFireBullet(BulletPointer Bullet, const FTBLaunchParams& LaunchParams, AActor* Owner, int32 DebugType = 0, const FTBProjectileId& Id = FTBProjectileId::None)
	{
		FTBBulletSimData SimData = FTBBulletSimData(Bullet, DebugType);
		SimData.SetId(Id);
		return AddAndFireBullet(SimData, LaunchParams, Owner, DebugType);
	}

	FTBProjectileId AddAndFireBullet(FTBBulletSimData& SimData, const FTBLaunchParams& LaunchParams, AActor* Owner, int32 DebugType = 0);

	TArray<FTBProjectileId> AddAndFireBullets(TArray<FTBBulletSimData>& SimData, const FTBLaunchParams& LaunchParams, int32 DebugType = 0, TOptional<FTBBulletTaskCallbacks> Delegates = TOptional<FTBBulletTaskCallbacks>());
	TArray<FTBProjectileId> AddAndFireBullets(TArray<FTBBulletSimData>& SimData, const TArray<FTBLaunchParams>& LaunchParams, int32 DebugType = 0, TOptional<FTBBulletTaskCallbacks> Delegates = TOptional<FTBBulletTaskCallbacks>());

	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (AdvancedDisplay = "DebugType", ReturnDisplayName = "Id"))
	FTBProjectileId AddAndFireBulletWithCallbacks(
		const UBulletDataAsset* BulletDataAsset, 
		const FTBLaunchParams& LaunchParams,
		FBPOnProjectileComplete OnBulletComplete, 
		FBPOnBulletHit OnBulletHit, 
		FBPOnBulletExitHit OnBulletExitHit, 
		FBPOnBulletInjure OnBulletInjure,
		AActor* Owner,
		const FTBProjectileId& Id,
		UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType")) int32 DebugType = 0
	);

	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (AdvancedDisplay = "DebugType", ReturnDisplayName = "Id"))
	FTBProjectileId AddAndFireBulletWithCallbacksAndUpdate(
		const UBulletDataAsset* BulletDataAsset,
		const FTBLaunchParams& LaunchParams,
		FBPOnProjectileComplete OnBulletComplete,
		FBPOnBulletHit OnBulletHit,
		FBPOnBulletExitHit OnBulletExitHit,
		FBPOnBulletInjure OnBulletInjure,
		FBPOnProjectileUpdate OnUpdate,
		AActor* Owner,
		const FTBProjectileId& Id,
		UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType")) int32 DebugType = 0
	);

	FTBProjectileId AddAndFireBulletWithCallbacks(
		BulletPointer Bullet,
		const FTBLaunchParams& LaunchParams,
		FBPOnProjectileComplete OnBulletComplete,
		FBPOnBulletHit OnBulletHit,
		FBPOnBulletExitHit OnBulletExitHit,
		FBPOnBulletInjure OnBulletInjure,
		AActor* Owner,
		const FTBProjectileId& Id = FTBProjectileId::None,
		int32 DebugType = 0
	);

	FTBProjectileId AddAndFireBulletWithCallbacks(
		BulletPointer Bullet,
		const FTBLaunchParams& LaunchParams,
		FBPOnProjectileComplete OnBulletComplete,
		FBPOnBulletHit OnBulletHit,
		FBPOnBulletExitHit OnBulletExitHit,
		FBPOnBulletInjure OnBulletInjure,
		FBPOnProjectileUpdate OnUpdate,
		AActor* Owner,
		const FTBProjectileId& Id = FTBProjectileId::None,
		int32 DebugType = 0
	);

	FTBProjectileId AddAndFireBulletWithCallbacks(
		BulletPointer Bullet,
		const FTBLaunchParams& LaunchParams,
		TB::SimTasks::FBulletTaskDelegates Delegates,
		AActor* Owner,
		const FTBProjectileId& Id = FTBProjectileId::None,
		int32 DebugType = 0
	);

	FTBProjectileId AddAndFireBulletWithCallbacks(
		BulletPointer Bullet,
		const FTBLaunchParams& LaunchParams,
		TB::SimTasks::FBulletTaskDelegates Delegates,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdate,
		AActor* Owner,
		const FTBProjectileId& Id = FTBProjectileId::None,
		int32 DebugType = 0
	);

	FTBProjectileId AddAndFireBulletWithUpdate(
		BulletPointer Bullet,
		const FTBLaunchParams& LaunchParams,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdate,
		AActor* Owner,
		const FTBProjectileId& Id = FTBProjectileId::None,
		int32 DebugType = 0
	);

	FTBProjectileId AddAndFireBulletWithCallbacks(
		const UBulletDataAsset* BulletDataAsset,
		const FTBLaunchParams& LaunchParams,
		TB::SimTasks::FBulletTaskDelegates Delegates,
		AActor* Owner,
		const FTBProjectileId& Id = FTBProjectileId::None,
		int32 DebugType = 0
	);

	FTBProjectileId AddAndFireBulletWithCallbacks(
		const UBulletDataAsset* BulletDataAsset,
		const FTBLaunchParams& LaunchParams,
		TB::SimTasks::FBulletTaskDelegates Delegates,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdate,
		AActor* Owner,
		const FTBProjectileId& Id = FTBProjectileId::None,
		int32 DebugType = 0
	);

	FTBProjectileId AddAndFireBulletWithUpdate(
		const UBulletDataAsset* BulletDataAsset,
		const FTBLaunchParams& LaunchParams,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdate,
		AActor* Owner,
		const FTBProjectileId& Id = FTBProjectileId::None,
		int32 DebugType = 0
	);
#pragma endregion

#pragma region Projectile Functions
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem")
	void BindFunctionsToProjectileSimData(UPARAM(Ref)FTBProjectileSimData& ProjectileSimData, FBPOnProjectileComplete OnProjectileComplete, FBPOnProjectileHit OnProjectileHit, FBPOnProjectileExitHit OnProjectileExitHit, FBPOnProjectileInjure OnProjectileInjure);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (ReturnDisplayName = "Id", DefaultToSelf = "Owner"))
	FTBProjectileId AddProjectile(UPARAM(Ref)FTBProjectileSimData& ToAdd, AActor* Owner, AController* Instigator = nullptr);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (ReturnDisplayName = "Id", DefaultToSelf = "Owner"))
	FTBProjectileId AddProjectileWithCallbacks(
		UPARAM(Ref)FTBProjectileSimData& ToAdd,
		FBPOnProjectileComplete OnProjectileComplete,
		FBPOnProjectileHit OnProjectileHit,
		FBPOnProjectileExitHit OnProjectileExitHit,
		FBPOnProjectileInjure OnProjectileInjure,
		AActor* Owner
	);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (ReturnDisplayName = "Id", DefaultToSelf = "Owner"))
	FTBProjectileId AddProjectileWithCallbacksAndUpdate(
		UPARAM(Ref)FTBProjectileSimData& ToAdd,
		FBPOnProjectileComplete OnProjectileComplete,
		FBPOnProjectileHit OnProjectileHit,
		FBPOnProjectileExitHit OnProjectileExitHit,
		FBPOnProjectileInjure OnProjectileInjure,
		FBPOnProjectileUpdate OnUpdate,
		AActor* Owner
	);
	FTBProjectileId AddProjectileWithCallbacksAndUpdate(
		UPARAM(Ref)FTBProjectileSimData& ToAdd,
		FBPOnProjectileComplete OnProjectileComplete,
		FBPOnProjectileHit OnProjectileHit,
		FBPOnProjectileExitHit OnProjectileExitHit,
		FBPOnProjectileInjure OnProjectileInjure,
		FBPOnProjectileUpdate OnUpdate,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
		AActor* Owner
	);
	FTBProjectileId AddProjectileWithCallbacksAndUpdate(
		UPARAM(Ref)FTBProjectileSimData& ToAdd,
		FBPOnProjectileComplete OnProjectileComplete,
		FBPOnProjectileHit OnProjectileHit,
		FBPOnProjectileExitHit OnProjectileExitHit,
		FBPOnProjectileInjure OnProjectileInjure,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
		AActor* Owner
	);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (ReturnDisplayName = "Id", DefaultToSelf = "Owner"))
	FTBProjectileId AddProjectileWithUpdate(
		UPARAM(Ref)FTBProjectileSimData& ToAdd,
		FBPOnProjectileUpdate OnUpdate,
		AActor* Owner
	);
	FTBProjectileId AddProjectileWithUpdate(
		UPARAM(Ref)FTBProjectileSimData& ToAdd,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
		AActor* Owner
	);
	FTBProjectileId AddProjectileWithUpdate(
		UPARAM(Ref)FTBProjectileSimData& ToAdd,
		FBPOnProjectileUpdate OnUpdate,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdateFunction,
		AActor* Owner
	);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (ReturnDisplayName = "Ids", DefaultToSelf = "Owner"))
	TArray<FTBProjectileId> AddProjectiles(UPARAM(Ref)TArray<FTBProjectileSimData>& ToAdd, AActor* Owner= nullptr);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem")
	void RemoveProjectile(const FTBProjectileSimData& ToRemove);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem")
	void RemoveProjectileById(const FTBProjectileId& Id);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem")
	void RemoveProjectiles(const TArray<FTBProjectileSimData>& ToRemove);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem")
	void RemoveProjectilesById(const TArray<FTBProjectileId>& ToRemove);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (AdvancedDisplay = "DebugType"))
	void FireProjectile(const FTBProjectileId& Id, const FTBLaunchParams& LaunchParams, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType")) int32 DebugType = 0);

	/**
	* Explicitly fires a Projectile.
	* Projectile is assumed to have already been added to the subsystem. If not, this can result in side effects.
	*/
	void FireProjectileExplicit(FTBProjectileSimData* ProjectileToFire, const FTBLaunchParams& LaunchParams, int32 DebugType = 0);

	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem")
	void FireProjectiles(const TArray<FTBLaunchData> ProjectilesToLaunch);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (AdvancedDisplay = "DebugType", DefaultToSelf = "Owner", ReturnDisplayName = "Id"))
	FTBProjectileId AddAndFireProjectile(UPARAM(Ref)FTBProjectileSimData& ProjectileSimData, const FTBLaunchParams& LaunchParams, AActor* Owner, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType")) int32 DebugType = 0);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (AdvancedDisplay = "DebugType", DefaultToSelf = "Owner", ReturnDisplayName = "Id"))
	FTBProjectileId AddAndFireProjectileWithCallbacks(
		UPARAM(Ref)FTBProjectileSimData& ProjectileSimData,
		const FTBLaunchParams& LaunchParams,
		FBPOnProjectileComplete OnProjectileComplete,
		FBPOnProjectileHit OnProjectileHit,
		FBPOnProjectileExitHit OnProjectileExitHit,
		FBPOnProjectileInjure OnProjectileInjure,
		AActor* Owner,
		UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType")) int32 DebugType = 0
	);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics Subsystem", meta = (AdvancedDisplay = "DebugType", DefaultToSelf = "Owner", ReturnDisplayName = "Id"))
	FTBProjectileId AddAndFireProjectileWithCallbacksAndUpdate(
		UPARAM(Ref)FTBProjectileSimData& ProjectileSimData,
		const FTBLaunchParams& LaunchParams,
		FBPOnProjectileComplete OnProjectileComplete,
		FBPOnProjectileHit OnProjectileHit,
		FBPOnProjectileExitHit OnProjectileExitHit,
		FBPOnProjectileInjure OnProjectileInjure,
		FBPOnProjectileUpdate OnUpdate,
		AActor* Owner,
		UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType")) int32 DebugType = 0
	);

	FTBProjectileId AddAndFireProjectileWithCallbacks(
		FTBProjectileSimData& ProjectileSimData,
		const FTBLaunchParams& LaunchParams,
		TB::SimTasks::FProjectileTaskDelegates Delegates,
		AActor* Owner,
		int32 DebugType = 0
	);

	FTBProjectileId AddAndFireProjectileWithCallbacks(
		FTBProjectileSimData& ProjectileSimData,
		const FTBLaunchParams& LaunchParams,
		TB::SimTasks::FProjectileTaskDelegates Delegates,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdate,
		AActor* Owner,
		int32 DebugType = 0
	);

	FTBProjectileId AddAndFireProjectileWithCallbacks(
		FTBProjectileSimData& ProjectileSimData,
		const FTBLaunchParams& LaunchParams,
		TFunction<void(const FTBProjectileFlightData&)> OnUpdate,
		AActor* Owner,
		int32 DebugType = 0
	);
#pragma endregion

	void BindDelegates(FTBBulletSimData& SimData, FTBBulletTaskCallbacks Delegates);
	void BindDelegates(FTBProjectileSimData& SimData, FTBProjectileTaskCallbacks Delegates);
protected:

	TArray<FTBProjectileId> AddBullets(TArray<FTBBulletSimData>& SimData, const TArray<FTBLaunchParams>& LaunchParams, TOptional<FTBBulletTaskCallbacks> Delegates = TOptional<FTBBulletTaskCallbacks>());
	TArray<FTBProjectileId> AddProjectiles(TArray<FTBProjectileSimData>& SimData, const TArray<FTBLaunchParams>& LaunchParams, TOptional<FTBProjectileTaskCallbacks> Delegates = TOptional<FTBProjectileTaskCallbacks>());

	friend class FTBGetSubsystemHelper;

	UWorld* GetOrUpdateWorld() const;

	static bool SupportsNetMode(const ENetMode NetMode);

	void RemoveBulletByIdIgnoreThread(const FTBProjectileId& Id);
	void RemoveProjectileByIdIgnoreThread(const FTBProjectileId& Id);

	bool TryGetEnvironmentSubsystem();

	static int32 ThreadCount;

	std::atomic<int32> ProjectilesLaunchedThisTick;

	std::atomic_bool bShuttingDown; // If the world cleanup has begun or the world is preparing to shut down, make sure we don't continue launching projectiles and firing delegates.

	UFUNCTION()
	void StartShutdown(UWorld* UnusedWorld, bool UnusedBool1 = false, bool UnusedBool2 = false);

	FDelegateHandle OnWorldBeginTearDownHandle;
	FDelegateHandle OnWorldCleanupHandle;

	UPROPERTY()
	UEnvironmentSubsystem* EnvironmentSubsystem = nullptr;

	UPROPERTY()
	mutable TObjectPtr<UWorld> World;
	
	bool bHasValidGameMode = false;

	bool bWasPaused = false;


	using ProjectileOrId = TVariant<FTBProjectileSimData, FTBProjectileId>;
	using BulletOrId = TVariant<FTBBulletSimData, FTBProjectileId>;

	TQueue<ProjectileOrId, EQueueMode::Mpsc> ProjectileRemovalQueue;
	TQueue<BulletOrId, EQueueMode::Mpsc> BulletRemovalQueue;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Terminal Ballistics Subsystem")
	TArray<FTBBulletSimData> ActiveBullets;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Terminal Ballistics Subsystem")
	TArray<FTBBulletSimData> InactiveBullets;

	TQueue<FTBBulletSimData, EQueueMode::Mpsc> BulletsToMakeActive;
	FTBBulletSimDataArray BulletsToAdd;
	TQueue<FTBLaunchData, EQueueMode::Mpsc> BulletLaunchQueue;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Terminal Ballistics Subsystem")
	TArray<FTBProjectileSimData> ActiveProjectiles;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Terminal Ballistics Subsystem")
	TArray<FTBProjectileSimData> InactiveProjectiles;

	TQueue<FTBProjectileSimData, EQueueMode::Mpsc> ProjectilesToMakeActive;
	FTBProjectileSimDataArray ProjectilesToAdd;
	TQueue<FTBLaunchData, EQueueMode::Mpsc> ProjectileLaunchQueue;

	friend struct FTBSimData;
	friend class FTBProjectileThread;
	UFUNCTION()
	void CallOnBulletComplete(const FTBProjectileId& Id, const TArray<FPredictProjectilePathPointData>& pathResults);
	UFUNCTION()
	void CallOnProjectileComplete(const FTBProjectileId& Id, const TArray<FPredictProjectilePathPointData>& pathResults);

	/* Default script delegates */
	FScriptDelegate onBulletHit;
	FScriptDelegate onBulletExitHit;
	FScriptDelegate onBulletInjure;

	FScriptDelegate onProjectileHit;
	FScriptDelegate onProjectileExitHit;
	FScriptDelegate onProjectileInjure;


	inline void BindDefaultDelegates(FTBBulletSimData& SimData)
	{
		if (GetOrUpdateWorld()->GetNetMode() < ENetMode::NM_Client) // Since the client can't access the game mode anyway, there's no reason to bind those delegates.
		{
			if (!SimData.DefaultDelegatesAreBound)
			{
				SimData.onBulletHit.Add(onBulletHit);
				SimData.onBulletExitHit.Add(onBulletExitHit);
				SimData.onBulletInjure.Add(onBulletInjure);
				SimData.DefaultDelegatesAreBound = true;
			}
		}
	}
	inline void BindDefaultDelegates(FTBBulletSimData* SimData)
	{
		if (GetOrUpdateWorld()->GetNetMode() < ENetMode::NM_Client)
		{
			if (!SimData->AreDefaultDelegatesBound())
			{
				SimData->onBulletHit.Add(onBulletHit);
				SimData->onBulletExitHit.Add(onBulletExitHit);
				SimData->onBulletInjure.Add(onBulletInjure);
				SimData->DefaultDelegatesAreBound = true;
			}
		}
	}
	inline void BindDefaultDelegates(FTBProjectileSimData& SimData)
	{
		if (GetOrUpdateWorld()->GetNetMode() < ENetMode::NM_Client)
		{
			if (!SimData.DefaultDelegatesAreBound)
			{
				SimData.onProjectileHit.Add(onProjectileHit);
				SimData.onProjectileExitHit.Add(onProjectileExitHit);
				SimData.onProjectileInjure.Add(onProjectileInjure);
				SimData.DefaultDelegatesAreBound = true;
			}
		}
	}
	inline void BindDefaultDelegates(FTBProjectileSimData* SimData)
	{
		if (GetOrUpdateWorld()->GetNetMode() < ENetMode::NM_Client)
		{
			if (!SimData->AreDefaultDelegatesBound())
			{
				SimData->onProjectileHit.Add(onProjectileHit);
				SimData->onProjectileExitHit.Add(onProjectileExitHit);
				SimData->onProjectileInjure.Add(onProjectileInjure);
				SimData->DefaultDelegatesAreBound = true;
			}
		}
	}

#pragma region Game Mode Calls
	UFUNCTION()
	void CallGameModeHit(const FTBImpactParams& ImpactParams);
	UFUNCTION()
	void CallGameModeHitBasic(const FTBImpactParamsBasic& ImpactParams);
	UFUNCTION()
	void CallGameModeExitHit(const FTBImpactParams& ImpactParams);
	UFUNCTION()
	void CallGameModeExitHitBasic(const FTBImpactParamsBasic& ImpactParams);
	UFUNCTION()
	void CallGameModeInjure(const FTBImpactParams& ImpactParams, const FTBProjectileInjuryParams& Injury);
	UFUNCTION()
	void CallGameModeInjureBasic(const FTBImpactParamsBasic& ImpactParams, const FTBProjectileInjuryParams& Injury);
#pragma endregion

private:
	template<typename TypeOfSimData>
	typename TEnableIf<TB::Traits::TIsSimData<TypeOfSimData>::Value, void>::Type _SetupProjectileLaunchData(TypeOfSimData* ProjectileToFire, const FTBLaunchParams& LaunchParams, int32 DebugType);
	void _FireBulletInternal(const FTBProjectileId& BulletId, const FTBLaunchParams& LaunchParams, int32 DebugType = 0);
	void _FireProjectileInternal(const FTBProjectileId& Id, const FTBLaunchParams& LaunchParams, int32 DebugType = 0);
	template<typename TypeOfSimData>
	typename TEnableIf<TB::Traits::TIsSimData<TypeOfSimData>::Value, void>::Type _FireProjectileImpl(TypeOfSimData* ProjectileToFire, const FTBLaunchParams& LaunchParams, int32 DebugType = 0);

	template<typename TypeOfSimData>
	typename TEnableIf<TB::Traits::TIsSimData<TypeOfSimData>::Value, void>::Type _SetupProjectile(TypeOfSimData* ProjectileToFire, const FTBLaunchParams& LaunchParams, int32 DebugType);

	template<typename TypeOfSimData>
	typename TEnableIf<TB::Traits::TIsSimData<std::decay_t<TypeOfSimData>>::Value, void>::Type FireMultiple(TArray<TypeOfSimData> ProjectilesToFire, const TArray<FTBLaunchData>& LaunchData);
protected:
#pragma region Threading
	class FTBProjectileThread* ProjectileThread = nullptr;

	UPROPERTY()
	UTBProjectileThreadQueue* ProjectileThreadQueue = nullptr;

	void InitProjectileThread();
	void ShutdownProjectileThread();

private:
	void GetResultsFromProjectileThread();

	/**
	* Function to do any cleanup or processing on a task result before it is destroyed.
	*/
	template<typename ImpactStruct> 
	void ProcessTaskResult(TTBBaseProjectileTaskResult<ImpactStruct> TaskResult); 


	mutable FRWLock ActiveBulletsLock;
	mutable FRWLock InactiveBulletsLock;
	mutable FRWLock ActiveProjectilesLock;
	mutable FRWLock InactiveProjectilesLock;

	mutable FRWLock BulletsToAddLock;
	mutable FRWLock ProjectilesToAddLock;

	mutable FCriticalSection ActiveBulletsAccessLock;
	mutable FCriticalSection InactiveBulletsAccessLock;
	mutable FCriticalSection ActiveProjectilesAccessLock;
	mutable FCriticalSection InactiveProjectilesAccessLock;

	mutable FCriticalSection AddBulletMutex;
	mutable FCriticalSection AddProjectileMutex;
#pragma endregion
};


class TERMINALBALLISTICS_API FTBGetSubsystemHelper : public FNoncopyable
{
public:
	static UTerminalBallisticsSubsystem* GetTBSubsystem(const UObject* WorldContextObject);
private:
	friend UTerminalBallisticsSubsystem;
	static UTerminalBallisticsSubsystem* TBSubsystem;
};
