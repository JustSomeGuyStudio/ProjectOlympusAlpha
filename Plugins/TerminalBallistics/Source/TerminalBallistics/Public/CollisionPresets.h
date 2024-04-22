// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"
#include "TerminalBallisticsConfiguration.h"


namespace CollisionPresets {

	static const TArray<TEnumAsByte<EObjectTypeQuery>> DefaultCollisionQueryTypes =
	{
		TEnumAsByte<EObjectTypeQuery>(0), //WorldStatic
		TEnumAsByte<EObjectTypeQuery>(1), //WorldDynamic,
		TEnumAsByte<EObjectTypeQuery>(2), //Pawn
		TEnumAsByte<EObjectTypeQuery>(3), //PhysicsBody
		TEnumAsByte<EObjectTypeQuery>(5)  //Destructible
	};
	static const auto DefaultCollisionQueryTypesBitfield = 
		ECC_TO_BITFIELD(ECC_WorldStatic)	|
		ECC_TO_BITFIELD(ECC_WorldDynamic)	|
		ECC_TO_BITFIELD(ECC_Pawn)			|
		ECC_TO_BITFIELD(ECC_PhysicsBody)	|
		ECC_TO_BITFIELD(ECC_Destructible);

	static const TArray<TEnumAsByte<EObjectTypeQuery>> DefaultCollisionQueryTypesWithoutPawn =
	{
		TEnumAsByte<EObjectTypeQuery>(0), //WorldStatic
		TEnumAsByte<EObjectTypeQuery>(1), //WorldDynamic,
		TEnumAsByte<EObjectTypeQuery>(3), //PhysicsBody
		TEnumAsByte<EObjectTypeQuery>(5)  //Destructible
	};
	static const auto DefaultCollisionQueryTypesWithoutPawnBitfield =
		ECC_TO_BITFIELD(ECC_WorldStatic) |
		ECC_TO_BITFIELD(ECC_WorldDynamic) |
		ECC_TO_BITFIELD(ECC_PhysicsBody) |
		ECC_TO_BITFIELD(ECC_Destructible);

	static const FCollisionObjectQueryParams DefaultCollsionObjectQueryParams = FCollisionObjectQueryParams(DefaultCollisionQueryTypesBitfield);
	static const FCollisionObjectQueryParams DefaultCollsionObjectQueryParamsWithoutPawn = FCollisionObjectQueryParams(DefaultCollisionQueryTypesWithoutPawnBitfield);

	static const ECollisionChannel ECC_Projectile = TB::Configuration::ProjectileTraceChannel;

	static const FCollisionResponseParams SoloResponseChannel(ECollisionChannel Channel, ECollisionResponse ResponseType = ECR_Block)
	{
		FCollisionResponseParams Ret = FCollisionResponseParams(ECR_Ignore);
		Ret.CollisionResponse.SetResponse(Channel, ResponseType);
		return Ret;
	}

	static const FCollisionResponseParams GetProjectileCRP()
	{
		return SoloResponseChannel(ECC_Projectile);
	}

};