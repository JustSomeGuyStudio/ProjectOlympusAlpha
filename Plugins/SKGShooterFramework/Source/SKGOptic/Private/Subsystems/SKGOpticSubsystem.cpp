// Copyright 2023, Dakota Dawe, All rights reserved


#include "Subsystems/SKGOpticSubsystem.h"

#include "Components/MeshComponent.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Engine/World.h"

USKGOpticSubsystem::USKGOpticSubsystem()
{
	
}

void USKGOpticSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MeshTest && MPCTest)
	{
		MPCTest->SetVectorParameterValue(FName("ScopeLocation"), MeshTest->GetSocketLocation(FName("S_Aim")));
	}
}

void USKGOpticSubsystem::SetTickTest(UMeshComponent* Mesh, UMaterialParameterCollection* MPC)
{
	MeshTest = Mesh;
	MPCTest = GetWorld()->GetParameterCollectionInstance(MPC);
}
