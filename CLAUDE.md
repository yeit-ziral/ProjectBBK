# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

> **Claude Code에게:** `/패턴정리` 명령어 입력 시 현재 세션에서 작업한 내용을 검토하고 Debugging Checklist / Common Patterns / Design Decisions에 추가할 항목을 제안할 것. 또한 Current Development Status 섹션의 각 테이블(Player Abilities / Monster Abilities / UI / Effects)에서 상태 변경이 필요한 항목(새로 완료된 항목, 계획 중→진행 중 전환 등)도 함께 제안할 것.

## Project Overview

ProjectBBK is an Unreal Engine 5.6 action RPG built with C++ and Blueprints. It uses the Gameplay Ability System (GAS) extensively for both player and monster combat. The primary module is `ProjectBBK` (Runtime).

**팀 담당 파트**
| 이름 | 담당 |
|------|------|
| 지호 | 플레이어 캐릭터 |
| 기용 | 스킬 |
| 선우 | 몬스터 |

---

## Build & Development

**Generate project files:**
Right-click `ProjectBBK.uproject` → "Generate Visual Studio project files"

**Build (from solution):**
Open `ProjectBBK.sln` in Visual Studio 2022, build with `Development Editor` configuration targeting `Win64`.

**Open in editor:**
Double-click `ProjectBBK.uproject` or run the editor from VS with F5 (Development Editor).

**Required VS components** are specified in `.vsconfig`:
- VC++ 14.38 toolchain (x86/x64)
- Windows 11 SDK 22621
- LLVM/Clang support
- Unreal Engine IDE tools

**Module dependencies** (defined in `Source/ProjectBBK/ProjectBBK.Build.cs`):
Core, CoreUObject, Engine, InputCore, EnhancedInput, GameplayAbilities, GameplayTags, GameplayTasks, UMG, Slate, SlateCore, Niagara, AIModule

**Default startup map:** `/Game/Maps/Untitled`
**Default GameMode:** `BP_GameMode`

---

## Architecture

### GAS Hierarchy (Dual Track)

The project maintains **two parallel GAS hierarchies** — one for players, one for monsters — that share the same damage effect (`CGE_Damage`).

**Player track:**
- `UC_CharacterASC` — base Ability System Component
- `UC_ChracterAttributeSetBase` — Health, MaxHealth, Shield, MaxShield, Mana, MaxMana, Stamina, MaxStamina, MoveSpeed, Damage, Level
- `UC_CharacterGA` — base Gameplay Ability with cooldown/cost management and `ProjectBBKAbilityID` binding

**Monster track:**
- `UC_MonsterASC` — handles death/interrupt logic
- `UC_MonsterAttributeSet` — CurHP, MaxHP, CurGroggy, MaxGroggy, Attack, Defense, MoveSpeed, AttackRange, NormalCooldown, SpecialCooldown
- Monster GAs per attack type (melee normal, ranged normal, boss beam, boss storm)

### Ability ID Enum

Defined in `Source/ProjectBBK/ProjectBBK.h`:
```
None, Confirm, Cancel, Attack, Sprint, Dodge, CommonSkill, UniqueSkill, Ultimate, Shield
```
Used to bind input to abilities on the ASC.

### Player Character Hierarchy

```
AC_BasePlayerCharactor (ACharacter, IAbilitySystemInterface)
└── AC_MeleeCharacter
```

Player state (`UC_PlayerState`) owns the ASC and attribute set. The character gets references from the player state on `PossessedBy`/`OnRep_PlayerState`.

### Monster Class Hierarchy

```
AC_BaseMonster (ACharacter)
├── AC_MeleeMonster      // Melee: normal attack + jump slam special
├── AC_RangedMonster     // Projectile spawning via AnimNotify
└── AC_BossMonster       // Beam pattern + Storm pattern GAs
```

Monsters are data-driven via `FMonsterData` (DataTable rows defined in `Monster/Data/MonsterData.h`). Stats, attack ranges, and cooldowns are loaded at runtime — avoid hardcoding these values in C++.

**Monster IDs** (defined in `Monster/Data/MonsterID.h`):
- `MONSTER_ID_BEAR = 1001`
- `MONSTER_ID_ROCKET = 1002`

**Groggy system:** Monsters accumulate `CurGroggy`; when it reaches `MaxGroggy` they enter a stun state with a reset timer. This is tracked in `UC_MonsterAttributeSet`.

**Attack Manager Component** (`C_AttackManagerComponent`) centralizes cooldown tracking and attack execution to avoid per-GA state duplication.

### Skill System

`UC_SkillBase` provides a data-table-driven skill framework with:
- Dynamic cooldown management via GameplayTags
- Event dispatchers: `OnCooldownStarted`, `OnSkillActivated` (bind these for UI updates)
- Skill metadata (name, description, icon, level) loaded from DataTable
- `FSkillData` struct (in `Skills/SkillData.h`) is the central data definition for all skills — includes GAS config, stats, animation, VFX, audio, and UI icon references

### AI

Monsters use Behavior Trees driven by `AC_MonsterAIController`. `UC_MonsterBTService` handles target acquisition and distance checks; `UC_BTTaskNormalAttack` executes attack GAs. Projectile spawning is triggered via AnimNotify (`C_SpawnProjectile_AnimNotify`).

### UI

Monster HP is displayed via 3D widgets (`C_NormalMonsterHPWidget`, `C_BossMonsterHPWidget`) bound to `UC_MonsterAttributeSet` attribute-change delegates.

Player HUD (`WBP_HUD`) is the root widget and contains:
- Health / Shield / Stamina bars with text blocks
- `WBP_SkillIcon Common` and `WBP_SkillIcon Unique` — skill slot icons (Horizontal Box)
- `WBP_UltimateGauge` — ultimate gauge, embedded as a child widget (not a separate top-level widget)

---

## Source Code Structure (`Source/ProjectBBK/`)

```
GAS/
├── Abilities/     - UC_CharacterASC, UC_CharacterGA (base GAS classes for players)
├── Attributes/    - UC_ChracterAttributeSetBase
└── C_AssetManager - Custom asset manager (set in DefaultEngine.ini)

Monster/
├── AI/            - C_MonsterAIController, BT tasks/services
├── Anim/          - C_MonsterAnimInstance, animation notifies
├── Data/          - AttackTypes.h, MonsterData.h, MonsterID.h, MonsterSpawnData.h
├── Manager/       - C_AttackManagerComponent, C_MonsterSpawnManager
├── M_Gas/         - Monster-specific ASC, AttributeSet, damage GE
├── Notify/        - Animation notifies (e.g., spawn projectile)
├── Object/        - C_BossBeam, C_BossStorm, C_RangedProjectile
└── UI/            - HP widgets (base, boss, normal variants)
    C_BaseMonster, C_BearMonster, C_BossMonster, C_RangedMonster

PlayerCharacter/
├── PlayerAI/      - C_PlayerAIController, C_PlayerController
└── C_BasePlayerCharactor, C_MeleeCharacter, C_PlayerState

Skills/
└── C_SkillBase, C_SkillIconWidget, C_UltimateGaugeWidget, SkillData.h
```

---

## Content Organization (`Content/`)

- `Monster/` — BPs, DataTables, AI (BT/BB), GAs, UI
- `PlayerCharacter/Blueprint/` — Animation montages, Data Assets, GAS abilities, UI
- `Skills/` — Skill definitions
- `UI/` — Shared UI widgets
- `Maps/` — Game maps (Untitled.umap)
- `ParagonXxx/` — Third-party Paragon character asset packs (read-only reference assets)

---

## Key Config Files

| File | Purpose |
|------|---------|
| `Config/DefaultGameplayTags.ini` | All gameplay tag definitions |
| `Config/DefaultEngine.ini` | DirectX 12, ray tracing settings, custom AssetManager (`C_AssetManager`) |
| `Config/DefaultGame.ini` | Default GameMode (`BP_GameMode`) |
| `ProjectBBK.uproject` | Plugin list (GameplayAbilities, AIModule), custom AssetManager |

**Gameplay Tags** — Key tag namespaces defined in `DefaultGameplayTags.ini`:
`Ability.*`, `State.*`, `Skill.*`, `Input.*`, `Event.*`

## Naming Conventions

- **Classes & Functions & Function Parameters:** PascalCase (e.g. `MyClass`, `GetValue`, `MaxCount`)
- **Variables:** camelCase (e.g. `myVariable`, `currentValue`)
- **Constants:** UPPER_SNAKE_CASE (e.g. `MAX_VALUE`, `DEFAULT_SIZE`)

---

## Git 협업 규칙

### 브랜치 전략
- 각자 개인 브랜치에서 작업
- 기능 구현 완료 시 `master`에 merge
- 다른 팀원이 `master`에 merge한 경우, `master`를 본인 브랜치로 merge하여 최신 상태 유지

### .uasset 충돌 방지
- 작업 **시작 / 종료** 시 Discord에 공유 — 같은 파일을 동시에 열지 않도록 주의
- 충돌로 파일 손상 시 **가장 최근 작업자의 파일로 덮어쓰기**

---

## Important Constraints

### Blueprint & Editor Limitations
- `.uasset` / `.umap` 파일은 직접 읽거나 편집할 수 없음 — C++ 변경 시 어떤 BP를 리컴파일해야 하는지 명시할 것

### GAS 작업 규칙
- Damage: 데미지는 무조건 전용 GE가 담당 — GA에서 직접 수치 적용 절대 금지
  - 즉발 데미지 → `GE_BasicDamage` (Set by Caller, `Data.Damage` 태그)
  - 지속 데미지(DoT) → `GE_DotDamage` (Set by Caller, `Data.Damage` 태그)
  - 새로운 데미지 유형(크리티컬 등) → 전용 GE를 새로 생성
- Cooldown: `GE_GenericCooldown`을 GA 간 공유 — GA별 개별 쿨다운 GE 생성 금지
- ASC 참조: 반드시 `PlayerState`에서 가져올 것 — `Character`에서 직접 참조 금지
- Overlap의 `Actors to Ignore`: `GetOwner()` 대신 `GetAvatarActorFromActorInfo()` 사용 — GA 컨텍스트에서는 `GetOwner()`가 동작하지 않음 (GA_MeleeUltimate 구현 시 확인된 사항)

---

## Current Development Status

> 작업 전 이 섹션을 확인하고, 완료 후 반드시 업데이트할 것.

### Player Abilities
| Ability | Key | 상태 | 비고 |
|---------|-----|------|------|
| GA_MeleeAttack | LMB | ✅ 완료 | ANS_Collider, GE_BasicDamage, GE_MeleeHitTag |
| GA_MeleeUnique | E | ✅ 완료 | ANS_Collider, GE_BasicDamage, GE_MeleeHitTag |
| GA_MeleeUltimate | Q | ✅ 완료 | AOE SphereOverlap, GE_BasicDamage, GE_HitTag |
| GA_Sprint | Shift | ✅ 완료 | GE_SprintBuff, GE_Sprint_Cost |
| GA_Dodge | — | ✅ 완료 | |
| GA_Shield | — | ✅ 완료 | GE_GiveShield |
| GA_SpeedBuff | F (Skill Wheel) | ✅ 완료 | GE_GenericCooldown 공유, GE_SpeedBuff |
| GA_Ablaze | F (Skill Wheel) | ✅ 완료 | 지면 AOE, GE_Ablaze(DoT), AC_FireZone, ReceivedTrueDamage |
| Skill Wheel 추가 스킬 x1~3 | F (Skill Wheel) | 📋 계획 중 | Skill Wheel 구현 전 선행 작업 |
| Skill Wheel (F키 슬롯 교체) | F | 📋 계획 중 | GA_SpeedBuff 포함 3~5개 스킬 순환 |

### Monster Abilities
| Ability | 상태 | 비고 |
|---------|------|------|
| BPC_MeleeMonsterNormalAttackGA | ✅ 완료 | AC_MeleeMonster 사용 |
| BPC_RangedMonsterNormalAttackGA | ✅ 완료 | AC_RangedMonster 사용 |
| BPC_BossBeamPatternGA | ✅ 완료 | AC_BossMonster 사용 |
| BPC_BossStormPatternGA | ✅ 완료 | AC_BossMonster 사용 |

### UI
| Widget | 상태 | 비고 |
|--------|------|------|
| WBP_HUD | ✅ 완료 | WBP_UltimateGauge 통합 완료 |
| WBP_SkillIcon | ✅ 완료 | WBP_HUD의 child widget (Common/Unique 2종) |
| WBP_UltimateGauge | ✅ 완료 | WBP_HUD의 child widget으로 포함 |
| BPC_NormalMonsterHPWidget | ✅ 완료 | 3D 위젯, UC_MonsterAttributeSet 바인딩 |
| BPC_BossMonsterHPWidget | ✅ 완료 | 3D 위젯, UC_MonsterAttributeSet 바인딩 |

### Effects
| Effect | 상태 | 비고 |
|--------|------|------|
| GE_PlayerAttributes | ✅ 완료 | 플레이어 초기 스탯 적용 |
| GE_BasicDamage | ✅ 완료 | Set by Caller, Data.Damage 태그 |
| GE_MeleeHitTag | ✅ 완료 | State.Hit 태그 |
| GE_MeleeAttack_Cooldown | ✅ 완료 | |
| GE_MeleeAttack_Cost | ✅ 완료 | |
| GE_GenericCooldown | ✅ 완료 | GA_SpeedBuff, GA_MeleeUnique 공유 |
| GE_SpeedBuff | ✅ 완료 | GA_SpeedBuff 사용 |
| GE_SprintBuff | ✅ 완료 | GA_Sprint 사용 |
| GE_Sprint_Cost | ✅ 완료 | GA_Sprint 사용 |
| GE_GiveShield | ✅ 완료 | GA_Shield 사용 |
| GE_Cost_Ultimate | ✅ 완료 | GA_MeleeUltimate 사용 |
| GE_UltimateBuff | ✅ 완료 | |
| GE_Ablaze | ✅ 완료 | 상태이상: 화염 — State.Ablaze 태그 부여 + GameplayCue.Debug.Ablaze, 데미지는 GE_DotDamage가 처리 |
| GE_DotDamage | ✅ 완료 | DoT 데미지 처리 — 플레이어(Health)·몬스터(ReceivedTrueDamage) 동시 지원, Set by Caller |
| GE_Wet | ✅ 완료 | 상태이상: 침수 |
| GE_ManaRegen | ✅ 완료 | |
| GE_ChargeMana | ✅ 완료 | |
| GE_StaminaRegen | ✅ 완료 | |
| GE_StaminaRegenDelay | ✅ 완료 | |
| GE_Recover_Health | ✅ 완료 | |
| GE_Recover_Stamina | ✅ 완료 | |

---

## Known Issues

> 알고 있지만 미해결인 버그를 기록. 해결 시 항목 삭제.

| 증상 | 재현 방법 | 담당 | 비고 |
|------|-----------|------|------|
| — | — | — | 현재 없음 |

---

## Debugging Checklist

새 GA/GE 작동 안 할 때 아래 순서로 확인:

1. **ASC 연결** — `ActivateAbility`의 `AbilitySystemComponent` 핀이 올바른 ASC에 연결되어 있는지 확인 (흔한 실수: 잘못된 타겟 핀)
2. **Collision 설정** — `ANS_Collider` NotifyEnd에서 `No Collision`이 아닌 `Query Only`로 설정되어 있는지 확인
3. **GameplayTag** — `DefaultGameplayTags.ini`에 태그가 등록되어 있는지 확인
4. **Ability 등록** — ASC에 GA가 `GiveAbility`로 등록되어 있는지 확인
5. **DrawDebugSphere** — AOE 디버그 시 `GA_MeleeUltimate`의 `DrawDebugSphere` 패턴 참고
6. **로그 확인** — `LogAbilitySystem` verbosity를 `Verbose`로 설정:
   ```ini
   ; DefaultEngine.ini
   [Core.Log]
   LogAbilitySystem=Verbose
   ```
7. **Set by Caller `Data None` 에러** — `FGameplayEffectSpec::GetMagnitude called for Data None` 발생 시 → GE의 Set by Caller Data Tag와 코드의 `RequestGameplayTag()` 태그명 일치 여부 확인
8. **GetHitResultUnderCursorByChannel이 항상 같은 위치 반환** — 3인칭 카메라(커서 숨김 + 마우스로 카메라 회전) 방식에서는 사용 불가 → 카메라 전방 `LineTraceByChannel`로 교체
9. **GameplayCue 이펙트가 맵 중앙(0,0,0)에 스폰될 때** — `GameplayCueNotify_Actor`의 Class Defaults에서 `Auto Attach to Owner = true` 확인. 또는 Begin Play에서 `Attach Actor to Component`로 캐릭터 Mesh에 명시적 부착
10. **`[C_SkillIconWidget] Failed to get skill data from: Default__GA_Heal_C` 경고** — 새로 추가한 스킬이 한 번도 활성화되지 않은 상태에서 위젯이 초기화될 때 발생. `C_SkillBase::LoadSkillData()`의 CDO 가드가 원인 (현재 코드에서 수정 완료 — CDO 가드 제거, `FindAbilityByTag`에서 인스턴스 우선 확인으로 변경)

---

## Common Patterns

### 새 PlayerGA 추가 체크리스트
1. `UC_CharacterGA` 상속 C++ 클래스 또는 Blueprint 생성
2. `ProjectBBKAbilityID` enum에 ID 추가 (필요 시)
3. `BP_MeleeCharacter`의 `GrantedAbilities`에 등록
4. Input binding: `UC_PlayerController`에서 `AbilityID`와 Enhanced Input Action 연결
5. 쿨다운 필요 시 `GE_GenericCooldown` 재사용, UI 자동 연동 확인

### Set by Caller 데미지 적용 패턴
```
ApplyGameplayEffectToTarget
  → Effect Class: GE_BasicDamage
  → Set by Caller Magnitude:
      Tag: Data.Damage
      Magnitude: (데미지 값)
```

### AOE Overlap 패턴 (GA_MeleeUltimate 참고)
```
SphereOverlapActors
  → Sphere Radius: (범위)
  → Actor Class Filter: AC_BaseMonster
  → Actors to Ignore: GetAvatarActorFromActorInfo
  → DrawDebugSphere (개발 중 시각화용)
→ ForEach → ApplyGameplayEffectToTarget
```

### 상태이상 GE 구조 패턴 (GE_Ablaze / GE_DotDamage 참고)
상태이상 태그 부여와 DoT 데미지 처리를 GE 두 개로 분리.
```
GE_Ablaze
  → 역할: State.Ablaze 태그 부여만 담당 (Modifier 없음)
  → Duration: 상태이상 지속 시간

GE_DotDamage
  → 역할: 실제 DoT 데미지 처리
  → Set by Caller (Data.Damage) 로 데미지 값 주입
  → 플레이어: UC_ChracterAttributeSetBase.Health 적용
  → 몬스터: UC_MonsterAttributeSet.ReceivedTrueDamage 적용
  → GAS가 타겟에 없는 AttributeSet Modifier를 자동 무시 → 분기 처리 불필요
```

### 지면 AOE 존 패턴 (GA_Ablaze / AC_FireZone 참고)
GA가 존을 스폰하고 즉시 EndAbility. 존이 자체적으로 GE 관리.
```
GA ActivateAbility
  → 지면 위치 탐색 (아래 패턴 참고)
  → SpawnActorFromClass (AC_FireZone)
  → Cast to AC_FireZone → Initialize
      (InstigatorASC, InstigatorActor, EffectClass, Radius, Lifetime, Damage)
  → ApplySkillCooldown → EndAbility

AC_FireZone::Initialize()
  → CollisionSphere 반경 설정
  → OnBeginOverlap 등록
  → OnInitialized(Radius) 호출 (BP에서 VFX 스케일 조정)
  → 수명 타이머 설정 후 OnExpired()에서 Destroy
```
- GE 클래스: `UC_SkillBase::GetTargetEffectClass(0)`으로 DT에서 가져옴
- Radius / zoneDuration / baseDamage: `GetSkillData → Break FSkillData`로 DT에서 가져옴

### Set by Caller + CurveTable 레벨 스케일링 패턴 (GA_Heal 참고)
`ApplySkillEffects()` 대신 GE Spec을 수동 생성해야 Set by Caller 값 주입 가능.
```
GA ActivateAbility
  → GetGameplayAttributeValue(UC_ChracterAttributeSetBase.level) → CurrentLevel
  → EvaluateCurveTableRow(CT_SkillData, RowName, CurrentLevel) → 수치
  → MakeOutgoingSpec(GE클래스, Level: 1)
  → AssignTagSetByCallerMagnitude(Data.태그, 수치)
  → ApplyGameplayEffectSpecToSelf
  → ApplySkillCooldown → EndAbility
```
- CT_SkillData 하나로 모든 스킬의 레벨별 수치를 통합 관리
- 레벨별 수치는 CSV Import로 일괄 입력 가능 (첫 행: 레벨, 첫 열: RowName)

### 지속 VFX GameplayCue 패턴 (GC_Heal / GC_Ablaze 참고)
GA가 EndAbility한 이후에도 GE 생명주기 동안 VFX가 자동 유지됨.
```
GameplayCueNotify_Actor 설정
  → Class Defaults: Auto Attach to Owner = true
  → Particle System Component 추가, Auto Activate = true
GE에 GameplayCue 태그 부여
  → GE 활성화 시: GC 액터 자동 스폰 + Owner(캐릭터)에 자동 부착
  → GE 만료 시: GC 액터 자동 소멸 + VFX 종료
```
타이머 없이 GE 생명주기와 VFX가 자동 동기화됨.

### 3인칭 카메라 지면 위치 탐색 패턴
커서가 없는 3인칭 카메라에서 캐릭터 전방 지면 좌표를 구하는 방법.
```
GetActorLocation + GetActorForwardVector × range → ForwardPoint

LineTraceByChannel
  Start: ForwardPoint + (0, 0, 500)
  End:   ForwardPoint + (0, 0, -500)
  TraceChannel: Visibility
→ ImpactPoint → 스폰 위치로 사용
```

---

## Design Decisions

> 설계 방식을 결정할 때 대안이 존재했던 경우 여기에 기록.
> 형식: 선택한 방식 / 고려했던 대안 / 선택 이유 / 트레이드오프

### GE_Ablaze + GE_DotDamage — 상태이상과 데미지 처리 분리
- **선택:** GE_Ablaze는 State.Ablaze 태그 부여 + GameplayCue.Debug.Ablaze 담당, 실제 DoT 데미지는 GE_DotDamage가 별도 처리
- **대안:** GE_Ablaze Modifier에 Health / ReceivedTrueDamage를 직접 포함해 단일 GE로 처리
- **선택 이유:** 상태이상 태그 관리와 데미지 수치 계산을 분리하면 각자 독립적으로 수정 가능. GE_DotDamage를 다른 상태이상(GE_Wet 등)에서도 재사용할 수 있음
- **트레이드오프:** GE가 두 개로 늘어나 적용 순서와 생명주기를 함께 관리해야 함

### GA_Heal — GE 단일 구조 (GE_Ablaze의 분리 구조 미적용)
- **선택:** GE_HealOverTime 하나로 Health Modifier + State.Healing 태그 + GameplayCue.Heal 통합
- **대안:** GE_Ablaze처럼 상태 태그용 GE와 수치 처리 GE를 분리
- **선택 이유:** GA_Heal은 회복 단일 목적이므로 분리할 책임이 없음. GE_Ablaze 분리는 DoT와 상태이상을 독립적으로 재사용하기 위한 것
- **트레이드오프:** 향후 State.Healing 태그를 다른 GE에서 재사용하려면 분리 구조로 리팩토링 필요

### Mana Charge — C++ SetByCaller vs MMC
- **선택:** C++ SetByCaller로 마나 충전량 전달
- **대안:** MMC(Modifier Magnitude Calculation)로 계산 로직 캡슐화
- **선택 이유:** 충전량이 전투 상황(피격, 공격 등)에 따라 호출 시점마다 달라지므로 런타임에 값을 직접 주입하는 SetByCaller가 적합. MMC는 AttributeSet 기반 정적 계산에 더 어울림
- **트레이드오프:** SetByCaller는 호출부에서 태그와 값을 직접 관리해야 하므로 태그 불일치 오류에 취약 (Debugging Checklist 7번 참고)