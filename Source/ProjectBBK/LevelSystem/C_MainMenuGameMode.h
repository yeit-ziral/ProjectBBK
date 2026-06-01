// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "C_MainMenuGameMode.generated.h"

class UC_MainMenuWidget;

UCLASS()
class PROJECTBBK_API AC_MainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UC_MainMenuWidget> MainMenuWidgetClass;
};
