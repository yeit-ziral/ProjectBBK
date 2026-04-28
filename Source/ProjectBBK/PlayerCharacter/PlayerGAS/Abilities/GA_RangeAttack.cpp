// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_RangeAttack.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "ProjectBBK/PlayerCharacter/C_PlayerRangedProjectile.h"

UGA_RangeAttack::UGA_RangeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	abilityInputID   = ProjectBBKAbilityID::Attack;
	abilityID        = ProjectBBKAbilityID::Attack;
}

void UGA_RangeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	waitEvent->EventReceived.AddDynamic(this, &UGA_RangeAttack::OnFireProjectile);
	waitEvent->ReadyForActivation();
}

void UGA_RangeAttack::OnFireProjectile(FGameplayEventData Payload)
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
		UE_LOG(LogTemp, Warning, TEXT("[GA_RangeAttack] Socket '%s' not found"), *ProjectileSocketName.ToString());
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
		return;
	}

	// ── 조준점 계산: 카메라 전방 LineTrace ────────────────────────────
	// 3인칭 카메라에서 마우스 커서가 없으므로 카메라 전방 벡터를 사용
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

	FVector aimPoint = traceEnd; // 히트 없으면 최대 거리 지점
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
		// GAS 정보 주입
		projectile->Initialize(
			GetAbilitySystemComponentFromActorInfo(),
			GetAvatarActorFromActorInfo(),
			DamageEffectClass,
			BaseDamage);

		// 발사 방향: 소켓 → 조준점
		const FVector fireDirection =
			(aimPoint - socketTransform.GetLocation()).GetSafeNormal();
		projectile->InitVelocity(fireDirection);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}
