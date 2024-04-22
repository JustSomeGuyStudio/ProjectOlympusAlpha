// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "BulletUtils.h"
#include "Bullets.h"


FTBBulletPhysicalProperties UBulletUtils::GetBulletFromType(ETBBulletCaliber Type)
{
	using namespace TB::Bullets::BulletPhysicalProperties;
	switch (Type)
	{
	case ETBBulletCaliber::Bullet_9mm:		return BULLET_9x19PARA;
	case ETBBulletCaliber::Bullet_556x45:	return BULLET_556x45NATO;
	case ETBBulletCaliber::Bullet_762x51:	return BULLET_762x51NATO;
	case ETBBulletCaliber::Bullet_762x54:	return BULLET_762x54;
	case ETBBulletCaliber::Bullet_762x39:	return BULLET_762x39;
	case ETBBulletCaliber::Bullet_545x39:	return BULLET_545x39;
	case ETBBulletCaliber::Bullet_50:		return BULLET_50BMG;
	case ETBBulletCaliber::Bullet_308:		return BULLET_308;
	case ETBBulletCaliber::Bullet_338:		return BULLET_338;
	case ETBBulletCaliber::Bullet_46x30:	return BULLET_46x30;
	case ETBBulletCaliber::Bullet_57x28:	return BULLET_57x28;

	case ETBBulletCaliber::Bullet_9x18:		return BULLET_9x18;

	case ETBBulletCaliber::Bullet_45ACP:	return BULLET_45ACP;

	case ETBBulletCaliber::Bullet_12Gauge00:return BULLET_12Gauge00Buck;
	default:								return BULLET_9x19PARA;
	}
}

FTBBullet UBulletUtils::GetFullBulletFromType(ETBBulletCaliber Type, TArray<ETBBulletVariation> Variation)
{
	using namespace TB::Bullets;
	BulletTypes types = BulletTypes::Get();
	switch (Type)
	{
	case ETBBulletCaliber::Bullet_9mm:			return types.Bullet_9x19Para;
	case ETBBulletCaliber::Bullet_556x45:		return types.Bullet_556x45NATO;
	case ETBBulletCaliber::Bullet_762x51:
		if (Variation.Num() > 0 && Variation.Contains(ETBBulletVariation::HOLLOW))
		{
			return types.Bullet_762x51NATOLR;
		}
		return types.Bullet_762x51NATO;
	case ETBBulletCaliber::Bullet_762x54:		return types.Bullet_762x54;
	case ETBBulletCaliber::Bullet_762x39:		return types.Bullet_762x39;
	case ETBBulletCaliber::Bullet_545x39:
		if (Variation.Num() > 0 && Variation.Contains(ETBBulletVariation::AP))
		{
			return types.Bullet_545x39AP;
		}
		else
		{
			return types.Bullet_545x39;
		}
	case ETBBulletCaliber::Bullet_50:			return types.Bullet_50BMG;
	case ETBBulletCaliber::Bullet_308:
		if (Variation.Num() > 0 && Variation.Contains(ETBBulletVariation::FMJ))
		{
			return types.Bullet_308FMJ;
		}
		else
		{
			return types.Bullet_308;
		}
	case ETBBulletCaliber::Bullet_338:			return types.Bullet_338;
	case ETBBulletCaliber::Bullet_46x30:		return types.Bullet_46x30;
	case ETBBulletCaliber::Bullet_57x28:
		// TODO: Add AP 5.7x28mm
		/*if (Variation.Num() > 0 && Variation.Contains(ETBBulletVariation::AP))
		{
			
		}
		else*/
		{
			return types.Bullet_57x28;
		}

	case ETBBulletCaliber::Bullet_22LR:			return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_65:			return FTBBullet::DefaultBullet; // NOT IMPLEMENTED

	case ETBBulletCaliber::Bullet_9x18:			return types.Bullet_9x18;
	case ETBBulletCaliber::Bullet_300AAC:
		if (Variation.Num() > 0 && Variation.Contains(ETBBulletVariation::SUBSONIC))
		{
			return types.Bullet_300AACSub;
		}
		else
		{
			return types.Bullet_300AAC;
		}

	case ETBBulletCaliber::Bullet_300SVG:		return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_300WHSP:		return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_300WINMAG:	return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_408:			return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_416:			return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_10mm:			return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_40:			return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_44Mag:		return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_44SW:			return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_444:			return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	
	case ETBBulletCaliber::Bullet_45ACP:		return types.Bullet_45ACP;

	case ETBBulletCaliber::Bullet_45WINMAG:		return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_50BEOWULF:	return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_50AE:			return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_127x108:		return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_145x114:		return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_38SP:			return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_38ACP:		return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_380:			return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_357:			return FTBBullet::DefaultBullet; // NOT IMPLEMENTED
	case ETBBulletCaliber::Bullet_357SIG:		return FTBBullet::DefaultBullet; // NOT IMPLEMENTED

	case ETBBulletCaliber::Bullet_12Gauge00:	return types.Bullet_12Gauge00Buckshot;
	default:									return FTBBullet::DefaultBullet;
	}
}

FTBBullet UBulletUtils::GetFullBulletFromName(ETBBulletNames Name)
{
	using namespace TB::Bullets;
	BulletTypes types = BulletTypes::Get();
	const auto Mappings = types.GetMappings();
	const auto Value = Mappings.Find(Name);
	if (Value)
	{
		return *Value;
	}
	else
	{
		return FTBBullet::DefaultBullet;
	}
}

ETBBulletNames UBulletUtils::GetNameFromBullet(const FTBBullet& Bullet)
{
	using namespace TB::Bullets;
	const BulletTypes& types = BulletTypes::Get();
	const auto Mappings = types.GetMappings();
	const auto key = Mappings.FindKey(Bullet);
	if (key)
	{
		return *key;
	}
	else
	{
		return ETBBulletNames::Default;
	}
}

double UBulletUtils::GetTypicalMuzzleVelocityForBullet(const ETBBulletCaliber Caliber)
{
	switch (Caliber)
	{
	case ETBBulletCaliber::Bullet_9mm:			return 350;
	case ETBBulletCaliber::Bullet_556x45:		return 961;
	case ETBBulletCaliber::Bullet_762x51:		return 850;
	case ETBBulletCaliber::Bullet_762x54:		return 828;
	case ETBBulletCaliber::Bullet_762x39:		return 730.3;
	case ETBBulletCaliber::Bullet_545x39:		return 880;
	case ETBBulletCaliber::Bullet_50:			return 860;
	case ETBBulletCaliber::Bullet_308:			return 940;
	case ETBBulletCaliber::Bullet_338:			return 836;
	case ETBBulletCaliber::Bullet_46x30:		return 720;
	case ETBBulletCaliber::Bullet_57x28:		return 792;
	case ETBBulletCaliber::Bullet_22LR:
		return 0.0;
	case ETBBulletCaliber::Bullet_65:
		return 0.0;
	case ETBBulletCaliber::Bullet_9x18:			return 298;
	case ETBBulletCaliber::Bullet_300AAC:		return 724;
	case ETBBulletCaliber::Bullet_300SVG:
		return 0.0;
	case ETBBulletCaliber::Bullet_300WHSP:
		return 0.0;
	case ETBBulletCaliber::Bullet_300WINMAG:
		return 0.0;
	case ETBBulletCaliber::Bullet_408:
		return 0.0;
	case ETBBulletCaliber::Bullet_416:
		return 0.0;
	case ETBBulletCaliber::Bullet_10mm:
		return 0.0;
	case ETBBulletCaliber::Bullet_40:
		return 0.0;
	case ETBBulletCaliber::Bullet_44Mag:
		return 0.0;
	case ETBBulletCaliber::Bullet_44SW:
		return 0.0;
	case ETBBulletCaliber::Bullet_444:
		return 0.0;
	case ETBBulletCaliber::Bullet_45ACP:		return 263.5;
	case ETBBulletCaliber::Bullet_45WINMAG:
		return 0.0;
	case ETBBulletCaliber::Bullet_50BEOWULF:
		return 0.0;
	case ETBBulletCaliber::Bullet_50AE:
		return 0.0;
	case ETBBulletCaliber::Bullet_127x108:
		return 0.0;
	case ETBBulletCaliber::Bullet_145x114:
		return 0.0;
	case ETBBulletCaliber::Bullet_38SP:
		return 0.0;
	case ETBBulletCaliber::Bullet_38ACP:
		return 0.0;
	case ETBBulletCaliber::Bullet_380:			return 291.0;
	case ETBBulletCaliber::Bullet_357:
		return 0.0;
	case ETBBulletCaliber::Bullet_357SIG:
		return 0.0;
	case ETBBulletCaliber::Bullet_12Gauge00:	return 373.38;
	default:
		return 0.0;
	}
}

double UBulletUtils::GetTypicalEffectiveRangeForBullet(const ETBBulletCaliber Caliber)
{
	static constexpr double DEFAULT_RANGE = 200;
	switch (Caliber)
	{
	case ETBBulletCaliber::Bullet_9mm:			return 400;
	case ETBBulletCaliber::Bullet_556x45:		return 600;
	case ETBBulletCaliber::Bullet_762x51:		return 900;
	case ETBBulletCaliber::Bullet_762x54:		return 1000;
	case ETBBulletCaliber::Bullet_762x39:		return 600;
	case ETBBulletCaliber::Bullet_545x39:		return 680;
	case ETBBulletCaliber::Bullet_50:			return 2000;
	case ETBBulletCaliber::Bullet_308:			return 1000;
	case ETBBulletCaliber::Bullet_338:			return 1500;
	case ETBBulletCaliber::Bullet_46x30:		return 400;
	case ETBBulletCaliber::Bullet_57x28:		return 500;
	case ETBBulletCaliber::Bullet_22LR:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_65:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_9x18:			return 50;
	case ETBBulletCaliber::Bullet_300AAC:		return 400;
	case ETBBulletCaliber::Bullet_300SVG:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_300WHSP:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_300WINMAG:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_408:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_416:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_10mm:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_40:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_44Mag:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_44SW:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_444:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_45ACP:		return 150.0;
	case ETBBulletCaliber::Bullet_45WINMAG:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_50BEOWULF:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_50AE:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_127x108:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_145x114:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_38SP:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_38ACP:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_380:			return 120.0;
	case ETBBulletCaliber::Bullet_357:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_357SIG:
		return DEFAULT_RANGE;
	case ETBBulletCaliber::Bullet_12Gauge00:	return 50.0;
	default:
		return DEFAULT_RANGE;
	}
}

void UBulletUtils::GetTypicalMuzzleVelocityAndEffectiveRangeForBullet(const ETBBulletCaliber Caliber, double& MuzzleVelocity, double& EffectiveRange)
{
	MuzzleVelocity = GetTypicalMuzzleVelocityForBullet(Caliber);
	EffectiveRange = GetTypicalEffectiveRangeForBullet(Caliber);
}
