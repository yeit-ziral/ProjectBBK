// Fill out your copyright notice in the Description page of Project Settings.


#include "GGHealthSet.h"
#include "GameplayEffectExtension.h"

UGGHealthSet::UGGHealthSet()
	: Health(40.0f), MaxHealth(100.0f), Shield(0.0f), MaxShield(50.0f), ShieldRegen(5.0f)
{
}

void UGGHealthSet::ClampAttributeOnChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxShield());
	}
}

void UGGHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if(Data.EvaluatedData.Attribute == GetInDamageAttribute())
	{
		const float DamageDone = GetInDamage();
		if(DamageDone > 0.0f)
		{
			float InDamageDone = GetInDamage();
			SetInDamage(0.0f);
			if (InDamageDone >= 0.0f)
			{
				if(OnDamageTaken.IsBound())
				{
					const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetContext();
					AActor* Instigator = EffectContext.GetOriginalInstigator();
					AActor* Causer = EffectContext.GetEffectCauser();

					OnDamageTaken.Broadcast(Instigator, Causer, Data.EffectSpec.CapturedSourceTags.GetSpecTags(), Data.EvaluatedData.Magnitude);
				}

				if (GetShield() > 0.0f)
				{
					const float NewShield = GetShield() - InDamageDone;	
					InDamageDone -= GetShield();
					SetShield(FMath::Clamp(NewShield, 0.0f, GetMaxShield()));
				}

				if(InDamageDone >0.0f && GetHealth() > 0.0f)
				{
					const float NewHealth = GetHealth() - InDamageDone;
					SetHealth(FMath::Clamp(NewHealth, 0.0f, GetMaxHealth()));
				}
			}
		}
	}
}
