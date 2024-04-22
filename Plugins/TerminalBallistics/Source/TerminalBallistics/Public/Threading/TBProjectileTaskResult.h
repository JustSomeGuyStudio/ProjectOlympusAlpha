// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Templates/Tuple.h"
#include "TerminalBallisticsTraits.h"
#include "Types/TBProjectileId.h"
#include "Types/TBProjectileInjury.h"
#include "UObject/ObjectPtr.h"



namespace TB::SimTasks::ExitCodes
{
	enum SimTaskExitCode : uint16
	{
		OTHER =					1 << 0,
		TIME_LIMIT =			1 << 1,
		ZERO_VELOCITY =			1 << 2,
		ITERATION_LIMIT =		1 << 3,

		FailureLevel =			1 << 4, // Exit Codes beyond this point are considered failures

		NO_EXIT =				1 << 5,
		INVALID_PROJECTILE =	1 << 6,
		EARLY_TERMINATION =		1 << 7,
		INVALID_DURATION =		1 << 8,

		BadData = INVALID_PROJECTILE | INVALID_DURATION
	};

	inline static bool IsExitCodeFailure(const SimTaskExitCode ExitCode)
	{
		return ExitCode < SimTaskExitCode::FailureLevel;
	}

	inline static bool IsExitCodeFailure(const int ExitCode)
	{
		return static_cast<SimTaskExitCode>(ExitCode) < SimTaskExitCode::FailureLevel;
	}

	inline static FString GetExitCodeName(const SimTaskExitCode ExitCode)
	{
		switch (ExitCode)
		{
		case SimTaskExitCode::TIME_LIMIT:			return FString("TIME_LIMIT");
		case SimTaskExitCode::NO_EXIT:				return FString("NO_EXIT");
		case SimTaskExitCode::ZERO_VELOCITY:		return FString("ZERO_VELOCITY");
		case SimTaskExitCode::INVALID_PROJECTILE:	return FString("INVALID_PROJECTILE");
		case SimTaskExitCode::EARLY_TERMINATION:	return FString("EARLY_TERMINATION");
		case SimTaskExitCode::ITERATION_LIMIT:		return FString("ITERATION_LIMIT");
		case SimTaskExitCode::INVALID_DURATION:		return FString("INVALID_DURATION");
		case SimTaskExitCode::OTHER:				return FString("OTHER");
		default:
			checkNoEntry();
			return FString();
		}
	}

	inline static FString GetExitCodeName(const int ExitCode)
	{
		return GetExitCodeName(static_cast<SimTaskExitCode>(ExitCode));
	}

	inline static void LogExitCode(const SimTaskExitCode ExitCode)
	{
		if ((ExitCode & BadData) != 0U)
		{
			TB_LOG_STR(Warning, GetExitCodeName(ExitCode));
		}
		else if (ExitCode > FailureLevel)
		{
			TB_LOG_STR(Error, GetExitCodeName(ExitCode));
		}
		else
		{
			TB_LOG_STR(Verbose, GetExitCodeName(ExitCode));
		}
	}
}

template<typename ImpactStruct>
struct TTBBaseProjectileTaskResult
{
public:
	static_assert(TB::Traits::TIsImpactStruct<ImpactStruct>::Value, "Invalid ImpactStruct provided.");

	typedef TTuple<ImpactStruct, FTBProjectileInjuryParams> InjuryValue;

	FTBProjectileId ProjectileId = FTBProjectileId::None;
	bool bDrawDebugTrace : 1;
	bool bPrintDebugInfo : 1;
	TB::SimTasks::ExitCodes::SimTaskExitCode ExitCode = TB::SimTasks::ExitCodes::OTHER;

	TArray<FPredictProjectilePathPointData> PathData;

	TArray<ImpactStruct> Hits;
	TArray<ImpactStruct> ExitHits;
	TArray<InjuryValue> Injuries;

	TObjectPtr<AActor> Instigator;

	FVector InitialVelocity;
	FVector StartLocation;

public:
	TTBBaseProjectileTaskResult()
		: bDrawDebugTrace(false)
		, bPrintDebugInfo(false)
	{}
	TTBBaseProjectileTaskResult(const FVector& InitialVelocity, const FVector& StartLocation, const bool bDrawDebugTrace, const bool bPrintDebugInfo, const FTBProjectileId& ProjectileId = FTBProjectileId::None)
		: ProjectileId(ProjectileId)
		, bDrawDebugTrace(bDrawDebugTrace)
		, bPrintDebugInfo(bPrintDebugInfo)
		, InitialVelocity(InitialVelocity)
		, StartLocation(StartLocation)
	{}

	~TTBBaseProjectileTaskResult()
	{
		Empty();
	}

	void Empty()
	{
		PathData.Empty();
		Hits.Empty();
		ExitHits.Empty();
		Injuries.Empty();
	}

	void Reserve(const int32 Num)
	{
		PathData.Reserve(Num);
		Hits.Reserve(Num);
		ExitHits.Reserve(Num);
		Injuries.Reserve(Num);
	}

	void Add(const FPredictProjectilePathResult& PathResult)
	{
		PathData.Append(PathResult.PathData);
	}
	void Add(const FPredictProjectilePathPointData& PointData)
	{
		PathData.Emplace(PointData);
	}
	void Add(const ImpactStruct& Hit, const bool bWasExitHit = false)
	{
		if (bWasExitHit)
		{
			ExitHits.Emplace(Hit);
		}
		else
		{
			Hits.Emplace(Hit);
		}
	}
	void Add(const ImpactStruct Hit, const FTBProjectileInjuryParams Injury)
	{
		Injuries.Emplace(InjuryValue(Hit, Injury));
	}

	void AddPoint(FVector Location, FVector Velocity, float Time)
	{
		PathData.Emplace(FPredictProjectilePathPointData(Location, Velocity, Time));
	}

	TTBBaseProjectileTaskResult SortHitsByActor(const TObjectPtr<AActor> Actor)
	{
		TTBBaseProjectileTaskResult ToReturn;
		auto Pred = [Actor](const ImpactStruct& Impact) { return Impact.HitResult.GetActor() == Actor; };
		ToReturn.Hits = Hits.FilterByPredicate(Pred);
		ToReturn.ExitHits = ExitHits.FilterByPredicate(Pred);
		ToReturn.Injuries = Injuries.FilterByPredicate([Actor](const FTBProjectileInjuryParams& Injury) { return Injury.HitActor == Actor; });
		return ToReturn;
	}
	TTBBaseProjectileTaskResult SortHitsByComponent(const TObjectPtr<USceneComponent> Component)
	{
		TTBBaseProjectileTaskResult ToReturn;
		auto Pred = [Component](const ImpactStruct& Impact) { return Impact.HitResult.GetComponent() == Component; };
		ToReturn.Hits = Hits.FilterByPredicate(Pred);
		ToReturn.ExitHits = ExitHits.FilterByPredicate(Pred);
		ToReturn.Injuries = Injuries.FilterByPredicate([Component](const FTBProjectileInjuryParams& Injury) { return Injury.HitComponent == Component; });
		return ToReturn;
	}
};

typedef TTBBaseProjectileTaskResult<struct FTBImpactParams> FBulletTaskResult;
typedef TTBBaseProjectileTaskResult<struct FTBImpactParamsBasic> FProjectileTaskResult;
