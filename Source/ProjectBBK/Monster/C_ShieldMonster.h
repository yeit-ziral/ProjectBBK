// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_BaseMonster.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "C_ShieldMonster.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UParticleSystem;
class USoundBase;

/**
 * 쉴드 몬스터 — 방패를 든 근접 탱커.
 * 정면(guardHalfAngle 반각) 안에서 들어온 데미지를 guardDamageReduction 비율만큼 깎는다.
 * 측면·후면 공격은 그대로 들어가므로 플레이어에게 "뒤로 돌아가라"는 공략을 강제한다.
 *
 * 감소는 AC_BaseMonster::ModifyIncomingDamage 훅으로 처리되며,
 * UC_MonsterAttributeSet::PostGameplayEffectExecute에서 방어력(defense) 감산 **이전**에 적용된다.
 *   최종 = max(0, RawDamage * (1 - guardDamageReduction) - defense)
 * guardDamageReduction 기본값 1.0 = 정면 완전 무효. 데미지가 0이 되면
 * PostGameplayEffectExecute의 `Mitigated > 0` 가드 때문에 일반 피격 몽타주·히트플래시가
 * 재생되지 않으므로, 대신 blockReactionMontage(방패 막기 모션)를 여기서 직접 재생한다.
 *
 * 가드는 **상시**다 — 타겟을 인지하면 방패를 들고 그 상태로 이동하며(상체 additive + 하체 로코모션),
 * 노말 공격이 나가는 동안에만 내려간다. 데미지 차단 판정(IsGuarding)과 가드 포즈가 같은 값을 쓰므로
 * "방패가 올라가 있으면 막힌다"가 화면과 정확히 일치한다.
 *
 * 공격은 노말 1종(normalAttackGAClass)만 사용하며 AC_MeleeMonster와 동일하게
 * AttackManager 쿨다운 → TryActivateAbilityByClass 순서로 실행된다.
 * BT에는 UC_BTTaskMeleeAutoAttack(표시명 "Monster Auto Attack")을 그대로 배치하면 된다 —
 * AC_BaseMonster::TryAutoAttack() 경유로 일반화되어 있어 쉴드 전용 Task 클래스가 필요 없다.
 *
 * ── 패링 ──
 * 공격 몽타주에 UANS_ShieldParryWindow(AnimNotifyState)를 얹으면
 * ── 공격 시퀀스 ──
 * 공격은 몽타주 4개를 타이머로 이어 붙인 단계 진행이다(ShieldNormalAttack이 진입점):
 *   Lower  : shieldLowerMontage  — 들고 있던 방패를 내린다
 *   Cast   : castMontage         — 시전 모션. **이 시점부터 패링 링이 표시**된다
 *   Strike : 노말 공격 GA 발동    — 원속 재생, 몽타주 중간에 타격 판정
 *   Raise  : shieldRaiseMontage  — 방패를 다시 든다. 끝나야 가드가 복귀한다
 * 각 몽타주는 미지정이면 해당 단계를 건너뛴다.
 *
 * 링 수렴 시간 = 캐스트 길이 + (공격 몽타주의 ANS_ShieldParryWindow 종료 시각 / 재생 배속).
 * 타격 시각은 GetAttackStrikeTime()이 몽타주의 노티파이에서 직접 읽으므로 수동 입력이 필요 없다.
 *
 *   Cast 시작   → BeginAttackTelegraph(총 수렴 시간): 검은 작은 원 + 흰 큰 원 표시
 *   Tick        → AdvanceAttackTelegraph: 흰 원이 검은 원 쪽으로 수렴(실시간 기준)
 *   NotifyEnd   → ResolveStrike(): 두 원이 겹치는 순간 = 실제 타격 시점
 * ResolveStrike 시점에 정면 부채꼴(parryHalfAngle) 안의 플레이어가 State.Shield를 들고 있으면
 * 데미지 이벤트를 아예 보내지 않고 몬스터가 즉시 그로기에 들어간다.
 */
UCLASS()
class PROJECTBBK_API AC_ShieldMonster : public AC_BaseMonster
{
	GENERATED_BODY()

public:
	AC_ShieldMonster();

	virtual bool CanAutoAttack() const override;
	virtual bool IsPlayingAttackAnimation() const override;
	virtual bool TryAutoAttack() override { return ShieldAutoAttack(); }

	// BT에서 호출 — 공격 실행 시 true, 쿨타임/조건 미충족 시 false
	UFUNCTION(BlueprintCallable, Category = "Attack")
	bool ShieldAutoAttack();

	UFUNCTION(BlueprintCallable, Category = "Attack")
	bool ShieldNormalAttack();

#pragma region Guard
	// 방패를 들고 있는 상태 — 데미지 차단 판정과 가드 포즈가 **같은 값**을 쓴다.
	// 타겟을 인지한 동안 계속 true이고, 노말 공격 몽타주 재생 중·그로기·사망·
	// 가드브레이크일 때만 false가 되어 방패가 내려간다
	UFUNCTION(BlueprintPure, Category = "Shield|Guard")
	bool IsGuarding() const;

	// 블랙보드에 타겟이 잡혀 있는지 (BT Service가 DetectionRange 안의 플레이어를 넣어준다).
	// bGuardRequiresTarget이 true면 이게 false인 동안 방패를 내린다
	UFUNCTION(BlueprintPure, Category = "Shield|Guard")
	bool HasEngagedTarget() const;

	// 방금 실제로 공격을 막은 직후 구간인지 (blockPoseDuration 동안 true).
	// **가드 포즈와는 무관하다** — 포즈는 IsGuarding()이 담당한다(상시 가드).
	// blockPoseDuration 기본값이 0이라 별도 설정 없이는 항상 false.
	// 피격 반응 연출(히트 플래시 등)을 따로 붙이고 싶을 때만 쓸 것
	UFUNCTION(BlueprintPure, Category = "Shield|Guard")
	bool IsBlockPoseActive() const { return bBlockPoseActive; }

	// 가드가 깨진 상태(guardBreakDuration 동안 무방비)
	UFUNCTION(BlueprintPure, Category = "Shield|Guard")
	bool IsGuardBroken() const { return bGuardBroken; }

	// 블록 리액션으로 이동이 잠긴 상태 — AnimBP에서 이동 애니를 정지 포즈로 고정할 때 사용
	UFUNCTION(BlueprintPure, Category = "Shield|Guard")
	bool IsBlockMovementLocked() const { return bBlockMovementLocked; }

	// 블록 정지를 즉시 풀고 이동 모드를 되돌린다. 공격이 나갈 때 호출되어
	// 정지가 남아 있어도 공격이 지연 없이 실행되게 한다
	UFUNCTION(BlueprintCallable, Category = "Shield|Guard")
	void CancelBlockMovementLock();

	// AttackerLocation이 방패로 막을 수 있는 각도 안인지 (수평면 기준)
	UFUNCTION(BlueprintPure, Category = "Shield|Guard")
	bool IsBlockedDirection(const FVector& AttackerLocation) const;

	// 외부에서 가드를 강제로 깨뜨릴 때 (패링·강공격 등)
	UFUNCTION(BlueprintCallable, Category = "Shield|Guard")
	void BreakGuard();

	UFUNCTION(BlueprintCallable, Category = "Shield|Guard")
	void RecoverGuard();

	virtual float ModifyIncomingDamage(float RawDamage, AActor* DamageInstigator, bool bTrueDamage) override;
#pragma endregion

#pragma region Parry
	// 텔레그래프 링 표시 시작. ConvergeSeconds는 두 원이 겹치기까지의 **실시간**.
	// 정상 경로에서는 캐스트 단계가 호출하고, UANS_ShieldParryWindow::NotifyBegin은
	// castMontage 미지정 시의 폴백이다(이미 진행 중이면 무시되어 링이 리셋되지 않는다)
	UFUNCTION(BlueprintCallable, Category = "Shield|Parry")
	void BeginAttackTelegraph(float ConvergeSeconds);

	// 링 수렴 진행 — Tick에서 실시간 델타로 호출된다.
	// 윈드업 정지 중에는 애니 시간이 멈추므로 애니 델타가 아니라 실시간을 써야 한다
	UFUNCTION(BlueprintCallable, Category = "Shield|Parry")
	void AdvanceAttackTelegraph(float DeltaSeconds);

	// UANS_ShieldParryWindow::NotifyEnd — 실제 타격 시점.
	// 패링 성공 시 데미지 이벤트를 보내지 않고 그로기로 전환, 실패 시 strikeEventTag를 자신에게 전송
	UFUNCTION(BlueprintCallable, Category = "Shield|Parry")
	void ResolveStrike();

	// 공격이 중단됐을 때(그로기·사망·몽타주 취소) 링만 정리
	UFUNCTION(BlueprintCallable, Category = "Shield|Parry")
	void CancelAttackTelegraph();

	UFUNCTION(BlueprintPure, Category = "Shield|Parry")
	bool IsTelegraphActive() const { return bTelegraphActive; }

	// 공격 시퀀스(방패 내리기 → 캐스트 → 공격 → 방패 들기) 진행 중인지.
	// 이 동안은 가드가 내려가 있고 BT의 재공격도 차단된다
	UFUNCTION(BlueprintPure, Category = "Attack")
	bool IsInAttackSequence() const { return attackPhase != EAttackPhase::None; }

	// 링 수렴 진행도 — 1: 흰 원이 가장 큼(공격 시작), 0: 두 원이 겹침(타격 시점)
	UFUNCTION(BlueprintPure, Category = "Shield|Parry")
	float GetTelegraphProgress() const;
#pragma endregion

#pragma region BP Events
	// 정면 공격을 막았을 때 — 방패 타격 VFX/사운드용. BlockedAmount = 깎아낸 양
	UFUNCTION(BlueprintImplementableEvent, Category = "Shield|Guard")
	void OnGuardBlocked(AActor* Attacker, float BlockedAmount, float FinalDamage);

	// 가드 상태가 바뀔 때 (공격 시작/종료, 그로기, 가드 브레이크)
	UFUNCTION(BlueprintImplementableEvent, Category = "Shield|Guard")
	void OnGuardStateChanged(bool bNowGuarding);

	UFUNCTION(BlueprintImplementableEvent, Category = "Shield|Guard")
	void OnGuardBroken();

	UFUNCTION(BlueprintImplementableEvent, Category = "Shield|Guard")
	void OnGuardRecovered();

	// 패링 성공 — 이펙트·사운드·히트스톱 연출용. Parrier = 쉴드를 들고 있던 플레이어
	UFUNCTION(BlueprintImplementableEvent, Category = "Shield|Parry")
	void OnParrySuccess(AActor* Parrier);

	// 패링에 실패해 공격이 그대로 나갔을 때
	UFUNCTION(BlueprintImplementableEvent, Category = "Shield|Parry")
	void OnStrikeLanded();

	// 텔레그래프 링 표시/해제 — 추가 VFX나 사운드를 붙일 때
	UFUNCTION(BlueprintImplementableEvent, Category = "Shield|Parry")
	void OnTelegraphStateChanged(bool bNowActive);
#pragma endregion

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

#pragma region Attack
	// BP에서 노말 공격 GA 할당 (BPC_ShieldMonsterNormalAttackGA 등)
	UPROPERTY(EditDefaultsOnly, Category = "Attack|GA")
	TSubclassOf<UGameplayAbility> normalAttackGAClass;

	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* normalAttackMontage = nullptr;

	// 노말 공격 몽타주 재생 배속 — 기본 1.0(원속).
	// 반응 시간은 배속이 아니라 castMontage 길이로 조절할 것
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float attackMontagePlayRate = 1.f;

	// 노말 공격이 실제로 나가는 순간(Strike 단계 진입) 재생. 캐스트·방패 내리기 단계에서는 안 나온다
	UPROPERTY(EditAnywhere, Category = "Attack|Sound")
	USoundBase* attackSound = nullptr;

	// 공격 직전 시전(준비) 자세 진입 시 재생 — castMontage와 같은 타이밍.
	// Terra_Effort_Ability_Primary 같은 기합 큐를 여기에 할당한다
	UPROPERTY(EditAnywhere, Category = "Attack|Sound")
	USoundBase* castSound = nullptr;

	// 공격 시작 시 방패를 내리는 모션. 미지정이면 이 단계를 건너뛴다
	UPROPERTY(EditAnywhere, Category = "Animation|Attack Sequence")
	UAnimMontage* shieldLowerMontage = nullptr;

	// 시전 모션 — **이 모션이 시작될 때 패링 링이 나타난다**.
	// 플레이어가 링을 보고 쉴드를 올릴 시간이 이 모션 길이에서 나오므로,
	// 반응 시간을 늘리고 싶으면 재생 배속을 늦추지 말고 이 모션을 길게 만들 것.
	// 미지정이면 링은 ANS_ShieldParryWindow 시작 시점(공격 몽타주 내부)에 나타난다
	UPROPERTY(EditAnywhere, Category = "Animation|Attack Sequence")
	UAnimMontage* castMontage = nullptr;

	// 공격이 끝난 뒤 방패를 다시 드는 모션. 이 모션이 끝나야 가드가 복귀한다
	UPROPERTY(EditAnywhere, Category = "Animation|Attack Sequence")
	UAnimMontage* shieldRaiseMontage = nullptr;

	// 공격 시퀀스(방패 내리기 ~ 방패 들기) 동안 제자리 고정 — 시전 중 미끄러지는 것을 막는다
	UPROPERTY(EditAnywhere, Category = "Animation|Attack Sequence")
	bool bLockMovementDuringAttackSequence = true;

	// 정면 공격을 막았을 때 재생 — **선택 사항**.
	// 막기 전용 애니가 없으면 비워두고 AnimBP에서 IsGuarding() 기반 Mesh Space Additive
	// 가드 포즈를 상시 출력하는 방식으로 대체한다. 비워둬도 아래 이동 잠금은 동작한다
	UPROPERTY(EditAnywhere, Category = "Animation")
	UAnimMontage* blockReactionMontage = nullptr;

	// 블록 반응 최소 간격(초) — 연타에 몽타주·이동잠금이 매 히트마다 리셋되는 것을 방지
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (ClampMin = "0.0"))
	float blockReactionInterval = 0.35f;

	// 정면 공격을 막았을 때 제자리에 버티는 시간(초).
	// "데미지는 안 들어갔지만 맞긴 맞았다"를 보여주는 리액션 — 이동만 멈추고 공격은 그대로 나간다.
	// 0이면 잠그지 않음(blockReactionMontage가 있으면 그 길이를 대신 사용)
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (ClampMin = "0.0"))
	float blockMoveLockDuration = 1.2f;

	// 잠금이 풀린 뒤 이만큼 지나야 다시 잠글 수 있다(초).
	// 없으면 플레이어가 연타할 때 타이머가 계속 갱신돼 몬스터가 영구히 못 움직인다.
	// 잠금 1.2초 + 쿨다운 0.6초 = 두들겨 맞는 중에도 0.6초씩은 반드시 움직인다
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (ClampMin = "0.0"))
	float blockMoveLockCooldown = 0.6f;

	// 피격 직후 IsBlockPoseActive()를 true로 유지하는 시간(초). **기본 0 = 사용 안 함**.
	// 가드 포즈는 IsGuarding()이 상시 담당하므로 여기에 값을 넣을 필요가 없다
	UPROPERTY(EditAnywhere, Category = "Animation", meta = (ClampMin = "0.0"))
	float blockPoseDuration = 0.f;
#pragma endregion

#pragma region Guard Settings
	// false면 가드 기능 자체를 끔 (일반 근접 몬스터처럼 동작)
	UPROPERTY(EditAnywhere, Category = "Shield|Guard")
	bool bGuardEnabled = true;

	// 정면 기준 **반각**(도). 90이면 전방 180도를 방어한다
	UPROPERTY(EditAnywhere, Category = "Shield|Guard", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float guardHalfAngle = 90.f;

	// 방어 성공 시 감소 비율 (0.8 = 80% 감소, 1.0 = 완전 무효)
	UPROPERTY(EditAnywhere, Category = "Shield|Guard", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float guardDamageReduction = 1.0f;

	// true면 DoT·상태이상(ReceivedTrueDamage)도 방어. 기본은 직격만 방어
	UPROPERTY(EditAnywhere, Category = "Shield|Guard")
	bool bGuardBlocksTrueDamage = false;

	// true면 공격 몽타주 재생 중에도 가드 유지. false면 공격 중이 딜 넣을 틈이 된다
	UPROPERTY(EditAnywhere, Category = "Shield|Guard")
	bool bGuardWhileAttacking = false;

	// true면 타겟을 인지한 동안에만 방패를 든다. false면 배회 중에도 상시 가드
	UPROPERTY(EditAnywhere, Category = "Shield|Guard")
	bool bGuardRequiresTarget = true;

	// 블랙보드의 타겟 키 이름 — UC_MonsterBTService가 채우는 키와 맞출 것
	UPROPERTY(EditAnywhere, Category = "Shield|Guard", meta = (EditCondition = "bGuardRequiresTarget"))
	FName guardTargetKeyName = TEXT("TargetActor");

	// 이 거리 안에 타겟이 있을 때만 가드. 0이면 거리 제한 없음
	// (BT Service의 DetectionRange가 이미 필터링하므로 기본은 0)
	UPROPERTY(EditAnywhere, Category = "Shield|Guard", meta = (ClampMin = "0.0", EditCondition = "bGuardRequiresTarget"))
	float guardEngageRange = 0.f;

	// 가드 **가능** 상태에서의 이동 속도 배율 (moveSpeed 어트리뷰트 기준).
	// IsGuarding()이 평상시 항상 true이므로 1이 아닌 값을 넣으면 상시 감속이 되고,
	// UC_BTTaskReposition이 지정하는 속도와 서로 덮어쓴다. 기본 1.0 = 이동에 관여 안 함.
	// "막을 때만 멈추는" 동작은 blockMoveLockDuration이 담당한다
	UPROPERTY(EditAnywhere, Category = "Shield|Guard", meta = (ClampMin = "0.0"))
	float guardMoveSpeedScale = 1.0f;
#pragma endregion

#pragma region Block VFX
	// 정면 공격을 막아냈을 때 방패에서 터지는 이펙트 (P_Terra_ShieldBlockHit 등).
	// 미지정이면 아무 것도 스폰하지 않는다 — 이펙트 없이도 나머지 방어 로직은 그대로 동작
	UPROPERTY(EditAnywhere, Category = "Shield|Guard|VFX")
	UParticleSystem* blockHitEffect = nullptr;

	// 이펙트를 붙일 메시 소켓. Terra 스켈레톤 기준 FX_Gem이 방패(shield_handle)에 달린 FX 소켓이다.
	// 소켓이 없으면 메시 원점에 붙으므로 스켈레톤을 바꿀 땐 이 이름도 같이 확인할 것
	UPROPERTY(EditAnywhere, Category = "Shield|Guard|VFX")
	FName blockHitSocketName = TEXT("FX_Gem");

	// 소켓 기준 상대 오프셋 — 방패 표면 바깥으로 살짝 띄우고 싶을 때
	UPROPERTY(EditAnywhere, Category = "Shield|Guard|VFX")
	FVector blockHitEffectOffset = FVector::ZeroVector;

	// 소켓 기준 상대 회전 — 소켓 축과 이펙트 방향이 어긋날 때 보정
	UPROPERTY(EditAnywhere, Category = "Shield|Guard|VFX")
	FRotator blockHitEffectRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "Shield|Guard|VFX", meta = (ClampMin = "0.01"))
	float blockHitEffectScale = 1.f;

	// true면 스폰 직후 이펙트를 공격자 쪽으로 돌린다 — 방향성 있는(원뿔·스파크) 이펙트용.
	// 방패에 붙어 따라다니므로 회전만 덮어쓰고 부착은 유지된다
	UPROPERTY(EditAnywhere, Category = "Shield|Guard|VFX")
	bool bBlockHitEffectFaceAttacker = true;

	// 연타 시 이펙트 스폰 최소 간격(초). 0이면 막을 때마다 매번 스폰.
	// blockReactionInterval(몽타주·이동잠금)과 별개로 둬서 이펙트만 촘촘하게 낼 수 있다
	UPROPERTY(EditAnywhere, Category = "Shield|Guard|VFX", meta = (ClampMin = "0.0"))
	float blockHitEffectInterval = 0.f;

	// 진단용 — 스폰 결과를 로그로 찍고 스폰 좌표에 청록색 구체를 3초간 그린다.
	// 구체는 보이는데 이펙트가 없으면 "스폰은 됐고 렌더가 안 되는 것",
	// 구체조차 없으면 "스폰 경로를 아예 안 탄 것"으로 갈린다. 확인 끝나면 끌 것
	UPROPERTY(EditAnywhere, Category = "Shield|Guard|VFX")
	bool bLogBlockHitEffect = false;

	// 방패로 막아냈을 때 재생 (Knight_Shield_Deflection 등). 이펙트와 같은 타이밍
	UPROPERTY(EditAnywhere, Category = "Shield|Guard|VFX")
	USoundBase* blockHitSound = nullptr;
#pragma endregion

#pragma region Guard Break
	// 누적 차단량이 이 값을 넘으면 가드가 깨진다. 0이면 가드 브레이크 비활성
	UPROPERTY(EditAnywhere, Category = "Shield|GuardBreak", meta = (ClampMin = "0.0"))
	float guardBreakThreshold = 0.f;

	// 가드가 깨진 뒤 무방비 상태로 있는 시간(초)
	UPROPERTY(EditAnywhere, Category = "Shield|GuardBreak", meta = (ClampMin = "0.0"))
	float guardBreakDuration = 4.f;

	// 누적 차단량이 초당 이만큼 감소 — 장기전에서 무조건 가드가 깨지는 것을 방지
	UPROPERTY(EditAnywhere, Category = "Shield|GuardBreak", meta = (ClampMin = "0.0"))
	float guardBreakDecayPerSecond = 10.f;
#pragma endregion

#pragma region Parry Settings
	// 타격 판정 부채꼴 **반각**(도). 이 안에서 쉴드를 들고 있어야 패링 인정
	UPROPERTY(EditAnywhere, Category = "Shield|Parry", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float parryHalfAngle = 60.f;

	// 판정 거리 = attackRange 어트리뷰트 + 이 값. 애니 타격 시점의 전진 거리를 보정
	UPROPERTY(EditAnywhere, Category = "Shield|Parry", meta = (ClampMin = "0.0"))
	float parryRangeBonus = 100.f;

	// 패링 성공 시 그로기 유지 시간(초). 0이면 GroggyComponent의 groggyDuration 사용
	UPROPERTY(EditAnywhere, Category = "Shield|Parry", meta = (ClampMin = "0.0"))
	float parryGroggyDuration = 5.f;

	// 타격 시점에 자신에게 보낼 게임플레이 이벤트 — 노말 공격 GA가 이걸 받아 데미지를 적용한다.
	// 미지정 시 Event.Monster.Melee.Hit (기존 근접 몬스터 GA를 그대로 복제해 쓸 수 있도록)
	UPROPERTY(EditAnywhere, Category = "Shield|Parry", meta = (Categories = "Event"))
	FGameplayTag strikeEventTag;

	// 텔레그래프 링 — 평면 메시. 기본은 카메라 정면 빌보드(bParryRingFaceCamera), 끄면 발밑 수평.
	// 머티리얼은 Unlit/Translucent + Disable Depth Test 권장 (지형에 파묻히지 않고 2D처럼 보임)
	UPROPERTY(EditAnywhere, Category = "Shield|Parry")
	UMaterialInterface* parryRingMaterial = nullptr;

	// 링의 최대 반지름(cm) — 흰 원이 여기서 시작해 검은 원까지 줄어든다. 판정과 무관한 순수 시각 크기
	UPROPERTY(EditAnywhere, Category = "Shield|Parry", meta = (ClampMin = "1.0"))
	float parryRingRadius = 420.f;

	// 링 평면의 바닥 띄움(cm) — 캡슐 발밑 기준. bParryRingFaceCamera가 false일 때만 사용
	UPROPERTY(EditAnywhere, Category = "Shield|Parry")
	float parryRingGroundOffset = 3.f;

	// true면 링이 지면에 눕지 않고 매 프레임 플레이어 카메라 정면을 바라보는 빌보드로 동작
	UPROPERTY(EditAnywhere, Category = "Shield|Parry")
	bool bParryRingFaceCamera = true;

	// 빌보드 모드에서 링 중심 높이(cm) — 액터 원점(캡슐 중심) 기준. 0이면 몬스터 중앙
	UPROPERTY(EditAnywhere, Category = "Shield|Parry")
	float parryRingCameraHeightOffset = 0.f;

	// 빌보드 모드에서 화면 기준 링 회전(도) — 머티리얼 웨지 시작 방향 보정용
	UPROPERTY(EditAnywhere, Category = "Shield|Parry")
	float parryRingRoll = 0.f;

	// 머티리얼의 진행도 스칼라 파라미터 이름 (1 → 0으로 전달)
	UPROPERTY(EditAnywhere, Category = "Shield|Parry")
	FName parryRingProgressParam = TEXT("Progress");
#pragma endregion

#pragma region Debug
	// true면 가드 각도를 매 프레임 시각화 (정면=초록, 경계선=파랑, 가드 해제 시 빨강)
	UPROPERTY(EditAnywhere, Category = "Shield|Debug")
	bool bDrawGuardDebug = false;

	// true면 패링 판정 부채꼴을 ResolveStrike 시점에 3초간 그린다 (성공=초록, 실패=빨강)
	UPROPERTY(EditAnywhere, Category = "Shield|Debug")
	bool bDrawParryDebug = false;
#pragma endregion

private:
	void UpdateGuardState();
	void ApplyGuardMovementSpeed();
	void AccumulateGuardBreak(float BlockedAmount);
	void DrawGuardDebug() const;
	void PlayBlockReaction();

	// 막기 피드백 — 방패 소켓에 부착된 이펙트 + 사운드. 둘 다 미지정이면 무동작
	void PlayBlockHitFeedback(AActor* Attacker);

	// 블록 모션 동안 제자리 고정 — 밀리거나 Reposition으로 빠져나가지 않게 한다
	void LockMovementForBlock(float Duration);
	void ReleaseBlockMovementLock();

	void EndBlockPose();

	// 부채꼴·거리 조건을 만족하면서 State.Shield를 들고 있는 플레이어. 없으면 nullptr
	AActor* FindParryingPlayer() const;

	// GA(BP)가 PlayMontageAndWait으로 재생한 공격 몽타주에 attackMontagePlayRate를 덮어씌운다
	void ApplyAttackMontagePlayRate();

	// 현재 재생 중인 공격 몽타주 (normalAttackMontage 우선, 없으면 활성 몽타주)
	UAnimMontage* GetActiveAttackMontage() const;

	// 공격 시퀀스 단계 진행 — 각 단계는 앞 몽타주 길이만큼의 타이머로 이어진다
	void StartAttackSequence();
	void BeginCastPhase();
	void BeginStrikePhase();
	void BeginRaisePhase();
	void EndAttackSequence();

	// 그로기·사망 등으로 시퀀스를 중간에 끊는다 (링 정리 + 이동 복구 포함)
	void AbortAttackSequence();

	// 공격 몽타주 안에서 실제 타격이 나가는 시각(애니 초).
	// ANS_ShieldParryWindow의 종료 시각을 몽타주에서 직접 읽는다 — 노티파이가 없으면 몽타주 전체 길이
	float GetAttackStrikeTime() const;

	// 시퀀스 이동 잠금 — bLockMovementDuringAttackSequence가 true일 때만 동작
	void LockMovementForAttackSequence();
	void ReleaseAttackSequenceMovement();

	void SetTelegraphRingVisible(bool bVisible);
	void PushTelegraphProgress();

	// 빌보드 모드에서 링을 카메라 정면으로 세운다 (텔레그래프 진행 중 매 프레임 호출)
	void UpdateTelegraphRingTransform();

	UPROPERTY(VisibleAnywhere, Category = "Shield|Parry")
	UStaticMeshComponent* parryRingMesh = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* parryRingMID = nullptr;

	// 마지막으로 BP/이동속도에 반영한 가드 상태 — 상태가 바뀔 때만 처리하기 위한 캐시
	bool bGuardActive = false;

	bool  bGuardBroken        = false;
	float guardBreakAccum     = 0.f;

	// 공격 시퀀스 단계. None이 아니면 "공격 중"으로 취급된다
	enum class EAttackPhase : uint8 { None, Lower, Cast, Strike, Raise };
	EAttackPhase attackPhase = EAttackPhase::None;
	FTimerHandle attackPhaseTimerHandle;

	bool  bTelegraphActive    = false;
	float telegraphDuration   = 0.f;
	float telegraphElapsed    = 0.f;

	// 첫 피격에서 무조건 재생되도록 충분히 과거로 초기화
	float lastBlockReactionTime = -1000.f;

	// blockHitEffectInterval 판정용 — 위와 같은 이유로 과거값으로 시작
	float lastBlockHitEffectTime = -1000.f;

	// 마지막으로 이동 잠금이 풀린 시각 — blockMoveLockCooldown 판정용
	float lastBlockMoveLockEndTime = -1000.f;

	bool bBlockMovementLocked = false;
	bool bBlockPoseActive     = false;

	FTimerHandle guardBreakTimerHandle;
	FTimerHandle blockMoveLockTimerHandle;
	FTimerHandle blockPoseTimerHandle;
};
