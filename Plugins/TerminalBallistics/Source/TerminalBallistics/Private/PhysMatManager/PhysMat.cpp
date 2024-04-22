// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "PhysMatManager/PhysMat.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Kismet/KismetNodeHelperLibrary.h"
#include "Materials/MaterialInterface.h"
#include "MathUtils.h"
#include "PhysicsUtils.h"
#include "PhysMatManager/PhysMatManager.h"
#include "Subsystems/TerminalBallisticsEngineSubsystem.h"
#include "TerminalBallisticsConfiguration.h"
#include "TerminalBallisticsLogChannels.h"
#include "TerminalBallisticsMacrosAndFunctions.h"
#include "TerminalBallisticsTags.h"



FArchive& operator<<(FArchive& Ar, FTBRicochetProperties& RicochetOverrides)
{
	TB_PACK_ARCHIVE_WITH_BITFIELDS_FOUR(Ar, RicochetOverrides.bAllowRicochets, RicochetOverrides.bLimitRicochetAngle, RicochetOverrides.bAddRicochetRandomness, RicochetOverrides.bProbabilityOverridesMaterialCalculations);
	Ar << RicochetOverrides.RicochetAngleCutoff;
	Ar << RicochetOverrides.RicochetProbability;
	return Ar;
}

#pragma region FPhysMatProperties
FPhysMatProperties FPhysMatProperties::DefaultPhysMat = FPhysMatProperties();

float FPhysMatProperties::EstimateCoeffientOfRestitution() const
{
	using namespace TB::UnitConversions;
	const float oy = bIsInBaseUnits ? GetYieldStrength() : MPaToPa(GetYieldStrength());
	const float E = bIsInBaseUnits ? YoungsModulus : GPaToPa(YoungsModulus);
	return fmin(sqrt(oy / E), 1); // Clamp to 1
}

double FPhysMatProperties::GetCriticalRicochetAngle() const
{
	double E = YoungsModulus;
	double oy = GetYieldStrength();
	double p = Density;
	if (!bIsInBaseUnits)
	{
		using namespace TB::UnitConversions;
		E = GPaToPa(YoungsModulus);
		oy = MPaToPa(GetYieldStrength());
		p *= 1000;
	}

	double CriticalAngle = FMath::RadiansToDegrees(acos(sqrt((E / oy) / (2 * p)))) * 0.85; // If the impact angle is above this, it is highly unlikely that it will ricochet
	if (bUseCustomRicochetProperties && RicochetProperties.bLimitRicochetAngle)
	{
		CriticalAngle = fmin(CriticalAngle, RicochetProperties.RicochetAngleCutoff);
	}
	return CriticalAngle;
}

bool FPhysMatProperties::CanEverHaveRicochet() const
{
	return !bUseCustomRicochetProperties
		|| (bUseCustomRicochetProperties
		&& RicochetProperties.bAllowRicochets
		&& (!RicochetProperties.bLimitRicochetAngle || RicochetProperties.RicochetAngleCutoff > 0.f)
		&& (!RicochetProperties.bAddRicochetRandomness || RicochetProperties.RicochetProbability > 0.f));
}

double FPhysMatProperties::EstimateMaxImpartedRicochetEnergy(const double ImpactArea, const double ObjectThickness) const
{
	float oy, oc, uts = 0.f;
	if (bIsInBaseUnits)
	{
		oy = GetYieldStrength();
		oc = CompressiveStrength;
		uts = UltimateTensileStrength;
	}
	else
	{
		using namespace TB::UnitConversions;
		oy = MPaToPa(GetYieldStrength());
		oc = MPaToPa(CompressiveStrength);
		uts = MPaToPa(UltimateTensileStrength);
	}
	const float StrengthSum = (oy + oc + uts);
	return ((StrengthSum / PoissonsRatio) * ImpactArea) * (ObjectThickness / 1e3);
	}

bool FPhysMatProperties::CanRicochet() const
	{
	if (bUseCustomRicochetProperties && RicochetProperties.bAddRicochetRandomness)
	{
		return RicochetProperties.RicochetProbability >= FMath::FRand();
	}
	else
	{
		return true;
	}
}

void FPhysMatProperties::ConvertToBaseUnits()
{
	if(!bIsInBaseUnits)
	{
		using namespace TB::UnitConversions;
		Density /= 1000.0;
		FractureToughness = MPaToPa(FractureToughness);
		UltimateTensileStrength = MPaToPa(UltimateTensileStrength);
		YieldStrength = MPaToPa(GetYieldStrength());
		CompressiveStrength = MPaToPa(CompressiveStrength);
		ShearStrength = MPaToPa(ShearStrength);
		YoungsModulus = GPaToPa(YoungsModulus);
		bIsInBaseUnits = true;
	}
}

void FPhysMatProperties::ConvertToDefaultUnits()
{
	if (bIsInBaseUnits)
	{
		using namespace TB::UnitConversions;
		Density *= 1000.0;
		FractureToughness = PaToMPa(FractureToughness);
		UltimateTensileStrength = PaToMPa(UltimateTensileStrength);
		YieldStrength = PaToMPa(GetYieldStrength());
		CompressiveStrength = PaToMPa(CompressiveStrength);
		ShearStrength = PaToMPa(ShearStrength);
		YoungsModulus = PaToGPa(YoungsModulus);
		bIsInBaseUnits = false;
	}
}

FPhysMatProperties FPhysMatProperties::InBaseUnits() const
{
	FPhysMatProperties PhysMat = *this;
	PhysMat.ConvertToBaseUnits();
	return PhysMat;
}

FPhysMatProperties FPhysMatProperties::InDefaultUnits() const
{
	FPhysMatProperties PhysMat = *this;
	PhysMat.ConvertToDefaultUnits();
	return PhysMat;
}

const FPhysMatProperties FPhysMatProperties::GetDefault()
{
	return DefaultPhysMat;
}

static const FString& PhysMatTagString = FString("PhysMat.");

const FPhysMatProperties FPhysMatProperties::GetFromName(const FName& Name)
{
	bool Success;
	return FPhysMatProperties::GetFromName(Name.ToString(), Success);
}
const FPhysMatProperties FPhysMatProperties::GetFromName(const FName& Name, bool& Success)
{
	return GetFromName(Name.ToString(), Success);
}

const FPhysMatProperties FPhysMatProperties::GetFromName(const FString& Name)
{
	bool Success;
	return FPhysMatProperties::GetFromName(Name, Success);
}
const FPhysMatProperties FPhysMatProperties::GetFromName(const FString& Name, bool& Success)
{
	FString FullTagName = Name;
	if (!Name.Contains(PhysMatTagString))
	{
		FullTagName = PhysMatTagString + Name;
	}

	const FGameplayTag& Tag = UGameplayTagsManager::Get().RequestGameplayTag(*FullTagName, false);
	if (Tag.IsValid())
	{
		return UTBPhysMatManager::Get().GetFromTag(Tag, Success);
	}
	else
	{
		Success = false;
		return DefaultPhysMat;
	}
}


const FPhysMatProperties FPhysMatProperties::GetFromTag(const FGameplayTag& Tag)
{
	bool Success;
	return UTBPhysMatManager::Get().GetFromTag(Tag, Success);
}
const FPhysMatProperties FPhysMatProperties::GetFromTag(const FGameplayTag& Tag, bool& Success)
{
	return UTBPhysMatManager::Get().GetFromTag(Tag, Success);
}

const FPhysMatProperties FPhysMatProperties::GetFromTagInterfaceMaterial(TWeakObjectPtr<UPhysicalMaterial> Material, bool& Success)
{
	using namespace TB::PhysMatConstants;

	if (!Material.IsValid())
	{
		Success = false;
		return FPhysMatProperties::DefaultPhysMat;
	}

	FGameplayTagContainer TagContainer;
	Cast<IGameplayTagAssetInterface>(Material.Get())->GetOwnedGameplayTags(TagContainer);
	if (TagContainer.HasTag(PhysMatTag()))
	{
		TArray<FGameplayTag> Tags;
		TagContainer.Filter(FGameplayTagContainer(PhysMatTag())).GetGameplayTagArray(Tags);
		if (Tags.Num() > 0)
		{
			FGameplayTag TagToSearchWith = Tags[0];
			if (Tags.Num() > 1)
			{
				TagToSearchWith = *Tags.FindByPredicate([](const FGameplayTag& Tag) { return Tag.GetGameplayTagParents().HasTag(PhysMatTag()); });
			}
			bool found = false;
			FPhysMatProperties PhysMat = UTBPhysMatManager::Get().GetFromTag(TagToSearchWith, found);
			if (found)
			{
				Success = true;
				return PhysMat;
			}
		}
	}
	Success = false;
	return DefaultPhysMat;
}

const FPhysMatProperties FPhysMatProperties::GetFromPhysMatWithTags(TObjectPtr<UTBPhysicalMaterialWithTags> Material, bool& Success)
{
	using namespace TB::PhysMatConstants;

	Success = false;

	if (!Material || !Material->IsValidLowLevel())
	{
		return DefaultPhysMat;
	}

	FPhysMatProperties properties = UTBPhysMatManager::Get().GetFromPhysMatWithTags(Material, Success);
	if (Success)
	{
		return properties;
	}
	else
	{
		return DefaultPhysMat;
	}
}

const bool FPhysMatProperties::GetFromPhysicalMaterial(TWeakObjectPtr<UPhysicalMaterial> Material, FPhysMatProperties& OutProperties)
{
	if (Material.IsValid())
	{
		bool Success = false;
		UPhysicalMaterial* MaterialPtr = Material.Get();
		if (TObjectPtr<UTBPhysicalMaterialWithTags> PhysMatWithTags = Cast<UTBPhysicalMaterialWithTags>(MaterialPtr))
		{
			OutProperties = FPhysMatProperties::GetFromPhysMatWithTags(PhysMatWithTags, Success);
		}
		else if (MaterialPtr->Implements<UGameplayTagAssetInterface>())
		{
			OutProperties = FPhysMatProperties::GetFromTagInterfaceMaterial(Material, Success);
		}
		if (!Success && MaterialPtr) // The tag seems to have been invalid, try checking against surface mappings
		{
			const UTBConfiguration* TBConfig = GetMutableDefault<UTBConfiguration>(); // GetMutableDefault really isn't needed here, but GetDefault doesn't seem to work...
			for (const auto& PhysMat : TBConfig->PhysMats)
			{
				if (PhysMat.HasPhysicalMaterialAsset() && PhysMat.PhysicalMaterial.Get() == MaterialPtr)
				{
					OutProperties = PhysMat.PhysMatProperties;
					return true;
				}
			}
		}
		if (Success)
		{
			return true;
		}
	}
	OutProperties = DefaultPhysMat;
	return false;
}

const FPhysMatProperties FPhysMatProperties::GetFromSurfaceType(const EPhysicalSurface SurfaceType)
{
	bool Success;
	const auto& ToReturn = UTBPhysMatManager::Get().GetFromSurfaceType(SurfaceType, Success);
	return Success ? ToReturn : DefaultPhysMat;
}
const FPhysMatProperties FPhysMatProperties::GetFromSurfaceType(const EPhysicalSurface SurfaceType, bool& Success)
{
	const auto& ToReturn = UTBPhysMatManager::Get().GetFromSurfaceType(SurfaceType, Success);
	return Success ? ToReturn : DefaultPhysMat;
}
const FPhysMatProperties FPhysMatProperties::GetFromSurfaceType(const TEnumAsByte<EPhysicalSurface> SurfaceType)
{
	return GetFromSurfaceType(SurfaceType.GetValue());
}
const FPhysMatProperties FPhysMatProperties::GetFromSurfaceType(const TEnumAsByte<EPhysicalSurface> SurfaceType, bool& Success)
{
	return GetFromSurfaceType(SurfaceType.GetValue(), Success);
}


const double FPhysMatProperties::CalculateStrainRate(const double ImpactVelocity, const double CrackLength)
{
	return (CrackLength / 2.0) * ImpactVelocity;
}

double FPhysMatProperties::CalculateStrainEnergyReleaseRate(const double ImpactVelocity, const double CrackLength) const
{
	double K = FractureToughness;
	double E = YoungsModulus;
	if (!IsInBaseUnits())
	{
		using namespace TB::UnitConversions;
		K = MPaToPa(K);
		E = GPaToPa(E);
	}
	using namespace TB::MathUtils;
	const double G = sqr(K) / (E * DOUBLE_PI * CrackLength);
	return G * gcem::pow(CalculateStrainRate(ImpactVelocity, CrackLength), StrainRateFactor);
}

double FPhysMatProperties::CalculateEnergyRequiredToPropagateFracture(const double ImpactVelocity, const double CrackArea, const double Depth) const
{
	const double G = CalculateStrainEnergyReleaseRate(ImpactVelocity, Depth);
	return G * CrackArea;
}

bool FPhysMatProperties::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	TB_PACK_ARCHIVE_WITH_BITFIELDS_FOUR(Ar, bIsFluid, bIsParticulate, bUseCustomRicochetProperties, bIsInBaseUnits);
	Ar << MaterialName;
	Ar << SurfaceType;
	Ar << Density;
	Ar << FractureToughness;
	Ar << UltimateTensileStrength;
	Ar << YieldStrength;
	Ar << CompressiveStrength;
	Ar << ShearStrength;
	Ar << PoissonsRatio;
	Ar << YoungsModulus;
	Ar << CoefficientOfFriction;
	Ar << Viscosity;
	Ar << StrainRateFactor;
	Ar << RicochetProperties;

	bool localSuccess = true;
	GameplayTag.NetSerialize(Ar, Map, localSuccess);

	bOutSuccess = localSuccess;
	return localSuccess;
}
#pragma endregion


namespace TB
{
	namespace PhysMatConstants
	{
		namespace Names
		{
			const FName PhysMatName_Acrylic = FName("Acrylic");
			const FName PhysMatName_Aluminum = FName("Aluminum");
			const FName PhysMatName_Asphalt = FName("Asphalt");
			const FName PhysMatName_Bone = FName("Bone");
			const FName PhysMatName_Brass = FName("Brass");
			const FName PhysMatName_Brick = FName("Brick");
			const FName PhysMatName_CarbonFiber = FName("CarbonFiber");
			const FName PhysMatName_CastIron = FName("CastIron");
			const FName PhysMatName_Ceramic = FName("Ceramic");
			const FName PhysMatName_Concrete = FName("Concrete");
			const FName PhysMatName_Copper = FName("Copper");
			const FName PhysMatName_Drywall = FName("Drywall");
			const FName PhysMatName_Flesh = FName("Flesh");
			const FName PhysMatName_Glass = FName("Glass");
			const FName PhysMatName_HardenedSteel = FName("HardenedSteel");
			const FName PhysMatName_HardPlastic = FName("HardPlastic");
			const FName PhysMatName_Hardwood = FName("Hardwood");
			const FName PhysMatName_Ice = FName("Ice");
			const FName PhysMatName_Kevlar = FName("Kevlar");
			const FName PhysMatName_Lead = FName("Lead");
			const FName PhysMatName_Limestone = FName("Limestone");
			const FName PhysMatName_Paper = FName("Paper");
			const FName PhysMatName_Plastic = FName("Plastic");
			const FName PhysMatName_Polystyrene = FName("Polystyrene");
			const FName PhysMatName_Rubber = FName("Rubber");
			const FName PhysMatName_Softwood = FName("Softwood");
			const FName PhysMatName_Steel = FName("Steel");
			const FName PhysMatName_Titanium = FName("Titanium");
			const FName PhysMatName_TungstenCarbide = FName("TungstenCarbide");
		};

		const FGameplayTag PhysMatTag()
		{
			return TB::Tags::FTerminalBallisticsTags::Get().PhysMatTag;
		}

		namespace DefaultPhysMats
		{
			using namespace TB::Tags::PhysMatTagNames;
			const FPhysMatProperties Acrylic()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Acrylic), SurfaceType1, Names::PhysMatName_Acrylic, 1.18, 1, 75, 70, 80, 70, 0.35, 2.7, 0.f);
			}
			const FPhysMatProperties Aluminum()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Aluminum), SurfaceType2, Names::PhysMatName_Aluminum, 2.7, 27, 200, 120, 120, 70, 0.33, 69, 0.f);
			}
			const FPhysMatProperties Asphalt()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Asphalt), SurfaceType3, Names::PhysMatName_Asphalt, 2.3, 0.5, 300, 200, 15, 0.3, 0.3, 1.5, 0.f);
			}
			const FPhysMatProperties Bone()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Bone), SurfaceType4, Names::PhysMatName_Bone, 2, 3, 155, 100, 100, 40, 0.3, 1.8, 0.f);
			}
			const FPhysMatProperties Brass()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Brass), SurfaceType5, Names::PhysMatName_Brass, 8.5, 1.5, 530, 375, 350, 220, 0.33, 125, 0.f);
			}
			const FPhysMatProperties Brick()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Brick), SurfaceType6, Names::PhysMatName_Brick, 2, 0.5, 20, 10, 15, 10, 0.2, 15, 0.f);
			}
			const FPhysMatProperties CarbonFiber()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_CarbonFiber), SurfaceType7, Names::PhysMatName_CarbonFiber, 1.7, 20, 5250, 3500, 400, 300, 0.3, 430, 0.f);
			}
			const FPhysMatProperties CastIron()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_CastIron), SurfaceType8, Names::PhysMatName_CastIron, 7.2, 4, 590, 420, 650, 370, 0.3, 170, 0.f);
			}
			const FPhysMatProperties Ceramic()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Ceramic), SurfaceType9, Names::PhysMatName_Ceramic, 2.75, 5, 200, -1, 800, 350, 0.25, 200, 0.f);
			}
			const FPhysMatProperties Concrete()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Concrete), SurfaceType10, Names::PhysMatName_Concrete, 2.3, 0.5, 140, 20, 30, 20, 0.2, 25, 0.f);
			}
			const FPhysMatProperties Copper()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Copper), SurfaceType11, Names::PhysMatName_Copper, 8.9, 1.5, 210, 70, 210, 120, 0.34, 130, 0.f);
			}
			const FPhysMatProperties Drywall()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Drywall), SurfaceType12, Names::PhysMatName_Drywall, 0.75, 1.1, 50, -1, 1.5, 1.5, 0.3, 1.5, 0.f);
			}
			const FPhysMatProperties Flesh()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Flesh), SurfaceType13, Names::PhysMatName_Flesh, 1.1, 0.007, 27, 5, 6, 0.3, 0.0006, 0.43, 0.f);
			}
			const FPhysMatProperties Glass()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Glass), SurfaceType14, Names::PhysMatName_Glass, 2.5, 0.85, 100, -1, 1500, 35, 0.25, 70, 0.f);
			}
			const FPhysMatProperties HardenedSteel()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_HardenedSteel), SurfaceType15, Names::PhysMatName_HardenedSteel, 7.8, 40, 1300, 1100, 1250, 850, 0.3, 200, 0.f, 2.f);
			}
			const FPhysMatProperties HardPlastic()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_HardPlastic), SurfaceType16, Names::PhysMatName_HardPlastic, 1.2, 1.4, 70, 60, 80, 70, 0.37, 2.4, 0.f);
			}
			const FPhysMatProperties Hardwood()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Hardwood), SurfaceType17, Names::PhysMatName_Hardwood, 0.6, 0.6, 60, 40, 45, 20, 0.3, 12, 0.f, 1.1f);
			}
			const FPhysMatProperties Ice()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Ice), SurfaceType18, Names::PhysMatName_Ice, 0.9, 0.2, 1.5, 1.5, 2, 0.3, 0.33, 9, 0.f);
			}
			const FPhysMatProperties Kevlar()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Kevlar), SurfaceType19, Names::PhysMatName_Kevlar, 1.44, 15, 3620, 2750, 150, 25, 0.35, 120, 0.f);
			}
			const FPhysMatProperties Lead()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Lead), SurfaceType20, Names::PhysMatName_Lead, 11.3, 0.18, 17, 12, 17, 4.5, 0.44, 16, 0.f);
			}
			const FPhysMatProperties Limestone()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Limestone), SurfaceType21, Names::PhysMatName_Limestone, 2.71, 0.6, 8, -1, 125, 30, 0.25, 40, 0.f);
			}
			const FPhysMatProperties Paper()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Paper), SurfaceType22, Names::PhysMatName_Paper, 0.6, 0.1, 30, -1, 30, 12, 0.43, 3, 0.f);
			}
			const FPhysMatProperties Plastic()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Plastic), SurfaceType23, Names::PhysMatName_Plastic, 0.95, 1, 20, 20, 25, 22.5, 0.4, 1.5, 0.f);
			}
			const FPhysMatProperties Polystyrene()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Polystyrene), SurfaceType24, Names::PhysMatName_Polystyrene, 1.05, 0.5, 38, 45, 0.5, 30, 0.33, 3.5, 0.f);
			}
			const FPhysMatProperties Rubber()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Rubber), SurfaceType25, Names::PhysMatName_Rubber, 1.2, 0.2, 25, 15, 5, 30, 0.5, 0.2, 0.f);
			}
			const FPhysMatProperties Softwood()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Softwood), SurfaceType26, Names::PhysMatName_Softwood, 0.5, 0.5, 60, 30, 30, 20, 0.3, 11, 0.f);
			}
			const FPhysMatProperties Steel()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Steel), SurfaceType27, Names::PhysMatName_Steel, 7.8, 30, 690, 590, 660, 450, 0.3, 200, 0.f);
			}
			const FPhysMatProperties Titanium()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_Titanium), SurfaceType28, Names::PhysMatName_Titanium, 4.5, 6, 900, 800, 900, 600, 0.34, 116, 0.f);
			}
			const FPhysMatProperties TungstenCarbide()
			{
				return FPhysMatProperties(FGameplayTag::RequestGameplayTag(TagName_TungstenCarbide), SurfaceType29, Names::PhysMatName_TungstenCarbide, 15, 10, 2600, 2400, 2500, 2200, 0.25, 650, 0.f);
			}

			TArray<FPhysMatProperties> GetDefaults()
			{
				return
				{
					Acrylic(),
					Aluminum(),
					Asphalt(),
					Bone(),
					Brass(),
					Brick(),
					CarbonFiber(),
					CastIron(),
					Ceramic(),
					Concrete(),
					Copper(),
					Drywall(),
					Flesh(),
					Glass(),
					HardenedSteel(),
					HardPlastic(),
					Hardwood(),
					Ice(),
					Kevlar(),
					Lead(),
					Limestone(),
					Paper(),
					Plastic(),
					Polystyrene(),
					Rubber(),
					Softwood(),
					Steel(),
					Titanium(),
					TungstenCarbide()
				};
			}

			TSoftObjectPtr<UPhysicalMaterial> GetDefaultPhysicalMaterialAssetForPhysMat(const FPhysMatProperties& PhysMat)
			{
				static const auto& DefaultMaterialPath = FString("/TerminalBallistics/PhysicalMaterials/");
				const FString MaterialName = PhysMat.MaterialName.ToString();
				return TSoftObjectPtr<UPhysicalMaterial>(FSoftObjectPath(DefaultMaterialPath + MaterialName + "." + MaterialName));
			}
		};
	};

	namespace PhysMatHelpers
	{
		UMaterialInterface* GetMaterialFromStaticMesh(const UStaticMeshComponent* StaticMeshComponent, const int32 MaterialIndex)
		{
			const TArray<UMaterialInterface*>& Materials = StaticMeshComponent->GetMaterials();

			if (Materials.IsValidIndex(MaterialIndex))
			{
				return Materials[MaterialIndex];
			}
			else
			{
				return nullptr;
			}
		}

		bool IsFlesh(const FPhysMatProperties& PhysMat)
		{
			const auto& Tags = TB::Tags::FTerminalBallisticsTags::Get();
			return PhysMat.GameplayTag == Tags.physMatTag_Flesh;
		}

		bool IsBone(const FPhysMatProperties& PhysMat)
		{
			const auto& Tags = TB::Tags::FTerminalBallisticsTags::Get();
			return PhysMat.GameplayTag == Tags.physMatTag_Bone;
		}

		bool IsFleshOrBone(const FPhysMatProperties& PhysMat)
		{
			const auto& Tags = TB::Tags::FTerminalBallisticsTags::Get();
			const auto& Tag = PhysMat.GameplayTag;
			return Tag == Tags.physMatTag_Flesh
				|| Tag == Tags.physMatTag_Bone;
		}

		bool IsFleshOrBone(const EPhysicalSurface SurfaceType)
		{
			const UTBPhysMatManager& PhysMatManager = UTBPhysMatManager::Get();
			return SurfaceType == PhysMatManager.GetSurfaceTypeFlesh()
				|| SurfaceType == PhysMatManager.GetSurfaceTypeBone();
		}


		FVector GetAdjustedVelocity(const FVector& Velocity, const FVector& SurfaceNormal, const FPhysMatProperties& ImpactedObjectProperties)
		{
			FVector TempVel = Velocity;
			const double VDotNorm = TempVel | SurfaceNormal;
			if (VDotNorm <= 0) // If the velocity is pointing away from the surface it won't collide so these don't apply
			{
				const FVector ProjectedNorm = SurfaceNormal * -VDotNorm;
				TempVel += ProjectedNorm;

				const float FrictionFactor = FMath::Clamp(-VDotNorm / TempVel.Size(), 0.0, 1.0); // Adjust friction force with angle
				TempVel *= FMath::Clamp(1.0 - (FrictionFactor * ImpactedObjectProperties.CoefficientOfFriction), 0.0, 1.0); // Apply friction force

				const float CoefficientOfRestituion = ImpactedObjectProperties.EstimateCoeffientOfRestitution();
				TempVel += ProjectedNorm * fmax(CoefficientOfRestituion, 0.f);
			}
			return TempVel;
		}
	};

};