// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagAssetInterface.h"
#include "Animation/SKGShooterFrameworkAnimInstance.h"
#include "SKGShooterPawnComponent.generated.h"

class USKGLightLaserComponent;
class USKGOpticComponent;
class UCurveVector;
class USKGShooterFrameworkAnimInstance;
class ISKGInfraredInterface;
class USKGFirearmComponent;
class USKGProceduralAnimComponent;
class UCameraComponent;
class USkeletalMeshComponent;
class UMaterialParameterCollection;
class APawn;

USTRUCT(BlueprintType)
struct FSKGFreeLookSettings
{
	GENERATED_BODY()
	// If true, it will auto set bUseControllerRotationYaw to true and false
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGFreeLookSettings")
	bool bAutoSetUseControllerRotationYaw {true};
	// The max you can look up
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGFreeLookSettings")
	float MaxPitchUp {40};
	// The max you can look down
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGFreeLookSettings")
	float MaxPitchDown {30};
	// The max you can look left
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGFreeLookSettings")
	float MaxYawLeft {35};
	// The max you can look right
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGFreeLookSettings")
	float MaxYawRight {35};
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPoseComplete, const FSKGProceduralPoseReplicationData&, CurrentPoseData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHeldActorSet, AActor*, NewHeldActor, AActor*, OldHeldActor);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SKGSHOOTERFRAMEWORK_API USKGShooterPawnComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()
	friend USKGShooterFrameworkAnimInstance;
	
public:	
	// Sets default values for this component's properties
	USKGShooterPawnComponent();

protected:
	/** The linked anim layer class to be used instead of the actual anim instance of the mesh
	 * so you do not need to reparent your animbp. Refer to example project for usage
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterPawnComponent|Initialize")
	TSubclassOf<USKGShooterFrameworkAnimInstance> LinkedAnimLayerClass;
	// If true, it will auto link the LinkedAnimLayerClass to your mesh anim instance for the procedurals
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterPawnComponent|Initialize")
	bool bAutoSetupLinkedAnimLayer {true};
	// If true, it will use the first found match as the first and third person mesh
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterPawnComponent|Initialize")
	bool bUseSingleMesh {true};
	// The name of the Pawns Third Person Mesh Component
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterPawnComponent|Initialize")
	FName ThirdPersonMeshComponentName {"CharacterMesh0"};
	// The name of the Pawns First Person Mesh Component
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterPawnComponent|Initialize")
	FName FirstPersonMeshComponentName {"MeshFP"};
	// The name of the Pawns Camera Component
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterPawnComponent|Initialize")
	FName CameraComponentName {"CameraComponent"};
	// The socket the camera is attached to on the pawn's mesh
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterPawnComponent|Initialize")
	FName CameraAttachedSocket {"S_Camera"};
	// Used for free look, turn in place, etc
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterPawnComponent|Initialize")
	bool bReplicateRemoteYaw {true};
	// Replication rate for the Yaw. 0 = every frame
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterPawnComponent|Initialize", meta = (EditCondition = "bReplicateRemoteYaw"))
	double RemoteYawReplicationRate {0.1};
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterPawnComponent|Initialize")
	FGameplayTagContainer GameplayTags;
	
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterPawnComponent|FreeLook")
	FSKGFreeLookSettings FreeLookSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterPawnComponent|Leaning")
	FSKGLeanLeftRightSettings LeanLeftRightSettings;

	UPROPERTY(BlueprintGetter = GetCameraComponent, Category = "SKGShooterPawnComponent")
	TObjectPtr<UCameraComponent> CameraComponent;
	UPROPERTY(BlueprintGetter = GetPawnMesh, Category = "SKGShooterPawnComponent")
	TObjectPtr<USkeletalMeshComponent> MeshFP;
	UPROPERTY(BlueprintGetter = GetPawnMesh, Category = "SKGShooterPawnComponent")
	TObjectPtr<USkeletalMeshComponent> MeshTP;
	UPROPERTY(BlueprintGetter = GetShooterFrameworkAnimInstance, Category = "SKGShooterPawnComponent")
	TObjectPtr<USKGShooterFrameworkAnimInstance> ShooterFrameworkAnimInstance;
	UPROPERTY(BlueprintGetter = GetOwningPawn, Category = "SKGShooterPawnComponent")
	TObjectPtr<APawn> OwningPawn;

	UPROPERTY(BlueprintGetter = GetProceduralShooterPawnData, Category = "SKGShooterPawnComponent")
	FSKGProceduralShooterPawnData ProceduralShooterPawnData;

	UPROPERTY(ReplicatedUsing = OnRep_TargetLeanAngleCompressed)
	uint8 TargetLeanAngleCompressed {0};
	float TargetLeanAngle {0.0f};
	float TargetLeanLeftAngle {0.0f};
	float TargetLeanRightAngle {0.0f};
	bool bLeaningLeft {false};
	bool bLeaningRight {false};
	UFUNCTION()
	void OnRep_TargetLeanAngleCompressed();
	
	UPROPERTY(ReplicatedUsing = OnRep_HeldActor, BlueprintGetter = GetHeldActor, Category = "SKGShooterPawnComponent")
	TObjectPtr<AActor> HeldActor;
	UPROPERTY(ReplicatedUsing = OnRep_IsAiming, BlueprintGetter = IsAiming, Category = "SKGShooterPawnComponent")
	bool bIsAiming {false};
	UPROPERTY(ReplicatedUsing = OnRep_CurrentProceduralPoseData, BlueprintGetter = GetCurrentProceduralPoseReplicationData, Category = "SKGShooterPawnComponent")
	FSKGProceduralPoseReplicationData CurrentProceduralPoseData;
	UPROPERTY(Replicated, BlueprintGetter = GetOffHandIKIsLeftHand, Category = "SKGShooterPawnComponent")
	bool bOffHandIKIsLeftHand {true};

	UPROPERTY(ReplicatedUsing = OnRep_InFreeLook)
	bool bInFreeLook {false};
	UFUNCTION()
	void OnRep_InFreeLook();
	FRotator FreeLookStartRotation {FRotator::ZeroRotator};
	bool bCanAddYaw {true};
	bool bCanAddPitch {true};
	
	UPROPERTY(Replicated)
	uint8 RemoteViewYaw {0};

	UPROPERTY()
	TObjectPtr<USKGFirearmComponent> CurrentFirearmComponent;
	UPROPERTY()
	TObjectPtr<USKGProceduralAnimComponent> CurrentProceduralAnimComponent;

	FTransform CameraOffset;
	bool bUseFirstPersonProceduralsAsLocal {true};
	bool bUsingCustomSwayMultiplier {false};
	float SwayMultiplier {1.0f};
	ECollisionChannel FirearmCollisionChannel {ECC_Visibility};
	
	UFUNCTION()
	virtual void OnRep_HeldActor(AActor* OldActor);
	UFUNCTION()
	void OnRep_IsAiming() {}
	UFUNCTION()
	void OnRep_CurrentProceduralPoseData() const;

	FVector2D MouseInput {FVector2D::ZeroVector};
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void PostInitProperties() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; }
	FORCEINLINE bool HasAuthority() const { return GetOwnerRole() == ROLE_Authority; }
	void SetupComponents();
	void SetCameraOffset();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetAiming(bool bAim);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Lean(const uint8 TargetAngle);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_PerformProceduralPose(const FGameplayTag& Tag, bool bExitPose);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetOffHandIKHand(bool bLeftHand);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetFreeLook(bool bFreeLook);
	UFUNCTION(Server, Unreliable, WithValidation)
	void Server_PerformCustomCurveUnreliable(const FSKGFirstAndThirdPersonCurveSettings& CurveData);
	UFUNCTION(Server, Unreliable, WithValidation)
	void Server_SetRemoteYaw(uint8 Yaw);
	
	UFUNCTION(NetMulticast, Unreliable, WithValidation)
	void Multi_PlayCustomCurveUnreliable(const FSKGFirstAndThirdPersonCurveSettings& CurveData);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_PerformCustomCurve(const FSKGFirstAndThirdPersonCurveSettings& CurveData);
	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void Multi_PlayCustomCurve(const FSKGFirstAndThirdPersonCurveSettings& CurveData);

	void ReplicateYaw(bool bForce);
	
	// Called from only the anim instance so a delegate can be fired
	void PoseComplete() const;

public:
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|HeldActor")
	void SetHeldActor(AActor* Actor);
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|HeldActor")
	void ClearHeldActor();
	
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|AnimationLayer")
	USKGShooterFrameworkAnimInstance* LinkAnimLayerClass();
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|AnimationLayer")
	USKGShooterFrameworkAnimInstance* LinkAnimLayerClassByInstance(UAnimInstance* AnimInstance);
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|AnimationLayer")
	void UnlinkAnimLayerClass();
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|AnimationLayer")
	void UnlinkAnimLayerClassByInstance(UAnimInstance* AnimInstance);
	
	// Used for freelook. If false, it means control rotation goes to far
	UFUNCTION(BlueprintPure, Category = "SKGShooterPawnComponent|FreeLook")
	bool CanAddYaw() const;
	// Used for freelook. If false, it means control rotation goes to far
	UFUNCTION(BlueprintPure, Category = "SKGShooterPawnComponent|FreeLook")
	bool CanAddPitch() const;
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|FreeLook")
	void StartFreeLook();
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|FreeLook")
	void StopFreeLook();
	UFUNCTION(BlueprintPure, Category = "SKGShooterPawnComponent|FreeLook")
	bool IsFreeLooking() const { return bInFreeLook; }

	// Used for deadzone
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|MouseInput")
	void SetMouseInput(float X, float Y);

	FORCEINLINE FTransform GetCameraOffset() const { return CameraOffset; }
	FSKGProceduralAnimInstanceData GetProceduralData();

	UFUNCTION(BlueprintPure, Category = "SKGShooterPawnComponent|Sensitivity")
	float GetMagnificationSensitivityMultiplier() const;
	// Outputs new sensivity based on whether or not free looking, zoomed with magnification, utilizes base turn rate
	UFUNCTION(BlueprintPure, Category = "SKGShooterPawnComponent|Sensitivity")
	void GetSensitivityMultiplier(const float X, const float XBaseTurnRate, const float Y, const float YBaseTurnRate, float& NewX, bool& AddYaw, float& NewY, bool& AddPitch) const;
	
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|Leaning")
	void LeanLeft(float TargetAngle);
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|Leaning")
	void LeanRight(float TargetAngle);
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|Leaning")
	void StopLeaningLeft();
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|Leaning")
	void StopLeaningRight();
	UFUNCTION(BlueprintPure, Category = "SKGShooterPawnComponent|Leaning")
	FORCEINLINE float GetTargetLeanAngle() const { return TargetLeanAngle; }
	
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|ProceduralAnimation")
	void StartAiming();
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|ProceduralAnimation")
	void StopAiming();
	UFUNCTION(BlueprintGetter, Category = "SKGShooterPawnComponent|ProceduralAnimation")
	FORCEINLINE bool IsAiming() const { return bIsAiming; }
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|ProceduralAnimation")
	void SetUseFirstPersonProceduralsAsLocal();
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|ProceduralAnimation")
	void SetUseThirdPersonProceduralsAsLocal();
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|ProceduralAnimation")
	void SetSwayMultiplier(const float Multiplier);
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|ProceduralAnimation")
	void ResetSwayMultiplier();
	UFUNCTION(BlueprintPure, Category = "SKGShooterPawnComponent|ProceduralAnimation")
	bool GetUsingCustomSwayMultiplier() const { return bUsingCustomSwayMultiplier; }
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|ProceduralAnimation")
	void PerformProceduralRecoil(const FRotator& ControlRotationMultiplier = FRotator(1.0, 1.0, 1.0), const FVector& LocationMultiplier = FVector(1.0), const FRotator& RotationMultiplier = FRotator(1.0, 1.0, 1.0));
	/**
		 * @param Tag used to search for the given pose
		 * @param bExitPose whether or not to use the return curve (return to normal) or the to curve (go from normal to pose)
		 */
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|ProceduralAnimation")
	void PerformProceduralPose(const FGameplayTag& Tag, bool bExitPose);
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|ProceduralAnimation")
	void PerformCustomCurve(const FSKGCurveSettings& CurveData);
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|ProceduralAnimation")
	void SetOffHandIKToLeftHand();
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|ProceduralAnimation")
	void SetOffHandIKToRightHand();
	UFUNCTION(BlueprintGetter, Category = "SKGShooterPawnComponent|ProceduralAnimation")
	const FSKGProceduralPoseReplicationData& GetCurrentProceduralPoseReplicationData() const { return CurrentProceduralPoseData; }
	UFUNCTION(BlueprintGetter, Category = "SKGShooterPawnComponent|ProceduralAnimation")
	bool GetOffHandIKIsLeftHand() const { return bOffHandIKIsLeftHand; }
	UFUNCTION(BlueprintPure, Category = "SKGShooterPawnComponent|ProceduralAnimation")
	FSKGToFromCurveSettings GetProceduralPoseData(const FGameplayTag& Tag) const;
	
	UFUNCTION(BlueprintPure, Category = "SKGShooterPawnComponent|Components")
	USKGFirearmComponent* GetCurrentFirearmComponent() const;
	UFUNCTION(BlueprintPure, Category = "SKGShooterPawnComponent|Components")
	USKGProceduralAnimComponent* GetCurrentProceduralAnimComponent() const;
	UFUNCTION(BlueprintPure, Category = "SKGShooterPawnComponent|Components")
	USKGOpticComponent* GetCurrentOpticComponent() const;
	UFUNCTION(BlueprintPure, Category = "SKGShooterPawnComponent|Components")
	TArray<USKGLightLaserComponent*> GetCurrentLightLaserComponents() const;
	
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|Misc", Meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool IsLocallyControlled() const;
	UFUNCTION(BlueprintCallable, Category = "SKGShooterPawnComponent|Misc")
	bool IsUsingFirstPersonProceduralsAsLocal() const { return bUseFirstPersonProceduralsAsLocal; }
	/* Returns control rotation if local controlled, returns replicated
	 * rotator (remote yaw and remote pitch) if remote client
	 */
	UFUNCTION(BlueprintPure, Category = "SKGShooterPawnComponent|Misc")
	FRotator GetControlRotation() const;

	UFUNCTION(BlueprintGetter)
	const FSKGProceduralShooterPawnData& GetProceduralShooterPawnData() const { return ProceduralShooterPawnData; }
	UFUNCTION(BlueprintGetter)
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }
	// Returns First Person Mesh if local, Third Person Mesh if remote
	UFUNCTION(BlueprintGetter)
	USkeletalMeshComponent* GetPawnMesh() const;
	UFUNCTION(BlueprintGetter)
	APawn* GetOwningPawn() const { return OwningPawn; }
	template< typename T >
	T* GetOwningPawn() const { return Cast<T>(GetOwningPawn()); }
	UFUNCTION(BlueprintGetter)
	USKGShooterFrameworkAnimInstance* GetShooterFrameworkAnimInstance() const { return ShooterFrameworkAnimInstance; }
	template< typename T >
	T* GetShooterFrameworkAnimInstance() const { return Cast<T>(GetShooterFrameworkAnimInstance()); }
	
	UFUNCTION(BlueprintGetter)
	AActor* GetHeldActor() const { return HeldActor; }
	template< typename T >
	T* GetHeldActor() const { return Cast<T>(GetHeldActor()); }

	UPROPERTY(BlueprintAssignable, Category = "SKGShooterPawnComponent|Events")
	FOnPoseComplete OnPoseComplete;
	UPROPERTY(BlueprintAssignable, Category = "SKGShooterPawnComponent|Events")
	FOnHeldActorSet OnHeldActorSet;

#if WITH_EDITORONLY_DATA
	// If true, firearm collision will be printed
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterFramework|Debug")
	bool bPrintHit {false};
	UPROPERTY(EditDefaultsOnly, Category = "SKGShooterFramework|Debug")
	bool bDrawDebugTrace {false};
#endif
};
