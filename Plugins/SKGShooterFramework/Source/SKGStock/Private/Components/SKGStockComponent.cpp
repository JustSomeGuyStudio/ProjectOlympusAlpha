// Copyright 2023, Dakota Dawe, All rights reserved

#include "Components/SKGStockComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

USKGStockComponent::USKGStockComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USKGStockComponent::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SetLengthOfPull();
	}
}

void USKGStockComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGStockComponent, bIsFolded, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGStockComponent, LengthOfPull, Params);
}

void USKGStockComponent::TryForceNetUpdate() const
{
	if (bAutoCallForceNetUpdate)
	{
		GetOwner()->ForceNetUpdate();
	}
}

void USKGStockComponent::SetLengthOfPull()
{
	const float OriginalLengthOfPull = LengthOfPull;
	if (bIsFolded)
	{
		if (LengthOfPull != FoldedLengthOfPull)
		{
			LengthOfPull = FoldedLengthOfPull;
		}
	}
	else
	{
		LengthOfPull = UnfoldedLengthOfPull;
		if (bCanBeAffectedByOffset)
		{
			LengthOfPull -= Offset;
		}
	}
	if (LengthOfPull != OriginalLengthOfPull)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGStockComponent, LengthOfPull, this);
		OnRep_LengthOfPull();
	}
}

void USKGStockComponent::SetOffset(float PositionOffset)
{
	if (HasAuthority())
	{
		Offset = PositionOffset;
		SetLengthOfPull();
	}
}

bool USKGStockComponent::Server_SetFoldedState_Validate(bool bFold)
{
	if (bFold && !bCanBeFolded)
	{
		return false;
	}
	return true;
}

void USKGStockComponent::Server_SetFoldedState_Implementation(bool bFold)
{
	if (bIsFolded != bFold)
	{
		bIsFolded = bFold;
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGStockComponent, bIsFolded, this);
		OnRep_IsFolded();
		SetLengthOfPull();
	}
}

void USKGStockComponent::Fold()
{
	if (bCanBeFolded && !bIsFolded)
	{
		bIsFolded = true;
		if (HasAuthority())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGStockComponent, bIsFolded, this);
			OnRep_IsFolded();
		}
		else
		{
			Server_SetFoldedState(bIsFolded);
		}
		SetLengthOfPull();
	}
}

void USKGStockComponent::Unfold()
{
	if (bCanBeFolded && bIsFolded)
	{
		bIsFolded = false;
		if (HasAuthority())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGStockComponent, bIsFolded, this);
			OnRep_IsFolded();
		}
		else
		{
			Server_SetFoldedState(bIsFolded);
		}
		SetLengthOfPull();
	}
}

void USKGStockComponent::ToggleFold()
{
	if (bCanBeFolded)
	{
		bIsFolded = !bIsFolded;
		if (HasAuthority())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGStockComponent, bIsFolded, this);
			OnRep_IsFolded();
		}
		else
		{
			Server_SetFoldedState(bIsFolded);
		}
		SetLengthOfPull();
	}
}

void USKGStockComponent::OnRep_IsFolded() const
{
	OnFolded.Broadcast(bIsFolded);
}

void USKGStockComponent::OnRep_LengthOfPull() const
{
	OnLengthOfPullChanged.Broadcast(LengthOfPull);
}