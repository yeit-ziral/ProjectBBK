// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "C_ChracterAttributeSetBase.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 
 */
UCLASS()
class PROJECTBBK_API UC_ChracterAttributeSetBase : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, Category = "level", ReplicatedUsing = OnRep_level)
	FGameplayAttributeData level;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, level)

	UPROPERTY(BlueprintReadOnly, Category = "health", ReplicatedUsing = OnRep_health)
	FGameplayAttributeData health;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, health)

	UPROPERTY(BlueprintReadOnly, Category = "health", ReplicatedUsing = OnRep_maxHealth)
	FGameplayAttributeData maxHealth;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, maxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "shield", ReplicatedUsing = OnRep_shield)
	FGameplayAttributeData shield;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, shield)

	UPROPERTY(BlueprintReadOnly, Category = "shield", ReplicatedUsing = OnRep_maxShield)
	FGameplayAttributeData maxShield;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, maxShield)

		//this is for damage calculation temporary use  So, not replicated
	UPROPERTY(BlueprintReadOnly, Category = "damage")
	FGameplayAttributeData damage;
	ATTRIBUTE_ACCESSORS(UC_ChracterAttributeSetBase, damage)

	UFUNCTION()
	virtual void OnRep_level(const FGameplayAttributeData& OldLevel);

	UFUNCTION()
	virtual void OnRep_health(const FGameplayAttributeData& OldHealth);
	UFUNCTION()
	virtual void OnRep_maxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	virtual void OnRep_shield(const FGameplayAttributeData& OldShield);
	UFUNCTION()
	virtual void OnRep_maxShield(const FGameplayAttributeData& OldMaxShield);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
};
