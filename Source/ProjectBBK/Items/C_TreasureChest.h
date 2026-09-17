#pragma once

#include "CoreMinimal.h"
#include "C_BaseItem.h"
#include "C_TreasureChest.generated.h"

class UDataTable;
class AC_ConsumableItem;
class AC_EquipmentItem;
class AC_MoneyItem;
class USoundBase;

UCLASS()
class PROJECTBBK_API AC_TreasureChest : public AC_BaseItem
{
	GENERATED_BODY()

public:
	AC_TreasureChest();

	virtual void OnInteract(AC_BasePlayerCharactor* Player) override;

protected:
	// 상자에 배정된 총 value 예산. 월드 배치 후 값 수정 가능.
	UPROPERTY(EditAnywhere, Category = "Treasure")
	int32 chestValue = 0;

	// true면 장비 아이템도 스폰 대상에 포함되고, 최소 1개가 보장됨. 상자 종류(BP)별로 고정.
	UPROPERTY(EditDefaultsOnly, Category = "Treasure")
	bool bIncludesEquipment = false;

	UPROPERTY(EditDefaultsOnly, Category = "Treasure|Data")
	UDataTable* consumableDataTable = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Treasure|Data")
	UDataTable* equipmentDataTable = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Treasure|Spawn")
	TSubclassOf<AC_ConsumableItem> consumableItemClass;

	UPROPERTY(EditDefaultsOnly, Category = "Treasure|Spawn")
	TSubclassOf<AC_EquipmentItem> equipmentItemClass;

	UPROPERTY(EditDefaultsOnly, Category = "Treasure|Spawn")
	TSubclassOf<AC_MoneyItem> moneyItemClass;

	// 상자를 열 때(상호작용 성공 시) 재생할 사운드. None이면 재생 안 함.
	UPROPERTY(EditDefaultsOnly, Category = "Treasure|Sound")
	USoundBase* openSound = nullptr;

	// 아이템/돈이 흩뿌려질 상자 주변 반경
	UPROPERTY(EditDefaultsOnly, Category = "Treasure|Spawn")
	float lootScatterRadius = 150.f;

	// 돈 몫 비율 범위(%). chestValue 전체 기준으로 랜덤 배정.
	UPROPERTY(EditDefaultsOnly, Category = "Treasure|Money", meta = (ClampMin = "0", ClampMax = "100"))
	int32 moneyPercentMin = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Treasure|Money", meta = (ClampMin = "0", ClampMax = "100"))
	int32 moneyPercentMax = 50;

	// 돈 value 1당 골드 환산량, 및 최종 골드의 랜덤 오차 범위
	UPROPERTY(EditDefaultsOnly, Category = "Treasure|Money")
	int32 goldPerMoneyValue = 10;

	UPROPERTY(EditDefaultsOnly, Category = "Treasure|Money")
	int32 goldVariance = 5;

private:
	struct FTreasureLootRow
	{
		FName itemID = NAME_None;
		int32 value = 0;
		bool bIsEquipment = false;

		FTreasureLootRow() {}
		FTreasureLootRow(FName InItemID, int32 InValue, bool bInIsEquipment)
			: itemID(InItemID), value(InValue), bIsEquipment(bInIsEquipment) {}
	};

	void RollAndSpawnLoot();

	// 장비 풀 중 value가 가장 낮은 행을 고른다(동률 시 랜덤). 예산 초과 리스크를 최소화하기 위해
	// 전체 풀에서 완전 랜덤으로 뽑지 않는다.
	bool PickCheapestEquipment(FTreasureLootRow& OutRow) const;

	// 반복 랜덤 추첨용 풀. Consumable은 항상 포함, Equipment는 bIncludesEquipment일 때만 포함.
	void CollectLootPool(TArray<FTreasureLootRow>& OutPool) const;

	FVector FindGroundSpawnPoint(const FVector& Center, float Radius) const;

	void SpawnConsumable(FName InItemID, const FVector& Location);
	void SpawnEquipment(FName InItemID, const FVector& Location);
	void SpawnMoney(int32 InGoldAmount, const FVector& Location);
};
