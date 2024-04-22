// Copyright 2023, Dakota Dawe, All rights reserved


#include "Components/SKGFirearmComponent.h"
#include "Components/SKGAttachmentManagerComponent.h"
#include "Components/SKGAttachmentComponent.h"
#include "Components/SKGOffHandIKComponent.h"
#include "Components/SKGLightLaserComponent.h"
#include "Components/SKGProceduralAnimComponent.h"
#include "Components/SKGMuzzleComponent.h"
#include "Components/SKGOpticComponent.h"
#include "Components/SKGFirearmAttachmentStatComponent.h"
#include "Components/SKGStockComponent.h"
#include "Statics/SKGShooterFrameworkHelpers.h"
#include "Subsystems/SKGProjectileWorldSubsystem.h"

#include "GameFramework/Actor.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameplayTagAssetInterface.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Statics/SKGAttachmentHelpers.h"

DECLARE_CYCLE_STAT(TEXT("GetProceduralData"), STAT_SKGGetProceduralData, STATGROUP_SKGShooterFrameworkFirearmComponent);
DECLARE_CYCLE_STAT(TEXT("GetProceduralDataCurrentProceduralComponent"), STAT_SKGGetProceduralDataCurrentProceduralComponent, STATGROUP_SKGShooterFrameworkFirearmComponent);

USKGFirearmComponent::USKGFirearmComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void USKGFirearmComponent::BeginPlay()
{
	Super::BeginPlay();

	if (FirearmStatsDataAsset && FirearmStats == FSKGFirearmStats())
	{
		FirearmStats.SetFirearmDefaultStats(FirearmStatsDataAsset);
	}
	
	SetupComponents();
	
	if (FirearmProceduralAnimComponent)
	{
		ProceduralAnimData.CycleAimingPointSettings = FirearmProceduralAnimComponent->GetCycleAimingPointSettings();
		ProceduralAnimData.MovementSwaySettings = FirearmProceduralAnimComponent->GetMovementSwaySettings();
		ProceduralAnimData.MovementLagSettings = FirearmProceduralAnimComponent->GetMovementLagSettings();
		ProceduralAnimData.RotationLagSettings = FirearmProceduralAnimComponent->GetRotationSettings();
		ProceduralAnimData.DeadzoneSettings = FirearmProceduralAnimComponent->GetDeadzoneSettings();
		ProceduralAnimData.RecoilSettings = FirearmProceduralAnimComponent->GetRecoilSettings();
		ProceduralAnimData.BasePoseOffset = FirearmProceduralAnimComponent->GetBasePoseOffset();
		ProceduralAnimData.ThirdPersonAimingOffset = FirearmProceduralAnimComponent->GetThirdPersonAimingOffset(true);
	}

	ProceduralAnimData.FirearmCollisionSettings.bUseFirearmCollision = FirearmCollisionSettings.bUseFirearmCollision;
	ProceduralAnimData.FirearmCollisionSettings.CollisionStopAimingDistance = FirearmCollisionSettings.StopAimingDistance;
	ProceduralAnimData.FirearmCollisionSettings.PoseLocation = FirearmCollisionSettings.PoseLocationCurve;
	ProceduralAnimData.FirearmCollisionSettings.PoseRotation = FirearmCollisionSettings.PoseRotationCurve;
	ProceduralAnimData.FirearmCollisionSettings.PoseScale = FirearmCollisionSettings.PoseScale;
	ProceduralAnimData.FirearmCollisionSettings.PoseLocationInterpSpeed = FirearmCollisionSettings.PoseLocationInterpSpeed;
	ProceduralAnimData.FirearmCollisionSettings.PoseRotationInterpSpeed = FirearmCollisionSettings.PoseRotationInterpSpeed;
	ProceduralAnimData.FirearmCollisionSettings.TraceDiameter = FirearmCollisionSettings.TraceDiameter;
}

void USKGFirearmComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	
	FDoRepLifetimeParams ParamsOwnerOnly;
	ParamsOwnerOnly.bIsPushBased = true;
	ParamsOwnerOnly.Condition = COND_OwnerOnly;

	DOREPLIFETIME_WITH_PARAMS_FAST(USKGFirearmComponent, CurrentProceduralAnimComponent, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGFirearmComponent, CurrentOffHandIKComponent, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGFirearmComponent, CurrentMuzzleComponent, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGFirearmComponent, CurrentOpticComponent, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGFirearmComponent, CurrentStockComponent, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGFirearmComponent, FirearmStats, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGFirearmComponent, MuzzleComponents, Params);
	
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGFirearmComponent, ProceduralAnimComponents, ParamsOwnerOnly);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGFirearmComponent, LightLaserComponents, ParamsOwnerOnly);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGFirearmComponent, OffHandIKComponents, ParamsOwnerOnly);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGFirearmComponent, AttachmentStatComponents, ParamsOwnerOnly);
}

void USKGFirearmComponent::SetupComponents()
{
	ensureAlwaysMsgf(!AttachmentManagerComponentName.IsEqual(NAME_None), TEXT("Attachment Manager Component Name must be valid on Actor: %s"), *GetOwner()->GetName());
	ensureAlwaysMsgf(!FirearmMeshComponentName.IsEqual(NAME_None), TEXT("Firearm Mesh Component Name must be valid on Actor: %s"), *GetOwner()->GetName());

	TInlineComponentArray<UActorComponent*> FirearmsComponents(GetOwner());
	for (UActorComponent* Component : FirearmsComponents)
	{
		if (Component->GetFName().IsEqual(AttachmentManagerComponentName))
		{
			if (USKGAttachmentManagerComponent* FoundAttachmentManager = Cast<USKGAttachmentManagerComponent>(Component))
			{
				AttachmentManager = FoundAttachmentManager;
				if (HasAuthority())
				{
					FoundAttachmentManager->OnAttachmentComponentAttachmentAdded.AddDynamic(this, &USKGFirearmComponent::OnAttachmentAdded);
					FoundAttachmentManager->OnAttachmentComponentAttachmentRemoved.AddDynamic(this, &USKGFirearmComponent::OnAttachmentRemoved);
					for (const FSKGAttachmentComponentItem& AttachmentComponentItem : FoundAttachmentManager->GetAttachmentComponents())
					{
						OnAttachmentAdded(AttachmentComponentItem.AttachmentComponent->GetAttachment());
					}
				}
			}
			else if (Component->GetFName().IsEqual(FirearmMeshComponentName))
			{
				FirearmMesh = Cast<UMeshComponent>(Component);
			}
		}
		else if (Component->GetFName().IsEqual(FirearmMeshComponentName))
		{
			FirearmMesh = Cast<UMeshComponent>(Component);
		}
	}
	
	checkf(AttachmentManager, TEXT("Attachment Manager not found on Actor: %s. Make sure to add this component and that the name: %s is correct"), *GetOwner()->GetName(), *AttachmentManagerComponentName.ToString());
	checkf(FirearmMesh, TEXT("Firearm Mesh not found on Actor: %s. Make sure Firearm Mesh Component Name matches, make sure that the name: %s is correct"), *GetOwner()->GetName(), *FirearmMeshComponentName.ToString());

	for (UActorComponent* Component : FirearmsComponents)
	{
		if (USKGProceduralAnimComponent* ProceduralComponent = Cast<USKGProceduralAnimComponent>(Component))
		{
			FirearmProceduralAnimComponent = ProceduralComponent;
			if (HasAuthority() && !CurrentProceduralAnimComponent)
			{
				if (!CurrentProceduralAnimComponent)
				{
					CurrentProceduralAnimComponent = FirearmProceduralAnimComponent;
					MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentProceduralAnimComponent, this);
					OnRep_CurrentProceduralAnimComponent();
				}
			}
		}
		else if (USKGOffHandIKComponent* OffHandIKComponent = Cast<USKGOffHandIKComponent>(Component))
		{
			FirearmOffHandIKComponent = OffHandIKComponent;
			if (HasAuthority())
			{
				if (!CurrentOffHandIKComponent)
				{
					CurrentOffHandIKComponent = FirearmOffHandIKComponent;
					MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentOffHandIKComponent, this);
					OnRep_CurrentOffHandIKComponent();
				}
			}
		}
		else if (USKGMuzzleComponent* MuzzleComponent = Cast<USKGMuzzleComponent>(Component))
		{
			FirearmMuzzleComponent = MuzzleComponent;
			if (HasAuthority())
			{
				if (!CurrentMuzzleComponent)
				{
					CurrentMuzzleComponent = FirearmMuzzleComponent;
					MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentMuzzleComponent, this);
					OnRep_CurrentMuzzleComponent();
				}
			}
		}
		else if (USKGStockComponent* StockComponent = Cast<USKGStockComponent>(Component))
		{
			CurrentStockComponent = StockComponent;
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentStockComponent, this);
			OnRep_CurrentStockComponent();
			if (const AActor* Stock = CurrentStockComponent->GetOwner())
			{
				UE_LOG(LogTemp, Warning, TEXT("Firearm: Stock Owner: %s"), *Stock->GetName());
				TArray<USKGAttachmentComponent*> StockOwnerAttachmentComponents = USKGAttachmentHelpers::GetAttachmentComponents(Stock);
				for (USKGAttachmentComponent* AttachmentComponent : StockOwnerAttachmentComponents)
				{
					if (AttachmentComponent && AttachmentComponent->GetAttachment() == Stock)
					{
						UE_LOG(LogTemp, Warning, TEXT("FIREARM FOUND STOCK COMPONENT"));
						AttachmentComponent->OnOffsetChanged.AddDynamic(this, &USKGFirearmComponent::OnStockOffsetChanged);
						break;
					}
				}
			}
		}
	}
}

bool USKGFirearmComponent::SetOpticComponent()
{
	bool bChanged = false;
	if (CurrentProceduralAnimComponent)
	{
		USKGOpticComponent* NewOpticComponent = USKGShooterFrameworkHelpers::GetOpticComponent(CurrentProceduralAnimComponent->GetOwner());
		if (NewOpticComponent != CurrentOpticComponent)
		{
			CurrentOpticComponent = NewOpticComponent;
			bChanged = true;
		}
	}
	return bChanged;
}

void USKGFirearmComponent::AddFirearmAttachmentStats_Implementation(USKGFirearmAttachmentStatComponent* StatComponent)
{
	FirearmStats += StatComponent;
}

void USKGFirearmComponent::RemoveFirearmAttachmentStats_Implementation(USKGFirearmAttachmentStatComponent* StatComponent)
{
	FirearmStats -= StatComponent;
}

void USKGFirearmComponent::CalculateProceduralValues_Implementation()
{
	if (FirearmProceduralAnimComponent)
	{
		FSKGProceduralStats ProceduralStats;
		ProceduralStats.AimInterpolationRate = FirearmProceduralAnimComponent->GetProceduralAimingSettings().DefaultAimingSpeed;
		ProceduralStats.CycleAimingPointSpringInterpSettings = FirearmProceduralAnimComponent->GetCycleAimingPointSettings().SpringInterpSettings;
		ProceduralStats.MovementLagSpringInterpSettings = FirearmProceduralAnimComponent->GetMovementLagSettings().SpringInterpSettings;
		ProceduralStats.MovementLagInterpSetting = FirearmProceduralAnimComponent->GetMovementLagSettings().InterpSpeed;
		ProceduralStats.RotationLagSpringInterpSettings = FirearmProceduralAnimComponent->GetRotationSettings().SpringInterpSettings;
		ProceduralStats.RotationLagInterpSettings = FirearmProceduralAnimComponent->GetRotationSettings().InterpSettings;
		
		ProceduralStats.ControlRotationRecoilMultipliers = FVector::OneVector;
		ProceduralStats.RecoilLocationMultipliers = FVector::OneVector;
		ProceduralStats.RecoilRotationMultipliers = FVector::OneVector;
		
		ProceduralAnimData.ProceduralStats = ProceduralStats;

		CalculateProceduralStats.Broadcast(ProceduralAnimData.ProceduralStats);
	}
}

void USKGFirearmComponent::OnAttachmentAdded(AActor* Attachment)
{
	if (Attachment)
	{
		TInlineComponentArray<UActorComponent*> Components(Attachment);
		for (UActorComponent* Component : Components)
		{
			if (USKGProceduralAnimComponent* ProceduralAnimComponent = Cast<USKGProceduralAnimComponent>(Component))
			{
				ProceduralAnimComponents.Add(ProceduralAnimComponent);
				SetBestProceduralAnimComponent();
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, ProceduralAnimComponents, this);
				OnRep_ProceduralAnimComponents();
			}
			else if (USKGLightLaserComponent* LightLaserComponent = Cast<USKGLightLaserComponent>(Component))
			{
				LightLaserComponents.Add(LightLaserComponent);
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, LightLaserComponents, this);
				OnRep_LightLaserComponents();
			}
			else if (USKGOffHandIKComponent* OffHandIKComponent = Cast<USKGOffHandIKComponent>(Component))
			{
				OffHandIKComponents.Add(OffHandIKComponent);
				SetBestOffHandIKComponent();
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, LightLaserComponents, this);
				OnRep_OffHandIKComponents();
			}
			else if (USKGMuzzleComponent* MuzzleComponent = Cast<USKGMuzzleComponent>(Component))
			{
				MuzzleComponents.Add(MuzzleComponent);
				SetBestMuzzleComponent();
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, MuzzleComponents, this);
				OnRep_MuzzleComponents();
			}
			else if (USKGFirearmAttachmentStatComponent* AttachmentStatComponent = Cast<USKGFirearmAttachmentStatComponent>(Component))
			{
				AttachmentStatComponents.Add(AttachmentStatComponent);
				OnRep_AttachmentStatComponents();
				AddFirearmAttachmentStats(AttachmentStatComponent);
				OnRep_FirearmStats();
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, FirearmStats, this);
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, AttachmentStatComponents, this);
			}
			else if (USKGStockComponent* StockComponent = Cast<USKGStockComponent>(Component))
			{
				CurrentStockComponent = StockComponent;
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentStockComponent, this);
				OnRep_CurrentStockComponent();
				if (const AActor* Stock = CurrentStockComponent->GetOwner())
				{
					if (const AActor* StockOwner = Stock->GetOwner())
					{
						TArray<USKGAttachmentComponent*> StockOwnerAttachmentComponents = USKGAttachmentHelpers::GetAttachmentComponents(StockOwner);
						for (USKGAttachmentComponent* AttachmentComponent : StockOwnerAttachmentComponents)
						{
							if (AttachmentComponent && AttachmentComponent->GetAttachment() == Stock)
							{
								CurrentStockComponent->SetOffset(AttachmentComponent->GetAttachmentOffset());
								AttachmentComponent->OnOffsetChanged.AddDynamic(this, &USKGFirearmComponent::OnStockOffsetChanged);
								break;
							}
						}
					}
				}
			}
		}
	}
}

void USKGFirearmComponent::OnAttachmentRemoved(AActor* Attachment)
{
	if (Attachment)
	{
		TInlineComponentArray<UActorComponent*> Components(Attachment);
		for (UActorComponent* Component : Components)
		{
			if (USKGProceduralAnimComponent* ProceduralComponent = Cast<USKGProceduralAnimComponent>(Component))
			{
				if (CurrentProceduralAnimComponent == ProceduralComponent)
            	{
            		CurrentProceduralAnimComponent = nullptr;
					MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentProceduralAnimComponent, this);
            	}
				ProceduralAnimComponents.Remove(ProceduralComponent);
				SetBestProceduralAnimComponent();
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, ProceduralAnimComponents, this);
				OnRep_ProceduralAnimComponents();
			}
			else if (USKGLightLaserComponent* LightLaserComponent = Cast<USKGLightLaserComponent>(Component))
			{
				LightLaserComponents.Remove(LightLaserComponent);
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, LightLaserComponents, this);
				OnRep_LightLaserComponents();
			}
			else if (USKGOffHandIKComponent* OffHandIKComponent = Cast<USKGOffHandIKComponent>(Component))
			{
				OffHandIKComponents.Remove(OffHandIKComponent);
				SetBestOffHandIKComponent();
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, OffHandIKComponents, this);
				OnRep_LightLaserComponents();
			}
			else if (USKGMuzzleComponent* MuzzleComponent = Cast<USKGMuzzleComponent>(Component))
			{
				MuzzleComponents.Remove(MuzzleComponent);
				SetBestMuzzleComponent();
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, MuzzleComponents, this);
				OnRep_ProceduralAnimComponents();
			}
			else if (USKGFirearmAttachmentStatComponent* AttachmentStatComponent = Cast<USKGFirearmAttachmentStatComponent>(Component))
			{
				AttachmentStatComponents.Remove(AttachmentStatComponent);
				OnRep_AttachmentStatComponents();
				RemoveFirearmAttachmentStats(AttachmentStatComponent);
				OnRep_FirearmStats();
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, FirearmStats, this);
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, AttachmentStatComponents, this);
			}
			else if (USKGStockComponent* StockComponent = Cast<USKGStockComponent>(Component))
			{
				if (CurrentStockComponent)
				{
					if (const AActor* Stock = CurrentStockComponent->GetOwner())
					{
						if (const AActor* StockOwner = Stock->GetOwner())
						{
							TArray<USKGAttachmentComponent*> StockOwnerAttachmentComponents = USKGAttachmentHelpers::GetAttachmentComponents(StockOwner);
							for (USKGAttachmentComponent* AttachmentComponent : StockOwnerAttachmentComponents)
							{
								if (AttachmentComponent && AttachmentComponent->GetAttachment() == Stock)
								{
									AttachmentComponent->OnOffsetChanged.RemoveDynamic(this, &USKGFirearmComponent::OnStockOffsetChanged);
									OnStockOffsetChanged(0);
									break;
								}
							}
						}
					}
				}
				CurrentStockComponent = nullptr;
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentStockComponent, this);
				OnRep_CurrentStockComponent();
			}
		}
	}
}

void USKGFirearmComponent::OnStockOffsetChanged(const float Offset)
{
	if (CurrentStockComponent)
	{
		CurrentStockComponent->SetOffset(Offset);
	}
}

bool USKGFirearmComponent::Server_SetAimingDevice_Validate(USKGProceduralAnimComponent* AnimComponent)
{
	return ProceduralAnimComponents.Contains(AnimComponent) || AnimComponent == FirearmProceduralAnimComponent;
}

void USKGFirearmComponent::Server_SetAimingDevice_Implementation(USKGProceduralAnimComponent* AnimComponent)
{
	CurrentProceduralAnimComponent = AnimComponent;
	MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentProceduralAnimComponent, this);
	OnRep_CurrentProceduralAnimComponent();
}

bool USKGFirearmComponent::SetupNewAimingDevice(USKGProceduralAnimComponent* AnimComponent, const bool bIsAiming)
{
	if (AnimComponent && CurrentProceduralAnimComponent != AnimComponent)
	{
		CurrentProceduralAnimComponent = AnimComponent;
		if (CurrentOpticComponent)
		{
			CurrentOpticComponent->StoppedAiming();
		}
		const bool bNewOpticComponent = SetOpticComponent();
		if (bNewOpticComponent)
		{
			if (CurrentOpticComponent && bIsAiming)
			{
				CurrentOpticComponent->StartedAiming();
			}
			OnRep_CurrentOpticComponent();
		}
		OnRep_CurrentProceduralAnimComponent();

		if (HasAuthority())
		{
			if (bNewOpticComponent)
			{
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentOpticComponent, this);
			}
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentProceduralAnimComponent, this);
		}
		else
		{
			Server_SetAimingDevice(CurrentProceduralAnimComponent);
		}
		return true;
	}
	return false;
}

bool USKGFirearmComponent::CycleAimingDevice(bool bIsAiming)
{
	USKGProceduralAnimComponent* NewProceduralComponent = nullptr;
	if (!bIsPointAiming)
	{
		if (ProceduralAnimComponents.Num())
		{
			int32 TempProceduralAnimComponentIndex = FMath::Clamp(ProceduralAnimComponentIndex, 0, ProceduralAnimComponents.Num() - 1);
			for (int32 i = 0; i < ProceduralAnimComponents.Num(); ++i)
			{
				USKGProceduralAnimComponent* TempProceduralAnimComponent = ProceduralAnimComponents[TempProceduralAnimComponentIndex];
				if (TempProceduralAnimComponent && TempProceduralAnimComponent->CanAim() && TempProceduralAnimComponent != CurrentProceduralAnimComponent)
				{
					ProceduralAnimComponentIndex = TempProceduralAnimComponentIndex;
					break;
				}
				if (++TempProceduralAnimComponentIndex > ProceduralAnimComponents.Num() - 1)
				{
					TempProceduralAnimComponentIndex = 0;
				}
			}
			if (ProceduralAnimComponentIndex < ProceduralAnimComponents.Num())
			{
				NewProceduralComponent = ProceduralAnimComponents[ProceduralAnimComponentIndex];
			}
			
			if (NewProceduralComponent && !NewProceduralComponent->CanAim())
			{
				NewProceduralComponent = nullptr;
			}
		}
		else
		{
			NewProceduralComponent = FirearmProceduralAnimComponent;
		}
	}
	return SetupNewAimingDevice(NewProceduralComponent, bIsAiming);;
}

bool USKGFirearmComponent::SetAimingDevice(USKGProceduralAnimComponent* AnimComponent, const bool bIsAiming)
{
	if (!bIsPointAiming && AnimComponent && AnimComponent->CanAim())
	{
		const int32 Index = ProceduralAnimComponents.Find(AnimComponent);
		if (Index != INDEX_NONE)
		{
			ProceduralAnimComponentIndex = Index;
			return SetupNewAimingDevice(AnimComponent, bIsAiming);
		}
	}
	return false;
}

void USKGFirearmComponent::StartPointAiming(bool bRightHandDominant)
{
	if (FirearmProceduralAnimComponent && !bIsPointAiming)
	{
		BeforePointAimProceduralAnimComponent = CurrentProceduralAnimComponent;
		if (FirearmProceduralAnimComponent->StartPointAiming(bRightHandDominant) && FirearmProceduralAnimComponent != CurrentProceduralAnimComponent)
		{
			CurrentProceduralAnimComponent = FirearmProceduralAnimComponent;
			OnRep_CurrentProceduralAnimComponent();
			if (HasAuthority())
			{
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentProceduralAnimComponent, this);
			}
			else
			{
				Server_SetAimingDevice(CurrentProceduralAnimComponent);
			}
		}
		if (CurrentOpticComponent)
		{
			CurrentOpticComponent->StoppedAiming();
			CurrentOpticComponent = nullptr;
		}
		bIsPointAiming = true;
	}
}

void USKGFirearmComponent::StopPointAiming(bool bIsAiming)
{
	if (BeforePointAimProceduralAnimComponent && bIsPointAiming)
	{
		if (CurrentProceduralAnimComponent != BeforePointAimProceduralAnimComponent)
		{
			CurrentProceduralAnimComponent = BeforePointAimProceduralAnimComponent;
			if (SetOpticComponent())
			{
				if (bIsAiming)
				{
					CurrentOpticComponent->StartedAiming();
				}
				if (HasAuthority())
				{
					MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentOpticComponent, this);
				}
			}
			
			OnRep_CurrentProceduralAnimComponent();

			if (HasAuthority())
			{
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentProceduralAnimComponent, this);
			}
			else
			{
				Server_SetAimingDevice(CurrentProceduralAnimComponent);
			}
		}
		
		CurrentProceduralAnimComponent->StopPointAiming();
	}
	else
	{
		CycleAimingDevice(bIsAiming);
	}
	bIsPointAiming = false;
}

FTransform USKGFirearmComponent::GetAimSocketWorldTransform() const
{
	if (CurrentProceduralAnimComponent)
	{
		return CurrentProceduralAnimComponent->GetAimWorldTransform();
	}
	else if (FirearmProceduralAnimComponent)
	{
		return FirearmProceduralAnimComponent->GetAimWorldTransform();
	}
	return FTransform();
}

FGameplayTag USKGFirearmComponent::GetProceduralGameplayTag() const
{
	if (FirearmProceduralAnimComponent)
	{
		return FirearmProceduralAnimComponent->GetProceduralGameplayTag();
	}
	return FGameplayTag();
}

FSKGProceduralAnimInstanceData& USKGFirearmComponent::GetProceduralData(bool bIsAiming, bool bOffHandIKIsLeftHand)
{
	SCOPE_CYCLE_COUNTER(STAT_SKGGetProceduralData);
	if (bIsAiming)
	{
		SCOPE_CYCLE_COUNTER(STAT_SKGGetProceduralDataCurrentProceduralComponent);
		if (CurrentProceduralAnimComponent)
		{
			CurrentProceduralAnimComponent->UpdateAimOffset(FirearmMesh);
			ProceduralAnimData.AimOffset = CurrentProceduralAnimComponent->GetAimOffset();
		}
		else if (FirearmProceduralAnimComponent)
		{
			FirearmProceduralAnimComponent->UpdateAimOffset(FirearmMesh);
			ProceduralAnimData.AimOffset = FirearmProceduralAnimComponent->GetAimOffset();
		}

		if (bOffHandIKIsLeftHand != bOldOffHandIKIsLeftHand && FirearmProceduralAnimComponent)
		{
			bOldOffHandIKIsLeftHand = bOffHandIKIsLeftHand;
			ProceduralAnimData.ThirdPersonAimingOffset = FirearmProceduralAnimComponent->GetThirdPersonAimingOffset(bOffHandIKIsLeftHand);
		}
	}
	
	if (CurrentOffHandIKComponent)
	{
		CurrentOffHandIKComponent->UpdateOffHandIK(FirearmMesh, bOffHandIKIsLeftHand);
		ProceduralAnimData.OffHandIKOffset = CurrentOffHandIKComponent->GetOffHandIKOffset();
		ProceduralAnimData.OffHandIKPose = CurrentOffHandIKComponent->GetOffHandIKPose();
	}
	else if (FirearmOffHandIKComponent)
	{
		FirearmOffHandIKComponent->UpdateOffHandIK(FirearmMesh, bOffHandIKIsLeftHand);
		ProceduralAnimData.OffHandIKOffset = FirearmOffHandIKComponent->GetOffHandIKOffset();
		ProceduralAnimData.OffHandIKPose = FirearmOffHandIKComponent->GetOffHandIKPose();
	}
	
	if (CurrentMuzzleComponent)
	{
		ProceduralAnimData.FirearmCollisionSettings.MuzzleRelativeTransform = CurrentMuzzleComponent->GetMuzzleTransformRelative(FirearmMesh);
		ProceduralAnimData.FirearmCollisionSettings.RootTransform = FirearmMesh->GetComponentTransform();
		ProceduralAnimData.FirearmCollisionSettings.MuzzleTransform = CurrentMuzzleComponent->GetMuzzleTransform();
	}

	if (CurrentStockComponent)
	{
		ProceduralAnimData.LengthOfPull = CurrentStockComponent->GetLengthOfPull();
	}
	else
	{
		ProceduralAnimData.LengthOfPull = 0.0f;
	}
	
	ProceduralAnimData.FirearmCollisionSettings.ActorsToIgnoreForTrace = AttachmentManager->GetAttachments();
	ProceduralAnimData.FirearmCollisionSettings.ActorsToIgnoreForTrace.Add(GetOwner());
	return ProceduralAnimData;
}

void USKGFirearmComponent::SetProceduralStats(const FSKGProceduralStats& ProceduralStatsData)
{
	ProceduralAnimData.ProceduralStats = ProceduralStatsData;
}

void USKGFirearmComponent::Held()
{
	OnHeld.Broadcast();
}

void USKGFirearmComponent::ShotPerformed()
{
	for (USKGMuzzleComponent* MuzzleComponent : MuzzleComponents)
	{
		if (MuzzleComponent)
		{
			MuzzleComponent->ShotPerformed();
		}
	}
}

void USKGFirearmComponent::ZeroOpticsForZeroAtLocation(const FVector& Location)
{
	if (USKGProjectileWorldSubsystem* ProjectileWorldSubsystem = GetWorld()->GetSubsystem<USKGProjectileWorldSubsystem>())
	{
		const FTransform MuzzleTransform = GetMuzzleProjectileTransform(100.0f, 0.0f);
		for (const USKGProceduralAnimComponent* ProceduralAnimComponent : ProceduralAnimComponents)
		{
			if (ProceduralAnimComponent)
			{
				if (USKGOpticComponent* OpticComponent = USKGShooterFrameworkHelpers::GetOpticComponent(ProceduralAnimComponent->GetOwner()))
				{
					FRotator LookAtRotation;
					if (ProjectileWorldSubsystem->GetProjectileZeroAtLocation(LookAtRotation, Location, MuzzleTransform, ProceduralAnimComponent->GetAimWorldTransform()))
					{
						OpticComponent->ApplyLookAtRotationZero(LookAtRotation);
					}
				}
			}
		}
	}
}

FSKGMuzzleTransform USKGFirearmComponent::GetMuzzleProjectileTransform(float ZeroDistanceMeters, float MOA) const
{
	FSKGMuzzleTransform MuzzleTransform;
	/*FTransform AimTransform;
	if (CurrentProceduralAnimComponent)
	{
		AimTransform = CurrentProceduralAnimComponent->GetAimMuzzleTransform();
	}
	else if (FirearmProceduralAnimComponent)
	{
		AimTransform = FirearmProceduralAnimComponent->GetAimMuzzleTransform();
	}
	const FVector Start = AimTransform.GetLocation();
	FVector End = Start + AimTransform.Rotator().Vector() * 120000.0f;
	DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 10.0f, 0, 1.0f);*/
	if (CurrentMuzzleComponent)
	{
		MuzzleTransform = CurrentMuzzleComponent->GetMuzzleProjectileTransform(MOA);
	}
	else if (FirearmMuzzleComponent)
	{
		MuzzleTransform = FirearmMuzzleComponent->GetMuzzleProjectileTransform(MOA);
	}
	return MuzzleTransform;
}

TArray<FSKGMuzzleTransform> USKGFirearmComponent::GetMuzzleProjectileTransforms(float ZeroDistanceMeters, float MOA,
	const int32 ProjectileCount) const
{
	TArray<FSKGMuzzleTransform> MuzzleTransforms;
	MuzzleTransforms.Reserve(ProjectileCount);
	for (int32 i = 0; i < ProjectileCount; ++i)
	{
		MuzzleTransforms.Add(GetMuzzleProjectileTransform(ZeroDistanceMeters, MOA));
	}
	return MuzzleTransforms;
}

FTransform USKGFirearmComponent::GetMuzzleTransform() const
{
	if (CurrentMuzzleComponent)
	{
		return CurrentMuzzleComponent->GetMuzzleTransform();
	}
	else if (FirearmMuzzleComponent)
	{
		return FirearmMuzzleComponent->GetMuzzleTransform();
	}
	return FTransform();
}

bool USKGFirearmComponent::GetPose(FGameplayTag Tag, FSKGToFromCurveSettings& PoseData)
{
	if (FirearmProceduralAnimComponent)
	{
		return FirearmProceduralAnimComponent->GetPose(Tag, PoseData);
	}
	return false;
}

void USKGFirearmComponent::SetBestMuzzleComponent()
{
	USKGMuzzleComponent* FoundMuzzle = nullptr;
	FGameplayTag CurrentFoundTag;
	for (USKGMuzzleComponent* Muzzle : MuzzleComponents)
	{
		FGameplayTag GameplayTag = Muzzle->GetMuzzleTag();
		if (GameplayTag == SKGGAMEPLAYTAGS::MuzzleComponentSuppressor)
		{
			FoundMuzzle = Muzzle;
			break;
		}
		if (GameplayTag == SKGGAMEPLAYTAGS::MuzzleComponentMuzzleDevice)
		{
			FoundMuzzle = Muzzle;
			CurrentFoundTag = SKGGAMEPLAYTAGS::MuzzleComponentMuzzleDevice;
		}
		else if (GameplayTag == SKGGAMEPLAYTAGS::MuzzleComponentBarrel && CurrentFoundTag != SKGGAMEPLAYTAGS::MuzzleComponentMuzzleDevice)
		{
			FoundMuzzle = Muzzle;
			CurrentFoundTag = SKGGAMEPLAYTAGS::MuzzleComponentBarrel;
		}
	}

	if (CurrentMuzzleComponent != FoundMuzzle)
	{
		CurrentMuzzleComponent = FoundMuzzle;
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentMuzzleComponent, this);
		OnRep_CurrentMuzzleComponent();
	}
}

void USKGFirearmComponent::SetBestOffHandIKComponent()
{
	USKGOffHandIKComponent* FoundOffHandIKComponent = nullptr;
	FGameplayTag CurrentFoundTag;
	for (USKGOffHandIKComponent* OffHandIKComponent : OffHandIKComponents)
	{
		if (const IGameplayTagAssetInterface* GameplayTagInterface = Cast<IGameplayTagAssetInterface>(OffHandIKComponent))
		{
			if (GameplayTagInterface->HasMatchingGameplayTag(SKGGAMEPLAYTAGS::OffHandIKComponentForwardGrip))
			{
				FoundOffHandIKComponent = OffHandIKComponent;
				break;
			}
			if (GameplayTagInterface->HasMatchingGameplayTag(SKGGAMEPLAYTAGS::OffHandIKComponentHandguard))
			{
				FoundOffHandIKComponent = OffHandIKComponent;
				CurrentFoundTag = SKGGAMEPLAYTAGS::OffHandIKComponentHandguard;
			}
			else if (GameplayTagInterface->HasMatchingGameplayTag(SKGGAMEPLAYTAGS::OffHandIKComponentFirearm) && CurrentFoundTag != SKGGAMEPLAYTAGS::OffHandIKComponentHandguard)
			{
				FoundOffHandIKComponent = OffHandIKComponent;
				CurrentFoundTag = SKGGAMEPLAYTAGS::OffHandIKComponentFirearm;
			}
		}
	}
	if (!CurrentFoundTag.IsValid())
	{
		FoundOffHandIKComponent = FirearmOffHandIKComponent;
	}
	if (CurrentOffHandIKComponent != FoundOffHandIKComponent)
	{
		CurrentOffHandIKComponent = FoundOffHandIKComponent;
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentOffHandIKComponent, this);
		OnRep_CurrentOffHandIKComponent();
	}
}

void USKGFirearmComponent::SetBestProceduralAnimComponent()
{
	if (!CurrentProceduralAnimComponent || (CurrentProceduralAnimComponent == FirearmProceduralAnimComponent))
	{
		USKGProceduralAnimComponent* NewProceduralAnimComponent = nullptr;
		for (int32 i = 0; i < ProceduralAnimComponents.Num(); ++i)
		{
			USKGProceduralAnimComponent* ProceduralAnimComponent = ProceduralAnimComponents[i];
			if (ProceduralAnimComponent && ProceduralAnimComponent->CanAim())
			{
				NewProceduralAnimComponent = ProceduralAnimComponent;
				ProceduralAnimComponentIndex = i;
				break;
			}
		}
		
		if (NewProceduralAnimComponent)
		{
			CurrentProceduralAnimComponent = NewProceduralAnimComponent;
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentProceduralAnimComponent, this);
			OnRep_CurrentProceduralAnimComponent();
		}
		else
		{
			CurrentProceduralAnimComponent = FirearmProceduralAnimComponent;
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentProceduralAnimComponent, this);
			OnRep_CurrentProceduralAnimComponent();
		}
		
		if (SetOpticComponent())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGFirearmComponent, CurrentOpticComponent, this);
			OnRep_CurrentOpticComponent();
		}
	}
}

void USKGFirearmComponent::OnRep_FirearmStats()
{
	OnFirearmStatsChanged.Broadcast(FirearmStats);

	CalculateProceduralValues();
}

void USKGFirearmComponent::OnRep_ProceduralAnimComponents()
{
	OnProceduralAnimComponentsUpdated.Broadcast();
}

void USKGFirearmComponent::OnRep_AttachmentStatComponents()
{
	
}

void USKGFirearmComponent::OnRep_CurrentProceduralAnimComponent()
{
	if (CurrentProceduralAnimComponent && FirearmMesh)
	{
		CurrentProceduralAnimComponent->UpdateAimOffset(FirearmMesh);
		ProceduralAnimData.AimOffset = CurrentProceduralAnimComponent->GetAimOffset();
		OnAimingDeviceCycled.Broadcast(CurrentProceduralAnimComponent);
	}
}

void USKGFirearmComponent::OnRep_CurrentMuzzleComponent()
{
	OnMuzzleComponentUpdated.Broadcast();
}
