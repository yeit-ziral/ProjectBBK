// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectBBK/PlayerCharacter/C_BasePlayerCharactor.h"
#include "C_MeleeCharacter.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTBBK_API AC_MeleeCharacter : public AC_BasePlayerCharactor
{
	GENERATED_BODY()
	
public:
	// Sets default values for this character's properties
	AC_MeleeCharacter(const class FObjectInitializer& ObjectInitalizer);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
