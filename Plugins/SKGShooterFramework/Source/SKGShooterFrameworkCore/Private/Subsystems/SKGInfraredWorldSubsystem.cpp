// Copyright 2023, Dakota Dawe, All rights reserved


#include "Subsystems/SKGInfraredWorldSubsystem.h"
#include "Interfaces/SKGInfraredInterface.h"
#include "DeveloperSettings/SKGShooterFrameworkDeveloperSettings.h"

#include "Components/ActorComponent.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Materials/MaterialParameterCollection.h"
#include "Engine/World.h"


USKGInfraredWorldSubsystem::USKGInfraredWorldSubsystem()
{
	// Lets go ahead and reserve some elements first to account for potentially multiple infrared devices getting added on begin play
	InfraredDevices.Reserve(20);
}

void USKGInfraredWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	if (const USKGShooterFrameworkDeveloperSettings* DeveloperSettings = GetDefault<USKGShooterFrameworkDeveloperSettings>())
	{
		SetInfraredMaterialParameterCollection(DeveloperSettings->NightVisionMaterialParameterCollection.NightVisionMaterialParameterCollection);
	}
}

void USKGInfraredWorldSubsystem::SetInfraredMaterialParameterCollection(UMaterialParameterCollection* MPC)
{
	if (GetWorld() && MPC)
	{
		InfraredMPC = GetWorld()->GetParameterCollectionInstance(MPC);
	}
}

void USKGInfraredWorldSubsystem::SetNightVisionColor(const FLinearColor& Color)
{
	if (InfraredMPC)
	{
		InfraredMPC->SetVectorParameterValue(NightVisionColorParameterName, Color);
	}
}

void USKGInfraredWorldSubsystem::EnableInfraredMode()
{
	if (!bInfraredModeEnabled)
	{
		if (InfraredMPC)
		{
			InfraredMPC->SetScalarParameterValue(NightVisionOnParameterName, 1.0f);
		}
		
		bInfraredModeEnabled = true;
		for (int32 i = 0; i < InfraredDevices.Num(); ++i)
		{
			if (UActorComponent* InfraredDevice = InfraredDevices[i])
			{
				Cast<ISKGInfraredInterface>(InfraredDevice)->OnInfraredEnabledForPlayer();
			}
			else
			{
				InfraredDevices.RemoveAt(i, 1, false);
			}
		}
	}
}

void USKGInfraredWorldSubsystem::DisableInfraredMode()
{
	if (bInfraredModeEnabled)
	{
		if (InfraredMPC)
		{
			InfraredMPC->SetScalarParameterValue(NightVisionOnParameterName, 0.0f);
		}
		
		bInfraredModeEnabled = false;
		for (int32 i = 0; i < InfraredDevices.Num(); ++i)
		{
			if (UActorComponent* InfraredDevice = InfraredDevices[i])
			{
				Cast<ISKGInfraredInterface>(InfraredDevice)->OnInfraredDisabledForPlayer();
			}
			else
			{
				InfraredDevices.RemoveAt(i, 1, false);
			}
		}
	}
}

void USKGInfraredWorldSubsystem::RegisterInfraredDevice(UActorComponent* InfraredDevice)
{
	if (InfraredDevice && InfraredDevice->Implements<USKGInfraredInterface>())
	{
		InfraredDevices.Add(InfraredDevice);
	}
}

void USKGInfraredWorldSubsystem::UnregisterInfraredDevice(UActorComponent* InfraredDevice)
{
	if (InfraredDevice)
	{
		InfraredDevices.RemoveSingle(InfraredDevice);
	}
}
