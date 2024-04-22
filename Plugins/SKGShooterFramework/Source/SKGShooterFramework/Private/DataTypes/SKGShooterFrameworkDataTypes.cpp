// Copyright 2023, Dakota Dawe, All rights reserved

#include "DataTypes/SKGShooterFrameworkDataTypes.h"
#include "Components/SKGFirearmAttachmentStatComponent.h"
#include "DataAssets/SKGPDAFirearmAttachmentStats.h"
#include "DataAssets/SKGPDAFirearmStats.h"

#include "Kismet/KismetMathLibrary.h"

void FSKGFirearmStats::SetFirearmDefaultStats(const USKGPDAFirearmStats* FirearmStats)
{
	Weight = FirearmStats->Weight;
	Durability = FirearmStats->Durability;
	Ergonomics = FirearmStats->Ergonomics;
	Accuracy = FirearmStats->Accuracy;
	VerticalRecoil = FirearmStats->VerticalRecoil;
	HorizontalRecoil = FirearmStats->HorizontalRecoil;
	FireRate = FirearmStats->FireRate;
}

FString FSKGFirearmStats::ToString() const
{
	return FString::Printf(TEXT("Weight=%3.3f Durability=%3.3f Ergonomics=%3.3f Accuracy=%3.3f Vertical Recoil=%3.3f Horizontal Recoil=%3.3f Muzzle Velocity Percentage=%3.3f Fire Rate Percentage=%3.3f"), Weight, Durability, Ergonomics, Accuracy, VerticalRecoil, HorizontalRecoil, MuzzleVelocityPercentage, FireRate);
}

FSKGFirearmStats& FSKGFirearmStats::operator+=(const USKGFirearmAttachmentStatComponent* StatComponent)
{
	if (StatComponent && StatComponent->GetAttachmentStats())
	{
		Weight += StatComponent->GetAttachmentStats()->Weight;
		Ergonomics += StatComponent->GetAttachmentStats()->Ergonomics;
		Accuracy += StatComponent->GetAttachmentStats()->Accuracy;
		
		const float VerticalRecoilMultiplier = 1.0f - (-StatComponent->GetAttachmentStats()->VerticalRecoil / 100.0f);
		const float HorizontalRecoilMultiplier = 1.0f - (-StatComponent->GetAttachmentStats()->HorizontalRecoil / 100.0f);
		const float MuzzleVelocityMultiplier = 1.0f - (-StatComponent->GetAttachmentStats()->MuzzleVelocityPercentage / 100.0f);
		const float FireRateMultiplier = 1.0f - (-StatComponent->GetAttachmentStats()->FireRateChangePercent / 100.0f);

		if (VerticalRecoilMultiplier != 0.0f)
		{
			VerticalRecoil *= VerticalRecoilMultiplier;
		}
		if (HorizontalRecoilMultiplier != 0.0f)
		{
			HorizontalRecoil *= HorizontalRecoilMultiplier;
		}
		if (MuzzleVelocityMultiplier != 0.0f)
		{
			MuzzleVelocityPercentage *= MuzzleVelocityMultiplier;
		}
		if (FireRateMultiplier != 0.0f)
		{
			FireRate *= FireRateMultiplier;
		}
	}
	return *this;
}

FSKGFirearmStats& FSKGFirearmStats::operator-=(const USKGFirearmAttachmentStatComponent* StatComponent)
{
	if (StatComponent && StatComponent->GetAttachmentStats())
	{
		Weight -= StatComponent->GetAttachmentStats()->Weight;
		Ergonomics -= StatComponent->GetAttachmentStats()->Ergonomics;
		Accuracy -= StatComponent->GetAttachmentStats()->Accuracy;
		
		const float VerticalRecoilMultiplier = 1.0f - (-StatComponent->GetAttachmentStats()->VerticalRecoil / 100.0f);
		const float HorizontalRecoilMultiplier = 1.0f - (-StatComponent->GetAttachmentStats()->HorizontalRecoil / 100.0f);
		const float MuzzleVelocityMultiplier = 1.0f - (-StatComponent->GetAttachmentStats()->MuzzleVelocityPercentage / 100.0f);
		const float FireRateMultiplier = 1.0f - (-StatComponent->GetAttachmentStats()->FireRateChangePercent / 100.0f);

		if (VerticalRecoilMultiplier != 0.0f)
		{
			VerticalRecoil /= VerticalRecoilMultiplier;
		}
		if (HorizontalRecoilMultiplier != 0.0f)
		{
			HorizontalRecoil /= HorizontalRecoilMultiplier;
		}
		if (MuzzleVelocityMultiplier != 0.0f)
		{
			MuzzleVelocityPercentage /= MuzzleVelocityMultiplier;
		}
		if (FireRateMultiplier != 0.0f)
		{
			FireRate /= FireRateMultiplier;
		}
	}
	return *this;
}
