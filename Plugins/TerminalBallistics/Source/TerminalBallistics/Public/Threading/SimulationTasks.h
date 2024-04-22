// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "TerminalBallisticsThreadingTypes.h"
#include "Subsystems/TerminalBallisticsSubsystem.h"

namespace TB::SimTasks
{
	LLM_DECLARE_TAG(TProjectileSimulationTask_BulletSimulationTask_ConsumeHit);
	LLM_DECLARE_TAG(TProjectileSimulationTask_ProjectileSimulationTask_ConsumeHit);

	class BulletSimulationTask : public TProjectileSimulationTask<UTerminalBallisticsSubsystem, FTBBulletSimData, FTBBulletPhysicalProperties, FBulletTaskDelegates>
	{
	public:
		typedef TProjectileSimulationTask<UTerminalBallisticsSubsystem, FTBBulletSimData, FTBBulletPhysicalProperties, FBulletTaskDelegates> Super;

		BulletSimulationTask(FTBBaseProjectileThread* InController, FTBBulletSimData& InSimData);

		virtual ~BulletSimulationTask() noexcept override;

		virtual void PopulateSimData(FTBBulletSimData& SimData) override;

		virtual ConsumeHitReturnCode ConsumeHit(const FExitResult& ExitHit) override;

		virtual void Update(const double dt) override;

		void Kill(const ExitCodes::SimTaskExitCode ExitCode = ExitCodes::OTHER) override;

		void LogExitCode(const ExitCodes::SimTaskExitCode ExitCode) override;

		virtual ETBThreadTaskAffinity GetAffinity() const override
		{
			return ETBThreadTaskAffinity::BULLET_TASK;
		}

		inline virtual double GetProjectileRadius() const override { return fmax(Bullet->BulletProperties.Radius, Bullet->BulletProperties.ApparentRadius); }

		virtual FTBBullet GetProjectile() const override
		{
			return *Bullet;
		}

		virtual const FPhysMatProperties& GetProjectilePhysMatProperties() const override
		{
			return Bullet->PhysicalProperties;
		}

	protected:
		BulletPointer Bullet;

		// Inherited via TProjectileSimulationTask
		virtual void BroadcastFlyBy(const FVector& Position, const double Distance, AActor* Actor) override;
	};



	class ProjectileSimulationTask : public TProjectileSimulationTask<UTerminalBallisticsSubsystem, FTBProjectileSimData, FTBProjectile, FProjectileTaskDelegates>
	{
	public:
		typedef TProjectileSimulationTask<UTerminalBallisticsSubsystem, FTBProjectileSimData, FTBProjectile, FProjectileTaskDelegates> Super;

		ProjectileSimulationTask(FTBBaseProjectileThread* InController, FTBProjectileSimData& InSimData);

		virtual void PopulateSimData(FTBProjectileSimData& SimData) override;

		virtual ConsumeHitReturnCode ConsumeHit(const FExitResult& ExitHit) override;

		virtual void Kill(const ExitCodes::SimTaskExitCode ExitCode = ExitCodes::OTHER) override;

		virtual void LogExitCode(const ExitCodes::SimTaskExitCode ExitCode) override;

		virtual ETBThreadTaskAffinity GetAffinity() const override
		{
			return ETBThreadTaskAffinity::PROJECTILE_TASK;
		}

		virtual FTBProjectile GetProjectile() const override
		{
			return ProjectileProperties;
		}

		virtual const FPhysMatProperties& GetProjectilePhysMatProperties() const override
		{
			return ProjectilePhysicalProperties;
		}

	protected:
		FPhysMatProperties ProjectilePhysicalProperties;

		// Inherited via TProjectileSimulationTask
		virtual void BroadcastFlyBy(const FVector& Position, const double Distance, AActor* Actor) override;
	};
};
