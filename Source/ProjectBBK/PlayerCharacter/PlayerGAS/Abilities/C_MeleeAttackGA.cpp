// Fill out your copyright notice in the Description page of Project Settings.

#include "C_MeleeAttackGA.h"
#include "AbilitySystemComponent.h"

void UC_MeleeAttackGA::ResetCombo()
{
	comboIndex = 0;
    bIsInputBuffered = false;
    bAdvancingCombo = false;

    if (AActor* Avatar = GetAvatarActorFromActorInfo())
    {
        Avatar->GetWorldTimerManager().ClearTimer(comboInputBufferTimerHandle);
    }

    swingHitActors.Reset();
}

void UC_MeleeAttackGA::BufferComboInput()
{
    bIsInputBuffered = true;

    // 스윙 아주 초반에 누른 입력이 윈도우까지 살아있으면 어색하다. 유효 시간을 걸어 오래된 입력은 스스로 사라지게 한다.
    if (AActor* Avatar = GetAvatarActorFromActorInfo())
    {
        Avatar->GetWorldTimerManager().SetTimer(
            comboInputBufferTimerHandle, this,
            &UC_MeleeAttackGA::OnComboInputBufferTimerExpired,
            inputBufferTime, false);
    }
}

bool UC_MeleeAttackGA::TryAdvanceCombo()
{
    if (!bIsInputBuffered)
        return false;

    if(comboMontages.Num() == 0)
		return false;

    // 스태미나 검사는 comboIndex를 올리기 "전"에 한다. 올린 뒤에 실패하면 인덱스만 어긋난 채 남아 다음 스윙이 엉뚱한 단부터 시작한다.
    if(!CanPayComboCost())
		return false;

    if (comboMontages.IsValidIndex(comboIndex + 1))
    {
		++comboIndex;
    }
    else
    {
		comboIndex = comboMontages.IsValidIndex(comboLoopStartIndex) ? comboLoopStartIndex : 0;
    }
	
	bIsInputBuffered = false;
	bAdvancingCombo = true;

    if(AActor* Avatar = GetAvatarActorFromActorInfo())
    {
        Avatar->GetWorldTimerManager().ClearTimer(comboInputBufferTimerHandle);
	}

    // 1타분은 활성화 시 CommitAbility가 이미 냈다. 2타부터는 여기서 낸다.
    PayComboCost();

    return true;
}

UAnimMontage* UC_MeleeAttackGA::GetCurrentComboMontage() const
{
	return comboMontages.IsValidIndex(comboIndex) ? comboMontages[comboIndex] : nullptr;
}

bool UC_MeleeAttackGA::ConsumeAdvancingFlag()
{
	const bool bWasAdvancing = bAdvancingCombo;
	bAdvancingCombo = false;
	return bWasAdvancing;
}

bool UC_MeleeAttackGA::TryRegisterHit(AActor* HitActor)
{
    if (!HitActor)
        return false;

    // Add는 이미 있던 원소면 bIsAlreadyInSet을 true로 채워준다. Contains + Add를 따로 부르는 것보다 해시 조회가 한 번 적다.
	bool bIsAlreadyInSet = false;
    swingHitActors.Add(HitActor, &bIsAlreadyInSet);
	return !bIsAlreadyInSet;
}

void UC_MeleeAttackGA::ClearSwingHits()
{
	swingHitActors.Reset();
}

bool UC_MeleeAttackGA::CanPayComboCost()
{
    // 콤보 전용 비용을 안 지정했으면 기존 경로 그대로(1타와 동일한 비용)
    if(!comboCostEffect)
		return CheckCost(CurrentSpecHandle, CurrentActorInfo);

    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC)
        return false;

    // CheckCost가 내부적으로 쓰는 것과 같은 함수. GE의 어트리뷰트 수정자를 지금 적용할 여력이 되는지만 본다.
    const UGameplayEffect* CostGE = comboCostEffect->GetDefaultObject<UGameplayEffect>();

    return ASC->CanApplyAttributeModifiers(
        CostGE,
        GetAbilityLevel(),
        MakeEffectContext(CurrentSpecHandle, CurrentActorInfo)
    );
}

void UC_MeleeAttackGA::PayComboCost()
{
    if (!comboCostEffect)
    {
        ApplyCost(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
		return;
    }

	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(comboCostEffect, GetAbilityLevel());

    if (Spec.IsValid())
    {
        ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, Spec);
	}
}

void UC_MeleeAttackGA::OnComboInputBufferTimerExpired()
{
    bIsInputBuffered = false;
}
