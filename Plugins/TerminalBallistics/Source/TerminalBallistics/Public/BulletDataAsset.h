// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "BulletUtils.h"
#include "Engine/DataAsset.h"
#include "NiagaraSystem.h"
#include "PhysMatManager/PhysMat.h"
#include "Types/TBEnums.h"
#include "Types/TBSpreadParams.h"
#include "BulletDataAsset.generated.h"

class UNiagaraSystem;
class UStaticMesh;
class UTexture2D;

/**
* Data Asset that describes a bullet.
* 
* Includes everything required to construct an FBullet, as well as some extra information like the bullet mesh, cartridge mesh, etc...
* This can be extended to include extra properties as needed.
*/
UCLASS()
class TERMINALBALLISTICS_API UBulletDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId("BulletDataAsset", GetFName()); };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ToolTip = "Name of the bullet", ShortToolTip = "Name"))
	FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ToolTip = "Physical Properties", ShortToolTip = "Properties", ShowOnlyInnerProperties, EditCondition = "!bUseExistingBullet"))
	FTBBulletPhysicalProperties BulletProperties;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	FPhysMatProperties BulletPhysMatProperties;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ToolTip = "Projectile Size (small, medium, large)", ShortToolTip = "Projectile Size"))
	ETBProjectileSize ProjectileSize = PS_Small;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ToolTip = "Caliber", ShortToolTip = "BulletType"))
	ETBBulletCaliber BulletType = BulletCaliber_Default;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ToolTip = "Bullet Variation", ShortToolTip = "Variation", NoElementDuplicate))
	TArray<ETBBulletVariation> BulletVariation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ToolTip = "Typical muzzle velocity (m/s)", Units = "m/s"))
	float MuzzleVelocity = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (Min = "1", UIMin = "1"))
	int32 ProjectileCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (EditCondition = "ProjectileCount > 1"))
	FTBSpreadParams SpreadParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Assets", meta = (AssetBundles = "Assets,Icon"))
	TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Assets|FX", meta = (AssetBundles = "Assets,Particles"))
	TSoftObjectPtr<UNiagaraSystem> BulletCasing;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Assets|FX", meta = (AssetBundles = "Assets,Particles"))
	TSoftObjectPtr<UNiagaraSystem> BulletSystem;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Assets|FX", meta = (AssetBundles = "Assets,Particles"))
	TSoftObjectPtr<UNiagaraSystem> TracerSystem;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Assets|FX", meta = (AssetBundles = "Assets,Particles", ToolTip = "If this bullet isn't specifically a tracer, this system will be used for visuals."))
	TSoftObjectPtr<UNiagaraSystem> DefaultTracerSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Assets|Meshes", meta = (AssetBundles = "Assets,Meshes"))
	TSoftObjectPtr<UStaticMesh> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Assets|Meshes", meta = (AssetBundles = "Assets,Meshes"))
	TSoftObjectPtr<UStaticMesh> BulletMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Assets|Meshes", meta = (AssetBundles = "Assets,Meshes"))
	TSoftObjectPtr<UStaticMesh> CartridgeMesh;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Preset")
	bool bUsePreset = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Preset", meta = (EditCondition = "bUsePreset && !bUseExistingBullet", EditConditionHides))
	ETBBulletCaliber PresetBulletType = ETBBulletCaliber::Bullet_9mm;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Preset", meta = (EditCondition = "bUsePreset && !bUseExistingBullet", EditConditionHides, NoElementDuplicate))
	TArray<ETBBulletVariation> Variations;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Preset", meta = (EditCondition = "bUsePreset", EditConditionHides))
	bool bUseExistingBullet = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Preset", meta = (EditCondition = "bUsePreset && bUseExistingBullet", EditConditionHides))
	ETBBulletNames PresetBullets = ETBBulletNames::Default;

#if WITH_EDITOR
	inline virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override
	{
		if (PropertyChangedEvent.Property)
		{
			const FName propertyName = PropertyChangedEvent.GetPropertyName();
			if (propertyName == FName("PresetBulletType") || propertyName == FName("bUsePreset") || propertyName == FName("Variations") || propertyName == FName("bUseExistingBullet") || propertyName == FName("PresetBullets"))
			{
				if (bUsePreset)
				{
					FTBBullet bullet = UBulletUtils::GetFullBulletFromType(PresetBulletType, Variations);

					if (bUseExistingBullet)
					{
						bullet = UBulletUtils::GetFullBulletFromName(PresetBullets);
					}

					BulletProperties = bullet.BulletProperties;
					BulletPhysMatProperties = bullet.PhysicalProperties;
					Name = bullet.BulletName;
					BulletType = bullet.BulletType;
					BulletVariation = bullet.BulletVariation;
					ProjectileSize = bullet.ProjectileSize;
					MuzzleVelocity = bullet.MuzzleVelocity;
					ProjectileCount = bullet.ProjectileCount;
				}
			}
		}
	}
#endif

	UFUNCTION(BlueprintPure, Category = "Bullet")
	inline bool IsValidBullet() const
	{
		return BulletProperties.CheckValid()
			&& BulletPhysMatProperties.CheckValid();
	}
};
