// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_FloatingTextWidget.generated.h"

class UTextBlock;

/* 몬스터 위에 잠깐 떴다 사라지는 텍스트. 표시만 담당하고 움직임은 액터가 계산한다. */

UCLASS()
class PROJECTBBK_API UC_FloatingTextWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// 액터가 매 틱 호출. 위젯 트리 전체에 적용되므로 내용이 Text든 Image든 동작한다.
	void SetAlpha(float InAlpha);

	// 텍스트를 쓰는 경우에만 의미가 있다. 이미지 전용 위젯이면 아무 일도 하지 않는다.
	void SetTextAndColor(const FText& InText, const FLinearColor& InColor);

protected:
	// Optional이라 WBP에 이 이름의 TextBlock이 없어도 컴파일된다. "CRITICAL!" 이미지 버전과 데미지 숫자 텍스트 버전을 같은 클래스로 쓰기 위함.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> txtValue;
};
