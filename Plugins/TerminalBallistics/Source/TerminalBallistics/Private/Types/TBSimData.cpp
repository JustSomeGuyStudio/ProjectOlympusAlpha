// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "Types/TBSimData.h"
#include "Bullets.h"


FTBBulletSimData::FTBBulletSimData(BulletPointer InBullet, const int32 DebugType, const int UpdateBroadcastInterval, const bool IsTestTask, const FTBProjectileId& InId)
	: FTBSimData(DebugType, UpdateBroadcastInterval, IsTestTask, InId)
{
	SetBullet(InBullet);
}

FTBBulletSimData::FTBBulletSimData(BulletPointer InBullet, const bool bDrawDebugTrace, const bool bPrintDebugInfo, const int UpdateBroadcastInterval, const bool IsTestTask, const FTBProjectileId& InId)
	: FTBSimData(bDrawDebugTrace, bPrintDebugInfo, UpdateBroadcastInterval, IsTestTask, InId)
{
	SetBullet(InBullet);
}

FTBBulletSimData::FTBBulletSimData(const UBulletDataAsset* BulletDataAsset, const int32 DebugType, const int UpdateBroadcastInterval, const bool IsTestTask, const FTBProjectileId& InId)
	: FTBSimData(DebugType, UpdateBroadcastInterval, IsTestTask, InId)
{
	SetBullet(MakeShareable<FTBBullet>(FTBBullet::CreateFromBulletDataAsset(BulletDataAsset)));
}

void FTBBulletSimData::SetBullet(BulletPointer InBullet)
{
	checkf(InBullet.IsValid(), TEXT("FTBBulletSimData: Attempted to set Bullet to an invalid value."));
	Bullet = InBullet;
	TracerSystem = Bullet->GetTracerSystem();
}

double FTBBulletSimData::GetRadius() const
{
	return Bullet->BulletProperties.Radius;
}
