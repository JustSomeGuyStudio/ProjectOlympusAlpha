// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SKGStockComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFolded, bool, Folded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLengthOfPullChanged, float, NewLengthOfPull);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SKGSTOCK_API USKGStockComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USKGStockComponent();

protected:
	// If true the stock can be folded/collapsed (think an underfolding AK or MCX)
	UPROPERTY(EditDefaultsOnly, BlueprintGetter = CanBeFolded, Category = "SKGStock|Initialize")
	bool bCanBeFolded {false};
	UPROPERTY(EditDefaultsOnly, BlueprintGetter = CanBeFolded, Category = "SKGStock|Initialize")
	bool bCanBeAffectedByOffset {true};
	// The length of pull when the stock is unfolded/open
	UPROPERTY(EditDefaultsOnly, BlueprintGetter = GetUnfoldedLengthOfPull, Category = "SKGStock|Initialize")
	float UnfoldedLengthOfPull {0.0f};
	// The length of pull when the stock is folded/collapsed
	UPROPERTY(EditDefaultsOnly, BlueprintGetter = GetFoldedLengthOfPull, Category = "SKGStock|Initialize", meta = (EditCondition = "bCanBeFolded"))
	float FoldedLengthOfPull {0.0f};
	// Useful if your NetUpdateFrequency is set super low
	UPROPERTY(EditDefaultsOnly, Category = "SKGStock|Initialize")
	bool bAutoCallForceNetUpdate {true};

	UPROPERTY(ReplicatedUsing = OnRep_IsFolded)
	bool bIsFolded {false};
	UFUNCTION()
	void OnRep_IsFolded() const;

	UPROPERTY(ReplicatedUsing = OnRep_IsFolded)
	float LengthOfPull {0.0f};
	UFUNCTION()
	void OnRep_LengthOfPull() const;

	float Offset {0.0f};
	
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	FORCEINLINE bool HasAuthority() const { return GetOwnerRole() == ROLE_Authority; }
	FORCEINLINE void TryForceNetUpdate() const;

	void SetLengthOfPull();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetFoldedState(bool bFold);
	
public:
	/**
	 * This is to be called server side and useful alongside an attachment system.
	 * If you slide the stock out (like an m4 with buffer tube) or in, this offset can be applied in addition.
	 */
	UFUNCTION(BlueprintCallable, Category = "SKGStock")
	void SetOffset(float PositionOffset);
	UFUNCTION(BlueprintCallable, Category = "SKGStock")
	void Fold();
	UFUNCTION(BlueprintCallable, Category = "SKGStock")
	void Unfold();
	UFUNCTION(BlueprintCallable, Category = "SKGStock")
	void ToggleFold();
	UFUNCTION(BlueprintPure, Category = "SKGStock")
	bool IsFolded() const { return bIsFolded; }
	UFUNCTION(BlueprintPure, Category = "SKGStock")
	float GetLengthOfPull() const { return LengthOfPull; }
	
	UFUNCTION(BlueprintGetter)
	bool CanBeFolded() const { return bCanBeFolded; }
	UFUNCTION(BlueprintGetter)
	float GetUnfoldedLengthOfPull() const { return UnfoldedLengthOfPull; }
	UFUNCTION(BlueprintGetter)
	float GetFoldedLengthOfPull() const { return FoldedLengthOfPull; }

	UPROPERTY(BlueprintAssignable, Category = "SKGStock|Events")
	FOnFolded OnFolded;
	UPROPERTY(BlueprintAssignable, Category = "SKGStock|Events")
	FOnLengthOfPullChanged OnLengthOfPullChanged;
};
