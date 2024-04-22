// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "Types/TBBulletInfo.h"
#include "Bullets.h"
#include "Engine/NetSerialization.h"


FTBBulletInfo::FTBBulletInfo()
{
	const FTBBulletInfo DefaultInfo = GetDefaultBulletInfo();
	BulletName = DefaultInfo.BulletName;
	BulletType = DefaultInfo.BulletType;
	BulletVariation = DefaultInfo.BulletVariation;
};

FTBBulletInfo::FTBBulletInfo(const FTBBulletInfo& InBulletInfo)
	: BulletName(InBulletInfo.BulletName)
	, BulletType(InBulletInfo.BulletType)
	, BulletVariation(InBulletInfo.BulletVariation)
{}

FTBBulletInfo::FTBBulletInfo(FName InName, ETBBulletCaliber InBulletType, TArray<ETBBulletVariation> InVariation)
	: BulletName(InName)
	, BulletType(InBulletType)
	, BulletVariation(InVariation)
{}

FTBBulletInfo::FTBBulletInfo(FTBBullet InBullet)
	: BulletName(InBullet.BulletName)
	, BulletType(InBullet.BulletType)
	, BulletVariation(InBullet.BulletVariation)
{}

FTBBulletInfo::FTBBulletInfo(FTBBullet* InBullet)
	: BulletName(InBullet->BulletName)
	, BulletType(InBullet->BulletType)
	, BulletVariation(InBullet->BulletVariation)
{}

FTBBulletInfo::FTBBulletInfo(BulletPointer InBullet)
	: BulletName(InBullet->BulletName)
	, BulletType(InBullet->BulletType)
	, BulletVariation(InBullet->BulletVariation)
{}

FTBBulletInfo::FTBBulletInfo(BulletPointerUnique InBullet)
	: BulletName(InBullet->BulletName)
	, BulletType(InBullet->BulletType)
	, BulletVariation(InBullet->BulletVariation)
{}

const FTBBulletInfo FTBBulletInfo::GetDefaultBulletInfo()
{
	return FTBBulletInfo(FTBBullet::DefaultBullet.BulletName, FTBBullet::DefaultBullet.BulletType);
}

bool FTBBulletInfo::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << *this;

	bOutSuccess = true;
	return bOutSuccess;
}

FArchive& operator<<(FArchive& Ar, FTBBulletInfo& BulletInfo)
{
	Ar << BulletInfo.BulletName;
	Ar << BulletInfo.BulletType;
	Ar << BulletInfo.BulletVariation;

	return Ar;
}