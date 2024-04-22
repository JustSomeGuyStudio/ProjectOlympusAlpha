// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SKGAttachmentHelpers.generated.h"


class AActor;
class USKGAttachmentManagerComponent;
class USKGAttachmentComponent;

DECLARE_STATS_GROUP(TEXT("SKGAttachmentHelpers"), STATGROUP_SKGAttachmentHelpers, STATCAT_Advanced);

UCLASS()
class SKGATTACHMENT_API USKGAttachmentHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

private:
	static void DestroyAttachments(USKGAttachmentManagerComponent* AttachmentManagerComponent);
	static void SetupAttachments(USKGAttachmentManagerComponent* AttachmentManagerComponent, const FSKGAttachmentActor& Data);
	
public:
	UFUNCTION(BlueprintPure, Category = "USKGAttachmentHelpers|Getters")
	static USKGAttachmentManagerComponent* GetAttachmentManagerComponent(const AActor* Actor);
	UFUNCTION(BlueprintPure, Category = "USKGAttachmentHelpers|Getters")
	static TArray<USKGAttachmentComponent*> GetAttachmentComponents(const AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "USKGAttachmentHelpers|Construction")
	static bool ConstructAttachmentManagerData(const USKGAttachmentManagerComponent* AttachmentManagerComponent, FSKGAttachmentActor& Data);
	UFUNCTION(BlueprintPure, Category = "USKGAttachmentHelpers|Construction")
	static bool SerializeAttachmentManagerToJson(const USKGAttachmentManagerComponent* AttachmentManagerComponent, FString& SerializedString);
	UFUNCTION(BlueprintPure, Category = "USKGAttachmentHelpers|Construction")
	static bool DeserializeAttachmentManagerJson(const FString& JsonString, FSKGAttachmentActor& Data);
	UFUNCTION(BlueprintPure, Category = "USKGAttachmentHelpers|Construction", meta = (WorldContext = "WorldContextObject"))
	static AActor* ConstructActorFromAttachmentManagerData(const UObject* WorldContextObject, const FSKGAttachmentActor& Data);
	UFUNCTION(BlueprintCallable, Category = "USKGAttachmentHelpers|Construction")
	static void ConstructExistingActorFromAttachmentManagerData(const AActor* Actor, const FSKGAttachmentActor& Data);

	UFUNCTION(BlueprintCallable, Category = "USKGAttachmentHelpers|File")
	static bool SaveStringToFile(const FString& Path, const FString& FileContent);
	UFUNCTION(BlueprintPure, Category = "USKGAttachmentHelpers|File")
	static bool LoadFileToString(const FString& Path, FString& OutString);
	UFUNCTION(BlueprintPure, Category = "USKGAttachmentHelpers|File")
	static bool GetAllFiles(FString Path, TArray<FString>& OutFiles);
};
