// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "C_PerkData.h"
#include "C_LevelUpPerkComponent.generated.h"

// 후보 보상 3개가 준비되면 UMG가 받아서 위젯을 띄우도록 알리는 델리게이트.
// C++는 "어떤 보상 후보가 있는지" 데이터만 넘기고, 위젯 생성/표시는 BP가 담당한다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPerkChoicesReady, const TArray<FPerkData>&, Choices);

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

	// BP(UMG)가 바인딩 → 후보 3개를 받아 위젯을 띄운다.
	UPROPERTY(BlueprintAssignable, Category = "Perk")
	FOnPerkChoicesReady OnPerkChoicesReady;

protected:
	// FPerkData 행들이 담긴 DataTable. PlayerState BP 디폴트에서 DT_Perks 지정.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk")
	TObjectPtr<UDataTable> PerkTable;

	// 한 번에 보여줄 후보 개수.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk")
	int32 ChoiceCount = 3;

private:
	// 다음 후보 세트를 뽑아 broadcast.
	void PresentNextChoices();

	// 지금 화면에 띄워둔 후보들.
	UPROPERTY()
	TArray<FPerkData> PendingChoices;

	// 한 번에 여러 레벨이 오른 경우를 위한 큐(남은 선택 횟수).
	int32 PendingLevelUps = 0;
};
