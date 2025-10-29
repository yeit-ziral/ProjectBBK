// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GGBaseSet.h"
#include "AbilitySystemComponent.h"
#include "GGHealthSet.generated.h"

DECLARE_MULTICAST_DELEGATE_FourParams(FDamageTakenEvent, AActor* /*EffectInstigator*/, AActor* /*EffectCauser*/, const FGameplayTagContainer& /*GameplayTagContainer*/, float /*Damage*/);

/**
 * 
 */
UCLASS()
class PROJECTBBK_API UGGHealthSet : public UGGBaseSet
{
	GENERATED_BODY()

public:
	UGGHealthSet();

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UGGHealthSet, Health);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UGGHealthSet, MaxHealth);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(UGGHealthSet, Shield);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxShield;
	ATTRIBUTE_ACCESSORS(UGGHealthSet, MaxShield);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData ShieldRegen;
	ATTRIBUTE_ACCESSORS(UGGHealthSet, ShieldRegen);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData InDamage;
	ATTRIBUTE_ACCESSORS(UGGHealthSet, InDamage);

	mutable FDamageTakenEvent OnDamageTaken;
	
protected:
	virtual void ClampAttributeOnChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
};
