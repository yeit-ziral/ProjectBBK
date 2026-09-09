// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "TutorialData.h"
#include "C_TutorialComponent.generated.h"

class UDataTable;
class UInputAction;
class UEnhancedInputComponent;
class UWidget;
class UC_TutorialPromptWidget;
class UC_TutorialPointerWidget;
struct FInputActionInstance;
struct FGameplayEventData;
class UAbilitySystemComponent;

// 파라미터 없는 DECLARE_DYNAMIC_MULTICAST_DELEGATE는 BP Bind Event에서 Signature Error가 나므로
// 둘 다 파라미터를 하나씩 갖는다 (Debugging Checklist #36)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTutorialStepChanged, int32, StepIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTutorialCompleted, int32, CompletedStepCount);

/**
 * 단계별 조작 안내 튜토리얼 진행 컴포넌트. AC_PlayerController가 소유한다.
 *
 * 기존 입력 핸들러(AC_BasePlayerCharactor::MyMove / OnAbilityInputPressed,
 * AC_PlayerController::On*Input)를 전혀 수정하지 않고, PlayerController의
 * UEnhancedInputComponent에 자기 핸들러를 추가로 바인딩해 입력을 관찰한다.
 * Enhanced Input은 입력 스택의 모든 InputComponent에 이벤트를 전달하므로
 * Pawn 쪽에 바인딩된 액션(IA_Move, IA_Attack 등)과 C++ 바인딩이 아예 없는
 * Blueprint 전용 액션(IA_SkillWheelToggle)까지 동일하게 감지된다.
 *
 * PlayerController의 InputComponent는 InitInputSystem에서 한 번만 생성되므로
 * 캐릭터 교체(Tab) 후에도 바인딩이 유지된다.
 *
 * StartTutorial이 호출되기 전까지는 완전히 휴면 상태 — 튜토리얼 맵이 아닌
 * 레벨에서는 AC_TutorialGameMode가 없으므로 아무 동작도 하지 않는다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTBBK_API UC_TutorialComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UC_TutorialComponent();

	// 튜토리얼 시작. StepTable이 비었거나 위젯 클래스가 없으면 즉시 완료 처리한다.
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void StartTutorial(UDataTable* StepTable, TSubclassOf<UC_TutorialPromptWidget> WidgetClass);

	// 입력으로 표현되지 않는 단계(드래그&드롭 장착 등)의 완료 통지
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void NotifyExternalEvent(FGameplayTag EventTag);

	// 개발·디버그용 — 현재 단계를 강제로 완료 처리
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SkipCurrentStep();

	// 튜토리얼 중단 — 위젯·타이머·입력 바인딩을 모두 정리한다. OnTutorialCompleted는 발생하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void AbortTutorial();

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	bool IsTutorialRunning() const { return bIsRunning; }

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	int32 GetCurrentStepIndex() const { return currentStepIndex; }

	UPROPERTY(BlueprintAssignable, Category = "Tutorial")
	FOnTutorialStepChanged OnTutorialStepChanged;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial")
	FOnTutorialCompleted OnTutorialCompleted;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 모든 단계를 마쳤을 때 위젯에 마지막으로 남길 문구.
	// textOverrideFile에 "_Completed=문구" 줄이 있으면 그쪽이 우선한다.
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	FText completedMessage;

	// 완료 문구를 띄운 채 유지할 시간 (초) — 이후 위젯이 사라진다
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial", meta = (ClampMin = "0.0"))
	float completedMessageDuration = 3.0f;

	// 안내 위젯 ZOrder. HUD 0 / 대화 9 / 상점·엔딩 10 관례 사이에 위치.
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	int32 promptZOrder = 5;

	// 화살표 오버레이 ZOrder. 안내 문구보다 아래, HUD보다 위.
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	int32 pointerZOrder = 4;

	// 안내 문구를 덮어쓸 텍스트 파일 경로 (프로젝트 폴더 기준 상대 경로).
	// 형식은 한 줄에 "RowName=문구", 빈 줄과 # · // 로 시작하는 줄은 무시.
	// 파일이 없거나 해당 키가 없으면 DataTable의 instruction을 그대로 쓴다.
	// 비워두면 오버라이드 자체를 건너뛴다.
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	FString textOverrideFile = TEXT("Tutorial/TutorialText.txt");

private:
	// ETriggerEvent::Started — 횟수 조건 단계용
	void HandleActionStarted(const FInputActionInstance& Instance);

	// ETriggerEvent::Triggered — 누적 시간 조건 단계용
	void HandleActionTriggered(const FInputActionInstance& Instance);

	// 현재 단계의 액션·방향 필터에 부합하는 입력인지
	bool MatchesCurrentStep(const FInputActionInstance& Instance) const;

	const FTutorialStepData* GetCurrentStep() const;

	void ShowCurrentStep();
	void CompleteCurrentStep();
	void AdvanceStep();
	void FinishTutorial();

	void BindInputActions();
	void UnbindInputActions();

	// 현재 단계의 externalEventTag를 플레이어 ASC의 게임플레이 이벤트로도 감시한다.
	// (C++/GA가 HandleGameplayEvent로 쏘는 이벤트를 Blueprint 중계 없이 바로 받기 위함)
	void BindStepGameplayEvent(const FTutorialStepData& Step);
	void UnbindStepGameplayEvent();

	// ASC 게임플레이 이벤트 수신 → NotifyExternalEvent와 동일 경로로 처리
	void HandleStepGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);
	void RemovePromptWidget();
	void RemovePointerWidget();

	// 현재 단계의 pointerTargetClass / pointerHint에 맞춰 화살표를 갱신한다.
	// 대상이 없으면 화살표 위젯을 감춘다 (매 단계 호출 — 이전 단계 화살표가 남지 않게)
	void UpdateStepPointer(const FTutorialStepData& Step);

	// 화살표로 가리킬 대상 위젯을 화면에서 찾는다.
	// WidgetName이 지정되면 화면 위젯들의 WidgetTree에서 그 이름의 자식(WBP_HUD의 StaminaBar 등)을 찾고,
	// 비어 있으면 TargetClass 인스턴스 자체를 찾는다. 둘 다 주면 해당 클래스 안으로 범위를 좁힌다.
	UWidget* FindPointerTarget(UClass* TargetClass, FName WidgetName) const;

	// 이 단계 전용 액터(튜토리얼 더미 몬스터 등)를 플레이어 앞에 스폰한다
	void SpawnStepActors(const FTutorialStepData& Step);

	// 이전 단계에서 스폰한 액터를 전부 제거한다.
	// 매 단계 진입 시·완료 시·중단 시 호출 — 더미가 다음 단계까지 남지 않게.
	void DestroyStepActors();

	// 어느 단계에서든 나중에 나타날 액터를 튜토리얼 시작 시점에 전부 숨긴다.
	// 아이템이 처음부터 월드에 놓여 있는 어색함을 없애기 위한 처리.
	void HideRevealTargets();

	// 이 단계에서 나타나야 할 액터를 보이게 하고 숨김 목록에서 뺀다
	void RevealStepActors(const FTutorialStepData& Step);

	// 아직 숨겨져 있는 액터를 전부 되돌린다.
	// 튜토리얼 완료·중단 시 반드시 호출 — 도달하지 못한 단계의 아이템이 영영 사라지면 안 된다.
	void RevealAllHiddenActors();

	// 이 단계가 해당 액터를 등장 대상으로 삼는지 (아이템 플래그 또는 Actor Tag 일치)
	bool StepRevealsActor(const FTutorialStepData& Step, const AActor* Actor) const;

	// 렌더링과 콜리전을 함께 토글한다 — 콜리전을 끄지 않으면 안 보이는 아이템에 상호작용이 걸린다
	static void SetActorRevealed(AActor* Actor, bool bRevealed);

	// 마나(궁극기 게이지)를 최대치로 채운다 — bFillManaOnEnter 단계용
	void FillManaToMax();

	// textOverrideFile을 읽어 RowName → 문구 맵을 채운다. 파일이 없으면 빈 맵.
	void LoadTextOverrides(TMap<FName, FString>& OutOverrides) const;

	// 완료 문구 오버라이드 예약 키 — 단계 Row Name과 겹치지 않도록 밑줄로 시작한다
	static const FName completedMessageKey;

	UEnhancedInputComponent* GetEnhancedInputComponent() const;

	// DataTable에서 읽어온 단계 목록
	TArray<FTutorialStepData> steps;

	// steps와 인덱스 1:1 — TSoftObjectPtr를 로드해 캐시 (GC 방지를 위해 UPROPERTY)
	UPROPERTY()
	TArray<TObjectPtr<UInputAction>> resolvedActions;

	UPROPERTY()
	TObjectPtr<UC_TutorialPromptWidget> promptWidget;

	UPROPERTY()
	TObjectPtr<UC_TutorialPointerWidget> pointerWidget;

	// 종료 시 RemoveBindingByHandle로 해제할 바인딩 핸들
	TArray<uint32> inputBindingHandles;

	// completedMessage에 텍스트 파일 오버라이드를 반영한 최종 문구 (StartTutorial에서 확정)
	FText resolvedCompletedMessage;

	// 현재 단계에서 스폰한 액터들 — 다음 단계로 넘어갈 때 제거한다
	TArray<TWeakObjectPtr<AActor>> stepSpawnedActors;

	// 현재 단계의 externalEventTag를 구독 중인 ASC와 핸들 (해제에 둘 다 필요)
	TWeakObjectPtr<UAbilitySystemComponent> boundEventASC;
	FGameplayTagContainer boundEventTagFilter;
	FDelegateHandle boundEventHandle;

	// HideRevealTargets가 숨긴 액터들 — 해당 단계에서 다시 보이게 한다.
	// 레벨 스트리밍·몬스터 사망 등으로 파괴될 수 있으므로 약참조로 들고 있는다.
	TArray<TWeakObjectPtr<AActor>> hiddenActors;

	// bFillManaOnEnter 단계에서 마나를 계속 최대치로 유지하는 타이머.
	// 궁극기가 빗나가면 마나만 소모돼 재시도가 불가능해지므로,
	// 그 단계가 끝날 때까지 주기적으로 다시 채운다.
	FTimerHandle manaRefillTimer;

	FTimerHandle advanceTimer;

	int32 currentStepIndex = INDEX_NONE;
	int32 currentCount = 0;
	float accumulatedHold = 0.f;

	bool bIsRunning = false;

	// 완료 후 delayAfterComplete 대기 중 추가 입력으로 중복 완료되는 것을 막는다
	bool bStepSatisfied = false;
};
