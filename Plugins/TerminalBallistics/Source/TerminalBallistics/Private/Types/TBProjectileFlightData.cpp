// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "Types/TBProjectileFlightData.h"
#include "MathUtils.h"
#include "Net/Core/Serialization/QuantizedVectorSerialization.h"


FVector FTBProjectileFlightData::GetNewAcceleration(const FVector& DragForce, const FVector& InitialVelocity, const double Gravity, const double Mass, const double Depth) const
{
	const FVector& GravityAccel = FVector(0, 0, Gravity);
	const FVector& DragAccel = (-(InitialVelocity * TB::MathUtils::expv(-DragForce * Depth)) / DragForce) / Mass; // a = F/m
	const FVector& TotalAcceleration = -DragAccel + GravityAccel; // m/s^2
	return TotalAcceleration * 100.0; // cm/s^2
}

bool FTBProjectileFlightData::UpdateVerlet(const double dt, const FVector& DragForce, const double Gravity, const double Mass, const int32 Depth)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FTBProjectileFlightData::UpdateVerlet);

	const FVector NewLocation = Location + Velocity * dt + 0.5 * Acceleration * dt * dt; // x + v*dt + 0.5 * a*dt²
	const FVector NewAcceleration = GetNewAcceleration(DragForce, Gravity, Mass);
	const FVector NewVelocity = Velocity + 0.5 * (NewAcceleration + Acceleration) * dt; // v + 0.5 * (a(t) + a(t + dt)) * dt

	if (NewAcceleration.Size() / Velocity.Size() > 100)
	{
		return false;
	}

	Location = NewLocation;
	Velocity = NewVelocity;
	Acceleration = NewAcceleration;
	return true;
}

void FTBProjectileFlightData::UpdateEuler(const double dt, const FVector& DragForce, const double Gravity, const double Mass)
{
	Acceleration = GetNewAcceleration(DragForce, Gravity, Mass);
	Velocity = Velocity + Acceleration * dt;
	Location = Location + Velocity * dt;
}

void FTBProjectileFlightData::UpdateInFluid(const double dt, const FVector& DragForce, const FVector& InitialVelocity, const double Gravity, const double Mass, const double Depth)
{
	using namespace TB::MathUtils;
	const FVector NewLocation = Location + Velocity * dt + 0.5 * Acceleration * dt * dt; // x + v*dt + 0.5 * a*dt²
	const FVector NewAcceleration = GetNewAcceleration(DragForce, InitialVelocity, Gravity, Mass, Depth);
	const FVector NewVelocity = Velocity + 0.5 * (NewAcceleration + Acceleration) * dt; // v + 0.5 * (a(t) + a(t + dt)) * dt

	Location = NewLocation;
	Velocity = NewVelocity;
	Acceleration = NewAcceleration;
}

FString FTBProjectileFlightData::ToString() const
{
	return FString::Printf(TEXT("Location: %s\nVelocity: %s\nAcceleration: %s"), *Location.ToCompactString(), *Velocity.ToCompactString(), *Acceleration.ToCompactString());
}

bool FTBProjectileFlightData::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	Ar << *this;
	return bOutSuccess;
}

FArchive& operator<<(FArchive& Ar, FTBProjectileFlightData& FlightData)
{
	using namespace UE::Net;
	SerializeQuantizedVector<100>(FlightData.Location, Ar);
	Ar << FlightData.Velocity;
	Ar << FlightData.Acceleration;
	Ar << FlightData.Id;
	return Ar;
}

bool FTBProjectileFlightDataSync::CheckForValidity(const double PositionalErrorTolerance, const double VelocityTolerance, const double AccelerationTolerance) const
{
	FTBProjectileFlightData TestData(PreviousFlightData);
	TestData.Update(dt, DragForce, Gravity, ProjectileMass);

	const bool ValidPosition = (TestData.Location - CurrentFlightData.Location).IsNearlyZero(PositionalErrorTolerance);
	const bool ValidVelocity = (TestData.Velocity - CurrentFlightData.Velocity).IsNearlyZero(VelocityTolerance);
	const bool ValidAcceleration = (TestData.Acceleration - CurrentFlightData.Acceleration).IsNearlyZero(AccelerationTolerance);

	return ValidPosition
		&& ValidVelocity
		&& ValidAcceleration;
}