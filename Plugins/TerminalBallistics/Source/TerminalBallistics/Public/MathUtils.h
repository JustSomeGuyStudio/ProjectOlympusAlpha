// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TerminalBallisticsTraits.h"
#include "ThirdParty/gcem/gcem.hpp"
#include <random>

#define TB_SQR(X) TB::MathUtils::sqr(X)


namespace TB::MathUtils
{
	inline constexpr float SinD(const float X)
	{
		return gcem::sin(PI / (180) * X);
	}
	inline constexpr double SinD(const double X)
	{
		return gcem::sin(DOUBLE_PI / (180.0) * X);
	}

	inline constexpr float CosD(const float X)
	{
		return gcem::cos(PI / (180) * X);
	}
	inline constexpr double CosD(const double X)
	{
		return gcem::cos(DOUBLE_PI / (180) * X);
	}

	inline constexpr float TanD(const float X)
	{
		return gcem::tan(PI / (180) * X);
	}
	inline constexpr double TanD(const double X)
	{
		return gcem::tan(DOUBLE_PI / (180) * X);
	}

	inline constexpr float AsinD(const float X)
	{
		return 180.f / PI * gcem::asin(X);
	}
	inline constexpr double AsinD(const double X)
	{
		return 180.0 / DOUBLE_PI * gcem::asin(X);
	}

	inline constexpr float AcosD(const float X)
	{
		return 180.f / PI * gcem::acos(X);
	}
	inline constexpr double AcosD(const double X)
	{
		return 180.0 / DOUBLE_PI * gcem::acos(X);
	}

	inline constexpr float AtanD(const float X)
	{
		return 180.f / PI * gcem::atan(X);
	}
	inline constexpr double AtanD(const double X)
	{
		return 180.0 / DOUBLE_PI * gcem::atan(X);
	}

	using ::atan;
	template<typename T, TEMPLATE_REQUIRES(TOr<TB::Traits::TIsFVector<T>, TB::Traits::TIsFVector2D<T>>::Value)>
	inline constexpr auto atan(const T& Vector)
	{
		if constexpr (std::is_same_v<UE::Math::TVector2<typename T::FReal>, T>)
		{
			return T(
				gcem::atan(Vector.X),
				gcem::atan(Vector.Y)
			);
		}
		else
		{
			return T(
				gcem::atan(Vector.X),
				gcem::atan(Vector.Y),
				gcem::atan(Vector.Z)
			);
		}
	}

	template<typename T>
	inline constexpr T sqr(const T X)
	{
		return X * X;
	}

	// FVector implementation of exp
	template<typename FReal = double>
	inline constexpr auto expv(const UE::Math::TVector<FReal>& Vector)
	{
		return UE::Math::TVector<FReal>(
			gcem::exp(Vector.X),
			gcem::exp(Vector.Y),
			gcem::exp(Vector.Z)
		);
	}

	
	template<typename T1, typename T2, typename T3, typename T4, typename T5, TEMPLATE_REQUIRES(TB::Traits::TAllAreArithmetic<T1, T2, T3, T4, T5>::Value)>
	inline static auto MapRangeClamped(const T1 InMin, const T2 InMax, const T3 OutMin, const T4 OutMax, const T5 Value)
	{
		using T = decltype(T1() * T2() * T3() * T4() * T5());
		return FMath::GetMappedRangeValueClamped(TRange<T>(InMin, InMax), TRange<T>(OutMin, OutMax), Value);
	}

	inline constexpr double NormalizeAngleToPlusMinus90DegreeRange(const double AngleDegrees)
	{
		double Normalized = gcem::fmod(AngleDegrees, 180);
		if (Normalized > 90.0)
		{
			Normalized = 90.0 - Normalized;
		}
		return Normalized;
	}

	inline constexpr double NormalizeAngleToPlusMinus90DegreeRangeRadians(const double AngleRadians)
	{
		double Normalized = gcem::fmod(AngleRadians, DOUBLE_PI);
		if (Normalized > DOUBLE_HALF_PI)
		{
			Normalized = DOUBLE_HALF_PI - Normalized;
		}
		return Normalized;
	}

	template<typename T = double, TEMPLATE_REQUIRES(std::is_arithmetic_v<T>)>
	static T GaussianRand(const T Mean = (T)0.5, const T StdDev = (T)(1.0 / 6.0))
	{
		static std::random_device rd{};
		static std::default_random_engine gen{ rd() };
		std::normal_distribution<T> dist{ Mean, StdDev };
		return dist(gen);
	}
	template<typename T = double, TEMPLATE_REQUIRES(std::is_arithmetic_v<T>)>
	static T GaussianSRand(const unsigned int Seed, const T Mean = (T)0.5, const T StdDev = (T)(1.0 / 6.0))
	{
		std::default_random_engine gen{ Seed };
		std::normal_distribution<T> dist{ Mean, StdDev };
		return dist(gen);
	}

	namespace VectorUtils
	{
		/**
		* Returns a random unit vector within the specified cone using a normal distribution.
		* Adapted from the uniform-distributed FMath::VRandCone function.
		*/
		static FVector VRandConeGuassian(const FVector& Direction, double ConeHalfAngleRad)
		{
			if (ConeHalfAngleRad <= 0.0)
			{
				return Direction.GetSafeNormal();
			}

			const double RandU = FMath::Clamp(GaussianRand(), 0.0, 1.0);
			const double RandV = FMath::Clamp(GaussianRand(), 0.0, 1.0);

			// Get spherical coords that have an even distribution over the unit sphere
			// Method described at http://mathworld.wolfram.com/SpherePointPicking.html	
			double Theta = 2.0 * UE_DOUBLE_PI * RandU;
			double Phi = FMath::Acos((2.0 * RandV) - 1.0);

			// restrict phi to [0, ConeHalfAngleRad]
			// this gives an even distribution of points on the surface of the cone
			// centered at the origin, pointing upward (z), with the desired angle
			Phi = FMath::Fmod(Phi, ConeHalfAngleRad);

			// get axes we need to rotate around
			FMatrix const DirMat = FRotationMatrix(Direction.Rotation());
			// note the axis translation, since we want the variation to be around X
			FVector const DirZ = DirMat.GetScaledAxis(EAxis::X);
			FVector const DirY = DirMat.GetScaledAxis(EAxis::Y);

			FVector Result = Direction.RotateAngleAxis(Phi * 180.0 / UE_DOUBLE_PI, DirY);
			Result = Result.RotateAngleAxis(Theta * 180.0 / UE_DOUBLE_PI, DirZ);

			// ensure it's a unit vector (might not have been passed in that way)
			Result = Result.GetSafeNormal();

			return Result;
		}

		/**
		 * This is a version of VRandConeGuassian that handles "squished" cones, i.e. with different angle limits in the Y and Z axes.
		 * Assumes world Y and Z, although this could be extended to handle arbitrary rotations.
		 * Adapted from the uniform-distributed FMath::VRandCone function.
		 */
		static FVector VRandConeGuassian(const FVector& Direction, const double HorizontalConeHalfAngleRad, const double VerticalConeHalfAngleRad)
		{
			if (HorizontalConeHalfAngleRad <= 0.0 || VerticalConeHalfAngleRad <= 0.0)
			{
				return Direction.GetSafeNormal();
			}

			const double RandU = FMath::Clamp(GaussianRand(), 0.0, 1.0);
			const double RandV = FMath::Clamp(GaussianRand(), 0.0, 1.0);

			// Get spherical coords that have an even distribution over the unit sphere
			// Method described at http://mathworld.wolfram.com/SpherePointPicking.html	
			double Theta = 2.0 * UE_DOUBLE_PI * RandU;
			double Phi = FMath::Acos((2.0 * RandV) - 1.0);

			// restrict phi to [0, ConeHalfAngleRad]
			// where ConeHalfAngleRad is now a function of Theta
			// (specifically, radius of an ellipse as a function of angle)
			// function is ellipse function (x/a)^2 + (y/b)^2 = 1, converted to polar coords
			float ConeHalfAngleRad = FMath::Square(FMath::Cos(Theta) / VerticalConeHalfAngleRad) + FMath::Square(FMath::Sin(Theta) / HorizontalConeHalfAngleRad);
			ConeHalfAngleRad = FMath::Sqrt(1.f / ConeHalfAngleRad);

			// clamp to make a cone instead of a sphere
			Phi = FMath::Fmod(Phi, ConeHalfAngleRad);

			// get axes we need to rotate around
			FMatrix const DirMat = FRotationMatrix(Direction.Rotation());
			// note the axis translation, since we want the variation to be around X
			FVector const DirZ = DirMat.GetScaledAxis(EAxis::X);
			FVector const DirY = DirMat.GetScaledAxis(EAxis::Y);

			FVector Result = Direction.RotateAngleAxis(Phi * 180.0 / UE_DOUBLE_PI, DirY);
			Result = Result.RotateAngleAxis(Theta * 180.0 / UE_DOUBLE_PI, DirZ);

			// ensure it's a unit vector (might not have been passed in that way)
			Result = Result.GetSafeNormal();

			return Result;
		}

		template<typename FReal = double>
		inline auto GetClosestPointOnLine(const UE::Math::TVector<FReal>& Point, const UE::Math::TVector<FReal>& LineStart, const UE::Math::TVector<FReal>& LineEnd)
		{
			const UE::Math::TVector<FReal> Direction = (LineEnd - LineStart).GetSafeNormal();
			return LineStart + (Direction * ((Point - LineStart) | Direction));
		}

		template<typename FReal = double>
		inline auto GetClosestPointOnLine(const UE::Math::TVector<FReal>& Point, const UE::Math::TVector<FReal>& LineStart, const UE::Math::TVector<FReal>& LineEnd, FReal& Distance)
		{
			const UE::Math::TVector<FReal> Direction = (LineEnd - LineStart).GetSafeNormal();
			const UE::Math::TVector<FReal> ClosestPoint = LineStart + (Direction * ((Point - LineStart) | Direction));
			Distance = UE::Math::TVector<FReal>::Dist(ClosestPoint, Point);
			return ClosestPoint;
		}

		template<typename FReal = double>
		inline FReal GetDistanceFromLine(const UE::Math::TVector<FReal>& Point, const UE::Math::TVector<FReal>& LineStart, const UE::Math::TVector<FReal>& LineEnd)
		{
			return UE::Math::TVector<FReal>::Dist(Point, GetClosestPointOnLine(Point, LineStart, LineEnd));
		}

		template<typename FReal = double>
		inline auto GetImpactAngle(const UE::Math::TVector<FReal>& SurfaceNormal, const UE::Math::TVector<FReal>& DirectionVector)
		{
			return DOUBLE_HALF_PI - acos(-SurfaceNormal.GetSafeNormal() | DirectionVector.GetSafeNormal());
		}

		template<typename FReal = double>
		inline auto GetImpactAngleAlt(const UE::Math::TVector<FReal>& SurfaceNormal, const UE::Math::TVector<FReal>& DirectionVector)
		{
			return acos(-SurfaceNormal | DirectionVector.GetSafeNormal());
		}

		template<typename FReal = double>
		inline UE::Math::TVector<FReal> ProjectObjectSpaceVectorOntoWorldSpaceDirectionVector(const UE::Math::TVector<FReal>& VectorToProject, const UE::Math::TVector<FReal>& DirectionVector, const UE::Math::TQuat<FReal>& ObjectOrientation)
		{
			using VectorType = UE::Math::TVector<FReal>;

			const VectorType Norm = DirectionVector.GetSafeNormal();

			const VectorType ObjectSpaceAxes[3] = {
				ObjectOrientation.GetAxisX(),
				ObjectOrientation.GetAxisY(),
				ObjectOrientation.GetAxisZ()
			};

			const double DotX = VectorToProject | (ObjectSpaceAxes[0] ^ Norm);
			const double DotY = VectorToProject | (ObjectSpaceAxes[1] ^ Norm);
			const double DotZ = VectorToProject | (ObjectSpaceAxes[2] ^ Norm);


			const VectorType ProjectionLocal = VectorType(DotX, DotY, DotZ);
			const VectorType ProjectionWorld = ObjectOrientation.Inverse().RotateVector(ProjectionLocal);
			return ProjectionWorld;
		}

		template<typename FReal = double>
		constexpr FReal GetDistance2D(const UE::Math::TVector2<FReal>& PointA, const UE::Math::TVector2<FReal>& PointB)
		{
			return sqrt(sqr(PointA.X - PointB.X) + sqr(PointA.Y - PointB.Y));
		}

		template<typename FReal = double>
		inline FReal GetVectorMagnitudeInDirection(const UE::Math::TVector<FReal>& Vector, const UE::Math::TVector<FReal>& DirectionVector)
		{
			using VectorType = UE::Math::TVector<FReal>;

			FReal Magnitude = Vector.Size();

			VectorType DirectionNorm = DirectionVector.GetSafeNormal();

			FReal Dot = Vector | DirectionNorm;

			return Magnitude * Dot;
		}
	};
};
