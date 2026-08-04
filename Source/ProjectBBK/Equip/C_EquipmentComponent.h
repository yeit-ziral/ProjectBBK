// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"   // FActiveGameplayEffectHandle
#include "../Items/ItemData.h"     // EEquipmentSlot, FEquipmentItemData, FBaseItemData
#include "C_EquipmentComponent.generated.h"

class UAbilitySystemComponent;
class UC_InventoryComponent;
class UDataTable;
class UGameplayEffect;

/**
 * 슬롯에 장착된 한 항목 — itemID + 적용된 GE 핸들(해제 시 RemoveActiveGameplayEffect용).
 * UObject 포인터가 없어 GC 대상 아님.
 */
struct FEquippedEntry
{
	FName itemID = NAME_None;
	FActiveGameplayEffectHandle bonusHandle;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentChanged);

/**
 * 장비 착용/해제 핸들러 (캐릭터별). AC_BasePlayerCharactor에 부착.
 * - 인벤토리(공유, PlayerController 보유)에서 아이템을 빼/넣고,
 *   GE_EquipBonus(SetByCaller 5종)를 현재 캐릭터 ASC에 적용/제거한다.
 * - ASC는 PlayerState 소유로 캐릭터 로스터 전체가 공유하므로, 장착 GE를 그대로 두면
 *   캐릭터를 교체해도 계속 적용된 채로 남는다(= 장비 없는 캐릭터도 보너스를 받음).
 *   AC_PlayerController::ExecuteCharacterSwitch가 SuspendEquipBonuses/ReapplyEquipBonuses를
 *   호출해 "지금 활성화된 캐릭터의 장비만" 보너스가 걸리도록 보장한다.
 * 규칙: GE 클래스/DataTable은 BP에서 주입(하드코딩 금지). 슬롯당 1개(비스택).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTBBK_API UC_EquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UC_EquipmentComponent();

	// 인벤토리의 itemID를 장착 — 슬롯은 DT의 equipSlot으로 결정. 같은 슬롯이면 기존 장비를 먼저 해제(가방 복귀).
	// 성공 시 true. 가방에 아이템이 없거나 미등록/슬롯None이면 false.
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipItem(FName itemID);

	// 해당 슬롯의 장비를 해제 — GE 제거 후 아이템을 가방으로 복귀. 비어 있으면 false.
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool UnequipItem(EEquipmentSlot slot);

	// 슬롯에 장착된 itemID 조회(없으면 None) — UI 아이콘 표시용.
	UFUNCTION(BlueprintPure, Category = "Equipment")
	FName GetEquippedItem(EEquipmentSlot slot) const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsSlotEquipped(EEquipmentSlot slot) const;

	// itemID의 장비 슬롯 종류 조회(미등록이면 None) — UI 드롭 시 슬롯 일치 검증용
	UFUNCTION(BlueprintPure, Category = "Equipment")
	EEquipmentSlot GetItemSlotType(FName itemID) const;

	// 현재 캐릭터가 이 아이템을 장착할 수 있는지 — Common은 누구나, Melee/Ranged 전용은 캐릭터 타입 일치 시만.
	// 미등록/슬롯None이면 false. UI에서 장착 가능 여부 표시용으로도 사용 가능.
	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool CanEquipItem(FName itemID) const;

	// itemID로 표시용 데이터(이름·아이콘·Mesh) 조회 — 장비 DT 기준. UI 슬롯 아이콘용.
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool GetItemData(FName itemID, FBaseItemData& OutData) const;

	// itemID로 장비 전용 데이터(슬롯·보너스 5종 포함) 전체 조회 — 툴팁 등 장비 스탯 표시용.
	// GetItemData는 FBaseItemData로 슬라이싱돼 보너스가 잘리므로 전체가 필요하면 이쪽 사용.
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool GetEquipmentData(FName itemID, FEquipmentItemData& OutData) const;

	// 공유 인벤토리 접근(소유 캐릭터 → 컨트롤러 → GetInventory). UI 드롭 처리 등에서 사용.
	UFUNCTION(BlueprintPure, Category = "Equipment")
	UC_InventoryComponent* GetInventory() const;

	// 장착 상태 변경 시 브로드캐스트 (장비창 UI 갱신용)
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnEquipmentChanged OnEquipmentChanged;

	// ASC가 캐릭터 로스터 전체에 공유되므로, 이 캐릭터가 비활성화될 때(교체로 다른 캐릭터로 전환)
	// 장착 보너스 GE를 ASC에서 제거 — 장착 상태(equipped)는 유지, GE만 잠시 뗌.
	// AC_PlayerController::ExecuteCharacterSwitch에서 OldChar에 호출.
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SuspendEquipBonuses();

	// 이 캐릭터가 다시 활성화될 때 장착된 항목들의 보너스 GE를 재적용.
	// AC_PlayerController::ExecuteCharacterSwitch에서 NewChar에 호출.
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void ReapplyEquipBonuses();

protected:
	// BP에서 GE_EquipBonus 지정 (C++ 하드코딩 금지)
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TSubclassOf<UGameplayEffect> equipBonusEffect;

	// BP에서 DT_EquipmentItem 지정 — 슬롯/보너스 조회용
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	UDataTable* equipmentTable = nullptr;

private:
	UAbilitySystemComponent* GetASC() const;

	// 소유 캐릭터 타입이 장비 등급(Common/Melee/Ranged)과 호환되는지 판정
	bool CanEquipClass(EEquipmentClass itemClass) const;

	// row의 보너스 5종을 SetByCaller로 주입해 현재 ASC에 적용 → 핸들 반환
	FActiveGameplayEffectHandle ApplyEquipGE(const FEquipmentItemData& row) const;

	// 슬롯 → 장착 항목
	TMap<EEquipmentSlot, FEquippedEntry> equipped;
};
