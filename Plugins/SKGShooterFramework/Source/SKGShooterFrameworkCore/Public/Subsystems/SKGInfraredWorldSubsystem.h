// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SKGInfraredWorldSubsystem.generated.h"

class UMaterialParameterCollection;
class UMaterialParameterCollectionInstance;

UCLASS()
class SKGSHOOTERFRAMEWORKCORE_API USKGInfraredWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	USKGInfraredWorldSubsystem();

private:
	UPROPERTY()
	TArray<UActorComponent*> InfraredDevices;
	UPROPERTY()
	TObjectPtr<UMaterialParameterCollectionInstance> InfraredMPC;
	
	bool bInfraredModeEnabled {false};
	const FName NightVisionColorParameterName {"NightVisionColor"};
	const FName NightVisionOnParameterName {"NightVisionOn"};

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	void SetInfraredMaterialParameterCollection(UMaterialParameterCollection* MPC);
	
public:
	UFUNCTION(BlueprintCallable, Category = "SKGShooterWorldSubsystem|Infrared")
	void SetNightVisionColor(const FLinearColor& Color);
	
	// If you enable something like night vision, this will turn all on devices that are in infrared mode to on
	UFUNCTION(BlueprintCallable, Category = "SKGShooterWorldSubsystem|Infrared")
	void EnableInfraredMode();
	// If you disable something like night vision, this will turn all off devices that are in infrared mode to off
	UFUNCTION(BlueprintCallable, Category = "SKGShooterWorldSubsystem|Infrared")
	void DisableInfraredMode();
	UFUNCTION(BlueprintCallable, Category = "SKGShooterWorldSubsystem|Infrared")
	void RegisterInfraredDevice(UActorComponent* InfraredDevice);
	UFUNCTION(BlueprintCallable, Category = "SKGShooterWorldSubsystem|Infrared")
	void UnregisterInfraredDevice(UActorComponent* InfraredDevice);
	UFUNCTION(BlueprintPure, Category = "SKGShooterWorldSubsystem|Infrared")
	const TArray<UActorComponent*>& GetInfraredDevices() const { return InfraredDevices; }
	UFUNCTION(BlueprintPure, Category = "SKGShooterWorldSubsystem|Infrared")
	bool IsInfraredModeEnabledOnPlayer() const { return bInfraredModeEnabled; }
};
