// Copyright 2023, Dakota Dawe, All rights reserved


#include "Components/SKGRangeFinderComponent.h"

#include "Components/TextRenderComponent.h"
#include "Components/MeshComponent.h"
#include "DeveloperSettings/SKGShooterFrameworkDeveloperSettings.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"


USKGRangeFinderComponent::USKGRangeFinderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

}

void USKGRangeFinderComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const USKGShooterFrameworkDeveloperSettings* DeveloperSettings = GetDefault<USKGShooterFrameworkDeveloperSettings>())
	{
		RangeFinderCollisionChannel = DeveloperSettings->RangeFinderCollisionChannel;
	}
	
	for (UActorComponent* Component : GetOwner()->GetComponents())
	{
		if (Component)
		{
			if (Component->GetFName().IsEqual(RangeFinderMeshName))
			{
				RangeFinderMesh = Cast<UMeshComponent>(Component);
			}
			else if (Component->GetFName().IsEqual(TextRenderComponentName))
			{
				TextRenderComponent = Cast<UTextRenderComponent>(Component);
			}
		}
	}

	ensureMsgf(RangeFinderMesh, TEXT("RangeFinderMesh NOT found on Actor: %s. Make sure RangeFinderMeshName matches the mesh"), *GetOwner()->GetName());

	if (bAutoRange)
	{
		SetComponentTickInterval(AutoRangeInterval);
		SetComponentTickEnabled(true);
	}
}

void USKGRangeFinderComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float Distance = GetRange();
}

bool USKGRangeFinderComponent::PerformTrace(FHitResult& HitResult) const
{
	if (RangeFinderMesh)
	{
		const FTransform SocketTransform = RangeFinderMesh->GetSocketTransform(RangeFinderLaserSocketName);
		const FVector Start = SocketTransform.GetLocation();
		const FVector End = Start + SocketTransform.GetRotation().GetForwardVector() * MaxTraceDistance;

		FCollisionQueryParams Params;
		Params.AddIgnoredActor(GetOwner());
		if (const AActor* ParentActor = GetOwner()->GetAttachParentActor())
		{
			Params.AddIgnoredActor(ParentActor);
		}
		
		return GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, RangeFinderCollisionChannel, Params);
	}
	return false;
}

float USKGRangeFinderComponent::ConvertDistance(float Distance) const
{
	if (bUseMetric)
	{
		if (bUseInYardMeter)
		{
			return Distance * 0.01f;
		}
		return Distance;
	}
	return bUseInYardMeter ? Distance / 91.44f : Distance / 2.54f;
}

FString USKGRangeFinderComponent::ConstructMeasurementString(const float Distance) const
{
	FString String = FString::FromInt(Distance);
	FString AppendingType = "";
	if (bUseMetric)
	{
		AppendingType = bUseInYardMeter ? MeasurementSymbols.MeterSymbol : MeasurementSymbols.CentimeterSymbol;
	}
	else
	{
		AppendingType = bUseInYardMeter ? MeasurementSymbols.YardSymbol : MeasurementSymbols.InchSymbol;
	}
	String.Append(AppendingType);
	return String;
}

float USKGRangeFinderComponent::GetRange() const
{
	FHitResult HitResult;
	float Distance = 0.0f;
	if (PerformTrace(HitResult))
	{
		Distance = HitResult.Distance;
		Distance = ConvertDistance(Distance);
		if (Distance < MinTraceDistance || Distance > MaxTraceDistance)
		{
			Distance = 0.0f;
		}
		OnRanged.Broadcast(Distance);
	}
	if (TextRenderComponent)
	{
		TextRenderComponent->SetText(FText::FromString(ConstructMeasurementString(Distance)));
	}
	return Distance;
}

FString USKGRangeFinderComponent::GetRangeAsString() const
{
	const float Distance = GetRange();
	return ConstructMeasurementString(Distance);
}

