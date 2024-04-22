// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagAssetInterface.h"
#include "SKGFirearmAttachmentStatComponent.generated.h"


class USKGPDAFirearmAttachmentStats;

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SKGSTATS_API USKGFirearmAttachmentStatComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USKGFirearmAttachmentStatComponent();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintGetter = GetAttachmentStats, Category = "SKGFirearmAttachmentStatComponent|Initialize")
	TObjectPtr<USKGPDAFirearmAttachmentStats> AttachmentStatsDataAsset;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFirearmAttachmentStatComponent|Initialize")
	FGameplayTagContainer GameplayTags;
	
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; }

public:
	UFUNCTION(BlueprintGetter)
	USKGPDAFirearmAttachmentStats* GetAttachmentStats() const { return AttachmentStatsDataAsset; }
};
