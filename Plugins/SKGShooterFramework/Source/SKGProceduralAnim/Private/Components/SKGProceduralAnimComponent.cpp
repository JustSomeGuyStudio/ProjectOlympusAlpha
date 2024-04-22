// Copyright 2023, Dakota Dawe, All rights reserved


#include "Components/SKGProceduralAnimComponent.h"

#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Actor.h"
#include "Components/MeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"


// Sets default values for this component's properties
USKGProceduralAnimComponent::USKGProceduralAnimComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USKGProceduralAnimComponent::BeginPlay()
{
	Super::BeginPlay();

	bUsedForAiming = ProceduralAimSocketNames.Num() > 0;
	SetupComponents();
}

void USKGProceduralAnimComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(USKGProceduralAnimComponent, AimSocketIndex, Params);
}

void USKGProceduralAnimComponent::SetupComponents()
{
	//ensureAlwaysMsgf(ProceduralAimSocketNames.Num(), TEXT("Aim Socket Name must be valid on Actor: %s"), *GetOwner()->GetName());
	for (UActorComponent* Component : GetOwner()->GetComponents())
	{
		if (Component && Component->GetFName().IsEqual(ProceduralMeshName))
		{
			ProceduralAnimMesh = Cast<UMeshComponent>(Component);
			break;
		}
	}

	ensureAlwaysMsgf(ProceduralAnimMesh, TEXT("Could not find mesh with name: %s on Actor: %s. Ensure it exists and the name matches what is entered in ProceduralMeshName"), *ProceduralMeshName.ToString(), *GetOwner()->GetName());

	if (CurrentAimSocket.Equals(FTransform()))
	{
		UpdateAimOffset(ProceduralAnimMesh);
	}
}

bool USKGProceduralAnimComponent::Server_SetAimSocketIndex_Validate(uint8 Index)
{
	return true;
}

void USKGProceduralAnimComponent::Server_SetAimSocketIndex_Implementation(uint8 Index)
{
	if (Index < ProceduralAimSocketNames.Num())
	{
		AimSocketIndex = Index;
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGProceduralAnimComponent, AimSocketIndex, this);
	}
}

bool USKGProceduralAnimComponent::CycleAimSocket()
{
	if (ProceduralAimSocketNames.Num())
	{
		PreviousAimSocketIndex = AimSocketIndex;
		if (++AimSocketIndex >= ProceduralAimSocketNames.Num())
		{
			AimSocketIndex = 0;
		}

		if (AimSocketIndex != PreviousAimSocketIndex)
		{
			if (HasAuthority())
			{
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGProceduralAnimComponent, AimSocketIndex, this);
			}
			else
			{
				Server_SetAimSocketIndex(AimSocketIndex);
			}
		}
		return AimSocketIndex != PreviousAimSocketIndex;
	}
	return false;
}

bool USKGProceduralAnimComponent::StartPointAiming(bool bRightHandDominant)
{
	const int32 Index = bRightHandDominant ? RightHandDominatePointAimAimSocketIndex : LeftHandDominatePointAimAimSocketIndex;
	if (Index != INDEX_NONE)
	{
		if (AimSocketIndex != Index)
		{
			PreviousAimSocketIndex = AimSocketIndex;
			AimSocketIndex = Index;
			if (HasAuthority())
			{
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGProceduralAnimComponent, AimSocketIndex, this);
			}
			else
			{
				Server_SetAimSocketIndex(AimSocketIndex);
			}
		}
		return true;
	}
	return false;
}

void USKGProceduralAnimComponent::StopPointAiming()
{
	if (AimSocketIndex != PreviousAimSocketIndex)
	{
		AimSocketIndex = PreviousAimSocketIndex;
		if (HasAuthority())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGProceduralAnimComponent, AimSocketIndex, this);
		}
		else
		{
			Server_SetAimSocketIndex(AimSocketIndex);
		}
	}
}

FTransform USKGProceduralAnimComponent::GetAimWorldTransform() const
{
	if (bUsedForAiming && ProceduralAnimMesh)
	{
		return ProceduralAnimMesh->GetSocketTransform(ProceduralAimSocketNames[AimSocketIndex], RTS_World);
	}
	return FTransform();
}

FTransform USKGProceduralAnimComponent::GetAimMuzzleTransform_Implementation()
{
	FTransform Transform = FTransform();
	if (bUsedForAiming && ProceduralAnimMesh)
	{
		const FTransform World = GetAimWorldTransform();
		FTransform Relative = ProceduralAnimMesh->GetSocketTransform(ProceduralAimSocketNames[AimSocketIndex], RTS_ParentBoneSpace);
	
		FRotator Rotator = Relative.Rotator();
		Rotator.Yaw += 90.0f;
		Relative.SetRotation(Rotator.Quaternion());
	
		Transform = UKismetMathLibrary::ComposeTransforms(Relative, World);
		Transform.SetLocation(World.GetTranslation());
	}
	return Transform;
}

const FSKGProceduralOffset& USKGProceduralAnimComponent::GetThirdPersonAimingOffset(bool bRightHanded) const
{
	return bRightHanded ? ThirdPersonRightHandedAimingOffset : ThirdPersonLeftHandedAimingOffset;
}

void USKGProceduralAnimComponent::UpdateAimOffset(UMeshComponent* ComponentRelativeTo)
{
	if (bUsedForAiming && ProceduralAnimMesh)
	{
		FTransform Relative;
		if (ComponentRelativeTo)
		{
			Relative = UKismetMathLibrary::MakeRelativeTransform(GetAimWorldTransform(), ComponentRelativeTo->GetComponentTransform());
		}
		else
		{
			Relative = UKismetMathLibrary::MakeRelativeTransform(GetAimWorldTransform(), ProceduralAnimMesh->GetComponentTransform());
		}
		
		FVector Location = Relative.GetLocation();
		const FRotator RelativeRotator = Relative.Rotator();
		FRotator Rotation;
		
		if (AimingSettings.ForwardAxis == EAxis::Type::Y)
		{
			Location.X = -Location.X;
			const float Distance = AimingSettings.bUseFixedAimingDistance ? -Location.Y : 0.0f;
			Location.Y = Distance + AimingSettings.AimingDistance;
			Location.Z = -Location.Z;
			Rotation = FRotator(-RelativeRotator.Pitch, RelativeRotator.Yaw, RelativeRotator.Roll);
		}
		else if (AimingSettings.ForwardAxis == EAxis::Type::X)
		{
			Location.Y = -Location.Y;
			const float Distance = AimingSettings.bUseFixedAimingDistance ? -Location.X : 0.0f;
			Location.X = Distance + AimingSettings.AimingDistance;
			Location.Z = -Location.Z;
			Rotation = FRotator(RelativeRotator.Roll, RelativeRotator.Yaw, RelativeRotator.Pitch);
		}
		else
		{
			Location.X = -Location.X;
			Location.Y = -Location.Y;
			const float Distance = AimingSettings.bUseFixedAimingDistance ? -Location.Z : 0.0f;
			Location.Z = Distance + AimingSettings.AimingDistance;
			Rotation = FRotator(-RelativeRotator.Pitch, RelativeRotator.Yaw, RelativeRotator.Roll);
		}
		
		CurrentAimSocket = FTransform(Rotation, Location);
	}
	LastRelativeToOffset = ComponentRelativeTo;
}

void USKGProceduralAnimComponent::UpdateAimOffsetWithSocket(UMeshComponent* ComponentRelativeTo, const FName& Socket)
{
	if (bUsedForAiming && ProceduralAnimMesh)
	{
		FTransform Relative;
		if (ComponentRelativeTo)
		{
			Relative = UKismetMathLibrary::MakeRelativeTransform(GetAimWorldTransform(), ComponentRelativeTo->GetSocketTransform(Socket));
		}
		else
		{
			Relative = UKismetMathLibrary::MakeRelativeTransform(GetAimWorldTransform(), ProceduralAnimMesh->GetComponentTransform());
		}
		FVector Location = Relative.GetLocation();
		Location.X = -Location.X;
		Location.Y = Location.X + 10;
		Location.Z = -Location.Z; // Consider adding Z offset option
		const FRotator Rotation = FRotator(-Relative.Rotator().Pitch, 0.0f, 0.0f);
		
		CurrentAimSocket = FTransform(Rotation, Location);
	}
	LastRelativeToOffset = ComponentRelativeTo;
}

bool USKGProceduralAnimComponent::GetPose(FGameplayTag Tag, FSKGToFromCurveSettings& PoseData)
{
	for (const FSKGToFromCurveSettings& Pose : PoseSettings)
	{
		if (Pose.CurveTag.MatchesTag(Tag))
		{
			PoseData = Pose;
			return true;
		}
	}
	return false;
}
