// Copyright 2023, Dakota Dawe, All rights reserved


#include "Components/SKGShooterPawnComponent.h"
#include "Animation/SKGShooterFrameworkAnimInstance.h"
#include "Components/SKGFirearmComponent.h"
#include "Components/SKGOpticComponent.h"
#include "Components/SKGProceduralAnimComponent.h"
#include "Statics/SKGShooterFrameworkHelpers.h"
#include "Statics/SKGShooterFrameworkCoreNetworkStatics.h"
#include "DeveloperSettings/SKGShooterFrameworkDeveloperSettings.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

constexpr double FreeLookTraceDistance = 100000.0;

// Sets default values for this component's properties
USKGShooterPawnComponent::USKGShooterPawnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void USKGShooterPawnComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningPawn = GetOwner<APawn>();

	if (const USKGShooterFrameworkDeveloperSettings* DeveloperSettings = GetDefault<USKGShooterFrameworkDeveloperSettings>())
	{
		FirearmCollisionChannel = DeveloperSettings->FirearmCollisionChannel;
	}
	
	SetupComponents();
	SetCameraOffset();

	if (HasAuthority() && bReplicateRemoteYaw)
	{
		SetComponentTickInterval(RemoteYawReplicationRate);
		SetComponentTickEnabled(true);
	}
}

void USKGShooterPawnComponent::PostInitProperties()
{
	Super::PostInitProperties();
}

void USKGShooterPawnComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (OwningPawn && OwningPawn->Controller)
	{
		const uint8 NewYaw = USKGShooterFrameworkCoreNetworkStatics::CompressFloatToByte(OwningPawn->Controller->GetControlRotation().Yaw);
		if (RemoteViewYaw != NewYaw)
		{
			RemoteViewYaw = NewYaw;
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, RemoteViewYaw, this);
		}
	}
}

void USKGShooterPawnComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGShooterPawnComponent, TargetLeanAngleCompressed, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGShooterPawnComponent, HeldActor, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGShooterPawnComponent, CurrentProceduralPoseData, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGShooterPawnComponent, bOffHandIKIsLeftHand, Params);

	Params.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGShooterPawnComponent, bIsAiming, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGShooterPawnComponent, bInFreeLook, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGShooterPawnComponent, RemoteViewYaw, Params);
}

void USKGShooterPawnComponent::SetupComponents()
{
	ensureAlwaysMsgf(FirstPersonMeshComponentName != NAME_None || ThirdPersonMeshComponentName != NAME_None, TEXT("First Person Mesh Component Name AND Third Person Mesh Component Name are NOT valid (None) on Actor: %s"), *GetOwner()->GetName());
	//ensureAlwaysMsgf(LinkedAnimLayerClass, TEXT("Linked Anim Layer Class NOT set on Actor: %s"), *GetOwner()->GetName());
	
	for (UActorComponent* Component : OwningPawn->GetComponents())
	{
		if (Component)
		{
			if (Component->GetFName() == CameraComponentName)
			{
				CameraComponent = Cast<UCameraComponent>(Component);
			}
			else if (Component->GetFName() == FirstPersonMeshComponentName)
			{
				MeshFP = Cast<USkeletalMeshComponent>(Component);
				if (bUseSingleMesh)
				{
					MeshTP = MeshFP;
				}
			}
			else if (Component->GetFName() == ThirdPersonMeshComponentName)
			{
				MeshTP = Cast<USkeletalMeshComponent>(Component);
				if (bUseSingleMesh)
				{
					MeshFP = MeshTP;
				}
			}
		}
	}

	checkf(MeshFP, TEXT("First Person Mesh Component not assigned, ensure that component name matches FirstPersonMeshComponentName"));
	checkf(MeshTP, TEXT("Third Person Mesh Component not assigned, ensure that component name matches ThirdPersonMeshComponentName"));
	ensureAlwaysMsgf(MeshFP->DoesSocketExist(CameraAttachedSocket), TEXT("Skeleton does NOT have the CameraAttachedSocket: %s"), *CameraAttachedSocket.ToString());

	if (bAutoSetupLinkedAnimLayer && LinkedAnimLayerClass)
	{
		LinkAnimLayerClass();
	}
	else
	{
		ShooterFrameworkAnimInstance = Cast<USKGShooterFrameworkAnimInstance>(IsLocallyControlled() ? MeshFP->GetAnimInstance() : MeshTP->GetAnimInstance());
	}
}

void USKGShooterPawnComponent::SetCameraOffset()
{
	CameraOffset = GetPawnMesh()->GetSocketTransform(CameraAttachedSocket, RTS_ParentBoneSpace);
	FRotator TempRotator = CameraOffset.Rotator();
	TempRotator.Pitch -= 90.0f;
	CameraOffset.SetRotation(TempRotator.Quaternion());
	CameraOffset.SetScale3D(FVector::OneVector);
}

bool USKGShooterPawnComponent::Server_SetOffHandIKHand_Validate(bool bLeftHand)
{
	return true;
}

void USKGShooterPawnComponent::Server_SetOffHandIKHand_Implementation(bool bLeftHand)
{
	if (bOffHandIKIsLeftHand != bLeftHand)
	{
		bOffHandIKIsLeftHand = bLeftHand;
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, bOffHandIKIsLeftHand, this);
	}
}

USKGFirearmComponent* USKGShooterPawnComponent::GetCurrentFirearmComponent() const
{
	return USKGShooterFrameworkHelpers::GetFirearmComponent(HeldActor);
}

USKGProceduralAnimComponent* USKGShooterPawnComponent::GetCurrentProceduralAnimComponent() const
{
	if (CurrentFirearmComponent)
	{
		return CurrentFirearmComponent->GetCurrentProceduralAnimComponent();
	}
	return CurrentProceduralAnimComponent;
}

USKGOpticComponent* USKGShooterPawnComponent::GetCurrentOpticComponent() const
{
	if (const USKGFirearmComponent* FirearmComponent = GetCurrentFirearmComponent())
	{
		return FirearmComponent->GetCurrentOpticComponent();
	}
	return USKGShooterFrameworkHelpers::GetOpticComponent(HeldActor);
}

TArray<USKGLightLaserComponent*> USKGShooterPawnComponent::GetCurrentLightLaserComponents() const
{
	if (const USKGFirearmComponent* FirearmComponent = GetCurrentFirearmComponent())
	{
		return FirearmComponent->GetLightLaserComponents();
	}
	return TArray({ USKGShooterFrameworkHelpers::GetLightLaserComponent(HeldActor) });
}

void USKGShooterPawnComponent::ReplicateYaw(bool bForce)
{
	if (bReplicateRemoteYaw)
	{
		if (bForce)
		{
			const uint8 NewYaw = USKGShooterFrameworkCoreNetworkStatics::CompressFloatToByte(GetControlRotation().Yaw);
			if (RemoteViewYaw != NewYaw)
			{
				RemoteViewYaw = NewYaw;
				if (HasAuthority())
				{
					MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, RemoteViewYaw, this);
				}
				else
				{
					Server_SetRemoteYaw(RemoteViewYaw);
				}
			}
		}
	}
}

void USKGShooterPawnComponent::PoseComplete() const
{
	OnPoseComplete.Broadcast(CurrentProceduralPoseData);
}

USKGShooterFrameworkAnimInstance* USKGShooterPawnComponent::LinkAnimLayerClass()
{
	const USkeletalMeshComponent* MeshToUse = IsLocallyControlled() ? MeshFP : MeshTP;
	ShooterFrameworkAnimInstance = Cast<USKGShooterFrameworkAnimInstance>(MeshToUse->GetLinkedAnimLayerInstanceByClass(LinkedAnimLayerClass));
	if (!ShooterFrameworkAnimInstance)
	{
		MeshFP->LinkAnimClassLayers(LinkedAnimLayerClass);
		if (MeshFP != MeshTP)
		{
			MeshTP->LinkAnimClassLayers(LinkedAnimLayerClass);
		}
		ShooterFrameworkAnimInstance = Cast<USKGShooterFrameworkAnimInstance>(MeshToUse->GetLinkedAnimLayerInstanceByClass(LinkedAnimLayerClass));
	}
	
	if (ShooterFrameworkAnimInstance)
	{
		ShooterFrameworkAnimInstance->SetupShooterPawnComponent();
	}
	return ShooterFrameworkAnimInstance;
}

USKGShooterFrameworkAnimInstance* USKGShooterPawnComponent::LinkAnimLayerClassByInstance(UAnimInstance* AnimInstance)
{
	ShooterFrameworkAnimInstance = Cast<USKGShooterFrameworkAnimInstance>(AnimInstance->GetLinkedAnimLayerInstanceByClass(LinkedAnimLayerClass));
	if (!ShooterFrameworkAnimInstance)
	{
		AnimInstance->LinkAnimClassLayers(LinkedAnimLayerClass);
		ShooterFrameworkAnimInstance = Cast<USKGShooterFrameworkAnimInstance>(AnimInstance->GetLinkedAnimLayerInstanceByClass(LinkedAnimLayerClass));
	}
	
	if (ShooterFrameworkAnimInstance)
	{
		ShooterFrameworkAnimInstance->SetupShooterPawnComponent();
	}
	return ShooterFrameworkAnimInstance;
}

void USKGShooterPawnComponent::UnlinkAnimLayerClass()
{
	MeshFP->UnlinkAnimClassLayers(LinkedAnimLayerClass);
	if (MeshFP != MeshTP)
	{
		MeshTP->UnlinkAnimClassLayers(LinkedAnimLayerClass);
	}
}

void USKGShooterPawnComponent::UnlinkAnimLayerClassByInstance(UAnimInstance* AnimInstance)
{
	AnimInstance->UnlinkAnimClassLayers(LinkedAnimLayerClass);
	ShooterFrameworkAnimInstance = nullptr;
}

bool USKGShooterPawnComponent::Server_SetRemoteYaw_Validate(uint8 Yaw)
{
	return true;
}

void USKGShooterPawnComponent::Server_SetRemoteYaw_Implementation(uint8 Yaw)
{
	if (RemoteViewYaw != Yaw)
	{
		RemoteViewYaw = Yaw;
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, RemoteViewYaw, this);
	}
}

bool USKGShooterPawnComponent::CanAddYaw() const
{
	return (bInFreeLook && bCanAddYaw) || !bInFreeLook;
}

bool USKGShooterPawnComponent::CanAddPitch() const
{
	return (bInFreeLook && bCanAddPitch) || !bInFreeLook;
}

void USKGShooterPawnComponent::SetMouseInput(float X, float Y)
{
	if (bInFreeLook)
	{
		const FRotator Difference = (GetControlRotation() - FreeLookStartRotation).GetNormalized();
		bCanAddYaw = (X >= 0.0f && Difference.Yaw < FreeLookSettings.MaxYawRight) || (X <= 0.0f && Difference.Yaw > -FreeLookSettings.MaxYawLeft);
		bCanAddPitch = (Y <= 0.0f && Difference.Pitch < FreeLookSettings.MaxPitchUp) || (Y >= 0.0f && Difference.Pitch > -FreeLookSettings.MaxPitchDown);
	}
	else
	{
		MouseInput = FVector2D(X, Y);
	}
}

FRotator USKGShooterPawnComponent::GetControlRotation() const
{
	if (OwningPawn)
	{
		if (IsLocallyControlled())
		{
			return OwningPawn->Controller->GetControlRotation();
		}
		else
		{
			const float Yaw = RemoteViewYaw == 0 ? OwningPawn->GetActorRotation().Yaw : USKGShooterFrameworkCoreNetworkStatics::DecompressByteToFloat(RemoteViewYaw);
			return FRotator(USKGShooterFrameworkCoreNetworkStatics::DecompressByteToFloat(OwningPawn->RemoteViewPitch), Yaw, 0.0f);
		}
	}
	return FRotator::ZeroRotator;
}

FSKGProceduralAnimInstanceData USKGShooterPawnComponent::GetProceduralData()
{
	FSKGProceduralAnimInstanceData AnimInstanceData = FSKGProceduralAnimInstanceData();
	ProceduralShooterPawnData = FSKGProceduralShooterPawnData();

	const FVector CameraLocation = GetPawnMesh()->GetSocketLocation(CameraAttachedSocket);
	ProceduralShooterPawnData.FreeLookLookAtLocation = CameraLocation + GetControlRotation().Vector() * FreeLookTraceDistance;
	
	if (HeldActor)
	{
		if (CurrentFirearmComponent)
		{
			AnimInstanceData = CurrentFirearmComponent->GetProceduralData(bIsAiming, bOffHandIKIsLeftHand);
			AnimInstanceData.bProceduralAnimDataSet = true;
			ProceduralShooterPawnData.ProceduralAnimGameplayTag = CurrentFirearmComponent->GetProceduralGameplayTag();
		}
		else if (CurrentProceduralAnimComponent)
		{
			CurrentProceduralAnimComponent->UpdateAimOffset(nullptr);
			AnimInstanceData.AimOffset = CurrentProceduralAnimComponent->GetAimOffset();
			AnimInstanceData.BasePoseOffset = CurrentProceduralAnimComponent->GetBasePoseOffset();
			AnimInstanceData.ThirdPersonAimingOffset = CurrentProceduralAnimComponent->GetThirdPersonAimingOffset(bOffHandIKIsLeftHand);
			AnimInstanceData.CycleAimingPointSettings = CurrentProceduralAnimComponent->GetCycleAimingPointSettings();
			AnimInstanceData.MovementSwaySettings = CurrentProceduralAnimComponent->GetMovementSwaySettings();
			AnimInstanceData.RotationLagSettings = CurrentProceduralAnimComponent->GetRotationSettings();
			AnimInstanceData.DeadzoneSettings = CurrentProceduralAnimComponent->GetDeadzoneSettings();
			AnimInstanceData.RecoilSettings = CurrentProceduralAnimComponent->GetRecoilSettings();
			AnimInstanceData.bProceduralAnimDataSet = true;
			AnimInstanceData.ProceduralStats.AimInterpolationRate = CurrentProceduralAnimComponent->GetProceduralAimingSettings().DefaultAimingSpeed;
			ProceduralShooterPawnData.ProceduralAnimGameplayTag = CurrentProceduralAnimComponent->GetProceduralGameplayTag();
		}
		
		if (bUsingCustomSwayMultiplier)
		{
			AnimInstanceData.MovementSwaySettings.LocationSettings.Multiplier = SwayMultiplier;
			AnimInstanceData.MovementSwaySettings.RotationSettings.Multiplier = SwayMultiplier;
		}
	}

	AnimInstanceData.FirearmCollisionSettings.CollisionChannel = FirearmCollisionChannel;
	AnimInstanceData.MouseInput = MouseInput;
	AnimInstanceData.bInFreeLook = bInFreeLook;
	AnimInstanceData.bOffHandIKIsLeftHand = bOffHandIKIsLeftHand;
	AnimInstanceData.LeanLeftRightSettings = LeanLeftRightSettings;
	AnimInstanceData.FreeLookStartRotation = FreeLookStartRotation;
	ProceduralShooterPawnData.bOffHandIKIsLeftHand = bOffHandIKIsLeftHand;
	return AnimInstanceData;
}

void USKGShooterPawnComponent::OnRep_InFreeLook()
{
	if (FreeLookSettings.bAutoSetUseControllerRotationYaw && OwningPawn)
	{
		OwningPawn->bUseControllerRotationYaw = !bInFreeLook;
	}
}

bool USKGShooterPawnComponent::Server_SetFreeLook_Validate(bool bFreeLook)
{
	return true;
}

void USKGShooterPawnComponent::Server_SetFreeLook_Implementation(bool bFreeLook)
{
	if (bInFreeLook != bFreeLook)
	{
		bInFreeLook = bFreeLook;
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, bInFreeLook, this);
		OnRep_InFreeLook();
	}
}

void USKGShooterPawnComponent::StartFreeLook()
{
	bInFreeLook = true;
	if (FreeLookSettings.bAutoSetUseControllerRotationYaw)
	{
		OwningPawn->bUseControllerRotationYaw = false;
	}
	FreeLookStartRotation = GetControlRotation();
	if (HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, bInFreeLook, this);
	}
	else
	{
		Server_SetFreeLook(bInFreeLook);
	}
}

void USKGShooterPawnComponent::StopFreeLook()
{
	bInFreeLook = false;
	if (ShooterFrameworkAnimInstance)
	{
		FreeLookStartRotation = ShooterFrameworkAnimInstance->GetFreeLookRecoilModifiedRotation();
	}
	OwningPawn->GetController()->SetControlRotation(FreeLookStartRotation);
	if (FreeLookSettings.bAutoSetUseControllerRotationYaw)
	{
		OwningPawn->bUseControllerRotationYaw = true;
	}
	if (HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, bInFreeLook, this);
	}
	else
	{
		Server_SetFreeLook(bInFreeLook);
	}

	ReplicateYaw(true);
}

float USKGShooterPawnComponent::GetMagnificationSensitivityMultiplier() const
{
	if (const USKGOpticComponent* OpticComponent = GetCurrentOpticComponent())
	{
		return 1.0f / OpticComponent->GetCurrentMagnification();
	}
	return 1.0f;
}

void USKGShooterPawnComponent::GetSensitivityMultiplier(const float X, const float XBaseTurnRate, const float Y, const float YBaseTurnRate, float& NewX, bool& AddYaw, float& NewY, bool& AddPitch) const
{
	NewX = X * XBaseTurnRate;
	NewY = Y * YBaseTurnRate;
	if (bInFreeLook)
	{
		AddYaw = CanAddYaw();
		AddPitch = CanAddPitch();
	}
	else if (bIsAiming)
	{
		const float MagnificationSensitivity = GetMagnificationSensitivityMultiplier();
		NewX *= MagnificationSensitivity;
		NewY *= MagnificationSensitivity;
		AddYaw = true;
		AddPitch = true;
	}
	else
	{
		AddYaw = true;
		AddPitch = true;
	}
}

USkeletalMeshComponent* USKGShooterPawnComponent::GetPawnMesh() const
{
	return IsLocallyControlled() ? MeshFP : MeshTP;
}

void USKGShooterPawnComponent::PerformProceduralRecoil(const FRotator& ControlRotationMultiplier, const FVector& LocationMultiplier, const FRotator& RotationMultiplier)
{
	if (ShooterFrameworkAnimInstance)
	{
		ShooterFrameworkAnimInstance->PerformRecoil(ControlRotationMultiplier, LocationMultiplier, RotationMultiplier);
	}
}

FSKGToFromCurveSettings USKGShooterPawnComponent::GetProceduralPoseData(const FGameplayTag& Tag) const
{
	FSKGToFromCurveSettings PoseData;
	if (CurrentFirearmComponent)
	{
		CurrentFirearmComponent->GetPose(Tag, PoseData);
	}
	else if (CurrentProceduralAnimComponent)
	{
		CurrentProceduralAnimComponent->GetPose(Tag, PoseData);
	}
	return PoseData;
}

void USKGShooterPawnComponent::OnRep_CurrentProceduralPoseData() const
{
	if (!IsLocallyControlled() && ShooterFrameworkAnimInstance)
	{
		if (const FSKGToFromCurveSettings PoseData = GetProceduralPoseData(CurrentProceduralPoseData.Tag))
		{
			ShooterFrameworkAnimInstance->TryPerformPose(PoseData, CurrentProceduralPoseData.bExitPose);
		}
	}
}

bool USKGShooterPawnComponent::Server_PerformProceduralPose_Validate(const FGameplayTag& Tag, bool bExitPose)
{
	return true;
}

void USKGShooterPawnComponent::Server_PerformProceduralPose_Implementation(const FGameplayTag& Tag, bool bExitPose)
{
	PerformProceduralPose(Tag, bExitPose);
}

void USKGShooterPawnComponent::PerformProceduralPose(const FGameplayTag& Tag, bool bExitPose)
{
	if (!bIsAiming)
	{
		if (const FSKGToFromCurveSettings PoseData = GetProceduralPoseData(Tag))
		{
			CurrentProceduralPoseData.Tag = Tag;
			CurrentProceduralPoseData.bExitPose = bExitPose;
			if (HasAuthority())
			{
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, CurrentProceduralPoseData, this);	
			}
			else
			{
				Server_PerformProceduralPose(Tag, bExitPose);
			}

			if (ShooterFrameworkAnimInstance)
			{
				ShooterFrameworkAnimInstance->TryPerformPose(PoseData, CurrentProceduralPoseData.bExitPose);
			}
		}
	}
}

bool USKGShooterPawnComponent::Multi_PlayCustomCurveUnreliable_Validate(const FSKGFirstAndThirdPersonCurveSettings& CurveData)
{
	return true;
}

void USKGShooterPawnComponent::Multi_PlayCustomCurveUnreliable_Implementation(
	const FSKGFirstAndThirdPersonCurveSettings& CurveData)
{
	if (!IsLocallyControlled() && ShooterFrameworkAnimInstance)
	{
		ShooterFrameworkAnimInstance->PerformCustomCurve(CurveData);
	}
}

bool USKGShooterPawnComponent::Server_PerformCustomCurveUnreliable_Validate(const FSKGFirstAndThirdPersonCurveSettings& CurveData)
{
	return true;
}

void USKGShooterPawnComponent::Server_PerformCustomCurveUnreliable_Implementation(
	const FSKGFirstAndThirdPersonCurveSettings& CurveData)
{
	if (CurveData)
	{
		Multi_PlayCustomCurveUnreliable(CurveData);
	}
}

bool USKGShooterPawnComponent::Multi_PlayCustomCurve_Validate(const FSKGFirstAndThirdPersonCurveSettings& CurveData)
{
	return true;
}

void USKGShooterPawnComponent::Multi_PlayCustomCurve_Implementation(
	const FSKGFirstAndThirdPersonCurveSettings& CurveData)
{
	if (!IsLocallyControlled() && ShooterFrameworkAnimInstance)
	{
		ShooterFrameworkAnimInstance->PerformCustomCurve(CurveData);
	}
}

bool USKGShooterPawnComponent::Server_PerformCustomCurve_Validate(const FSKGFirstAndThirdPersonCurveSettings& CurveData)
{
	return true;
}

void USKGShooterPawnComponent::Server_PerformCustomCurve_Implementation(
	const FSKGFirstAndThirdPersonCurveSettings& CurveData)
{
	if (CurveData)
	{
		Multi_PlayCustomCurve(CurveData);
	}
}

void USKGShooterPawnComponent::PerformCustomCurve(const FSKGCurveSettings& CurveData)
{
	if (CurveData)
	{
		if (CurveData.ReplicationSettings.bReplicateCurve)
		{
			if (HasAuthority())
			{
				if (CurveData.ReplicationSettings.bReliable)
				{
					Multi_PlayCustomCurve(CurveData.Curve);
				}
				else
				{
					Multi_PlayCustomCurveUnreliable(CurveData.Curve);
				}
			}
			else
			{
				if (CurveData.ReplicationSettings.bReliable)
				{
					Server_PerformCustomCurve(CurveData.Curve);
				}
				else
				{
					Server_PerformCustomCurveUnreliable(CurveData.Curve);
				}
			}
		}
		if (ShooterFrameworkAnimInstance)
		{
			ShooterFrameworkAnimInstance->PerformCustomCurve(CurveData.Curve);
		}
	}
}

void USKGShooterPawnComponent::SetOffHandIKToLeftHand()
{
	if (!bOffHandIKIsLeftHand)
	{
		bOffHandIKIsLeftHand = true;
		if (HasAuthority())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, bOffHandIKIsLeftHand, this);
		}
		else
		{
			Server_SetOffHandIKHand(bOffHandIKIsLeftHand);
		}
		if (CurrentFirearmComponent && CurrentFirearmComponent->IsPointAiming())
		{
			CurrentFirearmComponent->StopPointAiming(bIsAiming);
			CurrentFirearmComponent->StartPointAiming(bOffHandIKIsLeftHand);
		}
	}
}

void USKGShooterPawnComponent::SetOffHandIKToRightHand()
{
	if (bOffHandIKIsLeftHand)
	{
		bOffHandIKIsLeftHand = false;
		if (HasAuthority())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, bOffHandIKIsLeftHand, this);
		}
		else
		{
			Server_SetOffHandIKHand(bOffHandIKIsLeftHand);
		}
		if (CurrentFirearmComponent && CurrentFirearmComponent->IsPointAiming())
		{
			CurrentFirearmComponent->StopPointAiming(bIsAiming);
			CurrentFirearmComponent->StartPointAiming(bOffHandIKIsLeftHand);
		}
	}
}

bool USKGShooterPawnComponent::IsLocallyControlled() const
{
	return OwningPawn && OwningPawn->IsLocallyControlled();
}

void USKGShooterPawnComponent::OnRep_HeldActor(AActor* OldActor)
{
	if (HeldActor)
	{
		if (USKGFirearmComponent* FirearmComponent = GetCurrentFirearmComponent())
		{
			CurrentProceduralAnimComponent = FirearmComponent->GetCurrentProceduralAnimComponent();
			CurrentFirearmComponent = FirearmComponent;
			CurrentFirearmComponent->Held();
		}
		else
		{
			CurrentProceduralAnimComponent = USKGShooterFrameworkHelpers::GetProceduralAnimComponent(HeldActor);
		}
	}
	else
	{
		CurrentProceduralAnimComponent = nullptr;
		CurrentFirearmComponent = nullptr;
	}

	OnHeldActorSet.Broadcast(HeldActor, OldActor);
}

void USKGShooterPawnComponent::OnRep_TargetLeanAngleCompressed()
{
	TargetLeanAngle = USKGShooterFrameworkCoreNetworkStatics::DecompressByteToFloat(TargetLeanAngleCompressed);
	if (TargetLeanAngle > 90.0f)
	{
		TargetLeanAngle -= 360.0f;
	}
}

bool USKGShooterPawnComponent::Server_Lean_Validate(const uint8 TargetAngle)
{
	float Decompressed = USKGShooterFrameworkCoreNetworkStatics::DecompressByteToFloat(TargetLeanAngleCompressed);
	if (Decompressed > 90.0f)
	{
		Decompressed -= 360.0f;
	}
	return Decompressed + 1.0f >= -LeanLeftRightSettings.MaxLeanLeftAngle && Decompressed - 1.0f <= LeanLeftRightSettings.MaxLeanRightAngle;
}

void USKGShooterPawnComponent::Server_Lean_Implementation(const uint8 TargetAngle)
{
	TargetLeanAngleCompressed = TargetAngle;
	MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, TargetLeanAngleCompressed, this);
	OnRep_TargetLeanAngleCompressed();
}

void USKGShooterPawnComponent::LeanLeft(float TargetAngle)
{
	bLeaningLeft = true;
	TargetLeanLeftAngle = FMath::Clamp(-TargetAngle, -LeanLeftRightSettings.MaxLeanLeftAngle, 0.0f);
	float NewTargetAngle = TargetLeanLeftAngle;
	if (bLeaningRight)
	{
		NewTargetAngle = 0.0f;
	}

	if (GetTargetLeanAngle() != NewTargetAngle)
	{
		TargetLeanAngleCompressed = USKGShooterFrameworkCoreNetworkStatics::CompressFloatToByte(NewTargetAngle);
		if (HasAuthority())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, TargetLeanAngleCompressed, this);
		}
		else
		{
			Server_Lean(TargetLeanAngleCompressed);
		}
		OnRep_TargetLeanAngleCompressed();
	}
}

void USKGShooterPawnComponent::LeanRight(float TargetAngle)
{
	bLeaningRight = true;
	TargetLeanRightAngle = FMath::Clamp(TargetAngle, 0.0f, LeanLeftRightSettings.MaxLeanRightAngle);
	float NewTargetAngle = TargetLeanRightAngle;
	if (bLeaningLeft)
	{
		NewTargetAngle = 0.0f;
	}

	if (GetTargetLeanAngle() != NewTargetAngle)
	{
		TargetLeanAngleCompressed = USKGShooterFrameworkCoreNetworkStatics::CompressFloatToByte(NewTargetAngle);
		if (HasAuthority())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, TargetLeanAngleCompressed, this);
		}
		else
		{
			Server_Lean(TargetLeanAngleCompressed);
		}
		OnRep_TargetLeanAngleCompressed();
	}
}

void USKGShooterPawnComponent::StopLeaningLeft()
{
	bLeaningLeft = false;
	TargetLeanLeftAngle = 0.0f;
	float NewTargetAngle = 0.0f;
	
	if (bLeaningRight)
	{
		NewTargetAngle = FMath::Clamp(TargetLeanRightAngle, 0.0f, LeanLeftRightSettings.MaxLeanRightAngle);
	}

	if (GetTargetLeanAngle() != NewTargetAngle)
	{
		TargetLeanAngleCompressed = USKGShooterFrameworkCoreNetworkStatics::CompressFloatToByte(NewTargetAngle);
		if (HasAuthority())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, TargetLeanAngleCompressed, this);
		}
		else
		{
			Server_Lean(TargetLeanAngleCompressed);
		}
		OnRep_TargetLeanAngleCompressed();
	}
}

void USKGShooterPawnComponent::StopLeaningRight()
{
	bLeaningRight = false;
	TargetLeanRightAngle = 0.0f;
	float NewTargetAngle = 0.0f;
	
	if (bLeaningLeft)
	{
		NewTargetAngle = FMath::Clamp(TargetLeanLeftAngle, -LeanLeftRightSettings.MaxLeanLeftAngle, 0.0f);
	}

	if (GetTargetLeanAngle() != NewTargetAngle)
	{
		TargetLeanAngleCompressed = USKGShooterFrameworkCoreNetworkStatics::CompressFloatToByte(NewTargetAngle);
		if (HasAuthority())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, TargetLeanAngleCompressed, this);
		}
		else
		{
			Server_Lean(TargetLeanAngleCompressed);
		}
		OnRep_TargetLeanAngleCompressed();
	}
}

void USKGShooterPawnComponent::SetHeldActor(AActor* Actor)
{
	if (HasAuthority() && Actor)
	{
		AActor* PreviousActor = HeldActor;
		HeldActor = Actor;
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, HeldActor, this);
		OnRep_HeldActor(PreviousActor);
	}
}

void USKGShooterPawnComponent::ClearHeldActor()
{
	if (HasAuthority() && HeldActor)
	{
		AActor* PreviousActor = HeldActor;
		HeldActor = nullptr;
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, HeldActor, this);
		OnRep_HeldActor(PreviousActor);
	}
}

bool USKGShooterPawnComponent::Server_SetAiming_Validate(bool bAim)
{
	return true;
}

void USKGShooterPawnComponent::Server_SetAiming_Implementation(bool bAim)
{
	if (bIsAiming != bAim)
	{
		bIsAiming = bAim;
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, bIsAiming, this);
	}
}

void USKGShooterPawnComponent::StartAiming()
{
	if (!bIsAiming && HeldActor)
	{
		bIsAiming = true;

		if (USKGOpticComponent* OpticComponent = GetCurrentOpticComponent())
		{
			OpticComponent->StartedAiming();
		}
		
		if (HasAuthority())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, bIsAiming, this);
		}
		else
		{			
			Server_SetAiming(true);
		}
	}
}

void USKGShooterPawnComponent::StopAiming()
{
	if (bIsAiming && HeldActor)
	{
		bIsAiming = false;

		if (USKGOpticComponent* OpticComponent = GetCurrentOpticComponent())
		{
			OpticComponent->StoppedAiming();
		}
		
		if (HasAuthority())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGShooterPawnComponent, bIsAiming, this);
		}
		else
		{			
			Server_SetAiming(false);
		}
	}
}

void USKGShooterPawnComponent::SetUseFirstPersonProceduralsAsLocal()
{
	bUseFirstPersonProceduralsAsLocal = true;
}

void USKGShooterPawnComponent::SetUseThirdPersonProceduralsAsLocal()
{
	bUseFirstPersonProceduralsAsLocal = false;
}

void USKGShooterPawnComponent::SetSwayMultiplier(const float Multiplier)
{
	bUsingCustomSwayMultiplier = true;
	SwayMultiplier = Multiplier;
}

void USKGShooterPawnComponent::ResetSwayMultiplier()
{
	bUsingCustomSwayMultiplier = false;
	SwayMultiplier = 1.0f;
}
