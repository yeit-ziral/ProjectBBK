// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "C_GroggyComponent.generated.h"

class AC_BaseMonster;
class UC_MonsterHPDisplayComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTBBK_API UC_GroggyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UC_GroggyComponent();

	void Initialize(AC_BaseMonster* InOwner, UC_MonsterHPDisplayComponent* InHPDisplay);

	// BaseMonster::Tick에서 DeltaTime을 전달받아 호출
	void TickGroggy(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Groggy")
	void AddGroggy(float GroggyAmount);

	UFUNCTION(BlueprintCallable, Category = "Groggy")
	void ResetGroggy();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Groggy")
	UAnimMontage* groggyMontage = nullptr;

	// 초당 그로기 누적량
	UPROPERTY(EditDefaultsOnly, Category = "Groggy")
	float groggyAccumulationRate = 8.0f;

private:
	UPROPERTY()
	AC_BaseMonster* ownerMonster = nullptr;

	UPROPERTY()
	UC_MonsterHPDisplayComponent* hpDisplayComponent = nullptr;

	FTimerHandle groggyResetTimerHandle;

	void EnterGroggyState();
	void ExitGroggyState();
};
