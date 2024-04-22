// Copyright 2023, Dakota Dawe, All rights reserved


#include "Actors/SKGCharacter.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

ASKGCharacter::ASKGCharacter()
{
	// Set this character to call Tick() every .  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

void ASKGCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

UAISense_Sight::EVisibilityResult ASKGCharacter::CanBeSeenFrom(const FCanBeSeenFromContext& Context,
	FVector& OutSeenLocation, int32& OutNumberOfLoSChecksPerformed, int32& OutNumberOfAsyncLosCheckRequested,
	float& OutSightStrength, int32* UserData, const FOnPendingVisibilityQueryProcessedDelegate* Delegate)
{
	bool bIsVisible = false;
	FHitResult HitResult;
	FCollisionQueryParams Params;
	FVector StartTraceLocation = Context.ObserverLocation;
	if (Context.IgnoreActor)
	{
		Params.AddIgnoredActor(Context.IgnoreActor);
		if (const ACharacter* AICharacter = Cast<ACharacter>(Context.IgnoreActor))
		{
			if (AICharacter->GetMesh())
			{
				StartTraceLocation = AICharacter->GetMesh()->GetSocketLocation(FName("Camera"));
			}
		}
	}
	for (const FName& Bone : BonesToCheck)
	{
		const FVector TargetLocation = GetMesh()->GetSocketLocation(Bone);
		//DrawDebugLine(GetWorld(), StartTraceLocation, TargetLocation, FColor::Blue, false, -1, 0, 0.3f);
		if (GetWorld()->LineTraceSingleByChannel(HitResult, StartTraceLocation, TargetLocation, ECC_Visibility, Params))
		{
			if ((HitResult.GetActor() && HitResult.GetActor() == this) || HitResult.GetActor()->IsAttachedTo(this))
			{
				bIsVisible = true;
				break;
			}
		}
	}
	return bIsVisible ? UAISense_Sight::EVisibilityResult::Visible : UAISense_Sight::EVisibilityResult::NotVisible;
}
