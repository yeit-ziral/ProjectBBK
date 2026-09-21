// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectBBK/GAS/Abilities/C_CharacterGA.h"
#include "C_MeleeAttackGA.generated.h"

/**
 * 근접 평타 어빌리티. 콤보 상태(몇 단인지, 입력이 버퍼됐는지)를 여기서 관리하고,
 * 몽타주 재생·VFX 같은 연출은 BP 자식(GA_MeleeAttack)이 담당한다.
 */
UCLASS()
class PROJECTBBK_API UC_MeleeAttackGA : public UC_CharacterGA
{
	GENERATED_BODY()

public:
	// 콤보 단계별 몽타주. 배열 길이가 곧 최대 단계다.
	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	TArray<TObjectPtr<UAnimMontage>> comboMontages;

	// 어빌리티 시작 시 호출. 단계와 버퍼를 초기화한다.
	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ResetCombo();

	// Event.Combo.Input 수신 시 호출. 입력을 버퍼에 담는다.
	UFUNCTION(BlueprintCallable, Category = "Combo")
	void BufferComboInput();

	// Event.Combo.Window 수신 시 호출. 다음 단으로 갈 수 있으면 comboIndex를 올리고 true를 반환한다.
	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool TryAdvanceCombo();

	// 현재 단계의 몽타주. 없으면 nullptr.
	UFUNCTION(BlueprintPure, Category = "Combo")
	UAnimMontage* GetCurrentComboMontage() const;

	// On Interrupted에서 호출. "콤보 전환 때문이었나"를 묻고 플래그를 소모한다. true면 무시하고, false면 외부 방해이므로 EndAbility 해야 한다.
	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool ConsumeAdvancingFlag();

	// 이번 스윙에서 처음 맞는 대상이면 기록하고 true, 이미 맞았으면 false. Event.hit 수신 시 데미지 체인 앞에서 호출한다.
	UFUNCTION(BlueprintCallable, Category = "Combo")
	bool TryRegisterHit(AActor* HitActor);

	// 새 스윙이 시작될 때 호출. 맞은 대상 목록을 비운다.
	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ClearSwingHits();

protected:
	/** 현재 콤보 단계(0부터 시작) */
	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	int32 comboIndex = 0;

	/** 입력 버퍼링 여부 */
	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	bool bIsInputBuffered = false;
	
	/** 콤보 입력 버퍼링 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	float inputBufferTime = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	TArray<float> comboDamageMultipliers;

	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	int32 comboLoopStartIndex = 1;

	bool bAdvancingCombo = false;

	/** 버퍼된 입력의 유효 시간을 재는 타이머 핸들 */
	FTimerHandle comboInputBufferTimerHandle;

	/** 유효 시간이 지나면 버퍼를 비운다 */
	void OnComboInputBufferTimerExpired();

	// 이번 스윙에 이미 맞은 대상. 약한 참조라 몬스터가 죽어 파괴돼도 댕글링 포인터가 되지 않는다.
	TSet<TWeakObjectPtr<AActor>> swingHitActors;
};
