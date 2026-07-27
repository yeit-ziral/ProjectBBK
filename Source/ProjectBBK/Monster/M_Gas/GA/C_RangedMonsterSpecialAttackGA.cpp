// Fill out your copyright notice in the Description page of Project Settings.

#include "C_RangedMonsterSpecialAttackGA.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "../../C_BaseMonster.h"
#include "../../Object/C_SpecialProjectile.h"

UC_RangedMonsterSpecialAttackGA::UC_RangedMonsterSpecialAttackGA()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UC_RangedMonsterSpecialAttackGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!attackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* montageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, attackMontage, 1.0f, NAME_None, false
	);
	if (montageTask)
	{
		montageTask->OnCompleted.AddDynamic(this, &UC_RangedMonsterSpecialAttackGA::OnMontageCompleted);
		montageTask->OnInterrupted.AddDynamic(this, &UC_RangedMonsterSpecialAttackGA::OnMontageCancelled);
		montageTask->OnCancelled.AddDynamic(this, &UC_RangedMonsterSpecialAttackGA::OnMontageCancelled);
		montageTask->ReadyForActivation();
	}

	SpawnSpecialProjectile();
}

void UC_RangedMonsterSpecialAttackGA::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UC_RangedMonsterSpecialAttackGA::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UC_RangedMonsterSpecialAttackGA::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UC_RangedMonsterSpecialAttackGA::SpawnSpecialProjectile()
{
	if (!specialProjectileClass) return;

	AC_BaseMonster* monster = Cast<AC_BaseMonster>(GetAvatarActorFromActorInfo());
	if (!monster) return;

	UAbilitySystemComponent* sourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!sourceASC) return;

	// 스폰 위치 — 소켓 우선, 없으면 backOffset
	FVector spawnLocation = monster->GetActorLocation()
		+ monster->GetActorForwardVector() * backOffset.X
		+ monster->GetActorRightVector()   * backOffset.Y
		+ monster->GetActorUpVector()      * backOffset.Z;

	if (USkeletalMeshComponent* mesh = monster->GetMesh())
	{
		if (mesh->DoesSocketExist(backSocketName))
			spawnLocation = mesh->GetSocketLocation(backSocketName);
	}

	// 타겟 위치 — AI Focus 우선, 없으면 플레이어 0번
	AActor* target = nullptr;
	if (AAIController* ai = Cast<AAIController>(monster->GetController()))
		target = ai->GetFocusActor();
	if (!target)
		target = UGameplayStatics::GetPlayerPawn(monster->GetWorld(), 0);
	if (!target) return;

	FActorSpawnParameters spawnParams;
	spawnParams.Owner      = monster;
	spawnParams.Instigator = monster->GetInstigator();

	AActor* spawnedActor = monster->GetWorld()->SpawnActor<AActor>(
		specialProjectileClass, spawnLocation, FRotator::ZeroRotator, spawnParams);
	if (!spawnedActor) return;

	AC_SpecialProjectile* projectile = Cast<AC_SpecialProjectile>(spawnedActor);
	if (!projectile) return;

	const float attackValue = (float)monster->GetAttack() * damageMultiplier;
	projectile->InitSpecialProjectile(target->GetActorLocation(), specialArcHeight, specialProjectileSpeed);
	projectile->InitGASDamage(sourceASC, damageEffectClass, attackValue);

	// 미사일 발사음 (RM_Robot_Missile_Launch) — 발사 시점
	if (launchSound)
		UGameplayStatics::PlaySoundAtLocation(monster, launchSound, spawnLocation);
}
