#include "Types/ImpactParams.h"
#include "BulletDataAsset.h"
#include "Bullets.h"



FTBImpactParams::FTBImpactParams(const FHitResult& InHitResult, UBulletDataAsset* BulletDataAsset, FVector InVelocity, bool bIsPenetrating, TEnumAsByte<EPhysicalSurface> SurfaceType, const FVector& InStartLocation, double PenetrationDepth, double dV, bool bIsRicochet, const FTBProjectileId InProjectileId, const FVector& InRicochetVector)
	: FTBImpact(InHitResult, InVelocity, SurfaceType, InStartLocation, bIsRicochet, bIsPenetrating, PenetrationDepth, dV, InProjectileId, InRicochetVector)
{
	if (BulletDataAsset)
	{
		ProjectileSize = BulletDataAsset->ProjectileSize;
		BulletProperties = BulletDataAsset->BulletProperties;
		BulletInfo = FTBBulletInfo(BulletDataAsset->Name, BulletDataAsset->BulletType, BulletDataAsset->BulletVariation);
	}
}

FTBImpactParams::FTBImpactParams(const FHitResult& InHitResult, const FTBBullet& Bullet, const FVector& InVelocity, const bool bIsPenetrating, TEnumAsByte<EPhysicalSurface> SurfaceType, const FVector& InStartLocation, const bool bIsRicochet, const FTBProjectileId InProjectileId, const double dV, const FVector& InRicochetVector)
	: FTBImpact(InHitResult, InVelocity, SurfaceType, InStartLocation, Bullet.ProjectileSize, bIsRicochet, bIsPenetrating, 0.0, dV, InProjectileId, InRicochetVector)
	, BulletProperties(Bullet.BulletProperties)
	, BulletInfo(Bullet)
{}

FTBImpactParams::FTBImpactParams(const FHitResult& InHitResult, FTBBullet* Bullet, FVector InVelocity, bool bIsPenetrating, TEnumAsByte<EPhysicalSurface> SurfaceType, const FVector& InStartLocation, bool bIsRicochet, const FTBProjectileId InProjectileId, const FVector& InRicochetVector)
	: FTBImpact(InHitResult, InVelocity, SurfaceType, InStartLocation, Bullet->ProjectileSize, bIsRicochet, bIsPenetrating, 0.0, 0.0, InProjectileId, InRicochetVector)
	, BulletProperties(Bullet->BulletProperties)
	, BulletInfo(Bullet)
{}

FTBImpactParams::FTBImpactParams(const FHitResult& InHitResult, FTBBullet* Bullet, FVector InVelocity, bool bIsPenetrating, TEnumAsByte<EPhysicalSurface> SurfaceType, const FVector& InStartLocation, double PenetrationDepth, double dV, bool bIsRicochet, const FTBProjectileId InProjectileId, const FVector& InRicochetVector)
	: FTBImpact(InHitResult, InVelocity, SurfaceType, InStartLocation, Bullet->ProjectileSize, bIsRicochet, bIsPenetrating, PenetrationDepth, dV, InProjectileId, InRicochetVector)
	, BulletProperties(Bullet->BulletProperties)
	, BulletInfo(Bullet)
{}

bool FTBImpactParams::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	FTBImpact::NetSerialize(Ar, Map, bOutSuccess);

	Ar << BulletProperties;
	Ar << BulletInfo;

	return true;
}
