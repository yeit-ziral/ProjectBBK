// Fill out your copyright notice in the Description page of Project Settings.

#include "C_GroggyComponent.h"
#include "../C_BaseMonster.h"
#include "../M_Gas/C_MonsterASC.h"
#include "../M_Gas/C_MonsterAttributeSet.h"
#include "../UI/C_MonsterHPDisplayComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagContainer.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Animation/AnimInstance.h"

UC_GroggyComponent::UC_GroggyComponent()
{
	// 그로기 지속 중에만 켠다 (StartGroggy에서 활성화, ResetGroggy에서 해제)
	PrimaryComponentTick.bCanEverTick          = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UC_GroggyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TickGroggyDrain(DeltaTime);
}

void UC_GroggyComponent::TickGroggyDrain(float DeltaTime)
{
	if (!bGroggyDraining) return;

	if (!IsValid(ownerMonster) || groggyTotalTime <= 0.0f)
	{
		bGroggyDraining = false;
		SetComponentTickEnabled(false);
		return;
	}

	UC_MonsterAttributeSet* attrSet = ownerMonster->GetMonsterAttributeSet();
	if (!IsValid(attrSet))
	{
		bGroggyDraining = false;
		SetComponentTickEnabled(false);
		return;
	}

	groggyElapsedTime += DeltaTime;

	const float remainRatio = FMath::Clamp(1.0f - (groggyElapsedTime / groggyTotalTime), 0.0f, 1.0f);
	attrSet->SetcurGroggy(attrSet->GetmaxGroggy() * remainRatio);
}

void UC_GroggyComponent::Initialize(AC_BaseMonster* InOwner, UC_MonsterHPDisplayComponent* InHPDisplay)
{
	ownerMonster    = InOwner;
	hpDisplayComponent = InHPDisplay;
}

void UC_GroggyComponent::AddGroggy(float GroggyAmount)
{
	if (!IsValid(ownerMonster)) return;

	UWorld* world = GetWorld();
	if (!world) return;

	UC_MonsterAttributeSet* attrSet = ownerMonster->GetMonsterAttributeSet();
	if (!IsValid(attrSet)) return;

	if (!CanEnterGroggy()) return;

	const float cur    = attrSet->GetcurGroggy();
	const float max    = attrSet->GetmaxGroggy();
	const float newVal = FMath::Clamp(cur + GroggyAmount, 0.0f, max);

	attrSet->SetcurGroggy(newVal);

	if (newVal >= max)
	{
		if (!world->GetTimerManager().IsTimerActive(groggyResetTimerHandle))
			StartGroggy(groggyDuration);
	}
}

void UC_GroggyComponent::ForceGroggy(float Duration)
{
	if (!IsValid(ownerMonster)) return;

	UWorld* world = GetWorld();
	if (!world) return;

	if (!CanEnterGroggy()) return;
	if (world->GetTimerManager().IsTimerActive(groggyResetTimerHandle)) return;

	// 게이지도 꽉 채워 둔다 — HP 위젯의 그로기 바가 진입 상태와 어긋나지 않도록
	if (UC_MonsterAttributeSet* attrSet = ownerMonster->GetMonsterAttributeSet())
		attrSet->SetcurGroggy(attrSet->GetmaxGroggy());

	StartGroggy(Duration > 0.0f ? Duration : groggyDuration);
}

bool UC_GroggyComponent::CanEnterGroggy() const
{
	UC_MonsterASC* asc = ownerMonster ? ownerMonster->GetMonsterASC() : nullptr;
	if (!IsValid(asc)) return true;   // ASC가 없으면 태그 판정 불가 — 기존 동작 유지

	return !asc->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")))
	    && !asc->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Groggy")))
	    && !asc->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Invincible")));
}

void UC_GroggyComponent::StartGroggy(float Duration)
{
	EnterGroggyState();

	// 게이지 감소 시작 — 진입 시점 값이 max가 아닐 수 있으므로 여기서 max로 맞춘다
	if (UC_MonsterAttributeSet* attrSet = ownerMonster ? ownerMonster->GetMonsterAttributeSet() : nullptr)
		attrSet->SetcurGroggy(attrSet->GetmaxGroggy());

	bGroggyDraining   = true;
	groggyElapsedTime = 0.0f;
	groggyTotalTime   = Duration;
	SetComponentTickEnabled(true);

	if (UWorld* world = GetWorld())
	{
		world->GetTimerManager().SetTimer(
			groggyResetTimerHandle,
			this,
			&UC_GroggyComponent::ResetGroggy,
			Duration,
			false
		);
	}
}

void UC_GroggyComponent::ResetGroggy()
{
	bGroggyDraining = false;
	SetComponentTickEnabled(false);

	if (!ownerMonster) return;

	ExitGroggyState();

	if (UC_MonsterAttributeSet* attrSet = ownerMonster->GetMonsterAttributeSet())
		attrSet->SetcurGroggy(0.0f);
}

void UC_GroggyComponent::EnterGroggyState()
{
	if (!ownerMonster) return;

	UC_MonsterASC* asc = ownerMonster->GetMonsterASC();
	if (!asc) return;

	asc->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Groggy")));
	asc->InterruptCurrentAbilities();

	if (AAIController* AIC = Cast<AAIController>(ownerMonster->GetController()))
	{
		AIC->StopMovement();
		AIC->ClearFocus(EAIFocusPriority::Gameplay);
		if (UBehaviorTreeComponent* BTC = Cast<UBehaviorTreeComponent>(AIC->GetBrainComponent()))
			BTC->StopTree(EBTStopMode::Forced);
	}

	ownerMonster->GetCharacterMovement()->StopMovementImmediately();
	ownerMonster->GetCharacterMovement()->DisableMovement();

	if (groggyMontage)
		ownerMonster->PlayAnimMontage(groggyMontage);

	if (hpDisplayComponent)
		hpDisplayComponent->ShowGroggyStatus();
}

void UC_GroggyComponent::ExitGroggyState()
{
	if (!ownerMonster) return;

	UC_MonsterASC* asc = ownerMonster->GetMonsterASC();
	if (!asc) return;

	// 그로기 리셋 타이머 도중 몬스터가 사망한 경우 무시
	if (asc->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")))) return;

	asc->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Groggy")));

	if (groggyMontage)
	{
		if (UAnimInstance* anim = ownerMonster->GetMesh()->GetAnimInstance())
			anim->Montage_Stop(0.0f, groggyMontage);
	}

	ownerMonster->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	if (hpDisplayComponent)
		hpDisplayComponent->ClearGroggyStatus();

	if (AAIController* AIC = Cast<AAIController>(ownerMonster->GetController()))
	{
		if (UBehaviorTreeComponent* BTC = Cast<UBehaviorTreeComponent>(AIC->GetBrainComponent()))
		{
			if (UBehaviorTree* BT = ownerMonster->GetBehaviorTree())
				BTC->StartTree(*BT, EBTExecutionMode::Looped);
		}
	}
}
