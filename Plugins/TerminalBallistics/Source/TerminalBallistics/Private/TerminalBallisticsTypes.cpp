// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "TerminalBallisticsTypes.h"
#include "MathUtils.h"
#include "Net/Core/Serialization/QuantizedVectorSerialization.h"
#include "TBFlyByInterface.h"
#include "Types/TBEnums.h"



bool FTBLocationAndDirection::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << *this;
	bOutSuccess = true;
	return bOutSuccess;
}

FArchive& operator<<(FArchive& Ar, FTBLocationAndDirection& LocationAndDirection)
{
	using namespace UE::Net;
	SerializeFixedVector<1, 16>(LocationAndDirection.Location, Ar);
	SerializeFixedVector<1, 16>(LocationAndDirection.Direction, Ar);
	return Ar;
}

bool FTBLaunchParams::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	using namespace UE::Net;
	TB_PACK_ARCHIVE_WITH_BITFIELDS_THREE(Ar, IgnoreOwner, AddToOwnerVelocity, bForceNoTracer);
	Ar << ProjectileSpeed;
	Ar << EffectiveRange;
	Ar << Timescale;
	Ar << OwnerIgnoreDistance;
	Ar << FireTransform;
	bOutSuccess = SafeNetSerializeTArray_Default<63>(Ar, ToIgnore);
	bOutSuccess &= SafeNetSerializeTArray_Default<31>(Ar, ObjectTypes);
	SerializeOptionalValue(Ar.IsSaving(), Ar, TraceChannel, (TEnumAsByte<ECollisionChannel>)ECC_GameTraceChannel10);
	SerializeOptionalValue(Ar.IsSaving(), Ar, Owner, (TObjectPtr<AActor>)nullptr);
	SerializeOptionalValue(Ar.IsSaving(), Ar, Instigator, (TObjectPtr<AController>)nullptr);
	SerializeOptionalValue(Ar.IsSaving(), Ar, SimType, ETBProjectileSimulationType::Default);
	return bOutSuccess;
}

void FTBFlyBy::NotifyActorOfFlyBy()
{
	check(IsInGameThread());

	if (Actor->Implements<UTBFlyByInterface>())
	{
		ITBFlyByInterface::Execute_RecieveFlyByEvent(Actor, *this);
	}
}
