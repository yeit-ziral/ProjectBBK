// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BaseMonster.h"
#include "../LevelSystem/C_BBKGameMode.h"
#include "Manager/C_AttackManagerComponent.h"
#include "Manager/C_MonsterDataComponent.h"
#include "Manager/C_GroggyComponent.h"
#include "UI/C_MonsterHPDisplayComponent.h"
#include "AI/C_MonsterAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Animation/AnimInstance.h"
#include "../Skills/C_ExpOrb.h"

AC_BaseMonster::AC_BaseMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	monsterASC = CreateDefaultSubobject<UC_MonsterASC>(TEXT("MonsterASC"));
	monsterASC->SetIsReplicated(true);
	monsterASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	monsterAttributeSet = CreateDefaultSubobject<UC_MonsterAttributeSet>(TEXT("MonsterAttributes"));
	monsterASC->AddAttributeSetSubobject(monsterAttributeSet);

	attackManager      = CreateDefaultSubobject<UC_AttackManagerComponent>  (TEXT("AttackManager"));
	dataComponent      = CreateDefaultSubobject<UC_MonsterDataComponent>    (TEXT("DataComponent"));
	groggyComponent    = CreateDefaultSubobject<UC_GroggyComponent>         (TEXT("GroggyComponent"));
	hpDisplayComponent = CreateDefaultSubobject<UC_MonsterHPDisplayComponent>(TEXT("HPDisplayComponent"));

	AIControllerClass = AC_MonsterAIController::StaticClass();
	AutoPossessAI     = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement    = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate                  = FRotator(0.f, 720.f, 0.f);

	HpWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpWidget"));
	HpWidgetComponent->SetupAttachment(GetRootComponent());
	HpWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	HpWidgetComponent->SetDrawSize(FVector2D(500.0f, 110.0f));
	HpWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	HpWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HpWidgetComponent->SetTwoSided(true);
}

void AC_BaseMonster::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 위젯 클래스 설정 — BeginPlay 전에 위젯 객체가 생성되어야 함
	if (hpDisplayComponent)
		hpDisplayComponent->InitializeWidgetClass(HpWidgetComponent);
}

void AC_BaseMonster::BeginPlay()
{
	Super::BeginPlay();

	// Blueprint에 None이 직렬화된 경우 컴포넌트 목록에서 복구
	if (!attackManager)
		attackManager = FindComponentByClass<UC_AttackManagerComponent>();
	if (!dataComponent)
		dataComponent = FindComponentByClass<UC_MonsterDataComponent>();
	if (!groggyComponent)
		groggyComponent = FindComponentByClass<UC_GroggyComponent>();
	if (!hpDisplayComponent)
		hpDisplayComponent = FindComponentByClass<UC_MonsterHPDisplayComponent>();

	// GAS 초기화
	if (monsterASC)
	{
		monsterASC->InitAbilityActorInfo(this, this);

		for (const TSubclassOf<UGameplayAbility>& abilityClass : gamePlayAbilities)
		{
			if (!abilityClass) continue;
			monsterASC->GiveAbility(FGameplayAbilitySpec(abilityClass, 1, 0));
		}

		monsterASC->OnMonsterDeath.AddLambda([this](UC_MonsterASC* ASC)
		{
			ExecuteDeathSequence();
		});
	}

	// DataTable에서 스탯 로드 (서버/단독 실행에서만)
	if (HasAuthority() && dataComponent)
		dataComponent->Initialize(this);

	// curGroggy는 권한 여부와 관계없이 0으로 초기화
	if (monsterAttributeSet)
		monsterAttributeSet->SetcurGroggy(0.0f);

	ApplyMonsterTypeTag();

	// UI 초기화 (DataComponent 이후 — 스탯이 이미 세팅된 상태)
	if (hpDisplayComponent)
		hpDisplayComponent->Initialize(this, HpWidgetComponent);

	// 그로기 컴포넌트 초기화
	if (groggyComponent)
		groggyComponent->Initialize(this, hpDisplayComponent);

	// 어택 매니저 초기화
	UE_LOG(LogTemp, Warning, TEXT("[BaseMonster] BeginPlay — AttackMgr=%s  %s"),
		attackManager ? TEXT("valid") : TEXT("NULL"), *GetName());
	if (attackManager)
		attackManager->Initialize(this);

	GetMesh()->SetReceivesDecals(false);
}

void AC_BaseMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (hpDisplayComponent)
		hpDisplayComponent->UpdateWidgetRotation();

}

void AC_BaseMonster::ApplyMonsterTypeTag()
{
	if (!monsterASC || !monsterTypeTag.IsValid()) return;
	monsterASC->AddLooseGameplayTag(monsterTypeTag);
}

UC_AttackManagerComponent* AC_BaseMonster::GetAttackManager() const
{
	if (attackManager) return attackManager;
	return FindComponentByClass<UC_AttackManagerComponent>();
}

int32 AC_BaseMonster::GetMonsterID() const
{
	return dataComponent ? dataComponent->GetMonsterId() : 0;
}

FName AC_BaseMonster::GetRowName() const
{
	return dataComponent ? dataComponent->GetRowName() : NAME_None;
}

bool AC_BaseMonster::CanAutoAttack() const
{
	return attackManager && (attackManager->CanAttack() || attackManager->CanSpecialAttack());
}

void AC_BaseMonster::StartHitFlash()
{
	if (!hitFlashMaterial) return;
	GetWorld()->GetTimerManager().ClearTimer(hitFlashTimerHandle);
	hitFlashStep = 0;
	HitFlashTick();
}

void AC_BaseMonster::HitFlashTick()
{
	if (!IsValid(this) || !GetMesh()) return;

	GetMesh()->SetOverlayMaterial(hitFlashStep % 2 == 0 ? hitFlashMaterial : nullptr);
	hitFlashStep++;

	if (hitFlashStep < 6)  // 3번 점멸 (on/off 각 3회)
		GetWorld()->GetTimerManager().SetTimer(hitFlashTimerHandle, this, &AC_BaseMonster::HitFlashTick, 0.08f, false);
}

void AC_BaseMonster::TakeHitReaction()
{
	if (!hitReactionMontage) return;
	if (IsPlayingAttackAnimation()) return;

	UC_MonsterASC* asc = GetMonsterASC();
	if (IsValid(asc) && (asc->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")))
	                  || asc->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Groggy")))))
		return;

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	PlayAnimMontage(hitReactionMontage);

	if (UAnimInstance* animInst = GetMesh()->GetAnimInstance())
	{
		FOnMontageEnded endDelegate;
		endDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
		{
			// bInterrupted=true면 다음 피격 반응이 이동을 다시 차단 — 여기서 복구하지 않음
			if (bInterrupted) return;
			if (!IsValid(this)) return;

			UC_MonsterASC* curAsc = GetMonsterASC();
			if (IsValid(curAsc) && (curAsc->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")))
			                     || curAsc->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Groggy")))))
				return;

			GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		});
		animInst->Montage_SetEndDelegate(endDelegate, hitReactionMontage);
	}
}

bool  AC_BaseMonster::IsRepositionEnabled()       const { return dataComponent ? dataComponent->IsRepositionEnabled()       : false; }
float AC_BaseMonster::GetRepositionDesiredRange() const { return dataComponent ? dataComponent->GetRepositionDesiredRange() : 250.f; }
float AC_BaseMonster::GetRepositionMinRange()     const { return dataComponent ? dataComponent->GetRepositionMinRange()     : 0.f; }
float AC_BaseMonster::GetRepositionSpeed()        const { return dataComponent ? dataComponent->GetRepositionSpeed()        : 300.f; }
float AC_BaseMonster::GetRepositionStrafeWeight() const { return dataComponent ? dataComponent->GetRepositionStrafeWeight() : 0.5f; }
float AC_BaseMonster::GetRepositionBand()         const { return dataComponent ? dataComponent->GetRepositionBand()         : 60.f; }
float AC_BaseMonster::GetRepositionFlipInterval() const { return dataComponent ? dataComponent->GetRepositionFlipInterval() : 2.5f; }

void AC_BaseMonster::ExecuteDeathSequence()
{
	// 0. ExpOrb 드랍
	if (ExpOrbClass && dataComponent)
	{
		const float reward = dataComponent->GetExpReward();
		if (reward > 0.f)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AC_ExpOrb* Orb = GetWorld()->SpawnActor<AC_ExpOrb>(ExpOrbClass, GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
			if (Orb) Orb->InitOrb(reward);
		}
	}

	// GameMode에 몬스터 사망 알림
	if (AC_BBKGameMode* GM = GetWorld()->GetAuthGameMode<AC_BBKGameMode>())
	{
		GM->NotifyMonsterDead();
	}

	// 1. 진행 중인 GA 전부 즉시 취소 (빔·스톰 오브젝트 포함)
	if (monsterASC)
		monsterASC->CancelAllAbilities();

	// 2. AI 영구 정지 + 회전 고정
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		AIC->StopMovement();
		AIC->ClearFocus(EAIFocusPriority::Gameplay);
		if (UBrainComponent* Brain = AIC->GetBrainComponent())
			Brain->StopLogic(TEXT("Dead"));
	}
	bUseControllerRotationYaw = false;

	// 3. 캐릭터 이동 비활성화
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	// 4. 콜리전 비활성화 (플레이어 통과, 추가 피격 방지)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetWorld()->GetTimerManager().ClearTimer(hitFlashTimerHandle);
	if (GetMesh()) GetMesh()->SetOverlayMaterial(nullptr);

	// 5. HP 위젯 숨김
	if (HpWidgetComponent)
		HpWidgetComponent->SetVisibility(false);

	// 6. Death 몽타주 재생
	if (deathMontage)
	{
		PlayAnimMontage(deathMontage);

		// 블렌드아웃 시작 시점에 VFX 발동 (폴백)
		if (UAnimInstance* animInst = GetMesh()->GetAnimInstance())
		{
			FOnMontageBlendingOutStarted blendOutDelegate;
			blendOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				OnDeathMontageVFXPoint();
			});
			animInst->Montage_SetBlendingOutDelegate(blendOutDelegate, deathMontage);
		}
	}

	// deathVFXDelay 후 강제 트리거 — 몽타주가 길어도 이 시간 안에 연기+숨김 실행
	GetWorld()->GetTimerManager().SetTimer(
		deathVFXTimerHandle,
		this,
		&AC_BaseMonster::OnDeathMontageVFXPoint,
		deathVFXDelay,
		false
	);

	if (!deathMontage)
	{
		OnDeathMontageVFXPoint();
	}
}

void AC_BaseMonster::OnDeathMontageVFXPoint()
{
	// AnimNotify + 몽타주 종료 폴백 모두 이 함수를 호출할 수 있으므로 중복 방지
	if (bDeathVFXTriggered) return;
	bDeathVFXTriggered = true;

	// 1. 검은 안개 VFX 스폰 (fire-and-forget)
	if (deathFogVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			deathFogVFX,
			GetActorLocation(),
			GetActorRotation(),
			FVector(1.0f),
			true,   // bAutoDestroy
			true    // bAutoActivate
		);
	}

	// 2. 메시 숨김 (안개가 시체를 가리는 동안)
	GetMesh()->SetVisibility(false);

	// 3. deathDestroyDelay 후 액터 소멸
	GetWorld()->GetTimerManager().SetTimer(
		deathDestroyTimerHandle,
		this,
		&AC_BaseMonster::DestroyAfterDeath,
		deathDestroyDelay,
		false
	);
}

void AC_BaseMonster::DestroyAfterDeath()
{
	Destroy();
}

void AC_BaseMonster::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
