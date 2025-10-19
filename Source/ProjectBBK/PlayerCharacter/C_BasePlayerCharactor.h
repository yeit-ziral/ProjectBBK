// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Logging/LogMacros.h"
#include "C_BasePlayerCharactor.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogBasePlayerCharacter, Log, All);

UCLASS(Blueprintable, config = Game)
class PROJECTBBK_API AC_BasePlayerCharactor : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* springArm;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* camera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* playerMappingContext;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* moveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* lookAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* sprintAction;

public:
	// Sets default values for this character's properties
	AC_BasePlayerCharactor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Movement
	void MyMove(const FInputActionValue& Value);
	void MyLook(const FInputActionValue& Value);

	// Sprint
	void StartSprint();
	void EndSprint();

	//Stamina
	void UpdateStamina();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	float Health = 100.0f;
	const float MAX_HEALTH = 100.0f;


	UPROPERTY(EditAnywhere, Category = "Movement")
	float curStamina = 100.0f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float max_Stamina = 100.0f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float staminaDrainTime;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float staminaRefillTime;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float delayBeforeRefill;

	float curRefillDelayTime;
	bool bHasStamina = true;

	FVector movementInput;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float walkSpeed = 600.0f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float sprintSpeed = 900.0f;

	bool bIsSprinting = false;

	// 스위칭용 (Week 1에는 구조만)
	int32 ActiveCharacterIndex = 0;
};
