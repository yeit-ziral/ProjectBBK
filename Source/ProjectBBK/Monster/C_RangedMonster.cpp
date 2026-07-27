// Fill out your copyright notice in the Description page of Project Settings.


#include "C_RangedMonster.h"
#include "Manager/C_AttackManagerComponent.h"
#include "M_Gas/C_MonsterASC.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AC_RangedMonster::AC_RangedMonster()
{
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

bool AC_RangedMonster::IsPlayingAttackAnimation() const
{
	UAnimInstance* anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!anim) return false;
	return (rangedNormalMontage  && anim->Montage_IsPlaying(rangedNormalMontage))
		|| (rangedSpecialMontage && anim->Montage_IsPlaying(rangedSpecialMontage));
}

bool AC_RangedMonster::CanAutoAttack() const
{
	if (!attackManager) return false;
	return attackManager->CanAttack() || attackManager->CanSpecialAttack();
}

void AC_RangedMonster::BeginPlay()
{
	Super::BeginPlay();

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	monsterTypeTag = FGameplayTag::RequestGameplayTag(TEXT("Monster.Type.Normal"));

	// 공격 GA를 ASC에 등록 — TryActivateAbilityByClass는 GiveAbility된 어빌리티만 발동 가능
	if (monsterASC)
	{
		if (normalAttackGAClass && !monsterASC->FindAbilitySpecFromClass(normalAttackGAClass))
			monsterASC->GiveAbility(FGameplayAbilitySpec(normalAttackGAClass, 1, 0));
		if (specialAttackGAClass && !monsterASC->FindAbilitySpecFromClass(specialAttackGAClass))
			monsterASC->GiveAbility(FGameplayAbilitySpec(specialAttackGAClass, 1, 0));
	}
}

void AC_RangedMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 이동 발소리 (RM_Ranged_Walk) — 지면 이동 중, 공격 애님이 아닐 때 일정 간격 재생
	if (!walkSound) return;

	UCharacterMovementComponent* move = GetCharacterMovement();
	const bool bMoving = move && move->IsMovingOnGround()
		&& GetVelocity().Size2D() > walkSpeedThreshold
		&& !IsPlayingAttackAnimation();

	if (!bMoving)
	{
		// 멈추면 다음에 움직이기 시작할 때 즉시 첫 발소리가 나도록 리셋
		footstepTimer = 0.f;
		return;
	}

	footstepTimer -= DeltaTime;
	if (footstepTimer <= 0.f)
	{
		UGameplayStatics::PlaySoundAtLocation(this, walkSound, GetActorLocation());
		footstepTimer = footstepInterval;
	}
}

void AC_RangedMonster::RangedAutoAttack()
{
	if (!attackManager) return;

	// 스페셜 쿨타임이 끝났으면 스페셜, 아니면 노말
	if (attackManager->CanSpecialAttack())
	{
		RangedSpecialAttack();
	}
	else
	{
		RangedNormalAttack();
	}
}

void AC_RangedMonster::RangedNormalAttack()
{
	if (!attackManager) return;

	if (attackManager->DoNormalAttack())
	{
		// GA를 직접 활성화 → GA가 몽타주 재생 → AnimNotify → GameplayEvent → 발사체 스폰
		if (normalAttackGAClass && monsterASC)
			monsterASC->TryActivateAbilityByClass(normalAttackGAClass);
	}
}

void AC_RangedMonster::RangedSpecialAttack()
{
	if (!attackManager || !monsterASC || !specialAttackGAClass)
		return;
	if (!attackManager->DoSpecialAttack()) return;

	monsterASC->TryActivateAbilityByClass(specialAttackGAClass);
}

void AC_RangedMonster::SpawnProjectile()
{
	if (!projectileClass)
		return;

	UWorld* world = GetWorld();
	if (!world)
		return;

	// 스폰 위치/회전 결정
	FVector spawnLocation = GetActorLocation() + muzzleOffset;
	FRotator spawnRotation = GetActorRotation();

	USkeletalMeshComponent* meshComp = GetMesh();
	if (meshComp)
	{
		// 총구 소켓이 있는 경우 소켓에서 발사
		if (meshComp->DoesSocketExist(muzzleSocketName))
		{
			const FTransform socketTransform = meshComp->GetSocketTransform(muzzleSocketName);
			spawnLocation = socketTransform.GetLocation();
			spawnRotation = socketTransform.GetRotation().Rotator();
		}
	}

	// 타겟 방향 투사체 — 추후 TargetActor 도입 예정
	// (BaseMonster의 targetActor 구현 미루었기 때문에 임시로 전방 발사)
	// 여기서는 일단 항상 앞쪽 방향으로 발사
	FActorSpawnParameters spawnParams;
	spawnParams.Owner = this;
	spawnParams.Instigator = GetInstigator();

	AActor* projectile = world->SpawnActor<AActor>(projectileClass, spawnLocation, spawnRotation, spawnParams);
	if (!projectile)
		return;

}
