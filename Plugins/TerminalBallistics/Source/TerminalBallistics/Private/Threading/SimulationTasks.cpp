// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "Threading/SimulationTasks.h"
#include "BallisticFunctions.h"
#include "PhysicsUtils.h"
#include "TBFlyByInterface.h"
#include "TerminalBallisticsConfiguration.h"
#include "TerminalBallisticsMacrosAndFunctions.h"
#include "TerminalBallisticsStatics.h"
#include "TerminalBallisticsTags.h"



namespace TB::SimTasks
{
#pragma region BulletSimulationTask
	BulletSimulationTask::BulletSimulationTask(FTBBaseProjectileThread* InController, FTBBulletSimData& InSimData)
		: Super(InController, InSimData)
	{
		LLM_SCOPE_BYTAG(TProjectileSimulationTask);
		Controller = InController;
		Results = TTBBaseProjectileTaskResult<ImpactStruct>(InSimData.StartVelocity, InSimData.StartLocation, InSimData.bDrawDebugTrace, InSimData.bPrintDebugInfo, InSimData.GetId());
		/*Results.ProjectileId = InSimData.GetId();
		Results.bDrawDebugTrace = InSimData.bDrawDebugTrace;*/
		PopulateSimData(InSimData);
		Launch();
	}

	BulletSimulationTask::~BulletSimulationTask() noexcept
	{
		LLM_SCOPE_BYTAG(TProjectileSimulationTask);
		Bullet.Reset();
	}

	void BulletSimulationTask::PopulateSimData(FTBBulletSimData& SimData)
	{
		LLM_SCOPE_BYTAG(TProjectileSimulationTask);
		Delegates = DelegateStruct(SimData.onComplete, SimData.onBulletHit, SimData.onBulletExitHit, SimData.onBulletInjure, SimData.OnUpdateDelegate);
		TProjectileSimulationTask<UTerminalBallisticsSubsystem, FTBBulletSimData, FTBBulletPhysicalProperties, FBulletTaskDelegates>::PopulateSimData(SimData);
		ProjectileProperties = SimData.Bullet->BulletProperties;
		Bullet = SimData.Bullet;
		if (!Bullet)
		{
			Kill(ExitCodes::INVALID_PROJECTILE);
			return;
		}
		SetupSimulationVariables(SimData.PredictParams);
	}

	LLM_DEFINE_TAG(TProjectileSimulationTask_BulletSimulationTask_ConsumeHit);
	ConsumeHitReturnCode BulletSimulationTask::ConsumeHit(const FExitResult& ExitHit)
	{
		LLM_SCOPE_BYTAG(TProjectileSimulationTask_BulletSimulationTask_ConsumeHit);
		TRACE_CPUPROFILER_EVENT_SCOPE(TBulletSimulationTask::ConsumeHit);
		FPendingTaskGuard Guard(&PendingTaskSynch);
		TB_RET_COND_VALUE(!ExitHit.Component || !ExitHit.ExitHitResult.GetComponent(), ConsumeHitReturnCode::Invalid);
		using namespace Tags;
		using namespace PhysMatConstants;
		using namespace MathUtils;

		auto HasTag = [&ExitHit](const FName& Tag)
		{
			const bool bActorHasTag = TB_VALID(ExitHit.HitResult.GetActor()) && ExitHit.HitResult.GetActor()->ActorHasTag(Tag);
			const bool bComponentHasTag = TB_VALID(ExitHit.HitResult.GetComponent()) && ExitHit.HitResult.GetComponent()->ComponentHasTag(Tag);
			return bActorHasTag || bComponentHasTag;
		};

		if (HasTag(PlainTag_Ignore))
		{
			SetLocation(ExitHit.ExitLocation);
			return ConsumeHitReturnCode::Ignore;
		}

		FHitResult hitResult = ExitHit.HitResult;
		FVector impactVelocity = GetVelocity();
		const FVector ImpactVelocityMS = impactVelocity / 100.0;
		TEnumAsByte<EPhysicalSurface> surfaceType;
		FVector exitVelocity = impactVelocity;
		double objectThickness = ExitHit.PenetrationThickness; // cm
		bool isZero = ExitHit.PenetrationThickness == 0;
		bool isHitZone = false;
		bool isBone = false;
		bool isDead = false;
		bool isFlesh = false;

		if (impactVelocity.Size() <= 0)
		{
			SetLocation(hitResult.Location);
			Kill(ExitCodes::ZERO_VELOCITY);
			return ConsumeHitReturnCode::Kill;
		}

		if (HitResultsAreEqualStrict(PreviousHitResult, hitResult))
		{
			SetLocation(hitResult.Location);
			Kill(ExitCodes::EARLY_TERMINATION);
			return ConsumeHitReturnCode::Kill;
		}

		AddPoint(ExitHit.ImpactPoint, impactVelocity, CurrentTime);
		LatestHitResult = ExitHit.HitResult;


		const bool TooThin = objectThickness < TB::Configuration::MinPenetrationDepth;

		if (HitResultsAreEqualStrict(PreviousHitResult, hitResult, false, true, false, false, true) && hitResult.FaceIndex != -1 && PreviousHitResult.GetComponent()) // Same component, same face.
		{
			SetLocation(ExitHit.ExitLocation);
			return ConsumeHitReturnCode::Invalid;
		}

		const FPhysMatProperties SurfaceProperties = GetSurfaceProperties(hitResult, &isHitZone, &isBone, &isDead, &isFlesh);
		surfaceType = SurfaceProperties.SurfaceType;

		bool IsFlesh = TB::PhysMatHelpers::IsFlesh(SurfaceProperties);
		isHitZone |= IsFlesh;

		FVector impactLocation = ExitHit.ImpactPoint;

		PreviousHitLocation = impactLocation;

		PreviousHitResult = hitResult;

		const bool IsTooThin = objectThickness < TB::Configuration::MinPenetrationDepth;
		if (IsTooThin)
		{
			FTBImpactParams ImpactParams = FTBImpactParams(hitResult, Bullet.Get(), impactVelocity, true, surfaceType, StartLocation, objectThickness, 0.0, false, Id);
			Results.Add(ImpactParams);
			BroadcastHitDelegate(ImpactParams);
			ImpactParams.HitResult = ExitHit.ExitHitResult;
			Results.Add(ImpactParams, true);
			BroadcastExitHitDelegate(ImpactParams);
			SetLocation(ExitHit.HitResult.Location);
			return ConsumeHitReturnCode::Invalid;
		}

		/* Ricochet */
		bool RicochetOccured = false;
		FVector RicochetVector;
		double Angle;
		double dE = 0.0;
		if ((!isHitZone || isBone) && BallisticFunctions::ShouldRicochet(hitResult, *Bullet.Get(), ImpactVelocityMS, SurfaceProperties, Bullet->PhysicalProperties, objectThickness, 1.5 * ProjectileProperties.GetFrontalCSA(), Angle, dE, RicochetVector, true, Results.bDrawDebugTrace, Results.bPrintDebugInfo))
		{
			RicochetOccured = true;
			RicochetVector *= 100.0; // m/s to cm/s

			const FVector PreviousLoc = Results.PathData.Last().Location;

			FTBImpactParams impactParams = FTBImpactParams(hitResult, Bullet.Get(), impactVelocity, false, surfaceType, StartLocation, true, Id, RicochetVector);
			const double oldSpeed = GetVelocity().Size();
			SetVelocity(RicochetVector);
			SetLocation(PreviousLoc);
			impactParams.dV = oldSpeed - GetVelocity().Size();
			impactParams.PenetrationDepth = GetProjectileRadius() / 2.0;
			Results.Add(impactParams);
			if (isHitZone)
			{
				const double ImpartedEnergy = TB::CalculateKineticEnergy(Bullet->BulletProperties.Mass, impactParams.dV);
				const FTBWoundCavity Wound = TB::BallisticFunctions::CalculateCavitationRadii(impactVelocity.Size() / 100.0, ImpartedEnergy, impactParams.PenetrationDepth, objectThickness, ProjectileProperties, Bullet->PhysicalProperties, SurfaceProperties);
				const FTBProjectileInjuryParams Injury = FTBProjectileInjuryParams(Wound, impactParams.PenetrationDepth, ImpartedEnergy, impactVelocity, impactLocation, ExitHit.ExitLocation, hitResult, Instigator.Get(), Owner.Get());
				Results.Add(impactParams, Injury);
				BroadcastInjureDelegate(impactParams, Injury);
			}
			else
			{
				BroadcastHitDelegate(impactParams);
			}
		}
		else if (HasTag(PlainTag_IMPENETRABLE))
		{
			FTBImpactParams impactParams = FTBImpactParams(hitResult, Bullet.Get(), impactVelocity, false, surfaceType, StartLocation, false, Id);

			Results.Add(impactParams);
			BroadcastHitDelegate(impactParams);
			SetLocation(hitResult.Location);
			bIsDone = true;
			Kill(ExitCodes::EARLY_TERMINATION);
			return ConsumeHitReturnCode::Kill;
		}
		else if (HasTag(PlainTag_IgnorePenetration))
		{
			FTBImpactParams impactParams = FTBImpactParams(hitResult, Bullet.Get(), impactVelocity, false, surfaceType, StartLocation, false, Id);

			Results.Add(impactParams);
			BroadcastHitDelegate(impactParams);

			impactParams.HitResult = ExitHit.ExitHitResult;
			Results.Add(impactParams, true);
			BroadcastExitHitDelegate(impactParams);

			SetLocation(ExitHit.ExitLocation);
			SetVelocity(exitVelocity);
		}
		else
		{
			FTBImpactParams impactParams = FTBImpactParams(hitResult, Bullet.Get(), impactVelocity, true, surfaceType, StartLocation, false, Id);

			if (objectThickness > TB::Configuration::MinPenetrationDepth && objectThickness > 0)
			{
				double penDepth = 0.0;
				if(SurfaceProperties.bIsFluid)
				{
					const double DragForce = TB::Drag::CalculateDragForce(ProjectileProperties, impactVelocity.Size() / 100.0, 0.0, TB::Constants::FluidDensity_Water) / ProjectileProperties.Mass;
					double ExitSpeed = GetProjectileVelocityInFluid(impactVelocity.Size() / 100.0, DragForce, objectThickness / 100) * 100.0;
					ExitSpeed = ExitSpeed <= 0 ? 0 : ExitSpeed;
					exitVelocity = impactVelocity.GetSafeNormal() * ExitSpeed;
					if (ExitSpeed > 0)
					{
						penDepth = objectThickness;
						dE = CalculateKineticEnergy(ProjectileProperties.Mass, impactVelocity.Size() - ExitSpeed);
					}
					else
					{
						isZero = true;
						penDepth = BallisticFunctions::CalculateDepthOfPenetrationIntoFluid(impactVelocity.Size() / 100, DragForce);
						dE = CalculateKineticEnergy(ProjectileProperties.Mass, impactVelocity.Size());
					}
				}
				else
				{
					exitVelocity = BallisticFunctions::CalculateExitVelocity(hitResult, Bullet, impactVelocity, objectThickness, SurfaceProperties, isZero, dE, penDepth, ProjectileProperties.PenetrationMultiplier, Results.bPrintDebugInfo);
				}
				impactParams = FTBImpactParams(ExitHit.ExitHitResult, Bullet.Get(), impactVelocity, true, surfaceType, StartLocation, penDepth, impactVelocity.Size() - exitVelocity.Size(), false, Id);
				if (isHitZone)
				{
					FCollisionQueryParams params = FCollisionQueryParams(QueryParams);
					const FTBWoundCavity Wound = TB::BallisticFunctions::CalculateCavitationRadii(impactVelocity.Size() / 100.0, dE, impactParams.PenetrationDepth, objectThickness, ProjectileProperties, Bullet->PhysicalProperties, SurfaceProperties);
					const FTBProjectileInjuryParams Injury = FTBProjectileInjuryParams(Wound, impactParams.PenetrationDepth, dE, impactVelocity, impactLocation, ExitHit.ExitLocation, hitResult, Instigator.Get(), Owner.Get());
					Results.Add(impactParams, Injury);
					BroadcastInjureDelegate(impactParams, Injury);
				}
				if (isZero)
				{
					impactParams.HitResult = ExitHit.HitResult;
					Results.Add(impactParams);
					BroadcastHitDelegate(impactParams);
					SetLocation(hitResult.Location);
					bIsDone = true;
					Kill(ExitCodes::ZERO_VELOCITY);
					return ConsumeHitReturnCode::Kill;
				}
			}

			impactParams.HitResult = hitResult;
			Results.Add(impactParams);
			BroadcastHitDelegate(impactParams);

			impactParams.HitResult = ExitHit.ExitHitResult;
			Results.Add(impactParams, true);
			BroadcastExitHitDelegate(impactParams);

			SetLocation(ExitHit.ExitLocation);
			SetVelocity(exitVelocity);
		}
		CallUpdateFunc();
		Iterations++;
		return RicochetOccured ? ConsumeHitReturnCode::Ricochet : ConsumeHitReturnCode::Penetration;
	}

	void BulletSimulationTask::Update(const double dt)
	{
		ProjectileProperties = Bullet->BulletProperties; // Ensure bullet properties are kept current, as they might be changed by impacts.
		Super::Update(dt);
	}

	void BulletSimulationTask::Kill(const ExitCodes::SimTaskExitCode ExitCode)
	{
		LLM_SCOPE_BYTAG(TProjectileSimulationTask);
		Super::Kill(ExitCode);
		Controller->OnBulletTaskExit(MoveTemp(Results));
	}

	void BulletSimulationTask::LogExitCode(const ExitCodes::SimTaskExitCode ExitCode)
	{
		using namespace TB::Configuration;
		EExitCodeLogFilter AsExitCodeLogFilter = static_cast<EExitCodeLogFilter>(ExitCodeLogFilter);
		if (AsExitCodeLogFilter == EExitCodeLogFilter::BulletTasks || AsExitCodeLogFilter == EExitCodeLogFilter::Both)
		{
			Super::LogExitCode(ExitCode);
		}
	}

	void BulletSimulationTask::BroadcastFlyBy(const FVector& Position, const double Distance, AActor* Actor)
	{
		if (bIsShuttingDown)
		{
			return;
		}
		FPendingTaskGuard Guard(&PendingTaskSynch);
		if (bShouldBroadcastFlyByEvents && Actor)
		{
			FTBFlyBy FlyBy = FTBFlyBy(Id, Position, GetVelocity(), Distance, Actor, Bullet->BulletType);
			if (IsValid(FlyBy.Actor))
			{
				FFlyByTask::GraphTask::CreateTask().ConstructAndDispatchWhenReady(FlyBy, &PendingTaskSynch, &bIsShuttingDown);
			}
		}
	}
#pragma endregion

#pragma region ProjectileSimulationTask
	ProjectileSimulationTask::ProjectileSimulationTask(FTBBaseProjectileThread* InController, FTBProjectileSimData& InSimData)
		: Super(InController, InSimData)
	{
		LLM_SCOPE_BYTAG(TProjectileSimulationTask);
		Controller = InController;
		Results = TTBBaseProjectileTaskResult<ImpactStruct>(InSimData.StartVelocity, InSimData.StartLocation, InSimData.bDrawDebugTrace, InSimData.bDrawDebugTrace);
		Results.ProjectileId = InSimData.GetId();
		Results.bDrawDebugTrace = InSimData.bDrawDebugTrace;
		PopulateSimData(InSimData);
		Launch();
	}

	void ProjectileSimulationTask::PopulateSimData(FTBProjectileSimData& SimData)
	{
		LLM_SCOPE_BYTAG(TProjectileSimulationTask);
		Delegates = DelegateStruct(SimData.onComplete, SimData.onProjectileHit, SimData.onProjectileExitHit, SimData.onProjectileInjure, SimData.OnUpdateDelegate);
		Super::PopulateSimData(SimData);
		ProjectilePhysicalProperties = SimData.ProjectilePhysicalProperties;
		SetupSimulationVariables(SimData.PredictParams);
	}

	LLM_DEFINE_TAG(TProjectileSimulationTask_ProjectileSimulationTask_ConsumeHit);
	ConsumeHitReturnCode ProjectileSimulationTask::ConsumeHit(const FExitResult& ExitHit)
	{
		LLM_SCOPE_BYTAG(TProjectileSimulationTask_ProjectileSimulationTask_ConsumeHit);
		TRACE_CPUPROFILER_EVENT_SCOPE(ProjectileSimulationTask::ConsumeHit);
		FPendingTaskGuard Guard(&PendingTaskSynch);
		TB_RET_COND_VALUE(!ExitHit.Component, ConsumeHitReturnCode::Invalid);
		using namespace Tags;
		using namespace PhysMatConstants;
		using namespace MathUtils;

		auto HasTag = [&ExitHit](const FName& Tag)
		{
			const bool bActorHasTag = TB_VALID(ExitHit.HitResult.GetActor()) && ExitHit.HitResult.GetActor()->ActorHasTag(Tag);
			const bool bComponentHasTag = TB_VALID(ExitHit.HitResult.GetComponent()) && ExitHit.HitResult.GetComponent()->ComponentHasTag(Tag);
			return bActorHasTag || bComponentHasTag;
		};

		if (HasTag(PlainTag_Ignore))
		{
			SetLocation(ExitHit.ExitLocation);
			return ConsumeHitReturnCode::Ignore;
		}

		FHitResult hitResult = ExitHit.HitResult;
		FVector impactVelocity = GetVelocity();
		TEnumAsByte<EPhysicalSurface> surfaceType;
		FVector exitVelocity;
		double objectThickness = ExitHit.PenetrationThickness;
		bool isZero = ExitHit.PenetrationThickness == 0;
		bool isHitZone = false;
		bool isBone = false;
		bool isDead = false;

		AddPoint(ExitHit.ImpactPoint, impactVelocity, CurrentTime);
		LatestHitResult = ExitHit.HitResult;

		if (HitResultsAreEqualStrict(PreviousHitResult, hitResult))
		{
			Kill(ExitCodes::EARLY_TERMINATION);
			return ConsumeHitReturnCode::Kill;
		}

		if (HitResultsAreEqualStrict(PreviousHitResult, hitResult, false, true, false, false, true) && hitResult.FaceIndex != -1)
		{ // Same component, same face.
			SetLocation(ExitHit.ExitLocation);
			return ConsumeHitReturnCode::Invalid;
		}

		const FPhysMatProperties SurfaceProperties = GetSurfaceProperties(hitResult, &isHitZone, &isBone, &isDead);

		FVector impactLocation = ExitHit.ImpactPoint;

		PreviousHitLocation = impactLocation;

		PreviousHitResult = hitResult;

		/* Ricochet */
		bool RicochetOccured = false;
		FVector RicochetVector;
		double Angle;
		double dE = 0.0;
		if ((!isHitZone || isBone) && BallisticFunctions::ShouldRicochet(hitResult, ProjectileProperties, impactVelocity / 100.0, SurfaceProperties, ProjectilePhysicalProperties, objectThickness, 1.5 * ProjectileProperties.GetFrontalCSA(), Angle, dE, RicochetVector, true, Results.bDrawDebugTrace, Results.bPrintDebugInfo))
		{
			RicochetOccured = true;
			RicochetVector *= 100.0; // m/s to cm/s

			const FVector PreviousLoc = Results.PathData.Last().Location;

			FTBImpactParamsBasic impactParams = FTBImpactParamsBasic(hitResult, ProjectileProperties, impactVelocity, false, surfaceType, StartLocation, true, Id, 0.0, RicochetVector);
			const double oldSpeed = GetVelocity().Size();
			SetVelocity(RicochetVector);
			SetLocation(PreviousLoc);
			impactParams.dV = oldSpeed - GetVelocity().Size();
			impactParams.PenetrationDepth = GetProjectileRadius() / 2.0;
			Results.Add(impactParams);
			if (isHitZone)
			{
				const double ImpartedEnergy = TB::CalculateKineticEnergy(ProjectileProperties.Mass, impactParams.dV);
				const FTBWoundCavity Wound = TB::BallisticFunctions::CalculateCavitationRadii(impactVelocity.Size() / 100.0, ImpartedEnergy, impactParams.PenetrationDepth, objectThickness, ProjectileProperties, ProjectilePhysicalProperties, SurfaceProperties);
				const FTBProjectileInjuryParams Injury = FTBProjectileInjuryParams(Wound, impactParams.PenetrationDepth, ImpartedEnergy, impactVelocity, impactLocation, ExitHit.ExitLocation, hitResult, Instigator.Get(), Owner.Get());
				Results.Add(impactParams, Injury);
				BroadcastInjureDelegate(impactParams, Injury);
			}
			else
			{
				BroadcastHitDelegate(impactParams);
			}
		}
		else if (HasTag(PlainTag_IMPENETRABLE))
		{
			FTBImpactParamsBasic impactParams = FTBImpactParamsBasic(hitResult, ProjectileProperties, impactVelocity, false, surfaceType, StartLocation, false, Id);

			Results.Add(impactParams);
			BroadcastHitDelegate(impactParams);
			SetLocation(hitResult.Location);
			bIsDone = true;
			Kill(ExitCodes::EARLY_TERMINATION);
			return ConsumeHitReturnCode::Kill;
		}
		else if (HasTag(PlainTag_Ignore))
		{
			FTBImpactParamsBasic impactParams = FTBImpactParamsBasic(hitResult, ProjectileProperties, impactVelocity, false, surfaceType, StartLocation, false, Id);

			Results.Add(impactParams);
			BroadcastHitDelegate(impactParams);

			impactParams.HitResult = ExitHit.ExitHitResult;
			Results.Add(impactParams, true);
			BroadcastExitHitDelegate(impactParams);

			SetLocation(ExitHit.ExitLocation);
			SetVelocity(exitVelocity);
		}
		else
		{
			FTBImpactParamsBasic impactParams = FTBImpactParamsBasic(hitResult, ProjectileProperties, impactVelocity, true, surfaceType, StartLocation, false, Id);

			if (objectThickness > TB::Configuration::MinPenetrationDepth && objectThickness > 0)
			{
				double penDepth = 0.0;
				exitVelocity = UBallisticFunctions::CalculateExitVelocityForProjectile(World, hitResult, ProjectileProperties, impactVelocity, objectThickness, surfaceType, ProjectilePhysicalProperties, isZero, dE, penDepth, 1.0, Results.bPrintDebugInfo); //Determine exit velocity of the bullet, if it is able to penetrate.
				impactParams = FTBImpactParamsBasic(ExitHit.ExitHitResult, ProjectileProperties, impactVelocity, true, surfaceType, StartLocation, penDepth, impactVelocity.Size() - exitVelocity.Size(), false, Id);
				if (isHitZone)
				{
					const FTBWoundCavity Wound = TB::BallisticFunctions::CalculateCavitationRadii(impactVelocity.Size() / 100.0, dE, impactParams.PenetrationDepth, objectThickness, ProjectileProperties, ProjectilePhysicalProperties, SurfaceProperties);
					const FTBProjectileInjuryParams Injury = FTBProjectileInjuryParams(Wound, impactParams.PenetrationDepth, dE, impactVelocity, impactLocation, ExitHit.ExitLocation, hitResult, Instigator.Get(), Owner.Get());
					Results.Add(impactParams, Injury);
					BroadcastInjureDelegate(impactParams, Injury);
				}
				if (isZero) // Projectile stopped. We're done.
				{
					Results.Add(impactParams);
					BroadcastHitDelegate(impactParams);
					bIsDone = true;
					Kill(ExitCodes::ZERO_VELOCITY);
					return ConsumeHitReturnCode::Kill;
				}
			}
			impactParams.HitResult = hitResult;
			Results.Add(impactParams);
			BroadcastHitDelegate(impactParams);

			impactParams.HitResult = ExitHit.ExitHitResult;
			Results.Add(impactParams, true);
			BroadcastExitHitDelegate(impactParams);

			SetLocation(ExitHit.ExitLocation);
			SetVelocity(exitVelocity);
		}
		CallUpdateFunc();
		Iterations++;
		return RicochetOccured ? ConsumeHitReturnCode::Ricochet : ConsumeHitReturnCode::Penetration;
	}

	void ProjectileSimulationTask::Kill(const ExitCodes::SimTaskExitCode ExitCode)
	{
		LLM_SCOPE_BYTAG(TProjectileSimulationTask);
		Super::Kill(ExitCode);
		Controller->OnProjectileTaskExit(MoveTemp(Results));
	}

	void ProjectileSimulationTask::LogExitCode(const ExitCodes::SimTaskExitCode ExitCode)
	{
		using namespace TB::Configuration;
		EExitCodeLogFilter AsExitCodeLogFilter = static_cast<EExitCodeLogFilter>(ExitCodeLogFilter);
		if (AsExitCodeLogFilter == EExitCodeLogFilter::ProjectileTasks || AsExitCodeLogFilter == EExitCodeLogFilter::Both)
		{
			Super::LogExitCode(ExitCode);
		}
	}

	void ProjectileSimulationTask::BroadcastFlyBy(const FVector& Position, const double Distance, AActor* Actor)
	{
		if (bIsShuttingDown)
		{
			return;
		}
		if (bShouldBroadcastFlyByEvents && Actor)
		{
			FTBFlyBy FlyBy = FTBFlyBy(Id, Position, GetVelocity(), Distance, Actor, ProjectileProperties.ProjectileSize);
			if (FlyBy.Actor && FlyBy.Actor->Implements<UTBFlyByInterface>())
			{
				FFlyByTask::GraphTask::CreateTask().ConstructAndDispatchWhenReady(FlyBy, &PendingTaskSynch, &bIsShuttingDown);
			}
		}
	}
#pragma endregion
};
