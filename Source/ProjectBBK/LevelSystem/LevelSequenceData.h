// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "../PlayerCharacter/C_PerkData.h"
#include "LevelSequenceData.generated.h"

USTRUCT(BlueprintType)
struct FLevelEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TSoftObjectPtr<UWorld> Level;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TSoftObjectPtr<USoundBase> BGM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TSoftObjectPtr<UTexture2D> LoadingTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	FText LevelDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	FText LoadingTip;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	float MinLoadingTime = 2.0f;
};

// 레벨 간 유지할 캐릭터 1인의 상태
USTRUCT()
struct FPersistentCharacterState
{
	GENERATED_BODY()

	float health  = -1.f;  // -1 = 저장 없음 (기본 최대값 사용)
	float stamina = -1.f;
	float shield  = 0.f;
	float mana    = 0.f;
	bool  bIsDead = false;
	int32 activeSkillIndex = 0;
};

// 레벨 이동 시 GameInstance가 보관하는 전체 게임 상태
USTRUCT()
struct FPersistentGameState
{
	GENERATED_BODY()

	TArray<FPersistentCharacterState> characterStates;
	float experience          = 0.f;
	float characterLevel      = 1.f;
	float maxExperience       = 100.f;
	float maxHealth           = -1.f;   // -1 = 저장 없음 (기본값 사용)
	float maxStamina          = -1.f;
	float damage              = -1.f;
	int32 activeCharacterIndex = 0;
	bool  bHasSavedState      = false;
	TMap<FGameplayTag, FElementState> perkElements;
	FCritState perkCrit;
};

UCLASS(BlueprintType)
class PROJECTBBK_API UDA_LevelSequence : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Levels")
	TArray<FLevelEntry> Levels;

	UFUNCTION(BlueprintPure, Category = "Levels")
	bool IsValidIndex(int32 Index) const { return Levels.IsValidIndex(Index); }

	UFUNCTION(BlueprintPure, Category = "Levels")
	bool HasNextLevel(int32 CurrentIndex) const { return Levels.IsValidIndex(CurrentIndex + 1); }

	// C++ 전용 — Blueprint에 노출하지 않음 (raw pointer는 BP에서 사용 불가)
	const FLevelEntry* GetLevelEntry(int32 Index) const
	{
		return Levels.IsValidIndex(Index) ? &Levels[Index] : nullptr;
	}
};
