// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../../../Items/ItemData.h"
#include "C_InventoryComponent.generated.h"

class UDataTable;

/**
 * 인벤토리 한 칸 — 아이템 ID + 수량.
 * 같은 아이템은 항상 병합되므로(한 스택을 maxStack까지 채운 뒤 새 칸),
 * 같은 itemID의 부분 스택이 여러 칸에 동시에 존재하지 않는다.
 */
USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FName itemID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 quantity = 0;

	FInventorySlot() {}
	FInventorySlot(FName InItemID, int32 InQuantity) : itemID(InItemID), quantity(InQuantity) {}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

/**
 * 인벤토리 코어 컴포넌트 (UI/화폐/장착/플레이어 통합 제외).
 * 독립 UActorComponent — 추후 플레이어 또는 PlayerState에 부착해 사용.
 * 아이템 정의·스택 한도는 DataTable(FConsumableItemData.maxStack 등) 기반. 하드코딩 금지.
 * 규칙: 같은 아이템은 항상 병합, 칸 순서는 획득 순서 유지. 장비는 비스택(칸당 1).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTBBK_API UC_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UC_InventoryComponent();

	// count개 추가 — 못 넣은 잔여 수량 반환(0이면 전부 추가). 유효하지 않은 itemID면 count 그대로 반환.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(FName itemID, int32 count = 1);

	// itemID를 count개 제거 — 보유량이 부족하면 아무것도 제거하지 않고 false.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FName itemID, int32 count = 1);

	// 특정 슬롯에서 count개 제거 — 수량 0이 되면 슬롯 삭제.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveAtSlot(int32 slotIndex, int32 count = 1);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(FName itemID) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(FName itemID, int32 count = 1) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventorySlot> GetSlots() const { return slots; }

	// itemID의 최대 스택 수 — 소비: FConsumableItemData.maxStack, 장비: 1, 미등록: 0(유효하지 않음).
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetMaxStack(FName itemID) const;

	// itemID로 표시용 공통 데이터(이름·아이콘·설명·Mesh) 조회 — UI에서 itemID만으로 아이콘/이름 표시용.
	// 소비/장비 DataTable을 순서대로 찾아 공통 베이스(FBaseItemData)로 반환. 미등록이면 false.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetItemData(FName itemID, FBaseItemData& OutData) const;

	// 런타임에 조회용 DataTable 지정 — EditDefaultsOnly 슬롯을 코드/소유자에서 주입할 때.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetItemTables(UDataTable* Consumable, UDataTable* Equipment);

	// 슬롯 변경 시 브로드캐스트 (후속 UI 바인딩용)
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

protected:
	// itemID 조회용 DataTable — 소유자/BP에서 지정 (DT_ConsumableItem / DT_EquipmentItem)
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	UDataTable* consumableTable = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	UDataTable* equipmentTable = nullptr;

	// 최대 슬롯(칸) 수
	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 maxSlots = 30;

	UPROPERTY()
	TArray<FInventorySlot> slots;

private:
	// 소비/장비 DT 어디에 있는지 확인 — 미등록이면 false
	bool IsValidItem(FName itemID) const;
};
