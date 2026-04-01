#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NiagaraSystem.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "SkillData.generated.h"

//Skill type
UENUM(BlueprintType)
enum class ESkillType : uint8
{
	None		UMETA(DisplayName = "None"),
	UniqueSkill	UMETA(DisplayName = "Unique Skill"),
	CommonSkill	UMETA(DisplayName = "Common Skill"),
	Ultimate	UMETA(DisplayName = "Ultimate")
};

//Skill Data Struct
USTRUCT(BlueprintType)
struct PROJECTBBK_API FSkillData : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill Info")
	FName skillID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill Info")
	FText skillName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill Info")
	FText description;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Skill Info")
	ESkillType skillType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS|Tags")
	FGameplayTag skillTag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS|Ability")
	TSubclassOf<class UGameplayAbility> abilityClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS|Effects")
	TArray<TSoftClassPtr<UGameplayEffect>> effectsToSelf;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GAS|Effects")
	TArray<TSoftClassPtr<UGameplayEffect>> effectsToTarget;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UI")
	UTexture2D* skillIcon;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Cooldown")
	float cooldown = 10.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Damage")
	float baseDamage = 50.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Damage")
	float damageMultiplier = 1.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Range")
	float range = 500.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Range")
	float radius = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Range")
	float zoneDuration = 6.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	UAnimMontage* castAnimation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VFX")
	UNiagaraSystem* castEffect;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VFX")
	UNiagaraSystem* impactEffect;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sound")
	USoundBase* castSound;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sound")
	USoundBase* impactSound;

	FSkillData()
		: skillID(NAME_None)
		, skillName(FText::FromString("Default Skill"))
		, description(FText::FromString("Default skill description"))
		, skillType(ESkillType::None)
		, skillTag(FGameplayTag())
		, abilityClass(nullptr)
		, skillIcon(nullptr)
		, cooldown(10.0f)
		, baseDamage(50.0f)
		, damageMultiplier(1.0f)
		, range(500.0f)
		, radius(100.0f)
		, zoneDuration(6.0f)
		, castAnimation(nullptr)
		, castEffect(nullptr)
		, impactEffect(nullptr)
		, castSound(nullptr)
		, impactSound(nullptr)
	{
	}
};