// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AnimNodes/AnimNode_BlendListBase.h"
#include "SKGAnimNode_GameplayTagBlend.generated.h"

/**
 * 
 */
USTRUCT()
struct SKGANIMGRAPHNODES_API FSKGAnimNode_GameplayTagBlend : public FAnimNode_BlendListBase
{
	GENERATED_BODY()

public:
	FSKGAnimNode_GameplayTagBlend();
	
	UPROPERTY(EditAnywhere, Category = "Settings")
	FGameplayTagContainer GameplayTagContainer;
	
	UPROPERTY(EditAnywhere, Category = "Settings", Meta = (PinShownByDefault))
	FGameplayTag ActiveGameplayTag;
	FGameplayTag PreviousGameplayTag;
	int32 CurrentIndex {0};

protected:
	virtual int32 GetActiveChildIndex() override;

public:
	FORCEINLINE const FGameplayTag& GetActiveTag() const { return ActiveGameplayTag; }
	FORCEINLINE const FGameplayTagContainer& GetTags() const { return GameplayTagContainer; }

#if WITH_EDITOR
	void RefreshPoses();
#endif
};
