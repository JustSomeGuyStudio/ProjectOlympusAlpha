#pragma once

#include "DataTypes/SKGProceduralDataTypes.h"
#include "Engine/EngineTypes.h"
#include "SKGShooterFrameworkDataTypes.generated.h"

class UAnimSequence;
class USKGPDAFirearmStats;
class USKGFirearmAttachmentStatComponent;

USTRUCT(BlueprintType)
struct FSKGFirearmCollisionData
{
	GENERATED_BODY()
	bool bUseFirearmCollision {false};
	float CollisionStopAimingDistance {0.2f};
	UPROPERTY()
	TObjectPtr<UCurveVector> PoseLocation;
	UPROPERTY()
	TObjectPtr<UCurveVector> PoseRotation;
	float PoseScale {1.0f};
	float PoseLocationInterpSpeed {10.0f};
	float PoseRotationInterpSpeed {10.0f};
	TEnumAsByte<ECollisionChannel> CollisionChannel {ECC_Visibility};
	float TraceDiameter {2.0f};
	FTransform RootTransform {FTransform()};
	FTransform MuzzleTransform {FTransform()};
	FTransform MuzzleRelativeTransform {FTransform()};
	UPROPERTY()
	TArray<AActor*> ActorsToIgnoreForTrace;
};

USTRUCT(BlueprintType)
struct FSKGFirearmStats
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFirearmStats")
	float Weight {0.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFirearmStats")
	float Durability {0.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFirearmStats")
	float Ergonomics {0.0f};
	// Accuracy is in MOA
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFirearmStats")
	float Accuracy {0.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFirearmStats")
	float VerticalRecoil {0.0f}; // Percentage
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFirearmStats")
	float HorizontalRecoil {0.0f}; // Percentage
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFirearmStats")
	float MuzzleVelocityPercentage {0.0f}; // Percentage
	// Rounds per minute
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFirearmStats")
	float FireRate {0.0f};

	virtual ~FSKGFirearmStats() {}
	
	void SetFirearmDefaultStats(const USKGPDAFirearmStats* FirearmStats);
	// String conversion for display. Virtual to allow for overriding with children of this struct
	virtual FString ToString() const;
	
	bool operator == (const FSKGFirearmStats& RHS) const
	{
		return Weight == RHS.Weight && Durability == RHS.Durability && Ergonomics == RHS.Ergonomics && Accuracy == RHS.Accuracy && VerticalRecoil == RHS.VerticalRecoil && HorizontalRecoil == RHS.HorizontalRecoil && FireRate == RHS.FireRate;
	}

	FSKGFirearmStats& operator+=(const USKGFirearmAttachmentStatComponent* StatComponent);
	FSKGFirearmStats& operator-=(const USKGFirearmAttachmentStatComponent* StatComponent);
};

USTRUCT(BlueprintType)
struct FSKGProceduralStats
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float AimInterpolationRate {10.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float MovementSwayMultiplier {1.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FSKGCycleAimingPointSpringInterpSettings CycleAimingPointSpringInterpSettings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FSKGMovementLagSpringInterpSettings MovementLagSpringInterpSettings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float MovementLagInterpSetting {10.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FSKGRotationLagSpringInterpSettings RotationLagSpringInterpSettings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FSKGRotationLagInterpSettings RotationLagInterpSettings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FVector ControlRotationRecoilMultipliers = {FVector::OneVector};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FVector RecoilLocationMultipliers {FVector::OneVector};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FVector RecoilRotationMultipliers {FVector::OneVector};
};

USTRUCT()
struct FSKGProceduralAnimInstanceData
{
	GENERATED_BODY()
	
	FTransform AimOffset {FTransform()};
	float AimOffsetInterpSpeed {10.0f};
	FTransform OffHandIKOffset {FTransform()};
	bool bOffHandIKIsLeftHand {true};
	bool bInFreeLook {false};
	FRotator FreeLookStartRotation {FRotator::ZeroRotator};
	float LengthOfPull {0.0f};
	
	UPROPERTY()
	TObjectPtr<UAnimSequence> OffHandIKPose;
	FSKGLeanLeftRightSettings LeanLeftRightSettings;
	FSKGFirstAndThirdPersonBasePoseOffset BasePoseOffset;
	FSKGProceduralOffset ThirdPersonAimingOffset;
	FSKGCycleAimingPointSettings CycleAimingPointSettings;
	FSKGMovementSwaySettings MovementSwaySettings;
	FSKGMovementLagSettings MovementLagSettings;
	FSKGRotationLagSettings RotationLagSettings;
	FSKGDeadzoneSettings DeadzoneSettings;
	FSKGRecoilSettings RecoilSettings;
	FSKGFirearmCollisionData FirearmCollisionSettings;
	FVector2D MouseInput {FVector2D::ZeroVector};
	FSKGProceduralStats ProceduralStats;

	bool bProceduralAnimDataSet {false};

	operator bool () const
	{
		return bProceduralAnimDataSet;
	}
};

USTRUCT(BlueprintType)
struct FSKGProceduralShooterPawnData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFramework")
	FGameplayTag ProceduralAnimGameplayTag;
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFramework")
	bool bOffHandIKIsLeftHand {true};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFramework")
	FVector FreeLookLookAtLocation {FVector::ZeroVector};
};

USTRUCT(BlueprintType)
struct FSKGProceduralPoseReplicationData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFramework")
	FGameplayTag Tag;
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFramework")
	bool bExitPose {false};

	FSKGToFromCurveSettings PoseData;

	operator bool () const
	{
		return Tag.IsValid();
	}
};