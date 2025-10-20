// Fill out your copyright notice in the Description page of Project Settings.


#include "C_CooldownManager.h"

// Sets default values for this component's properties
UC_CooldownManager::UC_CooldownManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
	
}


// Called when the game starts
void UC_CooldownManager::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("CooldownManager initialized"));
	
}


// Called every frame
void UC_CooldownManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TArray<FName> SkillsToRemove;

	for (auto& Pair : activeCooldowns)
	{
		FName SkillID = Pair.Key;
		FCooldownInfo& Info = Pair.Value;

		Info.remainingTime -= DeltaTime;

		if (Info.remainingTime <= 0.0f)
		{
			SkillsToRemove.Add(SkillID);
			UE_LOG(LogTemp, Log, TEXT("Cooldown finished: %s"), *SkillID.ToString());
		}
	}

	for (const FName& SkillID : SkillsToRemove)
	{
		activeCooldowns.Remove(SkillID);
	}

	if (currentGlobalCooldown > 0.0f)
	{
		currentGlobalCooldown -= DeltaTime;
	}

	if (bShowDebugInfo && GEngine)
	{
		int32 Index = 0;
		for (const auto& Pair : activeCooldowns)
		{
			FString DebugText = FString::Printf(
				TEXT("%s: %.1f / %.1f"),
				*Pair.Key.ToString(),
				Pair.Value.remainingTime,
				Pair.Value.totalDuration
			);

			GEngine->AddOnScreenDebugMessage(
				Index++,
				0.0f,
				FColor::Yellow,
				DebugText
			);
		}
	}

}

void UC_CooldownManager::StartCooldown(FName SkillID, float Duration)
{
	if (Duration <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid cooldown duration: %.1f"), Duration);
		return;
	}

	FCooldownInfo NewInfo(SkillID, Duration, Duration);
	activeCooldowns.Add(SkillID, NewInfo);

	currentGlobalCooldown = globalCooldownDuration;

	UE_LOG(LogTemp, Log, TEXT("Cooldown started: %s (%.1f seconds)"),
		*SkillID.ToString(), Duration);
}

bool UC_CooldownManager::IsOnCooldown(FName SkillID) const
{
	if (currentGlobalCooldown > 0.0f)
		return true;

	return activeCooldowns.Contains(SkillID);
}

float UC_CooldownManager::GetRemainingCooldown(FName SkillID) const
{
	const FCooldownInfo* Info = activeCooldowns.Find(SkillID);
	return Info ? Info->remainingTime : 0.0f;
}

float UC_CooldownManager::GetCooldownPercent(FName SkillID) const
{
	const FCooldownInfo* Info = activeCooldowns.Find(SkillID);
	if (!Info || Info->totalDuration <= 0.0f)
		return 0.0f;

	return FMath::Clamp(Info->remainingTime / Info->totalDuration, 0.0f, 1.0f);
}

bool UC_CooldownManager::GetCooldownInfo(FName SkillID, FCooldownInfo& OutInfo) const
{
	const FCooldownInfo* Info = activeCooldowns.Find(SkillID);
	if (Info)
	{
		OutInfo = *Info;
		return true;
	}
	return false;
}

void UC_CooldownManager::ClearCooldown(FName SkillID)
{
	if (activeCooldowns.Remove(SkillID) > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Cooldown cleared: %s"), *SkillID.ToString());
	}
}

void UC_CooldownManager::ClearAllCooldowns()
{
	int32 Count = activeCooldowns.Num();
	activeCooldowns.Empty();

	UE_LOG(LogTemp, Log, TEXT("All cooldowns cleared (%d total)"), Count);
}

void UC_CooldownManager::ReduceCooldown(FName SkillID, float Amount)
{
	FCooldownInfo* Info = activeCooldowns.Find(SkillID);
	if (Info)
	{
		Info->remainingTime = FMath::Max(0.0f, Info->remainingTime - Amount);

		UE_LOG(LogTemp, Log, TEXT("Cooldown reduced: %s (-%f seconds, %.1f remaining)"),
			*SkillID.ToString(), Amount, Info->remainingTime);
	}
}

TArray<FCooldownInfo> UC_CooldownManager::GetAllCooldowns() const
{
	TArray<FCooldownInfo> Result;
	activeCooldowns.GenerateValueArray(Result);
	return Result;
}

