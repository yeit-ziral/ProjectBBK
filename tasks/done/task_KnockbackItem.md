# Task: KnockbackItem

> **상태:** 완료
> **작성일:** 2026-08-10
> **담당:** 기용

---

## 목표
사용 시 플레이어 주변 몬스터를 순수 물리적으로 밀어내는(넉백) 소비 아이템을 구현한다. GE 적용 없이 `LaunchCharacter`만 수행한다.

---

## 현재 상태
- 선행 태스크: `tasks/task_ConsumableAction.md` (`UC_ConsumableAction` 베이스 클래스) 완료 필요
- 관련 파일: 없음 (신규)
- 참고 패턴: `GA_MeleeUltimate`의 AOE Overlap 판정, `C_RangedUltimate::HandleNotifyEvent`의 `LaunchCharacter` 직접 호출 + `State.KnockbackImmune` 면역 체크

---

## 작업 범위

### 1. `UC_ConsumableAction` 서브클래스
- [x] `UC_KnockbackAction` (`UC_ConsumableAction` 상속, C++, `Source/ProjectBBK/Items/C_KnockbackAction.h/.cpp`) 생성
- [x] `Execute(UAbilitySystemComponent* ASC, AActor* AvatarActor)`:
  - `OverlapMultiByObjectType(AvatarActor 위치, radius, ECC_Pawn) + AC_BaseMonster 필터` — `C_RangedUltimate::HandleNotifyEvent`와 동일 스타일(Box 대신 Sphere)
  - `ForEach` HitActor:
    - `HasMatchingGameplayTag(State.KnockbackImmune)` → true면 skip
    - AI 컨트롤러 `StopMovement()` (BT 이동 명령이 넉백을 상쇄하지 않도록)
    - `LaunchCharacter(Direction(수평 정규화) * knockbackForce, XYOverride=true, ZOverride=false)` — `C_RangedUltimate` 패턴 재사용
  - GE 적용 로직 없음 (순수 밀어내기)
- [x] `EditDefaultsOnly` 프로퍼티로 `radius`, `knockbackForce` 노출

### 2. 데이터 연동
- [x] `DT_ConsumableItem`에 신규 row 추가, `actionClass`를 `UC_KnockbackAction` (또는 그 BP 서브클래스)로 지정

### 3. 테스트
- [x] PIE에서 아이템 사용 → 주변 몬스터가 밀려나는지 확인
- [x] `State.KnockbackImmune` 태그를 가진 몬스터는 밀려나지 않는지 확인
- [x] 반경 밖 몬스터는 영향받지 않는지 확인

---

## 제약 조건
없음 (CLAUDE.md GAS 작업 규칙 기본 적용)

---

## 완료 기준
- `UC_KnockbackAction` 구현 및 빌드 성공
- PIE에서 아이템 사용 → 반경 내 몬스터 넉백, 면역 몬스터 제외 확인

---

## 참고
| 항목 | 내용 |
|------|------|
| 관련 문서 | @docs/patterns.md (AOE Overlap 패턴 — `GA_MeleeUltimate`) |
| 관련 문서 | @docs/decisions.md (LaunchCharacter — GC 경유 vs C++ 직접 호출) |
| 관련 클래스 | `GA_MeleeUltimate`(AOE Overlap 판정 구조 참고), `C_RangedUltimate::HandleNotifyEvent`(LaunchCharacter 호출 구조 참고) |
| 관련 Tag | `State.KnockbackImmune` |
| 설계 결정 | GE 적용 없이 순수 `LaunchCharacter`만 수행 |

---

## 작업 로그
- 2026-08-10: 태스크 생성. GE 없이 순수 넉백만 수행하는 것으로 범위 확정.
- 2026-08-10: C++ 구현 완료 — `UC_KnockbackAction`(Items/C_KnockbackAction.h/.cpp). UnrealBuildTool `ProjectBBKEditor` Development Win64 빌드 성공 확인. 남은 작업: `DT_ConsumableItem` row 추가(에디터), PIE 테스트.
- 2026-08-10: 에디터 작업 완료 — `DT_ConsumableItem` row 추가(`actionClass` = `UC_KnockbackAction`). PIE 테스트 완료(반경 내 몬스터 넉백, 면역 몬스터 제외, 반경 밖 무영향). 작업 완료.
