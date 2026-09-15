// Fill out your copyright notice in the Description page of Project Settings.

#include "C_TutorialComponent.h"
#include "C_TutorialPromptWidget.h"
#include "C_TutorialPointerWidget.h"
#include "../PlayerCharacter/C_PlayerState.h"
#include "../Items/C_BaseItem.h"
#include "../PlayerCharacter/PlayerAI/C_PlayerController.h"
#include "../PlayerCharacter/C_BasePlayerCharactor.h"
#include "../Inventory/C_InventoryComponent.h"
#include "../Equip/C_EquipmentComponent.h"
#include "../Inventory/C_InventorySlotWidget.h"
#include "../Monster/Anim/ANC_MeleeNormalAttack.h"
#include "../Monster/Anim/ANC_MonsterGameplayEvent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "../GAS/Attributes/C_ChracterAttributeSetBase.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

const FName UC_TutorialComponent::completedMessageKey(TEXT("_Completed"));
const FName UC_TutorialComponent::equippableSlotPointerToken(TEXT("@EquippableSlot"));

namespace TutorialEventTags
{
	// ini 반영 전(에디터 미재시작)이면 무효 태그를 돌려준다 — ensure로 PIE를 멈추지 않게 ErrorIfNotFound=false.
	// 무효 태그는 NotifyExternalEvent에서 무시된다.
	static FGameplayTag ItemPickedUp()        { return FGameplayTag::RequestGameplayTag(TEXT("Event.Tutorial.ItemPickedUp"), false); }
	static FGameplayTag QuickSlotRegistered() { return FGameplayTag::RequestGameplayTag(TEXT("Event.Tutorial.QuickSlotRegistered"), false); }
	static FGameplayTag ItemEquipped()        { return FGameplayTag::RequestGameplayTag(TEXT("Event.Tutorial.ItemEquipped"), false); }
}

UC_TutorialComponent::UC_TutorialComponent()
{
	// 공격 일시정지 단계(pauseBeforeHitPrompt)에서만 켜고 평소에는 끈다
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	completedMessage = NSLOCTEXT("Tutorial", "TutorialCompleted", "튜토리얼 완료! 포탈로 이동하시오");
}

void UC_TutorialComponent::StartTutorial(UDataTable* StepTable, TSubclassOf<UC_TutorialPromptWidget> WidgetClass)
{
	// 위젯 생성도 입력 바인딩도 불가능한 소유자에서 시작하면 튜토리얼이 조용히 죽는다.
	// (CDO·아키타입이거나, 로컬 플레이어가 아직 붙지 않은 PlayerController)
	// 호출 측이 시점을 맞춰야 하는 문제이므로 여기서는 명확한 에러만 남기고 중단한다.
	APlayerController* OwnerPC = Cast<APlayerController>(GetOwner());
	if (!OwnerPC || OwnerPC->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) || OwnerPC->Player == nullptr)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[UC_TutorialComponent] 플레이어가 붙지 않은 PlayerController(%s)에서 StartTutorial이 호출돼 중단합니다."),
			OwnerPC ? *OwnerPC->GetName() : TEXT("None"));
		return;
	}

	// 재진입 방지 — 이미 진행 중이면 기존 상태를 먼저 정리
	if (bIsRunning)
	{
		AbortTutorial();
	}

	steps.Reset();
	resolvedActions.Reset();

	// 잘못된 Row Struct의 DataTable을 reinterpret_cast하면 메모리를 잘못 읽는다 — 반드시 먼저 검사
	if (StepTable && (StepTable->GetRowStruct() == nullptr ||
		!StepTable->GetRowStruct()->IsChildOf(FTutorialStepData::StaticStruct())))
	{
		UE_LOG(LogTemp, Error, TEXT("[UC_TutorialComponent] %s의 Row Struct가 FTutorialStepData가 아닙니다."),
			*StepTable->GetName());
		StepTable = nullptr;
	}

	// 문구만 외부 텍스트 파일로 뺄 수 있다 — 조건(액션·방향·시간)은 DataTable에 그대로 둔다.
	// 레벨 시작 시 한 번만 읽으므로, 파일을 고친 뒤에는 PIE를 다시 시작해야 반영된다.
	TMap<FName, FString> TextOverrides;
	LoadTextOverrides(TextOverrides);

	// 완료 문구는 단계 행이 아니므로 예약 키로 따로 덮어쓴다
	resolvedCompletedMessage = completedMessage;
	if (const FString* CompletedOverride = TextOverrides.Find(completedMessageKey))
	{
		resolvedCompletedMessage = FText::FromString(*CompletedOverride);
	}

	if (StepTable)
	{
		// GetAllRows는 내부 RowMap 순회 순서를 그대로 반환하므로 행 순서가 보장되지 않는다.
		// 튜토리얼은 순서가 곧 의미이므로 order 필드로 명시 정렬한다 (동률이면 Row Name 순).
		TArray<TPair<FName, FTutorialStepData>> SortedRows;
		for (const TPair<FName, uint8*>& RowPair : StepTable->GetRowMap())
		{
			if (const FTutorialStepData* Row = reinterpret_cast<const FTutorialStepData*>(RowPair.Value))
			{
				SortedRows.Emplace(RowPair.Key, *Row);
			}
		}

		SortedRows.Sort([](const TPair<FName, FTutorialStepData>& A, const TPair<FName, FTutorialStepData>& B)
			{
				if (A.Value.order != B.Value.order)
				{
					return A.Value.order < B.Value.order;
				}
				return A.Key.LexicalLess(B.Key);
			});

		for (const TPair<FName, FTutorialStepData>& RowPair : SortedRows)
		{
			// 소프트 참조를 미리 로드해 캐시 — 매 입력마다 해석하지 않도록
			UInputAction* Action = RowPair.Value.requiredAction.LoadSynchronous();

			// 완료 조건이 아예 없는 행은 튜토리얼을 영구히 멈추게 하므로 로드 시점에 경고
			if (Action == nullptr && !RowPair.Value.externalEventTag.IsValid())
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[UC_TutorialComponent] 행 '%s'에 requiredAction도 externalEventTag도 없습니다. SkipCurrentStep 없이는 진행이 멈춥니다."),
					*RowPair.Key.ToString());
			}

			FTutorialStepData Step = RowPair.Value;

			// 텍스트 파일에 같은 Row Name의 줄이 있으면 문구만 갈아끼운다
			if (const FString* Override = TextOverrides.Find(RowPair.Key))
			{
				Step.instruction = FText::FromString(*Override);
			}

			// 화살표 보조 문구는 "RowName.hint=" 키로 따로 덮어쓴다
			const FName HintKey(*(RowPair.Key.ToString() + TEXT(".hint")));
			if (const FString* HintOverride = TextOverrides.Find(HintKey))
			{
				Step.pointerHint = FText::FromString(*HintOverride);
			}

			// 화살표 대상 위젯 이름은 "RowName.pointer=" 키로 덮어쓴다.
			// DataTable을 열지 않고도 어느 UI를 가리킬지 바꿀 수 있게 하기 위한 경로.
			const FName PointerKey(*(RowPair.Key.ToString() + TEXT(".pointer")));
			if (const FString* PointerOverride = TextOverrides.Find(PointerKey))
			{
				const FString Trimmed = PointerOverride->TrimStartAndEnd();
				Step.pointerTargetWidgetName = Trimmed.IsEmpty() ? NAME_None : FName(*Trimmed);
			}

			// 액터 등장 시점은 "RowName.reveal=" 키로 지정한다.
			//   items → 레벨의 모든 픽업 아이템(AC_BaseItem)
			//   그 외 → 해당 Actor Tag를 가진 액터
			const FName RevealKey(*(RowPair.Key.ToString() + TEXT(".reveal")));
			if (const FString* RevealOverride = TextOverrides.Find(RevealKey))
			{
				const FString Trimmed = RevealOverride->TrimStartAndEnd();
				if (Trimmed.Equals(TEXT("items"), ESearchCase::IgnoreCase))
				{
					Step.bRevealItemsOnEnter = true;
					Step.revealActorTag = NAME_None;
				}
				else
				{
					Step.bRevealItemsOnEnter = false;
					Step.revealActorTag = Trimmed.IsEmpty() ? NAME_None : FName(*Trimmed);
				}
			}

			// 히트 직전 일시정지 문구·재개 태그는 "RowName.pauseHit=" / "RowName.resumeTag=" 키로 지정한다
			const FName PauseHitKey(*(RowPair.Key.ToString() + TEXT(".pauseHit")));
			if (const FString* PauseHitOverride = TextOverrides.Find(PauseHitKey))
			{
				Step.pauseBeforeHitPrompt = FText::FromString(*PauseHitOverride);
			}

			const FName ResumeTagKey(*(RowPair.Key.ToString() + TEXT(".resumeTag")));
			if (const FString* ResumeTagOverride = TextOverrides.Find(ResumeTagKey))
			{
				const FString Trimmed = ResumeTagOverride->TrimStartAndEnd();
				// ini에 없는 태그면 무효 태그가 된다 — ensure로 PIE를 멈추지 않게 ErrorIfNotFound=false
				Step.pauseResumeTag = Trimmed.IsEmpty() ? FGameplayTag() : FGameplayTag::RequestGameplayTag(FName(*Trimmed), false);
			}

			if (!Step.pauseBeforeHitPrompt.IsEmpty() && !Step.pauseResumeTag.IsValid())
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[UC_TutorialComponent] 행 '%s'에 pauseHit 문구는 있지만 유효한 resumeTag가 없어 공격을 멈추지 않습니다."),
					*RowPair.Key.ToString());
			}

			steps.Add(Step);
			resolvedActions.Add(Action);
		}
	}

	if (steps.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UC_TutorialComponent] 단계 DataTable이 비어 있어 튜토리얼을 즉시 완료 처리합니다."));
		OnTutorialCompleted.Broadcast(0);
		return;
	}

	if (WidgetClass)
	{
		promptWidget = CreateWidget<UC_TutorialPromptWidget>(OwnerPC, WidgetClass);
		if (promptWidget)
		{
			promptWidget->AddToViewport(promptZOrder);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[UC_TutorialComponent] 안내 위젯 클래스가 지정되지 않았습니다. 문구 없이 진행합니다."));
	}

	bIsRunning = true;
	currentStepIndex = 0;

	BindInputActions();

	// 반드시 ShowCurrentStep보다 먼저 — 0번 단계가 등장 단계인 경우
	// 숨겼다가 곧바로 다시 보여야 순서가 맞는다
	HideRevealTargets();

	ShowCurrentStep();
}

void UC_TutorialComponent::NotifyExternalEvent(FGameplayTag EventTag)
{
	if (!bIsRunning || bStepSatisfied || !EventTag.IsValid())
	{
		return;
	}

	const FTutorialStepData* Step = GetCurrentStep();
	if (!Step || !Step->externalEventTag.IsValid())
	{
		return;
	}

	if (Step->externalEventTag != EventTag)
	{
		return;
	}

	// 입력 단계와 동일하게 횟수를 센다 — "적의 공격을 2번 막기"처럼
	// 외부 이벤트도 여러 번 필요한 단계가 있다 (requiredCount 기본값 1이면 종전과 같음)
	currentCount++;

	if (promptWidget && Step->requiredCount > 1)
	{
		promptWidget->SetStepCount(currentCount, Step->requiredCount);
		promptWidget->SetStepProgress(static_cast<float>(currentCount) / static_cast<float>(Step->requiredCount));
	}

	if (currentCount >= Step->requiredCount)
	{
		CompleteCurrentStep();
	}
}

void UC_TutorialComponent::SkipCurrentStep()
{
	if (bIsRunning && !bStepSatisfied)
	{
		CompleteCurrentStep();
	}
}

void UC_TutorialComponent::AbortTutorial()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(advanceTimer);
		World->GetTimerManager().ClearTimer(manaRefillTimer);
		World->GetTimerManager().ClearTimer(pointerRetryTimer);
	}

	UnbindInputActions();
	UnbindGameObservers();
	ResumeAttackMontage();
	SetComponentTickEnabled(false);
	RemovePointerWidget();
	RemovePromptWidget();

	// 중단 시점에 숨겨둔 액터가 남아 있으면 그대로 사라진 채가 된다 — 반드시 되돌린다
	RevealAllHiddenActors();
	DestroyStepActors();
	UnbindStepGameplayEvent();

	bIsRunning = false;
	bStepSatisfied = false;
	currentStepIndex = INDEX_NONE;
	currentCount = 0;
	accumulatedHold = 0.f;
}

void UC_TutorialComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 타이머·입력 바인딩·위젯이 남지 않도록 반드시 정리
	AbortTutorial();

	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// 입력 처리
// ---------------------------------------------------------------------------

void UC_TutorialComponent::HandleActionStarted(const FInputActionInstance& Instance)
{
	const FTutorialStepData* Step = GetCurrentStep();
	if (!Step || Step->requiredHoldSeconds > 0.f)
	{
		// 누적 시간 조건 단계는 Triggered 쪽에서 처리
		return;
	}

	if (!MatchesCurrentStep(Instance))
	{
		return;
	}

	currentCount++;

	if (promptWidget && Step->requiredCount > 1)
	{
		promptWidget->SetStepCount(currentCount, Step->requiredCount);
		promptWidget->SetStepProgress(static_cast<float>(currentCount) / static_cast<float>(Step->requiredCount));
	}

	if (currentCount >= Step->requiredCount)
	{
		CompleteCurrentStep();
	}
}

void UC_TutorialComponent::HandleActionTriggered(const FInputActionInstance& Instance)
{
	const FTutorialStepData* Step = GetCurrentStep();
	if (!Step || Step->requiredHoldSeconds <= 0.f)
	{
		// 횟수 조건 단계는 Started 쪽에서 처리
		return;
	}

	if (!MatchesCurrentStep(Instance))
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Triggered는 조건이 유지되는 동안 프레임당 한 번 호출되므로 DeltaSeconds 누적이 곧 유지 시간이 된다.
	// 입력을 놓아도 누적값은 유지 — 중간에 손을 떼도 튜토리얼이 막히지 않게 한다.
	accumulatedHold += World->GetDeltaSeconds();

	if (promptWidget)
	{
		promptWidget->SetStepProgress(accumulatedHold / Step->requiredHoldSeconds);
	}

	if (accumulatedHold >= Step->requiredHoldSeconds)
	{
		CompleteCurrentStep();
	}
}

bool UC_TutorialComponent::MatchesCurrentStep(const FInputActionInstance& Instance) const
{
	if (!bIsRunning || bStepSatisfied)
	{
		return false;
	}

	const FTutorialStepData* Step = GetCurrentStep();
	if (!Step || Step->externalEventTag.IsValid())
	{
		// 외부 이벤트 조건 단계는 입력으로 완료되지 않는다
		return false;
	}

	if (!resolvedActions.IsValidIndex(currentStepIndex) || resolvedActions[currentStepIndex] == nullptr)
	{
		return false;
	}

	if (Instance.GetSourceAction() != resolvedActions[currentStepIndex])
	{
		return false;
	}

	// 방향 필터 — W/S/A/D를 같은 IA_Move 안에서 구분하기 위함
	if (!Step->requiredDirection.IsNearlyZero())
	{
		const FVector2D InputDir = Instance.GetValue().Get<FVector2D>();
		if (InputDir.IsNearlyZero())
		{
			return false;
		}

		float Dot = FVector2D::DotProduct(InputDir.GetSafeNormal(), Step->requiredDirection.GetSafeNormal());

		// 좌우(A/D)처럼 반대 방향도 같은 단계로 인정하면 축만 맞으면 통과
		if (Step->bAcceptOppositeDirection)
		{
			Dot = FMath::Abs(Dot);
		}

		if (Dot < Step->directionTolerance)
		{
			return false;
		}
	}

	return true;
}

// ---------------------------------------------------------------------------
// 진행
// ---------------------------------------------------------------------------

const FTutorialStepData* UC_TutorialComponent::GetCurrentStep() const
{
	return steps.IsValidIndex(currentStepIndex) ? &steps[currentStepIndex] : nullptr;
}

void UC_TutorialComponent::ShowCurrentStep()
{
	// 새 단계 진입 시 진행 상태를 반드시 초기화 — 이전 단계 값이 남으면
	// 다음 단계가 즉시 완료되거나 진행 바가 잘못 표시된다 (Debugging Checklist #25·#51과 동일 계열)
	currentCount = 0;
	accumulatedHold = 0.f;
	bStepSatisfied = false;

	const FTutorialStepData* Step = GetCurrentStep();
	if (!Step)
	{
		return;
	}

	// 이전 단계에서 멈춰 둔 공격이 남아 있으면 풀고, 이 단계가 일시정지 단계일 때만 매 프레임 감시한다
	ResumeAttackMontage();
	lastReleasedMontageInstanceId = INDEX_NONE;
	SetComponentTickEnabled(!Step->pauseBeforeHitPrompt.IsEmpty() && Step->pauseResumeTag.IsValid());

	if (promptWidget)
	{
		promptWidget->SetStep(Step->instruction, currentStepIndex, steps.Num());

		// SetStep이 진행 표시를 전부 끄고 시작하므로, 필요한 단계만 여기서 다시 켠다
		if (Step->requiredHoldSeconds > 0.f)
		{
			promptWidget->SetStepProgress(0.f);
		}
		else if (Step->requiredCount > 1)
		{
			// 아직 한 번도 안 눌렀어도 "0 / 3"으로 몇 번 눌러야 하는지 미리 보여준다
			promptWidget->SetStepCount(0, Step->requiredCount);
			promptWidget->SetStepProgress(0.f);
		}
	}

	// 이전 단계에서 스폰한 더미는 반드시 여기서 정리 — 다음 단계까지 남으면 안 된다
	DestroyStepActors();
	SpawnStepActors(*Step);

	// 아이템 획득·퀵슬롯 등록·장비 장착 관찰 — 새로 생긴 대상(드롭 아이템 등)까지 매 단계 보강한다
	BindGameObservers();

	// externalEventTag 단계는 ASC 게임플레이 이벤트로도 완료될 수 있게 구독을 갈아끼운다
	BindStepGameplayEvent(*Step);

	// 이 단계에서 등장하기로 한 액터(아이템 등)를 지금 나타나게 한다
	RevealStepActors(*Step);

	// 자원이 모자라면 시연 자체가 불가능한 단계(궁극기 등)를 위해 진입 시점에 채워준다.
	// 한 번만 채우면 빗나갔을 때 마나만 소모되고 재시도가 막히므로,
	// 이 단계가 끝날 때까지 주기적으로 다시 채운다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(manaRefillTimer);

		if (Step->bFillManaOnEnter)
		{
			FillManaToMax();
			World->GetTimerManager().SetTimer(
				manaRefillTimer, this, &UC_TutorialComponent::FillManaToMax, 1.0f, true);
		}
	}

	// 대상이 없는 단계에서도 반드시 호출 — 이전 단계 화살표가 남지 않게 (Debugging Checklist #25·#51과 동일 계열)
	UpdateStepPointer(*Step);

	OnTutorialStepChanged.Broadcast(currentStepIndex);
}

void UC_TutorialComponent::CompleteCurrentStep()
{
	if (bStepSatisfied)
	{
		return;
	}

	bStepSatisfied = true;

	const FTutorialStepData* Step = GetCurrentStep();
	const float Delay = Step ? Step->delayAfterComplete : 0.f;

	// 마지막 입력 직후 바가 살짝 덜 찬 채로 멈추지 않도록 두 조건 모두 꽉 채운다
	if (promptWidget && Step && (Step->requiredHoldSeconds > 0.f || Step->requiredCount > 1))
	{
		promptWidget->SetStepProgress(1.f);
	}

	UWorld* World = GetWorld();
	if (World && Delay > 0.f)
	{
		World->GetTimerManager().SetTimer(advanceTimer, this, &UC_TutorialComponent::AdvanceStep, Delay, false);
	}
	else
	{
		AdvanceStep();
	}
}

void UC_TutorialComponent::AdvanceStep()
{
	if (!bIsRunning)
	{
		return;
	}

	currentStepIndex++;

	if (!steps.IsValidIndex(currentStepIndex))
	{
		FinishTutorial();
		return;
	}

	ShowCurrentStep();
}

void UC_TutorialComponent::FinishTutorial()
{
	const int32 CompletedCount = steps.Num();

	// 더 이상 입력을 관찰할 필요가 없으므로 바인딩부터 해제
	UnbindInputActions();

	// 완료 문구는 남기되 화살표는 즉시 치운다
	RemovePointerWidget();

	// 건너뛴 단계가 있어도 아이템이 사라진 채로 남지 않게 전부 되돌린다
	RevealAllHiddenActors();

	// 마지막 단계의 더미가 튜토리얼이 끝난 뒤에도 남아 플레이어를 때리는 일이 없게 한다
	DestroyStepActors();
	UnbindStepGameplayEvent();
	UnbindGameObservers();
	ResumeAttackMontage();
	SetComponentTickEnabled(false);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(manaRefillTimer);
		World->GetTimerManager().ClearTimer(pointerRetryTimer);
	}

	bIsRunning = false;
	bStepSatisfied = false;
	currentStepIndex = INDEX_NONE;

	if (promptWidget)
	{
		promptWidget->ShowCompleted(resolvedCompletedMessage);

		UWorld* World = GetWorld();
		if (World && completedMessageDuration > 0.f)
		{
			World->GetTimerManager().SetTimer(
				advanceTimer, this, &UC_TutorialComponent::RemovePromptWidget, completedMessageDuration, false);
		}
		else
		{
			RemovePromptWidget();
		}
	}

	OnTutorialCompleted.Broadcast(CompletedCount);
}

// ---------------------------------------------------------------------------
// 바인딩 / 정리
// ---------------------------------------------------------------------------

UEnhancedInputComponent* UC_TutorialComponent::GetEnhancedInputComponent() const
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	return PC ? Cast<UEnhancedInputComponent>(PC->InputComponent) : nullptr;
}

void UC_TutorialComponent::BindInputActions()
{
	UEnhancedInputComponent* EIC = GetEnhancedInputComponent();
	if (!EIC)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[UC_TutorialComponent] PlayerController의 EnhancedInputComponent를 찾지 못했습니다. 입력 감지가 동작하지 않습니다."));
		return;
	}

	// 같은 액션이 여러 단계에 쓰이므로 중복 바인딩을 피한다
	TSet<UInputAction*> UniqueActions;
	for (UInputAction* Action : resolvedActions)
	{
		if (Action)
		{
			UniqueActions.Add(Action);
		}
	}

	for (UInputAction* Action : UniqueActions)
	{
		inputBindingHandles.Add(
			EIC->BindAction(Action, ETriggerEvent::Started, this, &UC_TutorialComponent::HandleActionStarted).GetHandle());
		inputBindingHandles.Add(
			EIC->BindAction(Action, ETriggerEvent::Triggered, this, &UC_TutorialComponent::HandleActionTriggered).GetHandle());
	}
}

void UC_TutorialComponent::UnbindInputActions()
{
	if (inputBindingHandles.Num() == 0)
	{
		return;
	}

	// 바인딩을 남겨두면 튜토리얼 종료 후에도 핸들러가 계속 호출된다 (Debugging Checklist #19와 동일 계열)
	if (UEnhancedInputComponent* EIC = GetEnhancedInputComponent())
	{
		for (const uint32 Handle : inputBindingHandles)
		{
			EIC->RemoveBindingByHandle(Handle);
		}
	}

	inputBindingHandles.Reset();
}

void UC_TutorialComponent::BindStepGameplayEvent(const FTutorialStepData& Step)
{
	// 단계가 바뀔 때마다 이전 구독을 반드시 먼저 해제 — 남아 있으면 다음 단계가 즉시 완료된다
	UnbindStepGameplayEvent();

	if (!Step.externalEventTag.IsValid())
	{
		return;
	}

	// ASC는 반드시 PlayerState 경유로 취득 (CLAUDE.md GAS 규칙)
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	AC_PlayerState* PS = PC ? PC->GetPlayerState<AC_PlayerState>() : nullptr;
	UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		// ASC 없이도 NotifyExternalEvent(BlueprintCallable) 경로는 살아 있으므로 경고만 남긴다
		UE_LOG(LogTemp, Warning,
			TEXT("[UC_TutorialComponent] ASC를 찾지 못해 %s 게임플레이 이벤트를 구독하지 못했습니다."),
			*Step.externalEventTag.ToString());
		return;
	}

	boundEventTagFilter.Reset();
	boundEventTagFilter.AddTag(Step.externalEventTag);

	boundEventHandle = ASC->AddGameplayEventTagContainerDelegate(
		boundEventTagFilter,
		FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(
			this, &UC_TutorialComponent::HandleStepGameplayEvent));

	boundEventASC = ASC;
}

void UC_TutorialComponent::UnbindStepGameplayEvent()
{
	if (UAbilitySystemComponent* ASC = boundEventASC.Get())
	{
		ASC->RemoveGameplayEventTagContainerDelegate(boundEventTagFilter, boundEventHandle);
	}

	boundEventASC.Reset();
	boundEventTagFilter.Reset();
	boundEventHandle.Reset();
}

void UC_TutorialComponent::HandleStepGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	// 횟수 누적·완료 판정은 전부 NotifyExternalEvent에 모아 둔다 (BlueprintCallable 경로와 동일하게)
	NotifyExternalEvent(EventTag);
}

void UC_TutorialComponent::LoadTextOverrides(TMap<FName, FString>& OutOverrides) const
{
	OutOverrides.Reset();

	if (textOverrideFile.IsEmpty())
	{
		return;
	}

	// 프로젝트 폴더(.uproject 옆) 기준 — 탐색기에서 바로 찾아 메모장으로 열 수 있는 위치
	const FString FullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / textOverrideFile);

	if (!FPaths::FileExists(FullPath))
	{
		// 파일이 없는 건 정상 — DataTable 문구를 그대로 쓴다
		UE_LOG(LogTemp, Log, TEXT("[UC_TutorialComponent] 문구 오버라이드 파일 없음 (%s). DataTable 값을 사용합니다."), *FullPath);
		return;
	}

	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *FullPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("[UC_TutorialComponent] 문구 오버라이드 파일을 읽지 못했습니다: %s"), *FullPath);
		return;
	}

	for (const FString& RawLine : Lines)
	{
		FString Line = RawLine.TrimStartAndEnd();

		// 빈 줄 / 주석 줄 건너뛰기
		if (Line.IsEmpty() || Line.StartsWith(TEXT("#")) || Line.StartsWith(TEXT("//")))
		{
			continue;
		}

		FString Key;
		FString Value;
		if (!Line.Split(TEXT("="), &Key, &Value))
		{
			UE_LOG(LogTemp, Warning, TEXT("[UC_TutorialComponent] '=' 가 없는 줄을 건너뜁니다: %s"), *Line);
			continue;
		}

		Key = Key.TrimStartAndEnd();
		Value = Value.TrimStartAndEnd();

		if (Key.IsEmpty())
		{
			continue;
		}

		// 문구 안에서 줄바꿈이 필요하면 \n 두 글자로 적는다
		Value = Value.Replace(TEXT("\\n"), TEXT("\n"));

		OutOverrides.Add(FName(*Key), Value);
	}

	UE_LOG(LogTemp, Log, TEXT("[UC_TutorialComponent] 문구 오버라이드 %d개를 읽었습니다: %s"), OutOverrides.Num(), *FullPath);
}

void UC_TutorialComponent::RemovePromptWidget()
{
	if (promptWidget)
	{
		promptWidget->RemoveFromParent();
		promptWidget = nullptr;
	}
}

void UC_TutorialComponent::RemovePointerWidget()
{
	if (pointerWidget)
	{
		pointerWidget->RemoveFromParent();
		pointerWidget = nullptr;
	}
}

UC_TutorialPointerWidget* UC_TutorialComponent::EnsurePointerWidget()
{
	if (pointerWidget)
	{
		return pointerWidget;
	}

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return nullptr;
	}

	pointerWidget = CreateWidget<UC_TutorialPointerWidget>(PC, UC_TutorialPointerWidget::StaticClass());
	if (!pointerWidget)
	{
		return nullptr;
	}

	// 앵커·크기를 지정하지 않아 뷰포트 슬롯 기본값(0,0,1,1)으로 전체 화면을 덮는다 (Debugging Checklist #56)
	pointerWidget->AddToViewport(pointerZOrder);
	return pointerWidget;
}

// ---------------------------------------------------------------------------
// 화살표 / 자원 보정
// ---------------------------------------------------------------------------

void UC_TutorialComponent::UpdateStepPointer(const FTutorialStepData& Step, bool bFromRetry)
{
	UClass* TargetClass = Step.pointerTargetClass.IsNull() ? nullptr : Step.pointerTargetClass.LoadSynchronous();
	const FName TargetName = Step.pointerTargetWidgetName;
	const bool bHasTarget = (TargetClass != nullptr) || !TargetName.IsNone();

	// 대상이 사용자가 여는 창(인벤토리·장비창)이면 단계 진입 시점엔 아직 없을 수 있고,
	// 장비창은 열 때마다 새 인스턴스가 생긴다 — 대상이 있는 단계 동안은 주기적으로 다시 찾는다
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		if (!bHasTarget)
		{
			TimerManager.ClearTimer(pointerRetryTimer);
		}
		else if (!TimerManager.IsTimerActive(pointerRetryTimer))
		{
			TimerManager.SetTimer(pointerRetryTimer, this, &UC_TutorialComponent::RetryStepPointer, pointerRetryInterval, true);
		}
	}

	UWidget* Target = nullptr;
	if (bHasTarget)
	{
		Target = (TargetName == equippableSlotPointerToken)
			? FindEquippableInventorySlot()
			: FindPointerTarget(TargetClass, TargetName);
	}

	if (!Target)
	{
		if (bHasTarget && !bFromRetry)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[UC_TutorialComponent] 화면에서 화살표 대상을 찾지 못했습니다 (class=%s, name=%s)."),
				TargetClass ? *TargetClass->GetName() : TEXT("None"), *TargetName.ToString());
		}

		if (pointerWidget)
		{
			pointerWidget->ClearPointer();
		}
		return;
	}

	if (!EnsurePointerWidget())
	{
		return;
	}

	// 재탐색에서 같은 대상을 다시 찾았으면 그대로 둔다 — PointAt이 표시 상태를 초기화해 깜빡이기 때문
	if (bFromRetry && pointerWidget->GetTargetWidget() == Target)
	{
		return;
	}

	// 폰트를 안내 위젯에서 물려받아야 한글이 깨지지 않는다
	const FSlateFontInfo Font = promptWidget ? promptWidget->GetInstructionFont() : FSlateFontInfo();
	const FSlateColor Color = promptWidget ? promptWidget->GetInstructionColor() : FSlateColor(FLinearColor::White);

	pointerWidget->PointAt(Target, Step.pointerHint, Font, Color);
}

UWidget* UC_TutorialComponent::FindPointerTarget(UClass* TargetClass, FName WidgetName) const
{
	// 이름이 없으면 클래스 자체가 대상 — 클래스도 없으면 가리킬 것이 없다
	if (WidgetName.IsNone() && (!TargetClass || !TargetClass->IsChildOf(UUserWidget::StaticClass())))
	{
		return nullptr;
	}

	// 이름으로 찾을 때는 어느 위젯 안에 있는지 모르므로 화면의 모든 UUserWidget을 뒤진다.
	// 클래스가 함께 지정되면 그 클래스 인스턴스로만 범위를 좁힌다.
	UClass* SearchClass = TargetClass ? TargetClass : UUserWidget::StaticClass();

	// WBP_HUD 안에 들어 있는 자식 위젯(WBP_UltimateGauge 등)까지 찾아야 하므로 TopLevelOnly = false
	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), Found, SearchClass, false);

	// 한 번도 그려지지 않아 좌표가 아직 0인 후보 — 끝까지 못 찾으면 이거라도 넘겨
	// 다음 프레임에 좌표가 잡히게 한다 (UC_TutorialPointerWidget::NativeTick이 매 프레임 재계산)
	UWidget* PendingCandidate = nullptr;

	for (UUserWidget* Widget : Found)
	{
		if (!Widget)
		{
			continue;
		}

		UWidget* Candidate = Widget;
		if (!WidgetName.IsNone())
		{
			// 디자이너 계층 이름으로 자식을 찾는다 — WBP_HUD의 StaminaBar처럼
			// 자기 UUserWidget 클래스를 갖지 않는 내부 위젯을 가리키기 위한 경로
			Candidate = Widget->WidgetTree ? Widget->WidgetTree->FindWidget(WidgetName) : nullptr;
			if (!Candidate)
			{
				continue;
			}
		}

		// 닫힌 창(RemoveFromParent)도 인스턴스가 남아 있으면 여기 잡힌다 — 뷰포트에 붙어 보이는 것만 대상으로 삼는다
		if (!UC_TutorialPointerWidget::IsWidgetOnScreen(Candidate))
		{
			continue;
		}

		// 화면에 실제로 그려진 인스턴스만 유효한 좌표를 갖는다
		if (Candidate->GetCachedGeometry().GetLocalSize().SizeSquared() > KINDA_SMALL_NUMBER)
		{
			return Candidate;
		}

		if (!PendingCandidate)
		{
			PendingCandidate = Candidate;
		}
	}

	return PendingCandidate;
}

// ---------------------------------------------------------------------------
// 단계 전용 액터 스폰 / 제거
// ---------------------------------------------------------------------------

void UC_TutorialComponent::SpawnStepActors(const FTutorialStepData& Step)
{
	if (Step.spawnActorClass.IsNull())
	{
		return;
	}

	UWorld* World = GetWorld();
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!World || !PlayerPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UC_TutorialComponent] 플레이어 폰이 없어 단계 액터를 스폰하지 못했습니다."));
		return;
	}

	UClass* SpawnClass = Step.spawnActorClass.LoadSynchronous();
	if (!SpawnClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UC_TutorialComponent] spawnActorClass를 로드하지 못했습니다: %s"),
			*Step.spawnActorClass.ToString());
		return;
	}

	const FVector PlayerLocation = PlayerPawn->GetActorLocation();
	const FVector Forward = PlayerPawn->GetActorForwardVector().GetSafeNormal2D();
	FVector SpawnLocation = PlayerLocation + Forward * Step.spawnDistance;

	// 지면 높이는 LineTrace로 찾는다 (경사·계단에서 수동 계산은 어긋남 — 기존 지면 탐색 패턴과 동일)
	FHitResult GroundHit;
	FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(TutorialSpawnGround), false, PlayerPawn);
	const bool bHitGround = World->LineTraceSingleByChannel(
		GroundHit,
		SpawnLocation + FVector(0.f, 0.f, 500.f),
		SpawnLocation - FVector(0.f, 0.f, 500.f),
		ECC_Visibility,
		GroundParams);

	if (bHitGround)
	{
		// 캐릭터는 원점이 캡슐 중심이므로 지면에서 캡슐 반높이만큼 띄워야 바닥에 파묻히지 않는다
		float HalfHeight = 0.f;
		if (const ACharacter* SpawnCDO = Cast<ACharacter>(SpawnClass->GetDefaultObject()))
		{
			if (const UCapsuleComponent* Capsule = SpawnCDO->GetCapsuleComponent())
			{
				HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			}
		}

		SpawnLocation = GroundHit.ImpactPoint + FVector(0.f, 0.f, HalfHeight);
	}
	// Hit이 없으면 플레이어와 같은 높이를 그대로 쓴다 — ImpactPoint(0,0,0) 원점 스폰 방지 (Debugging Checklist #22)

	// 플레이어를 바라보게 — Yaw만 사용해 캐릭터가 기울지 않게 한다
	FRotator SpawnRotation = (PlayerLocation - SpawnLocation).Rotation();
	SpawnRotation.Pitch = 0.f;
	SpawnRotation.Roll = 0.f;

	FActorSpawnParameters SpawnParams;
	// 좁은 곳에서도 반드시 나와야 튜토리얼이 막히지 않는다
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* Spawned = World->SpawnActor<AActor>(SpawnClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (!Spawned)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UC_TutorialComponent] %s 스폰에 실패했습니다."), *SpawnClass->GetName());
		return;
	}

	stepSpawnedActors.Add(Spawned);

	UE_LOG(LogTemp, Log, TEXT("[UC_TutorialComponent] 단계 %d 액터 스폰: %s"),
		currentStepIndex, *Spawned->GetName());
}

void UC_TutorialComponent::DestroyStepActors()
{
	for (const TWeakObjectPtr<AActor>& WeakActor : stepSpawnedActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			UE_LOG(LogTemp, Log, TEXT("[UC_TutorialComponent] 단계 액터 제거: %s"), *Actor->GetName());
			Actor->Destroy();
		}
	}

	stepSpawnedActors.Reset();
}

// ---------------------------------------------------------------------------
// 단계별 액터 등장
// ---------------------------------------------------------------------------

void UC_TutorialComponent::HideRevealTargets()
{
	hiddenActors.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 월드를 한 번만 순회하고, 어느 단계에서든 등장 대상인 액터를 전부 숨긴다.
	// (단계마다 순회하면 같은 액터를 여러 번 만나게 되고, 등장 이후 다시 숨겨질 위험도 생긴다)
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor))
		{
			continue;
		}

		bool bWillReveal = false;
		for (const FTutorialStepData& Step : steps)
		{
			if (StepRevealsActor(Step, Actor))
			{
				bWillReveal = true;
				break;
			}
		}

		if (!bWillReveal)
		{
			continue;
		}

		SetActorRevealed(Actor, false);
		hiddenActors.Add(Actor);
	}

	UE_LOG(LogTemp, Log, TEXT("[UC_TutorialComponent] 등장 대기 액터 %d개를 숨겼습니다."), hiddenActors.Num());
}

void UC_TutorialComponent::RevealStepActors(const FTutorialStepData& Step)
{
	if (hiddenActors.Num() == 0)
	{
		return;
	}

	// 뒤에서부터 순회 — 등장시킨 항목을 그 자리에서 제거해도 인덱스가 밀리지 않는다
	for (int32 Index = hiddenActors.Num() - 1; Index >= 0; --Index)
	{
		AActor* Actor = hiddenActors[Index].Get();

		// 이미 파괴된 액터는 목록에서만 빼면 된다
		if (!IsValid(Actor))
		{
			hiddenActors.RemoveAt(Index);
			continue;
		}

		if (!StepRevealsActor(Step, Actor))
		{
			continue;
		}

		SetActorRevealed(Actor, true);
		hiddenActors.RemoveAt(Index);
	}
}

void UC_TutorialComponent::RevealAllHiddenActors()
{
	for (const TWeakObjectPtr<AActor>& WeakActor : hiddenActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			SetActorRevealed(Actor, true);
		}
	}

	hiddenActors.Reset();
}

bool UC_TutorialComponent::StepRevealsActor(const FTutorialStepData& Step, const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	// 아이템은 별도 태그 없이 클래스로 찾는다 — 레벨에 놓인 아이템마다 태그를 다는 수작업이 필요 없다
	if (Step.bRevealItemsOnEnter && Actor->IsA(AC_BaseItem::StaticClass()))
	{
		return true;
	}

	if (Step.revealActorTag != NAME_None && Actor->ActorHasTag(Step.revealActorTag))
	{
		return true;
	}

	return false;
}

void UC_TutorialComponent::SetActorRevealed(AActor* Actor, bool bRevealed)
{
	if (!IsValid(Actor))
	{
		return;
	}

	Actor->SetActorHiddenInGame(!bRevealed);

	// 콜리전까지 꺼야 안 보이는 아이템에 Overlap이 걸려 상호작용 안내가 뜨는 일이 없다.
	// 다시 켤 때 UpdateOverlaps가 돌기 때문에, 등장 시 플레이어가 이미 그 자리에 서 있어도
	// Overlap이 정상적으로 발생한다.
	Actor->SetActorEnableCollision(bRevealed);
}

void UC_TutorialComponent::FillManaToMax()
{
	// ASC는 반드시 PlayerState 경유로 취득 (CLAUDE.md GAS 규칙) — 이 컴포넌트의 소유자는 PlayerController
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	AC_PlayerState* PS = PC ? PC->GetPlayerState<AC_PlayerState>() : nullptr;
	UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UC_TutorialComponent] ASC를 찾지 못해 마나를 채우지 못했습니다."));
		return;
	}

	const float MaxMana = ASC->GetNumericAttribute(UC_ChracterAttributeSetBase::GetmaxManaAttribute());

	// 이미 가득이면 건드리지 않는다 — 반복 타이머가 매초 어트리뷰트 변경 델리게이트를 깨우지 않게
	if (ASC->GetNumericAttribute(UC_ChracterAttributeSetBase::GetmanaAttribute()) >= MaxMana)
	{
		return;
	}

	// GE 없이 base value를 직접 설정 — 이 변경도 어트리뷰트 변경 델리게이트를 발화시키므로
	// UC_UltimateGaugeWidget의 게이지 표시가 즉시 갱신된다
	ASC->SetNumericAttributeBase(UC_ChracterAttributeSetBase::GetmanaAttribute(), MaxMana);
}

void UC_TutorialComponent::RetryStepPointer()
{
	const FTutorialStepData* Step = GetCurrentStep();
	if (!bIsRunning || !Step)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(pointerRetryTimer);
		}
		return;
	}

	// 이미 화면에 떠 있는 대상을 가리키고 있으면 다시 찾을 필요 없다.
	// 단, 장비 칸은 아이템을 옮기거나 그리드가 다시 생성되면 대상 칸이 바뀌므로 매번 다시 찾는다.
	const bool bDynamicTarget = (Step->pointerTargetWidgetName == equippableSlotPointerToken);
	if (!bDynamicTarget && pointerWidget && pointerWidget->IsTargetOnScreen())
	{
		return;
	}

	UpdateStepPointer(*Step, true);
}

// ---------------------------------------------------------------------------
// 게임 이벤트 관찰 (아이템 획득 / 퀵슬롯 등록 / 장비 장착)
// ---------------------------------------------------------------------------

void UC_TutorialComponent::BindGameObservers()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 인벤토리(퀵슬롯)는 PlayerController 소유 — 이 컴포넌트와 같은 액터에 붙어 있다
	if (!observedInventory.IsValid())
	{
		if (AC_PlayerController* PC = Cast<AC_PlayerController>(GetOwner()))
		{
			if (UC_InventoryComponent* Inventory = PC->GetInventory())
			{
				Inventory->OnQuickSlotChanged.AddUniqueDynamic(this, &UC_TutorialComponent::HandleQuickSlotChanged);
				observedInventory = Inventory;
			}
		}
	}

	// 장비 컴포넌트는 캐릭터별 — 로스터 전원에 구독해야 캐릭터 교체 후에도 장착을 감지한다
	for (TActorIterator<AC_BasePlayerCharactor> It(World); It; ++It)
	{
		UC_EquipmentComponent* Equip = It->FindComponentByClass<UC_EquipmentComponent>();
		if (!Equip || observedEquipments.Contains(TWeakObjectPtr<UC_EquipmentComponent>(Equip)))
		{
			continue;
		}

		Equip->OnEquipmentChanged.AddUniqueDynamic(this, &UC_TutorialComponent::HandleEquipmentChanged);
		observedEquipments.Add(Equip);
	}

	lastEquippedCount = CountEquippedSlots();

	// 아이템은 획득에 성공하면 Destroy된다 — 숨겨둔 아이템까지 미리 구독해 둔다
	for (TActorIterator<AC_BaseItem> It(World); It; ++It)
	{
		AC_BaseItem* Item = *It;
		if (!IsValid(Item) || observedItems.Contains(TWeakObjectPtr<AActor>(Item)))
		{
			continue;
		}

		Item->OnDestroyed.AddUniqueDynamic(this, &UC_TutorialComponent::HandleItemActorDestroyed);
		observedItems.Add(Item);
	}
}

void UC_TutorialComponent::UnbindGameObservers()
{
	// 구독을 남겨두면 튜토리얼이 끝난 뒤에도 핸들러가 호출된다 (Debugging Checklist #19와 동일 계열)
	if (UC_InventoryComponent* Inventory = observedInventory.Get())
	{
		Inventory->OnQuickSlotChanged.RemoveDynamic(this, &UC_TutorialComponent::HandleQuickSlotChanged);
	}
	observedInventory.Reset();
	lastQuickSlotItems.Reset();

	for (const TWeakObjectPtr<UC_EquipmentComponent>& WeakEquip : observedEquipments)
	{
		if (UC_EquipmentComponent* Equip = WeakEquip.Get())
		{
			Equip->OnEquipmentChanged.RemoveDynamic(this, &UC_TutorialComponent::HandleEquipmentChanged);
		}
	}
	observedEquipments.Reset();
	lastEquippedCount = 0;

	for (const TWeakObjectPtr<AActor>& WeakItem : observedItems)
	{
		if (AActor* Item = WeakItem.Get())
		{
			Item->OnDestroyed.RemoveDynamic(this, &UC_TutorialComponent::HandleItemActorDestroyed);
		}
	}
	observedItems.Reset();
}

void UC_TutorialComponent::HandleItemActorDestroyed(AActor* DestroyedActor)
{
	observedItems.RemoveAll([DestroyedActor](const TWeakObjectPtr<AActor>& WeakItem)
		{
			return !WeakItem.IsValid() || WeakItem.Get() == DestroyedActor;
		});

	// 숨겨진 아이템은 주울 수 없다 — 그 상태로 사라졌다면 획득이 아니다
	if (!DestroyedActor || DestroyedActor->IsHidden())
	{
		return;
	}

	NotifyExternalEvent(TutorialEventTags::ItemPickedUp());
}

void UC_TutorialComponent::HandleQuickSlotChanged(int32 SlotIndex)
{
	UC_InventoryComponent* Inventory = observedInventory.Get();
	if (!Inventory || SlotIndex < 0)
	{
		return;
	}

	// 튜토리얼은 빈 퀵슬롯에서 시작하므로 기준값을 미리 채우지 않고 필요할 때 늘린다
	if (!lastQuickSlotItems.IsValidIndex(SlotIndex))
	{
		lastQuickSlotItems.SetNum(SlotIndex + 1);
	}

	const FName NewItem = Inventory->GetQuickSlotItem(SlotIndex);
	const FName PrevItem = lastQuickSlotItems[SlotIndex];
	lastQuickSlotItems[SlotIndex] = NewItem;

	if (!NewItem.IsNone() && NewItem != PrevItem)
	{
		NotifyExternalEvent(TutorialEventTags::QuickSlotRegistered());
	}
}

void UC_TutorialComponent::HandleEquipmentChanged()
{
	const int32 EquippedCount = CountEquippedSlots();
	const bool bNewlyEquipped = EquippedCount > lastEquippedCount;
	lastEquippedCount = EquippedCount;

	if (bNewlyEquipped)
	{
		NotifyExternalEvent(TutorialEventTags::ItemEquipped());
	}
}

int32 UC_TutorialComponent::CountEquippedSlots() const
{
	int32 Count = 0;

	for (const TWeakObjectPtr<UC_EquipmentComponent>& WeakEquip : observedEquipments)
	{
		const UC_EquipmentComponent* Equip = WeakEquip.Get();
		if (!Equip)
		{
			continue;
		}

		for (uint8 SlotValue = static_cast<uint8>(EEquipmentSlot::Head);
			SlotValue <= static_cast<uint8>(EEquipmentSlot::Accessory); ++SlotValue)
		{
			if (Equip->IsSlotEquipped(static_cast<EEquipmentSlot>(SlotValue)))
			{
				++Count;
			}
		}
	}

	return Count;
}

UWidget* UC_TutorialComponent::FindEquippableInventorySlot() const
{
	// 장착 가능 여부는 캐릭터별 장비 컴포넌트가 판정한다 (Common은 누구나, Melee/Ranged는 타입 일치 시만)
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	const APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	const UC_EquipmentComponent* Equip = PlayerPawn ? PlayerPawn->FindComponentByClass<UC_EquipmentComponent>() : nullptr;
	if (!Equip)
	{
		return nullptr;
	}

	// 슬롯 위젯은 WBP_Inventory의 SlotGrid에 동적으로 생성되므로 TopLevelOnly = false
	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), Found, UC_InventorySlotWidget::StaticClass(), false);

	UWidget* EquipmentFallback = nullptr;

	for (UUserWidget* Widget : Found)
	{
		const UC_InventorySlotWidget* SlotWidget = Cast<UC_InventorySlotWidget>(Widget);
		if (!SlotWidget || SlotWidget->GetItemID().IsNone())
		{
			continue;
		}

		// 인벤토리를 닫아도 슬롯 인스턴스는 남아 있다 — 화면에 떠 있는 칸만 대상
		if (!UC_TutorialPointerWidget::IsWidgetOnScreen(Widget))
		{
			continue;
		}

		const FName ItemID = SlotWidget->GetItemID();
		if (Equip->CanEquipItem(ItemID))
		{
			return Widget;
		}

		if (!EquipmentFallback && Equip->GetItemSlotType(ItemID) != EEquipmentSlot::None)
		{
			EquipmentFallback = Widget;
		}
	}

	return EquipmentFallback;
}

// ---------------------------------------------------------------------------
// 히트 직전 공격 일시정지 (막기 단계 등)
// ---------------------------------------------------------------------------

void UC_TutorialComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const FTutorialStepData* Step = GetCurrentStep();
	if (!bIsRunning || !Step)
	{
		SetComponentTickEnabled(false);
		return;
	}

	UpdateHitPause(*Step);
}

void UC_TutorialComponent::UpdateHitPause(const FTutorialStepData& Step)
{
	// ASC는 반드시 PlayerState 경유로 취득 (CLAUDE.md GAS 규칙)
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	AC_PlayerState* PS = PC ? PC->GetPlayerState<AC_PlayerState>() : nullptr;
	UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
	const bool bResumeReady = ASC && ASC->HasMatchingGameplayTag(Step.pauseResumeTag);

	// 멈춰 있는 동안에는 재개 조건만 본다
	if (hitPausedMontage.IsValid() || hitPausedAnimInstance.IsValid())
	{
		// 몬스터가 사라졌으면 풀 대상이 없다 — 말풍선만 치운다
		if (bResumeReady || !hitPausedAnimInstance.IsValid())
		{
			ResumeAttackMontage();
		}
		return;
	}

	for (const TWeakObjectPtr<AActor>& WeakActor : stepSpawnedActors)
	{
		const ACharacter* Monster = Cast<ACharacter>(WeakActor.Get());
		UAnimInstance* AnimInstance = (Monster && Monster->GetMesh()) ? Monster->GetMesh()->GetAnimInstance() : nullptr;
		UAnimMontage* Montage = AnimInstance ? AnimInstance->GetCurrentActiveMontage() : nullptr;
		const FAnimMontageInstance* MontageInstance = Montage ? AnimInstance->GetActiveInstanceForMontage(Montage) : nullptr;
		if (!MontageInstance || !MontageInstance->IsPlaying())
		{
			continue;
		}

		const int32 InstanceId = MontageInstance->GetInstanceID();
		if (InstanceId == lastReleasedMontageInstanceId)
		{
			continue;
		}

		float HitTime = 0.f;
		if (!FindHitNotifyTime(Montage, HitTime))
		{
			continue;
		}

		const float Position = MontageInstance->GetPosition();
		if (Position < HitTime - Step.pauseLeadSeconds || Position >= HitTime)
		{
			continue;
		}

		// 이미 실드를 켜둔 채로 공격을 받는 중이면 멈출 필요가 없다
		if (bResumeReady)
		{
			lastReleasedMontageInstanceId = InstanceId;
			continue;
		}

		AnimInstance->Montage_Pause(Montage);
		hitPausedAnimInstance = AnimInstance;
		hitPausedMontage = Montage;
		hitPausedInstanceId = InstanceId;

		// 오른쪽 위 단계 문구는 그대로 두고, 플레이어 몸 옆에 별도 말풍선으로 띄운다
		if (UC_TutorialPointerWidget* Overlay = EnsurePointerWidget())
		{
			// 폰트를 안내 위젯에서 물려받아야 한글이 깨지지 않는다
			const FSlateFontInfo Font = promptWidget ? promptWidget->GetInstructionFont() : FSlateFontInfo();
			const FSlateColor Color = promptWidget ? promptWidget->GetInstructionColor() : FSlateColor(FLinearColor::White);

			Overlay->ShowWorldCallout(PC ? PC->GetPawn() : nullptr, Step.pauseBeforeHitPrompt, Font, Color);
		}
		return;
	}
}

void UC_TutorialComponent::ResumeAttackMontage()
{
	const bool bWasPaused = hitPausedAnimInstance.IsValid() || hitPausedMontage.IsValid();
	if (!bWasPaused)
	{
		return;
	}

	UAnimInstance* AnimInstance = hitPausedAnimInstance.Get();
	UAnimMontage* Montage = hitPausedMontage.Get();
	if (AnimInstance && Montage)
	{
		AnimInstance->Montage_Resume(Montage);
	}

	lastReleasedMontageInstanceId = hitPausedInstanceId;
	hitPausedAnimInstance.Reset();
	hitPausedMontage.Reset();
	hitPausedInstanceId = INDEX_NONE;

	if (pointerWidget)
	{
		pointerWidget->HideWorldCallout();
	}
}

bool UC_TutorialComponent::FindHitNotifyTime(const UAnimMontage* Montage, float& OutTime)
{
	if (!Montage)
	{
		return false;
	}

	const FGameplayTag HitTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Monster.Melee.Hit"), false);
	bool bFound = false;

	for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
	{
		bool bIsHitNotify = NotifyEvent.Notify && NotifyEvent.Notify->IsA<UANC_MeleeNormalAttack>();

		if (!bIsHitNotify)
		{
			if (const UANC_MonsterGameplayEvent* EventNotify = Cast<UANC_MonsterGameplayEvent>(NotifyEvent.Notify))
			{
				bIsHitNotify = HitTag.IsValid() && EventNotify->eventTag == HitTag;
			}
		}

		if (!bIsHitNotify)
		{
			continue;
		}

		const float TriggerTime = NotifyEvent.GetTriggerTime();
		if (!bFound || TriggerTime < OutTime)
		{
			OutTime = TriggerTime;
			bFound = true;
		}
	}

	return bFound;
}
