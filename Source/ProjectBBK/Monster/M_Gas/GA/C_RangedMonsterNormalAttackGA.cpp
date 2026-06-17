// Fill out your copyright notice in the Description page of Project Settings.


#include "C_RangedMonsterNormalAttackGA.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "../../Object/C_RangedProjectile.h"
#include "../../C_BaseMonster.h"


UC_RangedMonsterNormalAttackGA::UC_RangedMonsterNormalAttackGA()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UC_RangedMonsterNormalAttackGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	ACharacter* character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!character || !AttackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	AActor* targetActor = nullptr;

	if (AAIController* aiController = Cast<AAIController>(character->GetController()))
	{
		targetActor = aiController->GetFocusActor(); 
	}

	if (!targetActor)
	{
		targetActor = UGameplayStatics::GetPlayerPawn(character->GetWorld(), 0);
	}

	if (targetActor)
	{
		const FVector toTarget = (targetActor->GetActorLocation() - character->GetActorLocation());
		const FRotator lookRot = toTarget.Rotation();
		const FRotator newRot(0.f, lookRot.Yaw, 0.f);
		character->SetActorRotation(newRot);
	}

	character->PlayAnimMontage(AttackMontage);

	// ������ ����Ǹ���
	// ���߿�:
	// - AnimNotify���� Projectile Spawn
	// - �Ǵ� WaitMontageNotify / WaitMontageEnd �߰�

	UAbilityTask_WaitGameplayEvent* waitEvent =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag("Event.Montage.FireProjectile"),
			                                              nullptr, false, false);

	waitEvent->EventReceived.AddDynamic(this, &UC_RangedMonsterNormalAttackGA::OnFireProjectileEvent);

	waitEvent->ReadyForActivation();

}

void UC_RangedMonsterNormalAttackGA::OnFireProjectileEvent(FGameplayEventData Payload)
{
	ACharacter* character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!character || !ProjectileClass)
		return;

	USkeletalMeshComponent* mesh = character->GetMesh();
	if (!mesh)
		return;

	const FName socketName(TEXT("WeaponSocket"));
	if (!mesh->DoesSocketExist(socketName))
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponSocket not found"));
		return;
	}

	const FTransform spawnTM = mesh->GetSocketTransform(socketName);

	FActorSpawnParameters params;
	params.Owner = character;
	params.Instigator = character;

	AC_RangedProjectile* proj = GetWorld()->SpawnActor<AC_RangedProjectile>(ProjectileClass, spawnTM, params);

	AActor* targetActor = nullptr;

	if (AAIController* aiController = Cast<AAIController>(character->GetController()))
	{
		targetActor = aiController->GetFocusActor();
	}

	if (!targetActor)
	{
		targetActor = UGameplayStatics::GetPlayerPawn(character->GetWorld(), 0);
	}
	if (proj && targetActor)
	{
		const FVector dir = (targetActor->GetActorLocation() - proj->GetActorLocation()).GetSafeNormal();
		AC_BaseMonster* monster = Cast<AC_BaseMonster>(character);
		const float attackValue = monster ? static_cast<float>(monster->GetAttack()) * damageMultiplier : 0.f;
		proj->InitProjectile(GetAbilitySystemComponentFromActorInfo(), damageEffectClass, attackValue, dir);
	}


	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}
