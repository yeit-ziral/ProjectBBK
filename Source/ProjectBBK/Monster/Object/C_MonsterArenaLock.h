// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_MonsterArenaLock.generated.h"

class AC_BaseMonster;
class AC_BaseItem;
class UC_MonsterASC;
class UBillboardComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArenaCleared, AC_MonsterArenaLock*, Arena);

// 공터(아레나) 단위 잠금 — 지정한 몬스터가 전부 죽기 전까지 보상 상자(AC_BaseItem)와 상호작용할 수 없게 막는다.
// 상자 쪽 코드는 건드리지 않고, 상자의 콜리전을 꺼서 오버랩(= 상호작용 UI/입력 등록) 자체를 막는 방식.
// 레벨에 배치한 뒤 인스턴스 디테일에서 guardMonsters / lockedChests를 지정한다.
UCLASS()
class PROJECTBBK_API AC_MonsterArenaLock : public AActor
{
	GENERATED_BODY()

public:
	AC_MonsterArenaLock();

	UFUNCTION(BlueprintPure, Category = "Arena")
	bool IsCleared() const { return bCleared; }

	UFUNCTION(BlueprintPure, Category = "Arena")
	int32 GetRemainingMonsterCount() const { return remainingMonsterCount; }

	// 공터 몬스터 전멸 시 1회 브로드캐스트 (연출 연결용)
	UPROPERTY(BlueprintAssignable, Category = "Arena")
	FOnArenaCleared OnArenaCleared;

protected:
	virtual void BeginPlay() override;

	// 이 공터를 지키는 몬스터 — 전부 죽어야 잠금 해제
	UPROPERTY(EditInstanceOnly, Category = "Arena")
	TArray<TObjectPtr<AC_BaseMonster>> guardMonsters;

	// 잠글 상자(들) — AC_TreasureChest 등 AC_BaseItem 파생
	UPROPERTY(EditInstanceOnly, Category = "Arena")
	TArray<TObjectPtr<AC_BaseItem>> lockedChests;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TObjectPtr<UBillboardComponent> editorIcon;

private:
	void HandleMonsterDeath(UC_MonsterASC* DeadASC);
	void Unlock();

	// 사망 판정은 ASC 기준 — 같은 몬스터의 중복 브로드캐스트 방지
	TSet<TWeakObjectPtr<UC_MonsterASC>> aliveASCs;

	int32 remainingMonsterCount = 0;
	bool bCleared = false;
};
