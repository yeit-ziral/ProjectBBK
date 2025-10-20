// Fill out your copyright notice in the Description page of Project Settings.


#include "C_SkillManager.h"
#include "C_SkillBase.h"
#include "C_CooldownManager.h"

// Sets default values for this component's properties
UC_SkillManager::UC_SkillManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	PrimaryComponentTick.TickInterval = 0.05f; // 20Hz

	cooldownManager = nullptr;
	bAutoUpdate = true;
}


// Called when the game starts
void UC_SkillManager::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("SkillManagerComponent initialized (%d skills)"), skills.Num());

}


// Called every frame
void UC_SkillManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 자동 업데이트가 활성화되어 있으면 모든 스킬 업데이트
	if (bAutoUpdate)
	{
		for (UC_SkillBase* skill : skills)
		{
			if (skill)
			{
				skill->UpdateSkill(DeltaTime);
			}
		}
	}
}

bool UC_SkillManager::RegisterSkill(UC_SkillBase* skill)
{
	if (!skill)
	{
		UE_LOG(LogTemp, Error, TEXT("RegisterSkill: Skill is null"));
		return false;
	}

	FName skillID = skill->GetSkillData().skillID;

	// 중복 체크
	if (skillMap.Contains(skillID))
	{
		UE_LOG(LogTemp, Warning, TEXT("RegisterSkill: Skill already registered: %s"), *skillID.ToString());
		return false;
	}

	// CooldownManager 자동 연결
	if (cooldownManager)
	{
		skill->SetCooldownManager(cooldownManager);
	}

	// 등록
	skills.Add(skill);
	skillMap.Add(skillID, skill);

	UE_LOG(LogTemp, Log, TEXT("Skill registered: %s (Total: %d)"), *skillID.ToString(), skills.Num());

	return true;
}

bool UC_SkillManager::UnregisterSkill(FName skillID)
{
	UC_SkillBase** foundSkill = skillMap.Find(skillID);
	if (!foundSkill)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnregisterSkill: Skill not found: %s"), *skillID.ToString());
		return false;
	}

	skills.Remove(*foundSkill);
	skillMap.Remove(skillID);

	UE_LOG(LogTemp, Log, TEXT("Skill unregistered: %s"), *skillID.ToString());
	return true;
}

void UC_SkillManager::ClearAllSkills()
{
	int32 count = skills.Num();

	skills.Empty();
	skillMap.Empty();

	UE_LOG(LogTemp, Log, TEXT("All skills cleared (%d total)"), count);
}

bool UC_SkillManager::UseSkillByIndex(int32 index)
{
	if (!skills.IsValidIndex(index))
	{
		UE_LOG(LogTemp, Warning, TEXT("UseSkillByIndex: Invalid index %d (Total: %d)"), index, skills.Num());
		return false;
	}

	UC_SkillBase* skill = skills[index];
	if (!skill)
	{
		UE_LOG(LogTemp, Error, TEXT("UseSkillByIndex: Skill at index %d is null"), index);
		return false;
	}

	// 스킬 사용
	bool success = skill->CastSkill();

	if (success)
	{
		UE_LOG(LogTemp, Log, TEXT("Skill used by index: %d (%s)"),
			index, *skill->GetSkillData().skillName.ToString());
	}

	return success;
}

bool UC_SkillManager::UseSkillByID(FName skillID)
{
	UC_SkillBase** foundSkill = skillMap.Find(skillID);
	if (!foundSkill || !(*foundSkill))
	{
		UE_LOG(LogTemp, Warning, TEXT("UseSkillByID: Skill not found: %s"),
			*skillID.ToString());
		return false;
	}

	// 스킬 사용
	bool success = (*foundSkill)->CastSkill();

	if (success)
	{
		UE_LOG(LogTemp, Log, TEXT("Skill used by ID: %s"), *skillID.ToString());
	}

	return success;
}

UC_SkillBase* UC_SkillManager::GetSkillByIndex(int32 index) const
{
	if (!skills.IsValidIndex(index))
	{
		return nullptr;
	}

	return skills[index];
}

UC_SkillBase* UC_SkillManager::GetSkillByID(FName skillID) const
{
	UC_SkillBase* const* foundSkill = skillMap.Find(skillID);
	return foundSkill ? *foundSkill : nullptr;
}

void UC_SkillManager::SetCooldownManager(UC_CooldownManager* manager)
{
	cooldownManager = manager;
	
	// 이미 등록된 스킬들에게도 CooldownManager 설정
	for (UC_SkillBase* skill : skills)
	{
		if (skill)
		{
			skill->SetCooldownManager(cooldownManager);
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("CooldownManager set for SkillManager (%d skills updated)"), skills.Num());
}

