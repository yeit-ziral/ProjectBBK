// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "SkillData.h"
#include "C_SkillManagerComponent.generated.h"

class UC_SkillBase;
class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCommonSkillSwitched, UC_SkillBase*, NewSkillAbility);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTBBK_API UC_SkillManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UC_SkillManagerComponent();

	// Blueprint에서 CommonSkill GA 클래스 4개 설정 (index 순서 = WBP_SkillWheel 섹터 순서)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkillManager")
	TArray<TSubclassOf<UC_SkillBase>> registeredCommonSkills;

	// 현재 F키에 할당된 스킬의 인덱스
	UPROPERTY(BlueprintReadOnly, Category = "SkillManager")
	int32 activeSkillIndex;

	// SkillWheel 열림 여부 (Z키 토글 판단용)
	UPROPERTY(BlueprintReadOnly, Category = "SkillManager")
	bool bIsSkillWheelOpen;

	// AddCharacterAbilities 이후 호출 — index 0 스킬에 Ability.Skill.Common 태그 부여
	UFUNCTION(BlueprintCallable, Category = "SkillManager")
	void InitializeDefaultSkill();

	// SkillWheel에서 스킬 선택 시 호출 — DynamicAbilityTags로 Ability.Skill.Common 교체
	UFUNCTION(BlueprintCallable, Category = "SkillManager")
	void SwitchCommonSkill(int32 NewIndex);

	// SkillWheel 열기 — ASC에 State.SkillWheelOpen 태그 추가
	UFUNCTION(BlueprintCallable, Category = "SkillManager")
	void OpenSkillWheel();

	// SkillWheel 닫기 — ASC에서 State.SkillWheelOpen 태그 제거
	UFUNCTION(BlueprintCallable, Category = "SkillManager")
	void CloseSkillWheel();

	// 현재 F키에 할당된 GA 클래스 반환 — TryActivateAbilityByClass에서 사용
	UFUNCTION(BlueprintCallable, Category = "SkillManager")
	TSubclassOf<UC_SkillBase> GetActiveSkillClass() const;

	// WBP_SkillWheel 아이콘 로드용 — CDO에서 스킬 데이터 반환
	UFUNCTION(BlueprintCallable, Category = "SkillManager")
	bool GetSkillDataAtIndex(int32 Index, FSkillData& OutData);

	// SkillWheel 또는 캐릭터 교체 시 발생 — CommonSkill SkillIcon이 이 델리게이트로 갱신
	UPROPERTY(BlueprintAssignable, Category = "SkillManager")
	FOnCommonSkillSwitched OnCommonSkillSwitched;

private:
	UAbilitySystemComponent* GetASC() const;
};
