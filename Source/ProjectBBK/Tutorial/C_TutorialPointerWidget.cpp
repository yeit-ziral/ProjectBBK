// Fill out your copyright notice in the Description page of Project Settings.

#include "C_TutorialPointerWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Rendering/DrawElements.h"
#include "Engine/World.h"

const FLinearColor UC_TutorialPointerWidget::arrowColor = FLinearColor(1.0f, 0.82f, 0.15f, 1.0f);

TSharedRef<SWidget> UC_TutorialPointerWidget::RebuildWidget()
{
	// CreateWidget 경로에서 이미 Initialize가 돌아 WidgetTree가 만들어져 있지만,
	// 그렇지 않은 경로에서도 트리를 반드시 확보한다 (없으면 SSpacer가 되어 아무것도 안 그려진다)
	if (!WidgetTree)
	{
		Initialize();
	}

	// 네이티브 전용 UUserWidget이라 디자이너에서 만든 위젯 트리가 없다 — 직접 만든다.
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PointerRoot"));
		WidgetTree->RootWidget = RootCanvas;

		hintBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HintBorder"));
		hintBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.04f, 0.78f));
		hintBorder->SetPadding(FMargin(16.f, 9.f));

		hintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
		hintText->SetJustification(ETextJustify::Center);
		hintBorder->AddChild(hintText);

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(RootCanvas->AddChild(hintBorder)))
		{
			// AutoSize + Alignment(0.5, 1) → SetPosition에 넘긴 좌표가 문구 박스의 "아래쪽 중앙"이 된다
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		}

		hintBorder->SetVisibility(ESlateVisibility::Collapsed);
	}

	return Super::RebuildWidget();
}

void UC_TutorialPointerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 전체 화면을 덮으므로 반드시 히트 테스트에서 빠져야 아래 HUD·인벤토리 클릭이 막히지 않는다
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UC_TutorialPointerWidget::PointAt(UWidget* TargetWidget, const FText& Hint, const FSlateFontInfo& Font, const FSlateColor& TextColor)
{
	targetWidget = TargetWidget;
	bPointerVisible = false;

	if (hintText)
	{
		hintText->SetText(Hint);

		// 폰트는 안내 위젯에서 그대로 물려받는다 — 엔진 기본 폰트는 한글 글리프가 없어 깨진다
		if (Font.HasValidFont())
		{
			hintText->SetFont(Font);
		}
		hintText->SetColorAndOpacity(TextColor);
	}

	if (hintBorder)
	{
		const bool bShowHint = TargetWidget != nullptr && !Hint.IsEmpty();
		hintBorder->SetVisibility(bShowHint ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UC_TutorialPointerWidget::ClearPointer()
{
	targetWidget = nullptr;
	bPointerVisible = false;

	if (hintBorder)
	{
		hintBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UC_TutorialPointerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UWidget* Target = targetWidget.Get();
	if (!Target)
	{
		bPointerVisible = false;
		return;
	}

	// 대상이 아직 한 번도 그려지지 않았으면 캐시 지오메트리가 0이다 — 다음 프레임에 다시 시도
	const FGeometry& TargetGeometry = Target->GetCachedGeometry();
	const FVector2D AbsPos = FVector2D(TargetGeometry.GetAbsolutePosition());
	const FVector2D AbsSize = FVector2D(TargetGeometry.GetAbsoluteSize());
	if (AbsSize.X <= KINDA_SMALL_NUMBER || AbsSize.Y <= KINDA_SMALL_NUMBER)
	{
		bPointerVisible = false;
		return;
	}

	targetMin = FVector2D(MyGeometry.AbsoluteToLocal(AbsPos)) - FVector2D(targetPadding);
	targetMax = FVector2D(MyGeometry.AbsoluteToLocal(AbsPos + AbsSize)) + FVector2D(targetPadding);

	const FVector2D ScreenSize = FVector2D(MyGeometry.GetLocalSize());
	const FVector2D BorderSize = hintBorder ? FVector2D(hintBorder->GetCachedGeometry().GetLocalSize()) : FVector2D::ZeroVector;
	const float TargetCenterX = (targetMin.X + targetMax.X) * 0.5f;

	// 대상이 화면 위쪽에 붙어 있으면 문구를 아래에 둔다 — 그 외에는 위에서 아래로 가리킨다
	const bool bHintAbove = (targetMin.Y - arrowLength - BorderSize.Y) > 12.f;

	float HintX = TargetCenterX;
	if (BorderSize.X > 0.f)
	{
		const float HalfW = BorderSize.X * 0.5f;
		HintX = FMath::Clamp(TargetCenterX, HalfW + 12.f, FMath::Max(HalfW + 12.f, ScreenSize.X - HalfW - 12.f));
	}

	float HintY;
	if (bHintAbove)
	{
		HintY = targetMin.Y - arrowLength;
		arrowStart = FVector2D(HintX, HintY + 4.f);
		arrowEnd = FVector2D(TargetCenterX, targetMin.Y);
	}
	else
	{
		HintY = targetMax.Y + arrowLength;
		arrowStart = FVector2D(HintX, HintY - BorderSize.Y - 4.f);
		arrowEnd = FVector2D(TargetCenterX, targetMax.Y);
	}

	if (hintBorder)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(hintBorder->Slot))
		{
			// Alignment.Y는 위에 둘 때 1(아래쪽 기준), 아래에 둘 때 0(위쪽 기준)
			CanvasSlot->SetAlignment(FVector2D(0.5f, bHintAbove ? 1.0f : 0.0f));
			CanvasSlot->SetPosition(FVector2D(HintX, bHintAbove ? HintY : HintY - BorderSize.Y));
		}
	}

	if (const UWorld* World = GetWorld())
	{
		pulseAlpha = 0.55f + 0.45f * FMath::Sin(World->GetTimeSeconds() * 5.f);
	}

	bPointerVisible = true;
}

int32 UC_TutorialPointerWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (!bPointerVisible)
	{
		return MaxLayer;
	}

	const int32 DrawLayer = MaxLayer + 1;
	const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();

	// 1) 대상 위젯을 감싸는 강조 사각형 (깜빡임)
	FLinearColor BoxColor = arrowColor;
	BoxColor.A = pulseAlpha;

	TArray<FVector2D> BoxPoints;
	BoxPoints.Reserve(5);
	BoxPoints.Add(targetMin);
	BoxPoints.Add(FVector2D(targetMax.X, targetMin.Y));
	BoxPoints.Add(targetMax);
	BoxPoints.Add(FVector2D(targetMin.X, targetMax.Y));
	BoxPoints.Add(targetMin);
	FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer, PaintGeometry, BoxPoints,
		ESlateDrawEffect::None, BoxColor, true, lineThickness);

	// 2) 화살표 몸통
	TArray<FVector2D> ShaftPoints;
	ShaftPoints.Reserve(2);
	ShaftPoints.Add(arrowStart);
	ShaftPoints.Add(arrowEnd);
	FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer, PaintGeometry, ShaftPoints,
		ESlateDrawEffect::None, arrowColor, true, lineThickness);

	// 3) 화살촉 — 진행 방향 기준 좌우 대각선 두 줄
	const FVector2D Dir = (arrowEnd - arrowStart).GetSafeNormal();
	if (!Dir.IsNearlyZero())
	{
		const FVector2D Perp(-Dir.Y, Dir.X);
		const FVector2D Base = arrowEnd - Dir * arrowHeadSize;
		const FVector2D Wing = Perp * (arrowHeadSize * 0.5f);

		TArray<FVector2D> HeadPoints;
		HeadPoints.Reserve(3);
		HeadPoints.Add(Base + Wing);
		HeadPoints.Add(arrowEnd);
		HeadPoints.Add(Base - Wing);
		FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer, PaintGeometry, HeadPoints,
			ESlateDrawEffect::None, arrowColor, true, lineThickness);
	}

	return DrawLayer;
}
