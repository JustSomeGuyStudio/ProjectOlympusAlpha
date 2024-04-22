// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "BallisticFunctions.h"
#include "CollisionPresets.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "GraphTasks/GraphTasks.h"
#include "Misc/Optional.h"
#include "NiagaraCommon.h"
#include "NiagaraComponent.h"
#include "TBProjectileTaskResult.h"
#include "TerminalBallisticsStatics.h"
#include "TerminalBallisticsTags.h"
#include "TerminalBallisticsTraits.h"
#include "TerminalBallisticsTypes.h"
#include "Types/TBProjectile.h"
#include "Types/TBProjectileId.h"
#include "Types/TBProjectileInjury.h"
#include <chrono>
#include <functional>


namespace TB::SimTasks
{
	FLinearColor GetTraceColor(const double CurrentSpeed, const double InitialSpeed);
}

class FTBBaseProjectileThread
{
public:
	FTBBaseProjectileThread() = default;

	FTBBaseProjectileThread(class UTerminalBallisticsSubsystem* Controller)
		: Controller(Controller)
	{}

	virtual void OnBulletTaskExit(FBulletTaskResult&& Result) = 0;
	virtual void OnProjectileTaskExit(FProjectileTaskResult&& Result) = 0;

	virtual bool HasActive() const = 0;

	virtual int GetNumActive() const = 0;

	virtual const FTimespan GetTickRate() const = 0;

protected:
	class UTerminalBallisticsSubsystem* Controller;
};


namespace TB::SimTasks
{
	enum class ETBThreadTaskAffinity : uint8
	{
		NONE,
		BULLET_TASK,
		PROJECTILE_TASK,
		OTHER,
	};

	inline static FString GetThreadTaskAffinityAsString(const ETBThreadTaskAffinity TaskAffinity)
	{
		static const FString AffinityNames[4] =
		{
			"None",
			"Bullet Tasks",
			"Projectile Tasks",
			"Other"
		};
		return AffinityNames[static_cast<uint8>(TaskAffinity)];
	}

	enum class ETBThreadTaskOverflowType
	{
		Restrict,
		Allow,
		CreateNew
	};

	struct FTBProjectileWorkerParams
	{
		FTBProjectileWorkerParams() = default;

		FTBProjectileWorkerParams(const ETBThreadTaskAffinity Affinity, const bool IsStrongAffinity, const ETBThreadTaskOverflowType OverflowBehavior, const int32 MaxTasks)
			: Affinity(Affinity)
			, IsStrongAffinity(IsStrongAffinity)
			, OverflowBehavior(OverflowBehavior)
			, MaxTasks(MaxTasks)
		{}

		ETBThreadTaskAffinity Affinity = ETBThreadTaskAffinity::NONE;
		bool IsStrongAffinity = false;
		ETBThreadTaskOverflowType OverflowBehavior = ETBThreadTaskOverflowType::CreateNew;
		int32 MaxTasks = 100;

		static FTBProjectileWorkerParams GeneralPurpose;
		static FTBProjectileWorkerParams Bullets;
		static FTBProjectileWorkerParams Projectiles;
	};

	struct FTBProjectileThreadPoolParams
	{
		TArray<FTBProjectileWorkerParams> ThreadParams;
		FTimespan ThreadTickRate;
		int32 NumThreads = 3;
		uint32 ThreadStackSize = 128 * 1024;
		int32 MaxTasksPerThread = 100;
		bool CapNumThreads = true;
		int32 MaxThreads = 5;
		ETBThreadTaskOverflowType DefaultOverflowBehavior = ETBThreadTaskOverflowType::CreateNew;

		FTBProjectileThreadPoolParams(const FTimespan ThreadTickRate, const int32 NumThreads, const uint32 ThreadStackSize = 128 * 1024, const int32 MaxTasksPerThread = 100, const bool CapNumThreads = true, const int32 MaxThreads = 5, const ETBThreadTaskOverflowType DefaultOverflowBehavior = ETBThreadTaskOverflowType::CreateNew)
			: ThreadTickRate(ThreadTickRate)
			, NumThreads(NumThreads)
			, ThreadStackSize(ThreadStackSize)
			, MaxTasksPerThread(MaxTasksPerThread)
			, CapNumThreads(CapNumThreads)
			, MaxThreads(MaxThreads)
			, DefaultOverflowBehavior(DefaultOverflowBehavior)
		{}

		FTBProjectileThreadPoolParams(const FTimespan ThreadTickRate, TArray<FTBProjectileWorkerParams> ThreadParams, const uint32 ThreadStackSize = 128 * 1024, const int32 MaxTasksPerThread = 100, const bool CapNumThreads = true, const int32 MaxThreads = 5, const ETBThreadTaskOverflowType DefaultOverflowBehavior = ETBThreadTaskOverflowType::CreateNew)
			: ThreadParams(ThreadParams)
			, ThreadTickRate(ThreadTickRate)
			, NumThreads(ThreadParams.Num())
			, ThreadStackSize(ThreadStackSize)
			, MaxTasksPerThread(MaxTasksPerThread)
			, CapNumThreads(CapNumThreads)
			, MaxThreads(MaxThreads)
			, DefaultOverflowBehavior(DefaultOverflowBehavior)
		{}
	};

	class ITBProjectileTask
	{
	public:
		ITBProjectileTask() = default;

		virtual ~ITBProjectileTask() = default;

		virtual void Tick(const double dt) = 0;

		virtual void SetUpdateTime(const std::chrono::high_resolution_clock::time_point Now) = 0;

		virtual void Kill(const ExitCodes::SimTaskExitCode ExitCode = ExitCodes::OTHER) = 0;

		virtual FTBProjectileId GetId() const = 0;

		virtual ETBThreadTaskAffinity GetAffinity() const = 0;

		virtual void SetOnCompleteCallback(TUniqueFunction<void(ITBProjectileTask*)> CallbackFunction) = 0;

		virtual void SetIsShuttingDown() = 0;

		virtual void SetTimescale(const double NewTimescale) = 0;

		virtual double CalculateDeltaTimeSeconds(const std::chrono::high_resolution_clock::time_point Now) const = 0;

		FORCEINLINE bool operator==(const ITBProjectileTask* Other)
		{
			return GetId() == Other->GetId()
				&& GetAffinity() == Other->GetAffinity();
		}
	};

	class FFlyByTask : public FPendingTask, public FTaskWithCheckValue
	{
	public:
		using GraphTask = TGraphTask<FFlyByTask>;

		FFlyByTask(FTBFlyBy FlyByEvent, FPendingTaskSynch* SynchObject = nullptr, bool* CheckValue = nullptr)
			: FPendingTask(SynchObject)
			, FTaskWithCheckValue(CheckValue)
			, FlyBy(FlyByEvent)
		{}

		virtual ~FFlyByTask() = default;

		FORCEINLINE TStatId GetStatId() const
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(FFlyByTask, STATGROUP_TaskGraphTasks);
		}

		FORCEINLINE static ESubsequentsMode::Type GetSubsequentsMode()
		{
			return ESubsequentsMode::FireAndForget;
		}

		static ENamedThreads::Type GetDesiredThread()
		{
			return ENamedThreads::GameThread;
		}

		virtual void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
		{
			if (!Check())
			{
				return;
			}
			if (FlyBy.IsValid())
			{
				FlyBy.GetValue().NotifyActorOfFlyBy();
			}
			else
			{
				TRACE_BOOKMARK(TEXT("InvalidTaskParam"));
				UE_LOG(LogTerminalBallistics, Error, TEXT("Invalid TTaskParam"));
			}
		}

		TTaskParam<FTBFlyBy> FlyBy;
	};

	template<typename delegate_type>
	struct TOptionalDelegate
	{
		using DelegateType = delegate_type;

		TOptional<DelegateType> Delegate;

		TOptionalDelegate() = default;
		TOptionalDelegate(DelegateType&& Delegate)
			: Delegate(Delegate)
		{}
		TOptionalDelegate(const DelegateType& Delegate)
			: Delegate(Delegate)
		{}

		bool IsSet() const
		{
			return Delegate.IsSet();
		}

		DelegateType& Get()
		{
			return Delegate.GetValue();
		}

		operator DelegateType() const
		{
			return Delegate.Get(DelegateType());
		}

		void Clear()
		{
			CallIfPossible(&DelegateType::Clear);
		}

		bool IsBound()
		{
			if constexpr(TB::Traits::THasFunction(&DelegateType::IsBound))
			{
				return CallIfPossible(&DelegateType::IsBound);
			}
			else
			{
				return false;
			}
		}

		template<typename... Args>
		void Broadcast(Args&&... args)
		{
			if constexpr(TB::Traits::THasFunction(&DelegateType::Broadcast))
			{
				CallIfPossible(&DelegateType::Broadcast, std::forward<Args>(args)...);
			}
		}

		template<typename... Args>
		void Execute(Args&&... args)
		{
			if constexpr(TB::Traits::THasFunction(&DelegateType::Execute))
			{
				CallIfPossible(&DelegateType::Execute, std::forward<Args>(args)...);
			}
		}

		template<typename... Args>
		void ExecuteIfBound(Args&&... args)
		{
			if constexpr(TB::Traits::THasFunction(&DelegateType::ExecuteIfBound))
			{
				CallIfPossible(&DelegateType::ExecuteIfBound, std::forward<Args>(args)...);
			}
		}

	protected:
		template<typename Function, typename... Args>
		auto CallIfPossible(Function func, Args ...args)
		{
			using ReturnType = std::invoke_result_t<decltype(std::bind(std::declval<Function>(), std::declval<DelegateType>(), std::declval<Args>()...))>;
			if (Delegate.IsSet())
			{
				return (Delegate.GetValue().*func)(args...);
			}
			else
			{
				return ReturnType();
			}
		}
	};

	/* Template struct used to define and store delegates for use in TProjectileSimulationTask */
	template<typename CompleteDelegateType, typename HitDelegateType, typename ExitHitDelegateType, typename InjureDelegateType, typename UpdateDelegateType, typename FlightDataType = FTBProjectileFlightData>
	struct TSimTaskDelegates
	{
		using CompleteDelegate =	CompleteDelegateType;
		using HitDelegate =			HitDelegateType;
		using ExitHitDelegate =		ExitHitDelegateType;
		using InjureDelegate =		InjureDelegateType;
		using UpdateDelegate =		UpdateDelegateType;
		using UpdateParamType =		FlightDataType;

		TOptionalDelegate<CompleteDelegate>				OnComplete;
		TOptionalDelegate<HitDelegate>					OnHit;
		TOptionalDelegate<ExitHitDelegate>				OnExitHit;
		TOptionalDelegate<InjureDelegate>				OnInjure;
		TOptionalDelegate<UpdateDelegate>				OnUpdate;

		TSimTaskDelegates() = default;

		TSimTaskDelegates(CompleteDelegate OnComplete, HitDelegate OnHit, ExitHitDelegate OnExitHit, InjureDelegate OnInjure, UpdateDelegate OnUpdate)
			: OnComplete(OnComplete)
			, OnHit(OnHit)
			, OnExitHit(OnExitHit)
			, OnInjure(OnInjure)
			, OnUpdate(OnUpdate)
		{}

		FORCEINLINE void Clear()
		{
			OnComplete.Clear();
			OnComplete.Clear();
			OnHit.Clear();
			OnExitHit.Clear();
			OnInjure.Clear();
			OnUpdate.Clear();
		}

		~TSimTaskDelegates()
		{
			Clear();
		}
	};
};
namespace TB::Traits
{
	template<typename ...Ts>
	struct TIsSimTaskDelegateStruct<TB::SimTasks::TSimTaskDelegates<Ts...>>
	{
		enum
		{
			Value = true
		};
	};
};

namespace TB::SimTasks
{
	class FKillTracerTask : public FTaskWithCheckValue, public FPendingTask
	{
	public:
		using GraphTask = TGraphTask<FKillTracerTask>;

		FKillTracerTask(UNiagaraComponent* Tracer, FPendingTaskSynch* SynchObject = nullptr, bool* CheckValue = nullptr)
			: FTaskWithCheckValue(CheckValue)
			, FPendingTask(SynchObject)
			, Tracer(Tracer)
		{}

		virtual ~FKillTracerTask() = default;

		FORCEINLINE TStatId GetStatId() const
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(FKillTracerTask, STATGROUP_TaskGraphTasks);
		}

		FORCEINLINE static ESubsequentsMode::Type GetSubsequentsMode()
		{
			return ESubsequentsMode::TrackSubsequents;
		}

		static ENamedThreads::Type GetDesiredThread()
		{
			return ENamedThreads::GameThread;
		}

		virtual void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
		{
			if (Check() && Tracer)
			{
				static const FName KillName = FName("Kill");
				Tracer->SetVariableBool(KillName, true);
				if(Tracer->PoolingMethod == ENCPoolMethod::ManualRelease)
				{
					Tracer->ReleaseToPool();
				}
			}
		}

	private:
		UNiagaraComponent* Tracer;
	};

	template<typename HitFunctionType, typename InjureFunctionType>
	struct TSimTaskBoundFunctions
	{
		HitFunctionType HitFunction;
		HitFunctionType ExitHitFunction;
		InjureFunctionType InjureFunction;
	};

	enum class ConsumeHitReturnCode : uint8
	{
		Kill		= 1 << 0,
		Invalid		= 1 << 1,
		Ricochet	= 1 << 2,
		Penetration = 1 << 3,
		Ignore		= 1 << 4
	};

	LLM_DECLARE_TAG(TProjectileSimulationTask);
	LLM_DECLARE_TAG(TProjectileSimulationTask_Update);
	LLM_DECLARE_TAG(TProjectileSimulationTask_CheckForHits);
	LLM_DECLARE_TAG(TProjectileSimulationTask_ConsumeHits);

	using double_seconds = std::chrono::duration<double>;

	template<typename TBSubsystem, typename TypeOfSimData, typename ProjectilePropertyStruct, typename DelegateStructType>
	class TProjectileSimulationTask : public ITBProjectileTask
	{
	public:
		using SimDataType = TypeOfSimData;
		using DelegateStruct = DelegateStructType;
		using BoundFunctionStruct = TSimTaskBoundFunctions<decltype(TypeOfSimData::HitFunction), decltype(TypeOfSimData::InjureFunction)>;
		using ImpactStruct = typename ProjectilePropertyStruct::ImpactStruct;
		using FlightDataType = typename DelegateStructType::UpdateParamType;
		using ResultType = TTBBaseProjectileTaskResult<ImpactStruct>;

		static_assert(TIsDerivedFrom<ProjectilePropertyStruct, FTBProjectilePhysicalProperties>::Value, "Invalid Projectile Property Struct");
		static_assert(TB::Traits::TIsValidSimTask<ImpactStruct, TypeOfSimData, ProjectilePropertyStruct>::Value, "Invalid TProjectileSimulationTask");
		static_assert(TB::Traits::TIsSimTaskDelegateStruct<DelegateStructType>::Value, "Invalid Delegate Struct");

		TProjectileSimulationTask() = delete;

		TProjectileSimulationTask(FTBBaseProjectileThread* Controller, TypeOfSimData& InSimData)
			: Controller(Controller)
			, bIsReady(false)
			, bIsDone(false)
			, HasExited(false)
			, bFirstUpdate(true)
			, bRetryHitConsumption(false)
			, bNotifyImpacts(true)
			, bOnlyNotifySelfOfFlyBy(false)
			, bShouldBroadcastFlyByEvents(true)
			, HasUpdateDelegate(false)
			, CallOnUpdateFuncFromGameThread(true)
		{
			LLM_SCOPE_BYTAG(TProjectileSimulationTask);
			Results = TTBBaseProjectileTaskResult<ImpactStruct>(InSimData.StartVelocity, InSimData.StartLocation, InSimData.bDrawDebugTrace, InSimData.bPrintDebugInfo, InSimData.GetId());
		}

		virtual ~TProjectileSimulationTask()
		{
			LLM_SCOPE_BYTAG(TProjectileSimulationTask);
			bIsShuttingDown = true;

			if (Tracer)
			{
				KillTracer();
			}
			Results.Empty();
			Owner.Reset();

			Delegates.Clear();
		}

		virtual void SetIsShuttingDown() override final
		{
			bIsShuttingDown = true;
		}

		bool IsShuttingDown() const
		{
			return bIsShuttingDown;
		}

		inline virtual void Tick(const double dt)
		{
			if (bIsDone)
			{
				if(!HasExited)
				{
					Kill();
				}
				else
				{
					TryComplete();
				}
				return;
			}
			if (bIsReady && SimType == ETBProjectileSimulationType::Realtime)
			{
				if (!bIsDone)
				{
					PreviousUpdateTime = std::chrono::high_resolution_clock::now();
					bIsReady = false;
					Update(dt);
				}
			}
		}

		virtual FVector CalculateProjectileVelocityInCavityFormingPhase(const FVector& Velocity, FVector& NewLocation) { return FVector::ZeroVector; }
		
		void TryComplete();

		virtual void PopulateSimData(TypeOfSimData& SimData);

		virtual bool CheckForHits(const double PreviousTime, const double dt, const bool bBroadcastUpdate = true);

		void SetupSimulationVariables(const FPredictProjectilePathParams& PredictParams);

		virtual void PredictPathWithDrag();

		virtual void SimulateInstant();

		virtual ConsumeHitReturnCode ConsumeHit(const FExitResult& ExitHit) = 0;

		virtual void ConsumeHits();

		virtual void Update(const double dt);

		inline virtual void Launch()
		{
			if (!ProjectileProperties)
			{
				Kill(TB::SimTasks::ExitCodes::INVALID_PROJECTILE);
			}
			StartTime = World->TimeSeconds;
			bIsDone = false;
			HasExited = false;
			PredictPathWithDrag();
		}

		inline virtual const FlightDataType& GetCurrentFlightData() const
		{
			return CurrentFlightData;
		}

		inline virtual double GetProjectileRadius() const { return ProjectileProperties.Radius; }

		inline virtual void SetLocation(const FVector& NewLocation)
		{
			CurrentFlightData.SetLocation(NewLocation);
		}
		inline virtual const FVector& GetLocation() const
		{
			return CurrentFlightData.Location;
		}

		inline virtual void SetVelocity(const FVector& NewVelocity)
		{
			CurrentFlightData.SetVelocity(NewVelocity);
		}
		inline virtual const FVector& GetVelocity() const
		{
			return CurrentFlightData.Velocity;
		}

		inline void SetAcceleration(const FVector& NewAcceleration)
		{
			CurrentFlightData.Acceleration = NewAcceleration;
		}
		inline const FVector& GetAcceleration() const
		{
			return CurrentFlightData.Acceleration;
		}

		FORCEINLINE bool operator==(const TProjectileSimulationTask* Other)
		{
			return Id == Other->Id;
		}

		inline void AddPoint(const FVector& Location, const FVector& Velocity, float Time)
		{
			Results.PathData.Emplace(Location, Velocity, Time);
		}

		inline bool MaxSimTimeIsValid() const
		{
			return MaxSimTime > 0
				&& MaxSimTime <= TB::Configuration::MaximumProjectileSimulationDuration;
		}

		inline void MakeMaxSimTimeValid()
		{
			if (!MaxSimTimeIsValid())
			{
				MaxSimTime = TB::Configuration::MaximumProjectileSimulationDuration;
			}
		}

		inline bool SimTimeIsValid() const
		{
			return CurrentTime < MaxSimTime;
		}

		// Inherited via ITBProjectileTask
		inline virtual FTBProjectileId GetId() const override
		{
			return Id;
		}
		inline virtual ETBThreadTaskAffinity GetAffinity() const override
		{
			return ETBThreadTaskAffinity::NONE;
		}
		inline virtual void SetOnCompleteCallback(TUniqueFunction<void(ITBProjectileTask*)> CallbackFunction) override
		{
			if (CallbackFunction)
			{
				OnComplete = MoveTemp(CallbackFunction);
			}
		}
		inline virtual void SetTimescale(const double NewTimescale) override
		{
			if (NewTimescale > 0.0)
			{
				Timescale = NewTimescale;
			}
		}

		inline void UpdateDrag(const FVector& Location, const FVector& Velocity, const float FluidDensity = 0.0f)
		{
			const FVector VelocityMS = Velocity / 100;
			const bool bIsInFluid = FluidDensity > 0.0f;
			if (UseDragVector)
			{
				CurrentDragVector = TerminalBallisticsSubsystem->CalculateProjectileDrag(VelocityMS, GravityZ, Location, Velocity.ToOrientationRotator(), ProjectileProperties, DragCalculationType, bIsInFluid ? FluidDensity : -1, !bIsInFluid);
			}
			else
			{
				CurrentDrag = TerminalBallisticsSubsystem->UpdateProjectileDrag(VelocityMS.Size(), GravityZ, Location, ProjectileProperties, DragCalculationType, bIsInFluid ? FluidDensity : -1, !bIsInFluid);
			}
		}

		virtual typename ProjectilePropertyStruct::ProjectileStruct GetProjectile() const = 0;
		virtual const FPhysMatProperties& GetProjectilePhysMatProperties() const = 0;

		/**
		* Adds a function that will be called when the projectile updates.
		* This can be useful for keeping track of the projectile externally.
		*
		* @param Func						The function to call.
		* @param bShouldCallOnGameThread	Whether or not the function should be called from the game thread. Note: this can be more expensive but may be required for some functions to work properly.
		*/
		inline virtual void SetFunctionToCallOnUpdate(TUniqueFunction<void(const FlightDataType&)> Func, const bool bShouldCallOnGameThread = true)
		{
			if (Func)
			{
				CallOnUpdate = MoveTemp(Func);
				CallOnUpdateFuncFromGameThread = bShouldCallOnGameThread;
			}
		}

		inline virtual void SetUpdateTime(const std::chrono::high_resolution_clock::time_point Now) override
		{
			PreviousUpdateTime = Now;
		}

		inline virtual double CalculateDeltaTimeSeconds(const std::chrono::high_resolution_clock::time_point Now) const override
		{
			if (bFirstUpdate)
			{
				bFirstUpdate = false;
				return -1;
			}
			using namespace std::chrono;
			const auto DeltaTime = duration_cast<double_seconds>(Now - PreviousUpdateTime);
			return DeltaTime.count();
		}

	protected:
		int UpdateBroadcastInterval = 0; // Number of updates between each call of the "CallOnUpdate" function and update delegate broadcast. A value of 0 means it will be called every update.
		int UpdatesSinceLastCall = 0;
		BoundFunctionStruct BoundFunctions;
		DelegateStruct Delegates;
		TUniqueFunction<void(const FlightDataType&)> CallOnUpdate;
		using FOnUpdateTask = TDelegateBroadcastTask_OneParam<typename DelegateStructType::UpdateDelegate, const FlightDataType>;

		TUniqueFunction<void(ITBProjectileTask*)> OnComplete;
		TMulticastDelegate<void(const ResultType&)> CompletionDelegate;

		using FCompletionTask = TDelegateBroadcastTask_OneParam<TMulticastDelegate<void(const ResultType&)>, ResultType>;
		using FHitTask = TDelegateBroadcastTask_OneParam<typename DelegateStructType::HitDelegate, ImpactStruct>;
		using FExitHitTask = TDelegateBroadcastTask_OneParam<typename DelegateStructType::ExitHitDelegate, ImpactStruct>;
		using FInjureTask = TDelegateBroadcastTask_TwoParams<typename DelegateStructType::InjureDelegate, ImpactStruct, FTBProjectileInjuryParams>;
		using FCompletionTaskBasic = TDelegateBroadcastTask_TwoParams<typename DelegateStructType::CompleteDelegate, FTBProjectileId, TArray<FPredictProjectilePathPointData>>;

		FPendingTaskSynch PendingTaskSynch;

		virtual void CallUpdateFunc();
		virtual void BroadcastUpdateDelegate();

		void BroadcastHitDelegate(const ImpactStruct& ImpactParams);
		void BroadcastExitHitDelegate(const ImpactStruct& ImpactParams);
		void BroadcastInjureDelegate(const ImpactStruct& ImpactParams, const FTBProjectileInjuryParams& InjuryParams);

		virtual void Kill(const ExitCodes::SimTaskExitCode ExitCode = ExitCodes::OTHER) override;

		virtual void LogExitCode(const ExitCodes::SimTaskExitCode ExitCode);

		virtual void BroadcastCompletionDelegate();
		virtual void BroadcastCompletionDelegateBasic();

		virtual void BroadcastFlyBy(const FVector& Position, const double Distance, AActor* Actor) = 0;

		void UpdateTracer();
		void KillTracer();

		FGraphEventRef TracerKillTriggerEventRef;
		FGraphEventRef TracerKillEventRef;

		ResultType Results;

		FTBBaseProjectileThread* Controller;

		UNiagaraComponent* Tracer;
		int TracerUpdateInterval = 0;
		int UpdatesSinceLastTracerUpdate = 0;

		/**
		* Estimates a TracerUpdateInterval that balances accuracy with update frequency.
		* For example, a higher projectile speed might benefit from less tracer updates per frame, since each update will only cover a small distance.
		*/
		void EstimateTracerUpdateInterval();

		std::chrono::high_resolution_clock::time_point PreviousUpdateTime;

		double GravityZ; // cm/s^2
		double MaxSimTime;

		double StartTime;
		double CurrentTime;
		FVector StartLocation; // cm
		FVector StartVelocity; // cm/s

		FVector VelocityAtFluidEntry; // m/s

		FVector TraceStart = FVector::ZeroVector;

		TObjectPtr<USceneComponent> LastHitComponent = nullptr;

		int Iterations = 0;
		int MaxIterations;

		FCollisionQueryParams QueryParams;
		FCollisionObjectQueryParams ObjQueryParams;

		TBSubsystem* TerminalBallisticsSubsystem;
		UWorld* World;

		TWeakObjectPtr<AActor> Owner;
		TWeakObjectPtr<AController> Instigator;

		FHitResult LatestHitResult;
		FHitResult PreviousHitResult;
		FVector PreviousHitLocation;

		FTBProjectileId Id;

		ECollisionChannel TraceChannel;
		FCollisionResponseParams ResponseParams;

		float CurrentFluidDensity = 0.f;
		float CurrentFluidViscosity = 0.f;

		std::atomic_bool bIsReady = false;
		std::atomic_bool bIsDone = false;
		std::atomic_bool HasExited = false;

		
		bool bIsShuttingDown = false;

		double SimFrequency; // Used when SimType is ETBProjectileSimulationType::Instant

		mutable bool bFirstUpdate : 1; // Only used during the first update, when "PreviousUpdateTime" isn't set

		bool bRetryHitConsumption : 1; // Flag used when we cannot safely consume hits due to GC

		bool UseDragVector : 1;

		bool bNotifyImpacts : 1; // If false, impacts/injuries will not be reported and no dependent events will be triggered. Acts like a "test" simulation. Implies "Instant" SimType.

		bool bOnlyNotifySelfOfFlyBy : 1;

		bool bShouldBroadcastFlyByEvents : 1;

		bool HasUpdateDelegate : 1;
		bool CallOnUpdateFuncFromGameThread : 1;

		ETBDragComplexity DragCalculationType = ETBDragComplexity::DEFAULT;
		ETBProjectileSimulationType SimType = ETBProjectileSimulationType::Default;

		double OwnerIgnoreDistance = 10.0;

		double CurrentDistanceInFluid = 0.0;

		double Timescale = 1.0;

		ProjectilePropertyStruct ProjectileProperties;

		double CurrentDrag;
		FVector CurrentDragVector = FVector::ZeroVector;

		FlightDataType CurrentFlightData;
		FlightDataType PreviousFlightData;
		FlightDataType LastBroadcastedData;
		double CurrentDeltaTime;

		struct FAttemptHit
		{
			bool bFoundHit = false;
			bool bWasValidHit = false;
			bool bDoFlyBy = false;
			bool bStoppedByGC = false;
			double TimeToAdd = 0.0;

			double NewDeltaTime = 0.0;

			double FlyByDistance = 0.0;
			AActor* FlyByActor = nullptr;
			FVector FlyByPoint = FVector::ZeroVector;

			FVector StartOfTrace = FVector::ZeroVector;

			FVector LocationToAdd = FVector::ZeroVector;
			FVector VelocityToAdd = FVector::ZeroVector;

			FlightDataType NewPreviousFlightData;
			FlightDataType NewFlightData;

			FHitResult NewHitResult;
		};
		FAttemptHit AttemptHitCheck(const double PreviousTime, const double dt, const bool bBroadcastUpdate);
	};




#pragma region Inline Functions
	template<typename TBSubsystem, typename TypeOfSimData, typename ProjectilePropertyStruct, typename DelegateStructType>
	inline void TProjectileSimulationTask<TBSubsystem, TypeOfSimData, ProjectilePropertyStruct, DelegateStructType>::TryComplete()
	{
		if (bIsDone && HasExited) // We're done simulating.
		{
			if (PendingTaskSynch.Acquire()) // Make sure we don't have any pending graph tasks...
			{
				if (OnComplete)
				{
					OnComplete(this);
				}
			}
		}
	}

	template<typename TBSubsystem, typename ImpactStruct, typename ProjectilePropertyStruct, typename DelegateStructType>
	void TProjectileSimulationTask<TBSubsystem, ImpactStruct, ProjectilePropertyStruct, DelegateStructType>::Kill(const ExitCodes::SimTaskExitCode ExitCode)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TProjectileSimulationTask::Kill);
		LLM_SCOPE_BYTAG(TProjectileSimulationTask);
		HasExited = true;
		bIsReady = false;
		bIsDone = true;
		Results.ExitCode = ExitCode;
		if(Tracer)
		{
			KillTracer();
		}
		LogExitCode(ExitCode);
		CallUpdateFunc();
		BroadcastCompletionDelegate();
		BroadcastCompletionDelegateBasic();
		TryComplete();
	}

	template<typename TBSubsystem, typename ImpactStruct, typename ProjectilePropertyStruct, typename DelegateStructType>
	void TProjectileSimulationTask<TBSubsystem, ImpactStruct, ProjectilePropertyStruct, DelegateStructType>::CallUpdateFunc()
	{
		if (IsEngineExitRequested() || bIsShuttingDown)
		{
			return;
		}
		if (CallOnUpdate)
		{
			if (CallOnUpdateFuncFromGameThread)
			{
				GameThreadTask([this, FlightData = GetCurrentFlightData()]() {
					CallOnUpdate(FlightData);
				});
			}
			else
			{
				CallOnUpdate(GetCurrentFlightData());
			}
		}
		if (HasUpdateDelegate)
		{
			BroadcastUpdateDelegate();
		}
#if defined(ENABLE_DRAW_DEBUG) && ENABLE_DRAW_DEBUG
		const bool bDebugDraw = CVarBallisticsDebugDraw.GetValueOnAnyThread() || Results.bDrawDebugTrace;
		if (bDebugDraw)
		{
			GameThreadTask([WorldLocal = World, PrevData = PreviousFlightData, CurrData = GetCurrentFlightData(), InitialVel = Results.InitialVelocity.Size(), Vel = PreviousFlightData.Velocity.Size()]()
				{
					if (WorldLocal)
					{
						FColor LineColor = GetTraceColor(Vel, InitialVel).ToFColor(true);
						DrawDebugLine(WorldLocal, PrevData.Location, CurrData.Location, LineColor, false, 10.f);
					}
				});
		}
		PreviousFlightData = GetCurrentFlightData();
#endif
	}

#pragma region Delegates
	template<typename TBSubsystem, typename ImpactStruct, typename DelegateStructType, typename OnUpdateParamType>
	void TProjectileSimulationTask<TBSubsystem, ImpactStruct, DelegateStructType, OnUpdateParamType>::BroadcastCompletionDelegate()
	{
		if (bIsShuttingDown)
		{
			return;
		}
		if(CompletionDelegate.IsBound())
		{
			FCompletionTask::GraphTask::CreateTask().ConstructAndDispatchWhenReady(CompletionDelegate, Results, &PendingTaskSynch, &bIsShuttingDown);
		}
	}

	template<typename TBSubsystem, typename ImpactStruct, typename DelegateStructType, typename OnUpdateParamType>
	void TProjectileSimulationTask<TBSubsystem, ImpactStruct, DelegateStructType, OnUpdateParamType>::BroadcastCompletionDelegateBasic()
	{
		if (bIsShuttingDown)
		{
			return;
		}
		if(Delegates.OnComplete.IsBound())
		{
			FCompletionTaskBasic::GraphTask::CreateTask().ConstructAndDispatchWhenReady(Delegates.OnComplete, Id, Results.PathData, &PendingTaskSynch, &bIsShuttingDown);
		}
	}

	template<typename TBSubsystem, typename ImpactStruct, typename DelegateStructType, typename OnUpdateParamType>
	void TProjectileSimulationTask<TBSubsystem, ImpactStruct, DelegateStructType, OnUpdateParamType>::BroadcastHitDelegate(const ImpactStruct& ImpactParams)
	{
		if (bIsShuttingDown)
		{
			return;
		}
		if (bNotifyImpacts && ImpactParams.IsValid())
		{
			if(Delegates.OnHit.IsBound())
			{
				FHitTask::GraphTask::CreateTask().ConstructAndDispatchWhenReady(Delegates.OnHit, ImpactParams, &PendingTaskSynch, &bIsShuttingDown);
			}
			if (BoundFunctions.HitFunction)
			{
				auto Func = std::bind(BoundFunctions.HitFunction, ImpactParams);
				FLambdaTask::GraphTask::CreateTask().ConstructAndDispatchWhenReady(MoveTemp(Func), &PendingTaskSynch, &bIsShuttingDown);
			}
		}
	}

	template<typename TBSubsystem, typename ImpactStruct, typename DelegateStructType, typename OnUpdateParamType>
	void TProjectileSimulationTask<TBSubsystem, ImpactStruct, DelegateStructType, OnUpdateParamType>::BroadcastExitHitDelegate(const ImpactStruct& ImpactParams)
	{
		if (bIsShuttingDown)
		{
			return;
		}
		if (bNotifyImpacts && ImpactParams.IsValid())
		{
			if(Delegates.OnExitHit.IsBound())
			{
				FExitHitTask::GraphTask::CreateTask().ConstructAndDispatchWhenReady(Delegates.OnExitHit, ImpactParams, &PendingTaskSynch, &bIsShuttingDown);
			}
			if (BoundFunctions.ExitHitFunction)
			{
				auto Func = std::bind(BoundFunctions.ExitHitFunction, ImpactParams);
				FLambdaTask::GraphTask::CreateTask().ConstructAndDispatchWhenReady(MoveTemp(Func), &PendingTaskSynch, &bIsShuttingDown);
			}
		}
	}

	template<typename TBSubsystem, typename ImpactStruct, typename DelegateStructType, typename OnUpdateParamType>
	void TProjectileSimulationTask<TBSubsystem, ImpactStruct, DelegateStructType, OnUpdateParamType>::BroadcastInjureDelegate(const ImpactStruct& ImpactParams, const FTBProjectileInjuryParams& InjuryParams)
	{
		if (bIsShuttingDown)
		{
			return;
		}
		if (bNotifyImpacts && ImpactParams.IsValid() && InjuryParams.IsValid())
		{
			if (Delegates.OnInjure.IsBound())
			{
				FInjureTask::GraphTask::CreateTask().ConstructAndDispatchWhenReady(Delegates.OnInjure, ImpactParams, InjuryParams, &PendingTaskSynch, &bIsShuttingDown);
			}
			if (BoundFunctions.InjureFunction)
			{
				auto Func = std::bind(BoundFunctions.InjureFunction, ImpactParams, InjuryParams);
				FLambdaTask::GraphTask::CreateTask().ConstructAndDispatchWhenReady(MoveTemp(Func), &PendingTaskSynch, &bIsShuttingDown);
			}
		}
	}

	template<typename TBSubsystem, typename TypeOfSimData, typename ProjectilePropertyStruct, typename DelegateStructType>
	void TProjectileSimulationTask<TBSubsystem, TypeOfSimData, ProjectilePropertyStruct, DelegateStructType>::BroadcastUpdateDelegate()
	{
		if (bIsShuttingDown)
		{
			return;
		}
		if(Delegates.OnUpdate.IsBound())
		{
			FOnUpdateTask::GraphTask::CreateTask().ConstructAndDispatchWhenReady(Delegates.OnUpdate, GetCurrentFlightData(), &PendingTaskSynch, &bIsShuttingDown);
		}
	}
#pragma endregion

	template<typename TBSubsystem, typename TypeOfSimData, typename ProjectilePropertyStruct, typename DelegateStructType>
	inline void TProjectileSimulationTask<TBSubsystem, TypeOfSimData, ProjectilePropertyStruct, DelegateStructType>::UpdateTracer()
	{
		if (Tracer && !bIsShuttingDown)
		{
			static const FName BeamStartVariable = "BeamStart";
			static const FName BeamDirectionVariable = "BeamDirection";
			Tracer->SetVariablePosition(BeamDirectionVariable, GetVelocity().GetSafeNormal());
			Tracer->SetVariablePosition(BeamStartVariable, GetLocation());
		}
	}
	
	template<typename TBSubsystem, typename TypeOfSimData, typename ProjectilePropertyStruct, typename DelegateStructType>
	inline void TProjectileSimulationTask<TBSubsystem, TypeOfSimData, ProjectilePropertyStruct, DelegateStructType>::KillTracer()
	{
		if (Tracer)
		{
			TracerKillTriggerEventRef->DispatchSubsequents();
		}
	}

	template<typename TBSubsystem, typename TypeOfSimData, typename ProjectilePropertyStruct, typename DelegateStructType>
	void TProjectileSimulationTask<TBSubsystem, TypeOfSimData, ProjectilePropertyStruct, DelegateStructType>::LogExitCode(TB::SimTasks::ExitCodes::SimTaskExitCode ExitCode)
	{
		using namespace TB::Configuration;
		EExitCodeLogLevel AsLogLevel = static_cast<EExitCodeLogLevel>(ExitCodeLogLevel);
		if (AsLogLevel != EExitCodeLogLevel::None)
		{
			const bool bLogLevelIsError = AsLogLevel == EExitCodeLogLevel::Error;
			if (bLogLevelIsError)
			{
				if (ExitCode > ExitCodes::FailureLevel)
				{
					ExitCodes::LogExitCode(ExitCode);
				}
			}
			else if (ExitCode < ExitCodes::FailureLevel || AsLogLevel == EExitCodeLogLevel::Verbose)
			{
				ExitCodes::LogExitCode(ExitCode);
			}
		}
	}

	template<typename TBSubsystem, typename TypeOfSimData, typename ProjectilePropertyStruct, typename DelegateStructType>
	void TProjectileSimulationTask<TBSubsystem, TypeOfSimData, ProjectilePropertyStruct, DelegateStructType>::PopulateSimData(TypeOfSimData& SimData)
	{
		LLM_SCOPE_BYTAG(TProjectileSimulationTask);
		Id = SimData.GetId();
		DragCalculationType = SimData.DragCalculationType;
		SimType = SimData.SimType;
		MaxSimTime = SimData.PredictParams.MaxSimTime;
		OwnerIgnoreDistance = SimData.OwnerIgnoreDistance;
		SimFrequency = SimData.PredictParams.SimFrequency;
		TraceChannel = SimData.PredictParams.TraceChannel;
		MaxIterations = SimData.MaxIterations;
		StartLocation = SimData.StartLocation;
		StartVelocity = SimData.StartVelocity;
		QueryParams = SimData.QueryParams;
		ObjQueryParams = SimData.ObjQueryParams;
		TerminalBallisticsSubsystem = SimData.TerminalBallisticsSubsystem;
		Tracer = SimData.TracerComponent;
		World = SimData.World;
		Owner = SimData.Owner;
		Instigator = SimData.Instigator;
		CompletionDelegate = SimData.CompletionDelegate;
		UpdateBroadcastInterval = SimData.UpdateBroadcastInterval;
		SetTimescale(SimData.Timescale);

		if (Delegates.OnUpdate.IsBound())
		{
			HasUpdateDelegate = true;
		}

		BoundFunctions.HitFunction = MoveTemp(SimData.HitFunction);
		BoundFunctions.ExitHitFunction = MoveTemp(SimData.ExitHitFunction);
		BoundFunctions.InjureFunction = MoveTemp(SimData.InjureFunction);

		TracerUpdateInterval = 2; // Small delay before tracer updates kick in

		if (SimData.IsTestTask)
		{
			SimType = ETBProjectileSimulationType::Instant;
			bNotifyImpacts = false;
		}

		if (Tracer)
		{
			TracerKillTriggerEventRef = FGraphEvent::CreateGraphEvent();
			FGraphEventArray Prerequisites;
			Prerequisites.Add(TracerKillTriggerEventRef);
			TracerKillEventRef = TGraphTask<FKillTracerTask>::CreateTask(&Prerequisites).ConstructAndDispatchWhenReady(Tracer, &PendingTaskSynch, &bIsShuttingDown);
		}
	}

	template<typename TBSubsystem, typename TypeOfSimData, typename ProjectilePropertyStruct, typename DelegateStructType>
	inline typename TProjectileSimulationTask<TBSubsystem, TypeOfSimData, ProjectilePropertyStruct, DelegateStructType>::FAttemptHit TProjectileSimulationTask<TBSubsystem, TypeOfSimData, ProjectilePropertyStruct, DelegateStructType>::AttemptHitCheck(const double PreviousTime, const double dt, const bool bBroadcastUpdate)
	{
		FAttemptHit AttemptedHit{};
		if (UE::IsSavingPackage(nullptr) || IsGarbageCollecting())
		{
			/* Trying to continue further would result in a crash, since GC will interfere with any attempts to determine what we hit. */
			AttemptedHit.bStoppedByGC = true;
			return AttemptedHit;
		}

		AttemptedHit.StartOfTrace = GetLocation();

		FCollisionQueryParams TempQueryParams = QueryParams;

		if (FVector::Dist(StartLocation, GetLocation()) < OwnerIgnoreDistance)
		{
			if (const AActor* OwnerPtr = Owner.Get())
			{
				TempQueryParams.AddIgnoredActor(OwnerPtr);
				if (const AActor* OwnerPawn = OwnerPtr->GetOwner<APawn>())
				{
					TempQueryParams.AddIgnoredActor(OwnerPawn);
				}
			}
		}

		const FVector OldVelocity = GetVelocity();

		bool bIsInFluid = CurrentFluidViscosity > 0.f;

		const FCollisionShape Sphere = FCollisionShape::MakeSphere(ProjectileProperties.Radius);
		/* If we are currently overlapping the previously hit component, ignore it. For skeletal meshes, we're only checking to see if we are still overlapping the same body instance. */
		if (PreviousHitResult.GetComponent() && TB::DoesShapeOverlap(PreviousHitResult, GetLocation(), Sphere))
		{
			if (!bIsInFluid)
			{
				TempQueryParams.AddIgnoredComponent(PreviousHitResult.GetComponent());
			}
		}
		else
		{
			bIsInFluid = false;
		}

		const FQuat projectileOrientation = OldVelocity.ToOrientationQuat();
		FVector dragForce = FVector::ZeroVector;
		if (UseDragVector)
		{
			dragForce = CurrentDragVector;
		}
		else
		{
			dragForce = projectileOrientation.RotateVector(FVector(CurrentDrag, 0.0, 0.0));
		}
		AttemptedHit.NewDeltaTime = dt;
		AttemptedHit.NewPreviousFlightData = CurrentFlightData;

		AttemptedHit.NewFlightData = CurrentFlightData;
		if (bIsInFluid)
		{
			UpdateDrag(GetLocation(), GetVelocity(), CurrentFluidDensity);
			if (UseDragVector)
			{
				dragForce = CurrentDragVector;
			}
			else
			{
				dragForce = projectileOrientation.RotateVector(FVector(CurrentDrag, 0, 0));
			}
		}

		if (!AttemptedHit.NewFlightData.Update(dt, dragForce, GravityZ / 100.0, ProjectileProperties.Mass)) // Something went wrong. This can be caused by large step sizes or sudden drag changes.
		{
			AttemptedHit.NewFlightData = CurrentFlightData;
			const double FirstSubstepTime = fmin(dt / 10.0, 1e-4);
			AttemptedHit.NewFlightData.Update(dt, dragForce, GravityZ / 100.0, ProjectileProperties.Mass, 1);


			bool SubsteppingSucceded = true;
			const double SubstepTime = (dt - FirstSubstepTime) / 3.0;
			for (int i = 0; i < 3; i++)
			{
				UpdateDrag(AttemptedHit.NewFlightData.Location, AttemptedHit.NewFlightData.Velocity, CurrentFluidDensity);
				if (UseDragVector)
				{
					dragForce = CurrentDragVector;
				}
				else
				{
					dragForce = projectileOrientation.RotateVector(FVector(CurrentDrag, 0, 0));
				}
				SubsteppingSucceded |= AttemptedHit.NewFlightData.Update(dt, dragForce, GravityZ / 100.0, ProjectileProperties.Mass, 1);
			}
			if (!SubsteppingSucceded)
			{
				AttemptedHit.VelocityToAdd = FVector::ZeroVector;
				AttemptedHit.LocationToAdd = GetLocation();
				AttemptedHit.TimeToAdd = PreviousTime + dt;
				return AttemptedHit;
			}
		}

		FVector LocationToAdd = AttemptedHit.NewFlightData.Location;
		FVector VelocityToAdd = FVector::ZeroVector;
		{
			FHitResult HitResult;
			AttemptedHit.bFoundHit = TB::SweepSingle(World, Sphere, FQuat::Identity, HitResult, AttemptedHit.StartOfTrace, AttemptedHit.NewFlightData.Location, TraceChannel, TempQueryParams);
			if (AttemptedHit.bFoundHit)
			{
				AttemptedHit.bWasValidHit = IsValid(HitResult.GetComponent()) && !HitResult.Normal.IsNearlyZero();
				AttemptedHit.NewHitResult = HitResult;
				if (AttemptedHit.bWasValidHit)
				{
					const double HitTimeDelta = dt * AttemptedHit.NewHitResult.Time;
					AttemptedHit.TimeToAdd = PreviousTime + HitTimeDelta;
					AttemptedHit.LocationToAdd = AttemptedHit.NewHitResult.Location;
					const FVector VelocityAtHit = OldVelocity + (AttemptedHit.NewFlightData.Acceleration * HitTimeDelta);
					AttemptedHit.VelocityToAdd = VelocityAtHit;
				}
				else
				{
					return AttemptedHit; // HitResult was invalid.
				}
			}


			FCollisionShape Shape = FCollisionShape::MakeCapsule(TB::Configuration::FlyByTraceRadius, (FVector::Dist(LocationToAdd, AttemptedHit.StartOfTrace) / 2.0));
			FQuat Rotation = projectileOrientation;
			TMap<AActor*, FTBOverlapArray> Overlaps;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(FlyByTrace), false);
			FCollisionObjectQueryParams ObjParams(ECC_TO_BITFIELD(ECC_Pawn));
			FTBCollisionParams CollisionParams(Params, ObjParams);
			if (UTerminalBallisticsStatics::OverlapMultiSortedByActorWithShape(World, Overlaps, (AttemptedHit.StartOfTrace + LocationToAdd) / 2 + (Rotation.GetForwardVector() * TB::Configuration::FlyByTraceRadius), Rotation, CollisionParams, Shape))
			{
				for (const auto& Overlap : Overlaps)
				{
					if (Overlap.Key)
					{
						FTBOverlapArray OverlapResults = Overlap.Value;
						/*FHitResult TempHit;
						const FVector Start = (AttemptedHit.StartOfTrace + LocationToAdd) / 2;
						OverlapResults.Overlaps = OverlapResults.Overlaps.FilterByPredicate([&](const FOverlapResult& Result) {
							if (!IsValid(Result.GetActor()) || !IsValid(Result.GetComponent()))
							{
								return false;
							}

							const bool hit = FPhysicsInterface::RaycastSingle(World, TempHit, Start, Result.GetComponent()->GetComponentLocation(), TraceChannel, TempQueryParams, FCollisionResponseParams::DefaultResponseParam);
							return TempHit.GetActor() == Result.GetActor();
						});*/

						FOverlapResult ClosestOverlap = UTerminalBallisticsStatics::GetClosestOverlap(OverlapResults, (AttemptedHit.StartOfTrace + LocationToAdd) / 2);
						if (ClosestOverlap.GetComponent())
						{
							const FPath Path = FPath::FromTwoPoints(AttemptedHit.StartOfTrace, LocationToAdd);

							const FVector Point = Path.GetClosestPointOnPath(ClosestOverlap.GetComponent()->GetComponentLocation(), AttemptedHit.FlyByDistance);
							AttemptedHit.FlyByPoint = Point;
							AttemptedHit.FlyByActor = ClosestOverlap.GetActor();
							AttemptedHit.bDoFlyBy = true;
						}
					}
				}
			}
		}

		return AttemptedHit;
	}

	template<typename TBSubsystem, typename TypeOfSimData, typename ProjectilePropertyStruct, typename DelegateStructType>
	bool TProjectileSimulationTask<TBSubsystem, TypeOfSimData, ProjectilePropertyStruct, DelegateStructType>::CheckForHits(const double PreviousTime, const double dt, const bool bBroadcastUpdate)
	{
		LLM_SCOPE_BYTAG(TProjectileSimulationTask_CheckForHits);
		TRACE_CPUPROFILER_EVENT_SCOPE(TProjectileSimulationTask::CheckForHits);

		/* Enum used here to help with readability of return value. */
		enum
		{
			FOUND_HIT = true,
			NO_HIT = false
		};

		bIsReady = false;

		const FVector OldDragVector = CurrentDragVector;
		const double OldDrag = CurrentDrag;

		int32 RetryAttempts = 0;
		FAttemptHit AttemptedHit = AttemptHitCheck(PreviousTime, dt, bBroadcastUpdate);

		/* If we got a hit but it wasn't valid for some reason, retry up to 3 times. */
		while (RetryAttempts < 3 && (AttemptedHit.bFoundHit && !AttemptedHit.bWasValidHit))
		{
			AttemptedHit = AttemptHitCheck(PreviousTime, dt, bBroadcastUpdate);
			if (!AttemptedHit.bFoundHit || (AttemptedHit.bFoundHit && AttemptedHit.bWasValidHit))
			{
				break;
			}
		}
		bool bSuccess = true;
		if (AttemptedHit.bStoppedByGC && AttemptedHit.bFoundHit)
		{
			// Bail out since GC tends to ruin hit information.
			bSuccess = false;

			// Restore previous drag values
			CurrentDragVector = OldDragVector;
			CurrentDrag = OldDrag;

			bIsReady = true;
			return NO_HIT;
		}
		if (RetryAttempts == 3)
		{
			TB_LOG(Error, TEXT("AttemptHitCheck failed."));
			bSuccess = false;
		}

		AddPoint(AttemptedHit.LocationToAdd, AttemptedHit.VelocityToAdd, AttemptedHit.TimeToAdd);
		TraceStart = AttemptedHit.StartOfTrace;
		CurrentDeltaTime = AttemptedHit.NewDeltaTime;
		PreviousFlightData = AttemptedHit.NewPreviousFlightData;
		CurrentFlightData = AttemptedHit.NewFlightData;
		LatestHitResult = AttemptedHit.NewHitResult;
		PreviousHitResult = LatestHitResult;

		if (AttemptedHit.bDoFlyBy)
		{
			BroadcastFlyBy(AttemptedHit.FlyByPoint, AttemptedHit.FlyByDistance, AttemptedHit.FlyByActor);
		}
		QueryParams.ClearIgnoredComponents();

		{
			FGCScopeGuard GCGuard;
			const FPhysMatProperties& SurfaceProperties = TB::GetSurfaceProperties(LatestHitResult);
			if (SurfaceProperties.bIsFluid)
			{
				if (CurrentFluidDensity > 0.f && SurfaceProperties.InBaseUnits().Density == CurrentFluidDensity) // We're already travelling in a fluid, and it has the same density as the new one.
				{
					CurrentDistanceInFluid += FVector::Dist(AttemptedHit.LocationToAdd, PreviousFlightData.Location);
				}
				else
				{
					VelocityAtFluidEntry = GetVelocity() / 100.0;

					CurrentDistanceInFluid = 0.0;
					CurrentFluidDensity = SurfaceProperties.InBaseUnits().Density;
					CurrentFluidViscosity = SurfaceProperties.InBaseUnits().Viscosity;

					FVector NewLocation = GetLocation();
					FVector NewVelocity = CalculateProjectileVelocityInCavityFormingPhase(VelocityAtFluidEntry, NewLocation);
					TB_LOG(Error, TEXT("%s"), *NewVelocity.ToString());
					GameThreadTask([NewLocation, world = World]()
					{
						if (world)
						{
							DrawDebugPoint(world, NewLocation, 5.0f, FColor::Magenta, false, 30.0f, 1U);
						}
					});

					/*const double ImpactAngle = TB::MathUtils::VectorUtils::GetImpactAngleAlt(LatestHitResult.Normal, GetVelocity());
					if (!FMath::IsNearlyZero(abs(ImpactAngle)))
					{
						using namespace TB::MathUtils;

						const double WettedArea = ProjectileProperties.GetFrontalCSA();
						const FVector Pressure = CurrentFluidDensity * sqr(GetVelocity() / 100.0f) * sqr(cos(ImpactAngle));
						const FVector Force = Pressure * WettedArea;
						CurrentFlightData.Velocity -= 100 * (Force / ProjectileProperties.Mass) * dt;
						TB_LOG(Error, TEXT("%d"), 100 * (Force / ProjectileProperties.Mass) * dt);
					}*/
				}
			}
		}

		EstimateTracerUpdateInterval();

		if (AttemptedHit.bFoundHit && AttemptedHit.bWasValidHit)
		{
			if (LatestHitResult.GetComponent())
			{
				return FOUND_HIT;
			}
			else
			{
				ensureAlways(false);
				return NO_HIT;
			}
		}
		else
		{
			bIsReady = true;
			return NO_HIT;
		}
	}

	template<typename TBSubsystem, typename TypeOfSimData, typename ProjectilePropertyStruct, typename DelegateStructType>
	void TProjectileSimulationTask<TBSubsystem, TypeOfSimData, ProjectilePropertyStruct, DelegateStructType>::SetupSimulationVariables(const FPredictProjectilePathParams& PredictParams)
	{
		LLM_SCOPE_BYTAG(TProjectileSimulationTask);
		if (!ProjectileProperties)
		{
			Kill(ExitCodes::INVALID_PROJECTILE);
			return;
		}

		if (!MaxSimTimeIsValid())
		{
			MakeMaxSimTimeValid();
		}

		// Keep MaxIterations in line with configuration.
		if (MaxIterations > TB::Configuration::MaxPenetrations)
		{
			MaxIterations = TB::Configuration::MaxPenetrations;
		}

		const bool SimFrequencyIsValid = PredictParams.SimFrequency > KINDA_SMALL_NUMBER;
		const bool IgnoreSimFrequencyValidity = SimType != ETBProjectileSimulationType::Instant; // Don't care about the SimFrequency if we aren't using "Instant" simulation.
		const bool Valid = IgnoreSimFrequencyValidity || SimFrequencyIsValid;
		if (World && Valid)
		{
			UseDragVector = DragCalculationType == ETBDragComplexity::WIND3D || DragCalculationType == ETBDragComplexity::COMPLEX;

			StartLocation = PredictParams.StartLocation;
			StartVelocity = PredictParams.LaunchVelocity;

			GravityZ = FMath::IsNearlyEqual(PredictParams.OverrideGravityZ, 0.0) ? World->GetGravityZ() : PredictParams.OverrideGravityZ;

			ResponseParams = CollisionPresets::SoloResponseChannel(PredictParams.TraceChannel);

			QueryParams = FCollisionQueryParams(SCENE_QUERY_STAT(PredictPathWithDrag), PredictParams.bTraceComplex);
			QueryParams.bReturnPhysicalMaterial = true;
			QueryParams.bReturnFaceIndex = true;
			QueryParams.AddIgnoredActors(PredictParams.ActorsToIgnore);

			for (auto Iter = PredictParams.ObjectTypes.CreateConstIterator(); Iter; ++Iter)
			{
				const ECollisionChannel& Channel = UCollisionProfile::Get()->ConvertToCollisionChannel(false, *Iter);
				ObjQueryParams.AddObjectTypesToQuery(Channel);
			}

			CurrentFlightData.Id = Id;
			SetLocation(PredictParams.StartLocation);
			SetVelocity(PredictParams.LaunchVelocity);
			SetAcceleration(FVector::ZeroVector);
			PreviousFlightData = GetCurrentFlightData();

			TraceStart = PredictParams.StartLocation;
			CurrentTime = 0.0;

			AddPoint(TraceStart, GetVelocity(), CurrentTime);

			if (UTerminalBallisticsStatics::OverlapsObjectWithTag(World, PredictParams.StartLocation, FTBCollisionParams(), GetProjectileRadius(), TB::Tags::PlainTag_IMPENETRABLE))
			{
				Kill(ExitCodes::EARLY_TERMINATION);
			}
		}
		else
		{
			Kill(ExitCodes::OTHER);
		}
	}

	template<typename TBSubsystem, typename TypeOfSimData, typename ProjectilePropertyStruct, typename DelegateStructType>
	inline void TProjectileSimulationTask<TBSubsystem, TypeOfSimData, ProjectilePropertyStruct, DelegateStructType>::PredictPathWithDrag()
	{
		if (SimType == ETBProjectileSimulationType::Instant)
		{
			SimulateInstant();
		}
		else
		{
			bIsReady = true;
		}
	}

	template<typename TBSubsystem, typename TypeOfSimData, typename ProjectilePropertyStruct, typename DelegateStructType>
	inline void TProjectileSimulationTask<TBSubsystem, TypeOfSimData, ProjectilePropertyStruct, DelegateStructType>::SimulateInstant()
	{
		const double SubstepDeltaTime = 1.0 / SimFrequency;
		while (SimTimeIsValid() && Iterations < MaxIterations && Iterations < TB::Configuration::MaxPenetrations)
		{
			// Limit step to not go further than total time.
			const double ActualStepDeltaTime = FMath::Min(MaxSimTime - CurrentTime, SubstepDeltaTime);
			Update(ActualStepDeltaTime);
		}
		const bool IsOverIterationLimit = Iterations >= MaxIterations;
		if (IsOverIterationLimit)
		{
			Kill(ExitCodes::ITERATION_LIMIT);
		}
		else
		{
			Kill(ExitCodes::TIME_LIMIT);
		}
	}

	template<typename TBSubsystem, typename TypeOfSimData, typename ProjectilePropertyStruct, typename DelegateStructType>
	void TProjectileSimulationTask<TBSubsystem, TypeOfSimData, ProjectilePropertyStruct, DelegateStructType>::ConsumeHits()
	{
		LLM_SCOPE_BYTAG(TProjectileSimulationTask_ConsumeHits);
		TRACE_CPUPROFILER_EVENT_SCOPE(TProjectileSimulationTask::ConsumeHits);

		bRetryHitConsumption = false;
		bool exitFound;
		bool bMultipleHits;
		if (UE::IsSavingPackage(nullptr) || IsGarbageCollecting())
		{
			/* Trying to continue further would result in a crash, since GC will interfere with any attempts to determine what we hit. */
			bRetryHitConsumption = true;
			return;
		}
		const FPhysMatProperties& SurfaceProperties = TB::GetSurfaceProperties(LatestHitResult);
		const FExitResults& ExitResults = TB::GetExitLocation(World, LatestHitResult, ProjectileProperties.Radius, exitFound, bMultipleHits, TraceChannel);
		if (!exitFound)
		{
			const ImpactStruct ImpactParams = ImpactStruct(LatestHitResult, GetProjectile(), GetVelocity(), true, SurfaceProperties.SurfaceType, StartLocation, false, Id, GetVelocity().Size());
			BroadcastHitDelegate(ImpactParams);
			Results.Add(ImpactParams);
			Kill(ExitCodes::NO_EXIT);
			return;
		}

		if (SurfaceProperties.bIsFluid)
		{
			if (CurrentFluidDensity > 0.f && SurfaceProperties.Density == CurrentFluidDensity) // We're already travelling in a fluid, and it has the same density as the new one.
			{
				return;
			}
			else
			{
				VelocityAtFluidEntry = GetVelocity() / 100.0;

				CurrentDistanceInFluid = 0.0;
				CurrentFluidDensity = SurfaceProperties.InBaseUnits().Density;
				CurrentFluidViscosity = SurfaceProperties.InBaseUnits().Viscosity;

				if (CVarPrintBallisticsDebugInfo.GetValueOnAnyThread())
				{
					TB_LOG(Warning, TEXT("1 Hit\n%s"), *SurfaceProperties.MaterialName.ToString());
				}
				FVector NewVelocity = GetVelocity();
				double ImpactAngle = 0.0, dE = 0.0;
				const bool bRicochet = TB::BallisticFunctions::ShouldRicochet(LatestHitResult, GetProjectile(), VelocityAtFluidEntry, SurfaceProperties, GetProjectilePhysMatProperties(), ExitResults.GetFirst().PenetrationThickness, ProjectileProperties.GetFrontalCSA(), ImpactAngle, dE, NewVelocity);
				if (bRicochet)
				{
					NewVelocity *= 100.0; // m/s to cm/s
					SetVelocity(NewVelocity);
					SetLocation(LatestHitResult.Location + NewVelocity.GetSafeNormal() * 0.1); // Set new location, nudge forwards to help depenetrate

					/* Assume the ricochet brought us out of the fluid */
					CurrentFluidDensity = 0.0f;
					CurrentFluidViscosity = 0.0f;
					CurrentDistanceInFluid = 0.0;
				}
				const ImpactStruct ImpactParams = ImpactStruct(LatestHitResult, GetProjectile(), VelocityAtFluidEntry, true, SurfaceProperties.SurfaceType, StartLocation, bRicochet, Id, VelocityAtFluidEntry.Size() - NewVelocity.Size(), NewVelocity);
				BroadcastHitDelegate(ImpactParams);
				AddPoint(LatestHitResult.ImpactPoint, GetVelocity(), CurrentTime);
				Results.Add(ImpactParams);

				bIsReady = true;
				return;
			}
		}


		CurrentFluidDensity = 0.f;
		CurrentFluidViscosity = 0.f;
		CurrentDistanceInFluid = 0.0;

		if (CVarPrintBallisticsDebugInfo.GetValueOnAnyThread())
		{
			TB_LOG(Warning, TEXT("%i Hit(s)"), ExitResults.Num());
		}

		for (const FExitResult& ExitResult : ExitResults)
		{
			const ConsumeHitReturnCode ExitCode = ConsumeHit(ExitResult);
			const bool WasRicochet = ExitCode == ConsumeHitReturnCode::Ricochet;
			if (WasRicochet)
			{
				break;
			}
			else if (ExitCode == ConsumeHitReturnCode::Kill)
			{
				return;
			}
		}

		SetLocation(GetLocation() + (GetVelocity().GetSafeNormal() * 0.1)); // Nudge to help with depenetration

		bIsReady = true;
	}

	template<typename TBSubsystem, typename TypeOfSimData, typename ProjectilePropertyStruct, typename DelegateStructType>
	void TProjectileSimulationTask<TBSubsystem, TypeOfSimData, ProjectilePropertyStruct, DelegateStructType>::Update(const double dt)
	{
		LLM_SCOPE_BYTAG(TProjectileSimulationTask_Update);
		TRACE_CPUPROFILER_EVENT_SCOPE(TProjectileSimulationTask::Update);
		if (CurrentTime < MaxSimTime && Iterations < MaxIterations)
		{
			if (bRetryHitConsumption)
			{
				bIsReady = false;
				ConsumeHits();
			}

			// Limit step to not go further than total time.
			const double PreviousTime = CurrentTime;

			const double ActualStepDeltaTime = FMath::Min(MaxSimTime - CurrentTime, dt * Timescale);
			CurrentTime += ActualStepDeltaTime;

			UpdatesSinceLastCall++;
			if (UpdatesSinceLastCall >= UpdateBroadcastInterval)
			{
				UpdatesSinceLastCall = 0;
				CallUpdateFunc();
			}
			if (Tracer)
			{
				UpdatesSinceLastTracerUpdate++;
				if (UpdatesSinceLastTracerUpdate >= TracerUpdateInterval)
				{
					UpdatesSinceLastTracerUpdate = 0;
					UpdateTracer();
				}
			}

			bool bIsInFluid = CurrentFluidViscosity > 0.f;
			//if (bIsInFluid)
			//{
			//	const FCollisionShape Sphere = FCollisionShape::MakeSphere(ProjectileProperties.Radius);
			//	if (PreviousHitResult.GetComponent() && !TB::DoesShapeOverlap(PreviousHitResult, GetLocation(), Sphere))
			//	{
			//		bIsInFluid = false;
			//		CurrentFluidDensity = 0.0f;
			//		CurrentFluidViscosity = 0.0f;
			//		CurrentDistanceInFluid = 0.0;
			//		/*UPrimitiveComponent* Component = PreviousHitResult.GetComponent();
			//		FHitResult Temp;
			//		if (Component->LineTraceComponent(Temp, GetLocation(), PreviousFlightData.Location, FCollisionQueryParams("FluidTest", true)))
			//		{

			//		}*/
			//	}
			//}

			UpdateDrag(GetLocation(), GetVelocity(), bIsInFluid ? CurrentFluidDensity : 0.0f);

			const bool bHit = CheckForHits(PreviousTime, ActualStepDeltaTime);
			if (bHit)
			{
				ConsumeHits();
			}
			bIsReady = true;
		}
		else
		{
			const bool IsOverIterationLimit = Iterations >= MaxIterations;
			if (IsOverIterationLimit)
			{
				Kill(ExitCodes::ITERATION_LIMIT);
			}
			else
			{
				Kill(ExitCodes::TIME_LIMIT);
			}
		}
	}

	template<typename TBSubsystem, typename TypeOfSimData, typename ProjectilePropertyStruct, typename DelegateStructType>
	inline void TProjectileSimulationTask<TBSubsystem, TypeOfSimData, ProjectilePropertyStruct, DelegateStructType>::EstimateTracerUpdateInterval()
	{
		const double Speed = GetVelocity().Size() / 100.0;
		const double DistancePerFrame = Speed / 60;
		if (DistancePerFrame <= 1)
		{
			TracerUpdateInterval = 1;
		}
		else
		{
			TracerUpdateInterval = 2 + ceil(DistancePerFrame / 2);
		}
	}
};
#pragma endregion
