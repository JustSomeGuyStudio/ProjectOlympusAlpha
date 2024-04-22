// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "Components/ActorComponent.h"
#include "DataTypes/SKGProceduralDataTypes.h"
#include "SKGProceduralAnimComponent.generated.h"

class UCurveVector;
class UMeshComponent;

// Component to be applied for procedurals such as aiming, sway, poses, recoil, etc
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SKGPROCEDURALANIM_API USKGProceduralAnimComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USKGProceduralAnimComponent();

private:
	// The mesh name to be found for the to be used for an optic, laser, firearm, etc. to aim with
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Initialize")
	FName ProceduralMeshName {"StaticMesh"};
	// The sockets on the ProceduralMeshName that can be used for aiming
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Initialize")
	TArray<FName> ProceduralAimSocketNames {"S_Aim"};
	/** The index to use if you decide to point aim. Primarily useful for the
	 * procedural anim component on a firearm. -1 = no point aim socket
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Initialize")
	int32 RightHandDominatePointAimAimSocketIndex {INDEX_NONE};
	/** The index to use if you decide to point aim. Primarily useful for the
	 * procedural anim component on a firearm. -1 = no point aim socket
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Initialize")
	int32 LeftHandDominatePointAimAimSocketIndex {INDEX_NONE};
	// Used to tell what you are holding (such as a rifle, pistol, optic, etc)
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Initialize")
	FGameplayTag ProceduralAnimGameplayTag;
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Initialize")
	FGameplayTagContainer GameplayTags;
	
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Procedural")
	FSKGProceduralAimingSettings AimingSettings;
	// The held offset position of the held actor. Separate for first and third person
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Procedural")
	FSKGFirstAndThirdPersonBasePoseOffset BasePoseOffset;
	// Offset applied when aiming for third person view with left hand holding firearm
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Procedural")
	FSKGProceduralOffset ThirdPersonRightHandedAimingOffset;
	// Offset applied when aiming for third person view with right hand holding firearm
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Procedural")
	FSKGProceduralOffset ThirdPersonLeftHandedAimingOffset;
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Procedural")
	FSKGCycleAimingPointSettings CycleAimingPointSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Procedural")
	FSKGMovementSwaySettings MovementSwaySettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Procedural")
	FSKGMovementLagSettings MovementLagSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Procedural")
	FSKGRotationLagSettings RotationLagSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Procedural")
	FSKGDeadzoneSettings DeadzoneSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Procedural")
	FSKGRecoilSettings RecoilSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGProceduralAnim|Procedural")
	TArray<FSKGToFromCurveSettings> PoseSettings;
	
	UPROPERTY(BlueprintGetter = GetProceduralAnimMesh, Category = "SKGProceduralAnim|Mesh")
	TObjectPtr<UMeshComponent> ProceduralAnimMesh;
	bool bUsedForAiming {false};
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; }
	FORCEINLINE bool HasAuthority() const { return GetOwnerRole() == ROLE_Authority; }
	void SetupComponents();

	int32 PreviousAimSocketIndex {0};
	UPROPERTY(Replicated)
	uint8 AimSocketIndex {0};
	UPROPERTY(BlueprintGetter = GetAimOffset, Category = "SKGProceduralAnim")
	FTransform CurrentAimSocket;
	UPROPERTY()
	TObjectPtr<UMeshComponent> LastRelativeToOffset;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetAimSocketIndex(uint8 Index);
	
public:
	// Should only be used when manually setting the value for construction
	void SetProceduralMeshName(const FName& Name) { ProceduralMeshName = Name; }
	// Should only be used when manually setting the value for construction
	void SetProceduralAimSocketNames(const TArray<FName>& Names) { ProceduralAimSocketNames = Names; }
	// Should only be used when manually setting the value for construction
	void SetRightHandDominatePointAimAimSocketIndex(int32 Index) { RightHandDominatePointAimAimSocketIndex = Index; }
	// Should only be used when manually setting the value for construction
	void SetLeftHandDominatePointAimAimSocketIndex(int32 Index) { LeftHandDominatePointAimAimSocketIndex = Index; }
	// Should only be used when manually setting the value for construction
	void SetProceduralAnimGameplayTag(const FGameplayTag& Tag) { ProceduralAnimGameplayTag = Tag; }
	
	UFUNCTION(BlueprintPure, Category = "SKGProceduralAnimComponent|Aim")
	const FSKGProceduralAimingSettings& GetProceduralAimingSettings() const { return AimingSettings; }
	UFUNCTION(BlueprintPure, Category = "SKGProceduralAnimComponent|Aim")
	bool CanAim() const { return AimingSettings.bCanAim; }
	UFUNCTION(BlueprintCallable, Category = "SKGProceduralAnimComponent|Aim")
	void SetCanAim(bool bCanAim) { AimingSettings.bCanAim = bCanAim; }
	UFUNCTION(BlueprintPure, Category = "SKGProceduralAnimComponent|Aim")
	FTransform GetAimWorldTransform() const;
	UFUNCTION(BlueprintCallable, Category = "SKGProceduralAnimComponent|Aim")
	bool CycleAimSocket();
	// Returns true if point aim possible and set successfully
	UFUNCTION(BlueprintCallable, Category = "SKGProceduralAnimComponent|Aim")
	bool StartPointAiming(bool bRightHandDominant);
	UFUNCTION(BlueprintCallable, Category = "SKGProceduralAnimComponent|Aim")
	void StopPointAiming();
	// If ComponentRelativeTo is null, then self will be used
	UFUNCTION(BlueprintCallable, Category = "SKGProceduralAnim|Aim")
	void UpdateAimOffset(UMeshComponent* ComponentRelativeTo);
	// If ComponentRelativeTo is null, then self will be used
	UFUNCTION(BlueprintCallable, Category = "SKGProceduralAnim|Aim")
	void UpdateAimOffsetWithSocket(UMeshComponent* ComponentRelativeTo, const FName& Socket);

	UFUNCTION(BlueprintPure, Category = "SKGProceduralAnim|Pose")
	bool GetPose(FGameplayTag Tag, FSKGToFromCurveSettings& PoseData);
	
	/**
	 * Overrideable, utilizes the aim socket to calculate where to zero the muzzle at
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SKGProceduralAnimComponent|Muzzle")
	FTransform GetAimMuzzleTransform();
	virtual FTransform GetAimMuzzleTransform_Implementation();
	
	const FGameplayTag GetProceduralGameplayTag() const { return ProceduralAnimGameplayTag; }
	const FSKGFirstAndThirdPersonBasePoseOffset& GetBasePoseOffset() const { return BasePoseOffset; }
	const FSKGProceduralOffset& GetThirdPersonAimingOffset(bool bRightHanded) const;
	const FSKGCycleAimingPointSettings& GetCycleAimingPointSettings() const { return CycleAimingPointSettings; }
	const FSKGMovementSwaySettings& GetMovementSwaySettings() const { return MovementSwaySettings; }
	const FSKGMovementLagSettings& GetMovementLagSettings() const { return MovementLagSettings; }
	const FSKGRotationLagSettings& GetRotationSettings() const { return RotationLagSettings; }
	const FSKGDeadzoneSettings& GetDeadzoneSettings() const { return DeadzoneSettings; }
	const FSKGRecoilSettings& GetRecoilSettings() const { return RecoilSettings; }
	
	UFUNCTION(BlueprintGetter)
	FORCEINLINE FTransform GetAimOffset() const { return CurrentAimSocket; }
	UFUNCTION(BlueprintGetter)
	FORCEINLINE UMeshComponent* GetProceduralAnimMesh() const { return ProceduralAnimMesh; }
	template< typename T >
	FORCEINLINE T* GetProceduralAnimMesh() const { return Cast<T>(ProceduralAnimMesh); }
};
