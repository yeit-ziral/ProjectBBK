// Fill out your copyright notice in the Description page of Project Settings.


#include "C_BombMonster.h"
#include "M_Gas/C_MonsterASC.h"
#include "M_Gas/C_MonsterAttributeSet.h"
#include "../PlayerCharacter/C_BasePlayerCharactor.h"
#include "../GAS/Attributes/C_ChracterAttributeSetBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Materials/MaterialInterface.h"

AC_BombMonster::AC_BombMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	// 비행 몬스터 — 지면 이동 대신 Flying 모드 사용
	UCharacterMovementComponent* move = GetCharacterMovement();
	move->DefaultLandMovementMode    = MOVE_Flying;
	move->bOrientRotationToMovement  = true;
	move->RotationRate               = FRotator(0.f, 720.f, 0.f);
	move->BrakingDecelerationFlying  = 2048.f;
	bUseControllerRotationYaw        = false;

	// 캡슐 충돌로도 접촉 기폭이 가능하도록 Hit 이벤트 활성화
	GetCapsuleComponent()->SetNotifyRigidBodyCollision(true);
}

void AC_BombMonster::BeginPlay()
{
	Super::BeginPlay();

	monsterTypeTag = FGameplayTag::RequestGameplayTag(TEXT("Monster.Type.Normal"));
	ApplyMonsterTypeTag();

	// BP에서 미설정 시 프로젝트 공용 데미지 태그로 폴백
	if (!explosionDamageTag.IsValid())
		explosionDamageTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"), false);

	// DataTable 스탯이 적용된 뒤(Super::BeginPlay) 비행 속도 동기화
	UCharacterMovementComponent* move = GetCharacterMovement();
	move->SetMovementMode(MOVE_Flying);
	move->MaxAcceleration = flyAcceleration;
	if (monsterAttributeSet)
		move->MaxFlySpeed = monsterAttributeSet->GetmoveSpeed();

	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AC_BombMonster::OnCapsuleHit);

	ApplyEmitterMaterials(bodyEmitterMaterials);

	// 배회 기준점 = 스폰 위치
	homeLocation = GetActorLocation();
	PickPatrolPoint();

	// 여러 마리가 동시에 같은 궤적을 그리지 않도록 위상을 개체마다 흩뿌림
	weavePhase     = FMath::FRandRange(0.f, 2.f * PI);
	patrolBobPhase = FMath::FRandRange(0.f, 2.f * PI);
}

void AC_BombMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 기폭(bFuseStarted) 중에도 추격을 계속한다 — 플레이어는 3초 안에 폭발 반경 밖으로 도망쳐야 함
	if (bExploded || IsDead()) return;

	searchTimer -= DeltaTime;
	if (searchTimer <= 0.f)
	{
		UpdateTarget();
		searchTimer = targetSearchInterval;
	}

	if (targetActor.IsValid())
	{
		// GE_Slowed 등 moveSpeed 변경을 비행 속도에도 반영
		// (UC_MonsterAttributeSet::PostAttributeChange는 MaxWalkSpeed만 갱신함)
		if (monsterAttributeSet)
			GetCharacterMovement()->MaxFlySpeed = monsterAttributeSet->GetmoveSpeed();

		TickCharge(DeltaTime);

		// 접촉 판정 — 캡슐 Hit 이벤트가 누락되는 각도/속도를 거리 체크로 보완
		if (FVector::Dist(GetActorLocation(), targetActor->GetActorLocation()) <= contactDistance)
			StartFuse();
	}
	else
	{
		GetCharacterMovement()->MaxFlySpeed = patrolSpeed;
		TickPatrol(DeltaTime);
	}
}

void AC_BombMonster::ApplyEmitterMaterials(const TArray<UMaterialInterface*>& Materials)
{
	if (Materials.Num() == 0) return;

	// BP에서 추가한 파티클 컴포넌트라 C++에서 이름으로 참조할 수 없으므로 클래스로 수집
	TArray<UParticleSystemComponent*> particleComps;
	GetComponents<UParticleSystemComponent>(particleComps);

	for (UParticleSystemComponent* comp : particleComps)
	{
		if (!comp) continue;
		for (int32 i = 0; i < Materials.Num(); ++i)
		{
			if (Materials[i])
				comp->SetMaterial(i, Materials[i]);
		}
	}
}

void AC_BombMonster::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	// UC_GroggyComponent::ExitGroggyState / AC_BaseMonster::TakeHitReaction이
	// MOVE_Walking을 하드코딩하므로, 비행 상태로 되돌린다.
	// (SetMovementMode가 이 함수를 재귀 호출하지만 Flying에서는 조건이 false라 1단계에서 멈춤)
	if (bExploded || IsDead()) return;

	if (GetCharacterMovement()->MovementMode == MOVE_Walking)
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
}

void AC_BombMonster::UpdateTarget()
{
	TArray<AActor*> players;
	UGameplayStatics::GetAllActorsOfClass(this, AC_BasePlayerCharactor::StaticClass(), players);

	AActor* closest  = nullptr;
	float   bestDist = FLT_MAX;

	for (AActor* player : players)
	{
		// 캐릭터 로스터 구조상 비활성 캐릭터도 월드에 존재 — 조종 중인 캐릭터만 대상
		APawn* pawn = Cast<APawn>(player);
		if (!pawn || !pawn->IsPlayerControlled()) continue;

		const float dist = FVector::Dist(player->GetActorLocation(), GetActorLocation());
		if (dist > detectionRange) continue;
		if (dist < bestDist)
		{
			bestDist = dist;
			closest  = player;
		}
	}

	if (closest)
	{
		targetActor = closest;
		return;
	}

	// 인식 범위 밖 — 이미 락온한 대상은 계속 추격 (자폭 몬스터는 한 번 인식하면 끝까지 쫓음)
	APawn* current = Cast<APawn>(targetActor.Get());
	if (!current || !current->IsPlayerControlled())
		targetActor = nullptr;
}

void AC_BombMonster::TickCharge(float DeltaTime)
{
	const FVector aimPoint = targetActor->GetActorLocation() + FVector(0.f, 0.f, hoverHeightOffset);
	const FVector toTarget = aimPoint - GetActorLocation();

	const FVector forward = toTarget.GetSafeNormal();
	if (forward.IsNearlyZero()) return;

	weavePhase += DeltaTime * weaveFrequency * 2.f * PI;

	// 타겟에 가까워질수록 흔들림을 줄여 마지막엔 직선으로 파고들게 함
	// (끝까지 흔들면 플레이어 주위를 맴돌기만 하고 접촉 판정에 걸리지 않음)
	const float falloff = (weaveFalloffDistance > KINDA_SMALL_NUMBER)
		? FMath::Clamp(toTarget.Size() / weaveFalloffDistance, 0.f, 1.f)
		: 1.f;

	// 진행 방향 기준 횡방향(좌우)이 큰 S자를 만들고,
	// 상하는 더 빠른 비정수배 주기로 흔들어 그 위에 잔진동을 얹는다
	const FVector right   = FVector::CrossProduct(FVector::UpVector, forward).GetSafeNormal();
	const FVector lateral = right * (FMath::Sin(weavePhase) * weaveAmplitude * falloff);
	const FVector vertical = FVector::UpVector
		* (FMath::Sin(weavePhase * weaveVerticalFrequencyRatio) * weaveVerticalAmplitude * falloff);

	const FVector dir = (forward + lateral + vertical).GetSafeNormal();
	if (dir.IsNearlyZero()) return;

	AddMovementInput(dir, 1.f);
}

void AC_BombMonster::TickPatrol(float DeltaTime)
{
	patrolRepickTimer -= DeltaTime;
	patrolBobPhase    += DeltaTime * patrolBobFrequency * 2.f * PI;

	const FVector myLocation = GetActorLocation();

	// 목표점 도달 또는 타임아웃(벽에 낀 경우) 시 새 지점 선정
	if (patrolRepickTimer <= 0.f ||
	    FVector::Dist2D(myLocation, patrolTargetPoint) <= patrolPointTolerance)
	{
		PickPatrolPoint();
	}

	// 제자리 부유감 — 목표점 위에 상하 오프셋을 얹어 둥실거리게 함
	const FVector bobbedPoint = patrolTargetPoint
		+ FVector(0.f, 0.f, FMath::Sin(patrolBobPhase) * patrolBobAmplitude);

	const FVector dir = (bobbedPoint - myLocation).GetSafeNormal();
	if (dir.IsNearlyZero()) return;

	AddMovementInput(dir, 1.f);
}

void AC_BombMonster::PickPatrolPoint()
{
	const FVector2D offset2D = FMath::RandPointInCircle(patrolRadius);

	patrolTargetPoint = homeLocation
		+ FVector(offset2D.X, offset2D.Y, FMath::FRandRange(-patrolVerticalRange, patrolVerticalRange));

	patrolRepickTimer = patrolRepickInterval;
}

void AC_BombMonster::OnCapsuleHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	APawn* pawn = Cast<AC_BasePlayerCharactor>(OtherActor);
	if (!pawn || !pawn->IsPlayerControlled()) return;

	StartFuse();
}

void AC_BombMonster::StartFuse()
{
	if (bFuseStarted || bExploded || IsDead()) return;
	bFuseStarted  = true;
	fuseStartTime = GetWorld()->GetTimeSeconds();

	// 정지시키지 않는다 — 기폭 후에도 계속 추격하므로 플레이어는 도망쳐야 함

	if (fuseSound)
		UGameplayStatics::PlaySoundAtLocation(this, fuseSound, GetActorLocation());

	if (fuseTime <= 0.f)
	{
		Explode();
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		fuseTimerHandle, this, &AC_BombMonster::Explode, fuseTime, false);

	// 즉시 빨간색으로 전환 후 점멸 시작
	TickFuseBlink();
}

void AC_BombMonster::TickFuseBlink()
{
	if (bExploded || fuseBlinkMaterials.Num() == 0) return;

	bBlinkOn = !bBlinkOn;
	ApplyEmitterMaterials(bBlinkOn ? fuseBlinkMaterials : bodyEmitterMaterials);

	ScheduleNextFuseBlink();
}

void AC_BombMonster::ScheduleNextFuseBlink()
{
	if (bExploded) return;

	// 폭발이 가까워질수록 점멸이 빨라지게 보간
	const float elapsed  = GetWorld()->GetTimeSeconds() - fuseStartTime;
	const float progress = FMath::Clamp(elapsed / FMath::Max(fuseTime, KINDA_SMALL_NUMBER), 0.f, 1.f);
	const float interval = FMath::Lerp(fuseBlinkIntervalStart, fuseBlinkIntervalEnd, progress);

	GetWorld()->GetTimerManager().SetTimer(
		fuseBlinkTimerHandle, this, &AC_BombMonster::TickFuseBlink,
		FMath::Max(interval, 0.01f), false);
}

void AC_BombMonster::Explode()
{
	if (bExploded) return;
	bExploded = true;

	GetWorld()->GetTimerManager().ClearTimer(fuseTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(fuseBlinkTimerHandle);

	GetCharacterMovement()->StopMovementImmediately();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (explosionVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), explosionVFX, GetActorLocation(), GetActorRotation(),
			FVector(1.f), true, true);
	}

	if (explosionSound)
		UGameplayStatics::PlaySoundAtLocation(this, explosionSound, GetActorLocation());

	// 폭발 순간 본체(불덩이 파티클 포함) 숨김 — VFX/사운드는 별도 액터라 영향 없음
	SetActorHiddenInGame(true);

	ApplyExplosionDamage();

	// 자신 사망 처리 — AC_BaseMonster::ExecuteDeathSequence로 이어짐 (ExpOrb 드랍·소멸 포함)
	if (monsterASC)
		monsterASC->HandleDeath();
}

void AC_BombMonster::ApplyExplosionDamage()
{
	if (!explosionDamageGEClass || !monsterASC) return;

	TArray<TEnumAsByte<EObjectTypeQuery>> objectTypes;
	objectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	TArray<AActor*> ignore;
	ignore.Add(this);

	TArray<AActor*> hitActors;
	UKismetSystemLibrary::SphereOverlapActors(
		this, GetActorLocation(), explosionRadius, objectTypes,
		AC_BasePlayerCharactor::StaticClass(), ignore, hitActors);

	// 캐릭터 로스터는 PlayerState의 ASC를 공유 — 같은 ASC에 중복 적용되지 않게 방지
	TSet<UAbilitySystemComponent*> damagedASCs;

	for (AActor* hitActor : hitActors)
	{
		APawn* pawn = Cast<APawn>(hitActor);
		if (!pawn || !pawn->IsPlayerControlled()) continue;

		IAbilitySystemInterface* ascInterface = Cast<IAbilitySystemInterface>(hitActor);
		if (!ascInterface) continue;

		UAbilitySystemComponent* targetASC = ascInterface->GetAbilitySystemComponent();
		if (!targetASC || damagedASCs.Contains(targetASC)) continue;
		damagedASCs.Add(targetASC);

		// 대상 최대 체력 비율 고정 데미지 (몬스터 Attack 스탯 미사용)
		const float maxHealth = targetASC->GetNumericAttribute(
			UC_ChracterAttributeSetBase::GetmaxHealthAttribute());
		if (maxHealth <= 0.f) continue;

		const float damage = maxHealth * explosionHealthPercent;

		FGameplayEffectContextHandle context = monsterASC->MakeEffectContext();
		context.AddInstigator(this, this);

		FGameplayEffectSpecHandle spec = monsterASC->MakeOutgoingSpec(explosionDamageGEClass, 1.f, context);
		if (!spec.IsValid()) continue;

		if (explosionDamageTag.IsValid())
			spec.Data->SetSetByCallerMagnitude(explosionDamageTag, damage);

		monsterASC->ApplyGameplayEffectSpecToTarget(*spec.Data.Get(), targetASC);
	}
}

void AC_BombMonster::ExecuteDeathSequence()
{
	GetWorld()->GetTimerManager().ClearTimer(fuseTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(fuseBlinkTimerHandle);

	// 피격 사망 시에도 폭발시키는 옵션
	// (Explode 내부의 HandleDeath는 State.Dead 태그 가드로 재진입하지 않음)
	if (bExplodeOnDeath && !bExploded)
		Explode();

	Super::ExecuteDeathSequence();
}

bool AC_BombMonster::IsDead() const
{
	return monsterASC && monsterASC->HasMatchingGameplayTag(
		FGameplayTag::RequestGameplayTag(FName("State.Dead")));
}
