// Fill out your copyright notice in the Description page of Project Settings.


#include "C_RangedMonster.h"
#include "Object/C_SpecialProjectile.h"
#include "Manager/C_AttackManagerComponent.h"
#include "M_Gas/C_MonsterASC.h"
#include "Data/AttackTypes.h"

#include "GameFramework/Actor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "UI/C_NormalMonsterHPWidget.h"

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
		{
			monsterASC->TryActivateAbilityByClass(normalAttackGAClass);
		}
	}
}

void AC_RangedMonster::RangedSpecialAttack()
{
	if (!attackManager)
		return;

	if (attackManager->DoSpecialAttack())
	{
		PlayAnimMontage(rangedSpecialMontage);
		SpawnSpecialProjectile();
	}
}


void AC_RangedMonster::SpawnSpecialProjectile()
{
	if (!specialProjectileClass)
		return;

	UWorld* world = GetWorld();
	if (!world) return;

	// 등 뒤 스폰 위치 결정
	FVector spawnLocation = GetActorLocation() + GetActorForwardVector() * backOffset.X
	                                           + GetActorRightVector()   * backOffset.Y
	                                           + GetActorUpVector()      * backOffset.Z;

	USkeletalMeshComponent* meshComp = GetMesh();
	if (meshComp && meshComp->DoesSocketExist(backSocketName))
	{
		spawnLocation = meshComp->GetSocketLocation(backSocketName);
	}

	// 타겟(플레이어) 위치 획득
	APawn* playerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!playerPawn)
		return;
	const FVector targetPos = playerPawn->GetActorLocation();

	// 스폰
	FActorSpawnParameters spawnParams;
	spawnParams.Owner = this;
	spawnParams.Instigator = GetInstigator();

	AActor* spawnedActor = world->SpawnActor<AActor>(specialProjectileClass, spawnLocation, FRotator::ZeroRotator, spawnParams);
	if (!spawnedActor)
		return;

	AC_SpecialProjectile* specialProjectile = Cast<AC_SpecialProjectile>(spawnedActor);
	if (!specialProjectile)
		return;

	specialProjectile->InitSpecialProjectile(targetPos, specialArcHeight, specialProjectileSpeed);
	specialProjectile->InitGASDamage(GetMonsterASC(), specialDamageEffectClass, static_cast<float>(GetAttack()) * 0.2f);
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
