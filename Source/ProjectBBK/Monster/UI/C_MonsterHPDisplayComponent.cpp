// Fill out your copyright notice in the Description page of Project Settings.

#include "C_MonsterHPDisplayComponent.h"
#include "../C_BaseMonster.h"
#include "../M_Gas/C_MonsterASC.h"
#include "../M_Gas/C_MonsterAttributeSet.h"
#include "../Manager/C_MonsterDataComponent.h"
#include "C_MonsterHPWidgetBase.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

UC_MonsterHPDisplayComponent::UC_MonsterHPDisplayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UC_MonsterHPDisplayComponent::InitializeWidgetClass(UWidgetComponent* InWidgetComp)
{
	if (!InWidgetComp || !hpWidgetClass) return;

	widgetComp = InWidgetComp;
	widgetComp->SetWidget(nullptr);
	widgetComp->SetWidgetClass(hpWidgetClass);
}

void UC_MonsterHPDisplayComponent::Initialize(AC_BaseMonster* InOwner, UWidgetComponent* InWidgetComp)
{
	ownerMonster = InOwner;
	widgetComp   = InWidgetComp;

	if (!ownerMonster || !widgetComp) return;

	UUserWidget* Widget = widgetComp->GetUserWidgetObject();
	if (!Widget) return;

	monsterHpWidget = Cast<UC_MonsterHPWidgetBase>(Widget);
	if (!monsterHpWidget) return;

	UC_MonsterAttributeSet* attrSet = ownerMonster->GetMonsterAttributeSet();
	if (attrSet)
	{
		monsterHpWidget->SetMaxHp(attrSet->GetmaxHP());
		monsterHpWidget->SetCurrentHp(attrSet->GetcurHP());
		monsterHpWidget->SetMaxGroggy(attrSet->GetmaxGroggy());
		monsterHpWidget->SetCurrentGroggy(attrSet->GetcurGroggy());
	}

	if (UC_MonsterDataComponent* dataComp = ownerMonster->GetDataComponent())
	{
		monsterHpWidget->SetMonsterLevel(ownerMonster->level);
		monsterHpWidget->SetMonsterName(FText::FromName(dataComp->GetRowName()));
	}

	BindAttributeDelegates();
}

void UC_MonsterHPDisplayComponent::UpdateWidgetRotation()
{
	if (!widgetComp) return;

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController) return;

	FRotator CameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
	widgetComp->SetWorldRotation(FRotator(0.0f, CameraRotation.Yaw + 180.0f, 0.0f));
}

void UC_MonsterHPDisplayComponent::ShowGroggyStatus()
{
	if (!monsterHpWidget) return;

	monsterHpWidget->SetGroggyActive(true);
	monsterHpWidget->SetStatusText(FText::FromString(TEXT("Groggy")));
}

void UC_MonsterHPDisplayComponent::ClearGroggyStatus()
{
	if (!monsterHpWidget) return;

	monsterHpWidget->SetGroggyActive(false);
	monsterHpWidget->ClearStatusText();
}

void UC_MonsterHPDisplayComponent::BindAttributeDelegates()
{
	UC_MonsterASC* asc = ownerMonster->GetMonsterASC();
	if (!asc) return;

	asc->GetGameplayAttributeValueChangeDelegate(
		UC_MonsterAttributeSet::GetcurHPAttribute()).AddUObject(this, &UC_MonsterHPDisplayComponent::OnHpChanged);

	asc->GetGameplayAttributeValueChangeDelegate(
		UC_MonsterAttributeSet::GetmaxHPAttribute()).AddUObject(this, &UC_MonsterHPDisplayComponent::OnMaxHpChanged);

	asc->GetGameplayAttributeValueChangeDelegate(
		UC_MonsterAttributeSet::GetcurGroggyAttribute()).AddUObject(this, &UC_MonsterHPDisplayComponent::OnGroggyChanged);

	asc->GetGameplayAttributeValueChangeDelegate(
		UC_MonsterAttributeSet::GetmaxGroggyAttribute()).AddUObject(this, &UC_MonsterHPDisplayComponent::OnMaxGroggyChanged);
}

void UC_MonsterHPDisplayComponent::OnHpChanged(const FOnAttributeChangeData& ChangeData)
{
	if (monsterHpWidget)
		monsterHpWidget->SetCurrentHp(ChangeData.NewValue);
}

void UC_MonsterHPDisplayComponent::OnMaxHpChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!monsterHpWidget) return;
	monsterHpWidget->SetMaxHp(ChangeData.NewValue);
	if (UC_MonsterAttributeSet* attrSet = ownerMonster->GetMonsterAttributeSet())
		monsterHpWidget->SetCurrentHp(attrSet->GetcurHP());
}

void UC_MonsterHPDisplayComponent::OnGroggyChanged(const FOnAttributeChangeData& ChangeData)
{
	if (monsterHpWidget)
		monsterHpWidget->SetCurrentGroggy(ChangeData.NewValue);
}

void UC_MonsterHPDisplayComponent::OnMaxGroggyChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!monsterHpWidget) return;
	monsterHpWidget->SetMaxGroggy(ChangeData.NewValue);
	if (UC_MonsterAttributeSet* attrSet = ownerMonster->GetMonsterAttributeSet())
		monsterHpWidget->SetCurrentGroggy(attrSet->GetcurGroggy());
}
