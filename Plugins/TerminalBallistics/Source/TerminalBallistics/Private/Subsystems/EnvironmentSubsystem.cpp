// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "Subsystems/EnvironmentSubsystem.h"
#include "Components/WindDirectionalSourceComponent.h"
#include "Engine/WindDirectionalSource.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsUtils.h"
#include "SceneInterface.h"
#include "SceneManagement.h"


UEnvironmentSubsystem::UEnvironmentSubsystem()
	: UWorldSubsystem()
{}

void UEnvironmentSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UWorldSubsystem::Initialize(Collection);

	if (const UTBConfiguration* TBConfig = GetDefault<UTBConfiguration>())
	{
		EnvironmentData = TBConfig->EnvironmentData;
	}
}

void UEnvironmentSubsystem::Deinitialize()
{
	UWorldSubsystem::Deinitialize();
}

void UEnvironmentSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	UWorldSubsystem::OnWorldBeginPlay(InWorld);

	if (GetWorld()->GetWorldSettings()->Implements<UTBWorldSettingsInterface>())
	{
		EnvironmentData = Cast<ITBWorldSettingsInterface>(GetWorld()->GetWorldSettings())->GetEnvironmentData();
	}
}

double UEnvironmentSubsystem::GetBaseAltitude() const
{
	return EnvironmentData.WorldAltitude;
}

double UEnvironmentSubsystem::GetAltitude(const FVector& Location) const
{
	const double InputZ = Location.Z == 0.0 ? 0.0 : Location.Z / 100.0; // Prevents division by zero and converts to meters
	return EnvironmentData.WorldAltitude + InputZ;
}

double UEnvironmentSubsystem::GetTemperatureBaseline() const
{
	return EnvironmentData.WorldTemperature;
}

double UEnvironmentSubsystem::GetTemperatureAtLocation(const FVector& Location) const
{
	const double AltitudeOffsetPerKm = FMath::GetMappedRangeValueClamped(TRange<double>(0.0, 1.0), TRange<double>(9.8, 5), GetRelativeHumidityAtLocation(Location)); // 9.8 C/km in dry air, 5 C/km in moist air
	const double AltitudeTempOffset = GetTemperatureBaseline() - (GetAltitude(Location) * (AltitudeOffsetPerKm / 1000));
	return AltitudeTempOffset;
}

double UEnvironmentSubsystem::GetRelativeHumidityBaseline() const
{
	return EnvironmentData.WorldHumidity;
}

double UEnvironmentSubsystem::GetRelativeHumidityAtLocation(const FVector& Location) const
{
	if (GetRelativeHumidityBaseline() == 1.0)
	{
		return 1.0;
	}
	const double Altitude = GetAltitude(Location);
	if (Altitude <= 0)
	{
		return GetRelativeHumidityBaseline();
	}
	else
	{
		double gravity = -9.81;
		if(UWorld* World = GetWorld())
		{
			gravity = World->GetGravityZ() / 100.0; // World->GravityZ() is in cm/s^2, convert it to m/s^2
		}
		return TB::CalculateHumidityAtAltitude(gravity, Altitude, GetTemperatureBaseline(), GetRelativeHumidityBaseline());
	}
}

FEnvironmentData UEnvironmentSubsystem::GetEnvironmentData() const
{
	return EnvironmentData;
}

void UEnvironmentSubsystem::SetEnvironmentData(const FEnvironmentData& NewEnvironmentData)
{
	EnvironmentData = NewEnvironmentData;
}

void UEnvironmentSubsystem::SetBaseAltitude(const double NewAltitude)
{
	EnvironmentData.WorldAltitude = NewAltitude;
}

void UEnvironmentSubsystem::SetBaseTemperature(const double NewTemperature)
{
	EnvironmentData.WorldTemperature = NewTemperature;
}

void UEnvironmentSubsystem::SetBaseHumidity(const double NewHumidity)
{
	EnvironmentData.WorldHumidity = NewHumidity;
}

FWindData UEnvironmentSubsystem::GetWindDataAtLocation(const FVector& Location) const
{
	if (!GetWorld())
	{
		return FWindData();
	}
	FWindData WindData = FWindData();
	if (FSceneInterface* Scene = GetWorld()->Scene)
	{
		FVector Direction = FVector::ZeroVector;
		float Speed, MinGust, MaxGust = 0.f;
		Scene->GetWindParameters_GameThread(Location, Direction, Speed, MinGust, MaxGust);
		WindData.Direction = Direction;
		WindData.Speed = Speed * TB::Configuration::WindSpeedMultiplier;
		WindData.MinGustAmt = MinGust;
		WindData.MaxGustAmt = MaxGust;
	}
	return WindData;
}

void UEnvironmentSubsystem::GetWindDataAtLocationBP(const FVector& Location, double& Speed, double& MinGustAmount, double& MaxGustAmount, FVector& Direction) const
{
	FWindData WindData = GetWindDataAtLocation(Location);
	Speed = WindData.Speed;
	MinGustAmount = WindData.MinGustAmt;
	MaxGustAmount = WindData.MaxGustAmt;
	Direction = WindData.Direction;
}
