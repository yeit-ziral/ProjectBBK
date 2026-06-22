// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BossNormalAttackGA.h"

#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "../../Object/C_BossProjectile.h"
#include "../../C_BaseMonster.h"

UC_BossNormalAttackGA::UC_BossNormalAttackGA()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UC_BossNormalAttackGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo,
                                             const FGameplayAbilityActivationInfo ActivationInfo,
                                             const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* boss = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!boss || !attackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (!throwEventTag.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 플레이어 방향으로 Yaw 회전
	AActor* target = nullptr;
	if (AAIController* ai = Cast<AAIController>(boss->GetController()))
		target = ai->GetFocusActor();
	if (!target)
		target = UGameplayStatics::GetPlayerPawn(boss->GetWorld(), 0);

	if (target)
	{
		const FVector toTarget = target->GetActorLocation() - boss->GetActorLocation();
		boss->SetActorRotation(FRotator(0.f, toTarget.Rotation().Yaw, 0.f));
	}

	// 몽타주를 Ability Task로 재생 — 완료/중단/취소 어느 경로로 끝나든 반드시 EndAbility 호출.
	// (throw 이벤트만으로 종료하면 몽타주가 중간에 끊겼을 때 GA가 active 상태로 박혀
	//  이후 모든 노말 어택이 TryActivate=FAILED가 됨)
	UAbilityTask_PlayMontageAndWait* montageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, attackMontage);
	montageTask->OnCompleted.AddDynamic(this, &UC_BossNormalAttackGA::OnMontageEnded);
	montageTask->OnInterrupted.AddDynamic(this, &UC_BossNormalAttackGA::OnMontageEnded);
	montageTask->OnCancelled.AddDynamic(this, &UC_BossNormalAttackGA::OnMontageEnded);
	montageTask->OnBlendOut.AddDynamic(this, &UC_BossNormalAttackGA::OnMontageEnded);
	montageTask->ReadyForActivation();

	// AnimNotify(C_SpawnProjectile_AnimNotify)가 throwEventTag 이벤트를 보내면 투사체 스폰
	UAbilityTask_WaitGameplayEvent* waitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, throwEventTag, nullptr, false, false);
	waitTask->EventReceived.AddDynamic(this, &UC_BossNormalAttackGA::OnThrowEvent);
	waitTask->ReadyForActivation();
}

void UC_BossNormalAttackGA::OnThrowEvent(FGameplayEventData Payload)
{
	// 종료는 몽타주 종료 콜백(OnMontageEnded)이 담당 — 여기서 EndAbility 하면 던지기 애니가 중간에 잘림
	ACharacter* boss = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!boss || !projectileClass)
		return;

	USkeletalMeshComponent* bossMesh = boss->GetMesh();
	if (!bossMesh || !bossMesh->DoesSocketExist(throwSocketName))
		return;

	const FTransform socketTM = bossMesh->GetSocketTransform(throwSocketName);

	// 현재 플레이어 위치 재탐색 (몽타주 재생 중 이동했을 수 있음)
	AActor* target = nullptr;
	if (AAIController* ai = Cast<AAIController>(boss->GetController()))
		target = ai->GetFocusActor();
	if (!target)
		target = UGameplayStatics::GetPlayerPawn(boss->GetWorld(), 0);

	FActorSpawnParameters params;
	params.Owner = boss;
	params.Instigator = boss;

	AC_BossProjectile* proj = GetWorld()->SpawnActor<AC_BossProjectile>(
		projectileClass, socketTM, params);

	if (proj && target)
	{
		// 중력 드롭 보정: 플레이어 위치보다 약간 위를 조준
		const FVector targetPos = target->GetActorLocation() + FVector(0.f, 0.f, 60.f);
		const FVector fireDir = (targetPos - socketTM.GetLocation()).GetSafeNormal();

		AC_BaseMonster* bossMonster = Cast<AC_BaseMonster>(boss);
		const float attackValue = bossMonster ? static_cast<float>(bossMonster->GetAttack()) * damageMultiplier : 0.f;
		proj->InitProjectile(GetAbilitySystemComponentFromActorInfo(), damageGEClass, attackValue, fireDir);
	}
}

void UC_BossNormalAttackGA::OnMontageEnded()
{
	if (!IsActive())
		return;

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}
