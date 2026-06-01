// Fill out your copyright notice in the Description page of Project Settings.

#include "C_SettingsWidget.h"
#include "Components/Button.h"

void UC_SettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
		CloseButton->OnClicked.AddDynamic(this, &UC_SettingsWidget::OnCloseClicked);
}

void UC_SettingsWidget::OnCloseClicked()
{
	RemoveFromParent();
}
