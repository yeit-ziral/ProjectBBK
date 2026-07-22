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
| UC_BTTaskReposition (Idle Reposition / Strafe) | 🔧 에디터 작업 필요 | C++ 완료. BT_Monster_Melee·Ranged·Boss에 fallback 브랜치로 배치 + TargetActorKey 바인딩 필요. FMonsterData DataTable에 Reposition 컬럼 값 입력 필요 |

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
| WBP_LoadingScreen | ✅ 완료 | UC_LoadingScreenWidget 기반. MoviePlayer 대신 Viewport Overlay 방식 사용. BP 미할당 시 Slate 폴백(검정+텍스트) 자동 사용 — BindWidget: LoadingBackground·DescriptionText·TipText·LoadingBar |
| WBP_EndingScreen | ✅ 완료 | UC_EndingScreenWidget 기반. OnReturnToMainMenu·OnQuitGame 버튼 포함 |
| WBP_GameOverScreen | ✅ 완료 | UC_GameOverWidget 기반. 전원 사망 시 HandleCharacterDeath에서 자동 표시 |
| WBP_MainMenu | ✅ 완료 | UC_MainMenuWidget 기반. BindWidget: StartButton·SettingsButton·QuitButton. 게임 시작: StartGame() 경유 로딩 오버레이 포함 |
| WBP_Settings | ✅ 완료 | UC_SettingsWidget 기반. BindWidget: CloseButton·MasterVolumeSlider·BGMVolumeSlider·SFXVolumeSlider·MasterVolumeText·BGMVolumeText·SFXVolumeText. Master/BGM/SFX 볼륨 슬라이더 + 퍼센트 텍스트. UC_BBKGameUserSettings 연동, 슬라이더 조작 시 즉시 적용·저장 |
| WBP_Status | ✅ 완료 | UC_StatusWidget 기반. BindWidget: MaxHPText·MaxStaminaText·MoveSpeedText·DefenseText·AttackText. ASC 어트리뷰트 변경 델리게이트로 실시간 반영. SizeBox WindowRoot + 드래그 이동. 캐릭터 교체·전원 사망 시 자동 처리. IA_Status 토글 키. |
| WBP_UseItem | ✅ 완료 | UC_UseItemSlotWidget 기반. 퀵슬롯(2개) — 인벤토리 드래그&드롭으로 소비 아이템 참조 등록(인벤토리에서 제거 안 함), IA_UseItem0/1(1·2키)로 사용, 재고 0 시 아이콘 반투명 유지. 쿨다운(섹션 7)·재고없음 알림 사운드(섹션 8) C++ 구현 완료, PIE 확인. HUD 중복 생성으로 인한 사운드 중복 재생 이슈 있음(Known Issues 참고, BP 수정 예정) |

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
| GE_EquipBonus | ✅ 완료 | 장비 공용 GE. Infinite Duration, SetByCaller Modifier 5개 (MaxHealth, MaxStamina, MoveSpeed, Defense, Damage) |

### Objects
| Object | 상태 | 비고 |
|--------|------|------|
| C_ExpOrb / BP_ExpOrb | ✅ 완료 (C++ 구현) | Overlap → GE_GainExperience 적용 후 Destroy, 스폰 주체 미구현 |
| C_BaseItem / ItemData.h | ✅ 완료 | 상호작용 시 인벤토리에 itemID 추가 + Destroy. EnhancedInput(IA_Interact) 기반, 다중 Overlap 배열 관리. FBaseItemData·FConsumableItemData·FConsumableEffectEntry·FEquipmentItemData·EEquipmentSlot 정의 |
| C_ConsumableItem / BP_ConsumableItem | ✅ 완료 | InitItem만 담당 (DT 로드 + Mesh). FConsumableEffectEntry 배열로 다중 GE 지원. 효과 적용은 인벤토리에서 처리 |
| C_EquipmentItem / BP_EquipItem | ✅ 완료 | InitItem만 담당 (DT 로드 + Mesh). 장착/해제는 인벤토리에서 처리 (미구현) |
| C_MoneyItem / BP_Money | ✅ 완료 | AC_BaseItem 상속. moneyAmount(EditAnywhere), BeginPlay에서 cachedItemName 포맷, OnInteract에서 AddMoney + Destroy |
| C_InteractionWidget / WBP_Interaction | ✅ 완료 | 상호작용 UI 위젯. BindWidget: InteractionText |

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
