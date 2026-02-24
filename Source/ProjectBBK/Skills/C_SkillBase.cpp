// Fill out your copyright notice in the Description page of Project Settings.


#include "C_SkillBase.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h" 
#include "Animation/AnimMontage.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

UC_SkillBase::UC_SkillBase()
{
	// 기본값 설정
	SkillDataTable = nullptr;
	SkillRowName = NAME_None;
	GenericCooldownGE = nullptr;
	CooldownTag = FGameplayTag();
	bSkillDataLoaded = false;

	// Ability 기본 설정
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UC_SkillBase::OnGiveAbility(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	// Ability가 부여될 때 Skill Data 로드Fge
	LoadSkillData();
}

bool UC_SkillBase::LoadSkillData()
{
	if (!SkillDataTable || SkillRowName.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("[C_SkillBase] SkillDataTable or SkillRowName not set!"));
		return false;
	}

	// DataTable에서 Row 찾기
	FSkillData* RowData = SkillDataTable->FindRow<FSkillData>(
		SkillRowName,
		TEXT("LoadSkillData"),
		true
	);

	if (!RowData)
	{
		UE_LOG(LogTemp, Error, TEXT("[C_SkillBase] Skill Data not found! Row: %s"), *SkillRowName.ToString());
		return false;
	}

	// 캐시에 저장
	CachedSkillData = *RowData;
	bSkillDataLoaded = true;

	UE_LOG(LogTemp, Log, TEXT("[C_SkillBase] Skill Data Loaded: %s"), *CachedSkillData.skillName.ToString());

	return true;
}

bool UC_SkillBase::GetSkillData(FSkillData& OutSkillData)
{
	// 캐시된 데이터가 없으면 로드
	if (!bSkillDataLoaded)
	{
		if (!LoadSkillData())
		{
			return false;
		}
	}

	OutSkillData = CachedSkillData;
	return true;
}

void UC_SkillBase::ApplySkillCooldown()
{
	if (!bSkillDataLoaded)
	{
		UE_LOG(LogTemp, Warning, TEXT("[C_SkillBase] Skill Data not loaded! Cannot apply cooldown."));
		return;
	}

	ApplyGenericCooldown(CachedSkillData.cooldown);
}

void UC_SkillBase::ApplyGenericCooldown(float CooldownDuration)
{
	// 1. 유효성 검사
	if (!GenericCooldownGE)
	{
		UE_LOG(LogTemp, Error, TEXT("[C_SkillBase] GenericCooldownGE not set!"));
		return;
	}

	if (CooldownDuration <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[C_SkillBase] Cooldown Duration is 0! Skipping cooldown."));
		return;
	}

	if (!CooldownTag.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[C_SkillBase] CooldownTag not set!"));
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("[C_SkillBase] AbilitySystemComponent not found!"));
		return;
	}

	// 2. Make Outgoing Gameplay Effect Spec
	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
		GenericCooldownGE,
		GetAbilityLevel(),
		EffectContext
	);

	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[C_SkillBase] Failed to create GE Spec!"));
		return;
	}

	// 3. Set Duration by Caller
	FGameplayTag DurationTag = FGameplayTag::RequestGameplayTag(FName("Data.Cooldown.Duration"));
	SpecHandle.Data->SetSetByCallerMagnitude(DurationTag, CooldownDuration);

	// 4. Add Cooldown Tag dynamically
	SpecHandle.Data->DynamicGrantedTags.AddTag(CooldownTag);

	// 5. Apply!
	FActiveGameplayEffectHandle ActiveGEHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);

	if (ActiveGEHandle.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("[C_SkillBase] Cooldown Applied! Duration: %.1f, Tag: %s"),
			CooldownDuration,
			*CooldownTag.ToString()
		);

		// 6. Notify UI
		if (bSkillDataLoaded)
		{
			UE_LOG(LogTemp, Warning, TEXT("[C_SkillBase] 🔔 Broadcasting OnCooldownStarted! SkillTag: %s"),
				*CachedSkillData.SkillTag.ToString());
			OnCooldownStarted.Broadcast(CooldownDuration, CachedSkillData.SkillTag, CooldownTag);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[C_SkillBase] ❌ SkillData not loaded! Cannot broadcast!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[C_SkillBase] Failed to apply cooldown GE!"));
	}
}

bool UC_SkillBase::IsOnCooldown() const
{
	if (!CooldownTag.IsValid())
	{
		return false;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return false;
	}

	return ASC->HasMatchingGameplayTag(CooldownTag);
}


bool UC_SkillBase::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	// 부모 체크
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// 쿨다운 체크
	if (IsOnCooldown())
	{
		UE_LOG(LogTemp, Warning, TEXT("[C_SkillBase] Skill is on cooldown!"));
		return false;
	}

	return true;
}

void UC_SkillBase::ApplySkillEffects()
{
	if (!bSkillDataLoaded)
	{
		UE_LOG(LogTemp, Warning, TEXT("[C_SkillBase] Skill Data not loaded! Cannot apply effects."));
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// Apply Effects to Self ⭐
	for (const TSoftClassPtr<UGameplayEffect>& SoftEffectClass : CachedSkillData.EffectsToSelf)
	{
		// 소프트 레퍼런스 동기 로드
		TSubclassOf<UGameplayEffect> EffectClass = SoftEffectClass.LoadSynchronous();

		if (!EffectClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("[C_SkillBase] EffectClass is null after load!"));
			continue;
		}

		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
			EffectClass,
			GetAbilityLevel(),
			EffectContext
		);

		if (SpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
			UE_LOG(LogTemp, Log, TEXT("[C_SkillBase] Applied effect: %s"),
				*EffectClass->GetName());
		}
	}

	// TODO: Apply Effects to Target (타겟 시스템 구현 후)
	// for (TSubclassOf<UGameplayEffect> EffectClass : CachedSkillData.EffectsToTarget)
	// {
	//     ...
	// }
}

void UC_SkillBase::PlaySkillAnimation()
{
	if (!bSkillDataLoaded || !CachedSkillData.castAnimation)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	// Play Montage
	AnimInstance->Montage_Play(CachedSkillData.castAnimation);

	UE_LOG(LogTemp, Log, TEXT("[C_SkillBase] Playing animation: %s"),
		*CachedSkillData.castAnimation->GetName()
	);
}

void UC_SkillBase::SpawnSkillVFX()
{
	if (!bSkillDataLoaded || !CachedSkillData.castEffect)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return;
	}

	// Spawn Niagara Effect at Actor Location
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		CachedSkillData.castEffect,
		AvatarActor->GetActorLocation(),
		AvatarActor->GetActorRotation()
	);

	UE_LOG(LogTemp, Log, TEXT("[C_SkillBase] Spawned VFX: %s"),
		*CachedSkillData.castEffect->GetName()
	);
}

void UC_SkillBase::PlaySkillSound()
{
	if (!bSkillDataLoaded || !CachedSkillData.castSound)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return;
	}

	// Play Sound at Actor Location
	UGameplayStatics::PlaySoundAtLocation(
		GetWorld(),
		CachedSkillData.castSound,
		AvatarActor->GetActorLocation()
	);

	UE_LOG(LogTemp, Log, TEXT("[C_SkillBase] Playing sound: %s"),
		*CachedSkillData.castSound->GetName()
	);
}

UTexture2D* UC_SkillBase::GetSkillIcon() const
{
	return bSkillDataLoaded ? CachedSkillData.skillIcon : nullptr;
}

FText UC_SkillBase::GetSkillName() const
{
	return bSkillDataLoaded ? CachedSkillData.skillName : FText::FromString("Unknown");
}

FText UC_SkillBase::GetSkillDescription() const
{
	return bSkillDataLoaded ? CachedSkillData.description : FText::FromString("No description");
}

float UC_SkillBase::GetCooldownDuration() const
{
	return bSkillDataLoaded ? CachedSkillData.cooldown : 0.0f;
}