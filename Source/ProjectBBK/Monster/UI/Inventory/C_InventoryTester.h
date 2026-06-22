// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "C_InventoryTester.generated.h"

class UC_InventoryComponent;
class UC_InventoryWidget;
class UDataTable;

/** 인벤토리 테스트용 추가 아이템 한 항목 (itemID = DataTable Row Name). */
USTRUCT(BlueprintType)
struct FInventoryTestEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Test")
	FName itemID = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Test")
	int32 count = 1;
};

/**
 * 인벤토리 디버그 테스터 — BeginPlay에서 테이블 주입 + 아이템 추가 + 인벤토리 위젯을 화면에 표시.
 * 레벨에 배치하고 Details에서 DataTable/위젯클래스/테스트아이템을 지정한 뒤 Play하면 됨.
 * (참조는 전부 에디터 지정 — 하드코딩 없음.)
 */
UCLASS()
class PROJECTBBK_API AC_InventoryTester : public AActor
{
	GENERATED_BODY()

public:
	AC_InventoryTester();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	UC_InventoryComponent* inventory;

	// DT_ConsumableItem / DT_EquipmentItem 지정
	UPROPERTY(EditAnywhere, Category = "Inventory")
	UDataTable* consumableTable = nullptr;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	UDataTable* equipmentTable = nullptr;

	// WBP_Inventory 지정
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UC_InventoryWidget> inventoryWidgetClass;

	// 추가할 테스트 아이템 (itemID = Row Name 예: HpPotion, IronSword)
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TArray<FInventoryTestEntry> testItems;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	bool bShowMouseCursor = true;
};
