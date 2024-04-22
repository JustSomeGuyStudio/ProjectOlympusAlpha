// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SKGOpticSubsystem.generated.h"

class UMaterialParameterCollection;
class UMaterialParameterCollectionInstance;
class UMeshComponent;

UCLASS()
class SKGOPTIC_API USKGOpticSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	USKGOpticSubsystem();
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(USKGOpticSubsystem, STATGROUP_Tickables); }
	virtual void Tick(float DeltaTime) override;
	
	
	UPROPERTY()
	TObjectPtr<UMaterialParameterCollectionInstance> MPCTest;
	UPROPERTY()
	TObjectPtr<UMeshComponent> MeshTest;
	
	UFUNCTION(BlueprintCallable, Category = "TEST")
	void SetTickTest(UMeshComponent* Mesh, UMaterialParameterCollection* MPC);
};
