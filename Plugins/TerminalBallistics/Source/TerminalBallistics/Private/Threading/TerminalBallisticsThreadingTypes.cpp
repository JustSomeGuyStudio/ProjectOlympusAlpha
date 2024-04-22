// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "Threading/TerminalBallisticsThreadingTypes.h"


namespace TB::SimTasks
{
	LLM_DEFINE_TAG(TProjectileSimulationTask);
	LLM_DEFINE_TAG(TProjectileSimulationTask_Update);
	LLM_DEFINE_TAG(TProjectileSimulationTask_CheckForHits);
	LLM_DEFINE_TAG(TProjectileSimulationTask_ConsumeHits);

	FTBProjectileWorkerParams FTBProjectileWorkerParams::GeneralPurpose = FTBProjectileWorkerParams(ETBThreadTaskAffinity::NONE, false, ETBThreadTaskOverflowType::Restrict, 100);
	FTBProjectileWorkerParams FTBProjectileWorkerParams::Bullets = FTBProjectileWorkerParams(ETBThreadTaskAffinity::BULLET_TASK, true, ETBThreadTaskOverflowType::CreateNew, 100);
	FTBProjectileWorkerParams FTBProjectileWorkerParams::Projectiles = FTBProjectileWorkerParams(ETBThreadTaskAffinity::PROJECTILE_TASK, true, ETBThreadTaskOverflowType::CreateNew, 100);
	
	FLinearColor GetTraceColor(const double CurrentSpeed, const double InitialSpeed)
	{
		if (InitialSpeed > 0.0)
		{
			double p = CurrentSpeed > InitialSpeed ? 1.0 : CurrentSpeed / InitialSpeed;
			double hue = 0.0;
			double saturation = 1.0;
			if (p > 0.75)
			{
				hue = FMath::Lerp(80.0, 0.0, (p - 0.75) * 4.0);
			}
			else if (p > 0.5)
			{
				hue = FMath::Lerp(160.0, 80.0, (p - 0.5) * 4.0);
			}
			else if (p > 0.25)
			{
				hue = FMath::Lerp(240.0, 160.0, (p - 0.25) * 4.0);
			}
			else
			{
				hue = FMath::Lerp(360.0, 240.0, p * 4.0);
				saturation = FMath::Lerp(0.0, 1.0, p * 4.0);
			}
			return FLinearColor(hue, saturation, 1.0).HSVToLinearRGB();
		}
		else
		{
			return FLinearColor();
		}
	}
};
