// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BossMonster.h"
#include "M_Gas/C_MonsterASC.h"
#include "UI/C_BossMonsterHPWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Object/C_BossStorm.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "BrainComponent.h"

AC_BossMonster::AC_BossMonster()
{
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AC_BossMonster::BeginPlay()
{
    Super::BeginPlay();

    monsterTypeTag = FGameplayTag::RequestGameplayTag(TEXT("Monster.Type.Boss"));

    if (HpWidgetComponent)
    {
        HpWidgetComponent->SetVisibility(false);
        HpWidgetComponent->SetHiddenInGame(true);
    }

    InitializeBossHpWidget();

    if (monsterASC)
    {
        monsterASC->GetGameplayAttributeValueChangeDelegate(
            UC_MonsterAttributeSet::GetcurHPAttribute()
        ).AddUObject(this, &AC_BossMonster::OnBossHpChanged);

        monsterASC->GetGameplayAttributeValueChangeDelegate(
            UC_MonsterAttributeSet::GetcurGroggyAttribute()
        ).AddUObject(this, &AC_BossMonster::OnBossGroggyChanged);
    }

    if (monsterASC)
    {
        monsterASC->RegisterGameplayTagEvent(
            FGameplayTag::RequestGameplayTag(FName("State.Invincible")),
            EGameplayTagEventType::NewOrRemoved
        ).AddWeakLambda(this, [this](const FGameplayTag& Tag, int32 NewCount)
        {
            OnInvincibleTagChanged(Tag, NewCount);
        });

        // 패턴 지속 시간 동안 쿨타임이 소진되는 문제는 AC_BaseMonster의 공격 시계가 해결한다
        // — 패턴 GA가 활성인 동안 IsPlayingAttackAnimation()이 true라 시계 자체가 멈추므로,
        //   패턴 종료 시점에 lastPatternAttackTime을 다시 찍을 필요가 없다.
    }

    // GA 등록 — BT Task에서 활성화
    if (monsterASC)
    {
        if (bossNormalAttackGA)
            monsterASC->GiveAbility(FGameplayAbilitySpec(bossNormalAttackGA, 1, 0));
        if (bossStormPatternGA)
            monsterASC->GiveAbility(FGameplayAbilitySpec(bossStormPatternGA, 1, 0));
        if (bossBeamPatternGA)
            monsterASC->GiveAbility(FGameplayAbilitySpec(bossBeamPatternGA, 1, 0));
        if (bossGridLaserPatternGA)
            monsterASC->GiveAbility(FGameplayAbilitySpec(bossGridLaserPatternGA, 1, 0));
    }
}

void AC_BossMonster::ExecuteDeathSequence()
{
    RemoveBossHpWidget();

    // StormGA가 이미 정상 종료된 뒤 보스가 사망하는 경우 대비 — 월드에 남은 Storm 액터 일괄 제거
    TArray<AActor*> remainingStorms;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AC_BossStorm::StaticClass(), remainingStorms);
    for (AActor* storm : remainingStorms)
        if (IsValid(storm)) storm->Destroy();

    Super::ExecuteDeathSequence();
}

// ─── 공격 쿨다운 체크 ─────────────────────────────────────────────────────────

bool AC_BossMonster::IsPlayingAttackAnimation() const
{
    return IsAbilityActive(bossNormalAttackGA)
        || IsAbilityActive(bossStormPatternGA)
        || IsAbilityActive(bossBeamPatternGA)
        || IsAbilityActive(bossGridLaserPatternGA);
}

bool AC_BossMonster::CanAutoAttack() const
{
    if (!monsterASC) return false;
    if (monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"))))
        return false;
    if (monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Boss.BeamPattern"))) ||
        monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Boss.StormPattern"))))
        return false;
    return CanNormalAttack() || CanPatternAttack();
}

bool AC_BossMonster::CanNormalAttack() const
{
    if (!monsterASC) return false;
    if (monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"))))
        return false;
    if (monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Boss.BeamPattern"))) ||
        monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Boss.StormPattern"))))
        return false;

    return GetAttackClock() - lastNormalAttackTime >= GetAttackCooldown();
}

bool AC_BossMonster::CanPatternAttack() const
{
    if (monsterASC && monsterASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"))))
        return false;
    return GetAttackClock() - lastPatternAttackTime >= GetSpecialCooldown();
}

// ─── BT Task 호출 진입점 ──────────────────────────────────────────────────────

void AC_BossMonster::BossNormalAttack()
{
    if (!monsterASC || !bossNormalAttackGA)
        return;
    if (!CanNormalAttack()) return;

    lastNormalAttackTime = GetAttackClock();
    monsterASC->TryActivateAbilityByClass(bossNormalAttackGA);
}

void AC_BossMonster::BossPatternAttack()
{
    if (!monsterASC) return;
    if (!CanPatternAttack()) return;

    lastPatternAttackTime = GetAttackClock();

    // Storm → Beam → Storm 교대
    if (bNextPatternIsStorm)
    {
        if (bossStormPatternGA)
        {
            bNextPatternIsStorm = false;
            monsterASC->TryActivateAbilityByClass(bossStormPatternGA);
        }
    }
    else
    {
        if (bossBeamPatternGA)
        {
            bNextPatternIsStorm = true;
            monsterASC->TryActivateAbilityByClass(bossBeamPatternGA);
        }
    }
}

// ─── HP 위젯 ─────────────────────────────────────────────────────────────────

void AC_BossMonster::InitializeBossHpWidget()
{
    if (!bossHpWidgetClass)
        return;

    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (!PlayerController)
        return;

    bossHpWidget = CreateWidget<UC_BossMonsterHPWidget>(PlayerController, bossHpWidgetClass);
    if (!bossHpWidget)
        return;

    bossHpWidget->AddToViewport();
    bossHpWidget->SetMaxHp(GetmaxHP());
    bossHpWidget->SetCurrentHp(GetcurHP());
    bossHpWidget->SetMaxGroggy(GetmaxGroggy());
    bossHpWidget->SetCurrentGroggy(GetcurGroggy());
    bossHpWidget->SetMonsterLevel(50);
    bossHpWidget->SetMonsterName(FText::FromName(GetRowName()));

    // 컷신 트리거가 미리 숨김을 요청했다면 생성 직후 숨김 적용
    if (bSuppressHpWidget)
        bossHpWidget->SetVisibility(ESlateVisibility::Hidden);
}

void AC_BossMonster::SetHpWidgetSuppressed(bool bSuppressed)
{
    bSuppressHpWidget = bSuppressed;

    if (bossHpWidget)
        bossHpWidget->SetVisibility(bSuppressed ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
}

void AC_BossMonster::RemoveBossHpWidget()
{
    if (!bossHpWidget)
        return;

    bossHpWidget->RemoveFromParent();
    bossHpWidget = nullptr;
}

// ─── 이벤트 콜백 ─────────────────────────────────────────────────────────────

void AC_BossMonster::OnBossHpChanged(const FOnAttributeChangeData& ChangeData)
{
    if (bossHpWidget)
        bossHpWidget->SetCurrentHp(ChangeData.NewValue);

    if (!bPhase2Triggered && monsterASC && bossGridLaserPatternGA)
    {
        const float maxHp = GetmaxHP();
        const float ratio = maxHp > 0.f ? ChangeData.NewValue / maxHp : 1.f;

        if (maxHp > 0.f && ratio < phase2HpRatio && ratio > 0.1f)
        {
            bPhase2Triggered = true;

            // 진행 중인 모든 GA 즉시 취소 (빔·스톰 등)
            monsterASC->CancelAllAbilities();

            // 월드에 남아있는 Storm 오브젝트 일괄 제거
            TArray<AActor*> remainingStorms;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), AC_BossStorm::StaticClass(), remainingStorms);
            for (AActor* storm : remainingStorms)
                if (IsValid(storm)) storm->Destroy();

            monsterASC->TryActivateAbilityByClass(bossGridLaserPatternGA);
        }
    }
}

void AC_BossMonster::OnBossGroggyChanged(const FOnAttributeChangeData& ChangeData)
{
    if (!bossHpWidget) return;
    bossHpWidget->SetCurrentGroggy(ChangeData.NewValue);
}

void AC_BossMonster::OnInvincibleTagChanged(const FGameplayTag& Tag, int32 NewCount)
{
    if (bossHpWidget)
        bossHpWidget->SetInvincible(NewCount > 0);

    if (NewCount > 0)
    {
        // 반피 패턴 시작: BT 정지 + 숨김 + 충돌 비활성 + 이동 정지
        if (AAIController* AIC = Cast<AAIController>(GetController()))
        {
            AIC->StopMovement();
            if (UBrainComponent* Brain = AIC->GetBrainComponent())
                Brain->StopLogic(TEXT("Phase2"));
        }
        GetMesh()->SetVisibility(false);
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->DisableMovement();
    }
    else
    {
        // 패턴 종료: 복구 + BT 재시작
        GetMesh()->SetVisibility(true);
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);

        // 반피 패턴 지속 시간만큼 쿨타임이 소진됐으므로 종료 시점 기준으로 리셋
        const float now = GetAttackClock();
        lastNormalAttackTime  = now;
        lastPatternAttackTime = now;

        if (AAIController* AIC = Cast<AAIController>(GetController()))
        {
            if (UBrainComponent* Brain = AIC->GetBrainComponent())
                Brain->RestartLogic();
        }
    }
}
