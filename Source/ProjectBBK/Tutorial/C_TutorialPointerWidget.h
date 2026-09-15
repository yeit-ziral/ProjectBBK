// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_TutorialPointerWidget.generated.h"

class UBorder;
class UTextBlock;
class AActor;

/**
 * 튜토리얼 단계에서 특정 HUD 위젯(궁극기 게이지 등)을 화살표로 가리키고
 * 보조 설명 문구를 띄우는 오버레이 위젯.
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

	// 화살표·문구를 모두 감춘다 (대상 없는 단계로 넘어갈 때)
	void ClearPointer();

	// 월드 액터(플레이어) 옆에 화살표 없는 말풍선 문구를 띄운다. 화면 좌표는 매 프레임 다시 투영한다.
	// PointAt(HUD 위젯 가리키기)과 독립적이라 둘을 동시에 띄울 수 있다.
	void ShowWorldCallout(AActor* AnchorActor, const FText& Text, const FSlateFontInfo& Font, const FSlateColor& TextColor);
	void HideWorldCallout();

	// 지금 가리키고 있는 대상 (없거나 파괴됐으면 nullptr)
	UWidget* GetTargetWidget() const { return targetWidget.Get(); }

	// 가리키는 대상이 지금 화면에 떠 있는지 (창이 닫혔으면 false)
	bool IsTargetOnScreen() const { return IsWidgetOnScreen(targetWidget.Get()); }

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
	// RebuildWidget에서 만든 위젯들 (GC 방지를 위해 UPROPERTY)
	UPROPERTY()
	TObjectPtr<UBorder> hintBorder;

	UPROPERTY()
	TObjectPtr<UTextBlock> hintText;

	UPROPERTY()
	TObjectPtr<UBorder> calloutBorder;

	UPROPERTY()
	TObjectPtr<UTextBlock> calloutText;

	// 말풍선을 따라 붙일 액터 — 캐릭터 교체·사망으로 사라질 수 있으므로 약참조
	TWeakObjectPtr<AActor> calloutActor;

	// 말풍선 위치를 앵커 액터의 화면 투영점으로 갱신 (투영 실패 시 숨김)
	void UpdateWorldCallout();

	// 앵커 액터(캡슐 중심) 투영점에서 화면 오른쪽으로 띄울 거리(px) — 몸을 가리지 않게 옆에 둔다
	static constexpr float calloutSideOffset = 70.f;

	// 가리킬 대상. 대상 위젯이 먼저 파괴될 수 있으므로 약참조.
	TWeakObjectPtr<UWidget> targetWidget;

	// NativeTick에서 계산하고 NativePaint에서 읽는 화면 좌표 (이 위젯의 로컬 공간)
	FVector2D arrowStart = FVector2D::ZeroVector;
	FVector2D arrowEnd = FVector2D::ZeroVector;
	FVector2D targetMin = FVector2D::ZeroVector;
	FVector2D targetMax = FVector2D::ZeroVector;

	// 대상 위젯의 캐시 지오메트리가 아직 0이면 그리지 않는다 (첫 프레임 대비)
	bool bPointerVisible = false;

	// 보조 문구가 있는지 — 대상이 화면에서 사라졌다 돌아올 때 문구 표시 여부를 되살리기 위함
	bool bHasHint = false;

	// 강조 표시 깜빡임 계수 (0~1)
	float pulseAlpha = 1.f;

	// 화살표 색 — 궁극기 게이지의 노란 계열과 맞춘다
	static const FLinearColor arrowColor;

	// 화살표 길이(px)와 대상 사각형 여백
	static constexpr float arrowLength = 96.f;
	static constexpr float targetPadding = 6.f;
	static constexpr float arrowHeadSize = 16.f;
	static constexpr float lineThickness = 3.f;
};
