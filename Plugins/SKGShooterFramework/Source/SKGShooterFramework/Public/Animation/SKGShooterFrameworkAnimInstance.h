// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Components/SKGFirearmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "SKGShooterFrameworkAnimInstance.generated.h"

DECLARE_STATS_GROUP(TEXT("SKGShooterFrameworkAnimInstanceStatGroup"), STATGROUP_SKGShooterFrameworkAnimInstance,
					STATCAT_Advanced);

class USKGFirearmComponent;
class USKGProceduralAnimComponent;
class USKGShooterPawnComponent;
struct FTraceHandle;
struct FTraceDatum;

DECLARE_DELEGATE_TwoParams(FFirearmTraceDelegate, const FTraceHandle&, FTraceDatum&);

UCLASS()
class SKGSHOOTERFRAMEWORK_API USKGShooterFrameworkAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:	
	UFUNCTION(BlueprintImplementableEvent, Category = "SKGShooterFrameworkAnimInstance|Events")
	void OnHeldActorSet(AActor* NewHeldActor, AActor* OldHeldActor);
	virtual void OnHeldActorSet_Implementation(AActor* NewHeldActor, AActor* OldHeldActor) {}
	
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	void SetupMandatoryData();
	void SetupInitialData();
	void HandlePawnVelocity();
	void HandleOffsets(float DeltaSeconds);
	void SetCameraOffset(const FTransform& Offset);

	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Components")
	TObjectPtr<USKGShooterPawnComponent> ShooterPawnComponent;
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|LocallyControlled")
	float IsLocallyControlledAlpha {0.0f};

	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterFrameworkAnimInstance|Settings|FirearmCollision")
	FName FirearmCollisionBoneName {"VB FirearmCollision"};
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterFrameworkAnimInstance|Settings|OffhandIK")
	FName OffhandIKCurveName {"OffhandIKCurveAlpha"};
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterFrameworkAnimInstance|Settings|Pose")
	float DefaultPoseInterpolationSpeedMultiplier {20.0f};
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterFrameworkAnimInstance|Settings|Aiming")
	FName AimCurveName {"AimCurveAlpha"};
	
	FSKGProceduralAnimInstanceData ProceduralAnimData;

	UPROPERTY()
	TObjectPtr<AActor> CurrentActor;
	UPROPERTY()
	TObjectPtr<USKGFirearmComponent> CurrentFirearmComponent;
	UPROPERTY()
	TObjectPtr<USKGProceduralAnimComponent> CurrentProceduralAnimComponent;

	FVector PawnVelocity {FVector::ZeroVector};
	float PawnHorizontalVelocity {0.0f};
	FRotator ControlRotation {FRotator::ZeroRotator};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|LocallyControlled")
	bool bIsLocallyControlled {false};
	bool bUseFirstPerson {false};

#pragma region ComponentSpaceValues
	// Rotation/Movement Lag, Deadzone
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|ComponentSpace")
	FVector ComponentSpaceFinalLocation {FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|ComponentSpace")
	FRotator ComponentSpaceFinalRotation {FRotator::ZeroRotator};
#pragma  endregion ComponentSpaceValues

#pragma region ParentBoneSpaceValues
	// Collision, Recoil, CustomCurve
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|ComponentSpace")
	FVector ParentBoneSpaceFinalLocation {FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|ComponentSpace")
	FRotator ParentBoneSpaceFinalRotation {FRotator::ZeroRotator};
#pragma  endregion ParentBoneSpaceValues
	
#pragma region Spine
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterFrameworkAnimInstance|Settings|Spine")
	bool bUseProceduralSpineUpDown {true};
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterFrameworkAnimInstance|Settings|Spine")
	bool bUseProceduralSpineLeftRight {true};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Spine")
	FRotator SpineLeanLeftRight {FRotator::ZeroRotator};
	float TargetSpineLeanLeftRightAngle {0.0f};
	FFloatSpringState LeanLeftRightSpringState;
	
	// Amount to blend spine bones by (lower spine bones can bend less than upper spine bones). Make sure total value equals 1 and count is <= 4
	FRotator SpineRotationUpDownToInterpTo {FRotator::ZeroRotator};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Spine")
	FRotator SpineRotationUpDown {FRotator::ZeroRotator};
	FRotator OldSpineControlRotation {FRotator::ZeroRotator};
	FRotator SpineRotationToInterpTo {FRotator::ZeroRotator};
	void HandleProceduralSpine(float DeltaSeconds);
#pragma endregion Spine

	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Offsets")
	FVector BasePoseOffsetLocation {FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Offsets")
	FRotator BasePoseOffsetRotation {FRotator::ZeroRotator};
	
#pragma region Aiming
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Aim")
	bool bIsTryingToAim {false};
	bool bIsAiming {false};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Aim")
	float AimAlpha {0.0f};
	// Used for controlling how much movement/rotation a montage has while aiming
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Aim")
	float AimCurveAlpha {0.0f};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Aim")
	FRotator HeadStraightRotation {FRotator(90.0f, 0.0f, 0.0f)};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Aim")
	FVector CameraOffsetLocation {FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Aim")
	FRotator CameraOffsetRotation {FRotator::ZeroRotator};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Aim")
	FVector SightOffsetLocation {FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Aim")
	FRotator SightOffsetRotation {FRotator::ZeroRotator};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Aim|Offsets")
	FVector ThirdPersonAimingOffsetLocation {FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Aim|Offsets")
	FRotator ThirdPersonAimingOffsetRotation {FRotator::ZeroRotator};
	
	FVectorSpringState AimingLocationSpringState;
	FQuaternionSpringState AimingRotationSpringState;

	float TargetAimAlpha {0.0f};
	bool bInterpAimAlpha {false};

	void SetAimingData();
	void HandleAiming(float DeltaSeconds);
#pragma endregion Aiming

#pragma region Sway
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|MovementSway")
	FVector MovementSwayLocation {FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|MovementSway")
	FRotator MovementSwayRotation {FRotator::ZeroRotator};
	FVector MovementSwayCurveLocation {FVector::ZeroVector};
	FVector MovementSwayCurveRotation {FVector::ZeroVector};
	float MovementSwayLocationMultiplier {1.0f};
	float MovementSwayRotationMultiplier {1.0f};
	float MovementSwayVelocityMultiplier {1.0f};
	float MovementSwayAccumulator {0.0f};
	void SetMovementSwayData(float DeltaSeconds);
	void HandleMovementSway(float DeltaSeconds);
#pragma endregion Sway

#pragma region MovementLag
	// COMPONENT SPACE
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|MovementLag")
	FRotator MovementLagRotation {FRotator::ZeroRotator};
	FQuaternionSpringState MovementLagSpringState;
	void HandleMovementLag(float DeltaSeconds);
#pragma endregion MovementLag

#pragma region RotationLag
	// COMPONENT SPACE
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|RotationLag")
	FVector RotationLagLocation {FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|RotationLag")
	FRotator RotationLagRotation {FRotator::ZeroRotator};
	FRotator OldControlRotation {FRotator::ZeroRotator};
	FRotator UnalteredRotationLagRotation {FRotator::ZeroRotator};

	FVectorSpringState RotationLagLocationSpringState;
	FQuaternionSpringState RotationLagRotationSpringState;
	void HandleRotationLag(float DeltaSeconds);
#pragma endregion RotationLag

#pragma region Deadzone
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|DeadzoneAiming")
	FRotator DeadzoneHandRotation {FRotator::ZeroRotator};
	float DeadzoneYaw {0.0f};
	float DeadzonePitch {0.0f};

	void HandleDeadzone(float DeltaSeconds);
#pragma endregion Deadzone
	
#pragma region OffHandIK
	UPROPERTY(BlueprintReadWrite, Category = "SKGShooterFrameworkAnimInstance|OffHandIK")
	float OffHandIKAlpha {0.0f};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|OffHandIK")
	FVector OffHandIKLocation {FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|OffHandIK")
	TObjectPtr<UAnimSequence> OffHandIKPose;
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|OffHandIK")
	bool bOffHandIKIsLeftHand {true};
#pragma endregion OffHandIK

#pragma region Recoil
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Recoil")
	FVector RecoilLocation {FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Recoil")
	FRotator RecoilRotation {FRotator::ZeroRotator};
	FVector ControlRotationRecoilTarget {FVector::ZeroVector};
	FVector RecoilLocationTarget {FVector::ZeroVector};
	FVector RecoilRotationTarget {FVector::ZeroVector};
	float RecoilStartTimeStamp {0.0f};
	
	FVector RecoilControlRotationMultiplier {FVector::OneVector};
	FVector RecoilLocationMultiplier {FVector::OneVector};
	FVector RecoilRotationMultiplier {FVector::OneVector};
	
	bool bPerformingRecoil {false};
	FVector RandomRecoilLocation {FVector::ZeroVector};
	FVector RandomRecoilRotation {FVector::ZeroVector};
	FVector RandomControlRotationRecoil {FVector::ZeroVector};
	FRotator ControlRotationRecoilTargetRot {FRotator::ZeroRotator};
	FRotator AccumulatedControlRotationRecoil {FRotator::ZeroRotator};
	FRotator FreeLookRecoilRotation = {FRotator::ZeroRotator};
	
	void SetRecoilData();
	void InterpRecoilToNone(float DeltaSeconds);
	void HandleRecoil(float DeltaSeconds);
#pragma endregion Recol

#pragma region LengthOfPull
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|MovementLag")
	float LengthOfPull {0.0};
	//void HandleLengthOfPull(float DeltaSeconds);
#pragma endregion

#pragma region Curve
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Pose")
	FVector PoseLocation {FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|Pose")
	FRotator PoseRotation {FRotator::ZeroRotator};

	FVector TargetPoseLocation {FVector::ZeroVector};
	FRotator TargetPoseRotation {FRotator::ZeroRotator};

	bool bReachedEndOfPoseCurve {false};
	bool bInterpolatePose {false};
	float PoseStartTime {0.0f};
	float PoseCurveTime {0.0f};
	float PoseInterpolationSpeed {0.0f};
	float PostInterpolationTimeStart {0.0f};
	bool bInterpolatePoseTransform {false};
	
	FSKGToFromCurveSettings CurrentPoseData;
	FSKGCurve CurrentPose;
	void SetPoseData(float DeltaSeconds);
	void HandlePose(float DeltaSeconds);
#pragma endregion Curve

#pragma region FirearmCollision
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|FirearmCollision")
	FVector FirearmCollisionLocation {FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|FirearmCollision")
	FRotator FirearmCollisionRotation {FRotator::ZeroRotator};

	FVector FirearmCollisionTraceStart {FVector::ZeroVector};
	FVector FirearmCollisionTraceEnd {FVector::ZeroVector};
	FVector FirearmCollisionTargetLocation {FVector::ZeroVector};
	FVector FirearmCollisionTargetRotation {FVector::ZeroVector};
	
	FFirearmTraceDelegate FirearmTraceDelegate;
	void OnFirearmTraceComplete(const FTraceHandle& TraceHandle, FTraceDatum& TraceData);
	bool bFirearmCollisionAllowTrace {true};
	
	bool bFirearmCollisionHit {false};
	FVector FirearmCollisionHitLocation {FVector::ZeroVector};
	float FirearmCollisionPushAmount {0.0f};
	void SetFirearmCollisionData();
	void HandleFirearmCollision(float DeltaSeconds);

	bool bSettingTestPose {false};
#pragma endregion FirearmCollision

#pragma region CustomCurve
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|CustomCurve")
	FVector CustomCurveLocation {FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|CustomCurve")
	FRotator CustomCurveRotation {FRotator::ZeroRotator};

	bool bPlayCustomCurve {false};
	float CustomCurveStartTime {0.0f};
	
	FSKGCurve CurrentCustomCurve;
	void SetCustomCurveData(float DeltaSeconds);
#pragma endregion CustomCurve
	
public:
	void SetupShooterPawnComponent();
	const FRotator& GetFreeLookRecoilModifiedRotation() const { return FreeLookRecoilRotation; }
	/**
	 * Performs the procedural recoil set by the RecoilData (typically on the FirearmComponent)
	 * @param Multiplier How much to increase/decrease the recoil
	 */
	void PerformRecoil(const FRotator& ControlRotationMultiplier = FRotator(1.0, 1.0, 1.0), const FVector& LocationMultiplier = FVector(1.0), const FRotator& RotationMultiplier = FRotator(1.0, 1.0, 1.0));
	/**
	 * @param PoseData Pose to go to/from with all the information about how to get there
	 * @param ExitPose If true it will use the FromPoseCurve to go back to 0 (if non existent, it will just interpolate to 0), otherwise it will use ToPoseCurve
	 */
	void TryPerformPose(const FSKGToFromCurveSettings& PoseData, const bool ExitPose);
	UFUNCTION(BlueprintCallable, Category = "SKGShooterFrameworkAnimInstance")
	void PerformCustomCurve(const FSKGFirstAndThirdPersonCurveSettings& CurveData);
	
	UPROPERTY(BlueprintReadOnly, Category = "SKGShooterFrameworkAnimInstance|CustomCurve")
	FRotator TestHitRotation {FRotator::ZeroRotator};
	FRotator TestHitTargetRotation {FRotator::ZeroRotator};
	UFUNCTION(BlueprintCallable, Category = "SKGShooterFrameworkAnimInstance")
	void TestHit(const FHitResult& HitResult);
	
	UFUNCTION(BlueprintCallable, Category = "SKGEDITORONLY")
	void SetTestPose(const FVector& Location, const FVector& Rotation, bool bFirearmCollision);
	UFUNCTION(BlueprintCallable, Category = "SKGEDITORONLY")
	void GetCurrentPoseValues(FVector& Location, FVector& Rotation, bool bFirearmCollision);
};
