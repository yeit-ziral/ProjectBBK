// Fill out your copyright notice in the Description page of Project Settings.


#include "C_FloatingTextWidget.h"
#include "Components/TextBlock.h"

void UC_FloatingTextWidget::SetAlpha(float InAlpha)
{
	SetRenderOpacity(InAlpha);
}

void UC_FloatingTextWidget::SetTextAndColor(const FText& InText, const FLinearColor& InColor)
{
	if (!txtValue)
		return;

	txtValue->SetText(InText);
	txtValue->SetColorAndOpacity(FSlateColor(InColor));
}
