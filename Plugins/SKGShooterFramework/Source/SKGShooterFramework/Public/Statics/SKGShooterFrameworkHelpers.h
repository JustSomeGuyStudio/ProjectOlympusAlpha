// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SKGShooterFrameworkHelpers.generated.h"

/**
 * 
 */
UCLASS()
class SKGSHOOTERFRAMEWORK_API USKGShooterFrameworkHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "SKGShooterFrameworkStatics")
	static class USKGShooterPawnComponent* GetShooterPawnComponent(const AActor* Actor);
	UFUNCTION(BlueprintPure, Category = "SKGShooterFrameworkStatics")
	static class USKGProceduralAnimComponent* GetProceduralAnimComponent(const AActor* Actor);
	UFUNCTION(BlueprintPure, Category = "SKGShooterFrameworkStatics")
	static class USKGFirearmComponent* GetFirearmComponent(const AActor* Actor);
	UFUNCTION(BlueprintPure, Category = "SKGShooterFrameworkStatics")
	static class USKGOpticComponent* GetOpticComponent(const AActor* Actor);
	UFUNCTION(BlueprintPure, Category = "SKGShooterFrameworkStatics")
	static class USKGFirearmAttachmentStatComponent* GetFirearmAttachmentStatComponent(const AActor* Actor);
	UFUNCTION(BlueprintPure, Category = "SKGShooterFrameworkStatics")
	static class USKGLightLaserComponent* GetLightLaserComponent(const AActor* Actor);
	// Searches for the firearm that Actor is attached to
	UFUNCTION(BlueprintPure, Category = "SKGShooterFrameworkStatics")
	static AActor* GetParentFirearmComponent(const AActor* Actor);
};