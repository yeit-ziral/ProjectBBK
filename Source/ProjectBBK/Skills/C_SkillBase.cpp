// Fill out your copyright notice in the Description page of Project Settings.


#include "C_SkillBase.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"

UC_SkillBase::UC_SkillBase()
{
	owner = nullptr;
	currentState = ESkillState::Ready;
	currentCooldown = 0.0f;
	bIsCasting = false;
	castTime = 0.0f;
	currentCastTime = 0.0f;
}

void UC_SkillBase::InitializeSkill(AActor* inOwner, const FSkillData& inSkillData)
{
	owner = inOwner;
	skillData = inSkillData;
	currentState = ESkillState::Ready;
	currentCooldown = 0.0f;

	UE_LOG(LogTemp, Log, TEXT("Skill Initialized: %s"), *skillData.skillName.ToString());
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

	if (IsOnCooldown())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot use skill: On cooldown (%.1f seconds remaining)"), currentCooldown);
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

void UC_SkillBase::UpdateSkill(float deltaTime)
{
	if (currentCooldown > 0.0f)
	{
		currentCooldown -= deltaTime;

		if (currentCooldown <= 0.0f)
		{
			currentCooldown = 0.0f;
			currentState = ESkillState::Ready;
			UE_LOG(LogTemp, Log, TEXT("Skill ready: %s"), *skillData.skillName.ToString());
		}
	}

	// 시전 시간 체크 (필요시)
	if (bIsCasting)
	{
		currentCastTime += deltaTime;

		if (castTime > 0.0f && currentCastTime >= castTime)
		{
			// 시전 완료
			bIsCasting = false;
			OnSkillEnd();
		}
	}
}

void UC_SkillBase::CancelSkill()
{
	if (bIsCasting)
	{
		bIsCasting = false;
		currentState = ESkillState::Ready;
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

void UC_SkillBase::SpawnSkillEffect(int32 effectType, FVector location)
{
	UNiagaraSystem* EffectToSpawn = nullptr;

	if (effectType == 0)
	{
		// Cast Effect
		EffectToSpawn = skillData.castEffect;
	}
	else if (effectType == 1)
	{
		// Impact Effect
		EffectToSpawn = skillData.impactEffect;
	}

	if (EffectToSpawn && owner)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			owner->GetWorld(),
			EffectToSpawn,
			location,
			FRotator::ZeroRotator,
			FVector(1.0f),
			true,
			true,
			ENCPoolMethod::AutoRelease
		);

		UE_LOG(LogTemp, Log, TEXT("Spawned effect at location: %s"), *location.ToString());
	}
}

void UC_SkillBase::PlaySkillSound(int32 soundType)
{
	USoundBase* SoundToPlay = nullptr;

	if (soundType == 0)
	{
		// Cast Sound
		SoundToPlay = skillData.castSound;
	}
	else if (soundType == 1)
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
	currentCooldown = skillData.cooldown;
	currentState = ESkillState::Cooldown;

	UE_LOG(LogTemp, Log, TEXT("Cooldown started: %s (%.1f seconds)"), *skillData.skillName.ToString(), currentCooldown);
}

float UC_SkillBase::GetCooldownPercent() const
{
	if (skillData.cooldown <= 0.0f)
		return 0.0f;

	return FMath::Clamp(currentCooldown / skillData.cooldown, 0.0f, 1.0f);
}
