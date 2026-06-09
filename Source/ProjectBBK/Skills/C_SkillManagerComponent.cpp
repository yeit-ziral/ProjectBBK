// Fill out your copyright notice in the Description page of Project Settings.

#include "C_SkillManagerComponent.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "C_SkillBase.h"
#include "../PlayerCharacter/C_BasePlayerCharactor.h"
#include "../PlayerCharacter/C_PlayerState.h"

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
