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

### 자폭 몬스터 AI — Tick 상태머신 vs BehaviorTree
- **선택:** `AC_BombMonster`의 Tick에서 배회 ↔ 돌진을 직접 전환 (`behaviorTree` 미할당, BT/BB 에셋 없음)
- **대안:** `BB_Monster` + `BT_Monster_Bomb` 신규 생성 후 Selector + Patrol/Charge Task 2개 + `UC_MonsterBTService` 배치
- **선택 이유:**
  - 상태 전환 조건이 `targetActor.IsValid()` 하나뿐 — Selector로 고를 분기가 사실상 없음
  - **비행 몬스터라 네비메시를 못 씀** → BT를 써도 `MoveTo`가 아니라 이동 수학을 직접 짜야 함.
    실제로 기존 `UC_BTTaskReposition::TickTask`도 `MoveTo` 없이 벡터 계산 + `AddMovementInput`만 함 —
    같은 코드가 BT Task에 있느냐 Actor Tick에 있느냐의 차이뿐
  - `BehaviorTree`/`BlackboardData` 내부(`root_node`, `blackboard_asset`)가 Python에 노출되지 않아
    MCP로 트리 배선 불가 → 수동 에디터 작업 필요
  - `AC_MonsterAIController::OnPossess`가 BT null이면 스킵하므로 미할당 상태로 안전하게 동작
- **트레이드오프:** 팀의 다른 몬스터 4종과 구조가 달라 BT를 찾다가 헤맬 수 있고, **BT 디버거로 상태 시각화 불가**.
  전환 시 비용은 낮게 유지 — `TickPatrol()` / `TickCharge()`가 분리돼 있어 Task에서 호출만 하면 되고,
  타겟 탐색은 `UC_MonsterBTService`로 대체 가능(`UpdateTarget()` 폐기).
  행동이 3개 이상으로 늘거나 실행 중 인터럽트(Observer Abort)가 필요해지면 그때 전환할 것

### 몬스터 Reposition/Strafe — 단일 데이터 구동 Task vs Idle/Strafe 분리 Task
- **선택:** `UC_BTTaskReposition` 하나로 노말 "Idle Repositioning"과 보스 "Strafe 서클링"을 통합
- **대안:** `UC_BTTaskIdleReposition`(노말 전용) + `UC_BTTaskBossStrafe`(보스 전용) 분리
- **선택 이유:** 두 동작의 알고리즘(목표 반경 유지 + 접선 이동)이 동일함. 시각 차이는 `StrafeWeight`(노말 ~0.4, 보스 ~0.9)·`RepositionSpeed`·`FlipInterval`·`DesiredRange` 등 데이터값만으로 표현 가능. CLAUDE.md 데이터 구동 원칙 준수, 이동 수학 중복 제거. 세 BT 에셋 모두 동일 Task로 배치 가능
- **트레이드오프:** 보스 전용 로직(예: 2페이즈 서클링 반경 조정)이 필요해지면 서브클래스로 분리하거나 FMonsterData에 추가 필드를 넣어야 함

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

### 레벨 전환 로딩 화면 — Viewport Overlay vs MoviePlayer
- **선택:** `GEngine->GameViewport->AddViewportWidgetContent()` + GameInstance를 outer로 UMG 위젯 생성
- **대안:** `GetMoviePlayer()->SetupLoadingScreen()` + `TakeWidget()`
- **선택 이유:** MoviePlayer는 Slate 렌더 파이프라인만 지원하여 UMG Designer 레이아웃을 렌더링하지 못함. Event Construct는 실행되지만 화면에 아무것도 표시되지 않음. GameViewport는 레벨 전환 중에도 살아있어 오버레이가 확실히 표시됨.
- **트레이드오프:** `LoadComplete()`에서 명시적으로 `HideLoadingOverlay()` 호출 필요. MinLoadingTime은 타이머로 직접 구현.

### 레벨 간 상태 저장 위치 — GameInstance vs PlayerState (Seamless Travel)
- **선택:** `UC_BBKGameInstance`에 `FPersistentGameState` 저장, Non-seamless travel 사용
- **대안:** Seamless Travel로 PlayerState 유지 (멀티플레이어 표준 방식)
- **선택 이유:** 이 프로젝트는 싱글플레이어이며 `OpenLevel`(Non-seamless) 사용. PlayerState는 레벨 전환 시 재생성되므로 GameInstance가 유일한 영속 저장소. Seamless Travel은 멀티플레이어 인프라를 요구해 범위 초과.
- **트레이드오프:** 비활성 캐릭터 어트리뷰트는 새 레벨에서 Possess 전까지 초기화 상태. `InjectPreSavedState`로 `savedState`를 미리 주입해 Possess 시 `RestoreCharacterState`가 자동 적용되도록 처리.

### 레벨 시작 캐릭터 인덱스 — 항상 0 vs 저장 인덱스
- **선택:** `GI->HasSavedState() ? GetSavedActiveCharacterIndex() : 0`으로 동적 결정
- **대안:** 항상 0번 캐릭터로 시작
- **선택 이유:** 레벨 이동 중에도 플레이어가 선택한 캐릭터가 유지되어야 자연스러운 게임 흐름. 저장 상태 없는 최초 시작 시 0번으로 폴백.
- **트레이드오프:** `FPersistentGameState.activeCharacterIndex`를 `SaveGameState` 시 반드시 저장해야 함. `RestoreGameState`의 `ActiveIndex` 파라미터도 이 값으로 전달해야 어트리뷰트 복원 대상이 올바르게 설정됨.

### Game Over UI 상속 구조 — UC_EndingScreenWidget 상속 vs 독립 클래스
- **선택:** `UC_GameOverWidget : public UC_EndingScreenWidget` — 빈 서브클래스로 시작
- **대안:** `UUserWidget`을 직접 상속한 독립 클래스
- **선택 이유:** "메인 메뉴로", "게임 종료" 버튼 로직(`OnReturnToMainMenu`, `OnQuitGame`)을 그대로 재사용. 향후 Retry 버튼은 서브클래스에만 추가하면 됨
- **트레이드오프:** Ending(클리어)과 GameOver(실패)가 의미상 다른 화면이지만, 현재 버튼 동작이 동일하므로 분리 실익이 없음. Retry 기능 추가 시 서브클래스 확장으로 처리

### Game Over 트리거 위치 — PlayerController vs GameMode
- **선택:** `AC_PlayerController::HandleCharacterDeath()`에서 `NextLivingIndex == -1` 조건으로 직접 `ShowGameOverScreen()` 호출
- **대안:** GameMode에서 플레이어 사망을 구독해 별도 처리
- **선택 이유:** `HandleCharacterDeath`가 이미 "다음 생존 캐릭터 없음" 조건을 감지하고 있어 중복 로직 불필요. UI 표시도 PlayerController 책임 범위 안에 있음
- **트레이드오프:** 멀티플레이어로 확장 시 서버-클라이언트 분리 필요. 싱글플레이어 전제이므로 현재 범위에서는 문제 없음

### OnReturnToMainMenu — StartGame() 재사용 vs TravelToMainMenu() 분리
- **선택:** `TravelToMainMenu()` 신규 추가 — `MainMenuLevel` TSoftObjectPtr을 직접 `OpenLevelBySoftObjectPtr`
- **대안:** `StartGame()` 재사용 — `Levels[0]`(첫 번째 게임 레벨)을 열어 메인 메뉴처럼 활용
- **선택 이유:** 메인 메뉴 레벨은 `UDA_LevelSequence` 배열 밖에 있어야 함. `StartGame()`은 로딩 오버레이 + `bIsTransitioning` 가드가 달린 게임 시작 전용 함수이므로 복귀 경로에 재사용하면 책임이 뒤섞임
- **트레이드오프:** `BP_GameInstance`에 `MainMenuLevel` 슬롯 할당이 추가로 필요. 미할당 시 `TravelToMainMenu()`가 무동작으로 실패

### TravelToMainMenu 로딩 오버레이 — 생략 vs Slate 폴백
- **선택:** 로딩 오버레이 없이 바로 `OpenLevelBySoftObjectPtr`
- **대안:** `FLevelEntry` 없이 단순 검정 Slate를 직접 생성해 표시
- **선택 이유:** 메인 메뉴 레벨은 빈 레벨(Empty Level)이므로 로딩이 체감되지 않음. `ShowLoadingOverlay`는 `FLevelEntry`(텍스트·최소 시간·텍스처)를 요구하는데 메인 메뉴는 해당 데이터가 없음
- **트레이드오프:** 메인 메뉴 레벨이 무거워질 경우 별도 Slate 폴백 추가 필요

### AC_ConsumableItem / AC_EquipmentItem 유지 결정
- **선택:** 두 C++ 클래스 현행 유지
- **대안:** 삭제 후 BP_ConsumableItem · BP_EquipItem을 AC_BaseItem 직속 상속으로 전환, `InitItem`을 `BlueprintNativeEvent`로 변경해 BP에서 DT 조회 구현
- **선택 이유:** 삭제 시 `InitItem` BlueprintNativeEvent 전환 + 두 BP Reparent + DT 변수 재추가 + InitItem 이벤트 구현이 필요. 현재 이득(파일 4개 제거) 대비 비용이 큼
- **트레이드오프:** `InitItem`을 `BlueprintNativeEvent`로 전환할 시점에 함께 정리 권장

### 볼륨 저장 위치 — UGameUserSettings vs USaveGame
- **선택:** `UC_BBKGameUserSettings` (UGameUserSettings 서브클래스), `UPROPERTY(Config)`로 `GameUserSettings.ini`에 자동 직렬화
- **대안:** `USaveGame`으로 세이브 슬롯에 저장
- **선택 이유:** 볼륨처럼 게임 진행과 무관한 유저 환경 설정은 SaveGame(세이브 슬롯)보다 UserSettings가 의미적으로 적합. `SaveSettings()` 한 줄로 저장, `GetGameUserSettings()`로 어디서든 접근 가능
- **트레이드오프:** `DefaultEngine.ini`에 `GameUserSettingsClassName` 등록 필요. 기존 `UGameUserSettings::Get()` 호출부를 서브클래스 `UC_BBKGameUserSettings::Get()`으로 교체해야 함

### SkillWheel 닫기 이벤트 — Component BlueprintAssignable vs Actor BlueprintImplementableEvent
- **선택:** `AC_BasePlayerCharactor`에 `OnSkillWheelShouldClose` `BlueprintImplementableEvent` 선언, `UC_SkillManagerComponent::CloseSkillWheel()`에서 `Cast<AC_BasePlayerCharactor>(GetOwner())->OnSkillWheelShouldClose()` 호출
- **대안 1:** `UC_SkillManagerComponent`에 `DECLARE_DYNAMIC_MULTICAST_DELEGATE`(파라미터 없음) 델리게이트 추가 → Blueprint `Bind Event` 노드로 바인딩
- **대안 2:** `UC_SkillManagerComponent`에 `BlueprintImplementableEvent` 선언 → 컴포넌트 Blueprint 서브클래싱으로 override
- **선택 이유:** 파라미터 없는 `DECLARE_DYNAMIC_MULTICAST_DELEGATE`는 Blueprint `Bind Event` 노드에서 Signature Error 발생(Debugging #36). 컴포넌트 `BlueprintImplementableEvent`는 캐릭터 Blueprint의 Override 목록에 나타나지 않음(Debugging #35). Actor의 `BlueprintImplementableEvent`는 캐릭터 Blueprint에서 바로 override 가능하고 `OnASCInitialized` 기존 패턴과 일치.
- **트레이드오프:** 위젯 정리 로직이 캐릭터 Blueprint에 위치해 SkillWheel 관련 로직이 `UC_SkillManagerComponent`와 캐릭터 Blueprint 두 곳에 분산됨.

### 장비 스탯 적용 — 공용 GE + SetByCaller vs 장비별 개별 GE
- **선택:** `GE_EquipBonus` 1개에 Modifier 5개 (SetByCaller), DT에서 수치만 변경
- **대안:** 장비마다 개별 GE 에셋 생성 (Modifier 값 하드코딩)
- **선택 이유:** 장비 추가 시 GE 에셋 생성 불필요. DT Row 추가만으로 신규 장비 등록 가능. 프로젝트의 기존 SetByCaller 패턴(GE_BasicDamage 등)과 일치
- **트레이드오프:** 미적용 스탯에도 0 값을 넣어야 함. 장비별로 다른 Duration 정책(예: 일시 버프)이 필요하면 별도 GE 필요

### 아이템 픽업 — 상호작용 키 입력 vs 즉시 Overlap 픽업
- **선택:** Overlap 시 상호작용 UI 표시, 키 입력(E) 시 획득 처리
- **대안:** C_ExpOrb처럼 Overlap 즉시 획득
- **선택 이유:** 인벤토리 시스템 추가 시 아이템 정보 확인 후 선택적 획득이 필요. 상호작용 UI에 아이템 이름을 표시해 플레이어에게 정보 제공
- **트레이드오프:** Tick에서 키 입력을 폴링 (Overlap 중에만 Tick 활성화로 성능 영향 최소화)

### 아이템 데이터 구조 — FBaseItemData 상속 vs 별도 구조체
- **선택:** `FConsumableItemData : FBaseItemData`, `FEquipmentItemData : FBaseItemData` 상속 구조
- **대안:** 각각 독립 구조체로 공통 필드 중복 정의
- **선택 이유:** itemID, itemName, description, itemIcon, worldMesh 등 공통 필드를 한 곳에서 관리. DataTable Row 구조체 상속은 UE5에서 지원됨
- **트레이드오프:** 소비/장비 DT가 각각 별도 DataTable이므로 통합 아이템 조회 시 두 테이블을 모두 검색해야 함

### 상호작용 대상 관리 — overlappingItems 배열 vs 단일 포인터
- **선택:** `TArray<TWeakObjectPtr<AC_BaseItem>> overlappingItems` 배열로 Overlap 중인 아이템 전체 관리
- **대안:** 단일 `TWeakObjectPtr<AC_BaseItem>` 포인터
- **선택 이유:** 단일 포인터는 A→B 순서로 Overlap 시 A를 덮어쓰고, B Destroy 후 A와 상호작용 불가. 배열은 Destroy 시 해당 항목만 제거되어 나머지 아이템과 자동 전환
- **트레이드오프:** 배열 관리 + RemoveAll 비용 (아이템 수가 적으므로 무시 가능)

### OnInteract 책임 분리 — 인벤토리 등록 vs 직접 효과 적용
- **선택:** BaseItem의 OnInteract는 인벤토리에 itemID 추가 + Destroy만 담당. 효과 적용(사용/장착)은 `UC_InventoryComponent`에서 처리
- **대안:** 서브클래스별 OnInteract에서 GE 직접 적용 후 Destroy
- **선택 이유:** 아이템 픽업과 사용을 분리하면 인벤토리 시스템과 자연스럽게 통합. 서브클래스의 OnInteract 중복 제거
- **트레이드오프:** 효과 적용 로직이 인벤토리 컴포넌트에 의존 (UseItem/EquipItem 구현 필요)

### 소비 아이템 GE 데이터 — FConsumableEffectEntry 구조체 vs 병렬 배열
- **선택:** `FConsumableEffectEntry` (effect, magnitudeTag, magnitude 묶음)의 `TArray`
- **대안:** `TArray<TSubclassOf<UGameplayEffect>>`, `TArray<FGameplayTag>`, `TArray<float>` 병렬 배열 3개
- **선택 이유:** 한 효과의 GE/태그/수치가 한 구조체에 묶여 인덱스 불일치 오류 원천 차단. 에디터에서 항목 단위로 추가/제거 가능
- **트레이드오프:** 구조체 하나 추가됨 (복잡도 미미)

### 드래그 가능 팝업창 루트 위젯 — Overlay vs Canvas Panel
- **선택:** `Overlay` (전체 뷰포트를 덮는 루트)
- **대안:** `Canvas Panel`
- **선택 이유:** Canvas Panel이 루트이면 `SetRenderTranslation`으로 창을 이동할 때 마우스 히트 영역도 같이 밀려 창 밖에서 마우스를 놓으면 `MouseButtonUp`을 못 받음. Overlay는 전체 뷰포트를 덮으므로 창이 어디로 이동해도 이벤트를 안정적으로 수신
- **트레이드오프:** Overlay 루트는 배경이 없으므로 창 본체(WindowRoot)에 배경 처리 필요. Canvas Panel은 자식을 절대 좌표로 배치해야 할 때 (장비 슬롯 등) 적합

### 드래그 창 WindowRoot — SizeBox vs Border
- **선택:** `SizeBox` (Width/Height Override로 크기 고정)
- **대안:** `Border`
- **선택 이유:** Border는 자식 텍스트 길이에 따라 크기가 바뀜 — 스탯 수치가 변경될 때마다 창 크기가 흔들림. SizeBox로 고정하면 수치 변화와 무관하게 창 크기 유지
- **트레이드오프:** SizeBox는 자식을 하나만 허용하므로 배경 Image와 콘텐츠를 겹치려면 내부에 Overlay 추가 필요. Width/Height Override 수치 미입력 시 크기 0 → 클릭 불가 (Debugging #34)

### SoundMix/SoundClass ref 위치 — GameInstance vs SettingsWidget
- **선택:** `UC_BBKGameInstance`에 `UPROPERTY(EditDefaultsOnly)` 4개로 보관
- **대안:** `UC_SettingsWidget`에 보관
- **선택 이유:** 레벨 로드 후 `LoadComplete()`에서 자동 볼륨 복원이 필요. SettingsWidget에 두면 설정창을 열지 않는 한 복원 불가. GameInstance는 항상 유효하므로 모든 시점에서 접근 가능
- **트레이드오프:** `BP_GameInstance`에 Audio 슬롯 4개(`GameSoundMix`, `SC_Master`, `SC_BGM`, `SC_SFX`) 할당 필요. 미할당 시 `ApplyVolumeSettings()`가 무동작으로 실패

### UseItem 구현 위치 — GA 신설 vs `UC_InventoryComponent` 직접 구현
- **선택:** `UC_InventoryComponent::UseItem`에서 GE 직접 적용, 새 GA 생성 안 함
- **대안:** `GA_UseItem` 신설 후 `TryActivateAbility`로 발동
- **선택 이유:** `AC_ExpOrb`(스폰형 픽업 아이템 패턴)와 동일한 선례 — 몽타주·애니메이션·입력 차단 등 GA 특유의 생명주기가 필요 없는 단발성 효과 적용은 GA 래퍼가 불필요한 오버헤드. CLAUDE.md 제약("UseItem은 인벤토리 컴포넌트에 둠")과도 일치
- **트레이드오프:** 향후 "아이템 사용 애니메이션 재생" 등 GA 특유 기능이 필요해지면 GA로 리팩토링 필요

### 퀵슬롯 상태 저장 위치 — 위젯 로컬 vs `UC_InventoryComponent`
- **선택:** `quickSlots`를 `UC_InventoryComponent`(PlayerController 소유)에 저장
- **대안:** `UC_UseItemSlotWidget` 로컬 변수로 등록 아이템 보관
- **선택 이유:** 위젯 인스턴스는 HUD 재생성 등으로 파괴될 수 있지만 인벤토리 컴포넌트는 캐릭터 교체와 무관하게 PlayerController에 상주 — 등록 상태가 위젯 생명주기에 종속되지 않음
- **트레이드오프:** 없음 (기존 `OnInventoryChanged`/`OnMoneyChanged`와 동일한 위치 원칙)

### 캐릭터 교체 시 장비 보너스 격리 — Suspend/Reapply vs GE 태그 필터링
- **선택:** `UC_EquipmentComponent::SuspendEquipBonuses()`/`ReapplyEquipBonuses()` — 교체 시 GE를 직접 제거/재적용
- **대안:** GE_EquipBonus에 캐릭터 식별 정보를 심고 AttributeSet의 PostAttributeChange 등에서 "현재 활성 캐릭터"와 불일치하면 무시하는 필터링
- **선택 이유:** ASC가 PlayerState 소유로 로스터 전체가 공유되는 기존 구조를 바꾸지 않고, 이미 존재하는 SaveActiveEffects/RestoreActiveEffects 패턴과 동일한 철학(교체 시점에 뗐다 다시 건다)으로 일관되게 처리 가능. 필터링 방식은 AttributeSet에 "현재 캐릭터가 누구인지" 판별 로직을 추가해야 해서 결합도가 높아짐
- **트레이드오프:** equipped 맵의 GE handle이 교체마다 새로 발급됨 — handle을 안정적 식별자로 쓰면 안 되고 itemID 기준으로 관리해야 함

### 퀵슬롯 재고 0 처리 — 자동 해제 vs 반투명 유지
- **선택:** 재고 0이 돼도 등록 유지, 아이콘만 반투명 처리(`RenderOpacity=0.35`) + 수량 텍스트 숨김
- **대안:** 재고 0 시 `UnregisterQuickSlot` 자동 호출
- **선택 이유:** 포션류를 다시 파밍/구매했을 때 재드래그 없이 바로 재사용 가능하게 하기 위함
- **트레이드오프:** 영구히 안 쓸 아이템이 슬롯을 계속 점유 — 수동 해제 UI(우클릭 등)는 이번 범위 밖

### HUD 중복 생성 방지 — PlayerController 캐시 가드 vs 방치 vs C++ 방어 코드
- **선택:** PlayerController에 `CachedHUD` 참조를 저장해 최초 1개만 생성, 이후 캐릭터는 생성 스킵 (BP 수정)
- **대안 1:** 방치 — 게임플레이(GE 적용)는 중복 안 되고 UI 반응만 겹침, 현재는 체감 안 됨
- **대안 2:** C++ 위젯에 "실제 표시 중인 HUD 소속인지" 체크하는 방어 코드 추가
- **선택 이유:** `UC_InventoryComponent`의 `OnQuickSlotChanged`/`OnQuickSlotUseFailed`가 PlayerController 레벨 공유 델리게이트라, 캐릭터 로스터 구조상 캐릭터별로 생성되는 `WBP_HUD` 인스턴스 수만큼 중복 바인딩됨(Debugging Checklist #40). 근본 원인(HUD 중복 생성)을 없애야 향후 비멱등적 기능이 이 델리게이트에 추가될 때 조용히 N배로 실행되는 잠재 버그를 막을 수 있음. 스킬 아이콘/게이지가 이미 쓰는 "단일 HUD 재초기화" 패턴과도 구조적으로 일치
- **트레이드오프:** 적용 완료. 캐릭터 `BeginPlay`가 Possess보다 먼저 실행되는 구조(Debugging Checklist #18)라 `Get Controller`가 None을 반환해 참조를 얻을 수 없었음 — `Get Player Controller`(Player Index 0)로 대체해 해결(Debugging Checklist #41 참고). `OnCharacterSwitched.Broadcast(0)` 기반 재초기화 흐름은 그대로 유지되며 HUD 자체 그래프는 수정하지 않음

### 몬스터별 자동공격 BT Task — 몬스터마다 Task 클래스 신설 vs 베이스 가상 함수 일반화
- **선택:** `AC_BaseMonster::TryAutoAttack()` 가상 함수 신설. `UC_BTTaskMeleeAutoAttack`이 `AC_BaseMonster`로만 Cast하고 `TryAutoAttack()`을 호출하도록 일반화 — 몬스터별 Task 클래스 불필요
- **대안:** 몬스터마다 전용 Task 클래스 (`UC_BTTaskShieldAutoAttack` 등)를 만들고 BT에서 해당 노드로 교체
- **선택 이유:** BT 에셋 내부(`root_node`·`Children`·`BlackboardKey`)가 Python에 노출되지 않아 **MCP로 노드 교체 불가** — 몬스터를 추가할 때마다 수동 에디터 작업이 강제됨. `BT_Monster_Shield`는 `BT_Monster_Melee` 복제본이라 이미 `C_BTTaskMeleeAutoAttack` 노드를 갖고 있고, Task를 일반화하면 **BT를 손대지 않고 그대로 동작**. `CanAutoAttack()` / `IsPlayingAttackAnimation()`이 이미 `AC_BaseMonster`의 가상 함수여서 `TryAutoAttack()` 하나만 추가하면 일관성이 맞음
- **트레이드오프:** 클래스명이 `UC_BTTaskMeleeAutoAttack`으로 남아 실제 역할(공통)과 어긋남 — 기존 BT 에셋이 클래스 경로를 참조하므로 이름을 바꾸면 노드가 끊어져서 표시명(`NodeName`)만 "Monster Auto Attack"으로 변경. 기존 `UC_BTTaskEliteAutoAttack` / `UC_BTTaskRangedAutoAttack`은 각 BT 에셋이 참조 중이라 그대로 두고, 신규 `UC_BTTaskShieldAutoAttack`만 참조가 없어 삭제
### 소비 아이템 복합 동작(AOE 판정·액터 스폰) — GA vs UC_ConsumableAction(UObject)
- **선택:** `UC_ConsumableAction`(UObject, Blueprintable) 베이스 클래스 신설. `FConsumableItemData.actionClass`로 DataTable에서 지정, `UC_InventoryComponent::UseItem()`이 `NewObject`로 생성해 `Execute(ASC, AvatarActor)` 호출
- **대안:** GA 신설 후 `TryActivateAbilityByClass`로 발동 (SkillWheel과 동일 방식)
- **선택 이유:** `UseQuickSlot()`이 ASC를 거치지 않는 직접 함수 호출 경로라 GA의 핵심 이점(ActivationBlockedTags, 비용/쿨다운 GE)이 애초에 적용되지 않음. 아이템은 이미 자체 타임스탬프 쿨다운(`IsItemOnCooldown`)이 있어 GA 쿨다운과 중복됨. `UC_ConsumableAction`은 GAS 오버헤드 없이 "클래스 참조로 동작을 갈아끼우는" 확장성만 재사용
- **트레이드오프:** 몽타주 재생·2단계 입력(조준→발사) 등 GA 고유 생명주기가 필요한 아이템은 이 구조로 못 만듦 — 필요해지면 서브클래스 내부에서 `TryActivateAbilityByClass`로 위임하는 "GA 브릿지" 액션 검토 (아직 미구현)

### AC_HealZone 회복 방식 — Instant GE + 존 자체 반복 타이머 vs Duration GE(GE_HealOverTime) 1회 적용
- **선택:** `GE_HealZoneTick`(Instant, Set by Caller `Data.Heal`) 신규 생성. `AC_HealZone`이 `OnBeginOverlap`에 반복 타이머를 시작해 매 tick마다 Instant GE를 적용하고, `OnEndOverlap`에 타이머를 멈추는 방식
- **대안:** 기존 `GE_HealOverTime`(Duration+Period)을 `AC_FireZone`처럼 진입 시 1회만 적용
- **선택 이유:** Duration GE를 1회 적용하면 장판을 스치기만 해도 전체 Duration 동안 회복이 지속돼 "장판 안에 있는 동안만 회복"이라는 의도와 어긋남. Instant GE + 존이 직접 관리하는 타이머는 실제 체류 시간과 회복이 정확히 일치함
- **트레이드오프:** `AC_FireZone`엔 없던 `OnEndOverlap` 처리와 타이머 시작/정지 로직이 추가로 필요해 구조가 조금 더 복잡함. Instant GE는 활성 핸들이 없으므로(Debugging Checklist #12) "제거"가 아니라 "타이머 정지"로 회복을 멈춤

### Foot IK 기준면 — Root 소켓 vs 캡슐 바닥
- **선택:** `ActorLocation.Z - GetScaledCapsuleHalfHeight()`(캡슐 바닥)를 기준면으로 사용
- **대안:** `Get Socket Location("Root")`의 Z (다수 튜토리얼의 기본 방식, `ABP_Melee` 초기 구현)
- **선택 이유:** `Get Socket Location`은 애니메이션이 적용된 현재 포즈의 본 위치를 반환하므로, Root 본을 움직이는 애니메이션(`Root Motion Mode = Root Motion from Montages`)이 재생되면 기준면이 매 프레임 흔들림. Idle 상태에서는 두 값이 정확히 일치해 문제가 드러나지 않다가 전투 중에만 발이 어긋나는 재현 어려운 버그가 됨. 캡슐 바닥은 물리적으로 정의된 값이라 포즈와 무관하게 항상 안정적이고, 캡슐 크기 상수를 하드코딩하지 않아도 됨
- **트레이드오프:** BP에서는 `Melee Character` 참조를 거쳐 두 노드(`Get Actor Location` / `Get Scaled Capsule Half Height`)를 조합해야 해서 Root 소켓 한 번 읽는 것보다 그래프가 조금 늘어남

### Foot IK 오차 보정 — Mesh Z 상수 이동 vs 수식 정정
- **선택:** Mesh Relative Z를 `-(Capsule Half Height)`인 정확한 값(-96)으로 되돌리고, FootTrace 수식의 하드코딩 상수를 제거해 근본 원인을 제거
- **대안:** 계단에서 뜨는 만큼 Mesh Relative Z를 눈대중으로 내려서 상쇄 (초기 대응)
- **선택 이유:** Mesh Z는 **모든 지형에 동일하게 적용되는 상수 평행이동**이라 지형 의존적 오차를 원리적으로 보정할 수 없음. 계단에 맞추면 평지에서 정확히 그만큼 발이 파묻히는 트레이드오프가 강제됨. 실제로 이번 문제의 원인은 ① Mesh Z가 -96이 아닌 -90/-100이었던 상수 오차와 ② FootTrace의 하드코딩 상수 두 가지였고, 둘 다 제거하자 계단·평지가 동시에 맞았음
- **트레이드오프:** 없음. 상수 보정은 두 지형 중 하나를 반드시 포기해야 하는 구조였음

### Blink 벽 통과 방지 — 사전 LineTrace vs SetActorLocation(sweep=true)
- **선택:** `LineTraceSingleByChannel`로 목표 지점까지 미리 검사 후 충돌 지점에서 캡슐 반경만큼 당긴 위치로 이동
- **대안:** `C_EliteMonsterSpecialAttackGA_Pull`처럼 `SetActorLocation(NewLoc, bSweep=true)`로 이동 자체를 스윕 처리
- **선택 이유:** Blink는 이동 거리가 크고(기본 600) 즉시 텔레포트라는 의도가 명확 — sweep은 목표 지점까지 콜리전을 따라 밀어내는 방식이라 "순간이동"의 즉시성과 다르게 느껴질 수 있고, 벽에 스치듯 닿았을 때 예측하기 어려운 최종 위치가 나올 수 있음. 사전 LineTrace는 "막히면 벽 앞에서 멈춘다"는 동작이 명확
- **트레이드오프:** LineTrace 한 번이 추가 비용. 아주 얇은 장애물(캡슐 반경보다 얇은 벽) 뒤로 착지하는 예외 케이스는 미검증

### WBP_Status 증가분 계산 — 소스별 명시적 추적 vs GAS 표준 BaseValue/CurrentValue 차이
- **선택:** 장비는 `UC_EquipmentComponent`가 아는 DT bonus 값을 합산, 포션은 `State.PotionBuff` 태그로 필터링한 활성 GE의 Modifier 평가치를 합산 — 두 소스를 독립 계산 후 합산
- **대안:** ASC 어트리뷰트의 `CurrentValue - BaseValue` 차이를 그대로 사용
- **선택 이유:** 이 프로젝트엔 `GE_SpeedBuff`(스킬)/`GE_SprintBuff`(Shift)/`GE_Slowed`(디버프) 등 동일 속성(moveSpeed/damage 등)을 건드리는 Duration/Infinite GE가 이미 다수 존재. 표준 차이값 방식은 이들을 전부 뭉뚱그려 표시해 "장비/포션 특유의 체감 안 되는 효과를 보여준다"는 원래 목적과 어긋남. `State.PotionBuff` 태그 필터링으로 포션발 증가분만 정확히 분리
- **트레이드오프:** `State.PotionBuff` 태그를 포션 전용으로 계속 유지해야 함(다른 GE가 재사용하면 계산이 오염됨). 새 버프형 소모품을 추가할 때마다 이 태그를 명시적으로 부여해야 위젯에 자동 반영됨
