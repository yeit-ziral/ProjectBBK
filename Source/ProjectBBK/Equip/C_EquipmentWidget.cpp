// Fill out your copyright notice in the Description page of Project Settings.

#include "C_EquipmentWidget.h"
#include "C_EquipmentSlotWidget.h"
#include "C_EquipmentComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

void UC_EquipmentWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CollectSlotWidgets();
}

void UC_EquipmentWidget::NativeDestruct()
{
	if (equipComp.IsValid())
		equipComp->OnEquipmentChanged.RemoveDynamic(this, &UC_EquipmentWidget::HandleEquipmentChanged);

	Super::NativeDestruct();
}

void UC_EquipmentWidget::CollectSlotWidgets()
{
	slotWidgets.Reset();
	if (!WidgetTree)
		return;

	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		if (UC_EquipmentSlotWidget* SlotWidget = Cast<UC_EquipmentSlotWidget>(Widget))
			slotWidgets.Add(SlotWidget);
	});
}

void UC_EquipmentWidget::SetEquipment(UC_EquipmentComponent* Equip)
{
	// 기존 바인딩 해제 (캐릭터 교체 시 이전 컴포넌트 이벤트 미수신 방지 — HUD 재초기화 패턴과 동일)
	if (equipComp.IsValid())
		equipComp->OnEquipmentChanged.RemoveDynamic(this, &UC_EquipmentWidget::HandleEquipmentChanged);

	equipComp = Equip;

	if (Equip)
		Equip->OnEquipmentChanged.AddDynamic(this, &UC_EquipmentWidget::HandleEquipmentChanged);

	// 슬롯 수집이 아직 안 됐으면(SetEquipment가 Construct보다 먼저 호출되는 경우) 보장
	if (slotWidgets.Num() == 0)
		CollectSlotWidgets();

	RefreshAllSlots();
}

FReply UC_EquipmentWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 슬롯은 자체적으로 입력을 소비하므로, 여기로 버블링된 좌클릭 = 프레임(배경) 클릭 → 창 이동 시작
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && WindowRoot)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(WindowRoot->Slot))
		{
			bDraggingWindow = true;
			const FVector2D LocalMouse = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
			dragOffset = LocalMouse - CanvasSlot->GetPosition();
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UC_EquipmentWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDraggingWindow && WindowRoot)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(WindowRoot->Slot))
		{
			const FVector2D LocalMouse = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
			CanvasSlot->SetPosition(LocalMouse - dragOffset);
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UC_EquipmentWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDraggingWindow)
	{
		bDraggingWindow = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UC_EquipmentWidget::HandleEquipmentChanged()
{
	RefreshAllSlots();
}

void UC_EquipmentWidget::RefreshAllSlots()
{
	UC_EquipmentComponent* Equip = equipComp.Get();
	for (UC_EquipmentSlotWidget* SlotWidget : slotWidgets)
	{
		if (SlotWidget)
			SlotWidget->RefreshSlot(Equip);
	}
}
