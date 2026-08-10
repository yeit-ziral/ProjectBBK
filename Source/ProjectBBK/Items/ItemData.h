#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "ItemData.generated.h"

class UC_ConsumableAction;

UENUM(BlueprintType)
enum class EEquipmentClass : uint8
{
	Common  UMETA(DisplayName = "Common"),
	Melee   UMETA(DisplayName = "Melee Only"),
	Ranged  UMETA(DisplayName = "Ranged Only")
};

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	None       UMETA(DisplayName = "None"),
	Head       UMETA(DisplayName = "Head"),
	Chest      UMETA(DisplayName = "Chest"),
	Legs       UMETA(DisplayName = "Legs"),
	Weapon     UMETA(DisplayName = "Weapon"),
	Accessory  UMETA(DisplayName = "Accessory")
};

USTRUCT(BlueprintType)
struct PROJECTBBK_API FBaseItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item Info")
	FName itemID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item Info")
	FText itemName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item Info")
	FText description;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UI")
	UTexture2D* itemIcon;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World")
	UStaticMesh* worldMesh;

	FBaseItemData()
		: itemID(NAME_None)
		, itemName(FText::GetEmpty())
		, description(FText::GetEmpty())
		, itemIcon(nullptr)
		, worldMesh(nullptr)
	{
	}
};

USTRUCT(BlueprintType)
struct FConsumableEffectEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> effect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag magnitudeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float magnitude = 0.f;
};

USTRUCT(BlueprintType)
struct PROJECTBBK_API FConsumableItemData : public FBaseItemData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Consumable")
	TArray<FConsumableEffectEntry> consumeEffects;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Consumable")
	int32 maxStack = 99;

	// 사용 후 재사용까지 대기 시간(초). 0 = 쿨다운 없음.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Consumable")
	float cooldown = 0.f;

	// GE 즉시 적용만으로 표현 불가능한 동작(AOE 판정, 액터 스폰 등) 실행. None이면 consumeEffects만 적용.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Consumable")
	TSubclassOf<UC_ConsumableAction> actionClass;
};

USTRUCT(BlueprintType)
struct PROJECTBBK_API FEquipmentItemData : public FBaseItemData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Equipment")
	EEquipmentSlot equipSlot = EEquipmentSlot::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Equipment")
	EEquipmentClass equipClass = EEquipmentClass::Common;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Equipment|Stats")
	float bonusMaxHealth = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Equipment|Stats")
	float bonusMaxStamina = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Equipment|Stats")
	float bonusMoveSpeed = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Equipment|Stats")
	float bonusDefense = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Equipment|Stats")
	float bonusDamage = 0.f;

	FEquipmentItemData()
		: equipSlot(EEquipmentSlot::None)
		, equipClass(EEquipmentClass::Common)
		, bonusMaxHealth(0.f)
		, bonusMaxStamina(0.f)
		, bonusMoveSpeed(0.f)
		, bonusDefense(0.f)
		, bonusDamage(0.f)
	{
	}
};
