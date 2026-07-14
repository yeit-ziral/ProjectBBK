// Fill out your copyright notice in the Description page of Project Settings.

#include "C_PlayerController.h"
#include "../../UI/C_StatusWidget.h"
#include "../../Items/C_BaseItem.h"
#include "../../Skills/C_SkillManagerComponent.h"
#include "../../LevelSystem/C_BBKGameInstance.h"
#include "../../LevelSystem/C_EndingScreenWidget.h"
#include "../../LevelSystem/C_GameOverWidget.h"
#include "../C_PlayerState.h"
#include "../C_BasePlayerCharactor.h"
#include "../../Inventory/C_InventoryComponent.h"
#include "../../Inventory/C_InventoryWidget.h"
#include "../../Equip/C_EquipmentComponent.h"
#include "../../Equip/C_EquipmentWidget.h"
#include "../../NPC/C_MerchantNPC.h"
#include "../../NPC/C_MerchantDialogueWidget.h"
#include "../../NPC/C_ShopWidget.h"
#include "Camera/PlayerCameraManager.h"
#include "AbilitySystemComponent.h"
#include "../../GAS/Attributes/C_ChracterAttributeSetBase.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/CapsuleComponent.h"
#include "../C_LevelUpPerkComponent.h"


AC_PlayerController::AC_PlayerController()
{
	// 캐릭터 교체와 무관하게 유지되도록 컨트롤러에 부착 (DataTable은 BP 컴포넌트 디테일에서 지정)
	inventory = CreateDefaultSubobject<UC_InventoryComponent>(TEXT("Inventory"));
}

float AC_PlayerController::GetSwitchCooldownRemaining() const
{
	if (!bSwitchOnCooldown)
		return 0.0f;
	float elapsed = GetWorld()->GetTimeSeconds() - switchCooldownStartTime;
	return FMath::Max(0.f, switchCooldownDuration - elapsed);
}

void AC_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 레벨 재시작·전환 후 항상 게임 입력 모드로 초기화 (엔딩 화면 이후 되돌아올 때 대비)
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);

	// 인벤토리 컴포넌트에 아이템 조회용 DataTable 주입 (AddItem의 유효성 검사에 필요)
	if (inventory)
	{
		inventory->SetItemTables(consumableTable, equipmentTable);

		// 테스트/디버그용 시작 아이템 적재
		for (const TPair<FName, int32>& it : startingItems)
			inventory->AddItem(it.Key, it.Value);

		// 테스트/디버그용 시작 금액
		if (startingMoney > 0)
			inventory->AddMoney(startingMoney);
	}

	// 서버(싱글플레이어 포함)에서만 스폰
	if (!HasAuthority())
		return;

	// IMC를 컨트롤러에서 한번만 추가 - 캐릭터 교체와 무관하게 유지됨
	if (ULocalPlayer *LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
		{
			if (playerMappingContext)
				Subsystem->AddMappingContext(playerMappingContext, 0);
		}
	}

	// 스폰 위치: 레벨에 배치된 첫 번째 PlayerStart 기준
	FTransform SpawnTransform = FTransform::Identity;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		SpawnTransform = It->GetActorTransform();
		break;
	}

	if (characterRosterClasses.Num() == 0)
		return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (TSubclassOf<AC_BasePlayerCharactor> &CharClass : characterRosterClasses)
	{
		if (!CharClass)
			continue;

		AC_BasePlayerCharactor *Char = GetWorld()->SpawnActor<AC_BasePlayerCharactor>(
			CharClass, SpawnTransform, SpawnParams);

		if (Char)
		{
			// 일단 모두 비활성화 (0번만 나중에 활성화)
			Char->SetActorHiddenInGame(true);
			Char->SetActorEnableCollision(false);
			characterRoster.Add(Char);
		}
	}

	// 저장된 상태가 있으면 해당 캐릭터 인덱스로 시작, 없으면 0번
	UC_BBKGameInstance *GI_Ref = Cast<UC_BBKGameInstance>(GetGameInstance());
	const int32 startIndex = (GI_Ref && GI_Ref->HasSavedState())
								 ? FMath::Clamp(GI_Ref->GetSavedActiveCharacterIndex(), 0, characterRoster.Num() - 1)
								 : 0;

	if (characterRoster.IsValidIndex(startIndex))
	{
		characterRoster[startIndex]->SetActorHiddenInGame(false);
		characterRoster[startIndex]->SetActorEnableCollision(true);
		Possess(characterRoster[startIndex]);
		currentCharacterIndex = startIndex;

		// WBP_HUD는 각 캐릭터의 BeginPlay(Possess 전)에서 생성되므로
		// NativeConstruct 시점엔 어빌리티가 아직 없음.
		// Possess 완료(= AddCharacterAbilities 완료) 후 브로드캐스트해야 HUD 초기화 가능.
		OnCharacterSwitched.Broadcast(startIndex);

		// HUD 초기화 완료 후 이전 레벨의 저장 상태 복원 (activeCharacterIndex 기반 어트리뷰트 적용)
		if (GI_Ref)
		{
			UAbilitySystemComponent *SharedASC = nullptr;
			AC_PlayerState* PS = GetPlayerState<AC_PlayerState>();
			if (PS)
				SharedASC = PS->GetAbilitySystemComponent();

			UC_LevelUpPerkComponent* PerkComp =
				PS ? PS->FindComponentByClass<UC_LevelUpPerkComponent>() : nullptr;

			if (PerkComp)
				PerkComp->SetIgnoreLevelChanges(true); // 캐릭터 교체 시 레벨업 퍽 UI가 뜨지 않도록 잠깐 끔
			GI_Ref->RestoreGameState(characterRoster, startIndex, SharedASC);
			if(PerkComp)
				PerkComp->SetIgnoreLevelChanges(false);
		}
	}

	// 게임 클리어 이벤트 바인딩 — 마지막 레벨 포탈 진입 시 엔딩 화면 표시
	if (UC_BBKGameInstance *GI = Cast<UC_BBKGameInstance>(GetGameInstance()))
	{
		GI->OnGameClear.AddDynamic(this, &AC_PlayerController::ShowEndingScreen);
	}
}

void AC_PlayerController::OnPossess(APawn *InPawn)
{
	Super::OnPossess(InPawn);

	AC_PlayerState *PS = GetPlayerState<AC_PlayerState>();
	if (PS)
	{
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, InPawn);
	}
}

void AC_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent *EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (IA_SwitchChar0)
			EIC->BindAction(IA_SwitchChar0, ETriggerEvent::Started, this, &AC_PlayerController::OnSwitchChar0Input);

		if (IA_SwitchChar1)
			EIC->BindAction(IA_SwitchChar1, ETriggerEvent::Started, this, &AC_PlayerController::OnSwitchChar1Input);

		if (IA_SwitchCharNext)
			EIC->BindAction(IA_SwitchCharNext, ETriggerEvent::Started, this, &AC_PlayerController::SwitchToNextCharacter);

		if (IA_Inventory)
			EIC->BindAction(IA_Inventory, ETriggerEvent::Started, this, &AC_PlayerController::ToggleInventory);

		if (IA_Equipment)
			EIC->BindAction(IA_Equipment, ETriggerEvent::Started, this, &AC_PlayerController::ToggleEquipment);

		if (IA_Status)
			EIC->BindAction(IA_Status, ETriggerEvent::Started, this, &AC_PlayerController::ToggleStatus);

		if (IA_Interact)
			EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &AC_PlayerController::OnInteractInput);

		if (IA_TalkToNPC)
			EIC->BindAction(IA_TalkToNPC, ETriggerEvent::Started, this, &AC_PlayerController::OnTalkToNPCInput);

		if (IA_UseItem0)
			EIC->BindAction(IA_UseItem0, ETriggerEvent::Started, this, &AC_PlayerController::OnUseItemSlot0Input);

		if (IA_UseItem1)
			EIC->BindAction(IA_UseItem1, ETriggerEvent::Started, this, &AC_PlayerController::OnUseItemSlot1Input);
	}
}

void AC_PlayerController::ToggleInventory()
{
	if (bInventoryOpen)
		CloseInventory();
	else
		OpenInventory();
}

void AC_PlayerController::OpenInventory()
{
	if (bInventoryOpen || !inventoryWidgetClass)
		return;

	// 최초 1회만 생성 — 닫아도 인스턴스 유지(드래그한 위치 보존)
	if (!inventoryWidget)
	{
		inventoryWidget = CreateWidget<UC_InventoryWidget>(this, inventoryWidgetClass);
		if (!inventoryWidget)
			return;
	}

	inventoryWidget->AddToViewport();

	// 닫을 때 NativeDestruct가 OnInventoryChanged 바인딩을 해제하므로 매 open마다 재연결 + 현재 내용으로 갱신.
	// (이게 없으면 최초 1회 이후 닫았다 열면 위젯이 컴포넌트 변경을 수신하지 못해 새 아이템이 표시되지 않음)
	inventoryWidget->SetInventory(inventory);

	bInventoryOpen = true;

	// 커서 ON + 카메라 회전 차단 + UMG가 마우스(드래그) 입력 받도록 GameAndUI.
	// 스킬휠 등 다른 UI와 커서 소유권이 겹쳐도 PushMouseUI가 멱등하게 처리.
	PushMouseUI(EMouseUISource::Inventory, inventoryWidget);
}

void AC_PlayerController::CloseInventory()
{
	if (!bInventoryOpen)
		return;

	if (inventoryWidget)
		inventoryWidget->RemoveFromParent();

	bInventoryOpen = false;

	// 인벤토리 커서 요청 해제 — 스킬휠 등 다른 UI가 남아 있으면 커서는 유지됨
	PopMouseUI(EMouseUISource::Inventory);
}

void AC_PlayerController::PushMouseUI(EMouseUISource source, UUserWidget* widgetToFocus)
{
	activeMouseUISources |= static_cast<uint8>(source);

	SetShowMouseCursor(true);
	SetIgnoreLookInput(true);

	FInputModeGameAndUI inputMode;
	inputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (widgetToFocus)
		inputMode.SetWidgetToFocus(widgetToFocus->TakeWidget());
	SetInputMode(inputMode);
}

void AC_PlayerController::PopMouseUI(EMouseUISource source)
{
	activeMouseUISources &= ~static_cast<uint8>(source);

	// 아직 커서를 요청 중인 다른 UI가 남아 있으면 커서/입력모드를 유지
	if (activeMouseUISources != 0)
	{
		SetShowMouseCursor(true);

		FInputModeGameAndUI inputMode;
		inputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(inputMode);
		return;
	}

	// 모든 UI가 닫힘 — 커서 OFF + 카메라 회전 복원 + 게임 입력 모드 복귀
	SetShowMouseCursor(false);
	ResetIgnoreLookInput();
	SetInputMode(FInputModeGameOnly());
}

void AC_PlayerController::ToggleEquipment()
{
	if (bEquipmentOpen)
		CloseEquipment();
	else
		OpenEquipment();
}

void AC_PlayerController::OpenEquipment()
{
	if (bEquipmentOpen)
		return;

	// 장비창은 캐릭터별(멜리/레인지 배경) — 현재 Pawn에서 위젯 클래스와 장비 컴포넌트를 가져옴
	AC_BasePlayerCharactor* Char = Cast<AC_BasePlayerCharactor>(GetPawn());
	if (!Char)
		return;

	TSubclassOf<UC_EquipmentWidget> WidgetClass = Char->GetEquipmentWidgetClass();
	UC_EquipmentComponent* Equip = Char->FindComponentByClass<UC_EquipmentComponent>();
	if (!WidgetClass || !Equip)
		return;

	equipmentWidget = CreateWidget<UC_EquipmentWidget>(this, WidgetClass);
	if (!equipmentWidget)
		return;

	equipmentWidget->AddToViewport();
	equipmentWidget->SetEquipment(Equip);

	bEquipmentOpen = true;

	// 인벤토리와 동시에 떠도 커서/입력모드가 안 꼬이도록 플래그로 관리
	PushMouseUI(EMouseUISource::Equipment);
}

void AC_PlayerController::CloseEquipment()
{
	if (!bEquipmentOpen)
		return;

	if (equipmentWidget)
		equipmentWidget->RemoveFromParent();
	equipmentWidget = nullptr;

	bEquipmentOpen = false;
	PopMouseUI(EMouseUISource::Equipment);
}

void AC_PlayerController::ToggleStatus()
{
	if (bStatusOpen)
		CloseStatus();
	else
		OpenStatus();
}

void AC_PlayerController::OpenStatus()
{
	if (bStatusOpen || !statusWidgetClass)
		return;

	// 최초 1회만 생성 — 닫아도 인스턴스 유지 (드래그 위치 보존)
	if (!statusWidget)
	{
		statusWidget = CreateWidget<UC_StatusWidget>(this, statusWidgetClass);
		if (!statusWidget)
			return;
	}

	statusWidget->AddToViewport();

	UAbilitySystemComponent* ASC = nullptr;
	if (AC_PlayerState* PS = GetPlayerState<AC_PlayerState>())
		ASC = PS->GetAbilitySystemComponent();

	statusWidget->InitializeStatWindow(ASC);

	bStatusOpen = true;
	PushMouseUI(EMouseUISource::Status, statusWidget);
}

void AC_PlayerController::CloseStatus()
{
	if (!bStatusOpen)
		return;

	if (statusWidget)
		statusWidget->RemoveFromParent();

	bStatusOpen = false;
	PopMouseUI(EMouseUISource::Status);
}

void AC_PlayerController::OnSwitchChar0Input()
{
	SwitchToCharacter(0);
}

void AC_PlayerController::OnSwitchChar1Input()
{
	SwitchToCharacter(1);
}

void AC_PlayerController::OnUseItemSlot0Input()
{
	if (inventory)
		inventory->UseQuickSlot(0);
}

void AC_PlayerController::OnUseItemSlot1Input()
{
	if (inventory)
		inventory->UseQuickSlot(1);
}

void AC_PlayerController::SwitchToCharacter(int32 NextIndex, bool bForce)
{
	if (bIsSwitching)
		return;
	if (NextIndex == currentCharacterIndex)
		return;
	if (!characterRoster.IsValidIndex(NextIndex))
		return;

	// 인벤토리/장비창이 열려 있으면 캐릭터 교체 차단 (강제 교체는 허용)
	if (!bForce && (bInventoryOpen || bEquipmentOpen))
		return;

	// SkillWheel이 열려 있으면 캐릭터 교체 차단
	if (!bForce)
	{
		if (AC_PlayerState *PS = GetPlayerState<AC_PlayerState>())
		{
			static const FGameplayTag SkillWheelTag = FGameplayTag::RequestGameplayTag(FName("State.SkillWheelOpen"));
			if (PS->GetAbilitySystemComponent()->HasMatchingGameplayTag(SkillWheelTag))
				return;
		}
	}

	AC_BasePlayerCharactor *OldChar = characterRoster[currentCharacterIndex];
	AC_BasePlayerCharactor *NewChar = characterRoster[NextIndex];

	if (!OldChar || !NewChar)
		return;

	if (!bForce && NewChar->bIsDead)
		return;

	bIsSwitching = true;
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	for (AC_BasePlayerCharactor *Char : characterRoster)
		if (Char)
			Char->bSuppressDeath = true;

	savedControlRotation = GetControlRotation();

	if (switchEffect)
	{
		FVector feetLocation = OldChar->GetActorLocation();
		feetLocation.Z -= OldChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		UNiagaraComponent *spawnedEffect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, switchEffect, feetLocation);

		if (spawnedEffect)
		{
			FLinearColor blue = FLinearColor(0.0f, 0.3f, 1.0f, 1.0f);
			spawnedEffect->SetColorParameter(TEXT("Color_Circle"), blue);
			spawnedEffect->SetColorParameter(TEXT("Color_Smoke"), blue);
			spawnedEffect->SetColorParameter(TEXT("Color_Sparks1"), blue);
			spawnedEffect->SetColorParameter(TEXT("Color_Spiral1"), blue);
		}
	}

	FTimerDelegate SwitchDelegate = FTimerDelegate::CreateUObject(this, &AC_PlayerController::ExecuteCharacterSwitch, NextIndex);
	GetWorldTimerManager().SetTimer(switchDelayTimerHandle, SwitchDelegate, 0.5f, false);
}

void AC_PlayerController::ExecuteCharacterSwitch(int32 NextIndex)
{
	if (!characterRoster.IsValidIndex(currentCharacterIndex) || !characterRoster.IsValidIndex(NextIndex))
	{
		for (AC_BasePlayerCharactor *Char : characterRoster)
			if (Char)
				Char->bSuppressDeath = false;
		ResetIgnoreMoveInput();
		ResetIgnoreLookInput();
		bIsSwitching = false;
		return;
	}

	AC_BasePlayerCharactor *OldChar = characterRoster[currentCharacterIndex];
	AC_BasePlayerCharactor *NewChar = characterRoster[NextIndex];

	if (!OldChar || !NewChar)
	{
		for (AC_BasePlayerCharactor *Char : characterRoster)
			if (Char)
				Char->bSuppressDeath = false;
		ResetIgnoreMoveInput();
		ResetIgnoreLookInput();
		bIsSwitching = false;
		return;
	}

	// 새 캐릭터를 현재 위치/회전으로 이동
	NewChar->SetActorLocationAndRotation(OldChar->GetActorLocation(), OldChar->GetActorRotation());

	// 속도 이어받기 (공중에서 교체 시 자연스러운 낙하)
	NewChar->GetCharacterMovement()->Velocity = OldChar->GetCharacterMovement()->Velocity;

	// 새 캐릭터 활성화
	NewChar->SetActorHiddenInGame(false);
	NewChar->SetActorEnableCollision(true);

	if (!OldChar->bIsDead)
		OldChar->SaveCharacterState();

	UAbilitySystemComponent *ASC = nullptr;
	if (AC_PlayerState *PS = GetPlayerState<AC_PlayerState>())
		ASC = PS->GetAbilitySystemComponent();

	if (ASC)
	{
		OldChar->SaveActiveEffects(ASC);

		FGameplayTagContainer tagsToRemove;
		tagsToRemove.AddTag(FGameplayTag::RequestGameplayTag(FName("State")));
		tagsToRemove.AddTag(FGameplayTag::RequestGameplayTag(FName("Effect.Cooldown")));
		ASC->RemoveActiveEffectsWithGrantedTags(tagsToRemove);
	}

	// 이전 캐릭터의 어빌리티를 ASC에서 제거 (characterAbilitiesGiven = false로 리셋됨)
	OldChar->RemoveCharacterAbilities();

	// 교체할 때 레벨 복원이 퍽 선택창을 띄우지 않게 잠시 무시
	UC_LevelUpPerkComponent* PerkComp = nullptr;
	if(AC_PlayerState* PerkPS = GetPlayerState<AC_PlayerState>())
		PerkComp = PerkPS->FindComponentByClass<UC_LevelUpPerkComponent>();

	if (PerkComp)
		PerkComp->SetIgnoreLevelChanges(true);	

	// Possess → PossessedBy → InitAbilityActorInfo(PS, NewChar) 자동 호출
	Possess(NewChar);
	currentCharacterIndex = NextIndex;

	SetControlRotation(savedControlRotation);

	if (ASC)
	{
		NewChar->RestoreActiveEffects(ASC);

		// 공유 ASC에서 State.Dead 태그 제거(죽은 캐릭터에서 오염됨)
		FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(FName("State.Dead"));
		ASC->RemoveLooseGameplayTag(DeadTag);
	}

	// 레벨 복원이 끝났으니 퍽 감지 다시 켬
	if (PerkComp)
		PerkComp->SetIgnoreLevelChanges(false);

	// 이전 캐릭터 비활성화
	OldChar->SetActorHiddenInGame(true);
	OldChar->SetActorEnableCollision(false);

	// HUD가 여기에 바인딩해서 위젯을 재초기화함 (Possess 이후이므로 새 어빌리티가 ASC에 등록된 상태)
	OnCharacterSwitched.Broadcast(NextIndex);

	// 스탯창이 열려있으면 새 캐릭터 값으로 즉시 갱신
	if (bStatusOpen && statusWidget && ASC)
		statusWidget->InitializeStatWindow(ASC);

	// HUD 초기화(SwitchCommonSkill(0)) 이후 저장된 스킬 인덱스 복원
	// InitializeDefaultSkill이 항상 0번으로 리셋하므로 Broadcast 이후 덮어써야 함
	if (UC_SkillManagerComponent *SM = NewChar->FindComponentByClass<UC_SkillManagerComponent>())
	{
		const int32 SavedSkillIndex = NewChar->GetSavedActiveSkillIndex();
		if (SavedSkillIndex > 0)
			SM->SwitchCommonSkill(SavedSkillIndex);
	}

	// HUD 초기화(SwitchCommonSkill(0)) 이후 저장된 스킬 인덱스 복원
	// InitializeDefaultSkill이 항상 0번으로 리셋하므로 Broadcast 이후 덮어써야 함
	if (UC_SkillManagerComponent *SM = NewChar->FindComponentByClass<UC_SkillManagerComponent>())
	{
		const int32 SavedSkillIndex = NewChar->GetSavedActiveSkillIndex();
		if (SavedSkillIndex > 0)
			SM->SwitchCommonSkill(SavedSkillIndex);
	}

	for (AC_BasePlayerCharactor *Char : characterRoster)
		if (Char)
			Char->bSuppressDeath = false;

	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	bIsSwitching = false;

	// 교체 완료 후 쿨타임 시작
	bSwitchOnCooldown = true;
	switchCooldownStartTime = GetWorld()->GetTimeSeconds();
	OnSwitchCooldownStarted.Broadcast(switchCooldownDuration, NextIndex);

	GetWorldTimerManager().SetTimer(
		switchCooldownTimerHandle,
		[this]()
		{ bSwitchOnCooldown = false; },
		switchCooldownDuration, false);
}

void AC_PlayerController::HandleCharacterDeath(AC_BasePlayerCharactor *DeadCharacter)
{
	if (UC_SkillManagerComponent* SM = DeadCharacter->FindComponentByClass<UC_SkillManagerComponent>())
		SM->CloseSkillWheel();

	// UI가 열린 채 전원 사망 시 입력 모드 복원
	CloseInventory();
	CloseEquipment();
	CloseStatus();

	int32 NextLivingIndex = -1;
	for (int32 i = 0; i < characterRoster.Num(); i++)
	{
		if (characterRoster[i] == DeadCharacter)
			continue;

		if (characterRoster[i] && characterRoster[i]->IsAlive())
		{
			NextLivingIndex = i;
			break;
		}
	}

	if (NextLivingIndex != -1)
	{
		SwitchToCharacter(NextLivingIndex, true);
	}
	else
	{
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
		ShowGameOverScreen();
	}
}

void AC_PlayerController::SwitchToNextCharacter()
{
	if (characterRoster.Num() == 0)
		return;
	const int32 NextIndex = (currentCharacterIndex + 1) % characterRoster.Num();
	SwitchToCharacter(NextIndex);
}

void AC_PlayerController::SaveStateForLevelTransition()
{
	UAbilitySystemComponent *SharedASC = nullptr;
	if (AC_PlayerState *PS = GetPlayerState<AC_PlayerState>())
		SharedASC = PS->GetAbilitySystemComponent();

	if (UC_BBKGameInstance *GI = Cast<UC_BBKGameInstance>(GetGameInstance()))
		GI->SaveGameState(characterRoster, currentCharacterIndex, SharedASC);
}

void AC_PlayerController::ShowEndingScreen()
{
	if (!ensure(EndingScreenClass))
		return;

	UC_EndingScreenWidget *Widget = CreateWidget<UC_EndingScreenWidget>(this, EndingScreenClass);
	if (!Widget)
		return;

	Widget->AddToViewport(10);
	SetInputMode(FInputModeUIOnly());
	SetShowMouseCursor(true);
}

void AC_PlayerController::ShowGameOverScreen()
{
	if (!ensure(GameOverScreenClass))
		return;

	UC_GameOverWidget *Widget = CreateWidget<UC_GameOverWidget>(this, GameOverScreenClass);
	if (!Widget)
		return;

	Widget->AddToViewport(10);
	SetInputMode(FInputModeUIOnly());
	SetShowMouseCursor(true);
}

void AC_PlayerController::OnInteractInput()
{
	// 무효화된 항목 정리 후 마지막(가장 최근 진입) 아이템과 상호작용
	overlappingItems.RemoveAll([](const TWeakObjectPtr<AC_BaseItem>& Ptr) { return !Ptr.IsValid(); });
	if (overlappingItems.Num() == 0) return;

	AC_BasePlayerCharactor* PlayerChar = Cast<AC_BasePlayerCharactor>(GetPawn());
	if (!PlayerChar) return;

	overlappingItems.Last()->OnInteract(PlayerChar);
}

void AC_PlayerController::SetCurrentInteractable(AC_BaseItem* Item)
{
	overlappingItems.AddUnique(Item);
}

void AC_PlayerController::ClearCurrentInteractable(AC_BaseItem* Item)
{
	overlappingItems.RemoveAll([Item](const TWeakObjectPtr<AC_BaseItem>& Ptr) { return Ptr.Get() == Item; });
}

// ===== 상인 NPC / 상점 =====

void AC_PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	UpdateMerchantGaze();
}

void AC_PlayerController::UpdateMerchantGaze()
{
	// 상인 UI(대화/상점)가 열려 있으면 시선 감지 중지 — 포커스 유지, 재판정 안 함
	if (activeMouseUISources != 0)
		return;

	APawn* P = GetPawn();
	if (!P)
	{
		if (focusedMerchant.IsValid())
			focusedMerchant->SetFocused(false);
		focusedMerchant.Reset();
		return;
	}

	FVector viewLoc;
	FRotator viewRot;
	GetPlayerViewPoint(viewLoc, viewRot);

	const FVector traceEnd = viewLoc + viewRot.Vector() * merchantGazeDistance;

	FHitResult hit;
	FCollisionQueryParams params;
	params.AddIgnoredActor(P);

	AC_MerchantNPC* newFocus = nullptr;
	if (GetWorld()->LineTraceSingleByChannel(hit, viewLoc, traceEnd, ECC_Visibility, params))
		newFocus = Cast<AC_MerchantNPC>(hit.GetActor());

	if (newFocus == focusedMerchant.Get())
		return;

	if (focusedMerchant.IsValid())
		focusedMerchant->SetFocused(false);

	if (newFocus)
		newFocus->SetFocused(true);

	focusedMerchant = newFocus;
}

void AC_PlayerController::OnTalkToNPCInput()
{
	// 이미 상인 UI가 열려 있으면 무시
	if (activeMouseUISources & static_cast<uint8>(EMouseUISource::Merchant))
		return;

	if (focusedMerchant.IsValid())
		OpenMerchantDialogue(focusedMerchant.Get());
}

void AC_PlayerController::OpenMerchantDialogue(AC_MerchantNPC* NPC)
{
	if (!NPC || !dialogueWidgetClass)
		return;

	if (dialogueWidget)
		dialogueWidget->RemoveFromParent();

	dialogueWidget = CreateWidget<UC_MerchantDialogueWidget>(this, dialogueWidgetClass);
	if (!dialogueWidget)
		return;

	dialogueWidget->AddToViewport(9);
	dialogueWidget->Setup(NPC, this);

	NPC->PlayTalkAnim(); // 말 걸면 talk 모션

	PushMouseUI(EMouseUISource::Merchant, dialogueWidget);
}

void AC_PlayerController::CloseMerchantDialogue()
{
	if (dialogueWidget)
	{
		dialogueWidget->RemoveFromParent();
		dialogueWidget = nullptr;
	}

	PopMouseUI(EMouseUISource::Merchant);

	if (focusedMerchant.IsValid())
	{
		focusedMerchant->StopTalkAnim();
		focusedMerchant->SetFocused(false);
	}
	focusedMerchant.Reset();
}

void AC_PlayerController::OnDialogueBuyChosen(AC_MerchantNPC* NPC)
{
	// 대화창만 닫고 상점으로 (Merchant 커서는 유지 — OpenShop이 다시 Push)
	if (dialogueWidget)
	{
		dialogueWidget->RemoveFromParent();
		dialogueWidget = nullptr;
	}

	OpenShop(NPC);
}

void AC_PlayerController::OpenShop(AC_MerchantNPC* NPC)
{
	if (!NPC || !shopWidgetClass)
		return;

	if (shopWidget)
		shopWidget->RemoveFromParent();

	shopWidget = CreateWidget<UC_ShopWidget>(this, shopWidgetClass);
	if (!shopWidget)
		return;

	shopWidget->AddToViewport(10);
	shopWidget->Setup(NPC, inventory, this);
	bShopOpen = true;

	// 내 인벤토리도 동시에 표시
	OpenInventory();

	PushMouseUI(EMouseUISource::Merchant, shopWidget);
}

void AC_PlayerController::CloseShop()
{
	if (shopWidget)
	{
		shopWidget->RemoveFromParent();
		shopWidget = nullptr;
	}
	bShopOpen = false;

	CloseInventory();
	PopMouseUI(EMouseUISource::Merchant);

	if (focusedMerchant.IsValid())
	{
		focusedMerchant->StopTalkAnim();
		focusedMerchant->SetFocused(false);
	}
	focusedMerchant.Reset();
}
