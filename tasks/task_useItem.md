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
- [x] `WBP_UseItem` UMG 에셋 생성, `UC_UseItemSlotWidget` reparent, BindWidget 매칭

### 4. Enhanced Input + PlayerController 연동 (2슬롯)
- [x] `IA_UseItem0`, `IA_UseItem1` (`UInputAction*`, EditDefaultsOnly) — `C_PlayerController.h`에 `IA_SwitchChar0/1`과 동일한 형태로 선언
- [x] `SetupInputComponent()`에 2개 `BindAction(..., ETriggerEvent::Started, this, &AC_PlayerController::OnUseItemSlot0Input / OnUseItemSlot1Input)` 추가
- [x] `OnUseItemSlot0Input()` / `OnUseItemSlot1Input()` — 각각 `inventory->UseQuickSlot(0)` / `UseQuickSlot(1)` 호출
- [x] `IA_UseItem0`/`IA_UseItem1` 에셋 생성 + 키(1/2) 매핑 + `playerMappingContext`(IMC)에 등록 + `BP_PlayerController`에서 두 슬롯에 할당

### 5. `WBP_HUD` 배치
- [x] `WBP_UseItem` 인스턴스 2개를 `WBP_HUD`에 추가, 각각 `SetSlotIndex(Inventory, 0)`, `SetSlotIndex(Inventory, 1)` 호출 (Inventory는 `PlayerController->GetInventory()`)

### 6. 테스트 (PIE)
- [x] 인벤토리 창을 열고 소비 아이템을 `WBP_UseItem`으로 드래그 → 아이콘 표시 확인 (인벤토리에는 그대로 남아있는지 확인)
- [x] 대응 키 입력 → GE 적용(어트리뷰트 변화) + 인벤토리 스택 1 감소 + 퀵슬롯 수량 텍스트 갱신 확인
- [x] 스택이 0이 될 때까지 사용 → 슬롯이 반투명 처리되고 키를 눌러도 아무 일 없는지 확인
- [x] 같은 아이템을 다시 획득 → 별도 재드래그 없이 퀵슬롯이 다시 정상 표시/사용되는지 확인

### 7. 소비 아이템 쿨다운 (추가 요구사항)
- [x] `FConsumableItemData`에 `cooldown` 필드 추가 (`float`, 기본값 0 = 쿨다운 없음) — DT에서 아이템별 값 입력, 하드코딩 금지
- [x] `UC_InventoryComponent`에 아이템별 쿨다운 종료 시각 저장소 추가 (`TMap<FName, float> itemCooldownEndTime` 등) — GAS/GE 없이 `GetWorld()->GetTimeSeconds()` 기반 타임스탬프로 관리. `UseItem`은 GA가 아니므로 "Cooldown: GE_GenericCooldown 공유" 규칙과 무관(스킬 쿨다운과 별개 체계)
- [x] `IsItemOnCooldown(FName itemID) const -> bool`, `GetItemCooldownRemaining(FName itemID) const -> float` (BlueprintPure) 추가
- [x] `UseItem`에 쿨다운 체크 추가 — `IsItemOnCooldown(itemID)`가 true면 GE 적용 전에 false 반환 (재고 확인과 동일하게 GE 적용보다 먼저 체크)
- [x] `UseItem` 성공 시 `data.cooldown > 0`이면 `itemCooldownEndTime[itemID] = 현재시각 + data.cooldown` 기록
- [x] 적용 범위: 아이템별 개별 쿨다운 (같은 itemID만 잠김, 다른 소비 아이템 사용에는 영향 없음 — 전체 공용 쿨다운 아님)
- [x] `WBP_UseItem`(`UC_UseItemSlotWidget`)에 쿨다운 오버레이 추가 — `WBP_SkillIcon`과 동일한 Progress 오버레이 패턴 재사용(Progress = 남은시간/전체쿨다운, 줄어드는 방향 — @docs/decisions.md 쿨다운 오버레이 방향 참고)
  - `OnQuickSlotChanged` 수신(=사용 성공) 시 오버레이 시작, 이후 Tick 또는 타이머로 `GetItemCooldownRemaining` 폴링해 갱신
  - 남은 시간 0 도달 시 오버레이 숨김
- [x] 테스트(PIE): 쿨다운 있는 소비 아이템 연속 사용 시도 → 쿨다운 중 재사용 불가(GE 미적용, 스택 미소모) + 오버레이 표시 확인 / 쿨다운 종료 후 정상 재사용 확인 / 서로 다른 소비 아이템은 쿨다운 중에도 독립적으로 사용 가능한지 확인

### 8. 재고 없음 알림 사운드 (추가 요구사항)
- [x] `UC_InventoryComponent`에 `OnQuickSlotUseFailed(int32 SlotIndex)` 델리게이트 추가 (`FOnQuickSlotChanged`와 동일 시그니처 재사용 가능)
- [x] `UseQuickSlot(SlotIndex)`에서 등록된 itemID는 있으나 재고 0(`HasItem(itemID, 1)` 실패)이면 `OnQuickSlotUseFailed.Broadcast(SlotIndex)` 후 false 반환. 슬롯 자체가 미등록(itemID None)인 경우는 브로드캐스트하지 않고 그냥 false만 반환(소리 없음)
- [x] `UC_UseItemSlotWidget`에 `outOfStockSound`(`USoundBase*`, `EditAnywhere`, Category "Inventory|QuickSlot|Sound") 추가
- [x] `SetSlotIndex`에서 `OnQuickSlotUseFailed`도 `OnQuickSlotChanged`와 동일한 방식으로 바인딩(재호출 시 기존 바인딩 해제 후 재등록), `NativeDestruct`에서 해제
- [x] 핸들러에서 `SlotIndex` 일치 시 `UGameplayStatics::PlaySound2D(this, outOfStockSound)` 호출 — `C_UltimateGaugeWidget::readySound`/`PlaySound2D` 패턴 참고 (프로젝트 내 UI 알림 사운드 유일 선례)
- [x] `WBP_UseItem` 에디터에서 `outOfStockSound`에 사운드 에셋 할당 (2개 인스턴스 공용 사운드 하나로 충분)
- [x] 테스트(PIE): 등록된 아이템의 재고가 0인 상태에서 대응 키 입력 → 사운드만 재생되고 GE 적용·스택 변화 없는지 확인. 미등록 슬롯에서 키 입력 시 사운드가 재생되지 않는지 확인

### 9. 퀵슬롯 아이템 교체 시 쿨다운 UI 잔존 버그 수정
- [x] `UC_UseItemSlotWidget::RestoreCooldownState()`에서 새로 등록된 아이템에 쿨다운이 없는 경우(`GetItemCooldownRemaining(itemID) <= 0.f`) 조기 `return` 대신 `currentCooldownTime = 0.f` / `maxCooldownTime = 0.f` 리셋 후 `SetCooldownVisible(false)` 호출
  - 원인: 드래그&드롭으로 슬롯에 아이템을 교체 등록하면 `RegisterQuickSlot`(`C_InventoryComponent.cpp:311`) → `OnQuickSlotChanged` 브로드캐스트 → `OnQuickSlotChangedHandler`(`C_UseItemSlotWidget.cpp:87`) → `RestoreCooldownState()` 경로를 타는데, 새 아이템이 쿨다운 중이 아니면 조기 return되어 이전 아이템의 `currentCooldownTime`이 리셋되지 않음. `NativeTick`이 이 값을 계속 감소시키며 오버레이를 그대로 표시 (`SetSlotIndex`의 리셋 로직(69~73줄)은 이 경로에서 호출되지 않음 — Debugging Checklist #25와 동일 원인)
- [ ] 테스트(PIE): 쿨다운 진행 중인 아이템 슬롯에 (a) 쿨다운 없는 소비 아이템으로 교체 등록 → 오버레이 즉시 사라짐 확인 (b) 쿨다운 중인 다른 소비 아이템으로 교체 등록 → 새 아이템의 남은 시간으로 오버레이 갱신 확인

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
- 쿨다운이 설정된 소비 아이템은 쿨다운 중 재사용이 차단되고(GE 미적용, 스택 미소모), 종료 후 정상 재사용 가능 (PIE 확인)
- `WBP_UseItem`에 쿨다운 오버레이가 사용 시점부터 종료까지 정상 표시/소멸
- 등록된 아이템의 재고가 0인 상태에서 사용 시도 시 알림 사운드가 재생되고, 미등록 슬롯에서는 재생되지 않음 (PIE 확인)

---

## 참고
| 항목 | 내용 |
|------|------|
| 관련 문서 | @docs/patterns.md (Set by Caller 데미지 적용 패턴, Set by Caller + CurveTable 레벨 스케일링 패턴, 상호작용형 픽업 아이템 패턴, 스폰형 픽업 아이템 패턴), @docs/decisions.md (쿨다운 오버레이 방향 — 줄어드는 방향 vs 차오르는 방향) |
| 관련 클래스 | `UC_InventoryComponent`, `UC_InventorySlotWidget`, `UC_EquipmentComponent`(ASC 취득 패턴 참고), `AC_PlayerController`, `UC_PlayerState`, `ItemData.h`(`FConsumableEffectEntry`), `C_UltimateGaugeWidget`(UI 알림 사운드 패턴 참고) |
| 선행 태스크 | `tasks/task_Inventory.md` (인벤토리 코어, 완료) |
| 관련 Tag | 아이템별 `consumeEffects[i].magnitudeTag` (DT에서 지정, 예: `Data.Heal` 등 — GE의 Set by Caller 태그와 일치 필수) |

---

## 작업 로그
- 2026-07-11: 태스크 생성. 인벤토리 컴포넌트/장비 컴포넌트/드래그드롭 위젯 기존 코드 조사 후 설계 확정 — 퀵슬롯 상태는 `UC_InventoryComponent`에 저장(itemID 참조, 위젯은 표시 전용), `UseItem`은 HasItem→GE적용→RemoveItem 순서로 처리, 재고 0 시 자동 해제 대신 반투명 표시 유지, Enhanced Input은 `IA_SwitchChar0/1`과 동일한 슬롯별 고정 바인딩 패턴으로 2슬롯(`IA_UseItem0/1`) 채택.
- 2026-07-14: C++ 구현 완료 — `UC_InventoryComponent::UseItem`(HasItem→consumeEffects 순회 MakeOutgoingSpec/SetSetByCallerMagnitude/ApplyGameplayEffectSpecToSelf→RemoveItem, ASC는 `Cast<APlayerController>(GetOwner())->GetPlayerState<AC_PlayerState>()->GetAbilitySystemComponent()`로 취득) + 퀵슬롯 API(`quickSlots`, `RegisterQuickSlot`/`UnregisterQuickSlot`/`GetQuickSlotItem`/`UseQuickSlot`/`OnQuickSlotChanged`) 추가. `UC_UseItemSlotWidget` 신규 생성(드래그&드롭 등록, 재고 0 반투명 처리). `AC_PlayerController`에 `IA_UseItem0/1` 선언 + `SetupInputComponent` 바인딩 + 핸들러 추가. VS 프로젝트 파일 재생성+빌드 필요. 남은 작업은 전부 에디터(.uasset) 작업: `WBP_UseItem` 생성/reparent, `IA_UseItem0/1` 에셋+키 매핑+IMC 등록, `WBP_HUD`에 2개 배치, PIE 테스트.
- 2026-07-14: 드래그&드롭 등록 시 인벤토리 원본 슬롯 아이콘이 사라지는 버그 수정 — `NativeOnDragDetected`가 숨긴 아이콘/수량은 기존엔 `OnInventoryChanged`(그리드 재생성) 또는 `NativeOnDragCancelled`에서만 복구됐는데, `WBP_UseItem` 드롭은 `OnInventoryChanged`를 브로드캐스트하지 않아 복구가 누락됐음. `UC_InventorySlotWidget::RestoreDisplay()` 공개 함수로 분리해 `UC_UseItemSlotWidget::NativeOnDrop`에서도 호출하도록 수정.
- 2026-07-14: 에디터 작업(`WBP_UseItem` 생성, IMC 키 매핑, `WBP_HUD` 배치) + PIE 테스트까지 전부 완료 확인. 추가 요구사항으로 "소비 아이템 쿨다운" 도입 결정 — 인벤토리 컴포넌트 타임스탬프 기반(GAS/GE 미사용, `UseItem`이 GA가 아니므로 `GE_GenericCooldown` 공유 규칙과 무관), 아이템별 개별 쿨다운(전체 공용 아님), `WBP_UseItem`에 `WBP_SkillIcon`과 동일한 Progress 오버레이 추가로 확정. 섹션 7로 작업 범위 추가.
- 2026-07-14: 쿨다운 구현에 GE를 쓰지 않기로 한 이유 정리 — (1) `UseItem`은 애초에 GA가 아니라 `UC_InventoryComponent`에서 직접 GE를 적용하는 구조(스폰형 픽업 패턴과 동일 선례)라, "Cooldown: GE_GenericCooldown 공유" 규칙은 GA 쿨다운을 겨냥한 것이지 소비 아이템 쿨다운에는 적용 대상이 아님. (2) GE_GenericCooldown 방식을 쓰려면 아이템별로 구분되는 동적 GameplayTag(예: `Cooldown.Item.HpPotion`)가 필요한데, 태그는 `DefaultGameplayTags.ini`에 사전 등록해야 하므로 아이템을 추가할 때마다 태그도 함께 등록해야 함 — DataTable Row 추가만으로 신규 아이템을 등록하던 기존 데이터 구동 흐름이 깨짐. (3) 프로젝트는 싱글플레이어 전제(레벨 간 상태 저장 결정과 동일 근거)라 GE의 서버 권위·복제 이점이 무의미함. → `TMap<FName, float>` 타임스탬프 방식은 신규 아이템 추가 시 DT의 `cooldown` 필드만 채우면 되고, 별도 태그·GE 에셋 관리가 필요 없어 유지비용이 가장 낮음.
- 2026-07-14: 추가 요구사항으로 "재고 없음 알림 사운드" 도입 결정 — `C_UltimateGaugeWidget`의 `readySound`/`PlaySound2D` 패턴이 프로젝트 내 유일한 UI 알림 사운드 선례라 이를 재사용. `UC_InventoryComponent::UseQuickSlot`에서 등록된 아이템의 재고가 0일 때만(미등록 슬롯은 제외) `OnQuickSlotUseFailed(SlotIndex)`를 브로드캐스트하고, `UC_UseItemSlotWidget`이 이를 구독해 `outOfStockSound`를 재생하는 구조로 확정. 섹션 8로 작업 범위 추가.
- 2026-08-18: 퀵슬롯 아이템 교체 시 이전 아이템 쿨다운 UI가 남는 버그 확인 — `RestoreCooldownState()`가 새 아이템 쿨다운 없을 때 조기 return하며 이전 상태를 리셋하지 않는 것이 원인. 섹션 9로 작업 범위 추가, 아직 미구현.
- 2026-08-18: 섹션 9 C++ 수정 완료 — `RestoreCooldownState()`를 "쿨다운 활성 여부"를 먼저 판별한 뒤, 비활성이면 무조건 `currentCooldownTime`/`maxCooldownTime`을 0으로 리셋하고 `SetCooldownVisible(false)`를 호출하도록 변경(기존엔 조건별로 조기 return만 하고 리셋이 없었음). 빌드 필요. 남은 작업은 PIE 테스트.
- 2026-07-21: 섹션 7·8 C++ 구현 완료 — `FConsumableItemData.cooldown` 필드, `UC_InventoryComponent`에 `itemCooldownEndTime`(TMap 타임스탬프)/`IsItemOnCooldown`/`GetItemCooldownRemaining` 추가 + `UseItem`에 쿨다운 체크(재고 확인 다음, GE 적용 전)·성공 시 종료시각 기록, `OnQuickSlotUseFailed` 델리게이트(등록됨+재고 0일 때만 브로드캐스트) 추가. `UC_UseItemSlotWidget`에 `C_SkillIconWidget`과 동일한 패턴(다이나믹 머티리얼 Progress 파라미터 + NativeTick 폴링)으로 `CooldownOverlay`/`CooldownText`/`UpdateCooldown`/`SetCooldownVisible` 추가, `SetSlotIndex`/`OnQuickSlotChanged` 양쪽에서 `RestoreCooldownState()`로 진행 중 쿨다운 복원, `outOfStockSound` + `OnQuickSlotUseFailed` 바인딩으로 `PlaySound2D` 재생. 빌드 필요. 남은 작업은 전부 에디터: DT_ConsumableItem에 `cooldown` 컬럼 값 입력, `WBP_UseItem`에 `CooldownOverlay`(Image)·`CooldownText`(TextBlock) 위젯 추가+머티리얼 할당(`WBP_SkillIcon`의 쿨다운 머티리얼 재사용 가능), `outOfStockSound` 사운드 할당, PIE 테스트.
