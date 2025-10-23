// Fill out your copyright notice in the Description page of Project Settings.


#include "C_ChracterAttributeSetBase.h"
#include "Net/UnrealNetwork.h"

void UC_ChracterAttributeSetBase::OnRep_level(const FGameplayAttributeData& OldLevel)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UC_ChracterAttributeSetBase, level, OldLevel);
}

void UC_ChracterAttributeSetBase::OnRep_health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UC_ChracterAttributeSetBase, health, OldHealth);
}

void UC_ChracterAttributeSetBase::OnRep_maxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UC_ChracterAttributeSetBase, maxHealth, OldMaxHealth);
}

void UC_ChracterAttributeSetBase::OnRep_shield(const FGameplayAttributeData& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UC_ChracterAttributeSetBase, shield, OldShield);
}

void UC_ChracterAttributeSetBase::OnRep_maxShield(const FGameplayAttributeData& OldMaxShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UC_ChracterAttributeSetBase, maxShield, OldMaxShield);
}

void UC_ChracterAttributeSetBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, level, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, maxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, maxShield, COND_None, REPNOTIFY_Always);
}
