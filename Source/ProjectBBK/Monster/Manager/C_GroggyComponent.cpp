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
	PrimaryComponentTick.bCanEverTick = false;
}

void UC_GroggyComponent::Initialize(AC_BaseMonster* InOwner, UC_MonsterHPDisplayComponent* InHPDisplay)
{
	ownerMonster    = InOwner;
	hpDisplayComponent = InHPDisplay;
}

void UC_GroggyComponent::TickGroggy(float DeltaTime)
{
	AddGroggy(groggyAccumulationRate * DeltaTime);
}

void UC_GroggyComponent::AddGroggy(float GroggyAmount)
{
	if (!IsValid(ownerMonster)) return;

	UWorld* world = GetWorld();
	if (!world) return;

	UC_MonsterAttributeSet* attrSet = ownerMonster->GetMonsterAttributeSet();
	if (!IsValid(attrSet)) return;

	UC_MonsterASC* asc = ownerMonster->GetMonsterASC();
	if (IsValid(asc) && (asc->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")))
	                  || asc->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Groggy")))
	                  || asc->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Invincible")))))
		return;

	const float cur    = attrSet->GetcurGroggy();
	const float max    = attrSet->GetmaxGroggy();
	const float newVal = FMath::Clamp(cur + GroggyAmount, 0.0f, max);

	attrSet->SetcurGroggy(newVal);

	if (newVal >= max)
	{
		if (!world->GetTimerManager().IsTimerActive(groggyResetTimerHandle))
		{
			EnterGroggyState();
			GetWorld()->GetTimerManager().SetTimer(
				groggyResetTimerHandle,
				this,
				&UC_GroggyComponent::ResetGroggy,
				5.0f,
				false
			);
		}
	}
}

void UC_GroggyComponent::ResetGroggy()
{
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
	asc->CancelAllAbilities();

	if (AAIController* AIC = Cast<AAIController>(ownerMonster->GetController()))
	{
		AIC->StopMovement();
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
