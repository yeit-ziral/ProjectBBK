// Fill out your copyright notice in the Description page of Project Settings.


#include "C_ChracterAttributeSetBase.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "../Abilities/C_CharacterASC.h"


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

void UC_ChracterAttributeSetBase::OnRep_mana(const FGameplayAttributeData& OldMana)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UC_ChracterAttributeSetBase, mana, OldMana);
}

void UC_ChracterAttributeSetBase::OnRep_maxMana(const FGameplayAttributeData& OldMaxMana)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UC_ChracterAttributeSetBase, maxMana, OldMaxMana);
}

void UC_ChracterAttributeSetBase::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GethealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetmaxHealth());
	}
	else if (Attribute == GetmanaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetmaxMana());
	}
	//else if (Attribute == GetStaminaAttribute())
	//{
	//	NewValue = FMath::Clamp(NewValue, 0.0f, GetmaxStamina());
	//}
	else if (Attribute == GetshieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetmaxShield());
	}
}

void UC_ChracterAttributeSetBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	UAbilitySystemComponent* SourceASC = Context.GetOriginalInstigatorAbilitySystemComponent();
	const FGameplayTagContainer& SourceTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();

	// Get target (this character)
	AActor* TargetActor = nullptr;
	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
	}

	// ===== Health 처리 =====
	if (Data.EvaluatedData.Attribute == GethealthAttribute())
	{
		Sethealth(FMath::Clamp(Gethealth(), 0.0f, GetmaxHealth()));

		// 죽음 처리
		if (Gethealth() <= 0.0f)
		{
			// TODO: 죽음 이벤트
			UE_LOG(LogTemp, Warning, TEXT("Character Dead!"));
		}
	}

	// ===== ⭐ Mana 처리 (궁극기 게이지) =====
	else if (Data.EvaluatedData.Attribute == GetmanaAttribute())
	{
		Setmana(FMath::Clamp(Getmana(), 0.0f, GetmaxMana()));

		// ⭐ UI 업데이트 델리게이트 호출
		// BP에서 OnManaChanged 이벤트로 받음

		// 100% 도달 시
		if (Getmana() >= GetmaxMana())
		{
			UE_LOG(LogTemp, Warning, TEXT("=== MANA FULL! ULTIMATE READY! ==="));
			// TODO: 준비 완료 이벤트
		}
	}

	//// ===== Stamina 처리 =====
	//else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	//{
	//	Setstamina(FMath::Clamp(Getstamina(), 0.0f, GetmaxStamina()));
	//}

	// ===== Shield 처리 =====
	else if (Data.EvaluatedData.Attribute == GetshieldAttribute())
	{
		Setshield(FMath::Clamp(Getshield(), 0.0f, GetmaxShield()));
	}

	// ===== ⭐ Damage 처리 (기존 코드와 통합) =====
	else if (Data.EvaluatedData.Attribute == GetdamageAttribute())
	{
		const float LocalDamageDone = Getdamage();
		Setdamage(0.0f);  // Reset

		if (LocalDamageDone > 0.0f)
		{
			// Shield 먼저 감소
			const float OldShield = Getshield();
			const float NewShield = FMath::Max(0.0f, OldShield - LocalDamageDone);
			Setshield(NewShield);

			const float ShieldDamage = OldShield - NewShield;
			const float RemainingDamage = LocalDamageDone - ShieldDamage;

			// Health 감소
			if (RemainingDamage > 0.0f)
			{
				const float NewHealth = Gethealth() - RemainingDamage;
				Sethealth(FMath::Clamp(NewHealth, 0.0f, GetmaxHealth()));
			}

			// 데미지 델리게이트 호출
			if (UC_CharacterASC* TargetASC = Cast<UC_CharacterASC>(Data.Target.AbilityActorInfo->AbilitySystemComponent.Get()))
			{
				UC_CharacterASC* SourceASCCasted = Cast<UC_CharacterASC>(SourceASC);
				TargetASC->ReceiveDamage(SourceASCCasted, LocalDamageDone, LocalDamageDone);
			}
		}
	}
}

void UC_ChracterAttributeSetBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, level, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, maxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, maxShield, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, maxMana, COND_None, REPNOTIFY_Always);

	//DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, stamina, COND_None, REPNOTIFY_Always);
	//DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, maxStamina, COND_None, REPNOTIFY_Always);
}

void UC_ChracterAttributeSetBase::AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty)
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	const float CurrentMaxValue = MaxAttribute.GetCurrentValue();

	if (!FMath::IsNearlyEqual(CurrentMaxValue, NewMaxValue) && ASC)
	{
		const float CurrentValue = AffectedAttribute.GetCurrentValue();
		const float NewDelta = (CurrentMaxValue > 0.f) ? (CurrentValue * NewMaxValue / CurrentMaxValue) - CurrentValue : NewMaxValue;

		ASC->ApplyModToAttributeUnsafe(AffectedAttributeProperty, EGameplayModOp::Additive, NewDelta);
	}
}
