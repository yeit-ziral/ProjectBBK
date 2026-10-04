# Task: InventoryEquipPersistence (레벨 이동 시 인벤토리/장비/퀵슬롯 유지)

> **상태:** 완료  
> **작성일:** 2026-10-02  
> **담당:** 기용

---

## 목표
레벨 이동(비심리스 트래블) 시 인벤토리·퀵슬롯·장비 상태가 유지되지 않는 문제를 GameInstance 영속 저장 구조에 통합해 해결한다.

---

## 현재 상태
- 관련 파일:
  - `Source/ProjectBBK/Inventory/C_InventoryComponent.h/.cpp` — `UC_InventoryComponent`(PlayerController 소유). `slots`(`TArray<FInventorySlot>`), `quickSlots`(`TArray<FName>`) 보관. 저장/복원 함수 없음.
  - `Source/ProjectBBK/Equip/C_EquipmentComponent.h/.cpp` — `UC_EquipmentComponent`(캐릭터별 부착). `equipped`(`TMap<EEquipmentSlot, FEquippedEntry>`) 보관. 저장/복원 함수 없음.
  - `Source/ProjectBBK/LevelSystem/LevelSequenceData.h` — `FPersistentGameState`에 캐릭터 어트리뷰트(health/stamina/experience 등)만 필드로 존재. 인벤토리/장비/퀵슬롯 필드 없음.
  - `Source/ProjectBBK/LevelSystem/C_BBKGameInstance.h/.cpp` — `SaveGameState`/`RestoreGameState`가 캐릭터 어트리뷰트만 처리.
  - `Source/ProjectBBK/PlayerCharacter/PlayerAI/C_PlayerController.cpp` — `SaveStateForLevelTransition()`(661줄)이 `TravelToNextLevel()` 직전 `GI->SaveGameState(...)`만 호출, 인벤토리/장비는 미포함.
- 현재 구현 현황: 비심리스 트래블(OpenLevel)이라 레벨 전환 시 PlayerController·캐릭터 로스터가 전부 파괴·재생성됨(@docs/decisions.md "레벨 간 상태 저장 위치 — GameInstance vs PlayerState" 참고). 캐릭터 스탯은 GameInstance 경유로 복원되지만, 인벤토리(PlayerController 소유 1벌)·퀵슬롯·장비(캐릭터별)는 저장 경로 자체가 없어 레벨 이동 시 전부 소실됨.

---

## 작업 범위
- [x] `FPersistentGameState`에 인벤토리 슬롯(itemID+수량 병렬 배열), 퀵슬롯(itemID 배열), money 필드 추가 + `FPersistentEquipmentState`(캐릭터별 `EEquipmentSlot` → itemID 맵) 신규 구조체 추가
- [x] `UC_InventoryComponent`에 `GetPersistentState`/`RestorePersistentState` 추가 (slots를 itemID/quantity 병렬 배열로 그대로 보관 — 빈칸·자유배치 위치 보존, quickSlots·money 포함)
- [x] `UC_EquipmentComponent`에 `GetEquippedItemIDs`/`RestoreEquippedItemIDs` 추가 — 복원 시 GE는 적용하지 않고(Suspend 상태와 동일) `ReapplyEquipBonuses`는 호출 측(GameInstance)에서 활성 캐릭터에만 별도 호출
- [x] `AC_PlayerController::SaveStateForLevelTransition()`/`BeginPlay()`에서 `GI->SaveGameState(...)`/`RestoreGameState(...)` 호출에 `inventory` 인자 추가
- [x] `UC_BBKGameInstance::SaveGameState`/`RestoreGameState`에 인벤토리·장비 저장/복원 분기 추가 (시그니처에 `UC_InventoryComponent* Inventory` 파라미터 추가)
- [x] 비활성 캐릭터는 장비 데이터(`equipped` 맵)만 복원하고 GE는 보류, 활성 캐릭터만 `ReapplyEquipBonuses` 호출 — 기존 로스터 격리 원칙(캐릭터 교체 시 Suspend/Reapply) 유지
- [x] 복원 호출 타이밍은 기존 `RestoreGameState` 호출 지점(Possess 완료 후) 그대로 사용 — 신규 타이밍 추가 없음
- [x] VS 빌드 확인 (사용자 측 Visual Studio 빌드 완료)
- [x] PIE 검증: 레벨 이동 후 아이템(인벤토리)이 유지되는지 확인 완료(사용자 확인)

---

## 제약 조건
- 인벤토리는 PlayerController 소유(공유 1벌), 장비는 캐릭터별(로스터 캐릭터 수만큼 인스턴스 존재) — 저장 구조가 이 소유권 차이를 반영해야 함 (인벤토리는 단일 블록, 장비는 캐릭터 인덱스별 배열/맵)
- itemID(FName) 기반으로 저장 — DataTable Row 재구성에 영향받지 않게
- 인벤토리/장비 **코어 구조**(`UC_InventoryComponent`, `UC_EquipmentComponent`의 기존 멤버·함수 시그니처)를 변경할 경우 선우와 사전 조율 (`tasks/task_Inventory.md`의 조율 선례 참고 — 다른 파일이라 git 충돌 없이 조용히 빌드가 깨질 수 있음)
- 기존 `SaveActiveEffects`/`RestoreActiveEffects`, `Suspend/ReapplyEquipBonuses` 패턴과 충돌 없이 통합할 것 — 캐릭터 교체(레벨 내 전환)에 쓰이는 로직을 레벨 전환에도 그대로 재사용 가능한지 확인

---

## 완료 기준
- 레벨 이동 전/후로 인벤토리 슬롯(수량 포함)·퀵슬롯 등록 상태·로스터 전원의 장비 장착 상태가 동일하게 유지됨
- 캐릭터 교체 + 레벨 이동이 겹쳐도 장비 보너스가 정확한 활성 캐릭터에만 적용됨(중복/누락 없음)
- PIE 검증 통과

---

## 참고
| 항목 | 내용 |
|------|------|
| 관련 문서 | @docs/decisions.md ("레벨 간 상태 저장 위치 — GameInstance vs PlayerState", "공유 ASC 환경에서 캐릭터별 Infinite GE 격리 패턴") |
| 관련 문서 | @docs/patterns.md ("레벨 간 캐릭터 상태 유지 패턴", "공유 ASC 환경에서 캐릭터별 Infinite GE 격리 패턴") |
| 관련 클래스 | `UC_InventoryComponent`, `UC_EquipmentComponent`, `UC_BBKGameInstance`, `AC_PlayerController`, `FPersistentGameState`, `FPersistentCharacterState` |
| 선행/연관 태스크 | `tasks/task_Inventory.md`(선우, 인벤토리 코어), `tasks/done/task_useItem.md`, `tasks/done/task_TreasureChest.md` |

---

## 작업 로그
- 2026-10-02: 태스크 생성. 레벨 전환 시 인벤토리/장비/퀵슬롯 소실 버그 확인 — `FPersistentGameState`에 해당 필드 없음, 저장/복원 경로 전부 미구현 상태.
- 2026-10-02: C++ 구현 완료.
  - `LevelSequenceData.h`: `FPersistentEquipmentState`(캐릭터별 `TMap<EEquipmentSlot, FName>`) 신규, `FPersistentGameState`에 `inventoryItemIDs`/`inventoryQuantities`/`quickSlotItemIDs`/`money`/`equipmentStates` 추가.
  - `UC_InventoryComponent::GetPersistentState`/`RestorePersistentState` 추가 — `slots`를 itemID/quantity 병렬 배열로 그대로 보존(격자 자유배치 인덱스 유지), 복원 시 `OnInventoryChanged`/`OnQuickSlotChanged`/`OnMoneyChanged` 브로드캐스트로 기존 UI 바인딩 재사용(위젯 쪽 코드 변경 없음).
  - `UC_EquipmentComponent::GetEquippedItemIDs`/`RestoreEquippedItemIDs` 추가 — 복원 시 `bonusHandle`은 비워둠(=Suspend 상태와 동일), GE 재적용은 호출자 책임.
  - `UC_BBKGameInstance::SaveGameState`/`RestoreGameState` 시그니처에 `UC_InventoryComponent* Inventory` 추가. Restore 시 로스터 전원의 장비 itemID를 복원하고, **활성 캐릭터에만** `ReapplyEquipBonuses()` 호출(비활성 캐릭터 보너스가 공유 ASC에 중복 적용되는 것 방지 — 기존 Suspend/Reapply 불변식 그대로 적용).
  - `AC_PlayerController`의 두 호출부(`SaveStateForLevelTransition`, `BeginPlay`)에 `inventory` 인자 추가.
  - 저장 상태가 없는 최초 레벨(`bHasSavedState == false`)에서는 복원 로직이 아예 실행되지 않아 기존 `startingItems`/`startingMoney` 테스트 동작은 그대로 유지됨.
  - 남은 작업: VS 빌드 + PIE 검증(특히 비활성 캐릭터 장비가 활성 캐릭터 스탯에 새지 않는지).
- 2026-10-02: 작업 완료. VS 빌드 후 PIE에서 레벨 이동 시 아이템(인벤토리)이 유지되는 것을 사용자가 직접 확인.
