// Fill out your copyright notice in the Description page of Project Settings.


#include "C_ShieldMonster.h"
#include "Manager/C_AttackManagerComponent.h"
#include "Manager/C_GroggyComponent.h"
#include "M_Gas/C_MonsterASC.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Anim/ANS_ShieldParryWindow.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"

AC_ShieldMonster::AC_ShieldMonster()
{
	// 가드 방향 판정이 액터 정면 기준이므로 AI 포커스(컨트롤러 회전)를 따라가야 한다
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// 텔레그래프 링 — Decal이 아닌 평면 메시. 기본은 카메라 정면 빌보드라 지형·시점과 무관하게 정원으로 보인다
	parryRingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ParryRingMesh"));
	parryRingMesh->SetupAttachment(RootComponent);
	parryRingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	parryRingMesh->SetCollisionProfileName(TEXT("NoCollision"));
	parryRingMesh->SetCastShadow(false);
	parryRingMesh->bReceivesDecals = false;
	parryRingMesh->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> planeMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (planeMesh.Succeeded())
		parryRingMesh->SetStaticMesh(planeMesh.Object);
}

void AC_ShieldMonster::BeginPlay()
{
	Super::BeginPlay();

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	monsterTypeTag = FGameplayTag::RequestGameplayTag(TEXT("Monster.Type.Normal"));

	// 공격 GA를 ASC에 등록 — TryActivateAbilityByClass는 GiveAbility된 어빌리티만 발동 가능
	if (monsterASC && normalAttackGAClass && !monsterASC->FindAbilitySpecFromClass(normalAttackGAClass))
		monsterASC->GiveAbility(FGameplayAbilitySpec(normalAttackGAClass, 1, 0));

	// 스탯 로드(DataComponent) 이후이므로 moveSpeed 기준 가드 속도를 바로 적용할 수 있음
	bGuardActive = IsGuarding();
	ApplyGuardMovementSpeed();
	OnGuardStateChanged(bGuardActive);

	if (!strikeEventTag.IsValid())
		strikeEventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Monster.Melee.Hit"));

	if (parryRingMesh)
	{
		// Plane 기본 크기가 100x100이므로 지름(2R) 기준으로 스케일을 잡는다
		const float ringScale = FMath::Max(parryRingRadius, 1.f) * 2.f / 100.f;
		parryRingMesh->SetRelativeScale3D(FVector(ringScale, ringScale, 1.f));

		// 빌보드는 몬스터 yaw와 무관하게 카메라만 따라야 하므로 부모 회전을 상속받지 않는다
		parryRingMesh->SetUsingAbsoluteRotation(bParryRingFaceCamera);
		UpdateTelegraphRingTransform();

		if (parryRingMaterial)
		{
			parryRingMID = UMaterialInstanceDynamic::Create(parryRingMaterial, this);
			parryRingMesh->SetMaterial(0, parryRingMID);
		}
	}

	SetTelegraphRingVisible(false);
}

void AC_ShieldMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (guardBreakAccum > 0.f && guardBreakDecayPerSecond > 0.f)
		guardBreakAccum = FMath::Max(0.f, guardBreakAccum - guardBreakDecayPerSecond * DeltaTime);

	UpdateGuardState();

	if (bTelegraphActive)
	{
		// 카메라가 계속 움직이므로 링이 보이는 동안 매 프레임 정면을 다시 맞춘다
		UpdateTelegraphRingTransform();

		// 링 수렴의 **유일한** 시간 소스. 윈드업 구간은 감속 → 원속으로 배속이 바뀌어
		// 애니 시간과 실시간이 비례하지 않으므로 실시간으로 진행시킨다
		AdvanceAttackTelegraph(DeltaTime);

		// 공격 몽타주가 통째로 사라져 NotifyEnd가 끝내 들어오지 않는 경로 대비.
		// 캐스트 단계에는 아직 공격 몽타주가 없으므로 Strike 단계에서만 검사한다
		if (attackPhase == EAttackPhase::Strike && !GetActiveAttackMontage())
			CancelAttackTelegraph();
	}

	// 그로기·사망으로 공격이 끊긴 경로 — 링과 시퀀스 타이머가 남지 않도록 정리한다
	if (monsterASC
		&& (monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")))
		 || monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Groggy")))))
	{
		if (bTelegraphActive)
			CancelAttackTelegraph();

		AbortAttackSequence();
	}

	if (bDrawGuardDebug)
		DrawGuardDebug();
}

#pragma region Attack
bool AC_ShieldMonster::CanAutoAttack() const
{
	if (!attackManager) return false;
	return attackManager->CanAttack();
}

bool AC_ShieldMonster::IsPlayingAttackAnimation() const
{
	// 시퀀스 전체(방패 내리기 ~ 방패 들기)를 공격 중으로 본다 —
	// BT의 재공격 차단과 가드 해제가 시퀀스 전체에 걸려야 순서가 맞는다
	if (attackPhase != EAttackPhase::None) return true;

	if (!normalAttackMontage) return false;

	UAnimInstance* anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!anim) return false;

	return anim->Montage_IsPlaying(normalAttackMontage);
}

bool AC_ShieldMonster::ShieldAutoAttack()
{
	return ShieldNormalAttack();
}

bool AC_ShieldMonster::ShieldNormalAttack()
{
	if (!attackManager || !monsterASC || !normalAttackGAClass) return false;

	// 시퀀스가 끝나기 전에 다시 들어오면 몽타주가 겹치고 링이 리셋된다
	if (attackPhase != EAttackPhase::None) return false;

	if (!attackManager->DoNormalAttack()) return false;

	// 블록 정지 중이었더라도 공격은 그대로 나간다 — 잠금을 남겨두면 잠금 타이머가
	// 시퀀스 도중에 풀리면서 이동 모드가 뒤늦게 복구된다.
	// 쿨타임 게이트(CanAutoAttack)는 시간 기준이라 잠금과 무관하게 이미 통과한 상태
	CancelBlockMovementLock();

	StartAttackSequence();
	return true;
}

UAnimMontage* AC_ShieldMonster::GetActiveAttackMontage() const
{
	UAnimInstance* anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!anim) return nullptr;

	// normalAttackMontage 미지정(GA가 다른 몽타주를 쓰는 경우)이면 현재 활성 몽타주를 대상으로 한다
	if (normalAttackMontage && anim->Montage_IsPlaying(normalAttackMontage))
		return normalAttackMontage;

	return anim->GetCurrentActiveMontage();
}

void AC_ShieldMonster::ApplyAttackMontagePlayRate()
{
	if (FMath::IsNearlyEqual(attackMontagePlayRate, 1.f)) return;

	UAnimInstance* anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	UAnimMontage* montage = GetActiveAttackMontage();
	if (!anim || !montage) return;

	anim->Montage_SetPlayRate(montage, attackMontagePlayRate);
}

float AC_ShieldMonster::GetAttackStrikeTime() const
{
	if (!normalAttackMontage) return 0.f;

	// ANS_ShieldParryWindow의 **종료 시각**이 곧 타격 시점이다(노티파이 배치 규약).
	// 몽타주에서 직접 읽으므로 노티파이를 옮겨도 링 타이밍이 자동으로 따라온다
	for (const FAnimNotifyEvent& notifyEvent : normalAttackMontage->Notifies)
	{
		if (notifyEvent.NotifyStateClass && notifyEvent.NotifyStateClass->IsA<UANS_ShieldParryWindow>())
			return notifyEvent.GetEndTriggerTime();
	}

	// 노티파이가 없으면 몽타주 끝을 타격 시점으로 본다
	return normalAttackMontage->GetPlayLength();
}

void AC_ShieldMonster::LockMovementForAttackSequence()
{
	if (!bLockMovementDuringAttackSequence) return;

	if (UCharacterMovementComponent* movement = GetCharacterMovement())
	{
		movement->StopMovementImmediately();
		movement->DisableMovement();
	}

	// BT의 MoveTo/Reposition이 시전 자세로 미끄러지게 하는 것도 함께 끊는다
	if (AAIController* aiController = Cast<AAIController>(GetController()))
		aiController->StopMovement();
}

void AC_ShieldMonster::ReleaseAttackSequenceMovement()
{
	if (!bLockMovementDuringAttackSequence) return;

	// 블록 리액션 잠금이 걸려 있으면 복구는 그쪽 타이머가 담당한다
	if (bBlockMovementLocked) return;

	// 그로기·사망 중이면 각 상태의 종료 로직이 이동을 복구한다 — 여기서 되살리면 안 됨
	if (monsterASC
		&& (monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")))
		 || monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Groggy")))))
		return;

	if (UCharacterMovementComponent* movement = GetCharacterMovement())
		movement->SetMovementMode(MOVE_Walking);
}

void AC_ShieldMonster::StartAttackSequence()
{
	UWorld* world = GetWorld();
	if (!world) return;

	attackPhase = EAttackPhase::Lower;
	LockMovementForAttackSequence();

	// 이 시점부터 IsPlayingAttackAnimation()이 true → 가드가 내려간다.
	// AnimBP의 additive 가드 포즈가 블렌드 아웃되는 동안 방패 내리는 몽타주가 재생된다
	const float lowerLength = shieldLowerMontage ? PlayAnimMontage(shieldLowerMontage) : 0.f;

	if (lowerLength > 0.f)
		world->GetTimerManager().SetTimer(attackPhaseTimerHandle, this, &AC_ShieldMonster::BeginCastPhase, lowerLength, false);
	else
		BeginCastPhase();
}

void AC_ShieldMonster::BeginCastPhase()
{
	UWorld* world = GetWorld();
	if (!world) return;

	attackPhase = EAttackPhase::Cast;

	const float castLength = castMontage ? PlayAnimMontage(castMontage) : 0.f;

	// 시전(준비) 자세 진입 순간 — 공격이 나가기 전에 들려야 하는 기합
	if (castSound)
		UGameplayStatics::PlaySoundAtLocation(this, castSound, GetActorLocation());

	// 링은 캐스트 시작부터 공격 몽타주의 타격 순간까지 줄어든다.
	// 캐스트는 원속 재생이므로 길이가 곧 실시간, 공격 구간만 배속으로 나눈다
	const float rate = FMath::Max(attackMontagePlayRate, 0.01f);
	const float convergeSeconds = castLength + GetAttackStrikeTime() / rate;

	BeginAttackTelegraph(convergeSeconds);

	if (bDrawParryDebug)
	{
		UE_LOG(LogTemp, Log, TEXT("[ShieldParry] cast=%.3f  strike=%.3f  converge=%.3f"),
			castLength, GetAttackStrikeTime() / rate, convergeSeconds);
	}

	if (castLength > 0.f)
		world->GetTimerManager().SetTimer(attackPhaseTimerHandle, this, &AC_ShieldMonster::BeginStrikePhase, castLength, false);
	else
		BeginStrikePhase();
}

void AC_ShieldMonster::BeginStrikePhase()
{
	UWorld* world = GetWorld();
	if (!world) return;

	attackPhase = EAttackPhase::Strike;

	if (!monsterASC || !normalAttackGAClass || !monsterASC->TryActivateAbilityByClass(normalAttackGAClass))
	{
		// GA 발동 실패 — 링을 지우고 방패를 다시 든다(시퀀스가 멈춰 굳는 것 방지)
		CancelAttackTelegraph();
		BeginRaisePhase();
		return;
	}

	// 공격이 실제로 나가는 순간 — GA 발동 실패 경로에서는 재생되지 않는다
	if (attackSound)
		UGameplayStatics::PlaySoundAtLocation(this, attackSound, GetActorLocation());

	// GA(BP)의 PlayMontageAndWait은 ActivateAbility 안에서 동기 실행되므로 몽타주는 이미 재생 중
	ApplyAttackMontagePlayRate();

	// GA가 어떤 몽타주를 틀었는지에 맞춰 길이를 잡는다(normalAttackMontage와 다를 수 있음)
	const float rate = FMath::Max(attackMontagePlayRate, 0.01f);
	float attackLength = 0.f;
	if (UAnimMontage* montage = GetActiveAttackMontage())
		attackLength = montage->GetPlayLength() / rate;

	if (attackLength > 0.f)
		world->GetTimerManager().SetTimer(attackPhaseTimerHandle, this, &AC_ShieldMonster::BeginRaisePhase, attackLength, false);
	else
		BeginRaisePhase();
}

void AC_ShieldMonster::BeginRaisePhase()
{
	UWorld* world = GetWorld();
	if (!world) return;

	attackPhase = EAttackPhase::Raise;

	const float raiseLength = shieldRaiseMontage ? PlayAnimMontage(shieldRaiseMontage) : 0.f;

	if (raiseLength > 0.f)
		world->GetTimerManager().SetTimer(attackPhaseTimerHandle, this, &AC_ShieldMonster::EndAttackSequence, raiseLength, false);
	else
		EndAttackSequence();
}

void AC_ShieldMonster::EndAttackSequence()
{
	if (UWorld* world = GetWorld())
		world->GetTimerManager().ClearTimer(attackPhaseTimerHandle);

	// 여기서 None이 되어야 IsPlayingAttackAnimation()이 false → 가드(방패 든 자세)가 복귀한다
	attackPhase = EAttackPhase::None;
	ReleaseAttackSequenceMovement();
}

void AC_ShieldMonster::AbortAttackSequence()
{
	if (attackPhase == EAttackPhase::None) return;

	CancelAttackTelegraph();
	EndAttackSequence();
}

void AC_ShieldMonster::CancelBlockMovementLock()
{
	if (!bBlockMovementLocked) return;

	if (UWorld* world = GetWorld())
		world->GetTimerManager().ClearTimer(blockMoveLockTimerHandle);

	// 잠금 해제 시각도 여기서 기록된다 → 공격 직후 blockMoveLockCooldown 동안은 재잠금 안 됨
	ReleaseBlockMovementLock();
}
#pragma endregion

#pragma region Guard
bool AC_ShieldMonster::HasEngagedTarget() const
{
	AAIController* ai = Cast<AAIController>(GetController());
	if (!ai) return false;

	UBlackboardComponent* bb = ai->GetBlackboardComponent();
	if (!bb) return false;

	const AActor* target = Cast<AActor>(bb->GetValueAsObject(guardTargetKeyName));
	if (!IsValid(target)) return false;

	if (guardEngageRange > 0.f
		&& FVector::DistSquared(target->GetActorLocation(), GetActorLocation()) > FMath::Square(guardEngageRange))
		return false;

	return true;
}

bool AC_ShieldMonster::IsGuarding() const
{
	if (!bGuardEnabled || bGuardBroken) return false;

	if (monsterASC
		&& (monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")))
		 || monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Groggy")))))
		return false;

	// 노말 공격 중에는 방패를 내린다 — 이 구간이 플레이어가 딜을 넣을 틈이다
	if (!bGuardWhileAttacking && IsPlayingAttackAnimation()) return false;

	// 배회 중에는 방패를 내리고 있다가 플레이어를 인지하면 든다
	if (bGuardRequiresTarget && !HasEngagedTarget()) return false;

	return true;
}

bool AC_ShieldMonster::IsBlockedDirection(const FVector& AttackerLocation) const
{
	const FVector toAttacker = (AttackerLocation - GetActorLocation()).GetSafeNormal2D();
	if (toAttacker.IsNearlyZero()) return true;   // 완전히 겹친 위치 — 정면으로 간주

	const FVector forward = GetActorForwardVector().GetSafeNormal2D();
	if (forward.IsNearlyZero()) return false;

	const float angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(forward, toAttacker), -1.f, 1.f)));
	return angle <= guardHalfAngle;
}

float AC_ShieldMonster::ModifyIncomingDamage(float RawDamage, AActor* DamageInstigator, bool bTrueDamage)
{
	if (RawDamage <= 0.f) return RawDamage;
	if (bTrueDamage && !bGuardBlocksTrueDamage) return RawDamage;
	if (!IsGuarding()) return RawDamage;

	// EffectContext에 Instigator가 없으면 방향을 알 수 없음 → 방어 실패로 처리
	if (!IsValid(DamageInstigator)) return RawDamage;
	if (!IsBlockedDirection(DamageInstigator->GetActorLocation())) return RawDamage;

	const float finalDamage = RawDamage * (1.f - FMath::Clamp(guardDamageReduction, 0.f, 1.f));
	const float blocked     = RawDamage - finalDamage;

	AccumulateGuardBreak(blocked);
	PlayBlockReaction();
	PlayBlockHitFeedback(DamageInstigator);
	OnGuardBlocked(DamageInstigator, blocked, finalDamage);

	return finalDamage;
}

void AC_ShieldMonster::PlayBlockReaction()
{
	UWorld* world = GetWorld();
	if (!world) return;

	// 연타 시 몽타주가 매 히트마다 처음부터 다시 시작하지 않도록 최소 간격을 둔다
	const float now = world->GetTimeSeconds();
	if (now - lastBlockReactionTime < blockReactionInterval) return;

	lastBlockReactionTime = now;

	// blockReactionMontage는 선택 사항 — 막기 전용 애니가 없으면 비워두고
	// AnimBP의 Mesh Space Additive 가드 포즈(IsGuarding)로 대신한다.
	// 그 경우에도 아래 이동 잠금은 그대로 동작해야 하므로 여기서 return하지 않는다
	const float montageLength = blockReactionMontage ? PlayAnimMontage(blockReactionMontage) : 0.f;

	// 가드 포즈 자체는 IsGuarding()이 상시 담당한다(방패는 이미 올라가 있음).
	// 이건 피격 순간에만 켜지는 별도 플래그 — 히트 플래시 등 추가 연출용, 기본값 0이라 미사용
	if (blockPoseDuration > 0.f)
	{
		bBlockPoseActive = true;
		world->GetTimerManager().SetTimer(blockPoseTimerHandle, this, &AC_ShieldMonster::EndBlockPose, blockPoseDuration, false);
	}

	// 막는 동안은 제자리에서 버틴다 — 데미지가 0이라 히트 리액션이 없는 대신
	// "멈칫"으로 피격을 읽히게 한다. Reposition 스트레이프로 빠져나가는 것도 같이 막힘.
	// 이미 잠겨 있으면 연장하지 않고, 풀린 뒤에도 blockMoveLockCooldown 동안은 재잠금 안 함 —
	// 안 그러면 연타에 타이머가 계속 갱신돼 영구 정지가 된다
	const float lockDuration = blockMoveLockDuration > 0.f ? blockMoveLockDuration : montageLength;
	if (lockDuration > 0.f
		&& !bBlockMovementLocked
		&& now - lastBlockMoveLockEndTime >= blockMoveLockCooldown)
	{
		LockMovementForBlock(lockDuration);
	}
}

void AC_ShieldMonster::PlayBlockHitFeedback(AActor* Attacker)
{
	USkeletalMeshComponent* mesh = GetMesh();
	UWorld* world = GetWorld();

	// 이펙트와 독립 — 파티클이 없어도 막는 소리는 나가야 한다
	if (blockHitSound)
		UGameplayStatics::PlaySoundAtLocation(this, blockHitSound, GetActorLocation());

	if (bLogBlockHitEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BlockHitFX] 진입 — effect=%s mesh=%s world=%s socket=%s(exists=%d)"),
			*GetNameSafe(blockHitEffect),
			*GetNameSafe(mesh),
			world ? TEXT("valid") : TEXT("NULL"),
			*blockHitSocketName.ToString(),
			(mesh && mesh->DoesSocketExist(blockHitSocketName)) ? 1 : 0);
	}

	if (!blockHitEffect || !mesh || !world) return;

	// 0이면 매 블록마다 스폰. 값이 있으면 연타 중에도 그 간격으로만 터진다
	if (blockHitEffectInterval > 0.f)
	{
		const float now = world->GetTimeSeconds();
		if (now - lastBlockHitEffectTime < blockHitEffectInterval)
		{
			if (bLogBlockHitEffect)
				UE_LOG(LogTemp, Warning, TEXT("[BlockHitFX] 스로틀에 걸려 스킵 (interval=%.2f)"), blockHitEffectInterval);
			return;
		}
		lastBlockHitEffectTime = now;
	}

	// 소켓에 부착 — 막는 도중 몬스터가 돌거나 방패가 움직여도 이펙트가 방패를 따라간다.
	// bAutoDestroy=true라 원샷 이펙트는 재생이 끝나면 알아서 정리된다
	UParticleSystemComponent* effect = UGameplayStatics::SpawnEmitterAttached(
		blockHitEffect,
		mesh,
		blockHitSocketName,
		blockHitEffectOffset,
		blockHitEffectRotation,
		FVector(FMath::Max(blockHitEffectScale, 0.01f)),
		EAttachLocation::KeepRelativeOffset,
		true);

	if (!effect)
	{
		if (bLogBlockHitEffect)
			UE_LOG(LogTemp, Error, TEXT("[BlockHitFX] SpawnEmitterAttached가 NULL 반환"));
		return;
	}

	if (bBlockHitEffectFaceAttacker && IsValid(Attacker))
	{
		const FVector dir = (Attacker->GetActorLocation() - effect->GetComponentLocation()).GetSafeNormal();
		if (!dir.IsNearlyZero())
			effect->SetWorldRotation(dir.Rotation());   // 부착은 유지되고 회전만 덮어써진다
	}

	if (bLogBlockHitEffect)
	{
		const FVector spawnLoc = effect->GetComponentLocation();

		UE_LOG(LogTemp, Warning, TEXT("[BlockHitFX] 스폰 성공 — comp=%s loc=%s scale=%s active=%d visible=%d hidden=%d"),
			*effect->GetName(),
			*spawnLoc.ToCompactString(),
			*effect->GetComponentScale().ToCompactString(),
			effect->IsActive() ? 1 : 0,
			effect->IsVisible() ? 1 : 0,
			effect->bHiddenInGame ? 1 : 0);

		// 이펙트가 안 보여도 이 구체는 남는다 — 스폰 좌표가 방패 위인지 눈으로 확인용
		DrawDebugSphere(world, spawnLoc, 20.f, 12, FColor::Cyan, false, 3.f, 0, 1.5f);
	}
}

void AC_ShieldMonster::EndBlockPose()
{
	bBlockPoseActive = false;
}

void AC_ShieldMonster::LockMovementForBlock(float Duration)
{
	UWorld* world = GetWorld();
	if (!world) return;

	if (UCharacterMovementComponent* movement = GetCharacterMovement())
	{
		movement->StopMovementImmediately();
		movement->DisableMovement();
	}

	// BT의 MoveTo는 AddMovementInput을 거치지 않으므로 경로 추종도 함께 끊는다
	if (AAIController* aiController = Cast<AAIController>(GetController()))
		aiController->StopMovement();

	bBlockMovementLocked = true;

	// 재잠금 가드는 호출부(PlayBlockReaction)에 있다 — 여기 도달했으면 항상 새 잠금이다
	world->GetTimerManager().SetTimer(blockMoveLockTimerHandle, this, &AC_ShieldMonster::ReleaseBlockMovementLock, Duration, false);
}

void AC_ShieldMonster::ReleaseBlockMovementLock()
{
	bBlockMovementLocked = false;

	if (UWorld* world = GetWorld())
		lastBlockMoveLockEndTime = world->GetTimeSeconds();

	// 그로기·사망 중이면 각 상태의 종료 로직(ExitGroggyState 등)이 이동을 복구한다 — 여기서 되살리면 안 됨
	if (monsterASC
		&& (monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")))
		 || monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Groggy")))))
		return;

	if (UCharacterMovementComponent* movement = GetCharacterMovement())
		movement->SetMovementMode(MOVE_Walking);
}

void AC_ShieldMonster::AccumulateGuardBreak(float BlockedAmount)
{
	if (guardBreakThreshold <= 0.f) return;   // 가드 브레이크 비활성

	guardBreakAccum += BlockedAmount;
	if (guardBreakAccum >= guardBreakThreshold)
		BreakGuard();
}

void AC_ShieldMonster::BreakGuard()
{
	if (bGuardBroken) return;

	bGuardBroken    = true;
	guardBreakAccum = 0.f;

	OnGuardBroken();
	UpdateGuardState();

	if (UWorld* world = GetWorld())
	{
		world->GetTimerManager().ClearTimer(guardBreakTimerHandle);
		if (guardBreakDuration > 0.f)
			world->GetTimerManager().SetTimer(guardBreakTimerHandle, this, &AC_ShieldMonster::RecoverGuard, guardBreakDuration, false);
		else
			RecoverGuard();
	}
}

void AC_ShieldMonster::RecoverGuard()
{
	if (!bGuardBroken) return;

	bGuardBroken    = false;
	guardBreakAccum = 0.f;

	if (UWorld* world = GetWorld())
		world->GetTimerManager().ClearTimer(guardBreakTimerHandle);

	OnGuardRecovered();
	UpdateGuardState();
}

void AC_ShieldMonster::UpdateGuardState()
{
	const bool bNowGuarding = IsGuarding();
	if (bNowGuarding == bGuardActive) return;

	bGuardActive = bNowGuarding;
	ApplyGuardMovementSpeed();
	OnGuardStateChanged(bGuardActive);
}

void AC_ShieldMonster::ApplyGuardMovementSpeed()
{
	if (FMath::IsNearlyEqual(guardMoveSpeedScale, 1.f)) return;   // 이동 속도 미관여

	UCharacterMovementComponent* movement = GetCharacterMovement();
	if (!movement || !monsterAttributeSet) return;

	const float baseSpeed = monsterAttributeSet->GetmoveSpeed();
	if (baseSpeed <= 0.f) return;

	movement->MaxWalkSpeed = bGuardActive ? baseSpeed * FMath::Max(guardMoveSpeedScale, 0.f) : baseSpeed;
}
#pragma endregion

#pragma region Parry
void AC_ShieldMonster::BeginAttackTelegraph(float ConvergeSeconds)
{
	// 정상 경로에서는 캐스트 단계가 이미 시작해 둔 상태 —
	// 공격 몽타주의 NotifyBegin이 여기 다시 들어와도 링을 리셋하면 안 된다.
	// castMontage가 미지정이라 텔레그래프가 아직 없을 때만 NotifyBegin이 폴백으로 시작시킨다
	if (bTelegraphActive) return;

	telegraphDuration  = FMath::Max(ConvergeSeconds, 0.01f);
	telegraphElapsed   = 0.f;
	bTelegraphActive   = true;

	UpdateTelegraphRingTransform();
	SetTelegraphRingVisible(true);
	PushTelegraphProgress();
	OnTelegraphStateChanged(true);
}

void AC_ShieldMonster::AdvanceAttackTelegraph(float DeltaSeconds)
{
	if (!bTelegraphActive) return;

	telegraphElapsed = FMath::Min(telegraphElapsed + DeltaSeconds, telegraphDuration);
	PushTelegraphProgress();
}

float AC_ShieldMonster::GetTelegraphProgress() const
{
	if (!bTelegraphActive || telegraphDuration <= 0.f) return 0.f;
	return FMath::Clamp(1.f - telegraphElapsed / telegraphDuration, 0.f, 1.f);
}

void AC_ShieldMonster::ResolveStrike()
{
	const bool bWasActive = bTelegraphActive;
	CancelAttackTelegraph();

	if (!bWasActive) return;

	// 몽타주가 그로기·사망으로 중단되면 NotifyEnd도 함께 호출된다 — 이때는 타격이 나가면 안 됨
	if (monsterASC
		&& (monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")))
		 || monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Groggy")))))
		return;

	AActor* parrier = FindParryingPlayer();

	if (bDrawParryDebug)
	{
		const FVector origin  = GetActorLocation();
		const FVector forward = GetActorForwardVector().GetSafeNormal2D();
		const float   range   = GetAttackRange() + parryRangeBonus;
		const FColor  color   = parrier ? FColor::Green : FColor::Red;

		DrawDebugLine(GetWorld(), origin, origin + forward.RotateAngleAxis(+parryHalfAngle, FVector::UpVector) * range, color, false, 3.f, 0, 2.f);
		DrawDebugLine(GetWorld(), origin, origin + forward.RotateAngleAxis(-parryHalfAngle, FVector::UpVector) * range, color, false, 3.f, 0, 2.f);
		DrawDebugCircle(GetWorld(), origin, range, 32, color, false, 3.f, 0, 2.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	}

	if (parrier)
	{
		// 데미지 이벤트를 아예 보내지 않는다 — GA가 이 이벤트를 받아야만 데미지를 적용하므로 공격이 무효화됨
		if (UC_GroggyComponent* groggy = GetGroggyComponent())
			groggy->ForceGroggy(parryGroggyDuration);

		OnParrySuccess(parrier);
		return;
	}

	if (strikeEventTag.IsValid())
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, strikeEventTag, FGameplayEventData());
	}

	OnStrikeLanded();
}

void AC_ShieldMonster::CancelAttackTelegraph()
{
	if (!bTelegraphActive) return;

	bTelegraphActive = false;
	telegraphElapsed = telegraphDuration;

	SetTelegraphRingVisible(false);
	OnTelegraphStateChanged(false);
}

AActor* AC_ShieldMonster::FindParryingPlayer() const
{
	UWorld* world = GetWorld();
	if (!world) return nullptr;

	const FVector origin  = GetActorLocation();
	const FVector forward = GetActorForwardVector().GetSafeNormal2D();
	if (forward.IsNearlyZero()) return nullptr;

	const float range        = GetAttackRange() + parryRangeBonus;
	const float rangeSquared = range * range;

	static const FGameplayTag shieldTag = FGameplayTag::RequestGameplayTag(FName("State.Shield"));

	for (FConstPlayerControllerIterator it = world->GetPlayerControllerIterator(); it; ++it)
	{
		APlayerController* pc = it->Get();
		if (!pc) continue;

		APawn* pawn = pc->GetPawn();
		if (!IsValid(pawn)) continue;

		const FVector toPawn = pawn->GetActorLocation() - origin;
		if (toPawn.SizeSquared2D() > rangeSquared) continue;

		const FVector dir = toPawn.GetSafeNormal2D();
		if (!dir.IsNearlyZero())
		{
			const float angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(forward, dir), -1.f, 1.f)));
			if (angle > parryHalfAngle) continue;
		}

		// ASC는 PlayerState 소유 — IAbilitySystemInterface 경유로 받아야 캐릭터 교체와 무관하게 유효하다
		UAbilitySystemComponent* asc = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(pawn);
		if (!asc) continue;

		if (asc->HasMatchingGameplayTag(shieldTag))
			return pawn;
	}

	return nullptr;
}

void AC_ShieldMonster::SetTelegraphRingVisible(bool bVisible)
{
	if (parryRingMesh)
		parryRingMesh->SetHiddenInGame(!bVisible);
}

void AC_ShieldMonster::PushTelegraphProgress()
{
	if (parryRingMID)
		parryRingMID->SetScalarParameterValue(parryRingProgressParam, GetTelegraphProgress());
}

void AC_ShieldMonster::UpdateTelegraphRingTransform()
{
	if (!parryRingMesh) return;

	if (!bParryRingFaceCamera)
	{
		// 기존 방식 — 발밑에 수평으로 눕힌 평면
		const float halfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
		parryRingMesh->SetRelativeLocation(FVector(0.f, 0.f, -halfHeight + parryRingGroundOffset));
		parryRingMesh->SetRelativeRotation(FRotator::ZeroRotator);
		return;
	}

	parryRingMesh->SetRelativeLocation(FVector(0.f, 0.f, parryRingCameraHeightOffset));

	APlayerCameraManager* camManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!camManager) return;

	// Plane 메시는 로컬 +Z가 앞면 → Z를 카메라 쪽으로, Y를 화면 위쪽으로 맞추면 화면 정면 빌보드가 된다.
	// 카메라 위치가 아니라 카메라 회전 기준이라 화면 가장자리에서도 원근으로 찌그러지지 않음
	const FMatrix camMatrix = FRotationMatrix(camManager->GetCameraRotation());
	const FVector ringForward = -camMatrix.GetUnitAxis(EAxis::X);   // 카메라 전방의 반대 = 카메라를 바라봄
	const FVector ringUp      =  camMatrix.GetUnitAxis(EAxis::Z);   // 화면 위쪽

	FQuat ringRot = FRotationMatrix::MakeFromZY(ringForward, ringUp).ToQuat();
	if (!FMath::IsNearlyZero(parryRingRoll))
		ringRot = ringRot * FQuat(FVector::ZAxisVector, FMath::DegreesToRadians(parryRingRoll));

	parryRingMesh->SetWorldRotation(ringRot);
}
#pragma endregion

void AC_ShieldMonster::DrawGuardDebug() const
{
	UWorld* world = GetWorld();
	if (!world) return;

	const FVector  origin  = GetActorLocation();
	const FVector  forward = GetActorForwardVector().GetSafeNormal2D();
	const float    length  = 200.f;
	const FColor   color   = bGuardActive ? FColor::Green : FColor::Red;

	DrawDebugLine(world, origin, origin + forward * length, color, false, -1.f, 0, 3.f);

	// 가드 경계선 두 개
	const FVector leftEdge  = forward.RotateAngleAxis(+guardHalfAngle, FVector::UpVector);
	const FVector rightEdge = forward.RotateAngleAxis(-guardHalfAngle, FVector::UpVector);
	DrawDebugLine(world, origin, origin + leftEdge  * length, FColor::Blue, false, -1.f, 0, 2.f);
	DrawDebugLine(world, origin, origin + rightEdge * length, FColor::Blue, false, -1.f, 0, 2.f);
}
