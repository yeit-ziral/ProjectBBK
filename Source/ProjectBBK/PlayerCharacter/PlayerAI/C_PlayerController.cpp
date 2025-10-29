// Fill out your copyright notice in the Description page of Project Settings.


#include "C_PlayerController.h"
#include "../C_PlayerState.h"
#include "AbilitySystemComponent.h"

void AC_PlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AC_PlayerState* PS = GetPlayerState<AC_PlayerState>();

	if (PS)
	{
		PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, InPawn);
	}
}

// TODO -- Add HUD stuff