# CLAUDE.md

This file provides guidance to Claude Code when working with code in this repository.

> **Claude Code에게:** `/패턴정리` 명령어 입력 시 현재 세션에서 작업한 내용을 검토하고 @docs/debugging.md 의 Debugging Checklist / @docs/patterns.md 의 Common Patterns / @docs/decisions.md 의 Design Decisions에 추가할 항목을 제안할 것. 또한 @docs/status.md 의 Current Development Status 섹션 각 테이블에서 상태 변경이 필요한 항목도 함께 제안할 것.

---

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

**Build:**
Open `ProjectBBK.sln` → `Development Editor` / `Win64`

**Required VS components:** `.vsconfig` 참고
- VC++ 14.38 toolchain / Windows 11 SDK 22621 / LLVM·Clang / Unreal Engine IDE tools

**Module dependencies** (`Source/ProjectBBK/ProjectBBK.Build.cs`):
Core, CoreUObject, Engine, InputCore, EnhancedInput, GameplayAbilities, GameplayTags, GameplayTasks, UMG, Slate, SlateCore, Niagara, AIModule

**Default startup map:** `/Game/Maps/Untitled` | **Default GameMode:** `BP_GameMode`

---

## Key Config Files

| File | Purpose |
|------|---------|
| `Config/DefaultGameplayTags.ini` | All gameplay tag definitions |
| `Config/DefaultEngine.ini` | DirectX 12, ray tracing, custom AssetManager |
| `Config/DefaultGame.ini` | Default GameMode |
| `ProjectBBK.uproject` | Plugin list, custom AssetManager |

**Gameplay Tags** — Key namespaces: `Ability.*`, `State.*`, `Skill.*`, `Input.*`, `Event.*`

---

## Naming Conventions

- **Classes / Functions / Parameters:** PascalCase
- **Variables:** camelCase
- **Constants:** UPPER_SNAKE_CASE

---

## Git 협업 규칙

- 각자 개인 브랜치에서 작업 → 완료 시 `master`에 merge
- 타 팀원이 merge한 경우 `master`를 본인 브랜치로 merge해 최신화
- `.uasset` 작업 시작/종료 시 Discord 공유 — 동시 편집 금지
- 충돌 시 **가장 최근 작업자 파일로 덮어쓰기**

---

## Important Constraints

### Blueprint & Editor
- `.uasset` / `.umap` 직접 편집 불가 — C++ 변경 시 리컴파일 필요한 BP 명시

### GAS 작업 규칙
- **데미지:** 무조건 전용 GE 담당 — GA 직접 수치 적용 절대 금지
  - 즉발 → `GE_BasicDamage` (Set by Caller, `Data.Damage`)
  - DoT → `GE_DotDamage` (Set by Caller, `Data.Damage`)
  - 새 유형 → 전용 GE 신규 생성
- **Cooldown:** `GE_GenericCooldown` 공유 — GA별 개별 GE 금지
- **ASC 참조:** 반드시 `PlayerState`에서 취득 — `Character` 직접 참조 금지
- **Overlap Ignore:** `GetAvatarActorFromActorInfo()` 사용 — `GetOwner()` 금지
- **몬스터 스탯:** `FMonsterData` DataTable 기반 — C++ 하드코딩 금지

---

## 상세 문서

| 문서 | 내용 |
|------|------|
| @docs/architecture.md | GAS 계층, 클래스 구조, AI, UI, Skill System |
| @docs/status.md | Current Development Status (Player Abilities / Monster / UI / Effects) |
| @docs/patterns.md | Common Patterns (구현 레퍼런스) |
| @docs/debugging.md | Known Issues + Debugging Checklist |
| @docs/decisions.md | Design Decisions (설계 결정 기록) |
