// Copyright 2023, Dakota Dawe, All rights reserved


#include "AnimationNotifies/UAnimNotify_SKGPlayFootstepSound.h"
#include "PhysicalMaterial/SKGPhysicalMaterial.h"

#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequenceBase.h"

void UUAnimNotify_SKGPlayFootstepSound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                               const FAnimNotifyEventReference& EventReference)
{
	if (ensureMsgf(MeshComp->DoesSocketExist(FootBoneName), TEXT("Socket: %s does NOT exist on: %s on Animation: %s"), *FootBoneName.ToString(), *MeshComp->GetName(), *Animation->GetName()))
	{
		if (ensureMsgf(PhysicalMaterialEffectTag.IsValid(), TEXT("Effect tag is NOT valid in a notify on Animation: %s"), *Animation->GetName()))
		{
			const FVector Start = MeshComp->GetSocketLocation(FootBoneName);
			const FVector End = FVector(Start.X, Start.Y, Start.Z - 100.0);

			FHitResult HitResult;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(MeshComp->GetOwner());
			Params.bReturnPhysicalMaterial = true;
			if (MeshComp->GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, TraceChannel, Params))
			{
				if (USKGPhysicalMaterial* PhysicalMaterial = Cast<USKGPhysicalMaterial>(HitResult.PhysMaterial))
				{
					PhysicalMaterial->PlayEffect(HitResult, false, PhysicalMaterialEffectTag);
				}
			}
		}
	}
}
