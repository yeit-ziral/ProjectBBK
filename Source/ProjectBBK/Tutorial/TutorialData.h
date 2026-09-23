// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "TutorialData.generated.h"

class UInputAction;
class UUserWidget;

/**
 * 튜토리얼 한 단계 — 안내 문구 + 완료 조건.
 * 조건 타입 enum 없이 필드 조합으로 표현한다.
 *   externalEventTag 설정  → 입력 무시, NotifyExternalEvent로만 완료
 *   requiredHoldSeconds > 0 → 조건을 만족한 입력의 누적 시간으로 판정
 *   그 외                   → requiredCount 횟수로 판정
 * 문구·수치는 전부 DataTable 값이므로 재빌드 없이 에디터에서 조정 가능.
 */
USTRUCT(BlueprintType)
struct FTutorialStepData : public FTableRowBase
{
	GENERATED_BODY()

	// 진행 순서 (오름차순). UDataTable::GetAllRows는 행 순서를 보장하지 않으므로
	// 이 값으로 명시적으로 정렬한다. 같은 값이면 Row Name 순.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	int32 order = 0;

	// 화면 우상단에 표시할 안내 문구
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FText instruction;

	// 완료 조건이 되는 입력 액션 (IA_Move, IA_SwitchNextChar 등).
	// externalEventTag가 설정된 경우 무시된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TSoftObjectPtr<UInputAction> requiredAction;

	// 축 입력 방향 필터. ZeroVector면 방향을 보지 않는다.
	// AC_BasePlayerCharactor::MyMove의 규약과 동일 — W=(0,1) S=(0,-1) D=(1,0) A=(-1,0)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FVector2D requiredDirection = FVector2D::ZeroVector;

	// 정규화된 입력 방향과의 내적이 이 값 이상이어야 인정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float directionTolerance = 0.7f;

	// 반대 방향 입력도 인정할지 — "A 또는 D"처럼 축만 맞으면 되는 단계용.
	// requiredDirection이 (1,0)이면 D와 A 모두, (0,1)이면 W와 S 모두 통과한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	bool bAcceptOppositeDirection = false;

	// 횟수 조건 — requiredHoldSeconds가 0일 때만 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial", meta = (ClampMin = "1"))
	int32 requiredCount = 1;

	// 누적 시간 조건 — 0보다 크면 횟수 대신 이쪽으로 판정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial", meta = (ClampMin = "0.0"))
	float requiredHoldSeconds = 0.f;

	// 키를 누른 것만으로는 인정하지 않고, 캐릭터가 실제로 이 속도(cm/s) 이상으로 움직이는 동안만 인정한다.
	// 0이면 검사하지 않는다 (시점 회전처럼 이동이 없는 단계).
	// 공격 몽타주 중에는 입력이 Triggered로 들어와도 캐릭터가 제자리이므로 게이지가 차지 않게 된다.
	// TutorialText.txt에서 "RowName.requireMove=true" 또는 "RowName.requireMove=속도" 로 지정할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial", meta = (ClampMin = "0.0"))
	float minMoveSpeed = 0.f;

	// 키를 누른 것만으로는 인정하지 않고, 실제로 공격 몽타주가 재생된 횟수로 판정한다.
	// 쿨다운·자원 부족으로 어빌리티가 발동하지 않은 연타 입력(좌클릭 연타)을 걸러낸다.
	// 켜지면 requiredAction 입력은 세지 않는다 — 몽타주 재생만으로 진행된다.
	// 근접 평타(UC_MeleeAttackGA)는 한 번의 활성화 안에서 콤보를 돌리므로 어빌리티 발동 횟수로는 셀 수 없다.
	// TutorialText.txt에서 "RowName.requireAttack=true" 또는 "RowName.requireAttack=이름조각"으로 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	bool bRequireAttackMontage = false;

	// bRequireAttackMontage 단계에서 인정할 몽타주 애셋 이름 조각 (대소문자 무시).
	// 비어 있으면 어떤 몽타주든 인정하므로, 회피 등 다른 몽타주가 섞이는 단계에서는 지정할 것.
	// 예: 근접 평타 "MeleeAttack" (AM_MeleeAttack_A/B/C) · 원거리 평타 "Fire" (Primary_Fire_Med_Montage)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FString attackMontageNameFilter;

	// 입력으로 표현되지 않는 단계용 확장 포인트 (드래그&드롭 장착 등).
	// 설정 시 requiredAction 대신 NotifyExternalEvent(EventTag)로 완료된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FGameplayTag externalEventTag;

	// 완료 후 다음 단계로 넘어가기까지의 여유 (문구를 읽을 시간)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial", meta = (ClampMin = "0.0"))
	float delayAfterComplete = 1.0f;

	// 이 단계에 진입할 때 마나(궁극기 게이지)를 최대치로 채운다.
	// 궁극기 단계처럼 자원이 모자라면 아예 시연할 수 없는 단계용.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	bool bFillManaOnEnter = false;

	// 이 단계에 진입할 때 플레이어 앞에 스폰하고, 단계를 벗어날 때 제거할 액터.
	// 튜토리얼용 더미 몬스터처럼 그 단계에서만 존재해야 하는 대상에 쓴다.
	// 미리 배치해두고 숨기는 방식(revealActorTag)과 달리 AI·Tick이 아예 돌지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Spawn")
	TSoftClassPtr<AActor> spawnActorClass;

	// 스폰 위치 — 플레이어 전방으로 떨어뜨릴 거리(cm). 지면 높이는 LineTrace로 찾는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Spawn", meta = (ClampMin = "0.0"))
	float spawnDistance = 400.f;

	// 이 단계에서 스폰한 몬스터의 공격 몽타주를 데미지 판정(히트 노티파이) 직전에 멈추고 이 문구를 띄운다.
	// 비워두면 멈추지 않는다. 막기처럼 타이밍 맞추기가 어려운 단계에서 플레이어에게 여유를 주기 위함.
	// TutorialText.txt에서 "RowName.pauseHit=문구" 형식으로 지정할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|HitPause")
	FText pauseBeforeHitPrompt;

	// 플레이어 ASC가 이 태그를 가지면 멈춘 공격을 이어서 재생한다 (예: State.Shield).
	// 비어 있으면 풀 방법이 없으므로 pauseBeforeHitPrompt가 있어도 멈추지 않는다.
	// TutorialText.txt에서 "RowName.resumeTag=태그" 형식으로 지정할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|HitPause")
	FGameplayTag pauseResumeTag;

	// 히트 노티파이보다 몇 초 앞에서 멈출지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|HitPause", meta = (ClampMin = "0.01"))
	float pauseLeadSeconds = 0.15f;

	// 이 단계에 진입할 때 레벨의 모든 픽업 아이템(AC_BaseItem)을 나타나게 한다.
	// 튜토리얼 시작 시점에 미리 숨겨두므로, 아이템 획득 단계 전에는 월드에 보이지 않는다.
	// TutorialText.txt에서 "RowName.reveal=items" 형식으로 지정할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Reveal")
	bool bRevealItemsOnEnter = false;

	// 이 단계에 진입할 때 나타나게 할 액터의 Actor Tag (아이템 외 임의 액터용).
	// bRevealItemsOnEnter와 마찬가지로 튜토리얼 시작 시점에 미리 숨겨둔다.
	// TutorialText.txt에서 "RowName.reveal=태그이름" 형식으로 지정할 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Reveal")
	FName revealActorTag = NAME_None;

	// 이 단계에서 화살표로 가리킬 화면 위젯의 클래스 (예: C_UltimateGaugeWidget).
	// 화면에 떠 있는 인스턴스를 클래스로 찾아 가리킨다.
	// pointerTargetWidgetName과 함께 쓰면 "이 클래스의 위젯 안에서 그 이름의 자식"으로 범위가 좁혀진다.
	// 둘 다 비어 있으면 화살표를 표시하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Pointer")
	TSoftClassPtr<UUserWidget> pointerTargetClass;

	// 자기 클래스를 갖지 않는 내부 위젯을 가리킬 때 쓰는 이름 (예: WBP_HUD 안의 "StaminaBar").
	// 디자이너 계층의 위젯 이름과 정확히 같아야 하며, Is Variable 체크 여부와는 무관하다.
	// TutorialText.txt에서 "RowName.pointer=위젯이름" 형식으로 덮어쓸 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Pointer")
	FName pointerTargetWidgetName = NAME_None;

	// 화살표와 함께 띄울 보조 설명 문구.
	// TutorialText.txt에서 "RowName.hint=문구" 형식으로 덮어쓸 수 있다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Pointer")
	FText pointerHint;
};
