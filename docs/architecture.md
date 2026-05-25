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
