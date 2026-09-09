// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_TutorialPromptWidget.generated.h"

class UTextBlock;
class UProgressBar;

/**
 * 튜토리얼 안내 문구 위젯 — 화면 우상단 고정.
 * WBP_TutorialPrompt 루트는 Overlay(HAlign Right / VAlign Top)이며,
 * 위젯 전체 Visibility는 Self Hit Test Invisible로 두어 아래 UI 클릭을 막지 않는다.
 */
UCLASS()
class PROJECTBBK_API UC_TutorialPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 새 단계 표시. StepIndex는 0-based, 표시할 때 +1 된다.
	UFUNCTION(BlueprintCallable, Category = "Tutorial|UI")
	void SetStep(const FText& Instruction, int32 StepIndex, int32 TotalSteps);

	// 0~1 진행도. 음수면 바를 숨긴다 (진행 표시가 필요 없는 단계).
	UFUNCTION(BlueprintCallable, Category = "Tutorial|UI")
	void SetStepProgress(float Ratio);

	// 횟수 조건 단계의 "1 / 3" 카운터. Required가 1 이하면 숨긴다.
	UFUNCTION(BlueprintCallable, Category = "Tutorial|UI")
	void SetStepCount(int32 Current, int32 Required);

	// 모든 단계 완료 시 마지막으로 남길 문구
	UFUNCTION(BlueprintCallable, Category = "Tutorial|UI")
	void ShowCompleted(const FText& Message);

	// 단계가 바뀔 때마다 호출 — BP에서 등장 애니메이션 등에 사용
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial|UI")
	void OnStepChanged();

	// 화살표 위젯(UC_TutorialPointerWidget)이 같은 폰트를 쓰도록 넘겨준다.
	// 엔진 기본 폰트에는 한글 글리프가 없어 문구가 깨지므로 반드시 여기서 물려받아야 한다.
	FSlateFontInfo GetInstructionFont() const;
	FSlateColor GetInstructionColor() const;

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* InstructionText;

	// "3 / 17" 형태의 진행 표시 — 없어도 동작
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ProgressText;

	// 누적 시간 조건 단계에서만 채워짐 — 없어도 동작
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* StepProgressBar;

	// "1 / 3" 형태의 횟수 카운터 — 여러 번 눌러야 하는 단계에서만 표시. 없어도 동작
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* CountText;
};
