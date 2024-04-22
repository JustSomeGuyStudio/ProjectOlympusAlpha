// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagAssetInterface.h"
#include "Components/SKGMuzzleComponent.h"
#include "DataTypes/SKGShooterFrameworkDataTypes.h"
#include "SKGFirearmComponent.generated.h"

DECLARE_STATS_GROUP(TEXT("SKGShooterFrameworkFirearmComponentStatGroup"), STATGROUP_SKGShooterFrameworkFirearmComponent,
                    STATCAT_Advanced);

class USKGFirearmAttachmentStatComponent;
class USKGMuzzleComponent;
class USKGOffHandIKComponent;
class USKGLightLaserComponent;
class USKGProceduralAnimComponent;
class USKGAttachmentComponent;
class USKGAttachmentManagerComponent;
class USKGOpticComponent;
class USKGPDAFirearmStats;
class USKGStockComponent;
class UCurveVector;
class UMeshComponent;

USTRUCT(BlueprintType)
struct FSKGFirearmCollisionSettings
{
	GENERATED_BODY()
	// Whether or not to use firearm collision
	UPROPERTY(EditDefaultsOnly, Category = "SKGFirearmComponent")
	bool bUseFirearmCollision {true};
	// The diameter to be used for the firearm collision trace (runs down the barrel)
	UPROPERTY(EditDefaultsOnly, Category = "SKGFirearmComponent", meta = (EditCondition = "bUseFirearmCollision"))
	float TraceDiameter {2.0f};
	// How far you can go into firearm collision before it stops/prevents aiming
	UPROPERTY(EditDefaultsOnly, Category = "SKGFirearmComponent", meta = (EditCondition = "bUseFirearmCollision"))
	float StopAimingDistance {0.2f};
	// The curve in which the firearm moves to as it moves through firearm collision
	UPROPERTY(EditDefaultsOnly, Category = "SKGFirearmComponent", meta = (EditCondition = "bUseFirearmCollision"))
	TObjectPtr<UCurveVector> PoseLocationCurve;
	// The curve in which the firearm rotates to as it moves through firearm collision
	UPROPERTY(EditDefaultsOnly, Category = "SKGFirearmComponent", meta = (EditCondition = "bUseFirearmCollision"))
	TObjectPtr<UCurveVector> PoseRotationCurve;
	// Higher value = further iteration through the curve for each unit of distance pushed from collision
	UPROPERTY(EditDefaultsOnly, Category = "SKGFirearmComponent", meta = (EditCondition = "bUseFirearmCollision"))
	float PoseScale {2.0f};
	// The speed in which the firearm collision interpolates to a new value in the curve
	UPROPERTY(EditDefaultsOnly, Category = "SKGFirearmComponent", meta = (EditCondition = "bUseFirearmCollision"))
	float PoseLocationInterpSpeed {10.0f};
	// The speed in which the firearm collision interpolates to a new value in the curve
	UPROPERTY(EditDefaultsOnly, Category = "SKGFirearmComponent", meta = (EditCondition = "bUseFirearmCollision"))
	float PoseRotationInterpSpeed {10.0f};
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnProceduralAnimComponentsUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMuzzleComponentUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHeld);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAimingDeviceCycled, USKGProceduralAnimComponent*, ProceduralAnimComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFirearmStatsChanged, FSKGFirearmStats, FirearmStats);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCalculateProceduralStats, const FSKGProceduralStats&, ProceduralStats);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SKGSHOOTERFRAMEWORK_API USKGFirearmComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USKGFirearmComponent();

protected:
	// The name of the mesh to be used/considered the firearm
	UPROPERTY(EditDefaultsOnly, Category = "SKGFirearmComponent|Initialize")
	FName FirearmMeshComponentName {NAME_None};
	// The name of the MANDATORY attachment manager added to this parent actor class
	UPROPERTY(EditDefaultsOnly, Category = "SKGFirearmComponent|Initialize")
	FName AttachmentManagerComponentName {NAME_None};
	UPROPERTY(EditDefaultsOnly, Category = "SKGFirearmComponent|Initialize")
	TObjectPtr<USKGPDAFirearmStats> FirearmStatsDataAsset;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFirearmComponent|Initialize")
	FGameplayTagContainer GameplayTags;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFirearmComponent|FirearmCollision")
	FSKGFirearmCollisionSettings FirearmCollisionSettings;

protected:
	UPROPERTY()
	TObjectPtr<UMeshComponent> FirearmMesh;
	UPROPERTY(BlueprintGetter = GetAttachmentManagerComponent, Category = "SKGFirearmComponent|Components")
	TObjectPtr<USKGAttachmentManagerComponent> AttachmentManager;

	UPROPERTY(ReplicatedUsing = OnRep_FirearmStats, BlueprintGetter = GetFirearmStats, Category = "SKGFirearmComponent|FirearmStats")
	FSKGFirearmStats FirearmStats;
	UFUNCTION()
	void OnRep_FirearmStats();
	
	// Consider removing and making Index 0 reserved for firearm specific components
	UPROPERTY()
	TObjectPtr<USKGProceduralAnimComponent> FirearmProceduralAnimComponent;
	UPROPERTY()
	TObjectPtr<USKGOffHandIKComponent> FirearmOffHandIKComponent;
	UPROPERTY()
	TObjectPtr<USKGMuzzleComponent> FirearmMuzzleComponent;
	
	int32 ProceduralAnimComponentIndex {0};
	UPROPERTY(BlueprintGetter = GetProceduralAnimComponents, ReplicatedUsing = OnRep_ProceduralAnimComponents, Category = "SKGFirearmComponent|Components")
	TArray<USKGProceduralAnimComponent*> ProceduralAnimComponents;
	UFUNCTION()
	virtual void OnRep_ProceduralAnimComponents();
	
	UPROPERTY(BlueprintGetter = GetOffHandIKComponents, ReplicatedUsing = OnRep_OffHandIKComponents, Category = "SKGFirearmComponent|Components")
	TArray<USKGOffHandIKComponent*> OffHandIKComponents;
	UFUNCTION()
	virtual void OnRep_OffHandIKComponents() {}
	
	UPROPERTY(BlueprintGetter = GetLightLaserComponents, ReplicatedUsing = OnRep_LightLaserComponents, Category = "SKGFirearmComponent|Components")
	TArray<USKGLightLaserComponent*> LightLaserComponents;
	UFUNCTION()
	virtual void OnRep_LightLaserComponents() {}
	
	UPROPERTY(BlueprintGetter = GetMuzzleComponents, ReplicatedUsing = OnRep_MuzzleComponents, Category = "SKGFirearmComponent|Components")
	TArray<USKGMuzzleComponent*> MuzzleComponents;
	UFUNCTION()
	virtual void OnRep_MuzzleComponents() {}

	UPROPERTY(BlueprintGetter = GetAttachmentStatComponents, ReplicatedUsing = OnRep_AttachmentStatComponents, Category = "SKGFirearmComponent|Components")
	TArray<USKGFirearmAttachmentStatComponent*> AttachmentStatComponents;
	UFUNCTION()
	virtual void OnRep_AttachmentStatComponents();
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentProceduralAnimComponent, BlueprintGetter = GetCurrentProceduralAnimComponent, Category = "SKGFirearmComponent|Components")
	TObjectPtr<USKGProceduralAnimComponent> CurrentProceduralAnimComponent;
	UFUNCTION()
	virtual void OnRep_CurrentProceduralAnimComponent();
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentOffHandIKComponent, BlueprintGetter = GetCurrentOffHandIKComponent, Category = "SKGFirearmComponent|Components")
	TObjectPtr<USKGOffHandIKComponent> CurrentOffHandIKComponent;
	UFUNCTION()
	virtual void OnRep_CurrentOffHandIKComponent() {}
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentMuzzleComponent, BlueprintGetter = GetCurrentMuzzleComponent, Category = "SKGFirearmComponent|Components")
	TObjectPtr<USKGMuzzleComponent> CurrentMuzzleComponent;
	UFUNCTION()
	virtual void OnRep_CurrentMuzzleComponent();
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentOpticComponent, BlueprintGetter = GetCurrentOpticComponent, Category = "SKGFirearmComponent|Components")
	TObjectPtr<USKGOpticComponent> CurrentOpticComponent;
	UFUNCTION()
	virtual void OnRep_CurrentOpticComponent() {}

	UPROPERTY(ReplicatedUsing = OnRep_CurrentStockComponent, BlueprintGetter = GetCurrentStockComponent, Category = "SKGFirearmComponent|Components")
	TObjectPtr<USKGStockComponent> CurrentStockComponent;
	UFUNCTION()
	virtual void OnRep_CurrentStockComponent() {}

	UPROPERTY()
	TObjectPtr<USKGProceduralAnimComponent> BeforePointAimProceduralAnimComponent;
	bool bIsPointAiming {false};

	FSKGProceduralAnimInstanceData ProceduralAnimData;
	
	bool bOldOffHandIKIsLeftHand {true};
	
	void SetBestMuzzleComponent();
	void SetBestOffHandIKComponent();
	void SetBestProceduralAnimComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; }
	FORCEINLINE bool HasAuthority() const { return GetOwnerRole() == ROLE_Authority; }
	void SetupComponents();
	// Returns true if changed
	bool SetOpticComponent();

	UFUNCTION(BlueprintNativeEvent, Category = "SKGFirearmComponent|Stats")
	void AddFirearmAttachmentStats(USKGFirearmAttachmentStatComponent* StatComponent);
	virtual void AddFirearmAttachmentStats_Implementation(USKGFirearmAttachmentStatComponent* StatComponent);
	UFUNCTION(BlueprintNativeEvent, Category = "SKGFirearmComponent|Stats")
	void RemoveFirearmAttachmentStats(USKGFirearmAttachmentStatComponent* StatComponent);
	virtual void RemoveFirearmAttachmentStats_Implementation(USKGFirearmAttachmentStatComponent* StatComponent);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SKGFirearmComponent|Stats")
	void CalculateProceduralValues();
	virtual void CalculateProceduralValues_Implementation();
	
	UFUNCTION()
	void OnAttachmentAdded(AActor* Attachment);
	UFUNCTION()
	void OnAttachmentRemoved(AActor* Attachment);
	UFUNCTION()
	void OnStockOffsetChanged(const float Offset);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetAimingDevice(USKGProceduralAnimComponent* AnimComponent);
	bool SetupNewAimingDevice(USKGProceduralAnimComponent* AnimComponent, const bool bIsAiming);
	
public:
	// Should only be used when manually setting the value for construction
	void SetFirearmMeshComponentName(const FName& Name) { FirearmMeshComponentName = Name; }
	// Should only be used when manually setting the value for construction
	void SetAttachmentManagerComponentName(const FName& Name) { AttachmentManagerComponentName = Name; }
	
	FGameplayTag GetProceduralGameplayTag() const;
	FSKGProceduralAnimInstanceData& GetProceduralData(bool bIsAiming, bool bOffHandIKIsLeftHand);
	UFUNCTION(BlueprintCallable, Category = "SKGFirearmComponent|ProceduralStats")
	void SetProceduralStats(const FSKGProceduralStats& ProceduralStatsData);
	UFUNCTION(BlueprintPure, Category = "SKGFirearmComponent|ProceduralStats")
	const FSKGProceduralStats& GetProceduralStats() const { return ProceduralAnimData.ProceduralStats; }
	
	void Held();
	
	UFUNCTION(BlueprintCallable, Category = "SKGFirearmComponent|Zero")
	void ZeroOpticsForZeroAtLocation(const FVector& Location);

	// To be called when you fire your firearm to work with muzzle temperature
	UFUNCTION(BlueprintCallable, Category = "SKGFirearmComponent|Muzzle")
	void ShotPerformed();
	UFUNCTION(BlueprintPure, Category = "SKGFirearmComponent|Muzzle")
	FSKGMuzzleTransform GetMuzzleProjectileTransform(float ZeroDistanceMeters = 100.0f, float MOA = 1.0f) const;
	UFUNCTION(BlueprintPure, Category = "SKGFirearmComponent|Muzzle")
	TArray<FSKGMuzzleTransform> GetMuzzleProjectileTransforms(float ZeroDistanceMeters = 100.0f, float MOA = 1.0f, const int32 ProjectileCount = 8) const;
	UFUNCTION(BlueprintPure, Category = "SKGFirearmComponent|Muzzle")
	FTransform GetMuzzleTransform() const;

	UFUNCTION(BlueprintPure, Category = "SKGFirearmComponent|Pose")
	bool GetPose(FGameplayTag Tag, FSKGToFromCurveSettings& PoseData);
	
	UFUNCTION(BlueprintCallable, Category = "SKGFirearmComponent|Aim")
	bool CycleAimingDevice(bool bIsAiming);
	UFUNCTION(BlueprintCallable, Category = "SKGFirearmComponent|Aim")
	bool SetAimingDevice(USKGProceduralAnimComponent* AnimComponent, const bool bIsAiming);
	UFUNCTION(BlueprintCallable, Category = "SKGFirearmComponent|Aim")
	void StartPointAiming(bool bRightHandDominant);
	UFUNCTION(BlueprintCallable, Category = "SKGFirearmComponent|Aim")
	void StopPointAiming(bool bIsAiming);
	UFUNCTION(BlueprintPure, Category = "SKGFirearmComponent|Aim")
	bool IsPointAiming() const { return bIsPointAiming; }
	UFUNCTION(BlueprintPure, Category = "SKGFirearmComponent|Aim")
	FTransform GetAimSocketWorldTransform() const;

	UFUNCTION(BlueprintCallable, Category = "SKGFirearmComponent|FirearmStats")
	void SetFirearmStats(const FSKGFirearmStats& Stats) { FirearmStats = Stats; }
	
	UFUNCTION(BlueprintGetter)
	const TArray<USKGProceduralAnimComponent*>& GetProceduralAnimComponents() const { return ProceduralAnimComponents; }
	UFUNCTION(BlueprintGetter)
	const TArray<USKGOffHandIKComponent*>& GetOffHandIKComponents() const { return OffHandIKComponents; }
	UFUNCTION(BlueprintGetter)
	const TArray<USKGLightLaserComponent*>& GetLightLaserComponents() const { return LightLaserComponents; }
	UFUNCTION(BlueprintGetter)
	const TArray<USKGMuzzleComponent*>& GetMuzzleComponents() const { return MuzzleComponents; }
	UFUNCTION(BlueprintGetter)
	const TArray<USKGFirearmAttachmentStatComponent*>& GetAttachmentStatComponents() const { return AttachmentStatComponents; }
	UFUNCTION(BlueprintGetter)
	const FSKGFirearmStats& GetFirearmStats() const { return FirearmStats; }
	UFUNCTION(BlueprintGetter)
	const USKGAttachmentManagerComponent* GetAttachmentManagerComponent() const { return AttachmentManager; }
	UFUNCTION(BlueprintGetter)
	USKGProceduralAnimComponent* GetCurrentProceduralAnimComponent() const { return CurrentProceduralAnimComponent; }	
	UFUNCTION(BlueprintGetter)
	USKGOffHandIKComponent* GetCurrentOffHandIKComponent() const { return CurrentOffHandIKComponent; }
	UFUNCTION(BlueprintGetter)
	USKGMuzzleComponent* GetCurrentMuzzleComponent() const { return CurrentMuzzleComponent; }
	UFUNCTION(BlueprintGetter)
	USKGOpticComponent* GetCurrentOpticComponent() const { return CurrentOpticComponent; }
	UFUNCTION(BlueprintGetter)
	USKGStockComponent* GetCurrentStockComponent() const { return CurrentStockComponent; }
	
	UPROPERTY(BlueprintAssignable, Category = "SKGFirearmComponent|Events")
	FOnAimingDeviceCycled OnAimingDeviceCycled;
	UPROPERTY(BlueprintAssignable, Category = "SKGFirearmComponent|Events")
	FOnProceduralAnimComponentsUpdated OnProceduralAnimComponentsUpdated;
	UPROPERTY(BlueprintAssignable, Category = "SKGFirearmComponent|Events")
	FOnMuzzleComponentUpdated OnMuzzleComponentUpdated;
	UPROPERTY(BlueprintAssignable, Category = "SKGFirearmComponent|Events")
	FOnHeld OnHeld;
	UPROPERTY(BlueprintAssignable, Category = "SKGFirearmComponent|Events")
	FOnFirearmStatsChanged OnFirearmStatsChanged;
	/*
	 * The purpose of this delegate/event is so you do not have to make a child class of this component to override CalculateProceduralValues.
	 * This is intended moreso for blueprint so you simply override this event, create your SKGProceduralStats with values you want,
	 * then call SetProceduralStats. You can get the current values by calling GetProceduralStats as well. This event fires from the
	 * CalculateProceduralValues_Implementation AFTER the defaults are set. If you override CalculateProceduralValues_Implementation without
	 * calling the parent this event will not fire.
	 */
	UPROPERTY(BlueprintAssignable, Category = "SKGFirearmComponent|Events")
	FCalculateProceduralStats CalculateProceduralStats;
};