// Fill out your copyright notice in the Description page of Project Settings.

#include "C_TutorialPointerWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/Actor.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Engine/World.h"

const FLinearColor UC_TutorialPointerWidget::arrowColor = FLinearColor(1.0f, 0.82f, 0.15f, 1.0f);

TSharedRef<SWidget> UC_TutorialPointerWidget::RebuildWidget()
{
	EnsureWidgetTree();

	return Super::RebuildWidget();
}

void UC_TutorialPointerWidget::EnsureWidgetTree()
{
	// CreateWidget 경로에서 이미 Initialize가 돌아 WidgetTree가 만들어져 있지만,
	// 그렇지 않은 경로에서도 트리를 반드시 확보한다 (없으면 SSpacer가 되어 아무것도 안 그려진다)
	if (!WidgetTree)
	{
		Initialize();
	}

	// 네이티브 전용 UUserWidget이라 디자이너에서 만든 위젯 트리가 없다 — 직접 만든다.
	if (!WidgetTree || WidgetTree->RootWidget != nullptr)
	{
		return;
	}

	rootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PointerRoot"));
	WidgetTree->RootWidget = rootCanvas;

	// 문구 박스는 가리킬 대상 수에 맞춰 늘린다 — 기본 1개
	EnsureHintSlots(1);

	// 플레이어 옆에 띄우는 말풍선 — 보조 문구와 같은 모양, 화살표 없음
	calloutBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CalloutBorder"));
	calloutBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.04f, 0.78f));
	calloutBorder->SetPadding(FMargin(16.f, 9.f));

	calloutText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CalloutText"));
	calloutText->SetJustification(ETextJustify::Center);
	calloutBorder->AddChild(calloutText);

	if (UCanvasPanelSlot* CalloutSlot = Cast<UCanvasPanelSlot>(rootCanvas->AddChild(calloutBorder)))
	{
		// AutoSize + Alignment(0, 0.5) → SetPosition에 넘긴 좌표가 말풍선의 "왼쪽 가운데"가 된다
		CalloutSlot->SetAutoSize(true);
		CalloutSlot->SetAlignment(FVector2D(0.0f, 0.5f));
	}

	calloutBorder->SetVisibility(ESlateVisibility::Collapsed);
}

void UC_TutorialPointerWidget::EnsureHintSlots(int32 Count)
{
	if (!rootCanvas || !WidgetTree)
	{
		return;
	}

	while (hintBorders.Num() < Count)
	{
		const int32 Index = hintBorders.Num();

		UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(*FString::Printf(TEXT("HintBorder_%d"), Index)));
		Border->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.04f, 0.78f));
		Border->SetPadding(FMargin(16.f, 9.f));

		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*FString::Printf(TEXT("HintText_%d"), Index)));
		Text->SetJustification(ETextJustify::Center);
		Border->AddChild(Text);

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(rootCanvas->AddChild(Border)))
		{
			// AutoSize + Alignment(0.5, 1) → SetPosition에 넘긴 좌표가 문구 박스의 "아래쪽 중앙"이 된다
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		}

		Border->SetVisibility(ESlateVisibility::Collapsed);

		hintBorders.Add(Border);
		hintTexts.Add(Text);
	}
}

void UC_TutorialPointerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 전체 화면을 덮으므로 반드시 히트 테스트에서 빠져야 아래 HUD·인벤토리 클릭이 막히지 않는다
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UC_TutorialPointerWidget::PointAt(UWidget* TargetWidget, const FText& Hint, const FSlateFontInfo& Font, const FSlateColor& TextColor)
{
	PointAtMultiple(TArray<UWidget*>{ TargetWidget }, TArray<FText>{ Hint }, Font, TextColor);
}

void UC_TutorialPointerWidget::PointAtMultiple(const TArray<UWidget*>& Targets, const TArray<FText>& Hints, const FSlateFontInfo& Font, const FSlateColor& TextColor,
	const TArray<FString>& HighlightMarkers)
{
	EnsureWidgetTree();

	pointers.Reset();
	TArray<FText> EntryHints;

	for (int32 i = 0; i < Targets.Num(); ++i)
	{
		if (!Targets[i])
		{
			continue;
		}

		const FText Hint = Hints.IsValidIndex(i) ? Hints[i] : FText::GetEmpty();

		FTutorialPointerEntry& Entry = pointers.AddDefaulted_GetRef();
		Entry.target = Targets[i];
		Entry.bHasHint = !Hint.IsEmpty();
		Entry.highlightFrom = HighlightMarkers.IsValidIndex(i) ? HighlightMarkers[i] : FString();
		EntryHints.Add(Hint);
	}

	EnsureHintSlots(pointers.Num());

	for (int32 i = 0; i < hintBorders.Num(); ++i)
	{
		const bool bUsed = pointers.IsValidIndex(i);

		if (bUsed && hintTexts.IsValidIndex(i) && hintTexts[i])
		{
			hintTexts[i]->SetText(EntryHints[i]);

			// 폰트는 안내 위젯에서 그대로 물려받는다 — 엔진 기본 폰트는 한글 글리프가 없어 깨진다
			if (Font.HasValidFont())
			{
				hintTexts[i]->SetFont(Font);
			}
			hintTexts[i]->SetColorAndOpacity(TextColor);
		}

		if (hintBorders[i])
		{
			const bool bShowHint = bUsed && pointers[i].bHasHint && IsWidgetOnScreen(pointers[i].target.Get());
			hintBorders[i]->SetVisibility(bShowHint ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

void UC_TutorialPointerWidget::ClearPointer()
{
	pointers.Reset();

	for (UBorder* Border : hintBorders)
	{
		if (Border)
		{
			Border->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

bool UC_TutorialPointerWidget::IsPointingAt(const TArray<UWidget*>& Targets) const
{
	int32 EntryIndex = 0;
	for (const UWidget* Target : Targets)
	{
		if (!Target)
		{
			continue;
		}

		if (!pointers.IsValidIndex(EntryIndex) || pointers[EntryIndex].target.Get() != Target)
		{
			return false;
		}
		++EntryIndex;
	}

	return EntryIndex == pointers.Num();
}

bool UC_TutorialPointerWidget::IsTargetOnScreen() const
{
	if (pointers.Num() == 0)
	{
		return false;
	}

	for (const FTutorialPointerEntry& Entry : pointers)
	{
		if (!IsWidgetOnScreen(Entry.target.Get()))
		{
			return false;
		}
	}

	return true;
}

void UC_TutorialPointerWidget::ShowWorldCallout(AActor* AnchorActor, const FText& Text, const FSlateFontInfo& Font, const FSlateColor& TextColor)
{
	calloutActor = AnchorActor;

	if (calloutText)
	{
		calloutText->SetText(Text);

		// 폰트는 안내 위젯에서 물려받는다 — 엔진 기본 폰트는 한글 글리프가 없어 깨진다
		if (Font.HasValidFont())
		{
			calloutText->SetFont(Font);
		}
		calloutText->SetColorAndOpacity(TextColor);
	}

	// 바로 투영해서 켠다 — 다음 틱까지 기다리면 이전 위치(또는 0,0)에 한 프레임 보일 수 있다
	UpdateWorldCallout();
}

void UC_TutorialPointerWidget::HideWorldCallout()
{
	calloutActor.Reset();

	if (calloutBorder)
	{
		calloutBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UC_TutorialPointerWidget::UpdateWorldCallout()
{
	if (!calloutBorder)
	{
		return;
	}

	const AActor* Anchor = calloutActor.Get();
	APlayerController* OwningPC = GetOwningPlayer();

	// DPI 스케일이 반영된 위젯 좌표로 투영한다 — 이 위젯은 뷰포트 전체를 덮으므로 그대로 캔버스 좌표가 된다.
	// 카메라 뒤에 있으면 투영에 실패하므로 숨긴다.
	FVector2D ScreenPosition = FVector2D::ZeroVector;
	const bool bProjected = Anchor && OwningPC &&
		UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(OwningPC, Anchor->GetActorLocation(), ScreenPosition, false);

	if (!bProjected)
	{
		calloutBorder->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (UCanvasPanelSlot* CalloutSlot = Cast<UCanvasPanelSlot>(calloutBorder->Slot))
	{
		CalloutSlot->SetPosition(ScreenPosition + FVector2D(calloutSideOffset, 0.f));
	}

	calloutBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
}

bool UC_TutorialPointerWidget::ComputeSubstringHighlight(const UWidget* Target, const FString& Marker, FVector2D& OutAbsMin, FVector2D& OutAbsMax)
{
	const UTextBlock* TextBlock = Cast<UTextBlock>(Target);
	if (!TextBlock || Marker.IsEmpty())
	{
		return false;
	}

	const FString FullText = TextBlock->GetText().ToString();
	const int32 MarkerIndex = FullText.Find(Marker);
	if (MarkerIndex == INDEX_NONE)
	{
		return false;
	}

	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}

	// 강조 구간이 문자열 끝까지이므로(예: "120 (+20)"의 "(+20)") 왼쪽이 아니라 **오른쪽 끝을 기준**으로 잡는다.
	// 앞쪽 글자("120 ")의 측정 오차가 박스를 통째로 밀어버리는 것을 막기 위함 —
	// 오른쪽 끝은 위젯의 Desired Size(실제로 그려진 텍스트 폭)라 정확하고, 오차는 박스 왼쪽 변에만 남는다.
	const FGeometry& TargetGeometry = Target->GetCachedGeometry();
	const float LayoutScale = FMath::Max(TargetGeometry.GetAccumulatedLayoutTransform().GetScale(), KINDA_SMALL_NUMBER);

	// STextBlock::ComputeDesiredSize와 같은 방식 — 레이아웃 배율로 재고 다시 나눠야 힌팅 차이가 없다
	const TSharedRef<FSlateFontMeasure> MeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateFontInfo& Font = TextBlock->GetFont();
	const float SubWidth = static_cast<float>(MeasureService->Measure(FullText.Mid(MarkerIndex), Font, LayoutScale).X) / LayoutScale;
	if (SubWidth <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	// 텍스트가 위젯 왼쪽 끝에서 시작한다고 본다 — 스탯 수치 칸은 HAlign_Fill + Justification Left라 성립.
	// 따라서 그려진 텍스트의 오른쪽 끝 = Desired Size 폭. (측정 실패 시 위젯 폭으로 폴백)
	const float LocalWidth = static_cast<float>(TargetGeometry.GetLocalSize().X);
	const float DesiredWidth = static_cast<float>(TextBlock->GetDesiredSize().X);
	const float TextRight = (DesiredWidth > KINDA_SMALL_NUMBER) ? FMath::Min(DesiredWidth, LocalWidth) : LocalWidth;

	const float LocalHeight = static_cast<float>(TargetGeometry.GetLocalSize().Y);
	OutAbsMin = FVector2D(TargetGeometry.LocalToAbsolute(FVector2D(FMath::Max(TextRight - SubWidth, 0.f), 0.f)));
	OutAbsMax = FVector2D(TargetGeometry.LocalToAbsolute(FVector2D(TextRight, LocalHeight)));

	// 눈으로 맞춘 보정 — 화면 px 단위라 배율과 무관하게 같은 거리만큼 민다
	OutAbsMin.X += substringHighlightNudgeX;
	OutAbsMax.X += substringHighlightNudgeX;
	return true;
}

bool UC_TutorialPointerWidget::IsWidgetOnScreen(const UWidget* Widget)
{
	const UWidget* Current = Widget;
	while (Current)
	{
		if (!Current->IsVisible())
		{
			return false;
		}

		if (const UUserWidget* AsUserWidget = Cast<UUserWidget>(Current))
		{
			if (AsUserWidget->IsInViewport())
			{
				return true;
			}
		}

		if (const UPanelWidget* Parent = Current->GetParent())
		{
			Current = Parent;
			continue;
		}

		// 부모 패널이 없으면 WidgetTree의 루트 — 그 트리를 소유한 UUserWidget으로 올라간다
		// (WBP_HUD 안의 WBP_AmmoCylinder 같은 중첩 위젯 경로). 소유자가 없으면 화면 밖.
		Current = Current->GetTypedOuter<UUserWidget>();
	}

	return false;
}

void UC_TutorialPointerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 말풍선은 가리키기 대상 유무와 무관하게 매 프레임 따라간다 — 아래 조기 return보다 먼저
	if (calloutActor.IsValid())
	{
		UpdateWorldCallout();
	}

	if (pointers.Num() == 0)
	{
		return;
	}

	// 1) 대상별 강조 사각형 계산 — 화면에 없거나 아직 그려지지 않은 대상은 이번 프레임에서 뺀다
	TArray<int32> VisibleIndices;
	float GroupTop = TNumericLimits<float>::Max();
	float GroupBottom = TNumericLimits<float>::Lowest();
	float MaxHintHeight = 0.f;

	for (int32 i = 0; i < pointers.Num(); ++i)
	{
		FTutorialPointerEntry& Entry = pointers[i];
		Entry.bVisible = false;

		UBorder* Border = hintBorders.IsValidIndex(i) ? hintBorders[i].Get() : nullptr;
		UWidget* Target = Entry.target.Get();

		// 창을 닫아도 캐시 지오메트리는 마지막 위치 그대로 남는다 — 화면에서 사라진 대상은 가리키지 않는다
		const bool bTargetOnScreen = Target && IsWidgetOnScreen(Target);
		if (Border)
		{
			Border->SetVisibility((bTargetOnScreen && Entry.bHasHint) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}

		if (!bTargetOnScreen)
		{
			continue;
		}

		// 대상이 아직 한 번도 그려지지 않았으면 캐시 지오메트리가 0이다 — 다음 프레임에 다시 시도
		const FGeometry& TargetGeometry = Target->GetCachedGeometry();
		const FVector2D AbsPos = FVector2D(TargetGeometry.GetAbsolutePosition());
		const FVector2D AbsSize = FVector2D(TargetGeometry.GetAbsoluteSize());
		if (AbsSize.X <= KINDA_SMALL_NUMBER || AbsSize.Y <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		// 스탯창의 "120 (+20)"처럼 텍스트 일부만 감싸야 하는 대상은 그 구간으로 좁힌다
		FVector2D HighlightMin = AbsPos;
		FVector2D HighlightMax = AbsPos + AbsSize;
		if (!Entry.highlightFrom.IsEmpty())
		{
			FVector2D SubMin, SubMax;
			if (ComputeSubstringHighlight(Target, Entry.highlightFrom, SubMin, SubMax))
			{
				// 측정값이 어긋나도 위젯 밖으로 크게 삐져나가지는 않게 클램프.
				// 오른쪽 한계는 보정값(substringHighlightNudgeX)만큼 여유를 둬야 보정이 깎이지 않는다
				const float RightLimit = AbsPos.X + AbsSize.X + substringHighlightNudgeX;
				HighlightMin.X = FMath::Clamp(SubMin.X, AbsPos.X, RightLimit);
				HighlightMax.X = FMath::Clamp(SubMax.X, HighlightMin.X, RightLimit);
			}
		}

		Entry.targetMin = FVector2D(MyGeometry.AbsoluteToLocal(HighlightMin)) - FVector2D(targetPadding);
		Entry.targetMax = FVector2D(MyGeometry.AbsoluteToLocal(HighlightMax)) + FVector2D(targetPadding);

		GroupTop = FMath::Min(GroupTop, Entry.targetMin.Y);
		GroupBottom = FMath::Max(GroupBottom, Entry.targetMax.Y);
		if (Border && Entry.bHasHint)
		{
			MaxHintHeight = FMath::Max(MaxHintHeight, static_cast<float>(Border->GetCachedGeometry().GetLocalSize().Y));
		}

		VisibleIndices.Add(i);
	}

	if (VisibleIndices.Num() == 0)
	{
		return;
	}

	// 문구가 하나뿐인데 대상이 여럿이면(스탯창 증가분 여러 줄 등) 박스마다 화살표를 그리지 않고
	// 제일 위 박스 하나에서만 화살표를 올린다 — 같은 설명에 화살표가 여러 개 붙어 지저분해지는 것 방지.
	int32 TopIndex = VisibleIndices[0];
	int32 HintCount = 0;
	int32 SingleHintIndex = INDEX_NONE;
	for (int32 Index : VisibleIndices)
	{
		if (pointers[Index].targetMin.Y < pointers[TopIndex].targetMin.Y)
		{
			TopIndex = Index;
		}

		if (pointers[Index].bHasHint)
		{
			++HintCount;
			SingleHintIndex = Index;
		}
	}

	// 문구·화살표가 기준으로 삼을 박스 — 위 경우에만 제일 위 박스로 바뀌고, 그 외에는 자기 자신
	const bool bAnchorToTop = (HintCount == 1 && VisibleIndices.Num() > 1);
	auto AnchorIndexOf = [&](int32 Index)
		{
			return (bAnchorToTop && Index == SingleHintIndex) ? TopIndex : Index;
		};

	// 2) 문구 박스 배치 — 대상이 화면 위쪽에 붙어 있으면 아래에 두고, 그 외에는 위에서 아래로 가리킨다.
	// 대상이 여러 개면 같은 높이에 나란히 둔다.
	const FVector2D ScreenSize = FVector2D(MyGeometry.GetLocalSize());
	const bool bHintAbove = (GroupTop - arrowLength - MaxHintHeight) > screenMargin;
	const float BaselineY = bHintAbove ? GroupTop - arrowLength : GroupBottom + arrowLength;

	struct FHintLayout
	{
		int32 index;
		float width;
		float height;
		float left;
	};

	TArray<FHintLayout> Layouts;
	for (int32 Index : VisibleIndices)
	{
		const FTutorialPointerEntry& Entry = pointers[Index];
		const UBorder* Border = hintBorders.IsValidIndex(Index) ? hintBorders[Index].Get() : nullptr;
		if (!Border || !Entry.bHasHint)
		{
			continue;
		}

		const FTutorialPointerEntry& Anchor = pointers[AnchorIndexOf(Index)];
		const FVector2D HintSize = FVector2D(Border->GetCachedGeometry().GetLocalSize());
		const float TargetCenterX = (Anchor.targetMin.X + Anchor.targetMax.X) * 0.5f;
		Layouts.Add(FHintLayout{ Index, static_cast<float>(HintSize.X), static_cast<float>(HintSize.Y), TargetCenterX - static_cast<float>(HintSize.X) * 0.5f });
	}

	// 대상 중심 기준으로 정렬한 뒤, 왼쪽부터 겹침을 밀어내고 오른쪽 화면 끝을 넘으면 오른쪽부터 되민다.
	// 퀵슬롯 두 칸처럼 대상이 붙어 있으면 문구가 더 넓어서 겹치기 때문.
	Layouts.Sort([](const FHintLayout& A, const FHintLayout& B)
		{
			return (A.left + A.width * 0.5f) < (B.left + B.width * 0.5f);
		});

	for (int32 i = 0; i < Layouts.Num(); ++i)
	{
		const float MinLeft = (i == 0) ? screenMargin : Layouts[i - 1].left + Layouts[i - 1].width + hintSpacing;
		Layouts[i].left = FMath::Max(Layouts[i].left, MinLeft);
	}

	for (int32 i = Layouts.Num() - 1; i >= 0; --i)
	{
		const float MaxRight = (i == Layouts.Num() - 1) ? ScreenSize.X - screenMargin : Layouts[i + 1].left - hintSpacing;
		Layouts[i].left = FMath::Min(Layouts[i].left, MaxRight - Layouts[i].width);
	}

	for (const FHintLayout& Layout : Layouts)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(hintBorders[Layout.index]->Slot))
		{
			// Alignment.Y는 위에 둘 때 1(아래쪽 기준), 아래에 둘 때 0(위쪽 기준)
			CanvasSlot->SetAlignment(FVector2D(0.5f, bHintAbove ? 1.0f : 0.0f));
			CanvasSlot->SetPosition(FVector2D(Layout.left + Layout.width * 0.5f, bHintAbove ? BaselineY : BaselineY - Layout.height));
		}
	}

	// 3) 화살표 — 문구 박스 안에서 대상 쪽으로 가장 가까운 지점에서 출발한다.
	// 나란히 놓인 박스의 화살표끼리 교차하지 않고, 대상이 하나일 때는 종전처럼 박스 중앙에서 곧게 내려간다.
	for (int32 Index : VisibleIndices)
	{
		FTutorialPointerEntry& Entry = pointers[Index];
		Entry.bVisible = true;

		// 문구가 붙은 대상에만 화살표를 그린다 (문구가 아예 없는 단계는 종전대로 전부 그린다)
		Entry.bHasArrow = Entry.bHasHint || HintCount == 0;
		if (!Entry.bHasArrow)
		{
			continue;
		}

		const FTutorialPointerEntry& Anchor = pointers[AnchorIndexOf(Index)];
		const float TargetCenterX = (Anchor.targetMin.X + Anchor.targetMax.X) * 0.5f;

		float StartX = TargetCenterX;
		float HintHeight = 0.f;
		if (const FHintLayout* Layout = Layouts.FindByPredicate([Index](const FHintLayout& L) { return L.index == Index; }))
		{
			const float Inset = FMath::Min(10.f, Layout->width * 0.5f);
			StartX = FMath::Clamp(TargetCenterX, Layout->left + Inset, Layout->left + Layout->width - Inset);
			HintHeight = Layout->height;
		}

		if (bHintAbove)
		{
			Entry.arrowStart = FVector2D(StartX, BaselineY + 4.f);
			Entry.arrowEnd = FVector2D(TargetCenterX, Anchor.targetMin.Y);
		}
		else
		{
			Entry.arrowStart = FVector2D(StartX, BaselineY - HintHeight - 4.f);
			Entry.arrowEnd = FVector2D(TargetCenterX, Anchor.targetMax.Y);
		}
	}

	if (const UWorld* World = GetWorld())
	{
		pulseAlpha = 0.55f + 0.45f * FMath::Sin(World->GetTimeSeconds() * 5.f);
	}
}

int32 UC_TutorialPointerWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const int32 DrawLayer = MaxLayer + 1;
	const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();

	FLinearColor BoxColor = arrowColor;
	BoxColor.A = pulseAlpha;

	bool bDrewAny = false;

	for (const FTutorialPointerEntry& Entry : pointers)
	{
		if (!Entry.bVisible)
		{
			continue;
		}
		bDrewAny = true;

		// 1) 대상 위젯을 감싸는 강조 사각형 (깜빡임)
		TArray<FVector2D> BoxPoints;
		BoxPoints.Reserve(5);
		BoxPoints.Add(Entry.targetMin);
		BoxPoints.Add(FVector2D(Entry.targetMax.X, Entry.targetMin.Y));
		BoxPoints.Add(Entry.targetMax);
		BoxPoints.Add(FVector2D(Entry.targetMin.X, Entry.targetMax.Y));
		BoxPoints.Add(Entry.targetMin);
		FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer, PaintGeometry, BoxPoints,
			ESlateDrawEffect::None, BoxColor, true, lineThickness);

		if (!Entry.bHasArrow)
		{
			continue;
		}

		// 2) 화살표 몸통
		TArray<FVector2D> ShaftPoints;
		ShaftPoints.Reserve(2);
		ShaftPoints.Add(Entry.arrowStart);
		ShaftPoints.Add(Entry.arrowEnd);
		FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer, PaintGeometry, ShaftPoints,
			ESlateDrawEffect::None, arrowColor, true, lineThickness);

		// 3) 화살촉 — 진행 방향 기준 좌우 대각선 두 줄
		const FVector2D Dir = (Entry.arrowEnd - Entry.arrowStart).GetSafeNormal();
		if (!Dir.IsNearlyZero())
		{
			const FVector2D Perp(-Dir.Y, Dir.X);
			const FVector2D Base = Entry.arrowEnd - Dir * arrowHeadSize;
			const FVector2D Wing = Perp * (arrowHeadSize * 0.5f);

			TArray<FVector2D> HeadPoints;
			HeadPoints.Reserve(3);
			HeadPoints.Add(Base + Wing);
			HeadPoints.Add(Entry.arrowEnd);
			HeadPoints.Add(Base - Wing);
			FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer, PaintGeometry, HeadPoints,
				ESlateDrawEffect::None, arrowColor, true, lineThickness);
		}
	}

	return bDrewAny ? DrawLayer : MaxLayer;
}
