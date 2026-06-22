// Fill out your copyright notice in the Description page of Project Settings.

#include "C_EliteMonsterSpecialAttackGA_Swamp.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "../../Object/C_EliteSwampZone.h"

UC_EliteMonsterSpecialAttackGA_Swamp::UC_EliteMonsterSpecialAttackGA_Swamp()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UC_EliteMonsterSpecialAttackGA_Swamp::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 장판은 즉시 스폰 (몬스터에 부착되어 자체 생명주기로 동작)
	SpawnZone();

	// 시전 몽타주가 있으면 재생 후 종료, 없으면 즉시 종료
	if (castMontage)
	{
		UAbilityTask_PlayMontageAndWait* montageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, castMontage, 1.0f, NAME_None, false);
		if (montageTask)
		{
			montageTask->OnCompleted.AddDynamic(this, &UC_EliteMonsterSpecialAttackGA_Swamp::OnMontageEnded);
			montageTask->OnInterrupted.AddDynamic(this, &UC_EliteMonsterSpecialAttackGA_Swamp::OnMontageEnded);
			montageTask->OnCancelled.AddDynamic(this, &UC_EliteMonsterSpecialAttackGA_Swamp::OnMontageEnded);
			montageTask->ReadyForActivation();
			return;
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

void UC_EliteMonsterSpecialAttackGA_Swamp::SpawnZone()
{
	if (!swampZoneClass) return;

	AActor* avatar = GetAvatarActorFromActorInfo();
	if (!avatar) return;

	UWorld* world = avatar->GetWorld();
	if (!world) return;

	FActorSpawnParameters params;
	params.Owner = avatar;
	params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AC_EliteSwampZone* zone = world->SpawnActor<AC_EliteSwampZone>(
		swampZoneClass, avatar->GetActorLocation(), FRotator::ZeroRotator, params);
	if (!zone) return;

	// 몬스터에 부착 → 따라다님
	zone->AttachToActor(avatar, FAttachmentTransformRules::KeepWorldTransform);

	zone->InitSwampZone(GetAbilitySystemComponentFromActorInfo(), zoneRadius, healthPercentPerTick, tickRate, duration);
}

void UC_EliteMonsterSpecialAttackGA_Swamp::OnMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}
