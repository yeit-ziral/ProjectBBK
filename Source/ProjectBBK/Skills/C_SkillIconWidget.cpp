// Fill out your copyright notice in the Description page of Project Settings.


#include "C_SkillIconWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "AbilitySystemComponent.h"
#include "C_SkillBase.h"
#include "AbilitySystemGlobals.h"
#include "SkillData.h"

UC_SkillIconWidget::UC_SkillIconWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	currentCooldownTime = 0.0f;
	maxCooldownTime = 0.0f;
	cooldownMaterial = nullptr;
	SkillAbility = nullptr;
	CachedASC = nullptr;
}

void UC_SkillIconWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!img_SkillIcon)
	{
		UE_LOG(LogTemp, Error, TEXT("Img_SkillIcon is not bound!"));
	}

	if (!img_CooldownOverlay)
	{
		UE_LOG(LogTemp, Error, TEXT("Img_CooldownOverlay is not bound!"));
	}

	if (!txt_Cooldown)
	{
		UE_LOG(LogTemp, Error, TEXT("Txt_Cooldown is not bound!"));
	}

	if (img_CooldownOverlay)
	{
		UMaterialInterface* Material = img_CooldownOverlay->GetDynamicMaterial();
		if (Material)
		{
			cooldownMaterial = UMaterialInstanceDynamic::Create(Material, this);
			img_CooldownOverlay->SetBrushFromMaterial(cooldownMaterial);

			UE_LOG(LogTemp, Log, TEXT("Cooldown material created"));
		}
	}

	SetCooldownVisible(false);

	if (SkillTag.IsValid())
	{
		// Owner로부터 ASC 가져오기
		APawn* OwnerPawn = GetOwningPlayerPawn();
		if (OwnerPawn)
		{
			UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerPawn);
			if (ASC)
			{
				InitializeSkillIcon(ASC);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[C_SkillIconWidget] ASC not found on owner pawn!"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[C_SkillIconWidget] SkillTag not set!"));
	}

}

void UC_SkillIconWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (currentCooldownTime > 0.0f)
	{
		currentCooldownTime -= InDeltaTime;

		if (currentCooldownTime <= 0.0f)
		{
			currentCooldownTime = 0.0f;
			SetCooldownVisible(false);
		}
		else
		{
			UpdateCooldown(currentCooldownTime, maxCooldownTime);
		}
	}
}

void UC_SkillIconWidget::SetSkillIcon(UTexture2D* IconTexture)
{
	if (img_SkillIcon && IconTexture)
	{
		img_SkillIcon->SetBrushFromTexture(IconTexture);

		UE_LOG(LogTemp, Log, TEXT("Skill icon set: %s"), *IconTexture->GetName());
	}
}

void UC_SkillIconWidget::UpdateCooldown(float CurrentCooldown, float MaxCooldown)
{
	if (MaxCooldown <= 0.0f) return;

	currentCooldownTime = CurrentCooldown;
	maxCooldownTime = MaxCooldown;

	// 진행률 계산 (0.0 ~ 1.0)
	float Progress = 1.0f - (CurrentCooldown / MaxCooldown);

	// 원형 머티리얼 업데이트
	if (cooldownMaterial)
	{
		cooldownMaterial->SetScalarParameterValue(TEXT("Progress"), Progress);
	}

	// 텍스트 업데이트
	if (txt_Cooldown)
	{
		int32 Seconds = FMath::CeilToInt(CurrentCooldown);
		FText CooldownText = FText::AsNumber(Seconds);
		txt_Cooldown->SetText(CooldownText);
	}

	// 쿨타임 중이면 표시
	if (CurrentCooldown > 0.0f)
	{
		SetCooldownVisible(true);
	}
}

void UC_SkillIconWidget::SetCooldownVisible(bool bShow)
{
	if (img_CooldownOverlay)
	{
		ESlateVisibility OverlayVisibility = bShow ?
			ESlateVisibility::Visible :
			ESlateVisibility::Collapsed;

		img_CooldownOverlay->SetVisibility(OverlayVisibility);
	}

	if (txt_Cooldown)
	{
		ESlateVisibility OverlayVisibility = bShow ?
			ESlateVisibility::Visible :
			ESlateVisibility::Collapsed;

		txt_Cooldown->SetVisibility(OverlayVisibility);
	}
}

UC_SkillBase* UC_SkillIconWidget::FindAbilityByTag(UAbilitySystemComponent* ASC, FGameplayTag SearchTag)
{
	if (!ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("[C_SkillIconWidget] ASC is null!"));
		return nullptr;
	}

	if (!SearchTag.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[C_SkillIconWidget] SearchTag is invalid!"));
		return nullptr;
	}

	// ⭐ Get Activatable Abilities (C++에서만 가능!)
	TArray<FGameplayAbilitySpec>& ActivatableAbilities = ASC->GetActivatableAbilities();

	UE_LOG(LogTemp, Log, TEXT("[C_SkillIconWidget] Searching for ability with tag: %s"), *SearchTag.ToString());
	UE_LOG(LogTemp, Log, TEXT("[C_SkillIconWidget] Total abilities: %d"), ActivatableAbilities.Num());

	// 모든 Ability 순회
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities)
	{
		if (!Spec.Ability)
		{
			continue;
		}

		// C_SkillBase로 캐스팅
		UC_SkillBase* SkillBase = Cast<UC_SkillBase>(Spec.Ability);
		if (!SkillBase)
		{
			UE_LOG(LogTemp, Log, TEXT("[C_SkillIconWidget] Ability is not C_SkillBase: %s"),
				*Spec.Ability->GetName());
			continue;
		}

		// Skill Data 가져오기
		FSkillData SkillData;
		if (SkillBase->GetSkillData(SkillData))
		{
			UE_LOG(LogTemp, Log, TEXT("[C_SkillIconWidget] Checking ability: %s, Tag: %s"),
				*SkillData.skillName.ToString(),
				*SkillData.SkillTag.ToString());

			// Tag 비교
			if (SkillData.SkillTag.MatchesTagExact(SearchTag))
			{
				UE_LOG(LogTemp, Log, TEXT("[C_SkillIconWidget] ✅ Found matching ability: %s"),
					*SkillData.skillName.ToString());
				return SkillBase;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[C_SkillIconWidget] Failed to get skill data from: %s"),
				*SkillBase->GetName());
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[C_SkillIconWidget] ❌ No ability found with tag: %s"),
		*SearchTag.ToString());
	return nullptr;
}

void UC_SkillIconWidget::InitializeSkillIcon(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("[C_SkillIconWidget] ASC is null!"));
		return;
	}

	CachedASC = ASC;

	// SkillTag로 Ability 찾기
	UC_SkillBase* FoundSkillAbility = FindAbilityByTag(ASC, SkillTag);
	if (!FoundSkillAbility)
	{
		UE_LOG(LogTemp, Warning, TEXT("[C_SkillIconWidget] Skill ability not found for tag: %s"),
			*SkillTag.ToString());
		return;
	}

	SkillAbility = FoundSkillAbility;

	// 스킬 데이터 가져오기
	FSkillData SkillData;
	if (SkillAbility->GetSkillData(SkillData))
	{
		// 아이콘 설정
		SetSkillIcon(SkillData.skillIcon);

		UE_LOG(LogTemp, Log, TEXT("[C_SkillIconWidget] Skill initialized: %s"),
			*SkillData.skillName.ToString());
	}

	// 쿨다운 델리게이트 바인딩 ⭐
	SkillAbility->OnCooldownStarted.AddDynamic(this, &UC_SkillIconWidget::OnCooldownStarted);

	UE_LOG(LogTemp, Log, TEXT("[C_SkillIconWidget] Cooldown delegate bound!"));
}

void UC_SkillIconWidget::OnCooldownStarted(float CooldownDuration, FGameplayTag InSkillTag, FGameplayTag CooldownTag)
{
	if (!InSkillTag.MatchesTagExact(SkillTag))
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[C_SkillIconWidget] Cooldown started! Duration: %.1f"), CooldownDuration);

	// 쿨다운 시작
	currentCooldownTime = CooldownDuration;
	maxCooldownTime = CooldownDuration;

	SetCooldownVisible(true);
	UpdateCooldown(currentCooldownTime, maxCooldownTime);
}
