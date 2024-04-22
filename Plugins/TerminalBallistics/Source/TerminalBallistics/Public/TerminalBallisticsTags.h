// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

namespace TB::Tags
{
	extern TERMINALBALLISTICS_API const FName PlainTag_IMPENETRABLE;
	extern TERMINALBALLISTICS_API const FName PlainTag_Ignore;
	extern TERMINALBALLISTICS_API const FName PlainTag_IgnorePenetration;
	extern TERMINALBALLISTICS_API const FName PlainTag_HitZone;
	extern TERMINALBALLISTICS_API const FName PlainTag_Flesh;
	extern TERMINALBALLISTICS_API const FName PlainTag_Bone;

	namespace PhysMatTagNames
	{
		extern const ANSICHAR* TagName_PhysMatTag;
		extern const ANSICHAR* TagName_Acrylic;
		extern const ANSICHAR* TagName_Aluminum;
		extern const ANSICHAR* TagName_Asphalt;
		extern const ANSICHAR* TagName_Bone;
		extern const ANSICHAR* TagName_Brass;
		extern const ANSICHAR* TagName_Brick;
		extern const ANSICHAR* TagName_CarbonFiber;
		extern const ANSICHAR* TagName_CastIron;
		extern const ANSICHAR* TagName_Ceramic;
		extern const ANSICHAR* TagName_Concrete;
		extern const ANSICHAR* TagName_Copper;
		extern const ANSICHAR* TagName_Drywall;
		extern const ANSICHAR* TagName_Flesh;
		extern const ANSICHAR* TagName_Glass;
		extern const ANSICHAR* TagName_HardenedSteel;
		extern const ANSICHAR* TagName_HardPlastic;
		extern const ANSICHAR* TagName_Hardwood;
		extern const ANSICHAR* TagName_Ice;
		extern const ANSICHAR* TagName_Kevlar;
		extern const ANSICHAR* TagName_Lead;
		extern const ANSICHAR* TagName_Limestone;
		extern const ANSICHAR* TagName_Paper;
		extern const ANSICHAR* TagName_Plastic;
		extern const ANSICHAR* TagName_Polystyrene;
		extern const ANSICHAR* TagName_Rubber;
		extern const ANSICHAR* TagName_Softwood;
		extern const ANSICHAR* TagName_Steel;
		extern const ANSICHAR* TagName_Titanium;
		extern const ANSICHAR* TagName_TungstenCarbide;
	};

	struct FTerminalBallisticsTags
	{
		FTerminalBallisticsTags() {}

	public:

		static bool bHasBeenInitialized;

		static const FTerminalBallisticsTags& Get() { return GameplayTags; }

		inline static void InitializeNativeTags()
		{
			GameplayTags.AddAllTags();

			bHasBeenInitialized = true;
		}

	public:

		FGameplayTag HitZone;
		FGameplayTag HitZoneFlesh;
		FGameplayTag HitZoneBone;
		FGameplayTag HitZoneDead;

		FGameplayTag tag_impenetrable;

#pragma region PhysMats
		FGameplayTag PhysMatTag;
		FGameplayTag physMatTag_Acrylic;
		FGameplayTag physMatTag_Aluminum;
		FGameplayTag physMatTag_Asphalt;
		FGameplayTag physMatTag_Bone;
		FGameplayTag physMatTag_Brass;
		FGameplayTag physMatTag_Brick;
		FGameplayTag physMatTag_CarbonFiber;
		FGameplayTag physMatTag_CastIron;
		FGameplayTag physMatTag_Ceramic;
		FGameplayTag physMatTag_Concrete;
		FGameplayTag physMatTag_Copper;
		FGameplayTag physMatTag_Drywall;
		FGameplayTag physMatTag_Flesh;
		FGameplayTag physMatTag_Glass;
		FGameplayTag physMatTag_HardenedSteel;
		FGameplayTag physMatTag_HardPlastic;
		FGameplayTag physMatTag_Hardwood;
		FGameplayTag physMatTag_Ice;
		FGameplayTag physMatTag_Kevlar;
		FGameplayTag physMatTag_Lead;
		FGameplayTag physMatTag_Limestone;
		FGameplayTag physMatTag_Paper;
		FGameplayTag physMatTag_Plastic;
		FGameplayTag physMatTag_Polystyrene;
		FGameplayTag physMatTag_Rubber;
		FGameplayTag physMatTag_Softwood;
		FGameplayTag physMatTag_Steel;
		FGameplayTag physMatTag_Titanium;
		FGameplayTag physMatTag_TungstenCarbide;

		TArray<FGameplayTag> GetPhysMatTags() const
		{
			return
			{
				physMatTag_Acrylic,
				physMatTag_Aluminum,
				physMatTag_Asphalt,
				physMatTag_Bone,
				physMatTag_Brass,
				physMatTag_Brick,
				physMatTag_CarbonFiber,
				physMatTag_CastIron,
				physMatTag_Ceramic,
				physMatTag_Concrete,
				physMatTag_Copper,
				physMatTag_Drywall,
				physMatTag_Flesh,
				physMatTag_Glass,
				physMatTag_HardenedSteel,
				physMatTag_HardPlastic,
				physMatTag_Hardwood,
				physMatTag_Ice,
				physMatTag_Kevlar,
				physMatTag_Lead,
				physMatTag_Limestone,
				physMatTag_Paper,
				physMatTag_Plastic,
				physMatTag_Polystyrene,
				physMatTag_Rubber,
				physMatTag_Softwood,
				physMatTag_Steel,
				physMatTag_Titanium,
				physMatTag_TungstenCarbide
			};
		}
#pragma endregion


	protected:
		void AddAllTags();
		void AddTag(FGameplayTag& OutTag, const ANSICHAR* TagName);
	private:
		static FTerminalBallisticsTags GameplayTags;
	};
};
