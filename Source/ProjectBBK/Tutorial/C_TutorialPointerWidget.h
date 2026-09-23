// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_TutorialPointerWidget.generated.h"

class UBorder;
class UTextBlock;
class UCanvasPanel;
class AActor;

// 가리키는 대상 하나의 상태 — NativeTick에서 계산하고 NativePaint에서 읽는다
struct FTutorialPointerEntry
{
	// 대상 위젯이 먼저 파괴될 수 있으므로 약참조
	TWeakObjectPtr<UWidget> target;

	// 보조 문구가 있는지 — 대상이 화면에서 사라졌다 돌아올 때 문구 표시 여부를 되살리기 위함
	bool bHasHint = false;

	// 대상의 캐시 지오메트리가 아직 0이거나 화면 밖이면 그리지 않는다
	bool bVisible = false;

	// 이 대상에 화살표를 그릴지 — 문구가 하나뿐이면 제일 위 박스 하나에만 그린다
	bool bHasArrow = false;

	// 비어 있지 않고 대상이 UTextBlock이면, 강조 사각형을 이 문자열이 시작되는 지점부터 텍스트 끝까지로 좁힌다.
	// 스탯창의 "120 (+20)"에서 "(+20)"만 감싸기 위한 용도.
	FString highlightFrom;

	// 이 위젯의 로컬 공간 좌표
	FVector2D arrowStart = FVector2D::ZeroVector;
	FVector2D arrowEnd = FVector2D::ZeroVector;
	FVector2D targetMin = FVector2D::ZeroVector;
	FVector2D targetMax = FVector2D::ZeroVector;
};

/**
 * 튜토리얼 단계에서 특정 HUD 위젯(궁극기 게이지 등)을 화살표로 가리키고
 * 보조 설명 문구를 띄우는 오버레이 위젯. 대상을 여러 개 동시에 가리킬 수 있다(퀵슬롯 두 칸 등).
 *
 * .uasset 없이 C++만으로 동작한다 — RebuildWidget에서 Canvas+Border+Text 트리를
 * 직접 만들고, 화살표와 강조 사각형은 NativePaint에서 선으로 그린다.
 * 폰트는 WBP_TutorialPrompt의 InstructionText에서 복사해 오므로 한글이 깨지지 않는다.
 *
 * AddToViewport 기본 앵커가 (0,0,1,1) 전체 화면 스트레치이므로(Debugging Checklist #56)
 * 위치·크기를 따로 지정하지 않고 그대로 화면 전체를 좌표계로 쓴다.
 * Visibility는 HitTestInvisible — 아래 HUD 클릭을 막지 않는다.
 */
UCLASS()
class PROJECTBBK_API UC_TutorialPointerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 가리킬 대상과 문구를 지정한다.
	 * @param TargetWidget  화면상의 대상 위젯. nullptr이면 화살표가 사라진다.
	 * @param Hint          화살표 옆에 띄울 설명 문구
	 * @param Font          문구에 쓸 폰트 (유효하지 않으면 기본값 유지)
	 * @param TextColor     문구 색
	 */
	void PointAt(UWidget* TargetWidget, const FText& Hint, const FSlateFontInfo& Font, const FSlateColor& TextColor);

	/**
	 * 여러 대상을 동시에 가리킨다 — 대상마다 강조 사각형·화살표·문구를 따로 그린다.
	 * 문구 박스는 같은 높이에 나란히 두고, 서로 겹치지 않게 좌우로 벌린다.
	 * @param Targets  화면상의 대상 위젯들. nullptr 항목은 건너뛴다.
	 * @param Hints    Targets와 인덱스 1:1. 비어 있거나 모자라면 그 대상은 문구 없이 화살표만 그린다.
	 *                 문구가 하나뿐이고 대상이 여럿이면 화살표는 제일 위 대상 하나에만 그린다.
	 * @param HighlightMarkers  Targets와 인덱스 1:1(선택). 항목이 비어 있지 않고 대상이 UTextBlock이면
	 *                          강조 사각형을 그 문자열이 시작되는 지점부터 텍스트 끝까지로 좁힌다.
	 */
	void PointAtMultiple(const TArray<UWidget*>& Targets, const TArray<FText>& Hints, const FSlateFontInfo& Font, const FSlateColor& TextColor,
		const TArray<FString>& HighlightMarkers = TArray<FString>());

	// 화살표·문구를 모두 감춘다 (대상 없는 단계로 넘어갈 때)
	void ClearPointer();

	// 월드 액터(플레이어) 옆에 화살표 없는 말풍선 문구를 띄운다. 화면 좌표는 매 프레임 다시 투영한다.
	// PointAt(HUD 위젯 가리키기)과 독립적이라 둘을 동시에 띄울 수 있다.
	void ShowWorldCallout(AActor* AnchorActor, const FText& Text, const FSlateFontInfo& Font, const FSlateColor& TextColor);
	void HideWorldCallout();

	// 지금 가리키고 있는 첫 번째 대상 (없거나 파괴됐으면 nullptr)
	UWidget* GetTargetWidget() const { return pointers.Num() > 0 ? pointers[0].target.Get() : nullptr; }

	// 지금 가리키고 있는 대상 수
	int32 GetTargetCount() const { return pointers.Num(); }

	// 지금 가리키는 대상이 Targets와 순서까지 같은지 — 재탐색에서 같은 대상을 다시 세팅해 깜빡이지 않게
	bool IsPointingAt(const TArray<UWidget*>& Targets) const;

	// 가리키는 대상이 전부 화면에 떠 있는지 (하나라도 창이 닫혔거나, 대상이 없으면 false)
	bool IsTargetOnScreen() const;

	// 위젯이 뷰포트에 붙은 위젯 트리 안에서 보이는 상태인지.
	// 닫힌 창(RemoveFromParent)은 인스턴스와 캐시 지오메트리가 남아 있어도 false —
	// 부모 패널과 소유 UUserWidget을 따라 뷰포트에 붙은 루트까지 올라가며 확인한다.
	static bool IsWidgetOnScreen(const UWidget* Widget);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	// 네이티브 전용 UUserWidget이라 디자이너 트리가 없다 — 루트 캔버스·말풍선을 직접 만든다 (이미 있으면 무동작)
	void EnsureWidgetTree();

	// Count개까지 문구 박스를 확보한다. 대상 수가 줄어도 지우지 않고 남는 박스는 감춘다.
	void EnsureHintSlots(int32 Count);

	// 말풍선 위치를 앵커 액터의 화면 투영점으로 갱신 (투영 실패 시 숨김)
	void UpdateWorldCallout();

	// 텍스트 위젯 안에서 Marker가 시작되는 지점부터 끝까지의 절대 좌표 사각형.
	// 대상이 UTextBlock이 아니거나 Marker가 없으면 false — 이때는 위젯 전체를 감싼다.
	static bool ComputeSubstringHighlight(const UWidget* Target, const FString& Marker, FVector2D& OutAbsMin, FVector2D& OutAbsMax);

	UPROPERTY()
	TObjectPtr<UCanvasPanel> rootCanvas;

	// 대상별 문구 박스 — pointers와 인덱스 1:1 (GC 방지를 위해 UPROPERTY)
	UPROPERTY()
	TArray<TObjectPtr<UBorder>> hintBorders;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> hintTexts;

	UPROPERTY()
	TObjectPtr<UBorder> calloutBorder;

	UPROPERTY()
	TObjectPtr<UTextBlock> calloutText;

	// 말풍선을 따라 붙일 액터 — 캐릭터 교체·사망으로 사라질 수 있으므로 약참조
	TWeakObjectPtr<AActor> calloutActor;

	// 앵커 액터(캡슐 중심) 투영점에서 화면 오른쪽으로 띄울 거리(px) — 몸을 가리지 않게 옆에 둔다
	static constexpr float calloutSideOffset = 70.f;

	TArray<FTutorialPointerEntry> pointers;

	// 강조 표시 깜빡임 계수 (0~1)
	float pulseAlpha = 1.f;

	// 화살표 색 — 궁극기 게이지의 노란 계열과 맞춘다
	static const FLinearColor arrowColor;

	// 부분 문자열 강조 박스를 오른쪽으로 밀어주는 보정값(화면 px). 1080p 기준 1cm ≈ 38px.
	// 폰트 측정 폭이 실제 글리프 시작점과 조금 어긋나는 것을 눈으로 맞춘 값이라 정렬을 바꾸면 다시 조정 필요.
	static constexpr float substringHighlightNudgeX = 30.8f;   // 27 + 0.1cm(3.8px)

	// 화살표 길이(px)와 대상 사각형 여백
	static constexpr float arrowLength = 96.f;
	static constexpr float targetPadding = 6.f;
	static constexpr float arrowHeadSize = 16.f;
	static constexpr float lineThickness = 3.f;

	// 나란히 놓인 문구 박스 사이의 최소 간격, 화면 가장자리 여백(px)
	static constexpr float hintSpacing = 12.f;
	static constexpr float screenMargin = 12.f;
};
