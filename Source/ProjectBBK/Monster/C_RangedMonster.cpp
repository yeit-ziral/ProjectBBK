// Fill out your copyright notice in the Description page of Project Settings.


#include "C_RangedMonster.h"
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
	//if (MonsterHpWidget)
	//{
	//	MonsterHpWidget->SetCurrentHp(GetmaxHP() * 0.5f);
	//}

}

void AC_RangedMonster::RangedNormalAttack()
{
	if (!attackManager)
		return;

	UE_LOG(LogTemp, Warning, TEXT("RangedMonster 공격 시도"));

	if (attackManager->DoNormalAttack())
	{
		if (!rangedNormalMontage)
			UE_LOG(LogTemp, Warning, TEXT("no normal montage"))
		else
			UE_LOG(LogTemp, Warning, TEXT("yes normal montage"));

		PlayAnimMontage(rangedNormalMontage);

		// 원거리 핵심: 쏘기
		SpawnProjectile();
	}
}

void AC_RangedMonster::RangedSpecialAttack()
{
	UE_LOG(LogTemp, Warning, TEXT("RangedSpecialAttack 호출됨 attackManager = %s"),
		attackManager ? TEXT("VALID") : TEXT("NULL"));

	if (!attackManager)
		return;

	UE_LOG(LogTemp, Warning, TEXT("RangedMonster 특수 공격 시도"));

	if (attackManager->DoSpecialAttack())
	{
		if (!rangedSpecialMontage)
			UE_LOG(LogTemp, Warning, TEXT("no montage"));
		PlayAnimMontage(rangedSpecialMontage);

		// 특수 공격도 일단 발사 1회 (원하면 연사/산탄으로 확장)
		SpawnProjectile();
	}
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

	// 스폰 위치/회전 계산
	FVector spawnLocation = GetActorLocation() + muzzleOffset;
	FRotator spawnRotation = GetActorRotation();

	USkeletalMeshComponent* meshComp = GetMesh();
	if (meshComp)
	{
		// 소켓이 있으면 소켓 기준으로 발사
		if (meshComp->DoesSocketExist(muzzleSocketName))
		{
			const FTransform socketTransform = meshComp->GetSocketTransform(muzzleSocketName);
			spawnLocation = socketTransform.GetLocation();
			spawnRotation = socketTransform.GetRotation().Rotator();
		}
	}

	// 타겟 방향으로 쏘고 싶으면(가장 흔한 방식) "현재 바라보는 방향" 대신 TargetActor를 써도 됨
	// (BaseMonster에 targetActor 같은 게 있으면 거기에 맞춰 바꿔줄게)
	// 여기서는 일단 몬스터 전방 기준으로 발사
	FActorSpawnParameters spawnParams;
	spawnParams.Owner = this;
	spawnParams.Instigator = GetInstigator();

	AActor* projectile = world->SpawnActor<AActor>(projectileClass, spawnLocation, spawnRotation, spawnParams);
	if (!projectile)
	{
		UE_LOG(LogTemp, Warning, TEXT("RangedMonster: projectile spawn failed"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("RangedMonster: projectile spawned"));
}
