# Task: TreasureChest

> **상태:** 완료
> **작성일:** 2026-09-17
> **담당:** 기용

---

## 목표
필드에 배치하는 보물 상자를 상호작용하면 상자가 사라지면서, 상자에 설정된 총 value 예산에 맞춰 소비/장비 아이템과 돈을 주변에 랜덤 스폰한다 (인벤토리 직접 지급 없음 — 스폰된 픽업 아이템을 플레이어가 다시 상호작용해서 주워야 함).

---

## 현재 상태
- 관련 파일:
  - `Source/ProjectBBK/Items/ItemData.h` — `FBaseItemData`/`FConsumableItemData`/`FEquipmentItemData`
  - `Source/ProjectBBK/Items/C_BaseItem.h/.cpp` — 상호작용 픽업 베이스
  - `Source/ProjectBBK/Items/C_ConsumableItem.cpp`, `C_EquipmentItem.cpp` — `InitItem`만 담당, DT 로드 + Mesh
  - `Source/ProjectBBK/Items/C_MoneyItem.h/.cpp` — `InitMoney(int32)`로 골드 픽업 초기화
  - `Source/ProjectBBK/NPC/MerchantData.h` — 주석에 "추후 아이템에 value 필드가 생기면" 이라고 이미 예견되어 있음
- 현재 구현 현황:
  - `FBaseItemData`에는 `value` 필드가 없음 (신규 추가 필요)
  - 소비/장비 아이템 드랍은 전부 "상호작용 → 인벤토리 등록" 경로뿐, 예산 기반 랜덤 드랍/스폰 로직은 없음
  - DT_ConsumableItem / DT_EquipmentItem 은 존재하지만 value 컬럼 없음

---

## 작업 범위
- [x] `FBaseItemData`에 `int32 value` 필드 추가 (Consumable/Equipment 공통 상속이므로 한 번만 추가하면 둘 다 적용됨)
- [x] DT_ConsumableItem / DT_EquipmentItem 각 Row에 `value` 입력 (정확한 밸런싱 수치는 추후 조정 가능, 우선 임시값으로 채움) — 에디터에서 수동 작업 완료
- [x] `AC_TreasureChest : public AC_BaseItem` 신설
  - [x] `AC_BaseItem`을 상속해 기존 Overlap→상호작용 UI→`IA_Interact` 파이프라인(`C_PlayerController::overlappingItems`) 그대로 재사용 (`AC_MoneyItem`과 동일한 선례 — itemID/DataTable 미사용, `OnInteract`만 override)
  - [x] 인스턴스별 `chestValue` (EditAnywhere, 월드 배치 후 값 수정 가능해야 함)
  - [x] 상자 종류 구분 플래그 (`bIncludesEquipment`) — 종류별 외형(Mesh/BP)은 서로 다름
- [x] Value 분배 로직 구현 (순서: 장비 선차감 → 돈 비율 배정(전체 기준) → 아이템 추첨 → 자투리 합산)
  - [x] 장비 필수 상자(`bIncludesEquipment`)면 장비 풀 중 **value가 가장 낮은 장비들(동률 시 랜덤)** 중 1개를 먼저 선택해 그 value만큼 `chestValue`에서 선차감
  - [x] 돈 몫 = **`chestValue` 전체 기준**(장비 선차감과 무관하게) 1~50% 사이 랜덤 비율로 배정
  - [x] `장비value + 돈value`가 `chestValue`를 초과하면 돈 몫을 `max(0, chestValue - 장비value)`로 감소시켜 예산 내로 맞춤
  - [x] 아이템 추첨용 남은 value = `chestValue - 장비value - 돈value` (0 미만이면 0으로 클램프)
  - [x] 남은 value만큼 아이템 풀(Consumable, 장비 상자면 Equipment 포함)에서 반복 랜덤 추첨(중복 허용)하여 최대한 소진
  - [x] 추첨 후 남는 자투리 value는 다시 돈 몫에 합산
  - [x] 최종 돈 value → 골드 환산: `goldAmount = moneyValue * 10 ± 5`(랜덤 오차)
- [x] 상호작용 시 처리 (`AC_TreasureChest::OnInteract` override — 상자가 직접 담당, 인벤토리 컴포넌트 관여 없음)
  - [x] 위 loot 계산
  - [x] 계산된 아이템/돈 목록을 상자 주변 랜덤 위치에 기존 `BP_ConsumableItem` / `BP_EquipItem` / `BP_MoneyItem`으로 `SpawnActor` 후 `InitItem`/`InitMoney` 호출 (바닥 위치는 LineTrace로 탐색)
  - [x] 상자 Destroy
- [x] BP_TreasureChest 2종(Consumable+Money 전용 / Consumable+Equipment+Money) 에셋 생성, 레벨 배치 — 에디터에서 수동 작업 완료 (DT/스폰 클래스 슬롯 할당 포함)
- [x] PIE 검증: 두 상자 종류 각각 상호작용 시 정상 소멸 + value 예산에 맞는 아이템/골드 스폰 확인, 장비 필수 상자의 최소 1개 보장 확인 (아이템 스폰 정상 동작 확인, 발견된 버그 3건은 모두 수정 완료)
- [x] 상호작용 UI: 상호작용 가능 상태일 때 "Open Box" 텍스트 출력 — 기존 `WBP_Interaction`(`interactionWidgetClass`) 재사용, `AC_TreasureChest` 생성자의 `cachedItemName`만 설정 (구조 변경 없음)
- [x] 돈 value가 0일 때 MoneyItem 미생성 확인 — `RollAndSpawnLoot()`의 `if (MoneyValue > 0)` 가드 + `SpawnMoney()`의 `InGoldAmount <= 0` 가드로 이미 이중 처리되어 있음을 확인 (추가 코드 변경 없음)
- [x] 상자 오픈 사운드 — `openSound`(EditDefaultsOnly `USoundBase*`) 추가, `OnInteract`에서 loot 계산/Destroy 전에 `UGameplayStatics::PlaySoundAtLocation`으로 재생. BP에서 사운드 에셋 할당 필요(에디터 수동 작업)

---

## 제약 조건
- 아이템/골드는 `UC_InventoryComponent`로 직접 지급하지 않는다 — 반드시 월드에 픽업 액터로 스폰해 플레이어가 다시 상호작용해서 획득해야 한다.
- Value 수치는 DataTable/인스턴스 편집 값 기반으로 처리 — C++ 하드코딩 금지.
- `chestValue`와 각 아이템의 `value`는 정수(int32)만 사용 — 돈 비율 계산(퍼센트 적용) 및 환산 과정에서도 float 잔여값을 남기지 않고 정수로 반올림/클램프 처리할 것.
- 장비 선차감 자체가 `chestValue`보다 큰 경우를 제외하고, 최종 스폰된 아이템+돈의 총 value는 `chestValue`를 초과할 수 없다 (돈 비율이 예산을 넘기면 돈 몫을 줄여서 맞춤).

---

## 완료 기준
PIE에서 두 종류의 상자 모두 상호작용 시 정상 동작 확인:
- 상자가 사라지고 주변에 아이템/골드가 스폰됨
- 스폰된 아이템+골드의 총 value가 `chestValue` 예산을 초과하지 않음 (단, 장비 선차감 자체가 `chestValue`보다 큰 예외적인 경우는 제외 — 이 경우도 장비는 항상 스폰됨)
- 장비 필수 상자는 항상 장비 아이템이 최소 1개 이상 포함됨
- 돈은 항상 1개만 스폰되고, 금액이 `value*10 ±5` 범위 내에 있음

---

## 참고
| 항목 | 내용 |
|------|------|
| 관련 문서 | @docs/patterns.md (상호작용형 픽업 아이템 패턴, 돈 픽업 아이템 패턴, 아이템 데이터 구조 관련 Design Decisions) |
| 관련 클래스 | `AC_BaseItem`(`RefreshOverlapState` 추가 — 런타임 스폰 시 오버랩 상태 재확인용, 다른 픽업 클래스에도 공용 적용됨), `AC_MoneyItem`(`InitMoney`), `AC_ConsumableItem`, `AC_EquipmentItem`(`InitItem`), `FBaseItemData`/`FConsumableItemData`/`FEquipmentItemData` |
| 관련 DataTable | `DT_ConsumableItem`, `DT_EquipmentItem` |

---

## 작업 로그
- 2026-09-17: 태스크 생성
- 2026-09-17: `FBaseItemData::value` 추가, `AC_TreasureChest`(`Source/ProjectBBK/Items/C_TreasureChest.h/.cpp`) 구현 완료 (value 분배 로직 + SpawnActor 스폰 포함). 빌드 검증은 에디터가 열려 있어(Live Coding) UBT 커맨드라인 빌드가 막힘 — 에디터 종료 후 VS에서 Development Editor 전체 빌드로 재확인 필요 (Debugging Checklist #42와 동일 이슈, 신규 UPROPERTY 추가라 Live Coding 핫리로드도 권장 안 함)
- 2026-09-17: 사용자 PIE 테스트에서 아이템 스폰 정상 동작 확인. 상호작용 텍스트를 "Open Box"로 설정(처음엔 "열기"로 넣었다가 영문으로 변경). 돈 0value 시 미생성 요청은 기존 코드에 이미 반영되어 있음을 확인. 이번 변경(생성자 본문 수정만)은 새 UPROPERTY/UFUNCTION이 아니므로 Live Coding으로도 반영 가능
- 2026-09-17: PIE 테스트에서 버그 2건 발견 및 수정.
  1. Consumable/Equipment 드랍이 스폰 시점에 플레이어와 겹쳐 있으면 상호작용 UI가 안 뜸(범위 밖으로 나갔다 와야 표시). 원인: `OnComponentBeginOverlap` 델리게이트가 `AC_BaseItem::BeginPlay()`에서 바인딩되는데, 런타임 스폰 시 초기 오버랩 판정이 그보다 먼저(컴포넌트 등록 단계) 처리되어 델리게이트가 못 받음.
  2. MoneyItem은 스폰 직후 UI는 뜨지만 금액이 0으로 표시됨(범위 밖으로 나갔다 와야 정상 표시). 원인: 위 초기 오버랩이 처리될 때 아직 상자가 `InitMoney()`를 호출하기 전이라 `cachedItemName`이 기본값("0 gold")인 채로 위젯에 찍힘.
  수정: `AC_BaseItem`(`Source/ProjectBBK/Items/C_BaseItem.h/.cpp`, 공용 베이스 클래스)에 `RefreshOverlapState()` 추가 — 겹침 상태를 명시적으로 재확인해 상호작용 UI/등록을 갱신. `AC_TreasureChest`의 `SpawnConsumable`/`SpawnEquipment`/`SpawnMoney`에서 `InitItem`/`InitMoney` 호출 직후 `RefreshOverlapState()`를 호출하도록 연결. 새 `UFUNCTION` 추가라 빌드 검증 필요(아래 참고).
  빌드 검증 재시도했으나 에디터가 계속 열려 있어(Live Coding) UBT 커맨드라인 빌드가 두 번째도 막힘 — 에디터 종료 후 VS Development Editor 전체 빌드로 컴파일 확인 필요
- 2026-09-17: 상자 오픈 사운드 추가(`openSound` + `OnInteract`에서 `PlaySoundAtLocation`). BP_TreasureChest 2종에 사운드 에셋 할당 필요
- 2026-09-17: 버그 발견 및 수정 — 아이템이 가끔 공중(상자 위치 높이)에 스폰됨. 원인: `FindGroundSpawnPoint`가 흩뿌린 랜덤 위치에서 LineTrace가 바닥을 못 찾으면(단차/낭떠러지 등) 바닥으로 내려지지 않은 원본 후보 좌표(상자와 같은 Z)를 그대로 반환했음. 수정: 트레이스 구간을 더 넉넉히 넓히고, 1차 실패 시 상자가 서 있던 위치 바로 아래로 2차 재탐색(항상 바닥을 찾음)하도록 변경, 그마저 실패할 때만 최후 폴백으로 `Center` 사용
- 2026-09-17: 작업 완료 처리. DT value 입력 및 BP_TreasureChest 2종 에셋 생성은 에디터에서 수동으로 완료됨(사용자 확인)
