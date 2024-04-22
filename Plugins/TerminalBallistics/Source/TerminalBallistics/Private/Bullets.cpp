// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "Bullets.h"
#include "BulletDataAsset.h"
#include "Engine/NetSerialization.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "NiagaraSystem.h"
#include "PhysicsUtils.h"
#include "PhysMatManager/PhysMatManager.h"
#include "TerminalBallisticsMacrosAndFunctions.h"


namespace TB::Bullets
{
	constexpr double InToCm(const double Inches)
	{
		return Inches * 2.54;
	}

	constexpr double GrainsToKilograms(const double Grains)
	{
		return 6.479891E-5 * Grains;
	}

	constexpr double GramsToKilograms(const double Grams)
	{
		return Grams / 1000.0;
	}

	namespace BulletNames
	{
		const FName BulletName9mm =					FName("9x19mm");
		const FName BulletName556x45 =				FName("5.56x45 NATO");
		const FName BulletName556x45_M855 =			FName("5.56x45 M855 Ball");
		const FName BulletName556x45_M856 =			FName("5.56x45 NATO Tracer (Red)");
		const FName BulletName762x51 =				FName("7.62x51 NATO");
		const FName BulletName762x51LR =			FName("7.62x51 NATO LR");
		const FName BulletName762x54 =				FName("7.62x54mmr");
		const FName BulletName762x39 =				FName("7.62x39 NATO");
		const FName BulletName545x39 =				FName("5.45x39");
		const FName BulletName545x39AP =			FName("5.45x39 AP");
		const FName BulletName50 =					FName(".50 BMG");
		const FName BulletName308 =					FName(".308");
		const FName BulletName308FMJ =				FName(".308 FMJ");
		const FName BulletName338 =					FName(".338");
		const FName BulletName46x30 =				FName("4.6x30mm");
		const FName BulletName57x28 =				FName("5.7x28");
		const FName BulletName22 =					FName(".22LR");
		const FName BulletName65 =					FName("6.5mm Creedmoor");
		const FName BulletName9x18 =				FName("9x18mm");
		const FName BulletName300Blackout =			FName(".300 AAC Blackout");
		const FName BulletName300BlackoutSubsonic =	FName(".300 AAC Blackout (Subsonic)");
		const FName BulletName300Savage =			FName(".300 Savage");
		const FName BulletName300Whisper =			FName(".300 Whisper");
		const FName BulletName300WinMag =			FName(".300 Winchester Magnum");
		const FName BulletName380ACP =				FName(".380 ACP");
		const FName BulletName408CheyTac =			FName(".408 Cheyenne Tactical");
		const FName BulletName416Barrett =			FName(".416 Barrett");
		const FName BulletName45ACP =				FName(".45 ACP");
		const FName BulletName45WinMag =			FName(".45 Winchester Magnum");
		const FName BulletName50Beowulf =			FName(".50 Beowulf");
		const FName BulletName127x108 =				FName("12.7x108mm");
		const FName BulletName145x114 =				FName("14.5x114mm");

		const FName BulletName12Gauge00Buckshot = FName("12 Gauge 00 Buckshot");
	};

	namespace BulletOgives
	{
		const double Radius300BLK = InToCm(0.308 / 2.0);
		constexpr auto x = 0.75919613 / 10000;

		/*											Radius			OgiveRadius		Length		CSA							Radius			Length	*/
		const FTBOgive Ogive_9x19PARA =		FTBOgive(0.45,							0.45,		0.000065246,	FTBCylinder(0.45,			0.5583));
		const FTBOgive Ogive_556x45NATO =	FTBOgive(0.285,			5.781,			1.2,		0.0000439,		FTBCylinder(0.285,			1.14));
		const FTBOgive Ogive_762x51NATO =	FTBOgive(0.39116,		3.83375,		1.6256,		0.00008715,		FTBCylinder(0.39116,		1.3811));
		const FTBOgive Ogive_762x51NATOLR =	FTBOgive(0.39116,		3.83375,		1.6256,		0.00008715,		FTBCylinder(0.39116,		1.3811));
		const FTBOgive Ogive_762x39 =		FTBOgive(0.391,			3.1225,			1.4733,		0.00008107,		FTBCylinder(0.391,			1.2962));
		const FTBOgive Ogive_762x54 =		FTBOgive(0.395,			7.9714,			2.3622,		0.00013068,		FTBCylinder(0.3949,			0.8636));
		const FTBOgive Ogive_50BMG =		FTBOgive(0.6477,		10.5758,		3.3641,		0.000313549,	FTBCylinder(0.6,			0.2825));
		const FTBOgive Ogive_545x39=		FTBOgive(0.2436,		2.4296,			0.7975,		0.00002686,		FTBCylinder(0.2515,			1.529));
		const FTBOgive Ogive_545x39AP =		FTBOgive(0.2436,		2.4296,			0.7975,		0.00002686,		FTBCylinder(0.2515,			1.529));
		const FTBOgive Ogive_57x28 =		FTBOgive(0.285,			3.977,			1.0,		0.0000255,		FTBCylinder(0.285,			1.053));
		const FTBOgive Ogive_300Blackout =	FTBOgive(Radius300BLK,	7.5,			1.5778,		7.676e-5,		FTBCylinder(Radius300BLK,	0.847));
		const FTBOgive Ogive_308 =			FTBOgive(0.3899,		5.0367,			1.7221,		0.00009987,		FTBCylinder(0.3899,			1.152));
		const FTBOgive Ogive_308FMJ =		FTBOgive(0.3899,		5.0367,			1.7221,		0.00009987,		FTBCylinder(0.3899,			1.152));
		const FTBOgive Ogive_338 =			FTBOgive(0.4305,		10.215,			2.3126,		7.043e-5*2,		FTBCylinder(0.4305,			2.426));
		const FTBOgive Ogive_46x30 =		FTBOgive(0.2124,		2.3571,			0.9356,		0.00003141,		FTBCylinder(0.2325,			0.59158));
		const FTBOgive Ogive_9x18 =			FTBOgive(0.464,							0.749,		2.728e-5*2,		FTBCylinder(0.464,			0.619));
		const FTBOgive Ogive_45ACP =		FTBOgive(0.533,							0.907,		7.5919613e-5,	FTBCylinder(0.533,			0.82320973));
		const FTBOgive Ogive_380ACP =		FTBOgive(0.452755,						1.186162,	4.464e-05,		FTBCylinder(0.452755,		0.558475));
	};

	namespace Buckshot
	{
		struct Buckshot00
		{
			static constexpr double Mass = GrainsToKilograms(53);
			static constexpr double Diameter = InToCm(0.33);
			static constexpr int32 Count_2_75in = 8;
			static constexpr int32 Count_3in = 15;
			inline static const FTBSpreadParams SpreadParams = FTBSpreadParams();
		};
	};

	namespace BulletPhysicalProperties
	{
		constexpr double CalculateBulletDensity(const double OverallMass, const double PenetratorMass, const double BulletDensity, const double PenetratorDensity)
		{
			const double PenetratorContribution = PenetratorMass / OverallMass;
			const double RemainingContribution = 1 - PenetratorContribution;
			return (PenetratorDensity * PenetratorContribution) + (BulletDensity * RemainingContribution);
		}

		constexpr double DensityAluminum = 2.7;
		constexpr double DensityCopper = 7.5;
		constexpr double DensityLead = 11.35;
		constexpr double DensitySteel = 7.8;
		constexpr double DensityTungstenCarbide = 15;

		constexpr double DensityJacket = (DensityCopper + DensityLead) / 2.0;

		using namespace TB::Drag::DragModels;

		constexpr double MassM855A1 = GrainsToKilograms(62);
		constexpr double MassM856 = GrainsToKilograms(63.7);

		constexpr double Mass7N6 = GrainsToKilograms(53);
		constexpr double Mass7N6_Penetrator = GrainsToKilograms(1.43);
		constexpr double Density7N6 = CalculateBulletDensity(Mass7N6, Mass7N6_Penetrator, (DensityCopper + DensityLead) / 2, DensitySteel);

		constexpr double Mass7N39 = GrainsToKilograms(63.3);
		constexpr double Mass7N39_Penetrator = GrainsToKilograms(29.3);
		constexpr double Density7N39 = CalculateBulletDensity(Mass7N39, Mass7N39_Penetrator, (DensityCopper + DensityLead) / 2, DensityTungstenCarbide);

		constexpr double Mass300 = GrainsToKilograms(110.0);
		constexpr double Mass300Sub = GrainsToKilograms(220.0);
		constexpr double Density300 = Mass300 * 952.872;

		constexpr double Mass338 = GrainsToKilograms(285.0);

		constexpr double Mass9x18 = GrainsToKilograms(92.6);
		constexpr double Density9x18 = (2.0 / 3.0 * DensitySteel) + (1.0 / 3.0 * DensityJacket); // 2/3 Steel core with copper/lead jacket

		constexpr double MassSS198LF = GrainsToKilograms(27);
		constexpr double DensitySS198LF = (1.0 / 3.0 * DensityAluminum) + (2.0 / 3.0 * DensityJacket);

		constexpr double Mass45ACP = GrainsToKilograms(230.0);
		constexpr double Mass45ACP_Slug = GramsToKilograms(11.06);
		constexpr double Mass45ACP_Jacket = Mass45ACP - Mass45ACP_Slug;
		constexpr double Density45ACP = (Mass45ACP_Slug / Mass45ACP) * DensityLead + DensityJacket * (Mass45ACP_Jacket / Mass45ACP);

		constexpr double Mass380ACP = GrainsToKilograms(95.0);


		using BulletOgives::Radius300BLK;

		/*																					Mass		Length		Density			Radius			CSA				AR			AL			AIA			ARIA	BC		SD		*/
		const FTBBulletPhysicalProperties BULLET_9x19PARA =		FTBBulletPhysicalProperties(0.00745,	1.5,		8.675,			0.45,			0.0001154894,	0.325943,	0.3132,		0.333759,	15.5,	0.166,	0.130, G1, BulletOgives::Ogive_9x19PARA, 2.0, 1.5); // 115gr FMJ
		const FTBBulletPhysicalProperties BULLET_556x45NATO =	FTBBulletPhysicalProperties(MassM855A1,	2.34,		8.859,			0.285,			0.0001102,		0.181950,	0.6,		0.104005,	11.7,	0.152,	0.184, G7, BulletOgives::Ogive_556x45NATO, 1.25); // M855A1
		const FTBBulletPhysicalProperties BULLET_762x51NATO =	FTBBulletPhysicalProperties(0.009525,	3.13,		9.309,			0.3912,			0.0001194544,	0.234800,	0.5419,		0.173199,	12.24,	0.209,	0.231, G7, BulletOgives::Ogive_762x51NATO, 1.1); // M80 Ball
		const FTBBulletPhysicalProperties BULLET_762x51NATOLR =	FTBBulletPhysicalProperties(0.011339,	3.13,		10.2401,		0.3912,			0.0001194544,	0.234800,	0.5419,		0.173199,	12.0,	0.250,	0.263, G7, BulletOgives::Ogive_762x51NATOLR, 1.6, 3.0); // M118LR BTHP FMJ
		const FTBBulletPhysicalProperties BULLET_762x39 =		FTBBulletPhysicalProperties(0.0079,		2.728,		7.73867,		0.391,			0.000181841,	0.251,		0.477,		0.19856,	13.2,	0.152,	0.184, G7, BulletOgives::Ogive_762x39); // 57-N-231
		const FTBBulletPhysicalProperties BULLET_762x54 =		FTBBulletPhysicalProperties(0.0096,		3.2258,		11.462542,		0.395,			0.000194193,	0.237850,	0.7874,		0.177728,	8.52,	0.187,	0.266, G7, BulletOgives::Ogive_762x54);	// 57-N-323S
		const FTBBulletPhysicalProperties BULLET_50BMG =		FTBBulletPhysicalProperties(0.049,		6.096,		8.61512,		0.635,			0.000641328,	0.4071,		1.1216,		1.93212,	9.5,	0.535,	0.412, G7, BulletOgives::Ogive_50BMG); // Hornady A-MAX UHC 5165
		const FTBBulletPhysicalProperties BULLET_545x39 =		FTBBulletPhysicalProperties(Mass7N6,	2.327,		Density7N6,		0.28,			0.000108539,	0.2207,		0.5976,		0.15298,	16.2,	0.176,	0.164, G7, BulletOgives::Ogive_545x39); // 7N10
		const FTBBulletPhysicalProperties BULLET_545x39AP =		FTBBulletPhysicalProperties(Mass7N39,	2.327,		Density7N39,	0.28,			0.000108539,	0.2207,		0.5976,		0.15298,	16.2,	0.185,	0.172, G7, BulletOgives::Ogive_545x39AP, 1.0, 5.0); // 7N39
		const FTBBulletPhysicalProperties BULLET_57x28 =		FTBBulletPhysicalProperties(MassSS198LF,2.05,		DensitySS198LF, 0.285,			0.0,			0.208,		0.501,		0.136,		12.3,	0.120,	0.306, G7, BulletOgives::Ogive_57x28); // SS198LF
		const FTBBulletPhysicalProperties BULLET_300Blackout =	FTBBulletPhysicalProperties(Mass300,	2.425,		Density300,		Radius300BLK,	0.00014298,		0.19445,	0.526,		0.1188,		14.7,	0.150,	0.166, G7, BulletOgives::Ogive_300Blackout); // 110gr V-MAX
		const FTBBulletPhysicalProperties BULLET_300Subsonic =	FTBBulletPhysicalProperties(Mass300Sub,	2.425,		Density300,		Radius300BLK,	0.00014298,		0.19445,	0.526,		0.1188,		14.7,	0.304,	0.331, G7, BulletOgives::Ogive_300Blackout); // 220gr Federal Subsonic
		const FTBBulletPhysicalProperties BULLET_308 =			FTBBulletPhysicalProperties(0.008,		2.5,		10,				0.3899,			0.000186294,	0.2573,		0.574,		0.253965,	9.8,	0.250,	0.188, G7, BulletOgives::Ogive_308); // 125gr Spitzer
		const FTBBulletPhysicalProperties BULLET_308FMJ =		FTBBulletPhysicalProperties(0.01133,	2.64,		13.7,			0.3899,			0.000186294,	0.2573,		0.574,		0.253965,	9.8,	0.223,	0.264, G7, BulletOgives::Ogive_308FMJ, 1.2, 3.25); // 175gr BTHP FMJ
		const FTBBulletPhysicalProperties BULLET_338 =			FTBBulletPhysicalProperties(Mass338,	4.739,		8.5,			0.4305,			0,				0.3764,		1.5417,		0.445091,	8.4,	0.394,	0.356, G7, BulletOgives::Ogive_338, 1.0, 2.5); // Hornady .338 285gr ELD Match
		const FTBBulletPhysicalProperties BULLET_46x30 =		FTBBulletPhysicalProperties(0.002,		1.5,		9.5,			0.23,			0.000058,		0.1485,		0.3119,		0.069279,	10.9,	0.146,	0.131, G7, BulletOgives::Ogive_46x30, 2.0, 5.0); // 2g Ultimate Combat
		const FTBBulletPhysicalProperties BULLET_9x18 =			FTBBulletPhysicalProperties(Mass9x18,	1.368,		Density9x18,	0.464,			0,				0.401,		0.375,		0.441786,	19.6,	0.135,	0.102, G1, BulletOgives::Ogive_9x18); // 57-N-181S
		const FTBBulletPhysicalProperties BULLET_45ACP =		FTBBulletPhysicalProperties(Mass45ACP,	1.73,		Density45ACP,	0.533,			0.00016309,		0.397267,	0.302,		0.495811,	18.7,	0.162,	0.161, G7, BulletOgives::Ogive_45ACP); // Winchester 230gr FMJ
		const FTBBulletPhysicalProperties BULLET_380ACP =		FTBBulletPhysicalProperties(Mass380ACP,	1.186,		DensityJacket,	0.452755,		9.52103e-5,		0.337453,	0.209,		0.357747,	22.6,	0.15,	0.107, G1, BulletOgives::Ogive_380ACP); // Federal 95gr FMJ

		// 5.56 Variations
		const FTBBulletPhysicalProperties BULLET_556x45_M855 =	FTBBulletPhysicalProperties(0.004018,	2.34,		7.83,			0.285,			0.0001102,		0.181950,	0.6,		0.104005,	11.7,	0.151,	0.177, G7, BulletOgives::Ogive_556x45NATO, 1.25); // M855/SS109
		const FTBBulletPhysicalProperties BULLET_556x45_M856 =	FTBBulletPhysicalProperties(MassM856,	2.34,		8.81,			0.285,			0.0001102,		0.181950,	0.6,		0.104005,	11.7,	0.151,	0.166, G7, BulletOgives::Ogive_556x45NATO, 1.25); // M856 (orange tip)

		using namespace Buckshot;
		const FTBBulletPhysicalProperties BULLET_12Gauge00Buck = FTBBulletPhysicalProperties(Buckshot00::Mass, DensityLead, Buckshot00::Diameter / 2.0, 1.5, 1.0);
	};

	void BulletTypes::Init()
	{
		using namespace BulletNames;
		using namespace BulletPhysicalProperties;
		const FTBPresetMaterialProperties Presets = UTBPhysMatManager::GetPropertiesForPresets();
		const FPhysMatProperties Lead = Presets.Lead.Get(TB::PhysMatConstants::DefaultPhysMats::Lead());
		const FPhysMatProperties HardenedSteel = Presets.HardenedSteel.Get(TB::PhysMatConstants::DefaultPhysMats::HardenedSteel());
		const FPhysMatProperties Steel = Presets.Steel.Get(TB::PhysMatConstants::DefaultPhysMats::Steel());

		StaticBulletTypes.Bullet_9x19Para =		FTBBullet(BULLET_9x19PARA, Lead, BulletName9mm, ETBBulletCaliber::Bullet_9mm, PS_Small, 350);
		StaticBulletTypes.Bullet_556x45NATO =	FTBBullet(BULLET_556x45NATO, Lead, BulletName556x45, ETBBulletCaliber::Bullet_556x45, PS_Small, 961, FMJ_BulletVariation);
		StaticBulletTypes.Bullet_556x45_M855 =	FTBBullet(BULLET_556x45_M855, Lead, BulletName556x45_M855, ETBBulletCaliber::Bullet_556x45, PS_Small, 948, FMJ_BulletVariation);
		StaticBulletTypes.Bullet_556x45_M856 =	FTBBullet(BULLET_556x45_M856, Lead, BulletName556x45_M856, ETBBulletCaliber::Bullet_556x45, PS_Small, 951, { FMJ_BulletVariation, Tracer_BulletVariation });
		StaticBulletTypes.Bullet_762x51NATO =	FTBBullet(BULLET_762x51NATO, Lead, BulletName762x51, ETBBulletCaliber::Bullet_762x51, PS_Medium, 850);
		StaticBulletTypes.Bullet_762x51NATOLR = FTBBullet(BULLET_762x51NATOLR, Lead, BulletName762x51LR, ETBBulletCaliber::Bullet_762x51, PS_Medium, 790, { FMJ_BulletVariation, HollowPoint_BulletVariation });
		StaticBulletTypes.Bullet_762x39 =		FTBBullet(BULLET_762x39, Lead, BulletName762x39, ETBBulletCaliber::Bullet_762x39, PS_Medium, 790.3, FMJ_BulletVariation);
		StaticBulletTypes.Bullet_762x54 =		FTBBullet(BULLET_762x54, Lead, BulletName762x54, ETBBulletCaliber::Bullet_762x54, PS_Medium, 828, FMJ_BulletVariation);
		StaticBulletTypes.Bullet_50BMG =		FTBBullet(BULLET_50BMG, Lead, BulletName50, ETBBulletCaliber::Bullet_50, PS_Large, 860);
		StaticBulletTypes.Bullet_545x39 =		FTBBullet(BULLET_545x39, Lead, BulletName545x39, ETBBulletCaliber::Bullet_545x39, PS_Small, 880, FMJ_BulletVariation);
		StaticBulletTypes.Bullet_545x39AP =		FTBBullet(BULLET_545x39AP, HardenedSteel, BulletName545x39AP, ETBBulletCaliber::Bullet_545x39, PS_Small, 850, { FMJ_BulletVariation, AP_BulletVariation });
		StaticBulletTypes.Bullet_308 =			FTBBullet(BULLET_308, Lead, BulletName308, ETBBulletCaliber::Bullet_308, PS_Medium, 940);
		StaticBulletTypes.Bullet_308FMJ =		FTBBullet(BULLET_308FMJ, Lead, BulletName308FMJ, ETBBulletCaliber::Bullet_308, PS_Medium, 806, { FMJ_BulletVariation, HollowPoint_BulletVariation });
		StaticBulletTypes.Bullet_338 =			FTBBullet(BULLET_338, Lead, BulletName338, ETBBulletCaliber::Bullet_338, PS_Medium, 863);
		StaticBulletTypes.Bullet_46x30 =		FTBBullet(BULLET_46x30, HardenedSteel, BulletName46x30, ETBBulletCaliber::Bullet_46x30, PS_Small, 720, AP_BulletVariation);
		StaticBulletTypes.Bullet_57x28 =		FTBBullet(BULLET_57x28, Lead, BulletName57x28, ETBBulletCaliber::Bullet_57x28, PS_Small, 792, { FMJ_BulletVariation, HollowPoint_BulletVariation });
		StaticBulletTypes.Bullet_22LR =			FTBBullet();
		StaticBulletTypes.Bullet_65 =			FTBBullet();
		StaticBulletTypes.Bullet_9x18 =			FTBBullet(BULLET_9x18, Steel, BulletName9x18, ETBBulletCaliber::Bullet_9x18, PS_Small, 298, FMJ_BulletVariation);
		StaticBulletTypes.Bullet_300AAC =		FTBBullet(BULLET_300Blackout, Lead, BulletName300Blackout, ETBBulletCaliber::Bullet_300AAC, PS_Small, 724, FMJ_BulletVariation);
		StaticBulletTypes.Bullet_300AACSub =	FTBBullet(BULLET_300Subsonic, Lead, BulletName300BlackoutSubsonic, ETBBulletCaliber::Bullet_300AAC, PS_Small, 320.4, { FMJ_BulletVariation, Subsoncic_BulletVariation });
		StaticBulletTypes.Bullet_300SVG =		FTBBullet();
		StaticBulletTypes.Bullet_300WHSP =		FTBBullet();
		StaticBulletTypes.Bullet_300WinMag =	FTBBullet();
		StaticBulletTypes.Bullet_380ACP =		FTBBullet(BULLET_380ACP, Lead, BulletName380ACP, ETBBulletCaliber::Bullet_380, PS_Small, 291, FMJ_BulletVariation);
		StaticBulletTypes.Bullet_408 =			FTBBullet();
		StaticBulletTypes.Bullet_416 =			FTBBullet();
		StaticBulletTypes.Bullet_45ACP =		FTBBullet(BULLET_45ACP, Lead, BulletName45ACP, ETBBulletCaliber::Bullet_45ACP, PS_Large, 263.5, FMJ_BulletVariation);
		StaticBulletTypes.Bullet_45WinMag =		FTBBullet();
		StaticBulletTypes.Bullet_50Beowulf =	FTBBullet();
		StaticBulletTypes.Bullet_127x108 =		FTBBullet();
		StaticBulletTypes.Bullet_145x114 =		FTBBullet();

		StaticBulletTypes.Bullet_12Gauge00Buckshot = FTBBullet(BULLET_12Gauge00Buck, Lead, BulletName12Gauge00Buckshot, ETBBulletCaliber::Bullet_12Gauge00, PS_Medium, 373.38, ETBBulletVariation::BUCKSHOT, Buckshot00::Count_3in, Buckshot00::SpreadParams);

		StaticBulletTypes.BulletMappings =
		{
			{ ETBBulletNames::Name_9mm,				StaticBulletTypes.Bullet_9x19Para },
			{ ETBBulletNames::Name_556x45,			StaticBulletTypes.Bullet_556x45NATO },
			{ ETBBulletNames::Name_556x45_M855,		StaticBulletTypes.Bullet_556x45_M855 },
			{ ETBBulletNames::Name_556x45_M856,		StaticBulletTypes.Bullet_556x45_M856 },
			{ ETBBulletNames::Name_762x51,			StaticBulletTypes.Bullet_762x51NATO },
			{ ETBBulletNames::Name_762x51LR,		StaticBulletTypes.Bullet_762x51NATOLR },
			{ ETBBulletNames::Name_762x39,			StaticBulletTypes.Bullet_762x39 },
			{ ETBBulletNames::Name_762x54,			StaticBulletTypes.Bullet_762x54 },
			{ ETBBulletNames::Name_50,				StaticBulletTypes.Bullet_50BMG },
			{ ETBBulletNames::Name_545x39,			StaticBulletTypes.Bullet_545x39 },
			{ ETBBulletNames::Name_545x39AP,		StaticBulletTypes.Bullet_545x39AP },
			{ ETBBulletNames::Name_308,				StaticBulletTypes.Bullet_308 },
			{ ETBBulletNames::Name_308FMJ,			StaticBulletTypes.Bullet_308FMJ },
			{ ETBBulletNames::Name_338,				StaticBulletTypes.Bullet_338 },
			{ ETBBulletNames::Name_46x30,			StaticBulletTypes.Bullet_46x30 },
			{ ETBBulletNames::Name_57x28,			StaticBulletTypes.Bullet_57x28 },
			{ ETBBulletNames::Name_22LR,			StaticBulletTypes.Bullet_22LR },
			{ ETBBulletNames::Name_65,				StaticBulletTypes.Bullet_65 },
			{ ETBBulletNames::Name_9x18,			StaticBulletTypes.Bullet_9x18 },
			{ ETBBulletNames::Name_300AAC,			StaticBulletTypes.Bullet_300AAC },
			{ ETBBulletNames::Name_300SVG,			StaticBulletTypes.Bullet_300SVG },
			{ ETBBulletNames::Name_300WHSP,			StaticBulletTypes.Bullet_300WHSP },
			{ ETBBulletNames::Name_300WINMAG,		StaticBulletTypes.Bullet_300WinMag },
			{ ETBBulletNames::Name_380ACP,			StaticBulletTypes.Bullet_380ACP },
			{ ETBBulletNames::Name_408,				StaticBulletTypes.Bullet_408 },
			{ ETBBulletNames::Name_416,				StaticBulletTypes.Bullet_416 },
			{ ETBBulletNames::Name_45ACP,			StaticBulletTypes.Bullet_45ACP },
			{ ETBBulletNames::Name_45WINMAG,		StaticBulletTypes.Bullet_45WinMag },
			{ ETBBulletNames::Name_50BEOWULF,		StaticBulletTypes.Bullet_50Beowulf },
			{ ETBBulletNames::Name_127x108,			StaticBulletTypes.Bullet_127x108 },
			{ ETBBulletNames::Name_145x114,			StaticBulletTypes.Bullet_145x114 },
			{ ETBBulletNames::Name_300AAC_Subsonic,	StaticBulletTypes.Bullet_300AACSub },

			{ ETBBulletNames::Name_12Gauge00Buck,	StaticBulletTypes.Bullet_12Gauge00Buckshot }
		};
	}

	BulletTypes BulletTypes::StaticBulletTypes = BulletTypes();

};

FTBBullet FTBBullet::DefaultBullet = FTBBullet(); // Temporary. The real value is set during the engine initialization by TBEngineSubsystem.

FTBBullet::FTBBullet(TSoftClassPtr<UBulletDataAsset> DataAsset)
	: FTBBullet(CastChecked<UBulletDataAsset>(DataAsset.LoadSynchronous()->GetDefaultObject()))
{}

FTBBullet::FTBBullet(const UBulletDataAsset* DataAsset)
	: BulletProperties(DataAsset->BulletProperties)
	, PhysicalProperties(DataAsset->BulletPhysMatProperties)
	, BulletName(DataAsset->Name)
	, BulletType(DataAsset->BulletType)
	, ProjectileSize(DataAsset->ProjectileSize)
	, BulletVariation(DataAsset->BulletVariation)
	, MuzzleVelocity(DataAsset->MuzzleVelocity)
	, ProjectileCount(DataAsset->ProjectileCount)
	, SpreadParams(DataAsset->SpreadParams)
	, Icon(DataAsset->Icon)
	, Mesh(DataAsset->Mesh)
	, BulletMesh(DataAsset->BulletMesh)
	, CartridgeMesh(DataAsset->CartridgeMesh)
	, BulletCasing(DataAsset->BulletCasing)
	, TracerSystem(DataAsset->TracerSystem)
	, DefaultTracerSystem(DataAsset->DefaultTracerSystem)
{
#if WITH_EDITOR
	if (DataAsset->bUseExistingBullet)
	{
		if (const auto FoundBulletProperties = TB::Bullets::BulletTypes::Get().GetMappings().Find(DataAsset->PresetBullets))
		{
			BulletProperties = *FoundBulletProperties;
		}
	}
#endif
}

bool FTBBullet::CanUseTracer() const
{
	const bool TracerIsAvailable = TracerSystem.IsPending() || TracerSystem.IsValid();
	return TracerIsAvailable && BulletVariation.Contains(Tracer_BulletVariation);
}

bool FTBBullet::CanUseDefaultTracer() const
{
	const bool CanUseDefaultTracer = DefaultTracerSystem.IsPending() || DefaultTracerSystem.IsValid();
	return CanUseDefaultTracer;
}

UNiagaraSystem* FTBBullet::GetTracerSystem() const
{
	if (CanUseTracer())
	{
		return TracerSystem.LoadSynchronous();
	}
	else if (CanUseDefaultTracer())
	{
		return DefaultTracerSystem.LoadSynchronous();
	}
	return nullptr;
}

FTBBullet* FTBBullet::CreateFromBulletDataAsset(TSoftClassPtr<UBulletDataAsset> DataAsset)
{
	return CreateFromBulletDataAsset(CastChecked<UBulletDataAsset>(DataAsset.LoadSynchronous()->GetDefaultObject()));
}

FTBBullet* FTBBullet::CreateFromBulletDataAsset(const UBulletDataAsset* DataAsset)
{
	if (TB_VALID(DataAsset) && DataAsset->IsValidBullet())
	{
		return new FTBBullet(DataAsset);
	}
	else
	{
		return FTBBullet::GetDefaultBullet();
	}
}

TSharedPtr<FTBBullet> FTBBullet::CreatePtrFromBulletDataAsset(TSoftClassPtr<UBulletDataAsset> DataAsset)
{
	return CreatePtrFromBulletDataAsset(CastChecked<UBulletDataAsset>(DataAsset.LoadSynchronous()->GetDefaultObject()));
}

TSharedPtr<FTBBullet> FTBBullet::CreatePtrFromBulletDataAsset(const UBulletDataAsset* DataAsset)
{
	if (TB_VALID(DataAsset) && DataAsset->IsValidBullet())
	{
		return MakeShared<FTBBullet>(DataAsset);
	}
	else
	{
		return GetDefaultBulletPtr();
	}
}

FTBBullet* FTBBullet::GetDefaultBullet()
{
	return new FTBBullet(DefaultBullet);
}

TSharedPtr<FTBBullet> FTBBullet::GetDefaultBulletPtr()
{
	return MakeShared<FTBBullet>(DefaultBullet);
}

bool FTBBullet::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << BulletName;
	Ar << BulletProperties;
	Ar << BulletType;
	Ar << ProjectileSize;

	bOutSuccess = true;
	bOutSuccess &= SafeNetSerializeTArray_Default<16>(Ar, BulletVariation);

	Ar << MuzzleVelocity;

	bool localSuccess = true;
	PhysicalProperties.NetSerialize(Ar, Map, localSuccess);
	bOutSuccess &= localSuccess;

	Ar << ProjectileCount;
	TB::SerializeValueConditional(Ar.IsSaving(), Ar, SpreadParams, FTBSpreadParams(), ProjectileCount > 1);

	SerializeOptionalValue(Ar.IsSaving(), Ar, Icon, (TSoftObjectPtr<UTexture2D>)nullptr);
	SerializeOptionalValue(Ar.IsSaving(), Ar, Mesh, (TSoftObjectPtr<UStaticMesh>)nullptr);
	SerializeOptionalValue(Ar.IsSaving(), Ar, BulletMesh, (TSoftObjectPtr<UStaticMesh>)nullptr);
	SerializeOptionalValue(Ar.IsSaving(), Ar, CartridgeMesh, (TSoftObjectPtr<UStaticMesh>)nullptr);
	SerializeOptionalValue(Ar.IsSaving(), Ar, BulletCasing, (decltype(BulletCasing))nullptr);
	SerializeOptionalValue(Ar.IsSaving(), Ar, BulletSystem, (decltype(BulletSystem))nullptr);
	SerializeOptionalValue(Ar.IsSaving(), Ar, TracerSystem, (decltype(TracerSystem))nullptr);
	SerializeOptionalValue(Ar.IsSaving(), Ar, DefaultTracerSystem, (decltype(DefaultTracerSystem))nullptr);

	return bOutSuccess;
}
