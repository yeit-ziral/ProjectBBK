// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Delegates/DelegateCombinations.h"
#include "C_ChaAbilitySystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FReceivedDamageDelegate, UC_ChaAbilitySystemComponent*, SourceASC, float, UnmitigatedDamage, float, MitigatedDamage);

UCLASS()
class PROJECTBBK_API UC_ChaAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public:
	bool characterAbilitiesGiven = false;
	bool startupEffectsApplied = false;

	FReceivedDamageDelegate receivedDamage;

	virtual void ReceiveDamage(UC_ChaAbilitySystemComponent* SourceASC, float UnmitigatedDamage, float MitigatedDamage);


};
