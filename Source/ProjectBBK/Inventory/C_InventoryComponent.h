// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Items/ItemData.h"
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoneyChanged, int32, NewAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuickSlotChanged, int32, SlotIndex);

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

	// 슬롯 간 이동(드래그&드롭) — From의 아이템을 To로. 빈칸이면 이동, 같은 아이템이면 병합, 그 외엔 교환.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveSlot(int32 FromIndex, int32 ToIndex);

	// itemID의 최대 스택 수 — 소비: FConsumableItemData.maxStack, 장비: 1, 미등록: 0(유효하지 않음).
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetMaxStack(FName itemID) const;

	// itemID로 표시용 공통 데이터(이름·아이콘·설명·Mesh) 조회 — UI에서 itemID만으로 아이콘/이름 표시용.
	// 소비/장비 DataTable을 순서대로 찾아 공통 베이스(FBaseItemData)로 반환. 미등록이면 false.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetItemData(FName itemID, FBaseItemData& OutData) const;

	// itemID로 소비 아이템 전용 데이터(consumeEffects 포함) 전체 조회 — 툴팁 등 효과 표시용.
	// GetItemData는 FBaseItemData로 슬라이싱돼 효과가 잘리므로 전체가 필요하면 이쪽 사용. 소비 DT에 없으면 false.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetConsumableData(FName itemID, FConsumableItemData& OutData) const;

	// 소비 아이템 사용 — consumeEffects를 순회해 ASC(PlayerState 경유)에 GE 적용 후 1개 소모.
	// 미보유·효과 없음·ASC 취득 실패 시 false (GE 미적용, 스택도 유지).
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseItem(FName itemID);

	// ===== 퀵슬롯 (F1/F2 등으로 즉시 사용할 소비 아이템 슬롯, 총 2개) =====

	// SlotIndex(0~1)에 소비 아이템을 등록 — 인벤토리에서 제거하지 않고 itemID 참조만 저장. 소비 아이템이 아니면 false.
	UFUNCTION(BlueprintCallable, Category = "Inventory|QuickSlot")
	bool RegisterQuickSlot(int32 SlotIndex, FName ItemID);

	// 등록 해제 (재고 0이어도 자동 해제되지 않으므로 별도 호출 필요 시 사용)
	UFUNCTION(BlueprintCallable, Category = "Inventory|QuickSlot")
	bool UnregisterQuickSlot(int32 SlotIndex);

	// SlotIndex에 등록된 itemID (미등록이면 None)
	UFUNCTION(BlueprintPure, Category = "Inventory|QuickSlot")
	FName GetQuickSlotItem(int32 SlotIndex) const;

	// SlotIndex에 등록된 아이템을 사용 (UseItem 호출)
	UFUNCTION(BlueprintCallable, Category = "Inventory|QuickSlot")
	bool UseQuickSlot(int32 SlotIndex);

	// 퀵슬롯 등록/해제/사용(재고 변화) 시 브로드캐스트 — WBP_UseItem 갱신용
	UPROPERTY(BlueprintAssignable, Category = "Inventory|QuickSlot")
	FOnQuickSlotChanged OnQuickSlotChanged;

	// 등록된 아이템의 재고가 0인 상태에서 사용 시도 시 브로드캐스트 — WBP_UseItem 알림 사운드용. 미등록 슬롯은 브로드캐스트하지 않음.
	UPROPERTY(BlueprintAssignable, Category = "Inventory|QuickSlot")
	FOnQuickSlotChanged OnQuickSlotUseFailed;

	// itemID가 현재 쿨다운 중인지 여부 (GAS/GE 미사용, 타임스탬프 기반)
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsItemOnCooldown(FName itemID) const;

	// itemID의 남은 쿨다운(초). 쿨다운 없으면 0.
	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetItemCooldownRemaining(FName itemID) const;

	// 런타임에 조회용 DataTable 지정 — EditDefaultsOnly 슬롯을 코드/소유자에서 주입할 때.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetItemTables(UDataTable* Consumable, UDataTable* Equipment);

	// 슬롯 변경 시 브로드캐스트 (후속 UI 바인딩용)
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	// ===== 돈(골드) =====

	// 보유 금액 조회
	UFUNCTION(BlueprintPure, Category = "Inventory|Money")
	int32 GetMoney() const { return money; }

	// 금액 추가 (amount > 0)
	UFUNCTION(BlueprintCallable, Category = "Inventory|Money")
	void AddMoney(int32 amount);

	// 금액 차감 — 보유액이 부족하면 아무것도 안 하고 false
	UFUNCTION(BlueprintCallable, Category = "Inventory|Money")
	bool RemoveMoney(int32 amount);

	// 보유 금액 변경 시 브로드캐스트 (UI 바인딩용, 변경 후 금액 전달)
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Money")
	FOnMoneyChanged OnMoneyChanged;

protected:
	// itemID 조회용 DataTable — 소유자/BP에서 지정 (DT_ConsumableItem / DT_EquipmentItem)
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	UDataTable* consumableTable = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	UDataTable* equipmentTable = nullptr;

	// 최대 슬롯(칸) 수
	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 maxSlots = 32;

	// 보유 금액 (시작값 — EditDefaultsOnly로 초기 금액 지정 가능)
	UPROPERTY(EditDefaultsOnly, Category = "Inventory|Money", meta = (ClampMin = "0"))
	int32 money = 0;

	// 고정 크기(maxSlots) 슬롯 배열 — 빈칸은 itemID=None. 인덱스 = 격자 위치(자유 배치).
	UPROPERTY()
	TArray<FInventorySlot> slots;

	virtual void BeginPlay() override;

private:
	// 소비/장비 DT 어디에 있는지 확인 — 미등록이면 false
	bool IsValidItem(FName itemID) const;

	// slots 배열을 maxSlots 고정 크기로 보장 (빈칸 포함). 모든 변경 함수 진입 시 호출.
	void EnsureSlots();

	// 퀵슬롯 개수 (요구사항상 고정 2개)
	static constexpr int32 NumQuickSlots = 2;

	// 퀵슬롯별 등록된 itemID (참조만 — 인벤토리 slots와 무관, 생성자에서 NumQuickSlots 크기로 고정)
	UPROPERTY()
	TArray<FName> quickSlots;

	// 아이템별 쿨다운 종료 시각(GetWorld()->GetTimeSeconds() 기준) — 쿨다운 없는 아이템은 항목 없음
	UPROPERTY()
	TMap<FName, float> itemCooldownEndTime;
};
