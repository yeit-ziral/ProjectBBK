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

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

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

	// 그로기 지속 동안 curGroggy를 max → 0으로 선형 감소시키기 위한 상태.
	// 타이머 하나만으로는 게이지가 종료 순간에 툭 끊기므로 매 프레임 직접 보간한다.
	bool  bGroggyDraining   = false;
	float groggyElapsedTime = 0.0f;
	float groggyTotalTime   = 0.0f;

	// curGroggy를 남은 시간 비율에 맞춰 갱신 (TickComponent에서 호출)
	void TickGroggyDrain(float DeltaTime);

	// State.Dead / State.Groggy / State.Invincible 중 하나라도 있으면 false
	bool CanEnterGroggy() const;

	// EnterGroggyState + 해제 타이머 시작 (AddGroggy·ForceGroggy 공통 경로)
	void StartGroggy(float Duration);

	void EnterGroggyState();
	void ExitGroggyState();
};
