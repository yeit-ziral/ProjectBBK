// Fill out your copyright notice in the Description page of Project Settings.

#include "C_EquipmentComponent.h"
#include "../Inventory/C_InventoryComponent.h"
#include "../Items/ItemData.h"
#include "../PlayerCharacter/C_BasePlayerCharactor.h"
#include "../PlayerCharacter/C_MeleeCharacter.h"
#include "../PlayerCharacter/C_RangeCharacter.h"
#include "../PlayerCharacter/PlayerAI/C_PlayerController.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Engine/DataTable.h"

UC_EquipmentComponent::UC_EquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UAbilitySystemComponent* UC_EquipmentComponent::GetASC() const
{
	// 소유 캐릭터의 ASC (IAbilitySystemInterface) — PlayerState 경유와 동일 결과
	if (AC_BasePlayerCharactor* Char = Cast<AC_BasePlayerCharactor>(GetOwner()))
		return Char->GetAbilitySystemComponent();
	return nullptr;
}

UC_InventoryComponent* UC_EquipmentComponent::GetInventory() const
{
	// 인벤토리는 PlayerController가 보유(캐릭터 교체와 무관하게 공유)
	if (AC_BasePlayerCharactor* Char = Cast<AC_BasePlayerCharactor>(GetOwner()))
	{
		if (AC_PlayerController* PC = Cast<AC_PlayerController>(Char->GetController()))
			return PC->GetInventory();
	}
	return nullptr;
}

bool UC_EquipmentComponent::GetItemData(FName itemID, FBaseItemData& OutData) const
{
	if (itemID.IsNone() || !equipmentTable) return false;

	if (const FBaseItemData* row = equipmentTable->FindRow<FBaseItemData>(itemID, TEXT("EquipGetItemData"), false))
	{
		OutData = *row;
		return true;
	}
	return false;
}

bool UC_EquipmentComponent::GetEquipmentData(FName itemID, FEquipmentItemData& OutData) const
{
	if (itemID.IsNone() || !equipmentTable) return false;

	if (const FEquipmentItemData* row = equipmentTable->FindRow<FEquipmentItemData>(itemID, TEXT("EquipGetEquipmentData"), false))
	{
		OutData = *row;
		return true;
	}
	return false;
}

FName UC_EquipmentComponent::GetEquippedItem(EEquipmentSlot slot) const
{
	if (const FEquippedEntry* entry = equipped.Find(slot))
		return entry->itemID;
	return NAME_None;
}

bool UC_EquipmentComponent::IsSlotEquipped(EEquipmentSlot slot) const
{
	return equipped.Contains(slot);
}

bool UC_EquipmentComponent::CanEquipClass(EEquipmentClass itemClass) const
{
	// 공용 장비는 근/원 관계없이 누구나 장착 가능
	if (itemClass == EEquipmentClass::Common)
		return true;

	const AActor* Owner = GetOwner();
	if (!Owner)
		return false;

	// 전용 장비는 소유 캐릭터 타입이 일치할 때만 장착 가능
	if (itemClass == EEquipmentClass::Melee)
		return Owner->IsA(AC_MeleeCharacter::StaticClass());
	if (itemClass == EEquipmentClass::Ranged)
		return Owner->IsA(AC_RangeCharacter::StaticClass());

	return true;
}

bool UC_EquipmentComponent::CanEquipItem(FName itemID) const
{
	if (itemID.IsNone() || !equipmentTable)
		return false;

	const FEquipmentItemData* row = equipmentTable->FindRow<FEquipmentItemData>(itemID, TEXT("CanEquipItem"), false);
	if (!row || row->equipSlot == EEquipmentSlot::None)
		return false;

	return CanEquipClass(row->equipClass);
}

EEquipmentSlot UC_EquipmentComponent::GetItemSlotType(FName itemID) const
{
	if (itemID.IsNone() || !equipmentTable)
		return EEquipmentSlot::None;

	if (const FEquipmentItemData* row = equipmentTable->FindRow<FEquipmentItemData>(itemID, TEXT("GetItemSlotType"), false))
		return row->equipSlot;
	return EEquipmentSlot::None;
}

FActiveGameplayEffectHandle UC_EquipmentComponent::ApplyEquipGE(const FEquipmentItemData& row) const
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !equipBonusEffect)
		return FActiveGameplayEffectHandle();

	// GE_EquipBonus의 SetByCaller 태그 (DefaultGameplayTags.ini 등록 = RequestGameplayTag 안전)
	static const FGameplayTag TagMaxHealth  = FGameplayTag::RequestGameplayTag(FName("Data.Equip.MaxHealth"));
	static const FGameplayTag TagMaxStamina = FGameplayTag::RequestGameplayTag(FName("Data.Equip.MaxStamina"));
	static const FGameplayTag TagMoveSpeed  = FGameplayTag::RequestGameplayTag(FName("Data.Equip.MoveSpeed"));
	static const FGameplayTag TagDefense    = FGameplayTag::RequestGameplayTag(FName("Data.Equip.Defense"));
	static const FGameplayTag TagDamage     = FGameplayTag::RequestGameplayTag(FName("Data.Equip.Damage"));

	FGameplayEffectContextHandle ctx = ASC->MakeEffectContext();
	ctx.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle spec = ASC->MakeOutgoingSpec(equipBonusEffect, 1.f, ctx);
	if (!spec.IsValid())
		return FActiveGameplayEffectHandle();

	spec.Data->SetSetByCallerMagnitude(TagMaxHealth,  row.bonusMaxHealth);
	spec.Data->SetSetByCallerMagnitude(TagMaxStamina, row.bonusMaxStamina);
	spec.Data->SetSetByCallerMagnitude(TagMoveSpeed,  row.bonusMoveSpeed);
	spec.Data->SetSetByCallerMagnitude(TagDefense,    row.bonusDefense);
	spec.Data->SetSetByCallerMagnitude(TagDamage,     row.bonusDamage);

	return ASC->ApplyGameplayEffectSpecToSelf(*spec.Data);
}

bool UC_EquipmentComponent::EquipItem(FName itemID)
{
	if (itemID.IsNone() || !equipmentTable)
		return false;

	const FEquipmentItemData* row = equipmentTable->FindRow<FEquipmentItemData>(itemID, TEXT("EquipItem"), false);
	if (!row)
		return false;

	const EEquipmentSlot slot = row->equipSlot;
	if (slot == EEquipmentSlot::None)
		return false;

	// 근/원 전용 장비는 캐릭터 타입이 맞지 않으면 장착 불가 (근접이 원거리 전용 장착 시도 등)
	if (!CanEquipClass(row->equipClass))
		return false;

	UC_InventoryComponent* inv = GetInventory();
	if (!inv || !inv->HasItem(itemID, 1))
		return false;

	// 같은 슬롯에 이미 장착된 게 있으면 먼저 해제(가방으로 복귀) → 교체
	if (equipped.Contains(slot))
		UnequipItem(slot);

	// 가방에서 1개 제거 (실패 시 중단)
	if (!inv->RemoveItem(itemID, 1))
		return false;

	// ApplyEquipGE가 어트리뷰트 변경 델리게이트를 동기적으로 발화시키므로,
	// GetTotalEquipBonuses()가 이 아이템을 포함해서 계산하도록 GE 적용보다 먼저 맵에 등록
	FEquippedEntry entry;
	entry.itemID = itemID;
	equipped.Add(slot, entry);

	equipped[slot].bonusHandle = ApplyEquipGE(*row);

	OnEquipmentChanged.Broadcast();
	return true;
}

void UC_EquipmentComponent::SuspendEquipBonuses()
{
	UAbilitySystemComponent* ASC = GetASC();
	if (!ASC)
		return;

	for (TPair<EEquipmentSlot, FEquippedEntry>& Pair : equipped)
	{
		if (Pair.Value.bonusHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Pair.Value.bonusHandle);
			Pair.Value.bonusHandle = FActiveGameplayEffectHandle();
		}
	}
}

void UC_EquipmentComponent::ReapplyEquipBonuses()
{
	if (!equipmentTable)
		return;

	for (TPair<EEquipmentSlot, FEquippedEntry>& Pair : equipped)
	{
		if (const FEquipmentItemData* row = equipmentTable->FindRow<FEquipmentItemData>(Pair.Value.itemID, TEXT("ReapplyEquipBonuses"), false))
			Pair.Value.bonusHandle = ApplyEquipGE(*row);
	}
}

FEquipBonusTotals UC_EquipmentComponent::GetTotalEquipBonuses() const
{
	FEquipBonusTotals totals;
	if (!equipmentTable)
		return totals;

	for (const TPair<EEquipmentSlot, FEquippedEntry>& Pair : equipped)
	{
		if (const FEquipmentItemData* row = equipmentTable->FindRow<FEquipmentItemData>(Pair.Value.itemID, TEXT("GetTotalEquipBonuses"), false))
		{
			totals.maxHealth  += row->bonusMaxHealth;
			totals.maxStamina += row->bonusMaxStamina;
			totals.moveSpeed  += row->bonusMoveSpeed;
			totals.defense    += row->bonusDefense;
			totals.damage     += row->bonusDamage;
		}
	}

	return totals;
}

bool UC_EquipmentComponent::UnequipItem(EEquipmentSlot slot)
{
	const FEquippedEntry* found = equipped.Find(slot);
	if (!found)
		return false;

	// 맵에서 제거하기 전에 값 복사 (Remove 후 포인터 무효)
	const FName itemID = found->itemID;
	const FActiveGameplayEffectHandle handle = found->bonusHandle;

	// RemoveActiveGameplayEffect가 어트리뷰트 변경 델리게이트를 동기적으로 발화시키므로,
	// GetTotalEquipBonuses()가 이 아이템을 제외하고 계산하도록 GE 제거보다 먼저 맵에서 제거
	equipped.Remove(slot);

	// 보너스 GE 제거
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		if (handle.IsValid())
			ASC->RemoveActiveGameplayEffect(handle);
	}

	// 아이템을 가방으로 복귀
	if (UC_InventoryComponent* inv = GetInventory())
		inv->AddItem(itemID, 1);

	OnEquipmentChanged.Broadcast();
	return true;
}
