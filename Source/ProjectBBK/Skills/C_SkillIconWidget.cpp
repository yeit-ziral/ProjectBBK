// Fill out your copyright notice in the Description page of Project Settings.


#include "C_SkillIconWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"

UC_SkillIconWidget::UC_SkillIconWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	currentCooldownTime = 0.0f;
	maxCooldownTime = 0.0f;
	cooldownMaterial = nullptr;
}

void UC_SkillIconWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!img_SkillIcon)
	{
		UE_LOG(LogTemp, Error, TEXT("Img_SkillIcon is not bound!"));
	}

	if (!img_CooldownOverlay)
	{
		UE_LOG(LogTemp, Error, TEXT("Img_CooldownOverlay is not bound!"));
	}

	if (!txt_Cooldown)
	{
		UE_LOG(LogTemp, Error, TEXT("Txt_Cooldown is not bound!"));
	}

	if (img_CooldownOverlay)
	{
		UMaterialInterface* Material = img_CooldownOverlay->GetDynamicMaterial();
		if (Material)
		{
			cooldownMaterial = UMaterialInstanceDynamic::Create(Material, this);
			img_CooldownOverlay->SetBrushFromMaterial(cooldownMaterial);

			UE_LOG(LogTemp, Log, TEXT("Cooldown material created"));
		}
	}

	SetCooldownVisible(false);

}

void UC_SkillIconWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (currentCooldownTime > 0.0f)
	{
		currentCooldownTime -= InDeltaTime;

		if (currentCooldownTime <= 0.0f)
		{
			currentCooldownTime = 0.0f;
			SetCooldownVisible(false);
		}
		else
		{
			UpdateCooldown(currentCooldownTime, maxCooldownTime);
		}
	}
}

void UC_SkillIconWidget::SetSkillIcon(UTexture2D* IconTexture)
{
	if (img_SkillIcon && IconTexture)
	{
		img_SkillIcon->SetBrushFromTexture(IconTexture);

		UE_LOG(LogTemp, Log, TEXT("Skill icon set: %s"), *IconTexture->GetName());
	}
}

void UC_SkillIconWidget::UpdateCooldown(float CurrentCooldown, float MaxCooldown)
{
	if (MaxCooldown <= 0.0f) return;

	currentCooldownTime = CurrentCooldown;
	maxCooldownTime = MaxCooldown;

	// 진행률 계산 (0.0 ~ 1.0)
	float Progress = 1.0f - (CurrentCooldown / MaxCooldown);

	// 원형 머티리얼 업데이트
	if (cooldownMaterial)
	{
		cooldownMaterial->SetScalarParameterValue(TEXT("Progress"), Progress);
	}

	// 텍스트 업데이트
	if (txt_Cooldown)
	{
		int32 Seconds = FMath::CeilToInt(CurrentCooldown);
		FText CooldownText = FText::AsNumber(Seconds);
		txt_Cooldown->SetText(CooldownText);
	}

	// 쿨타임 중이면 표시
	if (CurrentCooldown > 0.0f)
	{
		SetCooldownVisible(true);
	}
}

void UC_SkillIconWidget::SetCooldownVisible(bool bShow)
{
	if (img_CooldownOverlay)
	{
		ESlateVisibility OverlayVisibility = bShow ?
			ESlateVisibility::Visible :
			ESlateVisibility::Collapsed;

		img_CooldownOverlay->SetVisibility(OverlayVisibility);
	}

	if (txt_Cooldown)
	{
		ESlateVisibility OverlayVisibility = bShow ?
			ESlateVisibility::Visible :
			ESlateVisibility::Collapsed;

		txt_Cooldown->SetVisibility(OverlayVisibility);
	}
}
