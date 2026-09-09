// Fill out your copyright notice in the Description page of Project Settings.

#include "C_TutorialPromptWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UC_TutorialPromptWidget::SetStep(const FText& Instruction, int32 StepIndex, int32 TotalSteps)
{
	if (InstructionText)
	{
		InstructionText->SetText(Instruction);
	}

	if (ProgressText)
	{
		ProgressText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), StepIndex + 1, TotalSteps)));
	}

	// 새 단계는 진행 표시를 모두 끈 상태에서 시작한다 — 이전 단계 값이 남지 않게
	// (Debugging Checklist #25·#51과 동일 계열). 표시가 필요한 단계는 컴포넌트가 다시 켠다.
	SetStepProgress(-1.f);
	SetStepCount(0, 0);

	OnStepChanged();
}

void UC_TutorialPromptWidget::SetStepProgress(float Ratio)
{
	if (!StepProgressBar)
	{
		return;
	}

	// 0은 "아직 시작 안 함"이라 빈 바를 보여줘야 한다 — 숨김은 음수로만 요청한다
	if (Ratio < 0.f)
	{
		StepProgressBar->SetPercent(0.f);
		StepProgressBar->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	StepProgressBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	StepProgressBar->SetPercent(FMath::Clamp(Ratio, 0.f, 1.f));
}

void UC_TutorialPromptWidget::SetStepCount(int32 Current, int32 Required)
{
	if (!CountText)
	{
		return;
	}

	// 한 번만 누르면 되는 단계에서 "1 / 1"은 정보가 없다 — 아예 숨긴다
	if (Required <= 1)
	{
		CountText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	CountText->SetVisibility(ESlateVisibility::HitTestInvisible);
	CountText->SetText(FText::FromString(
		FString::Printf(TEXT("%d / %d"), FMath::Clamp(Current, 0, Required), Required)));
}

void UC_TutorialPromptWidget::ShowCompleted(const FText& Message)
{
	if (InstructionText)
	{
		InstructionText->SetText(Message);
	}

	if (ProgressText)
	{
		ProgressText->SetVisibility(ESlateVisibility::Collapsed);
	}

	SetStepProgress(-1.f);
	SetStepCount(0, 0);
}

FSlateFontInfo UC_TutorialPromptWidget::GetInstructionFont() const
{
	return InstructionText ? InstructionText->GetFont() : FSlateFontInfo();
}

FSlateColor UC_TutorialPromptWidget::GetInstructionColor() const
{
	return InstructionText ? InstructionText->GetColorAndOpacity() : FSlateColor(FLinearColor::White);
}
