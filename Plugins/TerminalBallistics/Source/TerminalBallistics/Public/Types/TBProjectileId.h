// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Serialization/StructuredArchive.h"
#include "TBProjectileId.generated.h"



struct FGuid;
class UPackageMap;

USTRUCT(BlueprintType, meta = (DisplayName = "ProjectileId", DisableSplitPin))
struct TERMINALBALLISTICS_API FTBProjectileId
{
	GENERATED_BODY()
public:
	FGuid Guid;

	static FTBProjectileId None;

	FTBProjectileId();
	explicit FTBProjectileId(const FString& GuidString);
	explicit FTBProjectileId(const FGuid& Guid);

	bool IsValid() const;
	
	/* Unless already valid, generates a new Guid */
	void Generate();

	/* Generates a new Guid */
	void Regenerate();

	static FTBProjectileId CreateNew();

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

	friend FArchive& operator<<(FArchive& Ar, FTBProjectileId& ProjectileId);

	friend FStructuredArchive::FSlot& operator<<(FStructuredArchive::FSlot& Slot, FTBProjectileId& ProjectileId);

	bool Serialize(FArchive& Ar);

	bool Serialize(FStructuredArchive::FSlot& Slot);

	friend uint32 GetTypeHash(const FTBProjectileId& ProjectileId);

	bool operator==(const FTBProjectileId& rhs) const;

	bool operator!=(const FTBProjectileId& rhs) const;
};
template<> struct TCanBulkSerialize<FTBProjectileId> { enum { Value = true }; };
template<> struct TIsPODType<FTBProjectileId> { enum { Value = true }; };
template<> struct TStructOpsTypeTraits<FTBProjectileId> : public TStructOpsTypeTraitsBase2<FTBProjectileId>
{
	enum
	{
		WithIdenticalViaEquality = true,
		WithSerializer = true,
		WithStructuredSerializer = true,
		WithNetSerializer = true,
		WithNetSharedSerialization = true
	};
};
