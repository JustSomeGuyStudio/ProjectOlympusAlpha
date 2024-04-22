// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TBProjectileId.h"
#include "TerminalBallisticsTraits.h"
#include "TBProjectileFlightData.generated.h"



USTRUCT(BlueprintType, DisplayName = "Projectile Flight Data")
/**
* Struct used to keep track of position, velocity, and acceleration of a projectile.
* Values are stored with unreal units for length (cm, cm/s, cm/s^2). Use the appropriate getters/setters when using meters.
*/
struct TERMINALBALLISTICS_API FTBProjectileFlightData
{
	GENERATED_BODY()
	TB_WITH_OPTIMIZED_SERIALIZER();
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Projectile Flight Data")
	FVector Location = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Projectile Flight Data")
	// Velocity (cm/s)
	FVector Velocity = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Projectile Flight Data")
	// Acceleration (cm/s^2)
	FVector Acceleration = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Projectile Flight Data")
	FTBProjectileId Id = FTBProjectileId::None;

	FTBProjectileFlightData() = default;

	FTBProjectileFlightData(const FVector& Location, const FVector& Velocity, const FVector& Acceleration, const FTBProjectileId& Id = FTBProjectileId::None)
		: Location(Location)
		, Velocity(Velocity)
		, Acceleration(Acceleration)
		, Id(Id)
	{}

	virtual ~FTBProjectileFlightData() = default;

	inline void SetLocation(const FVector& NewLocation)
	{
		Location = NewLocation;
	}
	inline void SetLocationMeters(const FVector& NewLocation)
	{
		Location = NewLocation * 100.0;
	}
	inline FVector GetLocationMeters() const
	{
		return Location / 100.0;
	}

	/**
	* Sets new velocity (cm/s)
	* @param NewVelocity	New velocity (cm/s).
	*/
	inline void SetVelocity(const FVector& NewVelocity)
	{
		Velocity = NewVelocity;
	}
	/**
	* Sets new velocity (m/s)
	* @param NewVelocity	New velocity (m/s).
	*/
	inline void SetVelocityMeters(const FVector& NewVelocity)
	{
		Velocity = NewVelocity * 100.0;
	}
	inline FVector GetVelocityMeters() const
	{
		return Velocity / 100.0;
	}

	/**
	* Sets new acceleration (cm/s^2)
	* @param NewAcceleration	New acceleration (cm/s^2).
	*/
	inline void SetAcceleration(const FVector& NewAcceleration)
	{
		Acceleration = NewAcceleration;
	}
	/**
	* Sets new acceleration (m/s^2)
	* @param NewAcceleration	New acceleration (m/s^2).
	*/
	inline void SetAccelerationMeters(const FVector& NewAcceleration)
	{
		Acceleration = NewAcceleration * 100.0;
	}
	inline FVector GetAccelerationMeters() const
	{
		return Acceleration / 100.0;
	}

	inline FVector GetNewAcceleration(const FVector& DragForce, const double Gravity, const double Mass) const
	{
		const FVector& GravityAccel = FVector(0, 0, Gravity);
		const FVector& DragAccel = DragForce / Mass; // a = F/m
		const FVector& TotalAcceleration = -DragAccel + GravityAccel; // m/s^2
		return TotalAcceleration * 100.0; // cm/s^2
	}

	FVector GetNewAcceleration(const FVector& DragForce, const FVector& InitialVelocity, const double Gravity, const double Mass, const double Depth) const;
	
	/**
	* Updates projectile location, velocity, and acceleration.
	* @param dt			DeltaTime
	* @param DragForce	Force due to drag.
	* @param Gravity	Acceleration due to gravity, in the Z direction. (m/s²)
	* @param Mass		Mass of the object being described by this struct. (kg)
	*/
	inline bool Update(const double dt, const FVector& DragForce, const double Gravity, const double Mass, const int32 Depth = 0)
	{
		//UpdateEuler(dt, DragForce, Gravity, Mass);
		return UpdateVerlet(dt, DragForce, Gravity, Mass, Depth);
	}

	// Implements the second-order "Velocity-Verlet" method of numerical integration
	bool UpdateVerlet(const double dt, const FVector& DragForce, const double Gravity, const double Mass, const int32 Depth = 0);

	// Implements the first-order "Semi-Implicit Euler" method of numerical integration
	void UpdateEuler(const double dt, const FVector& DragForce, const double Gravity, const double Mass);

	/* 
		Note: Both of the above methods of integration are computationally trivial at O(N) time complexity and provide low accumulated error with a suffiently small time-step. 
		However, Velocity-Verlet has marginally higher memory complexity and performs more arithmetic operations per step.
		Despite that, Velocity-Verlet is more accurate both in general and across a range of time-steps, while mainly accumulating positional errors.
		The Semi-Implicit Euler method may become unstable at higher time-steps and it accumulates errors in energy over time, but is still suitable in most cases and may provide higher performance at scale.
	*/

	void UpdateInFluid(const double dt, const FVector& DragForce, const FVector& InitialVelocity, const double Gravity, const double Mass, const double Depth);

	FString ToString() const;

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	friend FArchive& operator<<(FArchive& Ar, FTBProjectileFlightData& FlightData);

	bool operator==(const FTBProjectileFlightData& other) const
	{
		return Location.Equals(other.Location, 1e-2)
			&& Velocity.Equals(other.Velocity)
			&& Acceleration.Equals(other.Acceleration)
			&& Id == other.Id;
	}

	bool operator!=(const FTBProjectileFlightData& other) const
	{
		return !(*this == other);
	}
};
template<> struct TIsPODType<FTBProjectileFlightData> { enum { Value = true }; };
template<> struct TStructOpsTypeTraits<FTBProjectileFlightData> : public TStructOpsTypeTraitsBase2<FTBProjectileFlightData>
{
	enum
	{
		WithIdenticalViaEquality = true,
		WithNetSerializer = true,
		WithNetSharedSerialization = true
	};
};
DECLARE_DYNAMIC_DELEGATE_OneParam(FBPOnProjectileUpdate, const FTBProjectileFlightData&, FlightData);

USTRUCT(BlueprintInternalUseOnly)
struct TERMINALBALLISTICS_API FTBProjectileFlightDataSync
{
	GENERATED_BODY()
public:
	double dt = 0.0;
	double Gravity = 0.0;
	double ProjectileMass = 0.0;
	FVector DragForce = FVector::ZeroVector;
	FTBProjectileFlightData PreviousFlightData;
	FTBProjectileFlightData CurrentFlightData;

	FTBProjectileFlightDataSync() = default;

	FTBProjectileFlightDataSync(double dt, double Gravity, double ProjectileMass, const FVector& DragForce, const FTBProjectileFlightData& PreviousFlightData, const FTBProjectileFlightData& CurrentFlightData)
		: dt(dt)
		, Gravity(Gravity)
		, ProjectileMass(ProjectileMass)
		, DragForce(DragForce)
		, PreviousFlightData(PreviousFlightData)
		, CurrentFlightData(CurrentFlightData)
	{}

	bool CheckForValidity(const double PositionalErrorTolerance = 1.0, const double VelocityTolerance = 1.0, const double AccelerationTolerance = 1.0) const;
};

USTRUCT(BlueprintInternalUseOnly)
struct TERMINALBALLISTICS_API FTBSavedUpdate_SimTask
{
	GENERATED_BODY()
public:
	enum class EUpdateType
	{
		Exit,
		Movement,
		Impact
	};

	FTBProjectileId Id;

	FTBProjectileFlightData OldFlightData;
	FTBProjectileFlightData NewFlightData;

	double DeltaTime;
	double CurrentTime;

	double Gravity;
	double ProjectileMass;
	FVector DragForce;

	EUpdateType UpdateType;

public:
	FTBSavedUpdate_SimTask() = default;

	bool IsUpdateValid(const double PositionalErrorTolerance = 1.0, const double VelocityTolerance = 1.0, const double AccelerationTolerance = 1.0) const
	{
		if (UpdateType == EUpdateType::Exit)
		{
			return true;
		}
		else if (DeltaTime <= 0)
		{
			return false;
		}

		return IsValidMovement(PositionalErrorTolerance, VelocityTolerance, AccelerationTolerance);
	}

	bool IsValidMovement(const double PositionalErrorTolerance = 1.0, const double VelocityTolerance = 1.0, const double AccelerationTolerance = 1.0) const
	{
		FTBProjectileFlightData TestData(OldFlightData);
		TestData.Update(DeltaTime, DragForce, Gravity, ProjectileMass);

		const bool ValidPosition = (TestData.Location - NewFlightData.Location).IsNearlyZero(PositionalErrorTolerance);
		const bool ValidVelocity = (TestData.Velocity - NewFlightData.Velocity).IsNearlyZero(VelocityTolerance);
		const bool ValidAcceleration = (TestData.Acceleration - NewFlightData.Acceleration).IsNearlyZero(AccelerationTolerance);

		return ValidPosition
			&& ValidVelocity
			&& ValidAcceleration;
	}

	friend FArchive& operator<<(FArchive& Ar, FTBSavedUpdate_SimTask& Update)
	{
		Ar << Update.Id;
		Ar << Update.OldFlightData;
		Ar << Update.NewFlightData;
		Ar << Update.DeltaTime;
		Ar << Update.CurrentTime;
		Ar << Update.Gravity;
		Ar << Update.ProjectileMass;
		Ar << Update.DragForce;
		Ar << Update.UpdateType;
		return Ar;
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << *this;
		bOutSuccess = true;
		return true;
	}
};
template<> struct TIsPODType<FTBSavedUpdate_SimTask> { enum { Value = true }; };
template<> struct TStructOpsTypeTraits<FTBSavedUpdate_SimTask> : public TStructOpsTypeTraitsBase2<FTBSavedUpdate_SimTask>
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = true
	};
};