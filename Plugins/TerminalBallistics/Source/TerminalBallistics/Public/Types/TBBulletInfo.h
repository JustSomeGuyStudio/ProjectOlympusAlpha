// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/TBEnums.h"
#include "TBBulletInfo.generated.h"


struct FTBBullet;
typedef TSharedPtr<FTBBullet> BulletPointer;
typedef TUniquePtr<FTBBullet> BulletPointerUnique;


USTRUCT(BlueprintType)
/**
* Struct that holds non-critical information about a bullet.
* (Name, Caliber, etc...)
*/
struct TERMINALBALLISTICS_API FTBBulletInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = BulletInfo)
	FName BulletName;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = BulletInfo)
	ETBBulletCaliber BulletType;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = BulletInfo)
	TArray<ETBBulletVariation> BulletVariation;

	FTBBulletInfo();

	FTBBulletInfo(const FTBBulletInfo& InBulletInfo);

	FTBBulletInfo(FName InName, ETBBulletCaliber InBulletType, TArray<ETBBulletVariation> InVariation = {});

	explicit FTBBulletInfo(FTBBullet InBullet);

	explicit FTBBulletInfo(FTBBullet* InBullet);

	explicit FTBBulletInfo(BulletPointer InBullet);

	explicit FTBBulletInfo(BulletPointerUnique InBullet);

	static const FTBBulletInfo GetDefaultBulletInfo();

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

	friend FArchive& operator<<(FArchive& Ar, FTBBulletInfo& BulletInfo);

	FORCEINLINE bool operator==(const FTBBulletInfo& other) const
	{
		return BulletName == other.BulletName
			&& BulletType == other.BulletType
			&& BulletVariation == other.BulletVariation;
	}
	FORCEINLINE bool operator!=(const FTBBulletInfo& other) const
	{
		return !(*this == other);
	}
};
template<> struct TStructOpsTypeTraits<FTBBulletInfo> : public TStructOpsTypeTraitsBase2<FTBBulletInfo>
{
	enum
	{
		WithIdenticalViaEquality = true,
		WithNetSerializer = true,
		WithNetSharedSerialization = true
	};
};
