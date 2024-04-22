// Copyright 2023, Dakota Dawe, All rights reserved


#include "Components/SKGAttachmentManagerComponent.h"
#include "Components/SKGAttachmentComponent.h"

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

constexpr uint8 BitHidden = 0;
constexpr uint8 BitCollision = 1;

USKGAttachmentManagerComponent::USKGAttachmentManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

TArray<USKGAttachmentComponent*> USKGAttachmentManagerComponent::GetAllActorAttachmentComponentsWithAttachments(const AActor* Actor)
{
	TArray<USKGAttachmentComponent*> FoundComponents;
	if (Actor)
	{
		TArray<AActor*> AttachedActors;
		Actor->GetAttachedActors(AttachedActors);
		// Reserve assumption of 5 components per attached actors and self
		FoundComponents.Reserve((AttachedActors.Num() + 1) * 5);
		for (const AActor* AttachedActor : AttachedActors)
		{
			TArray<USKGAttachmentComponent*> Components = GetActorAttachmentComponentsWithAttachments(AttachedActor);
			if (Components.Num())
			{
				FoundComponents.Append(Components);
			}
		}
		FoundComponents.Shrink();
	}
	return FoundComponents;
}

TArray<USKGAttachmentComponent*> USKGAttachmentManagerComponent::GetActorAttachmentComponentsWithAttachments(const AActor* Actor)
{
	TArray<USKGAttachmentComponent*> FoundComponents;
	if (Actor)
	{
		TInlineComponentArray<USKGAttachmentComponent*> AttachmentComponents(Actor);
		FoundComponents.Reserve(AttachmentComponents.Num());
		for (USKGAttachmentComponent* Component : AttachmentComponents)
		{
			if (Component->GetAttachment())
			{
				FoundComponents.Add(Component);
			}
		}
		FoundComponents.Shrink();
	}
	return FoundComponents;
}

// Called when the game starts
void USKGAttachmentManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USKGAttachmentManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.RepNotifyCondition = REPNOTIFY_Always;
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGAttachmentManagerComponent, AttachmentComponents, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGAttachmentManagerComponent, Attachments, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGAttachmentManagerComponent, bIsHidden, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGAttachmentManagerComponent, bHasCollision, Params);
}

void USKGAttachmentManagerComponent::TryForceNetUpdate() const
{
	if (bAutoCallForceNetUpdate)
	{
		GetOwner()->ForceNetUpdate();
	}
}

void USKGAttachmentManagerComponent::OnRep_Attachments(TArray<AActor*> OldAttachments)
{
	SetAttachmentsToState();
}

void USKGAttachmentManagerComponent::OnRep_AttachmentsStates()
{
	SetAttachmentsToState();
}

void USKGAttachmentManagerComponent::SetAttachmentsToState()
{
	for (AActor* Attachment : Attachments)
	{
		if (Attachment)
		{
			if (Attachment->IsHidden() != bIsHidden)
			{
				Attachment->SetActorHiddenInGame(bIsHidden);
			}
			if (Attachment->GetActorEnableCollision() != bHasCollision)
			{
				Attachment->SetActorEnableCollision(bHasCollision);
			}
		}
	}
}

void USKGAttachmentManagerComponent::OnRep_AttachmentComponents()
{
	OnAttachmentComponentsChanged.Broadcast();
}

void USKGAttachmentManagerComponent::RegisterAttachmentComponent(USKGAttachmentComponent* AttachmentComponent)
{
	if (HasAuthority())
	{
		AttachmentComponents.Add(AttachmentComponent);
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGAttachmentManagerComponent, AttachmentComponents, this);
		TryForceNetUpdate();
		OnRep_AttachmentComponents();
	}
}

void USKGAttachmentManagerComponent::UnregisterAttachmentComponent(USKGAttachmentComponent* AttachmentComponent)
{
	if (HasAuthority())
	{
		AttachmentComponents.Remove(AttachmentComponent);
		OnAttachmentComponentRemoved.Broadcast(AttachmentComponent);
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGAttachmentManagerComponent, AttachmentComponents, this);
		TryForceNetUpdate();
		OnRep_AttachmentComponents();
	}
}

void USKGAttachmentManagerComponent::RegisterAttachmentComponentsAttachment(USKGAttachmentComponent* AttachmentComponent, AActor* Attachment)
{
	if (HasAuthority() && Attachment && AttachmentComponents.Items.Contains(AttachmentComponent))
	{
		const TArray<AActor*> OldAttachmentsArray = Attachments;
		Attachments.Add(Attachment);
		OnRep_Attachments(OldAttachmentsArray);
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGAttachmentManagerComponent, Attachments, this);
		TryForceNetUpdate();
		OnAttachmentComponentAttachmentAdded.Broadcast(Attachment);
	}
}

void USKGAttachmentManagerComponent::UnregisterAttachmentComponentsAttachment(USKGAttachmentComponent* AttachmentComponent, AActor* Attachment)
{
	if (HasAuthority() && AttachmentComponents.Items.Contains(AttachmentComponent))
	{
		const TArray<AActor*> OldAttachmentsArray = Attachments;
		if (Attachments.Remove(Attachment) > 0)
		{
			OnRep_Attachments(OldAttachmentsArray);
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGAttachmentManagerComponent, Attachments, this);
			TryForceNetUpdate();
			OnAttachmentComponentAttachmentRemoved.Broadcast(Attachment);
		}
	}
}

bool USKGAttachmentManagerComponent::Server_HideAllAttachments_Validate(bool Hide)
{
	return bAllowClientSideModification;
}

void USKGAttachmentManagerComponent::Server_HideAllAttachments_Implementation(bool Hide)
{
	if (bIsHidden != Hide)
	{
		bIsHidden = Hide;
		SetAttachmentsToState();
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGAttachmentManagerComponent, bIsHidden, this);
		TryForceNetUpdate();
	}
}

bool USKGAttachmentManagerComponent::GetCompatibleAttachmentComponentsFromActor(TArray<USKGAttachmentComponent*>& Components, const AActor* Actor) const
{
	return Actor ? GetCompatibleAttachmentComponentsFromClass(Components, Actor->GetClass()) : false;
}

bool USKGAttachmentManagerComponent::GetCompatibleAttachmentComponentsFromClass(TArray<USKGAttachmentComponent*>& Components, const UClass* Class) const
{
	bool bFoundCompatibleComponent = false;
	if (Class)
	{
		const TArray<FSKGAttachmentComponentItem>& AttachmentComponentItems = GetAttachmentComponents();
		Components.Reserve(AttachmentComponentItems.Num());
		for (const FSKGAttachmentComponentItem& AttachmentComponentItem : AttachmentComponentItems)
		{
			USKGAttachmentComponent* Component = AttachmentComponentItem.AttachmentComponent;
			if (Component && Component->IsAttachmentClassCompatible(Class))
			{
				Components.Add(Component);
				bFoundCompatibleComponent = true;
			}
		}
		Components.Shrink();
	}
	return bFoundCompatibleComponent;
}

USKGAttachmentComponent* USKGAttachmentManagerComponent::GetAttachmentComponentWithAttachment(AActor* Attachment)
{
	if (Attachment)
	{
		for (const FSKGAttachmentComponentItem& AttachmentComponent : AttachmentComponents.Items)
		{
			if (USKGAttachmentComponent* Component = AttachmentComponent.AttachmentComponent)
			{
				if (Component->GetAttachment() == Attachment)
				{
					return Component;
				}
			}
		}
	}
	return nullptr;
}

void USKGAttachmentManagerComponent::HideAllAttachments()
{
	if (!bIsHidden)
	{
		if (HasAuthority())
		{
			Server_HideAllAttachments_Implementation(true);
		}
		else
		{
			Server_HideAllAttachments(true);
			bIsHidden = true;
			SetAttachmentsToState();
		}
	}
}

void USKGAttachmentManagerComponent::UnhideAllAttachments()
{
	if (bIsHidden)
	{
		if (HasAuthority())
		{
			Server_HideAllAttachments_Implementation(false);
		}
		else
		{
			Server_HideAllAttachments(false);
			bIsHidden = false;
			SetAttachmentsToState();
		}
	}
}

bool USKGAttachmentManagerComponent::Server_SetAllAttachmentsCollision_Validate(bool EnableCollision)
{
	return bAllowClientSideModification;
}

void USKGAttachmentManagerComponent::Server_SetAllAttachmentsCollision_Implementation(bool EnableCollision)
{
	if (bHasCollision != EnableCollision)
	{
		bHasCollision = EnableCollision;
		SetAttachmentsToState();
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGAttachmentManagerComponent, bHasCollision, this);
		TryForceNetUpdate();
	}
}

void USKGAttachmentManagerComponent::DisableCollisionAllAttachments()
{
	if (bHasCollision)
	{
		if (HasAuthority())
		{
			Server_SetAllAttachmentsCollision_Implementation(false);
		}
		else
		{
			Server_SetAllAttachmentsCollision(false);
			bHasCollision = false;
			SetAttachmentsToState();
		}
	}
}

void USKGAttachmentManagerComponent::EnableCollisionAllAttachments()
{
	if (!bHasCollision)
	{
		if (HasAuthority())
		{
			Server_SetAllAttachmentsCollision_Implementation(true);
		}
		else
		{
			Server_SetAllAttachmentsCollision(true);
			bHasCollision = true;
			SetAttachmentsToState();
		}
	}
}