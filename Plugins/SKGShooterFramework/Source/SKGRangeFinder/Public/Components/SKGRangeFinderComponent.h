// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SKGRangeFinderComponent.generated.h"

class UTextRenderComponent;
class UMeshComponent;
struct FHitResult;

USTRUCT(BlueprintType)
struct FSKGMeasurementSymbols
{
	GENERATED_BODY()
	// The symbol that gets appended to the end of the optional text render component
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "SKGRangeFinder|Settings")
	FString MeterSymbol {"m"};
	// The symbol that gets appended to the end of the optional text render component
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "SKGRangeFinder|Settings")
	FString YardSymbol {"yd"};
	// The symbol that gets appended to the end of the optional text render component
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "SKGRangeFinder|Settings")
	FString InchSymbol {"in"};
	// The symbol that gets appended to the end of the optional text render component
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "SKGRangeFinder|Settings")
	FString CentimeterSymbol {"cm"};
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRanged, float, Distance);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SKGRANGEFINDER_API USKGRangeFinderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USKGRangeFinderComponent();
	
protected:
	// The mesh used for the range finder
	UPROPERTY(EditDefaultsOnly, Category = "SKGRangeFinder|Initialize")
	FName RangeFinderMeshName {"StaticMesh"};
	// The socket to be used for performing the trace from
	UPROPERTY(EditDefaultsOnly, Category = "SKGRangeFinder|Initialize")
	FName RangeFinderLaserSocketName {"S_Laser"};
	// Optional, but if set the distance will auto be applied as text to the found component
	UPROPERTY(EditDefaultsOnly, Category = "SKGRangeFinder|Initialize")
	FName TextRenderComponentName {"TextRenderComponent"};
	
	ECollisionChannel RangeFinderCollisionChannel {ECC_Visibility};
	UPROPERTY(EditDefaultsOnly, Category = "SKGRangeFinder|Settings")
	float MinTraceDistance {0.0f};
	UPROPERTY(EditDefaultsOnly, Category = "SKGRangeFinder|Settings")
	float MaxTraceDistance {100000.0f};
	// The symbols that gets appended to the end of the optional text render component and GetRangeAsString
	UPROPERTY(EditDefaultsOnly, Category = "SKGRangeFinder|Settings")
	FSKGMeasurementSymbols MeasurementSymbols;
	// If false, will use feet and centimeter
	UPROPERTY(EditDefaultsOnly, Category = "SKGRangeFinder|Settings")
	bool bUseInYardMeter {true};
	// If false it will use imperial units
	UPROPERTY(EditDefaultsOnly, Category = "SKGRangeFinder|Settings")
	bool bUseMetric {true};
	// If true the range will be gotten automatically at the given interval
	UPROPERTY(EditDefaultsOnly, Category = "SKGRangeFinder|Settings")
	bool bAutoRange {false};
	// Time between ranging
	UPROPERTY(EditDefaultsOnly, Category = "SKGRangeFinder|Settings", meta = (EditCondition = "bAutoRange"))
	float AutoRangeInterval {1.0f};

	UPROPERTY(BlueprintGetter = GetRangeFinderMesh, Category = "SKGRangeFinder|Mesh")
	TObjectPtr<UMeshComponent> RangeFinderMesh;
	UPROPERTY(BlueprintGetter = GetTextRenderComponent, Category = "SKGRangeFinder|Mesh")
	TObjectPtr<UTextRenderComponent> TextRenderComponent;
	
	// Called when the game starts
	virtual void BeginPlay() override;

	bool PerformTrace(FHitResult& HitResult) const;
	float ConvertDistance(float Distance) const;
	FString ConstructMeasurementString(const float Distance) const;
	
public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "SKGRangeFinder|Range")
	float GetRange() const;
	UFUNCTION(BlueprintCallable, Category = "SKGRangeFinder|Range")
	FString GetRangeAsString() const;
	
	UFUNCTION(BlueprintGetter)
	UMeshComponent* GetRangeFinderMesh() const { return RangeFinderMesh; }
	UFUNCTION(BlueprintGetter)
	UTextRenderComponent* GetTextRenderComponent() const { return TextRenderComponent; }

	UPROPERTY(BlueprintAssignable, Category = "SKGRangeFinder|Events")
	FOnRanged OnRanged;
};
