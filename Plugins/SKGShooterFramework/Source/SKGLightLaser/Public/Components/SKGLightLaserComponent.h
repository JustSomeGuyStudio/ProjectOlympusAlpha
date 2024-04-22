// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "Components/ActorComponent.h"
#include "Interfaces/SKGInfraredInterface.h"
#include "SKGLightLaserComponent.generated.h"

DECLARE_STATS_GROUP(TEXT("SKGShooterFrameworkLightLaserComponentStatsGroup"), STATGROUP_SKGLightLaserComponent, STATCAT_Advanced);

class ULightComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class APlayerController;

UENUM(BlueprintType)
enum class ESKGLaserMode : uint8
{
	Off		UMETA(DisplayName = "Off"),
	On		UMETA(DisplayName = "On")
};

UENUM(BlueprintType)
enum class ESKGLightMode : uint8
{
	Off		UMETA(DisplayName = "Off"),
	On		UMETA(DisplayName = "On"),
	Strobe	UMETA(DisplayName = "Strobe")
};

USTRUCT(BlueprintType)
struct FSKGLaserMaterial
{
	GENERATED_BODY()
	// Material to be used for the laser mesh
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser")
	TObjectPtr<UMaterialInterface> Laser;
	// Material to be used for the laser dot
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser")
	TObjectPtr<UMaterialInterface> LaserDot;

	FSKGLaserMaterial() {};
	FSKGLaserMaterial(UMaterialInterface* LaserMaterial, UMaterialInterface* DecalMaterial)
	{
		Laser = LaserMaterial;
		LaserDot = DecalMaterial;
	}
	
	operator bool() const
	{
		return Laser && LaserDot;
	}

	bool operator ==(const FSKGLaserMaterial& RHS) const
	{
		return Laser == RHS.Laser && LaserDot == RHS.LaserDot;
	}

	bool operator !=(const FSKGLaserMaterial& RHS) const
	{
		return Laser != RHS.Laser || LaserDot != RHS.LaserDot;
	}
};

USTRUCT(BlueprintType)
struct FSKGLaserState
{
	GENERATED_BODY()
	UPROPERTY()
	ESKGLaserMode LaserMode {ESKGLaserMode::Off};

	bool bCanScaleLaser {false};
	FSKGLaserMaterial CurrentLaserMaterial;
};

USTRUCT(BlueprintType)
struct FSKGLaserSettings
{
	GENERATED_BODY()
	ECollisionChannel LaserCollisionChannel {ECC_Visibility};
	// Max distance the laser will go
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser")
	float MaxLaserDistance {20000.0f};
	// Based on length of your laser mesh, the provided example is 2000 units long so the scale factor is 20
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser")
	float LaserScaleFactor {20.0f};
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser")
	FSKGLaserMaterial InfraredLaserMaterial;
	UPROPERTY()
	FSKGLaserMaterial DefaultLaserMaterial;

public:
	FSKGLaserMaterial GetMaterial(bool bInfrared) const
	{
		return bInfrared && InfraredLaserMaterial ? InfraredLaserMaterial : DefaultLaserMaterial;
	}

	void SetupDefaultMaterial(const FSKGLaserMaterial& LaserMaterial)
	{
		DefaultLaserMaterial.Laser = LaserMaterial.Laser;
		DefaultLaserMaterial.LaserDot = LaserMaterial.LaserDot;
	}
};

USTRUCT(BlueprintType)
struct FSKGLightState
{
	GENERATED_BODY()
	UPROPERTY()
	ESKGLightMode LightMode {ESKGLightMode::Off};
	UPROPERTY()
	int8 LightIntensityIndex {0};

	float OnIntensity {0.0f};
	bool bCanStrobe {false};
};

USTRUCT(BlueprintType)
struct FSKGLightSettings
{
	GENERATED_BODY()
	// Time between the light turning on/off for the strobing
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser")
	double LightStrobeInterval {0.08f};

protected:
	// Light brightness settings
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser")
	TArray<float> LightIntensities {50.0f};
	// Infrared (night vision) brightness settings
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser")
	TArray<float> LightIntensitiesInfrared {10.0f};

public:
	float GetLightIntensity(bool bInfrared, int32 Index) const
	{
		const TArray<float>* Intensities = bInfrared ? &LightIntensitiesInfrared : &LightIntensities;
		Index = FMath::Clamp(Index, 0, Intensities->Num());
		return (*Intensities)[Index];
	}

	bool IsValidIndex(bool bInfrared, int32 Index) const
	{
		return (bInfrared ? &LightIntensitiesInfrared : &LightIntensities)->IsValidIndex(Index);
	}

	int32 GetNextLightIntensityIndex(bool bInfrared, int32 Index) const
	{
		const int32 ArrayLength = (bInfrared ? LightIntensitiesInfrared : LightIntensities).Num();
		return ++Index >= ArrayLength ? 0 : Index;
	}
};

USTRUCT(BlueprintType)
struct FSKGLightLaserCycleMode
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser")
	ESKGLaserMode LaserMode {ESKGLaserMode::Off};
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser")
	ESKGLightMode LightMode {ESKGLightMode::Off};
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser")
	bool bInfrared {false};

	bool operator == (const FSKGLightLaserCycleMode& RHS) const
	{
		return RHS.LaserMode == LaserMode && RHS.LightMode == LightMode && RHS.bInfrared == bInfrared;
	}
};

USTRUCT(BlueprintType)
struct FSKGLightLaserCycleModes
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser")
	TArray<FSKGLightLaserCycleMode> LightLaserModes;

private:
	int32 Index {0};

public:
	const FSKGLightLaserCycleMode& GetNextLightLaserMode()
	{
		if (++Index >= LightLaserModes.Num())
		{
			Index = 0;
		}
		return LightLaserModes[Index];
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLaserStateChanged, ESKGLaserMode, LaserMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLightModeChanged, ESKGLightMode, LightMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInfraredModeChanged, bool, bInInfraredMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLightStrobed, bool, bOn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLaserImpact, const FHitResult&, HitResult, bool, bHit);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SKGLIGHTLASER_API USKGLightLaserComponent : public UActorComponent, public IGameplayTagAssetInterface, public ISKGInfraredInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USKGLightLaserComponent();

protected:
	// Optional name of the Light Component (Such as SpotLight) to be used
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser|Initialize")
	FName LightComponentName {"SpotLight"};
	// Optional name of the laser mesh component to be used
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser|Initialize")
	FName LaserMeshComponentName {"LaserMesh"};
	// Optional name of the laser dot mesh component to be used
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser|Initialize")
	FName LaserDotComponentName {"LaserDot"};
	// Whether or not this device supports infrared mode
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser|Initialize")
	bool bHasInfraredMode {false};
	// Useful if your NetUpdateFrequency is set super low
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser|Initialize")
	bool bAutoCallForceNetUpdate {true};
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser|Initialize")
	FGameplayTagContainer GameplayTags;
	
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser|Settings")
	FSKGLaserSettings LaserSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser|Settings")
	FSKGLightSettings LightSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGLightLaser|Settings")
	FSKGLightLaserCycleModes LightLaserCycleModes;

private:
	UPROPERTY(BlueprintGetter = GetLightSource, Category = "SKGLightLaser|Data")
	TObjectPtr<ULightComponent> LightSource;
	UPROPERTY(BlueprintGetter = GetLaserMesh, Category = "SKGLightLaser|Data")
	TObjectPtr<UStaticMeshComponent> LaserMesh;
	UPROPERTY(BlueprintGetter = GetLaserDot, Category = "SKGLightLaser|Data")
	TObjectPtr<UStaticMeshComponent> LaserDot;

protected:
	FVector LaserScale {FVector::OneVector};

	UPROPERTY()
	TObjectPtr<APlayerController> LocalPlayerController;

	UPROPERTY(ReplicatedUsing = OnRep_DeviceInfraredOn, BlueprintGetter = IsInInfraredMode, Category = "SKGLightLaser|Data")
	bool bDeviceInfraredOn {false};
	UFUNCTION()
	void OnRep_DeviceInfraredOn();
	
	UPROPERTY(ReplicatedUsing = OnRep_LaserState)
	FSKGLaserState LaserState;
	UFUNCTION()
	void OnRep_LaserState();
	
	UPROPERTY(ReplicatedUsing = OnRep_LightState)
	FSKGLightState LightState;
	UFUNCTION()
	void OnRep_LightState();
	double PreviousStrobeTimeStamp {0.0f};
	
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; }
	FORCEINLINE bool HasAuthority() const { return GetOwnerRole() == ROLE_Authority; }
	FORCEINLINE void TryForceNetUpdate() const;
	void SetComponentsAndState();
	void RegisterAsInfraredDevice();
	void UnregisterAsInfraredDevice();
	void PerformLaserScaling();
	void PerformLightStrobing();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetInfraredMode(bool bInfraredOn);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetLaserMode(ESKGLaserMode LaserMode);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetLightMode(ESKGLightMode LightMode);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetLightIntensityIndex(int32 Index);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetLightLaserMode(const FSKGLightLaserCycleMode& LightLaserMode);

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Should only be used when manually setting the value for construction
	void SetLightComponentName(const FName& Name) { LightComponentName = Name; }
	// Should only be used when manually setting the value for construction
	void SetLaserMeshComponentName(const FName& Name) { LaserMeshComponentName = Name; }
	// Should only be used when manually setting the value for construction
	void SetLaserDotComponentName(const FName& Name) { LaserDotComponentName = Name; }
	
	// Sets whether the device should be in infrared mode (for night vision use)
	UFUNCTION(BlueprintCallable, Category = "SKGLightLaser|LightLaser")
	void SetInfraredMode(bool InfraredModeOn);
	// Cycles through the defined lightlaser modes in LightLaserCycleModes (similar to tarkov setup)
	UFUNCTION(BlueprintCallable, Category = "SKGLightLaser|Laser")
	void CycleLightLaserMode();
	
	// Sets the laser to be off/on
	UFUNCTION(BlueprintCallable, Category = "SKGLightLaser|Laser")
	void SetLaserMode(ESKGLaserMode LaserMode);
	// Gets whether or not the laser is in the On or Off state
	UFUNCTION(BlueprintPure, Category = "SKGLightLaser|Laser")
	ESKGLaserMode GetLaserMode() const { return LaserState.LaserMode; }
	UFUNCTION(BlueprintPure, Category = "SKGLightLaser|Laser")
	bool IsLaserOn() const { return LaserState.LaserMode != ESKGLaserMode::Off; }
	
	// Sets the light to be Off/On/Strobe
	UFUNCTION(BlueprintCallable, Category = "SKGLightLaser|Light")
	void SetLightMode(ESKGLightMode LightMode);
	// Gets the current state of the light mode such as On/Off/Strobe
	UFUNCTION(BlueprintPure, Category = "SKGLightLaser|Light")
	ESKGLightMode GetLightMode() const { return LightState.LightMode; }
	// Gets whether the light is in On/Strobe (true) or Off (false)
	UFUNCTION(BlueprintPure, Category = "SKGLightLaser|Light")
	bool IsLightOn() const { return LightState.LightMode != ESKGLightMode::Off; }
	// Manually set the light intensity to a specific index in the in use array (normal/infrared)
	UFUNCTION(BlueprintCallable, Category = "SKGLightLaser|Light")
	void SetLightIntensityIndex(int32 Index);
	// Cycles through the light intensities set in the settings (uses infrared settings if in infrared mode)
	UFUNCTION(BlueprintCallable, Category = "SKGLightLaser|Light")
	void CycleLightIntensity() { SetLightIntensityIndex(LightSettings.GetNextLightIntensityIndex(bDeviceInfraredOn, LightState.LightIntensityIndex)); }

	UFUNCTION(BlueprintGetter)
	ULightComponent* GetLightSource() const { return LightSource; }
	UFUNCTION(BlueprintGetter)
	UStaticMeshComponent* GetLaserMesh() const { return LaserMesh; }
	UFUNCTION(BlueprintGetter)
	UStaticMeshComponent* GetLaserDot() const { return LaserDot; }
	UFUNCTION(BlueprintGetter)
	bool IsInInfraredMode() const { return bDeviceInfraredOn; }
	
	// START OF SKGInfraredInterface-----------------------------------------
	virtual bool IsInfraredModeOnForDevice() const override { return bDeviceInfraredOn; }
	virtual void OnInfraredEnabledForPlayer() override;
	virtual void OnInfraredDisabledForPlayer() override;
	// END OF SKGInfraredInterface-------------------------------------------

	// Events
	UPROPERTY(BlueprintAssignable, Category = "SKGLightLaser|Events")
	FOnLaserStateChanged OnLaserStateChanged;
	UPROPERTY(BlueprintAssignable, Category = "SKGLightLaser|Events")
	FOnLightModeChanged OnLightModeChanged;
	UPROPERTY(BlueprintAssignable, Category = "SKGLightLaser|Events")
	FOnInfraredModeChanged OnInfraredModeChanged;
	UPROPERTY(BlueprintAssignable, Category = "SKGLightLaser|Events")
	FOnLightStrobed OnLightStrobed;
	UPROPERTY(BlueprintAssignable, Category = "SKGLightLaser|Events")
	FOnLaserImpact OnLaserImpact;
};