// Fill out your copyright notice in the Description page of Project Settings.

#include "C_EquipmentSlotWidget.h"
#include "C_EquipmentComponent.h"
#include "../Inventory/C_InventorySlotWidget.h"
#include "../Items/ItemData.h"
#include "../Items/C_ItemTooltipWidget.h"
#include "Components/Image.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/SlateBlueprintLibrary.h"   // AbsoluteToViewport
#include "GameFramework/PlayerController.h"

void UC_EquipmentSlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 슬롯 테두리 브러시 구성 (인벤토리 슬롯과 동일)
	if (SlotFrame && slotFrameTexture)
	{
		FSlateBrush brush;
		brush.SetResourceObject(slotFrameTexture);
		brush.DrawAs    = ESlateBrushDrawType::Image;
		brush.ImageSize = FVector2D(96.f, 96.f);
		SlotFrame->SetBrush(brush);
	}
}

void UC_EquipmentSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (activeTooltip || !tooltipWidgetClass || !equipComp.IsValid())
		return;

	const FName itemID = equipComp->GetEquippedItem(slotType);
	if (itemID.IsNone())
		return;   // 빈 슬롯 → 툴팁 없음

	FEquipmentItemData equipData;
	if (!equipComp->GetEquipmentData(itemID, equipData))
		return;

	UC_ItemTooltipWidget* tooltip = CreateWidget<UC_ItemTooltipWidget>(this, tooltipWidgetClass);
	if (!tooltip)
		return;

	tooltip->SetItem(equipData);
	tooltip->SetVisibility(ESlateVisibility::HitTestInvisible);   // 커서/hover 가로채지 않게

	// 뷰포트 슬롯은 추가 시점의 오프셋(크기·위치)을 잡아두므로, AddToViewport 전에 전부 확정할 것.
	tooltip->TakeWidget();          // Slate 위젯 생성 — 프리패스 대상 확보
	tooltip->ForceLayoutPrepass();  // desired size 확정

	// FGameViewportWidgetSlot의 기본 Anchors는 (0,0,1,1) = 전체화면 스트레치.
	// SetPositionInViewport / SetDesiredSizeInViewport가 내부에서 Anchors를 (0,0)으로 리셋해 주지만,
	// 둘 다 스킵되면(크기 0 + 마우스 위치 취득 실패) 기본값이 남아 툴팁이 화면 전체로 늘어난다.
	// → 앵커를 먼저 명시적으로 고정해 스트레치 가능성 자체를 제거. (반드시 다른 Set*InViewport보다 앞)
	tooltip->SetAnchorsInViewport(FAnchors(0.f, 0.f));

	// 크기가 0이면 슬롯이 AutoSize로 동작해 콘텐츠 크기에 맞춰짐 — 앵커가 스트레치가 아닐 때만 성립
	const FVector2D tooltipSize = tooltip->GetDesiredSize();
	if (!tooltipSize.IsNearlyZero())
		tooltip->SetDesiredSizeInViewport(tooltipSize);

	// 커서 근처에 배치 (살짝 오프셋).
	// PlayerController::GetMousePosition은 마우스 캡처 중(드래그&드롭 등) 뷰포트 캐시 좌표가 (-1,-1)이라
	// false를 반환한다 → 마우스 이벤트의 절대 좌표를 뷰포트 로컬 좌표로 변환해 사용(캡처 상태와 무관).
	FVector2D pixelPos    = FVector2D::ZeroVector;
	FVector2D viewportPos = FVector2D::ZeroVector;
	USlateBlueprintLibrary::AbsoluteToViewport(this, InMouseEvent.GetScreenSpacePosition(), pixelPos, viewportPos);
	tooltip->SetPositionInViewport(viewportPos + FVector2D(16.f, 16.f), false);   // 이미 뷰포트 로컬 → DPI 보정 불필요

	tooltip->AddToViewport(1000);   // 창·HUD 위에
	activeTooltip = tooltip;
}

void UC_EquipmentSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (activeTooltip)
	{
		activeTooltip->RemoveFromParent();
		activeTooltip = nullptr;
	}
}

void UC_EquipmentSlotWidget::NativeDestruct()
{
	if (activeTooltip)
	{
		activeTooltip->RemoveFromParent();
		activeTooltip = nullptr;
	}

	Super::NativeDestruct();
}

void UC_EquipmentSlotWidget::RefreshSlot(UC_EquipmentComponent* Equip)
{
	equipComp = Equip;

	// 드롭 대상이 되도록 항상 hit-test 가능
	SetVisibility(ESlateVisibility::Visible);

	const FName itemID = Equip ? Equip->GetEquippedItem(slotType) : NAME_None;

	UTexture2D* icon = nullptr;
	if (Equip && !itemID.IsNone())
	{
		FBaseItemData data;
		if (Equip->GetItemData(itemID, data))
			icon = data.itemIcon;
	}
	currentIcon = icon;   // 드래그 비주얼용 캐시

	if (ItemIcon)
	{
		if (icon)
		{
			FSlateBrush brush;
			brush.SetResourceObject(icon);
			brush.DrawAs    = ESlateBrushDrawType::Image;
			brush.ImageSize = FVector2D(64.f, 64.f);
			ItemIcon->SetBrush(brush);
			ItemIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// 툴팁은 NativeOnMouseEnter/Leave에서 hover 시점에 직접 띄움 — 여기서 미리 설정하지 않음
}

FReply UC_EquipmentSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 장착돼 있을 때만 상호작용
	if (equipComp.IsValid() && equipComp->IsSlotEquipped(slotType))
	{
		// 우클릭 → 즉시 해제
		if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			equipComp->UnequipItem(slotType);   // OnEquipmentChanged → 장비창 갱신
			return FReply::Handled();
		}

		// 좌클릭 → 드래그 감지 시작(인벤토리로 끌어 해제). 안 끌면 더블클릭으로 처리됨
		if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UC_EquipmentSlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 좌클릭 더블클릭 → 인벤토리로 해제
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && equipComp.IsValid() && equipComp->IsSlotEquipped(slotType))
	{
		equipComp->UnequipItem(slotType);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void UC_EquipmentSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (!equipComp.IsValid() || !equipComp->IsSlotEquipped(slotType))
		return;

	UDragDropOperation* Op = NewObject<UDragDropOperation>(GetTransientPackage(), UDragDropOperation::StaticClass());
	Op->Payload = this;                       // 드롭 시 출발 장비 슬롯 식별용
	Op->Pivot   = EDragPivot::CenterCenter;

	// 드래그 따라다니는 아이콘 비주얼
	if (currentIcon)
	{
		UImage* dragVisual = NewObject<UImage>(this);
		FSlateBrush brush;
		brush.SetResourceObject(currentIcon);
		brush.DrawAs    = ESlateBrushDrawType::Image;
		brush.ImageSize = FVector2D(64.f, 64.f);
		dragVisual->SetBrush(brush);
		Op->DefaultDragVisual = dragVisual;
	}

	// 드래그하는 동안 슬롯 아이콘 숨김(집어든 느낌, 커서 비주얼과 중복 방지)
	// 드롭 성공 시 OnEquipmentChanged로 갱신 / 취소 시 NativeOnDragCancelled에서 복구
	if (ItemIcon)
		ItemIcon->SetVisibility(ESlateVisibility::Hidden);

	OutOperation = Op;
}

void UC_EquipmentSlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

	// 인벤토리 밖 등 유효하지 않은 곳에 드롭 → 아직 장착 상태이므로 아이콘 복구
	RefreshSlot(equipComp.Get());
}

void UC_EquipmentSlotWidget::RequestUnequip()
{
	if (equipComp.IsValid() && equipComp->IsSlotEquipped(slotType))
		equipComp->UnequipItem(slotType);   // 가방으로 복귀 + OnEquipmentChanged
}

bool UC_EquipmentSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!InOperation || !equipComp.IsValid())
		return false;

	// 인벤토리 슬롯에서 끌어온 것만 장착 대상
	UC_InventorySlotWidget* source = Cast<UC_InventorySlotWidget>(InOperation->Payload);
	if (!source)
		return false;

	const FName itemID = source->GetItemID();
	if (itemID.IsNone())
		return false;

	// 이 칸의 slotType과 드롭한 아이템의 장비 슬롯이 일치할 때만 장착
	// (예: 무기를 머리 칸에 드롭하면 거부). 장비가 아닌 소비 아이템(None)도 자동 거부.
	if (equipComp->GetItemSlotType(itemID) != slotType)
		return false;

	// 장착 (가방에서 제거 + GE 적용 → OnEquipmentChanged로 장비창 갱신). 미보유면 false
	return equipComp->EquipItem(itemID);
}
