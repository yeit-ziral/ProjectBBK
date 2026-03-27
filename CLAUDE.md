# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

> **Claude Code에게:** 작업 중 아래 상황이 발생하면 해당 섹션에 추가할 것을 **반드시 제안**할 것.
> - 예상치 못한 GAS/UE 동작, 함수 호출 오류, 삽질 끝에 해결한 문제 → **Debugging Checklist** 추가 제안
> - 새로운 GA/GE/UI 구현 패턴이 확립된 경우 → **Common Patterns** 추가 제안

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
├── AC_BearMonster       // Melee: normal attack + jump slam special
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
- Damage: `GE_BasicDamage`와 Set by Caller(`Data.Damage` 태그) 사용 — 하드코딩 절대 금지
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
| Skill Wheel 추가 스킬 x2~4 | F (Skill Wheel) | 📋 계획 중 | Skill Wheel 구현 전 선행 작업 |
| Skill Wheel (F키 슬롯 교체) | F | 📋 계획 중 | GA_SpeedBuff 포함 3~5개 스킬 순환 |

### Monster Abilities
| Ability | 상태 | 비고 |
|---------|------|------|
| BPC_MeleeMonsterNormalAttackGA | ✅ 완료 | AC_BearMonster 사용 |
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
| GE_Ablaze | ✅ 완료 | 상태이상: 화염 |
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