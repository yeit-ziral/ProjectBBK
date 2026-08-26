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

	UFUNCTION(BlueprintCallable, Category = "Groggy")
	void AddGroggy(float GroggyAmount);

	// 게이지 누적을 건너뛰고 즉시 그로기 진입 (패링 성공 등).
	// Duration <= 0이면 groggyDuration을 사용한다.
	UFUNCTION(BlueprintCallable, Category = "Groggy")
	void ForceGroggy(float Duration = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Groggy")
	void ResetGroggy();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Groggy")
	UAnimMontage* groggyMontage = nullptr;

	// 그로기 유지 시간(초) — AddGroggy·ForceGroggy 공통 기본값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Groggy", meta = (ClampMin = "0.1"))
	float groggyDuration = 5.0f;

private:
	UPROPERTY()
	AC_BaseMonster* ownerMonster = nullptr;

	UPROPERTY()
	UC_MonsterHPDisplayComponent* hpDisplayComponent = nullptr;

	FTimerHandle groggyResetTimerHandle;

	// State.Dead / State.Groggy / State.Invincible 중 하나라도 있으면 false
	bool CanEnterGroggy() const;

	// EnterGroggyState + 해제 타이머 시작 (AddGroggy·ForceGroggy 공통 경로)
	void StartGroggy(float Duration);

	void EnterGroggyState();
	void ExitGroggyState();
};
