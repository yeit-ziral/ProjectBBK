// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "C_SkillIconWidget.generated.h"

class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;
class UAbilitySystemComponent;
class UC_SkillBase;

UCLASS()
class PROJECTBBK_API UC_SkillIconWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// ������
	UC_SkillIconWidget(const FObjectInitializer& ObjectInitializer);

protected:
	// �ʱ�ȭ (BeginPlay ���� ��)
	virtual void NativeConstruct() override;

	// �� ������ ������Ʈ
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// ===== UI ������ (��������Ʈ���� ���ε�) =====
	UPROPERTY(meta = (BindWidget))
	UImage* img_SkillIcon;

	UPROPERTY(meta = (BindWidget))
	UImage* img_CooldownOverlay;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* txt_Cooldown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Config")
	FGameplayTag SkillTag;

	UFUNCTION(BlueprintCallable, Category = "Skill Icon")
	void SetSkillIcon(UTexture2D* IconTexture);

	UFUNCTION(BlueprintCallable, Category = "Skill Icon")
	void UpdateCooldown(float CurrentCooldown, float MaxCooldown);

	UFUNCTION(BlueprintCallable, Category = "Skill Icon")
	void SetCooldownVisible(bool bShow);

	UFUNCTION(BlueprintCallable, Category = "Skill Icon")
	UC_SkillBase* FindAbilityByTag(UAbilitySystemComponent* ASC, FGameplayTag SearchTag);

	UFUNCTION(BlueprintCallable, Category = "Skill Icon")
	void InitializeSkillIcon(UAbilitySystemComponent* ASC);

	// CommonSkill 전용 — SkillWheel 교체 및 캐릭터 교체 시 직접 어빌리티 인스턴스로 초기화
	UFUNCTION(BlueprintCallable, Category = "Skill Icon")
	void InitializeFromCommonSkill(UC_SkillBase* NewSkill);

protected:
	UPROPERTY()
	UMaterialInstanceDynamic* cooldownMaterial;

	float currentCooldownTime;

	float maxCooldownTime;

	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	UC_SkillBase* SkillAbility;

	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	UAbilitySystemComponent* CachedASC;

private:
	UFUNCTION()
	void OnCooldownStarted(float CooldownDuration, FGameplayTag InSkillTag, FGameplayTag CooldownTag);
};
