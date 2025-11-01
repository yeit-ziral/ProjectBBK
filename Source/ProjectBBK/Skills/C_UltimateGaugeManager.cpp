// Fill out your copyright notice in the Description page of Project Settings.


#include "C_UltimateGaugeManager.h"

// Sets default values for this component's properties
UC_UltimateGaugeManager::UC_UltimateGaugeManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	PrimaryComponentTick.TickInterval = 0.1f;  // 10Hz 업데이트

	maxGauge = 100.0f;
	currentGauge = 0.0f;
	bIsReady = false;
	bAutoIncrease = false;
	increaseRate = 0.1f;
}


// Called when the game starts
void UC_UltimateGaugeManager::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("UltimateGaugeComponent initialized"));
}


// Called every frame
void UC_UltimateGaugeManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAutoIncrease && currentGauge < maxGauge)
	{
		currentGauge -= increaseRate * DeltaTime;
		currentGauge = FMath::Max(currentGauge, 0.0f);

		// 게이지 변경 이벤트
		onGaugeChanged.Broadcast(GetGaugePercent());

		// Ready 상태 업데이트
		if (bIsReady && currentGauge < maxGauge)
		{
			bIsReady = false;
		}
	}
}

void UC_UltimateGaugeManager::AddGauge(float Amount)
{
	if (Amount <= 0.0f)
		return;

	float OldGauge = currentGauge;
	currentGauge = FMath::Clamp(currentGauge + Amount, 0.0f, maxGauge);

	UE_LOG(LogTemp, Log, TEXT("Gauge added: +%.1f (%.1f / %.1f)"), Amount, currentGauge, maxGauge);

	// 게이지 변경 이벤트
	onGaugeChanged.Broadcast(GetGaugePercent());

	// 100% 도달 체크
	if (!bIsReady && currentGauge >= maxGauge)
	{
		bIsReady = true;
		onGaugeFull.Broadcast();

		UE_LOG(LogTemp, Warning, TEXT("=== ULTIMATE READY! ==="));
	}
}

bool UC_UltimateGaugeManager::ConsumeGauge()
{
	if (!bIsReady || currentGauge < maxGauge)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot use ultimate: Not ready"));
		return false;
	}

	currentGauge = 0.0f;
	bIsReady = false;

	// 게이지 변경 이벤트
	onGaugeChanged.Broadcast(0.0f);

	UE_LOG(LogTemp, Warning, TEXT("Ultimate gauge consumed!"));

	return true;
}

void UC_UltimateGaugeManager::ResetGauge()
{
	currentGauge = 0.0f;
	bIsReady = false;

	onGaugeChanged.Broadcast(0.0f);

	UE_LOG(LogTemp, Log, TEXT("Gauge reset"));
}

bool UC_UltimateGaugeManager::IsUltimateReady() const
{
	return bIsReady && currentGauge >= maxGauge;
}

float UC_UltimateGaugeManager::GetGaugePercent() const
{
	if (maxGauge <= 0.0f)
		return 0.0f;

	return FMath::Clamp(currentGauge / maxGauge, 0.0f, 1.0f);
}

