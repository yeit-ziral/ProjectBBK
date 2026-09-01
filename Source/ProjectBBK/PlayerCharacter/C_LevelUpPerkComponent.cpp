// Fill out your copyright notice in the Description page of Project Settings.

#include "C_LevelUpPerkComponent.h"
#include "C_PlayerState.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "AbilitySystemBlueprintLibrary.h"

void UC_LevelUpPerkComponent::HandleLevelUp(int32 NewLevel, int32 OldLevel)
{
	// 캐릭터 바꿀 때 레벨 변화가 퍽 띄우지 않게 잠깐 끔
	if (bIgnoreLevelChanges)
		return;

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

	// 2) 아직 뽑을 수 있는 행만 풀에 넣는다. 만렙 퍽은 여기서 걸러지므로
	//    "3개 자리 중 하나를 만렙 퍽이 낭비하는" 일이 생기지 않는다.
	PendingChoices.Reset();

	TArray<int32> Pool;
	Pool.Reserve(Rows.Num());
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		if (IsPerkAvailable(*Rows[i]))
		{
			Pool.Add(i);
		}
	}

	// 남은 후보가 하나도 없으면(전부 만렙) 선택창을 띄우지 않고 큐만 비운다.
	// 안 그러면 빈 창이 뜬 채로 SelectPerk를 못 눌러 게임이 멈춘다.
	if (Pool.Num() == 0)
	{
		PendingLevelUps = 0;
		OnPerkSelectionFinished.Broadcast();
		return;
	}

	// 3) 풀에서 서로 다른 ChoiceCount개를 랜덤으로 뽑는다(부족하면 있는 만큼).
	//    RemoveAtSwap으로 뽑은 인덱스를 빼서 중복을 막는다.
	const int32 Pick = FMath::Min(ChoiceCount, Pool.Num());
	for (int32 i = 0; i < Pick; ++i)
	{
		const int32 r = FMath::RandRange(0, Pool.Num() - 1);
		PendingChoices.Add(*Rows[Pool[r]]);
		Pool.RemoveAtSwap(r);
	}

	// 4) BP(UMG)에게 "이 후보들을 띄워라" 통보. 배열 길이가 3보다 작을 수 있으므로
	//    위젯은 Length를 보고 남는 버튼을 Collapsed 처리해야 한다.
	OnPerkChoicesReady.Broadcast(PendingChoices);
}

int32 UC_LevelUpPerkComponent::GetPerkLevel(const FPerkData& Perk) const
{
	// 퍽 종류별로 레벨이 다른 곳에 저장돼 있다. 그 분기를 여기 한 곳에만 둔다.
	// (예전엔 IsPerkAvailable과 위젯이 각자 분기해서 크리 레벨이 UI에 안 잡혔다)
	if (Perk.elementTag.IsValid())
	{
		return GetElementLevel(Perk.elementTag);
	}

	if (Perk.perkType == EPerkType::Crit)
	{
		return crit.Level;
	}

	return 0; // 스탯 퍽은 레벨 개념이 없다
}

int32 UC_LevelUpPerkComponent::GetPerkMaxLevel(const FPerkData& Perk) const
{
	if (Perk.elementTag.IsValid())
	{
		return Perk.elementMaxLevel;
	}

	if (Perk.perkType == EPerkType::Crit)
	{
		return Perk.critMaxLevel;
	}

	return 0; // 0 = 상한 없음(무한 반복 가능)
}

FText UC_LevelUpPerkComponent::GetPerkEffectText(const FPerkData& Perk) const
{
	const int32 NextLevel = GetPerkLevel(Perk) + 1;

	if (Perk.elementTag.IsValid())
	{
		const float Damage = Perk.elementDamagePerLevel * NextLevel;
		return FText::Format(
			NSLOCTEXT("Perk", "ElementEffect", "Elemental Damage : {0}"),
			FText::AsNumber(Damage));
	}

	if(Perk.perkType == EPerkType::Crit)
	{
		// FCritState::GetChance/GetMultiplier와 같은 식이지만, 아직 미보유(Lv.0)일 수 있어
		// 컴포넌트 상태가 아니라 DT 행의 계수에서 직접 계산한다.
		const float Chance = Perk.ciritChanceBase + (NextLevel - 1) * Perk.critChancePerLevel;
		const float Mult = Perk.critMultiplierBase + (NextLevel - 1) * Perk.critMultiplierPerLevel;

		return FText::Format(
			NSLOCTEXT("Perk", "CritEffect", "Crit : {0} / Multiplier : {1}x"),
			FText::AsPercent(Chance), FText::AsNumber(Mult));
	}

	return FText::GetEmpty();
}

bool UC_LevelUpPerkComponent::IsPerkAvailable(const FPerkData& Perk) const
{
	const int32 MaxLevel = GetPerkMaxLevel(Perk);

	// 상한이 없는 퍽(스탯)은 언제나 후보. 있으면 만렙인지 본다.
	return MaxLevel <= 0 || GetPerkLevel(Perk) < MaxLevel;
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
		FinishSelection();
		return; // 속성 보상은 여기서 끝. 아래 GE 적용 코드는 실행하지 않는다.
	}

	// 크리티컬 보상도 GE가 아니라 컴포넌트 내부 상태로 관리한다.
	// (어트리뷰트로 빼면 몬스터 공격에도 크리가 걸릴 위험이 있어 일부러 분리했다)
	if (Chosen.perkType == EPerkType::Crit)
	{
		AddCritLevel(Chosen);
		FinishSelection();
		return;
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
	FinishSelection();
}

void UC_LevelUpPerkComponent::FinishSelection()
{
	PendingChoices.Reset();
	PendingLevelUps = FMath::Max(0, PendingLevelUps - 1);

	if (PendingLevelUps > 0)
	{
		PresentNextChoices();
	}
	else
	{
		// 예전엔 스탯 경로에서 이걸 안 불러서, 마지막 선택이 스탯 퍽이면
		// 창 닫기 신호가 안 나갔다. 한 곳으로 모으면서 같이 해결.
		OnPerkSelectionFinished.Broadcast();
	}
}


void UC_LevelUpPerkComponent::AddCritLevel(const FPerkData& Perk)
{
	// DT의 계수들을 상태로 옮겨둔다. 이후엔 DT를 다시 안 봐도 레벨만으로 수치가 나온다.
	crit.ChanceBase         = Perk.ciritChanceBase;
	crit.ChancePerLevel     = Perk.critChancePerLevel;
	crit.MultiplierBase     = Perk.critMultiplierBase;
	crit.MultiplierPerLevel = Perk.critMultiplierPerLevel;
	crit.MaxLevel           = Perk.critMaxLevel;

	crit.Level = FMath::Min(crit.Level + 1, crit.MaxLevel);

	// 목표 발동률이 바뀌었으니 PRD 상수 C를 다시 역산한다.
	// 역산은 여기서 딱 한 번. 타격 때는 캐시된 C만 읽는다.
	critPrd.SetTargetChance(crit.GetChance());
}

float UC_LevelUpPerkComponent::RollCriticalDamage(float BaseDamage, bool& bOutCritical)
{
	bOutCritical = false;

	// 미보유면 판정 자체를 하지 않는다. PRD 카운터도 건드리지 않아야
	// 나중에 퍽을 얻었을 때 깨끗한 상태에서 시작한다.
	if (crit.Level <= 0)
	{
		return BaseDamage;
	}

	bOutCritical = critPrd.Roll();
	return bOutCritical ? BaseDamage * crit.GetMultiplier() : BaseDamage;
}

void UC_LevelUpPerkComponent::RestoreCritState(const FCritState& InCrit)
{
	crit = InCrit;

	// 저장된 건 레벨/계수뿐이라, 파생값인 PRD 상수는 여기서 다시 만들어야 한다.
	critPrd.SetTargetChance(crit.GetChance());
}

void UC_LevelUpPerkComponent::AddElementLevel(const FPerkData& Perk)
{
	// 해당 속성 상태를 찾거나 새로 만든다.
	FElementState& State = elements.FindOrAdd(Perk.elementTag);
	State.DamagePerLevel = Perk.elementDamagePerLevel;
	State.MaxLevel = Perk.elementMaxLevel;
	State.Color = Perk.elementColor;
	State.DisplayName = Perk.DisplayName;
	State.Icon = Perk.Icon;

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

TArray<FPerkDisplayInfo> UC_LevelUpPerkComponent::GetOwnedPerks() const
{
	TArray<FPerkDisplayInfo> Result;

	for (const TPair<FGameplayTag, FElementState>& Pair : elements)
	{
		FPerkDisplayInfo Info;
		Info.ElementTag = Pair.Key;
		Info.DisplayName = Pair.Value.DisplayName;
		Info.Level = Pair.Value.Level;
		Info.MaxLevel = Pair.Value.MaxLevel;
		Info.Color = Pair.Value.Color;
		Info.Icon = Pair.Value.Icon;
		Result.Add(Info);
	}
	return Result;
}

int32 UC_LevelUpPerkComponent::GetElementLevel(FGameplayTag ElementTag) const
{
	if(const FElementState* State = elements.Find(ElementTag))
	{
		return State->Level;
	}
	return 0;
}

void UC_LevelUpPerkComponent::BroadcastCurrentState()
{
	const FElementState* Primary = elements.Find(primaryElement);
	OnPrimaryElementChanged.Broadcast(primaryElement, Primary ? Primary->Color : FLinearColor::Black);
}

void UC_LevelUpPerkComponent::RestoreElementState(const TMap<FGameplayTag, FElementState>& InElements)
{
	elements = InElements;

	// primary 재계산 + ASC 태그 복원
	int32 BestLevel = -1;
	primaryElement = FGameplayTag();

	AC_PlayerState* PS = Cast<AC_PlayerState>(GetOwner());
	UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;

	for (const TPair<FGameplayTag, FElementState>& Pair : elements)
	{
		if (Pair.Value.Level > BestLevel)
		{
			BestLevel = Pair.Value.Level;
			primaryElement = Pair.Key;
		}
		if (ASC)
		{
			ASC->AddLooseGameplayTag(Pair.Key); // 태그 복원(오러 등 참고용)
		}
	}
}
