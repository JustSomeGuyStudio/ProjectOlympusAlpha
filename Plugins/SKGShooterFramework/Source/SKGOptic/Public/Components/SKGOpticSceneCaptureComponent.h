// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "DataTypes/SKGOpticCoreDataTypes.h"
#include "SKGOpticSceneCaptureComponent.generated.h"

class UMaterialInstanceDynamic;

USTRUCT(BlueprintType)
struct FSKGOpticSceneCaptureComponentRenderTargetSettings
{
	GENERATED_BODY()
	// The rate at which the scene capture updates
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticSceneCaptureComponent")
	float TickInterval {60.0f};
	// The resolution of the render target to be created for this scene capture
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticSceneCaptureComponent")
	int32 ResolutionX {512};
	// The resolution of the render target to be created for this scene capture
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticSceneCaptureComponent")
	int32 ResolutionY {512};
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticSceneCaptureComponent")
	TEnumAsByte<ETextureRenderTargetFormat> TextureRenderTargetFormat {RTF_RGBA16f};
};

USTRUCT(BlueprintType)
struct FSKGOpticSceneCaptureComponentMaterialSettings
{
	GENERATED_BODY()
	//This is the material parameter used with the TextureTarget/RenderTarget from the scene capture
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticSceneCaptureComponent")
	FName TextureTargetParameterName {"RenderTarget"};
	//This is the material parameter used with the Reticle Size 
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticSceneCaptureComponent")
	FName ReticleSizeParameterName {"ReticleSize"};
	//This is the material parameter used with the EyeboxRange (simulating the eyebox)
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticSceneCaptureComponent")
	FName EyeboxSensitivityParameterName {"EyeboxSensitivity"};
};

USTRUCT(BlueprintType)
struct FSKGOpticSceneCaptureComponentSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticSceneCaptureComponent")
	FSKGOpticSceneCaptureComponentRenderTargetSettings RenderTargetSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticSceneCaptureComponent")
	FSKGOpticSceneCaptureComponentMaterialSettings MaterialSettings;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SKGOPTIC_API USKGOpticSceneCaptureComponent : public USceneCaptureComponent2D, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

	struct FSKGOpticSceneCaptureComponentMagnificationSettings
	{
		bool bIsFirstFocalPlane {true};
		float StartingMagnification {1.0f};
		bool bSmoothZoom {true};
		float SmoothZoomRate {10.0f};
		bool bShrinkEyeboxWithMagnification {true};
		float ShrinkEyeboxMultiplier {1.0f};
	};

public:
	// Sets default values for this component's properties
	USKGOpticSceneCaptureComponent();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticSceneCaptureComponent|Initialize")
	FGameplayTagContainer GameplayTags;
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticSceneCaptureComponent|Settings")
	FSKGOpticSceneCaptureComponentSettings OpticSceneCaptureComponentSettings;

	FSKGOpticSceneCaptureComponentMagnificationSettings MagnificationSettings;
	FSKGOpticReticle CurrentReticleMaterial;
	FRotator DefaultRelativeRotation;
	float CurrentMagnification {1.0f};
	bool bShouldCapture {false};
	bool bInterpolateMagnification {false};
	float SmoothZoomTargetMagnification {1.0f};
	
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; }
	void SetupSceneCaptureComponent();

	// Returns true if Magnification equals target (used for smooth zoom, returns true otherwise)
	bool SetNewMagnification(const float Magnification);

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void SetupReticleMaterial(FSKGOpticReticle& ReticleMaterial);
	void Initialize(const bool IsFirstFocalPlane, const float StartingMagnification, const bool SmoothZoom, const float SmoothZoomRate, const bool ShrinkEyeboxWithMagnification, const float ShrinkEyeboxMultiplier);

	UFUNCTION(BlueprintCallable, Category = "SKGOpticSceneCaptureComponent|Tick")
	void StartCapture();
	UFUNCTION(BlueprintCallable, Category = "SKGOpticSceneCaptureComponent|Tick")
	void StopCapture();

	UFUNCTION(BlueprintCallable, Category = "SKGOpticSceneCaptureComponent|Zoom")
	void Zoom(const FSKGOpticReticle& ReticleMaterial, const float TargetMagnification);

	UFUNCTION(BlueprintCallable, Category = "SKGOpticSceneCaptureComponent|Zero")
	void PointOfImpactUpDownDefault();
	UFUNCTION(BlueprintCallable, Category = "SKGOpticSceneCaptureComponent|Zero")
	void PointOfImpactLeftRightDefault();
	UFUNCTION(BlueprintCallable, Category = "SKGOpticSceneCaptureComponent|Zero")
	void PointOfImpactUp(const double RotationAmount);
	UFUNCTION(BlueprintCallable, Category = "SKGOpticSceneCaptureComponent|Zero")
	void PointOfImpactDown(const double RotationAmount);
	UFUNCTION(BlueprintCallable, Category = "SKGOpticSceneCaptureComponent|Zero")
	void PointOfImpactLeft(const double RotationAmount);
	UFUNCTION(BlueprintCallable, Category = "SKGOpticSceneCaptureComponent|Zero")
	void PointOfImpactRight(const double RotationAmount);

	UFUNCTION(BlueprintCallable, Category = "SKGOptic|SceneCapture|Zero")
	void ApplyLookAtRotationZero(FRotator LookAtRotation);
};
