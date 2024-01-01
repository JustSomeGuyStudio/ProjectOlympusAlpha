// Copyright 2022-2023 Dominik Lips. All Rights Reserved.

#include "GMCPlayerController.h"
#include "GMCPawn.h"
#include "GMCReplicationComponent.h"
#include "WorldTime.h"
#include "GMCAggregator.h"
#include "GMCLog.h"
#include "GMCPlayerController_DBG.h"

namespace GMCCVars
{
#if ALLOW_CONSOLE && !NO_LOGGING

  int32 StatPing = 0;
  FAutoConsoleVariableRef CVarStatPing(
    TEXT("gmc.StatPing"),
    StatPing,
    TEXT("Display the current ping of the local machine to the server. Only applicable on clients. 0: Disable, 1: Enable"),
    ECVF_Default
  );

  int32 LogNetWorldTime = 0;
  FAutoConsoleVariableRef CVarLogNetWorldTime(
    TEXT("gmc.LogNetWorldTime"),
    LogNetWorldTime,
    TEXT("Log the current world time on server and client with a UTC-timestamp. 0: Disable, 1: Enable"),
    ECVF_Default
  );

#endif
}

AGMC_PlayerController::AGMC_PlayerController()
{
  bAllowTickBeforeBeginPlay = false;
  bReplicates = true;
}

void AGMC_PlayerController::BeginPlay()
{
  Super::BeginPlay();

  gmc_ck(PrimaryActorTick.IsTickFunctionRegistered())

  GMCAggregator = AGMC_Aggregator::GetGMCAggregator(this);

  if (IsValid(GMCAggregator))
  {
    GMCAggregator->RegisterController(this);
  }

#if !NO_LOGGING

  // Only check this on the client since it is the most relevant there and the server potentially has many player controllers (so it could spam the log).
  if (IsNetMode(NM_Client))
  {
    TArray<AActor*> Actors{};
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGMC_WorldTimeReplicator::StaticClass(), Actors);
    gmc_ck(Actors.Num() == 1)

    GMC_CLOG(
      Actors.Num() == 0,
      LogGMCReplication,
      GetPawn(),
      Error,
      TEXT("No actor of class %s found in the current world."),
      TO_STR(AGMC_WorldTimeReplicator)
    )

    GMC_CLOG(
      Actors.Num() > 1,
      LogGMCReplication,
      GetPawn(),
      Error,
      TEXT("More than one actor of class %s found in the current world."),
      TO_STR(AGMC_WorldTimeReplicator)
    )
  }

#endif
}

void AGMC_PlayerController::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
  if (GetLocalRole() == ROLE_AutonomousProxy)
  {
    // Update the client world time before input actions are called from the parent tick.
    CL_UpdateWorldTime();
  }
  else if (GetLocalRole() == ROLE_Authority)
  {
    if (SV_AdaptiveDelayPackets.Num() > 0)
    {
      if (UGMC_ReplicationCmp::CheckReliableBuffer(this, SEND_ADAPTIVE_DELAY_OVERFLOW_PROTECTION))
      {
        CL_SendAdaptiveDelayParams(SV_AdaptiveDelayPackets);
      }
      SV_AdaptiveDelayPackets.Reset();
    }
  }

  gmc_ck(SV_AdaptiveDelayPackets.Num() == 0)

  Super::TickActor(DeltaTime, TickType, ThisTickFunction);

  DEBUG_STAT_PING
  DEBUG_LOG_NET_WORLD_TIME
}

void AGMC_PlayerController::PlayerTick(float DeltaTime)
{
  UGMC_ReplicationCmp* ReplicationComponent{nullptr};
  if (const auto& GMCPawn = Cast<AGMC_Pawn>(GetPawn()))
  {
    ReplicationComponent = GMCPawn->GetReplicationComponent();
  }

  if (IsValid(ReplicationComponent))
  {
    CALL_NATIVE_EVENT_CONDITIONAL(ReplicationComponent->bNoBlueprintEvents, ReplicationComponent, PrePlayerTick, DeltaTime);
  }

  Super::PlayerTick(DeltaTime);

  if (IsValid(ReplicationComponent))
  {
    CALL_NATIVE_EVENT_CONDITIONAL(ReplicationComponent->bNoBlueprintEvents, ReplicationComponent, PostPlayerTick, DeltaTime);
  }
}

void AGMC_PlayerController::UpdateCameraManager(float DeltaSeconds)
{
  bDeferredAutonomousProxyCameraManagerUpdate = false;

  if (GetLocalRole() != ROLE_AutonomousProxy)
  {
    Super::UpdateCameraManager(DeltaSeconds);
    return;
  }

  const auto& GMCPawn = Cast<AGMC_Pawn>(GetPawn());
  if (!IsValid(GMCPawn))
  {
    Super::UpdateCameraManager(DeltaSeconds);
    return;
  }

  const auto& ReplicationComponent = Cast<UGMC_ReplicationCmp>(GMCPawn->GetMovementComponent());
  if (!IsValid(ReplicationComponent))
  {
    Super::UpdateCameraManager(DeltaSeconds);
    return;
  }

  if (!ReplicationComponent->ComponentStatus.bOnWorldTickStartExecuted)
  {
    Super::UpdateCameraManager(DeltaSeconds);
    return;
  }

  if (
    (ReplicationComponent->CL_ShouldUseSmoothCorrections() && ReplicationComponent->CL_SmoothCorrection.HasData()) ||
    ReplicationComponent->IsNonPredictedAutonomousProxy()
  )
  {
    bDeferredAutonomousProxyCameraManagerUpdate = true;
    return;
  }

  Super::UpdateCameraManager(DeltaSeconds);
}

void AGMC_PlayerController::CL_ManualUpdateCamera_Implementation(float DeltaTime)
{
  gmc_ck(GetLocalRole() == ROLE_AutonomousProxy)

  const auto& ControlledPawn = GetPawn();
  if (!ControlledPawn)
  {
    return;
  }

  if (const auto& SpringArm = ControlledPawn->FindComponentByClass<USpringArmComponent>())
  {
    // If we have a spring arm component we want to update the arm location before running the camera manager update to get the correct camera view.
    // TickComponent just calls USpringArmComponent::UpdateDesiredArmLocation which is a protected function so using the public tick function the is the easiest
    // way to update the spring arm.
    SpringArm->TickComponent(DeltaTime, ELevelTick::LEVELTICK_All/*not used*/, nullptr/*not used*/);
  }

  // The camera view needs to be updated to the new location and rotation. Usually the camera manager update is called automatically but in this case it should
  // have been deferred for the autonomous proxy (so that we can run it now). The UpdateCameraManager function is overridden to defer the camera manager update
  // so make sure that the correct super implementation is called here.
  gmc_ck(WasAutonomousProxyCameraManagerUpdateDeferred())
  Super::UpdateCameraManager(DeltaTime);
}

bool AGMC_PlayerController::WasAutonomousProxyCameraManagerUpdateDeferred() const
{
  return bDeferredAutonomousProxyCameraManagerUpdate;
}

double AGMC_PlayerController::CL_GetSyncedWorldTimeSeconds() const
{
  gmc_ck(GetLocalRole() < ROLE_Authority)
  return CL_SyncedWorldTime;
}

float AGMC_PlayerController::CL_GetTimeSyncAdjustment() const
{
  gmc_ck(GetLocalRole() == ROLE_AutonomousProxy)

  if (const auto& World = GetWorld())
  {
    // The in/out parameters can be adjusted as desired.
    int32 constexpr MinFrameRate = 30;
    int32 constexpr MaxFrameRate = 120;
    float constexpr MinAdjustment = 0.1f;
    float constexpr MaxAdjustment = 0.5f;

    // Static asserts for safety.
    static_assert(MinFrameRate > 0);
    static_assert(MaxFrameRate > 0);
    static_assert(MinAdjustment > 0.f);
    static_assert(MaxAdjustment > 0.f);
    static_assert(MinFrameRate != MaxFrameRate);
    static_assert(MinAdjustment != MaxAdjustment);

    // Calculate the adjustment for this frame.
    float constexpr MinFrameTime = 1.f / MinFrameRate;
    float constexpr MaxFrameTime = 1.f / MaxFrameRate;
    float constexpr Slope = (MaxAdjustment - MinAdjustment) / (MaxFrameTime - MinFrameTime);
    return FMath::Clamp(MinAdjustment + Slope * (World->DeltaRealTimeSeconds - MinFrameTime), MinAdjustment, MaxAdjustment);
  }

  return 0.f;
}

void AGMC_PlayerController::CL_UpdateWorldTime()
{
  gmc_ck(GetLocalRole() == ROLE_AutonomousProxy)

  const auto& World = GetWorld();
  if (!World)
  {
    return;
  }

  float DeltaTime = World->DeltaRealTimeSeconds;

  if (CL_bDoNotUpdateWorldTime)
  {
    // Do not add to the world time this frame because we already set the updated value from the server.
    CL_bDoNotUpdateWorldTime = false;
  }
  else if (CL_bSlowWorldTime)
  {
    // Only add a fraction of the delta time to bring the client time closer to the server time. This will effectively speed up movement for one frame (which is
    // usually imperceptible with low adjustment values).
    const float Adjustment = CL_GetTimeSyncAdjustment();
    CL_SyncedWorldTime += DeltaTime * (1.f - Adjustment);
    CL_bSlowWorldTime = false;
    GMC_LOG(LogGMCController, GetPawn(), VeryVerbose, TEXT("Client world time was slowed down for this frame (adjustment = %f)."), Adjustment)
  }
  else if (CL_bSpeedUpWorldTime)
  {
    // Add a higher delta time value to bring the client time closer to the server time. This will effectively slow down movement for one frame (which is
    // usually imperceptible with low adjustment values).
    const float Adjustment = CL_GetTimeSyncAdjustment();
    CL_SyncedWorldTime += DeltaTime * (1.f + Adjustment);
    CL_bSpeedUpWorldTime = false;
    GMC_LOG(LogGMCController, GetPawn(), VeryVerbose, TEXT("Client world time was sped up for this frame (adjustment = %f)."), Adjustment)
  }
  else
  {
    CL_SyncedWorldTime += DeltaTime;
  }

  gmc_ck(!CL_bDoNotUpdateWorldTime)
  gmc_ck(!CL_bSlowWorldTime)
  gmc_ck(!CL_bSpeedUpWorldTime)

  GMC_CLOG(
    CL_SyncedWorldTime <= CL_SyncedTimeLastFrame,
    LogGMCController,
    GetPawn(),
    Verbose,
    TEXT("Client has world time inconsistency: Timestamp current frame (%9.3f) <= Timestamp last frame (%9.3f)"),
    CL_SyncedWorldTime,
    CL_SyncedTimeLastFrame
  )

  CL_SyncedTimeLastFrame = CL_SyncedWorldTime;
}

void AGMC_PlayerController::CL_SyncWithServerTime(double LastReplicatedServerWorldTime)
{
  gmc_ck(GetLocalRole() == ROLE_AutonomousProxy)

  // Always reset these in the beginning in case this function is called more than once during one frame.
  CL_bDoNotUpdateWorldTime = false;
  CL_bSlowWorldTime = false;
  CL_bSpeedUpWorldTime = false;

  const auto& World = GetWorld();
  if (!World)
  {
    GMC_LOG(LogGMCController, GetPawn(), Warning, TEXT("Client time could not not be synced with the server."))
    gmc_ckne()
    return;
  }

  // The client ping (round-trip time) in milliseconds.
  float AvgRTTInMs = 0.f;
  if (USE_PLAYER_STATE_PING)
  {
    AvgRTTInMs = GetPingInMilliseconds();
  }
  else if (const auto& Connection = GetNetConnection())
  {
    AvgRTTInMs = Connection->AvgLag * 1000.f;
  }
  GMC_CLOG(AvgRTTInMs <= 0.f, LogGMCController, GetPawn(), Verbose, TEXT("Client ping could not be retrieved."))

  const float EstimatedLatency = FMath::Min(AvgRTTInMs / 1000.f / 2.f, MaxExpectedPing / 2.f);
  const double NewWorldTime = LastReplicatedServerWorldTime + EstimatedLatency;
  const float DeltaTime = World->DeltaRealTimeSeconds;
  const double SignedTimeDiscrepancy = NewWorldTime - (CL_SyncedWorldTime + DeltaTime/*the time will be updated afterwards*/);
  const double TimeDiscrepancy = FMath::Abs(SignedTimeDiscrepancy);
  if (TimeDiscrepancy > MaxClientTimeDifferenceHardLimit)
  {
    // If the client time deviates by more than the set limit at any point we sync the local time with the server time directly.
    GMC_LOG(
      LogGMCController,
      GetPawn(),
      VeryVerbose,
      TEXT("Synced client world time with server, time discrepancy was %f s (max allowed is %f s): ")
      TEXT("new world time = %f s | old world time = %f s | ping = %.0f ms | jitter = %.0f ms"),
      TimeDiscrepancy,
      MaxClientTimeDifferenceHardLimit,
      NewWorldTime,
      CL_SyncedWorldTime,
      AvgRTTInMs,
      GetNetConnection() ? GetNetConnection()->GetAverageJitterInMS() : 0.f
    )
    CL_SyncedWorldTime = NewWorldTime;

    // Do not add to the time locally this frame, the received server world time is already the updated time for this frame. The flag is processed and reset in
    // CL_UpdateWorldTime.
    CL_bDoNotUpdateWorldTime = true;

    // Clear the buffer after a hard re-sync of the client time.
    CL_TimeDiscrepancyBuffer.Reset();

    GMC_LOG(
      LogGMCController,
      GetPawn(),
      Verbose,
      TEXT("Corrected client time discrepancy of %f seconds."),
      TimeDiscrepancy
    )
  }
  else
  {
    CL_TimeDiscrepancyBuffer.Add(SignedTimeDiscrepancy);
    const double AvgSignedTimeDiscrepancy = CL_TimeDiscrepancyBuffer.GetMean();
    const double AvgTimeDiscrepancy = FMath::Abs(AvgSignedTimeDiscrepancy);

    GMC_LOG(
      LogGMCController,
      GetPawn(),
      VeryVerbose,
      TEXT("Queried server time, client is %f seconds %s."),
      AvgTimeDiscrepancy,
      AvgSignedTimeDiscrepancy > 0. ? TEXT("behind") : TEXT("ahead")
    )

    // We can never assure more accurate time synchronisation than the current delta time value because the packet may arrive at any point during the processing
    // of a frame.
    constexpr float DeltaTimeTolerance = 0.001f;
    const bool bIsExceedingSoftLimit = AvgTimeDiscrepancy > FMath::Max(DeltaTime + DeltaTimeTolerance, MaxClientTimeDifferenceSoftLimit);
    if (AvgSignedTimeDiscrepancy < 0. && bIsExceedingSoftLimit)
    {
      // The client is ahead of the server, set the flag to slow time down (processed and reset in CL_UpdateWorldTime).
      CL_bSlowWorldTime = true;
    }
    else if (AvgSignedTimeDiscrepancy > 0. && bIsExceedingSoftLimit)
    {
      // The client is lagging behind the server, set the flag to speed up time (processed and reset in CL_UpdateWorldTime).
      CL_bSpeedUpWorldTime = true;
    }
  }
}

float AGMC_PlayerController::GetPingInMilliseconds() const
{
  if (!PlayerState)
  {
    return 0.f;
  }

  return PlayerState->GetPingInMilliseconds();
}

void AGMC_PlayerController::SV_EnqueueAdaptiveDelay(const FGMC_AdaptiveDelayServerPacket& DelayPacket)
{
  gmc_ckc(
    for (const auto& Packet : SV_AdaptiveDelayPackets)
    {
      // New delay parameters should only be enqueued once per tick from each replication component.
      gmc_ck(Packet.TargetComponent != DelayPacket.TargetComponent)
    }
  )

  SV_AdaptiveDelayPackets.Emplace(DelayPacket);
}

void AGMC_PlayerController::CL_SendAdaptiveDelayParams_Implementation(const TArray<FGMC_AdaptiveDelayServerPacket>& AdaptiveDelayParams) const
{
  for (const auto& Params : AdaptiveDelayParams)
  {
    // The target component may not exist on the client yet (or it may have been destroyed already).
    if (IsValid(Params.TargetComponent))
    {
      Params.TargetComponent->CL_SendAdaptiveDelayParams(Params);
    }
  }
}

void AGMC_PlayerController::SV_RequestAdaptiveDelayBufferTime_Implementation(const FGMC_AdaptiveDelayClientPacket& NewBufferTime) const
{
  // The target component may have been destroyed already on the server.
  if (IsValid(NewBufferTime.TargetComponent))
  {
    NewBufferTime.TargetComponent->SV_UpdateAdaptiveDelayBufferTime(const_cast<AGMC_PlayerController*>(this), NewBufferTime.DynamicBufferTime);
  }
}

bool AGMC_PlayerController::SV_RequestAdaptiveDelayBufferTime_Validate(const FGMC_AdaptiveDelayClientPacket& NewBufferTime) const
{
  // The buffer time should have been clamped during net deserialization.
  gmc_ck(NewBufferTime.DynamicBufferTime >= FGMC_AdaptiveDelayPersistentParams::MIN_BUFFER_TIME - UE_KINDA_SMALL_NUMBER)
  gmc_ck(NewBufferTime.DynamicBufferTime <= FGMC_AdaptiveDelayPersistentParams::MAX_BUFFER_TIME + UE_KINDA_SMALL_NUMBER)
  return true;
}
