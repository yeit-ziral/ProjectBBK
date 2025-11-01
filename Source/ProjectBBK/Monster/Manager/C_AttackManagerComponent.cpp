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



bool UC_AttackManagerComponent::DoNormalAttack()
{
	if (!ownerMonster || !CanAttack())
		return false;

	const int32 id = ownerMonster ? ownerMonster->GetMonsterID() : 0;

	switch (id)
	{
	case MONSTER_ID_BEAR:
	{
		DoBearNormalAttack();
		return true;
	}


	default:
		UE_LOG(LogTemp, Warning, TEXT("No NormalAttack implementation for MonsterId=%d"), id);
		StartCooldown(ownerMonster ? ownerMonster->GetAttackCooldown() : coolDownTime);
		return false;
	}
}

bool UC_AttackManagerComponent::DoSpecialAttack()
{
	if (!ownerMonster || !CanAttack()) 
		return false;

	const int32 id = ownerMonster ? ownerMonster->GetMonsterID() : 0;

	switch (id)
	{
	case MONSTER_ID_BEAR:
	{
		DoBearSpecialAttackJump();
		return true;
	}

	default:
		UE_LOG(LogTemp, Warning, TEXT("No NormalAttack implementation for MonsterId=%d"), id);
		StartCooldown(ownerMonster ? ownerMonster->GetAttackCooldown() : coolDownTime);
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

	const FVector start = ownerMonster->GetActorLocation();    // 몬스터위치
	const FVector dir = ownerMonster->GetActorForwardVector(); // 몬스터 정면 방향
	const float   range = ownerMonster->GetAttackRange();      // 몬스터 공격 범위
	const FVector end = start + dir * range;                   // 몬스터 공격 끝 위치
	 
	TArray<TEnumAsByte<EObjectTypeQuery>> types;
	types.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn)); //공격 대상을 pawn으로 설정

	TArray<AActor*> ignore;
	ignore.Add(ownerMonster);
	TArray<AActor*> allMonsters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AC_BaseMonster::StaticClass(), allMonsters); 

	for (AActor* actor : allMonsters)
	{
		ignore.Add(actor);
	}
	// 자기자신과 BaseMonster클래스 전부 공격대상에서 제외

	FHitResult hit; //공격 대상 정보 저장

	const bool hitOk = UKismetSystemLibrary::SphereTraceSingleForObjects  // 단일공격
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
				ownerMonster->GetAttack(),                    // 나중에 계수 추가
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

	const FVector monster = ownerMonster->GetActorLocation(); // 공격하는 몬스터의 위치

	TArray<TEnumAsByte<EObjectTypeQuery>> types;
	types.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn)); //공격 대상을 pawn으로 설정

	TArray<AActor*> ignore;
	ignore.Add(ownerMonster);
	TArray<AActor*> allMonsters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AC_BaseMonster::StaticClass(), allMonsters);

	for (AActor* actor : allMonsters)
	{
		ignore.Add(actor);
	}
	// 자기자신과 BaseMonster클래스 전부 공격대상에서 제외

	TArray<FHitResult> hits;

	const bool hitsOk = UKismetSystemLibrary::SphereTraceMultiForObjects  // 광역공격
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
				ownerMonster->GetAttack(),         // 나중에 계수 추가    
				ownerMonster->GetController(),
				ownerMonster,
				nullptr
			);

			const FVector dir = (target->GetActorLocation() - monster).GetSafeNormal2D();     // 플레이어 넉백 거리 조절
			const FVector impulse = dir * knockbackStrength + FVector(0, 0, knockupStrength);

			target->LaunchCharacter(impulse, true, true);
	
		}
	}

	StartCooldown(ownerMonster->GetAttackCooldown()); // 나중에 스페셜 쿨타임으로 변경
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

