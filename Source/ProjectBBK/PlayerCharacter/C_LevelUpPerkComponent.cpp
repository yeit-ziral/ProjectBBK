// Fill out your copyright notice in the Description page of Project Settings.

#include "C_LevelUpPerkComponent.h"
#include "C_PlayerState.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "AbilitySystemBlueprintLibrary.h"

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

	//속성 보상이면 스탯 GE 경로 대신 속성 처리로 분기
	if (Chosen.elementTag.IsValid())
	{
		AddElementLevel(Chosen);

		//선택 처리 마무리 (기존 SelectPerk 끝부분과 동일하게 큐 정리)
		PendingChoices.Reset();
		PendingLevelUps = FMath::Max(0, PendingLevelUps - 1);
		if (PendingLevelUps > 0) 
			PresentNextChoices();
		else
			OnPerkSelectionFinished.Broadcast();

		return; // 속성 보상은 여기서 끝. 아래 GE 적용 코드는 실행하지 않는다.
	}

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


void UC_LevelUpPerkComponent::AddElementLevel(const FPerkData& Perk)
{
	// 해당 속성 상태를 찾거나 새로 만든다.
	FElementState& State = elements.FindOrAdd(Perk.elementTag);
	State.DamagePerLevel = Perk.elementDamagePerLevel;
	State.MaxLevel = Perk.elementMaxLevel;
	State.Color = Perk.elementColor;

	// 레벨 +1
	State.Level = FMath::Min(State.Level + 1, State.MaxLevel);

	// 다른 시스템이 참조할 수 있게 ASC에도 속성 태그 부여
	if(AC_PlayerState* PS = Cast<AC_PlayerState>(GetOwner()))
	{
		if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
		{
			ASC->AddLooseGameplayTag(Perk.elementTag);
		}
	}

	// primary 다시 계산
	int32 BestLevel = -1;

	for (const TPair<FGameplayTag, FElementState>& Pair : elements)
	{
		if (Pair.Value.Level > BestLevel)
		{
			BestLevel = Pair.Value.Level;
			primaryElement = Pair.Key;
		}
	}

	// 무기 이펙트 갈아끼우라고 BP에 알림 (최고 레벨의 속성 것만 보임)
	const FElementState* Primary = elements.Find(primaryElement);
	OnPrimaryElementChanged.Broadcast(primaryElement, Primary ? Primary->Color : FLinearColor::Black);
}

float UC_LevelUpPerkComponent::ComputeElementalTrueDamage() const
{
	// primary 속성만 100% 적용. 나머지는 40% 적용.
	const FElementState* Primary = elements.Find(primaryElement);
	if(!Primary)
		return 0.f;

	float Total = Primary->Level * Primary->DamagePerLevel;

	// 나머지 속성 합산
	for(const auto& Pair : elements)
	{
		if(Pair.Key != primaryElement)
		{
			Total += Pair.Value.Level * Pair.Value.DamagePerLevel * 0.4f;
		}
	}

	return Total;
}

void UC_LevelUpPerkComponent::ApplyElementalDamage(AActor* TargetActor)
{
	if(!TargetActor)
		return;

	AC_PlayerState* PS = Cast<AC_PlayerState>(GetOwner());
	UAbilitySystemComponent* SourceASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
	
	// 맞은 적의 ASC
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	
	if (!SourceASC || !TargetASC || !elementalDamageEffect)
		return;

	const float TrueDamage = ComputeElementalTrueDamage();
	if (TrueDamage <= 0.f)
		return;

	FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(elementalDamageEffect, 1.f, Ctx);
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(
			FGameplayTag::RequestGameplayTag(FName("Data.Damage")), TrueDamage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
	}
}

void UC_LevelUpPerkComponent::DebugSelectPerk(FName RowName)
{
	if (!PerkTable)
		return;

	const FPerkData* Row = PerkTable->FindRow<FPerkData>(RowName, TEXT("DebugSelectPerk"));
	if (!Row)
		return;

	// 속성 보상이면 속성 레벨 부여 (SelectPerk의 속성 분기와 동일)
	if (Row->elementTag.IsValid())
	{
		AddElementLevel(*Row);
	}
	// 스탯 보상도 테스트 하려면 여기서 Row->Effect를 ASC에 적용하면 된다.
}
