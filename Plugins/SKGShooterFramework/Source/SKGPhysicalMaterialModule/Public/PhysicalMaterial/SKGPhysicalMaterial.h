// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "Particles/WorldPSCPool.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "SKGPhysicalMaterial.generated.h"

class USoundBase;
class UMaterialInterface;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FSKGPhysMatSoundSettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGShooterFramework")
	TObjectPtr<USoundBase> Sound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGShooterFramework")
	float VolumeMultiplier {1.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGShooterFramework")
	float PitchMultiplier {1.0f};

	operator bool () const
	{
		return Sound != nullptr;
	}
};

USTRUCT(BlueprintType)
struct FSKGPhysMatVisualSettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGShooterFramework")
	TObjectPtr<UNiagaraSystem> ImpactEffect;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGShooterFramework")
	TObjectPtr<UMaterialInterface> ImpactDecal;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGShooterFramework")
	EPSCPoolMethod PoolingMethod {EPSCPoolMethod::AutoRelease};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGShooterFramework")
	bool bUseParticlesRotation {false};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGShooterFramework")
	FVector DecalSize {FVector(2.5f, 2.5f, 2.5f)};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGShooterFramework")
	float DecalLifeTime {8.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGShooterFramework")
	float DecalFadeScreenSize {0.001f};

	operator bool () const
	{
		return ImpactEffect != nullptr || ImpactDecal != nullptr;
	}
};

USTRUCT(BlueprintType)
struct FSKGPhysMatEffect
{
	GENERATED_BODY()
	// This tag is used as an identifier for what to play such as projectile and footstep
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGShooterFramework")
	FGameplayTag Tag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGShooterFramework")
	FSKGPhysMatVisualSettings ImpactVisualSettings;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKGShooterFramework")
	FSKGPhysMatSoundSettings ImpactSoundSettings;

	operator bool () const
	{
		return Tag.IsValid() && (ImpactVisualSettings || ImpactSoundSettings);
	}
};

UCLASS()
class SKGPHYSICALMATERIALMODULE_API USKGPhysicalMaterial : public UPhysicalMaterial, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

protected:
	/** If the speed of sound has less than this delay it will not play the sound
	 * with the speed of sound delay, instead it will be instant like normal
	 * to save resources
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SKGPhysicalMaterial|Initialize")
	float SpeedOfSoundTolerance = 0.2f;
	UPROPERTY(EditDefaultsOnly, Category = "SKGPhysicalMaterial|Initialize")
	FGameplayTagContainer GameplayTags;

	UPROPERTY(EditDefaultsOnly, Category = "SKGPhysicalMaterial|Effects")
	TArray<FSKGPhysMatEffect> Effects;

	static void PlayVisualEffect(const UWorld* World, const FHitResult& HitResult, const FSKGPhysMatVisualSettings& Settings);
	static void PlaySoundEffect(const UWorld* World, const FHitResult& HitResult, const bool bUseSpeedOfSound, const float SpeedOfSoundTolerance, const FSKGPhysMatSoundSettings& Settings);
public:
	UFUNCTION(BlueprintCallable, Category = "SKGPhysicalMaterial")
	void PlayEffect(const FHitResult& HitResult, const bool bUseSpeedOfSound, const FGameplayTag& ImpactEffectTag);
	UFUNCTION(BlueprintPure, Category = "SKGPhysicalMaterial")
	const TArray<FSKGPhysMatEffect>& GetEffects() const { return Effects; }
	// Returns true if match found
	UFUNCTION(BlueprintPure, Category = "SKGPhysicalMaterial")
	bool GetEffect(FSKGPhysMatEffect& OUTEffect, const FGameplayTag& EffectTag) const;
	
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; }
};
