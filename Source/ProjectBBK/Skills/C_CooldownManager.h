// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillData.h"
#include "C_CooldownManager.generated.h"

class UC_SkillBase;

USTRUCT(BlueprintType)
struct FCooldownInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName skillID;

	UPROPERTY(BlueprintReadOnly)
	float remainingTime;

	UPROPERTY(BlueprintReadOnly)
	float totalDuration;

	FCooldownInfo()
		: skillID(NAME_None)
		, remainingTime(0.0f)
		, totalDuration(0.0f)
	{
	}

	FCooldownInfo(FName InID, float InRemaining, float InTotal)
		: skillID(InID)
		, remainingTime(InRemaining)
		, totalDuration(InTotal)
	{
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTBBK_API UC_CooldownManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UC_CooldownManager();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//Duration - 쿨타임 시간
	
	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	void StartCooldown(FName SkillID, float Duration);

	
	//return 쿨타임 중이면 true
	UFUNCTION(BlueprintPure, Category = "Cooldown")
	bool IsOnCooldown(FName SkillID) const;

	//return 남은 시간 (쿨타임 없으면 0)
	UFUNCTION(BlueprintPure, Category = "Cooldown")
	float GetRemainingCooldown(FName SkillID) const;

	//쿨타임 비율 반환 (0.0 ~ 1.0)
	//return 쿨타임 비율
	UFUNCTION(BlueprintPure, Category = "Cooldown")
	float GetCooldownPercent(FName SkillID) const;

	//return 쿨타임이 있으면 true
	UFUNCTION(BlueprintPure, Category = "Cooldown")
	bool GetCooldownInfo(FName SkillID, FCooldownInfo& OutInfo) const;

	//쿨타임 강제 종료
	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	void ClearCooldown(FName SkillID);

	//모든 쿨타임 초기화
	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	void ClearAllCooldowns();

	//쿨타임 감소 (버프 효과 등)
	//Amount - 감소량 (초)
	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	void ReduceCooldown(FName SkillID, float Amount);

	//return 쿨타임 정보 배열
	UFUNCTION(BlueprintPure, Category = "Cooldown")
	TArray<FCooldownInfo> GetAllCooldowns() const;

protected:
	// 쿨타임 맵 <SkillID, CooldownInfo>
	UPROPERTY()
	TMap<FName, FCooldownInfo> activeCooldowns;

	// 디버그 모드
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowDebugInfo = false;

};
