// Fill out your copyright notice in the Description page of Project Settings.

#include "C_LevelUpPerkComponent.h"
#include "C_PlayerState.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

void UC_LevelUpPerkComponent::HandleLevelUp(int32 NewLevel, int32 OldLevel)
{
	// ⚠️ level 어트리뷰트는 최초 초기화(예: 0→1) 때도 변경 델리게이트가 불린다.
	//    "실제로 레벨이 올랐을 때"만 처리해야 게임 시작 직후 선택창이 뜨는 사고를 막는다.
	if (NewLevel <= OldLevel)
	{
		return;
	}

	// 한 번에 2레벨 이상 오를 수 있으므로(경험치를 왕창 얻은 경우) 차이만큼 큐에 쌓는다.
	PendingLevelUps += (NewLevel - OldLevel);

	// 이미 선택창이 떠 있으면(=후보가 살아있으면) 새로 띄우지 않는다.
	// 선택을 마치면 SelectPerk가 다음 세트를 알아서 띄운다.
	if (PendingChoices.Num() == 0)
	{
		PresentNextChoices();
	}
}

void UC_LevelUpPerkComponent::PresentNextChoices()
{
	if (PendingLevelUps <= 0 || !PerkTable)
	{
		return;
	}

	// 1) DataTable의 모든 보상 행을 모은다.
	TArray<FPerkData*> Rows;
	PerkTable->GetAllRows<FPerkData>(TEXT("PresentNextChoices"), Rows);
	if (Rows.Num() == 0)
	{
		return;
	}

	// 2) 서로 다른 ChoiceCount개를 랜덤으로 뽑는다(행이 부족하면 있는 만큼).
	//    인덱스 풀에서 뽑은 것을 RemoveAtSwap으로 빼서 중복을 막는다.
	PendingChoices.Reset();

	TArray<int32> Pool;
	Pool.Reserve(Rows.Num());
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		Pool.Add(i);
	}

	const int32 Pick = FMath::Min(ChoiceCount, Pool.Num());
	for (int32 i = 0; i < Pick; ++i)
	{
		const int32 r = FMath::RandRange(0, Pool.Num() - 1);
		PendingChoices.Add(*Rows[Pool[r]]);
		Pool.RemoveAtSwap(r);
	}

	// 3) BP(UMG)에게 "이 후보들을 띄워라" 통보.
	OnPerkChoicesReady.Broadcast(PendingChoices);
}

void UC_LevelUpPerkComponent::SelectPerk(int32 Index)
{
	if (!PendingChoices.IsValidIndex(Index))
	{
		return;
	}

	// 후보를 복사해 둔다(아래에서 PendingChoices를 비우기 때문).
	const FPerkData Chosen = PendingChoices[Index];

	// ASC는 CLAUDE.md 규칙대로 PlayerState에서 취득. (Character 직접 참조 금지)
	AC_PlayerState* PS = Cast<AC_PlayerState>(GetOwner());
	UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;

	if (ASC && Chosen.Effect)
	{
		// 데미지에서 쓰던 SetByCaller와 동일한 방식. GE의 Modifier Magnitude를 데이터로 주입한다.
		FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Chosen.Effect, 1.f, Ctx);
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(
				FGameplayTag::RequestGameplayTag(FName("Data.Perk")), Chosen.Magnitude);
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		}
	}

	// 풀회복 같은 즉발 효과(있을 때만).
	if (ASC && Chosen.OnApplyEffect)
	{
		FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Chosen.OnApplyEffect, 1.f, Ctx);
		if (Spec.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		}
	}

	// 이번 선택 처리 완료. 큐에 남은 레벨업이 있으면 다음 선택창을 띄운다.
	PendingChoices.Reset();
	PendingLevelUps = FMath::Max(0, PendingLevelUps - 1);
	if (PendingLevelUps > 0)
	{
		PresentNextChoices();
	}
}
