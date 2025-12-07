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

    // 자기 자신에게 적용되는 GE 델리게이트에 바인딩
    OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UC_MonsterASC::OnEffectAppliedToSelf);

    // GE 제거 델리게이트 (필요하면 사용)
    OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UC_MonsterASC::OnEffectRemoved);

    // Ability 종료 델리게이트
    OnAbilityEnded.AddUObject(this, &UC_MonsterASC::OnAbilityEndedCallback);
}

void UC_MonsterASC::HandleDeath()
{
    if (HasMatchingGameplayTag(TagStateDead))
    {
        return;
    }

    // 상태 태그 추가
    AddLooseGameplayTag(TagStateDead);

    // 앞으로 확장: State.Groggy 제거, 버프/디버프 제거, AI 중단 등
    // 예: 모든 Ability 취소
    CancelAllAbilities();

    OnMonsterDeath.Broadcast(this);
}

void UC_MonsterASC::InterruptCurrentAbilities()
{
    CancelAllAbilities();
}

bool UC_MonsterASC::CanUseAbilityByTag(FGameplayTag AbilityTag) const
{
    //  죽었거나 쿨타임일 때
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
