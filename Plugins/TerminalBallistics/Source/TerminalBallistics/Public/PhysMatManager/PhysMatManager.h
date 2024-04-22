#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Misc/Optional.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PhysMat.h"
#include "UObject/Object.h"

#include "PhysMatManager.generated.h"

UCLASS()
class TERMINALBALLISTICS_API UTBPhysicalMaterialWithTags : public UPhysicalMaterial, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:

	UTBPhysicalMaterialWithTags(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get())
		: Super(ObjectInitializer)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PhysicalProperties, meta = (Categories = "PhysMat"))
	FGameplayTag Tag;

	// The name of this physical material. Usually maps to a row in the MaterialPropertiesTable.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PhysicalProperties)
	FName PhysicalMaterialName;

	// Inherited via IGameplayTagAssetInterface
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = Tag.GetSingleTagContainer(); }

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PhysicalProperties, meta = (ToolTip = "When true, \"PhysicalMaterialName\" and \"Tag\" will be automatically filled in when \"SurfaceType\" is changed."))
	bool bAutofill = true;
#endif
#if WITH_EDITOR
	inline virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override
	{
		if (bAutofill && PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UTBPhysicalMaterialWithTags, SurfaceType))
		{
			const auto PhysicsSettings = UPhysicsSettings::Get();
			const TArray<FPhysicalSurfaceName> SurfaceNames = PhysicsSettings->PhysicalSurfaces;
			const FPhysicalSurfaceName* Name = SurfaceNames.FindByPredicate([type = SurfaceType](const FPhysicalSurfaceName& name) {return name.Type == type; });
			if (Name)
			{
				PhysicalMaterialName = Name->Name;
				const auto tagStr = TEXT("PhysMat.")+(PhysicalMaterialName.ToString());
				Tag = FGameplayTag::RequestGameplayTag(FName(tagStr));
			}
		}
		Super::PostEditChangeProperty(PropertyChangedEvent);
	}
#endif
};

class UDataTable;

struct FTBPresetMaterialProperties
{
	mutable TOptional<FPhysMatProperties> Acrylic;
	mutable TOptional<FPhysMatProperties> Aluminum;
	mutable TOptional<FPhysMatProperties> Asphalt;
	mutable TOptional<FPhysMatProperties> Bone;
	mutable TOptional<FPhysMatProperties> Brass;
	mutable TOptional<FPhysMatProperties> Brick;
	mutable TOptional<FPhysMatProperties> CarbonFiber;
	mutable TOptional<FPhysMatProperties> CastIron;
	mutable TOptional<FPhysMatProperties> Ceramic;
	mutable TOptional<FPhysMatProperties> Concrete;
	mutable TOptional<FPhysMatProperties> Copper;
	mutable TOptional<FPhysMatProperties> Drywall;
	mutable TOptional<FPhysMatProperties> Flesh;
	mutable TOptional<FPhysMatProperties> Glass;
	mutable TOptional<FPhysMatProperties> HardenedSteel;
	mutable TOptional<FPhysMatProperties> HardPlastic;
	mutable TOptional<FPhysMatProperties> Hardwood;
	mutable TOptional<FPhysMatProperties> Ice;
	mutable TOptional<FPhysMatProperties> Kevlar;
	mutable TOptional<FPhysMatProperties> Lead;
	mutable TOptional<FPhysMatProperties> Limestone;
	mutable TOptional<FPhysMatProperties> Paper;
	mutable TOptional<FPhysMatProperties> Plastic;
	mutable TOptional<FPhysMatProperties> Polystyrene;
	mutable TOptional<FPhysMatProperties> Rubber;
	mutable TOptional<FPhysMatProperties> Softwood;
	mutable TOptional<FPhysMatProperties> Steel;
	mutable TOptional<FPhysMatProperties> Titanium;
	mutable TOptional<FPhysMatProperties> TungstenCarbide;

	mutable bool bHasBeenInitialized = false;

	FTBPresetMaterialProperties() = default;
	FTBPresetMaterialProperties(const TMap<FGameplayTag, FPhysMatProperties>& PhysMatMap);

	void Init(const TMap<FGameplayTag, FPhysMatProperties>& PhysMatMap) const;

	static FTBPresetMaterialProperties GetDefaults();
};

UCLASS(Config=Game, DefaultConfig)
class TERMINALBALLISTICS_API UTBPhysMatManager : public UObject
{
	GENERATED_BODY()
public:
	FORCEINLINE static UTBPhysMatManager& Get()
	{
		if (Singleton == nullptr)
		{
			Initialize();
		}
		return *Singleton;
	}
	FORCEINLINE static UTBPhysMatManager* GetIfValid()
	{
		return Singleton;
	}

	~UTBPhysMatManager() noexcept
	{
		Singleton = nullptr;
	}

	FPhysMatProperties GetFromTag(const FGameplayTag& Tag) const;
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|PhysMatManager")
	FPhysMatProperties GetFromTag(const FGameplayTag& Tag, bool& Success) const;

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|PhysMatManager")
	FPhysMatProperties GetFromSurfaceType(TEnumAsByte<EPhysicalSurface> SurfaceType, bool& Success) const;

	FPhysMatProperties GetFromPhysicalMaterial(UPhysicalMaterial* Material, bool& Success) const;

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|PhysMatManager")
	FPhysMatProperties GetFromPhysMatWithTags(const UTBPhysicalMaterialWithTags* Material, bool& Success) const;

	FPhysMatProperties GetFromHitResult(const FHitResult& HitResult) const;
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|PhysMatManager")
	FPhysMatProperties GetFromHitResult(const FHitResult& HitResult, bool& Success) const;

	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|PhysMatManager")
	// Attempts to add a new mapping. Will return false if it already exists.
	bool AddEntry(const FGameplayTag& Tag, const FPhysMatProperties& Properties);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|PhysMatManager")
	// Attempts to update an existing mapping. Will return false if that mapping does not exist.
	bool UpdateEntry(const FGameplayTag& Tag, const FPhysMatProperties& NewProperties);
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|PhysMatManager")
	// Adds a new mapping, or updates an existing one.
	void AddOrUpdateEntry(const FGameplayTag& Tag, const FPhysMatProperties& Properties);

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|PhysMatManager")
	TArray<FPhysMatProperties> GetAllMaterials() const;

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|PhysMatManager")
	TEnumAsByte<EPhysicalSurface> GetSurfaceTypeFlesh() const;
	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|PhysMatManager")
	TEnumAsByte<EPhysicalSurface> GetSurfaceTypeBone() const;

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|PhysMatManager")
	bool Contains(const FGameplayTag& Tag) const;
	bool Contains(const EPhysicalSurface SurfaceType) const;
	bool Contains(const UPhysicalMaterial* PhysicalMaterialAsset) const;

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|PhysMatManager")
	const TMap<FGameplayTag, FPhysMatProperties> GetMap() { return PhysMatMap; }

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|PhysMatManager")
	FGameplayTag GetKey(const FPhysMatProperties& Properties, bool& Found) const;

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|PhysMatManager")
	UPhysicalMaterial* GetPhysicalMaterialForPhysMat(const FPhysMatProperties& Properties) const;

	UPROPERTY()
	TArray<FSoftObjectPath> DataTablePathsToLoad;
	UPROPERTY()
	TMap<FGameplayTag, FPhysMatProperties> PhysMatMap;

	static FTBPresetMaterialProperties GetPropertiesForPresets();

	void CallOrRegister_OnInitialized(FSimpleMulticastDelegate::FDelegate&& Delegate);

	void SetToDefaults();
private:
	mutable FTBPresetMaterialProperties PresetProperties;
	mutable bool bIsInitialized = false;

	mutable TOptional<EPhysicalSurface> SurfaceTypeFlesh;
	mutable TOptional<EPhysicalSurface> SurfaceTypeBone;

	static FSimpleMulticastDelegate OnInitialized();

	void BuildMap();

	void SyncWithConfig(const bool bUseConfig = true);

	void Add(const FPhysMatProperties& ToAdd, const bool bCheckIfExists = false);
	void AddFromDataTable(const TObjectPtr<UDataTable> DataTable);

	static UTBPhysMatManager* Singleton;

	static void Initialize();

	void LoadMaterialPropertyTables(bool bAllowAsyncLoad);

	void LoadDeprecatedMappings();

	void DataTablesUpdated(const TArray<FSoftObjectPath>& NewTables);
	void PhysMatsUpdated(const TArray<struct FTBPhysMatMapping>& NewPhysMats);

	UPROPERTY()
	TArray<TObjectPtr<UDataTable>> MaterialPropertyTables;
};

UCLASS()
class TERMINALBALLISTICS_API UPhysMatManagerBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Terminal Ballistics|PhysMatManager", meta = (UnsafeDuringActorConstruction))
	static UTBPhysMatManager* GetPhysMatManager();
};
