// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SKGPDAAttachmentCompatibility.generated.h"

USTRUCT(BlueprintType)
struct FSKGDAAttachment
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGAttachmentDataTypes")
	TSoftClassPtr<AActor> Class;
};

UCLASS()
class SKGATTACHMENT_API USKGPDAAttachmentCompatibility : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SKGAttachment")
	TArray<FSKGDAAttachment> Attachments;
};
