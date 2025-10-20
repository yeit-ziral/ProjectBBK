// Fill out your copyright notice in the Description page of Project Settings.


#include "C_UltimateGaugeWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UC_UltimateGaugeWidget::UC_UltimateGaugeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	currentPercent = 0.0f;
	bIsReady = false;
	glowAnimTime = 0.0f;

	gaugeColor = FLinearColor(1.0f, 0.8f, 0.0f, 1.0f); // Gold
	readyColor = FLinearColor(0.0f, 1.0f, 0.5f, 1.0f); // Green
	bEnableGlowEffect = true;
	bPlayReadySound = true;
}

void UC_UltimateGaugeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Widget binding check
	if (!bar_Gauge)
	{
		UE_LOG(LogTemp, Error, TEXT("Bar_Gauge is not bound!"));
	}

	if (!txt_Percent)
	{
		UE_LOG(LogTemp, Error, TEXT("Txt_Percent is not bound!"));
	}

	if (!img_Glow)
	{
		UE_LOG(LogTemp, Warning, TEXT("Img_Glow is not bound (optional)"));
	}

	// Initial state
	UpdateGauge(0.0f);
	SetGaugeReady(false);

	UE_LOG(LogTemp, Log, TEXT("UltimateGaugeWidget constructed"));
}

void UC_UltimateGaugeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Glow animation (pulsating effect)
	if (bIsReady && img_Glow && bEnableGlowEffect)
	{
		glowAnimTime += InDeltaTime * 2.0f; // 2x speed

		// Sine wave (0.5 ~ 1.0)
		float opacity = 0.75f + FMath::Sin(glowAnimTime) * 0.25f;

		FLinearColor glowColor = readyColor;
		glowColor.A = opacity;

		img_Glow->SetColorAndOpacity(glowColor);
	}
}

void UC_UltimateGaugeWidget::UpdateGauge(float percent)
{
	currentPercent = FMath::Clamp(percent, 0.0f, 1.0f);

	// Update progress bar
	if (bar_Gauge)
	{
		bar_Gauge->SetPercent(currentPercent);

		// Color change based on percentage
		if (currentPercent >= 1.0f)
		{
			bar_Gauge->SetFillColorAndOpacity(readyColor);
		}
		else
		{
			bar_Gauge->SetFillColorAndOpacity(gaugeColor);
		}
	}

	// Update text
	if (txt_Percent)
	{
		int32 percentValue = FMath::RoundToInt(currentPercent * 100.0f);
		FText percentText = FText::Format(
			FText::FromString(TEXT("{0}%")),
			FText::AsNumber(percentValue)
		);
		txt_Percent->SetText(percentText);
	}

	// Check if ready
	bool newReadyState = (currentPercent >= 1.0f);
	if (newReadyState != bIsReady)
	{
		SetGaugeReady(newReadyState);
	}
}

void UC_UltimateGaugeWidget::SetGaugeReady(bool bReady)
{
	if (bReady == bIsReady)
		return;

	bReady = bIsReady;

	if (bReady)
	{
		UE_LOG(LogTemp, Warning, TEXT("=== ULTIMATE GAUGE READY! ==="));

		// Show glow effect
		if (img_Glow && bEnableGlowEffect)
		{
			img_Glow->SetVisibility(ESlateVisibility::Visible);
			glowAnimTime = 0.0f;
		}

		// Play animation
		PlayReadyAnimation();

		// Play sound
		if (bPlayReadySound && readySound)
		{
			UGameplayStatics::PlaySound2D(this, readySound);
		}
	}
	else
	{
		// Hide glow effect
		if (img_Glow)
		{
			img_Glow->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UC_UltimateGaugeWidget::PlayReadyAnimation()
{
	// Blueprint에서 애니메이션 재생
	// 또는 여기서 UWidgetAnimation 사용

	UE_LOG(LogTemp, Log, TEXT("PlayReadyAnimation called"));

	// TODO: UWidgetAnimation 재생
	// if (ReadyAnimation)
	// {
	//     PlayAnimation(ReadyAnimation);
	// }
}
