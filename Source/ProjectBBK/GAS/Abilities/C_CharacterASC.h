// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Delegates/DelegateCombinations.h"
#include "C_CharacterASC.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FReceivedDamageDelegate, UC_CharacterASC*, SourceASC, float, UnmitigatedDamage, float, MitigatedDamage);

UCLASS()
class PROJECTBBK_API UC_CharacterASC : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public:
	virtual void ReceiveDamage(UC_CharacterASC* SourceASC, float UnmitigatedDamage, float MitigatedDamage);

public:
	bool characterAbilitiesGiven = false;
	bool startupEffectsApplied = false;

	FReceivedDamageDelegate receivedDamage;
};
