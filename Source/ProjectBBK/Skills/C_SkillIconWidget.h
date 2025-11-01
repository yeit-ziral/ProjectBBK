// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "C_SkillIconWidget.generated.h"

class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;

UCLASS()
class PROJECTBBK_API UC_SkillIconWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// 생성자
	UC_SkillIconWidget(const FObjectInitializer& ObjectInitializer);

protected:
	// 초기화 (BeginPlay 같은 것)
	virtual void NativeConstruct() override;

	// 매 프레임 업데이트
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// ===== UI 위젯들 (블루프린트에서 바인딩) =====
	UPROPERTY(meta = (BindWidget))
	UImage* img_SkillIcon;

	UPROPERTY(meta = (BindWidget))
	UImage* img_CooldownOverlay;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* txt_Cooldown;

	UFUNCTION(BlueprintCallable, Category = "Skill Icon")
	void SetSkillIcon(UTexture2D* IconTexture);

	UFUNCTION(BlueprintCallable, Category = "Skill Icon")
	void UpdateCooldown(float CurrentCooldown, float MaxCooldown);

	UFUNCTION(BlueprintCallable, Category = "Skill Icon")
	void SetCooldownVisible(bool bShow);

protected:
	UPROPERTY()
	UMaterialInstanceDynamic* cooldownMaterial;

	float currentCooldownTime;

	float maxCooldownTime;
};
