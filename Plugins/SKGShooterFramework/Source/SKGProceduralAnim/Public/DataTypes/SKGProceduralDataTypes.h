#pragma once
#include "GameplayTagContainer.h"
#include "Curves/CurveVector.h"
#include "SKGProceduralDataTypes.generated.h"

class UCurveVector;
class UAnimationAsset;

USTRUCT(BlueprintType)
struct FSKGMinMax
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float Min {0.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float Max {0.0f};
	FSKGMinMax() {}
	FSKGMinMax(float Minimum, float Maximum)
	{
		Min = Minimum;
		Max =Maximum;
	}

	float GetRandom() const
	{
		return FMath::RandRange(Min, Max);
	}
};

USTRUCT(BlueprintType)
struct FSKGLeanLeftRightSettings
{
	GENERATED_BODY()
	// Whether or not to use movement sway (such as idle breathing and walking sway)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	bool bUseSpringInterp {true};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseSpringInterp"))
	float Stiffness {1.5f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseSpringInterp"))
	float CriticalDampingFactor {0.55f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseSpringInterp"))
	float Mass {0.015f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseSpringInterp"))
	float TargetVelocityAmount {0.0f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "!bUseSpringInterp"))
	float InterpSpeed {10.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float MaxLeanLeftAngle {45.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float MaxLeanRightAngle {45.0f};
};

USTRUCT(BlueprintType)
struct FSKGCycleAimingPointSpringInterpSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float LocationStiffness {12.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float LocationCriticalDampingFactor {0.65f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float LocationMass {0.05f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float LocationTargetVelocityAmount {0.0f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float RotationStiffness {18.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float RotationCriticalDampingFactor {0.6f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float RotationMass {0.05f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float RotationTargetVelocityAmount {0.0f};
};


// Settings used for when changing aiming points (such as going from one optic to another for aiming, or point aiming)
USTRUCT(BlueprintType)
struct FSKGCycleAimingPointSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	bool bUseSpringInterpForCyclingAimingPoint {true};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseSpringInterpForCyclingAimingPoint"))
	FSKGCycleAimingPointSpringInterpSettings SpringInterpSettings;
};

USTRUCT(BlueprintType)
struct FSKGMovementSwayLocationSettings
{
	GENERATED_BODY()
	// How much to add/reduce the sway provided from the movement sway curve
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float Multiplier {1.0f};
	// How fast it will interpolate to the new sway multiplier
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float MultiplierInterpolationRate {4.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	TObjectPtr<UCurveVector> Curve;
	// The speed in which it interpolates to a new value in the curve
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float InterpSpeed {10.0f};
};

USTRUCT(BlueprintType)
struct FSKGMovementSwaySettings
{
	GENERATED_BODY()
	// Whether or not to use movement sway (such as idle breathing and walking sway)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	bool bUseMovementSway {true};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseMovementSway"))
	FSKGMovementSwayLocationSettings LocationSettings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseMovementSway"))
	FSKGMovementSwayLocationSettings RotationSettings;
};

USTRUCT(BlueprintType)
struct FSKGMovementLagSpringInterpSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float SpringStiffness {6.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float SpringCriticalDampingFactor {0.45f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float SpringMass {0.05f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float SpringTargetVelocityAmount {1.0f};
};

USTRUCT(BlueprintType)
struct FSKGMovementLagSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	bool bUseMovementLag {true};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseMovementLag"))
	bool bUseSpringInterpolation {true};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseMovementLag && bUseSpringInterpolation"))
	FSKGMovementLagSpringInterpSettings SpringInterpSettings;
	
	// The max amount the procedurals will move left/right
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseMovementLag"))
	float LeftRightMax {100.0f};
	// The max amount the procedurals will move up/down
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseMovementLag"))
	float UpDownMax {100.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseMovementLag"))
	FRotator Multiplier {FRotator(1.0f, 1.0f, 1.0f)};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseMovementLag && !bUseSpringInterpolation"))
	float InterpSpeed {8.0f};
};

USTRUCT(BlueprintType)
struct FSKGRotationLagSpringInterpSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float RotationSpringStiffness {24.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float RotationSpringCriticalDampingFactor {0.6f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float RotationSpringMass {0.1f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float RotationSpringTargetVelocityAmount {0.5f};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float LocationSpringStiffness {12.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float LocationSpringCriticalDampingFactor {0.65f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float LocationSpringMass {0.05f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float LocationSpringTargetVelocityAmount {0.0f};
};

USTRUCT(BlueprintType)
struct FSKGRotationLagInterpSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float RotationInterpSpeed {10.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float LocationInterpSpeed {10.0f};
};

USTRUCT(BlueprintType)
struct FSKGRotationLagSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	bool bUseRotationLag {true};
	// I advise this to be false if you are using deadzone, in my opinion it gives a better result
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseRotationLag"))
	bool bUseSpringInterpolation {true};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseRotationLag && bUseSpringInterpolation"))
	FSKGRotationLagSpringInterpSettings SpringInterpSettings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseRotationLag && !bUseSpringInterpolation"))
	FSKGRotationLagInterpSettings InterpSettings;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseRotationLag"))
	FRotator MaxRotation {FRotator(10.0f, 10.0f, 10.0f)};
	// Default in BP: X = Roll, Y = Pitch, Z = Yaw
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseRotationLag"))
	FRotator RotationMultiplier {FRotator(0.5f, 0.5f, 0.5f)};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseRotationLag"))
	FVector LocationMultiplier {FVector(0.25f, 0.25f, 0.25f)};
};

USTRUCT(BlueprintType)
struct FSKGRecoilSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	TObjectPtr<UCurveVector> ControlRotationCurve;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "ControlRotationCurve != nullptr"))
	FSKGMinMax ControlPitchRandom {FSKGMinMax(1.0f, 2.0f)};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "ControlRotationCurve != nullptr"))
	FSKGMinMax ControlYawRandom {FSKGMinMax(1.0f, 2.0f)};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "ControlRotationCurve != nullptr"))
	FSKGMinMax ControlRollRandom {FSKGMinMax(1.0f, 2.0f)};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	TObjectPtr<UCurveVector> LocationCurve;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "LocationCurve != nullptr"))
	float LocationInterpToNoneSpeed {8.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "LocationCurve != nullptr"))
	FSKGMinMax XRandom {FSKGMinMax(1.0f, 2.0f)};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "LocationCurve != nullptr"))
	FSKGMinMax YRandom {FSKGMinMax(1.0f, 2.0f)};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "LocationCurve != nullptr"))
	FSKGMinMax ZRandom {FSKGMinMax(1.0f, 2.0f)};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	TObjectPtr<UCurveVector> RotationCurve;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "RotationCurve != nullptr"))
	float RotationInterpToNoneSpeed {8.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "RotationCurve != nullptr"))
	FSKGMinMax PitchRandom {FSKGMinMax(-1.0f, 3.0f)};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "RotationCurve != nullptr"))
	FSKGMinMax YawRandom {FSKGMinMax(1.0f, 2.0f)};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "RotationCurve != nullptr"))
	FSKGMinMax RollRandom {FSKGMinMax(-12.0f, 12.0f)};
};

USTRUCT(BlueprintType)
struct FSKGProceduralAimingSettings
{
	GENERATED_BODY()
	/*
	 * The forward axis of the firearm (NOT the attachment) which is used for correcting the aiming distance
	 * and it must be set on all attachments used to aim
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	TEnumAsByte<EAxis::Type> ForwardAxis {EAxis::Type::Y};
	// The higher the value the further the aim point will be from your camera
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float AimingDistance {10.0f};
	/*
	 * If true, the position of the aim point is discarded and will the aim point to be exactly
	 * the AimingDistance behind the camera
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	bool bUseFixedAimingDistance {false};
	// This is the base speed used for aiming. If you have a firearm with an optic, the value for the firearm is used.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float DefaultAimingSpeed {10.0f};
	// If false, this aiming device will not be used when cycling aiming devices
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	bool bCanAim {true};
};

USTRUCT(BlueprintType)
struct FSKGDeadzoneSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	bool bUseDeadzone {false};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseDeadzone"))
	float PitchRate {0.6f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseDeadzone"))
	float YawRate {0.6f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseDeadzone"))
	float PitchLimit {5.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseDeadzone"))
	float YawLimit {10.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseDeadzone"))
	float InterpolationSpeed {10.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseDeadzone"))
	bool bDisableDeadzoneWhenAiming {false};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bUseDeadzone && bDisableDeadzoneWhenAiming"))
	float DeadzoneAimingDisableInterpolationSpeed {6.0f};
};

USTRUCT(BlueprintType)
struct FSKGProceduralOffset
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FVector Location {FVector::ZeroVector};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FRotator Rotation {FRotator::ZeroRotator};
};

USTRUCT(BlueprintType)
struct FSKGFirstAndThirdPersonBasePoseOffset
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FSKGProceduralOffset FirstPersonOffset;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FSKGProceduralOffset ThirdPersonOffset;
};

USTRUCT(BlueprintType)
struct FSKGCurve
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	TObjectPtr<UCurveVector> LocationCurve;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	TObjectPtr<UCurveVector> RotationCurve;
	// Rate/Speed at which the curves play
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float PlayRate {4.0f};
	/**
	 * Time for when the curve is finished. For example, if your in high port and going back to
	 * none (normal idle curve with firearm), your CurveEndTime should be the Time in the curve
	 * in which the Values in the CurveLocation AND CurveRotation are 0
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	float CurveEndTime {1.0f};

	bool GetVectorValues(const float Time, FVector& Location, FRotator& Rotation, bool bOffHandIKIsLeftHand) 
	{
		bool bValid = false;
		if (LocationCurve)
		{
			Location = LocationCurve->GetVectorValue(Time);
			Location.X *= bOffHandIKIsLeftHand ? 1.0 : -1.0;
			bValid = true;
		}
		if (RotationCurve)
		{
			const FVector RotationVector = RotationCurve->GetVectorValue(Time);
			Rotation = FRotator(RotationVector.X, RotationVector.Y, RotationVector.Z);
			Rotation.Pitch *= bOffHandIKIsLeftHand ? 1.0 : -1.0;
			Rotation.Yaw *= bOffHandIKIsLeftHand ? 1.0 : -1.0;
			bValid = true;
		}
		return bValid;
	}

	operator bool () const
	{
		return LocationCurve || RotationCurve;
	}
};

USTRUCT(BlueprintType)
struct FSKGFirstAndThirdPersonCurveSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FSKGCurve FirstPersonCurve;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FSKGCurve ThirdPersonCurve;

	operator bool () const
	{
		return FirstPersonCurve || ThirdPersonCurve;
	}
};

USTRUCT(BlueprintType)
struct FSKGToFromCurveSettings
{
	GENERATED_BODY()
	// Going from none to the curve
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FSKGFirstAndThirdPersonCurveSettings ToCurve;
	// Going from the curve to none
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FSKGFirstAndThirdPersonCurveSettings FromCurve;
	// Tag for this curve. This is used to recognize what curve this is.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FGameplayTag CurveTag;
	// True will interpolate the curve (not follow the graph strictly, but add some additional blending
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	bool bInterpolate {false};
	// Will interpolate for the first X amount of time before it stops interpolation
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bInterpolate"))
	float InterpolationTime {3.0f};
	// Speed as which the curve is interpolated between. This will allow for some general smoothing while following the curve
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bInterpolate"))
	float InterpolationSpeed {10.0f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bInterpolate"))
	float InterpolationSpeedMultiplier {5.0f};

	operator bool () const
	{
		return (ToCurve && FromCurve) && CurveTag.IsValid();
	}
};

USTRUCT(BlueprintType)
struct FSKGCurveReplicationSettings
{
	GENERATED_BODY()
	// True replicate the playing of the curve
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	bool bReplicateCurve {false};
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural", meta = (EditCondition = "bReplicateCurve"))
	bool bReliable {false};
};

USTRUCT(BlueprintType)
struct FSKGCurveSettings
{
	GENERATED_BODY()
	// Going from none to the curve
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FSKGFirstAndThirdPersonCurveSettings Curve;
	// Replication settings for playing the curve (Not replicated by default)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGProcedural")
	FSKGCurveReplicationSettings ReplicationSettings;

	operator bool () const
	{
		return Curve;
	}
};