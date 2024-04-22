// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "TerminalBallisticsConfiguration.h"
#include "UObject/Interface.h"
#include "EnvironmentSubsystem.generated.h"

class AWindDirectionalSource;
class FWindData;


UINTERFACE(MinimalAPI)
class UTBWorldSettingsInterface : public UInterface
{
	GENERATED_BODY()
};

/* Interface for classes derived from AWorldSettings to allow environment data to be retrieved from world settings. */
class TERMINALBALLISTICS_API ITBWorldSettingsInterface
{
	GENERATED_BODY()
public:
	virtual FEnvironmentData GetEnvironmentData() const = 0;
};



UCLASS()
class TERMINALBALLISTICS_API UEnvironmentSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:

	UEnvironmentSubsystem();

	// USubsystem implementation Begin
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//USubsystem implementation End

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Subsystems|Environment")
	double GetBaseAltitude() const;
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Subsystems|Environment")
	double GetAltitude(const FVector& Location) const;

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Subsystems|Environment", meta = (ToolTip = "Returns the base world temperature."))
	double GetTemperatureBaseline() const;
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Subsystems|Environment")
	double GetTemperatureAtLocation(const FVector& Location) const;

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Subsystems|Environment")
	double GetRelativeHumidityBaseline() const;
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Subsystems|Environment")
	double GetRelativeHumidityAtLocation(const FVector& Location) const;

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Subsystems|Environment")
	FEnvironmentData GetEnvironmentData() const;

	FWindData GetWindDataAtLocation(const FVector& Location) const;

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Subsystems|Environment")
	void GetWindDataAtLocationBP(const FVector& Location, double& Speed, double& MinGustAmount, double& MaxGustAmount, FVector& Direction) const;

	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|Subsystems|Environment")
	void SetEnvironmentData(const FEnvironmentData& NewEnvironmentData);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|Subsystems|Environment")
	void SetBaseAltitude(const double NewAltitude);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|Subsystems|Environment")
	void SetBaseTemperature(const double NewTemperature);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|Subsystems|Environment")
	void SetBaseHumidity(const double NewHumidity);

protected:
	FEnvironmentData EnvironmentData;
};
