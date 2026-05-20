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
| GA_RockSpear | F (Skill Wheel) | ✅ 완료 | 2단계 입력, C_StoneSpearProjectile, GE_BasicDamage, GE_Slowed |
| Skill Wheel 추가 스킬 x1~3 | F (Skill Wheel) | 📋 계획 중 | GA_RockSpear 포함 1개 완료, 나머지 미구현 |
| Skill Wheel (F키 슬롯 교체) | F | ✅ 완료 | Z키 토글, WBP_SkillWheel, UC_SkillManagerComponent, DynamicAbilityTags → TryActivateAbilityByClass 방식 |
| GA_RanagedUnique | E | ✅ 완료 | C_TrapZone + BP_TrapZone, TriggerCapsule 감지 → CapsuleOverlap 데미지, GE_BasicDamage |
| GA_RangedUltimate | Q | ✅ 완료 | C_RangedUltimate, BoxOverlapActors 판정, LaunchCharacter(C++ 직접), GE_BasicDamage(BP CurveTable), State.UsingUltimate 입력 차단 |

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
| WBP_RockSpearAim | ✅ 완료 | 조준선 위젯, GA_RockSpear 생명주기 직접 관리, 마우스 위치 추적 |
| WBP_SkillWheel | ✅ 완료 | Z키 토글, 마우스 각도 기반 섹터 판정, 호버 강조, 클릭 시 스킬 교체 |
| WBP_HUD (캐릭터 교체 연동) | ✅ 완료 | OnCharacterSwitched 델리게이트로 교체 시 SkillIcon·UltimateGauge 재초기화 |

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
| GE_Slowed | ✅ 완료 | 상태이상: 감속 — State.Slowed 태그 부여 + MoveSpeed × 0.2, Duration 5초, GA_RockSpear 사용 |
| GE_GainExperience | ✅ 완료 | Set by Caller, Data.Exp 태그, experience 어트리뷰트 가산 |

### Objects
| Object | 상태 | 비고 |
|--------|------|------|
| C_ExpOrb / BP_ExpOrb | ✅ 완료 (C++ 구현) | Overlap → GE_GainExperience 적용 후 Destroy, 스폰 주체 미구현 |

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
11. **Duration GE가 attribute 변경 시 외부 컴포넌트에 반영 안 될 때** — `PostGameplayEffectExecute`는 Instant GE에만 호출됨. Duration GE의 Modifier는 `PostAttributeChange`에서 처리해야 함. 예: `GE_Slowed`로 `moveSpeed`를 바꿔도 `MaxWalkSpeed`가 갱신되지 않는 경우 → `PostAttributeChange` 오버라이드로 해결
12. **Instant GE의 `FActiveGameplayEffectHandle.IsValid()`가 항상 false** — Instant GE는 즉시 실행 후 사라지므로 Active Handle이 존재하지 않음. `IsValid() = false`가 실패를 의미하지 않음 — 실제 적용 여부는 `PostGameplayEffectExecute` 로그로 확인
13. **몬스터에게 데미지가 0으로 들어올 때** — `PostGameplayEffectExecute`의 방어력 감산 로직 확인: `Mitigated = Max(0, RawDamage - defense)`. defense 값이 데미지보다 크면 실제 HP 변화 없음
14. **`TryActivateAbilitiesByTag`로 DynamicAbilityTags 태그가 있는 GA가 발동 안 될 때** — `TryActivateAbilitiesByTag`는 `DynamicAbilityTags`를 조회하지 않음. `UC_SkillManagerComponent::GetActiveSkillClass()`로 클래스를 직접 가져와 `TryActivateAbilityByClass` 사용
15. **GA Blueprint 내부에서 `GetAbilitySystemComponent`가 None 반환** — GA Blueprint에서 Character 경유로 ASC를 가져오면 PIE 종료 시 또는 타이밍에 따라 None 반환 가능 → GA 내부에서는 반드시 `GetAbilitySystemComponentFromActorInfo` 사용 (IsValid 체크 불필요, 어빌리티 활성 중 항상 유효)
16. **Duration GE 기반 GameplayCue 이펙트가 두 번째 시전부터 안 나올 때** — GC Manager가 Actor를 재활용(Recycle)하므로 두 번째 시전 시 `BeginPlay`가 호출되지 않음. `Auto Activate`만으로는 재시작 불가 → `HandleGameplayCue` 이벤트 override → `Switch on EGameplayCueEvent Type` → OnActive 브랜치에서 Niagara `Activate(Reset=true)` 명시 호출. `Reset=true` 없이 `Activate()`만 하면 이미 완료된 Niagara가 재시작되지 않음
17. **캐릭터 교체 후 HUD(스킬 아이콘·궁극기 게이지)가 기본값만 표시될 때** — `RemoveCharacterAbilities`의 early-return 조건 확인: `!abilitySystemComponent->characterAbilitiesGiven` 이어야 함. `!` 누락 시 어빌리티가 등록된 상태(`true`)에서 early return → 제거도, 신규 추가도 모두 안 됨
18. **캐릭터 로스터 시스템에서 최초 실행 시 HUD가 기본값만 표시될 때** — 캐릭터의 `BeginPlay`에서 WBP_HUD를 생성하는 경우, 컨트롤러가 캐릭터들을 일괄 스폰한 뒤 `Possess`를 호출하는 구조에서는 `BeginPlay` 시점이 `Possess` 이전임. `NativeConstruct`가 실행될 때 아직 `AddCharacterAbilities`가 호출되지 않아 ASC에 어빌리티가 없음 → 초기 `Possess` 완료 후 `OnCharacterSwitched.Broadcast(0)` 호출로 해결 (이 시점엔 델리게이트 바인딩·어빌리티 등록이 모두 완료된 상태)
19. **캐릭터 교체 후 `C_SkillIconWidget` 쿨다운이 반응 없을 때** — `InitializeSkillIcon` 재호출 전 기존 `OnCooldownStarted` 델리게이트를 `RemoveDynamic`으로 해제해야 함. 누락 시 이미 제거된 어빌리티 인스턴스에 바인딩이 남아 이벤트를 수신하지 못함. `C_UltimateGaugeWidget::InitializeGauge`도 동일하게 기존 `ManaChangedHandle`을 `Remove` 후 재바인딩
20. **UFUNCTION 이름이 부모 클래스와 충돌 시 UHT 에러** — `Override of UFUNCTION 'X' in parent 'Y' cannot have a UFUNCTION() declaration` 에러 발생. `UGameplayAbility`에 이미 `GetCooldownTimeRemaining`이 존재하는 것처럼, 부모 클래스의 UFUNCTION과 동일한 이름으로 선언 불가. 함수 추가 전 부모 클래스 API를 먼저 확인할 것 (예: `GetCooldownTimeRemaining` → `QuerySkillCooldown`으로 변경)
21. **DecalComponent가 원형이 아닌 사각형으로 투영될 때** — Decal은 로컬 **-X 방향**으로 투영. 바닥 수직 투영을 위해 Pitch -90도 필요. BP 컴포넌트 Transform에서 **Y = -90** 설정 (Y가 Pitch). DecalSize Y ≠ Z이면 직사각형으로 나오므로 Y = Z도 함께 확인.
22. **LineTrace `bBlockingHit == false` 시 `ImpactPoint` 값** — Hit 없을 때 `ImpactPoint = FVector(0, 0, 0)` (월드 원점). 체크 없이 사용하면 액터가 원점에 스폰됨. `bBlockingHit == false` 시 스폰하지 않고 `EndAbility`로 처리할 것.
23. **Persistent Debug Line이 Actor 소멸 후에도 남아있을 때** — `DrawDebugCapsule(bPersistentLines=true)`는 Actor `Destroy()` 시 자동 클리어되지 않음. `DestroyTrap` 등 소멸 함수 내에서 `FlushPersistentDebugLines(GetWorld())`를 명시적으로 호출해야 함. 단, 월드 전체 Persistent 라인을 일괄 클리어하므로 다른 Persistent 드로우와 충돌 가능.
24. **GameplayCueNotify_Static에서 Instigator가 None일 때** — Instant GE의 GC Parameters에서 `Instigator`가 None으로 전달되는 경우가 있음. 방향 계산처럼 Instigator에 의존하는 로직은 GC에 두지 말고 C++ GA의 HandleNotifyEvent 등에서 직접 처리할 것. 면역은 ASC 태그(`State.KnockbackImmune` 등) 수동 체크로 구현.
25. **캐릭터 교체 후 이전 캐릭터의 쿨다운 오버레이가 계속 표시될 때** — `InitializeSkillIcon`에서 `currentCooldownTime = 0.f` / `maxCooldownTime = 0.f` 리셋 확인. 누락 시 `NativeTick`이 이전 값으로 계속 `UpdateCooldown`을 호출해 `SetCooldownVisible(true)`가 재호출됨. `InitializeFromCommonSkill`에는 리셋이 있으므로 두 경로 비교할 것.
26. **`InstancedPerActor` GA의 쿨다운 델리게이트가 캐릭터 교체 후 수신 안 될 때** — `InstancedPerActor` 정책에서 첫 `ActivateAbility` 전에는 인스턴스가 없어 `GetPrimaryInstance()` = null → CDO에 바인딩됨. 실제 쿨다운 브로드캐스트는 인스턴스에서 발생하므로 수신 불가. `ApplyGenericCooldown`에서 `GetClass()->GetDefaultObject<UC_SkillBase>()` 경유로 CDO에도 함께 브로드캐스트해야 함.
27. **`InitializeSkillIcon` 재호출 후 `OnCooldownStarted`가 SkillTag mismatch로 무시될 때** — `InitializeSkillIcon`에서 찾은 어빌리티의 `skillTag`로 위젯의 `SkillTag`를 업데이트하지 않으면 이전 캐릭터의 태그가 남아 새 어빌리티의 쿨다운 브로드캐스트가 tag check에서 필터링됨. `SkillTag = SkillData.skillTag` 업데이트 필수. `InitializeFromCommonSkill`에는 있으나 `InitializeSkillIcon`에서 누락되기 쉬움.
28. **`QuerySkillCooldown`의 `OutDuration`이 Remaining과 같은 값으로 반환될 때** — `GetActiveEffectsTimeRemainingAndDuration`의 `Value`(Duration)가 SetByCaller Duration GE에서 올바르지 않은 값을 반환하는 경우가 있음. `OutDuration`을 GE에서 읽지 말고 `CachedSkillData.cooldown`에서 직접 가져올 것.

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

### Duration GE 기반 GameplayCue Actor 패턴 (GC_Haste 참고)
GC Manager의 Actor 재활용에 대응하기 위해 `BeginPlay`의 Auto Activate 대신 `HandleGameplayCue`에서 명시적으로 Niagara를 재시작.
```
GameplayCueNotify_Actor 설정
  → Class Defaults: Auto Attach to Owner = true
  → Niagara Component: Local Space = true (캐릭터 추적 시)
  → Niagara Component: Location Z = -(캡슐 Half Height) (바닥 정렬 시)

HandleGameplayCue (EventType)
  → Switch on EGameplayCueEvent Type
      OnActive → Niagara Component들 Activate(Reset=true)   ← Reset=true 필수
GE 설정
  → Gameplay Cues 섹션에 태그 추가   ← Granted Tags 섹션 아님
```
- `Auto Attach to Owner`는 캡슐 중심에 부착 → 바닥 이펙트는 Z 오프셋 필요
- Local Space = true로 캐릭터 추적 가능하지만, World Space 파티클은 Actor 이동과 무관하게 동작

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

### 설치형 트랩 존 패턴 (C_TrapZone / BP_TrapZone 참고)
GA가 트랩을 스폰하고 즉시 EndAbility. 동시 1개 제한은 GA가 `CurrentTrap` 레퍼런스로 관리.
트리거 판정(`UCapsuleComponent` Overlap)과 데미지 판정(`CapsuleOverlapActors`)을 별도 크기로 분리해 감지 범위와 피해 범위를 독립 조정.
```
GA ActivateAbility
  → IsValid(CurrentTrap) → DestroyTrap()        // 기존 트랩 제거
  → LineTrace 아래 방향 → bBlockingHit false → EndAbility (원점 스폰 방지)
  → SpawnActor(TrapZoneClass, ImpactPoint + Z*TriggerHalfHeight)
      Owner: AvatarActor
  → InitTrap(ASC, DamageMagnitude, LifeTime, TriggerRadius, TriggerHalfHeight, DamageRadius, DamageHalfHeight)
  → ApplySkillCooldown → EndAbility

AC_TrapZone::InitTrap()
  → TriggerCapsule 크기 설정 + DecalMaterial 적용(SetDecalMaterial) + LifeTimer 시작
  → LifeTimer는 반드시 InitTrap에서 시작 (BeginPlay 시점엔 수치 미주입)

AC_TrapZone::OnTriggerOverlap (TriggerCapsule)
  → bTriggered 플래그로 중복 방지
  → ClearTimer(LifeTimer) + DecalComp/PointLightComp 숨김 + ImpactNiagara 스폰
  → CapsuleOverlapActors(DamageCapsule 크기, AC_BaseMonster 필터) → GE_BasicDamage 적용
  → 4.5초 후 DestroyTrap

DestroyTrap()
  → ClearTimer + FlushPersistentDebugLines + Destroy
```
- `DamageEffectClass`는 BP에서 지정 (C++ 하드코딩 금지) → BP_TrapZone에서 GE_BasicDamage 지정
- DecalComponent 바닥 투영: BP에서 컴포넌트 Transform Y = -90 (Pitch -90도)
- `CapsuleOverlapActors`의 `ActorsToIgnore`: AC_BaseMonster 클래스 필터로 플레이어 자동 제외 → 빈 배열로 충분

### 몽타주 + AnimNotify 기반 궁극기 패턴 (C_RangedUltimate 참고)
GA가 몽타주를 재생하고, AnimNotify 수신 시점에 Box 판정·넉백을 실행. 데미지는 Blueprint(CurveTable 레벨 조회), 넉백은 C++ 직접 처리.
```
ActivateAbility
  → CommitAbility 실패 시 EndAbility(bWasCancelled=true) → return
  → PlayMontageAndWait: OnCompleted/Interrupted/Cancelled → OnMontageEnded → EndAbility
  → WaitGameplayEvent(NotifyEventTag) → HandleNotifyEvent
  → SetIgnoreMoveInput(true)

HandleNotifyEvent (C++)
  → BoxCenter = AvatarLocation + ForwardVector * BoxOriginOffsetX
  → BoxOverlapActors(ObjectTypeQuery3, AC_BaseMonster 필터, ActorsToIgnore: AvatarActor)
  → ForEach HitActor:
      HasMatchingGameplayTag(State.KnockbackImmune) → skip
      LaunchCharacter(Direction(수평 정규화) * KnockbackForce, XYOverride=true, ZOverride=false)
  → OnNotifyReceived_BP(HitActors, BoxCenter, BoxExtent)   ← 데미지·DrawDebugBox는 BP

OnNotifyReceived_BP (Blueprint)
  → Level 조회 → EvaluateCurveTableRow(CT_SkillData) → Damage
  → ForEach HitActors: MakeOutgoingSpec(DamageEffect) → SetByCallerMagnitude(Data.Damage) → Apply
  → DrawDebugBox(BoxCenter, BoxExtent, Red, LifeTime=5)

EndAbility
  → ResetIgnoreMoveInput → ApplySkillCooldown → Super::EndAbility
```
- BoxStart = BoxCenter - ForwardVector * BoxExtent.X → Niagara 스폰 위치 (캐릭터 바로 앞, 박스 전체 커버)
- AnimNotify(AN_UltimateFire)의 SendGameplayEventToActor Target: 반드시 `Try Get Pawn Owner` 사용

### 어빌리티 사용 중 입력 전체 차단 패턴
이동은 C++, 어빌리티 발동은 GAS 태그로 각각 차단. 카메라는 영향 없음.
```
// C++: 이동 차단
ActivateAbility → PlayerController → SetIgnoreMoveInput(true)
EndAbility      → PlayerController → ResetIgnoreMoveInput()

// Blueprint Details: 어빌리티 발동 차단
GA_Ultimate   → Tags → Activation Owned Tags  : State.UsingUltimate  ← 활성 중 자동 부여
다른 어빌리티 → Tags → Activation Blocked Tags: State.UsingUltimate  ← 태그 있으면 발동 불가
```

### 플레이어 Projectile 스킬 패턴 (C_StoneSpearProjectile 참고)
GA가 Projectile을 스폰하고 즉시 EndAbility. Projectile이 자체적으로 GE 관리.
```
GA ActivateAbility
  → SpawnActor(BP_StoneSpearProjectile)
  → Cast → InitProjectile(InstigatorASC, InstigatorActor, DamageGE, StatusGE, Damage, Direction)
  → ApplySkillCooldown → EndAbility

AC_StoneSpearProjectile::InitProjectile()
  → TWeakObjectPtr에 ASC·Actor·GE 클래스 저장
  → ProjectileMovement->Velocity 설정
  → Collision->IgnoreActorWhenMoving(InstigatorActor)   // 발사자 충돌 제외

OnSphereBeginOverlap (ECC_Pawn → Overlap)
  → Cast to AC_BaseMonster 성공 시 ApplyEffectsToTarget → HandleImpact

OnComponentHit (WorldStatic/Dynamic → Block)
  → HandleImpact

HandleImpact()
  → StopMovementImmediately + NoCollision
  → ProjectileEffect->Deactivate, ImpactEffect->Activate
  → SetLifeSpan(2.0f)   // Impact VFX 재생 후 소멸
```
- NiagaraComponent를 비행용(ProjectileEffect)·충돌용(ImpactEffect) 두 개로 분리
- 즉시 Destroy 대신 SetLifeSpan 사용 — Actor 살아있는 동안 ImpactEffect 재생 보장
- Instant GE는 `FActiveGameplayEffectHandle.IsValid() = false`여도 정상 적용 (Debugging Checklist 12번 참고)

### 2단계 입력 어빌리티 패턴 (GA_RockSpear 참고)
조준 → 발사 구조처럼 동일 키를 두 번 눌러 단계를 진행하는 어빌리티.
```
F키 Input Action (Pressed 트리거)
→ SendGameplayEventToActor (Tag: Event.Skill.XXX.Fire)  ← 반드시 먼저
→ TryActivateAbilityByTag

GA ActivateAbility
  → SetIgnoreLookInput(true)
  → Create Widget → Add to Viewport
  → Wait Gameplay Event (Tag: Event.Skill.XXX.Fire) → On Event → 발사 확정 로직
  → CheckCancelInput 호출 (폴링 루프)

CheckCancelInput (Custom Event)
  → Wait Game Time (0.05)
  → Is Input Key Down (LMB or RMB) → True: 취소 / False: 재귀 호출

종료 시퀀스 (확정·취소 공통, EndAbility 직전)
  → Remove from Parent (Widget)
  → SetIgnoreLookInput(false)
```
- `SendGameplayEventToActor`가 `TryActivateAbilityByTag`보다 반드시 앞에 와야 함 (순서 반대 시 즉시 발사)
- `IA_CommonSkill` 트리거 타입은 반드시 `Pressed`로 설정 (`Triggered`면 매 프레임 발동)
- `OnInputPressed` BlueprintImplementableEvent: `UC_CharacterGA`에서 `InputPressed` C++ override → Blueprint 노출. 어빌리티 활성 중 동일 입력 감지 필요 시 활용 가능 (단, `TryActivateAbilityByTag` 방식에서는 `AbilityLocalInputPressed` 미호출로 동작 안 함)

### SkillWheel + 입력 차단 패턴 (UC_SkillManagerComponent / WBP_SkillWheel 참고)
Z키 토글로 SkillWheel을 열고, 마우스 각도 기반으로 스킬을 선택해 F키 슬롯을 교체.
```
// 입력 차단 — ActivationBlockedTags에 State.SkillWheelOpen 추가 (차단할 GA 전부)
OpenSkillWheel()
  → ASC->AddLooseGameplayTag(State.SkillWheelOpen)   // 등록된 GA 자동 차단
  → bIsSkillWheelOpen = true

CloseSkillWheel()
  → ASC->RemoveLooseGameplayTag(State.SkillWheelOpen)
  → bIsSkillWheelOpen = false

// Z키 Blueprint 핸들러
Branch: bIsSkillWheelOpen
  False → SetShowMouseCursor(true) → SetMouseLocation(Center)
          → SetIgnoreLookInput(true) → OpenSkillWheel
          → Create WBP_SkillWheel → Add to Viewport
          → Set Input Mode Game and UI   ← 첫 번째 클릭 정상 인식에 필수
  True  → CloseSkillWheel → Remove from Parent
          → SetShowMouseCursor(false) → SetIgnoreLookInput(false)
          → Set Input Mode Game Only

// WBP_SkillWheel Event Tick — 섹터 판정
GetMousePosition (PlayerController) → DeltaX, DeltaY (화면 중심 기준)
Vector2D Length(Delta) → Branch: < 휠 반지름
  False → HoveredIndex = -1 (데드존)
  True  → Atan2(DeltaY, DeltaX) + 180 → / 90 → Floor → HoveredIndex
          → 해당 슬롯 Border 강조

// WBP_SkillWheel OnMouseButtonDown
Branch: HoveredIndex == -1 → 아무것도 안 함
  False → SkillManagerComp → SwitchCommonSkill(HoveredIndex) → CloseSkillWheel
          → Remove from Parent → SetShowMouseCursor(false) → SetIgnoreLookInput(false)
          → Set Input Mode Game Only
```
- `TryActivateAbilitiesByTag`는 DynamicAbilityTags 미조회 → `GetActiveSkillClass + TryActivateAbilityByClass` 사용 (Debugging Checklist 14번 참고)
- 조준 중인 GA(GA_StoneSpear 등) 취소: CheckCancelInput 폴링 루프에 `HasMatchingGameplayTag(State.SkillWheelOpen)` 조건 추가 + `GetAbilitySystemComponentFromActorInfo` 사용 (Debugging Checklist 15번 참고)

### 3인칭 카메라 기반 Projectile 조준 패턴 (GA_RockSpear 참고)
조준선(커서 대체 위젯)과 발사 방향을 완전히 일치시키는 방법.
```
// 조준 방향 계산
GetMousePosition → DeprojectScreenPositionToWorld → WorldOrigin, WorldDir

// Projectile 스폰
SpawnLocation = WorldOrigin + WorldDir * 100   // 카메라 앞 100cm
FireDir = WorldDir                              // 크로스헤어와 완전 일치

// 조준선 위젯 위치 (SetPositionInViewport)
SetPositionInViewport(MouseX - DesiredSize.X / 2, MouseY - DesiredSize.Y / 2)
// 위젯 기준점이 좌상단이므로 크기의 절반만큼 offset 필수
```
- `(TargetPoint - SpawnLocation).Normalize()` 방식은 카메라-스폰 시차(Parallax)로 근거리 오차 발생
- WorldDir 직접 사용 시 LineTrace 불필요 — `bBlockingHit` 폴백 처리도 생략 가능
- 조준 모드 진입 시 `SetIgnoreLookInput(true)`로 카메라 회전 차단 → 마우스 위치가 의미 있어짐

### 캐릭터 교체 후 HUD 재초기화 패턴 (OnCharacterSwitched 참고)
캐릭터 로스터 시스템에서 교체 시마다 HUD 위젯을 새 어빌리티 인스턴스로 재바인딩.
```
// C_PlayerController
OnCharacterSwitched (BlueprintAssignable, FOnCharacterSwitched)

SwitchToCharacter(NextIndex)
  → Possess(NewChar)                          // PossessedBy → AddCharacterAbilities 완료
  → OnCharacterSwitched.Broadcast(NextIndex)  // 어빌리티 등록 후 브로드캐스트

BeginPlay() 초기 Possess 직후
  → OnCharacterSwitched.Broadcast(0)
  // WBP_HUD는 각 캐릭터 BeginPlay(Possess 전)에서 생성 → NativeConstruct에서 델리게이트 바인딩
  // Possess 완료(= AddCharacterAbilities 완료) 후 브로드캐스트 → 어빌리티가 ASC에 있는 상태에서 초기화

// 위젯 재초기화 공통 원칙
InitializeSkillIcon(ASC) / InitializeGauge()
  → 기존 델리게이트/핸들 먼저 해제 (RemoveDynamic / FDelegateHandle.Remove)
  → 새 어빌리티 탐색 및 바인딩, 현재 값으로 초기 표시 갱신

// WBP_HUD Event Construct
→ Bind Event to OnCharacterSwitched (PlayerController 경유)
→ 핸들러: GetControlledPawn → GetASC
    → WBP_SkillIcon Unique: InitializeSkillIcon(ASC)
    → WBP_SkillIcon Common: SwitchCommonSkill(activeSkillIndex) 경유 (아래 패턴 참고)
    → WBP_UltimateGauge: InitializeGauge()
```
- 재초기화 함수에서 기존 바인딩 해제를 빠트리면 제거된 인스턴스에 델리게이트가 남아 쿨다운·게이지 이벤트 미수신
- `OnASCInitialized`는 `AddCharacterAbilities` 이전에 호출되므로 위젯 초기화 트리거로 사용 불가

### CommonSkill 아이콘 동적 갱신 패턴 (SkillWheel + 캐릭터 교체 연동)
UniqueSkill(고정)과 달리 CommonSkill은 SkillWheel로 독립 변경되므로 별도 델리게이트로 관리.
```
// C_SkillManagerComponent
OnCommonSkillSwitched (BlueprintAssignable, FOnCommonSkillSwitched)

SwitchCommonSkill(NewIndex)
  → activeSkillIndex 갱신
  → ASC에서 클래스로 어빌리티 탐색 (FindAbilityByTag 아님)
  → OnCommonSkillSwitched.Broadcast(FoundAbility)

// C_SkillIconWidget
InitializeFromCommonSkill(NewSkill)
  → 기존 OnCooldownStarted 해제
  → 아이콘 설정 + SkillTag 동적 업데이트 (FSkillData.skillTag 값으로)
  → QuerySkillCooldown(ASC, Remaining, Duration)  ← 교체 시점 쿨다운 상태 복원
      쿨다운 중: UpdateCooldown(Remaining, Duration)
      아닌 경우: SetCooldownVisible(false) 유지
  → OnCooldownStarted 재바인딩

// WBP_HUD OnCharacterSwitched 핸들러
IsValid(CurrentSkillManager)
  True  → UnbindEvent OnCommonSkillSwitched ──┐
  False ──────────────────────────────────────┘
    → Get Component by Class → Set CurrentSkillManager
    → BindEvent OnCommonSkillSwitched → OnCommonSkillSwitchedHandler
    → SwitchCommonSkill(activeSkillIndex)   ← 초기화 트리거 겸용
    → InitializeSkillIcon(ASC)              ← UniqueSkill
    → InitializeGauge()

// OnCommonSkillSwitchedHandler (Custom Event, 파라미터: UC_SkillBase* NewSkillAbility)
→ SkillIcon Common → InitializeFromCommonSkill(NewSkillAbility)
```
- `SwitchCommonSkill`은 SkillWheel 교체와 캐릭터 교체 초기화 양쪽의 단일 진입점
- CommonSkill 위젯의 `SkillTag`는 에디터에서 설정하지 않고 `InitializeFromCommonSkill` 내부에서 동적 설정
- WBP_HUD는 캐릭터 교체 시마다 이전 `OnCommonSkillSwitched` 바인딩을 해제하고 새 캐릭터의 컴포넌트에 재등록

### 스폰형 픽업 아이템 패턴 (C_ExpOrb 참고)
GA 없이 AActor 단독으로 Overlap → GE 적용 → Destroy하는 픽업 구조.
```
AC_ExpOrb (AActor)
  생성자
    → CollisionSphere: QueryOnly, 모두 Ignore, ECC_Pawn만 Overlap
    → NiagaraEffect: SetupAttachment

  BeginPlay
    → OnComponentBeginOverlap 바인딩

  OnOrbOverlap
    → bConsumed 체크 → true 시 return        // 중복 수집 방지
    → Cast<AC_BasePlayerCharactor> 실패 → return
    → GetAbilitySystemComponent() → nullptr 체크
    → bConsumed = true
    → MakeOutgoingSpec → SetSetByCallerMagnitude(Data.Exp, ExpAmount)
    → ApplyGameplayEffectSpecToSelf
    → Destroy()
```
- Collision Profile 대신 Cast 필터로 수집 대상 제한 → 커스텀 프로파일 불필요 (Design Decisions 참고)
- `bConsumed = true`는 GE 적용 직전에 설정 — Destroy 지연 프레임 대비
- `ExpAmount`와 `GE_GainExperience`는 BP에서 할당 (C++ 하드코딩 금지)
- 스폰 주체(몬스터 Die 등)는 C_ExpOrb 외부에서 처리

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

### PostAttributeChange vs PostGameplayEffectExecute — attribute 변경 외부 반영 위치
- **선택:** `PostAttributeChange`에서 `moveSpeed` → `MaxWalkSpeed` 동기화 (`UC_MonsterAttributeSet` 구현)
- **대안:** `PostGameplayEffectExecute`에서 처리
- **선택 이유:** `PostGameplayEffectExecute`는 Instant GE 실행 시에만 호출됨. Duration GE(GE_Slowed 등)의 Modifier 변경은 `PostAttributeChange`를 통해야 모든 케이스 커버 가능
- **트레이드오프:** `PostAttributeChange`는 모든 attribute 변경에 호출되므로 attribute 분기 처리 필수

### Projectile SpawnLocation — 카메라 위치 기반
- **선택:** `WorldOrigin + WorldDir * 100` (카메라 기준 스폰)
- **대안:** 캐릭터 위치 + ForwardVector offset
- **선택 이유:** 캐릭터-카메라 시차(Parallax)로 인해 캐릭터 기준 발사 시 크로스헤어 방향과 실제 발사 방향이 어긋남. 카메라 기준 발사 시 크로스헤어와 완전 일치
- **트레이드오프:** 투사체가 캐릭터 모델이 아닌 카메라 앞 허공에서 시작하는 것처럼 보일 수 있음 (VFX로 커버 가능)

### SkillWheel 스킬 교체 — TryActivateAbilityByClass vs TryActivateAbilitiesByTag
- **선택:** `UC_SkillManagerComponent::GetActiveSkillClass()`로 현재 활성 GA 클래스를 가져와 `TryActivateAbilityByClass` 호출
- **대안:** 활성 스킬에 `DynamicAbilityTags`로 `Ability.Skill.Common` 태그를 추가하고 `TryActivateAbilitiesByTag`로 발동
- **선택 이유:** `TryActivateAbilitiesByTag`는 `DynamicAbilityTags`를 조회하지 않아 발동되지 않음이 실험으로 확인됨
- **트레이드오프:** F키 Blueprint에서 SkillManagerComponent 참조가 필요해짐. DynamicAbilityTags 조작 코드는 불필요해져 `SwitchCommonSkill`이 단순히 `activeSkillIndex`만 갱신하는 구조로 단순화됨

### CommonSkill 아이콘 갱신 — OnCommonSkillSwitched 델리게이트 vs OnCharacterSwitched + FindAbilityByTag
- **선택:** `SkillManagerComponent`에 `OnCommonSkillSwitched` 델리게이트 추가, `SwitchCommonSkill()` 호출 시 브로드캐스트
- **대안:** `OnCharacterSwitched` 시 `FindAbilityByTag`로 CommonSkill 탐색
- **선택 이유:** CommonSkill은 캐릭터 교체와 무관하게 SkillWheel로 독립 변경됨. tag-based 탐색은 고정 태그가 필요한데 CommonSkill은 SkillWheel 리팩토링 후 고정 GAS 태그 없음. `SwitchCommonSkill`을 단일 진입점으로 사용하면 교체·초기화 두 경우를 동일 코드로 처리 가능
- **트레이드오프:** WBP_HUD가 캐릭터 교체 시마다 `OnCommonSkillSwitched` 바인딩을 해제·재등록해야 함

### TrapZone 스폰 위치 — LineTrace vs 수동 계산
- **선택:** 캐릭터 발 위치 아래로 `LineTraceByChannel`, `ImpactPoint + TriggerHalfHeight`를 스폰 위치로 사용
- **대안:** `ActorLocation.Z - CapsuleHalfHeight + TriggerHalfHeight` 수동 계산
- **선택 이유:** 경사면·계단에서 수동 계산은 실제 지면과 Z값이 어긋남. LineTrace는 지형에 무관하게 정확한 지면 좌표 반환
- **트레이드오프:** `bBlockingHit == false` 엣지 케이스 처리 필요 → EndAbility로 처리 (Debugging Checklist 22번 참고)

### TriggerCapsule / DamageCapsule 분리 — 단일 컴포넌트 vs 분리
- **선택:** 트리거 판정(`UCapsuleComponent` Overlap)과 데미지 판정(`CapsuleOverlapActors`)을 별도 크기 파라미터로 분리
- **대안:** 동일 컴포넌트·동일 크기로 감지와 피해 범위를 통일
- **선택 이유:** 발동 감지 범위(좁게)와 실제 피해 범위(넓게)를 독립적으로 조정 가능. GA Blueprint 변수로 각각 노출해 디자인 조정 용이
- **트레이드오프:** InitTrap 파라미터가 늘어남 (반경 2개 + 반높이 2개)

### 쿨다운 오버레이 방향 — 줄어드는 방향 vs 차오르는 방향
- **선택:** `Progress = CurrentCooldown / MaxCooldown` — 쿨다운 시작 시 꽉 찬 상태에서 시간이 지날수록 줄어듦
- **대안:** `Progress = 1.0f - (CurrentCooldown / MaxCooldown)` — 빈 상태에서 차오르다가 완료
- **선택 이유:** 줄어드는 방향이 "남은 시간이 소진된다"는 시각적 직관에 부합
- **트레이드오프:** 머티리얼의 Progress 파라미터가 1→0 방향으로 동작해야 하므로 머티리얼 설계 방향과 맞춰야 함

### Mana Charge — C++ SetByCaller vs MMC
- **선택:** C++ SetByCaller로 마나 충전량 전달
- **대안:** MMC(Modifier Magnitude Calculation)로 계산 로직 캡슐화
- **선택 이유:** 충전량이 전투 상황(피격, 공격 등)에 따라 호출 시점마다 달라지므로 런타임에 값을 직접 주입하는 SetByCaller가 적합. MMC는 AttributeSet 기반 정적 계산에 더 어울림
- **트레이드오프:** SetByCaller는 호출부에서 태그와 값을 직접 관리해야 하므로 태그 불일치 오류에 취약 (Debugging Checklist 7번 참고)

### LaunchCharacter — GC 경유 vs C++ 직접 호출
- **선택:** C++ HandleNotifyEvent에서 `LaunchCharacter` 직접 호출, 면역은 `State.KnockbackImmune` ASC 태그 수동 체크
- **대안:** GE_Knockback → GC_Knockback(GameplayCueNotify_Static) On Execute에서 LaunchCharacter, Instigator로 방향 계산
- **선택 이유:** Instant GE의 GC Parameters에서 Instigator가 None으로 전달되어 방향 계산 불가 확인(Debugging Checklist 24번). C++에서 AvatarActor·Target을 직접 참조하면 방향 계산이 확실하고 GE/GC 에셋 불필요
- **트레이드오프:** GAS 내장 Immunity GE 시스템 우회 → `State.KnockbackImmune` 태그 수동 체크로 대체. 넉백 면역 몬스터 추가 시 해당 태그를 GE로 부여하면 됨

### Niagara 이펙트 스폰 위치 — BoxStart vs BoxCenter
- **선택:** `BoxStart = BoxCenter - ForwardVector * BoxExtent.X` (박스 시작점, 캐릭터 바로 앞)에서 스폰
- **대안:** BoxCenter(박스 중심)에서 스폰
- **선택 이유:** BoxCenter 스폰 시 캐릭터에서 너무 멀고, 이펙트 크기가 박스와 같을 때 절반만 박스 안에 위치. BoxStart에서 스폰하면 캐릭터 바로 앞에서 시작해 박스 전체를 커버
- **트레이드오프:** Blueprint에서 `ForwardVector * BoxExtent.X`를 빼는 계산이 추가 필요. C++ 파라미터 추가 없이 BP에서 처리 가능

### 픽업 아이템 Collision — Collision Profile vs Cast 필터
- **선택:** `QueryOnly` + `SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap)` + `OnOrbOverlap`에서 `Cast<AC_BasePlayerCharactor>` 필터
- **대안:** 커스텀 Collision Profile `"OverlapOnlyPawn"` 정의
- **선택 이유:** 어차피 콜백에서 Cast 필터를 사용하므로 커스텀 프로파일은 중복 관리. 기본 채널 설정만으로 동일한 결과 달성 가능
- **트레이드오프:** Pawn 채널 전체가 Overlap 대상이 되므로 몬스터 Pawn도 콜백 진입하나, Cast 실패로 즉시 return되므로 실질적 영향 없음

### `QuerySkillCooldown`의 `OutDuration` 소스 — GE ActiveEffect vs 스킬 데이터
- **선택:** `OutDuration = bSkillDataLoaded ? CachedSkillData.cooldown : Results[0].Value` — 스킬 데이터 원본값 우선
- **대안:** `Results[0].Value` (GE의 Duration)만 사용
- **선택 이유:** `GetActiveEffectsTimeRemainingAndDuration`이 SetByCaller Duration GE에서 전체 Duration 대신 TimeRemaining과 같은 값을 반환하는 케이스가 확인됨
- **트레이드오프:** GE Duration과 스킬 데이터 cooldown 값이 다른 경우(런타임 쿨다운 스케일링 등)엔 스킬 데이터 값이 부정확할 수 있음