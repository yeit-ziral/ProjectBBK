// Fill out your copyright notice in the Description page of Project Settings.

#include "C_InventoryComponent.h"
#include "../../../Items/ItemData.h"
#include "Engine/DataTable.h"

UC_InventoryComponent::UC_InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UC_InventoryComponent::GetMaxStack(FName itemID) const
{
	if (itemID.IsNone()) return 0;

	if (consumableTable)
	{
		if (const FConsumableItemData* row = consumableTable->FindRow<FConsumableItemData>(itemID, TEXT("GetMaxStack"), false))
			return FMath::Max(1, row->maxStack);
	}
	if (equipmentTable)
	{
		if (equipmentTable->FindRow<FEquipmentItemData>(itemID, TEXT("GetMaxStack"), false))
			return 1;   // 장비는 비스택
	}
	return 0;   // 미등록 아이템
}

bool UC_InventoryComponent::IsValidItem(FName itemID) const
{
	return GetMaxStack(itemID) > 0;
}

void UC_InventoryComponent::SetItemTables(UDataTable* Consumable, UDataTable* Equipment)
{
	consumableTable = Consumable;
	equipmentTable  = Equipment;
}

bool UC_InventoryComponent::GetItemData(FName itemID, FBaseItemData& OutData) const
{
	if (itemID.IsNone()) return false;

	// 소비/장비 행 구조 모두 FBaseItemData를 상속 → 베이스 뷰로 공통 필드 읽기
	if (consumableTable)
	{
		if (const FBaseItemData* row = consumableTable->FindRow<FBaseItemData>(itemID, TEXT("GetItemData"), false))
		{
			OutData = *row;
			return true;
		}
	}
	if (equipmentTable)
	{
		if (const FBaseItemData* row = equipmentTable->FindRow<FBaseItemData>(itemID, TEXT("GetItemData"), false))
		{
			OutData = *row;
			return true;
		}
	}
	return false;
}

int32 UC_InventoryComponent::AddItem(FName itemID, int32 count)
{
	if (count <= 0) return count;

	const int32 maxStack = GetMaxStack(itemID);
	if (maxStack <= 0) return count;   // 유효하지 않은 itemID — 아무것도 못 넣음

	int32 remaining = count;

	// 1) 같은 아이템의 기존 스택을 maxStack까지 먼저 채움 (항상 병합, 획득 순서 유지)
	for (FInventorySlot& slot : slots)
	{
		if (remaining <= 0) break;
		if (slot.itemID != itemID || slot.quantity >= maxStack) continue;

		const int32 space = maxStack - slot.quantity;
		const int32 add   = FMath::Min(space, remaining);
		slot.quantity += add;
		remaining     -= add;
	}

	// 2) 초과분은 뒤에 새 슬롯으로 (용량 한도 내에서)
	while (remaining > 0 && slots.Num() < maxSlots)
	{
		const int32 add = FMath::Min(maxStack, remaining);
		slots.Emplace(itemID, add);
		remaining -= add;
	}

	if (remaining != count)   // 하나라도 추가됐으면 변경 알림
		OnInventoryChanged.Broadcast();

	return remaining;   // 못 넣은 잔여 (0이면 전부 추가)
}

bool UC_InventoryComponent::RemoveItem(FName itemID, int32 count)
{
	if (count <= 0) return false;
	if (GetItemCount(itemID) < count) return false;   // 부족하면 아무것도 안 함

	int32 remaining = count;
	for (int32 i = 0; i < slots.Num() && remaining > 0; )
	{
		if (slots[i].itemID != itemID)
		{
			++i;
			continue;
		}

		const int32 take = FMath::Min(slots[i].quantity, remaining);
		slots[i].quantity -= take;
		remaining         -= take;

		if (slots[i].quantity <= 0)
			slots.RemoveAt(i);   // 빈 슬롯 제거 (다음 슬롯이 i로 당겨지므로 i 유지)
		else
			++i;
	}

	OnInventoryChanged.Broadcast();
	return true;
}

bool UC_InventoryComponent::RemoveAtSlot(int32 slotIndex, int32 count)
{
	if (count <= 0) return false;
	if (!slots.IsValidIndex(slotIndex)) return false;

	slots[slotIndex].quantity -= count;
	if (slots[slotIndex].quantity <= 0)
		slots.RemoveAt(slotIndex);

	OnInventoryChanged.Broadcast();
	return true;
}

int32 UC_InventoryComponent::GetItemCount(FName itemID) const
{
	int32 total = 0;
	for (const FInventorySlot& slot : slots)
	{
		if (slot.itemID == itemID)
			total += slot.quantity;
	}
	return total;
}

bool UC_InventoryComponent::HasItem(FName itemID, int32 count) const
{
	return GetItemCount(itemID) >= count;
}
