// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "C_BasePlayerCharactor.generated.h"

UCLASS()
class PROJECTBBK_API AC_BasePlayerCharactor : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AC_BasePlayerCharactor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void MyMoveForward(float Value);
	void MyMoveRight(float Value);

	void Turn(float Value);
	void LookUp(float Value);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;



protected:
	UPROPERTY(EditDefaultsOnly)
	class USpringArmComponent* springArm;

	UPROPERTY(EditDefaultsOnly)
	class UCameraComponent* camera;

	float Health = 100.0f;
	float Stamina = 100.0f;

	const float MAX_HEALTH = 100.0f;
	const float MAX_STAMINA = 100.0f;

	FVector movementInput;

	// 스위칭용 (Week 1에는 구조만)
	int32 ActiveCharacterIndex = 0;
};
