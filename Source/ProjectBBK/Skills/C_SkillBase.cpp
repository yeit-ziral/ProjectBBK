// Fill out your copyright notice in the Description page of Project Settings.


#include "C_SkillBase.h"
#include "C_CooldownManager.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"


UC_SkillBase::UC_SkillBase()
{
	owner = nullptr;
	cooldownManager = nullptr;
	currentState = ESkillState::Ready;
	bIsCasting = false;
	castTime = 0.0f;
	currentCastTime = 0.0f;
}

void UC_SkillBase::InitializeSkill(AActor* InOwner, const FSkillData& InSkillData)
{
	owner = InOwner;
	skillData = InSkillData;
	currentState = ESkillState::Ready;
	bIsCasting = false;

	UE_LOG(LogTemp, Log, TEXT("Skill Initialized: %s"), *skillData.skillName.ToString());
}

void UC_SkillBase::SetCooldownManager(UC_CooldownManager* Manager)
{
	cooldownManager = Manager;

	if (cooldownManager)
	{
		UE_LOG(LogTemp, Log, TEXT("CooldownManager set for skill: %s"), *skillData.skillName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("CooldownManager is null!"));
	}
}


bool UC_SkillBase::CanUseSkill() const
{
	if (!owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot use skill: No owner"));
		return false;
	}

	if (currentState != ESkillState::Ready)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot use skill: Not ready (State: %d)"), (int32)currentState);
		return false;
	}

	if (cooldownManager->IsOnCooldown(skillData.skillID))
	{
		float remaining = cooldownManager->GetRemainingCooldown(skillData.skillID);
		UE_LOG(LogTemp, Warning, TEXT("CanUseSkill: On cooldown (%.1fs remaining)"), remaining);
		return false;
	}

	if (bIsCasting)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot use skill: Already casting"));
		return false;
	}

	// 추가 조건 체크 (자식 클래스에서 오버라이드 가능)
	return true;
}

bool UC_SkillBase::CastSkill()
{
	if (!CanUseSkill())
	{
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("=== Casting Skill: %s ==="), *skillData.skillName.ToString());

	// 상태 변경
	currentState = ESkillState::Casting;
	bIsCasting = true;
	currentCastTime = 0.0f;

	OnSkillStart();

	PlaySkillAnimation();

	PlaySkillSound(0);

	// 시전 이펙트
	if (owner)
	{
		SpawnSkillEffect(0, owner->GetActorLocation());
	}

	ExecuteSkill();

	StartCooldown();

	bIsCasting = false;
	currentState = ESkillState::Cooldown;
	OnSkillEnd();

	return true;
}

void UC_SkillBase::UpdateSkill(float DeltaTime)
{
	// 시전 시간 체크 (필요시)
	if (bIsCasting)
	{
		currentCastTime += DeltaTime;

		if (castTime > 0.0f && currentCastTime >= castTime)
		{
			// 시전 완료
			bIsCasting = false;
			OnSkillEnd();
		}
	}

	if (currentState == ESkillState::Cooldown)
	{
		if (cooldownManager && !cooldownManager->IsOnCooldown(skillData.skillID))
		{
			currentState = ESkillState::Ready;
			UE_LOG(LogTemp, Log, TEXT("Skill ready: %s"), *skillData.skillName.ToString());
		}
	}
}

void UC_SkillBase::CancelSkill()
{
	if (bIsCasting)
	{
		bIsCasting = false;
		currentState = ESkillState::Ready;
		currentCastTime = 0.0f;

		OnSkillCancelled();

		UE_LOG(LogTemp, Warning, TEXT("Skill cancelled: %s"), *skillData.skillName.ToString());
	}
}

void UC_SkillBase::OnSkillStart_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("Skill Start: %s"), *skillData.skillName.ToString());
}


void UC_SkillBase::OnSkillEnd_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("Skill End: %s"), *skillData.skillName.ToString());
}

void UC_SkillBase::OnSkillCancelled_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("Skill Cancelled: %s"), *skillData.skillName.ToString());
}

void UC_SkillBase::PlaySkillAnimation()
{
	if (!skillData.castAnimation || !owner)
		return;

	ACharacter* OwnerCharacter = Cast<ACharacter>(owner);
	if (OwnerCharacter)
	{
		UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Play(skillData.castAnimation);
			UE_LOG(LogTemp, Log, TEXT("Playing animation for: %s"), *skillData.skillName.ToString());
		}
	}
}

void UC_SkillBase::SpawnSkillEffect(int32 EffectType, FVector Location)
{
	UNiagaraSystem* EffectToSpawn = nullptr;

	if (EffectType == 0)
	{
		// Cast Effect
		EffectToSpawn = skillData.castEffect;
	}
	else if (EffectType == 1)
	{
		// Impact Effect
		EffectToSpawn = skillData.impactEffect;
	}

	if (EffectToSpawn && owner)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			owner->GetWorld(),
			EffectToSpawn,
			Location,
			FRotator::ZeroRotator,
			FVector(1.0f),
			true,
			true,
			ENCPoolMethod::AutoRelease
		);

		UE_LOG(LogTemp, Log, TEXT("Spawned effect at location: %s"), *Location.ToString());
	}
}

void UC_SkillBase::PlaySkillSound(int32 SoundType)
{
	USoundBase* SoundToPlay = nullptr;

	if (SoundType == 0)
	{
		// Cast Sound
		SoundToPlay = skillData.castSound;
	}
	else if (SoundType == 1)
	{
		// Impact Sound
		SoundToPlay = skillData.impactSound;
	}

	if (SoundToPlay && owner)
	{
		UGameplayStatics::PlaySoundAtLocation(
			owner->GetWorld(),
			SoundToPlay,
			owner->GetActorLocation()
		);
	}
}

void UC_SkillBase::StartCooldown()
{
	if (!cooldownManager)
	{
		UE_LOG(LogTemp, Error, TEXT("StartCooldown: CooldownManager is null!"));
		return;
	}

	cooldownManager->StartCooldown(skillData.skillID, skillData.cooldown);
	currentState = ESkillState::Cooldown;
}

bool UC_SkillBase::IsOnCooldown() const
{
	if (!cooldownManager)
		return false;

	return cooldownManager->IsOnCooldown(skillData.skillID);
}

float UC_SkillBase::GetCurrentCooldown() const
{
	if (!cooldownManager)
		return 0.0f;

	return cooldownManager->GetRemainingCooldown(skillData.skillID);
}


float UC_SkillBase::GetCooldownPercent() const
{
	if (!cooldownManager)
		return 0.0f;

	return cooldownManager->GetCooldownPercent(skillData.skillID);
}
