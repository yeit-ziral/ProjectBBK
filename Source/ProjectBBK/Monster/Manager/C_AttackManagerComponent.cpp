// Fill out your copyright notice in the Description page of Project Settings.


#include "C_AttackManagerComponent.h"
#include "../C_BaseMonster.h"
#include "../Data/MonsterID.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"


// Sets default values for this component's properties
UC_AttackManagerComponent::UC_AttackManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void UC_AttackManagerComponent::Initialize(AC_BaseMonster* OwnerMonster)
{
	ownerMonster = OwnerMonster;
}


bool UC_AttackManagerComponent::CanAttack() const
{
	const float nowTime = GetWorld()->GetTimeSeconds();

	const float cooldown = ownerMonster ? ownerMonster->GetAttackCooldown()
		                                : coolDownTime;

	return (nowTime - lastAttackTime) >= cooldown;
}

void UC_AttackManagerComponent::StartCooldown(float Seconds)
{
	lastAttackTime = GetWorld()->GetTimeSeconds();
}

bool UC_AttackManagerComponent::CanSpecialAttack() const
{
	if (!ownerMonster) return false;
	const float now = GetWorld()->GetTimeSeconds();
	return (now - lastSpecialAttackTime) >= ownerMonster->GetSpecialCooldown();
}

void UC_AttackManagerComponent::StartSpecialCooldown()
{
	lastSpecialAttackTime = GetWorld()->GetTimeSeconds();
}



bool UC_AttackManagerComponent::DoNormalAttack()
{
	if (!ownerMonster) return false;

	const int32 id = ownerMonster->GetMonsterID();

	switch (id)
	{
	case MONSTER_ID_BEAR:
	{
		if (!CanAttack()) return false;
		DoBearNormalAttack();
		return true;
	}
	case MONSTER_ID_ROCKET:
	{
		// 실제 공격(몽타주+발사체)은 C_RangedMonster에서 처리 — 여기선 쿨타임만 관리
		if (!CanAttack()) return false;
		StartCooldown(ownerMonster->GetAttackCooldown());
		return true;
	}
	default:
		UE_LOG(LogTemp, Warning, TEXT("No NormalAttack implementation for MonsterId=%d"), id);
		return false;
	}
}

bool UC_AttackManagerComponent::DoSpecialAttack()
{
	if (!ownerMonster) return false;

	const int32 id = ownerMonster->GetMonsterID();

	switch (id)
	{
	case MONSTER_ID_BEAR:
	{
		if (!CanAttack()) return false;
		DoBearSpecialAttackJump();
		return true;
	}
	case MONSTER_ID_ROCKET:
	{
		// 실제 공격(몽타주+발사체)은 C_RangedMonster에서 처리 — 여기선 스페셜 쿨타임만 관리
		if (!CanSpecialAttack()) return false;
		StartSpecialCooldown();
		return true;
	}
	default:
		UE_LOG(LogTemp, Warning, TEXT("No SpecialAttack implementation for MonsterId=%d"), id);
		return false;
	}
}

void UC_AttackManagerComponent::DoSlam()
{
	DoBearSpecialAttackSlam();
}

void UC_AttackManagerComponent::DoBearNormalAttack()
{

	if (!ownerMonster)
		return;

	const FVector start = ownerMonster->GetActorLocation();    // ������ġ
	const FVector dir = ownerMonster->GetActorForwardVector(); // ���� ���� ����
	const float   range = ownerMonster->GetAttackRange();      // ���� ���� ����
	const FVector end = start + dir * range;                   // ���� ���� �� ��ġ
	 
	TArray<TEnumAsByte<EObjectTypeQuery>> types;
	types.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn)); //���� ����� pawn���� ����

	TArray<AActor*> ignore;
	ignore.Add(ownerMonster);
	TArray<AActor*> allMonsters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AC_BaseMonster::StaticClass(), allMonsters); 

	for (AActor* actor : allMonsters)
	{
		ignore.Add(actor);
	}
	// �ڱ��ڽŰ� BaseMonsterŬ���� ���� ���ݴ�󿡼� ����

	FHitResult hit; //���� ��� ���� ����

	const bool hitOk = UKismetSystemLibrary::SphereTraceSingleForObjects  // ���ϰ���
	(
		ownerMonster, start, end, traceRadius, types, false, ignore,
		debug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		hit, true
	);

	if (hitOk)
	{
		if (ACharacter* target = Cast<ACharacter>(hit.GetActor()))
		{

			UGameplayStatics::ApplyDamage
			(
				target,
				ownerMonster->GetAttack(),                    // ���߿� ��� �߰�
				ownerMonster->GetController(),
				ownerMonster,
				nullptr
			);
			UE_LOG(LogTemp, Warning, TEXT("Bear Normal hit %s (DMG=%d)"), *target->GetName(), ownerMonster->GetAttack());
		}
	}
	

	StartCooldown(ownerMonster->GetAttackCooldown());
}

void UC_AttackManagerComponent::DoBearSpecialAttackJump()
{
	if (ACharacter* bear = Cast<ACharacter>(ownerMonster))
	{
		
		FVector velocity = bear->GetVelocity();
		velocity.Z = jumpPower;
		bear->LaunchCharacter(velocity, true, true);
	}

}

void UC_AttackManagerComponent::DoBearSpecialAttackSlam()
{
	if (!ownerMonster)
		return;

	const FVector monster = ownerMonster->GetActorLocation(); // �����ϴ� ������ ��ġ

	TArray<TEnumAsByte<EObjectTypeQuery>> types;
	types.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn)); //���� ����� pawn���� ����

	TArray<AActor*> ignore;
	ignore.Add(ownerMonster);
	TArray<AActor*> allMonsters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AC_BaseMonster::StaticClass(), allMonsters);

	for (AActor* actor : allMonsters)
	{
		ignore.Add(actor);
	}
	// �ڱ��ڽŰ� BaseMonsterŬ���� ���� ���ݴ�󿡼� ����

	TArray<FHitResult> hits;

	const bool hitsOk = UKismetSystemLibrary::SphereTraceMultiForObjects  // ��������
	(
		ownerMonster, monster, monster + FVector(0, 0, 1), slamRadius,
		types, false, ignore, debug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		hits, true
	);

	if (hitsOk)
	{
		for (const FHitResult& hit : hits)
		{
			ACharacter* target = Cast<ACharacter>(hit.GetActor());
			if (!target) 
				continue;

			UGameplayStatics::ApplyDamage(
				target,
				ownerMonster->GetAttack(),         // ���߿� ��� �߰�    
				ownerMonster->GetController(),
				ownerMonster,
				nullptr
			);

			const FVector dir = (target->GetActorLocation() - monster).GetSafeNormal2D();     // �÷��̾� �˹� �Ÿ� ����
			const FVector impulse = dir * knockbackStrength + FVector(0, 0, knockupStrength);

			target->LaunchCharacter(impulse, true, true);
	
		}
	}

	StartCooldown(ownerMonster->GetAttackCooldown()); // ���߿� ����� ��Ÿ������ ����
}


// Called when the game starts
void UC_AttackManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UC_AttackManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

