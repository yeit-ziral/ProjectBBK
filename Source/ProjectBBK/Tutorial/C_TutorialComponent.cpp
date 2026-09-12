// Fill out your copyright notice in the Description page of Project Settings.

#include "C_TutorialComponent.h"
#include "C_TutorialPromptWidget.h"
#include "C_TutorialPointerWidget.h"
#include "../PlayerCharacter/C_PlayerState.h"
#include "../Items/C_BaseItem.h"
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

UC_TutorialComponent::UC_TutorialComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

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
	}

	UnbindInputActions();
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

		const float Dot = FVector2D::DotProduct(InputDir.GetSafeNormal(), Step->requiredDirection.GetSafeNormal());
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

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(manaRefillTimer);
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

// ---------------------------------------------------------------------------
// 화살표 / 자원 보정
// ---------------------------------------------------------------------------

void UC_TutorialComponent::UpdateStepPointer(const FTutorialStepData& Step)
{
	UClass* TargetClass = Step.pointerTargetClass.IsNull() ? nullptr : Step.pointerTargetClass.LoadSynchronous();
	const FName TargetName = Step.pointerTargetWidgetName;
	const bool bHasTarget = (TargetClass != nullptr) || !TargetName.IsNone();

	UWidget* Target = bHasTarget ? FindPointerTarget(TargetClass, TargetName) : nullptr;

	if (!Target)
	{
		if (bHasTarget)
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

	if (!pointerWidget)
	{
		APlayerController* PC = Cast<APlayerController>(GetOwner());
		if (!PC)
		{
			return;
		}

		pointerWidget = CreateWidget<UC_TutorialPointerWidget>(PC, UC_TutorialPointerWidget::StaticClass());
		if (!pointerWidget)
		{
			return;
		}

		// 앵커·크기를 지정하지 않아 뷰포트 슬롯 기본값(0,0,1,1)으로 전체 화면을 덮는다 (Debugging Checklist #56)
		pointerWidget->AddToViewport(pointerZOrder);
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
}

void UC_TutorialComponent::DestroyStepActors()
{
	for (const TWeakObjectPtr<AActor>& WeakActor : stepSpawnedActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
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
