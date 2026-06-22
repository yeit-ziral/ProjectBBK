# Task: BaseItem

> **상태:** 진행 중  
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
- [x] **[임시]** 서브클래스에서 효과 적용 후 Destroy
- 월드 스폰은 `BP_ConsumableItem` / `BP_EquipItem`만 사용 (C++ 직접 스폰 금지)
- 스폰 주체(인벤토리 드롭, 몬스터 사망 등)가 `SpawnActor` → `InitItem(ItemID)` 호출하는 구조

### 3. 상호작용 UI
- [x] `UC_InteractionWidget` 위젯 클래스 생성 (BindWidget: InteractionText)
- [x] `C_BaseItem`의 WidgetComponent가 Overlap 시 표시 / 해제 시 숨김
- [x] WBP_Interaction Blueprint 생성 (에디터) — InteractionText 텍스트블록 배치

### 4. 소비 아이템 클래스
- [x] `C_ConsumableItem` (C_BaseItem 상속) 클래스 생성
- [x] `InitItem` — consumableDataTable에서 FConsumableItemData 로드 + Mesh 적용
- [x] `OnInteract` — GE Spec 생성 + SetByCaller 주입 + Apply + Destroy (임시)

### 5. 장비 아이템 클래스
- [x] `C_EquipmentItem` (C_BaseItem 상속) 클래스 생성
- [x] `InitItem` — equipmentDataTable에서 FEquipmentItemData 로드 + Mesh 적용
- [x] **[임시]** `OnInteract` — `GE_EquipBonus` Apply + 5개 스탯 SetByCaller 주입 + Destroy
- [ ] 장착(Equip) — `FActiveGameplayEffectHandle` 저장 (인벤토리 구현 후)
- [ ] 해제(Unequip) — Handle로 `RemoveActiveGameplayEffect` (인벤토리 구현 후)
- [ ] `EEquipmentSlot` 기반 부위 중복 장착 방지 (인벤토리 구현 후)

### 6. 인벤토리 시스템 (후속 태스크)
- [ ] 인벤토리 컴포넌트 또는 관리 구조 구현 (아이템 추가/제거/조회)
- [ ] 인벤토리 UI 위젯 연동
- [ ] BaseItem의 임시 Destroy 로직을 인벤토리 등록으로 교체
- [ ] 소비 아이템 스택 감소 / 0이면 인벤토리에서 제거

### 7. 테스트
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
- [ ] `C_BaseItem`에서 Tick 기반 키 체크 제거
- [ ] `PlayerController`에 `IA_Interact` InputAction 등록
- [ ] Overlap 시 `PlayerController`에 현재 아이템 참조 전달, 해제 시 초기화
- [ ] `PlayerController`가 상호작용 키 입력 감지 → 참조 중인 아이템의 `OnInteract()` 호출

### 2. 소비 아이템 다중 GE 지원
- [ ] `FConsumableItemData` 구조체 수정
  - `consumeEffect` → `TArray<TSubclassOf<UGameplayEffect>> consumeEffects`
  - `magnitudeTag` → `TArray<FGameplayTag> magnitudeTags`
  - `magnitude` → `TArray<float> magnitudes`
- [ ] `C_ConsumableItem::OnInteract` — 배열 순회하며 GE Spec 생성 + SetByCaller 주입 + Apply
- [ ] DT_ConsumableItem 에디터에서 기존 데이터 새 구조에 맞게 수정

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