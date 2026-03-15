// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_MonsterHPWidgetBase.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTBBK_API UC_MonsterHPWidgetBase : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void SetMaxHp(float NewMaxHp);
	virtual void SetCurrentHp(float NewCurrentHp);

	void SetMaxGroggy(float NewMaxGroggy);
	void SetCurrentGroggy(float NewCurrentGroggy);

	void SetMonsterLevel(int32 NewLevel);
	virtual void SetMonsterName(const FText& NewName);

	void SetStatusText(const FText& NewStatusText);
	void ClearStatusText();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    float maxHp = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    float currentHp = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    float maxGroggy = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    float currentGroggy = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    int32 monsterLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    FText monsterName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    bool bUseStatusText = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    FText statusText;

    virtual void UpdateWidget() PURE_VIRTUAL(UC_MonsterHPWidgetBase::UpdateWidget, );
	
};
