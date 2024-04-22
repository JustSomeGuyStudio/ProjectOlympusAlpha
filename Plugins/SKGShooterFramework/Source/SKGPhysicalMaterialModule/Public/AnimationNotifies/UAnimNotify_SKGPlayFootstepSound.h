// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "UAnimNotify_SKGPlayFootstepSound.generated.h"

// Notify specific for playing footstep sounds
UCLASS()
class SKGPHYSICALMATERIALMODULE_API UUAnimNotify_SKGPlayFootstepSound : public UAnimNotify
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKG|AnimNotify|FootstepSound")
	FName FootBoneName {"foot_r"};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKG|AnimNotify|FootstepSound")
	FGameplayTag PhysicalMaterialEffectTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SKG|AnimNotify|FootstepSound")
	TEnumAsByte<ECollisionChannel> TraceChannel {ECC_Visibility};
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	UFUNCTION(BlueprintCallable, Category = "SKG|AnimNotify|FootstepSound")
	void SetFootBoneName(const FName& BoneName) { FootBoneName = BoneName; }
	UFUNCTION(BlueprintCallable, Category = "SKG|AnimNotify|FootstepSound")
	void SetPhysicalMaterialEffectTag(const FGameplayTag& Tag) { PhysicalMaterialEffectTag = Tag; }
	UFUNCTION(BlueprintCallable, Category = "SKG|AnimNotify|FootstepSound")
	void SetTraceChannel(TEnumAsByte<ECollisionChannel> Channel) { TraceChannel = Channel; }
};
