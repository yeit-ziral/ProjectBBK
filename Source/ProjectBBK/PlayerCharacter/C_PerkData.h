// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"      // FTableRowBase
#include "GameplayEffect.h"        // UGameplayEffect (TSubclassOf 대상)
#include "GameplayTagContainer.h"  // FGameplayTag (속성 식별)
#include "C_PerkData.generated.h"

class UTexture2D;
class UNiagaraSystem;

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
};
