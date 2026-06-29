// Fill out your copyright notice in the Description page of Project Settings.

#include "C_SkillManagerComponent.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "C_SkillBase.h"
#include "../PlayerCharacter/C_BasePlayerCharactor.h"
#include "../PlayerCharacter/C_PlayerState.h"
#include "../PlayerCharacter/PlayerAI/C_PlayerController.h"

UC_SkillManagerComponent::UC_SkillManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	activeSkillIndex = 0;
	bIsSkillWheelOpen = false;
}

UAbilitySystemComponent* UC_SkillManagerComponent::GetASC() const
{
	AC_BasePlayerCharactor* Character = Cast<AC_BasePlayerCharactor>(GetOwner());
	if (!Character) return nullptr;

	AC_PlayerState* PS = Character->GetPlayerState<AC_PlayerState>();
	if (!PS) return nullptr;

	return PS->GetAbilitySystemComponent();
}

void UC_SkillManagerComponent::InitializeDefaultSkill()
{
	activeSkillIndex = 0;
}

void UC_SkillManagerComponent::SwitchCommonSkill(int32 NewIndex)
{
	if (!registeredCommonSkills.IsValidIndex(NewIndex)) return;
	activeSkillIndex = NewIndex;

	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC) return;

	TSubclassOf<UC_SkillBase> SkillClass = registeredCommonSkills[NewIndex];
	if (!SkillClass) return;

	UC_SkillBase* FoundAbility = nullptr;
	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.Ability || !Spec.Ability->IsA(SkillClass)) continue;

		UGameplayAbility* Instance = Spec.GetPrimaryInstance();
		FoundAbility = Instance ? Cast<UC_SkillBase>(Instance) : Cast<UC_SkillBase>(Spec.Ability);
		break;
	}

	if (FoundAbility)
	{
		OnCommonSkillSwitched.Broadcast(FoundAbility);
	}
}

void UC_SkillManagerComponent::OpenSkillWheel()
{
	if (bIsSkillWheelOpen) return;

	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC) return;

	// State.SkillWheelOpen 태그 추가 → GA_StoneSpear의 CheckCancelInput 폴링 루프가
	// 다음 틱(~50ms)에 이 태그를 감지하고 스스로 cleanup → EndAbility 처리
	ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.SkillWheelOpen")));
	bIsSkillWheelOpen = true;

	// 커서 ON + 카메라 회전 차단 + GameAndUI 입력 모드 — 인벤토리와 동일하게 PlayerController가
	// 플래그로 관리하므로 다른 UI(인벤토리)와 커서 소유권이 겹쳐도 안전하게 동작
	if (AC_BasePlayerCharactor* Char = Cast<AC_BasePlayerCharactor>(GetOwner()))
	{
		if (AC_PlayerController* PC = Cast<AC_PlayerController>(Char->GetController()))
			PC->PushMouseUI(EMouseUISource::SkillWheel);
	}

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), skillWheelTimeDilation);
}

void UC_SkillManagerComponent::CloseSkillWheel()
{
	if (!bIsSkillWheelOpen) return;

	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC) return;

	ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.SkillWheelOpen")));
	bIsSkillWheelOpen = false;

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);

	if (AC_BasePlayerCharactor* Char = Cast<AC_BasePlayerCharactor>(GetOwner()))
	{
		// 스킬휠 커서 요청 해제 — 인벤토리 등 다른 UI가 남아 있으면 커서는 유지됨.
		// Z키 닫기·캐릭터 사망 양쪽 경로가 모두 이 함수를 거치므로 단일 진입점.
		if (AC_PlayerController* PC = Cast<AC_PlayerController>(Char->GetController()))
			PC->PopMouseUI(EMouseUISource::SkillWheel);

		Char->OnSkillWheelShouldClose();
	}
}

TSubclassOf<UC_SkillBase> UC_SkillManagerComponent::GetActiveSkillClass() const
{
	if (registeredCommonSkills.IsValidIndex(activeSkillIndex))
	{
		return registeredCommonSkills[activeSkillIndex];
	}
	return nullptr;
}

bool UC_SkillManagerComponent::GetSkillDataAtIndex(int32 Index, FSkillData& OutData)
{
	if (!registeredCommonSkills.IsValidIndex(Index) || !registeredCommonSkills[Index]) return false;

	// CDO에서 데이터 로드 (C_SkillIconWidget과 동일한 패턴, Debugging Checklist #10 참고)
	UC_SkillBase* CDO = Cast<UC_SkillBase>(registeredCommonSkills[Index]->GetDefaultObject());
	if (!CDO) return false;

	return CDO->GetSkillData(OutData);
}
