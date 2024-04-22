// Copyright 2023, Dakota Dawe, All rights reserved

#include "AnimGraphNodes/SKGAnimNode_GameplayTagBlend.h"

FSKGAnimNode_GameplayTagBlend::FSKGAnimNode_GameplayTagBlend()
{
	
}

int32 FSKGAnimNode_GameplayTagBlend::GetActiveChildIndex()
{
	if (ActiveGameplayTag != PreviousGameplayTag && ActiveGameplayTag.IsValid())
	{
		TArray<FGameplayTag> GameplayTags;
		GameplayTagContainer.GetGameplayTagArray(GameplayTags);
		CurrentIndex = GameplayTags.Find(ActiveGameplayTag) + 1;
	}
	else
	{
		CurrentIndex = 0;
	}
	
	return CurrentIndex;
}

#if WITH_EDITOR
void FSKGAnimNode_GameplayTagBlend::RefreshPoses()
{
	TArray<FGameplayTag> GameplayTags;
	GetTags().GetGameplayTagArray(GameplayTags);

	const int32 TagCount = BlendPose.Num() - GameplayTags.Num() - 1;
	
	if (TagCount == 0)
	{
		return;
	}

	if (TagCount > 0)
	{
		for (int32 i = TagCount; i > 0; i--)
		{
			RemovePose(BlendPose.Num() - 1);
		}
	}
	else
	{
		for (int32 i = TagCount; i < 0; i++)
		{
			AddPose();
		}
	}
}
#endif
