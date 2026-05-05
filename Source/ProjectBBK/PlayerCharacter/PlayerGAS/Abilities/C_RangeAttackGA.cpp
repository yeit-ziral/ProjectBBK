// Fill out your copyright notice in the Description page of Project Settings.


#include "C_RangeAttackGA.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "ProjectBBK/PlayerCharacter/C_PlayerRangedProjectile.h"

UC_RangeAttackGA::UC_RangeAttackGA()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UC_RangeAttackGA::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
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

	character->PlayAnimMontage(AttackMontage);

	// AnimNotify → Send Gameplay Event "Event.Montage.FireProjectile" 을 기다림
	UAbilityTask_WaitGameplayEvent* waitEvent =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			FGameplayTag::RequestGameplayTag("Event.Montage.FireProjectile"),
			nullptr, false, false);

	waitEvent->EventReceived.AddDynamic(this, &UC_RangeAttackGA::OnFireProjectile);
	waitEvent->ReadyForActivation();
}

void UC_RangeAttackGA::OnFireProjectile(FGameplayEventData Payload)
{
	ACharacter* character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!character || !ProjectileClass)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
		return;
	}

	USkeletalMeshComponent* mesh = character->GetMesh();
	if (!mesh || !mesh->DoesSocketExist(ProjectileSocketName))
	{
		UE_LOG(LogTemp, Warning, TEXT("[C_RangeAttackGA] Socket '%s' not found"), *ProjectileSocketName.ToString());
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
		return;
	}

	// ── 조준점 계산: 카메라 전방 LineTrace ────────────────────────────
	FVector cameraLocation  = FVector::ZeroVector;
	FVector cameraForward   = character->GetActorForwardVector();

	if (AController* controller = character->GetController())
	{
		FRotator controlRot;
		controller->GetPlayerViewPoint(cameraLocation, controlRot);
		cameraForward = controlRot.Vector();
	}

	const FVector traceEnd = cameraLocation + cameraForward * AimTraceDistance;

	FHitResult hitResult;
	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(character);

	FVector aimPoint = traceEnd;
	if (GetWorld()->LineTraceSingleByChannel(hitResult, cameraLocation, traceEnd,
	                                          ECC_Visibility, queryParams))
	{
		aimPoint = hitResult.ImpactPoint;
	}

	// ── 투사체 스폰 ────────────────────────────────────────────────────
	const FTransform socketTransform = mesh->GetSocketTransform(ProjectileSocketName);

	FActorSpawnParameters spawnParams;
	spawnParams.Owner      = character;
	spawnParams.Instigator = character;

	AC_PlayerRangedProjectile* projectile =
		GetWorld()->SpawnActor<AC_PlayerRangedProjectile>(
			ProjectileClass, socketTransform, spawnParams);

	if (projectile)
	{
		projectile->Initialize(
			GetAbilitySystemComponentFromActorInfo(),
			GetAvatarActorFromActorInfo(),
			DamageEffectClass,
			BaseDamage);

		const FVector fireDirection =
			(aimPoint - socketTransform.GetLocation()).GetSafeNormal();
		projectile->InitVelocity(fireDirection);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}
