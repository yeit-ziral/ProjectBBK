// Fill out your copyright notice in the Description page of Project Settings.

#include "C_MerchantDialogueWidget.h"
#include "C_MerchantNPC.h"
#include "../PlayerCharacter/PlayerAI/C_PlayerController.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UC_MerchantDialogueWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BuyButton && !BuyButton->OnClicked.IsBound())
		BuyButton->OnClicked.AddDynamic(this, &UC_MerchantDialogueWidget::OnBuyClicked);

	if (LeaveButton && !LeaveButton->OnClicked.IsBound())
		LeaveButton->OnClicked.AddDynamic(this, &UC_MerchantDialogueWidget::OnLeaveClicked);
}

void UC_MerchantDialogueWidget::Setup(AC_MerchantNPC* NPC, AC_PlayerController* PC)
{
	merchant = NPC;
	ownerPC  = PC;

	lines.Reset();
	if (NPC)
		lines = NPC->GetDialogueLines();

	currentLine   = 0;
	bChoicesShown = false;

	// 선택지 숨김 (대사부터)
	if (ChoicePanel)
		ChoicePanel->SetVisibility(ESlateVisibility::Collapsed);

	if (lines.Num() > 0)
	{
		if (DialogueText)
			DialogueText->SetText(lines[0]);
	}
	else
	{
		// 대사가 없으면 바로 선택지로
		if (DialogueText)
			DialogueText->SetText(FText::GetEmpty());
		ShowChoices();
	}
}

FReply UC_MerchantDialogueWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !bChoicesShown)
	{
		AdvanceDialogue();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UC_MerchantDialogueWidget::AdvanceDialogue()
{
	if (currentLine < lines.Num() - 1)
	{
		++currentLine;
		if (DialogueText)
			DialogueText->SetText(lines[currentLine]);
	}
	else
	{
		ShowChoices();
	}
}

void UC_MerchantDialogueWidget::ShowChoices()
{
	bChoicesShown = true;
	if (ChoicePanel)
		ChoicePanel->SetVisibility(ESlateVisibility::Visible);
}

void UC_MerchantDialogueWidget::OnBuyClicked()
{
	if (ownerPC.IsValid())
		ownerPC->OnDialogueBuyChosen(merchant.Get());
}

void UC_MerchantDialogueWidget::OnLeaveClicked()
{
	if (ownerPC.IsValid())
		ownerPC->CloseMerchantDialogue();
}
