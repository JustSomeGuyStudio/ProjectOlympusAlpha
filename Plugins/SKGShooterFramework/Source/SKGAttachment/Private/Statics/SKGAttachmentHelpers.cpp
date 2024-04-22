// Copyright 2023, Dakota Dawe, All rights reserved


#include "Statics/SKGAttachmentHelpers.h"
#include "Components/SKGAttachmentComponent.h"
#include "Components/SKGAttachmentManagerComponent.h"
#include "DataTypes/SKGAttachmentDataTypes.h"

#include "JsonObjectConverter.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "HAL/FileManagerGeneric.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DECLARE_CYCLE_STAT(TEXT("SKGGetAttachmentComponents"), STAT_SKGGetAttachmentComponents, STATGROUP_SKGAttachmentHelpers);
DECLARE_CYCLE_STAT(TEXT("SKGConstructActorFromAttachmentManagerData"), STAT_SKGConstructActorFromAttachmentManagerData, STATGROUP_SKGAttachmentHelpers);
DECLARE_CYCLE_STAT(TEXT("SKGConstructAttachmentManagerData"), STAT_SKGConstructAttachmentManagerData, STATGROUP_SKGAttachmentHelpers);

USKGAttachmentManagerComponent* USKGAttachmentHelpers::GetAttachmentManagerComponent(const AActor* Actor)
{
	if (Actor)
	{
		return Actor->FindComponentByClass<USKGAttachmentManagerComponent>();
	}
	return nullptr;
}

TArray<USKGAttachmentComponent*> USKGAttachmentHelpers::GetAttachmentComponents(const AActor* Actor)
{
	if (Actor)
	{
		TArray<USKGAttachmentComponent*> Components;
		Actor->GetComponents<USKGAttachmentComponent>(Components);
		return Components;
	}
	return TArray<USKGAttachmentComponent*>();
}

bool USKGAttachmentHelpers::ConstructAttachmentManagerData(const USKGAttachmentManagerComponent* AttachmentManagerComponent, FSKGAttachmentActor& Data)
{
	if (AttachmentManagerComponent && AttachmentManagerComponent->GetOwner())
	{
		SCOPE_CYCLE_COUNTER(STAT_SKGConstructAttachmentManagerData);
		Data.ActorClass = AttachmentManagerComponent->GetOwner()->GetClass();
		
		const TArray<FSKGAttachmentComponentItem>& AttachmentComponentItems = AttachmentManagerComponent->GetAttachmentComponents();
		Data.Attachments.Reserve(AttachmentComponentItems.Num());
		for (const FSKGAttachmentComponentItem& Item : AttachmentComponentItems)
		{
			if (const USKGAttachmentComponent* AttachmentComponent = Item.AttachmentComponent)
			{
				if (const AActor* Attachment = AttachmentComponent->GetAttachment())
				{
					FSKGActorsAttachment AttachmentData;
					AttachmentData.ActorClass = Attachment->GetClass();
					AttachmentData.AttachmentComponentName = AttachmentComponent->GetFName();
					AttachmentData.AttachmentOffset = AttachmentComponent->GetAttachmentOffset();
					Data.Attachments.Add(AttachmentData);
				}
			}
		}
		Data.Attachments.Shrink();
		return true;
	}
	return false;
}

bool USKGAttachmentHelpers::SerializeAttachmentManagerToJson(const USKGAttachmentManagerComponent* AttachmentManagerComponent, FString& SerializedString)
{
	FSKGAttachmentActor AttachmentData;
	if (ConstructAttachmentManagerData(AttachmentManagerComponent, AttachmentData))
	{
		return FJsonObjectConverter::UStructToJsonObjectString(AttachmentData, SerializedString);
	}
	return false;
}

bool USKGAttachmentHelpers::DeserializeAttachmentManagerJson(const FString& JsonString, FSKGAttachmentActor& Data)
{
	return !JsonString.IsEmpty() && FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &Data);
}

void USKGAttachmentHelpers::DestroyAttachments(USKGAttachmentManagerComponent* AttachmentManagerComponent)
{
	TArray<FSKGAttachmentComponentItem> AttachmentComponentItems = AttachmentManagerComponent->GetAttachmentComponents();
	for (const FSKGAttachmentComponentItem& Item : AttachmentComponentItems)
	{
		if (USKGAttachmentComponent* AttachmentComponent = Item.AttachmentComponent)
		{
			AttachmentComponent->DestroyAttachment();
		}
	}
}

void USKGAttachmentHelpers::SetupAttachments(USKGAttachmentManagerComponent* AttachmentManagerComponent,
	const FSKGAttachmentActor& Data)
{
	for (const FSKGActorsAttachment& Attachment : Data.Attachments)
	{
		if (Attachment)
		{
			TArray<FSKGAttachmentComponentItem> AttachmentComponentItems = AttachmentManagerComponent->GetAttachmentComponents();
			for (const FSKGAttachmentComponentItem& Item : AttachmentComponentItems)
			{
				if (USKGAttachmentComponent* AttachmentComponent = Item.AttachmentComponent)
				{
					if (!AttachmentComponent->HasAttachment() && AttachmentComponent->GetFName().IsEqual(Attachment.AttachmentComponentName) && AttachmentComponent->IsAttachmentClassCompatible(Attachment.ActorClass))
					{
						AttachmentComponent->SetAttachmentOffset(Attachment.AttachmentOffset);
						AttachmentComponent->TrySetupAttachment(Attachment.ActorClass);
						break;
					}
				}
			}
		}
	}
}

AActor* USKGAttachmentHelpers::ConstructActorFromAttachmentManagerData(const UObject* WorldContextObject, const FSKGAttachmentActor& Data)
{
	if (WorldContextObject && WorldContextObject->GetWorld() && Data)
	{
		if (AActor* Actor = WorldContextObject->GetWorld()->SpawnActor<AActor>(Data.ActorClass, FTransform()))
		{
			if (USKGAttachmentManagerComponent* AttachmentManagerComponent = GetAttachmentManagerComponent(Actor))
			{
				SCOPE_CYCLE_COUNTER(STAT_SKGConstructActorFromAttachmentManagerData);
				AttachmentManagerComponent->SetCanSpawnDefaultAttachments(false);
				
				DestroyAttachments(AttachmentManagerComponent);
				SetupAttachments(AttachmentManagerComponent, Data);

				AttachmentManagerComponent->SetCanSpawnDefaultAttachments(true);
			}
			else
			{
				Actor->Destroy();
			}
			return Actor;
		}
	}
	return nullptr;
}

void USKGAttachmentHelpers::ConstructExistingActorFromAttachmentManagerData(const AActor* Actor, const FSKGAttachmentActor& Data)
{
	if (Actor)
	{
		if (USKGAttachmentManagerComponent* AttachmentManagerComponent = GetAttachmentManagerComponent(Actor))
		{
			SCOPE_CYCLE_COUNTER(STAT_SKGConstructActorFromAttachmentManagerData);
			AttachmentManagerComponent->SetCanSpawnDefaultAttachments(false);
				
			DestroyAttachments(AttachmentManagerComponent);
			SetupAttachments(AttachmentManagerComponent, Data);

			AttachmentManagerComponent->SetCanSpawnDefaultAttachments(true);
		}
	}
}

bool USKGAttachmentHelpers::SaveStringToFile(const FString& Path, const FString& FileContent)
{
	return FFileHelper::SaveStringToFile(FileContent, *Path);
}

bool USKGAttachmentHelpers::LoadFileToString(const FString& Path, FString& OutString)
{
	return FFileHelper::LoadFileToString(OutString, *Path);
}

bool USKGAttachmentHelpers::GetAllFiles(FString Path, TArray<FString>& OutFiles)
{
	bool ValidFiles = false;
	if (FPaths::DirectoryExists(Path))
	{
		ValidFiles = true;
		Path.Append("*");
		FFileManagerGeneric::Get().FindFiles(OutFiles, *Path, true, false);
	}

	return ValidFiles;
}
