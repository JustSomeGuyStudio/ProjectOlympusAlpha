#pragma once

#include "Net/Serialization/FastArraySerializer.h"
#include "SKGAttachmentDataTypes.generated.h"

class USKGAttachmentComponent;

USTRUCT(BlueprintType)
struct FSKGAttachmentComponentItem : public FFastArraySerializerItem
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "SKGAttachmentDataTypes")
	TObjectPtr<USKGAttachmentComponent> AttachmentComponent;
	
	FSKGAttachmentComponentItem() {}
	FSKGAttachmentComponentItem(USKGAttachmentComponent* Component)
	{
		AttachmentComponent = Component;
	}

	bool operator == (const FSKGAttachmentComponentItem& RHS) const
	{
		return AttachmentComponent == RHS.AttachmentComponent;
	}

	bool operator == (const USKGAttachmentComponent* RHS) const
	{
		return AttachmentComponent == RHS;
	}
};

USTRUCT(BlueprintType)
struct FSKGAttachmentComponentItems : public FFastArraySerializer
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<FSKGAttachmentComponentItem> Items;

	// Returns true if added, false if match was found
	bool AddUnique(USKGAttachmentComponent* AttachmentComponent)
	{
		for (const FSKGAttachmentComponentItem& ExistingItem : Items)
		{
			if (ExistingItem == AttachmentComponent)
			{
				return false;
			}
		}
		Add(AttachmentComponent);
		return true;
	}
	
	void Add(USKGAttachmentComponent* AttachmentComponent)
	{
		MarkItemDirty(Items.Add_GetRef(FSKGAttachmentComponentItem(AttachmentComponent)));
	}
	// Returns true if successful removal
	bool Remove(USKGAttachmentComponent* AttachmentComponent)
	{
		if (Items.Remove(AttachmentComponent))
		{
			MarkArrayDirty();
			return true;
		}
		return false;
	}

	void RemoveAt(int32 Index)
	{
		if (Index < Items.Num())
		{
			Items.RemoveAt(Index, 1, true);
			MarkArrayDirty();
		}
	}

	void RemoveAt(TArray<int32>& Indexes)
	{
		for (const int32 i : Indexes)
		{
			Items.RemoveAt(i, 1, false);
		}
		Items.Shrink();
		MarkArrayDirty();
	}

	int32 Num() const
	{
		return Items.Num();
	}
	
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FastArrayDeltaSerialize<FSKGAttachmentComponentItem, FSKGAttachmentComponentItems>(Items, DeltaParams, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FSKGAttachmentComponentItems> : public TStructOpsTypeTraitsBase2<FSKGAttachmentComponentItems>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};

/* Data structure that represents an actors attachment.
 * Used for serializing and deserializing to reconstructing an actor.
 */
USTRUCT(BlueprintType)
struct FSKGActorsAttachment
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSKGAttachment")
	TSubclassOf<AActor> ActorClass;
	// The name of the component the attachment is attached to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSKGAttachment")
	FName AttachmentComponentName {""};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSKGAttachment")
	float AttachmentOffset {0.0f};
	// Populate with more info as time goes on such as offset

	operator bool () const
	{
		return ActorClass != nullptr;
	}
};

/* Data structure that represents an actor with all attachments.
 * Used for serializing and deserializing to reconstructing an actor.
 */
USTRUCT(BlueprintType)
struct FSKGAttachmentActor
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSKGAttachmentActor")
	TSubclassOf<AActor> ActorClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FSKGAttachmentActor")
	TArray<FSKGActorsAttachment> Attachments;

	operator bool () const
	{
		return ActorClass != nullptr;
	}
};