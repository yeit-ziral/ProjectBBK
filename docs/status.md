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
| AC_BombMonster (자폭 돌진) | ✅ 완료 (PIE 검증) | BT 미사용 — Tick 상태머신(배회 ↔ S자 위빙 돌진 → 접촉 기폭 → 폭발 → 자기 사망). BPC_BombMonster + DT_MonsterData_Bomb + GE_BombExplosion + 흑백 불덩이 머티리얼 전부 연결. PIE 검증: 배회(홈 반경 263/속도 250), 인식→돌진, 폭발 데미지 정확히 30 (100→70, 최대체력 30% true damage), 자기 소멸 확인. `weaveFrequency`는 0.35 권장(1.2는 S자가 아닌 잔떨림 — 측정치 편차 29cm vs 47cm) |
| AC_ShieldMonster (전면 방어 + 패링) | ✅ 완료 (PIE 검증 대기) | 정면 `guardHalfAngle`(90도) 반각 내 데미지를 `guardDamageReduction`(1.0 = 완전 무효)만큼 감소(방어력 감산 이전), 측/후면 정상 피해. `AC_BaseMonster::ModifyIncomingDamage` 훅 → `UC_MonsterAttributeSet::PostGameplayEffectExecute`에서 호출. 노말 공격 타격 순간 플레이어가 `State.Shield`면 패링 → `UC_GroggyComponent::ForceGroggy`로 즉시 그로기. 타이밍 링은 `M_ParryRing`(Disable Depth Test 평면 메시, Decal 아님). 에셋 전부 연결됨: `BPC_ShieldMonster` / `ABP_ShieldMonster`(GuardAlpha C++ 계산 + Apply Mesh Space Additive) / `BS_ShieldMonster` / AM_Attack·Groggy·Death / `BPC_ShieldMonsterNormalAttackGA` / `DT_MonsterData_Shield`(Row Id 1004) / `BT_Monster_Shield`. 남은 작업: PIE 검증(`bDrawGuardDebug`·`bDrawParryDebug`) |
| UC_BTTaskReposition (Idle Reposition / Strafe) | 🔧 에디터 작업 필요 | C++ 완료. BT_Monster_Melee·Ranged·Boss에 fallback 브랜치로 배치 + TargetActorKey 바인딩 필요. FMonsterData DataTable에 Reposition 컬럼 값 입력 필요 |

### UI
| Widget | 상태 | 비고 |
|--------|------|------|
| WBP_HUD | ✅ 완료 | WBP_UltimateGauge 통합 완료. PlayerController `CachedHUD` 가드로 캐릭터 로스터에서도 단일 인스턴스만 생성 |
| WBP_SkillIcon | ✅ 완료 | WBP_HUD의 child widget (Common/Unique 2종) |
| WBP_UltimateGauge | ✅ 완료 | WBP_HUD의 child widget으로 포함 |
| BPC_NormalMonsterHPWidget | ✅ 완료 | 3D 위젯, UC_MonsterAttributeSet 바인딩 |
| BPC_BossMonsterHPWidget | ✅ 완료 | 3D 위젯, UC_MonsterAttributeSet 바인딩 |
| WBP_RockSpearAim | ✅ 완료 | 조준선 위젯, GA_RockSpear 생명주기 직접 관리, 마우스 위치 추적 |
| WBP_SkillWheel | ✅ 완료 | Z키 토글, 마우스 각도 기반 섹터 판정, 호버 강조, 클릭 시 스킬 교체 |
| WBP_HUD (캐릭터 교체 연동) | ✅ 완료 | OnCharacterSwitched 델리게이트로 교체 시 SkillIcon·UltimateGauge 재초기화 |
| WBP_LoadingScreen | ✅ 완료 | UC_LoadingScreenWidget 기반. MoviePlayer 대신 Viewport Overlay 방식 사용. BP 미할당 시 Slate 폴백(검정+텍스트) 자동 사용 — BindWidget: LoadingBackground·DescriptionText·TipText·LoadingBar |
| WBP_EndingScreen | ✅ 완료 | UC_EndingScreenWidget 기반. OnReturnToMainMenu·OnQuitGame 버튼 포함 |
| WBP_GameOverScreen | ✅ 완료 | UC_GameOverWidget 기반. 전원 사망 시 HandleCharacterDeath에서 자동 표시 |
| WBP_MainMenu | ✅ 완료 | UC_MainMenuWidget 기반. BindWidget: StartButton·SettingsButton·QuitButton. 게임 시작: StartGame() 경유 로딩 오버레이 포함 |
| WBP_Settings | ✅ 완료 | UC_SettingsWidget 기반. BindWidget: CloseButton·MasterVolumeSlider·BGMVolumeSlider·SFXVolumeSlider·MasterVolumeText·BGMVolumeText·SFXVolumeText. Master/BGM/SFX 볼륨 슬라이더 + 퍼센트 텍스트. UC_BBKGameUserSettings 연동, 슬라이더 조작 시 즉시 적용·저장 |
| WBP_Status | ✅ 완료 | UC_StatusWidget 기반. BindWidget: MaxHPText·MaxStaminaText·MoveSpeedText·DefenseText·AttackText. ASC 어트리뷰트 변경 델리게이트로 실시간 반영. SizeBox WindowRoot + 드래그 이동. 캐릭터 교체·전원 사망 시 자동 처리. IA_Status 토글 키. 장비/포션(State.PotionBuff)으로 인한 스탯 증가분은 "총합 (+N)" 형태로 표시(스킬 버프/디버프는 제외). |
| WBP_UseItem | ✅ 완료 | UC_UseItemSlotWidget 기반. 퀵슬롯(2개) — 인벤토리 드래그&드롭으로 소비 아이템 참조 등록(인벤토리에서 제거 안 함), IA_UseItem0/1(1·2키)로 사용, 재고 0 시 아이콘 반투명 유지. 쿨다운(섹션 7)·재고없음 알림 사운드(섹션 8) C++ 구현 완료, PIE 확인. HUD 중복 생성으로 인한 사운드 중복 재생 이슈는 `CachedHUD` 가드 적용으로 해결. 구매/획득 등 UseItem 이외 경로의 수량 변화도 NotifyQuickSlotsForItem으로 표시 갱신. 퀵슬롯 아이템 교체 시 이전 쿨다운 오버레이가 남던 버그(`RestoreCooldownState` 리셋 누락, Debugging Checklist #46) 수정 완료 |
| WBP_Equipment | ✅ 완료 | UC_EquipmentComponent 연동. 인벤토리 슬롯 드래그&드롭 장착, 우클릭/더블클릭 해제, 슬롯 타입 검증(GetItemSlotType), 툴팁(C_ItemTooltipWidget). 캐릭터 교체 시 장비 보너스 GE는 Suspend/ReapplyEquipBonuses로 활성 캐릭터에만 적용 |

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
| GE_EquipBonus | ✅ 완료 | 장비 공용 GE. Infinite Duration, SetByCaller Modifier 5개 (MaxHealth, MaxStamina, MoveSpeed, Defense, Damage). 캐릭터 교체 시 UC_EquipmentComponent::Suspend/ReapplyEquipBonuses로 활성 캐릭터에만 적용되도록 격리 |
| GE_HealZoneTick | ✅ 완료 | Instant, Set by Caller Data.Heal — AC_HealZone이 체류 중 반복 적용 |
| GE_IncreaseMaxST | ✅ 완료 | 스탯 증가 포션 — Has Duration, State.PotionBuff 태그, maxStamina 증가 |
| GE_IncreaseDamage | ✅ 완료 | 스탯 증가 포션 — Has Duration, State.PotionBuff 태그, damage 증가 |
| GE_IncreaseDefense | ✅ 완료 | 스탯 증가 포션 — Has Duration, State.PotionBuff 태그, defense 증가 |
| GE_IncreaseSpeed | ✅ 완료 | 스탯 증가 포션 — Has Duration, State.PotionBuff 태그, moveSpeed 증가 |

### Objects
| Object | 상태 | 비고 |
|--------|------|------|
| C_ExpOrb / BP_ExpOrb | ✅ 완료 (C++ 구현) | Overlap → GE_GainExperience 적용 후 Destroy, 스폰 주체 미구현 |
| C_BaseItem / ItemData.h | ✅ 완료 | 상호작용 시 인벤토리에 itemID 추가 + Destroy. EnhancedInput(IA_Interact) 기반, 다중 Overlap 배열 관리. FBaseItemData·FConsumableItemData·FConsumableEffectEntry·FEquipmentItemData·EEquipmentSlot 정의. FConsumableItemData.useSound(USoundBase*, 사용 시 사운드) 필드 추가 |
| C_ConsumableItem / BP_ConsumableItem | ✅ 완료 | InitItem만 담당 (DT 로드 + Mesh). FConsumableEffectEntry 배열로 다중 GE 지원. 효과 적용은 인벤토리에서 처리 |
| C_EquipmentItem / BP_EquipItem | ✅ 완료 | InitItem만 담당 (DT 로드 + Mesh). 장착/해제는 UC_EquipmentComponent에서 처리 (구현 완료) |
| C_MoneyItem / BP_Money | ✅ 완료 | AC_BaseItem 상속. moneyAmount(EditAnywhere), BeginPlay에서 cachedItemName 포맷, OnInteract에서 AddMoney + Destroy |
| C_InteractionWidget / WBP_Interaction | ✅ 완료 | 상호작용 UI 위젯. BindWidget: InteractionText |
| UC_ConsumableAction | ✅ 완료 | GE 즉시 자기 적용만으로 표현 안 되는 소비 아이템 동작(AOE 판정, 액터 스폰 등) 처리용 UObject 베이스. FConsumableItemData.actionClass로 DT 연동 |
| UC_SpawnHealZoneAction / AC_HealZone (BP_HealZone) | ✅ 완료 | 힐장판 소비 아이템 — Instant GE(GE_HealZoneTick) + 존 자체 반복 타이머로 체류 중에만 회복 |
| UC_KnockbackAction | ✅ 완료 | 넉백 소비 아이템 — GE 없이 순수 LaunchCharacter, State.KnockbackImmune 면역 체크. 사용 위치에 useVFX 원샷 스폰 추가 |
| UC_BlinkAction / BP_BlinkItem | ✅ 완료 | 순간이동 소비 아이템 — GE 없이 LineTrace로 벽 충돌 체크 후 SetActorLocation, 쿨다운 있음(기존 아이템 쿨다운 시스템 재사용). 도착 위치에 arrivalVFX 원샷 스폰 추가 |
| AC_TreasureChest / BP_TreasureChest ×2 | ✅ 완료 (PIE 검증 완료) | Value 예산 기반 랜덤 드랍 상자. `AC_BaseItem` 상속(기존 상호작용 파이프라인 재사용), 장비 필수 상자는 최저-value 장비 1개 선차감 보장, 돈은 항상 1개(±5 오차), 인벤토리 미관여(월드에 픽업 스폰만). 오픈 시 사운드 재생. 런타임 스폰 오버랩 인식/표시값 버그는 `AC_BaseItem::RefreshOverlapState()`로 해결(다른 픽업에도 공용 적용), 지면 스폰 실패 시 공중 스폰 버그는 `FindGroundSpawnPoint` 폴백 체인으로 해결 |

### Level System
| Class / Asset | 상태 | 비고 |
|---------------|------|------|
| UDA_LevelSequence (LevelSequenceData.h) | ✅ 완료 | 에디터에서 DA_LevelSequence 에셋 생성 후 Levels 배열에 레벨·BGM·텍스처 항목 채우기 필요 |
| UC_BBKGameInstance (C_BBKGameInstance) | ✅ 완료 | 레벨 이동·로딩 오버레이·캐릭터 상태 저장/복원·에셋 프리로드·TravelToMainMenu·볼륨 설정 적용 포함. BP_GameInstance: DA_LevelSequence·MainMenuLevel·ActorClassesToPreload·Audio(GameSoundMix·SC_Master·SC_BGM·SC_SFX) 슬롯 할당 필요 |
| UC_BBKGameUserSettings (C_BBKGameUserSettings) | ✅ 완료 | UGameUserSettings 서브클래스. Master/BGM/SFX 볼륨 저장/로드/적용. DefaultEngine.ini GameUserSettingsClassName 등록 완료. BP_GameInstance Audio 슬롯 할당 필요 |
| AC_BBKGameMode (C_BBKGameMode) | ✅ 완료 | DefaultPawnClass=nullptr 설정 완료. BeginPlay에서 AC_BaseMonster·AC_Portal 자동 수집. 각 레벨 GameMode로 설정 필요 |
| AC_Portal (C_Portal) | ✅ 완료 | BP_Portal 생성 후 Niagara 에셋 할당, 각 레벨에 배치 필요. 기본 비활성화 → 몬스터 전멸 시 GameMode가 ActivatePortal() 호출 |
| AC_MainMenuGameMode (C_MainMenuGameMode) | ✅ 완료 | 메인 메뉴 레벨 전용 GameMode. BeginPlay에서 WBP_MainMenu 생성 + UIOnly 입력 모드 설정 |
| BGM 재생 로직 | 📋 계획 중 | 구현 예정 |

### Tutorial
| 항목 | 상태 | 비고 |
|------|------|------|
| UC_TutorialComponent / AC_TutorialGameMode | ✅ 완료 (PIE 검증 대기) | 단계 진행·입력 관찰·화살표·문구. 문구/pointer/hint/reveal은 `Tutorial/TutorialText.txt`로 재빌드 없이 수정 |
| DT_TutorialSteps (20단계) | ✅ 완료 (PIE 검증 대기) | order 1~20. 07·09·10·12에 Idle 더미, 08에 Attacker 더미 스폰 설정. 03_MoveSide는 `bAcceptOppositeDirection`으로 A/D 모두 인정 |
| 단계별 아이템 등장 | ✅ 완료 (PIE 검증 대기) | `bRevealItemsOnEnter`/`revealActorTag` — 시작 시 숨김(렌더+콜리전), 15_Interact에서 공개. L_Tutorial에 `HpPotion`(400,-600) + `IronChest`(-400,-600, 공용 장비 — 이 시점엔 원거리 캐릭터라 근접 전용 장비는 장착 불가) 배치 완료 |
| 단계별 더미 스폰/제거 | ✅ 완료 (PIE 검증 대기) | `spawnActorClass`/`spawnDistance` — 진입 시 플레이어 앞 지면에 스폰, 다음 단계 진입·완료·중단 시 Destroy |
| BPC_TutorialDummy_Idle / _Attacker | ✅ 완료 (PIE 검증 대기) | BPC_MeleeMonster 자식. DT_MonsterData_TutorialDummy(Idle 1006 / Attacker 1007). MoveSpeed 0 + bEnableReposition false로 제자리, SpecialCooldown 9999로 특수공격 봉인. HP 위젯 이름은 `FMonsterData.DisplayName`("튜토리얼") — 비워두면 Row Name 표시 |
| 08_Shield 완료 조건 | ✅ 완료 (PIE 검증 대기) | `Event.Player.ShieldBlocked` × 2회. UC_ChracterAttributeSetBase의 실드 무효화 지점에서 HandleGameplayEvent 발신 → 튜토리얼이 ASC 구독으로 수신. 타이밍 완화: Attacker 더미 공격 몽타주를 히트 노티파이 0.15초 전에 `Montage_Pause` + 플레이어 몸 옆 말풍선(`UC_TutorialPointerWidget::ShowWorldCallout`, 매 프레임 월드→화면 투영)으로 "지금 C를 눌러 몬스터의 공격을 막으세요" 표시(우상단 단계 문구는 유지), 플레이어가 `State.Shield`를 얻으면 `Montage_Resume` (TutorialText `08_Shield.pauseHit`/`.resumeTag`, 몬스터·GA 코드 미수정) |
| 10_Ultimate 완료 조건 | ✅ 완료 (PIE 검증 대기) | 시전이 아니라 **명중** 기준 — `Event.Player.UltimateHit` × 1회. UC_MonsterAttributeSet이 데미지 소스의 `Ability.Skill.Ultimate` 태그(2순위: 시전자 `State.UsingUltimate`)를 보고 시전자 ASC에 발신. 빗나가도 재시도 가능하도록 `bFillManaOnEnter` 단계는 1초 주기로 마나 유지 |
| 12_RangedAttack (원거리 일반공격·탄알) | ✅ 완료 (PIE 검증 대기) | 11_SwitchChar(근거리→원거리) 직후. `IA_Attack` × 3회, Idle 더미 400cm 스폰. 우하단 `WBP_AmmoCylinder`를 노란 박스+화살표로 강조하고 "원거리 캐릭터가 일반공격을 하면 탄알을 소모합니다" hint 표시. C++ 변경 없음(DT 행 + TutorialText.txt만). ⚠️ 원거리 공격이 첫 발 이후 막히는 문제는 `GA_RangeAttack` 치명타 Branch False 핀 미연결이 원인(담당 팀원 파일 — 미수정) |
| 15_Interact 완료 조건 | ✅ 완료 (PIE 검증 대기) | `Event.Tutorial.ItemPickedUp` × 2 — UC_TutorialComponent가 AC_BaseItem의 OnDestroyed를 관찰(획득 성공 시 Destroy). 아이템 코드 미수정 |
| 17_RegisterQuickSlot (포션 퀵슬롯 등록) | ✅ 완료 (PIE 검증 대기) | `Event.Tutorial.QuickSlotRegistered` — UC_InventoryComponent::OnQuickSlotChanged 관찰, 슬롯 아이템이 바뀌어 채워진 경우만 인정. 화살표 대상 `HorizontalBox_115`(퀵슬롯 2칸) |
| 20_EquipItem (장비 장착) | ✅ 완료 (PIE 검증 대기) | `Event.Tutorial.ItemEquipped` — 로스터 전원의 UC_EquipmentComponent::OnEquipmentChanged 관찰, 장착 슬롯 수 증가 시 인정. 화살표 대상 `@EquippableSlot`(인벤토리에서 현재 캐릭터가 장착 가능한 장비가 든 칸 하나 — SlotGrid 전체는 스크롤 영역보다 커서 박스가 창을 넘어감) + "장비를 더블 클릭 혹은 드래그로 장비를 장착할 수 있습니다" |
| 튜토리얼 화살표 재탐색 | ✅ 완료 (PIE 검증 대기) | 대상이 있는 단계 동안 0.5초마다 재탐색 — 단계 진입 후에 연 창도 가리킴. 닫힌 창(RemoveFromParent)은 캐시 지오메트리가 남아도 가리키지 않음(`UC_TutorialPointerWidget::IsWidgetOnScreen`) |

### Animation / IK
| 항목 | 상태 | 비고 |
|------|------|------|
| ABP_Melee Foot IK | ✅ 완료 | EventGraph(BP) 구현. IK_Melee 리그 + Transform(Modify) Bone. Mesh Z = -96(= -CapsuleHalfHeight) 정렬, FootTrace 하드코딩 상수 제거로 평지·계단 모두 정상 동작 확인 |
| ABP_Melee Foot IK — 잔여 개선 | 🔧 에디터 작업 필요 | ① FootTrace 기준면을 Root 소켓 → 캡슐 바닥으로 교체(루트모션 몽타주 대비) ② Offset Clamp 추가(절벽 대비) ③ 디버그 Print String/Text 노드 정리 |
| ABP_Range Foot IK (UC_RangeAnimInstance) | ✅ 완료 (C++ 구현) | 캡슐 바닥 기준, ImpactPoint 사용, Clamp·급경사 회전 제외 포함. BP 판 대비 안전장치가 갖춰진 참조 구현 |
