// Copyright 2023, Dakota Dawe, All rights reserved

#include "Components/SKGOffHandIKComponent.h"

#include "Components/MeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

namespace SKGGAMEPLAYTAGS
{
	UE_DEFINE_GAMEPLAY_TAG(OffHandIKComponentFirearm, "OffHandIKComponentType.Firearm");
	UE_DEFINE_GAMEPLAY_TAG(OffHandIKComponentHandguard, "OffHandIKComponentType.Handguard");
	UE_DEFINE_GAMEPLAY_TAG(OffHandIKComponentForwardGrip, "OffHandIKComponentType.ForwardGrip");
}

// Sets default values for this component's properties
USKGOffHandIKComponent::USKGOffHandIKComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}


// Called when the game starts
void USKGOffHandIKComponent::BeginPlay()
{
	Super::BeginPlay();

	SetupComponents();
}

void USKGOffHandIKComponent::SetupComponents()
{
	ensureAlwaysMsgf(!OffHandIKMeshName.IsEqual(NAME_None), TEXT("Off Hand IK Mesh Name must be valid on Actor: %s"), *GetOwner()->GetName());
	for (UActorComponent* Component : GetOwner()->GetComponents())
	{
		if (Component && Component->GetFName().IsEqual(OffHandIKMeshName))
		{
			OffHandIKMesh = Cast<UMeshComponent>(Component);
			ensureAlwaysMsgf(OffHandIKMesh->DoesSocketExist(LeftHandIKSocketName) || OffHandIKMesh->DoesSocketExist(RightHandIKSocketName), TEXT("Attach To Mesh on Actor: %s Does NOT contain either socket: %s | %s"), *GetOwner()->GetName(), *LeftHandIKSocketName.ToString(), *RightHandIKSocketName.ToString());
			// Incase it's not attached to anything (such as holding a light/optic it will work)
			break;
		}
	}

	ensureAlwaysMsgf(OffHandIKMesh, TEXT("Could not find mesh with name: %s on Actor: %s. Ensure it exists and the name matches what is entered in Off Hand IK Mesh Name"), *OffHandIKMeshName.ToString(), *GetOwner()->GetName());

	if (OffHandIKOffset.Equals(FTransform()))
	{
		UpdateOffHandIK(OffHandIKMesh, true);
	}
}

FTransform USKGOffHandIKComponent::GetOffHandIKWorldTransform(bool bLeftHand) const
{
	return OffHandIKMesh->GetSocketTransform(GetHandSocketName(bLeftHand), RTS_World);
}

const FName& USKGOffHandIKComponent::GetHandSocketName(bool bLeftHand) const
{
	return bLeftHand ? LeftHandIKSocketName : RightHandIKSocketName;
}

void USKGOffHandIKComponent::UpdateOffHandIK(UPrimitiveComponent* ComponentRelativeTo, bool bLeftHand)
{
	if (OffHandIKMesh)
	{
		OffHandIKOffset = UKismetMathLibrary::MakeRelativeTransform(GetOffHandIKWorldTransform(bLeftHand), ComponentRelativeTo->GetComponentTransform());
	}
	LastRelativeToOffset = ComponentRelativeTo;
	OffHandIKPose = bLeftHand ? LeftHandIKPose : RightHandIKPose;
}
