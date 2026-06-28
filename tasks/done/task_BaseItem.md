# Task: BaseItem

> **상태:** 완료  
> **작성일:** 2026-06-18  
> **담당:** 기용

---

## 목표
인벤토리·필드 드롭·장비의 기반이 되는 BaseItem 클래스와 소비/장비 서브클래스를 구현한다.

---

## 현재 상태
- 관련 파일: 없음
- 현재 구현 현황: 아이템 관련 코드·에셋 전혀 없음. 기존 C_ExpOrb(픽업 패턴)만 참고 가능.

---

## 작업 범위

### 1. 데이터 구조
- [x] `FBaseItemData` 구조체 정의 (아이템 ID, 이름, 설명, 아이콘, 월드 Mesh 등 공통 필드)
- [x] ~~`FConsumableItemData` 구조체 정의 (consumeEffect, magnitudeTag, magnitude, maxStack)~~ → 개선 사항으로 대체
- [x] `EEquipmentSlot` enum 정의 (None, Head, Chest, Legs, Weapon, Accessory)
- [x] `FEquipmentItemData` 구조체 정의 (equipSlot + SetByCaller 5종: MaxHealth, MaxStamina, MoveSpeed, Defense, Damage)
- [x] `DefaultGameplayTags.ini`에 `Data.Equip.*` 태그 5개 추가
- [x] `GE_EquipBonus` 공용 GE 생성 (에디터) — Duration: Infinite, Modifier 5개 (SetByCaller)
- [x] 소비 아이템 DataTable 생성 (에디터)
- [x] 장비 아이템 DataTable 생성 (에디터)

### 2. BaseItem 클래스
- [x] `C_BaseItem` AActor 클래스 생성 — 월드에 스폰되는 아이템 액터의 기반
- [x] Collision(SphereComponent) + `UStaticMeshComponent` + `UWidgetComponent` 구성
- [x] `InitItem(FName ItemID)` — 서브클래스에서 DT 로드 → `ApplyWorldMesh` (미지정 시 DefaultMesh)
- [x] 플레이어 Overlap 시 상호작용 UI(WidgetComponent) 표시
- [x] 플레이어 Overlap 해제 시 상호작용 UI 숨김
- [x] ~~상호작용 키 입력 시 `OnInteract` 호출 (Tick에서 키 체크, Overlap 시에만 Tick 활성화)~~ → 개선 사항으로 대체
- [x] `OnInteract` — 인벤토리에 itemID 추가 + Destroy (효과 적용은 인벤토리에서 담당)
- 월드 스폰은 `BP_ConsumableItem` / `BP_EquipItem`만 사용 (C++ 직접 스폰 금지)
- 스폰 주체(인벤토리 드롭, 몬스터 사망 등)가 `SpawnActor` → `InitItem(ItemID)` 호출하는 구조

### 3. 상호작용 UI
- [x] `UC_InteractionWidget` 위젯 클래스 생성 (BindWidget: InteractionText)
- [x] `C_BaseItem`의 WidgetComponent가 Overlap 시 표시 / 해제 시 숨김
- [x] WBP_Interaction Blueprint 생성 (에디터) — InteractionText 텍스트블록 배치

### 4. 소비 아이템 클래스
- [x] `C_ConsumableItem` (C_BaseItem 상속) 클래스 생성
- [x] `InitItem` — consumableDataTable에서 FConsumableItemData 로드 + itemName/Mesh 적용
- OnInteract 제거 — 베이스 클래스가 인벤토리 추가 처리

### 5. 장비 아이템 클래스
- [x] `C_EquipmentItem` (C_BaseItem 상속) 클래스 생성
- [x] `InitItem` — equipmentDataTable에서 FEquipmentItemData 로드 + itemName/Mesh 적용
- OnInteract 제거 — 베이스 클래스가 인벤토리 추가 처리
- 사용/장착/해제 로직은 `UC_InventoryComponent` 태스크에서 구현

### 6. 돈(골드) 아이템 클래스
- [x] `C_MoneyItem` (C_BaseItem 상속) 클래스 생성
- [x] `moneyAmount` (EditAnywhere int32) — 인스턴스별 금액 설정
- [x] `BeginPlay` — `FText::Format("{0} gold", moneyAmount)`로 cachedItemName 포맷 후 Super 호출, ApplyWorldMesh(nullptr)로 defaultMesh 적용
- [x] `OnInteract` — `Inv->AddMoney(moneyAmount)` + Destroy
- itemID·DataTable 불필요. BP_Money에서 defaultMesh + moneyAmount만 설정

### 6. 테스트
- [x] 소비 아이템 1종 DataTable 등록 + 필드 드롭 → Overlap → 상호작용 UI 표시 → 키 입력 → 효과 적용 + Destroy 확인

---

## 완료 기준
- BaseItem 클래스, 소비 아이템 클래스, 장비 아이템 클래스 생성 완료
- 소비 아이템 1개가 필드 드롭 → Overlap 상호작용 UI → 키 입력 → 효과 적용 + Destroy 정상 동작
- 인벤토리 구현 시 교체할 임시 로직이 명확히 분리되어 있을 것

---

## 참고
| 항목 | 내용 |
|------|------|
| 관련 문서 | @docs/patterns.md (스폰형 픽업 아이템 패턴 — C_ExpOrb 참고) |
| 관련 클래스 | `C_ExpOrb` (Overlap → GE 적용 → Destroy 패턴) |
| GAS 규칙 | 데미지/회복 등 수치 적용은 반드시 전용 GE 사용 (GA 직접 수치 적용 금지) |

---

## 개선 사항

### 1. 상호작용 입력 방식 변경 (Tick → EnhancedInput)
- [x] `C_BaseItem`에서 Tick 기반 키 체크 제거
- [x] `PlayerController`에 `IA_Interact` InputAction 등록
- [x] Overlap 시 `PlayerController`에 현재 아이템 참조 전달, 해제 시 초기화
- [x] `PlayerController`가 상호작용 키 입력 감지 → 참조 중인 아이템의 `OnInteract()` 호출
- [x] 다중 Overlap 대응: 단일 포인터 → `overlappingItems` 배열로 변경. 가장 최근 진입 아이템과 상호작용, Destroy 시 다음 아이템으로 자동 전환

### 2. 소비 아이템 다중 GE 지원
- [x] `FConsumableEffectEntry` 구조체 추가 (Effect, MagnitudeTag, Magnitude 묶음)
- [x] `FConsumableItemData` 구조체 수정
  - 단일 `consumeEffect` / `magnitudeTag` / `magnitude` → `TArray<FConsumableEffectEntry> consumeEffects`
- [x] `C_ConsumableItem::OnInteract` — 배열 순회하며 GE Spec 생성 + SetByCaller 주입 + Apply
- [x] DT_ConsumableItem 에디터에서 기존 데이터 새 구조에 맞게 수정

---

## 작업 로그
- 2026-06-18: 태스크 생성
- 2026-06-18: C++ 구현 완료 (빌드 성공)
  - `Source/ProjectBBK/Items/` 폴더 생성
  - ItemData.h, C_BaseItem, C_InteractionWidget, C_ConsumableItem, C_EquipmentItem
  - DefaultGameplayTags.ini에 Data.Equip.* 태그 5개 추가
  - 참고: 플레이어 AttributeSet에 `defense` 어트리뷰트 없음 — GE_EquipBonus 설정 시 추가 필요 (지호 담당)
- 2026-06-22: 에디터 작업 완료
  - GE_EquipBonus, DT_ConsumableItem, DT_EquipmentItem, WBP_Interaction, BP_ConsumableItem, BP_EquipItem 생성
  - 소비 아이템 상호작용 동작 확인 완료
- 2026-06-22: 개선 사항 도출 — 상호작용 입력 방식 및 소비 아이템 다중 GE 지원
- 2026-06-25: 개선 사항 C++ 구현 완료 (빌드 성공)
  - 개선 1: Tick 키 체크 제거, PlayerController에 IA_Interact 바인딩 + SetCurrentInteractable/ClearCurrentInteractable 추가, C_BaseItem Overlap에서 PlayerController에 참조 전달
  - 개선 2: FConsumableEffectEntry 구조체 추가 + FConsumableItemData를 TArray<FConsumableEffectEntry>로 변경
  - OnInteract를 인벤토리 등록 방식으로 변경: BaseItem이 인벤토리에 itemID 추가 + Destroy. 서브클래스(C_ConsumableItem, C_EquipmentItem)에서 OnInteract 제거 — InitItem(DT 로드 + Mesh)만 담당
  - 에디터 작업 필요: BP_PlayerController에 IA_Interact 할당, DT_ConsumableItem 데이터 새 구조에 맞게 수정
- 2026-06-25: 다중 Overlap 대응 — PlayerController의 currentInteractableItem(단일 WeakPtr) → overlappingItems(TArray) 변경. bConsumed 제거
- 2026-06-25: 작업 완료
- 2026-06-26: C_MoneyItem 추가 — AC_BaseItem 상속, moneyAmount + BeginPlay 포맷 + OnInteract AddMoney + Destroy