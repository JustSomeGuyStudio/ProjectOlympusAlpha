// Copyright 2022, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SKGCharacterMovementComponent.generated.h"

class FSavedMove_SKGSF;
class FNetworkPredictionData_Client;

UCLASS()
class SKGSHOOTERFRAMEWORK_API USKGCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	USKGCharacterMovementComponent();

	virtual void BeginPlay() override;
	
	friend FSavedMove_SKGSF;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual float GetMaxSpeed() const override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

protected:
	UPROPERTY(EditAnywhere, Category = "SKGMovementComponent|Sprinting")
	float SprintSpeed {275.0f};
	UPROPERTY(EditAnywhere, Category = "SKGMovementComponent|Sprinting")
	float SuperSprintSpeed {300.0f};
	UPROPERTY(EditAnywhere, Category = "SKGMovementComponent|Sprinting")
	float MoveDirectionTolerance {0.5f};
	UPROPERTY(EditAnywhere, Category = "SKGMovementComponent|Sprinting")
	bool bCanOnlySprintForwards {false};
	UPROPERTY(EditAnywhere, Category = "SKGMovementComponent|Sprinting", meta = (EditCondition = "!bCanOnlySprintForwards", EditConditionHides))
	bool bSprintSideways {true};
	UPROPERTY(EditAnywhere, Category = "SKGMovementComponent|Sprinting", meta = (EditCondition = "!bCanOnlySprintForwards && bSprintSideways", EditConditionHides))
	float SprintSidewaysSpeed {250.0f};

	UPROPERTY(EditAnywhere, Category = "SKGMovementComponent|Walking")
	float MinWalkSpeedMultiplier {0.25f};
	UPROPERTY(EditAnywhere, Category = "SKGMovementComponent|Walking")
	float MaxWalkSpeedMultiplier {1.0f};
	
	float DefaultMaxWalkSpeed {1.0f};
	uint8 bWantsToSprint : 1;
	uint8 bWantsToSuperSprint : 1;

	float DefaultWalkSpeedMultiplier {1.0f};
	float RequestedWalkSpeedMultiplier {1.0f};

public:
	bool IsMovingForwards() const;
	bool IsMovingSideways() const;
	
	UFUNCTION(BlueprintCallable, Category = "SKGMovementComponent|CharacterMovement")
	void StartSprinting();
	UFUNCTION(BlueprintCallable, Category = "SKGMovementComponent|CharacterMovement")
	void StopSprinting();
	UFUNCTION(BlueprintPure, Category = "SKGMovementComponent|CharacterMovement")
	bool IsSprinting() const { return bWantsToSprint; }
	UFUNCTION(BlueprintCallable, Category = "SKGMovementComponent|CharacterMovement")
	void SetWalkSpeedMultiplier(float WalkSpeedMultiplier);
	UFUNCTION(BlueprintCallable, Category = "SKGMovementComponent|CharacterMovement")
	void IncreaseWalkSpeedMultiplier(float IncreaseAmount = 0.1f);
	UFUNCTION(BlueprintCallable, Category = "SKGMovementComponent|CharacterMovement")
	void DecreaseWalkSpeedMultiplier(float DecreaseAmount = 0.1f);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetWalkSpeedMultiplier(float WalkSpeedMultiplier);
	
	float GetSprintSpeed() const;
};

class FSavedMove_SKGSF : public FSavedMove_Character
{
public:
	typedef FSavedMove_Character Super;
	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character & ClientData) override;
	virtual void PrepMoveFor(ACharacter* Character) override;

	uint8 bSavedWantsToSprint : 1;
	uint8 bSavedWantsToSuperSprint : 1;
	float SavedRequestedWalkSpeedMultiplier {1.0f};
};

class FNetworkPredictionData_Client_SKGSF : public FNetworkPredictionData_Client_Character
{
public:
	FNetworkPredictionData_Client_SKGSF(const UCharacterMovementComponent& ClientMovement):Super(ClientMovement){}
	typedef FNetworkPredictionData_Client_Character Super;
	virtual FSavedMovePtr AllocateNewMove() override;
};