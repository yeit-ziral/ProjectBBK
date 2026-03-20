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
    FGameplayAttributeData curHP;
    UFUNCTION() void OnRep_CurHP(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHP)
    FGameplayAttributeData maxHP;
    UFUNCTION() void OnRep_MaxHP(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Groggy)
    FGameplayAttributeData curGroggy;
    UFUNCTION() void OnRep_Groggy(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxGroggy)
    FGameplayAttributeData maxGroggy;
    UFUNCTION() void OnRep_MaxGroggy(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Attack)
    FGameplayAttributeData attack;
    UFUNCTION() void OnRep_Attack(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Defense)
    FGameplayAttributeData defense;
    UFUNCTION() void OnRep_Defense(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MoveSpeed)
    FGameplayAttributeData moveSpeed;
    UFUNCTION() void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_AttackRange)
    FGameplayAttributeData attackRange;
    UFUNCTION() void OnRep_AttackRange(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_NormalCooldown)
    FGameplayAttributeData normalCooldown;
    UFUNCTION() void OnRep_NormalCooldown(const FGameplayAttributeData& OldValue);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_SpecialCooldown)
    FGameplayAttributeData specialCooldown;
    UFUNCTION() void OnRep_SpecialCooldown(const FGameplayAttributeData& OldValue);


protected:

	void ChargeAttackerMana(const FGameplayEffectModCallbackData& Data, float ActualDamage);

	// Mana 충전 GE
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Mana")
	TSubclassOf<UGameplayEffect> GE_ChargeMana;

	// 충전량 설정
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Mana",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ManaChargeRate = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Mana")
	float MinManaCharge = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Mana")
	float MaxManaCharge = 20.0f;
#pragma endregion


public:
#pragma region StatAttributes
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,           curHP)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,           maxHP)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,       curGroggy)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,       maxGroggy)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,          attack)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,         defense)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,       moveSpeed)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,     attackRange)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet,  normalCooldown)
    ATTRIBUTE_ACCESSORS(UC_MonsterAttributeSet, specialCooldown)
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
