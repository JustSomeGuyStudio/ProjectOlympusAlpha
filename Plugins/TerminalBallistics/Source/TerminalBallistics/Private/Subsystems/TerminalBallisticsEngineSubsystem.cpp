// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "Subsystems/TerminalBallisticsEngineSubsystem.h"
#include "BulletDataAsset.h"
#include "Bullets.h"
#include "Engine/AssetManager.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PhysMatManager/PhysMatManager.h"
#include "TerminalBallisticsConfiguration.h"
#include "TerminalBallisticsLogChannels.h"
#include "TerminalBallisticsMacrosAndFunctions.h"
#include "TerminalBallisticsTags.h"


void UTBEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TB::Tags::FTerminalBallisticsTags::Get().InitializeNativeTags();


	/* Register delegates with tag manager and asset manager so we can initialize when they are both done initializing */

	FSimpleMulticastDelegate::FDelegate OnDoneAddingNativeTags;
	OnDoneAddingNativeTags.BindLambda([this]() { this->bTagManagerIsDoneAddingNativeTags = true; this->Init(); });
	UGameplayTagsManager::Get().CallOrRegister_OnDoneAddingNativeTagsDelegate(OnDoneAddingNativeTags);

	FSimpleMulticastDelegate::FDelegate OnAssetManagerCreated;
	OnAssetManagerCreated.BindLambda([this]() { this->bAssetManagerCreated = true; this->Init(); });
	UAssetManager::CallOrRegister_OnAssetManagerCreated(MoveTemp(OnAssetManagerCreated));
}

void UTBEngineSubsystem::LoadPhysMatConfig()
{
	if (!TBConfiguration)
	{
		TBConfiguration = GetMutableDefault<UTBConfiguration>();
	}

	if (TBConfiguration)
	{
		UTBPhysMatManager& PhysMatManager = UTBPhysMatManager::Get();

		bool Success;
		const FGameplayTag DefaultTag = TBConfiguration->GetDefaultPhysicalMaterial();
		FPhysMatProperties::DefaultPhysMat = PhysMatManager.GetFromTag(DefaultTag, Success);
		if (!Success)
		{
			UE_LOG(LogTerminalBallistics, Error, TEXT("Failed to set default PhysMat: [%s]"), *TBConfiguration->GetDefaultPhysicalMaterial().ToString());
			PhysMatManager.SetToDefaults();
			FPhysMatProperties::DefaultPhysMat = PhysMatManager.GetPropertiesForPresets().Concrete.GetValue();
		}
	}
}

const UTBConfiguration* UTBEngineSubsystem::GetTBConfig()
{
	return TBConfiguration;
}

void UTBEngineSubsystem::CallOrRegister_OnFullyInitialized(FSimpleMulticastDelegate::FDelegate&& Delegate)
{
	if (bInitialized)
	{
		Delegate.Execute();
	}
	else
	{
		OnFullyInitializedDelegate.Add(MoveTemp(Delegate));
	}
}

bool UTBEngineSubsystem::CanInitialize() const
{
	return !bInitialized
		&& bAssetManagerCreated
		&& bTagManagerIsDoneAddingNativeTags
		&& GEngine->IsInitialized();
}

void UTBEngineSubsystem::Init()
{
	if (CanInitialize())
	{
		bInitialized = true;
		TBConfiguration = GetMutableDefault<UTBConfiguration>();
		TBConfiguration->LoadConfig();

		UTBPhysMatManager& PhysMatManager = UTBPhysMatManager::Get();

		FSimpleMulticastDelegate::FDelegate OnPhysMatManagerInitialized;
		OnPhysMatManagerInitialized.BindLambda([this]()
		{
			LoadPhysMatConfig();
			SetupBullets();
			OnFullyInitializedDelegate.Broadcast();
		});
		PhysMatManager.CallOrRegister_OnInitialized(MoveTemp(OnPhysMatManagerInitialized));
	}
}

void UTBEngineSubsystem::SetDefaultPhysMat(const FGameplayTag PhysMatTag)
{
	TB_RET_COND(!bInitialized);
	UTBPhysMatManager& PhysMatManager = UTBPhysMatManager::Get();

	bool Success;
	FPhysMatProperties::DefaultPhysMat = PhysMatManager.GetFromTag(PhysMatTag, Success);
	if (!Success)
	{
		UE_LOG(LogTerminalBallistics, Error, TEXT("Failed to set default PhysMat: [%s]"), *TBConfiguration->GetDefaultPhysicalMaterial().ToString());
		PhysMatManager.SetToDefaults();
		FPhysMatProperties::DefaultPhysMat = PhysMatManager.GetPropertiesForPresets().Concrete.GetValue();
	}
}

void UTBEngineSubsystem::SetupBullets()
{
	using namespace TB::Bullets;
	BulletTypes::Init();
	FTBBullet::DefaultBullet = BulletTypes::Get().Bullet_9x19Para;
}
