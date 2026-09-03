// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * PRD(Pseudo-Random Distribution) 판정 채널.
 *
 * [문제]
 * 순수 베르누이 시행은 발동률 10%라도 20연속 실패가 12.2% 확률로 일어난다.
 * 플레이어는 이걸 "확률이 이상하다"로 체감한다. 기댓값은 맞는데 분산이 너무 크다.
 *
 * [모델]
 * n번째 연속 실패 뒤의 성공 확률을 P(n) = min(1, C*n) 으로 선형 증가시킨다.
 * 성공하면 n을 0으로 되돌린다. C*n >= 1 이 되는 n에서는 반드시 성공하므로
 * 연속 실패 길이에 하드 상한이 생긴다.
 *
 * 주의: 목표 발동률 p를 그대로 C에 넣으면 안 된다. P(n)이 커지는 만큼 실제
 * 발동률은 p보다 훨씬 높아진다. C는 p로부터 역산해야 하고, 그게 SolveConstant다.
 *
 * 사용처: 크리티컬, 아이템 드랍, 스킬 추가 발동 등 "체감이 중요한" 확률 전반.
 */
struct PROJECTBBK_API FPrdChannel
{
	// 목표 발동률(장기 평균)을 지정한다. 내부 상수 C를 역산해 캐시한다.
	// 확률이 바뀔 때만 호출할 것 — 역산 비용이 O(1/C)라 매 타격 호출용이 아니다.
	void SetTargetChance(float InTargetChance);

	// 한 번 판정. 성공하면 연속 실패 카운터를 0으로 되돌린다.
	bool Roll();

	// 결정론적 테스트용. Sample은 [0,1) 구간의 균일 난수.
	// Roll()과 로직을 공유하므로 테스트가 실제 경로를 검증한다.
	bool RollWith(float Sample);

	void ResetStreak() { failCount = 0; }

	float GetTargetChance() const { return targetChance; }
	float GetConstant()     const { return prdConstant; }
	int32 GetFailCount()    const { return failCount; }

	// 상수 C일 때의 실제 장기 발동률. 역산 결과 검증에 쓴다.
	static float EffectiveChance(float C);

private:
	// p(C)가 C에 대해 단조증가라는 성질을 이용한 이분법.
	static float SolveConstant(float InTargetChance);

	float targetChance = 0.f;
	float prdConstant  = 0.f;   // 위의 C
	int32 failCount    = 0;     // 마지막 성공 이후 연속 실패 횟수
};
