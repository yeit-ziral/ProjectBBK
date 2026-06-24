// Fill out your copyright notice in the Description page of Project Settings.

#include "C_InventoryTester.h"
#include "C_InventoryComponent.h"
#include "C_InventoryWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

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

	// 커서 표시 + UMG가 마우스(드래그) 입력을 받도록 GameAndUI 입력 모드 설정.
	// 단순 SetShowMouseCursor만으로는 위젯이 마우스 이벤트를 수신하지 못함.
	// AC_PlayerController::BeginPlay가 같은 프레임에 GameOnly + 커서 OFF로 덮어쓰므로
	// (BeginPlay 순서 미보장) 다음 틱에 적용해 확실히 후순위로 둔다.
	if (bShowMouseCursor && pc)
	{
		TWeakObjectPtr<APlayerController> weakPC = pc;
		TWeakObjectPtr<UC_InventoryWidget> weakWidget = widget;
		GetWorldTimerManager().SetTimerForNextTick([weakPC, weakWidget]()
		{
			if (!weakPC.IsValid()) return;

			weakPC->SetShowMouseCursor(true);

			FInputModeGameAndUI inputMode;
			if (weakWidget.IsValid())
				inputMode.SetWidgetToFocus(weakWidget->TakeWidget());
			inputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			weakPC->SetInputMode(inputMode);
		});
	}
}
