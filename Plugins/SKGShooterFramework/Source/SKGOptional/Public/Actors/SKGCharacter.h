// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Perception/AISightTargetInterface.h"
#include "GameFramework/Character.h"
#include "Perception/AISightTargetInterface.h"
#include "SKGCharacter.generated.h"

UCLASS()
class SKGOPTIONAL_API ASKGCharacter : public ACharacter, public IAISightTargetInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASKGCharacter();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "SKG|AI")
	TArray<FName> BonesToCheck {"head", "spine_03", "lowerarm_l", "hand_l", "lowerarm_r", "hand_r", "thigh_l", "calf_l", "thigh_r", "calf_r" };
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	virtual UAISense_Sight::EVisibilityResult CanBeSeenFrom(const FCanBeSeenFromContext& Context, FVector& OutSeenLocation, int32& OutNumberOfLoSChecksPerformed, int32& OutNumberOfAsyncLosCheckRequested, float& OutSightStrength, int32* UserData, const FOnPendingVisibilityQueryProcessedDelegate* Delegate) override;
};
