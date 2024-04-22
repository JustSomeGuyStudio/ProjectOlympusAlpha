// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "TerminalBallisticsTags.h"
#include "GameplayTagsManager.h"

namespace TB::Tags
{
	const FName PlainTag_IMPENETRABLE = FName("Impenetrable");
	const FName PlainTag_Ignore = FName("TBIgnore");
	const FName PlainTag_IgnorePenetration = FName("TBIgnorePenetration");
	const FName PlainTag_HitZone = FName("HitZone");
	const FName PlainTag_Flesh = FName("Flesh");
	const FName PlainTag_Bone = FName("Bone");

	namespace PhysMatTagNames
	{
		const ANSICHAR* TagName_PhysMatTag = "PhysMat";
		const ANSICHAR* TagName_Acrylic = "PhysMat.Acrylic";
		const ANSICHAR* TagName_Aluminum = "PhysMat.Aluminum";
		const ANSICHAR* TagName_Asphalt = "PhysMat.Asphalt";
		const ANSICHAR* TagName_Bone = "PhysMat.Bone";
		const ANSICHAR* TagName_Brass = "PhysMat.Brass";
		const ANSICHAR* TagName_Brick = "PhysMat.Brick";
		const ANSICHAR* TagName_CarbonFiber = "PhysMat.CarbonFiber";
		const ANSICHAR* TagName_CastIron = "PhysMat.CastIron";
		const ANSICHAR* TagName_Ceramic = "PhysMat.Ceramic";
		const ANSICHAR* TagName_Concrete = "PhysMat.Concrete";
		const ANSICHAR* TagName_Copper = "PhysMat.Copper";
		const ANSICHAR* TagName_Drywall = "PhysMat.Drywall";
		const ANSICHAR* TagName_Flesh = "PhysMat.Flesh";
		const ANSICHAR* TagName_Glass = "PhysMat.Glass";
		const ANSICHAR* TagName_HardenedSteel = "PhysMat.HardenedSteel";
		const ANSICHAR* TagName_HardPlastic = "PhysMat.HardPlastic";
		const ANSICHAR* TagName_Hardwood = "PhysMat.Hardwood";
		const ANSICHAR* TagName_Ice = "PhysMat.Ice";
		const ANSICHAR* TagName_Kevlar = "PhysMat.Kevlar";
		const ANSICHAR* TagName_Lead = "PhysMat.Lead";
		const ANSICHAR* TagName_Limestone = "PhysMat.Limestone";
		const ANSICHAR* TagName_Paper = "PhysMat.Paper";
		const ANSICHAR* TagName_Plastic = "PhysMat.Plastic";
		const ANSICHAR* TagName_Polystyrene = "PhysMat.Polystyrene";
		const ANSICHAR* TagName_Rubber = "PhysMat.Rubber";
		const ANSICHAR* TagName_Softwood = "PhysMat.Softwood";
		const ANSICHAR* TagName_Steel = "PhysMat.Steel";
		const ANSICHAR* TagName_Titanium = "PhysMat.Titanium";
		const ANSICHAR* TagName_TungstenCarbide = "PhysMat.TungstenCarbide";
	};

	FTerminalBallisticsTags FTerminalBallisticsTags::GameplayTags = FTerminalBallisticsTags();
	bool FTerminalBallisticsTags::bHasBeenInitialized = false;

	void FTerminalBallisticsTags::AddAllTags()
	{
		AddTag(tag_impenetrable, "Object.Impenetrable");

		AddTag(HitZone, "HitZone");
		AddTag(HitZoneFlesh, "HitZone.Type.Flesh");
		AddTag(HitZoneFlesh, "HitZone.Type.Bone");
		AddTag(HitZoneDead, "HitZone.Status.Dead");

		using namespace PhysMatTagNames;
		AddTag(PhysMatTag, TagName_PhysMatTag);

		AddTag(physMatTag_Acrylic, TagName_Acrylic);
		AddTag(physMatTag_Aluminum, TagName_Aluminum);
		AddTag(physMatTag_Asphalt, TagName_Asphalt);
		AddTag(physMatTag_Bone, TagName_Bone);
		AddTag(physMatTag_Brass, TagName_Brass);
		AddTag(physMatTag_Brick, TagName_Brick);
		AddTag(physMatTag_CarbonFiber, TagName_CarbonFiber);
		AddTag(physMatTag_CastIron, TagName_CastIron);
		AddTag(physMatTag_Ceramic, TagName_Ceramic);
		AddTag(physMatTag_Concrete, TagName_Concrete);
		AddTag(physMatTag_Copper, TagName_Copper);
		AddTag(physMatTag_Drywall, TagName_Drywall);
		AddTag(physMatTag_Flesh, TagName_Flesh);
		AddTag(physMatTag_Glass, TagName_Glass);
		AddTag(physMatTag_HardenedSteel, TagName_HardenedSteel);
		AddTag(physMatTag_HardPlastic, TagName_HardPlastic);
		AddTag(physMatTag_Hardwood, TagName_Hardwood);
		AddTag(physMatTag_Ice, TagName_Ice);
		AddTag(physMatTag_Kevlar, TagName_Kevlar);
		AddTag(physMatTag_Lead, TagName_Lead);
		AddTag(physMatTag_Limestone, TagName_Limestone);
		AddTag(physMatTag_Paper, TagName_Paper);
		AddTag(physMatTag_Plastic, TagName_Plastic);
		AddTag(physMatTag_Polystyrene, TagName_Polystyrene);
		AddTag(physMatTag_Rubber, TagName_Rubber);
		AddTag(physMatTag_Softwood, TagName_Softwood);
		AddTag(physMatTag_Steel, TagName_Steel);
		AddTag(physMatTag_Titanium, TagName_Titanium);
		AddTag(physMatTag_TungstenCarbide, TagName_TungstenCarbide);
	}

	void FTerminalBallisticsTags::AddTag(FGameplayTag& OutTag, const ANSICHAR* TagName)
	{
		TSharedPtr<FGameplayTagNode> Tag = UGameplayTagsManager::Get().FindTagNode(FName(TagName));
		if (!Tag)
		{
			OutTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName(TagName), FString(TEXT("(Native)")));
		}
		else
		{
			OutTag = Tag->GetCompleteTag();
		}
	}
}
