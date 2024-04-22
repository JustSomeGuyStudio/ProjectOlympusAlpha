// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "Engine/DataTable.h"
#include "Subsystems/EngineSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "TerminalBallisticsEngineSubsystem.generated.h"


class UTBConfiguration;
struct FPhysMatProperties;


UCLASS()
class TERMINALBALLISTICS_API UTBEngineSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	UTBEngineSubsystem()
		: UEngineSubsystem()
		, bInitialized(false)
		, bAssetManagerCreated(false)
		, bTagManagerIsDoneAddingNativeTags(false)
	{}

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	* Loads physical material configuration.
	*/
	void LoadPhysMatConfig();

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Subsystems|Engine")
	const UTBConfiguration* GetTBConfig();

	UFUNCTION(BlueprintPure, Category = "Terminal Ballistics|Subsystems|Engine")
	inline bool HasLoaded() const
	{
		return bInitialized;
	}

	void CallOrRegister_OnFullyInitialized(FSimpleMulticastDelegate::FDelegate&& Delegate);

protected:
	FSimpleMulticastDelegate OnFullyInitializedDelegate;

	bool CanInitialize() const;
	
private:
	friend class UTerminalBallisticsSubsystem;
	friend class UTBConfiguration;
	friend class UTBPhysMatManager;

	bool bInitialized = false;
	bool bAssetManagerCreated = false;
	bool bTagManagerIsDoneAddingNativeTags = false;

	UTBConfiguration* TBConfiguration = nullptr;

	void Init();
	void SetDefaultPhysMat(const struct FGameplayTag PhysMatTag);

	UFUNCTION()
	void SetupBullets();
};
