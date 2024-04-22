// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "NativeGameplayTags.h"
#include "Components/ActorComponent.h"
#include "SKGOffHandIKComponent.generated.h"

class UMeshComponent;
class UAnimSequence;

namespace SKGGAMEPLAYTAGS
{
	SKGPROCEDURALANIM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(OffHandIKComponentFirearm);
	SKGPROCEDURALANIM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(OffHandIKComponentHandguard);
	SKGPROCEDURALANIM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(OffHandIKComponentForwardGrip);
}

/**
 * Used for off hand ik. For example with a right handed shooter holding a rifle, the
 * offhand would be the left hand, for a left handed shooter the offhand would be the right hand
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SKGPROCEDURALANIM_API USKGOffHandIKComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USKGOffHandIKComponent();

protected:
	// The mesh used to gather data for the Left Hand IK (think forward grip/handguard)
	UPROPERTY(EditDefaultsOnly, Category = "SKGOffHandIK|Initialize")
	FName OffHandIKMeshName {"StaticMesh"};
	// Socket used for the Left Hand IK transform (S_OffHandIK in example project)
	UPROPERTY(EditDefaultsOnly, Category = "SKGOffHandIK|Initialize")
	FName LeftHandIKSocketName {NAME_None};
	// Animation to be played on the left hand for the left hand IK
	UPROPERTY(EditDefaultsOnly, BlueprintGetter = GetOffHandIKPose, Category = "SKGOffHandIK|Initialize")
	TObjectPtr<UAnimSequence> LeftHandIKPose;
	// Socket used for the Right Hand IK transform (S_OffHandIK in example project)
	UPROPERTY(EditDefaultsOnly, Category = "SKGOffHandIK|Initialize")
	FName RightHandIKSocketName {NAME_None};
	// Animation to be played on the Right hand for the Right hand IK
	UPROPERTY(EditDefaultsOnly, BlueprintGetter = GetOffHandIKPose, Category = "SKGOffHandIK|Initialize")
	TObjectPtr<UAnimSequence> RightHandIKPose;
	/**
	 * The tag to be used to determine which offhand ik component should be used. If complete firearm (no forward
	 * grip) = OffHandIKComponentType.Firearm, Handguard = OffHandIKComponentType.Handguard, ForwardGrip = OffHandIKComponentType.ForwardGrip
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SKGOffHandIK|Initialize")
	FGameplayTag OffHandIKTag {SKGGAMEPLAYTAGS::OffHandIKComponentFirearm};
	UPROPERTY(EditDefaultsOnly, Category = "SKGOffHandIK|Initialize")
	FGameplayTagContainer GameplayTags;
	
	UPROPERTY(BlueprintGetter = GetOffHandIKMesh, Category = "SKGOffHandIK|Mesh")
	TObjectPtr<UMeshComponent> OffHandIKMesh;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	FORCEINLINE bool HasAuthority() const { return GetOwnerRole() == ROLE_Authority; }
	void SetupComponents();
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; }
	FTransform GetOffHandIKWorldTransform(bool bLeftHand) const;

	UPROPERTY(BlueprintGetter = GetOffHandIKOffset, Category = "SKGOffHandIK")
	FTransform OffHandIKOffset;
	UPROPERTY()
	UPrimitiveComponent* LastRelativeToOffset;

	UPROPERTY(BlueprintGetter = GetOffHandIKPose, Category = "SKGOffHandIK")
	TObjectPtr<UAnimSequence> OffHandIKPose;

	FORCEINLINE const FName& GetHandSocketName(bool bLeftHand) const;
	
public:
	// Should only be used when manually setting the value for construction
	void SetOffHandIKMeshName(const FName& Name) { OffHandIKMeshName = Name; }
	// Should only be used when manually setting the value for construction
	void SetLeftHandIKSocketName(const FName& Name) { LeftHandIKSocketName = Name; }
	// Should only be used when manually setting the value for construction
	void SetRightHandIKSocketName(const FName& Name) { RightHandIKSocketName = Name; }
	
	UFUNCTION(BlueprintCallable, Category = "SKGOffHandIK")
	void UpdateOffHandIK(UPrimitiveComponent* ComponentRelativeTo, bool bLeftHand);
	UFUNCTION(BlueprintGetter)
	FORCEINLINE FTransform GetOffHandIKOffset() const { return OffHandIKOffset; }
	UFUNCTION(BlueprintGetter)
	FORCEINLINE UAnimSequence* GetOffHandIKPose() const { return OffHandIKPose; }
	UFUNCTION(BlueprintGetter)
	FORCEINLINE UMeshComponent* GetOffHandIKMesh() const { return OffHandIKMesh; }
	template< typename T >
	FORCEINLINE T* GetOffHandIKMesh() const { return Cast<T>(OffHandIKMesh); }
};
