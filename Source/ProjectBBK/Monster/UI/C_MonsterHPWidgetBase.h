// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_MonsterHPWidgetBase.generated.h"

class UProgressBar;

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

	// 그로기 진입/해제 시 UC_MonsterHPDisplayComponent가 호출 — 그로기 바 색을 전환한다.
	void SetGroggyActive(bool bActive);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    float maxHp = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    float curHp = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    float maxGroggy = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    float curGroggy = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    int32 monsterLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    FText monsterName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    bool bUseStatusText = false;

    // 그로기 상태 동안 true — UpdateGroggyBar()가 바 색을 groggyActiveColor로 바꾼다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    bool bGroggyActive = false;

    // 그로기 상태일 때의 그로기 바 색 (평상시엔 BP에서 지정한 원래 색을 그대로 사용)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    FLinearColor groggyActiveColor = FLinearColor(1.0f, 0.82f, 0.05f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI")
    FText statusText;

    // ProgressBar의 Percent는 위젯 가로폭 0~100%를 그대로 잘라내는데(Bar Fill Style = Mask),
    // 채움 텍스처의 바 그림은 캔버스를 꽉 채우지 않고 투명 여백 안에 들어 있다.
    // 그래서 ratio를 그대로 넣으면 "HP 50%인데 반이 안 참 / 앞쪽 몇 %는 아예 안 보임"이 된다.
    // 아래 값은 채움 텍스처에서 바 그림이 차지하는 X 구간(0~1)이며,
    // MapFillPercent()로 ratio를 이 구간에 매핑해 실제 눈에 보이는 길이와 일치시킨다.
    // 텍스처를 교체하면 이 두 값도 함께 갱신할 것.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI|Fill Region", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float hpFillStart = 0.124066f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI|Fill Region", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float hpFillEnd = 0.858001f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI|Fill Region", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float groggyFillStart = 0.083551f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster UI|Fill Region", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float groggyFillEnd = 0.918190f;

    // Ratio(0~1)를 채움 텍스처의 실제 바 구간(Start~End)으로 매핑한다.
    static float MapFillPercent(float Ratio, float Start, float End);

    // 그로기 바의 Percent + 색을 한 번에 갱신한다. 일반/보스 위젯이 바 이름만 다르고 로직은 같아서 베이스에 둔다.
    // 최초 호출 시 BP에서 지정한 Fill Color를 originalGroggyBarColor에 캐시한다
    // (bGroggyActive는 false로 시작하므로 항상 평상시 색이 캐시된다).
    void UpdateGroggyBar(UProgressBar* Bar);

    virtual void UpdateWidget() PURE_VIRTUAL(UC_MonsterHPWidgetBase::UpdateWidget, );

private:
    FLinearColor originalGroggyBarColor = FLinearColor::White;
    bool bGroggyBarColorCached = false;
	
};
