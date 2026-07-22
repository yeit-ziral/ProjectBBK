## Known Issues

> 알고 있지만 미해결인 버그를 기록. 해결 시 항목 삭제.

| 증상 | 재현 방법 | 담당 | 비고 |
|------|-----------|------|------|
| 캐릭터 로스터에서 `WBP_HUD`가 캐릭터 수만큼 중복 생성 → 공유 `Inventory` 델리게이트 중복 브로드캐스트 | 재고 0인 등록된 퀵슬롯 사용 시도 → 알림 사운드가 캐릭터 수만큼 겹쳐 재생(로그로 확인, 청감상 거의 안 들림) | 기용 | PlayerController에 HUD 캐시 가드 추가해 최초 1개만 생성하도록 BP 수정 예정 (미적용) |

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
34. **SizeBox Override 체크 후 값 미입력 시 내부 위젯 클릭 불가** — Height Override 또는 Width Override에 체크만 하고 수치를 입력하지 않으면 해당 축 크기가 0이 되어 hit-test 영역이 0으로 설정됨 → 내부 슬라이더·버튼 클릭 불가. Override 체크 시 반드시 수치 입력 확인. 슬라이더 Height는 SizeBox 대신 Slider의 `Bar Thickness` 속성으로 조절할 것. UMG 위젯 클릭 안 될 때는 Widget Reflector (Tools → Debug → Widget Reflector 또는 `Ctrl+Shift+W`) → **Pick Hit-Testable Widgets**로 실제 히트 대상 위젯 확인.
28. **`QuerySkillCooldown`의 `OutDuration`이 Remaining과 같은 값으로 반환될 때** — `GetActiveEffectsTimeRemainingAndDuration`의 `Value`(Duration)가 SetByCaller Duration GE에서 올바르지 않은 값을 반환하는 경우가 있음. `OutDuration`을 GE에서 읽지 말고 `CachedSkillData.cooldown`에서 직접 가져올 것.
29. **커스텀 GameMode에서 PlayerStart 위치에 구형 오브젝트(`DefaultPawn0`)가 스폰될 때** — `AGameModeBase`의 기본 `DefaultPawnClass`가 `ADefaultPawn`(구형 메시 + 콜리전)이기 때문. 커스텀 PlayerController가 캐릭터를 직접 스폰하는 구조에서는 GameMode 생성자에서 반드시 `DefaultPawnClass = nullptr` 설정.
30. **MoviePlayer 로딩 화면에서 UMG 위젯 레이아웃이 표시 안 될 때** — `GetMoviePlayer()->SetupLoadingScreen()`에 `UUserWidget::TakeWidget()`으로 변환한 Slate를 전달해도 UMG Designer 레이아웃이 렌더링되지 않음. Event Construct는 실행되지만 화면에 보이지 않음. → `GEngine->GameViewport->AddViewportWidgetContent()`로 교체. UMG 위젯은 `GameInstance`를 outer로 `CreateWidget` 하여 레벨 전환 중 GC 방지.
31. **레벨 전환 후 새 레벨에서 캐릭터를 조종할 수 없을 때** — `ShowEndingScreen()`이 `SetInputMode(FInputModeUIOnly)`를 설정한 상태에서 레벨이 재로드되면 새 PlayerController가 입력 모드를 초기화하지 않아 발생. `PlayerController::BeginPlay()` 시작부에 `SetInputMode(FInputModeGameOnly())` + `SetShowMouseCursor(false)` 명시 필요 (HasAuthority 체크 이전).
32. **PIE에서 Actor 스폰 시 매번 1~2초 프리징이 발생할 때** — PIE에서만 발생하고 Standalone Game에서는 미발생이면 실제 성능 문제가 아님. Blueprint Debugger 추적, Output Log 동기 갱신, World Outliner UI 업데이트 등 에디터 오버헤드가 원인. 코드 수정 불필요, 출시 빌드에서는 정상.
33. **캐릭터 로스터 시스템에서 `BeginPlay()`에 적용한 GE가 동작 안 할 때** — 캐릭터들이 Possess 전에 미리 스폰되는 구조에서는 `BeginPlay()` 시점에 `abilitySystemComponent.IsValid() == false`. ASC는 `PossessedBy()` → `InitializeStartingValues()`에서야 연결됨. Infinite/Persistent GE는 `BeginPlay()` 대신 `AddStartupEffects()` 내부에서 적용할 것 (`startupEffectsApplied` 플래그가 중복 적용도 방지).
35. **`ActorComponent`의 `BlueprintImplementableEvent`가 캐릭터 Blueprint 이벤트 목록에 없을 때** — `UActorComponent` 서브클래스에 선언한 `BlueprintImplementableEvent`는 컴포넌트를 Blueprint로 서브클래싱해야만 override 가능. 캐릭터 Blueprint의 Event Override 목록에 나타나지 않음 → 이벤트를 소유 Actor 클래스에 선언하고 컴포넌트에서 `Cast<ActorType>(GetOwner())->EventName()` 으로 호출. Actor Blueprint에서는 정상 override 가능.
36. **`DECLARE_DYNAMIC_MULTICAST_DELEGATE`(파라미터 없음) Blueprint 바인딩 시 Signature Error** — "Signature Error: The selected function/event is not bindable - is the function/event deprecated, pure or latent?" 에러 발생. `Bind Event to X` 노드에서 파라미터 없는 delegate 바인딩 불가 → 파라미터를 하나 이상 추가하거나 `BlueprintImplementableEvent` 방식으로 교체.
37. **인벤토리 슬롯 아이템을 외부 위젯(퀵슬롯 등)으로 드래그&드롭해 "참조만" 등록할 때 원본 인벤토리 슬롯 아이콘이 사라짐** — `UC_InventorySlotWidget::NativeOnDragDetected`가 드래그 시작 시 아이콘/수량을 숨기는데, 기존엔 이게 `OnInventoryChanged` 브로드캐스트(그리드 재생성) 또는 `NativeOnDragCancelled`(드롭 실패)에서만 복구됨. 드롭 대상이 인벤토리 데이터를 건드리지 않고 성공 처리(true 반환)하면 두 복구 경로 모두 안 타서 아이콘이 계속 숨겨진 채로 남음. → `RestoreDisplay()`를 공개 함수로 분리해, 인벤토리 데이터를 바꾸지 않는 외부 드롭 핸들러(`UC_UseItemSlotWidget::NativeOnDrop` 등)에서도 명시적으로 호출해야 함.
38. **`UWidget` 파생 클래스에서 지역 변수명을 `Visibility`로 지으면 빌드 실패(C4458)** — `UWidget::Visibility` 멤버를 가려서 발생 (프로젝트가 경고를 에러로 처리). Slate 관련 지역 변수명은 `NewVisibility` 등으로 구분할 것.
39. **`GetName()`으로 두 `UObject` 인스턴스가 "같은 객체"인지 판별하면 안 됨** — `GetName()`은 같은 Outer 안에서만 유일성이 보장됨. 서로 다른 부모(Outer, 예: 서로 다른 `WBP_HUD` 인스턴스)를 가진 위젯은 디자인 타임 이름이 같아도 실제로는 다른 인스턴스일 수 있음. 진짜 식별에는 `GetPathName()`(Outer 전체 경로 포함) 사용.
40. **캐릭터 로스터 시스템에서 `WBP_HUD`가 캐릭터 수만큼 중복 생성되면 `PlayerController` 소유 공유 컴포넌트(`UC_InventoryComponent` 등)의 델리게이트가 중복 브로드캐스트됨** — 스킬 아이콘/게이지처럼 캐릭터별 ASC 인스턴스에 바인딩되는 경우는 안 겹치지만, PlayerController 레벨 공유 상태에 바인딩된 위젯은 살아있는 HUD 인스턴스 수만큼 중복 반응(사운드 중복 재생 등). `GetPathName()`으로 인스턴스 비교해 확인 가능.
