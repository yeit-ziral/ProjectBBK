# Task: useItem (소비 아이템 사용 + 퀵슬롯 UI)

> **상태:** 진행 중
> **작성일:** 2026-07-11
> **담당:** 기용

---

## 목표
인벤토리의 소비 아이템(`FConsumableItemData.consumeEffects`)을 ASC에 GE로 적용하고 스택을 소모하는 `UseItem` 기능을 구현하고, 이를 키 입력으로 트리거하는 `WBP_UseItem` 퀵슬롯 위젯 2개를 `WBP_HUD`에 배치한다.

---

## 현재 상태
- 관련 파일:
  - `Source/ProjectBBK/Inventory/C_InventoryComponent.h/.cpp` — `AddItem`/`RemoveItem`/`HasItem`/`GetConsumableData` 등 존재. `UseItem`/퀵슬롯 기능 없음
  - `Source/ProjectBBK/Items/ItemData.h` — `FConsumableEffectEntry`(effect, magnitudeTag, magnitude), `FConsumableItemData.consumeEffects`
  - `Source/ProjectBBK/Equip/C_EquipmentComponent.cpp` — ASC 취득 패턴 참고용 (`GetOwner()` 캐스팅 후 `GetAbilitySystemComponent()`, 단 이쪽은 캐릭터에 부착됨)
  - `Source/ProjectBBK/Inventory/C_InventorySlotWidget.cpp` — 드래그&드롭 참고 패턴 (`NativeOnDragDetected`에서 `UDragDropOperation::Payload = this`, `NativeOnDrop`에서 `Cast<UC_InventorySlotWidget>(InOperation->Payload)`)
  - `Source/ProjectBBK/PlayerCharacter/PlayerAI/C_PlayerController.h/.cpp` — `IA_SwitchChar0`/`IA_SwitchChar1` 형태의 슬롯별 고정 Input Action 바인딩 패턴, `GetInventory()` 보유
  - `Source/ProjectBBK/PlayerCharacter/C_PlayerState.h` — `GetAbilitySystemComponent()` 제공 (PlayerState 경유 취득 지점)
- 현재 구현 현황: 소비 아이템 "사용" 로직 자체가 전무. 장착(`UC_EquipmentComponent::EquipItem`)만 구현됨. `WBP_HUD`는 C++ 베이스 없이 순수 Blueprint.

---

## 작업 범위

### 1. `UC_InventoryComponent::UseItem` (C++)
- [x] `bool UseItem(FName itemID)` 추가 (BlueprintCallable)
  - `GetConsumableData(itemID, data)` 실패 → false
  - `data.consumeEffects.IsEmpty()` → false (효과 없는 아이템의 무의미한 소모 방지)
  - `HasItem(itemID, 1)` 실패 → false (재고 확인이 GE 적용보다 먼저)
  - ASC 취득: `GetOwner()`를 `APlayerController`로 캐스트 → `GetPlayerState<UC_PlayerState>()` → `GetAbilitySystemComponent()` (PlayerState 경유, CLAUDE.md GAS 규칙 준수)
  - ASC 없음 → false
  - `consumeEffects` 순회: `ASC->MakeOutgoingSpec(entry.effect, 1.f, ASC->MakeEffectContext())` → `AssignTagSetByCallerMagnitude(entry.magnitudeTag, entry.magnitude)` → `ApplyGameplayEffectSpecToSelf`
  - 성공 시 `RemoveItem(itemID, 1)` 호출 후 true 반환

### 2. 퀵슬롯 등록/조회 (`UC_InventoryComponent`, C++)
- [x] `quickSlots` (`TArray<FName>`, private) — 슬롯 인덱스별 등록된 itemID (참조만, 인벤토리 슬롯과 무관). 생성자에서 고정 크기 2로 초기화
- [x] `RegisterQuickSlot(int32 SlotIndex, FName ItemID)` (BlueprintCallable) — 소비 아이템(`GetConsumableData` 성공)만 허용, 그 외 false
- [x] `UnregisterQuickSlot(int32 SlotIndex)` (BlueprintCallable) — 등록 해제
- [x] `GetQuickSlotItem(int32 SlotIndex) const -> FName` (BlueprintPure)
- [x] `UseQuickSlot(int32 SlotIndex) -> bool` (BlueprintCallable) — 등록된 itemID로 `UseItem` 호출. 재고 0이어도 등록은 유지(자동 해제 안 함)
- [x] `OnQuickSlotChanged` 델리게이트 (`FOnQuickSlotChanged`, int32 SlotIndex 파라미터) — 등록/해제/사용(재고 변화) 시 브로드캐스트

### 3. `WBP_UseItem` C++ 베이스 위젯
- [x] `UC_UseItemSlotWidget : public UUserWidget` 생성 (`Source/ProjectBBK/Inventory/C_UseItemSlotWidget.h/.cpp`)
  - BindWidget: `ItemIcon`(UImage), `QuantityText`(UTextBlock, OptionalWidget)
  - `SetSlotIndex(Inventory, Index)` — Inventory 주입 시점에 `OnQuickSlotChanged` 바인딩(재호출 시 기존 바인딩 해제 후 재등록) + `RefreshDisplay()`
  - `NativeOnDrop` — `Cast<UC_InventorySlotWidget>(InOperation->Payload)` → `GetItemID()` → `RegisterQuickSlot(slotIndex, itemID)` (인벤토리에서 제거하지 않음, 참조만. 소비 아이템 아니면 내부에서 거부)
  - `RefreshDisplay()` — 미등록 시 아이콘/수량 숨김. 등록 시 아이콘 표시, 재고 0이면 아이콘 `RenderOpacity=0.35`(반투명) + 수량 텍스트 숨김, 재고 1개 초과면 수량 텍스트 표시
- [ ] **(에디터 작업 필요)** `WBP_UseItem` UMG 에셋 생성, `UC_UseItemSlotWidget` reparent, BindWidget 매칭

### 4. Enhanced Input + PlayerController 연동 (2슬롯)
- [x] `IA_UseItem0`, `IA_UseItem1` (`UInputAction*`, EditDefaultsOnly) — `C_PlayerController.h`에 `IA_SwitchChar0/1`과 동일한 형태로 선언
- [x] `SetupInputComponent()`에 2개 `BindAction(..., ETriggerEvent::Started, this, &AC_PlayerController::OnUseItemSlot0Input / OnUseItemSlot1Input)` 추가
- [x] `OnUseItemSlot0Input()` / `OnUseItemSlot1Input()` — 각각 `inventory->UseQuickSlot(0)` / `UseQuickSlot(1)` 호출
- [ ] **(에디터 작업 필요)** `IA_UseItem0`/`IA_UseItem1` 에셋 생성 + `IA_UseItem0`/`IA_UseItem1`에 실제 키(예: 1/2) 매핑 + `playerMappingContext`(IMC)에 등록 + `BP_PlayerController`에서 두 슬롯에 할당

### 5. `WBP_HUD` 배치
- [ ] **(에디터 작업 필요)** `WBP_UseItem` 인스턴스 2개를 `WBP_HUD`에 추가 (Horizontal Box 등), 각각 `SetSlotIndex(Inventory, 0)`, `SetSlotIndex(Inventory, 1)` 호출 (Inventory는 `PlayerController->GetInventory()`)

### 6. 테스트 (PIE)
- [ ] 인벤토리 창을 열고 소비 아이템을 `WBP_UseItem`으로 드래그 → 아이콘 표시 확인 (인벤토리에는 그대로 남아있는지 확인)
- [ ] 대응 키 입력 → GE 적용(어트리뷰트 변화) + 인벤토리 스택 1 감소 + 퀵슬롯 수량 텍스트 갱신 확인
- [ ] 스택이 0이 될 때까지 사용 → 슬롯이 반투명 처리되고 키를 눌러도 아무 일 없는지 확인
- [ ] 같은 아이템을 다시 획득 → 별도 재드래그 없이 퀵슬롯이 다시 정상 표시/사용되는지 확인

---

## 제약 조건
- ASC는 반드시 PlayerState 경유로 취득 (`GetPlayerState<UC_PlayerState>()->GetAbilitySystemComponent()`) — Character 직접 참조 금지 (CLAUDE.md GAS 규칙)
- `UseItem`은 GA를 새로 만들지 않고 `UC_InventoryComponent`에서 직접 GE 적용 — `AC_ExpOrb`(스폰형 픽업 패턴)와 동일하게 GA 래퍼 없이 직접 호출하는 기존 선례를 따름
- 퀵슬롯 등록은 itemID 참조일 뿐 인벤토리 슬롯 이동/삭제를 유발하지 않음 (`MoveSlot`/`RemoveAtSlot` 호출 금지)
- 재고 0이 된 퀵슬롯은 자동 해제하지 않고 반투명 표시로만 처리 (재획득 시 재드래그 불필요)
- 데미지 처리 GE(GE_BasicDamage 등)와 무관 — 소비 효과는 각 아이템의 `consumeEffects`에 지정된 전용 GE만 사용, 신규 상태이상/버프가 필요하면 전용 GE 신규 생성 (하드코딩 금지)

---

## 완료 기준
- 소비 아이템 사용 시 GE 적용 확인 (PIE, 어트리뷰트 변화 로그/UI로 확인)
- 사용 시 인벤토리 스택 1 감소, 0개 시 슬롯 삭제 확인
- `WBP_HUD`에 배치된 2개의 `WBP_UseItem`이 각각 독립적으로 드래그 등록·표시·키 입력 사용 가능
- 인벤토리 창을 닫아도 퀵슬롯 등록 상태와 키 입력 사용이 유지됨

---

## 참고
| 항목 | 내용 |
|------|------|
| 관련 문서 | @docs/patterns.md (Set by Caller 데미지 적용 패턴, Set by Caller + CurveTable 레벨 스케일링 패턴, 상호작용형 픽업 아이템 패턴, 스폰형 픽업 아이템 패턴) |
| 관련 클래스 | `UC_InventoryComponent`, `UC_InventorySlotWidget`, `UC_EquipmentComponent`(ASC 취득 패턴 참고), `AC_PlayerController`, `UC_PlayerState`, `ItemData.h`(`FConsumableEffectEntry`) |
| 선행 태스크 | `tasks/task_Inventory.md` (인벤토리 코어, 완료) |
| 관련 Tag | 아이템별 `consumeEffects[i].magnitudeTag` (DT에서 지정, 예: `Data.Heal` 등 — GE의 Set by Caller 태그와 일치 필수) |

---

## 작업 로그
- 2026-07-11: 태스크 생성. 인벤토리 컴포넌트/장비 컴포넌트/드래그드롭 위젯 기존 코드 조사 후 설계 확정 — 퀵슬롯 상태는 `UC_InventoryComponent`에 저장(itemID 참조, 위젯은 표시 전용), `UseItem`은 HasItem→GE적용→RemoveItem 순서로 처리, 재고 0 시 자동 해제 대신 반투명 표시 유지, Enhanced Input은 `IA_SwitchChar0/1`과 동일한 슬롯별 고정 바인딩 패턴으로 2슬롯(`IA_UseItem0/1`) 채택.
- 2026-07-14: C++ 구현 완료 — `UC_InventoryComponent::UseItem`(HasItem→consumeEffects 순회 MakeOutgoingSpec/SetSetByCallerMagnitude/ApplyGameplayEffectSpecToSelf→RemoveItem, ASC는 `Cast<APlayerController>(GetOwner())->GetPlayerState<AC_PlayerState>()->GetAbilitySystemComponent()`로 취득) + 퀵슬롯 API(`quickSlots`, `RegisterQuickSlot`/`UnregisterQuickSlot`/`GetQuickSlotItem`/`UseQuickSlot`/`OnQuickSlotChanged`) 추가. `UC_UseItemSlotWidget` 신규 생성(드래그&드롭 등록, 재고 0 반투명 처리). `AC_PlayerController`에 `IA_UseItem0/1` 선언 + `SetupInputComponent` 바인딩 + 핸들러 추가. VS 프로젝트 파일 재생성+빌드 필요. 남은 작업은 전부 에디터(.uasset) 작업: `WBP_UseItem` 생성/reparent, `IA_UseItem0/1` 에셋+키 매핑+IMC 등록, `WBP_HUD`에 2개 배치, PIE 테스트.
