// Fill out your copyright notice in the Description page of Project Settings.


#include "C_TestSkill.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UC_TestSkill::UC_TestSkill()
	:dashDistance(500.0f), dashSpeed(2000.0f)
{
}

void UC_TestSkill::ExecuteSkill_Implementation()
{
	if (!owner)
		return;

	UE_LOG(LogTemp, Warning, TEXT("=== EXECUTING DASH SKILL ==="));

	ACharacter* OwnerCharacter = Cast<ACharacter>(owner);
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("Owner is not a character!"));
		return;
	}

	FVector ForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector DashDestination = OwnerCharacter->GetActorLocation() + (ForwardVector * dashDistance);

	UE_LOG(LogTemp, Warning, TEXT("Dashing from %s to %s"), *OwnerCharacter->GetActorLocation().ToString(), *DashDestination.ToString());
	
	// Method 1: Teleport (instant)
	OwnerCharacter->SetActorLocation(DashDestination, true);

	// Method 2: Launch (smooth)
	// FVector launchVelocity = forward * dashSpeed;
	// ownerChar->LaunchCharacter(launchVelocity, true, true);

	//SpawnSkillEffect(1, DashDestination);
	//PlaySkillSound(1);


	UE_LOG(LogTemp, Warning, TEXT("=== DASH SKILL COMPLETED ==="));
}
