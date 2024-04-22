// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagAssetInterface.h"
#include "GameFramework/Actor.h"
#include "DataTypes/SKGAttachmentDataTypes.h"
#include "SKGAttachmentManagerComponent.generated.h"

class USKGAttachmentComponent;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttachmentComponentsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttachmentComponentAdded, USKGAttachmentComponent*, AttachmentComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttachmentComponentRemoved, USKGAttachmentComponent*, AttachmentComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttachmentComponentAttachmentAdded, AActor*, Attachment);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttachmentComponentAttachmentRemoved, AActor*, Attachment);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SKGATTACHMENT_API USKGAttachmentManagerComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USKGAttachmentManagerComponent();
	static TArray<USKGAttachmentComponent*> GetAllActorAttachmentComponentsWithAttachments(const AActor* Actor);
	static TArray<USKGAttachmentComponent*> GetActorAttachmentComponentsWithAttachments(const AActor* Actor);

protected:
	// Can be used for whatever. In the example it is used for the save directory of saving/loading presets
	UPROPERTY(EditDefaultsOnly, BlueprintGetter = GetManagerName, Category = "SKGAttachmentManager|Initialize")
	FName ManagerName {""};
	// If true, clients can trigger a replicated hide/unhide and collision/nocollision to all attachments
	UPROPERTY(EditDefaultsOnly, Category = "SKGAttachmentManager|Initialize")
	bool bAllowClientSideModification {true};
	// Useful if your NetUpdateFrequency is set super low
	UPROPERTY(EditDefaultsOnly, Category = "SKGAttachmentManager|Initialize")
	bool bAutoCallForceNetUpdate {true};
	UPROPERTY(EditDefaultsOnly, Category = "SKGAttachmentManager|Initialize")
	FGameplayTagContainer GameplayTags;
	
	UPROPERTY(ReplicatedUsing = OnRep_AttachmentComponents)
	FSKGAttachmentComponentItems AttachmentComponents;
	UFUNCTION()
	void OnRep_AttachmentComponents();

	// Array of all attachments added to the parent
	UPROPERTY(ReplicatedUsing = OnRep_Attachments)
	TArray<AActor*> Attachments;
	UFUNCTION()
	void OnRep_Attachments(TArray<AActor*> OldAttachments);

	UPROPERTY(ReplicatedUsing = OnRep_AttachmentsStates)
	bool bIsHidden {false};
	UPROPERTY(ReplicatedUsing = OnRep_AttachmentsStates)
	bool bHasCollision {true};
	UFUNCTION()
	void OnRep_AttachmentsStates();
	void SetAttachmentsToState();

	bool bCanSpawnDefaultAttachments {true};
	
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; }
	FORCEINLINE bool HasAuthority() const { return GetOwnerRole() == ROLE_Authority; }
	FORCEINLINE void TryForceNetUpdate() const;
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_HideAllAttachments(bool Hide);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetAllAttachmentsCollision(bool EnableCollision);

public:
	void RegisterAttachmentComponent(USKGAttachmentComponent* AttachmentComponent);
	void UnregisterAttachmentComponent(USKGAttachmentComponent* AttachmentComponent);
	void RegisterAttachmentComponentsAttachment(USKGAttachmentComponent* AttachmentComponent, AActor* Attachment);
	void UnregisterAttachmentComponentsAttachment(USKGAttachmentComponent* AttachmentComponent, AActor* Attachment);
	void SetCanSpawnDefaultAttachments(bool bCanSpawn) { bCanSpawnDefaultAttachments = bCanSpawn; }
	bool CanSpawnDefaultAttachments() const { return bCanSpawnDefaultAttachments; }

	UFUNCTION(BlueprintPure, Category = "SKGAttachmentManagerComponent|Attachments")
	const TArray<FSKGAttachmentComponentItem>& GetAttachmentComponents() const { return AttachmentComponents.Items; }
	UFUNCTION(BlueprintPure, Category = "SKGAttachmentManagerComponent|Attachments")
	const TArray<AActor*>& GetAttachments() const { return Attachments; }
	TArray<AActor*>& GetAttachments() { return Attachments; }
	// Will return an array of attachment components that are compatible with the Actor
	UFUNCTION(BlueprintPure, Category = "SKGAttachmentManagerComponent|Attachments")
	bool GetCompatibleAttachmentComponentsFromActor(TArray<USKGAttachmentComponent*>& Components, const AActor* Actor) const;
	// Will return an array of attachment components that are compatible with the class
	UFUNCTION(BlueprintPure, Category = "SKGAttachmentManagerComponent|Attachments")
	bool GetCompatibleAttachmentComponentsFromClass(TArray<USKGAttachmentComponent*>& Components, const UClass* Class) const;
	// Will return the attachment component that holds the attachment
	UFUNCTION(BlueprintPure, Category = "SKGAttachmentManagerComponent|Attachments")
	USKGAttachmentComponent* GetAttachmentComponentWithAttachment(AActor* Attachment);
	
	UFUNCTION(BlueprintCallable, Category = "SKGAttachmentManagerComponent|AttachmentState")
	void HideAllAttachments();
	UFUNCTION(BlueprintCallable, Category = "SKGAttachmentManagerComponent|AttachmentState")
	void UnhideAllAttachments();
	UFUNCTION(BlueprintCallable, Category = "SKGAttachmentManagerComponent|AttachmentState")
	void DisableCollisionAllAttachments();
	UFUNCTION(BlueprintCallable, Category = "SKGAttachmentManagerComponent|AttachmentState")
	void EnableCollisionAllAttachments();

	UFUNCTION(BlueprintGetter)
	FName GetManagerName() const { return ManagerName; }

	// Called when the AttachmentComponents array changes via an add or removed both from client and server
	UPROPERTY(BlueprintAssignable, Category = "SKGAttachmentManagerComponent|Events")
	FOnAttachmentComponentsChanged OnAttachmentComponentsChanged;
	// Called on the server only when an attachment component is added
	UPROPERTY(BlueprintAssignable, Category = "SKGAttachmentManagerComponent|Events")
	FOnAttachmentComponentAdded OnAttachmentComponentAdded;
	// Called on the server only when an attachment component is added
	UPROPERTY(BlueprintAssignable, Category = "SKGAttachmentManagerComponent|Events")
	FOnAttachmentComponentRemoved OnAttachmentComponentRemoved;
	// Called on the server only when an attachment component is added
	UPROPERTY(BlueprintAssignable, Category = "SKGAttachmentManagerComponent|Events")
	FOnAttachmentComponentAttachmentAdded OnAttachmentComponentAttachmentAdded;
	// Called on the server only when an attachment component is added
	UPROPERTY(BlueprintAssignable, Category = "SKGAttachmentManagerComponent|Events")
	FOnAttachmentComponentAttachmentRemoved OnAttachmentComponentAttachmentRemoved;
};
