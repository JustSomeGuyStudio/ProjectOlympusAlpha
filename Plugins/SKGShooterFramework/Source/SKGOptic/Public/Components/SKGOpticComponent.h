// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "Components/ActorComponent.h"
#include "DataTypes/SKGOpticCoreDataTypes.h"
#include "SKGOpticComponent.generated.h"

class UMeshComponent;
class UMaterialInstanceDynamic;
class USKGOpticSceneCaptureComponent;

UENUM(BlueprintType)
enum class ESKGOpticAdjustmentType : uint8
{
	MOA		UMETA(DisplayName = "MOA"),
	MRAD	UMETA(DisplayName = "MRAD")
};

UENUM(BlueprintType)
enum class ESKGMRADAdjustment : uint8
{
	PointOne	UMETA(DisplayName = ".1")
};

UENUM(BlueprintType)
enum class ESKGMOAAdjustment : uint8
{
	OneEighth	UMETA(DisplayName = "1/8"),
	OneQuarter	UMETA(DisplayName = "1/4"),
	OneHalf		UMETA(DisplayName = "1/2"),
	One			UMETA(DisplayName = "1")
};

USTRUCT(BlueprintType)
struct FSKGOpticStartingZeroSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticComponent")
	bool bStartWithDefaultZero {false};
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticComponent", meta = (EditCondition = "bStartWithDefaultZero"))
	int32 DefaultElevationClicks {25};
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticComponent", meta = (EditCondition = "bStartWithDefaultZero"))
	int32 DefaultWindageClicks {25};
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticComponent", meta = (EditCondition = "!bStartWithDefaultZero"))
	bool bStartWithRandomZero {false};
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticComponent", meta = (EditCondition = "bStartWithRandomZero && !bStartWithDefaultZero"))
	int32 RandomMaxElevationClicks {25};
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticComponent", meta = (EditCondition = "bStartWithRandomZero && !bStartWithDefaultZero"))
	int32 RandomMaxWindageClicks {25};
};

USTRUCT(BlueprintType)
struct FSKGOpticZeroSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticComponent")
	ESKGOpticAdjustmentType AdjustmentType {ESKGOpticAdjustmentType::MRAD};
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticComponent", meta = (EditCondition = "AdjustmentType == ESKGOpticAdjustmentType::MOA", EditConditionHides))
	ESKGMOAAdjustment MOAAdjustmentAmount = ESKGMOAAdjustment::OneQuarter;
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticComponent", meta = (EditCondition = "AdjustmentType == ESKGOpticAdjustmentType::MRAD", EditConditionHides))
	ESKGMRADAdjustment MRADAdjustmentAmount = ESKGMRADAdjustment::PointOne;
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticComponent")
	FSKGOpticStartingZeroSettings StartingZeroSettings;
	
private:
	double MRADAdjustment {0.0572958};
	double MOAAdjustment {0.01666667};
	double AdjustmentAmount {0.0};
	
public:
	// Needs to be called by BeginPlay to initialize the given settings
	void Initialize()
	{
		if (AdjustmentType == ESKGOpticAdjustmentType::MRAD)
		{
			switch (MRADAdjustmentAmount)
			{
			case ESKGMRADAdjustment::PointOne :
				{
					AdjustmentAmount = MRADAdjustment * 0.1f;
					break;
				}
			}
		}
		else
		{
			switch (MOAAdjustmentAmount)
			{
			case ESKGMOAAdjustment::OneQuarter :
				{
					AdjustmentAmount = MOAAdjustment * 0.25;
					break;
				}
			case ESKGMOAAdjustment::OneHalf :
				{
					AdjustmentAmount = MOAAdjustment * 0.5;
					break;
				}
			case ESKGMOAAdjustment::One :
				{
					AdjustmentAmount = MOAAdjustment;
					break;
				}
			case ESKGMOAAdjustment::OneEighth :
				{
					AdjustmentAmount = MOAAdjustment * 0.125;
					break;
				}
			}
		}
	}

	double GetAdjustmentAmount() const
	{
		return AdjustmentAmount;
	}
};

USTRUCT(BlueprintType)
struct FSKGOpticReticleSettings
{
	GENERATED_BODY()
	// Material Index that your reticle is on
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic")
	int32 ReticleMaterialIndex {0};
	// Array of reticle materials to cycle through
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic")
	TArray<FSKGOpticReticle> ReticleMaterials;
	// Material to use when you are not aiming. Used only with magnified optics
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic")
	FSKGOpticReticle UnAimedReticleMaterial;
	// When you stop aiming, how long before the capture stops and reticle change happens. Used only with magnified optics
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic")
	float UnAimedCaptureDelay {2.0f};
	// Parameter name in the material that controls reticle brightness
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic")
	FName ReticleBrightnessParameterName {"ReticleBrightness"};
	// Brightness settings for default mode
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic")
	TArray<float> ReticleBrightnessSettings {1.0f};
	// Brightness settings for the night vision mode
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic")
	TArray<float> ReticleNightVisionBrightnessSettings;

	FTimerHandle UnAimedTimerHandle;
	
private:
	uint8 CurrentReticleIndex {0};
	bool bUsingReticleNightVisionBrightness {false};
	uint8 CurrentReticleBrightnessIndex {0};
	uint8 CurrentReticleNightVisionBrightnessIndex {0};

public:
	void ConstructDynamicMaterials(UObject* WorldContextObject, USKGOpticSceneCaptureComponent* OptionalOpticSceneCaptureComponent);
	
	void CycleReticleMaterial()
	{
		if (++CurrentReticleIndex >= ReticleMaterials.Num())
		{
			CurrentReticleIndex = 0;
		}
	}

	FSKGOpticReticle GetReticleMaterial()
	{
		if (ReticleMaterials.Num() > 0)
		{
			return ReticleMaterials[CurrentReticleIndex];
		}
		return FSKGOpticReticle();
	}
	
	// Returns true if brightness changed
	bool IncreaseReticleBrightnessSetting();
	// Returns true if brightness changed
	bool DecreaseReticleBrightnessSetting();

	float GetReticleBrightness() const
	{
		if (bUsingReticleNightVisionBrightness)
		{
			return ReticleNightVisionBrightnessSettings[CurrentReticleNightVisionBrightnessIndex];
		}
		return ReticleBrightnessSettings[CurrentReticleBrightnessIndex];
	}

	void CycleReticleNightVisionMode()
	{
		if (ReticleNightVisionBrightnessSettings.Num())
		{
			bUsingReticleNightVisionBrightness = !bUsingReticleNightVisionBrightness;
		}
	}
};

USTRUCT(BlueprintType)
struct FSKGOpticMagnificationSettings
{
	GENERATED_BODY()
	// Used in conjunction with SKGOpticSceneCaptureComponent
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic")
	TArray<float> Magnifications {1.0f};
	// If true, zooming in/out will be handled smoothly
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic")
	bool bSmoothZoom {true};
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic", meta = (EditCondition = "bSmoothZoom"))
	float SmoothZoomRate {10.0f};
	// Used in conjunction with SKGOpticSceneCaptureComponent. Allows the eyebox to shrink by a given amount as you zoon in
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic")
	bool bShrinkEyeboxWithMagnification {true};
	// Used in conjunction with SKGOpticSceneCaptureComponent. Amount to shrink the eyebox by as you zoom in
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic", meta = (EditCondition = "bShrinkEyeboxWithMagnification"))
	float ShrinkEyeboxMultiplier {0.5f};
	/**
	 * If true the optic will be first focal plane and the reticle will scale as you zoom in/out
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SKGOpticSceneCaptureComponent")
	bool bIsFirstFocalPlane {true};

private:
	uint8 CurrentMagnificationIndex {0};

public:
	// Returns true if Current Magnification changed
	bool SetNextMagnification()
	{
		if (++CurrentMagnificationIndex >= Magnifications.Num())
		{
			CurrentMagnificationIndex = Magnifications.Num() - 1;
			return false;
		}
		return true;
	}

	// Returns true if Current Magnification changed
	bool SetPreviousMagnification()
	{
		if (!CurrentMagnificationIndex - 1 < 0)
		{
			--CurrentMagnificationIndex;
			return true;
		}
		return false;
	}
	
	float GetCurrentMagnification() const
	{
		return Magnifications[CurrentMagnificationIndex];
	}

	float GetCurrentMagnificationFOV() const;
};

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SKGOPTIC_API USKGOpticComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USKGOpticComponent();

protected:
	// The name of the mesh that is used for the optic itself (contains the reticle)
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic|Initialize")
	FName OpticMeshName {"StaticMesh"};
	// Optional Scene Capature Component that gets controlled through this class
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic|Initialize")
	FName OpticSceneCaptureComponentName {"SKGOpticSceneCapture"};
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic|Initialize")
	FGameplayTagContainer GameplayTags;
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic|Settings")
	FSKGOpticReticleSettings ReticleSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic|Settings")
	FSKGOpticMagnificationSettings MagnificationSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic|Settings")
	FSKGOpticZeroSettings OpticZeroSettings;
	// Useful if your NetUpdateFrequency is set super low
	UPROPERTY(EditDefaultsOnly, Category = "SKGOptic|Settings")
	bool bAutoCallForceNetUpdate {true};
	
	UPROPERTY(BlueprintGetter = GetOpticMesh, Category = "SKGOptic|Mesh")
	TObjectPtr<UMeshComponent> OpticMesh;
	UPROPERTY()
	TObjectPtr<USKGOpticSceneCaptureComponent> OpticSceneCaptureComponent;

	int32 ZeroUpDownClicks {0};
	int32 ZeroLeftRightClicks {0};
	
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; }
	FORCEINLINE bool HasAuthority() const { return GetOwnerRole() == ROLE_Authority; }
	FORCEINLINE void TryForceNetUpdate() const;
	void SetComponents();
	void SetStartingZero();
	void UpdateOpticMaterialInstance();
	void UpdateReticleBrightness();

	void StartSceneCapture();
	void StopSceneCapture();
	
public:
	// Should only be used when manually setting the value for construction
	void SetOpticMeshName(const FName& Name) { OpticMeshName = Name; }
	// Should only be used when manually setting the value for construction
	void SetOpticSceneCaptureComponentName(const FName& Name) { OpticSceneCaptureComponentName = Name; }
	
	/**
	 * @return true, this optic is magnified
	 */
	UFUNCTION(BlueprintPure, Category = "SKGOptic")
	bool IsMagnifiedOptic() const { return OpticSceneCaptureComponent != nullptr; }
	// Cycles through the set reticles based on the given settings
	UFUNCTION(BlueprintCallable, Category = "SKGOptic|Reticle")
	void CycleReticle();
	// Increases your reticle brightness based on the given settings
	UFUNCTION(BlueprintCallable, Category = "SKGOptic|Reticle")
	void IncreaseReticleBrightness();
	// Decreases your reticle brightness based on the given settings
	UFUNCTION(BlueprintCallable, Category = "SKGOptic|Reticle")
	void DecreaseReticleBrightness();
	// Toggles the night vision setting on the optic (bright to not bright)
	UFUNCTION(BlueprintCallable, Category = "SKGOptic|Reticle")
	void ToggleReticleNightVisionSetting();

	// Zom your optic in
	UFUNCTION(BlueprintCallable, Category = "SKGOptic|Zoom")
	void ZoomIn();
	// Zoom your optic out
	UFUNCTION(BlueprintCallable, Category = "SKGOptic|Zoom")
	void ZoomOut();
	/**
	 * @return the current magnification of the optic
	 */
	UFUNCTION(BlueprintPure, Category = "SKGOptic|Zoom")
	float GetCurrentMagnification() const;

	/* Sets the point of impact for elevation back to default
	 * @return the amount of clicks required for turrent to return back to 0
	 */
	UFUNCTION(BlueprintCallable, Category = "SKGOptic|Zero")
	int32 PointOfImpactUpDownDefault();
	/* Sets the point of impact for windage back to default
	* @return the amount of clicks required for turrent to return back to 0
	*/
	UFUNCTION(BlueprintCallable, Category = "SKGOptic|Zero")
	int32 PointOfImpactLeftRightDefault();
	/**
	 * Adjust your zero so your point of impact moves up
	 * @param Clicks How many clicks of the turret
	 */
	UFUNCTION(BlueprintCallable, Category = "SKGOptic|Zero")
	void PointOfImpactUp(const int32 Clicks = 1);
	/**
	 * Adjust your zero so your point of impact moves Down
	 * @param Clicks How many clicks of the turret
	 */
	UFUNCTION(BlueprintCallable, Category = "SKGOptic|Zero")
	void PointOfImpactDown(const int32 Clicks = 1);
	/**
	 * Adjust your zero so your point of impact moves Left
	 * @param Clicks How many clicks of the turret
	 */
	UFUNCTION(BlueprintCallable, Category = "SKGOptic|Zero")
	void PointOfImpactLeft(const int32 Clicks = 1);
	/**
	 * Adjust your zero so your point of impact moves Right
	 * @param Clicks How many clicks of the turret
	 */
	UFUNCTION(BlueprintCallable, Category = "SKGOptic|Zero")
	void PointOfImpactRight(const int32 Clicks = 1);

	UFUNCTION(BlueprintCallable, Category = "SKGOptic|Zero")
	void ApplyLookAtRotationZero(const FRotator& LookAtRotation);
	
	UFUNCTION(BlueprintGetter, Category = "SKGOptic")
	UMeshComponent* GetOpticMesh() const { return OpticMesh; }

	UFUNCTION(BlueprintCallable, Category = "SKGOptic")
	void StartedAiming();
	UFUNCTION(BlueprintCallable, Category = "SKGOptic")
	void StoppedAiming();
};
