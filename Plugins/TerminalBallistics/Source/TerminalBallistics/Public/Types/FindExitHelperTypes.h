#pragma once

#include "TerminalBallisticsMacrosAndFunctions.h"
#include "Engine/Engine.h"

namespace TB
{
	struct TERMINALBALLISTICS_API FPath
	{
		FVector PathStart = FVector::ZeroVector;

		FPath() = default;

		explicit FPath(const FVector& Direction, const FVector& PathStart = FVector::ZeroVector)
			: PathStart(PathStart)
			, Direction(Direction.ToOrientationQuat())
			, DirectionAsVector(Direction)
		{}

		explicit FPath(const FQuat& Direction, const FVector& PathStart = FVector::ZeroVector)
			: PathStart(PathStart)
			, Direction(Direction)
			, DirectionAsVector(Direction.GetForwardVector())
		{}

		explicit FPath(const FRotator& Direction, const FVector& PathStart = FVector::ZeroVector)
			: FPath(FQuat(Direction), PathStart)
		{}

		static FPath FromTwoPoints(const FVector& FirstPoint, const FVector& SecondPoint)
		{
			return FPath(FRotationMatrix::MakeFromX(SecondPoint - FirstPoint).ToQuat(), FirstPoint);
		}

		void SetDirection(const FQuat& NewDirection)
		{
			if (Direction.ContainsNaN())
			{
				return;
			}
			Direction = NewDirection;
			DirectionAsVector = Direction.GetForwardVector();
		}

		void SetDirection(const FRotator& NewDirection)
		{
			SetDirection(FQuat(NewDirection));
		}

		FQuat GetDirection() const
		{
			return Direction;
		}

		FVector GetDirectionVector() const
		{
			return DirectionAsVector;
		}

		FVector GetClosestPointOnPath(const FVector& Point) const
		{
			return PathStart + (DirectionAsVector * ((Point - PathStart) | DirectionAsVector));
		}

		FVector GetClosestPointOnPath(const FVector& Point, double& DistanceFromPath) const
		{
			const FVector NewPoint = GetClosestPointOnPath(Point);
			DistanceFromPath = (NewPoint - Point).Size();
			return NewPoint;
		}

		double GetDistanceFromPath(const FVector& Point) const
		{
			return (GetClosestPointOnPath(Point) - Point).Size();
		}

		bool IsPointOnPath(const FVector& Point, double Tolerance = 1e-8) const
		{
			return FMath::IsNearlyZero(GetDistanceFromPath(Point), Tolerance);
		}

		/**
		* Moves a point onto this path.
		* @param Point	The point to be moved.
		* @return		Distance that the point had to be moved to fit on the path.
		*/
		double MoveToPath(FVector& Point) const
		{
			double Dist;
			Point = GetClosestPointOnPath(Point, Dist);
			return Dist;
		}

		/**
		* Moves a hit result onto this path.
		* Only ImpactPoint and Location are moved.
		*/
		void MoveToPath(FHitResult& HitResult) const
		{
			MoveToPath(HitResult.ImpactPoint);
			MoveToPath(HitResult.Location);
		}

		inline void MoveToPath(struct FExitResult& ExitResult) const;

		double GetDistanceAlongPath(const FVector& Point) const
		{
			if (IsPointOnPath(Point))
			{
				return FVector::PointPlaneDist(Point, PathStart, DirectionAsVector);
			}
			else
			{
				return FVector::PointPlaneDist(GetClosestPointOnPath(Point), PathStart, DirectionAsVector);
			}
		}

		/**
		* Determines if a point is further along the path than another point.
		* @param FirstPoint		The first point
		* @param SecondPoint	The second point
		* @return				Whether or not FirstPoint is further along the path than SecondPoint
		*/
		bool IsAfterPoint(const FVector& FirstPoint, const FVector& SecondPoint) const
		{
			return GetDistanceAlongPath(FirstPoint) > GetDistanceAlongPath(SecondPoint);
		}

		/**
		* Returns a point that lies on this path a given distance from PathStart.
		*/
		inline FVector GetPointAlongPath(const double DistanceAlongPath)
		{
			return PathStart + (DirectionAsVector * DistanceAlongPath);
		}

		/**
		* Moves a point along the path by a given amount.
		*/
		inline void NudgePointAlongPath(FVector& Point, const double Distance) const
		{
			Point += Distance * DirectionAsVector;
		}

		inline FVector GetPointNudgedAlongPath(const FVector& Point, const double Distance) const
		{
			FVector RetPoint = Point;
			NudgePointAlongPath(RetPoint, Distance);
			return RetPoint;
		}

	protected:
		FQuat Direction = FQuat(FVector::ForwardVector, 0.0);
		FVector DirectionAsVector = FVector::ForwardVector;
	};
	
	FORCEINLINE bool operator==(const FPath& Lhs, const FPath& Rhs)
	{
		return Lhs.GetDirection() == Rhs.GetDirection();
	}

	struct TERMINALBALLISTICS_API FExitResult
	{
		FVector ImpactPoint = FVector::ZeroVector;
		FVector ExitLocation = FVector::ZeroVector;
		double PenetrationThickness = 0.0;
		UPrimitiveComponent* Component = nullptr;
		FHitResult HitResult = FHitResult(NoInit);
		FHitResult ExitHitResult = FHitResult(NoInit);

		FExitResult() = default;

		FExitResult(const FVector& ImpactPoint, UPrimitiveComponent* Component)
			: ImpactPoint(ImpactPoint)
			, Component(Component)
		{}

		FExitResult(const FVector& ImpactPoint, UPrimitiveComponent* Component, const FHitResult& HitResult)
			: ImpactPoint(ImpactPoint)
			, Component(Component)
			, HitResult(HitResult)
		{}

		FExitResult(const FVector& ImpactPoint, const FVector& ExitLocation, UPrimitiveComponent* Component)
			: ImpactPoint(ImpactPoint)
			, ExitLocation(ExitLocation)
			, PenetrationThickness(FVector::Dist(ImpactPoint, ExitLocation))
			, Component(Component)
		{}

		FExitResult(const FVector& ImpactPoint, const FVector& ExitLocation, UPrimitiveComponent* Component, const FHitResult& HitResult, const FHitResult& ExitHitResult)
			: ImpactPoint(ImpactPoint)
			, ExitLocation(ExitLocation)
			, PenetrationThickness(FVector::Dist(ImpactPoint, ExitLocation))
			, Component(Component)
			, HitResult(HitResult)
			, ExitHitResult(ExitHitResult)
		{}

		FExitResult(const FVector& ImpactPoint, const FVector& ExitLocation, const double PenetrationThickness, UPrimitiveComponent* Component, const FHitResult& HitResult, const FHitResult& ExitHitResult)
			: ImpactPoint(ImpactPoint)
			, ExitLocation(ExitLocation)
			, PenetrationThickness(PenetrationThickness)
			, Component(Component)
			, HitResult(HitResult)
			, ExitHitResult(ExitHitResult)
		{}

		bool HasValidPhysicalMaterial() const;

		void CalcPenetrationThickness()
		{
			PenetrationThickness = FVector::Dist(ImpactPoint, ExitLocation);
		}

		void CalcExitVarsFromExitResult()
		{
			ExitLocation = ExitHitResult.Location;
			PenetrationThickness = FVector::Dist(ImpactPoint, ExitLocation);
		}

		void NudgeExit(const FVector& Direction, const double Amount)
		{
			ExitLocation += Direction * Amount;
			ExitHitResult.Location += Direction * Amount;
			ExitHitResult.ImpactPoint += Direction * Amount;
		}

		void Reset(bool bKeepStartAndComponent = false)
		{
			ExitLocation = FVector::ZeroVector;
			PenetrationThickness = 0.0;
			ExitHitResult.Reset(0.f, false);
			if (!bKeepStartAndComponent)
			{
				HitResult.Reset(0.f, false);
				ImpactPoint = FVector::ZeroVector;
				Component = nullptr;
			}
		}

		bool CanCombine(const FExitResult& Other) const;

		bool Combine(const FExitResult& Other);
	};

	struct TERMINALBALLISTICS_API FExitResults
	{
		FVector PathStart = FVector::ZeroVector;
		FVector PathDirection = FVector::ZeroVector;
		TArray<FExitResult> ExitResults;

		FExitResults() = default;

		explicit FExitResults(const FVector& PathStart)
			: PathStart(PathStart)
		{}

		FExitResults(const FVector& PathStart, const FVector& PathDirection)
			: PathStart(PathStart)
			, PathDirection(PathDirection)
		{}

		inline void AddResult(FExitResult ExitResult, bool MoveToPath = false)
		{
			if (MoveToPath)
			{
				FPath(PathDirection, PathStart).MoveToPath(ExitResult);
			}
			ExitResults.Add(ExitResult);
		}

		inline bool IsEmpty() const
		{
			return ExitResults.Num() == 0;
		}

		inline int32 Num() const
		{
			return ExitResults.Num();
		}

		inline FExitResult GetFirst()
		{
			return ExitResults[0];
		}

		inline const FExitResult GetFirst() const
		{
			return ExitResults[0];
		}

		TArray<UPrimitiveComponent*> GetComponents() const
		{
			if (IsEmpty())
			{
				return {};
			}

			TArray<UPrimitiveComponent*> Arr;
			Arr.Reserve(ExitResults.Num());
			for (FExitResult ExitResult : ExitResults)
			{
				Arr.Add(ExitResult.Component);
			}
			Arr.Shrink();

			return Arr;
		}

		/* Attempts to combine any possibly extraneous ExitResults */
		void TryShrink();

		inline bool IsValid() const
		{
			return PathDirection != FVector::ZeroVector
				&& !IsEmpty();
		}

		inline operator bool() const
		{
			return IsValid();
		}

		inline void Reserve(const int32 Num)
		{
			ExitResults.Reserve(Num);
		}

		inline void Shrink()
		{
			ExitResults.Shrink();
		}

		TB_ARRAY_WRAPPED_STRUCT_ITERATOR_FUNCS(ExitResults)
	};


	FORCEINLINE void FPath::MoveToPath(FExitResult& ExitResult) const
	{
		MoveToPath(ExitResult.HitResult);
		MoveToPath(ExitResult.ExitHitResult);
		ExitResult.ImpactPoint = ExitResult.HitResult.ImpactPoint;
		ExitResult.CalcExitVarsFromExitResult();
	}
};