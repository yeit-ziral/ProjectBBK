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
 * - 캐릭터별 ASC에 GE가 머무르므로 캐릭터 교체 시 재적용 불필요.
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

	// itemID로 표시용 데이터(이름·아이콘·Mesh) 조회 — 장비 DT 기준. UI 슬롯 아이콘용.
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool GetItemData(FName itemID, FBaseItemData& OutData) const;

	// 공유 인벤토리 접근(소유 캐릭터 → 컨트롤러 → GetInventory). UI 드롭 처리 등에서 사용.
	UFUNCTION(BlueprintPure, Category = "Equipment")
	UC_InventoryComponent* GetInventory() const;

	// 장착 상태 변경 시 브로드캐스트 (장비창 UI 갱신용)
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnEquipmentChanged OnEquipmentChanged;

protected:
	// BP에서 GE_EquipBonus 지정 (C++ 하드코딩 금지)
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	TSubclassOf<UGameplayEffect> equipBonusEffect;

	// BP에서 DT_EquipmentItem 지정 — 슬롯/보너스 조회용
	UPROPERTY(EditDefaultsOnly, Category = "Equipment")
	UDataTable* equipmentTable = nullptr;

private:
	UAbilitySystemComponent* GetASC() const;

	// row의 보너스 5종을 SetByCaller로 주입해 현재 ASC에 적용 → 핸들 반환
	FActiveGameplayEffectHandle ApplyEquipGE(const FEquipmentItemData& row) const;

	// 슬롯 → 장착 항목
	TMap<EEquipmentSlot, FEquippedEntry> equipped;
};
