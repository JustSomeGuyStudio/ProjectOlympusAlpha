// Copyright 2023, Dakota Dawe, All rights reserved

#include "Animation/SKGShooterFrameworkAnimInstance.h"
#include "Components/SKGProceduralAnimComponent.h"
#include "Components/SKGShooterPawnComponent.h"
#include "Statics/SKGShooterFrameworkHelpers.h"

#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Pawn.h"
#include "Curves/CurveVector.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"

DECLARE_CYCLE_STAT(TEXT("NativeUpdateAnimation"), STAT_SKGNativeUpdate, STATGROUP_SKGShooterFrameworkAnimInstance);
DECLARE_CYCLE_STAT(TEXT("SetupInitialData"), STAT_SKGSetupInitialData, STATGROUP_SKGShooterFrameworkAnimInstance);
DECLARE_CYCLE_STAT(TEXT("SetFirearmCollisionData"), STAT_SKGSetFirearmCollisionData, STATGROUP_SKGShooterFrameworkAnimInstance);
DECLARE_CYCLE_STAT(TEXT("OnFirearmTraceComplete"), STAT_SKGOnFirearmTraceComplete, STATGROUP_SKGShooterFrameworkAnimInstance);
DECLARE_CYCLE_STAT(TEXT("NativeThreadSafeUpdateAnimation"), STAT_SKGNativeThreadSafeUpdate, STATGROUP_SKGShooterFrameworkAnimInstance);

void USKGShooterFrameworkAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	
	SetupShooterPawnComponent();
}

void USKGShooterFrameworkAnimInstance::SetupShooterPawnComponent()
{
	ShooterPawnComponent = USKGShooterFrameworkHelpers::GetShooterPawnComponent(GetOwningActor());
	if (ensureMsgf(ShooterPawnComponent, TEXT("LET ME KNOW IF YOU READ THIS MESSAGE. NativeBeginPlay of USKGSHooterFrameworkAnimInstance, ShooterPawnComponent is invalid")))
	{
		ShooterPawnComponent->OnHeldActorSet.AddUniqueDynamic(this, &USKGShooterFrameworkAnimInstance::OnHeldActorSet);
		if (ShooterPawnComponent->GetHeldActor())
		{
			OnHeldActorSet(ShooterPawnComponent->GetHeldActor(), nullptr);
		}
	}
	
	FirearmTraceDelegate.BindUObject(this, &USKGShooterFrameworkAnimInstance::OnFirearmTraceComplete);
}

void USKGShooterFrameworkAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	SCOPE_CYCLE_COUNTER(STAT_SKGNativeUpdate);
	
	if (!ShooterPawnComponent)
	{
		return;
	}
	
	ProceduralAnimData = ShooterPawnComponent->GetProceduralData();
	FreeLookRecoilRotation = ProceduralAnimData.FreeLookStartRotation;
	bOffHandIKIsLeftHand = ProceduralAnimData.bOffHandIKIsLeftHand;
	
	SetupMandatoryData();
	SetupInitialData();
	
	if (ProceduralAnimData)
	{
		bUseFirstPerson = bIsLocallyControlled && ShooterPawnComponent->IsUsingFirstPersonProceduralsAsLocal();
		LengthOfPull = ProceduralAnimData.LengthOfPull;
		
		SetMovementSwayData(DeltaSeconds);
		SetRecoilData();
		SetPoseData(DeltaSeconds);
		SetFirearmCollisionData();
	}
	SetAimingData();
	SetCustomCurveData(DeltaSeconds);
}

void USKGShooterFrameworkAnimInstance::SetupMandatoryData()
{
	bIsLocallyControlled = ShooterPawnComponent->IsLocallyControlled();
	IsLocallyControlledAlpha = bIsLocallyControlled ? 1.0f : 0.0f;
	ControlRotation = bIsLocallyControlled ? ShooterPawnComponent->GetOwningPawn()->GetControlRotation() : ShooterPawnComponent->GetOwningPawn()->GetBaseAimRotation();
	TargetSpineLeanLeftRightAngle = ShooterPawnComponent->GetTargetLeanAngle();
	bIsTryingToAim = ShooterPawnComponent->IsAiming();
}

void USKGShooterFrameworkAnimInstance::SetupInitialData()
{
	SCOPE_CYCLE_COUNTER(STAT_SKGSetupInitialData);
		
	SetCameraOffset(ShooterPawnComponent->GetCameraOffset());
	OffHandIKLocation = ProceduralAnimData.OffHandIKOffset.GetTranslation();
	OffHandIKPose = ProceduralAnimData.OffHandIKPose;
}

void USKGShooterFrameworkAnimInstance::SetCameraOffset(const FTransform& Offset)
{
	CameraOffsetLocation = Offset.GetTranslation();
	CameraOffsetRotation = Offset.Rotator();
}

void USKGShooterFrameworkAnimInstance::SetAimingData()
{
	if (bIsAiming != ShooterPawnComponent->IsAiming())
	{
		if (!ProceduralAnimData.FirearmCollisionSettings.bUseFirearmCollision || FirearmCollisionPushAmount < ProceduralAnimData.FirearmCollisionSettings.CollisionStopAimingDistance)
		{
			bIsAiming = bIsTryingToAim;
			TargetAimAlpha = bIsAiming ? 1.0f : 0.0f;
			bInterpAimAlpha = true;
		}
	}
}

void USKGShooterFrameworkAnimInstance::SetMovementSwayData(float DeltaSeconds)
{
	if (ProceduralAnimData.MovementSwaySettings.bUseMovementSway)
	{
		const UCurveVector* LocationCurve = ProceduralAnimData.MovementSwaySettings.LocationSettings.Curve;
		const UCurveVector* RotationCurve = ProceduralAnimData.MovementSwaySettings.RotationSettings.Curve;
		
		MovementSwayVelocityMultiplier = UKismetMathLibrary::NormalizeToRange(PawnHorizontalVelocity, 0.0f, 250.0f);
        MovementSwayVelocityMultiplier = FMath::Clamp(MovementSwayVelocityMultiplier, 0.25f, 1.0f);
    
        MovementSwayAccumulator += DeltaSeconds * MovementSwayVelocityMultiplier;
		if (LocationCurve)
		{
			MovementSwayCurveLocation = LocationCurve->GetVectorValue(MovementSwayAccumulator);
		}
		if (RotationCurve)
		{
			MovementSwayCurveRotation = RotationCurve->GetVectorValue(MovementSwayAccumulator);
		}
	}
	else
	{
		MovementSwayCurveLocation = FVector::ZeroVector;
		MovementSwayCurveRotation = FVector::ZeroVector;
	}
}

void USKGShooterFrameworkAnimInstance::SetRecoilData()
{
	if (bPerformingRecoil)
	{
		const float ElapsedTimeSinceStart = GetWorld()->GetTimeSeconds() - RecoilStartTimeStamp;
		if (const UCurveVector* ControlRotationCurve = ProceduralAnimData.RecoilSettings.ControlRotationCurve)
		{
			ControlRotationRecoilTarget = ControlRotationCurve->GetVectorValue(ElapsedTimeSinceStart) * RecoilControlRotationMultiplier * ProceduralAnimData.ProceduralStats.ControlRotationRecoilMultipliers;
		}
		if (const UCurveVector* LocationCurve = ProceduralAnimData.RecoilSettings.LocationCurve)
		{
			RecoilLocationTarget = LocationCurve->GetVectorValue(ElapsedTimeSinceStart) * RecoilLocationMultiplier * ProceduralAnimData.ProceduralStats.RecoilLocationMultipliers;
		}
		if (const UCurveVector* RotationCurve = ProceduralAnimData.RecoilSettings.RotationCurve)
		{
			RecoilRotationTarget = RotationCurve->GetVectorValue(ElapsedTimeSinceStart) * RecoilRotationMultiplier * ProceduralAnimData.ProceduralStats.RecoilRotationMultipliers;
		}
	}
}

void USKGShooterFrameworkAnimInstance::SetPoseData(float DeltaSeconds)
{
	if (!bReachedEndOfPoseCurve)
	{
		if (CurrentPose)
		{
			PoseCurveTime = (GetWorld()->GetTimeSeconds() - PoseStartTime) * CurrentPose.PlayRate;
			if (!CurrentPose.GetVectorValues(PoseCurveTime, TargetPoseLocation, TargetPoseRotation, bOffHandIKIsLeftHand))
			{
				TargetPoseLocation = FVector::ZeroVector;
				TargetPoseRotation = FRotator::ZeroRotator;
				bReachedEndOfPoseCurve = true;
			}
			
			if (PoseCurveTime > CurrentPose.CurveEndTime)
			{
				bReachedEndOfPoseCurve = true;
				ShooterPawnComponent->PoseComplete();
			}
		}
		else
		{
			TargetPoseLocation = FVector::ZeroVector;
			TargetPoseRotation = FRotator::ZeroRotator;
			bReachedEndOfPoseCurve = true;
		}
	}
}

void USKGShooterFrameworkAnimInstance::SetFirearmCollisionData()
{
	if (ProceduralAnimData.FirearmCollisionSettings.bUseFirearmCollision)
	{
		SCOPE_CYCLE_COUNTER(STAT_SKGSetFirearmCollisionData);

		if (bFirearmCollisionAllowTrace)
		{
			bFirearmCollisionAllowTrace = false;
			FCollisionQueryParams FirearmCollisionParams;
			FirearmCollisionParams.bTraceComplex = false;
			FirearmCollisionParams.ClearIgnoredActors();
			FirearmCollisionParams.AddIgnoredActors(ProceduralAnimData.FirearmCollisionSettings.ActorsToIgnoreForTrace);
			FirearmCollisionParams.AddIgnoredActor(GetOwningActor());

			GetWorld()->AsyncSweepByChannel(EAsyncTraceType::Single, FirearmCollisionTraceStart, FirearmCollisionTraceEnd, FQuat::Identity, ProceduralAnimData.FirearmCollisionSettings.CollisionChannel, FCollisionShape::MakeSphere(ProceduralAnimData.FirearmCollisionSettings.TraceDiameter), FirearmCollisionParams, FCollisionResponseParams(), &FirearmTraceDelegate);
		}
	}
	else if (!bSettingTestPose)
	{
		FirearmCollisionPushAmount = 0.0f;
		FirearmCollisionLocation = FVector::ZeroVector;
		FirearmCollisionRotation = FRotator::ZeroRotator;
	}
}

void USKGShooterFrameworkAnimInstance::OnFirearmTraceComplete(const FTraceHandle& TraceHandle, FTraceDatum& TraceData)
{
	SCOPE_CYCLE_COUNTER(STAT_SKGOnFirearmTraceComplete);
	bFirearmCollisionAllowTrace = true;
	FHitResult HitResult;
	if (TraceData.OutHits.Num())
	{
		HitResult = TraceData.OutHits[0];
		bFirearmCollisionHit = true;
	}
	else
	{
		bFirearmCollisionHit = false;
	}
	
	if (bFirearmCollisionHit)
	{
		FirearmCollisionHitLocation = HitResult.Location;
	}
	else
	{	// Interpolate back to 0 from no hit
		FirearmCollisionHitLocation = FirearmCollisionTraceEnd;
	}

#if WITH_EDITOR
	if (ShooterPawnComponent)
	{
		if (ShooterPawnComponent->bPrintHit && bFirearmCollisionHit)
		{
			if (HitResult.GetComponent())
			{
				UE_LOG(LogTemp, Warning, TEXT("Hitting Component: %s of Actor: %s"), *HitResult.GetComponent()->GetName(), *HitResult.GetActor()->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Hitting Actor: %s"), *HitResult.GetActor()->GetName());
			}
		}
		if (ShooterPawnComponent->bDrawDebugTrace)
		{
			DrawDebugLine(GetWorld(), FirearmCollisionTraceStart, FirearmCollisionTraceEnd, FColor::Red, false, -1, 0, 3.0f);
		}
	}
#endif
	const float PushBackDistance = FVector::Dist(FirearmCollisionTraceEnd, FirearmCollisionHitLocation) * ProceduralAnimData.FirearmCollisionSettings.PoseScale;

	FirearmCollisionPushAmount = ((FirearmCollisionHitLocation - FirearmCollisionTraceEnd) / (FirearmCollisionTraceStart - FirearmCollisionTraceEnd)).Size();
	if (FMath::IsNaN(FirearmCollisionPushAmount))
	{
		FirearmCollisionPushAmount = 0.0f;
	}
	if (bIsAiming && FirearmCollisionPushAmount > ProceduralAnimData.FirearmCollisionSettings.CollisionStopAimingDistance)
	{
		bIsAiming = false;
		bInterpAimAlpha = true;
		TargetAimAlpha = 0.0f;
	}
	
	if (ProceduralAnimData.FirearmCollisionSettings.PoseLocation)
	{
		FirearmCollisionTargetLocation = ProceduralAnimData.FirearmCollisionSettings.PoseLocation->GetVectorValue(PushBackDistance);
		FirearmCollisionTargetLocation.X *= bOffHandIKIsLeftHand ? 1.0 : -1.0;
	}
	if (ProceduralAnimData.FirearmCollisionSettings.PoseRotation)
	{
		FirearmCollisionTargetRotation = ProceduralAnimData.FirearmCollisionSettings.PoseRotation->GetVectorValue(PushBackDistance);
		FirearmCollisionTargetRotation.X *= bOffHandIKIsLeftHand ? 1.0 : -1.0;
		FirearmCollisionTargetRotation.Y *= bOffHandIKIsLeftHand ? 1.0 : -1.0;
	}
}

void USKGShooterFrameworkAnimInstance::SetCustomCurveData(float DeltaSeconds)
{
	if (bPlayCustomCurve)
	{
		const float CustomCurveTime = (GetWorld()->GetTimeSeconds() - CustomCurveStartTime) * CurrentCustomCurve.PlayRate;
		
		if (!CurrentCustomCurve.GetVectorValues(CustomCurveTime, CustomCurveLocation, CustomCurveRotation, bOffHandIKIsLeftHand) || CustomCurveTime > CurrentCustomCurve.CurveEndTime)
		{
			bPlayCustomCurve = false;
		}
	}
}

void USKGShooterFrameworkAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	SCOPE_CYCLE_COUNTER(STAT_SKGNativeThreadSafeUpdate);

	HandleProceduralSpine(DeltaSeconds);

	AimCurveAlpha = GetCurveValue(AimCurveName);
	OffHandIKAlpha = 1.0f - GetCurveValue(OffhandIKCurveName);

	HandleAiming(DeltaSeconds);
	if (ProceduralAnimData)
	{
		HandlePawnVelocity();
		HandleOffsets(DeltaSeconds);
		HandleMovementSway(DeltaSeconds);
		HandleMovementLag(DeltaSeconds);
		HandleRotationLag(DeltaSeconds);
		HandleRecoil(DeltaSeconds);
		HandlePose(DeltaSeconds);
		HandleFirearmCollision(DeltaSeconds);

		if (bIsLocallyControlled)
		{
			if (ProceduralAnimData.DeadzoneSettings.bUseDeadzone)
			{
				// Mouse input only handled locally, no need to waste resources running logic on other machines
				HandleDeadzone(DeltaSeconds);
			}
		}

		ComponentSpaceFinalLocation = RotationLagLocation;
		ComponentSpaceFinalRotation = DeadzoneHandRotation + MovementLagRotation + RotationLagRotation;

		ParentBoneSpaceFinalLocation = RecoilLocation + CustomCurveLocation;
		ParentBoneSpaceFinalRotation = RecoilRotation + CustomCurveRotation;
	}

	TestHitRotation = FMath::RInterpTo(TestHitRotation, TestHitTargetRotation, DeltaSeconds, 5.0f);

	TestHitTargetRotation = FMath::RInterpTo(TestHitTargetRotation, FRotator::ZeroRotator, DeltaSeconds, 4.0f);
}

void USKGShooterFrameworkAnimInstance::HandlePawnVelocity()
{
	FVector Velocity = GetOwningActor()->GetVelocity();
	PawnVelocity = Velocity;
	Velocity.Z = 0.0f;
	PawnHorizontalVelocity = Velocity.Size();
}

void USKGShooterFrameworkAnimInstance::HandleOffsets(float DeltaSeconds)
{
	const FSKGProceduralOffset BasePoseOffset = bIsLocallyControlled && bUseFirstPerson? ProceduralAnimData.BasePoseOffset.FirstPersonOffset : ProceduralAnimData.BasePoseOffset.ThirdPersonOffset;
	BasePoseOffsetLocation = BasePoseOffset.Location;
	BasePoseOffsetRotation = BasePoseOffset.Rotation;

	if (!bIsLocallyControlled || !bUseFirstPerson)
	{
		ThirdPersonAimingOffsetLocation = FMath::VInterpTo(ThirdPersonAimingOffsetLocation, ProceduralAnimData.ThirdPersonAimingOffset.Location, DeltaSeconds, 10.0f);
		ThirdPersonAimingOffsetRotation = FMath::RInterpTo(ThirdPersonAimingOffsetRotation, ProceduralAnimData.ThirdPersonAimingOffset.Rotation, DeltaSeconds, 10.0f);
	}
	else
	{
		ThirdPersonAimingOffsetLocation = FVector::ZeroVector;
		ThirdPersonAimingOffsetRotation = FRotator::ZeroRotator;
	}
}

void USKGShooterFrameworkAnimInstance::HandleProceduralSpine(float DeltaSeconds)
{
	if (bUseProceduralSpineUpDown)
	{
		if (ProceduralAnimData.bInFreeLook && bPerformingRecoil)
		{
			OldSpineControlRotation = FreeLookRecoilRotation;
			OldSpineControlRotation -= AccumulatedControlRotationRecoil * 2.5f;
			FreeLookRecoilRotation = OldSpineControlRotation;
			SpineRotationToInterpTo = UKismetMathLibrary::NormalizedDeltaRotator(OldSpineControlRotation, GetOwningActor()->GetActorRotation());
		
			SpineRotationToInterpTo.Roll = (SpineRotationToInterpTo.Pitch * -1.0f);
			SpineRotationToInterpTo.Pitch = 0.0f;
			SpineRotationToInterpTo.Yaw = 0.0f;
		
			if (bIsLocallyControlled)
			{
				SpineRotationUpDown = SpineRotationToInterpTo;
			}
			else
			{
				SpineRotationUpDown = UKismetMathLibrary::RInterpTo(SpineRotationUpDown, SpineRotationToInterpTo, DeltaSeconds, 10.0f);
			}
		}
		else if (!ProceduralAnimData.bInFreeLook && (OldSpineControlRotation != ControlRotation || SpineRotationUpDown != SpineRotationToInterpTo))
		{
			AccumulatedControlRotationRecoil = FRotator::ZeroRotator;
			OldSpineControlRotation = ControlRotation;
			SpineRotationToInterpTo = UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation, GetOwningActor()->GetActorRotation());
		
			SpineRotationToInterpTo.Roll = (SpineRotationToInterpTo.Pitch * -1.0f);
			SpineRotationToInterpTo.Pitch = 0.0f;
			SpineRotationToInterpTo.Yaw = 0.0f;
		
			if (bIsLocallyControlled)
			{
				SpineRotationUpDown = SpineRotationToInterpTo;
			}
			else
			{
				SpineRotationUpDown = UKismetMathLibrary::RInterpTo(SpineRotationUpDown, SpineRotationToInterpTo, DeltaSeconds, 10.0f);
			}
		}
	}
	
	if (bUseProceduralSpineLeftRight && SpineLeanLeftRight.Pitch != TargetSpineLeanLeftRightAngle)
	{
		if (ProceduralAnimData.LeanLeftRightSettings.bUseSpringInterp)
		{
			SpineLeanLeftRight.Pitch = UKismetMathLibrary::FloatSpringInterp(SpineLeanLeftRight.Pitch, TargetSpineLeanLeftRightAngle, LeanLeftRightSpringState, ProceduralAnimData.LeanLeftRightSettings.Stiffness, ProceduralAnimData.LeanLeftRightSettings.CriticalDampingFactor, DeltaSeconds, ProceduralAnimData.LeanLeftRightSettings.Mass, ProceduralAnimData.LeanLeftRightSettings.TargetVelocityAmount);
		}
		else
		{
			SpineLeanLeftRight.Pitch = UKismetMathLibrary::FInterpTo(SpineLeanLeftRight.Pitch, TargetSpineLeanLeftRightAngle, DeltaSeconds, ProceduralAnimData.LeanLeftRightSettings.InterpSpeed);
		}
	}
}

void USKGShooterFrameworkAnimInstance::HandleAiming(float DeltaSeconds)
{
	if (!SightOffsetLocation.Equals(ProceduralAnimData.AimOffset.GetTranslation()) || !SightOffsetRotation.Equals(ProceduralAnimData.AimOffset.Rotator()))
	{
		if (SightOffsetLocation.Equals(FVector::ZeroVector))
		{ // Temp fix before rewrite so the first ADS should not have any weird path
			SightOffsetLocation = ProceduralAnimData.AimOffset.GetTranslation();
			SightOffsetRotation = ProceduralAnimData.AimOffset.Rotator();
		}
		
		if (ProceduralAnimData.CycleAimingPointSettings.bUseSpringInterpForCyclingAimingPoint)
		{
			const FSKGCycleAimingPointSpringInterpSettings SpringSettings = ProceduralAnimData.ProceduralStats.CycleAimingPointSpringInterpSettings;
			SightOffsetLocation = UKismetMathLibrary::VectorSpringInterp(SightOffsetLocation, ProceduralAnimData.AimOffset.GetTranslation(), AimingLocationSpringState, SpringSettings.LocationStiffness, SpringSettings.LocationCriticalDampingFactor, DeltaSeconds, SpringSettings.LocationMass, SpringSettings.LocationTargetVelocityAmount);
			
			SightOffsetRotation = UKismetMathLibrary::QuaternionSpringInterp(SightOffsetRotation.Quaternion(), ProceduralAnimData.AimOffset.GetRotation(), AimingRotationSpringState, SpringSettings.RotationStiffness, SpringSettings.RotationCriticalDampingFactor, DeltaSeconds, SpringSettings.RotationMass, SpringSettings.RotationTargetVelocityAmount).Rotator();
		}
		else
		{
			SightOffsetLocation = FMath::VInterpTo(SightOffsetLocation, ProceduralAnimData.AimOffset.GetTranslation(), DeltaSeconds, ProceduralAnimData.AimOffsetInterpSpeed);
			SightOffsetRotation = FMath::RInterpTo(SightOffsetRotation, ProceduralAnimData.AimOffset.Rotator(), DeltaSeconds, ProceduralAnimData.AimOffsetInterpSpeed);
		}
	}

	if (bInterpAimAlpha)
	{
		AimAlpha = FMath::FInterpTo(AimAlpha, TargetAimAlpha, DeltaSeconds, ProceduralAnimData.ProceduralStats.AimInterpolationRate);
		if (AimAlpha == TargetAimAlpha)
		{
			bInterpAimAlpha = false;
		}
	}
}

void USKGShooterFrameworkAnimInstance::HandleMovementSway(float DeltaSeconds)
{
	if (ProceduralAnimData.MovementSwaySettings.bUseMovementSway)
	{
		MovementSwayLocationMultiplier = FMath::FInterpTo(MovementSwayLocationMultiplier, ProceduralAnimData.MovementSwaySettings.LocationSettings.Multiplier, DeltaSeconds, ProceduralAnimData.MovementSwaySettings.LocationSettings.MultiplierInterpolationRate);
		MovementSwayRotationMultiplier = FMath::FInterpTo(MovementSwayRotationMultiplier, ProceduralAnimData.MovementSwaySettings.RotationSettings.Multiplier, DeltaSeconds, ProceduralAnimData.MovementSwaySettings.RotationSettings.MultiplierInterpolationRate);
		
		MovementSwayCurveLocation *= MovementSwayLocationMultiplier * ProceduralAnimData.ProceduralStats.MovementSwayMultiplier;
		MovementSwayCurveRotation *= MovementSwayRotationMultiplier * ProceduralAnimData.ProceduralStats.MovementSwayMultiplier;
		const FRotator TargetRotation = FRotator(MovementSwayCurveRotation.Y, MovementSwayCurveRotation.X, MovementSwayCurveRotation.Z);

		MovementSwayLocation = UKismetMathLibrary::VInterpTo(MovementSwayLocation, MovementSwayCurveLocation, DeltaSeconds, ProceduralAnimData.MovementSwaySettings.LocationSettings.InterpSpeed);
		MovementSwayRotation = UKismetMathLibrary::RInterpTo(MovementSwayRotation, TargetRotation, DeltaSeconds, ProceduralAnimData.MovementSwaySettings.RotationSettings.InterpSpeed);
	}
	else
	{
		MovementSwayLocation = FVector::ZeroVector;
		MovementSwayRotation = FRotator::ZeroRotator;
	}
}

void USKGShooterFrameworkAnimInstance::HandleMovementLag(float DeltaSeconds)
{
	if (ProceduralAnimData.MovementLagSettings.bUseMovementLag && (!MovementLagRotation.Equals(FRotator::ZeroRotator) || PawnVelocity.Size()))
	{
		float RightVelocity = FVector::DotProduct(PawnVelocity, GetOwningActor()->GetActorRightVector());
		RightVelocity = UKismetMathLibrary::NormalizeToRange(RightVelocity, 0.0f, ProceduralAnimData.MovementLagSettings.LeftRightMax);
		float VerticalVelocity = PawnVelocity.Z;
		VerticalVelocity = UKismetMathLibrary::NormalizeToRange(VerticalVelocity, 0.0f, ProceduralAnimData.MovementLagSettings.UpDownMax);
		const FRotator MovementLagMultiplier = ProceduralAnimData.MovementLagSettings.Multiplier;

		const FRotator MovementLagTargetRotation = FRotator(RightVelocity * MovementLagMultiplier.Roll, RightVelocity * MovementLagMultiplier.Yaw, VerticalVelocity * MovementLagMultiplier.Pitch);

		if (ProceduralAnimData.MovementLagSettings.bUseSpringInterpolation)
		{
			const FSKGMovementLagSpringInterpSettings SpringInterpSettings = ProceduralAnimData.ProceduralStats.MovementLagSpringInterpSettings;

			MovementLagRotation = UKismetMathLibrary::QuaternionSpringInterp(MovementLagRotation.Quaternion(), MovementLagTargetRotation.Quaternion(), MovementLagSpringState, SpringInterpSettings.SpringStiffness, SpringInterpSettings.SpringCriticalDampingFactor, DeltaSeconds, SpringInterpSettings.SpringMass, SpringInterpSettings.SpringTargetVelocityAmount).Rotator();
		}
		else
		{
			MovementLagRotation = FMath::RInterpTo(MovementLagRotation, MovementLagTargetRotation, DeltaSeconds, ProceduralAnimData.MovementLagSettings.InterpSpeed);
		}
	}
	else
	{
		MovementLagRotation = FRotator::ZeroRotator;
	}
}

void USKGShooterFrameworkAnimInstance::HandleRotationLag(float DeltaSeconds)
{
	if (ProceduralAnimData.RotationLagSettings.bUseRotationLag && (!ControlRotation.Equals(OldControlRotation) || (!RotationLagRotation.Equals(FRotator::ZeroRotator) || !RotationLagLocation.Equals(FVector::ZeroVector))) && !ProceduralAnimData.bInFreeLook)
	{
		const float Delta = 1.0f - DeltaSeconds;
		const FQuat RotationDifference = (ControlRotation - OldControlRotation).Quaternion() * Delta;
		OldControlRotation = ControlRotation;

		if (bIsLocallyControlled && ProceduralAnimData.RotationLagSettings.bUseSpringInterpolation)
		{
			const FSKGRotationLagSpringInterpSettings SpringInterpSettings = ProceduralAnimData.ProceduralStats.RotationLagSpringInterpSettings;
			
			RotationLagRotation = UKismetMathLibrary::QuaternionSpringInterp(UnalteredRotationLagRotation.Quaternion(), RotationDifference, RotationLagRotationSpringState, SpringInterpSettings.RotationSpringStiffness, SpringInterpSettings.RotationSpringCriticalDampingFactor, DeltaSeconds, SpringInterpSettings.RotationSpringMass, SpringInterpSettings.RotationSpringTargetVelocityAmount).Rotator();
			
			UnalteredRotationLagRotation = RotationLagRotation;
		
			const FRotator RotationLagMultiplier = ProceduralAnimData.RotationLagSettings.RotationMultiplier;
			// Roll, Yaw, Pitch
			RotationLagRotation = FRotator(RotationLagRotation.Yaw * RotationLagMultiplier.Roll, RotationLagRotation.Yaw * RotationLagMultiplier.Yaw, RotationLagRotation.Pitch * RotationLagMultiplier.Pitch);

			const float RollMax = ProceduralAnimData.RotationLagSettings.MaxRotation.Roll;
			const float YawMax = ProceduralAnimData.RotationLagSettings.MaxRotation.Yaw;
			const float PitchMax = ProceduralAnimData.RotationLagSettings.MaxRotation.Pitch;
			RotationLagRotation.Roll = FMath::Clamp(RotationLagRotation.Roll, -RollMax, RollMax);
			RotationLagRotation.Pitch = FMath::Clamp(RotationLagRotation.Pitch, -PitchMax, PitchMax);
			RotationLagRotation.Yaw = FMath::Clamp(RotationLagRotation.Yaw, -YawMax, YawMax);

			// Left/Right, Forward = Right | Backward = Left, Up/Down
			FVector TargetRotationLagLocation = FVector(RotationLagRotation.Yaw, RotationLagRotation.Yaw, RotationLagRotation.Roll);
			TargetRotationLagLocation *= ProceduralAnimData.RotationLagSettings.LocationMultiplier;
			
			RotationLagLocation = UKismetMathLibrary::VectorSpringInterp(RotationLagLocation, TargetRotationLagLocation, RotationLagLocationSpringState, SpringInterpSettings.LocationSpringStiffness, SpringInterpSettings.LocationSpringCriticalDampingFactor, DeltaSeconds, SpringInterpSettings.LocationSpringMass, SpringInterpSettings.LocationSpringTargetVelocityAmount);
		}
		else
		{
			RotationLagRotation = FMath::RInterpTo(UnalteredRotationLagRotation, RotationDifference.Rotator(), DeltaSeconds, ProceduralAnimData.ProceduralStats.RotationLagInterpSettings.RotationInterpSpeed);

			UnalteredRotationLagRotation = RotationLagRotation;
		
			const FRotator RotationLagMultiplier = ProceduralAnimData.RotationLagSettings.RotationMultiplier;
			// Roll, Yaw, Pitch
			RotationLagRotation = FRotator(RotationLagRotation.Yaw * RotationLagMultiplier.Roll, RotationLagRotation.Yaw * RotationLagMultiplier.Yaw, RotationLagRotation.Pitch * RotationLagMultiplier.Pitch);

			const float RollMax = ProceduralAnimData.RotationLagSettings.MaxRotation.Roll;
			const float YawMax = ProceduralAnimData.RotationLagSettings.MaxRotation.Yaw;
			const float PitchMax = ProceduralAnimData.RotationLagSettings.MaxRotation.Pitch;
			RotationLagRotation.Roll = FMath::Clamp(RotationLagRotation.Roll, -RollMax, RollMax);
			RotationLagRotation.Pitch = FMath::Clamp(RotationLagRotation.Pitch, -PitchMax, PitchMax);
			RotationLagRotation.Yaw = FMath::Clamp(RotationLagRotation.Yaw, -YawMax, YawMax);

			// Left/Right, Forward = Right | Backward = Left, Up/Down
			FVector TargetRotationLagLocation = FVector(RotationLagRotation.Yaw, RotationLagRotation.Yaw, RotationLagRotation.Roll);
			TargetRotationLagLocation *= ProceduralAnimData.RotationLagSettings.LocationMultiplier;
			RotationLagLocation = FMath::VInterpTo(RotationLagLocation, TargetRotationLagLocation, DeltaSeconds, ProceduralAnimData.ProceduralStats.RotationLagInterpSettings.LocationInterpSpeed);
		}
	}
	else
	{
		RotationLagLocation = FVector::ZeroVector;
		RotationLagRotation = FRotator::ZeroRotator;
	}
}

void USKGShooterFrameworkAnimInstance::HandlePose(float DeltaSeconds)
{
	if (bInterpolatePose)
	{
		if (bInterpolatePoseTransform)
		{
			PoseLocation = FMath::VInterpConstantTo(PoseLocation, TargetPoseLocation, DeltaSeconds, PoseInterpolationSpeed);
			PoseRotation = FMath::RInterpConstantTo(PoseRotation, TargetPoseRotation, DeltaSeconds, PoseInterpolationSpeed);
			const float PostInterpolationTime = (GetWorld()->GetTimeSeconds() - PostInterpolationTimeStart) * PoseInterpolationSpeed;
			PoseInterpolationSpeed += (PostInterpolationTime * DefaultPoseInterpolationSpeedMultiplier);
		}
		else
		{
			PoseLocation = TargetPoseLocation;
			PoseRotation = TargetPoseRotation;
		}
		
		if (bReachedEndOfPoseCurve && PoseLocation.Equals(TargetPoseLocation) && PoseRotation.Equals(TargetPoseRotation))
		{
			bInterpolatePose = false;
		}
	}
}

void USKGShooterFrameworkAnimInstance::HandleFirearmCollision(float DeltaSeconds)
{
	if (ProceduralAnimData.FirearmCollisionSettings.bUseFirearmCollision)
	{
		const FTransform MuzzleRelativeTransform = ProceduralAnimData.FirearmCollisionSettings.MuzzleRelativeTransform;
		const float MuzzleToRootDistance = MuzzleRelativeTransform.GetTranslation().Size();
		const FTransform EndTransform = MuzzleRelativeTransform * GetOwningComponent()->GetSocketTransform(FirearmCollisionBoneName, RTS_World);
		FirearmCollisionTraceEnd = EndTransform.GetTranslation();
		FirearmCollisionTraceStart = FirearmCollisionTraceEnd + (EndTransform.Rotator().Vector() * -1.0f) * MuzzleToRootDistance;
		
		FirearmCollisionLocation = FMath::VInterpTo(FirearmCollisionLocation, FirearmCollisionTargetLocation, DeltaSeconds, ProceduralAnimData.FirearmCollisionSettings.PoseLocationInterpSpeed);
		const FRotator TargetRot = FRotator(FirearmCollisionTargetRotation.X, FirearmCollisionTargetRotation.Y, FirearmCollisionTargetRotation.Z);
		FirearmCollisionRotation = FMath::RInterpTo(FirearmCollisionRotation, TargetRot, DeltaSeconds, ProceduralAnimData.FirearmCollisionSettings.PoseRotationInterpSpeed);
	}
}

void USKGShooterFrameworkAnimInstance::HandleDeadzone(float DeltaSeconds)
{
	const bool bInterpBackToZero = ProceduralAnimData.DeadzoneSettings.bDisableDeadzoneWhenAiming && bIsAiming;
	FRotator TargetRotation = FRotator::ZeroRotator;
	float TargetInterpSpeed = ProceduralAnimData.DeadzoneSettings.DeadzoneAimingDisableInterpolationSpeed;
	if (!bInterpBackToZero)
	{
		DeadzoneYaw += ProceduralAnimData.MouseInput.X * ProceduralAnimData.DeadzoneSettings.YawRate;
		DeadzonePitch += ProceduralAnimData.MouseInput.Y * ProceduralAnimData.DeadzoneSettings.PitchRate;
	
		DeadzoneYaw = FMath::Clamp(DeadzoneYaw, -ProceduralAnimData.DeadzoneSettings.YawLimit, ProceduralAnimData.DeadzoneSettings.YawLimit);
		DeadzonePitch = FMath::Clamp(DeadzonePitch, -ProceduralAnimData.DeadzoneSettings.PitchLimit, ProceduralAnimData.DeadzoneSettings.PitchLimit);

		TargetRotation = FRotator(0.0f, DeadzoneYaw, DeadzonePitch);
		TargetInterpSpeed = ProceduralAnimData.DeadzoneSettings.InterpolationSpeed;
	}
	
	DeadzoneHandRotation = UKismetMathLibrary::RInterpTo(DeadzoneHandRotation, TargetRotation, DeltaSeconds, TargetInterpSpeed);
}

void USKGShooterFrameworkAnimInstance::InterpRecoilToNone(float DeltaSeconds)
{
	RecoilLocation = FMath::VInterpTo(RecoilLocation, FVector::ZeroVector, DeltaSeconds, ProceduralAnimData.RecoilSettings.LocationInterpToNoneSpeed);
	RecoilRotation = FMath::RInterpTo(RecoilRotation, FRotator::ZeroRotator, DeltaSeconds, ProceduralAnimData.RecoilSettings.RotationInterpToNoneSpeed);

	if (RecoilLocation.Equals(FVector::ZeroVector) && RecoilRotation.Equals(FRotator::ZeroRotator))
	{
		bPerformingRecoil = false;
	}
}

void USKGShooterFrameworkAnimInstance::HandleRecoil(float DeltaSeconds)
{
	if (bPerformingRecoil)
	{
		const float Delta = 100.0f * DeltaSeconds;
		
		RecoilLocationTarget *= RandomRecoilLocation;
		RecoilRotationTarget *= RandomRecoilRotation;

		if (bIsLocallyControlled && !ControlRotationRecoilTarget.Equals(FVector::ZeroVector))
		{
			if (APawn* Pawn = TryGetPawnOwner())
			{
				ControlRotationRecoilTarget *= RandomControlRotationRecoil;
				ControlRotationRecoilTargetRot = FRotator(ControlRotationRecoilTarget.X, ControlRotationRecoilTarget.Y, ControlRotationRecoilTarget.Z) * Delta;
				AccumulatedControlRotationRecoil += ControlRotationRecoilTargetRot;
				Pawn->AddControllerPitchInput(ControlRotationRecoilTargetRot.Pitch);
				Pawn->AddControllerYawInput((ControlRotationRecoilTargetRot.Yaw));
			}
		}
		
		RecoilLocation += RecoilLocationTarget * Delta;
		RecoilRotation += FRotator(RecoilRotationTarget.X, RecoilRotationTarget.Y, RecoilRotationTarget.Z) * Delta;
		InterpRecoilToNone(DeltaSeconds);
	}
}

void USKGShooterFrameworkAnimInstance::PerformRecoil(const FRotator& ControlRotationMultiplier, const FVector& LocationMultiplier, const FRotator& RotationMultiplier)
{
	RecoilStartTimeStamp = GetWorld()->GetTimeSeconds();
	RecoilLocationMultiplier = LocationMultiplier;
	//XYZ = Roll, Pitch, Yaw
	RecoilRotationMultiplier = FVector(RotationMultiplier.Pitch, RotationMultiplier.Yaw, RotationMultiplier.Roll);
	// XYZ = Pitch, Yaw, Roll
	RecoilControlRotationMultiplier = FVector(ControlRotationMultiplier.Pitch, ControlRotationMultiplier.Yaw, ControlRotationMultiplier.Roll);

	// X = up/down, Y = forward/backward, Z = left/right
	RandomRecoilLocation = FVector(ProceduralAnimData.RecoilSettings.XRandom.GetRandom(), ProceduralAnimData.RecoilSettings.YRandom.GetRandom(), ProceduralAnimData.RecoilSettings.ZRandom.GetRandom());
	RandomRecoilRotation = FVector(ProceduralAnimData.RecoilSettings.RollRandom.GetRandom(), ProceduralAnimData.RecoilSettings.YawRandom.GetRandom(), ProceduralAnimData.RecoilSettings.PitchRandom.GetRandom());

	//RandomRecoilLocation *= ProceduralAnimData.ProceduralStats.VerticalRecoilMultiplier;
	//RandomRecoilRotation *= ProceduralAnimData.ProceduralStats.HorizontalRecoilMultiplier;
	
	if (bIsLocallyControlled)
	{
		RandomControlRotationRecoil = FVector(ProceduralAnimData.RecoilSettings.ControlPitchRandom.GetRandom(), ProceduralAnimData.RecoilSettings.ControlYawRandom.GetRandom(), ProceduralAnimData.RecoilSettings.ControlRollRandom.GetRandom());
	}
	bPerformingRecoil = true;
}

void USKGShooterFrameworkAnimInstance::TryPerformPose(const FSKGToFromCurveSettings& PoseData, const bool ExitPose)
{
	CurrentPoseData = PoseData;
	FSKGFirstAndThirdPersonCurveSettings SelectedPose = ExitPose ? CurrentPoseData.FromCurve : CurrentPoseData.ToCurve;
	if (bIsLocallyControlled)
	{
		CurrentPose = bUseFirstPerson? SelectedPose.FirstPersonCurve : SelectedPose.ThirdPersonCurve;
	}
	else
	{
		CurrentPose = SelectedPose.ThirdPersonCurve;
	}
	
	if (bReachedEndOfPoseCurve)
	{
		PoseStartTime = GetWorld()->GetTimeSeconds();
	}
	else if (CurrentPoseData == PoseData)
	{
		const float TimeFromEndOfCurve = (CurrentPose.CurveEndTime - PoseCurveTime) / CurrentPoseData.InterpolationSpeedMultiplier;
		PoseStartTime = GetWorld()->GetTimeSeconds() - TimeFromEndOfCurve;
	}
	
	bReachedEndOfPoseCurve = false;
	bInterpolatePose = true;
	
	PoseInterpolationSpeed = CurrentPoseData.InterpolationSpeed;
	bInterpolatePoseTransform = CurrentPoseData.bInterpolate;
	PostInterpolationTimeStart = GetWorld()->GetTimeSeconds();
}

void USKGShooterFrameworkAnimInstance::PerformCustomCurve(const FSKGFirstAndThirdPersonCurveSettings& CurveData)
{
	if (CurveData)
	{
		bPlayCustomCurve = true;
		CustomCurveStartTime = GetWorld()->GetTimeSeconds();
		if (bIsLocallyControlled)
		{
			CurrentCustomCurve = bUseFirstPerson? CurveData.FirstPersonCurve : CurveData.ThirdPersonCurve;
		}
		else
		{
			CurrentCustomCurve = CurveData.ThirdPersonCurve;
		}
	}
	else
	{
		bPlayCustomCurve = false;
	}
}

void USKGShooterFrameworkAnimInstance::SetTestPose(const FVector& Location, const FVector& Rotation, bool bFirearmCollision)
{
	if (bFirearmCollision)
	{
		FirearmCollisionLocation = Location;
		FirearmCollisionRotation = FRotator(Rotation.X, Rotation.Y, Rotation.Z);
		PoseLocation = FVector::ZeroVector;
		PoseRotation = FRotator::ZeroRotator;
		bSettingTestPose = true;
	}
	else
	{
		PoseLocation = Location;
		PoseRotation = FRotator(Rotation.X, Rotation.Y, Rotation.Z);
		FirearmCollisionLocation = FVector::ZeroVector;
		FirearmCollisionRotation = FRotator::ZeroRotator;
		bSettingTestPose = false;
	}
}

void USKGShooterFrameworkAnimInstance::GetCurrentPoseValues(FVector& Location, FVector& Rotation, bool bFirearmCollision)
{
	if (bFirearmCollision)
	{
		Location = FirearmCollisionLocation;
		Rotation = FVector(FirearmCollisionRotation.Pitch, FirearmCollisionRotation.Yaw, FirearmCollisionRotation.Roll);
	}
	else
	{
		Location = PoseLocation;
		Rotation = FVector(PoseRotation.Pitch, PoseRotation.Yaw, PoseRotation.Roll);
	}
}

void USKGShooterFrameworkAnimInstance::TestHit(const FHitResult& HitResult)
{
	FVector Normal = HitResult.ImpactNormal * 15;
	TestHitTargetRotation = FRotator(Normal.X, 0.0f, Normal.Y);

	UE_LOG(LogTemp, Warning, TEXT("Normal: %s"), *Normal.ToString());
}