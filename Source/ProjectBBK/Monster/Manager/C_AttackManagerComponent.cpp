// Fill out your copyright notice in the Description page of Project Settings.


#include "C_AttackManagerComponent.h"
#include "../C_BaseMonster.h"
#include "../Data/MonsterID.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayTagContainer.h"
#include "../../GAS/Attributes/C_ChracterAttributeSetBase.h"


// Sets default values for this component's properties
UC_AttackManagerComponent::UC_AttackManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	// 정지 판정은 AC_BaseMonster의 공격 시계가 담당하므로 여기선 Tick이 필요 없다
	PrimaryComponentTick.bCanEverTick = false;
}

void UC_AttackManagerComponent::Initialize(AC_BaseMonster* OwnerMonster)
{
	ownerMonster = OwnerMonster;
}


float UC_AttackManagerComponent::GetAttackClock() const
{
	if (ownerMonster) return ownerMonster->GetAttackClock();

	const UWorld* world = GetWorld();
	return world ? world->GetTimeSeconds() : 0.0f;
}

bool UC_AttackManagerComponent::CanAttack() const
{
	const float cooldown = ownerMonster ? ownerMonster->GetAttackCooldown()
		                                : coolDownTime;

	return (GetAttackClock() - lastAttackTime) >= cooldown;
}

void UC_AttackManagerComponent::StartCooldown(float Seconds)
{
	lastAttackTime = GetAttackClock();
}

bool UC_AttackManagerComponent::CanSpecialAttack() const
{
	if (!ownerMonster) return false;
	return (GetAttackClock() - lastSpecialAttackTime) >= ownerMonster->GetSpecialCooldown();
}

void UC_AttackManagerComponent::StartSpecialCooldown()
{
	lastSpecialAttackTime = GetAttackClock();
}



bool UC_AttackManagerComponent::DoNormalAttack()
{
	if (!ownerMonster) return false;

	// 몬스터 ID로 분기하지 않는다 — 실제 공격(몽타주·발사체·GA)은 각 몬스터 클래스가 담당하고
	// 여기서는 쿨타임만 관리하므로 모든 몬스터가 동일하게 동작한다.
	// 예전엔 ID switch였는데, 신규 몬스터를 추가할 때마다 case 누락으로 공격이 아예 안 나갔다
	// (쉴드 몬스터 1004 사례). 쿨타임 수치는 FMonsterData의 NormalCoolDown에서 온다
	if (!CanAttack()) return false;

	const float cooldown = ownerMonster->GetAttackCooldown();
	if (cooldown <= 0.f)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[AttackMgr] MonsterId=%d NormalCooldown이 0 — DataTable 미로드 의심"),
			ownerMonster->GetMonsterID());
	}

	StartCooldown(cooldown);
	return true;
}

bool UC_AttackManagerComponent::DoSpecialAttack()
{
	if (!ownerMonster) return false;

	const int32 id = ownerMonster->GetMonsterID();

	switch (id)
	{
	case MONSTER_ID_TANK:
	{
		if (!CanSpecialAttack()) return false;
		StartSpecialCooldown();
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
	if (!ownerMonster) return;

	const FVector center = ownerMonster->GetActorLocation();

	TArray<TEnumAsByte<EObjectTypeQuery>> types;
	types.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	// 몬스터 자신 및 다른 몬스터는 제외
	TArray<AActor*> ignore;
	ignore.Add(ownerMonster);
	TArray<AActor*> allMonsters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AC_BaseMonster::StaticClass(), allMonsters);
	for (AActor* actor : allMonsters)
	{
		ignore.Add(actor);
	}

	TArray<AActor*> hitActors;
	UKismetSystemLibrary::SphereOverlapActors(
		ownerMonster, center, slamRadius, types, nullptr, ignore, hitActors
	);

	if (debug)
	{
		//DrawDebugSphere(GetWorld(), center, slamRadius, 16, FColor::Orange, false, 2.0f);
	}

	for (AActor* actor : hitActors)
	{
		ACharacter* target = Cast<ACharacter>(actor);
		if (!target) continue;

		// 데미지 적용
		ApplyDirectDamage(target, 10.f);

		// 넉백: 몬스터에서 타겟 방향으로 날려보냄
		FVector knockbackDir = (target->GetActorLocation() - ownerMonster->GetActorLocation()).GetSafeNormal2D();
		knockbackDir.Z = 0.5f; // 약간의 상향 성분
		knockbackDir.Normalize();
		target->LaunchCharacter(knockbackDir * knockbackStrength, true, true);
	}
}

void UC_AttackManagerComponent::ApplyDirectDamage(ACharacter* Target, float DamageValue)
{
	if (!Target) return;

	UAbilitySystemComponent* targetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!targetASC) return;

	const FGameplayAttribute healthAttr = UC_ChracterAttributeSetBase::GethealthAttribute();
	const float currentHealth = targetASC->GetNumericAttribute(healthAttr);
	const float newHealth = FMath::Max(0.f, currentHealth - DamageValue);
	targetASC->SetNumericAttributeBase(healthAttr, newHealth);
}

void UC_AttackManagerComponent::ApplyGEDamage(ACharacter* Target, float DamageValue)
{
	if (!ownerMonster || !Target) return;

	if (!damageEffectClass) return;

	IAbilitySystemInterface* ascInterface = Cast<IAbilitySystemInterface>(Target);
	if (!ascInterface) return;

	UAbilitySystemComponent* targetASC = ascInterface->GetAbilitySystemComponent();
	if (!targetASC) return;

	UAbilitySystemComponent* sourceASC = ownerMonster->GetMonsterASC();
	if (!sourceASC) return;

	FGameplayEffectContextHandle context = sourceASC->MakeEffectContext();
	context.AddInstigator(ownerMonster, ownerMonster);

	FGameplayEffectSpecHandle spec = sourceASC->MakeOutgoingSpec(damageEffectClass, 1.f, context);
	if (!spec.IsValid()) return;

	spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.Damage"), DamageValue);

	sourceASC->ApplyGameplayEffectSpecToTarget(*spec.Data.Get(), targetASC);
}

void UC_AttackManagerComponent::DoMeleeNormalAttack()
{

	if (!ownerMonster)
		return;

	const FVector start = ownerMonster->GetActorLocation();    // 몬스터 위치
	const FVector dir = ownerMonster->GetActorForwardVector(); // 몬스터 앞 방향 벡터
	const float   range = ownerMonster->GetAttackRange();      // 공격 범위 설정
	const FVector end = start + dir * range;                   // 공격 범위 끝 위치
	 
	TArray<TEnumAsByte<EObjectTypeQuery>> types;
	types.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn)); //충돌 대상을 Pawn으로 설정

	TArray<AActor*> ignore;
	ignore.Add(ownerMonster);
	TArray<AActor*> allMonsters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AC_BaseMonster::StaticClass(), allMonsters); 

	for (AActor* actor : allMonsters)
	{
		ignore.Add(actor);
	}
	// 자기자신과 BaseMonster 클래스 같은 공격대상에서 제외

	FHitResult hit; // 히트 결과 저장 변수

	const bool hitOk = UKismetSystemLibrary::SphereTraceSingleForObjects  // 구체 탐지
	(
		ownerMonster, start, end, traceRadius, types, false, ignore,
		/*debug ? EDrawDebugTrace::ForDuration :*/ EDrawDebugTrace::None,
		hit, true
	);

	if (hitOk)
	{
		if (ACharacter* target = Cast<ACharacter>(hit.GetActor()))
		{
			ApplyDirectDamage(target, 5.f);
		}
	}
	

	StartCooldown(ownerMonster->GetAttackCooldown());
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
}

