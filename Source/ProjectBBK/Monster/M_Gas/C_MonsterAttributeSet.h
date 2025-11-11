// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "C_MonsterAttributeSet.generated.h"

/**
 * 
 */

#ifndef ATTRIBUTE_ACCESSORS
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
#endif

UCLASS()
class PROJECTBBK_API UC_MonsterAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:    

    UC_MonsterAttributeSet();



#pragma region onRep functions
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_CurHP)
    FGameplayAttributeData CurHP;
    UFUNCTION() void OnRep_CurHP(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHP)
    FGameplayAttributeData MaxHP;
    UFUNCTION() void OnRep_MaxHP(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Groggy)
    FGameplayAttributeData CurGroggy;
    UFUNCTION() void OnRep_Groggy(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxGroggy)
    FGameplayAttributeData MaxGroggy;
    UFUNCTION() void OnRep_MaxGroggy(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Attack)
    FGameplayAttributeData Attack;
    UFUNCTION() void OnRep_Attack(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Defense)
    FGameplayAttributeData Defense;
    UFUNCTION() void OnRep_Defense(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MoveSpeed)
    FGameplayAttributeData MoveSpeed;
    UFUNCTION() void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_AttackRange)
    FGameplayAttributeData AttackRange;
    UFUNCTION() void OnRep_AttackRange(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_NormalCooldown)
    FGameplayAttributeData NormalCooldown;
    UFUNCTION() void OnRep_NormalCooldown(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_SpecialCooldown)
    FGameplayAttributeData SpecialCooldown;
    UFUNCTION() void OnRep_SpecialCooldown(const FGameplayAttributeData& OldValue);

#pragma endregion


public:
#pragma region StatAttributes
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,           CurHP)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,           MaxHP)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,       CurGroggy)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,       MaxGroggy)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,          Attack)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,         Defense)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,       MoveSpeed)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,     AttackRange)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,  NormalCooldown)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet, SpecialCooldown)
#pragma endregion

#pragma region Meta Attributes
    UPROPERTY(BlueprintReadOnly, Category = "Meta")
    FGameplayAttributeData ReceivedDamage;
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet, ReceivedDamage)

#pragma endregion   

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

};
