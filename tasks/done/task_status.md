# Task: 스탯창

> **상태:** 완료  
> **작성일:** 2026-07-07  
> **담당:** 기용

---

## 목표

플레이어 스탯(MaxHP, MaxStamina, MoveSpeed, Defense, Attack)을 표시하는 인게임 UI 창 구현 — 키 입력으로 여닫고, 드래그로 위치 이동 가능하며, 캐릭터 교체·스탯 변화 시 실시간 반영

---

## 현재 상태

- 관련 파일:
  - `Source/ProjectBBK/PlayerCharacter/C_PlayerState.h/.cpp`
  - `Source/ProjectBBK/PlayerCharacter/C_BasePlayerCharactor.h/.cpp`
  - `Source/ProjectBBK/PlayerCharacter/PlayerAI/C_PlayerController.h/.cpp`
  - `Source/ProjectBBK/GAS/Abilities/C_CharacterASC.h/.cpp`
  - `Source/ProjectBBK/GAS/Attributes/C_ChracterAttributeSetBase.h/.cpp`
  - `Content/PlayerCharacter/Blueprint/BPC_MeleePlayer`
  - `Content/PlayerCharacter/Blueprint/BPC_RangedPlayer`
- 현재 구현 현황:
  - AttributeSet에 MaxHealth, MaxStamina, MoveSpeed, Defense 어트리뷰트 존재
  - PlayerController에 Enhanced Input 기반 입력 처리 구조 존재
  - 캐릭터 교체 시 `OnCharacterSwitched` 델리게이트 브로드캐스트 구조 존재
  - WBP_HUD 기존 HUD 구조 존재 (SkillIcon, UltimateGauge 포함)

---

## 작업 범위

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

---

## 제약 조건

- ASC 참조는 반드시 `PlayerState`에서 취득 — `Character` 직접 참조 금지
- 스탯창 열림 중 어빌리티 발동은 차단하지 않음 (SkillWheel과 달리 전투 중 확인 용도)
- 스탯 수치는 어트리뷰트 변경 델리게이트로 실시간 반영 — Tick 폴링 금지

---

## 완료 기준

- `IA_Status` 키 입력으로 스탯창이 열리고 닫힌다 ✅
- 스탯창에 MaxHP / MaxStamina / MoveSpeed / Defense / Attack 수치가 표시된다 ✅
- 마우스 드래그로 스탯창 위치를 자유롭게 이동할 수 있다 ✅
- 캐릭터 교체 후 스탯창에 새 캐릭터의 스탯이 표시된다 ✅
- 장비 장착/탈착, 레벨업 등 스탯 변화가 스탯창에 즉시 반영된다 ✅

---

## 참고

| 항목 | 내용 |
|------|------|
| 관련 문서 | @docs/architecture.md (GAS Hierarchy, Player track) |
| 관련 문서 | @docs/patterns.md (캐릭터 교체 후 HUD 재초기화 패턴) |
| 관련 문서 | @docs/debugging.md (#19 델리게이트 재바인딩, #15 ASC 참조) |
| 관련 클래스 | `UC_ChracterAttributeSetBase`, `UC_CharacterASC`, `AC_PlayerController` |
| 관련 클래스 | `UC_StatusWidget` (`Source/ProjectBBK/UI/`) |
| 참고 패턴 | `UC_InventoryWidget` 드래그 패턴 동일하게 적용 |
| 참고 패턴 | `EMouseUISource` 비트플래그로 커서 중복 요청 처리 |

---

## 구현 노트

- **WBP_Status 계층**: `[Overlay] → [SizeBox] WindowRoot → [Overlay] → [Image](배경) + [Vertical Box](콘텐츠)`
- **루트를 Overlay로 선택한 이유**: Canvas Panel과 달리 전체 뷰포트를 덮어 드래그 중 마우스가 창 밖으로 나가도 MouseButtonUp 안정적으로 수신
- **WindowRoot를 SizeBox로 선택한 이유**: 스탯 수치 변경 시 창 크기가 흔들리지 않도록 고정
- **attack 어트리뷰트**: AttributeSet 내부 이름은 `damage`, BindWidget 이름은 `AttackText`

---

## 작업 로그

- 2026-07-07: 태스크 생성
- 2026-07-07: C++ 구현 완료 (C_StatusWidget, C_PlayerController 수정)
- 2026-07-07: attack 항목 추가
- 2026-07-07: 작업 완료
