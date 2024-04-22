// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "Threading/TerminalBallisticsProjectileThread.h"
#include "TerminalBallisticsMacrosAndFunctions.h"
#include "Threading/SimulationTasks.h"
#include "Subsystems/TerminalBallisticsSubsystem.h"


LLM_DEFINE_TAG(FTBProjectileThreadPool);
LLM_DEFINE_TAG(FTBProjectileThreadPool_AddNewTask);
LLM_DEFINE_TAG(FTBProjectileThreadPool_AddThread);
LLM_DEFINE_TAG(FTBProjectileThread);
LLM_DEFINE_TAG(FTBProjectileWorkerThread);
LLM_DEFINE_TAG(FTBProjectileWorkerThread_Tick);
LLM_DEFINE_TAG(FTBProjectileWorkerThread_AddTask);
LLM_DEFINE_TAG(FTBProjectileWorkerThread_ProcessPending);

const TAutoConsoleVariable<int32> CVarWorkerThreadStackSize(
	TEXT("tb.Thread.WorkerThreadStackSize"),
	256,
	TEXT("Amount of memory to available to each worker thread. (KiB)"),
	ECVF_Cheat
);

#pragma region FTBProjectileThread
FTBProjectileThread::FTBProjectileThread(const FTimespan& TickRate, const TCHAR* ThreadName, UTerminalBallisticsSubsystem* Controller, UTBProjectileThreadQueue* DataQueue)
	: FTBBaseProjectileThread(Controller)
	, DataQueue(DataQueue)
	, IsStopping(false)
	, TickRate(TickRate)
{
	LLM_SCOPE_BYTAG(FTBProjectileThread);
	Paused = false;
	DataQueue->SetUnpauseFunction(std::bind(&FTBProjectileThread::Unpause, this));
	InactiveDelayTicks = ceil(InactiveDelaySeconds / TickRate.GetTotalSeconds());

	using namespace TB::SimTasks;
	TArray<FTBProjectileWorkerParams> WorkerThreadParams;
	WorkerThreadParams.Emplace(FTBProjectileWorkerParams::GeneralPurpose);
	WorkerThreadParams.Emplace(FTBProjectileWorkerParams::Bullets);
	WorkerThreadParams.Emplace(FTBProjectileWorkerParams::Bullets);
	WorkerThreadParams.Emplace(FTBProjectileWorkerParams::Projectiles);
	WorkerThreadParams.Emplace(FTBProjectileWorkerParams::Projectiles);
	WorkerThreadParams.Emplace(FTBProjectileWorkerParams::Projectiles);
	WorkerThreadParams.Emplace(FTBProjectileWorkerParams::Projectiles);

	FTBProjectileThreadPoolParams ThreadPoolParams = FTBProjectileThreadPoolParams(TickRate, WorkerThreadParams, CVarWorkerThreadStackSize.GetValueOnAnyThread() * 1024);
	ThreadPoolParams.MaxThreads = TB::Configuration::MaximumProjectileThreads;
	ThreadPool = MakeShareable(new FTBProjectileThreadPool(ThreadPoolParams));

	Thread = FRunnableThread::Create(this, ThreadName, 128 * 1024, EThreadPriority::TPri_AboveNormal);
}

bool FTBProjectileThread::Init()
{
	return true;
}

uint32 FTBProjectileThread::Run()
{
	LLM_SCOPE_BYTAG(FTBProjectileThread);
	HasStopped = false;

	while (!IsStopping && !HasStopped)
	{
		std::unique_lock<std::mutex> lock(TaskMutex);
		using TB::SimTasks::double_seconds;
		//TaskCondition.wait_for(lock, double_seconds(TickRate.GetTotalSeconds()), [&]() { return IsStopping || HasStopped || DataQueue->HasInputData(); });
		TaskCondition.wait(lock, [&]() { return IsStopping || HasStopped || DataQueue->HasInputData(); });

		if (IsStopping || HasStopped)
		{
			break;
		}
		if (Paused)
		{
			continue;
		}
		if(DataQueue->HasInputData())
		{
			ThreadTick();
		}
	}

	HasStopped = true;
	return 0;
}

void FTBProjectileThread::Stop()
{
	Pause();
	IsStopping = true;
}

void FTBProjectileThread::Tick()
{
	ThreadTick();
}

void FTBProjectileThread::ThreadTick()
{
	if (ThreadPool)
	{
		/* Check queue for incoming tasks */
		RecieveProjectiles();
		
		if (ThreadPool && !HasActive())
		{
			/* Pause thread if there is nothing to do and we've waited InactiveDelaySeconds since we last had work */
			if (InactiveTicks++ >= InactiveDelayTicks)
			{
				TRACE_BOOKMARK(TEXT("FTBProjectileThread Going Inactive"));
				TRACE_CPUPROFILER_EVENT_SCOPE(FTBProjectileThread::GoingInactive);
				ThreadPool->KillTasks();
				ThreadPool->Shrink(); // Get rid of any extra threads that were created to handle overflow
				InactiveTicks = 0;
				Pause();
			}
		}
		else
		{
			InactiveTicks = 0;
		}
	}
}

void FTBProjectileThread::AddNewBulletTask(FTBBulletSimData&& SimData)
{
	LLM_SCOPE_BYTAG(FTBProjectileThread);
	using namespace TB::SimTasks;
	ThreadPool->AddNewTask<BulletSimulationTask>(ETBThreadTaskAffinity::BULLET_TASK, this, std::forward<FTBBulletSimData>(SimData));
}

void FTBProjectileThread::AddNewProjectileTask(FTBProjectileSimData&& SimData)
{
	LLM_SCOPE_BYTAG(FTBProjectileThread);
	using namespace TB::SimTasks;
	ThreadPool->AddNewTask<ProjectileSimulationTask>(ETBThreadTaskAffinity::PROJECTILE_TASK, this, std::forward<FTBProjectileSimData>(SimData));
}

void FTBProjectileThread::Pause()
{
	SetPauseState(true);
}

void FTBProjectileThread::Unpause()
{
	SetPauseState(false);
}

void FTBProjectileThread::Shutdown()
{
	LLM_SCOPE_BYTAG(FTBProjectileThread);
	DataQueue->Empty();
	ThreadPool->KillThreads();
	IsStopping = true;
	TaskCondition.notify_one();
}

void FTBProjectileThread::SetTimeDilation(const float NewTimeDilation)
{
	if (NewTimeDilation > 0 && NewTimeDilation != CurrentTimeDilation)
	{
		CurrentTimeDilation = NewTimeDilation;
		ThreadPool->SetTimescale(CurrentTimeDilation);
	}
}

bool FTBProjectileThread::HasActive() const
{
	TB_RET_COND_VALUE(!ThreadPool, false);
	return ThreadPool->HasActive();
}

int FTBProjectileThread::GetNumActive() const
{
	TB_RET_COND_VALUE(!ThreadPool, 0);
	return ThreadPool->GetNumActive();
}

bool FTBProjectileThread::RequestProjectileTermination(const FTBProjectileId& ProjectileId, const TB::SimTasks::ETBThreadTaskAffinity TaskAffinity)
{
	if (ThreadPool)
	{
		return ThreadPool->KillTaskByIdAndAffinity(ProjectileId, TaskAffinity);
	}
	return false;
}

void FTBProjectileThread::OnBulletTaskExit(FBulletTaskResult&& Result)
{
	LLM_SCOPE_BYTAG(FTBProjectileThread);
	DataQueue->Enqueue(MoveTemp(Result));
	if (Paused)
	{
		Unpause();
	}
}

void FTBProjectileThread::OnProjectileTaskExit(FProjectileTaskResult&& Result)
{
	LLM_SCOPE_BYTAG(FTBProjectileThread);
	DataQueue->Enqueue(MoveTemp(Result));
	if (Paused)
	{
		Unpause();
	}
}

void FTBProjectileThread::RecieveProjectiles()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FTBProjectileThread::RecieveProjectiles);
	LLM_SCOPE_BYTAG(FTBProjectileThread);
	using namespace std::chrono;
	using namespace TB::SimTasks;

	FTBBulletSimData NewBulletData;
	FTBProjectileSimData NewProjectileData;

	const auto Max_Duration = double_seconds(TickRate.GetTotalSeconds());
	const auto Entry_time_point = high_resolution_clock::now();
	while (DataQueue->HasInputData() && duration_cast<double_seconds>(high_resolution_clock::now() - Entry_time_point) < Max_Duration)
	{
		/* Add any new bullets */
		if (DataQueue->Dequeue(NewBulletData))
		{
			ThreadPool->AddNewTask<BulletSimulationTask>(ETBThreadTaskAffinity::BULLET_TASK, this, MoveTemp(NewBulletData));
		}
		/* Add any new projectiles */
		if (DataQueue->Dequeue(NewProjectileData))
		{
			ThreadPool->AddNewTask<ProjectileSimulationTask>(ETBThreadTaskAffinity::PROJECTILE_TASK, this, MoveTemp(NewProjectileData));
		}
	}
}

void FTBProjectileThread::SetPauseState(const bool NewPauseState)
{
	if (NewPauseState != Paused)
	{
		Paused = NewPauseState;
		DataQueue->IsThreadPaused = NewPauseState;
		if (!Paused)  // Unsuspend if thread is no longer paused
		{
			IsSuspended = false;
		}
		TaskCondition.notify_one();

		if (ThreadPool)
		{
			if (Paused)
			{
				ThreadPool->PauseThreads();
			}
			else
			{
				ThreadPool->UnpauseThreads();
			}
		}
	}
}
#pragma endregion

#pragma region FTBProjectileThreadPool
typedef TSharedPtr<FTBProjectileWorkerThread> WorkerThreadPtr;
typedef TArray<WorkerThreadPtr> WorkerThreadArray;

WorkerThreadPtr FTBProjectileThreadPool::AddThread(ThreadTaskAffinity Affinity, const bool IsStrongAffinity, const TaskOverflowBehavior OverflowBehavior, const int32 MaxTasks)
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool_AddThread);
	if (CanAddThread())
	{
		FString ThreadName = "TBProjectileWorkerThread #";
		ThreadName += FString::FromInt(++StaticThreadCount);
		ThreadName += " (" + TB::SimTasks::GetThreadTaskAffinityAsString(Affinity) + ")";
		WorkerThreadPtr NewThread = Threads.Emplace_GetRef(MakeShareable(new FTBProjectileWorkerThread(this, ThreadTickRate, *ThreadName, ThreadStackSize, MaxTasks, OverflowBehavior, Affinity, IsStrongAffinity)));
		NewThread->Init();
		return NewThread;
	}
	else
	{
		return nullptr;
	}
}

WorkerThreadPtr FTBProjectileThreadPool::AddThread(TB::SimTasks::FTBProjectileWorkerParams Params)
{
	return AddThread(Params.Affinity, Params.IsStrongAffinity, Params.OverflowBehavior, Params.MaxTasks);
}

void FTBProjectileThreadPool::SetupInitialThreads()
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
	using namespace TB::SimTasks;

	Threads.Reserve(InitialThreads);
	switch (InitialThreads)
	{
	case 1:
		/* Since there's only one thread, make it general purpose */
		AddThread(FTBProjectileWorkerParams::GeneralPurpose);
		break;
	case 2:
		/* Two threads, make one for bullets and one for projectiles */
		AddThread(FTBProjectileWorkerParams::Bullets);
		AddThread(FTBProjectileWorkerParams::Projectiles);
		break;
	case 3:
		/* One of each preset */
		AddThread(FTBProjectileWorkerParams::GeneralPurpose);
		AddThread(FTBProjectileWorkerParams::Bullets);
		AddThread(FTBProjectileWorkerParams::Projectiles);
		break;
	default:
		/* First add a general purpose thread to accommodate task overflow */
		AddThread(FTBProjectileWorkerParams::GeneralPurpose);
		for (int i = 1; i < InitialThreads; i++)
		{
			AddThread();
		}
		break;
	}
}

void FTBProjectileThreadPool::SetupInitialThreads(FTBProjectileThreadPoolParams ThreadPoolParams)
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
	if (ThreadPoolParams.ThreadParams.IsEmpty())
	{
		SetupInitialThreads();
	}
	else
	{
		Threads.Reserve(ThreadPoolParams.ThreadParams.Num());
		for (const TB::SimTasks::FTBProjectileWorkerParams Params : ThreadPoolParams.ThreadParams)
		{
			AddThread(Params.Affinity, Params.IsStrongAffinity, Params.OverflowBehavior, Params.MaxTasks);
		}
	}
}

FTBProjectileThreadPool::~FTBProjectileThreadPool()
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
	Kill();
}

void FTBProjectileThreadPool::KillThreads()
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
	for (WorkerThreadPtr& Thread : Threads)
	{
		Thread->KillTasks();
		Thread->Stop();
	}
}

void FTBProjectileThreadPool::StopThreads()
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
	for (WorkerThreadPtr& Thread : Threads)
	{
		Thread->Stop();
	}
}

void FTBProjectileThreadPool::PauseThreads()
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
	for (WorkerThreadPtr& Thread : Threads)
	{
		Thread->Pause();
	}
}

void FTBProjectileThreadPool::UnpauseThreads()
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
	for (WorkerThreadPtr& Thread : Threads)
	{
		Thread->Unpause();
	}
}

void FTBProjectileThreadPool::SetTimescale(const float NewTimescale)
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
	if (NewTimescale > 0.f)
	{
		for (WorkerThreadPtr& Thread : Threads)
		{
			Thread->SetTimescale(NewTimescale);
		}
	}
}

void FTBProjectileThreadPool::Shrink()
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
	Threads.SetNum(InitialThreads);
}

void FTBProjectileThreadPool::SetThreadTaskAffinity(ThreadTaskAffinity Affinity, int32 ThreadIndex, bool IsStrongAffinity)
{
	if (!Threads.IsValidIndex(ThreadIndex))
	{
		return;
	}
	Threads[ThreadIndex]->SetTaskAffinity(Affinity, IsStrongAffinity);
}

bool FTBProjectileThreadPool::KillTask(ProjectileTask* Task)
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
	if (!HasActive())
	{
		return false;
	}

	for (auto& Thread : Threads)
	{
		if(Thread)
		{
			bool Killed = Thread->KillTask(Task);
			if (Killed)
			{
				return true;
			}
		}
	}

	return false;
}

bool FTBProjectileThreadPool::KillTaskByIdAndAffinity(const FTBProjectileId& Id, const ThreadTaskAffinity Affinity)
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
	if (!HasActive())
	{
		return false;
	}

	for (auto& Thread : Threads)
	{
		if(Thread)
		{
			bool Killed = Thread->KillTaskByIdAndAffinity(Id, Affinity);
			if (Killed)
			{
				return true;
			}
		}
	}

	return false;
}

void FTBProjectileThreadPool::KillTasks()
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
	for (auto& Thread : Threads)
	{
		if(Thread)
		{
			Thread->KillTasks();
		}
	}
}

WorkerThreadArray FTBProjectileThreadPool::GetNonFullThreads()
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
	auto ToReturn = Threads.FilterByPredicate([&](const WorkerThreadPtr Thread) { return Thread && Thread->GetNum() != MaxTasksPerThread; });
	ToReturn.Shrink();
	return ToReturn;
}

WorkerThreadArray FTBProjectileThreadPool::GetThreadsWithAffinity(ThreadTaskAffinity Affinity)
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
	auto ToReturn = Threads.FilterByPredicate([Affinity](const WorkerThreadPtr Thread) {return Thread && Thread->GetTaskAffinity() == Affinity; });
	ToReturn.Shrink();
	return ToReturn;
}

WorkerThreadPtr FTBProjectileThreadPool::FindOrAddThreadForTask(ThreadTaskAffinity Affinity)
{
	LLM_SCOPE_BYTAG(FTBProjectileThreadPool);
	TRACE_CPUPROFILER_EVENT_SCOPE(FTBProjectileThreadPool::FindOrAddThreadForTask);

	if (this && Threads.IsEmpty())
	{
		SetupInitialThreads();
		if (Threads.IsEmpty())
		{
			return nullptr;
		}
	}

	Threads.Sort();

	const auto AffinityPred = [Affinity](const WorkerThreadPtr Thread) {return Thread && Thread->GetTaskAffinity() == Affinity; };
	const auto CanAddPred = [Affinity](const WorkerThreadPtr Thread) {return Thread && Thread->CanAcceptTask(Affinity); };

	WorkerThreadPtr FoundThread = nullptr;

	/* First, see if there any any threads that aren't full. If not, try to add one.*/
	WorkerThreadArray NotFull = GetNonFullThreads();
	NotFull.Shrink();
	{
		if (NotFull.Num() == 0)
		{
			if (CanAddThread())
			{
				return AddThread(Affinity);
			}
			else
			{
				FoundThread = *Threads.FindByPredicate(AffinityPred);
				if (!FoundThread)
				{
					FoundThread = *Threads.FindByPredicate(CanAddPred);
					if (!FoundThread)
					{ // Can't add a new thread, and there are none with our preferred affinity
						WorkerThreadArray ThreadsWithNoAffinity = GetThreadsWithAffinity();
						if (ThreadsWithNoAffinity.Num() > 0)
						{
							return ThreadsWithNoAffinity[0];
						}
						else
						{
							return Threads[0];
						}
					}
				}
				return FoundThread;
			}
		}
	}

	/* Next, check threads that are looking for work */
	{
		WorkerThreadArray LookingForWork = NotFull.FilterByPredicate([](WorkerThreadPtr Thread) { return Thread->IsWaitingForWork(); });
		LookingForWork.Shrink();
		if (LookingForWork.Num() > 0)
		{
			int32 IndexOfThread = LookingForWork.IndexOfByPredicate(AffinityPred);
			IndexOfThread = IndexOfThread == INDEX_NONE ? 0 : IndexOfThread;

			WorkerThreadPtr ThreadToUse = LookingForWork[IndexOfThread];
			if (ThreadToUse && ThreadToUse->CanAcceptTask(Affinity)) // Make sure that thread can accept the new task, otherwise we'll keep looking.
			{
				return ThreadToUse;
			}
		}
	}

	/* See if there are any threads that will accept the task */
	{
		WorkerThreadArray CanAcceptTask = Threads.FilterByPredicate(CanAddPred);
		CanAcceptTask.Shrink();
		if (CanAcceptTask.Num() > 0)
		{
			WorkerThreadArray WithOurAffinity = CanAcceptTask.FilterByPredicate(AffinityPred);
			if (WithOurAffinity.Num() > 0)
			{
				return WithOurAffinity[0];
			}
		}
	}

	/* So far no threads are able to take the task, try to add a new one instead. */
	if (CanAddThread())
	{
		return AddThread(Affinity);
	}

	/* If all else fails, find the thread with the least number of tasks (and our affinity if possible, otherwise try to find a thread with no affinity) */
	WorkerThreadArray PreferredThreads = GetThreadsWithAffinity(Affinity);
	WorkerThreadArray ThreadsWithNoAffinity = GetThreadsWithAffinity();
	WorkerThreadArray ArrayToSearch = PreferredThreads.IsEmpty() ? ThreadsWithNoAffinity : PreferredThreads;
	ArrayToSearch = ArrayToSearch.IsEmpty() ? Threads : ArrayToSearch;
	TB_RET_COND_VALUE(ArrayToSearch.IsEmpty(), nullptr);
	return ArrayToSearch[0];
}
#pragma endregion


#pragma region FTBProjectileWorkerThread
void FTBProjectileWorkerThread::OnShutdown()
{
	LLM_SCOPE_BYTAG(FTBProjectileWorkerThread);
	IsStopping = true;
	TaskCondition.notify_one();
	for (auto& Task : SimTasks)
	{
		if(Task)
		{
			Task->SetIsShuttingDown();
		}
	}
}

bool FTBProjectileWorkerThread::Init()
{
	return true;
}

uint32 FTBProjectileWorkerThread::Run()
{
	LLM_SCOPE_BYTAG(FTBProjectileWorkerThread);
	HasStopped = false;

	while (!IsStopping && !HasStopped)
	{
		{
			std::unique_lock<std::mutex> lock(TaskMutex);
			using namespace std::chrono;
			using namespace TB::SimTasks;
			
			auto SleepDuration = double_seconds(TickRate.GetTotalSeconds());
			if (bFirstUpdate)
			{
				bFirstUpdate = false;
			}
			else
			{
				const auto TimeSpentInUpdate = abs(duration_cast<double_seconds>(TickExit - TickEntry));
				SleepDuration -= TimeSpentInUpdate;
			}
			TaskCondition.wait_for(lock, SleepDuration, [&]() { return (EarlyTick && !InTickLoop) || IsStopping || HasStopped; });
			EarlyTick = false;
		}

		if (IsStopping || HasStopped)
		{
			break;
		}
		if (InTickLoop)
		{
			continue;
		}
		if(!IsWaitingForWork() && !IsPaused)
		{
			TickTasks();
		}
	}

	HasStopped = true;
	return 0;
}

void FTBProjectileWorkerThread::Stop()
{
	IsStopping = true;
	std::lock_guard<std::mutex> lock(TaskMutex);
	TaskCondition.notify_one();
}

void FTBProjectileWorkerThread::KillTasks()
{
	LLM_SCOPE_BYTAG(FTBProjectileWorkerThread);
	for (TaskPtr& Task : SimTasks)
	{
		if(Task)
		{
			Task->Kill(TB::SimTasks::ExitCodes::EARLY_TERMINATION);
			NumActive--;
		}
	}
	SimTasks.Empty();
}

void FTBProjectileWorkerThread::TickTasks()
{
	TGuardValue<bool> TickLoopGuard(InTickLoop, true); // Sets "InTickLoop" to true while we're in this scope and restores it after

	LLM_SCOPE_BYTAG(FTBProjectileWorkerThread);
	TRACE_CPUPROFILER_EVENT_SCOPE(FTBProjectileWorkerThread::TickTasks);
	using namespace std::chrono;
	auto now = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(now - TickExit).count();
	TickEntry = now;

	ProcessPending();

	if (WasPaused) // We were previously paused, if we continue now then it would be as if the pause was just a long tick and projectiles would simulate as if we didn't pause.
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FTBProjectileWorkerThread::TickTasks::TickLoop);
		LLM_SCOPE_BYTAG(FTBProjectileWorkerThread_Tick);

		WasPaused = false;
		TickExit = TickEntry;
		TArrayView<TaskPtr> CurrentTasks = SimTasks;
		for (auto& Task : CurrentTasks)
		{
			Task->SetUpdateTime(now);
			now = high_resolution_clock::now();
			double dt = Task->CalculateDeltaTimeSeconds(now) * Timescale;
			if (dt < 0.0)
			{
				dt = TickRate.GetTotalSeconds() * Timescale;
			}
			Task->Tick(dt);
		}
	}
	else
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FTBProjectileWorkerThread::TickTasks::TickLoop);
		LLM_SCOPE_BYTAG(FTBProjectileWorkerThread_Tick);
		TArrayView<TaskPtr> CurrentTasks = SimTasks;
		for (auto& Task : CurrentTasks)
		{
			if (!Task)
			{
				continue;
			}
			now = high_resolution_clock::now();
			double dt = Task->CalculateDeltaTimeSeconds(now) * Timescale;
			if (dt < 0.0)
			{
				dt = TickRate.GetTotalSeconds() * Timescale;
			}
			Task->Tick(dt);
		}
	}

	TickExit = high_resolution_clock::now();

	if (TickRate.GetTotalSeconds() > 0)
	{
		const double NominalTickRate = TickRate.GetTotalSeconds();
		const double TimeSpentInUpdate = abs(duration_cast<TB::SimTasks::double_seconds>(TickExit - TickEntry)).count();
		if (TimeSpentInUpdate > NominalTickRate)
		{
			TB_LOG(Warning, TEXT("%s: Processing time exhausted. %fms / %fms"), *Thread->GetThreadName(), TimeSpentInUpdate * 1e3, NominalTickRate * 1e3);
			return;
		}
	}

	InTickLoop = false;
}

bool FTBProjectileWorkerThread::KillTask(ProjectileTask* TaskToKill)
{
	LLM_SCOPE_BYTAG(FTBProjectileWorkerThread);
	TRACE_CPUPROFILER_EVENT_SCOPE(FTBProjectileWorkerThread::KillTask);
	if (NumActive > 0)
	{
		TaskPtr* FoundTask = SimTasks.FindByPredicate([TaskToKill](const TaskPtr& Task) {
			if(Task)
			{
				return Task.Get() == TaskToKill;
			}
			return false;
		});
		if (FoundTask && *FoundTask)
		{
			(*FoundTask)->Kill();
			FScopeLock RemovalLock(&OwningPool->TaskRemovalMutex);
			SimTasks.RemoveSingleSwap(*FoundTask);
			NumActive--;
			return true;
		}
	}
	return false;
}

bool FTBProjectileWorkerThread::KillTaskByIdAndAffinity(const FTBProjectileId& TaskId, const ThreadTaskAffinity Affinity)
{
	LLM_SCOPE_BYTAG(FTBProjectileWorkerThread);
	TRACE_CPUPROFILER_EVENT_SCOPE(FTBProjectileWorkerThread::KillTask);
	if (NumActive > 0)
	{
		TaskPtr* FoundTask = SimTasks.FindByPredicate([TaskId, Affinity](const TaskPtr& Task) {
			if(Task)
			{
				return Task->GetId() == TaskId
					&& Task->GetAffinity() == Affinity;
			}
			return false;
		});
		if(FoundTask && *FoundTask)
		{
			(*FoundTask)->Kill();
			FScopeLock RemovalLock(&OwningPool->TaskRemovalMutex);
			SimTasks.RemoveSingleSwap(*FoundTask);
			NumActive--;
			return true;
		}
	}
	return false;
}

void FTBProjectileWorkerThread::RemoveTask(ProjectileTask* Task)
{
	ToRemove.Enqueue(Task);
	TaskCondition.notify_one();
}

void FTBProjectileWorkerThread::AddTask(FTBProjectileThread* ProjectileThread, FTBBulletSimData&& SimData)
{
	LLM_SCOPE_BYTAG(FTBProjectileWorkerThread);
	std::lock_guard<std::mutex> lock(TaskMutex);
	RecieveTask(FTBBulletTaskConstructionParams(ProjectileThread, MoveTemp(SimData)));
	//BulletsToAdd.Enqueue(MakeShareable<FTBBulletTaskConstructionParams>(new FTBBulletTaskConstructionParams(ProjectileThread, MoveTemp(SimData))));
	EarlyTick = true;
	TaskCondition.notify_one();
}

void FTBProjectileWorkerThread::AddTask(FTBProjectileThread* ProjectileThread, FTBProjectileSimData&& SimData)
{
	LLM_SCOPE_BYTAG(FTBProjectileWorkerThread);
	std::lock_guard<std::mutex> lock(TaskMutex);
	RecieveTask(FTBProjectileTaskConstructionParams(ProjectileThread, MoveTemp(SimData)));
	//ProjectilesToAdd.Enqueue(MakeShareable<FTBProjectileTaskConstructionParams>(new FTBProjectileTaskConstructionParams(ProjectileThread, MoveTemp(SimData))));
	EarlyTick = true;
	TaskCondition.notify_one();
}

void FTBProjectileWorkerThread::ProcessPending()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FTBProjectileWorkerThread::TickTasks::ProcessPending);
	LLM_SCOPE_BYTAG(FTBProjectileWorkerThread_ProcessPending);
	ProjectileTask* Task;
	while (ToRemove.Dequeue(Task))
	{
		const int32 Index = SimTasks.IndexOfByPredicate([Task](const TaskPtr& ToCheck) { return ToCheck && ToCheck.Get() == Task; });
		if (Index > INDEX_NONE)
		{
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(FTBProjectileWorkerThread::TaskRemoval)
					FScopeLock RemovalLock(&OwningPool->TaskRemovalMutex); // This lock is shared by all worker threads in the thread pool
				FWriteScopeLock SwapLock(TaskCopyLock);
				SimTasks.RemoveAtSwap(Index);
			}
			NumActive--;
			OwningPool->NumActive--;
		}
	}

	{
		TSharedPtr<FTBBulletTaskConstructionParams> ConstructionParams;
		while (BulletsToAdd.Dequeue(ConstructionParams))
		{
			NumActive++;
			OwningPool->NumActive++;
			TaskPtr NewTask;
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(FTBProjectileWorkerThread::TaskAddition)
					FScopeLock AdditionLock(&OwningPool->TaskAdditionMutex);
				NewTask = ConstructionParams->CreateTask();
			}
			NewTask->SetOnCompleteCallback(std::bind(&FTBProjectileWorkerThread::RemoveTask, this, std::placeholders::_1));

			FWriteScopeLock EmplaceLock(TaskCopyLock);
			SimTasks.Emplace(MoveTemp(NewTask));
		}
	}
	{
		TSharedPtr<FTBProjectileTaskConstructionParams> ConstructionParams;
		while (ProjectilesToAdd.Dequeue(ConstructionParams))
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(FTBProjectileWorkerThread::TaskAddition)
				NumActive++;
			OwningPool->NumActive++;
			TaskPtr NewTask;
			{
				FScopeLock AdditionLock(&OwningPool->TaskAdditionMutex);
				NewTask = ConstructionParams->CreateTask();
			}
			NewTask->SetOnCompleteCallback(std::bind(&FTBProjectileWorkerThread::RemoveTask, this, std::placeholders::_1));
			FWriteScopeLock EmplaceLock(TaskCopyLock);
			SimTasks.Emplace(MoveTemp(NewTask));
		}
	}
}

void FTBProjectileWorkerThread::SetPauseState(const bool NewPauseState)
{
	if (IsPaused != NewPauseState)
	{
		IsPaused = NewPauseState;
		if (NewPauseState == false)
		{
			WasPaused = true;
		}
		TaskCondition.notify_one();
	}
}
#pragma endregion
