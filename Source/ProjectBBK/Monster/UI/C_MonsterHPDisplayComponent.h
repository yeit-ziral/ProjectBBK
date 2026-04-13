// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilitySystemComponent.h"
#include "C_MonsterHPDisplayComponent.generated.h"

class AC_BaseMonster;
class UWidgetComponent;
class UC_MonsterHPWidgetBase;
class UUserWidget;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTBBK_API UC_MonsterHPDisplayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UC_MonsterHPDisplayComponent();

	// PostInitializeComponents에서 호출 — 위젯 클래스 설정
	void InitializeWidgetClass(UWidgetComponent* InWidgetComp);

	// BeginPlay에서 DataComponent 초기화 후 호출 — 위젯 값 바인딩
	void Initialize(AC_BaseMonster* InOwner, UWidgetComponent* InWidgetComp);

	// Tick에서 호출 — 위젯이 카메라를 향하도록 회전
	void UpdateWidgetRotation();

	UC_MonsterHPWidgetBase* GetWidget() const { return monsterHpWidget; }

	// GroggyComponent에서 호출
	void ShowGroggyStatus();
	void ClearGroggyStatus();

protected:
	// 각 몬스터 BP에서 설정 (일반: BPC_NormalMonsterHPWidget, 보스: 별도 처리)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> hpWidgetClass;

private:
	UPROPERTY()
	UWidgetComponent* widgetComp = nullptr;

	UPROPERTY()
	UC_MonsterHPWidgetBase* monsterHpWidget = nullptr;

	UPROPERTY()
	AC_BaseMonster* ownerMonster = nullptr;

	void BindAttributeDelegates();

	void OnHpChanged(const FOnAttributeChangeData& ChangeData);
	void OnMaxHpChanged(const FOnAttributeChangeData& ChangeData);
	void OnGroggyChanged(const FOnAttributeChangeData& ChangeData);
	void OnMaxGroggyChanged(const FOnAttributeChangeData& ChangeData);
};
