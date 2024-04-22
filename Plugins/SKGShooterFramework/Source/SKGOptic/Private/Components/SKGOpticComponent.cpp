// Copyright 2023, Dakota Dawe, All rights reserved


#include "Components/SKGOpticComponent.h"

#include "GameFramework/Actor.h"
#include "Components/MeshComponent.h"
#include "Components/SKGOpticSceneCaptureComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Statics/SKGOpticLibrary.h"

void FSKGOpticReticleSettings::ConstructDynamicMaterials(UObject* WorldContextObject, USKGOpticSceneCaptureComponent* OptionalOpticSceneCaptureComponent)
{
	const float Brightness = GetReticleBrightness();
	for (FSKGOpticReticle& ReticleMaterial : ReticleMaterials)
	{
		ReticleMaterial.DynamicReticleMaterial = UKismetMaterialLibrary::CreateDynamicMaterialInstance(WorldContextObject, ReticleMaterial.ReticleMaterial);	
		ReticleMaterial.DynamicReticleMaterial->SetScalarParameterValue(ReticleBrightnessParameterName, Brightness);
		if (OptionalOpticSceneCaptureComponent)
		{
			OptionalOpticSceneCaptureComponent->SetupReticleMaterial(ReticleMaterial);
		}
	}

	if (UnAimedReticleMaterial.ReticleMaterial)
	{
		UnAimedReticleMaterial.DynamicReticleMaterial = UKismetMaterialLibrary::CreateDynamicMaterialInstance(WorldContextObject, UnAimedReticleMaterial.ReticleMaterial);	
		UnAimedReticleMaterial.DynamicReticleMaterial->SetScalarParameterValue(ReticleBrightnessParameterName, Brightness);
		if (OptionalOpticSceneCaptureComponent)
		{
			OptionalOpticSceneCaptureComponent->SetupReticleMaterial(UnAimedReticleMaterial);
		}
	}
}

bool FSKGOpticReticleSettings::IncreaseReticleBrightnessSetting()
{
	if (bUsingReticleNightVisionBrightness)
	{
		if (++CurrentReticleNightVisionBrightnessIndex >= ReticleNightVisionBrightnessSettings.Num())
		{
			CurrentReticleNightVisionBrightnessIndex = ReticleNightVisionBrightnessSettings.Num() - 1;
			return false;
		}
	}
	else
	{
		if (++CurrentReticleBrightnessIndex >= ReticleBrightnessSettings.Num())
		{
			CurrentReticleBrightnessIndex = ReticleBrightnessSettings.Num() - 1;
			return false;
		}
	}
	return true;
}

bool FSKGOpticReticleSettings::DecreaseReticleBrightnessSetting()
{
	if (bUsingReticleNightVisionBrightness)
	{
		if (!CurrentReticleNightVisionBrightnessIndex - 1 < 0)
		{
			--CurrentReticleNightVisionBrightnessIndex;
			return true;
		}
	}
	else
	{
		if (!CurrentReticleBrightnessIndex - 1 < 0)
		{
			--CurrentReticleBrightnessIndex;
			return true;
		}
	}
	return false;
}

float FSKGOpticMagnificationSettings::GetCurrentMagnificationFOV() const
{
	return USKGOpticLibrary::MagnificationToFOVAngle(GetCurrentMagnification());
}

USKGOpticComponent::USKGOpticComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

// Called when the game starts
void USKGOpticComponent::BeginPlay()
{
	Super::BeginPlay();

	ensureAlwaysMsgf(ReticleSettings.ReticleMaterials.Num(), TEXT("NO Reticle Materials Found in ReticleData.ReticleMaterials on Actor: %s in Component: %s"), *GetOwner()->GetName(), *GetName());
	
	SetComponents();
	ReticleSettings.ConstructDynamicMaterials(this, OpticSceneCaptureComponent);
	OpticZeroSettings.Initialize();
	SetStartingZero();
	UpdateOpticMaterialInstance();
}

void USKGOpticComponent::TryForceNetUpdate() const
{
	if (bAutoCallForceNetUpdate)
	{
		GetOwner()->ForceNetUpdate();
	}
}

void USKGOpticComponent::SetComponents()
{
	ensureAlwaysMsgf(!OpticMeshName.IsEqual(NAME_None), TEXT("Optic Mesh Name must be valid on Actor: %s"), *GetOwner()->GetName());
	for (UActorComponent* Component : GetOwner()->GetComponents())
	{
		if (Component)
		{
			if (Component->GetFName().IsEqual(OpticMeshName))
			{
				OpticMesh = Cast<UMeshComponent>(Component);
			}
			else if (Component->GetFName().IsEqual(OpticSceneCaptureComponentName))
			{
				OpticSceneCaptureComponent = Cast<USKGOpticSceneCaptureComponent>(Component);
				if (OpticSceneCaptureComponent)
				{
					OpticSceneCaptureComponent->Initialize(MagnificationSettings.bIsFirstFocalPlane, MagnificationSettings.GetCurrentMagnification(), MagnificationSettings.bSmoothZoom, MagnificationSettings.SmoothZoomRate, MagnificationSettings.bShrinkEyeboxWithMagnification, MagnificationSettings.ShrinkEyeboxMultiplier);
				}
			}
		}
	}
	ensureAlwaysMsgf(OpticMesh, TEXT("Optic Mesh NOT found on Actor: %s, Make sure Optic Mesh Name matches the name of the Mesh Component you want to use for your optic"), *GetOwner()->GetName());
}

void USKGOpticComponent::SetStartingZero()
{
	if (OpticSceneCaptureComponent)
	{
		if (OpticZeroSettings.StartingZeroSettings.bStartWithDefaultZero)
		{
			const int32 ElevationClicks = OpticZeroSettings.StartingZeroSettings.DefaultElevationClicks;
			const int32 WindageClicks = OpticZeroSettings.StartingZeroSettings.DefaultWindageClicks;
			const double Elevation = OpticZeroSettings.GetAdjustmentAmount() * ElevationClicks;
			const double Windage = OpticZeroSettings.GetAdjustmentAmount() * WindageClicks;
			OpticSceneCaptureComponent->PointOfImpactUp(Elevation);
			OpticSceneCaptureComponent->PointOfImpactRight(Windage);
		}
		else if (OpticZeroSettings.StartingZeroSettings.bStartWithRandomZero)
		{
			const int32 ElevationClicks = FMath::RandRange(-OpticZeroSettings.StartingZeroSettings.RandomMaxElevationClicks, OpticZeroSettings.StartingZeroSettings.RandomMaxElevationClicks);
			const int32 WindageClicks = FMath::RandRange(-OpticZeroSettings.StartingZeroSettings.RandomMaxWindageClicks, OpticZeroSettings.StartingZeroSettings.RandomMaxWindageClicks);
			const double Elevation = OpticZeroSettings.GetAdjustmentAmount() * ElevationClicks;
			const double Windage = OpticZeroSettings.GetAdjustmentAmount() * WindageClicks;
			OpticSceneCaptureComponent->PointOfImpactUp(Elevation);
			OpticSceneCaptureComponent->PointOfImpactRight(Windage);
		}
	}
}

void USKGOpticComponent::UpdateOpticMaterialInstance()
{
	checkf(ReticleSettings.GetReticleMaterial().DynamicReticleMaterial, TEXT("Reticle Material INVALID on Actor: %s in Component: %s"), *GetOwner()->GetName(), *GetName());
	OpticMesh->SetMaterial(ReticleSettings.ReticleMaterialIndex, ReticleSettings.GetReticleMaterial().DynamicReticleMaterial);
	UpdateReticleBrightness();
	if (OpticSceneCaptureComponent)
	{
		OpticSceneCaptureComponent->Zoom(ReticleSettings.GetReticleMaterial(), MagnificationSettings.GetCurrentMagnification());
	}
}

void USKGOpticComponent::UpdateReticleBrightness()
{
	checkf(ReticleSettings.GetReticleMaterial().DynamicReticleMaterial, TEXT("Reticle Material INVALID on Actor: %s in Component: %s"), *GetOwner()->GetName(), *GetName());
	ReticleSettings.GetReticleMaterial().DynamicReticleMaterial->SetScalarParameterValue(ReticleSettings.ReticleBrightnessParameterName, ReticleSettings.GetReticleBrightness());
}

void USKGOpticComponent::CycleReticle()
{
	ReticleSettings.CycleReticleMaterial();
	UpdateOpticMaterialInstance();
}

void USKGOpticComponent::IncreaseReticleBrightness()
{
	if (ReticleSettings.IncreaseReticleBrightnessSetting())
	{
		UpdateReticleBrightness();
	}
}

void USKGOpticComponent::DecreaseReticleBrightness()
{
	if (ReticleSettings.DecreaseReticleBrightnessSetting())
	{
		UpdateReticleBrightness();
	}
}
void USKGOpticComponent::ToggleReticleNightVisionSetting()
{
	ReticleSettings.CycleReticleNightVisionMode();
	UpdateReticleBrightness();
}

void USKGOpticComponent::ZoomIn()
{
	if (OpticSceneCaptureComponent && MagnificationSettings.SetNextMagnification())
	{
		OpticSceneCaptureComponent->Zoom(ReticleSettings.GetReticleMaterial(), MagnificationSettings.GetCurrentMagnification());
	}
}

void USKGOpticComponent::ZoomOut()
{
	if (OpticSceneCaptureComponent && MagnificationSettings.SetPreviousMagnification())
	{
		OpticSceneCaptureComponent->Zoom(ReticleSettings.GetReticleMaterial(), MagnificationSettings.GetCurrentMagnification());
	}
}

float USKGOpticComponent::GetCurrentMagnification() const
{
	return MagnificationSettings.GetCurrentMagnification();
}

int32 USKGOpticComponent::PointOfImpactUpDownDefault()
{
	const int32 Clicks = ZeroUpDownClicks;
	ZeroUpDownClicks = 0;
	if (OpticSceneCaptureComponent)
	{
		OpticSceneCaptureComponent->PointOfImpactUpDownDefault();
	}
	return Clicks;
}

int32 USKGOpticComponent::PointOfImpactLeftRightDefault()
{
	const int32 Clicks = ZeroLeftRightClicks;
	ZeroLeftRightClicks = 0;
	if (OpticSceneCaptureComponent)
	{
		OpticSceneCaptureComponent->PointOfImpactLeftRightDefault();
	}
	return Clicks;
}

void USKGOpticComponent::PointOfImpactUp(const int32 Clicks)
{
	if (OpticSceneCaptureComponent)
	{
		ZeroUpDownClicks += Clicks;
		const double AmountToAdjust = OpticZeroSettings.GetAdjustmentAmount() * Clicks;
		OpticSceneCaptureComponent->PointOfImpactUp(AmountToAdjust);
	}
}

void USKGOpticComponent::PointOfImpactDown(const int32 Clicks)
{
	if (OpticSceneCaptureComponent)
	{
		ZeroUpDownClicks -= Clicks;
		const double AmountToAdjust = OpticZeroSettings.GetAdjustmentAmount() * Clicks;
		OpticSceneCaptureComponent->PointOfImpactDown(AmountToAdjust);
	}
}

void USKGOpticComponent::PointOfImpactLeft(const int32 Clicks)
{
	if (OpticSceneCaptureComponent)
	{
		ZeroLeftRightClicks += Clicks;
		const double AmountToAdjust = OpticZeroSettings.GetAdjustmentAmount() * Clicks;
		OpticSceneCaptureComponent->PointOfImpactLeft(AmountToAdjust);
	}
}

void USKGOpticComponent::PointOfImpactRight(const int32 Clicks)
{
	if (OpticSceneCaptureComponent)
	{
		ZeroLeftRightClicks -= Clicks;
		const double AmountToAdjust = OpticZeroSettings.GetAdjustmentAmount() * Clicks;
		OpticSceneCaptureComponent->PointOfImpactRight(AmountToAdjust);
	}
}

void USKGOpticComponent::ApplyLookAtRotationZero(const FRotator& LookAtRotation)
{
	if (OpticSceneCaptureComponent)
	{
		OpticSceneCaptureComponent->ApplyLookAtRotationZero(LookAtRotation);
	}
	else // Apply via material here
	{

	}
}

void USKGOpticComponent::StartSceneCapture()
{
	if (OpticSceneCaptureComponent)
	{
		OpticMesh->SetMaterial(ReticleSettings.ReticleMaterialIndex, ReticleSettings.GetReticleMaterial().DynamicReticleMaterial);
		UpdateReticleBrightness();
		OpticSceneCaptureComponent->Zoom(ReticleSettings.GetReticleMaterial(), MagnificationSettings.GetCurrentMagnification());
		OpticSceneCaptureComponent->StartCapture();
	}
}

void USKGOpticComponent::StartedAiming()
{
	GetWorld()->GetTimerManager().ClearTimer(ReticleSettings.UnAimedTimerHandle);
	StartSceneCapture();
}

void USKGOpticComponent::StopSceneCapture()
{
	if (OpticSceneCaptureComponent)
	{
		OpticSceneCaptureComponent->StopCapture();
		if (ReticleSettings.UnAimedReticleMaterial)
		{
			OpticMesh->SetMaterial(ReticleSettings.ReticleMaterialIndex, ReticleSettings.UnAimedReticleMaterial.DynamicReticleMaterial);
			OpticSceneCaptureComponent->Zoom(ReticleSettings.UnAimedReticleMaterial, MagnificationSettings.GetCurrentMagnification());
		}
	}
}

void USKGOpticComponent::StoppedAiming()
{
	if (ReticleSettings.UnAimedCaptureDelay > 0.0f && OpticSceneCaptureComponent)
	{
		GetWorld()->GetTimerManager().SetTimer(ReticleSettings.UnAimedTimerHandle, this, &USKGOpticComponent::StopSceneCapture, ReticleSettings.UnAimedCaptureDelay, false);
	}
	else
	{
		StopSceneCapture();
	}
}
