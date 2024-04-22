// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CollisionPresets.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LatentActions.h"
#include "MathUtils.h"
#include "PhysMatManager/PhysMat.h"
#include "Types/TBEnums.h"
#include "TerminalBallisticsMacrosAndFunctions.h"
#include "TerminalBallisticsTypes.h"
#include "Types/ImpactParams.h"
#include "Types/TBBulletPhysicalProperties.h"
#include "Types/TBProjectile.h"
#include "Types/TBProjectileFlightData.h"
#include "UObject/NoExportTypes.h"

#include "TerminalBallisticsStatics.generated.h"

namespace TB
{
	TERMINALBALLISTICS_API EPhysicalSurface GetSurfaceType(FHitResult& HitResult);

	TERMINALBALLISTICS_API TWeakObjectPtr<UPhysicalMaterial> GetPhysicalMaterialFromHitResult(const FHitResult& HitResult, bool& Success);

	TERMINALBALLISTICS_API FPhysMatProperties GetSurfaceProperties(FHitResult& HitResult, bool* IsHitZone = nullptr, bool* IsBone = nullptr, bool* IsDead = nullptr, bool* IsFlesh = nullptr);

	bool SweepSingle(const UWorld* World, const FCollisionShape& CollisionShape, const FQuat& Rot, FHitResult& OutHit, FVector Start, FVector End, ECollisionChannel TraceChannel, const FCollisionQueryParams& Params, const FCollisionResponseParams& ResponseParams = FCollisionResponseParams::DefaultResponseParam, const FCollisionObjectQueryParams& ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam);

	bool DoesShapeOverlap(const FHitResult& HitResult, const FVector& Location, const FCollisionShape& CollisionShape, const FQuat& Rot = FQuat::Identity);

	bool FindExitFromComponentWithoutBodyInstance(FPath Path, UPrimitiveComponent* ComponentToFindExitFrom, const double Radius, FExitResult& ExitResult, bool bTraceComplex = true, ECollisionChannel TraceChannel = CollisionPresets::ECC_Projectile);

	bool FindExitFromSkeletalMeshComponent(FPath Path, USkeletalMeshComponent* SkeletalMeshComponent, const double Radius, FExitResult& ExitResult, bool bTraceComplex = true, ECollisionChannel TraceChannel = CollisionPresets::ECC_Projectile);

	/**
	* Finds the first point along the given path that does not overlap the given component.
	* Excludes path start.
	*
	* @param PathStart				The start of the path
	* @param PathDirection			The direction of the path.
	* @param ComponentToExitFrom	Which component to find the exit of.
	* @param Radius					Radius of the shphere used to test for collision. If zero, a line sweep will be used instead.
	* @param ExitFound				Whether or not an exit was found.
	* @param Resolution				How precisely to check for exits.
	* @param bTraceComplex			Whether to trace against simple or complex collsion
	* @param bUseSecondaryCollision	For instances where the exit location is overlapping another component, should we check if it responds to the given trace channel before using it?
	* @param SecondaryTraceChannel	The collision channel to check against for secondary collisions.
	* @return						The exit location.
	*/
	FVector FindExitFromComponent(const FVector& PathStart, const FVector& PathDirection, UPrimitiveComponent* ComponentToFindExitFrom, const double Radius, bool& ExitFound, double Resolution = 1.0, bool bTraceComplex = true, bool bUseSecondaryCollision = false, ECollisionChannel SecondaryTraceChannel = CollisionPresets::ECC_Projectile);
	FVector FindExitFromComponent(const FVector& PathStart, const FVector& PathDirection, UPrimitiveComponent* ComponentToFindExitFrom, const double Radius, bool& ExitFound, FHitResult& HitResult, double Resolution = 1.0, bool bTraceComplex = true, bool bUseSecondaryCollision = false, ECollisionChannel SecondaryTraceChannel = CollisionPresets::ECC_Projectile);
	bool FindExitFromComponent(const FVector& PathStart, const FVector& PathDirection, UPrimitiveComponent* ComponentToFindExitFrom, const double Radius, FExitResult& ExitResult, double Resolution = 1.0, bool bTraceComplex = true, bool bUseSecondaryCollision = false, ECollisionChannel SecondaryTraceChannel = CollisionPresets::ECC_Projectile);

	FExitResults GetExitLocation(const UObject* WorldContextObject, const FHitResult& HitResult, const double Radius, bool& bExitFound, bool& bMultipleHits, ECollisionChannel TraceChannel);

	
	namespace DistributePointsOnSphere
	{
		/* Implements the standard Fibonacci Laticce mapping */
		TERMINALBALLISTICS_API TArray<FVector> FibLattice(const double SphereRadius, const int N);

		const TArray<FVector> Unit_FibLattice_250 = FibLattice(1.0, 250);

		/* Implements an "Offset Fibonacci Lattice" to map N points onto a sphere. */
		TERMINALBALLISTICS_API TArray<FVector> OffsetFibLattice(const double SphereRadius, const int N);

		const TArray<FVector> Unit_OffsetFibLattice_250 = OffsetFibLattice(1.0, 250);
	};
};


UCLASS(meta = (ScriptName = "TerminalBallisticsStatics"))
class TERMINALBALLISTICS_API UTerminalBallisticsStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
		
public:

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Utility")
	static TArray<FVector> DistributePointsOnSphere(const double SphereRadius, const int NumPoints);

#pragma region Physics Utils
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Environment")
	/**
	* Returns the altitude at the given location (meters)
	*/
	static double GetAltitude(const UObject* WorldContextObject, const FVector& Location);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Physics|Environment", meta = (WorldContext = "WorldContextObject"))
	/**
	* Returns the temperature at the given location (celsius)
	*/
	static double GetCurrentTemperature(const UObject* WorldContextObject, const FVector& Location);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Physics|Environment", meta = (WorldContext = "WorldContextObject"))
	/**
	* Returns the relative humidity at the given location
	*/
	static double GetRelativeHumidity(const UObject* WorldContextObject, const FVector& Location);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Physics", meta = (DisplayName = "Convert Pressure Units"))
	/**
	* Converts a value from one pressure unit to another
	*/
	static double ConvertToNewPressureUnit(ETBPressureUnit OldUnit, ETBPressureUnit NewUnit, double Pressure);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Physics", meta = (DisplayName = "Get Dynamic Pressure"))
	/**
	* Calcuates the dynamic pressure of an object.
	* @param Density	Density (kg/m^3)
	* @param Speed		Speed (m/s)
	* @return			Dynamic Pressure (Pa)
	*/
	static double CalculateDynamicPressure(const double Density, const double Speed);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Physics", meta = (DisplayName = "Get Kinetic Energy"))
	/*
	* Calcuates the kinetic energy of an object.
	* @param Mass		Mass (kg)
	* @param Speed		Speed (m/s)
	* @return			Kinetic Energy (J)
	*/
	static double CalculateKineticEnergy(const double Mass, const double Speed);

#pragma region Drag
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Physics|Drag", meta = (BlueprintThreadSafe, AdvancedDisplay = "p"))
	/**
	* Calculates the force required to overcome drag
	* Wind speed is not relative. For the wind to oppose the motion of the object, wind speed would have to be negative.
	* @param cd			Coefficient of drag for the object.
	* @param csa		Cross sectional area of the object. (m^2)
	* @param speed		Speed of the object. (m/s)
	* @param windSpeed	Wind speed. (m/s)
	* @param p			Fluid density of the medium the object is passing through. Defaults to air.
	*/
	static double CalculateDragForce(double cd, double csa, double speed, double windSpeed = 0, double p = -1);
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Physics|Drag", meta = (BlueprintThreadSafe, AdvancedDisplay = "p"))
	/**
	* Calculates the force required to overcome drag in 3D with wind
	* Wind velocity is not relative, so a wind velocity of {1, 0, 0} would act as a tailwind for a projectile moving in the +X direction.
	* @param cd				Coefficent of drag for the object on each axis.
	* @param csa			Cross sectional area of the object for each axis. (m^2)
	* @param velocity		Velocity of the object. (m/s)
	* @param windVelocity	Wind velocity. (m/s)
	* @param orientation	Rotation of the object.
	* @param p				Fluid density of the medium the object is passing through. Defaults to air.
	*/
	static FVector CalculateDragForce3D(const FVector& cd, const FVector& csa, const FVector& velocity, const FVector& windVelocity = FVector::ZeroVector, const FRotator& orientation = FRotator::ZeroRotator, double p = -1);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Physics|Drag", meta = (ReturnDisplayName = "Terminal Velocity (m/s)"))
	/**
	* Returns the terminal velocity (m/s) of an object with the given parameters
	* @param Mass		Mass of the object (kg)
	* @param g			Acceleration due to Gravity (m/s)
	* @param c			Coefficent of drag for the object on each axis.
	* @param a			Cross sectional area of the object for each axis. (m^2)
	* @param r			Rotation of the object
	* @param p			Fluid density of the medium the object is passing through. Defaults to air.
	*/
	static double GetTerminalVelocity(double m, double g, FVector c, FVector a, FRotator r, double p = -1);
#pragma endregion Drag

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Physics")
	/**
	* Converts power (W) to work (J) over time
	* @param Power		Power (W)
	* @param dt			Time (sec) over which work occurs.
	* @return			Work (J)
	*/
	static double ConvertPowerToWork(double Power, double dt);

	/**
	* Converts power (W) to work (J) over time in 3 dimensions
	* @param Power		Power (W)
	* @param dt			Time (sec) over which work occurs.
	* @return			Work (J)
	*/
	static FVector ConvertPowerToWork(FVector Power, double dt);


	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Physics")
	/**
	* Calculates change in velocity from given work (J).
	* @param Work			Work (J)
	* @param InitialSpeed	Initial Speed (m/s)
	* @param Mass			Mass (kg)
	* @return				dV (m/s)
	*/
	static double CalculateVelocityChangeFromWork(double Work, double InitialSpeed, double Mass);

	/**
	* Calculates change in velocity from given work (J).
	* @param Work			Work (J)
	* @param InitialSpeed	Initial Velocity (m/s)
	* @param Mass			Mass (kg)
	* @return				dV (m/s)
	*/
	static FVector CalculateVelocityChangeFromWork(FVector Work, FVector InitialVelocity, double Mass);

	/**
	* Calculates change in velocity from given work (J).
	* @param Work			Work (J)
	* @param WorkDirection	Direction in which work was applied
	* @param InitialSpeed	Initial Velocity (m/s)
	* @param Mass			Mass (kg)
	* @return				dV (m/s)
	*/
	static FVector CalculateVelocityChangeFromWork(double Work, FVector WorkDirection, FVector InitialVelocity, double Mass);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Physics")
	/**
	* Calculates change in velocity from given power (W) and speed (m/s)
	* @param Power			Power (W)
	* @param InitialSpeed	Initial Speed (m/s)
	* @param Mass			Mass (kg)
	* @return				dV (m/s)
	*/
	static double CalculateVelocityChangeFromPower(double Power, double InitialSpeed, double Mass, double dt);

	/**
	* Calculates change in velocity from given power (W) and speed (m/s)
	* @param Power			Power (W)
	* @param DragDirection	Direction in which power is applied.
	* @param InitialSpeed	Initial Speed (m/s)
	* @param Mass			Mass (kg)
	* @return				dV (m/s)
	*/
	static double CalculateVelocityChangeFromPower(double Power, FVector PowerDirection, double InitialSpeed, double Mass, double dt);

	/**
	* Calculates change in velocity from given power (W) and velocity (m/s)
	* @param Power				Power (W)
	* @param InitialVelocity	Initial Velocity (m/s)
	* @param Mass				Mass (kg)
	* @return					dV (m/s)
	*/
	static FVector CalculateVelocityChangeFromPower(FVector Power, FVector InitialVelocity, double Mass, double dt);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Physics|Environment")
	/**
	* Calculates the air pressure at a given altitude.
	* @param g			Acceleration due to Gravity (m/s)
	* @param h			Altitude (m)
	* @param T			Temperature (Celsius)
	* @param P0			Reference Pressure/Pressure at sea level (Pa)
	* @return Pressure (Pa)
	*/
	static double CalculateAirPressureAtAltitude(const double g, const double h, const double T, const double P0 = 101325/*Reference Pressure at Sea Level(Pa)*/);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Physics|Environment")
	/**
	* Calcutes the density of air with the given parameters
	* @param p			Absolute Pressure (Pa)
	* @param T			Absolute Temperature (Kelvin)
	* @param Rh			Relative Humitidy
	* @return			Air density (kg/m^3)
	*/
	static double CalculateAirDensity(const double p, const double T, double Rh = 0.5);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Physics|Environment")
	/**
	* Calculates the air density at a given altitude.
	* @param g			Acceleration due to Gravity (m/s)
	* @param h			Altitude (m)
	* @param T			Temperature (Celsius)
	* @param P0			Reference Pressure/Pressure at sea level (Pa)
	* @param Rh			Relative Humitidy
	* @return			Air density (kg/m^3)
	*/
	static double CalculateAirDensityAtAltitude(const double g, const double h, const double T, double P0 = -1, double Rh = -1);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Physics", meta = (DisplayName = "Get Work From Force"))
	/**
	* Calculate the work(J) given the applied force and distance
	* @param F			Applied force(N)
	* @param s			Distance(m) over which work is done
	*/
	static double GetWorkFromForce(double F, double s);
#pragma endregion

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Utility")
	static bool OverlapMultiSortedByActor(UWorld* World, TMap<AActor*, FTBOverlapArray>& OverlapsSortedByActor, const FVector& Location, const FQuat& Rotation, const FTBCollisionParams& CollisionParams, const double Radius);

	static bool OverlapMultiSortedByActorWithShape(UWorld* World, TMap<AActor*, FTBOverlapArray>& OverlapsSortedByActor, const FVector& Location, const FQuat& Rotation, const FTBCollisionParams& CollisionParams, const FCollisionShape& CollisionShape);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Utility")
	static bool OverlapsObjectWithTag(UWorld* World, const FVector& Location, const FTBCollisionParams& CollisionParams, const double Radius, const FName& tag);

	static FOverlapResult GetClosestOverlap(const FTBOverlapArray& Overlaps, FVector Location);

	/**
	* Returns a preset list of object types that may typically be used for Projectile tracing.
	* These types are:
	* WorldStatic
	* WorldDynamic
	* Pawn
	* PhysicsBody
	* Destructible
	*/
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Utility")
	static TArray<TEnumAsByte<EObjectTypeQuery>> GetDefaultObjectTypes() { return CollisionPresets::DefaultCollisionQueryTypes; }

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Utility", meta = (NativeMakeFunc, BlueprintThreadSafe))
	static FTBLocationAndDirection MakeLocationAndDirection(const FVector Location, const FRotator Direction);
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Utility", meta = (CompactNodeTitle = "->"))
	static FTBLocationAndDirection MakeLocationAndDirectionFromTransform(const FTransform Transform);
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Utility", meta = (CompactNodeTitle = "->"))
	static FTBLocationAndDirection MakeLocationAndDirectionFromVectors(const FVector Location, const FVector Direction);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Utility", meta = (NativeBreakFunc, BlueprintThreadSafe))
	static void BreakLocationAndDirection(const FTBLocationAndDirection& LocationAndDirection, FVector& Location, FVector& Direction, FRotator& Rotation);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Utility", meta = (CompactNodeTitle = "->"))
	static FTransform LocationAndDirectionToTransform(const FTBLocationAndDirection LocationAndDirection);

	/**
	* Make LaunchParams Structure
	* @param ProjectileSpeed		Speed of the Projectile (m/s).
	* @param EffectiveRange			Controls roughly how far the Projectile should travel (m). This is usually the maximum distance at which the Projectile is still dangerous.
	* @param Timescale				Controls how quickly the simulation will run. Can be used for a "Slow Motion" effect. A value of zero or less is ignored.
	* @param FireLocation			Where the Projectile should be launched from.
	* @param FireDirection			Which direction the Projectile should move.
	* @param ToIgnore				An array of actors that the Projectile should ignore while simulating.
	* @param ObjectTypes			Types of objects the Projectile should sweep for. (These are the object types that this Projectile will be able to hit).
	* @param TraceChannel			The collision channel to use for traces.
	* @param bIgnoreOwner			Whether or not the Projectile should ignore it's owner.
	* @param bAddToOwnerVelocity	When true, the velocity of the projectile will be added to the velocity of the owner.
	* @param bForceNoTracer			When true, no tracer will be used, even if one exists.
	* @param Owner					The actor that owns this Projectile.
	* @param Instigator				The controller that instigated this launch.
	* @param SimType				Type of simulation used (instant or realtime).
	* @param OwnerIgnoreDistance	Owning actor will be ignored until the projectile has travelled this distance (m). Helps to prevent erroneous hits on the owner due to latency.
	*/
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Utility", meta = (AdvancedDisplay = "OwnerIgnoreDistance", AutoCreateRefTerm = "ToIgnore, ObjectTypes", NativeMakeFunc, BlueprintThreadSafe, TraceChannel = "ECC_GameTraceChannel10", bIgnoreOwner = "true", bAddToOwnerVelocity = "true", bForceNoTracer = "false", ProjectileSpeed = "360", EffectiveRange = "150", Timescale = "1", DefaultToSelf = "Owner",
		ObjectTypes = "{TEnumAsByte<EObjectTypeQuery>(0), TEnumAsByte<EObjectTypeQuery>(1), EnumAsByte<EObjectTypeQuery>(2), TEnumAsByte<EObjectTypeQuery>(3), TEnumAsByte<EObjectTypeQuery>(5)}"
		))
	static FTBLaunchParams MakeLaunchParams(
		double ProjectileSpeed,
		double EffectiveRange,
		double Timescale,
		FVector FireLocation,
		FRotator FireDirection,
		TArray<AActor*> ToIgnore,
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes,
		TEnumAsByte<ECollisionChannel> TraceChannel,
		bool bIgnoreOwner,
		bool bAddToOwnerVelocity,
		bool bForceNoTracer,
		AActor* Owner,
		AController* Instigator,
		ETBProjectileSimulationType SimType,
		double OwnerIgnoreDistance = 10.0
	);
	/**
	* Make LaunchParams Structure
	* @param ProjectileSpeed		Speed of the Projectile (m/s).
	* @param EffectiveRange			Controls roughly how far the Projectile should travel (m). This is usually the maximum distance at which the Projectile is still dangerous.
	* @param Timescale				Controls how quickly the simulation will run. Can be used for a "Slow Motion" effect. A value of zero or less is ignored.
	* @param FireLocation			Where the Projectile should be launched from.
	* @param FireDirection			Which direction the Projectile should move.
	* @param ToIgnore				An array of actors that the Projectile should ignore while simulating.
	* @param ObjectTypes			Types of objects the Projectile should sweep for. (These are the object types that this Projectile will be able to hit).
	* @param TraceChannel			The collision channel to use for traces.
	* @param bIgnoreOwner			Whether or not the Projectile should ignore it's owner.
	* @param bAddToOwnerVelocity	When true, the velocity of the projectile will be added to the velocity of the owner.
	* @param bForceNoTracer			When true, no tracer will be used, even if one exists.
	* @param Owner					The actor that owns this Projectile.
	* @param Instigator				The controller that instigated this launch.
	* @param SimType				Type of simulation used (instant or realtime).
	* @param OwnerIgnoreDistance	Owning actor will be ignored until the projectile has travelled this distance (m). Helps to prevent erroneous hits on the owner due to latency.
	*/
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Utility", meta = (AdvancedDisplay = "OwnerIgnoreDistance", AutoCreateRefTerm = "ToIgnore, ObjectTypes", TraceChannel = "ECC_GameTraceChannel10", bIgnoreOwner = "true", bAddToOwnerVelocity = "true", bForceNoTracer = "false", ProjectileSpeed = "360", EffectiveRange = "150", Timescale = "1", DefaultToSelf = "Owner",
		ObjectTypes = "{TEnumAsByte<EObjectTypeQuery>(0), TEnumAsByte<EObjectTypeQuery>(1), EnumAsByte<EObjectTypeQuery>(2), TEnumAsByte<EObjectTypeQuery>(3), TEnumAsByte<EObjectTypeQuery>(5)}"
	))
	static FTBLaunchParams MakeLaunchParamsWithDirectionVector(
		const double ProjectileSpeed,
		const double EffectiveRange,
		const double Timescale,
		const FVector FireLocation,
		const FVector FireDirection,
		const TArray<AActor*> ToIgnore,
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes,
		TEnumAsByte<ECollisionChannel> TraceChannel,
		bool bIgnoreOwner,
		bool bAddToOwnerVelocity,
		bool bForceNoTracer,
		AActor* Owner,
		AController* Instigator,
		ETBProjectileSimulationType SimType,
		double OwnerIgnoreDistance = 10.0
	);
	/**
	* Break LaunchParams Structure
	* @param ProjectileSpeed		Speed of the Projectile (m/s).
	* @param EffectiveRange			Controls roughly how far the Projectile should travel (m). This is usually the maximum distance at which the Projectile is still dangerous.
	* @param Timescale				Controls how quickly the simulation will run. Can be used for a "Slow Motion" effect. A value of zero or less is ignored.
	* @param OwnerIgnoreDistance	Owning actor will be ignored until the projectile has travelled this distance (m). Helps to prevent erroneous hits on the owner due to latency.
	* @param FireLocation			Where the Projectile should be launched from.
	* @param FireDirection			Which direction the Projectile should move.
	* @param ToIgnore				An array of actors that the Projectile should ignore while simulating.
	* @param ObjectTypes			Types of objects the Projectile should sweep for. (These are the object types that this Projectile will be able to hit).
	* @param TraceChannel			The collision channel to use for traces.
	* @param bIgnoreOwner			Whether or not the Projectile should ignore it's owner.
	* @param bAddToOwnerVelocity	When true, the velocity of the projectile will be added to the velocity of the owner.
	* @param Owner					The actor that owns this Projectile.
	* @param Instigator				The controller that instigated this launch.
	* @param SimType				Type of simulation used (instant or realtime).
	*/
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Utility", meta = (AdvancedDisplay = "OwnerIgnoreDistance", NativeBreakFunc, BlueprintThreadSafe))
	static void BreakLaunchParams(
		const FTBLaunchParams& LaunchParams,
		double& ProjectileSpeed,
		double& EffectiveRange,
		double& Timescale,
		double& OwnerIgnoreDistance,
		FVector& FireLocation,
		FRotator& FireDirection,
		TArray<AActor*>& ToIgnore,
		TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes,
		TEnumAsByte<ECollisionChannel>& TraceChannel,
		bool& bIgnoreOwner,
		bool& bAddToOwnerVelocity,
		bool& bForceNoTracer,
		AActor*& Owner,
		AController*& Instigator,
		ETBProjectileSimulationType& SimType
	);

	UFUNCTION(BlueprintPure, Category = "TerminalBallistics|Utility", meta = (ToolTip = "Creates an array of launch params distributed evenly on a unit sphere, useful for an omnidirectional burst of projectiles."))
	static TArray<FTBLaunchParams> GenerateLaunchParamsDistributedOnSphere(const FTBLaunchParams& BaseLaunchParams, const int32 Num = 250, const double SphereRadius = 1);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Projectiles", meta = (ReturnDisplayName = "Id"))
	static FTBProjectileId GenerateProjectileId()
	{
		return FTBProjectileId::CreateNew();
	}

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Projectiles", meta =  (ReturnDisplayName = "Is Valid"))
	static bool IsProjectileIdValid(const FTBProjectileId& Id)
	{
		return Id.IsValid();
	}

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Projectiles", meta = (CompactNodeTitle = "->"))
	static FString ProjectileIdToString(const FTBProjectileId& Id);

	/**
	* @param Id		Used to override the autogenerated Id. Useful when you need to sync Ids across client and server. Use with caution, as duplicate Ids can result in unwanted behavior.
	*/
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|Projectiles", meta = (AdvancedDisplay = "DebugType, Id", AutoCreateRefTerm = "LaunchParams, Id", ReturnDisplayName = "Id", DefaultToSelf = "Owner", UnsafeDuringActorConstruction))
	static FTBProjectileId AddAndFireBullet(AActor* Owner, TSoftObjectPtr<class UBulletDataAsset> Bullet, UPARAM(Ref)FTBLaunchParams& LaunchParams, const FTBProjectileId& Id, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType")) int32 DebugType = 0);
	/**
	* @param Id		Used to override the autogenerated Id. Useful when you need to sync Ids across client and server. Use with caution, as duplicate Ids can result in unwanted behavior.
	*/
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|Projectiles", meta = (AdvancedDisplay = "DebugType, Id", AutoCreateRefTerm = "LaunchParams, Id", ReturnDisplayName = "Id", DefaultToSelf = "Owner", UnsafeDuringActorConstruction))
	static FTBProjectileId AddAndFireBulletWithCallbacks(AActor* Owner, TSoftObjectPtr<class UBulletDataAsset> Bullet, UPARAM(Ref)FTBLaunchParams& LaunchParams, FBPOnProjectileComplete OnBulletComplete, FBPOnBulletHit OnBulletHit, FBPOnBulletExitHit OnBulletExitHit, FBPOnBulletInjure OnBulletInjure, const FTBProjectileId& Id, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType")) int32 DebugType = 0);
	/**
	* @param Id		Used to override the autogenerated Id. Useful when you need to sync Ids across client and server. Use with caution, as duplicate Ids can result in unwanted behavior.
	*/
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|Projectiles", meta = (AdvancedDisplay = "DebugType, Id", AutoCreateRefTerm = "LaunchParams, Id", ReturnDisplayName = "Id", DefaultToSelf = "Owner", UnsafeDuringActorConstruction))
	static FTBProjectileId AddAndFireBulletWithCallbacksAndUpdate(AActor* Owner, TSoftObjectPtr<class UBulletDataAsset> Bullet, UPARAM(Ref)FTBLaunchParams& LaunchParams, FBPOnProjectileComplete OnBulletComplete, FBPOnBulletHit OnBulletHit, FBPOnBulletExitHit OnBulletExitHit, FBPOnBulletInjure OnBulletInjure, FBPOnProjectileUpdate OnUpdate, const FTBProjectileId& Id, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType")) int32 DebugType = 0);

	/**
	* @param Id		Used to override the autogenerated Id. Useful when you need to sync Ids across client and server. Use with caution, as duplicate Ids can result in unwanted behavior.
	*/
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|Projectiles", meta = (AdvancedDisplay = "DebugType, Id", AutoCreateRefTerm = "LaunchParams, Id", ReturnDisplayName = "Id", DefaultToSelf = "Owner", UnsafeDuringActorConstruction))
	static FTBProjectileId AddAndFireProjectile(AActor* Owner, const FTBProjectile& Projectile, const FPhysMatProperties& ProjectilePhysicalProperties, UPARAM(Ref)FTBLaunchParams& LaunchParams, const FTBProjectileId& Id, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType"))  int32 DebugType = 0);
	/**
	* @param Id		Used to override the autogenerated Id. Useful when you need to sync Ids across client and server. Use with caution, as duplicate Ids can result in unwanted behavior.
	*/
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|Projectiles", meta = (AdvancedDisplay = "DebugType, Id", AutoCreateRefTerm = "LaunchParams, Id", ReturnDisplayName = "Id", DefaultToSelf = "Owner", UnsafeDuringActorConstruction))
	static FTBProjectileId AddAndFireProjectileWithCallbacks(AActor* Owner, const FTBProjectile& Projectile, const FPhysMatProperties& ProjectilePhysicalProperties, UPARAM(Ref)FTBLaunchParams& LaunchParams, FBPOnProjectileComplete OnProjectileComplete, FBPOnProjectileHit OnProjectileHit, FBPOnProjectileExitHit OnProjectileExitHit, FBPOnProjectileInjure OnProjectileInjure, const FTBProjectileId& Id, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType"))  int32 DebugType = 0);
	/**
	* @param Id		Used to override the autogenerated Id. Useful when you need to sync Ids across client and server. Use with caution, as duplicate Ids can result in unwanted behavior.
	*/
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|Projectiles", meta = (AdvancedDisplay = "DebugType, Id", AutoCreateRefTerm = "LaunchParams, Id", ReturnDisplayName = "Id", DefaultToSelf = "Owner", UnsafeDuringActorConstruction))
	static FTBProjectileId AddAndFireProjectileWithCallbacksAndUpdate(AActor* Owner, const struct FTBProjectile& Projectile, const FPhysMatProperties& ProjectilePhysicalProperties, UPARAM(Ref)FTBLaunchParams& LaunchParams, FBPOnProjectileComplete OnProjectileComplete, FBPOnProjectileHit OnProjectileHit, FBPOnProjectileExitHit OnProjectileExitHit, FBPOnProjectileInjure OnProjectileInjure, FBPOnProjectileUpdate OnUpdate, const FTBProjectileId& Id, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType"))  int32 DebugType = 0);
	/**
	* @param Id		Used to override the autogenerated Id. Useful when you need to sync Ids across client and server. Use with caution, as duplicate Ids can result in unwanted behavior.
	*/
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|Projectiles", meta = (AdvancedDisplay = "DebugType, Ids", AutoCreateRefTerm = "LaunchParams, Ids", ReturnDisplayName = "Ids", DefaultToSelf = "Owner", UnsafeDuringActorConstruction))
	static TArray<FTBProjectileId> AddAndFireProjectiles(AActor* Owner, TArray<FTBProjectile> Projectiles, const FPhysMatProperties& ProjectilePhysicalProperties, TArray<FTBLaunchParams> LaunchParams, UPARAM(Ref)TArray<FTBProjectileId>& Ids, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType"))  int32 DebugType = 0);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|Projectiles", meta = (AdvancedDisplay = "DebugType, Ids", AutoCreateRefTerm = "LaunchParams, Ids", ReturnDisplayName = "Ids", DefaultToSelf = "Owner", UnsafeDuringActorConstruction))
	static TArray<FTBProjectileId> AddAndFireProjectilesWithCallbacks(AActor* Owner, TArray<FTBProjectile> Projectiles, const FPhysMatProperties& ProjectilePhysicalProperties, TArray<FTBLaunchParams> LaunchParams, UPARAM(Ref)TArray<FTBProjectileId>& Ids, FBPOnProjectileComplete OnProjectileComplete, FBPOnProjectileHit OnProjectileHit, FBPOnProjectileExitHit OnProjectileExitHit, FBPOnProjectileInjure OnProjectileInjure, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType"))  int32 DebugType = 0);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|Projectiles", meta = (AdvancedDisplay = "DebugType, Ids", AutoCreateRefTerm = "LaunchParams, Ids", ReturnDisplayName = "Ids", DefaultToSelf = "Owner", UnsafeDuringActorConstruction))
	static TArray<FTBProjectileId> AddAndFireProjectilesWithCallbacksAndUpdate(AActor* Owner, TArray<FTBProjectile> Projectiles, const FPhysMatProperties& ProjectilePhysicalProperties, TArray<FTBLaunchParams> LaunchParams, UPARAM(Ref)TArray<FTBProjectileId>& Ids, FBPOnProjectileComplete OnProjectileComplete, FBPOnProjectileHit OnProjectileHit, FBPOnProjectileExitHit OnProjectileExitHit, FBPOnProjectileInjure OnProjectileInjure, FBPOnProjectileUpdate OnUpdate, UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/TerminalBallistics.ETBBallisticsDebugType"))  int32 DebugType = 0);

	/**
	* @param Id		Used to override the autogenerated Id. Useful when you need to sync Ids across client and server. Use with caution, as duplicate Ids can result in unwanted behavior.
	*/
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|Projectiles", meta = (AdvancedDisplay = "Id", AutoCreateRefTerm = "Id", Latent, LatentInfo = "LatentInfo", WorldContext = "Owner", UnsafeDuringActorConstruction))
	static FTBProjectileId AddAndFireTestBullet(AActor* Owner, TSoftObjectPtr<class UBulletDataAsset> Bullet, UPARAM(Ref)FTBLaunchParams& LaunchParams, TArray<FPredictProjectilePathPointData>& Results, FLatentActionInfo LatentInfo, const FTBProjectileId& Id);
	static FTBProjectileId AddAndFireTestBullet(AActor* Owner, TSoftObjectPtr<class UBulletDataAsset> Bullet, FTBLaunchParams& LaunchParams, TFunction<void(const TArray<FPredictProjectilePathPointData>&)> CompletionFunction, const FTBProjectileId& Id = FTBProjectileId::None);


	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|Damage")
	/**
	* Approximates the wound cavity created by projectile penetration.
	* @param ProjectileSpeed				Projectile Speed (m/s)
	* @param ImpartedEnergy					Energy imparted to the impacted object (J)
	* @param PenetrationDepth				How far the projectile penetrated into the object (m)
	* @param ObjectThickness				Thickness of the object where it was struck by the projectile
	* @param ProjectileProperties			Projectile Properties
	* @param ProjectilePhysicalProperties	Physical Properties of the projectile (FPhysMatProperties)
	* @param ObjectPhysicalProperties		Physical Properties of the penetrated object (FPhysMatProperties)
	* @return								The wound cavity created by the projectile
	*/
	static FTBWoundCavity CalculateCavitationRadii(const double ProjectileSpeed, const double ImpartedEnergy, const double PenetrationDepth, const double ObjectThickness, const FTBProjectilePhysicalProperties& ProjectileProperties, const FPhysMatProperties& ProjectilePhysicalProperties, const FPhysMatProperties& ObjectPhysicalProperties);

	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|Utility", meta = (AdvancedDisplay = "bTraceComplex, bUseSecondaryTraceChannel, SecondaryTraceChannel",DefaultToSelf = "WorldContextObject", ReturnDisplayName = "Exit Location"))
	/**
	* Finds the first point along the given path that does not overlap the given component.
	* Excludes path start.
	*
	* @param PathStart				The start of the path
	* @param PathDirection			The direction of the path.
	* @param ComponentToExitFrom	Which component to find the exit of.
	* @param Radius					Radius of the shphere used to test for collision. If zero, a line sweep will be used instead.
	* @param ExitFound				Whether or not an exit was found.
	* @param Resolution				How precisely to check for exits.
	* @param bTraceComplex			Whether to trace against simple or complex collsion
	* @param bUseSecondaryCollision	For instances where the exit location is overlapping another component, should we check if it responds to the given trace channel before using it?
	* @param SecondaryTraceChannel	The collision channel to check against for secondary collisions.
	* @return						The exit location.
	*/
	static FVector FindExitFromComponent(const UObject* WorldContextObject, const FVector& PathStart, const FVector& PathDirection, UPrimitiveComponent* ComponentToFindExitFrom, const double Radius, bool& ExitFound, double Resolution = 1.0, bool bTraceComplex = true, bool bUseSecondaryCollision = false, ECollisionChannel SecondaryTraceChannel = ECC_Visibility);
};
