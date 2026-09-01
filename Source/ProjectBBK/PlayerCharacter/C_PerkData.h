// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"      // FTableRowBase
#include "GameplayEffect.h"        // UGameplayEffect (TSubclassOf 대상)
#include "GameplayTagContainer.h"  // FGameplayTag (속성 식별)
#include "C_PerkData.generated.h"

class UTexture2D;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class EPerkType : uint8
{
	Stat UMETA(DisplayName = "Stat"),
	Element UMETA(DisplayName = "Element"),
	Crit UMETA(DisplayName = "Crit")
};

// 속성 하나의 현재 상태
USTRUCT()
struct FElementState
{
	GENERATED_BODY()

	int32 Level = 0;
	float DamagePerLevel = 0.f;
	int32 MaxLevel = 1;

	UPROPERTY()
	FLinearColor Color = FLinearColor::Black;

	UPROPERTY()
	FText DisplayName;

	UPROPERTY()
	TObjectPtr<UTexture2D> Icon = nullptr;
};

// 크리티컬 퍽의 현재 상태. FElementState와 같은 역할이고, 맵 이동 시 통째로 저장된다.
// 발동률/배율을 여기서 파생시켜 "레벨 -> 수치" 규칙이 한 군데에만 있게 한다.
USTRUCT()
struct FCritState
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Level = 0;             // 0 = 아직 미보유

	UPROPERTY()
	float ChanceBase = 0.f;

	UPROPERTY()
	float ChancePerLevel = 0.f;

	UPROPERTY()
	float MultiplierBase = 1.f;

	UPROPERTY()
	float MultiplierPerLevel = 0.f;

	UPROPERTY()
	int32 MaxLevel = 1;

	// Lv.1이 ChanceBase가 되도록 (Level-1)을 곱한다. Lv.0은 미보유이므로 0.
	float GetChance() const
	{
		return Level > 0 ? ChanceBase + (Level - 1) * ChancePerLevel : 0.f;
	}

	// 미보유일 때 1.0을 돌려주면 호출부에서 분기 없이 곱하기만 하면 된다.
	float GetMultiplier() const
	{
		return Level > 0 ? MultiplierBase + (Level - 1) * MultiplierPerLevel : 1.f;
	}
};


USTRUCT(BlueprintType)
struct FPerkDisplayInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag ElementTag;

	UPROPERTY(BlueprintReadOnly) 
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly)
	int32 Level = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 MaxLevel = 1;

	UPROPERTY(BlueprintReadOnly)
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly) 
	TObjectPtr<UTexture2D> Icon = nullptr;
};

/**
 * 레벨업 특전(선택 보상) 한 개를 나타내는 DataTable 행.
 * 새 보상을 추가하려면 코드 수정 없이 DataTable에 행만 추가하면 된다.
 */
USTRUCT(BlueprintType)
struct FPerkData : public FTableRowBase   // ← DataTable 행이 되려면 FTableRowBase 상속 필수
{
	GENERATED_BODY()

	// 이 행이 어떤 종류의 보상인지. 크리티컬 퍽을 구분하는 데 쓴다.
	// 속성 퍽은 기존대로 elementTag 유효성으로 판별하므로 기존 DT 행은 손댈 필요 없다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk")
	EPerkType perkType = EPerkType::Stat;

	// UI에 표시될 이름 (예: "발이 빨라진다")
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk")
	FText DisplayName;

	// 설명 (예: "이동 속도 +10%")
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk")
	FText Description;

	// 아이콘 (선택, 없으면 비워둠)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// 영구 적용할 GE(Infinite). 증가량은 Magnitude를 SetByCaller("Data.Perk")로 주입한다.
	// → 어트리뷰트 종류별로 GE 하나만 만들고, 수치는 데이터로만 바꾼다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk")
	TSubclassOf<UGameplayEffect> Effect;

	// SetByCaller로 GE에 주입할 증가량 (예: 이동속도 60, 방어력 5)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk")
	float Magnitude = 0.f;

	// 적용 순간 1회용 GE(Instant). 예: 최대 체력을 올린 뒤 풀회복.
	// 최대 체력은 늘려도 현재 체력이 자동으로 따라오지 않으므로 필요한 보상에만 채운다. 없으면 비움.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk")
	TSubclassOf<UGameplayEffect> OnApplyEffect;

	// 속성 추가용
	// elementTag가 유효하면 이 보상은 "속성 부여"로 처리된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk|Element")
	FGameplayTag elementTag;   

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk|Element")
	float elementDamagePerLevel = 0.f;  // 레벨당 true 데미지

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk|Element")
	int elementMaxLevel = 1;  // 속성 레벨 최대치

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk|Element")
	FLinearColor elementColor = FLinearColor::Red; // 속성 부여 시 재생할 VFX. 없으면 비움.

	// 목표 발동률, PRD 상수 C는 이 값에서 코드가 역산하므로 여기 넣지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk|Crit", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ciritChanceBase = 0.10f;  // 기본 발동률

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk|Crit")
	float critChancePerLevel = 0.05f;  // 레벨당 발동률 증가량

	// 1레벨 배율 (2.0 = 2배 데미지)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk|Crit", meta = (ClampMin = "1.0"))
	float critMultiplierBase = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk|Crit")
	float critMultiplierPerLevel = 0.15f;  // 레벨당 배율 증가량

	// 속성 퍽(elementMaxLevel = 5)과 맞출 것
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk|Crit")
	int32 critMaxLevel = 5;  // 크리티컬 레벨 최대치
};
