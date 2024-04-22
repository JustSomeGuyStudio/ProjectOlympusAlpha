// Copyright 2023, Dakota Dawe, All rights reserved


#include "Statics/SKGShooterFrameworkCoreJsonStatics.h"

#include "JsonObjectWrapper.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "UObject/UnrealType.h"
#include "Engine/DataAsset.h"

EJson USKGShooterFrameworkCoreJsonStatics::GetPropertyJsonType(const FProperty* ObjectProperty)
{
	if (CastField<FBoolProperty>(ObjectProperty))
	{
		return EJson::Boolean;
	}
	if (CastField<FFloatProperty>(ObjectProperty))
	{
		return EJson::Number;
	}
	if (CastField<FDoubleProperty>(ObjectProperty))
	{
		return EJson::Number;
	}
	if (CastField<FIntProperty>(ObjectProperty))
	{
		return EJson::Number;
	}
	if (CastField<FByteProperty>(ObjectProperty))
	{
		return EJson::Number;
	}
	if (CastField<FStrProperty>(ObjectProperty))
	{
		return EJson::String;
	}
	if (CastField<FNameProperty>(ObjectProperty))
	{
		return EJson::String;
	}
	if (CastField<FStructProperty>(ObjectProperty))
	{
		return EJson::Object;
	}
	if (CastField<FObjectProperty>(ObjectProperty))
	{	// NOT FULLY IMPLEMENTED
		return EJson::String;
	}
	return EJson::None;
}

USKGShooterFrameworkCoreJsonStatics::ESKGNumberPropertyType USKGShooterFrameworkCoreJsonStatics::
GetPropertyNumberType(const FProperty* ObjectProperty)
{
	if (CastField<FFloatProperty>(ObjectProperty))
	{
		return ESKGNumberPropertyType::Float;
	}
	if (CastField<FDoubleProperty>(ObjectProperty))
	{
		return ESKGNumberPropertyType::Double;
	}
	if (CastField<FIntProperty>(ObjectProperty))
	{
		return ESKGNumberPropertyType::Int;
	}
	if (CastField<FByteProperty>(ObjectProperty))
	{
		return ESKGNumberPropertyType::BYTE;
	}
	return ESKGNumberPropertyType::None;
}

bool USKGShooterFrameworkCoreJsonStatics::GetPropertyValueBoolean(const FProperty* ObjectProperty, void* Object,
	bool& Value)
{
	if (const FBoolProperty* Property = CastField<FBoolProperty>(ObjectProperty))
	{
		Value = *Property->ContainerPtrToValuePtr<bool>(Object);
		return true;
	}
	return false;
}

bool USKGShooterFrameworkCoreJsonStatics::GetPropertyValueNumber(const FProperty* ObjectProperty, void* Object,
	int32& Value)
{
	if (const FIntProperty* Property = CastField<FIntProperty>(ObjectProperty))
	{
		Value = *Property->ContainerPtrToValuePtr<int32>(Object);
		return true;
	}
	return false;
}

bool USKGShooterFrameworkCoreJsonStatics::GetPropertyValueNumber(const FProperty* ObjectProperty, void* Object,
	float& Value)
{
	if (const FFloatProperty* Property = CastField<FFloatProperty>(ObjectProperty))
	{
		Value = *Property->ContainerPtrToValuePtr<float>(Object);
		return true;
	}
	return false;
}

bool USKGShooterFrameworkCoreJsonStatics::GetPropertyValueNumber(const FProperty* ObjectProperty, void* Object,
	double& Value)
{
	if (const FDoubleProperty* Property = CastField<FDoubleProperty>(ObjectProperty))
	{
		Value = *Property->ContainerPtrToValuePtr<double>(Object);
		return true;
	}
	return false;
}

bool USKGShooterFrameworkCoreJsonStatics::GetPropertyValueNumber(const FProperty* ObjectProperty, void* Object,
	uint8& Value)
{
	if (const FByteProperty* Property = CastField<FByteProperty>(ObjectProperty))
	{
		Value = *Property->ContainerPtrToValuePtr<uint8>(Object);
		return true;
	}
	return false;
}

bool USKGShooterFrameworkCoreJsonStatics::GetPropertyValueObject(const FStructProperty* Struct, void* Object,
	TSharedPtr<FJsonObject> JsonObject)
{
	void* StructValuePtr = Struct->ContainerPtrToValuePtr<void>(Object);
	for (TFieldIterator<FProperty> StructProperty(Struct->Struct); StructProperty; ++StructProperty)
	{
		SerializeProperty(*StructProperty, StructValuePtr, JsonObject);
	}
	return true;
}

bool USKGShooterFrameworkCoreJsonStatics::GetPropertyValueString(const FProperty* ObjectProperty, void* Object,
                                                                    FString& Value)
{
	if (const FStrProperty* Property = CastField<FStrProperty>(ObjectProperty))
	{
		Value = *Property->ContainerPtrToValuePtr<FString>(Object);
		return true;
	}
	if (const FObjectProperty* Property = CastField<FObjectProperty>(ObjectProperty))
	{
		Value = *Property->ContainerPtrToValuePtr<FString>(Object);
		return true;
	}
	return false;
}

bool USKGShooterFrameworkCoreJsonStatics::SetPropertyValueBoolean(FProperty* ObjectProperty, void* Object, const bool Value)
{
	if (FBoolProperty* Property = CastField<FBoolProperty>(ObjectProperty))
	{
		Property->SetPropertyValue_InContainer(Object, Value);
		return true;
	}
	return false;
}

bool USKGShooterFrameworkCoreJsonStatics::SetPropertyValueNumber(FProperty* ObjectProperty, void* Object,
	const int32 Value)
{
	if (FIntProperty* IntProperty = CastField<FIntProperty>(ObjectProperty))
	{
		IntProperty->SetPropertyValue_InContainer(Object, Value);
		return true;
	}
	return false;
}

bool USKGShooterFrameworkCoreJsonStatics::SetPropertyValueNumber(FProperty* ObjectProperty, void* Object,
	const float Value)
{
	if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(ObjectProperty))
	{
		FloatProperty->SetPropertyValue_InContainer(Object, Value);
		return true;
	}
	return false;
}

bool USKGShooterFrameworkCoreJsonStatics::SetPropertyValueNumber(FProperty* ObjectProperty, void* Object,
	const double Value)
{
	if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(ObjectProperty))
	{
		DoubleProperty->SetPropertyValue_InContainer(Object, Value);
		return true;
	}
	return false;
}

bool USKGShooterFrameworkCoreJsonStatics::SetPropertyValueNumber(FProperty* ObjectProperty, void* Object,
	const uint8 Value)
{
	if (FByteProperty* ByteProperty = CastField<FByteProperty>(ObjectProperty))
	{
		ByteProperty->SetPropertyValue_InContainer(Object, Value);
		return true;
	}
	return false;
}

bool USKGShooterFrameworkCoreJsonStatics::SetPropertyValueObject(const FStructProperty* Struct, void* Object,
	TSharedPtr<FJsonObject> JsonObject)
{
	void* StructValuePtr = Struct->ContainerPtrToValuePtr<void>(Object);
	for (TFieldIterator<FProperty> StructProperty(Struct->Struct); StructProperty; ++StructProperty)
	{
		DeserializeProperty(*StructProperty, StructValuePtr, JsonObject);
	}
	return true;
}

bool USKGShooterFrameworkCoreJsonStatics::SetPropertyValueString(FProperty* ObjectProperty, void* Object,
	const FString& Value)
{
	if (FStrProperty* Property = CastField<FStrProperty>(ObjectProperty))
	{
		Property->SetPropertyValue_InContainer(Object, Value);
		return true;
	}
	return false;
}

bool USKGShooterFrameworkCoreJsonStatics::SerializeProperty(FProperty* ObjectProperty, void* Object,
	TSharedPtr<FJsonObject> JsonObject)
{
	const FString FieldName = ObjectProperty->GetAuthoredName();
	switch (GetPropertyJsonType(ObjectProperty))
	{
	case EJson::Boolean :
		{
			bool bValue;
			if (!GetPropertyValueBoolean(ObjectProperty, Object, bValue))
			{
				return false;
			}
			JsonObject->SetBoolField(FieldName, bValue);
			break;
		}
	case EJson::Number :
		{
			switch (GetPropertyNumberType(ObjectProperty))
			{
			case ESKGNumberPropertyType::Int :
				{
					int32 Value;
					if (!GetPropertyValueNumber(ObjectProperty, Object, Value))
					{
						return false;
					}
					JsonObject->SetNumberField(FieldName, Value);
					break;
				}
			case ESKGNumberPropertyType::Float :
				{
					float Value;
					if (!GetPropertyValueNumber(ObjectProperty, Object, Value))
					{
						return false;
					}
					JsonObject->SetNumberField(FieldName, Value);
					break;
				}
			case ESKGNumberPropertyType::Double :
				{
					double Value;
					if (!GetPropertyValueNumber(ObjectProperty, Object, Value))
					{
						return false;
					}
					JsonObject->SetNumberField(FieldName, Value);
					break;
				}
			case ESKGNumberPropertyType::BYTE :
				{
					uint8 Value;
					if (!GetPropertyValueNumber(ObjectProperty, Object, Value))
					{
						return false;
					}
					JsonObject->SetNumberField(FieldName, Value);
					break;
				}
			default : return false;
			}
			break;
		}
	case EJson::Object :
		{
			FJsonObjectWrapper JsonObjectWrapper;
			TSharedPtr<FJsonObject> JsonObjectObject = JsonObjectWrapper.JsonObject;
			const FStructProperty* Struct = CastField<FStructProperty>(ObjectProperty);
			if (Struct && GetPropertyValueObject(Struct, Object, JsonObjectObject))
			{
			}
			else
			{
				return false;
			}
			JsonObject->SetObjectField(FieldName, JsonObjectObject);			
			break;
		}
	case EJson::String :
		{
			FString Value;
			if (!GetPropertyValueString(ObjectProperty, Object, Value))
			{
				return false;
			}
			JsonObject->SetStringField(FieldName, Value);
			break;
		}
	default : break;
	}
	return true;
}

bool USKGShooterFrameworkCoreJsonStatics::SerializeDataAssetToJsonStringIgnoreKeys(UPrimaryDataAsset* Object, FString& JsonString, const TArray<FString>& KeysToIgnore)
{
	FJsonObjectWrapper JsonObjectWrapper;
	if (Object)
	{
		TSharedPtr<FJsonObject> JsonObject = JsonObjectWrapper.JsonObject;
		for (TFieldIterator<FProperty> Property(Object->GetClass()); Property; ++Property)
		{
			if (!KeysToIgnore.Contains(Property->GetAuthoredName()) && !SerializeProperty(*Property, Object, JsonObject))
			{
				return false;
			}
		}
	}
	JsonObjectWrapper.JsonObjectToString(JsonString);
	return true;
}

bool USKGShooterFrameworkCoreJsonStatics::SerializeDataAssetToJsonStringAllowKeys(UPrimaryDataAsset* Object,
	FString& JsonString, const TArray<FString>& KeysToAllow)
{
	FJsonObjectWrapper JsonObjectWrapper;
	if (Object)
	{
		TSharedPtr<FJsonObject> JsonObject = JsonObjectWrapper.JsonObject;
		for (TFieldIterator<FProperty> Property(Object->GetClass()); Property; ++Property)
		{
			if (KeysToAllow.Contains(Property->GetAuthoredName()) && !SerializeProperty(*Property, Object, JsonObject))
			{
				return false;
			}
		}
	}
	JsonObjectWrapper.JsonObjectToString(JsonString);
	return true;
}

bool USKGShooterFrameworkCoreJsonStatics::DeserializeProperty(FProperty* ObjectProperty, void* Object,
                                                                 const TSharedPtr<FJsonObject> JsonObject)
{
	const FString Key = ObjectProperty->GetAuthoredName();
	bool bValidPropertySet = true;
	if (JsonObject->HasField(Key))
	{
		const EJson JsonType = GetPropertyJsonType(ObjectProperty);
		TSharedPtr<FJsonValue> JsonValue;
		switch (JsonType)
		{
		case EJson::Boolean :
			{
				JsonValue = JsonObject->GetField<EJson::Boolean>(Key);
				if (JsonValue)
				{
					bValidPropertySet = SetPropertyValueBoolean(ObjectProperty, Object, JsonValue->AsBool());
				}
				break;
			}
		case EJson::Number :
			{
				JsonValue = JsonObject->GetField<EJson::Number>(Key);
				if (JsonValue)
				{
					ESKGNumberPropertyType PropertyType = GetPropertyNumberType(ObjectProperty);
					switch (PropertyType)
					{
					case ESKGNumberPropertyType::Int :
						{
							bValidPropertySet = SetPropertyValueNumber(ObjectProperty, Object, static_cast<int32>(JsonValue->AsNumber()));
							break;
						}
					case ESKGNumberPropertyType::Float :
						{
							bValidPropertySet = SetPropertyValueNumber(ObjectProperty, Object, static_cast<float>(JsonValue->AsNumber()));
							break;
						}
					case ESKGNumberPropertyType::Double :
						{
							bValidPropertySet = SetPropertyValueNumber(ObjectProperty, Object, JsonValue->AsNumber());
							break;
						}
					case ESKGNumberPropertyType::BYTE :
						{
							bValidPropertySet = SetPropertyValueNumber(ObjectProperty, Object, static_cast<uint8>(JsonValue->AsNumber()));
							break;
						}
						default : return false;
					}
				}
				break;
			}
		case EJson::Object :
			{
				JsonValue = JsonObject->GetField<EJson::Object>(Key);
				if (JsonValue)
				{
					const FStructProperty* Struct = CastField<FStructProperty>(ObjectProperty);
					if (TSharedPtr<FJsonObject> JsonStruct = JsonValue->AsObject())
					{
						bValidPropertySet = SetPropertyValueObject(Struct, Object, JsonValue->AsObject());
					}
				}
				break;
			}
		case EJson::String :
			{
				JsonValue = JsonObject->GetField<EJson::String>(Key);
				if (JsonValue)
				{
					bValidPropertySet = SetPropertyValueString(ObjectProperty, Object, JsonValue->AsString());
				}
				break;
			}
		default: return false;
		}
	}
	else
	{
		return false;
	}
	return bValidPropertySet;
}

bool USKGShooterFrameworkCoreJsonStatics::ApplyJsonStringToDataAssetIgnoreKeys(const FString& String,
	UPrimaryDataAsset* Object, const TArray<FString>& KeysToIgnore)
{
	bool bValidJson = false;
	if (Object)
	{
		FJsonObjectWrapper JsonObjectWrapper;
		bValidJson = JsonObjectWrapper.JsonObjectFromString(String);
		if (bValidJson)
		{
			const TSharedPtr<FJsonObject> JsonObject = JsonObjectWrapper.JsonObject;
			for (TFieldIterator<FProperty> Property(Object->GetClass()); Property; ++Property)
			{
				if (!KeysToIgnore.Contains(Property->GetAuthoredName()) && !DeserializeProperty(*Property, Object, JsonObject))
				{
					UE_LOG(LogTemp, Error, TEXT("Failed: %s"), *Property->GetAuthoredName());
					return false;
				}
			}
		}
	}
	return bValidJson;
}

bool USKGShooterFrameworkCoreJsonStatics::ApplyJsonStringToDataAssetAllowKeys(const FString& String,
	UPrimaryDataAsset* Object, const TArray<FString>& KeysToAllow)
{
	bool bValidJson = false;
	if (Object)
	{
		FJsonObjectWrapper JsonObjectWrapper;
		bValidJson = JsonObjectWrapper.JsonObjectFromString(String);
		if (bValidJson)
		{
			const TSharedPtr<FJsonObject> JsonObject = JsonObjectWrapper.JsonObject;
			for (TFieldIterator<FProperty> Property(Object->GetClass()); Property; ++Property)
			{
				if (KeysToAllow.Contains(Property->GetAuthoredName()) && !DeserializeProperty(*Property, Object, JsonObject))
				{
					UE_LOG(LogTemp, Error, TEXT("Failed: %s"), *Property->GetAuthoredName());
					return false;
				}
			}
		}
	}
	return bValidJson;
}
