// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "Types/TBProjectileId.h"
#include "Misc/Guid.h"


FTBProjectileId FTBProjectileId::None = FTBProjectileId(FGuid());

FTBProjectileId::FTBProjectileId()
	: Guid()
{}
FTBProjectileId::FTBProjectileId(const FString & GuidString)
	: Guid(GuidString)
{}
FTBProjectileId::FTBProjectileId(const FGuid& Guid)
	: Guid(Guid)
{}

bool FTBProjectileId::IsValid() const
{
	return Guid.IsValid();
}

void FTBProjectileId::Generate()
{
	if (!IsValid())
	{
		Guid = FGuid::NewGuid();
	}
}

void FTBProjectileId::Regenerate()
{
	Guid = FGuid::NewGuid();
}

FTBProjectileId FTBProjectileId::CreateNew()
{
	FTBProjectileId Id;
	Id.Regenerate();
	return Id;
}

bool FTBProjectileId::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << *this;
	return true;
}

FArchive& operator<<(FArchive& Ar, FTBProjectileId& ProjectileId)
{
	Ar << ProjectileId.Guid;
	return Ar;
}

FStructuredArchive::FSlot& operator<<(FStructuredArchive::FSlot& Slot, FTBProjectileId& ProjectileId)
{
	Slot << ProjectileId.Guid;
	return Slot;
}

bool FTBProjectileId::Serialize(FArchive& Ar)
{
	Ar << *this;
	return true;
}

bool FTBProjectileId::Serialize(FStructuredArchive::FSlot& Slot)
{
	Slot << *this;
	return true;
}

uint32 GetTypeHash(const FTBProjectileId& ProjectileId)
{
	return GetTypeHash(ProjectileId.Guid);
}

bool FTBProjectileId::operator==(const FTBProjectileId& rhs) const
{
	return Guid == rhs.Guid;
}

bool FTBProjectileId::operator!=(const FTBProjectileId& rhs) const
{
	return !(*this == rhs);
}
