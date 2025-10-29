// Fill out your copyright notice in the Description page of Project Settings.


#include "C_ChaAbilitySystemComponent.h"

void UC_ChaAbilitySystemComponent::ReceiveDamage(UC_ChaAbilitySystemComponent* SourceASC, float UnmitigatedDamage, float MitigatedDamage)
{
	receivedDamage.Broadcast(SourceASC, UnmitigatedDamage, MitigatedDamage);
}
