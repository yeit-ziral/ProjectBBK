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

### 몬스터 Reposition/Strafe BT Task 패턴 (UC_BTTaskReposition 참고)
공격 쿨다운 대기 중 플레이어 주위를 이동하는 latent BT Task. 노말(Idle Reposition)과 보스(Strafe 서클링)를 단일 Task + DataTable 튜닝으로 표현.
```
// BT 구조 (3종 공통)
Selector (루트)
├── 공격 브랜치 (고우선): 기존 공격 Task
│     └── Decorator: 거리·쿨다운 조건, Observer aborts = Both (Lower Priority 이상)
└── UC_BTTaskReposition (저우선 fallback)
      → TargetActorKey → "TargetActor" 바인딩

// 이동 수학 (TickTask 매 프레임)
radial  = (MonsterLoc - TargetLoc).GetSafeNormal2D()           // 플레이어에서 멀어지는 방향
tangent = Cross(UpVector, radial) * StrafeSign                   // 접선(서클링) 방향
radialComp:
  dist > DesiredRange + Band → -radial (접근)
  dist < MinRange            → +radial (후퇴/kite)
  그 외                      → ZeroVector (사거리 내 대기)
dir = radialComp*(1-StrafeWeight) + tangent*StrafeWeight → Normalize → AddMovementInput

// 방향 전환
if (now >= NextFlipTime) { StrafeSign = -StrafeSign; NextFlipTime += FlipInterval; }
```
- 공격 Task가 쿨다운 시 Succeeded 반환하면 Selector가 fallback으로 넘어가지 않음 → 노말 공격 Task에 `if (!CanAutoAttack()) return Failed` 추가 (보스 패턴과 동일)
- Ranged/Boss는 생성자에서 `bUseControllerRotationYaw=true, bOrientRotationToMovement=false` 설정 → 스트레이프 중 플레이어 정면 유지, `Dir` 2D 블렌드스페이스 정상 동작
- OnTaskFinished: `MaxWalkSpeed = monsterAttributeSet->GetmoveSpeed()` 복원 (GAS 어트리뷰트 기반 쫓기 속도)
- DataTable 권장 초기값: 근거리(Weight 0.4, MinRange 0), 원거리(Weight 0.5, MinRange 350), 보스(Weight 0.9, MinRange 300)

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

### 캐릭터 사망 시 SkillWheel 강제 종료 패턴
GA 차단 태그 제거 + 위젯 정리를 사망 경로에서도 동일하게 보장하는 구조.
```
// AC_PlayerController::HandleCharacterDeath
if (UC_SkillManagerComponent* SM = DeadCharacter->FindComponentByClass<UC_SkillManagerComponent>())
    SM->CloseSkillWheel();

// UC_SkillManagerComponent::CloseSkillWheel() 말미
if (AC_BasePlayerCharactor* Char = Cast<AC_BasePlayerCharactor>(GetOwner()))
    Char->OnSkillWheelShouldClose();

// 캐릭터 Blueprint — OnSkillWheelShouldClose override
→ SkillWheelWidget → Remove from Parent
→ SetShowMouseCursor(false) + ResetIgnoreLookInput + Set Input Mode Game Only
```
- `bIsSkillWheelOpen` 가드가 있으므로 SkillWheel이 이미 닫혀있으면 무동작 — 사망 시 무조건 호출해도 안전
- Z키 닫기·캐릭터 사망 양쪽 경로에서 동일한 정리 코드 실행 보장
- `OnSkillWheelShouldClose`는 `AC_BasePlayerCharactor`의 `BlueprintImplementableEvent` — 컴포넌트 `BlueprintImplementableEvent`/`BlueprintAssignable` 방식의 Blueprint 연동 문제를 우회 (Debugging Checklist 35·36번 참고)

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

### 상호작용형 픽업 아이템 패턴 (C_BaseItem / C_ConsumableItem / C_EquipmentItem 참고)
Overlap → 상호작용 UI 표시 → EnhancedInput(IA_Interact) → 인벤토리 등록 + Destroy 구조.
효과 적용(사용/장착)은 `UC_InventoryComponent`에서 담당.
```
AC_BaseItem (AActor, Abstract)
  생성자
    → CollisionSphere: QueryOnly, 모두 Ignore, ECC_Pawn만 Overlap
    → StaticMeshComponent: NoCollision
    → WidgetComponent: Screen Space, 기본 숨김

  BeginPlay
    → Overlap 바인딩
    → itemID != NAME_None → InitItem(itemID)  // 에디터 배치 시 자동 초기화

  OnItemBeginOverlap
    → Cast<AC_BasePlayerCharactor> → PlayerController 취득
    → PC->SetCurrentInteractable(this)   // overlappingItems 배열에 추가
    → InteractionWidget에 cachedItemName 설정
    → WidgetComponent 표시

  OnItemEndOverlap
    → PC->ClearCurrentInteractable(this)   // 배열에서 제거
    → WidgetComponent 숨김

  OnInteract(Player)   ← PlayerController의 IA_Interact에서 호출
    → PC->GetInventory()->AddItem(itemID)
    → 성공(잔여 0) 시 Destroy()

AC_PlayerController (상호작용 관리)
  overlappingItems: TArray<TWeakObjectPtr<AC_BaseItem>>
  IA_Interact (ETriggerEvent::Started) → OnInteractInput
    → 무효 항목 정리 후 배열 마지막(가장 최근 진입) 아이템의 OnInteract 호출
    → Destroy 시 EndOverlap → 배열에서 제거 → 나머지 아이템과 자동 전환

AC_ConsumableItem / AC_EquipmentItem
  → InitItem만 override: DT에서 itemName + worldMesh 로드
  → OnInteract 미override — 베이스가 인벤토리 추가 처리
```
- `itemID`는 `EditAnywhere` — 에디터에서 인스턴스마다 다른 아이템 지정 가능
- 월드 스폰은 `BP_ConsumableItem` / `BP_EquipItem`만 사용

### 장비 아이템 공용 GE + SetByCaller 패턴 (GE_EquipBonus 참고)
장비마다 GE를 만들지 않고 `GE_EquipBonus` 하나로 모든 장비를 처리.
장착/해제 로직은 `UC_InventoryComponent`에서 담당.
```
GE_EquipBonus (Duration: Infinite, Modifier 5개)
  → maxHealth:  SetByCaller Tag = Data.Equip.MaxHealth
  → maxStamina: SetByCaller Tag = Data.Equip.MaxStamina
  → moveSpeed:  SetByCaller Tag = Data.Equip.MoveSpeed
  → damage:     SetByCaller Tag = Data.Equip.Damage
  → defense:    SetByCaller Tag = Data.Equip.Defense
```

### 돈(골드) 픽업 아이템 패턴 (C_MoneyItem 참고)
GA 없이 AC_BaseItem 상속으로 Overlap → 상호작용 → AddMoney → Destroy하는 구조. itemID·DataTable 불필요.
```
AC_MoneyItem (AC_BaseItem 상속)
  BeginPlay
    → cachedItemName = FText::Format("{0} gold", moneyAmount)   ← 포맷 후 Super 호출
    → Super::BeginPlay()
    → ApplyWorldMesh(nullptr)   ← itemID 없어도 defaultMesh 적용

  OnInteract(Player)
    → PC->GetInventory()->AddMoney(moneyAmount)
    → Destroy()

BP_Money
  → C_MoneyItem 상속
  → defaultMesh: 코인 메시 할당
  → moneyAmount: 인스턴스별 EditAnywhere 설정
```
- `cachedItemName`은 `protected`이므로 서브클래스에서 직접 포맷 가능
- OnItemBeginOverlap이 `private UFUNCTION()`이므로 override 불가 → BeginPlay에서 cachedItemName을 미리 포맷하면 위젯 텍스트가 자동 반영됨

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

### 전체 화면 UI 표시 패턴 (ShowEndingScreen / ShowGameOverScreen 참고)
PlayerController를 outer로 위젯을 생성해 뷰포트에 올리고 UI 전용 입력 모드로 전환.
```
CreateWidget<T>(this, WidgetClass)   // this = PlayerController
→ AddToViewport(10)                  // ZOrder 10: HUD 위에 표시
→ SetInputMode(FInputModeUIOnly())
→ SetShowMouseCursor(true)
```
- `PlayerController`를 outer로 생성 → 레벨 전환 없이 표시되는 화면에 적합
- 레벨 전환 중에도 위젯이 살아있어야 하는 경우(로딩 화면 등)에는 `GameInstance`를 outer로 사용 (아래 패턴 참고)

### 레벨 전환 로딩 화면 패턴 (UC_BBKGameInstance / UC_LoadingScreenWidget 참고)
MoviePlayer 대신 GameViewport 오버레이 방식 사용. UMG 위젯은 GameInstance를 outer로 생성해 레벨 전환 중 GC 방지.
```
TravelToNextLevel()
  → ShowLoadingOverlay(Entry)
      → LoadingScreenWidgetClass 할당 시:
          CreateWidget<UC_LoadingScreenWidget>(this, Class)  ← this = GameInstance (GC 방지)
          InitializeLoadingScreen(Entry)
          TakeWidget() → AddViewportWidgetContent(Slate, ZOrder=100)
      → 미할당 시: 순수 Slate(SOverlay + SColorBlock + STextBlock) 폴백
  → OpenLevelBySoftObjectPtr()

LoadComplete()
  → Elapsed 계산 → MinLoadingTime 남은 시간 타이머 후 HideLoadingOverlay()
      → RemoveViewportWidgetContent + LoadingScreenWidgetInstance = nullptr
```
- MoviePlayer는 Slate 렌더 파이프라인만 지원 — UMG Designer 레이아웃 렌더링 불가 (Debugging Checklist 30번)
- `HideLoadingOverlay()`는 `LoadComplete()`에서만 호출 — 절대 OpenLevel 이후 즉시 호출 금지

### 레벨 간 캐릭터 상태 유지 패턴 (UC_BBKGameInstance 참고)
GameInstance(레벨 간 영속)에 `FPersistentGameState`로 저장. Non-seamless travel은 PlayerState가 재생성되므로 GameInstance가 유일한 저장소.
```
// 저장 (레벨 이동 직전)
TravelToNextLevel()
  → PlayerController->SaveStateForLevelTransition()
      → SaveGameState(roster, activeIndex, SharedASC)
          활성 캐릭터: GetHealth/Stamina/Shield/Mana 직접 읽기
          비활성 캐릭터: GetSavedXxxValue() (마지막 SaveCharacterState 기록값)
          SharedASC: experience, level 읽기
          activeCharacterIndex 저장

// 복원 (새 레벨 PlayerController::BeginPlay())
startIndex = GI->HasSavedState() ? GetSavedActiveCharacterIndex() : 0
Possess(characterRoster[startIndex])
OnCharacterSwitched.Broadcast(startIndex)
GI->RestoreGameState(roster, startIndex, SharedASC)
  활성 캐릭터: SetNumericAttributeBase로 어트리뷰트 직접 설정
  비활성 캐릭터: InjectPreSavedState → 나중 Possess 시 RestoreCharacterState 자동 적용
  activeSkillIndex: SwitchCommonSkill(savedIndex) — HUD 초기화 이후 호출이므로 즉시 반영
```
- 비활성 캐릭터 스킬 인덱스는 Possess 시 `InitializeDefaultSkill`이 0으로 리셋하므로 레벨 전환 후 첫 교체 시 0번으로 시작 (허용 범위)

### 캐릭터 교체 후 CommonSkill 인덱스 유지 패턴
`InitializeDefaultSkill`이 항상 activeSkillIndex=0으로 리셋하므로 OnCharacterSwitched.Broadcast 이후 덮어써야 함.
```
SwitchToCharacter(NextIndex)
  OldChar->SaveCharacterState()
    → FCharacterSavedState.activeSkillIndex = SM->activeSkillIndex

  Possess(NewChar) → AddCharacterAbilities → InitializeDefaultSkill → activeSkillIndex = 0

  OnCharacterSwitched.Broadcast() → HUD → SwitchCommonSkill(0)

  [복원] SM->SwitchCommonSkill(NewChar->GetSavedActiveSkillIndex())
    → savedIndex > 0 인 경우만 — 0번은 InitializeDefaultSkill이 이미 처리
    → OnCommonSkillSwitched 브로드캐스트 → HUD 아이콘 갱신
```

### 메인 메뉴 GameMode 패턴 (AC_MainMenuGameMode 참고)
전투 로직 없는 메뉴 전용 레벨의 GameMode. BeginPlay에서 위젯 생성 + 입력 모드 전환.
```
AC_MainMenuGameMode::BeginPlay()
  → if (!MainMenuWidgetClass) return
  → PC = GetWorld()->GetFirstPlayerController()
  → CreateWidget<UC_MainMenuWidget>(PC, MainMenuWidgetClass)
  → Widget->AddToViewport()
  → PC->SetShowMouseCursor(true)
  → PC->SetInputMode(FInputModeUIOnly())

UPROPERTY(EditDefaultsOnly, Category = "UI")
TSubclassOf<UC_MainMenuWidget> MainMenuWidgetClass
  → BP_MainMenuGameMode에서 WBP_MainMenu 할당
```
- `AC_BBKGameMode`와 완전히 독립 — 포탈·몬스터·PlayerController 스폰 없음
- 빈 레벨(Empty Level) World Settings → GameMode Override로 지정
- 메인 메뉴 레벨은 `UDA_LevelSequence` 배열에 포함하지 않을 것 (시퀀스는 플레이 레벨부터 시작)

### 서브 위젯 ZOrder 오버레이 패턴 (WBP_MainMenu + WBP_Settings 참고)
부모 위젯을 유지한 채 자식 위젯을 위에 올리고, 닫으면 부모로 자연 복귀.
```
// 부모 위젯 (WBP_MainMenu, ZOrder 0)
OnSettingsClicked()
  → if (!SettingsWidgetClass) return   ← 미할당 시 크래시 방지
  → CreateWidget(GetOwningPlayer(), SettingsWidgetClass)
  → AddToViewport(ZOrder: 1)           ← 부모 위에 덮어씌움

// 자식 위젯 (WBP_Settings, ZOrder 1)
OnCloseClicked()
  → RemoveFromParent()   ← 자신만 제거, 부모(ZOrder 0)가 다시 보임
```
- 부모를 숨기거나 재생성할 필요 없음
- 자식 위젯 배경을 불투명하게 디자인해야 부모가 완전히 가려짐

### 볼륨 설정 패턴 (UC_BBKGameUserSettings / UC_SettingsWidget 참고)
UGameUserSettings 서브클래스로 볼륨 저장, SoundMix/SoundClass로 적용. SoundMix/SoundClass ref는 GameInstance에 보관 → 레벨 로드마다 자동 복원.
```
UC_BBKGameUserSettings (config=GameUserSettings)
  UPROPERTY(Config) float MasterVolume / BGMVolume / SFXVolume = 1.0f
  static Get() → Cast<UC_BBKGameUserSettings>(UGameUserSettings::GetGameUserSettings())
  ApplyVolumeSettings(WorldContext, SoundMix, SC_Master, SC_BGM, SC_SFX)
    → PushSoundMixModifier
    → SetSoundMixClassOverride × 3 (Volume, Pitch=1, FadeTime=0, bApplyToChildren=true)

UC_BBKGameInstance
  UPROPERTY(EditDefaultsOnly, Category="Audio") USoundMix* / USoundClass* × 3
  ApplyVolumeSettings() → Settings->ApplyVolumeSettings(this, ...)
  LoadComplete()에서 ApplyVolumeSettings() 호출 → 레벨 전환 후 자동 복원

UC_SettingsWidget::NativeConstruct
  → GameUserSettings에서 현재 값 읽어 슬라이더 초기화 + UpdateVolumeText
  → OnXxxVolumeChanged 바인딩

OnXxxVolumeChanged(float Value)
  → Settings->SetXxxVolume(Value)
  → UpdateVolumeText(TextBlock, Value)   // FString::Printf(TEXT("%d%%"), RoundToInt(Value*100))
  → GI->ApplyVolumeSettings()           // 즉시 반영
  → Settings->SaveSettings()            // 즉시 저장

DefaultEngine.ini [/Script/Engine.Engine]
  GameUserSettingsClassName=/Script/ProjectBBK.C_BBKGameUserSettings
```
에디터 설정:
- SC_BGM / SC_SFX → Parent Class = SC_Master 설정
- SM_GameSettings (Sound Class Mix 에셋)
- BP_GameInstance Audio 슬롯에 4개 에셋 할당
- 슬라이더 Height 조절: Bar Thickness 사용 (SizeBox Height Override 사용 금지 — Debugging Checklist 34번)
