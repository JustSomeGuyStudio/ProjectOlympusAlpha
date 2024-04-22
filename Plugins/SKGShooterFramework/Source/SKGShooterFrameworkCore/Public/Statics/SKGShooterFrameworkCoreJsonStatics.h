// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Serialization/JsonTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SKGShooterFrameworkCoreJsonStatics.generated.h"

class UPrimaryDataAsset;
class FProperty;
class FJsonObject;

UCLASS()
class SKGSHOOTERFRAMEWORKCORE_API USKGShooterFrameworkCoreJsonStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
private:
	enum class ESKGNumberPropertyType : uint8
	{
		None,
		Int,
		Float,
		Double,
		BYTE
	};
	
	static EJson GetPropertyJsonType(const FProperty* ObjectProperty);
	static ESKGNumberPropertyType GetPropertyNumberType(const FProperty* ObjectProperty);
	
	static bool GetPropertyValueBoolean(const FProperty* ObjectProperty, void* Object, bool& Value);
	static bool GetPropertyValueNumber(const FProperty* ObjectProperty, void* Object, int32& Value);
	static bool GetPropertyValueNumber(const FProperty* ObjectProperty, void* Object, float& Value);
	static bool GetPropertyValueNumber(const FProperty* ObjectProperty, void* Object, double& Value);
	static bool GetPropertyValueNumber(const FProperty* ObjectProperty, void* Object, uint8& Value);
	static bool GetPropertyValueObject(const FStructProperty* Struct, void* Object, TSharedPtr<FJsonObject> JsonObject);
	static bool GetPropertyValueString(const FProperty* ObjectProperty, void* Object, FString& Value);
	
	static bool SetPropertyValueBoolean(FProperty* ObjectProperty, void* Object, const bool Value);
	static bool SetPropertyValueNumber(FProperty* ObjectProperty, void* Object, const int32 Value);
	static bool SetPropertyValueNumber(FProperty* ObjectProperty, void* Object, const float Value);
	static bool SetPropertyValueNumber(FProperty* ObjectProperty, void* Object, const double Value);
	static bool SetPropertyValueNumber(FProperty* ObjectProperty, void* Object, const uint8 Value);
	static bool SetPropertyValueObject(const FStructProperty* Struct, void* Object, TSharedPtr<FJsonObject> JsonObject);
	static bool SetPropertyValueString(FProperty* ObjectProperty, void* Object, const FString& Value);

	static bool SerializeProperty(FProperty* ObjectProperty, void* Object, TSharedPtr<FJsonObject> JsonObject);
	static bool DeserializeProperty(FProperty* ObjectProperty, void* Object, const TSharedPtr<FJsonObject> JsonObject);
	
public:
	UFUNCTION(BlueprintCallable, Category = "SKGShooterFrameworkCoreNetworkStatics")
	static bool SerializeDataAssetToJsonStringIgnoreKeys(UPrimaryDataAsset* Object, FString& JsonString, const TArray<FString>& KeysToIgnore);
	UFUNCTION(BlueprintCallable, Category = "SKGShooterFrameworkCoreNetworkStatics")
	static bool SerializeDataAssetToJsonStringAllowKeys(UPrimaryDataAsset* Object, FString& JsonString, const TArray<FString>& KeysToAllow);
	UFUNCTION(BlueprintCallable, Category = "SKGShooterFrameworkCoreNetworkStatics")
	static bool ApplyJsonStringToDataAssetIgnoreKeys(const FString& String, UPrimaryDataAsset* Object, const TArray<FString>& KeysToIgnore);
	UFUNCTION(BlueprintCallable, Category = "SKGShooterFrameworkCoreNetworkStatics")
	static bool ApplyJsonStringToDataAssetAllowKeys(const FString& String, UPrimaryDataAsset* Object, const TArray<FString>& KeysToAllow);
};
