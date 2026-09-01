// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "C_PerkData.h"
#include "ProjectBBK/Core/C_Probability.h"
#include "C_LevelUpPerkComponent.generated.h"


class UAbilitySystemComponent;

// 후보 보상 3개가 준비되면 UMG가 받아서 위젯을 띄우도록 알리는 델리게이트.
// C++는 "어떤 보상 후보가 있는지" 데이터만 넘기고, 위젯 생성/표시는 BP가 담당한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPerkChoicesReady, const TArray<FPerkData>&, Choices);

// primary(최고 레벨) 속성이 바뀌면 무기 이펙트를 갈아끼우라고 BP에 알림
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPrimaryElementChanged, FGameplayTag, ElementTag, FLinearColor, Color);

// 선택이 모두 끝나(남은 레벨업 0) 창을 닫아도 될 때 알림
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPerkSelectionFinished);

/**
 * 레벨업 특전(선택 보상) 로직 담당 컴포넌트. AC_PlayerState에 붙인다.
 * - 레벨업 감지(HandleLevelUp) → 후보 3개 추출 → 델리게이트 broadcast
 * - 플레이어 선택(SelectPerk) → 해당 GE를 ASC에 적용
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTBBK_API UC_LevelUpPerkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// PlayerState의 CharacterLevelChanged에서 호출. (NewLevel > OldLevel 일 때만 동작)
	void HandleLevelUp(int32 NewLevel, int32 OldLevel);

	// UMG 버튼이 호출. 현재 띄워둔 후보(PendingChoices) 중 Index 보상을 적용한다.
	UFUNCTION(BlueprintCallable, Category = "Perk")
	void SelectPerk(int32 Index);

	// 평타 명중 시 투사체가 호출 -> 속성 true 데미지를 타겟에 적용
	UFUNCTION(BlueprintCallable, Category = "Perk|Element")
	void ApplyElementalDamage(AActor* TargetActor);

	// 디버그용. RowName에 해당하는 perk를 바로 적용한다.
	UFUNCTION(BlueprintCallable, Category = "Perk|Debug")
	void DebugSelectPerk(FName RowName);

	//캐릭터 바꿀 때 레벨 변화가 퍽 띄우지 않게 잠깐 끔
	UFUNCTION(BlueprintCallable, Category = "Perk")
	void SetIgnoreLevelChanges(bool bIgnore) { bIgnoreLevelChanges = bIgnore; }

	UFUNCTION(BlueprintCallable, Category = "Perk")
	TArray<FPerkDisplayInfo> GetOwnedPerks() const;

	//현재 해당 속성 레벨 조회(없으면 0)
	UFUNCTION(BlueprintCallable, Category = "Perk")
	int32 GetElementLevel(FGameplayTag ElementTag) const;

	// 종류를 가리지 않는 "이 퍽의 현재 레벨". UI는 이것만 부르면 되고
	// 속성/크리 분기를 알 필요가 없다. 레벨 개념이 없는 스탯 퍽은 0.
	UFUNCTION(BlueprintPure, Category = "Perk")
	int32 GetPerkLevel(const FPerkData& Perk) const;

	// 이 퍽의 최대 레벨. 스탯 퍽은 0을 돌려주며 "상한 없음"을 뜻한다.
	UFUNCTION(BlueprintPure, Category = "Perk")
	int32 GetPerkMaxLevel(const FPerkData& Perk) const;
	// Txt Damage에 넣을 "이 퍽을 찍으면 어떻게 되는지" 한 줄.
	// 퍽 종류마다 보여줄 수치가 달라서, 그 분기를 여기서 흡수한다.
	// BP는 결과를 SET Text에 그대로 꽂기만 하면 된다.
	UFUNCTION(BlueprintPure, Category = "Perk")
	FText GetPerkEffectText(const FPerkData& Perk) const;

	// 데미지를 확정하기 직전에 호출한다. 크리티컬이 터지면 배율을 곱해서 돌려준다.
	// 미보유(Lv.0)면 BaseDamage를 그대로 돌려주므로 호출부에 분기가 필요 없다.
	// 주의: 한 번의 타격당 정확히 한 번만 호출할 것. 두 번 부르면 PRD 카운터가 두 번 돈다.
	UFUNCTION(BlueprintCallable, Category = "Perk|Crit")
	float RollCriticalDamage(float BaseDamage, bool& bOutCritical);

	// UI 표시용 (현재 레벨 / 발동률 / 배율)
	UFUNCTION(BlueprintPure, Category = "Perk|Crit")
	int32 GetCritLevel() const { return crit.Level; }

	UFUNCTION(BlueprintPure, Category = "Perk|Crit")
	float GetCritChance() const { return crit.GetChance(); }

	UFUNCTION(BlueprintPure, Category = "Perk|Crit")
	float GetCritMultiplier() const { return crit.GetMultiplier(); }

	UFUNCTION(BlueprintCallable, Category = "Perk")
	void BroadcastCurrentState();

	// 맵 이동 간 저장/ 복원용
	const TMap<FGameplayTag, FElementState>& GetElementState() const { return elements; }
	void RestoreElementState(const TMap<FGameplayTag, FElementState>& InElements);

	const FCritState& GetCritState() const { return crit; }
	void RestoreCritState(const FCritState& InCrit);

	// BP(UMG)가 바인딩 → 후보 3개를 받아 위젯을 띄운다.
	UPROPERTY(BlueprintAssignable, Category = "Perk")
	FOnPerkChoicesReady OnPerkChoicesReady;

	// primary 속성이 바뀌면 broadcast (BP가 무기 소켓에 이펙트 부착)
	UPROPERTY(BlueprintAssignable, Category = "Perk|Element")
	FOnPrimaryElementChanged OnPrimaryElementChanged;

	UPROPERTY(BlueprintAssignable, Category = "Perk")
	FOnPerkSelectionFinished OnPerkSelectionFinished;

protected:
	// FPerkData 행들이 담긴 DataTable. PlayerState BP 디폴트에서 DT_Perks 지정.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk")
	TObjectPtr<UDataTable> PerkTable;

	// 한 번에 보여줄 후보 개수.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk")
	int32 ChoiceCount = 3;

	// 속성 데미지를 넣을 전용 GE (ReceivedTrueDamage 대상, SetByCaller Data.Damage)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk|Element")
	TSubclassOf<UGameplayEffect> elementalDamageEffect;

private:
	// 다음 후보 세트를 뽑아 broadcast.
	void PresentNextChoices();

	// 이 퍽을 아직 후보로 낼 수 있는가. (속성 퍽이 만렙이면 false)
	bool IsPerkAvailable(const FPerkData& Perk) const;

	// 지금 화면에 띄워둔 후보들.
	UPROPERTY()
	TArray<FPerkData> PendingChoices;

	// 한 번에 여러 레벨이 오른 경우를 위한 큐(남은 선택 횟수).
	int32 PendingLevelUps = 0;

	// 보유 속성들  키 = 속성 태그(State.Element.Fire 등)
	UPROPERTY()
	TMap<FGameplayTag, FElementState> elements;

	FGameplayTag primaryElement;				 // 현재 최고 레벨 속성

	// 크리티컬 상태. 레벨/계수는 저장 대상이고, PRD 카운터는 세션 한정이다.
	UPROPERTY()
	FCritState crit;

	// 연속 실패 보정을 들고 있는 판정 채널. 맵을 넘어가면 리셋돼도 무방해서 저장 안 한다.
	FPrdChannel critPrd;

	void AddElementLevel(const FPerkData& Perk); // 속성 레벨 +1, primary 속성 갱신
	void AddCritLevel(const FPerkData& Perk);    // 크리 레벨 +1, PRD 상수 재역산
	float ComputeElementalTrueDamage() const;		 // 이번 타격의 속성 true 데미지 합

	// 선택 1회 처리를 마무리한다(큐 감소 -> 다음 세트 or 종료 알림).
	// 세 갈래(스탯/속성/크리)가 똑같은 마무리를 해야 해서 한 곳으로 모았다.
	void FinishSelection();

	bool bIgnoreLevelChanges = false; // 캐릭터 바꿀 때 레벨 변화가 퍽 띄우지 않게 잠깐 끔
};
