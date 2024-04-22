// Copyright 2023, Dakota Dawe, All rights reserved


#include "Components/SKGOpticSceneCaptureComponent.h"
#include "Statics/SKGOpticLibrary.h"

#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

USKGOpticSceneCaptureComponent::USKGOpticSceneCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(false);
	
	bHiddenInGame = false;
	bCaptureEveryFrame = false;
	bCaptureOnMovement = false;
	bAlwaysPersistRenderingState = true;
	PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR;
	
	ShowFlags.DynamicShadows = 1;
	ShowFlags.AmbientOcclusion = 0;
	ShowFlags.AmbientCubemap = 0;
	ShowFlags.DistanceFieldAO = 0;
	ShowFlags.LightFunctions = 1;
	ShowFlags.LightShafts = 0;
	ShowFlags.ReflectionEnvironment = 1;
	ShowFlags.ScreenSpaceReflections = 0;
	ShowFlags.TexturedLightProfiles = 0;
	ShowFlags.VolumetricFog = 1;
	ShowFlags.MotionBlur = 0;

	PostProcessSettings.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::Lumen;
	PostProcessSettings.bOverride_DynamicGlobalIlluminationMethod = 1;
	PostProcessSettings.ReflectionMethod = EReflectionMethod::Lumen;
	PostProcessSettings.bOverride_ReflectionMethod = 1;
	PostProcessSettings.SceneFringeIntensity = 2.0f;
	PostProcessSettings.ChromaticAberrationStartOffset = 0.3f;

	PostProcessSettings.MotionBlurAmount = 0.0f;
	PostProcessSettings.MotionBlurMax = 0.0f;
	PostProcessSettings.MotionBlurPerObjectSize = 0.0f;
	PostProcessSettings.MotionBlurTargetFPS = 0.0f;
}

void USKGOpticSceneCaptureComponent::BeginPlay()
{
	Super::BeginPlay();

	SetupSceneCaptureComponent();
}

void USKGOpticSceneCaptureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	check(bRegistered);

	if (bInterpolateMagnification)
	{	// Interpolate float here, need current zoom value
		const float TargetMagnification = FMath::FInterpConstantTo(CurrentMagnification, SmoothZoomTargetMagnification, DeltaTime, MagnificationSettings.SmoothZoomRate);
		bInterpolateMagnification = !SetNewMagnification(TargetMagnification);
		if (!bInterpolateMagnification && !bShouldCapture)
		{
			SetComponentTickEnabled(false);
		}
	}
	
	if (bShouldCapture)
	{
		CaptureSceneDeferred();
	}
}

void USKGOpticSceneCaptureComponent::SetupSceneCaptureComponent()
{
	SetComponentTickInterval(1.0f / OpticSceneCaptureComponentSettings.RenderTargetSettings.TickInterval);
	// Incase someone sets it otherwise by accident, force tick off
	SetComponentTickEnabled(false);
	if (!TextureTarget)
	{
		TextureTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, OpticSceneCaptureComponentSettings.RenderTargetSettings.ResolutionX, OpticSceneCaptureComponentSettings.RenderTargetSettings.ResolutionY, OpticSceneCaptureComponentSettings.RenderTargetSettings.TextureRenderTargetFormat);
	}
	
	DefaultRelativeRotation = GetRelativeRotation();
}

void USKGOpticSceneCaptureComponent::SetupReticleMaterial(FSKGOpticReticle& ReticleMaterial)
{
	if (!TextureTarget)
	{
		SetupSceneCaptureComponent();
	}
	ReticleMaterial.DynamicReticleMaterial->GetScalarParameterValue(OpticSceneCaptureComponentSettings.MaterialSettings.ReticleSizeParameterName, ReticleMaterial.StartingReticleSizeParameterValue);
	ReticleMaterial.DynamicReticleMaterial->GetScalarParameterValue(OpticSceneCaptureComponentSettings.MaterialSettings.EyeboxSensitivityParameterName, ReticleMaterial.StartingEyeboxRangeParameterValue);
	ReticleMaterial.DynamicReticleMaterial->SetTextureParameterValue(OpticSceneCaptureComponentSettings.MaterialSettings.TextureTargetParameterName, TextureTarget);
}

void USKGOpticSceneCaptureComponent::Initialize(const bool IsFirstFocalPlane, const float StartingMagnification, const bool SmoothZoom, const float SmoothZoomRate, const bool ShrinkEyeboxWithMagnification, const float ShrinkEyeboxMultiplier)
{
	MagnificationSettings.bIsFirstFocalPlane = IsFirstFocalPlane;
	MagnificationSettings.bShrinkEyeboxWithMagnification = ShrinkEyeboxWithMagnification;
	MagnificationSettings.bSmoothZoom = SmoothZoom;
	MagnificationSettings.SmoothZoomRate = SmoothZoomRate;
	MagnificationSettings.ShrinkEyeboxMultiplier = ShrinkEyeboxMultiplier;

	CurrentMagnification = StartingMagnification;
	FOVAngle = USKGOpticLibrary::MagnificationToFOVAngle(StartingMagnification);
}

void USKGOpticSceneCaptureComponent::StartCapture()
{
	bShouldCapture = true;
	if (!IsComponentTickEnabled())
	{
		SetComponentTickEnabled(true);
	}
}

void USKGOpticSceneCaptureComponent::StopCapture()
{
	bShouldCapture = false;
	if (IsComponentTickEnabled())
	{
		SetComponentTickEnabled(false);
	}
}

bool USKGOpticSceneCaptureComponent::SetNewMagnification(const float Magnification)
{
	FOVAngle = USKGOpticLibrary::MagnificationToFOVAngle(Magnification);
	if (MagnificationSettings.bIsFirstFocalPlane)
	{
		CurrentReticleMaterial.DynamicReticleMaterial->SetScalarParameterValue(OpticSceneCaptureComponentSettings.MaterialSettings.ReticleSizeParameterName, Magnification * CurrentReticleMaterial.StartingReticleSizeParameterValue);
	}
	if (MagnificationSettings.bShrinkEyeboxWithMagnification)
	{
		CurrentReticleMaterial.DynamicReticleMaterial->SetScalarParameterValue(OpticSceneCaptureComponentSettings.MaterialSettings.EyeboxSensitivityParameterName, CurrentReticleMaterial.StartingEyeboxRangeParameterValue - (Magnification * MagnificationSettings.ShrinkEyeboxMultiplier));
	}

	CurrentMagnification = Magnification;
	return Magnification == SmoothZoomTargetMagnification;
}

void USKGOpticSceneCaptureComponent::Zoom(const FSKGOpticReticle& ReticleMaterial, const float TargetMagnification)
{
	CurrentReticleMaterial = ReticleMaterial;
	if (MagnificationSettings.bSmoothZoom)
	{
		SmoothZoomTargetMagnification = TargetMagnification;
		bInterpolateMagnification = true;
		if (!IsComponentTickEnabled())
		{
			SetComponentTickEnabled(true);
		}
	}
	else
	{
		SetNewMagnification(TargetMagnification);
	}
}

void USKGOpticSceneCaptureComponent::PointOfImpactUpDownDefault()
{
	const FRotator CurrentRotation = GetRelativeRotation();
	SetRelativeRotation(FRotator(DefaultRelativeRotation.Pitch, CurrentRotation.Yaw, CurrentRotation.Roll));
}

void USKGOpticSceneCaptureComponent::PointOfImpactLeftRightDefault()
{
	const FRotator CurrentRotation = GetRelativeRotation();
	SetRelativeRotation(FRotator(CurrentRotation.Pitch, DefaultRelativeRotation.Yaw, CurrentRotation.Roll));
}

void USKGOpticSceneCaptureComponent::PointOfImpactUp(const double RotationAmount)
{
	AddRelativeRotation(FRotator(-RotationAmount, 0.0, 0.0));
}

void USKGOpticSceneCaptureComponent::PointOfImpactDown(const double RotationAmount)
{
	AddRelativeRotation(FRotator(RotationAmount, 0.0, 0.0));
}

void USKGOpticSceneCaptureComponent::PointOfImpactLeft(const double RotationAmount)
{
	AddRelativeRotation(FRotator(0.0, RotationAmount, 0.0));
}

void USKGOpticSceneCaptureComponent::PointOfImpactRight(const double RotationAmount)
{
	AddRelativeRotation(FRotator(0.0, -RotationAmount, 0.0));
}

void USKGOpticSceneCaptureComponent::ApplyLookAtRotationZero(FRotator LookAtRotation)
{
	FRotator NewRotation = GetRelativeRotation();
	NewRotation.Pitch = LookAtRotation.Pitch;
	DefaultRelativeRotation = FRotator(NewRotation.Pitch, DefaultRelativeRotation.Yaw, DefaultRelativeRotation.Roll);
	SetRelativeRotation(NewRotation);
}
