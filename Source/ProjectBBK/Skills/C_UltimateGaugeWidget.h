// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystemComponent.h"
#include "C_UltimateGaugeWidget.generated.h"

struct FOnAttributeChangeData;

class UProgressBar;
class UTextBlock;
class UImage;

UCLASS()
class PROJECTBBK_API UC_UltimateGaugeWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UC_UltimateGaugeWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

public:
	// ===== UI Elements (BindWidget) =====
	UPROPERTY(meta = (BindWidget))
	UProgressBar* bar_Gauge;

	UPROPERTY(meta = (BindWidget))
	UImage* img_Glow;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* txt_Percent;

	// ===== Public Functions =====
	UFUNCTION(BlueprintCallable, Category = "Ultimate Gauge")
	void UpdateGauge(float percent);

	UFUNCTION(BlueprintCallable, Category = "Ultimate Gauge")
	void SetGaugeReady(bool bIsReady);

	UFUNCTION(BlueprintCallable, Category = "Ultimate Gauge")
	void PlayReadyAnimation();

private:
	void InitializeGauge(); 

	// Mana 변경 콜백
	void OnManaChanged(const FOnAttributeChangeData& Data);

	// ASC 캐싱
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	// Delegate Handle (언바인딩용)
	FDelegateHandle ManaChangedHandle;

protected:
	// ===== Settings =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Gauge|Appearance")
	FLinearColor gaugeColor = FLinearColor(1.0f, 0.8f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Gauge|Appearance")
	FLinearColor readyColor = FLinearColor(0.0f, 1.0f, 0.5f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Gauge|Appearance")
	bool bEnableGlowEffect = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Gauge|Sound")
	bool bPlayReadySound = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Gauge|Sound")
	USoundBase* readySound;

private:
	float currentPercent = 0.0f;
	bool bIsReady = false;
	float glowAnimTime = 0.0f;
};
