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

void UC_ChracterAttributeSetBase::OnRep_experience(const FGameplayAttributeData& OldExperience)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UC_ChracterAttributeSetBase, experience, OldExperience);
}

void UC_ChracterAttributeSetBase::OnRep_maxExperience(const FGameplayAttributeData& OldMaxExperience)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UC_ChracterAttributeSetBase, maxExperience, OldMaxExperience);
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

void UC_ChracterAttributeSetBase::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UC_ChracterAttributeSetBase, moveSpeed, OldMoveSpeed);
}

void UC_ChracterAttributeSetBase::OnRep_stamina(const FGameplayAttributeData& OldStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UC_ChracterAttributeSetBase, stamina, OldStamina);
}

void UC_ChracterAttributeSetBase::OnRep_maxStamina(const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UC_ChracterAttributeSetBase, maxStamina, OldMaxStamina);
}

void UC_ChracterAttributeSetBase::OnRep_ReceivedDamage(const FGameplayAttributeData& OldReceivedDamage)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UC_ChracterAttributeSetBase, receivedDamage, OldReceivedDamage);
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
	else if (Attribute == GetmaxStaminaAttribute())
	{
		AdjustAttributeForMaxChange(
			stamina,
			maxStamina,
			NewValue,
			GetstaminaAttribute()
		);
	}
	else if (Attribute == GetstaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetmaxStamina());
	}
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
			//UE_LOG(LogTemp, Warning, TEXT("=== MANA FULL! ULTIMATE READY! ==="));
			// TODO: 준비 완료 이벤트
		}
	}

	// ===== Stamina 처리 =====
	else if (Data.EvaluatedData.Attribute == GetstaminaAttribute())
	{
		Setstamina(FMath::Clamp(Getstamina(), 0.0f, GetmaxStamina()));

		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();

		// 태그 정의
		FGameplayTag EmptyTag = FGameplayTag::RequestGameplayTag(FName("State.Stamina.Empty"));
		FGameplayTag CanSprintTag = FGameplayTag::RequestGameplayTag(FName("State.CanSprint"));

		// 2. 스태미나가 0이 되었을 때 처리
		if (Getstamina() <= 0.0f)
		{
			if (!ASC->HasMatchingGameplayTag(EmptyTag))
			{
				// 스태미나 고갈 태그 추가
				ASC->AddLooseGameplayTag(EmptyTag);
				// 달리기 가능 태그 제거
				ASC->RemoveLooseGameplayTag(CanSprintTag);

				// 현재 실행 중인 모든 달리기 관련 어빌리티를 강제로 종료시킵니다.
				// GA_Sprint의 'Ability Tags' 섹션에 'State.Sprint'가 등록되어 있어야 합니다.
				FGameplayTagContainer CancelContainer;
				CancelContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Sprint")));

				// 이 함수가 실행되면 GA_Sprint의 OnEndAbility가 즉시 호출됩니다.
				ASC->CancelAbilities(&CancelContainer);
			}
		}

		// 3. 일정 이상(예: 20%) 회복되었을 때 태그 복구
		float RebuildThreshold = GetmaxStamina() * 0.2f; // 20% 지점
		if (Getstamina() >= RebuildThreshold)
		{
			if (ASC->HasMatchingGameplayTag(EmptyTag))
			{
				// 스태미나 고갈 태그 제거
				ASC->RemoveLooseGameplayTag(EmptyTag);
				// 다시 달리기 가능 태그 추가
				ASC->AddLooseGameplayTag(CanSprintTag);
			}
		}
	}

	// ===== Shield 처리 =====
	else if (Data.EvaluatedData.Attribute == GetshieldAttribute())
	{
		Setshield(FMath::Clamp(Getshield(), 0.0f, GetmaxShield()));
	}

	// ===== ⭐ Damage 처리 (기존 코드와 통합) =====
	// 
	//else if (Data.EvaluatedData.Attribute == GetdamageAttribute())
	//{
	//	const float LocalDamageDone = Getdamage();
	//	Setdamage(0.0f);  // Reset

	//	if (LocalDamageDone > 0.0f)
	//	{
	//		// Shield 먼저 감소
	//		const float OldShield = Getshield();
	//		const float NewShield = FMath::Max(0.0f, OldShield - LocalDamageDone);
	//		Setshield(NewShield);

	//		const float ShieldDamage = OldShield - NewShield;
	//		const float RemainingDamage = LocalDamageDone - ShieldDamage;

	//		// Health 감소
	//		if (RemainingDamage > 0.0f)
	//		{
	//			const float NewHealth = Gethealth() - RemainingDamage;
	//			Sethealth(FMath::Clamp(NewHealth, 0.0f, GetmaxHealth()));
	//		}

	//		// 데미지 델리게이트 호출
	//		if (UC_CharacterASC* TargetASC = Cast<UC_CharacterASC>(Data.Target.AbilityActorInfo->AbilitySystemComponent.Get()))
	//		{
	//			UC_CharacterASC* SourceASCCasted = Cast<UC_CharacterASC>(SourceASC);
	//			TargetASC->ReceiveDamage(SourceASCCasted, LocalDamageDone, LocalDamageDone);
	//		}
	//	}
	//}

	//else if (Data.EvaluatedData.Attribute == GetdamageAttribute())
	//{
	//	//Apply damage to shield first
	//	float RemainingDamage = Getdamage();
	//	if (RemainingDamage > 0.0f)
	//	{
	//		float CurrentShield = Getshield();

	//		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	//		if (ASC && ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("State.Guard.Active")))
	//		{
	//			// ���� ���� ���� ������ 0 ó��
	//			Setdamage(0.0f);

	//			// ������ �� �� ������ �ٷ� ����
	//			ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag("State.Guard.Broken"));
	//			ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag("State.Guard.Active"));

	//			return;
	//		}

	//		//this is for shield, need to change with upper codes
	//		if (CurrentShield > 0.0f)
	//		{
	//			if (RemainingDamage >= CurrentShield)
	//			{
	//				RemainingDamage -= CurrentShield;
	//				Setshield(0.0f);
	//			}
	//			else
	//			{
	//				Setshield(CurrentShield - RemainingDamage);
	//				RemainingDamage = 0.0f;
	//			}
	//		}
	//		//End shield application
	//	}
	//	//Apply remaining damage to health
	//	if (RemainingDamage > 0.0f)
	//	{
	//		float CurrentHealth = Gethealth();
	//		if (RemainingDamage >= CurrentHealth)
	//		{
	//			Sethealth(0.0f);
	//		}
	//		else
	//		{
	//			Sethealth(CurrentHealth - RemainingDamage); // Sethealth(FMath::Clamp(Gethealth(), 0.0f, GetmaxHealth()));
	//		}
	//	}
	//	//Reset damage to zero after applying
	//	Setdamage(0.0f);
	//}

	else if (Data.EvaluatedData.Attribute == GetreceivedDamageAttribute()) // damage -> ReceivedDamage로 변경
	{
		float RemainingDamage = GetreceivedDamage();
		UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();

		if (RemainingDamage > 0.0f && ASC)
		{
			// 1. State.Shield 태그 확인 (1회 무효화 로직)
			FGameplayTag ShieldTag = FGameplayTag::RequestGameplayTag(FName("State.Shield"));
			if (ASC->HasMatchingGameplayTag(ShieldTag))
			{
				// 데미지 완전 무효화
				SetreceivedDamage(0.0f);

				// 실드 제거 (LooseTag를 사용 중이시라면 RemoveLooseGameplayTag 사용)
				// 보통 GA에서 부여한 태그라면 RemoveActiveEffectsWithGrantedTags 등을 쓰기도 하지만, 
				// 직접 태그를 제어하신다면 아래 방식이 유지됩니다.
				ASC->RemoveLooseGameplayTag(ShieldTag);

				// 무효화되었으므로 이후 로직 중단
				return;
			}

			// 2. 가드 상태 확인 (기존 로직 유지 필요 시)
			FGameplayTag GuardTag = FGameplayTag::RequestGameplayTag(FName("State.Guard.Active"));
			if (ASC->HasMatchingGameplayTag(GuardTag))
			{
				SetreceivedDamage(0.0f);
				ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Guard.Broken")));
				ASC->RemoveLooseGameplayTag(GuardTag);
				return;
			}

			// 3. 실드 수치(Shield Attribute) 차감 로직
			float CurrentShield = Getshield();
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

			// 4. 남은 데미지를 체력(Health)에 적용
			if (RemainingDamage > 0.0f)
			{
				float CurrentHealth = Gethealth();
				// FMath::Clamp를 사용하여 안정적으로 수치 설정
				float NewHealth = FMath::Clamp(CurrentHealth - RemainingDamage, 0.0f, GetmaxHealth());
				Sethealth(NewHealth);
			}
		}

		// 데미지 처리 완료 후 초기화
		SetreceivedDamage(0.0f);
		}

	if (Data.EvaluatedData.Attribute == GetexperienceAttribute())
	{
		float currentExp = Getexperience();

		while (GetmaxExperience() > 0.f && currentExp >= GetmaxExperience())
		{
			currentExp -= GetmaxExperience();
			Setlevel(Getlevel() + 1);
			SetmaxExperience(FMath::RoundToFloat(GetmaxExperience() * 1.1f));
			SetmaxHealth(GetmaxHealth() + 50.f);
			Sethealth(FMath::Min(Gethealth() + 50.f, GetmaxHealth()));
			SetmaxStamina(GetmaxStamina() + 20.f);
			Setdamage(Getdamage() + 30.f);
		}

		Setexperience(FMath::Max(currentExp, 0.f));
	}
}

void UC_ChracterAttributeSetBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, level, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, experience, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, maxExperience, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, maxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, maxShield, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, maxMana, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, moveSpeed, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UC_ChracterAttributeSetBase, maxStamina, COND_None, REPNOTIFY_Always);
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
