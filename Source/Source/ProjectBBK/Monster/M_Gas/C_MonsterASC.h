// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "C_MonsterASC.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTBBK_API UC_MonsterASC : public UAbilitySystemComponent
{
	GENERATED_BODY()
public:
    UC_MonsterASC();

    // 몬스터가 죽었을 때 호출 
    UFUNCTION()
    void HandleDeath();

    // 현재 진행 중인 능력을 중단할 때 호출
    UFUNCTION()
    void InterruptCurrentAbilities();

    // 태그 기반으로 이 Ability를 사용할 수 있는지 검사
    UFUNCTION(BlueprintCallable, Category = "Monster|ASC")
    bool CanUseAbilityByTag(FGameplayTag AbilityTag) const;

    // 사망 알림
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnMonsterDeath, UC_MonsterASC*);
    FOnMonsterDeath OnMonsterDeath;
protected:
    virtual void BeginPlay() override;

    // GE가 자기 자신에게 적용될 때 호출되는 델리게이트에 바인딩
    UFUNCTION()
    void OnEffectAppliedToSelf(UAbilitySystemComponent* SourceASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);

    // GE가 제거될 때
    UFUNCTION()
    void OnEffectRemoved(const FActiveGameplayEffect& ActiveEffect);

    // Ability가 끝났을 때
    UFUNCTION()
    void OnAbilityEndedCallback(const FAbilityEndedData& Data);

protected:
    FGameplayTag TagStateDead;
    FGameplayTag TagStateGroggy;
	
};
