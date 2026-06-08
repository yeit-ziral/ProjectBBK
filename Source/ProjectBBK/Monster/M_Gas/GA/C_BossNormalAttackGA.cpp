// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BossNormalAttackGA.h"

#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
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

	boss->PlayAnimMontage(attackMontage);

	// AnimNotify(C_SpawnProjectile_AnimNotify)가 throwEventTag 이벤트를 보내면 투사체 스폰
	UAbilityTask_WaitGameplayEvent* waitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, throwEventTag, nullptr, false, false);
	waitTask->EventReceived.AddDynamic(this, &UC_BossNormalAttackGA::OnThrowEvent);
	waitTask->ReadyForActivation();
}

void UC_BossNormalAttackGA::OnThrowEvent(FGameplayEventData Payload)
{
	ACharacter* boss = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!boss || !projectileClass)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
		return;
	}

	USkeletalMeshComponent* bossMesh = boss->GetMesh();
	if (!bossMesh || !bossMesh->DoesSocketExist(throwSocketName))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
		return;
	}

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

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}
