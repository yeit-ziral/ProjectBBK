// Fill out your copyright notice in the Description page of Project Settings.

#include "C_InventoryComponent.h"
#include "../Items/ItemData.h"
#include "../PlayerCharacter/C_PlayerState.h"
#include "Engine/DataTable.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"

UC_InventoryComponent::UC_InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	quickSlots.SetNum(NumQuickSlots);
}

void UC_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureSlots();   // 고정 크기 빈 슬롯으로 초기화
}

void UC_InventoryComponent::EnsureSlots()
{
	if (slots.Num() != maxSlots)
		slots.SetNum(maxSlots);   // 늘어나면 기본(빈) 슬롯으로 채워짐
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

bool UC_InventoryComponent::GetConsumableData(FName itemID, FConsumableItemData& OutData) const
{
	if (itemID.IsNone() || !consumableTable) return false;

	if (const FConsumableItemData* row = consumableTable->FindRow<FConsumableItemData>(itemID, TEXT("GetConsumableData"), false))
	{
		OutData = *row;
		return true;
	}
	return false;
}

int32 UC_InventoryComponent::AddItem(FName itemID, int32 count)
{
	if (count <= 0) return count;

	const int32 maxStack = GetMaxStack(itemID);
	if (maxStack <= 0) return count;   // 유효하지 않은 itemID — 아무것도 못 넣음

	EnsureSlots();

	int32 remaining = count;

	// 1) 같은 아이템의 기존 스택을 maxStack까지 먼저 채움 (항상 병합)
	for (FInventorySlot& slot : slots)
	{
		if (remaining <= 0) break;
		if (slot.itemID != itemID || slot.quantity >= maxStack) continue;

		const int32 space = maxStack - slot.quantity;
		const int32 add   = FMath::Min(space, remaining);
		slot.quantity += add;
		remaining     -= add;
	}

	// 2) 초과분은 앞에서부터 첫 빈칸(itemID=None)에 배치
	for (FInventorySlot& slot : slots)
	{
		if (remaining <= 0) break;
		if (!slot.itemID.IsNone()) continue;   // 이미 찬 칸은 건너뜀

		const int32 add = FMath::Min(maxStack, remaining);
		slot.itemID   = itemID;
		slot.quantity = add;
		remaining     -= add;
	}

	if (remaining != count)   // 하나라도 추가됐으면 변경 알림
		OnInventoryChanged.Broadcast();

	return remaining;   // 못 넣은 잔여 (0이면 전부 추가)
}

bool UC_InventoryComponent::RemoveItem(FName itemID, int32 count)
{
	if (count <= 0) return false;
	if (GetItemCount(itemID) < count) return false;   // 부족하면 아무것도 안 함

	EnsureSlots();

	int32 remaining = count;
	for (FInventorySlot& slot : slots)
	{
		if (remaining <= 0) break;
		if (slot.itemID != itemID) continue;

		const int32 take = FMath::Min(slot.quantity, remaining);
		slot.quantity -= take;
		remaining      -= take;

		if (slot.quantity <= 0)
			slot = FInventorySlot();   // 위치 유지하며 빈칸으로 (배열 압축 안 함)
	}

	OnInventoryChanged.Broadcast();
	return true;
}

bool UC_InventoryComponent::RemoveAtSlot(int32 slotIndex, int32 count)
{
	if (count <= 0) return false;
	EnsureSlots();
	if (!slots.IsValidIndex(slotIndex) || slots[slotIndex].itemID.IsNone()) return false;

	slots[slotIndex].quantity -= count;
	if (slots[slotIndex].quantity <= 0)
		slots[slotIndex] = FInventorySlot();   // 빈칸으로 (위치 유지)

	OnInventoryChanged.Broadcast();
	return true;
}

bool UC_InventoryComponent::MoveSlot(int32 FromIndex, int32 ToIndex)
{
	EnsureSlots();

	if (FromIndex == ToIndex) return false;
	if (!slots.IsValidIndex(FromIndex) || !slots.IsValidIndex(ToIndex)) return false;
	if (slots[FromIndex].itemID.IsNone()) return false;   // 옮길 아이템 없음

	FInventorySlot& from = slots[FromIndex];
	FInventorySlot& to   = slots[ToIndex];

	if (to.itemID.IsNone())
	{
		// 빈칸으로 이동
		to   = from;
		from = FInventorySlot();
	}
	else if (to.itemID == from.itemID && GetMaxStack(from.itemID) > 1)
	{
		// 같은 스택형 아이템 → 병합 (넘치면 잔량은 from에 유지)
		const int32 space = GetMaxStack(from.itemID) - to.quantity;
		if (space > 0)
		{
			const int32 move = FMath::Min(space, from.quantity);
			to.quantity   += move;
			from.quantity -= move;
			if (from.quantity <= 0)
				from = FInventorySlot();
		}
		else
		{
			Swap(slots[FromIndex], slots[ToIndex]);   // 대상이 가득 → 교환
		}
	}
	else
	{
		// 다른 아이템 → 위치 교환
		Swap(slots[FromIndex], slots[ToIndex]);
	}

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

void UC_InventoryComponent::AddMoney(int32 amount)
{
	if (amount <= 0) return;

	money += amount;
	OnMoneyChanged.Broadcast(money);
}

bool UC_InventoryComponent::RemoveMoney(int32 amount)
{
	if (amount <= 0) return false;
	if (money < amount) return false;   // 부족하면 아무것도 안 함

	money -= amount;
	OnMoneyChanged.Broadcast(money);
	return true;
}

bool UC_InventoryComponent::UseItem(FName itemID)
{
	FConsumableItemData data;
	if (!GetConsumableData(itemID, data)) return false;      // 소비 아이템이 아님
	if (data.consumeEffects.IsEmpty()) return false;         // 효과 없는 아이템의 무의미한 소모 방지
	if (!HasItem(itemID, 1)) return false;                   // 재고 확인이 GE 적용보다 먼저

	// ASC는 반드시 PlayerState 경유로 취득 (CLAUDE.md GAS 규칙) — 인벤토리 소유자는 PlayerController
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	AC_PlayerState* PS = PC ? PC->GetPlayerState<AC_PlayerState>() : nullptr;
	UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
	if (!ASC) return false;

	FGameplayEffectContextHandle ctx = ASC->MakeEffectContext();
	ctx.AddSourceObject(GetOwner());

	for (const FConsumableEffectEntry& entry : data.consumeEffects)
	{
		if (!entry.effect) continue;

		FGameplayEffectSpecHandle spec = ASC->MakeOutgoingSpec(entry.effect, 1.f, ctx);
		if (!spec.IsValid()) continue;

		spec.Data->SetSetByCallerMagnitude(entry.magnitudeTag, entry.magnitude);
		ASC->ApplyGameplayEffectSpecToSelf(*spec.Data);
	}

	RemoveItem(itemID, 1);

	// 등록된 퀵슬롯 전부 갱신 (재고 감소분 반영, 0개 시 반투명 처리는 위젯 쪽 책임)
	for (int32 i = 0; i < quickSlots.Num(); ++i)
	{
		if (quickSlots[i] == itemID)
			OnQuickSlotChanged.Broadcast(i);
	}

	return true;
}

bool UC_InventoryComponent::RegisterQuickSlot(int32 SlotIndex, FName ItemID)
{
	if (!quickSlots.IsValidIndex(SlotIndex)) return false;

	FConsumableItemData data;
	if (!GetConsumableData(ItemID, data)) return false;   // 소비 아이템만 등록 허용

	quickSlots[SlotIndex] = ItemID;
	OnQuickSlotChanged.Broadcast(SlotIndex);
	return true;
}

bool UC_InventoryComponent::UnregisterQuickSlot(int32 SlotIndex)
{
	if (!quickSlots.IsValidIndex(SlotIndex) || quickSlots[SlotIndex].IsNone()) return false;

	quickSlots[SlotIndex] = NAME_None;
	OnQuickSlotChanged.Broadcast(SlotIndex);
	return true;
}

FName UC_InventoryComponent::GetQuickSlotItem(int32 SlotIndex) const
{
	return quickSlots.IsValidIndex(SlotIndex) ? quickSlots[SlotIndex] : NAME_None;
}

bool UC_InventoryComponent::UseQuickSlot(int32 SlotIndex)
{
	const FName itemID = GetQuickSlotItem(SlotIndex);
	if (itemID.IsNone()) return false;

	return UseItem(itemID);
}
