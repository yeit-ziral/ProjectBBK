// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MonsterASC.h"

UC_MonsterASC::UC_MonsterASC()
{
	TagStateDead = FGameplayTag::RequestGameplayTag(FName("State.Dead"));
	TagStateGroggy = FGameplayTag::RequestGameplayTag(FName("State.Groggy"));
}

void UC_MonsterASC::BeginPlay()
{
    Super::BeginPlay();

    // �ڱ� �ڽſ��� ����Ǵ� GE ��������Ʈ�� ���ε�
    OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UC_MonsterASC::OnEffectAppliedToSelf);

    // GE ���� ��������Ʈ (�ʿ��ϸ� ���)
    OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UC_MonsterASC::OnEffectRemoved);

    // Ability ���� ��������Ʈ
    OnAbilityEnded.AddUObject(this, &UC_MonsterASC::OnAbilityEndedCallback);
}

void UC_MonsterASC::HandleDeath()
{
    if (HasMatchingGameplayTag(TagStateDead))
    {
        return;
    }

    // ���� �±� �߰�
    AddLooseGameplayTag(TagStateDead);

    // 그로기 중 사망 시 충돌 방지 — Groggy 태그 제거
    if (HasMatchingGameplayTag(TagStateGroggy))
    {
        RemoveLooseGameplayTag(TagStateGroggy);
    }

    // 모든 Ability 취소
    CancelAllAbilities();

    // State.RemoveOnDeath 태그가 부여된 GE 제거 (DoT, 상태이상 등)
    FGameplayTagContainer RemoveTags;
    RemoveTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.RemoveOnDeath")));
    RemoveActiveEffectsWithGrantedTags(RemoveTags);

    OnMonsterDeath.Broadcast(this);
}

void UC_MonsterASC::InterruptCurrentAbilities()
{
    CancelAllAbilities();
}

bool UC_MonsterASC::CanUseAbilityByTag(FGameplayTag AbilityTag) const
{
    //  �׾��ų� ��Ÿ���� ��
    if (HasMatchingGameplayTag(TagStateDead) || HasMatchingGameplayTag(TagStateGroggy))
    {
        return false;
    }

    
    return true;
}


void UC_MonsterASC::OnEffectAppliedToSelf(UAbilitySystemComponent* SourceASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle)
{
}

void UC_MonsterASC::OnEffectRemoved(const FActiveGameplayEffect& ActiveEffect)
{
}

void UC_MonsterASC::OnAbilityEndedCallback(const FAbilityEndedData& Data)
{
}
