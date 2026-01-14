// Fill out your copyright notice in the Description page of Project Settings.


#include "C_MeleeMonsterNormalAttackGA.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

UC_MeleeMonsterNormalAttackGA::UC_MeleeMonsterNormalAttackGA()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UC_MeleeMonsterNormalAttackGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlayAttackMontageAndBindEvents();
}

void UC_MeleeMonsterNormalAttackGA::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UC_MeleeMonsterNormalAttackGA::PlayAttackMontageAndBindEvents()
{
	if (!attackMontage)
	{
		// 몽타주가 없으면 그냥 종료
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, true);
		return;
	}

	// 1) 몽타주 재생
	UAbilityTask_PlayMontageAndWait* montageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy
	(
		this, NAME_None, attackMontage, 1.0f, NAME_None, false
	);

	if (montageTask)
	{
		montageTask->OnCompleted.AddDynamic(this, &UC_MeleeMonsterNormalAttackGA::OnMontageCompleted);
		montageTask->OnInterrupted.AddDynamic(this, &UC_MeleeMonsterNormalAttackGA::OnMontageCancelled);
		montageTask->OnCancelled.AddDynamic(this, &UC_MeleeMonsterNormalAttackGA::OnMontageCancelled);
		montageTask->ReadyForActivation();
	}

	// 2) 히트 타이밍 이벤트 대기
	if (hitEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* hitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, hitEventTag, nullptr, false, false);

		if (hitEventTask)
		{
			hitEventTask->EventReceived.AddDynamic(this, &UC_MeleeMonsterNormalAttackGA::OnHitEventReceived);
			hitEventTask->ReadyForActivation();
		}
	}
}

void UC_MeleeMonsterNormalAttackGA::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UC_MeleeMonsterNormalAttackGA::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UC_MeleeMonsterNormalAttackGA::OnHitEventReceived(FGameplayEventData Payload)
{
	TArray<AActor*> targetActors = UAbilitySystemBlueprintLibrary::GetActorsFromTargetData(Payload.TargetData, 0);

	if (targetActors.Num() > 0 && targetActors[0])
	{
		ApplyDamageToTarget(targetActors[0]);
	}
}

void UC_MeleeMonsterNormalAttackGA::ApplyDamageToTarget(AActor* TargetActor)
{
	if (!damageEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* sourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!sourceASC)
	{
		return;
	}

	// 타겟 ASC 구하기
	UAbilitySystemComponent* targetASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);

	if (!targetASC)
	{
		return;
	}

	FGameplayEffectContextHandle effectContext = sourceASC->MakeEffectContext();
	effectContext.AddInstigator(CurrentActorInfo->AvatarActor.Get(), CurrentActorInfo->OwnerActor.Get());

	FGameplayEffectSpecHandle specHandle =
		sourceASC->MakeOutgoingSpec(damageEffectClass, GetAbilityLevel(), effectContext);

	if (!specHandle.IsValid())
	{
		return;
	}

	// SetByCaller로 데미지 전달
	if (setByCallerDamageTag.IsValid())
	{
		specHandle.Data->SetSetByCallerMagnitude(setByCallerDamageTag, baseDamage);
	}

	// 실제 적용
	sourceASC->ApplyGameplayEffectSpecToTarget(*specHandle.Data.Get(), targetASC);
}
