// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Attributes/GSCAttributeSetBase.h"
#include "AbilitySystemComponent.h"
#include "PO_XPAttributeSet.generated.h"

UCLASS()
class PROJECTOLYMPUSALPHA_API UPO_XPAttributeSet : public UGSCAttributeSetBase
{
	GENERATED_BODY()

public:

	// Sets default values for this AttributeSet attributes
	UPO_XPAttributeSet();

    // AttributeSet Overrides
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
        
    UPROPERTY(BlueprintReadOnly, Category = "", ReplicatedUsing = OnRep_XPtoLevelUp)
    FGameplayAttributeData XPtoLevelUp = 0.0;
    ATTRIBUTE_ACCESSORS(UPO_XPAttributeSet, XPtoLevelUp)    
    
    UPROPERTY(BlueprintReadOnly, Category = "", ReplicatedUsing = OnRep_CurrentXP)
    FGameplayAttributeData CurrentXP = 0.0;
    ATTRIBUTE_ACCESSORS(UPO_XPAttributeSet, CurrentXP)    

protected:
    
    UFUNCTION()
    virtual void OnRep_XPtoLevelUp(const FGameplayAttributeData& OldXPtoLevelUp);

    UFUNCTION()
    virtual void OnRep_CurrentXP(const FGameplayAttributeData& OldCurrentXP);
	
	
};
