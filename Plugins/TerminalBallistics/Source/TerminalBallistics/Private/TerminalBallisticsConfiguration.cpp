// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "TerminalBallisticsConfiguration.h"
#include "Engine/Engine.h"
#include "PhysMatManager/PhysMat.h"
#include "Subsystems/TerminalBallisticsEngineSubsystem.h"
#include "Threading/GraphTasks/GraphTasks.h"



const TAutoConsoleVariable<bool> CVarBallisticsDebugDraw(
	TEXT("tb.Ballistics.DrawImpactDebug"),
	false,
	TEXT("Whether or not to draw debug information regarding projectile penetration"),
	ECVF_Cheat
);
const TAutoConsoleVariable<bool> CVarBallisticsDebugDrawRealtime(
	TEXT("tb.Ballistics.RealtimeDebugDraw"),
	false,
	TEXT("If true, ballistics debug drawing will be done in realtime."),
	ECVF_Cheat
);
const TAutoConsoleVariable<bool> CVarPrintBallisticsDebugInfo(
	TEXT("tb.Ballistics.PrintBallisticsDebugInfo"),
	false,
	TEXT("Whether or not to print ballistics debug info"),
	ECVF_Cheat
);

FTBPhysMatMapping FTBPhysMatMapping::GetDefault()
{
	const auto& DefaultPhysMat = FPhysMatProperties::GetDefault();
	return FTBPhysMatMapping(TB::PhysMatConstants::DefaultPhysMats::GetDefaultPhysicalMaterialAssetForPhysMat(DefaultPhysMat), DefaultPhysMat);
}

void UTBConfiguration::PostInitProperties()
{
	Super::PostInitProperties();

	if (DefaultDragComplexity == ETBDragComplexity::DEFAULT)
	{
		DefaultDragComplexity = ETBDragComplexity::BASIC;
	}

	UpdateConfigValues();
}

FName UTBConfiguration::GetCategoryName() const
{
	return FName(TEXT("Plugins"));
}

void UTBConfiguration::UpdateConfigValues()
{
	TB::Configuration::bIgnoreImpactEventsWithInvalidData = bIgnoreImpactEventsWithInvalidData;
	TB::Configuration::ProjectileTraceChannel = ProjectileTraceChannel;
	TB::Configuration::PenetrationComplexity = PenetrationComplexity;
	TB::Configuration::bEnableRicochets = bEnableRicochets;
	TB::Configuration::bEnableProjectileRicochets = bEnableProjectileRicochets;
	TB::Configuration::bEnableBulletRicochets = bEnableBulletRicochets;
	TB::Configuration::RicochetEnergyRatioThreshold = RicochetEnergyRatioThreshold;
	TB::Configuration::RicochetAngleCutoff = RicochetGlobalAngleCutoff;
	TB::Configuration::MaximumProjectileSimulationDuration = MaximumProjectileSimulationDuration;
	TB::Configuration::MaxPenetrations = MaxProjectilePenetrations;
	TB::Configuration::MinPenetrationDepth = MinimumPenetrationDepth;
	TB::Configuration::bEnableBulletDeformation = bEnableBulletDeformation;
	TB::Configuration::MaxLaunchesPerTick = MaximumProjectileLaunchesPerTick;
	TB::Configuration::ThreadResponseConsumtionRate = ThreadResponseConsumtionRate;
	TB::Configuration::ThreadTickFrequency = ThreadTickFrequency;
	TB::Configuration::MaximumProjectileThreads = MaximumProjectileThreads;
	TB::Configuration::DefaultThreadUpdateCallbackInterval = DefaultThreadUpdateCallbackInterval;
	TB::Configuration::ExitCodeLogLevel = static_cast<int32>(ExitCodeLogVerbosity);
	TB::Configuration::ExitCodeLogFilter = static_cast<int32>(ExitCodeLogFilter);
	TB::Configuration::WindSpeedMultiplier = WindSpeedMultiplier;
	TB::Configuration::CVarFlyByTraceRadius->Set(FlyByTraceRadius);
}

#if WITH_EDITOR
void UTBConfiguration::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static const FName NAME_TBDefaultPhysMat = GET_MEMBER_NAME_CHECKED(UTBConfiguration, DefaultPhysicalMaterial);
	static const FName NAME_TBProjectileTraceChannel = GET_MEMBER_NAME_CHECKED(UTBConfiguration, ProjectileTraceChannel);
	static const FName NAME_TBPenetrationComplexity = GET_MEMBER_NAME_CHECKED(UTBConfiguration, ProjectileTraceChannel);
	static const FName NAME_TBDefaultThreadUpdateCallbackInterval = GET_MEMBER_NAME_CHECKED(UTBConfiguration, DefaultThreadUpdateCallbackInterval);
	static const FName NAME_TBEnableRicochets = GET_MEMBER_NAME_CHECKED(UTBConfiguration, bEnableRicochets);
	static const FName NAME_TBEnableBulletRicochets = GET_MEMBER_NAME_CHECKED(UTBConfiguration, bEnableBulletRicochets);
	static const FName NAME_TBEnableProjectileRicochets = GET_MEMBER_NAME_CHECKED(UTBConfiguration, bEnableProjectileRicochets);
	static const FName NAME_TBRicochetEnergyRatioThreshold = GET_MEMBER_NAME_CHECKED(UTBConfiguration, RicochetEnergyRatioThreshold);
	static const FName NAME_TBRicochetRicochetGlobalAngleCutoff = GET_MEMBER_NAME_CHECKED(UTBConfiguration, RicochetGlobalAngleCutoff);
	static const FName NAME_TBEnableBulletDeformation = GET_MEMBER_NAME_CHECKED(UTBConfiguration, bEnableBulletDeformation);
	static const FName NAME_TBMaximumProjectileSimulationDuration = GET_MEMBER_NAME_CHECKED(UTBConfiguration, MaximumProjectileSimulationDuration);
	static const FName NAME_TBMaxProjectilePenetrations = GET_MEMBER_NAME_CHECKED(UTBConfiguration, MaxProjectilePenetrations);
	static const FName NAME_TBMinimumPenetrationDepth = GET_MEMBER_NAME_CHECKED(UTBConfiguration, MinimumPenetrationDepth);
	static const FName NAME_TBMaximumProjectileLaunchesPerTick = GET_MEMBER_NAME_CHECKED(UTBConfiguration, MaximumProjectileLaunchesPerTick);
	static const FName NAME_TBThreadResponseConsumtionRate = GET_MEMBER_NAME_CHECKED(UTBConfiguration, ThreadResponseConsumtionRate);
	static const FName NAME_TBThreadTickFrequency = GET_MEMBER_NAME_CHECKED(UTBConfiguration, ThreadTickFrequency);
	static const FName NAME_TBMaximumProjectileThreads = GET_MEMBER_NAME_CHECKED(UTBConfiguration, MaximumProjectileThreads);
	static const FName NAME_TBExitCodeLogVerbosity = GET_MEMBER_NAME_CHECKED(UTBConfiguration, ExitCodeLogVerbosity);
	static const FName NAME_TBExitCodeLogFilter = GET_MEMBER_NAME_CHECKED(UTBConfiguration, ExitCodeLogFilter);
	static const FName NAME_TBAdditionalDataTables = GET_MEMBER_NAME_CHECKED(UTBConfiguration, AdditionalDataTablesToLoad);
	static const FName NAME_TBPhysMats = GET_MEMBER_NAME_CHECKED(UTBConfiguration, PhysMats);
	static const FName NAME_TBFlyBy = GET_MEMBER_NAME_CHECKED(UTBConfiguration, FlyByTraceRadius);

	UTBEngineSubsystem* TBEngineSubsystem = GEngine->GetEngineSubsystem<UTBEngineSubsystem>();
	if (PropertyChangedEvent.MemberProperty)
	{
		const FName Name = PropertyChangedEvent.MemberProperty->GetFName();
		if (Name == NAME_TBDefaultPhysMat)
		{
			TBEngineSubsystem->SetDefaultPhysMat(GetDefaultPhysicalMaterial());
		}
		else if (Name == NAME_TBProjectileTraceChannel)
		{
			TB::Configuration::ProjectileTraceChannel = ProjectileTraceChannel;
		}
		else if (Name == NAME_TBPenetrationComplexity)
		{
			TB::Configuration::PenetrationComplexity = PenetrationComplexity;
		}
		else if (Name == NAME_TBDefaultThreadUpdateCallbackInterval)
		{
			TB::Configuration::DefaultThreadUpdateCallbackInterval = DefaultThreadUpdateCallbackInterval;
		}
		else if (Name == NAME_TBEnableRicochets)
		{
			TB::Configuration::CVarEnableRicochets->Set(bEnableRicochets, ECVF_SetByProjectSetting);
		}
		else if (Name == NAME_TBEnableBulletRicochets)
		{
			TB::Configuration::CVarEnableBulletRicochets->Set(bEnableBulletRicochets, ECVF_SetByProjectSetting);
		}
		else if (Name == NAME_TBEnableProjectileRicochets)
		{
			TB::Configuration::CVarEnableProjectileRicochets->Set(bEnableProjectileRicochets, ECVF_SetByProjectSetting);
		}
		else if (Name == NAME_TBRicochetEnergyRatioThreshold)
		{
			TB::Configuration::CVarRicochetEnergyRatioThreshold->Set(RicochetEnergyRatioThreshold, ECVF_SetByProjectSetting);
		}
		else if (Name == NAME_TBRicochetRicochetGlobalAngleCutoff)
		{
			TB::Configuration::CVarRicochetAngleCutoff->Set(RicochetGlobalAngleCutoff, ECVF_SetByProjectSetting);
		}
		else if (Name == NAME_TBEnableBulletDeformation)
		{
			TB::Configuration::CVarEnableBulletDeformation->Set(bEnableBulletDeformation, ECVF_SetByProjectSetting);
		}
		else if (Name == NAME_TBMaximumProjectileSimulationDuration)
		{
			TB::Configuration::CVarMaxSimTime->Set(MaximumProjectileSimulationDuration, ECVF_SetByProjectSetting);
		}
		else if (Name == NAME_TBMaxProjectilePenetrations)
		{
			TB::Configuration::CVarMaxPenetrations->Set(MaxProjectilePenetrations, ECVF_SetByProjectSetting);
		}
		else if (Name == NAME_TBMinimumPenetrationDepth)
		{
			TB::Configuration::CVarMinPenetrationDepth->Set(MinimumPenetrationDepth, ECVF_SetByProjectSetting);
		}
		else if (Name == NAME_TBMaximumProjectileLaunchesPerTick)
		{
			TB::Configuration::CVarMaxLaunchesPerTick->Set(MaximumProjectileLaunchesPerTick, ECVF_SetByProjectSetting);
		}
		else if (Name == NAME_TBThreadResponseConsumtionRate)
		{
			TB::Configuration::CVarThreadResponseConsumtionRate->Set(ThreadResponseConsumtionRate, ECVF_SetByProjectSetting);
		}
		else if (Name == NAME_TBThreadTickFrequency)
		{
			TB::Configuration::CVarThreadTickFrequency->Set(ThreadTickFrequency, ECVF_SetByProjectSetting);
		}
		else if (Name == NAME_TBMaximumProjectileThreads)
		{
			TB::Configuration::CVarMaxProjectileThreads->Set(MaxProjectilePenetrations, ECVF_SetByProjectSetting);
		}
		else if (Name == NAME_TBExitCodeLogVerbosity)
		{
			TB::Configuration::CVarExitCodeLogLevel->Set(static_cast<int32>(ExitCodeLogVerbosity), ECVF_SetByProjectSetting);
		}
		else if (Name == NAME_TBExitCodeLogFilter)
		{
			TB::Configuration::CVarExitCodeLogFilter->Set(static_cast<int32>(ExitCodeLogFilter), ECVF_SetByProjectSetting);
		}
		else if (Name == NAME_TBAdditionalDataTables)
		{
			OnDataTablesChanged().Broadcast(AdditionalDataTablesToLoad);
		}
		else if (Name == NAME_TBPhysMats && !bIsUpdatingPhysMatMapInternally)
		{
			BroadcastPhysMatsChanged();
		}
		else if (Name == NAME_TBFlyBy)
		{
			TB::Configuration::CVarFlyByTraceRadius->Set(FlyByTraceRadius);
		}

		UpdateConfigValues();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UTBConfiguration::Register_OnPhysMatsChanged(TMulticastDelegate<void(const TArray<FTBPhysMatMapping>&)>::FDelegate Delegate)
{
	bool bAlreadyBound = Delegate.GetUObject() != nullptr ? OnPhysMatsChanged().IsBoundToObject(Delegate.GetUObject()) : false;
	if (!bAlreadyBound)
	{
		OnPhysMatsChanged().Add(Delegate);
	}
}

void UTBConfiguration::Register_OnDataTablesChanged(TMulticastDelegate<void(const TArray<FSoftObjectPath>&)>::FDelegate Delegate)
{
	bool bAlreadyBound = Delegate.GetUObject() != nullptr ? OnDataTablesChanged().IsBoundToObject(Delegate.GetUObject()) : false;
	if (!bAlreadyBound)
	{
		OnDataTablesChanged().Add(Delegate);
	}
}

TMulticastDelegate<void(const TArray<FTBPhysMatMapping>&)>& UTBConfiguration::OnPhysMatsChanged()
{
	static TMulticastDelegate<void(const TArray<FTBPhysMatMapping>&)> Delegate;
	return Delegate;
}

TMulticastDelegate<void(const TArray<FSoftObjectPath>&)>& UTBConfiguration::OnDataTablesChanged()
{
	static TMulticastDelegate<void(const TArray<FSoftObjectPath>&)> Delegate;
	return Delegate;
}

TArray<FString> UTBConfiguration::GetPhysicalMaterialOptions()
{
	/*using namespace TB::Tags;

	if (FTerminalBallisticsTags::Get().bHasBeenInitialized)
	{
		FGameplayTagContainer Container = UGameplayTagsManager::Get().RequestGameplayTagChildren(FTerminalBallisticsTags::Get().PhysMatTag);
		TArray<FString> OutArr;
		OutArr.Reserve(Container.Num());
		for (const FGameplayTag& Tag : Container)
		{
			OutArr.Add(Tag.ToString());
		}
		return OutArr;
	}*/
	TArray<FString> OutArr;
	for (const auto& PhysMat : PhysMats)
	{
		OutArr.Add(PhysMat.PhysMatProperties.GameplayTag.ToString());
	}
	return OutArr;
}

void UTBConfiguration::BroadcastPhysMatsChanged()
{
	bIsUpdatingPhysMatMapInternally = true;
	/* Ensure the materials used in hardcoded objects (mainly bullets) are available */
	using namespace TB::PhysMatConstants;

	const auto& Lead = DefaultPhysMats::Lead();
	const auto& HardenedSteel = DefaultPhysMats::HardenedSteel();
	using TB::PhysMatConstants::DefaultPhysMats::GetDefaultPhysicalMaterialAssetForPhysMat;
	if (!PhysMats.Contains(Lead.GameplayTag))
	{
		PhysMats.Add(FTBPhysMatMapping(GetDefaultPhysicalMaterialAssetForPhysMat(Lead), Lead));
	}
	if (!PhysMats.Contains(HardenedSteel.GameplayTag))
	{
		PhysMats.Add(FTBPhysMatMapping(GetDefaultPhysicalMaterialAssetForPhysMat(HardenedSteel), HardenedSteel));
	}
	OnPhysMatsChanged().Broadcast(PhysMats);
	bIsUpdatingPhysMatMapInternally = false;
}

namespace TB::Configuration
{
	ECollisionChannel ProjectileTraceChannel = ECC_GameTraceChannel1;
	ETBPenetrationComplexity PenetrationComplexity = ETBPenetrationComplexity::Complex;

	bool bEnableRicochets = true;
	FAutoConsoleVariableRef CVarEnableRicochets(
		TEXT("tb.Ballistics.Ricochet.All"),
		bEnableRicochets,
		TEXT("Enables or disables ricochets entirely."),
		ECVF_Cheat
	);

	bool bEnableProjectileRicochets = true;
	FAutoConsoleVariableRef CVarEnableProjectileRicochets(
		TEXT("tb.Ballistics.Ricochet.Projectiles"),
		bEnableProjectileRicochets,
		TEXT("Whether or not projectiles (not bullets) can ricochet."),
		ECVF_Cheat
	);

	bool bEnableBulletRicochets = true;
	FAutoConsoleVariableRef CVarEnableBulletRicochets(
		TEXT("tb.Ballistics.Ricochet.Bullets"),
		bEnableBulletRicochets,
		TEXT("Whether or not bullets can ricochet."),
		ECVF_Cheat
	);

	float MaximumProjectileSimulationDuration = 30.f;
	FAutoConsoleVariableRef CVarMaxSimTime(
		TEXT("tb.Ballistics.MaxSimTime"),
		MaximumProjectileSimulationDuration,
		TEXT("Maximum duration in seconds that a projectile can be simulated."),
		ECVF_Cheat
	);

	int32 MaxPenetrations = 25;
	FAutoConsoleVariableRef CVarMaxPenetrations(
		TEXT("tb.Ballistics.MaxPenetrations"),
		MaxPenetrations,
		TEXT("Maximum number of objects that a projectile can penetrate before it is destroyed."),
		ECVF_Cheat
	);

	float MinPenetrationDepth = 1e-3f;
	FAutoConsoleVariableRef CVarMinPenetrationDepth(
		TEXT("tb.Ballistics.MinPenDepth"),
		MinPenetrationDepth,
		TEXT("Minimum penetration depth allowed before simulation is skipped."),
		ECVF_Cheat
	);

	bool bEnableBulletDeformation = false;
	FAutoConsoleVariableRef CVarEnableBulletDeformation(
		TEXT("tb.Ballistics.BulletDeformation"),
		bEnableBulletDeformation,
		TEXT("Enables bullet deformation. EXPERIMENTAL"),
		ECVF_Cheat
	);

	int32 MaxLaunchesPerTick = 500;
	FAutoConsoleVariableRef CVarMaxLaunchesPerTick(
		TEXT("tb.Ballistics.MaxLaunchPerTick"),
		MaxLaunchesPerTick,
		TEXT("Maxmimum number of projectiles that can be launched in a single tick."),
		ECVF_Cheat
	);

	int32 ThreadResponseConsumtionRate = 100;
	FAutoConsoleVariableRef CVarThreadResponseConsumtionRate(
		TEXT("tb.Thread.ResponseConsumtionRate"),
		ThreadResponseConsumtionRate,
		TEXT("Maximum number of results that will be recieved from the projectile thread each tick.\n")
		TEXT("Incrasing this can heavily impact performance."),
		ECVF_Cheat
	);

	float ThreadTickFrequency = 120.f;
	FAutoConsoleVariableRef CVarThreadTickFrequency(
		TEXT("tb.Thread.TickFrequency"),
		ThreadTickFrequency,
		TEXT("Frequency at which the projectile thread ticks.\n")
		TEXT("ex: A frequency of 60 would result in the projectile thread ticking 60 times per second."),
		ECVF_Cheat
	);

	int32 MaximumProjectileThreads = 10;
	FAutoConsoleVariableRef CVarMaxProjectileThreads(
		TEXT("tb.Thread.MaxThreads"),
		MaximumProjectileThreads,
		TEXT("Maximum number of background threads that can be created and used for projectile simulation.\n")
		TEXT("Note: The typical number of threads will be lower than this. This is only a maximum."),
		ECVF_Cheat
	);

	int32 DefaultThreadUpdateCallbackInterval = 1;

	int32 ExitCodeLogLevel = 0;
	EExitCodeLogLevel GetExitCodeLogLevel()
	{
		return static_cast<EExitCodeLogLevel>(ExitCodeLogLevel);
	}
	FAutoConsoleVariableRef CVarExitCodeLogLevel(
		TEXT("tb.ExitCodeLogLevel"),
		ExitCodeLogLevel,
		TEXT("Logging level for projectile task exit codes\n")
		TEXT("0: None\n")
		TEXT("1: Errors\n")
		TEXT("2: Verbose"),
		ECVF_Cheat
	);

	int32 ExitCodeLogFilter = 0;
	EExitCodeLogFilter GetExitCodeLogFilter()
	{
		return static_cast<EExitCodeLogFilter>(ExitCodeLogFilter);
	}
	FAutoConsoleVariableRef CVarExitCodeLogFilter(
		TEXT("tb.ExitCodeLogFilter"),
		ExitCodeLogFilter,
		TEXT("Used to filter which tasks produce logs.\n")
		TEXT("0: Bullet Tasks\n")
		TEXT("1: Projectile Tasks\n")
		TEXT("2: All Tasks"),
		ECVF_Cheat
	);


	float RicochetEnergyRatioThreshold = 2.f;
	FAutoConsoleVariableRef CVarRicochetEnergyRatioThreshold(
		TEXT("tb.ballistics.ricochet.energyRatioThreshold"),
		RicochetEnergyRatioThreshold,
		TEXT("For a ricochet to occur, kinetic energy must be at least {this value} * {imparted energy}\nThis prevents situations where projectiles have very little kinetic energy after a ricochet."),
		ECVF_Cheat
	);

	float RicochetAngleCutoff = 85.f;
	FAutoConsoleVariableRef CVarRicochetAngleCutoff(
		TEXT("tb.ballistics.ricochet.angleCutoff"),
		RicochetAngleCutoff,
		TEXT("Prevents ricochets from occuring when the angle of impact is greater than this value."),
		ECVF_Cheat
	);

	float FlyByTraceRadius = 200.f;
	FAutoConsoleVariableRef CVarFlyByTraceRadius(
		TEXT("tb.ballistics.FlyByTraceRadius"),
		FlyByTraceRadius,
		TEXT("How far a pawn can be from a bullet while still recieving the \"FlyBy\" event."),
		ECVF_Cheat
	);

	float WindSpeedMultiplier = 100.f;
};
