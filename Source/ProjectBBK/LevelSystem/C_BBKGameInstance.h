// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Containers/Ticker.h"
#include "LevelSequenceData.h"
#include "C_LoadingScreenWidget.h"
#include "C_BBKGameInstance.generated.h"

class AC_BasePlayerCharactor;
class AC_PlayerController;
class UAbilitySystemComponent;
class USoundMix;
class USoundClass;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameClearDelegate);

UCLASS()
class PROJECTBBK_API UC_BBKGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UPROPERTY(BlueprintAssignable, Category = "Level")
	FOnGameClearDelegate OnGameClear;

	UFUNCTION(BlueprintCallable, Category = "Level")
	void StartGame();

	// 메인 메뉴 "튜토리얼" 버튼용 진입점. 튜토리얼 레벨은 LevelSequence 밖에 있으므로
	// CurrentLevelIndex를 -1로 두어, 튜토리얼 포탈의 TravelToNextLevel()이 자연스럽게
	// Levels[0](본편 첫 레벨)로 이어지게 한다.
	UFUNCTION(BlueprintCallable, Category = "Level")
	void StartTutorial();

	UFUNCTION(BlueprintCallable, Category = "Level")
	void TravelToNextLevel();

	UFUNCTION(BlueprintCallable, Category = "Level")
	void TravelToMainMenu();

	UFUNCTION(BlueprintPure, Category = "Level")
	int32 GetCurrentLevelIndex() const { return CurrentLevelIndex; }

	UFUNCTION(BlueprintCallable, Category = "Level")
	void SetCurrentLevelIndex(int32 NewIndex) { CurrentLevelIndex = NewIndex; }

	UFUNCTION(BlueprintPure, Category = "Level")
	const UDA_LevelSequence* GetLevelSequence() const { return LevelSequence.Get(); }

	void SaveGameState(const TArray<AC_BasePlayerCharactor*>& Roster,
		int32 ActiveIndex, UAbilitySystemComponent* SharedASC);

	void RestoreGameState(TArray<AC_BasePlayerCharactor*>& Roster,
		int32 ActiveIndex, UAbilitySystemComponent* SharedASC);

	bool HasSavedState()                const { return PersistedState.bHasSavedState;       }
	int32 GetSavedActiveCharacterIndex() const { return PersistedState.activeCharacterIndex; }

	void ApplyVolumeSettings();

protected:
	virtual void LoadComplete(const float LoadTime, const FString& MapName) override;

private:
	// 뷰포트에 로딩 오버레이 표시 / 제거
	void ShowLoadingOverlay(const FLevelEntry& Entry);

	UFUNCTION()
	void HideLoadingOverlay();

	// ── 레벨 시퀀스 ──────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "Level")
	TSoftObjectPtr<UDA_LevelSequence> LevelSequence;

	// 메인 메뉴 레벨 — BP_GameInstance에서 할당
	UPROPERTY(EditDefaultsOnly, Category = "Level")
	TSoftObjectPtr<UWorld> MainMenuLevel;

	// 튜토리얼 레벨 — BP_GameInstance에서 할당. LevelSequence의 Levels 배열에는 넣지 않는다.
	// FLevelEntry를 쓰므로 로딩 화면 문구·팁·최소 표시 시간을 본편 레벨과 동일하게 지정할 수 있다.
	UPROPERTY(EditDefaultsOnly, Category = "Level")
	FLevelEntry TutorialLevel;

	int32 CurrentLevelIndex = 0;
	bool  bIsTransitioning  = false;

	// 현재 튜토리얼 레벨을 플레이 중인지 — 본편으로 넘어갈 때 상태를 저장하지 않고 비우기 위함
	bool  bInTutorial       = false;

	// ── 로딩 오버레이 ─────────────────────────────────────
	// WBP_LoadingScreen 클래스 할당 — 미할당 시 순수 Slate 폴백 사용
	UPROPERTY(EditDefaultsOnly, Category = "Level|UI")
	TSubclassOf<UC_LoadingScreenWidget> LoadingScreenWidgetClass;

	// 레벨 전환 중 GC 방지를 위해 GameInstance가 직접 소유
	UPROPERTY()
	UC_LoadingScreenWidget* LoadingScreenWidgetInstance = nullptr;

	TSharedPtr<SWidget> LoadingScreenOverlay;
	float  PendingMinLoadingTime = 0.f;
	double LoadingStartTime      = 0.0;
	FTimerHandle HideOverlayTimerHandle;

	// 진행도 공유 변수 — Slate 속성 람다와 UMG SetPercent 양쪽에서 참조
	TSharedPtr<float> SharedLoadingProgress;
	// FTSTicker 핸들 — HideLoadingOverlay에서 반드시 제거
	FTSTicker::FDelegateHandle ProgressTickerHandle;

	// ── 캐릭터 상태 저장 ──────────────────────────────────
	FPersistentGameState PersistedState;

	// ── 볼륨 설정 ─────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundMix* GameSoundMix = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundClass* SC_Master = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundClass* SC_BGM = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundClass* SC_SFX = nullptr;

	// ── 에셋 프리로드 (GC 방지) ───────────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "Preload")
	TArray<TSoftClassPtr<AActor>> ActorClassesToPreload;

	UPROPERTY()
	TArray<UClass*> PreloadedClassRefs;
};
