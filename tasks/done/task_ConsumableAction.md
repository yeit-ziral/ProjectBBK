# Task: ConsumableAction

> **상태:** 완료
> **작성일:** 2026-08-10
> **담당:** 기용

---

## 목표
GE 즉시 적용만으로 표현할 수 없는 소비 아이템 동작(AOE 대상 판정, 액터 스폰 등)을 위해, GA 없이 재사용 가능한 `UC_ConsumableAction` 베이스 구조를 만든다.

---

## 현재 상태
- 관련 파일:
  - `Source/ProjectBBK/Items/ItemData.h` (`FConsumableItemData`)
  - `Source/ProjectBBK/Items/C_ConsumableItem.h/.cpp`
  - `Source/ProjectBBK/Inventory/C_InventoryComponent.h/.cpp` (`UseItem`)
- 현재 구현 현황: `FConsumableItemData.consumeEffects`(`TArray<FConsumableEffectEntry>`)로 자기 자신 대상 즉발 GE만 지원. `UseItem()`이 `ApplyGameplayEffectSpecToSelf`로 고정돼 있어, 타 대상 AOE 적용이나 액터 스폰이 필요한 아이템(힐장판, 넉백 등)은 이 구조로 표현 불가.

---

## 작업 범위

### 1. `UC_ConsumableAction` 베이스 클래스
- [x] `UObject` 상속, `Blueprintable`, `Abstract` UCLASS로 생성 (`Source/ProjectBBK/Items/C_ConsumableAction.h/.cpp`)
- [x] `Execute(UAbilitySystemComponent* ASC, AActor* AvatarActor)` — `BlueprintNativeEvent`로 선언, 서브클래스가 BP/C++에서 구현

### 2. 아이템 데이터 연동
- [x] `FConsumableItemData`에 필드 추가: `TSubclassOf<UC_ConsumableAction> actionClass`
- [x] 기존 `consumeEffects`, `maxStack`, `cooldown` 필드는 변경 없음

### 3. `UC_InventoryComponent::UseItem()` 분기
- [x] 기존 `consumeEffects` 순회 적용 로직 유지 (자기 자신 대상 즉발 GE)
- [x] `data.actionClass`가 설정돼 있으면 `NewObject<UC_ConsumableAction>(this, data.actionClass)->Execute(ASC, AvatarActor)` 호출
- [x] "효과 없는 아이템" 조기 반환 가드를 `consumeEffects.IsEmpty() && !data.actionClass`로 수정 (액션만 있고 `consumeEffects`는 비어있는 아이템이 걸러지지 않도록)

### 4. 빌드 확인
- [x] 신규 클래스 컴파일 성공 (UnrealBuildTool `ProjectBBKEditor` Development Win64, 정상 컴파일 확인)
- [x] 기존 `consumeEffects`만 쓰는 소비 아이템 회귀 없음 — 코드 검토로 확인(`actionClass`가 None인 기존 아이템은 분기 로직상 동작 변화 없음). PIE 실행 테스트는 미실시(이번 태스크 완료 기준 범위 밖)

---

## 제약 조건
없음 (CLAUDE.md의 GAS 작업 규칙 기본 적용 — ASC는 `PlayerState` 경유로 취득)

---

## 완료 기준
- `UC_ConsumableAction` 클래스, `actionClass` 필드, `UseItem()` 분기 로직 구현 완료 및 빌드 성공
- 기존 `consumeEffects` 전용 소비 아이템이 회귀 없이 동일하게 동작
- 힐장판·넉백 등 실제 `UC_ConsumableAction` 서브클래스 구현은 이 태스크 범위 밖 — 완료 후 별도 태스크로 분리

---

## 참고
| 항목 | 내용 |
|------|------|
| 관련 문서 | @docs/decisions.md (UseItem 구현 위치 — GA 신설 vs `UC_InventoryComponent` 직접 구현) |
| 관련 문서 | @docs/patterns.md (지면 AOE 존 패턴 — `GA_Ablaze`/`AC_FireZone`, AOE Overlap 패턴 — `GA_MeleeUltimate`) — 후속 서브클래스 구현 시 참고 |
| 관련 클래스 | `FConsumableItemData`, `UC_InventoryComponent::UseItem`, `AC_FireZone` (힐장판 서브클래스 구조 참고용) |
| 후속 태스크(예정) | 힐장판 아이템(`AC_HealZone` + `UC_ConsumableAction` 서브클래스), 넉백 아이템(서브클래스), 필요 시 GA 브릿지 액션(`UC_ActivateAbilityConsumableAction`) |

---

## 작업 로그
- 2026-08-10: 태스크 생성. `UC_ConsumableAction` 설계 논의 완료 — GA 대신 UObject 기반 Action 클래스로 소비 아이템의 AOE 판정/액터 스폰 동작을 다형성으로 처리하기로 결정. 판단 기준: 대상이 자기 자신뿐이고 GE 적용만 필요하면 `consumeEffects`, 그 외(타 대상 AOE, 액터 스폰, GE 외 로직)는 `UC_ConsumableAction`. 힐장판/넉백 등 구체 서브클래스는 이 베이스 구조 완료 후 별도 태스크로 분리하기로 함.
- 2026-08-10: C++ 구현 완료 — `UC_ConsumableAction`(Items/C_ConsumableAction.h/.cpp), `FConsumableItemData.actionClass` 필드, `UC_InventoryComponent::UseItem()` 분기(가드 수정 + actionClass 실행) 반영. UnrealBuildTool `ProjectBBKEditor` Development Win64 빌드 성공 확인. 작업 완료.
