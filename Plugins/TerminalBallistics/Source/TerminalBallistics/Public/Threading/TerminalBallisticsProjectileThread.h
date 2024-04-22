// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "Misc/SingleThreadRunnable.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Misc/Timespan.h"
#include "SimulationTasks.h"
#include "TerminalBallisticsThreadingTypes.h"
#include <chrono>
#include <condition_variable>
#include <functional>


LLM_DECLARE_TAG(FTBProjectileThreadPool);
LLM_DECLARE_TAG(FTBProjectileThreadPool_AddNewTask);
LLM_DECLARE_TAG(FTBProjectileThreadPool_AddThread);
LLM_DECLARE_TAG(FTBProjectileThread);
LLM_DECLARE_TAG(FTBProjectileWorkerThread);
LLM_DECLARE_TAG(FTBProjectileWorkerThread_Tick);
LLM_DECLARE_TAG(FTBProjectileWorkerThread_AddTask);
LLM_DECLARE_TAG(FTBProjectileWorkerThread_ProcessPending);

class FTBProjectileThreadPool
{
	using ProjectileTask = TB::SimTasks::ITBProjectileTask;
	using ThreadTaskAffinity = TB::SimTasks::ETBThreadTaskAffinity;
	using TaskOverflowBehavior = TB::SimTasks::ETBThreadTaskOverflowType;
	using FTBProjectileThreadPoolParams = TB::SimTasks::FTBProjectileThreadPoolParams;
	using WorkerThreadPtr = TSharedPtr<class FTBProjectileWorkerThread>;
public:
	explicit FTBProjectileThreadPool(const FTimespan ThreadTickRate, const int32 NumThreads = 3, const uint32 ThreadStackSize = 256 * 1024, const int32 MaxTasksPerThread = 100, bool CapNumThreads = true, const int32 MaxThreads = 10, TaskOverflowBehavior DefaultOverflowBehavior = TaskOverflowBehavior::CreateNew)
		: ThreadTickRate(ThreadTickRate)
		, ThreadStackSize(ThreadStackSize)
		, MaxTasksPerThread(MaxTasksPerThread)
		, MaxThreads(MaxThreads)
		, CapNumThreads(CapNumThreads)
		, InitialThreads(NumThreads)
	{
		LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
		SetupInitialThreads();
	}

	FTBProjectileThreadPool(const FTBProjectileThreadPoolParams& Params)
		: ThreadTickRate(Params.ThreadTickRate)
		, ThreadStackSize(Params.ThreadStackSize)
		, MaxTasksPerThread(Params.MaxTasksPerThread)
		, MaxThreads(Params.MaxThreads)
		, CapNumThreads(Params.CapNumThreads)
		, InitialThreads(Params.NumThreads)
	{
		LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
		SetupInitialThreads(Params);
	}

	~FTBProjectileThreadPool();

	void KillThreads();

	void StopThreads();

	void PauseThreads();

	void UnpauseThreads();

	void SetTimescale(const float NewTimescale);

	inline void Kill()
	{
		LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
		KillThreads();
		Threads.Empty();
	}

	void Shrink();

	inline int GetNumThreads() { return Threads.Num(); }

	inline bool HasActive() const { return NumActive != 0; }
	inline int GetNumActive() const { return NumActive; }

	void SetThreadTaskAffinity(ThreadTaskAffinity Affinity, int32 ThreadIndex, bool IsStrongAffinity = false);

	template<typename TaskType>
	void AddNewTask(ThreadTaskAffinity Affinity, class FTBProjectileThread* ProjectileThread, typename TaskType::SimDataType&& SimData);

	bool KillTask(ProjectileTask* Task);
	bool KillTaskByIdAndAffinity(const FTBProjectileId& Id, const ThreadTaskAffinity Affinity);

	void KillTasks();

	WorkerThreadPtr AddThread(ThreadTaskAffinity Affinity = ThreadTaskAffinity::NONE, const bool IsStrongAffinity = false, const TaskOverflowBehavior OverflowBehavior = TaskOverflowBehavior::CreateNew, const int32 MaxTasks = 100);
	WorkerThreadPtr AddThread(TB::SimTasks::FTBProjectileWorkerParams Params);

	void SetupInitialThreads();
	void SetupInitialThreads(FTBProjectileThreadPoolParams ThreadPoolParams);

	FCriticalSection TaskRemovalMutex;
	FCriticalSection TaskAdditionMutex;

protected:
	inline bool CanAddThread() const
	{
		if (CapNumThreads && Threads.Num() >= MaxThreads)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	TArray<WorkerThreadPtr> GetNonFullThreads();
	TArray<WorkerThreadPtr> GetThreadsWithAffinity(ThreadTaskAffinity Affinity = ThreadTaskAffinity::NONE);
	WorkerThreadPtr FindOrAddThreadForTask(ThreadTaskAffinity Affinity = ThreadTaskAffinity::NONE);

protected:
	friend class FTBProjectileWorkerThread;

	FTimespan ThreadTickRate;

	TArray<WorkerThreadPtr> Threads;

	std::atomic<int32> NumActive;

	uint32 ThreadStackSize;
	int32 MaxTasksPerThread;
	int32 MaxThreads;
	bool CapNumThreads;
	int32 InitialThreads;

	int32 StaticThreadCount = 0;

	TaskOverflowBehavior DefaultOverflowBehavior;
};


/**
* Main thread responsible for adding/removing projectiles.
* @see FTBProjectileThreadPool
* @see FTBProjectileWorkerThread
* @see TProjectileSimulationTask
*/
class FTBProjectileThread final : public FRunnable, public FSingleThreadRunnable, public FTBBaseProjectileThread
{
public:
	FTBProjectileThread(const FTimespan& TickRate, const TCHAR* ThreadName, UTerminalBallisticsSubsystem* Controller, class UTBProjectileThreadQueue* DataQueue);

	// FRunnable Implementation Begin
	inline virtual ~FTBProjectileThread() override final
	{
		LLM_SCOPE_BYTAG(FTBProjectileThread);
		IsStopping = true;
		TaskCondition.notify_one();
		if (Thread != nullptr)
		{
			Thread->Kill();
			delete Thread;
		}
		if (ThreadPool != nullptr)
		{
			ThreadPool->Kill();
			ThreadPool.Reset();
		}
	}
	inline virtual FSingleThreadRunnable* GetSingleThreadInterface() override final { return this; }
	virtual bool Init() override final;
	virtual uint32 Run() override final;
	virtual void Stop() override final;
	// FRunnable Implementation End

	// FSingleThreadRunnable Implementation Begin
	virtual void Tick() override final;
	// FSingleThreadRunnable Implementation End

	virtual bool HasActive() const override final;

	virtual int GetNumActive() const override final;

	bool RequestProjectileTermination(const FTBProjectileId& ProjectileId, const TB::SimTasks::ETBThreadTaskAffinity TaskAffinity);

	inline virtual const FTimespan GetTickRate() const override final { return TickRate; }

	void ThreadTick();

	void AddNewBulletTask(struct FTBBulletSimData&& SimData);
	void AddNewProjectileTask(struct FTBProjectileSimData&& SimData);

	inline void Sleep(const float DurationSeconds) { FPlatformProcess::Sleep(DurationSeconds); }

	void Pause();
	void Unpause();

	inline bool ThreadPaused() const { return Paused; }
	inline bool ThreadSuspended() const { return IsSuspended; }
	inline bool ThreadStopped() const { return HasStopped; }

	void Shutdown();

	void SetTimeDilation(const float NewTimeDilation);

	virtual void OnBulletTaskExit(FBulletTaskResult&& Result) override;
	virtual void OnProjectileTaskExit(FProjectileTaskResult&& Result) override;

	class UTBProjectileThreadQueue* DataQueue;

	std::mutex TaskMutex;
	std::condition_variable TaskCondition;

protected:
	void RecieveProjectiles();

	void SetPauseState(const bool NewPauseState);

	std::atomic_bool Paused;
	std::atomic_bool IsSuspended;
	std::atomic_bool HasStopped;

	std::atomic<int32> InactiveTicks; // Used to delay thread pause after all projectiles are inactive, just to make sure nothing gets missed.
	double InactiveDelaySeconds = 2;
	int InactiveDelayTicks;

	std::atomic_bool IsStopping;

	FRunnableThread* Thread;

	FTimespan TickRate;

	float CurrentTimeDilation = 1.f;

	class UTerminalBallisticsSubsystem* Controller;

	TSharedPtr<FTBProjectileThreadPool> ThreadPool;
};

template<typename TaskType>
struct TTBTaskConstructionParams
{
	using SimDataType = typename TaskType::SimDataType;
	FTBProjectileThread* Controller;
	SimDataType SimData;

	TTBTaskConstructionParams() = default;

	TTBTaskConstructionParams(FTBProjectileThread* Controller, SimDataType&& SimData)
		: Controller(Controller)
		, SimData(SimData)
	{}

	TSharedPtr<TaskType> CreateTask()
	{
		return MakeShareable(new TaskType(Controller, SimData));
	}
};

typedef TTBTaskConstructionParams<TB::SimTasks::BulletSimulationTask> FTBBulletTaskConstructionParams;
typedef TTBTaskConstructionParams<TB::SimTasks::ProjectileSimulationTask> FTBProjectileTaskConstructionParams;

class FTBProjectileWorkerThread final : public FRunnable, public FSingleThreadRunnable
{
	using ProjectileTask = TB::SimTasks::ITBProjectileTask;
	using ThreadTaskAffinity = TB::SimTasks::ETBThreadTaskAffinity;
	using TaskOverflowBehavior = TB::SimTasks::ETBThreadTaskOverflowType;
	using TaskPtr = TSharedPtr<ProjectileTask>;
public:
	FTBProjectileWorkerThread(FTBProjectileThreadPool* OwningPool, const FTimespan& TickRate, const TCHAR* ThreadName, const uint32 StackSize = 128 * 1024, const int32 InMaxTasks = 100, const TaskOverflowBehavior OverflowBehavior = TaskOverflowBehavior::CreateNew, const ThreadTaskAffinity TaskAffinity = ThreadTaskAffinity::NONE, const bool AffinityIsStrong = false)
		: EarlyTick(false)
		, OwningPool(OwningPool)
		, MaxTasks(InMaxTasks)
		, OverflowBehavior(OverflowBehavior)
		, TaskAffinity(TaskAffinity)
		, AffinityIsStrong(AffinityIsStrong)
		, IsPaused(false)
		, IsStopping(false)
		, HasStopped(false)
		, TickRate(TickRate)
	{
		LLM_SCOPE_BYTAG(FTBProjectileWorkerThread);
		if (MaxTasks <= 0)
		{
			MaxTasks = 100;
		}
		Thread = FRunnableThread::Create(this, ThreadName, StackSize, EThreadPriority::TPri_AboveNormal);
		SimTasks.Reserve(MaxTasks);
		PreEngineExitHandle = FCoreDelegates::OnEnginePreExit.AddRaw(this, &FTBProjectileWorkerThread::OnShutdown);
		OnWorldBeginTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddRaw(this, &FTBProjectileWorkerThread::OnShutdown);
		OnWorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddRaw(this, &FTBProjectileWorkerThread::OnShutdown);
	}
	
	void OnShutdown();
	void OnShutdown(UWorld*)
	{
		OnShutdown();
	}
	void OnShutdown(UWorld*, bool = false, bool = false)
	{
		OnShutdown();
	}

	// FRunnable Implementation Begin
	inline virtual ~FTBProjectileWorkerThread() noexcept override final
	{
		LLM_SCOPE_BYTAG(FTBProjectileWorkerThread);
		TRACE_CPUPROFILER_EVENT_SCOPE(FTBProjectileWorkerThread::ShuttingDown);
		IsStopping = true;
		TaskCondition.notify_one();
		FCoreDelegates::OnEnginePreExit.Remove(PreEngineExitHandle);
		FWorldDelegates::OnWorldBeginTearDown.Remove(OnWorldBeginTearDownHandle);
		FWorldDelegates::OnWorldCleanup.Remove(OnWorldCleanupHandle);
		OnShutdown();
		KillTasks();
		if (Thread != nullptr)
		{
			Thread->Kill();
			delete Thread;
		}
	}
	inline virtual FSingleThreadRunnable* GetSingleThreadInterface() override final { return this; }
	virtual bool Init() override final;
	virtual uint32 Run() override final;
	virtual void Stop() override final;
	// FRunnable Implementation End

	// FSingleThreadRunnable Implementation Begin
	inline virtual void Tick() override final { TickTasks(); }
	// FSingleThreadRunnable Implementation End

	void KillTasks();
	void TickTasks();

	template<typename T>
	void RecieveTask(TTBTaskConstructionParams<T>&& TaskConstructionParams);

	inline void SetTimescale(const float NewTimescale)
	{
		Timescale = NewTimescale;
	}

	inline bool IsWaitingForWork() const
	{
		return NumActive == 0
			&& BulletsToAdd.IsEmpty()
			&& ProjectilesToAdd.IsEmpty()
			&& ToRemove.IsEmpty();
	}

	inline int GetMax() const { return MaxTasks; }

	inline int GetNum() const { return NumTasks; }


	bool KillTask(ProjectileTask* Task);
	bool KillTaskByIdAndAffinity(const FTBProjectileId& TaskId, const ThreadTaskAffinity Affinity);

	inline bool CanAcceptTask() const
	{
		return NumTasks < MaxTasks || OverflowBehavior == TaskOverflowBehavior::Allow; // Accept if we're under MaxTasks or we don't enforce the maximum
	}

	inline bool CanAcceptTask(const ThreadTaskAffinity Affinity) const
	{
		const bool CanAcceptAnyTasks = CanAcceptTask();
		const bool CanAcceptTaskWithAffinity = !AffinityIsStrong || Affinity == TaskAffinity; // Don't accept if we have a strong affinity and the given affinity isn't the same as ours
		return CanAcceptAnyTasks && CanAcceptTaskWithAffinity;
	}

	ThreadTaskAffinity GetTaskAffinity() const
	{
		return TaskAffinity;
	}

	inline void SetTaskAffinity(const ThreadTaskAffinity NewAffinity, bool IsStrongAffinity = false)
	{
		TaskAffinity = NewAffinity;
		AffinityIsStrong = IsStrongAffinity;
	}

	inline bool HasStrongAffinity() const
	{
		return AffinityIsStrong;
	}

	inline TaskOverflowBehavior GetOverflowBehavior() const
	{
		return OverflowBehavior;
	}

	inline void SetOverflowBehavior(const TaskOverflowBehavior NewOverflowBehavior)
	{
		OverflowBehavior = NewOverflowBehavior;
	}

	void RemoveTask(ProjectileTask* Task);

	void AddTask(FTBProjectileThread* ProjectileThread, struct FTBBulletSimData&& SimData);
	void AddTask(FTBProjectileThread* ProjectileThread, struct FTBProjectileSimData&& SimData);

	inline void Pause()
	{
		SetPauseState(true);
	}

	inline void Unpause()
	{
		SetPauseState(false);
	}

	inline bool operator<(const FTBProjectileWorkerThread& Other)
	{
		return GetNum() < Other.GetNum();
	}

	std::mutex TaskMutex;
	std::condition_variable TaskCondition;

	std::atomic_bool EarlyTick;

protected:
	void ProcessPending();
	inline void Sleep(const float DurationSeconds) { FPlatformProcess::Sleep(DurationSeconds); }
	void SetPauseState(const bool NewPauseState);

protected:
	bool bFirstUpdate = true;

	std::atomic<int32> NumActive;

	FTBProjectileThreadPool* OwningPool;

	int32 MaxTasks;
	TaskOverflowBehavior OverflowBehavior;

	ThreadTaskAffinity TaskAffinity; // The type of tasks that this thread prefers
	std::atomic_bool AffinityIsStrong;

	std::atomic_bool IsPaused;
	std::atomic_bool IsStopping;
	std::atomic_bool HasStopped;

	bool InTickLoop = false;

	std::atomic_bool WasPaused;

	using time_point = std::chrono::steady_clock::time_point;
	time_point TickEntry;
	time_point TickExit;

	float Timescale = 1.f;

	FTimespan TickRate;

	FRunnableThread* Thread;

	std::atomic<int32> NumTasks;
	TArray<TaskPtr> SimTasks;

	TQueue<TSharedPtr<FTBBulletTaskConstructionParams>, EQueueMode::Mpsc> BulletsToAdd;
	TQueue<TSharedPtr<FTBProjectileTaskConstructionParams>, EQueueMode::Mpsc> ProjectilesToAdd;

	TQueue<ProjectileTask*, EQueueMode::Mpsc> ToRemove;

	mutable FRWLock TaskCopyLock;
	std::mutex RecieveTaskMutex;

	FDelegateHandle PreEngineExitHandle;
	FDelegateHandle OnWorldBeginTearDownHandle;
	FDelegateHandle OnWorldCleanupHandle;
};

FORCEINLINE bool operator<(const FTBProjectileWorkerThread& lhs, const FTBProjectileWorkerThread& rhs)
{
	return lhs.GetNum() < rhs.GetNum();
}
FORCEINLINE bool operator<(const TSharedPtr<FTBProjectileWorkerThread>& lhs, const TSharedPtr<FTBProjectileWorkerThread>& rhs)
{
	return lhs && rhs ? lhs->GetNum() < rhs->GetNum() : false;
}



template<typename TaskType>
inline void FTBProjectileThreadPool::AddNewTask(ThreadTaskAffinity Affinity, FTBProjectileThread* ProjectileThread, typename TaskType::SimDataType&& SimData)
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool_AddNewTask);
	TRACE_CPUPROFILER_EVENT_SCOPE(FTBProjectileThreadPool::AddNewTask);
	WorkerThreadPtr ThreadForTask = FindOrAddThreadForTask(Affinity);
	ensureAlways(ThreadForTask);
	if (ThreadForTask)
	{
		const int32 NumBeforeAdd = ThreadForTask->GetNum();
		if (NumBeforeAdd >= ThreadForTask->GetMax())
		{
			if (ThreadForTask->GetOverflowBehavior() != TaskOverflowBehavior::Allow)
			{
				TB_LOG(Display, TEXT("Warning: added task to thread with %i tasks."), NumBeforeAdd);
			}
		}
		FScopeLock AddLock(&TaskAdditionMutex);
		ThreadForTask->AddTask(ProjectileThread, MoveTemp(SimData));
	}
}

template<typename T>
inline void FTBProjectileWorkerThread::RecieveTask(TTBTaskConstructionParams<T>&& TaskConstructionParams)
{
	LLM_SCOPE_BYTAG(FTBProjectileWorkerThread);
	TRACE_CPUPROFILER_EVENT_SCOPE(FTBProjectileWorkerThread::RecieveTask);
	std::lock_guard lock(RecieveTaskMutex);
	TaskPtr NewTask = TaskConstructionParams.CreateTask();
	ensureAlways(NewTask);
	if(NewTask)
	{
		NumActive++;
		OwningPool->NumActive++;
		NewTask->SetOnCompleteCallback(std::bind(&FTBProjectileWorkerThread::RemoveTask, this, std::placeholders::_1));
		FWriteScopeLock AddLock(TaskCopyLock);
		SimTasks.Emplace(MoveTemp(NewTask));
	}
	else
	{
		TRACE_BOOKMARK(TEXT("Failed to create simulation task."))
		TB_LOG(Error, TEXT("Failed to create simulation task."));
	}
}
