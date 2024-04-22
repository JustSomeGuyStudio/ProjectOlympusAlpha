// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "HAL/IConsoleManager.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysMatManager/PhysMat.h"
#include "Types/TBEnums.h"
#include "TerminalBallisticsConfiguration.generated.h"



extern TERMINALBALLISTICS_API const TAutoConsoleVariable<bool> CVarBallisticsDebugDraw;
extern TERMINALBALLISTICS_API const TAutoConsoleVariable<bool> CVarBallisticsDebugDrawRealtime;
extern TERMINALBALLISTICS_API const TAutoConsoleVariable<bool> CVarPrintBallisticsDebugInfo;

USTRUCT(BlueprintType)
struct TERMINALBALLISTICS_API FEnvironmentData
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Environment Data", meta = (Units = "Meters"))
    // Altitude of the world at Z=0 (meters)
    double WorldAltitude = 0.0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Environment Data", meta = (Units = "Celsius"))
    // Temperature of the world at Z=0 (celsius)
    double WorldTemperature = 10.0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Environment Data", meta = (ClampMin = 0, ClampMax = 1, UIMin = 0, UIMax = 1))
    // Relative Humidity of the world at Z=0
    double WorldHumidity = 0.5;
};

USTRUCT(BlueprintType)
struct TERMINALBALLISTICS_API FTBPhysMatMapping
{
	GENERATED_BODY()
public:
	UPROPERTY(Config, BlueprintReadWrite, EditAnywhere, Category = Mapping, meta = (ToolTip = "PhysicalMaterial asset associated with this material.", ForceShowPluginContent, ForceShowEngineContent))
	TSoftObjectPtr<UPhysicalMaterial> PhysicalMaterial = nullptr;

	UPROPERTY(Config, BlueprintReadWrite, EditAnywhere, Category = Mapping)
	FPhysMatProperties PhysMatProperties;

	FTBPhysMatMapping() = default;

	FTBPhysMatMapping(const FPhysMatProperties& PhysMatProperties)
		: PhysMatProperties(PhysMatProperties)
	{}

	FTBPhysMatMapping(TSoftObjectPtr<UPhysicalMaterial> PhysicalMaterial, const FPhysMatProperties& PhysMatProperties)
		: PhysicalMaterial(PhysicalMaterial)
		, PhysMatProperties(PhysMatProperties)
	{}

	bool HasPhysicalMaterialAsset() const
	{
		if (!PhysicalMaterial.GetUniqueID().IsValid()) // Quick sanity check
		{
			return false;
		}

		if (UE::IsSavingPackage(nullptr) || IsGarbageCollecting()) // Calls to StaticFindObject will fail
		{
			return false;
		}
		if (PhysicalMaterial.IsNull())
		{
			return false;
		}
		else if (PhysicalMaterial.IsPending() || PhysicalMaterial.IsValid())
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	/* Two PhysMatMappings cannot have the same tag, nor can they use the same physical material (unless it isn't specified). */
	bool operator==(const FTBPhysMatMapping& rhs) const
	{
		return PhysMatProperties.GameplayTag == rhs.PhysMatProperties.GameplayTag
			|| (HasPhysicalMaterialAsset() && PhysicalMaterial == rhs.PhysicalMaterial);
	}

	bool operator==(const FGameplayTag& Tag) const
	{
		return PhysMatProperties.GameplayTag == Tag;
	}

	static FTBPhysMatMapping GetDefault();
};

UCLASS(Config=Game, DefaultConfig, meta = (DisplayName="Terminal Ballistics"))
class TERMINALBALLISTICS_API UTBConfiguration : public UDeveloperSettings
{
	GENERATED_BODY()
public:

	UTBConfiguration(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get())
		: UDeveloperSettings(ObjectInitializer)
	{
		LoadConfig();
	}

	UPROPERTY(Config, EditAnywhere, Category = "PhysicalMaterials", meta = (GetOptions = "GetPhysicalMaterialOptions"))
	FName DefaultPhysicalMaterial = FName("PhysMat.Concrete");

	FGameplayTag GetDefaultPhysicalMaterial() const { return FGameplayTag::RequestGameplayTag(DefaultPhysicalMaterial); }

	UPROPERTY(Config, EditAnywhere, Category = "Physical Materials", meta = (AllowedClasses = "/Script/Engine.DataTable", ToolTip = "Additional data tables containing materials to load."))
	TArray<FSoftObjectPath> AdditionalDataTablesToLoad;

	UPROPERTY(Config, EditAnywhere, Category = "Physical Materials", meta = (DisplayName = "Materials", NoElementDuplicate))
	TArray<FTBPhysMatMapping> PhysMats;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics")
	bool bIgnoreImpactEventsWithInvalidData = true;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ToolTip = "Controls whether simulations can be run by the client, server, or both."))
	ETBSimTaskNetMode SimulationNetMode = ETBSimTaskNetMode::Both;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ToolTip = "Collision Channel to use when tracing for projectile collision."))
	TEnumAsByte<ECollisionChannel> ProjectileTraceChannel = ECC_GameTraceChannel1;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics")
	ETBPenetrationComplexity PenetrationComplexity = ETBPenetrationComplexity::Complex;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (ValidEnumValues = "BASIC,ATMOSPHERIC,WIND,WIND3D,COMPLEX"))
	ETBDragComplexity DefaultDragComplexity = ETBDragComplexity::BASIC;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (DisplayName = "Default Simulation Type", ValidEnumValues = "Instant,Realtime"))
	ETBProjectileSimulationType DefaultSimType = ETBProjectileSimulationType::Realtime;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics")
	float FlyByTraceRadius = 200.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics")
	bool bEnableRicochets = true;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (EditCondition = "bEnableRicochets", EditConditionHides, ToolTip = "This controls whether or not projectiles (not bullets) can ricochet."))
	bool bEnableProjectileRicochets = true;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", meta = (EditCondition = "bEnableRicochets", EditConditionHides, ToolTip = "This controls whether or not bullets can ricochet."))
	bool bEnableBulletRicochets = true;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", AdvancedDisplay, meta = (ToolTip = "Enables bullet deformation. Caution: This is a work in progress!"))
	bool bEnableBulletDeformation = false;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", AdvancedDisplay, meta = (ToolTip = "For a ricochet to occur, kinetic energy must be at least {this value} * {imparted energy}\nThis prevents situations where projectiles have very little kinetic energy after a ricochet."))
	float RicochetEnergyRatioThreshold = 2.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", AdvancedDisplay, meta = (ToolTip = "Prevents ricochets from occuring when the angle of impact is greater than this value.", Units = "deg", UIMin = "0", UIMax = "90"))
	float RicochetGlobalAngleCutoff = 85.f;


	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", AdvancedDisplay, meta = (Min = "0.00001", ToolTip = "Maximum duration in seconds that a projectile can be simulated."))
	float MaximumProjectileSimulationDuration = 30.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", AdvancedDisplay, meta = (Min = "1", ToolTip = "Maximum number of objects that a projectile can penetrate before it is destroyed."))
	int32 MaxProjectilePenetrations = 25;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", AdvancedDisplay, meta = (Min = "0", ToolTip = "Minimum penetration depth allowed before simulation is skipped. 0 Means no limit."))
	float MinimumPenetrationDepth = 1e-3;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", AdvancedDisplay, meta = (Min = "1", ToolTip = "Maximum number of projectiles that can be launched in a single tick."))
	int32 MaximumProjectileLaunchesPerTick = 500;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", AdvancedDisplay, meta = (ToolTip = "Maximum number of results that will be recieved from the projectile thread each tick. Incrasing this can heavily impact performance."))
	int32 ThreadResponseConsumtionRate = 100;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", AdvancedDisplay, meta = (Min = "1", ToolTip = "Frequency at which the projectile thread ticks. (ex: A frequency of 60 would result in the projectile thread ticking 60 times per second)"))
	float ThreadTickFrequency = 120;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", AdvancedDisplay, meta = (Min = "1", ToolTip = "Maximum number of background threads that can be created and used for projectile simulation.\nNote: The typical number of threads will usually be lower than this. This is only a maximum."))
	int32 MaximumProjectileThreads = 10;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", AdvancedDisplay, meta = (Min = "0", ToolTip = "How many projectile updates between each broadcast of the \"update\" callback."))
	int32 DefaultThreadUpdateCallbackInterval = 1;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ballistics", AdvancedDisplay, meta = (Min = "8", Units = "kb", ToolTip = "Amount of memory to available to each worker thread. (KiB)\nWarning: Changing this value should be done with caution. Too little memory will result in a stack overflow.", ConsoleVariable = "tb.Thread.WorkerThreadStackSize"))
	int32 WorkerThreadStackSize = 128;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Logging", meta = (ToolTip = "Enables logging of projectile exit codes to the console and dictates verbosity. Projectile exit codes are generated when a projectile has finished simulation and they indicate the reason for termination."))
	EExitCodeLogLevel ExitCodeLogVerbosity = EExitCodeLogLevel::None;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Logging", meta = (ToolTip = "Controls whether bullets, projectiles, or both will log exit codes.", EditCondition = "ExitCodeLogVerbosity != EExitCodeLogLevel::None", EditConditionHides))
	EExitCodeLogFilter ExitCodeLogFilter = EExitCodeLogFilter::BulletTasks;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Environment")
	FEnvironmentData EnvironmentData;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Environment")
	float WindSpeedMultiplier = 100.f;

	static void Register_OnPhysMatsChanged(TMulticastDelegate<void(const TArray<FTBPhysMatMapping>&)>::FDelegate Delegate);
	static void Register_OnDataTablesChanged(TMulticastDelegate<void(const TArray<FSoftObjectPath>&)>::FDelegate Delegate);
public:
	virtual void PostInitProperties() override;
	virtual FName GetCategoryName() const override;

	static TMulticastDelegate<void(const TArray<FTBPhysMatMapping>&)>& OnPhysMatsChanged();
	static TMulticastDelegate<void(const TArray<FSoftObjectPath>&)>& OnDataTablesChanged();

	void UpdateConfigValues();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	bool bIsUpdatingPhysMatMapInternally = false;
protected:
	UFUNCTION(Category = "PhysicalMaterials")
	TArray<FString> GetPhysicalMaterialOptions();

	void BroadcastPhysMatsChanged();
};

namespace TB::Configuration
{
	extern TERMINALBALLISTICS_API ECollisionChannel ProjectileTraceChannel;
	extern TERMINALBALLISTICS_API ETBPenetrationComplexity PenetrationComplexity;
	extern TERMINALBALLISTICS_API bool bEnableRicochets;
	extern TERMINALBALLISTICS_API bool bEnableProjectileRicochets;
	extern TERMINALBALLISTICS_API bool bEnableBulletRicochets;
	extern TERMINALBALLISTICS_API float MaximumProjectileSimulationDuration;
	extern TERMINALBALLISTICS_API int32 MaxPenetrations;
	extern TERMINALBALLISTICS_API float MinPenetrationDepth;
	extern TERMINALBALLISTICS_API bool bEnableBulletDeformation;
	extern TERMINALBALLISTICS_API int32 MaxLaunchesPerTick;
	extern TERMINALBALLISTICS_API int32 ThreadResponseConsumtionRate;
	extern TERMINALBALLISTICS_API float ThreadTickFrequency;
	extern TERMINALBALLISTICS_API int32 MaximumProjectileThreads;
	extern TERMINALBALLISTICS_API int32 DefaultThreadUpdateCallbackInterval;
	extern TERMINALBALLISTICS_API int32 ExitCodeLogLevel;
	extern TERMINALBALLISTICS_API EExitCodeLogLevel GetExitCodeLogLevel();
	extern TERMINALBALLISTICS_API int32 ExitCodeLogFilter;
	extern TERMINALBALLISTICS_API EExitCodeLogFilter GetExitCodeLogFilter();
	extern TERMINALBALLISTICS_API float RicochetEnergyRatioThreshold;
	extern TERMINALBALLISTICS_API float RicochetAngleCutoff;
	extern TERMINALBALLISTICS_API float WindSpeedMultiplier;
	extern TERMINALBALLISTICS_API float FlyByTraceRadius;

	/* Console Variables */
	extern TERMINALBALLISTICS_API FAutoConsoleVariableRef CVarEnableRicochets;
	extern TERMINALBALLISTICS_API FAutoConsoleVariableRef CVarEnableBulletRicochets;
	extern TERMINALBALLISTICS_API FAutoConsoleVariableRef CVarEnableProjectileRicochets;
	extern TERMINALBALLISTICS_API FAutoConsoleVariableRef CVarRicochetEnergyRatioThreshold;
	extern TERMINALBALLISTICS_API FAutoConsoleVariableRef CVarRicochetAngleCutoff;
	extern TERMINALBALLISTICS_API FAutoConsoleVariableRef CVarEnableBulletDeformation;
	extern TERMINALBALLISTICS_API FAutoConsoleVariableRef CVarMaxSimTime;
	extern TERMINALBALLISTICS_API FAutoConsoleVariableRef CVarMaxPenetrations;
	extern TERMINALBALLISTICS_API FAutoConsoleVariableRef CVarMinPenetrationDepth;
	extern TERMINALBALLISTICS_API FAutoConsoleVariableRef CVarMaxLaunchesPerTick;
	extern TERMINALBALLISTICS_API FAutoConsoleVariableRef CVarThreadResponseConsumtionRate;
	extern TERMINALBALLISTICS_API FAutoConsoleVariableRef CVarThreadTickFrequency;
	extern TERMINALBALLISTICS_API FAutoConsoleVariableRef CVarMaxProjectileThreads;
	extern TERMINALBALLISTICS_API FAutoConsoleVariableRef CVarExitCodeLogLevel;
	extern TERMINALBALLISTICS_API FAutoConsoleVariableRef CVarExitCodeLogFilter;
	extern TERMINALBALLISTICS_API FAutoConsoleVariableRef CVarFlyByTraceRadius;
}
