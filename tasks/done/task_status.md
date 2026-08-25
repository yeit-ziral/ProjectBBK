# Task: 스탯창

> **상태:** 완료  
> **작성일:** 2026-07-07 (2차 작업 갱신: 2026-08-25)  
> **담당:** 기용

---

## 목표

**1차(완료):** 플레이어 스탯(MaxHP, MaxStamina, MoveSpeed, Defense, Attack)을 표시하는 인게임 UI 창 구현 — 키 입력으로 여닫고, 드래그로 위치 이동 가능하며, 캐릭터 교체·스탯 변화 시 실시간 반영

**2차(신규):** 기존 총합 수치 표시는 그대로 유지하면서, 장비 착용 또는 포션(버프 소모품) 섭취로 스탯이 증가한 경우 증가분을 "(+수치)" 형태로 함께 표시

---

## 현재 상태

- 관련 파일:
  - `Source/ProjectBBK/PlayerCharacter/C_PlayerState.h/.cpp`
  - `Source/ProjectBBK/PlayerCharacter/C_BasePlayerCharactor.h/.cpp`
  - `Source/ProjectBBK/PlayerCharacter/PlayerAI/C_PlayerController.h/.cpp`
  - `Source/ProjectBBK/GAS/Abilities/C_CharacterASC.h/.cpp`
  - `Source/ProjectBBK/GAS/Attributes/C_ChracterAttributeSetBase.h/.cpp`
  - `Source/ProjectBBK/UI/C_StatusWidget.h/.cpp`
  - `Source/ProjectBBK/Equip/C_EquipmentComponent.h/.cpp`
  - `Source/ProjectBBK/Items/ItemData.h` (`FEquipmentItemData` bonus 필드)
  - `Config/DefaultGameplayTags.ini`
  - `Content/PlayerCharacter/Blueprint/GAS/Effects/Potion/GE_IncreaseMaxST` (BP, 스탯 증가 포션 GE)
- 현재 구현 현황:
  - **1차 완료:** AttributeSet에 MaxHealth, MaxStamina, MoveSpeed, Defense, damage(Attack) 어트리뷰트 존재. `UC_StatusWidget`이 ASC 어트리뷰트 변경 델리게이트로 5개 텍스트를 실시간 갱신 중 (`RefreshStatValues` / `OnXxxChanged`)
  - `UC_EquipmentComponent`는 장착된 아이템의 `bonusMaxHealth/bonusMaxStamina/bonusMoveSpeed/bonusDefense/bonusDamage`를 DT(`GetEquipmentData`)에서 이미 조회 가능 — 장비 증가분은 `equipped` 맵을 순회해 합산하면 바로 얻을 수 있음 (신규 GAS 조회 불필요)
  - 스탯 증가 포션이 BP로 이미 구현됨: `GE_IncreaseMaxST`(Has Duration)가 damage/defense/maxStamina/moveSpeed Modifier를 적용하고 상태 태그를 부여. 현재는 기존 `State.Buff` 태그를 사용 중 — 이번 작업에서 `State.PotionBuff` 태그를 신설해 전용으로 교체 예정(다른 상태이상/버프 태그와 명확히 구분, 후속 확장 시 재사용 방지)
  - `GA_SpeedBuff`(F, `GE_SpeedBuff`), `GA_Sprint`(Shift, `GE_SprintBuff`), `GE_Slowed`(감속 디버프, `State.Slowed`) 등도 Duration/Infinite GE로 damage/moveSpeed류를 건드리지만 각자 별도 `State.*` 태그를 사용 — `State.PotionBuff` 태그 필터링으로 자연히 이번 기능 범위에서 제외됨

---

## 작업 범위

### 1차: 스탯창 기본 구현 (완료)
- [x] `IA_Status` Input Action 에셋 생성 (Digital bool, Pressed 트리거) — 에디터 작업
- [x] `C_PlayerController`에 `IA_Status` 바인딩 추가 — `ToggleStatus()` 토글 방식
- [x] `UC_StatusWidget` C++ 클래스 생성 (`Source/ProjectBBK/UI/C_StatusWidget.h/.cpp`)
  - [x] ASC 참조 저장 및 어트리뷰트 변경 델리게이트 바인딩
  - [x] `InitializeStatWindow(UAbilitySystemComponent* ASC)` 함수
  - [x] MaxHP / MaxStamina / MoveSpeed / Defense / Attack 텍스트 갱신
  - [x] 마우스 드래그 이동 (NativeOnMouseButtonDown / Move / Up)
- [x] `C_PlayerController` 수정
  - [x] `EMouseUISource::Status` 비트 추가
  - [x] `OpenStatus()` / `CloseStatus()` / `ToggleStatus()` 구현
  - [x] `statusWidgetClass` / `statusWidget` / `bStatusOpen` 멤버 추가
  - [x] 캐릭터 교체 시 (`ExecuteCharacterSwitch`) `InitializeStatWindow` 재호출
  - [x] 전원 사망 시 (`HandleCharacterDeath`) `CloseStatus()` 호출
- [x] `WBP_Status` 위젯 생성 — 에디터 작업 (계층 구조 가이드 완료)

### 2차: 장비/포션 증가분 표시 (신규)
- [x] `State.PotionBuff` GameplayTag 등록 (`Config/DefaultGameplayTags.ini`) — 이미 등록되어 있음, 확인 완료
- [x] `GE_IncreaseMaxST`의 Granted Tag를 `State.Buff` → `State.PotionBuff`로 교체 — 에디터 작업, 확인 완료
- [x] `UC_EquipmentComponent`에 장착 중인 5개 보너스 합산 조회 함수 추가 — `FEquipBonusTotals GetTotalEquipBonuses()`
- [x] `UC_StatusWidget`에 `State.PotionBuff` 태그로 ASC의 활성 GameplayEffect를 조회해 damage/defense/maxStamina/moveSpeed Modifier 평가치를 합산하는 로직 추가 — `GetPotionBonus()` (`FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags` + `GetActiveGameplayEffect`)
- [x] `RefreshStatValues()` 및 `OnMaxHealthChanged`/`OnMaxStaminaChanged`/`OnMoveSpeedChanged`/`OnDefenseChanged`/`OnAttackChanged` 5개 모두 `UpdateStatText()` 경유로 "총합 (+증가분)" 형식 표시하도록 수정 — 증가분이 0이면 괄호 숨김
- [x] MaxHP도 동일 로직 적용 (포션 쪽 합산은 항상 0이라 자연히 장비분만 표시됨)
- [x] Development Editor Win64 빌드 성공 확인 (`Build.bat ProjectBBKEditor Win64 Development`)
- [x] 포션 만료·장비 해제 시 "(+증가분)" 표시가 자동으로 사라지는지 PIE로 검증 — 확인 완료
- [x] 장비 장착/해제 시 "(+증가분)"이 한 박자 늦게 반영되던 버그 수정 — `UC_EquipmentComponent::EquipItem`/`UnequipItem`이 GE 적용/제거보다 `equipped` 맵 갱신을 먼저 하도록 순서 변경, PIE 확인 완료

---

## 제약 조건

- ASC 참조는 반드시 `PlayerState`에서 취득 — `Character` 직접 참조 금지
- 스탯창 열림 중 어빌리티 발동은 차단하지 않음 (SkillWheel과 달리 전투 중 확인 용도)
- 스탯 수치는 어트리뷰트 변경 델리게이트로 실시간 반영 — Tick 폴링 금지
- **(2차)** `State.PotionBuff`로 태그되지 않은 Duration/Infinite GE(`GA_SpeedBuff`, `GA_Sprint`, `GE_Slowed` 등 스킬 버프/디버프)는 증가분 표시에 포함하지 않는다 — 장비와 `State.PotionBuff` 포션만 반영
- **(2차)** `State.PotionBuff` 태그는 포션류 전용으로 유지 — 다른 GE가 재사용하지 않도록 주의
- **(2차)** 기존 "총합 수치" 표시는 변경하지 않는다 — "120 (+20)" 형식(총합 뒤에 증가분), "100 (+20)"(베이스만 분리) 아님

---

## 완료 기준

**1차 (완료):**
- `IA_Status` 키 입력으로 스탯창이 열리고 닫힌다 ✅
- 스탯창에 MaxHP / MaxStamina / MoveSpeed / Defense / Attack 수치가 표시된다 ✅
- 마우스 드래그로 스탯창 위치를 자유롭게 이동할 수 있다 ✅
- 캐릭터 교체 후 스탯창에 새 캐릭터의 스탯이 표시된다 ✅
- 장비 장착/탈착, 레벨업 등 스탯 변화가 스탯창에 즉시 반영된다 ✅

**2차 (완료):**
- 소모품(포션)으로 damage/defense/maxStamina/moveSpeed가 일시적으로 증가하면 스탯창에 해당 항목 옆에 "(+수치)"가 표시된다 ✅
- 장비 착용으로 MaxHP를 포함한 5개 스탯이 증가하면 모두 동일하게 "(+수치)"가 표시된다 ✅
- 포션 지속시간이 끝나거나 장비를 해제하면 "(+수치)" 표시가 사라지고 총합 수치도 원래대로 돌아온다 ✅
- 스킬 버프(스프린트, GA_SpeedBuff 등)나 디버프(GE_Slowed)는 이 증가분 표시에 영향을 주지 않는다 ✅
- 캐릭터 교체 후에도 증가분이 새 캐릭터 기준으로 정확히 갱신된다 ✅
- 장비 장착/해제 즉시(한 박자 지연 없이) "(+수치)"가 정확히 반영된다 ✅

---

## 참고

| 항목 | 내용 |
|------|------|
| 관련 문서 | @docs/architecture.md (GAS Hierarchy, Player track) |
| 관련 문서 | @docs/patterns.md (캐릭터 교체 후 HUD 재초기화 패턴, ASC 어트리뷰트 실시간 반영 위젯 패턴, 공유 ASC 환경에서 캐릭터별 Infinite GE 격리 패턴) |
| 관련 문서 | @docs/decisions.md (장비 스탯 적용 — 공용 GE + SetByCaller, 캐릭터 교체 시 장비 보너스 격리) |
| 관련 문서 | @docs/debugging.md (#19 델리게이트 재바인딩, #15 ASC 참조) |
| 관련 클래스 | `UC_ChracterAttributeSetBase`, `UC_CharacterASC`, `AC_PlayerController` |
| 관련 클래스 | `UC_StatusWidget` (`Source/ProjectBBK/UI/`), `UC_EquipmentComponent` (`Source/ProjectBBK/Equip/`) |
| 관련 Tag | `State.PotionBuff` (신규, 2차) — 제외 대상 예시: `State.Sprint`, `State.Slowed` |
| 참고 패턴 | `UC_InventoryWidget` 드래그 패턴 동일하게 적용 |
| 참고 패턴 | `EMouseUISource` 비트플래그로 커서 중복 요청 처리 |

---

## 구현 노트

- **WBP_Status 계층**: `[Overlay] → [SizeBox] WindowRoot → [Overlay] → [Image](배경) + [Vertical Box](콘텐츠)`
- **루트를 Overlay로 선택한 이유**: Canvas Panel과 달리 전체 뷰포트를 덮어 드래그 중 마우스가 창 밖으로 나가도 MouseButtonUp 안정적으로 수신
- **WindowRoot를 SizeBox로 선택한 이유**: 스탯 수치 변경 시 창 크기가 흔들리지 않도록 고정
- **attack 어트리뷰트**: AttributeSet 내부 이름은 `damage`, BindWidget 이름은 `AttackText`
- **(2차) 증가분 계산을 GAS 표준 `CurrentValue - BaseValue`로 하지 않는 이유**: 이 프로젝트엔 `State.PotionBuff` 외에도 moveSpeed/damage를 건드리는 Duration/Infinite GE(스프린트, 스킬 버프, 슬로우)가 이미 존재해 단순 차이값으로는 소스를 구분할 수 없음. 장비는 `UC_EquipmentComponent`가 이미 아는 값을 합산, 포션은 `State.PotionBuff` 태그로 필터링한 활성 이펙트의 Modifier만 합산하는 방식으로 소스를 정확히 분리
- **(2차) 만료 처리를 별도 구현하지 않는 이유**: 포션/장비 모두 실제 어트리뷰트 CurrentValue를 바꾸는 GE이므로, 만료·해제 시 기존에 이미 바인딩된 `OnXxxChanged` 델리게이트가 자동으로 재호출됨 — 그 안에서 증가분을 다시 계산하면 타이머 없이 자동 갱신됨

---

## 작업 로그

- 2026-07-07: 태스크 생성
- 2026-07-07: C++ 구현 완료 (C_StatusWidget, C_PlayerController 수정)
- 2026-07-07: attack 항목 추가
- 2026-07-07: 1차 작업 완료
- 2026-08-25: 2차 작업(장비/포션 증가분 표시) 범위 확정 — GAS 표준 BaseValue/CurrentValue 차이 대신, 장비는 EquipmentComponent 합산 + 포션은 `State.PotionBuff` 태그 필터링 방식으로 스킬 버프/디버프와 명확히 구분하기로 결정. 태스크 재오픈(`tasks/done` → `tasks`)
- 2026-08-25: MaxHP도 "(+증가분)" 표시 대상에 포함하기로 확정 (5개 스탯 전부 동일 로직 적용)
- 2026-08-25: 2차 C++ 구현 완료 — `UC_EquipmentComponent::GetTotalEquipBonuses()`, `UC_StatusWidget::GetPotionBonus()`/`UpdateStatText()` 추가. Development Editor Win64 빌드 성공. 남은 건 에디터에서 `GE_IncreaseMaxST` 태그 확인 + PIE 검증
- 2026-08-25: PIE 검증 중 장비 장착/해제 시 "(+수치)"가 한 박자 늦게(직전 아이템 제외하고) 반영되는 버그 발견 — GE 적용/제거가 어트리뷰트 델리게이트를 동기 발화시키는데 `equipped` 맵 갱신이 그 이후에 일어나던 순서 문제. `EquipItem`/`UnequipItem`에서 맵 갱신을 GE 적용/제거보다 먼저 하도록 수정
- 2026-08-25: PIE 재검증 완료, 2차 작업 완료. 태스크 완료 처리(`tasks` → `tasks/done`)
