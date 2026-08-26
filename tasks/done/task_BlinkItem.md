# Task: BlinkItem (순간이동 소비 아이템)

> **상태:** 완료
> **작성일:** 2026-08-18
> **담당:** 기용

---

## 목표
사용 시 캐릭터 전방으로 즉시 순간이동하는 신규 소비 아이템(BlinkItem)을 `UC_ConsumableAction` 서브클래스로 구현하고, 기존 퀵슬롯·쿨다운·재고알림 시스템에 그대로 등록해 사용 가능하게 한다.

---

## 현재 상태
- 관련 파일:
  - `Source/ProjectBBK/Items/C_ConsumableAction.h/.cpp` — 베이스 클래스, `Execute_Implementation(ASC, AvatarActor)` BlueprintNativeEvent
  - `Source/ProjectBBK/Items/C_KnockbackAction.h/.cpp` — GE 없이 순수 액터 조작만 하는 액션의 선례 (`LaunchCharacter`, `radius`/`knockbackForce` `EditDefaultsOnly`)
  - `Source/ProjectBBK/Inventory/C_InventoryComponent.cpp:258` (`UseItem`) — `data.consumeEffects.IsEmpty() && !data.actionClass` 조건으로 이미 "GE 없이 actionClass만 있는 아이템" 허용 확인됨 (BlinkItem처럼 `consumeEffects`를 비워도 문제없이 사용 가능) → `IsItemOnCooldown` 체크 → `HasItem` 체크 → `actionClass` 있으면 `NewObject` 후 `Execute` 호출 → `RemoveItem` → 쿨다운 기록 → `NotifyQuickSlotsForItem`
  - `Source/ProjectBBK/Items/ItemData.h` — `FConsumableItemData.actionClass`(`TSubclassOf<UC_ConsumableAction>`), `cooldown` 필드
  - `Source/ProjectBBK/Inventory/C_UseItemSlotWidget.h/.cpp` — 퀵슬롯 위젯, 쿨다운 오버레이 기존 구현됨 (`task_useItem.md` 섹션 7·8·9)
- 현재 구현 현황: BlinkItem 관련 코드 전무. `UC_ConsumableAction` 프레임워크와 퀵슬롯/쿨다운/재고알림 시스템은 이미 완성되어 있어 신규 액션 서브클래스 + DT row만 추가하면 됨.

---

## 작업 범위

### 1. `UC_BlinkAction` (C++)
- [x] `UC_BlinkAction : public UC_ConsumableAction` 생성 (`Source/ProjectBBK/Items/C_BlinkAction.h/.cpp`, `UC_KnockbackAction`과 동일한 클래스 구조)
- [x] `blinkDistance` (`float`, `EditDefaultsOnly`, 기본값 600) 프로퍼티 추가
- [x] `Execute_Implementation(ASC, AvatarActor)` 구현:
  - `TargetLocation = AvatarActor->GetActorLocation() + AvatarActor->GetActorForwardVector() * blinkDistance`
  - `LineTraceSingleByChannel`(캐릭터 위치 → `TargetLocation`)로 벽/장애물 충돌 체크 (`ActorsToIgnore`에 `AvatarActor` 필수)
  - `bBlockingHit == true`면 `ImpactPoint`에서 캐릭터 캡슐 반경만큼 앞으로 당긴 지점을 최종 위치로 사용 (벽 통과 방지), 아니면 `TargetLocation` 그대로 사용
  - `AvatarActor->SetActorLocation(FinalLocation, false)` (Sweep 없이 즉시 이동 — 순간이동이므로 `bSweep=false`)

### 2. DataTable 등록
- [x] `DT_ConsumableItem`에 BlinkItem row 추가 — `itemID`, `itemName`, `description`, `itemIcon`, `worldMesh`, `cooldown` 값 입력
- [x] `actionClass`에 `UC_BlinkAction` (또는 BP 서브클래스) 할당, `consumeEffects`는 비워둠 (GE 적용 없는 순수 액션이므로 — `UseItem()`이 이미 이 케이스를 허용하도록 구현돼 있음, 섹션 "현재 상태" 참고)

### 3. 월드에 BlinkItem 생성
- [x] `BP_ConsumableItem` 배치 후, `itemID`를 BlinkItem row로 지정 (기존 소비 아이템 배치 패턴과 동일)

### 4. 퀵슬롯 연동 테스트 (PIE)
- [x] `WBP_UseItem`에 BlinkItem 드래그 등록 → 아이콘 표시 확인
- [x] 대응 키 입력 → 캐릭터가 전방으로 즉시 이동 확인
- [x] 벽을 마주본 상태에서 사용 → 벽을 통과하지 않고 벽 앞에서 멈추는지 확인
- [x] 쿨다운 중 재사용 시도 → 차단 + 오버레이 표시 확인 (기존 쿨다운 시스템 재사용 확인)
- [x] 재고 0 상태에서 사용 시도 → 알림 사운드만 재생 확인 (기존 재고알림 시스템 재사용 확인)

---

## 제약 조건
- GE를 사용하지 않는 순수 액터 위치 조작 — `UC_KnockbackAction`과 동일하게 `UC_ConsumableAction` 서브클래스로 구현 (GA 신설 금지)
- 이동 방향은 카메라가 아닌 캐릭터 전방(`ForwardVector`) 기준
- 벽/장애물 통과 방지를 위한 `LineTrace` 충돌 체크 필수

---

## 완료 기준
- 퀵슬롯에 등록한 BlinkItem 사용 시 캐릭터가 전방으로 즉시 이동 (PIE 확인)
- 벽 앞에서 사용 시 벽을 통과하지 않음 (PIE 확인)
- 기존 쿨다운/재고 없음 알림 시스템이 BlinkItem에도 동일하게 정상 동작

---

## 참고
| 항목 | 내용 |
|------|------|
| 관련 문서 | @docs/patterns.md (소비 아이템 복합 동작 패턴, 3인칭 카메라 지면 위치 탐색 패턴 — LineTrace 사용 참고) |
| 관련 클래스 | `UC_ConsumableAction`, `UC_KnockbackAction`(구조 참고), `UC_InventoryComponent`, `ItemData.h` |
| 선행 태스크 | `tasks/task_useItem.md` (퀵슬롯·쿨다운·재고알림 시스템, 완료) |

---

## 작업 로그
- 2026-08-18: 태스크 생성. BlinkItem은 즉시 순간이동(자기 자신 대상, 캐릭터 전방 `ForwardVector` 기준), `LineTrace` 충돌 체크로 벽 통과 방지, 쿨다운 있음(기존 시스템 재사용), 퀵슬롯(1/2키)으로 사용으로 확정. `UC_ConsumableAction` 서브클래스(`UC_BlinkAction`)로 구현 예정. `UC_InventoryComponent::UseItem()`이 `consumeEffects` 없이 `actionClass`만 있는 아이템을 이미 허용함을 코드로 확인 — 별도 수정 불필요.
- 2026-08-18: `UC_BlinkAction` C++ 구현(`LineTraceSingleByChannel`(`ECC_Visibility`)로 벽 충돌 체크 후 캡슐 반경만큼 당겨 `SetActorLocation`) + DT row·`BP_BlinkItem` 등록 + 퀵슬롯 연동 PIE 테스트까지 전부 완료 확인. 작업 완료.
