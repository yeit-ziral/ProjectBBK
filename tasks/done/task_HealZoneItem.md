# Task: HealZoneItem

> **상태:** 완료
> **작성일:** 2026-08-10
> **담당:** 기용

---

## 목표
사용 시 플레이어 발밑에 힐 장판을 생성해, 장판 반경 안에 있는 동안만 사용자 본인을 지속 치유하는 소비 아이템을 구현한다.

---

## 현재 상태
- 선행 태스크: `tasks/task_ConsumableAction.md` (`UC_ConsumableAction` 베이스 클래스) 완료 필요
- 관련 파일:
  - `Source/ProjectBBK/Skills/C_FireZone.h/.cpp` — 구조 참고용 (상속 아님, GA_Ablaze가 스폰하는 기존 지면 AOE 존)
  - `GE_HealOverTime` — 기존 GA_Heal이 사용 중인 Duration+Period GE. **이번 아이템엔 재사용하지 않음** (아래 설계 결정 참고)
- 현재 구현 현황: 없음. `AC_FireZone`은 몬스터 필터가 코드에 하드코딩돼 있어 그대로 상속 불가 → 신규 `AC_HealZone` 작성으로 결정됨

---

## 설계 결정: Instant GE + 존 자체 반복 타이머 (Duration GE 재사용 안 함)
- **선택:** `GE_HealZoneTick`(Instant, Set by Caller `Data.Heal`) 신규 생성. `AC_HealZone`이 `OnBeginOverlap`에 반복 타이머를 시작하고 매 tick마다 Instant GE를 적용, `OnEndOverlap`에 타이머를 멈추는 방식
- **대안:** 기존 `GE_HealOverTime`(Duration+Period)을 `AC_FireZone`처럼 진입 시 1회만 적용
- **선택 이유:** Duration GE를 1회 적용하는 방식은 장판을 스치기만 해도 전체 Duration 동안 회복이 지속돼 "장판 안에 있는 동안만 회복"이라는 의도와 어긋남. Instant GE + 존이 직접 관리하는 타이머는 실제 체류 시간과 회복이 정확히 일치함
- **트레이드오프:** `AC_FireZone`엔 없던 `OnEndOverlap` 처리와 타이머 시작/정지 로직이 추가로 필요함 (구조가 FireZone보다 조금 더 복잡해짐). Instant GE는 활성 핸들이 없으므로(Debugging Checklist #12) "제거"가 아니라 "타이머 정지"로 회복을 멈춤

---

## 작업 범위

### 1. `GE_HealZoneTick` (에디터, 신규 GE 에셋)
- [x] Instant Duration, Health Modifier 1개, Set by Caller Tag: `Data.Heal`

### 2. `AC_HealZone` 액터 클래스 (`Source/ProjectBBK/Skills/C_HealZone.h/.cpp` — `AC_FireZone`/`AC_TrapZone` 관례에 맞춰 `Items/`가 아닌 `Skills/`에 배치)
- [x] `AC_FireZone` 구조 참고해 신규 작성 (Root: `USphereComponent`, Overlap 기반)
- [x] `Initialize(UAbilitySystemComponent* InInstigatorASC, AActor* InInstigatorActor, TSubclassOf<UGameplayEffect> InTickEffectClass, float InRadius, float InZoneLifetime, float InTickInterval, float InHealPerTick)`
- [x] 스폰 시점에 이미 반경 안에 있으면 즉시 `StartHealing()`
- [x] `OnBeginOverlap`: `OtherActor == InstigatorActor`(본인)일 때만 `StartHealing()`
- [x] `OnEndOverlap`: `OtherActor == InstigatorActor`일 때만 `StopHealing()`
- [x] `StartHealing()`: 이미 타이머가 도는 중이면 무시(중복 방지) → 즉시 1틱 적용 → `SetTimer(HealTickHandle, ..., TickInterval, true)`으로 반복 타이머 시작
- [x] `StopHealing()`: `ClearTimer(HealTickHandle)`
- [x] `ApplyHealTick()`: `MakeOutgoingSpec(TickEffectClass)` → `SetSetByCallerMagnitude(Data.Heal, HealPerTick)` → `ApplyGameplayEffectSpecToSelf`(InstigatorASC — 대상이 곧 소유자라 ToSelf로 구현)
- [x] `ZoneLifetime` 타이머 → `OnExpired`: `StopHealing()` + `Destroy()`
- [x] `OnInitialized(float Radius)` BlueprintImplementableEvent — BP에서 VFX 스케일 조정용

### 3. `UC_ConsumableAction` 서브클래스
- [x] `UC_SpawnHealZoneAction` (`UC_ConsumableAction` 상속, `Source/ProjectBBK/Items/C_SpawnHealZoneAction.h/.cpp`) 생성
- [x] `Execute()`:
  - 지면 위치 탐색 (LineTrace 아래 방향, `docs/patterns.md` "3인칭 카메라 지면 위치 탐색 패턴" — Visibility 채널, ±500 오프셋) → `bBlockingHit == false`면 스폰하지 않고 종료
  - `SpawnActor<AC_HealZone>(zoneClass, ImpactPoint, ...)`
  - `zone->Initialize(ASC, AvatarActor, tickEffectClass, radius, zoneLifetime, tickInterval, healPerTick)`
- [x] `EditDefaultsOnly` 프로퍼티로 `zoneClass`, `tickEffectClass`, `radius`, `zoneLifetime`, `tickInterval`, `healPerTick` 노출 (BP에서 아이템별로 조정 가능)

### 4. 데이터 연동
- [x] `DT_ConsumableItem`에 신규 row 추가, `actionClass`를 `UC_SpawnHealZoneAction` BP 서브클래스로 지정
- [x] `BP_HealZone` 블루프린트 생성 (`AC_HealZone` 상속) — Decal 기반 VFX. `OnInitialized(Radius)`에서 Decal Component 스케일을 `Radius`에 맞춰 조정해 실제 콜리전 반경과 시각적 범위 일치 (Decal `Y`/`Z`는 half-extent라 `Set Relative Scale 3D`로 스케일 보정, `Set Decal Size` 노드는 존재하지 않음 — `DecalSize`가 BlueprintReadOnly라 Get만 가능)

### 5. 테스트
- [x] PIE에서 아이템 사용 → 발밑에 장판 스폰 확인
- [x] 장판 반경 안에 머무는 동안 매 `tickInterval`마다 체력 회복 확인
- [x] 장판 진입 직후 즉시 1틱 적용되는지 확인
- [x] 장판을 벗어나면 즉시 회복이 멈추는지 확인 (재진입 시 재개되는지도 함께 확인)
- [x] `ZoneLifetime` 경과 후 장판 액터 소멸 확인

---

## 제약 조건
없음 (CLAUDE.md GAS 작업 규칙 기본 적용)

---

## 완료 기준
- `GE_HealZoneTick`, `AC_HealZone`, `UC_SpawnHealZoneAction` 구현 및 빌드 성공
- PIE에서 아이템 사용 → 장판 생성 → 체류 중에만 정확히 tick 단위로 회복, 이탈 시 즉시 중단 확인

---

## 참고
| 항목 | 내용 |
|------|------|
| 관련 문서 | @docs/patterns.md (지면 AOE 존 패턴 — `GA_Ablaze`/`AC_FireZone`, 설치형 트랩 존 패턴 — 지면 위치 탐색 부분) |
| 관련 문서 | @docs/decisions.md (TrapZone 스폰 위치 — LineTrace) |
| 관련 클래스 | `AC_FireZone`(구조 참고), `AC_TrapZone`(지면 LineTrace 스폰 패턴 참고) |
| 관련 Tag | `Data.Heal` (Set by Caller) |
| 설계 결정 | 치유 대상은 사용자 본인만. Instant GE + 존 자체 반복 타이머로 "체류 중에만 회복" 구현 (위 설계 결정 섹션 참고) — `GE_HealOverTime`(Duration GE) 재사용 안 함 |

---

## 작업 로그
- 2026-08-10: 태스크 생성. 대상 범위(본인만), 클래스 구조(AC_FireZone 신규 작성, 상속 아님) 확정.
- 2026-08-10: GE 방식 변경 — Duration GE(`GE_HealOverTime`) 1회 적용 대신 Instant GE(`GE_HealZoneTick`) + 존 자체 반복 타이머(Begin/EndOverlap 연동)로 설계 수정. 이유: Duration GE는 장판을 스치기만 해도 전체 지속시간 회복이 확정되어 "체류 중에만 회복" 의도와 불일치.
- 2026-08-10: C++ 구현 완료 — `AC_HealZone`(Skills/C_HealZone.h/.cpp, `AC_FireZone`/`AC_TrapZone` 관례에 맞춰 배치 폴더를 Items에서 Skills로 변경), `UC_SpawnHealZoneAction`(Items/C_SpawnHealZoneAction.h/.cpp). UnrealBuildTool `ProjectBBKEditor` Development Win64 빌드 성공 확인.
- 2026-08-10: 에디터 작업 완료 — `GE_HealZoneTick`, `DT_ConsumableItem` row, `BP_HealZone`(Decal 기반 VFX) 생성. Decal 범위와 SphereOverlap 범위 불일치 디버깅 — `DrawDebugSphere`로 실제 콜리전 반경 시각화 후 원인 확인(Decal `Y`/`Z`는 half-extent, `Set Decal Size` 노드 부재로 `Set Relative Scale 3D` 기반 스케일 보정으로 해결). PIE 테스트 완료(장판 생성, 체류 중 tick 회복, 이탈 시 즉시 중단, 수명 경과 후 소멸). 작업 완료.
