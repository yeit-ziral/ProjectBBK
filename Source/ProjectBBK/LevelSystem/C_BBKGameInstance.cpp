// Fill out your copyright notice in the Description page of Project Settings.

#include "C_BBKGameInstance.h"
#include "C_BBKGameUserSettings.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Containers/Ticker.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Styling/CoreStyle.h"
#include "../GAS/Attributes/C_ChracterAttributeSetBase.h"
#include "../PlayerCharacter/C_BasePlayerCharactor.h"
#include "../PlayerCharacter/PlayerAI/C_PlayerController.h"
#include "../Skills/C_SkillManagerComponent.h"
#include "../PlayerCharacter/C_LevelUpPerkComponent.h"

void UC_BBKGameInstance::Init()
{
	Super::Init();

	TArray<FSoftObjectPath> Paths;
	for (const TSoftClassPtr<AActor>& SoftClass : ActorClassesToPreload)
	{
		if (!SoftClass.IsNull())
			Paths.Add(SoftClass.ToSoftObjectPath());
	}

	if (Paths.Num() > 0)
	{
		FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
		Streamable.RequestAsyncLoad(Paths, FStreamableDelegate::CreateLambda([this, Paths]()
		{
			for (const FSoftObjectPath& Path : Paths)
			{
				if (UClass* LoadedClass = Cast<UClass>(Path.ResolveObject()))
					PreloadedClassRefs.AddUnique(LoadedClass);
			}
		}));
	}
}

void UC_BBKGameInstance::StartGame()
{
	if (bIsTransitioning) return;

	UDA_LevelSequence* Sequence = LevelSequence.LoadSynchronous();
	if (!Sequence || !Sequence->IsValidIndex(0)) return;

	bIsTransitioning = true;
	CurrentLevelIndex = 0;
	const FLevelEntry& Entry = Sequence->Levels[0];
	ShowLoadingOverlay(Entry);
	UGameplayStatics::OpenLevelBySoftObjectPtr(GetWorld(), Entry.Level);
}

void UC_BBKGameInstance::TravelToMainMenu()
{
	if (!MainMenuLevel.IsNull())
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(GetWorld(), MainMenuLevel);
	}
}

void UC_BBKGameInstance::TravelToNextLevel()
{
	if (bIsTransitioning) return;

	UDA_LevelSequence* Sequence = LevelSequence.LoadSynchronous();
	if (!Sequence) return;

	if (!Sequence->HasNextLevel(CurrentLevelIndex))
	{
		OnGameClear.Broadcast();
		return;
	}

	bIsTransitioning = true;

	// 레벨 이동 전 캐릭터 상태 저장
	if (APlayerController* PC = GetFirstLocalPlayerController(GetWorld()))
	{
		if (AC_PlayerController* BBK_PC = Cast<AC_PlayerController>(PC))
			BBK_PC->SaveStateForLevelTransition();
	}

	CurrentLevelIndex++;
	const FLevelEntry* Entry = Sequence->GetLevelEntry(CurrentLevelIndex);
	if (!Entry)
	{
		bIsTransitioning = false;
		return;
	}

	// 뷰포트 오버레이 표시 → OpenLevel → 레벨 로드 중 오버레이 유지
	ShowLoadingOverlay(*Entry);
	UGameplayStatics::OpenLevelBySoftObjectPtr(GetWorld(), Entry->Level);
}

void UC_BBKGameInstance::ApplyVolumeSettings()
{
	if (UC_BBKGameUserSettings* Settings = UC_BBKGameUserSettings::Get())
		Settings->ApplyVolumeSettings(this, GameSoundMix, SC_Master, SC_BGM, SC_SFX);
}

void UC_BBKGameInstance::LoadComplete(const float LoadTime, const FString& MapName)
{
	Super::LoadComplete(LoadTime, MapName);
	bIsTransitioning = false;

	ApplyVolumeSettings();

	// Ticker를 먼저 중단 — 이후 시간 기반 계산이 100%를 덮어쓰는 것을 방지
	FTSTicker::GetCoreTicker().RemoveTicker(ProgressTickerHandle);
	ProgressTickerHandle = FTSTicker::FDelegateHandle();

	// 로딩 완료 → 100%로 점프
	if (SharedLoadingProgress.IsValid())
		*SharedLoadingProgress = 1.0f;
	if (IsValid(LoadingScreenWidgetInstance))
		LoadingScreenWidgetInstance->SetLoadingProgress(1.0f);

	if (LoadingScreenOverlay.IsValid())
	{
		float Elapsed = (float)(FPlatformTime::Seconds() - LoadingStartTime);
		// MinLoadingTime 남은 시간을 기다리되, 최소 0.3초는 100% 상태를 표시
		float HideDelay = FMath::Max(PendingMinLoadingTime - Elapsed, 0.3f);

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				HideOverlayTimerHandle,
				this,
				&UC_BBKGameInstance::HideLoadingOverlay,
				HideDelay,
				false
			);
		}
		else
		{
			HideLoadingOverlay();
		}
	}
}

// ─────────────────────────────────────────────
// 로딩 오버레이
// ─────────────────────────────────────────────

void UC_BBKGameInstance::ShowLoadingOverlay(const FLevelEntry& Entry)
{
	if (!GEngine || !GEngine->GameViewport) return;

	HideLoadingOverlay();

	PendingMinLoadingTime = Entry.MinLoadingTime;
	LoadingStartTime      = FPlatformTime::Seconds();

	// 진행도 공유 변수 초기화 (0.0)
	SharedLoadingProgress = MakeShared<float>(0.0f);

	// ── UMG 위젯 우선 시도 ─────────────────────────────────────
	if (LoadingScreenWidgetClass)
	{
		LoadingScreenWidgetInstance = CreateWidget<UC_LoadingScreenWidget>(this, LoadingScreenWidgetClass);
		if (LoadingScreenWidgetInstance)
		{
			LoadingScreenWidgetInstance->InitializeLoadingScreen(Entry);
			LoadingScreenOverlay = LoadingScreenWidgetInstance->TakeWidget();
		}
	}

	// ── UMG 미할당 또는 생성 실패 시 Slate 폴백 ──────────────────
	if (!LoadingScreenOverlay.IsValid())
	{
		// Slate SProgressBar도 SharedLoadingProgress로 동기화
		TSharedPtr<float> ProgressRef = SharedLoadingProgress;

		LoadingScreenOverlay = SNew(SOverlay)
			+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			[
				SNew(SColorBlock).Color(FLinearColor::Black)
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Entry.LevelDescription)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 28))
				.ColorAndOpacity(FSlateColor(FLinearColor::White))
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, 100.f)
			[
				SNew(STextBlock)
				.Text(Entry.LoadingTip)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f, 1.f)))
			]
			+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom).Padding(50.f, 0.f, 50.f, 50.f)
			[
				SNew(SProgressBar)
				.Percent_Lambda([ProgressRef]() -> TOptional<float>
				{
					return ProgressRef.IsValid()
						? TOptional<float>(*ProgressRef)
						: TOptional<float>(0.f);
				})
			];
	}

	GEngine->GameViewport->AddViewportWidgetContent(LoadingScreenOverlay.ToSharedRef(), 100);

	// ── FTSTicker: 월드 무관 매 프레임 진행도 갱신 ────────────────
	// 0% → 90%: MinLoadingTime 동안 선형 증가
	// LoadComplete에서 100%로 점프
	ProgressTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this](float DeltaTime) -> bool
		{
			float Elapsed = float(FPlatformTime::Seconds() - LoadingStartTime);
			float Progress = FMath::Min(Elapsed / FMath::Max(PendingMinLoadingTime, 1.0f) * 0.9f, 0.9f);

			if (SharedLoadingProgress.IsValid())
				*SharedLoadingProgress = Progress;

			if (IsValid(LoadingScreenWidgetInstance))
				LoadingScreenWidgetInstance->SetLoadingProgress(Progress);

			return true;
		}),
		0.0f
	);
}

void UC_BBKGameInstance::HideLoadingOverlay()
{
	// 틱커 중단 — 반드시 위젯 제거 전에 처리
	FTSTicker::GetCoreTicker().RemoveTicker(ProgressTickerHandle);
	ProgressTickerHandle = FTSTicker::FDelegateHandle();

	if (LoadingScreenOverlay.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(LoadingScreenOverlay.ToSharedRef());
		LoadingScreenOverlay.Reset();
	}

	LoadingScreenWidgetInstance = nullptr;
	SharedLoadingProgress.Reset();
}

// ─────────────────────────────────────────────
// 캐릭터 상태 저장 / 복원
// ─────────────────────────────────────────────

void UC_BBKGameInstance::SaveGameState(const TArray<AC_BasePlayerCharactor*>& Roster,
	int32 ActiveIndex, UAbilitySystemComponent* SharedASC)
{
	PersistedState = FPersistentGameState();
	PersistedState.characterStates.SetNum(Roster.Num());
	PersistedState.activeCharacterIndex = ActiveIndex;

	for (int32 i = 0; i < Roster.Num(); i++)
	{
		AC_BasePlayerCharactor* Char = Roster[i];
		FPersistentCharacterState& State = PersistedState.characterStates[i];
		if (!Char) continue;

		State.bIsDead = Char->bIsDead;

		if (i == ActiveIndex)
		{
			State.health  = Char->GetHealth();
			State.stamina = Char->GetStamina();
			State.shield  = Char->GetShield();
			State.mana    = Char->GetMana();
		}
		else
		{
			State.health  = Char->GetSavedHealthValue();
			State.stamina = Char->GetSavedStaminaValue();
			State.shield  = Char->GetSavedShieldValue();
			State.mana    = Char->GetSavedManaValue();
		}

		if (UC_SkillManagerComponent* SM = Char->FindComponentByClass<UC_SkillManagerComponent>())
			State.activeSkillIndex = SM->activeSkillIndex;
	}

	if (SharedASC)
	{
		if (const UC_ChracterAttributeSetBase* AS = SharedASC->GetSet<UC_ChracterAttributeSetBase>())
		{
			PersistedState.experience     = AS->Getexperience();
			PersistedState.characterLevel = AS->Getlevel();
			PersistedState.maxExperience  = AS->GetmaxExperience();
			PersistedState.maxHealth      = AS->GetmaxHealth();
			PersistedState.maxStamina     = AS->GetmaxStamina();
			PersistedState.damage         = AS->Getdamage();
		}

		if (AActor* Owner = SharedASC->GetOwner())
		{
			if (UC_LevelUpPerkComponent* Perk = Owner->FindComponentByClass<UC_LevelUpPerkComponent>())
			{
				PersistedState.perkElements = Perk->GetElementState();
				PersistedState.perkCrit     = Perk->GetCritState();
			}
		}
	}

	PersistedState.bHasSavedState = true;
}

void UC_BBKGameInstance::RestoreGameState(TArray<AC_BasePlayerCharactor*>& Roster,
	int32 ActiveIndex, UAbilitySystemComponent* SharedASC)
{
	if (!PersistedState.bHasSavedState) return;

	if (SharedASC)
	{
		SharedASC->SetNumericAttributeBase(
			UC_ChracterAttributeSetBase::GetexperienceAttribute(), PersistedState.experience);
		SharedASC->SetNumericAttributeBase(
			UC_ChracterAttributeSetBase::GetlevelAttribute(), PersistedState.characterLevel);
		SharedASC->SetNumericAttributeBase(
			UC_ChracterAttributeSetBase::GetmaxExperienceAttribute(), PersistedState.maxExperience);
		if (PersistedState.maxHealth  >= 0.f)
			SharedASC->SetNumericAttributeBase(UC_ChracterAttributeSetBase::GetmaxHealthAttribute(),  PersistedState.maxHealth);
		if (PersistedState.maxStamina >= 0.f)
			SharedASC->SetNumericAttributeBase(UC_ChracterAttributeSetBase::GetmaxStaminaAttribute(), PersistedState.maxStamina);
		if (PersistedState.damage     >= 0.f)
			SharedASC->SetNumericAttributeBase(UC_ChracterAttributeSetBase::GetdamageAttribute(),     PersistedState.damage);
	}

	if (AActor* Owner = SharedASC->GetOwner())
	{
		if(UC_LevelUpPerkComponent* Perk = Owner->FindComponentByClass<UC_LevelUpPerkComponent>())
		{
			Perk->RestoreElementState(PersistedState.perkElements);
			Perk->RestoreCritState(PersistedState.perkCrit);
		}
	}

	for (int32 i = 0; i < Roster.Num() && i < PersistedState.characterStates.Num(); i++)
	{
		AC_BasePlayerCharactor* Char = Roster[i];
		const FPersistentCharacterState& State = PersistedState.characterStates[i];
		if (!Char) continue;

		if (State.bIsDead)
		{
			Char->bIsDead = true;
			Char->SetActorHiddenInGame(true);
			Char->SetActorEnableCollision(false);
			continue;
		}

		if (i == ActiveIndex)
		{
			if (SharedASC && State.health >= 0.f)
			{
				using AS = UC_ChracterAttributeSetBase;
				if (State.health  >= 0.f) SharedASC->SetNumericAttributeBase(AS::GethealthAttribute(),  State.health);
				if (State.stamina >= 0.f) SharedASC->SetNumericAttributeBase(AS::GetstaminaAttribute(), State.stamina);
				SharedASC->SetNumericAttributeBase(AS::GetshieldAttribute(), State.shield);
				SharedASC->SetNumericAttributeBase(AS::GetmanaAttribute(),   State.mana);
			}

			if (State.activeSkillIndex > 0)
			{
				if (UC_SkillManagerComponent* SM = Char->FindComponentByClass<UC_SkillManagerComponent>())
					SM->SwitchCommonSkill(State.activeSkillIndex);
			}
		}
		else
		{
			Char->InjectPreSavedState(State.health, State.stamina, State.shield, State.mana);
		}
	}

	PersistedState.bHasSavedState = false;
}
