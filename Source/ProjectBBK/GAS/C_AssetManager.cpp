// Fill out your copyright notice in the Description page of Project Settings.


#include "C_AssetManager.h"
#include "AbilitySystemGlobals.h"

void UC_AssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	
	// Custom asset loading logic can be added here
	UAbilitySystemGlobals::Get().InitGlobalData();
	UE_LOG(LogTemp, Warning, TEXT("My Asset Manager!!"));
}
