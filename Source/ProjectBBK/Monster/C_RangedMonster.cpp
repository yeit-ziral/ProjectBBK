// Fill out your copyright notice in the Description page of Project Settings.


#include "C_RangedMonster.h"
#include "Object/C_SpecialProjectile.h"
#include "Manager/C_AttackManagerComponent.h"
#include "Data/AttackTypes.h"

#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "UI/C_NormalMonsterHPWidget.h"

AC_RangedMonster::AC_RangedMonster()
{

}

void AC_RangedMonster::BeginPlay()
{
	Super::BeginPlay();
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
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("RangedMonster: normalAttackGAClass or monsterASC is NULL"));
		}
	}
}

void AC_RangedMonster::RangedSpecialAttack()
{
	UE_LOG(LogTemp, Warning, TEXT("RangedSpecialAttack ȣ��� attackManager = %s"),
		attackManager ? TEXT("VALID") : TEXT("NULL"));

	if (!attackManager)
		return;

	UE_LOG(LogTemp, Warning, TEXT("RangedMonster Ư�� ���� �õ�"));

	if (attackManager->DoSpecialAttack())
	{
		if (!rangedSpecialMontage)
			UE_LOG(LogTemp, Warning, TEXT("no special montage"));
		PlayAnimMontage(rangedSpecialMontage);

		SpawnSpecialProjectile();
	}
}


void AC_RangedMonster::SpawnSpecialProjectile()
{
	if (!specialProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("RangedMonster: specialProjectileClass is NULL"));
		return;
	}

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
	{
		UE_LOG(LogTemp, Warning, TEXT("RangedMonster: no player pawn found for special attack"));
		return;
	}
	const FVector targetPos = playerPawn->GetActorLocation();

	// 스폰
	FActorSpawnParameters spawnParams;
	spawnParams.Owner = this;
	spawnParams.Instigator = GetInstigator();

	AActor* spawnedActor = world->SpawnActor<AActor>(specialProjectileClass, spawnLocation, FRotator::ZeroRotator, spawnParams);
	if (!spawnedActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("RangedMonster: special projectile spawn failed"));
		return;
	}

	AC_SpecialProjectile* specialProjectile = Cast<AC_SpecialProjectile>(spawnedActor);
	if (!specialProjectile)
	{
		UE_LOG(LogTemp, Warning, TEXT("RangedMonster: specialProjectileClass is not AC_SpecialProjectile"));
		return;
	}

	specialProjectile->InitSpecialProjectile(targetPos, specialArcHeight, specialProjectileSpeed);
}

void AC_RangedMonster::SpawnProjectile()
{
	if (!projectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("RangedMonster: projectileClass is NULL"));
		return;
	}

	UWorld* world = GetWorld();
	if (!world)
		return;

	// ���� ��ġ/ȸ�� ���
	FVector spawnLocation = GetActorLocation() + muzzleOffset;
	FRotator spawnRotation = GetActorRotation();

	USkeletalMeshComponent* meshComp = GetMesh();
	if (meshComp)
	{
		// ������ ������ ���� �������� �߻�
		if (meshComp->DoesSocketExist(muzzleSocketName))
		{
			const FTransform socketTransform = meshComp->GetSocketTransform(muzzleSocketName);
			spawnLocation = socketTransform.GetLocation();
			spawnRotation = socketTransform.GetRotation().Rotator();
		}
	}

	// Ÿ�� �������� ��� ������(���� ���� ���) "���� �ٶ󺸴� ����" ��� TargetActor�� �ᵵ ��
	// (BaseMonster�� targetActor ���� �� ������ �ű⿡ ���� �ٲ��ٰ�)
	// ���⼭�� �ϴ� ���� ���� �������� �߻�
	FActorSpawnParameters spawnParams;
	spawnParams.Owner = this;
	spawnParams.Instigator = GetInstigator();

	AActor* projectile = world->SpawnActor<AActor>(projectileClass, spawnLocation, spawnRotation, spawnParams);
	if (!projectile)
	{
		UE_LOG(LogTemp, Warning, TEXT("RangedMonster: projectile spawn failed"));
		return;
	}

}
