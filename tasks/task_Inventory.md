# Task: Inventory (인벤토리 코어)

> **상태:** 진행 중  
> **작성일:** 2026-06-23  
> **담당:** 선우

---

## 목표
아이템 보관/추가/제거/조회/스택을 담당하는 **독립 인벤토리 컴포넌트(코어)** 를 구현한다. 나중에 플레이어(또는 PlayerState)에 부착해 사용한다. UI·화폐·장비 장착·플레이어 통합은 후속 태스크.

---

## 현재 상태
- 관련 파일 (기용님 BaseItem 작업, 이미 존재):
  - `Source/ProjectBBK/Items/ItemData.h` — `FBaseItemData`(itemID:FName, itemName, description, itemIcon, worldMesh), `FConsumableItemData`(consumeEffect, magnitudeTag, magnitude, **maxStack=99**), `FEquipmentItemData`(equipSlot, bonus 5종)
  - `C_BaseItem` / `C_ConsumableItem` / `C_EquipmentItem` — 월드 드롭 아이템 액터
  - `DT_ConsumableItem`, `DT_EquipmentItem`, `GE_EquipBonus`
- 현재 구현 현황: **인벤토리·화폐 시스템 없음.** 아이템 정의·월드 드롭·소비 상호작용까지만 존재.

---

## 작업 범위 (코어만 — UI/화폐/장착/통합 제외)

### 1. 인벤토리 컴포넌트
- [ ] `UC_InventoryComponent` (`UActorComponent`) 생성 — 독립 클래스, 나중에 PlayerState/캐릭터에 부착
- [ ] `consumableTable` / `equipmentTable` (`UDataTable*`, EditDefaultsOnly) — itemID 조회용 (소유자/BP에서 지정)
- [ ] `maxSlots` (int32, EditDefaultsOnly) — 용량 제한

### 2. 슬롯 데이터 구조
- [ ] `FInventorySlot` USTRUCT { `FName itemID`; `int32 quantity`; }
- [ ] `TArray<FInventorySlot> slots` 보관

### 3. 추가/제거/조회 로직 (BlueprintCallable)
- [ ] `AddItem(FName itemID, int32 count = 1)` — **항상 병합**: 같은 아이템의 기존 스택을 `maxStack`까지 먼저 채우고(획득 순서 유지), 초과분만 뒤에 새 슬롯 추가. 장비는 비스택(슬롯당 1). 슬롯 가득 시 못 넣은 잔여 수량 반환
- [ ] `RemoveItem(FName itemID, int32 count = 1)` / `RemoveAtSlot(int32 index, int32 count = 1)`
- [ ] `GetItemCount(FName itemID)` / `HasItem(FName itemID, int32 count = 1)`
- [ ] `GetSlots()` 조회 (const)
- [ ] 아이템 타입/스택 판별 헬퍼 — itemID가 소비/장비 DT 어디에 있는지 `FindRow` → maxStack·스택 여부 결정

### 4. 변경 알림
- [ ] `OnInventoryChanged` 델리게이트 (BlueprintAssignable) — 슬롯 변경 시 브로드캐스트 (후속 UI 바인딩용, 이번엔 바인딩 없음)

### 5. 테스트
- [ ] 임의 액터에 컴포넌트 부착 → BlueprintCallable로 AddItem/RemoveItem 호출 → `GetSlots`/로그로 스택 누적·용량 제한·제거 동작 확인

---

## 제약 조건
- **화폐(골드)·인벤토리 UI·장비 장착(GE_EquipBonus Apply/Remove)·플레이어 통합은 제외** — 전부 후속 태스크
- 아이템 정의·스택 한도는 기존 DataTable / `FConsumableItemData.maxStack` 재사용 — **하드코딩 금지** (CLAUDE.md 데이터 구동 원칙)
- 소비 = `maxStack`까지 스택 / 장비 = 비스택(슬롯당 1개)
- **담당 조율 필요:** 기용님 `task_BaseItem.md` 후속 계획에 "인벤토리 시스템"이 이미 있음 → 누가 구현할지 합의 (`.uasset`/코드 동시 편집 금지). 인벤토리는 플레이어에 부착되므로 **통합 시점·위치는 지호(플레이어)와 협의**
- **[중요] `ItemData.h` `maxStack` 의존** — `GetMaxStack`이 `FConsumableItemData.maxStack`를 읽음. 기용님 "소비 다중 GE" 개선 시 `maxStack` 필드를 지우거나 이름 바꾸면 **인벤토리 컴파일 깨짐**. 이건 서로 다른 파일이라 git 충돌도 안 뜨고(무충돌 머지 → 빌드에서 터짐), 기용님 쪽 Claude도 그 트리에 인벤토리 파일이 없어 경고 못 함 → **기용님과 직접 공유: "maxStack 유지·이름 유지, DT 재입력 시 행별 값 보존, 바꾸면 알려주기"**

---

## 완료 기준
- `UC_InventoryComponent`가 UI·플레이어 통합 없이 **독립 컴파일·동작**
- AddItem/RemoveItem/조회/스택/용량 제한이 정상 동작
- 임의 액터에 부착 후 BlueprintCallable 테스트 통과
- 후속(UI/화폐/장착/플레이어 통합)에서 붙일 지점(`OnInventoryChanged`, 부착 대상)이 명확히 분리되어 있을 것

---

## 참고
| 항목 | 내용 |
|------|------|
| 관련 문서 | @docs/patterns.md (스폰형 픽업 아이템 패턴 — C_ExpOrb) |
| 관련 클래스 | `C_BaseItem`, `C_ConsumableItem`, `C_EquipmentItem`, `ItemData.h` |
| 관련 데이터 | `DT_ConsumableItem`, `DT_EquipmentItem` |
| 선행/연관 태스크 | `tasks/task_BaseItem.md` (기용 — 아이템 클래스·DT·인벤토리 후속 계획) |
| 후속 태스크 | 인벤토리 UI / 화폐(골드) / 장비 장착 / 상점 NPC(`npc`) |

---

## 작업 로그
- 2026-06-23: 태스크 생성 (상점 NPC → 인벤토리 선행으로 전환 결정). 저장 방식은 독립 `UActorComponent`로 만들어 추후 플레이어/PlayerState에 부착하기로 결정.
- 2026-06-23: 슬롯 규칙 확정 — **항상 병합(같은 아이템 한 스택부터 채움) + 획득 순서**. 정렬/수동배치/중복 부분스택은 안 함(후속 UI에서).
- 2026-06-23: C++ 코어 구현 — `UC_InventoryComponent`(AddItem/RemoveItem/RemoveAtSlot/GetItemCount/HasItem/GetSlots/GetMaxStack) + `FInventorySlot` + `OnInventoryChanged`. DataTable(maxStack/장비=1) 기반 검증. (사용자 VS 빌드 예정)
- 2026-06-23: UI 지원 헬퍼 `GetItemData(itemID, out FBaseItemData)` 추가 — 소비/장비 DT를 베이스 뷰로 조회해 이름·아이콘·설명·Mesh 반환(UI가 itemID만으로 표시 가능).
- 2026-06-23: 인벤토리 컴포넌트 + UI 위젯 텍스처/외형 `Source/ProjectBBK/Monster/UI/Inventory/` · `/Game/Monster/UI/Inventory/`로 이동(사용자 요청, 작업자 기준 정리).
- 2026-06-23: UI 로직을 BP 그래프 대신 **C++ 위젯 베이스**로 구현(프로젝트 관행 일치) — `UC_InventorySlotWidget`(BindWidget: ItemIcon/QuantityText, `SetSlot`), `UC_InventoryWidget`(BindWidget: SlotGrid, `SetInventory`/`RefreshInventory`, OnInventoryChanged 바인딩, slotWidgetClass·columns 노출). WBP 2개 reparent + slotWidgetClass=WBP_InventorySlot 지정 완료.
- 2026-06-23: 디버그용 `AC_InventoryTester`(BeginPlay에서 SetItemTables+AddItem+위젯표시) + `UC_InventoryComponent::SetItemTables` 추가. MonsterTest에 배치해 PIE 검증.
- 2026-06-23: **PIE 동작 확인 완료** — 슬롯 3개(99/51 병합분할 + 검) 96×96 딱 붙어 정렬. 버그였던 슬롯 거대화 = 슬롯 SizeBox Override 체크 꺼짐(bOverride_*) → ON으로 수정(디버깅 체크리스트 34번 케이스). DT Row Name(HpPotion/IronSword)으로 추가해야 함(itemID 필드 아님).
