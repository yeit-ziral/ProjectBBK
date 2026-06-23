#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "ItemData.generated.h"

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
struct PROJECTBBK_API FConsumableItemData : public FBaseItemData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Consumable")
	TSubclassOf<UGameplayEffect> consumeEffect;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Consumable")
	FGameplayTag magnitudeTag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Consumable")
	float magnitude = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Consumable")
	int32 maxStack = 99;

	FConsumableItemData()
		: consumeEffect(nullptr)
		, magnitudeTag(FGameplayTag())
		, magnitude(0.f)
		, maxStack(99)
	{
	}
};

USTRUCT(BlueprintType)
struct PROJECTBBK_API FEquipmentItemData : public FBaseItemData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Equipment")
	EEquipmentSlot equipSlot = EEquipmentSlot::None;

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
		, bonusMaxHealth(0.f)
		, bonusMaxStamina(0.f)
		, bonusMoveSpeed(0.f)
		, bonusDefense(0.f)
		, bonusDamage(0.f)
	{
	}
};
