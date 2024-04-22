// Copyright 2023, Dakota Dawe, All rights reserved


#include "Components/SKGMuzzleComponent.h"

#include "Components/MeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

constexpr double MRADAdjustment {0.0572958};
constexpr double MOAAdjustment {0.01666667};

namespace SKGGAMEPLAYTAGS
{
	UE_DEFINE_GAMEPLAY_TAG(MuzzleComponentBarrel, "MuzzleComponentType.Barrel");
	UE_DEFINE_GAMEPLAY_TAG(MuzzleComponentMuzzleDevice, "MuzzleComponentType.MuzzleDevice");
	UE_DEFINE_GAMEPLAY_TAG(MuzzleComponentSuppressor, "MuzzleComponentType.Suppressor");
}

USKGMuzzleComponent::USKGMuzzleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

}

void USKGMuzzleComponent::BeginPlay()
{
	Super::BeginPlay();

	SetupComponents();
	SetComponentTickInterval(MuzzleTemperatureTickInterval);
}

void USKGMuzzleComponent::SetupComponents()
{
	ensureAlwaysMsgf(MuzzleMeshComponentName != NAME_None, TEXT("Muzzle Mesh Component Name must be valid on Actor: %s"), *GetOwner()->GetName());
	ensureAlwaysMsgf(MuzzleSocketName != NAME_None, TEXT("Muzzle Socket Name must be valid on Actor: %s"), *GetOwner()->GetName());
	ensureAlwaysMsgf(MuzzleTag.IsValid(), TEXT("Muzzle Tag must be valid on Actor: %s"), *GetOwner()->GetName());

	for (UActorComponent* Component : GetOwner()->GetComponents())
	{
		if (Component && Component->GetFName().IsEqual(MuzzleMeshComponentName))
		{
			MuzzleMesh = Cast<UMeshComponent>(Component);;
		}
	}

	ensureAlwaysMsgf(MuzzleMesh, TEXT("MuzzleMesh NOT found on Actor: %s"), *GetOwner()->GetName());
	if (MuzzleMesh)
	{
		ensureAlwaysMsgf(MuzzleMesh->DoesSocketExist(MuzzleSocketName), TEXT("Socket: %s does NOT exist on Actor: %s"), *MuzzleSocketName.ToString(), *GetOwner()->GetName());
	}
}

void USKGMuzzleComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CurrentMuzzleTemperature -= DecreaseMuzzleTemperatureAmountPerTick * DeltaTime;
	if (CurrentMuzzleTemperature < 0.0f)
	{
		CurrentMuzzleTemperature = 0.0f;
	}
	OnMuzzleTemperatureChanged.Broadcast(CurrentMuzzleTemperature);
	
	if (CurrentMuzzleTemperature <= 0.0f)
	{
		SetComponentTickEnabled(false);
	}
}

void USKGMuzzleComponent::ShotPerformed()
{
	if (bUseMuzzleTemperatureSystem)
	{
		CurrentMuzzleTemperature += IncreaseMuzzleTemperatureAmountFahrenheit;
		OnMuzzleTemperatureChanged.Broadcast(CurrentMuzzleTemperature);
		if (!IsComponentTickEnabled())
		{
			SetComponentTickEnabled(true);
		}
	}
}

float USKGMuzzleComponent::GetMuzzleTemperatureNormalized() const
{
	return FMath::Clamp(1.0f + (MuzzleTemperatureNormalizeRate * FMath::Pow((1.0f - (CurrentMuzzleTemperature + 1.0f)), -1.0f)), 0.0f, 1.0f);
}

FTransform USKGMuzzleComponent::GetMuzzleTransform() const
{
	if (MuzzleMesh)
	{
		return MuzzleMesh->GetSocketTransform(MuzzleSocketName);
	}
	return FTransform();
}

FTransform USKGMuzzleComponent::GetMuzzleTransformRelative(const UPrimitiveComponent* ComponentRelativeTo) const
{
	if (ComponentRelativeTo)
	{
		return GetMuzzleTransform().GetRelativeTransform(ComponentRelativeTo->GetComponentTransform());
	}
	return FTransform();
}

FSKGMuzzleTransform USKGMuzzleComponent::GetMuzzleProjectileTransformCompensated(float ZeroDistanceMeters, float MOA, const FTransform& AimTransform) const
{
	ZeroDistanceMeters = FMath::Clamp(ZeroDistanceMeters * 100.0f, 0.0f, 10000000.0f);
	MOA *= MOAAdjustment;
	float AdjustedMOA = MOA;
	
	const FVector SightZeroDistanceLocation = AimTransform.GetTranslation() + AimTransform.GetRotation().Vector() * ZeroDistanceMeters;
	FRotator MuzzleLookAtSightRotation = UKismetMathLibrary::FindLookAtRotation(GetMuzzleTransform().GetTranslation(), SightZeroDistanceLocation);
	
	AdjustedMOA = FMath::RandRange(-MOA, MOA) * 0.5f;
	MuzzleLookAtSightRotation.Pitch += AdjustedMOA * 0.6f; // Multiply by 0.6 for more realistic consistency
	AdjustedMOA = FMath::RandRange(-MOA, MOA) * 0.5f;
	MuzzleLookAtSightRotation.Yaw += AdjustedMOA;

	FTransform MuzzleTransform = GetMuzzleTransform();
	MuzzleTransform.SetRotation(MuzzleLookAtSightRotation.Quaternion());
	return MuzzleTransform;
}

FSKGMuzzleTransform USKGMuzzleComponent::GetMuzzleProjectileTransform(float MOA) const
{
	MOA *= MOAAdjustment;
	float AdjustedMOA = MOA;
	FTransform MuzzleTransform = GetMuzzleTransform();
	FRotator MuzzleRotation = MuzzleTransform.Rotator();
	
	AdjustedMOA = FMath::RandRange(-MOA, MOA) * 0.5f;
	MuzzleRotation.Pitch += AdjustedMOA * 0.6f; // Multiply by 0.6 for more realistic consistency
	AdjustedMOA = FMath::RandRange(-MOA, MOA) * 0.5f;
	MuzzleRotation.Yaw += AdjustedMOA;

	MuzzleTransform.SetRotation(MuzzleRotation.Quaternion());
	return MuzzleTransform;
}