// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "C_UltimateGaugeManager.generated.h"


// 델리게이트 (이벤트)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGaugeChanged, float, NewPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGaugeFull);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTBBK_API UC_UltimateGaugeManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UC_UltimateGaugeManager();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Ultimate Gauge")
	void AddGauge(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Ultimate Gauge")
	bool ConsumeGauge();

	UFUNCTION(BlueprintCallable, Category = "Ultimate Gauge")
	void ResetGauge();

	UFUNCTION(BlueprintPure, Category = "Ultimate Gauge")
	bool IsUltimateReady() const;

	UFUNCTION(BlueprintPure, Category = "Ultimate Gauge")
	float GetGaugePercent() const;

	UFUNCTION(BlueprintPure, Category = "Ultimate Gauge")
	float GetCurrentGauge() const { return currentGauge; }

	UFUNCTION(BlueprintPure, Category = "Ultimate Gauge")
	float GetMaxGauge() const { return maxGauge; }

	// ===== 델리게이트 (이벤트) =====
	UPROPERTY(BlueprintAssignable, Category = "Ultimate Gauge")
	FOnGaugeChanged onGaugeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Ultimate Gauge")
	FOnGaugeFull onGaugeFull;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Gauge")
	float maxGauge = 100.0f;

		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ultimate Gauge")
	float currentGauge = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ultimate Gauge")
	bool bIsReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Gauge")
	bool bAutoIncrease = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ultimate Gauge", meta = (EditCondition = "bAutoDecay"))
	float increaseRate = 5.0f;
};
