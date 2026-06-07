// Fill out your copyright notice in the Description page of Project Settings.

#include "C_SettingsWidget.h"
#include "C_BBKGameUserSettings.h"
#include "C_BBKGameInstance.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"

void UC_SettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
		CloseButton->OnClicked.AddDynamic(this, &UC_SettingsWidget::OnCloseClicked);

	if (UC_BBKGameUserSettings* Settings = UC_BBKGameUserSettings::Get())
	{
		if (MasterVolumeSlider) MasterVolumeSlider->SetValue(Settings->GetMasterVolume());
		if (BGMVolumeSlider)    BGMVolumeSlider->SetValue(Settings->GetBGMVolume());
		if (SFXVolumeSlider)    SFXVolumeSlider->SetValue(Settings->GetSFXVolume());

		UpdateVolumeText(MasterVolumeText, Settings->GetMasterVolume());
		UpdateVolumeText(BGMVolumeText,    Settings->GetBGMVolume());
		UpdateVolumeText(SFXVolumeText,    Settings->GetSFXVolume());
	}

	if (MasterVolumeSlider)
		MasterVolumeSlider->OnValueChanged.AddDynamic(this, &UC_SettingsWidget::OnMasterVolumeChanged);
	if (BGMVolumeSlider)
		BGMVolumeSlider->OnValueChanged.AddDynamic(this, &UC_SettingsWidget::OnBGMVolumeChanged);
	if (SFXVolumeSlider)
		SFXVolumeSlider->OnValueChanged.AddDynamic(this, &UC_SettingsWidget::OnSFXVolumeChanged);
}

void UC_SettingsWidget::OnCloseClicked()
{
	RemoveFromParent();
}

void UC_SettingsWidget::OnMasterVolumeChanged(float Value)
{
	if (UC_BBKGameUserSettings* Settings = UC_BBKGameUserSettings::Get())
		Settings->SetMasterVolume(Value);
	UpdateVolumeText(MasterVolumeText, Value);
	ApplyAndSave();
}

void UC_SettingsWidget::OnBGMVolumeChanged(float Value)
{
	if (UC_BBKGameUserSettings* Settings = UC_BBKGameUserSettings::Get())
		Settings->SetBGMVolume(Value);
	UpdateVolumeText(BGMVolumeText, Value);
	ApplyAndSave();
}

void UC_SettingsWidget::OnSFXVolumeChanged(float Value)
{
	if (UC_BBKGameUserSettings* Settings = UC_BBKGameUserSettings::Get())
		Settings->SetSFXVolume(Value);
	UpdateVolumeText(SFXVolumeText, Value);
	ApplyAndSave();
}

void UC_SettingsWidget::UpdateVolumeText(UTextBlock* TextBlock, float Value)
{
	if (TextBlock)
		TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f))));
}

void UC_SettingsWidget::ApplyAndSave()
{
	if (UC_BBKGameInstance* GI = Cast<UC_BBKGameInstance>(GetGameInstance()))
		GI->ApplyVolumeSettings();

	if (UC_BBKGameUserSettings* Settings = UC_BBKGameUserSettings::Get())
		Settings->SaveSettings();
}
