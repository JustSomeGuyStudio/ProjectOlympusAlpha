// Copyright 2023, Dakota Dawe, All rights reserved


#include "PhysicalMaterial/SKGPhysicalMaterial.h"
#include "Statics/SKGShooterFrameworkCoreEffectStatics.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"


void USKGPhysicalMaterial::PlayVisualEffect(const UWorld* World, const FHitResult& HitResult, const FSKGPhysMatVisualSettings& Settings)
{
	if (Settings)
	{
		FRotator Rotation = FRotator::ZeroRotator;
		if (!Settings.bUseParticlesRotation)
		{
			Rotation = UKismetMathLibrary::MakeRotFromX(HitResult.Normal);
		}
		
		if (UNiagaraSystem* NEffect = Cast<UNiagaraSystem>(Settings.ImpactEffect))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, NEffect, HitResult.Location, Rotation);
		}
			
		if (UMaterialInterface* ImpactDecal = Settings.ImpactDecal)
		{
			if (USceneComponent* HitComponent = HitResult.GetComponent())
			{
				Rotation.Pitch += 180.0f;
				if (UDecalComponent* DecalComponent = UGameplayStatics::SpawnDecalAttached(ImpactDecal, Settings.DecalSize, HitComponent, NAME_None,
					HitResult.Location, Rotation, EAttachLocation::KeepWorldPosition, Settings.DecalLifeTime))
				{
					DecalComponent->SetFadeScreenSize(Settings.DecalFadeScreenSize);
				}
			}
		}
	}
}

void USKGPhysicalMaterial::PlaySoundEffect(const UWorld* World, const FHitResult& HitResult, const bool bUseSpeedOfSound, const float SpeedOfSoundTolerance, const FSKGPhysMatSoundSettings& Settings)
{
	if (Settings)
	{
		float SoundDelay = 0.0f;
		if (bUseSpeedOfSound && UGameplayStatics::GetPlayerPawn(World, 0))
		{
			SoundDelay = USKGShooterFrameworkCoreEffectStatics::GetSpeedOfSoundDelay(HitResult.GetActor()->GetActorLocation(), UGameplayStatics::GetPlayerPawn(World, 0)->GetActorLocation());
		}
		if (!bUseSpeedOfSound || SoundDelay < SpeedOfSoundTolerance)
		{
			UGameplayStatics::SpawnSoundAtLocation(World, Settings.Sound, HitResult.Location, FRotator::ZeroRotator, Settings.VolumeMultiplier, Settings.PitchMultiplier, 0.0f, Settings.Sound->AttenuationSettings);
		}
		else
		{
			USKGShooterFrameworkCoreEffectStatics::PlayImpactSoundEffectSpeedOfSound(World, SoundDelay, HitResult.Location, Settings.Sound, Settings.VolumeMultiplier, Settings.PitchMultiplier);
		}
	}
}

void USKGPhysicalMaterial::PlayEffect(const FHitResult& HitResult, const bool bUseSpeedOfSound, const FGameplayTag& ImpactEffectTag)
{
	if (HitResult.GetActor() && HitResult.GetActor()->GetWorld())
	{
		const UWorld* World = HitResult.GetActor()->GetWorld();
		FSKGPhysMatEffect ImpactEffect;
		if (World && GetEffect(ImpactEffect, ImpactEffectTag) && ImpactEffect)
		{
			PlayVisualEffect(World, HitResult, ImpactEffect.ImpactVisualSettings);
			PlaySoundEffect(World, HitResult, bUseSpeedOfSound, SpeedOfSoundTolerance, ImpactEffect.ImpactSoundSettings);
		}
	}
}

bool USKGPhysicalMaterial::GetEffect(FSKGPhysMatEffect& OUTEffect, const FGameplayTag& EffectTag) const
{
	for (const FSKGPhysMatEffect& ImpactEffect : Effects)
	{
		if (ImpactEffect.Tag.MatchesTagExact(EffectTag))
		{
			OUTEffect = ImpactEffect;
			return true;
		}
	}
	return false;
}
