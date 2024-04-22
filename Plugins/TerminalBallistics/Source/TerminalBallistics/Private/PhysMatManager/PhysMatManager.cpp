#include "PhysMatManager/PhysMatManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Internationalization/Regex.h"
#include "Materials/MaterialInterface.h"
#include "Misc/ConfigCacheIni.h"
#include "TerminalBallisticsConfiguration.h"
#include "TerminalBallisticsMacrosAndFunctions.h"
#include "TerminalBallisticsTags.h"
#include "UObject/LinkerLoad.h"
#include "UObject/Package.h"



FTBPresetMaterialProperties::FTBPresetMaterialProperties(const TMap<FGameplayTag, FPhysMatProperties>& PhysMatMap)
{
	Init(PhysMatMap);
}

void FTBPresetMaterialProperties::Init(const TMap<FGameplayTag, FPhysMatProperties>& PhysMatMap) const
{
	using namespace TB::Tags;
	if (FTerminalBallisticsTags::bHasBeenInitialized && !bHasBeenInitialized)
	{
		const auto& Tags = FTerminalBallisticsTags::Get();

		auto SetProperty = [PhysMatMap](const FGameplayTag& Tag, TOptional<FPhysMatProperties>* PhysMatProperty)
		{
			if (const FPhysMatProperties* Value = PhysMatMap.Find(Tag))
			{
				*PhysMatProperty = TOptional<FPhysMatProperties>(*Value);
			}
		};

		SetProperty(Tags.physMatTag_Acrylic, &Acrylic);
		SetProperty(Tags.physMatTag_Aluminum, &Aluminum);
		SetProperty(Tags.physMatTag_Asphalt, &Asphalt);
		SetProperty(Tags.physMatTag_Bone, &Bone);
		SetProperty(Tags.physMatTag_Brass, &Brass);
		SetProperty(Tags.physMatTag_Brick, &Brick);
		SetProperty(Tags.physMatTag_CarbonFiber, &CarbonFiber);
		SetProperty(Tags.physMatTag_CastIron, &CastIron);
		SetProperty(Tags.physMatTag_Ceramic, &Ceramic);
		SetProperty(Tags.physMatTag_Concrete, &Concrete);
		SetProperty(Tags.physMatTag_Copper, &Copper);
		SetProperty(Tags.physMatTag_Drywall, &Drywall);
		SetProperty(Tags.physMatTag_Flesh, &Flesh);
		SetProperty(Tags.physMatTag_Glass, &Glass);
		SetProperty(Tags.physMatTag_HardenedSteel, &HardenedSteel);
		SetProperty(Tags.physMatTag_HardPlastic, &HardPlastic);
		SetProperty(Tags.physMatTag_Hardwood, &Hardwood);
		SetProperty(Tags.physMatTag_Ice, &Ice);
		SetProperty(Tags.physMatTag_Kevlar, &Kevlar);
		SetProperty(Tags.physMatTag_Lead, &Lead);
		SetProperty(Tags.physMatTag_Limestone, &Limestone);
		SetProperty(Tags.physMatTag_Paper, &Paper);
		SetProperty(Tags.physMatTag_Plastic, &Plastic);
		SetProperty(Tags.physMatTag_Polystyrene, &Polystyrene);
		SetProperty(Tags.physMatTag_Rubber, &Rubber);
		SetProperty(Tags.physMatTag_Softwood, &Softwood);
		SetProperty(Tags.physMatTag_Steel, &Steel);
		SetProperty(Tags.physMatTag_Titanium, &Titanium);
		SetProperty(Tags.physMatTag_TungstenCarbide, &TungstenCarbide);

		bHasBeenInitialized = true;
	}
}

FTBPresetMaterialProperties FTBPresetMaterialProperties::GetDefaults()
{
	FTBPresetMaterialProperties Defaults;

	using namespace TB::PhysMatConstants;

	Defaults.Acrylic.Emplace(DefaultPhysMats::Acrylic());
	Defaults.Aluminum.Emplace(DefaultPhysMats::Aluminum());
	Defaults.Asphalt.Emplace(DefaultPhysMats::Asphalt());
	Defaults.Bone.Emplace(DefaultPhysMats::Bone());
	Defaults.Brass.Emplace(DefaultPhysMats::Brass());
	Defaults.Brick.Emplace(DefaultPhysMats::Brick());
	Defaults.CarbonFiber.Emplace(DefaultPhysMats::CarbonFiber());
	Defaults.CastIron.Emplace(DefaultPhysMats::CastIron());
	Defaults.Ceramic.Emplace(DefaultPhysMats::Ceramic());
	Defaults.Concrete.Emplace(DefaultPhysMats::Concrete());
	Defaults.Copper.Emplace(DefaultPhysMats::Copper());
	Defaults.Drywall.Emplace(DefaultPhysMats::Drywall());
	Defaults.Flesh.Emplace(DefaultPhysMats::Flesh());
	Defaults.Glass.Emplace(DefaultPhysMats::Glass());
	Defaults.HardenedSteel.Emplace(DefaultPhysMats::HardenedSteel());
	Defaults.HardPlastic.Emplace(DefaultPhysMats::HardPlastic());
	Defaults.Hardwood.Emplace(DefaultPhysMats::Hardwood());
	Defaults.Ice.Emplace(DefaultPhysMats::Ice());
	Defaults.Kevlar.Emplace(DefaultPhysMats::Kevlar());
	Defaults.Lead.Emplace(DefaultPhysMats::Lead());
	Defaults.Limestone.Emplace(DefaultPhysMats::Limestone());
	Defaults.Paper.Emplace(DefaultPhysMats::Paper());
	Defaults.Plastic.Emplace(DefaultPhysMats::Plastic());
	Defaults.Polystyrene.Emplace(DefaultPhysMats::Polystyrene());
	Defaults.Rubber.Emplace(DefaultPhysMats::Rubber());
	Defaults.Softwood.Emplace(DefaultPhysMats::Softwood());
	Defaults.Steel.Emplace(DefaultPhysMats::Steel());
	Defaults.Titanium.Emplace(DefaultPhysMats::Titanium());
	Defaults.TungstenCarbide.Emplace(DefaultPhysMats::TungstenCarbide());

	Defaults.bHasBeenInitialized = true;

	return Defaults;
}


UTBPhysMatManager* UTBPhysMatManager::Singleton = nullptr;

void UTBPhysMatManager::Initialize()
{
	if (Singleton)
	{
		return;
	}
	if (!GEngine)
	{
		return;
	}
	
	Singleton = NewObject<UTBPhysMatManager>(GetTransientPackage(), NAME_None);
	Singleton->AddToRoot();
	Singleton->LoadMaterialPropertyTables(true);

	TDelegate<void(const TArray<FSoftObjectPath>&)> OnDataTablesChanged;
	OnDataTablesChanged.BindUObject(Singleton, &UTBPhysMatManager::DataTablesUpdated);
	TDelegate<void(const TArray<FTBPhysMatMapping>&)> OnPhysMatsChanged;
	OnPhysMatsChanged.BindUObject(Singleton, &UTBPhysMatManager::PhysMatsUpdated);

	UTBConfiguration::Register_OnDataTablesChanged(MoveTemp(OnDataTablesChanged));
	UTBConfiguration::Register_OnPhysMatsChanged(MoveTemp(OnPhysMatsChanged));

	Singleton->BuildMap();
}

void UTBPhysMatManager::LoadMaterialPropertyTables(bool bAllowAsyncLoad)
{
	MaterialPropertyTables.Empty();

#if !WITH_EDITOR
	// If we're a cooked build and in a safe spot, start an async load so we can pipeline it
	if (bAllowAsyncLoad &&!IsLoading() && DataTablePathsToLoad.Num() > 0)
	{
		for (FSoftObjectPath DataTablePath : DataTablePathsToLoad)
		{
			LoadPackageAsync(DataTablePath.GetLongPackageName());
		}

		return;
	}
#endif

	for (FSoftObjectPath DataTablePath : DataTablePathsToLoad)
	{
		UDataTable* TagTable = LoadObject<UDataTable>(nullptr, *DataTablePath.ToString(), nullptr, LOAD_None, nullptr);

		// Handle case where the module is dynamically-loaded within a LoadPackage stack, which would otherwise
		// result in the table not having its RowStruct serialized in time.
		if (TagTable)
		{
			FLinkerLoad* TagLinker = TagTable->GetLinker();
			if (TagLinker)
			{
				TagTable->GetLinker()->Preload(TagTable);
			}
		}
		MaterialPropertyTables.Add(TagTable);
	}
}

void UTBPhysMatManager::DataTablesUpdated(const TArray<FSoftObjectPath>& NewTables)
{
	DataTablePathsToLoad = NewTables;
	MaterialPropertyTables.Empty();
}

void UTBPhysMatManager::PhysMatsUpdated(const TArray<FTBPhysMatMapping>& NewPhysMats)
{
	PhysMatMap.Empty();
	for (const auto& PhysMat : NewPhysMats)
	{
		PhysMatMap.Add(PhysMat.PhysMatProperties.GameplayTag, PhysMat.PhysMatProperties);
	}
	PresetProperties = FTBPresetMaterialProperties(PhysMatMap);
}

void UTBPhysMatManager::LoadDeprecatedMappings()
{
	UTBConfiguration* TBConfig = GetMutableDefault<UTBConfiguration>();
	FString Str;
	static const auto SectionName = TEXT("/Script/TerminalBallistics.TBConfiguration");
	static const auto Key = TEXT("PhysicalMaterialMappings");
	const bool FoundDeprecatedConfig = GConfig->GetString(SectionName, Key, Str, GGameIni);
	if(FoundDeprecatedConfig)
	{
		Str.ReplaceEscapedCharWithCharInline();
		Str.ReplaceInline(TEXT("PhysicalMaterialMappings="), TEXT(""));
		FString LStr, RStr;
		FString splitWith = FString("Mappings=");
		Str.Split(splitWith, &LStr, &RStr);
		Str = RStr;
		splitWith = ")),";

		static const FRegexPattern& Pattern = FRegexPattern(TEXT("\\(\"(\\w+)\".+\"(PhysMat\\.\\w+)\".+SurfaceType(\\d+),\\s*.+\"([\\w\\/\\.]+)\"")); // Group 1: Name, Group 2: GameplayTag, Group 3: SurfaceType, Group 4: Physical Material Asset

		bool Found = Str.Split(splitWith, &LStr, &RStr);
		while (Found)
		{
			FRegexMatcher Matcher(Pattern, LStr);

			if (Matcher.FindNext())
			{
				const FName& Name = *Matcher.GetCaptureGroup(1);
				const FGameplayTag& Tag = FGameplayTag::RequestGameplayTag(*Matcher.GetCaptureGroup(2));
				const EPhysicalSurface SurfaceType = static_cast<EPhysicalSurface>(FCString::Atoi(*Matcher.GetCaptureGroup(3)));
				const TSoftObjectPtr<UPhysicalMaterial> PhysicalMaterial = TSoftObjectPtr<UPhysicalMaterial>(FSoftObjectPath(*Matcher.GetCaptureGroup(4)));

				for (auto& Mapping : TBConfig->PhysMats)
				{
					if (Mapping.PhysMatProperties.GameplayTag == Tag || Mapping.PhysMatProperties.MaterialName == Name)
					{
						TBConfig->bIsUpdatingPhysMatMapInternally = true;
						Mapping.PhysicalMaterial = PhysicalMaterial;
						TBConfig->bIsUpdatingPhysMatMapInternally = false;
					}
				}

				if (FTBPhysMatMapping* ActualMapping = TBConfig->PhysMats.FindByPredicate([Tag](const FTBPhysMatMapping& X) { return X.PhysMatProperties.GameplayTag == Tag; }))
				{
					ActualMapping->PhysicalMaterial = PhysicalMaterial;
				}
			}

			Str = RStr;
			Found = Str.Split(splitWith, &LStr, &RStr);
		}

		/* Now remove the old config line */
		GConfig->RemoveKey(SectionName, Key, GGameIni);
	}
}

FPhysMatProperties UTBPhysMatManager::GetFromTag(const FGameplayTag& Tag) const
{
	bool Success;
	return GetFromTag(Tag, Success);
}

FPhysMatProperties UTBPhysMatManager::GetFromTag(const FGameplayTag& Tag, bool& Success) const
{
	const FPhysMatProperties* Mat = PhysMatMap.Find(Tag);
	if (Mat)
	{
		Success = true;
		return *Mat;
	}
	else
	{
		Success = false;
		return FPhysMatProperties::GetDefault();
	}
}

FPhysMatProperties UTBPhysMatManager::GetFromSurfaceType(TEnumAsByte<EPhysicalSurface> SurfaceType, bool& Success) const
{
	for (const auto& Pair : PhysMatMap)
	{
		if (Pair.Value.SurfaceType == SurfaceType)
		{
			Success = true;
			return Pair.Value;
		}
	}
	Success = false;
	return FPhysMatProperties::GetDefault();
}

FPhysMatProperties UTBPhysMatManager::GetFromPhysicalMaterial(UPhysicalMaterial* Material, bool& Success) const
{
	Success = false;
	FPhysMatProperties Properties = FPhysMatProperties::GetDefault();
	TB_RET_COND_VALUE(!Material, Properties);

	const UTBConfiguration* TBConfig = GetDefault<UTBConfiguration>();
	if (const UTBPhysicalMaterialWithTags* PhysMatWithTags = Cast<UTBPhysicalMaterialWithTags>(Material))
	{
		Properties = GetFromPhysMatWithTags(PhysMatWithTags, Success);
	}
	else if (Material->Implements<UGameplayTagAssetInterface>())
	{
		FGameplayTagContainer TagContainer;
		Cast<IGameplayTagAssetInterface>(Material)->GetOwnedGameplayTags(TagContainer);
		if (TagContainer.HasTag(TB::PhysMatConstants::PhysMatTag()))
		{
			TArray<FGameplayTag> Tags;
			TagContainer.Filter(FGameplayTagContainer(TB::PhysMatConstants::PhysMatTag())).GetGameplayTagArray(Tags);
			if (Tags.Num() > 0)
			{
				FGameplayTag TagToSearchWith = Tags[0];
				if (Tags.Num() > 1)
				{
					TagToSearchWith = *Tags.FindByPredicate([](const FGameplayTag& Tag) { return Tag.GetGameplayTagParents().HasTag(TB::PhysMatConstants::PhysMatTag()); });
				}
				Properties = GetFromTag(TagToSearchWith, Success);
			}
		}
	}
	if (!Success) // The tag seems to have been invalid, try checking against surface mappings
	{
		for (const auto& PhysMat : TBConfig->PhysMats)
		{
			if (PhysMat.HasPhysicalMaterialAsset() && PhysMat.PhysicalMaterial.Get() == Material)
			{
				Success = true;
				Properties = PhysMat.PhysMatProperties;
			}
		}
	}
	return Properties;
}

FPhysMatProperties UTBPhysMatManager::GetFromPhysMatWithTags(const UTBPhysicalMaterialWithTags* Material, bool& Success) const
{
	FPhysMatProperties Out = GetFromTag(Material->Tag, Success);
	if (!Success)
	{
		Out = GetFromSurfaceType(Material->SurfaceType, Success);
	}
	return Out;
}

bool UTBPhysMatManager::AddEntry(const FGameplayTag& Tag, const FPhysMatProperties& Properties)
{
	if (PhysMatMap.Contains(Tag))
	{
		return false;
	}

	PhysMatMap.Add(Tag, Properties);
	SyncWithConfig(false);
	return true;
}

bool UTBPhysMatManager::UpdateEntry(const FGameplayTag& Tag, const FPhysMatProperties& NewProperties)
{
	if (!PhysMatMap.Contains(Tag))
	{
		return false;
	}

	*(PhysMatMap.Find(Tag)) = NewProperties;
	SyncWithConfig(false);
	return true;
}

void UTBPhysMatManager::AddOrUpdateEntry(const FGameplayTag& Tag, const FPhysMatProperties& Properties)
{
	PhysMatMap.FindOrAdd(Tag) = Properties;
	SyncWithConfig(false);
}

FPhysMatProperties UTBPhysMatManager::GetFromHitResult(const FHitResult& HitResult) const
{
	bool Success;
	return GetFromHitResult(HitResult, Success);
}
FPhysMatProperties UTBPhysMatManager::GetFromHitResult(const FHitResult& HitResult, bool& Success) const
{
	Success = false;
	if (!HitResult.GetComponent() || !HitResult.GetActor())
	{
		return FPhysMatProperties::GetDefault();
	}

	using namespace TB::PhysMatConstants;
	if (HitResult.GetComponent()->Implements<UGameplayTagAssetInterface>())
	{
		IGameplayTagAssetInterface* AsGameplayTagAssetInterface = Cast<IGameplayTagAssetInterface>(HitResult.GetComponent());
		const FGameplayTag& PhysMatGameplayTag = PhysMatTag();
		if (AsGameplayTagAssetInterface->HasMatchingGameplayTag(PhysMatGameplayTag))
		{
			FGameplayTagContainer Tags;
			AsGameplayTagAssetInterface->GetOwnedGameplayTags(Tags);
			Tags = Tags.Filter(PhysMatGameplayTag.GetSingleTagContainer());
			
			FPhysMatProperties Properties = GetFromTag(Tags.First(), Success);
			if (Success)
			{
				return Properties;
			}
		}
	}
	else if (HitResult.GetActor()->Implements<UGameplayTagAssetInterface>())
	{
		IGameplayTagAssetInterface* AsGameplayTagAssetInterface = Cast<IGameplayTagAssetInterface>(HitResult.GetActor());
		const FGameplayTag& PhysMatGameplayTag = PhysMatTag();
		if (AsGameplayTagAssetInterface->HasMatchingGameplayTag(PhysMatGameplayTag))
		{
			FGameplayTagContainer Tags;
			AsGameplayTagAssetInterface->GetOwnedGameplayTags(Tags);
			Tags = Tags.Filter(PhysMatGameplayTag.GetSingleTagContainer());
			FPhysMatProperties Properties = GetFromTag(Tags.First(), Success);
			if (Success)
			{
				return Properties;
			}
		}
	}

	TWeakObjectPtr<UPhysicalMaterial> physMat = HitResult.PhysMaterial;
	int32 zero = 0;
	TWeakObjectPtr<UMaterialInterface> mat;
	if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(HitResult.GetComponent()))
	{
		UMaterialInterface* Material = TB::PhysMatHelpers::GetMaterialFromStaticMesh(StaticMeshComp, 0);
		if (Material)
		{
			mat = MakeWeakObjectPtr(Material);
		}
	}
	else if (HitResult.FaceIndex == -1)
	{
		mat = TWeakObjectPtr<UMaterialInterface>(HitResult.GetComponent()->GetMaterial(0));
	}
	else
	{
		mat = TWeakObjectPtr<UMaterialInterface>(HitResult.GetComponent()->GetMaterialFromCollisionFaceIndex(HitResult.FaceIndex, zero));
	}

	if (physMat.IsValid() || mat.IsValid())
	{
		TWeakObjectPtr<UPhysicalMaterial> physMatToUse = physMat;
		if ((!physMat.IsValid() && mat.IsValid()) || (mat.IsValid() && physMat.IsValid() && physMat.Get() == GEngine->DefaultPhysMaterial))
		{
			physMatToUse = TWeakObjectPtr<UPhysicalMaterial>(mat.Get()->GetPhysicalMaterial());
		}

		FPhysMatProperties Properties;
		Success = FPhysMatProperties::GetFromPhysicalMaterial(physMatToUse, Properties);
		if (Success)
		{
			return Properties;
		}
	}
	return FPhysMatProperties::GetDefault();
}

TArray<FPhysMatProperties> UTBPhysMatManager::GetAllMaterials() const
{
	TArray<FPhysMatProperties> OutArr;
	PhysMatMap.GenerateValueArray(OutArr);
	return OutArr;
}

TEnumAsByte<EPhysicalSurface> UTBPhysMatManager::GetSurfaceTypeFlesh() const
{
	if (!SurfaceTypeFlesh.IsSet())
	{
		SurfaceTypeFlesh.Emplace(PresetProperties.Flesh.Get(TB::PhysMatConstants::DefaultPhysMats::Flesh()).SurfaceType);
	}
	return SurfaceTypeFlesh.GetValue();;
}

TEnumAsByte<EPhysicalSurface> UTBPhysMatManager::GetSurfaceTypeBone() const
{
	if (!SurfaceTypeBone.IsSet())
	{
		SurfaceTypeBone.Emplace(PresetProperties.Bone.Get(TB::PhysMatConstants::DefaultPhysMats::Bone()).SurfaceType);
	}
	return SurfaceTypeBone.GetValue();;
}

FTBPresetMaterialProperties UTBPhysMatManager::GetPropertiesForPresets()
{
	if (Singleton)
	{
		Singleton->PresetProperties.Init(Singleton->PhysMatMap);
		return Singleton->PresetProperties;
	}
	else
	{
		return FTBPresetMaterialProperties::GetDefaults();
	}
}

void UTBPhysMatManager::CallOrRegister_OnInitialized(FSimpleMulticastDelegate::FDelegate&& Delegate)
{
	if (bIsInitialized)
	{
		Delegate.Execute();
	}
	else
	{
		bool bAlreadyBound = Delegate.GetUObject() != nullptr ? OnInitialized().IsBoundToObject(Delegate.GetUObject()) : false;
		if (!bAlreadyBound)
		{
			OnInitialized().Add(Delegate);
		}
	}
}

FSimpleMulticastDelegate UTBPhysMatManager::OnInitialized()
{
	static FSimpleMulticastDelegate Delegate;
	return Delegate;
}

void UTBPhysMatManager::SetToDefaults()
{
	using namespace TB::PhysMatConstants;
	PhysMatMap.Empty(NumPhysMats);

	Add(DefaultPhysMats::Acrylic());
	Add(DefaultPhysMats::Aluminum());
	Add(DefaultPhysMats::Asphalt());
	Add(DefaultPhysMats::Bone());
	Add(DefaultPhysMats::Brass());
	Add(DefaultPhysMats::Brick());
	Add(DefaultPhysMats::CarbonFiber());
	Add(DefaultPhysMats::CastIron());
	Add(DefaultPhysMats::Ceramic());
	Add(DefaultPhysMats::Concrete());
	Add(DefaultPhysMats::Copper());
	Add(DefaultPhysMats::Drywall());
	Add(DefaultPhysMats::Flesh());
	Add(DefaultPhysMats::Glass());
	Add(DefaultPhysMats::HardenedSteel());
	Add(DefaultPhysMats::HardPlastic());
	Add(DefaultPhysMats::Hardwood());
	Add(DefaultPhysMats::Ice());
	Add(DefaultPhysMats::Kevlar());
	Add(DefaultPhysMats::Lead());
	Add(DefaultPhysMats::Limestone());
	Add(DefaultPhysMats::Paper());
	Add(DefaultPhysMats::Plastic());
	Add(DefaultPhysMats::Polystyrene());
	Add(DefaultPhysMats::Rubber());
	Add(DefaultPhysMats::Softwood());
	Add(DefaultPhysMats::Steel());
	Add(DefaultPhysMats::Titanium());
	Add(DefaultPhysMats::TungstenCarbide());

	PresetProperties = FTBPresetMaterialProperties(PhysMatMap);
	SurfaceTypeFlesh.Emplace(PresetProperties.Flesh.Get(TB::PhysMatConstants::DefaultPhysMats::Flesh()).SurfaceType);
	SurfaceTypeBone.Emplace(PresetProperties.Bone.Get(TB::PhysMatConstants::DefaultPhysMats::Bone()).SurfaceType);
}

void UTBPhysMatManager::BuildMap()
{
	UTBConfiguration* TBConfig = GetMutableDefault<UTBConfiguration>();

	if (TBConfig->PhysMats.IsEmpty())
	{
		TBConfig->bIsUpdatingPhysMatMapInternally = true;
		const auto& MaterialArr = TB::PhysMatConstants::DefaultPhysMats::GetDefaults();

		using TB::PhysMatConstants::DefaultPhysMats::GetDefaultPhysicalMaterialAssetForPhysMat;
		for (const auto& PhysMat : MaterialArr)
		{
			TBConfig->PhysMats.Emplace(GetDefaultPhysicalMaterialAssetForPhysMat(PhysMat), PhysMat);
		}
		TBConfig->bIsUpdatingPhysMatMapInternally = false;
	}

	for (const auto& PhysMat : TBConfig->PhysMats)
	{
		Add(PhysMat.PhysMatProperties, true);
	}

	if (MaterialPropertyTables.Num() == 0)
	{
		LoadMaterialPropertyTables(true);
	}

	if (MaterialPropertyTables.Num() > 0)
	{
		for (const auto& DataTable : MaterialPropertyTables)
		{
			AddFromDataTable(DataTable);
		}
	}

	if (PhysMatMap.IsEmpty())
	{
		SetToDefaults();
	}
	else
	{
		/* Ensure the materials used in hardcoded objects (mainly bullets) are available */
		using namespace TB::PhysMatConstants;
		Add(DefaultPhysMats::Bone(), true);
		Add(DefaultPhysMats::Flesh(), true);
		Add(DefaultPhysMats::HardenedSteel(), true);
		Add(DefaultPhysMats::Lead(), true);
	}

	LoadDeprecatedMappings();

	PresetProperties.Init(PhysMatMap);

	SurfaceTypeFlesh.Emplace(PresetProperties.Flesh.Get(TB::PhysMatConstants::DefaultPhysMats::Flesh()).SurfaceType);
	SurfaceTypeBone.Emplace(PresetProperties.Bone.Get(TB::PhysMatConstants::DefaultPhysMats::Bone()).SurfaceType);

	bIsInitialized = true;
	OnInitialized().Broadcast();
}

bool UTBPhysMatManager::Contains(const FGameplayTag& Tag) const
{
	return PhysMatMap.Contains(Tag);
}

bool UTBPhysMatManager::Contains(const EPhysicalSurface SurfaceType) const
{
	for (const auto& PhysMat : PhysMatMap)
	{
		if (PhysMat.Value.SurfaceType == SurfaceType)
		{
			return true;
		}
	}
	return false;
}

bool UTBPhysMatManager::Contains(const UPhysicalMaterial* PhysicalMaterialAsset) const
{
	TB_RET_COND_VALUE(!PhysicalMaterialAsset, false);
	bool Success = false;
	const UTBConfiguration* TBConfig = GetDefault<UTBConfiguration>();
	if (const UTBPhysicalMaterialWithTags* PhysMatWithTags = Cast<UTBPhysicalMaterialWithTags>(PhysicalMaterialAsset))
	{
		GetFromPhysMatWithTags(PhysMatWithTags, Success);
	}
	else if (PhysicalMaterialAsset->Implements<UGameplayTagAssetInterface>())
	{
		FGameplayTagContainer TagContainer;
		Cast<IGameplayTagAssetInterface>(PhysicalMaterialAsset)->GetOwnedGameplayTags(TagContainer);
		if (TagContainer.HasTag(TB::PhysMatConstants::PhysMatTag()))
		{
			TArray<FGameplayTag> Tags;
			TagContainer.Filter(FGameplayTagContainer(TB::PhysMatConstants::PhysMatTag())).GetGameplayTagArray(Tags);
			if (Tags.Num() > 0)
			{
				FGameplayTag TagToSearchWith = Tags[0];
				if (Tags.Num() > 1)
				{
					TagToSearchWith = *Tags.FindByPredicate([](const FGameplayTag& Tag) { return Tag.GetGameplayTagParents().HasTag(TB::PhysMatConstants::PhysMatTag()); });
				}
				Success = Contains(TagToSearchWith);
			}
		}
	}
	if (!Success) // The tag seems to have been invalid, try checking against surface mappings
	{
		for (const auto& PhysMat : TBConfig->PhysMats)
		{
			if (PhysMat.HasPhysicalMaterialAsset() && PhysMat.PhysicalMaterial.Get() == PhysicalMaterialAsset)
			{
				return true;
			}
		}
	}
	if (Success)
	{
		return true;
	}
	return false;
}

void UTBPhysMatManager::SyncWithConfig(const bool bUseConfig)
{
	UTBConfiguration* TBConfig = GetMutableDefault<UTBConfiguration>();
	if (bUseConfig)
	{
		PhysMatMap.Empty();
		for (const auto& PhysMat : TBConfig->PhysMats)
		{
			PhysMatMap.Add(PhysMat.PhysMatProperties.GameplayTag, PhysMat.PhysMatProperties);
		}
		PresetProperties = FTBPresetMaterialProperties(PhysMatMap);
		SurfaceTypeFlesh.Emplace(PresetProperties.Flesh.Get(TB::PhysMatConstants::DefaultPhysMats::Flesh()).SurfaceType);
		SurfaceTypeBone.Emplace(PresetProperties.Bone.Get(TB::PhysMatConstants::DefaultPhysMats::Bone()).SurfaceType);
	}
	else if (PhysMatMap.Num() > 0)
	{
		TBConfig->bIsUpdatingPhysMatMapInternally = true;
		const auto& MaterialArr = TB::PhysMatConstants::DefaultPhysMats::GetDefaults();
		TArray<FTBPhysMatMapping> MappingArr;
		MappingArr.Reserve(MaterialArr.Num());
		for (const auto& PhysMat : MaterialArr)
		{
			FTBPhysMatMapping* FoundMapping = TBConfig->PhysMats.FindByPredicate([PhysMat](const FTBPhysMatMapping& Mapping) {return Mapping.PhysMatProperties == PhysMat; });
			if (FoundMapping)
			{
				FoundMapping->PhysMatProperties = PhysMat;
			}
			else
			{
				TBConfig->PhysMats.Emplace(PhysMat);
			}
		}
		TBConfig->bIsUpdatingPhysMatMapInternally = false;
	}
}

void UTBPhysMatManager::Add(const FPhysMatProperties& ToAdd, const bool CheckIfExists)
{
	if (ToAdd.GameplayTag.IsValid())
	{
		if (CheckIfExists && PhysMatMap.Contains(ToAdd.GameplayTag))
		{
			return;
		}
		else
		{
			PhysMatMap.Emplace(ToAdd.GameplayTag, ToAdd);
		}
	}
}

void UTBPhysMatManager::AddFromDataTable(const TObjectPtr<UDataTable> DataTable)
{
	TArray<FPhysMatProperties*> TableRows;
	DataTable->GetAllRows(FString(), TableRows);
	for (const auto& Row : TableRows)
	{
		if (Row)
		{
			Add(*Row);
		}
	}
}

FGameplayTag UTBPhysMatManager::GetKey(const FPhysMatProperties& Properties, bool& Found) const
{
	const FGameplayTag* Key = PhysMatMap.FindKey(Properties);
	if (Key)
	{
		Found = true;
		return *Key;
	}
	else
	{
		Found = false;
		return FGameplayTag();
	}
}

UPhysicalMaterial* UTBPhysMatManager::GetPhysicalMaterialForPhysMat(const FPhysMatProperties& Properties) const
{
	const auto TBConfig = GetDefault<UTBConfiguration>();
	const FTBPhysMatMapping* FoundMapping = TBConfig->PhysMats.FindByPredicate([Properties](const FTBPhysMatMapping& Mapping) { return Mapping.PhysMatProperties == Properties; });
	if (FoundMapping)
	{
		const auto Mat = FoundMapping->PhysicalMaterial.LoadSynchronous();
		return Mat;
	}
	return nullptr;
}

UTBPhysMatManager* UPhysMatManagerBPLibrary::GetPhysMatManager()
{
	return UTBPhysMatManager::GetIfValid();
}
