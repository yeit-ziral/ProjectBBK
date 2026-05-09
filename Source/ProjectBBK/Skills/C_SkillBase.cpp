// Fill out your copyright notice in the Description page of Project Settings.


#include "C_SkillBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/AssetManager.h"
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

	// Ability가 부여될 때 Skill Data 로드 후 GE 에셋 선행 로드
	if (LoadSkillData())
	{
		PreloadSkillAssets();
	}
}

bool UC_SkillBase::LoadSkillData()
{
	if (!SkillDataTable || SkillRowName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[C_SkillBase] SkillDataTable or SkillRowName not set! Check BP defaults: %s"), *GetName());
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

void UC_SkillBase::PreloadSkillAssets()
{
	TArray<FSoftObjectPath> AssetsToLoad;

	for (const TSoftClassPtr<UGameplayEffect>& SoftClass : CachedSkillData.effectsToSelf)
	{
		if (!SoftClass.IsNull())
			AssetsToLoad.Add(SoftClass.ToSoftObjectPath());
	}

	for (const TSoftClassPtr<UGameplayEffect>& SoftClass : CachedSkillData.effectsToTarget)
	{
		if (!SoftClass.IsNull())
			AssetsToLoad.Add(SoftClass.ToSoftObjectPath());
	}

	if (AssetsToLoad.Num() == 0) return;

	SkillAssetHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		AssetsToLoad,
		FStreamableDelegate::CreateLambda([this]()
		{
			UE_LOG(LogTemp, Log, TEXT("[C_SkillBase] Skill assets preloaded: %s"), *SkillRowName.ToString());
		})
	);
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
	// GE_GenericCooldown 하나를 모든 스킬이 공유하기 위해 태그를 런타임에 주입.
	// 이 방식 때문에 GAS 내장 CheckCooldown()이 동작하지 않으며,
	// CanActivateAbility()에서 IsOnCooldown()으로 수동 체크해야 한다.
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
				*CachedSkillData.skillTag.ToString());
			OnCooldownStarted.Broadcast(CooldownDuration, CachedSkillData.skillTag, CooldownTag);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[C_SkillBase] SkillData not loaded, cooldown UI not updated."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[C_SkillBase] Failed to apply cooldown GE!"));
	}
}

bool UC_SkillBase::QuerySkillCooldown(UAbilitySystemComponent* ASC, float& OutRemaining, float& OutDuration) const
{
	OutRemaining = 0.f;
	OutDuration = 0.f;

	if (!ASC || !CooldownTag.IsValid()) return false;

	FGameplayTagContainer CooldownTagContainer;
	CooldownTagContainer.AddTag(CooldownTag);

	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTagContainer);
	TArray<TPair<float, float>> Results = ASC->GetActiveEffectsTimeRemainingAndDuration(Query);

	if (Results.Num() == 0) return false;

	OutRemaining = Results[0].Key;
	OutDuration = Results[0].Value;

	return OutRemaining > 0.f;
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


void UC_SkillBase::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	// 의도적으로 비워둠. 쿨다운은 ApplySkillCooldown()이 담당.
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

	// [주의] 수동 쿨다운 체크가 필요한 이유:
	// GAS 내장 CheckCooldown()은 GetCooldownTags() 반환값을 기준으로 동작하는데,
	// 이 클래스는 GE_GenericCooldown 하나를 공유하고 쿨다운 태그를 DynamicGrantedTags로
	// 동적 주입하는 방식을 사용한다. GetCooldownTags()를 오버라이드하지 않았으므로
	// Super의 쿨다운 체크는 항상 통과하고, 여기서 직접 ASC의 태그를 확인해야 한다.
	// ApplyGenericCooldown() 참고.
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
	for (const TSoftClassPtr<UGameplayEffect>& SoftEffectClass : CachedSkillData.effectsToSelf)
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

}

void UC_SkillBase::ApplySkillEffectsToTargets(const TArray<AActor*>& Targets, AActor* EffectCauser)
{
	if (!bSkillDataLoaded)
	{
		UE_LOG(LogTemp, Warning, TEXT("[C_SkillBase] Skill Data not loaded! Cannot apply effects to targets."));
		return;
	}

	if (CachedSkillData.effectsToTarget.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[C_SkillBase] effectsToTarget is empty!"));
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return;
	}

	// Instigator: Controller 우선, 없으면 Avatar
	// EffectCauser: 호출 측이 지정한 오브젝트(투사체, FireZone 등), 없으면 Avatar
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	AActor* Instigator = AvatarActor;
	if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo())
	{
		if (ActorInfo->PlayerController.IsValid())
		{
			Instigator = ActorInfo->PlayerController.Get();
		}
	}
	AActor* Causer = EffectCauser ? EffectCauser : AvatarActor;

	for (AActor* Target : Targets)
	{
		if (!Target) continue;

		UAbilitySystemComponent* TargetASC =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);

		if (!TargetASC) continue;

		for (const TSoftClassPtr<UGameplayEffect>& SoftEffectClass : CachedSkillData.effectsToTarget)
		{
			TSubclassOf<UGameplayEffect> EffectClass = SoftEffectClass.LoadSynchronous();
			if (!EffectClass) continue;

			FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
			Context.AddInstigator(Instigator, Causer);

			FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(
				EffectClass,
				GetAbilityLevel(),
				Context
			);

			if (Spec.IsValid())
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
				UE_LOG(LogTemp, Log, TEXT("[C_SkillBase] Applied %s to %s"),
					*EffectClass->GetName(), *Target->GetName());
			}
		}
	}
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

TSubclassOf<UGameplayEffect> UC_SkillBase::GetTargetEffectClass(int32 Index) const
{
	if (!bSkillDataLoaded)
	{
		UE_LOG(LogTemp, Warning, TEXT("[C_SkillBase] Skill Data not loaded!"));
		return nullptr;
	}

	if (!CachedSkillData.effectsToTarget.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("[C_SkillBase] effectsToTarget index %d out of range!"), Index);
		return nullptr;
	}

	return CachedSkillData.effectsToTarget[Index].LoadSynchronous();
}