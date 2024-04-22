// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/ImpactParams.h"
#include "Types/TBProjectile.h"
#include "TerminalBallisticsGameModeBaseInterface.generated.h"



UINTERFACE(MinimalAPI)
class UTerminalBallisticsGameModeBaseInterface : public UInterface
{
	GENERATED_BODY()
};

class TERMINALBALLISTICS_API ITerminalBallisticsGameModeBaseInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Terminal Ballistics|Game Mode")
	void ProjectileImpactEvent(const FTBImpactParamsBasic& impactParams);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Terminal Ballistics|Game Mode")
	void ProjectileExitEvent(const FTBImpactParamsBasic& impactParams);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Terminal Ballistics|Game Mode")
	void ProjectileInjureEvent(const FTBImpactParamsBasic& impactParams, const FTBProjectileInjuryParams& injuryParams);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Terminal Ballistics|Game Mode")
	void BulletImpactEvent(const FTBImpactParams& impactParams);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Terminal Ballistics|Game Mode")
	void BulletExitEvent(const FTBImpactParams& impactParams);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Terminal Ballistics|Game Mode")
	void BulletInjureEvent(const FTBImpactParams& impactParams, const FTBProjectileInjuryParams& injuryParams);
};
