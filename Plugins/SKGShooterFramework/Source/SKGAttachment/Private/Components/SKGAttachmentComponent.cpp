// Copyright 2023, Dakota Dawe, All rights reserved

#include "Components/SKGAttachmentComponent.h"
#include "DataAssets/SKGPDAAttachmentCompatibility.h"
#include "Components/SKGAttachmentManagerComponent.h"
#include "Statics/SKGAttachmentHelpers.h"

#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

DECLARE_CYCLE_STAT(TEXT("SKGDestroyAttachment"), STAT_SKGDestroyAttachment, STATGROUP_SKGAttachmentComponent);

// Sets default values for this component's properties
USKGAttachmentComponent::USKGAttachmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	
}

void USKGAttachmentComponent::OnRep_Attachment(AActor* OldAttachment)
{
	// Destroy as a precaution to make it easier
	DestroyPreviewAttachment();
	OnAttachmentChanged.Broadcast();
	if (Attachment)
	{
		OnAttachmentAdded.Broadcast(Attachment);
		SetupAttachmentWithLeaderPoseComponent(Attachment);
		SetRelativeAttachmentOffset(Attachment);
	}
	if (OldAttachment)
	{
		OnAttachmentRemoved.Broadcast(OldAttachment);
	}
}

void USKGAttachmentComponent::BeginPlay()
{
	Super::BeginPlay();

	SetupComponents();
	SetupCompatibilityCache();
	
	if (HasAuthority())
	{
		RegisterComponentWithManager(this);

		if (const USKGAttachmentManagerComponent* AttachmentManagerComponent = CachedAttachmentManager.Get())
		{
			if (AttachmentManagerComponent->CanSpawnDefaultAttachments())
			{
				TSoftClassPtr<AActor> InitialAttachment = DefaultAttachment;
				if (bRandomDefaultAttachment && CompatibleAttachments.Num())
				{
					if (USKGPDAAttachmentCompatibility* PDA  = CompatibleAttachments[FMath::RandRange(0, CompatibleAttachments.Num() - 1)])
					{
						const int32 Index = FMath::RandRange(0, PDA->Attachments.Num() - 1);
						InitialAttachment = PDA->Attachments[Index].Class;
					}
				}
				TryLoadSetupAttachment(InitialAttachment);
			}
		}

		if (AttachmentOffset != OffsetSettings.DefaultAttachmentOffset)
		{
			// Clamp it regardless due to weird editor bug not fully correcting it in PostEditChangeProperties
			UE_LOG(LogTemp, Warning, TEXT("OffsetSettings.DefaultAttachmentOffset: %f"), OffsetSettings.DefaultAttachmentOffset);
			OffsetSettings.DefaultAttachmentOffset = FMath::Clamp(OffsetSettings.DefaultAttachmentOffset, OffsetSettings.MinimumOffsetAllowed, OffsetSettings.MaximumOffsetAllowed);
			AttachmentOffset = OffsetSettings.DefaultAttachmentOffset;
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGAttachmentComponent, AttachmentOffset, this);
			TryForceNetUpdate();
			OnRep_AttachmentOffset();
		}
	}
}

#if WITH_EDITOR
void USKGAttachmentComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (OffsetSettings.MinimumOffsetAllowed != 0.0f)
	{
		OffsetSettings.MinimumOffsetAllowed = FMath::RoundToInt(OffsetSettings.MinimumOffsetAllowed / OffsetSettings.OffsetSnapDistance) * OffsetSettings.OffsetSnapDistance;
	}
	if (OffsetSettings.MaximumOffsetAllowed != 0.0f)
	{
		OffsetSettings.MaximumOffsetAllowed = FMath::RoundToInt(OffsetSettings.MaximumOffsetAllowed / OffsetSettings.OffsetSnapDistance) * OffsetSettings.OffsetSnapDistance;
	}
	if (OffsetSettings.DefaultAttachmentOffset != 0.0f)
	{
		OffsetSettings.DefaultAttachmentOffset = FMath::Clamp(FMath::RoundToInt(OffsetSettings.DefaultAttachmentOffset / OffsetSettings.OffsetSnapDistance) * OffsetSettings.OffsetSnapDistance, OffsetSettings.MinimumOffsetAllowed, OffsetSettings.MaximumOffsetAllowed);
	}
}
#endif

void USKGAttachmentComponent::SetupComponents()
{
	ensureAlwaysMsgf(!AttachToSocket.IsEqual(NAME_None), TEXT("AttachToSocket is NOT valid (None) on Actor: %s"), *GetOwner()->GetName());
	for (UActorComponent* Component : GetOwner()->GetComponents())
	{
		if (Component && Component->GetFName().IsEqual(AttachToMeshName))
		{
			AttachToMesh = Cast<UMeshComponent>(Component);
			ensureAlwaysMsgf(AttachToMesh->DoesSocketExist(AttachToSocket), TEXT("AttachToMesh on Actor: %s Does NOT contain socket: %s"), *GetOwner()->GetName(), *AttachToSocket.ToString());
			break;
		}
	}

	ensureAlwaysMsgf(AttachToMesh, TEXT("Could not find mesh with name: %s on Actor: %s. Ensure it exists and the name matches what is entered in AttachToMeshName"), *AttachToMeshName.ToString(), *GetOwner()->GetName());
}

void USKGAttachmentComponent::SetupCompatibilityCache()
{
	for (const USKGPDAAttachmentCompatibility* AttachmentsData : CompatibleAttachments)
	{
		if (ensureAlwaysMsgf(AttachmentsData, TEXT("Attachments Data is invalid in CompatibleAttachments on Actor: %s in Component: %s (CompatibleAttachments has an element in it that does not contain a data asset"), *GetOwner()->GetName(), *GetName()))
		{
			CachedCompatibleAttachments.Reserve(CachedCompatibleAttachments.Num() + AttachmentsData->Attachments.Num());
			for (const FSKGDAAttachment& AttachmentData : AttachmentsData->Attachments)
			{
				CachedCompatibleAttachments.Add(AttachmentData);
			}
		}
	}
}

void USKGAttachmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (EndPlayReason == EEndPlayReason::Type::Destroyed)
	{
		UnregisterAttachmentWithManager(Attachment);
		UnregisterComponentWithManager(this);
	}
}

void USKGAttachmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(USKGAttachmentComponent, Attachment, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGAttachmentComponent, AttachmentOffset, Params);
}

void USKGAttachmentComponent::TryForceNetUpdate() const
{
	if (bAutoCallForceNetUpdate)
	{
		GetOwner()->ForceNetUpdate();
	}
}

FVector USKGAttachmentComponent::GetAttachmentDirectionVector() const
{
	FVector DirectionalVector = FVector::ZeroVector;
	if (AttachToMesh)
	{
		switch (OffsetSettings.OffsetAxis)
		{
		case EAxis::Y : DirectionalVector = AttachToMesh->GetRightVector(); break;
		case EAxis::X : DirectionalVector = AttachToMesh->GetForwardVector(); break;
		case EAxis::Z : DirectionalVector = AttachToMesh->GetUpVector(); break;
		default : DirectionalVector = AttachToMesh->GetForwardVector();
		}
	}
	return DirectionalVector;
}

USKGAttachmentManagerComponent* USKGAttachmentComponent::FindAttachmentManager() const
{
	for (const AActor* CurrentOwner = GetOwner(); CurrentOwner; CurrentOwner = CurrentOwner->GetOwner())
	{
		if (USKGAttachmentManagerComponent* AttachmentManager = USKGAttachmentHelpers::GetAttachmentManagerComponent(CurrentOwner))
		{
			return AttachmentManager;
		}
	}
	return nullptr;
}

bool USKGAttachmentComponent::Server_TrySetupAttachment_Validate(UClass* AttachmentClass)
{
	return bAllowClientSideModification && IsAttachmentClassCompatible(AttachmentClass);
}

void USKGAttachmentComponent::Server_TrySetupAttachment_Implementation(UClass* AttachmentClass)
{
	TrySetupAttachment(AttachmentClass);
}

bool USKGAttachmentComponent::Server_TrySetupExistingAttachment_Validate(AActor* AttachmentToSetup)
{
	return bAllowClientSideModification && IsAttachmentCompatible(AttachmentToSetup);
}

void USKGAttachmentComponent::Server_TrySetupExistingAttachment_Implementation(AActor* AttachmentToSetup)
{
	TrySetupExistingAttachment(AttachmentToSetup);
}

bool USKGAttachmentComponent::Server_TrySetupExistingAttachmentNoAttach_Validate(AActor* AttachmentToSetup)
{
	return bAllowClientSideModification && IsAttachmentCompatible(AttachmentToSetup);
}

void USKGAttachmentComponent::Server_TrySetupExistingAttachmentNoAttach_Implementation(AActor* AttachmentToSetup)
{
	TrySetupExistingAttachmentNoAttach(AttachmentToSetup);
}

bool USKGAttachmentComponent::Server_RemoveAttachment_Validate()
{
	return bAllowClientSideModification;
}

void USKGAttachmentComponent::Server_RemoveAttachment_Implementation()
{
	RemoveAttachment();
}

bool USKGAttachmentComponent::Server_DestroyAttachment_Validate()
{
	return bAllowClientSideModification;
}

void USKGAttachmentComponent::Server_DestroyAttachment_Implementation()
{
	DestroyAttachment();
}

USKGAttachmentManagerComponent* USKGAttachmentComponent::GetAttachmentManager()
{
	USKGAttachmentManagerComponent* AttachmentManager = CachedAttachmentManager.Get();
	if (!AttachmentManager)
	{
		AttachmentManager = FindAttachmentManager();
		CachedAttachmentManager = AttachmentManager;
	}
	return AttachmentManager;
}

void USKGAttachmentComponent::RegisterComponentWithManager(USKGAttachmentComponent* Component)
{
	if (Component)
	{
		if (USKGAttachmentManagerComponent* AttachmentManager = GetAttachmentManager())
		{
			AttachmentManager->RegisterAttachmentComponent(Component);
		}
	}
}

void USKGAttachmentComponent::UnregisterComponentWithManager(USKGAttachmentComponent* Component)
{
	if (Component)
	{
		if (USKGAttachmentManagerComponent* AttachmentManager = GetAttachmentManager())
		{
			AttachmentManager->UnregisterAttachmentComponent(Component);
		}
	}
}

void USKGAttachmentComponent::RegisterAttachmentWithManager(AActor* AttachmentToRegister)
{
	if (AttachmentToRegister)
	{
		if (USKGAttachmentManagerComponent* AttachmentManager = GetAttachmentManager())
		{
			AttachmentManager->RegisterAttachmentComponentsAttachment(this, AttachmentToRegister);
		}
	}
}

void USKGAttachmentComponent::UnregisterAttachmentWithManager(AActor* AttachmentToRegister)
{
	if (AttachmentToRegister)
	{
		if (USKGAttachmentManagerComponent* AttachmentManager = GetAttachmentManager())
		{
			AttachmentManager->UnregisterAttachmentComponentsAttachment(this, AttachmentToRegister);
		}
	}
}

void USKGAttachmentComponent::TryLoadSetupAttachment(const TSoftClassPtr<AActor>& AttachmentToLoad)
{	// Check compatibility here
	if (!AttachmentToLoad.IsNull())
	{
		if (AttachmentToLoad.IsValid())
		{
			OnAttachmentLoadedForSetup(AttachmentToLoad);
		}
		else
		{
			const FStreamableDelegate Delegate = FStreamableDelegate::CreateUObject(this, &USKGAttachmentComponent::OnAttachmentLoadedForSetup, AttachmentToLoad);
			UAssetManager::GetStreamableManager().RequestAsyncLoad({AttachmentToLoad.ToSoftObjectPath()}, Delegate, FStreamableManager::DefaultAsyncLoadPriority, false, false);
		}
	}
}

void USKGAttachmentComponent::TrySetupAttachment(UClass* AttachmentClass)
{
	if (HasAuthority())
	{
		if (IsAttachmentClassCompatible(AttachmentClass))
		{
			FActorSpawnParameters Params;
			Params.Owner = GetOwner();
			if (AActor* SpawnedAttachment = GetWorld()->SpawnActor(AttachmentClass, &GetOwner()->GetActorTransform(), Params))
			{
				SetupAttachment(SpawnedAttachment);
			}
		}
	}
	else if (bAllowClientSideModification && IsAttachmentClassCompatible(AttachmentClass))
	{
		Server_TrySetupAttachment(AttachmentClass);
	}
}

void USKGAttachmentComponent::TrySetupExistingAttachment(AActor* AttachmentToSetup)
{
	if (AttachmentToSetup)
	{
		if (HasAuthority())
		{
			if (IsAttachmentCompatible(AttachmentToSetup))
			{
				SetupAttachment(AttachmentToSetup);
				TArray<USKGAttachmentComponent*> AttachmentComponents = USKGAttachmentHelpers::GetAttachmentComponents(AttachmentToSetup);
				for (USKGAttachmentComponent* Component : AttachmentComponents)
				{
					RegisterComponentWithManager(Component);
				}
				
				TArray<AActor*> AttachedToAttachmentToSetupActors;
				AttachmentToSetup->GetAttachedActors(AttachedToAttachmentToSetupActors, true, true);
				for (AActor* Attached : AttachedToAttachmentToSetupActors)
				{
					RegisterAttachmentWithManager(Attached);
					AttachmentComponents = USKGAttachmentHelpers::GetAttachmentComponents(Attached);
					for (USKGAttachmentComponent* Component : AttachmentComponents)
					{
						RegisterComponentWithManager(Component);
					}
				}
			}
		}
		else if (bAllowClientSideModification && IsAttachmentCompatible(AttachmentToSetup))
		{
			Server_TrySetupExistingAttachment(AttachmentToSetup);
		}
	}
}

void USKGAttachmentComponent::TrySetupExistingAttachmentNoAttach(AActor* AttachmentToSetup)
{
	if (AttachmentToSetup)
	{
		if (HasAuthority())
		{
			if (IsAttachmentCompatible(AttachmentToSetup))
			{
				AActor* OldAttachment = Attachment;
				Attachment = AttachmentToSetup;
				Attachment->SetOwner(GetOwner());
				TArray<USKGAttachmentComponent*> AttachmentComponents = USKGAttachmentHelpers::GetAttachmentComponents(AttachmentToSetup);
				for (USKGAttachmentComponent* Component : AttachmentComponents)
				{
					RegisterComponentWithManager(Component);
				}
				RegisterAttachmentWithManager(Attachment);
				DestroyPreviewAttachment();
				MARK_PROPERTY_DIRTY_FROM_NAME(USKGAttachmentComponent, Attachment, this);
				OnRep_Attachment(OldAttachment);
				TryForceNetUpdate();
				
				TArray<AActor*> AttachedToAttachmentToSetupActors;
				AttachmentToSetup->GetAttachedActors(AttachedToAttachmentToSetupActors, true, true);
				for (AActor* Attached : AttachedToAttachmentToSetupActors)
				{
					RegisterAttachmentWithManager(Attached);
					AttachmentComponents = USKGAttachmentHelpers::GetAttachmentComponents(Attached);
					for (USKGAttachmentComponent* Component : AttachmentComponents)
					{
						RegisterComponentWithManager(Component);
					}
				}
			}
		}
		else if (bAllowClientSideModification && IsAttachmentCompatible(AttachmentToSetup))
		{
			Server_TrySetupExistingAttachmentNoAttach(AttachmentToSetup);
		}
	}
}

AActor* USKGAttachmentComponent::RemoveAttachment()
{
	if (Attachment)
	{
		if (HasAuthority())
		{
			AActor* CurrentAttachment = Attachment;
			UnregisterAttachmentWithManager(Attachment);
			TArray<USKGAttachmentComponent*> AttachmentComponents = USKGAttachmentHelpers::GetAttachmentComponents(Attachment);
			for (USKGAttachmentComponent* Component : AttachmentComponents)
			{
				UnregisterComponentWithManager(Component);
			}
			
			TArray<AActor*> AttachedToAttachmentToSetupActors;
			Attachment->GetAttachedActors(AttachedToAttachmentToSetupActors, true, true);
			for (AActor* Attached : AttachedToAttachmentToSetupActors)
			{
				UnregisterAttachmentWithManager(Attached);
				AttachmentComponents = USKGAttachmentHelpers::GetAttachmentComponents(Attached);
				for (USKGAttachmentComponent* Component : AttachmentComponents)
				{
					UnregisterComponentWithManager(Component);
				}
			}
			
			Attachment = nullptr;
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGAttachmentComponent, Attachment, this);
			OnRep_Attachment(CurrentAttachment);
			TryForceNetUpdate();

			return CurrentAttachment;
		}
		else if (bAllowClientSideModification)
		{
			Server_RemoveAttachment();
		}
	}
	return nullptr;
}

void USKGAttachmentComponent::DestroyAttachment()
{
	if (Attachment)
	{
		if (HasAuthority())
		{
			SCOPE_CYCLE_COUNTER(STAT_SKGDestroyAttachment);
			TArray<AActor*> RootAttachedActors;
			Attachment->GetAttachedActors(RootAttachedActors, false, true);
			TArray<AActor*> AllAttachedActors = RootAttachedActors;
			AllAttachedActors.Reserve(AllAttachedActors.Num() + 30);

			for (AActor* Actor : RootAttachedActors)
			{
				if (Actor)
				{
					Actor->Destroy();
				}
			}
		
			OnAttachmentDestroyed.Broadcast(Attachment);
			UnregisterAttachmentWithManager(Attachment);
			Attachment->Destroy();
			Attachment = nullptr;
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGAttachmentComponent, Attachment, this);
			OnRep_Attachment(nullptr);
			TryForceNetUpdate();
		}
		else if (bAllowClientSideModification)
		{
			Server_DestroyAttachment();
		}
	}
}

const TArray<FSKGDAAttachment>& USKGAttachmentComponent::GetCompatibleAttachments() const
{
	return CachedCompatibleAttachments;
}

bool USKGAttachmentComponent::IsAttachmentClassCompatible(const UClass* Class)
{
	if (Class && CompatibleAttachments.Num())
	{
		for (const FSKGDAAttachment& AttachmentData : CachedCompatibleAttachments)
		{
			if (AttachmentData.Class == Class)
			{
				return true;
			}
		}
		return false;
	}
	// If no compatible attachments specified, allow all
	return Class ? true : false;
}

bool USKGAttachmentComponent::IsAttachmentCompatible(const AActor* Actor)
{	// If actor is valid, return if its class is allowed, otherwise false
	return Actor ? IsAttachmentClassCompatible(Actor->GetClass()) : false;
}

void USKGAttachmentComponent::SetPreviewAttachment(UClass* AttachmentClass)
{
	if (IsAttachmentClassCompatible(AttachmentClass))
	{
		DestroyPreviewAttachment();
		
		PreviewAttachment = GetWorld()->SpawnActorDeferred<AActor>(AttachmentClass, FTransform());
		if (PreviewAttachment)
		{
			PreviewAttachment->SetActorEnableCollision(false);
			PreviewAttachment->SetReplicates(false);
			UGameplayStatics::FinishSpawningActor(PreviewAttachment, FTransform());
			AttachAttachment(PreviewAttachment);
			SetupAttachmentWithLeaderPoseComponent(PreviewAttachment);
			SetRelativeAttachmentOffset(PreviewAttachment);

			if (Attachment && !Attachment->IsHidden())
			{
				Attachment->SetActorHiddenInGame(true);
			}

			OnPreviewAttachmentAdded.Broadcast(PreviewAttachment);
		}
	}
}

void USKGAttachmentComponent::DestroyPreviewAttachment()
{
	if (PreviewAttachment)
	{
		OnPreviewAttachmentRemoved.Broadcast(PreviewAttachment);
		PreviewAttachment->Destroy();
	}
	if (Attachment && Attachment->IsHidden())
	{
		Attachment->SetActorHiddenInGame(false);
	}
}

FTransform USKGAttachmentComponent::GetAttachOffsetTransform() const
{
	FTransform StartTransform = GetAttachTransform();
	if (OffsetSettings.MinimumOffsetAllowed < 0.0f)
	{
		const FVector StartLocation = StartTransform.GetLocation() + GetAttachmentDirectionVector() * AttachmentOffset;
		StartTransform.SetLocation(StartLocation);
	}
	return StartTransform;
}

FTransform USKGAttachmentComponent::GetAttachTransform() const
{
	return AttachToMesh ? AttachToMesh->GetSocketTransform(AttachToSocket) : FTransform();
}

FTransform USKGAttachmentComponent::GetAttachStartTransform() const
{
	FTransform StartTransform = GetAttachTransform();
	if (OffsetSettings.MinimumOffsetAllowed < 0.0f)
	{
		const FVector StartLocation = StartTransform.GetLocation() + GetAttachmentDirectionVector() * OffsetSettings.MinimumOffsetAllowed;
		StartTransform.SetLocation(StartLocation);
	}
	return StartTransform;
}

FTransform USKGAttachmentComponent::GetAttachEndTransform() const
{
	FTransform StartTransform = GetAttachTransform();
	if (OffsetSettings.MaximumOffsetAllowed > 0.0f)
	{
		const FVector StartLocation = StartTransform.GetLocation() + GetAttachmentDirectionVector() * OffsetSettings.MaximumOffsetAllowed;
		StartTransform.SetLocation(StartLocation);
	}
	return StartTransform;
}

TArray<FVector> USKGAttachmentComponent::GetAttachSnapPoints() const
{
	TArray<FVector> WorldSnapPoints;
	const int32 SnapPointCount = OffsetSettings.GetSnapPointCount();
	WorldSnapPoints.Reserve(SnapPointCount + 2);// +2 to account for start/end point

	const FVector Direction = GetAttachmentDirectionVector();
	FVector WorldSnapPoint = AttachToMesh->GetSocketLocation(AttachToSocket) + Direction * OffsetSettings.MinimumOffsetAllowed;
	for (int i = 0; i < SnapPointCount + 1; ++i)
	{
		WorldSnapPoints.Add(WorldSnapPoint);
		WorldSnapPoint += Direction * OffsetSettings.OffsetSnapDistance;
	}
	return WorldSnapPoints;
}

void USKGAttachmentComponent::OnAttachmentLoadedForSetup(TSoftClassPtr<AActor> LoadedAttachment)
{
	if (LoadedAttachment.IsValid())
	{
		TrySetupAttachment(LoadedAttachment.Get());
	}
}

void USKGAttachmentComponent::SetupAttachment(AActor* AttachmentToSetup)
{
	if (AttachmentToSetup)
	{
		AActor* OldAttachment = Attachment;
		Attachment = AttachmentToSetup;
		Attachment->SetOwner(GetOwner());
		AttachAttachment(Attachment);
		RegisterAttachmentWithManager(Attachment);
		DestroyPreviewAttachment();
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGAttachmentComponent, Attachment, this);
		OnRep_Attachment(OldAttachment);
		TryForceNetUpdate();
	}
}

void USKGAttachmentComponent::AttachAttachment(AActor* AttachmentToSetup) const
{
	const FAttachmentTransformRules TransformRules(AttachmentRules.AttachLocationRule, AttachmentRules.AttachRotationRule, AttachmentRules.AttachScaleRule, AttachmentRules.bWeldSimulatedBodies);
	AttachmentToSetup->AttachToComponent(AttachToMesh, TransformRules, AttachToSocket);
}

void USKGAttachmentComponent::SetupAttachmentWithLeaderPoseComponent(AActor* AttachmentToSetup) const
{
	if (bAutoSetLeaderPoseComponent)
	{
		if (USkeletalMeshComponent* AttachToSkeletalMesh = Cast<USkeletalMeshComponent>(AttachToMesh))
		{
			TArray<USkeletalMeshComponent*> Components;
			AttachmentToSetup->GetComponents<USkeletalMeshComponent>(Components);
			for (USkeletalMeshComponent* MeshComponent : Components)
			{
				if (MeshComponent && MeshComponent->GetFName().IsEqual(LeaderPoseAttachmentMeshName))
				{
					MeshComponent->SetLeaderPoseComponent(AttachToSkeletalMesh);
				}
			}
		}
	}
}

void USKGAttachmentComponent::OnRep_AttachmentOffset()
{
	SetRelativeAttachmentOffset(Attachment);
	OnOffsetChanged.Broadcast(AttachmentOffset);
}

void USKGAttachmentComponent::SetRelativeAttachmentOffset(AActor* AttachmentToSet)
{
	if (AttachmentToSet)
	{
		FVector RelativeOffset = FVector(0.0f, AttachmentOffset, 0.0f);
		switch (OffsetSettings.OffsetAxis)
		{
		case EAxis::X : RelativeOffset = FVector(AttachmentOffset, 0.0f, 0.0f); break;
		case EAxis::Z : RelativeOffset = FVector(0.0f, 0.0f, AttachmentOffset); break;
		}
		AttachmentToSet->SetActorRelativeLocation(RelativeOffset);
	}
}

bool USKGAttachmentComponent::Server_SetAttachmentOffset_Validate(const float Offset)
{
	return bAllowClientSideModification;
}

void USKGAttachmentComponent::Server_SetAttachmentOffset_Implementation(const float Offset)
{
	if (Offset != AttachmentOffset)
	{
		AttachmentOffset = Offset;
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGAttachmentComponent, AttachmentOffset, this);
		TryForceNetUpdate();
	}
	OnRep_AttachmentOffset();
}

void USKGAttachmentComponent::FinalizeAttachmentOffset()
{
	if (OldAttachmentOffset != AttachmentOffset)
	{
		OldAttachmentOffset = AttachmentOffset;
		if (HasAuthority())
		{
			Server_SetAttachmentOffset_Implementation(AttachmentOffset);
		}
		else if (bAllowClientSideModification)
		{
			Server_SetAttachmentOffset(AttachmentOffset);
			OnRep_AttachmentOffset();
		}
	}
}

void USKGAttachmentComponent::SetAttachmentOffset(float Offset)
{
	if (OffsetSettings.OffsetSnapDistance > 0.0f)
	{
		Offset = FMath::RoundToInt(Offset / OffsetSettings.OffsetSnapDistance) * OffsetSettings.OffsetSnapDistance;
	}
	Offset = FMath::Clamp(Offset, OffsetSettings.MinimumOffsetAllowed, OffsetSettings.MaximumOffsetAllowed);
	if (Offset != AttachmentOffset)
	{
		AttachmentOffset = Offset;
		if (OffsetSettings.bReplicateOffsetEachChange)
		{
			FinalizeAttachmentOffset();
		}
		SetRelativeAttachmentOffset(Attachment);
		OnOffsetChanged.Broadcast(AttachmentOffset);
	}
}

void USKGAttachmentComponent::IncrementOffset()
{
	SetAttachmentOffset(AttachmentOffset + OffsetSettings.OffsetSnapDistance);
}

void USKGAttachmentComponent::DecrementOffset()
{
	SetAttachmentOffset(AttachmentOffset - OffsetSettings.OffsetSnapDistance);
}
