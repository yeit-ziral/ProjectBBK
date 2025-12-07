// Fill out your copyright notice in the Description page of Project Settings.


#include "C_ChracterAttributeSetBase.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

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

void UC_ChracterAttributeSetBase::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetmaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f, GetmaxHealth()); //this is to prevent max health to be negative value or over max health

		////Adjust current health to keep the same percentage when max health changes
		//AdjustAttributeForMaxChange(health, maxHealth, NewValue);
	}
	else if (Attribute == GetmaxShieldAttribute())
	{
		////Adjust current shield to keep the same percentage when max shield changes
		//AdjustAttributeForMaxChange(shield, maxShield, NewValue);
	}
}

void UC_ChracterAttributeSetBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetdamageAttribute())
	{
		//Apply damage to shield first
		float RemainingDamage = Getdamage();
		if (RemainingDamage > 0.0f)
		{
			float CurrentShield = Getshield();

			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
			if (ASC && ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("State.Guard.Active")))
			{
				// 가드 중일 때는 데미지 0 처리
				Setdamage(0.0f);

				// 가드는 한 번 막으면 바로 깨짐
				ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag("State.Guard.Broken"));
				ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag("State.Guard.Active"));

				return;
			}

			 //this is for shield, need to change with upper codes
			if (CurrentShield > 0.0f)
			{
				if (RemainingDamage >= CurrentShield)
				{
					RemainingDamage -= CurrentShield;
					Setshield(0.0f);
				}
				else
				{
					Setshield(CurrentShield - RemainingDamage);
					RemainingDamage = 0.0f;
				}
			}
			//End shield application
		}
		//Apply remaining damage to health
		if (RemainingDamage > 0.0f)
		{
			float CurrentHealth = Gethealth();
			if (RemainingDamage >= CurrentHealth)
			{
				Sethealth(0.0f);
			}
			else
			{
				Sethealth(CurrentHealth - RemainingDamage); // Sethealth(FMath::Clamp(Gethealth(), 0.0f, GetmaxHealth()));
			}
		}
		//Reset damage to zero after applying
		Setdamage(0.0f);
	}
}
