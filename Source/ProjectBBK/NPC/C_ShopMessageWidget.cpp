// Fill out your copyright notice in the Description page of Project Settings.

#include "C_ShopMessageWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "TimerManager.h"

void UC_ShopMessageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ConfirmButton && !ConfirmButton->OnClicked.IsBound())
		ConfirmButton->OnClicked.AddDynamic(this, &UC_ShopMessageWidget::OnConfirmClicked);
}

void UC_ShopMessageWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(autoCloseTimer);

	Super::NativeDestruct();
}

void UC_ShopMessageWidget::ShowMessage(const FText& Message)
{
	if (MessageText)
		MessageText->SetText(Message);

	if (AutoCloseSeconds > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(autoCloseTimer);
			World->GetTimerManager().SetTimer(
				autoCloseTimer, this, &UC_ShopMessageWidget::Close, AutoCloseSeconds, false);
		}
	}
}

void UC_ShopMessageWidget::OnConfirmClicked()
{
	Close();
}

void UC_ShopMessageWidget::Close()
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(autoCloseTimer);

	RemoveFromParent();
}
