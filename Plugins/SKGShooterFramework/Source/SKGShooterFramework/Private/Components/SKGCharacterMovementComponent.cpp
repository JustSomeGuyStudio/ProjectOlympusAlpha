// Copyright 2022, Dakota Dawe, All rights reserved


#include "Components/SKGCharacterMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

USKGCharacterMovementComponent::USKGCharacterMovementComponent()
{
	MaxAcceleration = 1365.0f;
	BrakingFriction = 1.0f;
	MaxWalkSpeed = 150.0f;
	DefaultMaxWalkSpeed = 150.0f;

	MaxWalkSpeedCrouched = 75.0f;
}

void USKGCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	DefaultMaxWalkSpeed = MaxWalkSpeed;
	RequestedWalkSpeedMultiplier = FMath::Clamp(DefaultWalkSpeedMultiplier, MinWalkSpeedMultiplier, MaxWalkSpeedMultiplier);
}

bool USKGCharacterMovementComponent::IsMovingForwards() const
{
	if (CharacterOwner)
	{
		FVector Forward = CharacterOwner->GetActorForwardVector();
		FVector MoveDirection = Velocity.GetSafeNormal();

		Forward.Z = 0.0f;
		MoveDirection.Z = 0.0f;
		return FVector::DotProduct(Forward, MoveDirection) > MoveDirectionTolerance;
	}
	return false;
}

bool USKGCharacterMovementComponent::IsMovingSideways() const
{
	if (CharacterOwner)
	{
		FVector Right = CharacterOwner->GetActorRightVector();
		FVector MoveDirection = Velocity.GetSafeNormal();

		Right.Z = 0.0f;
		MoveDirection.Z = 0.0f;
		const float DotProduct = FVector::DotProduct(Right, MoveDirection);
		return DotProduct > MoveDirectionTolerance || DotProduct < -MoveDirectionTolerance;
	}
	return false;
}

void USKGCharacterMovementComponent::StartSprinting()
{
	bWantsToSprint = true;
	bWantsToSuperSprint = false;
}

void USKGCharacterMovementComponent::StopSprinting()
{
	bWantsToSprint = false;
	bWantsToSuperSprint = false;
}

bool USKGCharacterMovementComponent::Server_SetWalkSpeedMultiplier_Validate(float WalkSpeedMultiplier)
{
	return true;
}

void USKGCharacterMovementComponent::Server_SetWalkSpeedMultiplier_Implementation(float WalkSpeedMultiplier)
{
	RequestedWalkSpeedMultiplier = FMath::Clamp(WalkSpeedMultiplier, MinWalkSpeedMultiplier, MaxWalkSpeedMultiplier);
}

void USKGCharacterMovementComponent::SetWalkSpeedMultiplier(float WalkSpeedMultiplier)
{
	RequestedWalkSpeedMultiplier = FMath::Clamp(WalkSpeedMultiplier, MinWalkSpeedMultiplier, MaxWalkSpeedMultiplier);
	if (GetOwnerRole() < ROLE_Authority)
	{
		Server_SetWalkSpeedMultiplier(RequestedWalkSpeedMultiplier);
	}
}

void USKGCharacterMovementComponent::IncreaseWalkSpeedMultiplier(float IncreaseAmount)
{
	SetWalkSpeedMultiplier(RequestedWalkSpeedMultiplier + IncreaseAmount);
}

void USKGCharacterMovementComponent::DecreaseWalkSpeedMultiplier(float DecreaseAmount)
{
	SetWalkSpeedMultiplier(RequestedWalkSpeedMultiplier - DecreaseAmount);
}

float USKGCharacterMovementComponent::GetSprintSpeed() const
{
	if(IsMovingForwards())
	{
		return bWantsToSuperSprint ? SuperSprintSpeed : SprintSpeed;
	}
	
	return SprintSidewaysSpeed;
}

float USKGCharacterMovementComponent::GetMaxSpeed() const
{
	if (bWantsToSprint || bWantsToSuperSprint)
	{
		return GetSprintSpeed();
	}
	if (IsCrouching())
	{
		return MaxWalkSpeedCrouched;
	}
	return MaxWalkSpeed * RequestedWalkSpeedMultiplier;
}

void USKGCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation,
	const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
}

void USKGCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	
	bWantsToSprint = (Flags&FSavedMove_Character::FLAG_Custom_0) != 0;
	bWantsToSuperSprint = (Flags&FSavedMove_Character::FLAG_Custom_1) != 0;
}

FNetworkPredictionData_Client* USKGCharacterMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		USKGCharacterMovementComponent* MutableThis = const_cast<USKGCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_SKGSF(*this);
	}
	return ClientPredictionData;
}

void FSavedMove_SKGSF::Clear()
{
	Super::Clear();
	bSavedWantsToSprint = false;
	bSavedWantsToSuperSprint = false;
	SavedRequestedWalkSpeedMultiplier = 1.0f;
}

uint8 FSavedMove_SKGSF::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if (bSavedWantsToSprint)
	{
		Result |= FLAG_Custom_0;
	}
	if (bSavedWantsToSuperSprint)
	{
		Result |= FLAG_Custom_1;
	}
	return Result;
}

bool FSavedMove_SKGSF::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{
	if (bSavedWantsToSprint != ((FSavedMove_SKGSF*)&NewMove)->bSavedWantsToSprint)
	{
		return false;
	}
	if (bSavedWantsToSuperSprint != ((FSavedMove_SKGSF*)&NewMove)->bSavedWantsToSuperSprint)
	{
		return false;
	}
	if (SavedRequestedWalkSpeedMultiplier != ((FSavedMove_SKGSF*)&NewMove)->SavedRequestedWalkSpeedMultiplier)
	{
		return false;
	}
	return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void FSavedMove_SKGSF::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	if (const USKGCharacterMovementComponent* CustomMovementComponent = Cast<USKGCharacterMovementComponent>(C->GetCharacterMovement()))
	{
		bSavedWantsToSprint = CustomMovementComponent->bWantsToSprint;
		bSavedWantsToSuperSprint = CustomMovementComponent->bWantsToSuperSprint;
		SavedRequestedWalkSpeedMultiplier = CustomMovementComponent->RequestedWalkSpeedMultiplier;
	}
}

void FSavedMove_SKGSF::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	if (USKGCharacterMovementComponent* CustomMovementComponent = Cast<USKGCharacterMovementComponent>(Character->GetCharacterMovement()))
	{
		CustomMovementComponent->bWantsToSprint = bSavedWantsToSprint;
		CustomMovementComponent->bWantsToSuperSprint = bSavedWantsToSuperSprint;
		CustomMovementComponent->RequestedWalkSpeedMultiplier = SavedRequestedWalkSpeedMultiplier;
	}
}

FSavedMovePtr FNetworkPredictionData_Client_SKGSF::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_SKGSF);
}