// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "C_SkillManager.generated.h"

class UC_SkillBase;
class UC_CooldownManager;
struct FSkillData;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTBBK_API UC_SkillManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UC_SkillManager();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Skill Manager")
	bool RegisterSkill(UC_SkillBase* skill);

	UFUNCTION(BlueprintCallable, Category = "Skill Manager")
	bool UnregisterSkill(FName skillID);

	UFUNCTION(BlueprintCallable, Category = "Skill Manager")
	void ClearAllSkills();

	// ===== 스킬 사용 =====
	UFUNCTION(BlueprintCallable, Category = "Skill Manager")
	bool UseSkillByIndex(int32 index);

	UFUNCTION(BlueprintCallable, Category = "Skill Manager")
	bool UseSkillByID(FName skillID);

	UFUNCTION(BlueprintPure, Category = "Skill Manager")
	UC_SkillBase* GetSkillByIndex(int32 index) const;

	UFUNCTION(BlueprintPure, Category = "Skill Manager")
	UC_SkillBase* GetSkillByID(FName skillID) const;

	UFUNCTION(BlueprintPure, Category = "Skill Manager")
	TArray<UC_SkillBase*> GetAllSkills() const { return skills; }


	UFUNCTION(BlueprintPure, Category = "Skill Manager")
	int32 GetSkillCount() const { return skills.Num(); }

	// ===== CooldownManager 연동 =====
	UFUNCTION(BlueprintCallable, Category = "Skill Manager")
	void SetCooldownManager(UC_CooldownManager* manager);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Skill Manager")
	TArray<UC_SkillBase*> skills;

	UPROPERTY()
	TMap<FName, UC_SkillBase*> skillMap;

	UPROPERTY()
	UC_CooldownManager* cooldownManager;

	UPROPERTY(EditAnywhere, Category = "Skill Manager")
	bool bAutoUpdate = true;


};
