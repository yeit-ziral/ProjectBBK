// Fill out your copyright notice in the Description page of Project Settings.

#include "C_Probability.h"

float FPrdChannel::EffectiveChance(float C)
{
	if (C <= 0.f) return 0.f;
	if (C >= 1.f) return 1.f;

	// 첫 성공까지 걸린 시행 횟수 N의 기댓값을 직접 세고, 발동률 = 1 / E[N] 로 얻는다.
	//   P(n)  = min(1, C*n)              n번째 시행의 성공 확률
	//   surv  = ∏_{i<n} (1 - P(i))       n-1회까지 전부 실패했을 확률
	//   f(n)  = surv * P(n)              정확히 n번째에 첫 성공할 확률
	//   E[N]  = Σ n * f(n)
	// C*n >= 1 이 되는 n에서 반드시 성공하므로 합은 유한하다. 무한급수가 아니다.
	const int32 nMax = FMath::CeilToInt(1.f / C);

	// float은 누적곱에서 정밀도가 빠르게 무너진다. 여기만 double로 계산한다.
	double surv      = 1.0;
	double expectedN = 0.0;

	for (int32 n = 1; n <= nMax; ++n)
	{
		const double pn = FMath::Min(1.0, static_cast<double>(C) * n);
		expectedN += n * surv * pn;
		surv *= (1.0 - pn);

		// 남은 확률질량이 무시할 수준이면 조기 종료. C가 아주 작을 때
		// nMax가 수십만까지 커지는 걸 막아준다(오차는 1e-12 * n 수준).
		if (surv < 1e-12)
		{
			break;
		}
	}

	return expectedN > 0.0 ? static_cast<float>(1.0 / expectedN) : 0.f;
}

float FPrdChannel::SolveConstant(float InTargetChance)
{
	if (InTargetChance <= 0.f) return 0.f;
	if (InTargetChance >= 1.f) return 1.f;

	// p(C)는 닫힌 해가 없다. 대신 C에 대해 단조증가라서 이분법이 항상 수렴한다.
	// 40회면 구간 폭이 2^-40 ≈ 9e-13 으로 float 정밀도를 이미 넘어선다.
	double lo = 0.0;
	double hi = 1.0;

	for (int32 i = 0; i < 40; ++i)
	{
		const double mid = 0.5 * (lo + hi);
		if (EffectiveChance(static_cast<float>(mid)) < InTargetChance)
		{
			lo = mid;
		}
		else
		{
			hi = mid;
		}
	}

	return static_cast<float>(0.5 * (lo + hi));
}

void FPrdChannel::SetTargetChance(float InTargetChance)
{
	targetChance = FMath::Clamp(InTargetChance, 0.f, 1.f);
	prdConstant  = SolveConstant(targetChance);

	// failCount는 일부러 유지한다. "연속 n회 실패"라는 사실은 C가 바뀌어도
	// 그대로 유효하고, 초기화하면 플레이어가 쌓은 보정을 뺏는 꼴이 된다.
}

bool FPrdChannel::RollWith(float Sample)
{
	if (prdConstant <= 0.f)
	{
		return false;
	}

	// 이번이 (failCount + 1)번째 시도 → P = C * (failCount + 1)
	const float chance = FMath::Min(1.f, prdConstant * (failCount + 1));

	if (Sample < chance)
	{
		failCount = 0;
		return true;
	}

	++failCount;
	return false;
}

bool FPrdChannel::Roll()
{
	// FRand는 "균일 난수 소스"로만 쓴다. 분포 성형(pity 곡선)은 위에서 직접 했다.
	return RollWith(FMath::FRand());
}


#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPrdChannelTest, "ProjectBBK.Core.PrdChannel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPrdChannelTest::RunTest(const FString& Parameters)
{
	// 1) 역산 검증: C를 구한 뒤 다시 발동률을 계산하면 목표값으로 돌아와야 한다.
	for (float Target : { 0.05f, 0.10f, 0.15f, 0.20f, 0.25f, 0.30f, 0.50f })
	{
		FPrdChannel Ch;
		Ch.SetTargetChance(Target);
		const float Back = FPrdChannel::EffectiveChance(Ch.GetConstant());
		TestTrue(FString::Printf(TEXT("역산 왕복 p=%.2f -> C=%.6f -> p=%.6f"),
			Target, Ch.GetConstant(), Back), FMath::Abs(Back - Target) < 1e-4f);
	}

	// 2) 연속 실패 상한: C*n >= 1 이면 반드시 성공해야 한다.
	//    Sample=0.999(거의 항상 실패)를 계속 넣어도 상한 안에서 성공이 나와야 한다.
	{
		FPrdChannel Ch;
		Ch.SetTargetChance(0.10f);
		const int32 HardCap = FMath::CeilToInt(1.f / Ch.GetConstant());

		bool bHit = false;
		for (int32 i = 0; i < HardCap; ++i)
		{
			if (Ch.RollWith(0.999f)) { bHit = true; break; }
		}
		TestTrue(TEXT("PRD는 연속 실패에 하드 상한이 있어야 한다"), bHit);
	}

	// 3) 경험적 발동률: 균일 표본을 넣고 돌리면 목표 발동률에 수렴해야 한다.
	{
		FPrdChannel Ch;
		Ch.SetTargetChance(0.20f);

		FRandomStream Rng(12345);   // 시드 고정 → 실패해도 재현 가능
		int32 Hits = 0;
		const int32 Trials = 200000;
		for (int32 i = 0; i < Trials; ++i)
		{
			if (Ch.RollWith(Rng.GetFraction())) ++Hits;
		}
		const float Rate = static_cast<float>(Hits) / Trials;
		TestTrue(FString::Printf(TEXT("경험적 발동률 %.4f (목표 0.20)"), Rate),
			FMath::Abs(Rate - 0.20f) < 0.01f);
	}

	// 4) 경계값: 0이면 절대 발동 안 하고, 1이면 항상 발동해야 한다.
	{
		FPrdChannel Zero; Zero.SetTargetChance(0.f);
		TestFalse(TEXT("p=0 이면 발동 없음"), Zero.RollWith(0.f));

		FPrdChannel One;  One.SetTargetChance(1.f);
		TestTrue(TEXT("p=1 이면 항상 발동"), One.RollWith(0.999f));
	}

	return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS
