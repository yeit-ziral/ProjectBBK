// Fill out your copyright notice in the Description page of Project Settings.

#include "C_InventoryTester.h"
#include "C_InventoryComponent.h"
#include "C_InventoryWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

AC_InventoryTester::AC_InventoryTester()
{
	PrimaryActorTick.bCanEverTick = false;
	inventory = CreateDefaultSubobject<UC_InventoryComponent>(TEXT("Inventory"));
}

void AC_InventoryTester::BeginPlay()
{
	Super::BeginPlay();

	if (!inventory) return;

	// 테이블 주입 + 테스트 아이템 추가
	inventory->SetItemTables(consumableTable, equipmentTable);
	for (const FInventoryTestEntry& entry : testItems)
		inventory->AddItem(entry.itemID, entry.count);

	// 위젯 생성 + 연결 + 표시
	if (!inventoryWidgetClass) return;

	APlayerController* pc = UGameplayStatics::GetPlayerController(this, 0);
	UC_InventoryWidget* widget = CreateWidget<UC_InventoryWidget>(pc ? pc : GetWorld()->GetFirstPlayerController(), inventoryWidgetClass);
	if (!widget) return;

	widget->SetInventory(inventory);
	widget->AddToViewport();

	if (bShowMouseCursor && pc)
		pc->SetShowMouseCursor(true);
}
