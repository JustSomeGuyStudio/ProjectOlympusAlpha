// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Attributes/GSCAttributeSetBase.h"
#include "AbilitySystemComponent.h"
#include "PO_AttributeSet.generated.h"

UCLASS()
class PROJECTOLYMPUSALPHA_API UPO_AttributeSet : public UGSCAttributeSetBase
{
	GENERATED_BODY()

public:

	// Sets default values for this AttributeSet attributes
	UPO_AttributeSet();

    // AttributeSet Overrides
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
        
    UPROPERTY(BlueprintReadOnly, Category = "", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth = 0.0;
    ATTRIBUTE_ACCESSORS(UPO_AttributeSet, MaxHealth)    
    
    UPROPERTY(BlueprintReadOnly, Category = "", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health = 0.0;
    ATTRIBUTE_ACCESSORS(UPO_AttributeSet, Health)    
    
    UPROPERTY(BlueprintReadOnly, Category = "", ReplicatedUsing = OnRep_MaxArmor)
    FGameplayAttributeData MaxArmor = 0.0;
    ATTRIBUTE_ACCESSORS(UPO_AttributeSet, MaxArmor)    
    
    UPROPERTY(BlueprintReadOnly, Category = "", ReplicatedUsing = OnRep_Armor)
    FGameplayAttributeData Armor = 0.0;
    ATTRIBUTE_ACCESSORS(UPO_AttributeSet, Armor)    
    
    UPROPERTY(BlueprintReadOnly, Category = "", ReplicatedUsing = OnRep_MaxStamina)
    FGameplayAttributeData MaxStamina = 0.0;
    ATTRIBUTE_ACCESSORS(UPO_AttributeSet, MaxStamina)    
    
    UPROPERTY(BlueprintReadOnly, Category = "", ReplicatedUsing = OnRep_Stamina)
    FGameplayAttributeData Stamina = 0.0;
    ATTRIBUTE_ACCESSORS(UPO_AttributeSet, Stamina)    

protected:
    
    UFUNCTION()
    virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

    UFUNCTION()
    virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

    UFUNCTION()
    virtual void OnRep_MaxArmor(const FGameplayAttributeData& OldMaxArmor);

    UFUNCTION()
    virtual void OnRep_Armor(const FGameplayAttributeData& OldArmor);

    UFUNCTION()
    virtual void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina);

    UFUNCTION()
    virtual void OnRep_Stamina(const FGameplayAttributeData& OldStamina);
	
	
};
