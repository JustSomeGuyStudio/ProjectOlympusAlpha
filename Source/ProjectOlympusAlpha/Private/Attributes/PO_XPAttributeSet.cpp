// Copyright 2023, Dakota Dawe, All rights reserved


#include "Attributes/PO_XPAttributeSet.h"

#include "Net/UnrealNetwork.h"

// Sets default values
UPO_XPAttributeSet::UPO_XPAttributeSet()
{
	// Default constructor
}

void UPO_XPAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    // This is called whenever attributes change, so for max attributes we want to scale the current totals to match
    Super::PreAttributeChange(Attribute, NewValue);

    // Set adjust code here
    //
    // Example:
    //
    // If a Max value changes, adjust current to keep Current % of Current to Max
    //
    // if (Attribute == GetMaxHealthAttribute())
    // {
    //     AdjustAttributeForMaxChange(Health, MaxHealth, NewValue, GetHealthAttribute());
    // }
}

void UPO_XPAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    FGSCAttributeSetExecutionData ExecutionData;
    GetExecutionDataFromMod(Data, ExecutionData);

    // Set clamping or handling or "meta" attributes here (like damages)

    // Example 1: Using helpers to handle each attribute in their own methods (See GSCAttributeSet.cpp)

    // if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    // {
    //     HandleHealthAttribute(ExecutionData);
    // }

    // Example 2: Basic example to clamp the value of an Health Attribute between 0 and another MaxHealth Attribute

    // if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    // {
    //     SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
    // }
}

void UPO_XPAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        
    DOREPLIFETIME_CONDITION_NOTIFY(UPO_XPAttributeSet, XPtoLevelUp, COND_None, REPNOTIFY_Always);
    
    DOREPLIFETIME_CONDITION_NOTIFY(UPO_XPAttributeSet, CurrentXP, COND_None, REPNOTIFY_Always);
}

void UPO_XPAttributeSet::OnRep_XPtoLevelUp(const FGameplayAttributeData& OldXPtoLevelUp)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UPO_XPAttributeSet, XPtoLevelUp, OldXPtoLevelUp);
}

void UPO_XPAttributeSet::OnRep_CurrentXP(const FGameplayAttributeData& OldCurrentXP)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UPO_XPAttributeSet, CurrentXP, OldCurrentXP);
}
