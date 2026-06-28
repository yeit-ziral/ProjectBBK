// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "C_PerkData.h"
#include "C_LevelUpPerkComponent.generated.h"


class UAbilitySystemComponent;

// 속성 하나의 현재 상태
USTRUCT()
struct FElementState
{
	GENERATED_BODY()

	int32 Level = 0;
	float DamagePerLevel = 0.f;
	int32 MaxLevel = 1;

	UPROPERTY()
	FLinearColor Color = FLinearColor::Black;
};

// 후보 보상 3개가 준비되면 UMG가 받아서 위젯을 띄우도록 알리는 델리게이트.
// C++는 "어떤 보상 후보가 있는지" 데이터만 넘기고, 위젯 생성/표시는 BP가 담당한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPerkChoicesReady, const TArray<FPerkData>&, Choices);

// primary(최고 레벨) 속성이 바뀌면 무기 이펙트를 갈아끼우라고 BP에 알림
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPrimaryElementChanged, FGameplayTag, ElementTag, FLinearColor, Color);

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

	// BP(UMG)가 바인딩 → 후보 3개를 받아 위젯을 띄운다.
	UPROPERTY(BlueprintAssignable, Category = "Perk")
	FOnPerkChoicesReady OnPerkChoicesReady;

	// primary 속성이 바뀌면 broadcast (BP가 무기 소켓에 이펙트 부착)
	UPROPERTY(BlueprintAssignable, Category = "Perk|Element")
	FOnPrimaryElementChanged OnPrimaryElementChanged;

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

	// 지금 화면에 띄워둔 후보들.
	UPROPERTY()
	TArray<FPerkData> PendingChoices;

	// 한 번에 여러 레벨이 오른 경우를 위한 큐(남은 선택 횟수).
	int32 PendingLevelUps = 0;

	// 보유 속성들  키 = 속성 태그(State.Element.Fire 등)
	UPROPERTY()
	TMap<FGameplayTag, FElementState> elements;

	FGameplayTag primaryElement;				 // 현재 최고 레벨 속성

	void AddElementLevel(const FPerkData& Perk); // 속성 레벨 +1, primary 속성 갱신
	float ComputeElementalTrueDamage() const;		 // 이번 타격의 속성 true 데미지 합
};
